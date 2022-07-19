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

void FilteringRenderer::UpdateJobs()
{
	for (auto job : jobs)
		job->status = JobStatus::OUTDATED;
}

void FilteringRenderer::ProcessJob(Job* job)
{
	TIMER(generation);
	job->funcs.Resize(pool.get_thread_count());

	Bounds bounds = job->bounds;

	// ===== Seed Generation =====
	for (auto& vec : seeds)
		vec.clear();

	int seedsPerThread = seedNum / pool.get_thread_count();
	std::vector<std::future<void>> futs;
	for (int ti = 0; ti < pool.get_thread_count(); ti++)
		futs.push_back(pool.submit(ProximalBracketingGenerator::Generate, &seeds[ti], job->funcs[ti], bounds, 16, filterMeshRes, seedsPerThread));

	for (auto& fut : futs)
		fut.wait();

	STOP_LOG(generation);

	if (keepSeeds)
	{
		if (jobSeeds.contains(job->id))
			*jobSeeds[job->id] = seeds;
		else
			jobSeeds[job->id] = std::make_shared<Seeds>(seeds);
	}

	// ===== Mesh Generation =====
	TIMER(meshConstruct);
	mesh.boxes.resize(Pow4(filterMeshRes));
	memset(mesh.boxes.data(), false, mesh.boxes.size());

	mesh.bounds = bounds;
	mesh.dim = Pow2(filterMeshRes);

	// Enable mesh boxes containing or neigbouring seeds
	for (const auto& seedVec : seeds)
	{
		for (const Seed& s : seedVec)
			InsertSeed(s);
	}

	STOP_LOG(meshConstruct);

	if (keepMesh)
	{
		if (jobMeshes.contains(job->id))
		{
			*jobMeshes[job->id] = mesh;
		}
		else
			jobMeshes[job->id] = std::make_shared<Mesh>(mesh);
	}

	// ===== Data Output =====
	job->verts.clear();

	ContourMesh(job->verts, job->funcs);
}

void FilteringRenderer::InsertSeed(const Seed& s)
{
	int64_t boxXI = floor((s.x - mesh.bounds.xmin) / mesh.bounds.w() * mesh.dim);
	int64_t boxYI = floor((s.y - mesh.bounds.ymin) / mesh.bounds.h() * mesh.dim);

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
	int finalDim = 1 << finalMeshRes;
	int bufSize = finalDim + 1;

	// Calculate which regions of the image to dedicate to each thread
	int threadNum = pool.get_thread_count();
	funcs.Resize(threadNum + 2);

	std::vector<int> startRows(threadNum);
	std::vector<int> endRows(threadNum);

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
		int gy = (ti < threadNum) ? startRows[ti] : endRows[threadNum - 1] + 1;
		ValueBuffer* outPtr = &boundaries[ti];
		Function* funcPtr = funcs[ti];
		futs.push_back(pool.submit([=]() { this->FillBuffer(outPtr, funcPtr, gy); }));
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
		futs[ti] = pool.submit([=]() { this->ContourRows(outPtr, funcPtr, startRows[ti], endRows[ti], bottom, top); });
	}

	for (auto& future : futs) future.wait();

	// Collect outputs into a single vector
	int finalValueNum = 0;
	for (const auto& vec : threadOutputs) finalValueNum += vec.size();

	lineVerts.reserve(finalValueNum);
	for (int ti = 0; ti < threadNum; ti++)
	{
		for (float f : threadOutputs[ti])
			lineVerts.push_back(f);
	}
}

void FilteringRenderer::ContourRows(std::vector<double>* lineVerts, Function* funcPtr, int startRow, int endRow, const ValueBuffer* bottom, const ValueBuffer* top) const
{
	int finalDim = 1 << finalMeshRes;
	int bufSize = finalDim + 1;

	ValueBuffer downBuf(bufSize);
	ValueBuffer upBuf(bufSize);

	const Bounds& bounds = mesh.bounds;

	// Fill downBuf with values from param
	downBuf = *bottom;

	for (int gy = startRow + 1; gy <= endRow + 1; gy++)
	{
		// Fill upBuf with values
		if (gy < endRow + 1)
			FillBuffer(&upBuf, funcPtr, gy);
		else
			upBuf = *top;

		// Compare buffers to identify squares with lines
		for (int gx = 0; gx < finalDim; gx++)
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

void FilteringRenderer::FillBuffer(ValueBuffer* bufPtr, Function* funcPtr, int y) const
{
	ValueBuffer& buf = *bufPtr;
	Function& func = *funcPtr;
	const Bounds& bounds = mesh.bounds;

	buf.SetActive(false);
	int64_t finalDim = (int64_t)1 << finalMeshRes;
	int sqsPerTile = (int)(finalDim / mesh.dim);
	double worldY = (double)y / finalDim * bounds.h() + bounds.ymin;

	double delX = bounds.w() / finalDim;

	if (y == 0)  // top row
	{
		bool lastTile = false;
		bool currentTile;

		int majorIndex = 0;
		for (int majorX = 0; majorX < mesh.dim; majorX++)
		{
			currentTile = mesh.boxes[majorX];
			double worldX = majorIndex * delX + bounds.xmin;
			if (lastTile || currentTile) buf[majorIndex] = func(worldX, worldY);

			if (currentTile)
			{
				for (int minorX = 1; minorX < sqsPerTile; minorX++)
				{
					worldX += delX;
					int bufIndex = majorIndex + minorX;
					buf[bufIndex] = func(worldX, worldY);
				}
			}
			majorIndex += sqsPerTile;
			lastTile = currentTile;
		}
		if (mesh.boxes[mesh.dim - 1]) buf[finalDim] = func(bounds.xmax, worldY);
	}
	else if (y == finalDim) // bottom row
	{
		bool lastTile = false;
		bool currentTile;

		int majorIndex = 0;
		for (int majorX = 0; majorX < mesh.dim; majorX++)
		{
			currentTile = mesh.boxes[(mesh.dim - 1) * mesh.dim + majorX];
			double worldX = majorIndex * delX + bounds.xmin;
			if (lastTile || currentTile) buf[majorIndex] = func(worldX, worldY);

			if (currentTile)
			{
				for (int minorX = 1; minorX < sqsPerTile; minorX++)
				{
					worldX += delX;
					int bufIndex = majorIndex + minorX;
					buf[bufIndex] = func(worldX, worldY);
				}
			}
			majorIndex += sqsPerTile;
			lastTile = currentTile;
		}
		if (mesh.boxes[mesh.dim * mesh.dim - 1]) buf[finalDim] = func(bounds.xmax, worldY);
	}
	else // Middle row
	{
		if (y % sqsPerTile == 0)
		{
			// Boundary row
			bool lastTile = false;
			bool currentTile;

			int rowIndexOffset = (y - 1) / sqsPerTile * mesh.dim;

			int majorIndex = 0;
			for (int majorX = 0; majorX < mesh.dim; majorX++)
			{
				currentTile = mesh.boxes[rowIndexOffset + majorX] || mesh.boxes[rowIndexOffset + mesh.dim + majorX];
				double worldX = majorIndex * delX + bounds.xmin;
				if (lastTile || currentTile) buf[majorIndex] = func(worldX, worldY);

				if (currentTile)
				{
					for (int minorX = 1; minorX < sqsPerTile; minorX++)
					{
						worldX += delX;
						int bufIndex = majorIndex + minorX;
						buf[bufIndex] = func(worldX, worldY);
					}
				}
				majorIndex += sqsPerTile;
				lastTile = currentTile;
			}
			if (mesh.boxes[rowIndexOffset + mesh.dim - 1]) buf[finalDim] = func(bounds.xmax, worldY);
		}
		else
		{
			// Non boundary row
			bool lastTile = false;
			bool currentTile;

			int rowIndexOffset = y / sqsPerTile * mesh.dim;

			int majorIndex = 0;
			for (int majorX = 0; majorX < mesh.dim; majorX++)
			{
				currentTile = mesh.boxes[rowIndexOffset + majorX];
				double worldX = majorIndex * delX + bounds.xmin;
				if (lastTile || currentTile) buf[majorIndex] = func(worldX, worldY);

				if (currentTile)
				{
					for (int minorX = 1; minorX < sqsPerTile; minorX++)
					{
						worldX += delX;
						int bufIndex = majorIndex + minorX;
						buf[bufIndex] = func(worldX, worldY);
					}
				}
				majorIndex += sqsPerTile;
				lastTile = currentTile;
			}
			if (mesh.boxes[rowIndexOffset + mesh.dim - 1]) buf[finalDim] = func(bounds.xmax, worldY);
		}
	}
}

Lines FilteringRenderer::GetTileLines(double* xs, double* ys, double* vals) const
{
	// LUT
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

	int caseIndex = 0;
	caseIndex |= (vals[0] < 0);
	caseIndex |= (vals[1] < 0) << 1;
	caseIndex |= (vals[2] < 0) << 2;
	caseIndex |= (vals[3] < 0) << 3;

	lines.n = lNum[caseIndex];
	if (lines.n == 0) { return lines; }

	double xInt[4];
	double yInt[4];

	for (int li = 0; li < lines.n * 2; li++)
	{
		int i = indicies[caseIndex][li];
		if (i % 2)
		{
			xInt[li] = xs[i];
			double y1 = ys[i];
			double y2 = ys[(i + 1) % 4];

			double v1 = vals[i];
			double v2 = vals[(i + 1) % 4];

			yInt[li] = (y1 * v2 - v1 * y2) / (v2 - v1);
		}
		else
		{
			yInt[li] = ys[i];
			double x1 = xs[i];
			double x2 = xs[i + 1];

			double v1 = vals[i];
			double v2 = vals[i + 1];

			xInt[li] = (x1 * v2 - v1 * x2) / (v2 - v1);
		}
	}

	for (int i = 0; i < lines.n * 2; i++)
	{
		lines.xs[i] = xInt[i];
		lines.ys[i] = yInt[i];
	}
	return lines;
}