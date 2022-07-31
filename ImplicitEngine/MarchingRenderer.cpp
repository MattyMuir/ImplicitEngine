#include "MarchingRenderer.h"

MarchingRenderer::MarchingRenderer(CallbackFun refreshFun, int finalMeshRes_)
	: Renderer(refreshFun), finalMeshRes(finalMeshRes_), pool(std::thread::hardware_concurrency() - 1) {}

void MarchingRenderer::SetFinalMeshRes(int value)
{
	finalMeshRes = value;
}

int MarchingRenderer::GetFinalMeshRes()
{
	return finalMeshRes;
}

void MarchingRenderer::ProcessJob(Job* job)
{
	TIMER(frame);
	DoProcessJobSingle(job);
	STOP_LOG(frame);
}

void MarchingRenderer::DoProcessJobSingle(Job* job)
{
	Bounds bounds = job->bounds;
	Function& func = *(job->funcs[0]);
	job->verts.clear();

	size_t finalMeshDim = Pow2(finalMeshRes);
	double squareW = bounds.w() / finalMeshDim;
	double squareH = bounds.h() / finalMeshDim;
	std::vector<double> downBuf(finalMeshDim + 1), upBuf(finalMeshDim + 1);

	for (size_t y = 0; y <= finalMeshDim; y++)
	{
		double worldY = bounds.ymin + bounds.h() * y / finalMeshDim;
		for (size_t x = 0; x <= finalMeshDim; x++)
		{
			double worldX = bounds.xmin + bounds.w() * x / finalMeshDim;
			upBuf[x] = func(worldX, worldY);

			if (x > 1 && y > 1)
			{
				double lx = worldX - squareW;
				double rx = worldX;
				double ty = worldY;
				double by = worldY - squareH;

				double xs[4] = { lx, rx, rx, lx };
				double ys[4] = { ty, ty, by, by };
				double vals[4] = { upBuf[x - 1], upBuf[x], downBuf[x], downBuf[x - 1] };
				Lines lines = GetTileLines(xs, ys, vals);

				for (int vi = 0; vi < lines.n * 2; vi++)
				{
					job->verts.push_back(lines.xs[vi]);
					job->verts.push_back(lines.ys[vi]);
				}
			}
		}
		std::swap(downBuf, upBuf);
	}
}

void MarchingRenderer::DoProcessJobMulti(Job* job)
{
	Bounds bounds = job->bounds;

	// Calculate which regions of the image to dedicate to each thread
	size_t finalMeshDim = Pow2(finalMeshRes);
	int threadNum = pool.get_thread_count();
	job->funcs.Resize(threadNum + 1);

	std::vector<size_t> startRows(threadNum);
	std::vector<size_t> endRows(threadNum);

	for (int ti = 0; ti < threadNum; ti++)
		startRows[ti] = finalMeshDim * ti / threadNum;
	for (int ti = 0; ti < threadNum - 1; ti++)
		endRows[ti] = startRows[ti + 1];
	endRows.back() = finalMeshDim;

	// First compute the values on the boundaries of the thread areas
	std::vector<std::vector<double>> boundaries(threadNum + 1);

	std::vector<std::future<void>> futs;
	for (int i = 0; i <= threadNum; i++)
	{
		size_t y = (i < threadNum) ? startRows[i] : endRows.back();

		auto outPtr = &boundaries[i];
		auto funcPtr = job->funcs[i];
		futs.push_back(pool.submit([=]() { FillBuffer(outPtr, y, &bounds, finalMeshDim, funcPtr); }));
	}

	for (auto& fut : futs)
		fut.wait();

	// Boundary values have been calculated, dispatch threads on blocks
	std::vector<std::vector<double>> blockVerts(threadNum);

	futs.clear();
	for (int ti = 0; ti < threadNum; ti++)
	{
		auto outPtr = &blockVerts[ti];
		Function* funcPtr = job->funcs[ti];

		auto bottom = &boundaries[ti];
		auto top = &boundaries[ti + 1];
		futs.push_back(pool.submit([=]() { ContourRows(outPtr, startRows[ti], endRows[ti], &bounds, funcPtr, bottom, top); }));
	}

	for (auto& fut : futs)
		fut.wait();

	// Collect verts into one vec
	job->verts.clear();

	for (auto& vec : blockVerts)
	{
		for (double val : vec)
			job->verts.push_back(val);
	}
}

void MarchingRenderer::FillBuffer(std::vector<double>* buf, size_t y, const Bounds* boundsPtr, size_t finalMeshDim, Function* funcPtr)
{
	if (buf->size() < finalMeshDim + 1)
		buf->resize(finalMeshDim + 1);

	Function& func = *funcPtr;
	const Bounds& bounds = *boundsPtr;

	double worldY = bounds.ymin + (double)y / finalMeshDim * bounds.h();
	for (size_t x = 0; x < finalMeshDim; x++)
	{
		double worldX = bounds.xmin + (double)x / finalMeshDim * bounds.w();
		(*buf)[x] = func(worldX, worldY);
	}
}

void MarchingRenderer::ContourRows(std::vector<double>* verts, size_t startY, size_t endY, const Bounds* boundsPtr, Function* funcPtr, const std::vector<double>* bottom, const std::vector<double>* top)
{
	Function& func = *funcPtr;
	const Bounds& bounds = *boundsPtr;

	size_t finalMeshDim = Pow2(finalMeshRes);
	double squareW = bounds.w() / finalMeshDim;
	double squareH = bounds.h() / finalMeshDim;

	std::vector<double> upBuf(finalMeshDim + 1), downBuf = *bottom;

	for (size_t y = startY + 1; y <= endY; y++)
	{
		double worldY = bounds.ymin + (double)y / finalMeshDim * bounds.h();

		for (size_t x = 0; x <= finalMeshDim; x++)
		{
			double worldX = bounds.xmin + (double)x / finalMeshDim * bounds.w();
			if (y < endY)
			{
				// Fill buffer normally
				upBuf[x] = func(worldX, worldY);
			}
			else
			{
				// Fill buffer with values from 'top'
				upBuf[x] = (*top)[x];
			}

			if (x > 0)
			{
				// Compute verticies for this square
				double lx = worldX - squareW;
				double rx = worldX;
				double ty = worldY;
				double by = worldY - squareH;

				double xs[4] = { lx, rx, rx, lx };
				double ys[4] = { ty, ty, by, by };
				double vals[4] = { upBuf[x - 1], upBuf[x], downBuf[x], downBuf[x - 1] };
				Lines lines = GetTileLines(xs, ys, vals);

				for (int vi = 0; vi < lines.n * 2; vi++)
				{
					verts->push_back(lines.xs[vi]);
					verts->push_back(lines.ys[vi]);
				}
			}
		}

		std::swap(upBuf, downBuf);
	}
}

Lines MarchingRenderer::GetTileLines(double* xs, double* ys, double* vals) const
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