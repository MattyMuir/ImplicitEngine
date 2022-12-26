#include "FilteringRenderer.h"

FilteringRenderer::FilteringRenderer(CallbackFun refreshFun, int seedNum_, int filterMeshRes_, int finalMeshRes_)
	: Renderer(refreshFun), seedNum(seedNum_), filterMeshRes(filterMeshRes_), finalMeshRes(finalMeshRes_),
	pool(std::thread::hardware_concurrency() - 1), seeds(pool.get_thread_count()), mesh(filterMeshRes) {}

FilteringRenderer::~FilteringRenderer()
{
	pool.wait_for_tasks();
}

int FilteringRenderer::GetSeedNum()
{
	return seedNum;
}

int FilteringRenderer::GetFilterMeshRes()
{
	return filterMeshRes;
}

int FilteringRenderer::GetFinalMeshRes()
{
	return finalMeshRes;
}

void FilteringRenderer::SetSeedNum(int value)
{
	seedNum = value;
	UpdateJobs();
}

void FilteringRenderer::SetFilterMeshRes(int value)
{
	filterMeshRes = value;
	UpdateJobs();
}

void FilteringRenderer::SetFinalMeshRes(int value)
{
	finalMeshRes = value;
	UpdateJobs();
}

void FilteringRenderer::KeepSeeds(bool keep)
{
	keepSeeds = keep;
	if (!keepSeeds) jobSeeds.clear();
	UpdateJobs();
}

void FilteringRenderer::KeepMesh(bool keep)
{
	keepMesh = keep;
	if (!keepMesh) jobMeshes.clear();
	UpdateJobs();
}

std::optional<std::shared_ptr<Seeds>> FilteringRenderer::GetSeeds(size_t id)
{
	if (jobSeeds.contains(id))
		return jobSeeds[id];
	else
		return {};
}

std::optional<std::shared_ptr<Mesh>> FilteringRenderer::GetMesh(size_t id)
{
	if (jobMeshes.contains(id))
		return jobMeshes[id];
	else
		return {};
}

void FilteringRenderer::ProcessJob(Job* job)
{
	job->funcs.Resize(pool.get_thread_count());

	Bounds bounds = job->bounds;

	TIMER(frame);

	// ===== Seed Generation =====
	for (auto& vec : seeds)
		vec.clear();

	int threadNum = pool.get_thread_count();
	int seedsPerThread = seedNum / threadNum;
	std::vector<std::future<void>> futs;
	for (int ti = 0; ti < threadNum; ti++)
		futs.push_back(pool.submit(ProximalBracketingGenerator::Generate, &seeds[ti], job->funcs[ti], bounds, 16, filterMeshRes, seedsPerThread));

	for (auto& fut : futs)
		fut.wait();

	if (keepSeeds)
	{
		if (jobSeeds.contains(job->id))
			*jobSeeds[job->id] = seeds;
		else
			jobSeeds[job->id] = std::make_shared<Seeds>(seeds);
	}

	// ===== Mesh Generation =====
	mesh.boxes.resize(Pow4(filterMeshRes));
	memset(mesh.boxes.data(), false, mesh.boxes.size());

	mesh.bounds = bounds;
	mesh.dim = (int)Pow2(filterMeshRes);

	// Enable mesh boxes containing or neigbouring seeds
	for (const auto& seedVec : seeds)
	{
		for (const Seed& s : seedVec)
			InsertSeed(s);
	}

	if (keepMesh)
	{
		if (jobMeshes.contains(job->id))
			*jobMeshes[job->id] = mesh;
		else
			jobMeshes[job->id] = std::make_shared<Mesh>(mesh);
	}

	// ===== Contouring =====
	job->verts.clear();
	ContourMesh(job->verts, job->funcs);

	STOP_LOG(frame);
}

void FilteringRenderer::InsertSeed(const Seed& s)
{
	int64_t boxXI = (int64_t)floor((s.x - mesh.bounds.xmin) / mesh.bounds.w() * mesh.dim);
	int64_t boxYI = (int64_t)floor((s.y - mesh.bounds.ymin) / mesh.bounds.h() * mesh.dim);

	bool xFullyIn = (boxXI > 0) && (boxXI < mesh.dim - 1);
	bool yFullyIn = (boxYI > 0) && (boxYI < mesh.dim - 1);

	if (xFullyIn && yFullyIn) [[likely]]
	{
		int64_t index = boxYI * mesh.dim + boxXI;
		mesh.boxes[index] = true;
		mesh.boxes[index + 1] = true;
		mesh.boxes[index - 1] = true;
		mesh.boxes[index + mesh.dim] = true;
		mesh.boxes[index - mesh.dim] = true;
	}
	else [[unlikely]]
	{
		bool fullyOut = (boxXI < -1) || (boxXI > mesh.dim)
			|| (boxYI < -1) || (boxYI > mesh.dim);
		
		if (fullyOut) return;

		// All the edge cases
		// Self, U, D, R, L
		bool adj[5] = { true, true, true, true, true };
		if (boxXI == 0)
		{
			adj[4] = false;
		}
		else if (boxXI == mesh.dim - 1)
		{
			adj[3] = false;
		}
		else if (boxXI == -1)
		{
			adj[0] = false;
			adj[1] = false;
			adj[2] = false;
			adj[4] = false;
		}
		else if (boxXI == mesh.dim)
		{
			adj[0] = false;
			adj[1] = false;
			adj[2] = false;
			adj[3] = false;
		}

		if (boxYI == 0)
		{
			adj[2] = false;
		}
		else if (boxYI == mesh.dim - 1)
		{
			adj[1] = false;
		}
		else if (boxYI == -1)
		{
			adj[2] = false;
			adj[3] = false;
			adj[4] = false;
			adj[0] = false;
		}
		else if (boxYI == mesh.dim)
		{
			adj[0] = false;
			adj[1] = false;
			adj[3] = false;
			adj[4] = false;
		}

		int64_t index = boxYI * mesh.dim + boxXI;
		if (adj[0]) mesh.boxes[index] = true;
		if (adj[3]) mesh.boxes[index + 1] = true;
		if (adj[4]) mesh.boxes[index - 1] = true;
		if (adj[1]) mesh.boxes[index + mesh.dim] = true;
		if (adj[2]) mesh.boxes[index - mesh.dim] = true;
	}
}

void FilteringRenderer::ContourMesh(std::vector<double>& lineVerts, FunctionPack& funcs) 
{
	// Compute a few useful values
	uint64_t finalDim = (uint64_t)1 << finalMeshRes;
	uint64_t bufSize = finalDim + 1;

	// Calculate which regions of the image to dedicate to each thread
	int threadNum = pool.get_thread_count();
	funcs.Resize(threadNum + 1);

	std::vector<uint64_t> startRows(threadNum);
	std::vector<uint64_t> endRows(threadNum);

	for (int ti = 0; ti < threadNum; ti++)
		startRows[ti] = finalDim * ti / threadNum;
	for (int ti = 0; ti < threadNum - 1; ti++)
		endRows[ti] = startRows[ti + 1] - 1;
	endRows[threadNum - 1] = finalDim - 1;

	// First compute the values on the boundaries of the thread areas
	std::vector<ValueBuffer> boundaries;
	boundaries.reserve(threadNum + 1);
	for (int i = 0; i <= threadNum; i++)
		boundaries.emplace_back(bufSize);

	std::vector<std::future<void>> futs;
	for (int ti = 0; ti <= threadNum; ti++)
	{
		uint64_t gy = (ti < threadNum) ? startRows[ti] : finalDim;
		ValueBuffer* outPtr = &boundaries[ti];
		Function* funcPtr = funcs[ti];
		futs.push_back(pool.submit([=, this]() { this->FillBuffer(outPtr, funcPtr, gy); }));
	}
	for (auto& future : futs) future.wait();

	// Initialize threads to fully contour one section of the image each
	std::vector<std::vector<double>> threadOutputs(threadNum);
	for (int ti = 0; ti < threadNum; ti++)
	{
		std::vector<double>* outPtr = &threadOutputs[ti];
		Function* funcPtr = funcs[ti];
		ValueBuffer* bottom = &boundaries[ti];
		ValueBuffer* top = &boundaries[ti + 1];
		futs[ti] = pool.submit([=, this]() { this->ContourRows(outPtr, funcPtr, startRows[ti], endRows[ti], bottom, top); });
	}

	for (auto& future : futs) future.wait();

	// Collect outputs into a single vector
	uint64_t finalValueNum = 0;
	for (const auto& vec : threadOutputs) finalValueNum += vec.size();

	lineVerts.reserve(finalValueNum);
	for (int ti = 0; ti < threadNum; ti++)
	{
		for (double v : threadOutputs[ti])
			lineVerts.push_back(v);
	}
}

void FilteringRenderer::ContourRows(std::vector<double>* lineVerts, Function* funcPtr, uint64_t startRow, uint64_t endRow, const ValueBuffer* bottom, const ValueBuffer* top) const
{
	uint64_t finalDim = (uint64_t)1 << finalMeshRes;
	uint64_t bufSize = finalDim + 1;

	ValueBuffer downBuf(bufSize);
	ValueBuffer upBuf(bufSize);

	const Bounds& bounds = mesh.bounds;

	// Fill downBuf with values from param
	downBuf = *bottom;

	for (uint64_t gy = startRow + 1; gy <= endRow + 1; gy++)
	{
		// Fill upBuf with values
		if (gy < endRow + 1)
			FillBuffer(&upBuf, funcPtr, gy);
		else
			upBuf = *top;

		// Compare buffers to identify squares with lines
		for (uint64_t gx = 0; gx < finalDim; gx++)
		{
			// Check if whole square is valid
			if (downBuf.active[gx] && downBuf.active[gx + 1] && upBuf.active[gx] && upBuf.active[gx + 1]) {}
			else { continue; }

			double lx = (double)gx / finalDim * bounds.w()  + bounds.xmin;
			double rx = (double)(gx + 1) / finalDim * bounds.w() + bounds.xmin;
			double ty = (double)gy / finalDim * bounds.h() + bounds.ymin;
			double by = (double)(gy - 1) / finalDim * bounds.h() + bounds.ymin;

			double xs[4] = { lx, rx, rx, lx };
			double ys[4] = { ty, ty, by, by };
			double vals[4] = { upBuf[gx], upBuf[gx + 1], downBuf[gx + 1], downBuf[gx] };
			Lines lines = GetTileLines(xs, ys, vals);

			for (int n = 0; n < lines.n; n++)
			{
				lineVerts->push_back(lines.xs[n * 2]);
				lineVerts->push_back(lines.ys[n * 2]);
				lineVerts->push_back(lines.xs[n * 2 + 1]);
				lineVerts->push_back(lines.ys[n * 2 + 1]);
			}
		}

		// Swap buffers
		std::swap(downBuf.vals, upBuf.vals);
		std::swap(downBuf.active, upBuf.active);
	}
}

void FilteringRenderer::FillBuffer(ValueBuffer* bufPtr, Function* funcPtr, uint64_t y) const
{
	// References for readability
	ValueBuffer& buf = *bufPtr;
	Function& func = *funcPtr;
	const Bounds& bounds = mesh.bounds;

	buf.SetAllActive(false);

	// Calculate useful values
	uint64_t finalDim = (uint64_t)1 << finalMeshRes;
	int sqsPerTile = (int)(finalDim / mesh.dim);
	double worldY = (double)y / finalDim * bounds.h() + bounds.ymin;
	double delX = bounds.w() / finalDim;

	if (y == 0 || y == finalDim || y % sqsPerTile != 0) // top, bottom, or non-boundary row
	{
		bool currentTile, lastTile = false;

		uint64_t filterRowOffset = ((y == finalDim) ? (mesh.dim - 1) : y / sqsPerTile) * mesh.dim; // Offset for accessing filter-mesh arr
		uint64_t tileStartIndex = 0;
		for (int majorX = 0; majorX < mesh.dim; majorX++, tileStartIndex += sqsPerTile, lastTile = currentTile)
		{
			currentTile = mesh.boxes[filterRowOffset + majorX];
			double worldX = tileStartIndex * delX + bounds.xmin;
			if (lastTile || currentTile) buf[tileStartIndex] = func(worldX, worldY); // leftmost value
			if (!currentTile) continue; //Skip this tile

			// Fill all 'fully-internal' tile values
			for (int minorX = 1; minorX < sqsPerTile; minorX++)
			{
				worldX += delX;
				uint64_t bufIndex = tileStartIndex + minorX;
				buf[bufIndex] = func(worldX, worldY);
			}
			if (mesh.boxes[filterRowOffset + mesh.dim - 1]) buf[finalDim] = func(bounds.xmax, worldY);
		}
	}
	else // Boundary row
	{
		bool currentTile, lastTile = false;

		int64_t filterRowOffset = (y - 1) / sqsPerTile * mesh.dim;
		uint64_t tileStartIndex = 0;
		for (int majorX = 0; majorX < mesh.dim; majorX++, tileStartIndex += sqsPerTile, lastTile = currentTile)
		{
			// Boundary row, tile above and below need to be checked
			currentTile = mesh.boxes[filterRowOffset + majorX] || mesh.boxes[filterRowOffset + mesh.dim + majorX];
			double worldX = tileStartIndex * delX + bounds.xmin;
			if (lastTile || currentTile) buf[tileStartIndex] = func(worldX, worldY); // leftmost value
			if (!currentTile) continue; //Skip this tile

			for (int minorX = 1; minorX < sqsPerTile; minorX++)
			{
				worldX += delX;
				uint64_t bufIndex = tileStartIndex + minorX;
				buf[bufIndex] = func(worldX, worldY);
			}
		}
		if (mesh.boxes[filterRowOffset + mesh.dim - 1]) buf[finalDim] = func(bounds.xmax, worldY);
	}
}

Lines FilteringRenderer::GetTileLines(double* xs, double* ys, double* vals) const
{
	// LUTs
	static constexpr int indicies[16][4] = { {0, 0, 0, 0},{0, 3, 0, 0},
		{0, 1, 0, 0}, {1, 3, 0, 0},
		{1, 2, 0, 0}, {0, 1, 2, 3},
		{0, 2, 0, 0}, {2, 3, 0, 0},
		{2, 3, 0, 0}, {0, 2, 0, 0},
		{0, 3, 1, 2}, {1, 2, 0, 0},
		{1, 3, 0, 0}, {0, 1, 0, 0},
		{0, 3, 0, 0}, {0, 0, 0, 0} };
	static constexpr int lNum[16] = { 0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 0 };

	Lines lines;

	// Determine caseIndex based on signs of verts
	int caseIndex = 0;
	caseIndex |= (vals[0] < 0);
	caseIndex |= (vals[1] < 0) << 1;
	caseIndex |= (vals[2] < 0) << 2;
	caseIndex |= (vals[3] < 0) << 3;

	lines.n = lNum[caseIndex];
	if (lines.n == 0) { return lines; }

	for (int li = 0; li < lines.n * 2; li++)
	{
		int i = indicies[caseIndex][li];
		if (i % 2)
		{
			lines.xs[li] = xs[i];
			double y1 = ys[i];
			double y2 = ys[(i + 1) % 4];

			double v1 = vals[i];
			double v2 = vals[(i + 1) % 4];

			lines.ys[li] = (y1 * v2 - v1 * y2) / (v2 - v1);
		}
		else
		{
			lines.ys[li] = ys[i];
			double x1 = xs[i];
			double x2 = xs[i + 1];

			double v1 = vals[i];
			double v2 = vals[i + 1];

			lines.xs[li] = (x1 * v2 - v1 * x2) / (v2 - v1);
		}
	}
	return lines;
}