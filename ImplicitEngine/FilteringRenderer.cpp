#include "FilteringRenderer.h"

FilteringRenderer::FilteringRenderer(CallbackFun refreshFun, int seedNum_, int filterMeshRes_, int finalMeshRes_)
	: Renderer(refreshFun), seedNum(seedNum_), filterMeshRes(filterMeshRes_), finalMeshRes(finalMeshRes_),
	pool(std::thread::hardware_concurrency() - 2) {}

void FilteringRenderer::ProcessJob(Job* job)
{
	// TODO
	std::this_thread::sleep_for(milliseconds(100));
	int o = (job->jobID - 15000) * 2;
	job->verts = { -0.5 + o, -0.5, 0.0 + o, 0.5, o + 0.5 * rand() / RAND_MAX, -0.5 };
}