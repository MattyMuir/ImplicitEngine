#include "FilteringRenderer.h"

FilteringRenderer::FilteringRenderer(CallbackFun refreshFun, int seedNum_, int filterMeshRes_, int finalMeshRes_)
	: Renderer(refreshFun), seedNum(seedNum_), filterMeshRes(filterMeshRes_), finalMeshRes(finalMeshRes_)
{
	// Initialize thread pool
	// TODO
}

void FilteringRenderer::ProcessJob(Job* job)
{
	// TODO
	std::this_thread::sleep_for(milliseconds(100));
	if (job->jobID == 15000)
		job->verts = { -0.5, -0.5, 0, 0.5, 0.5 * rand() / RAND_MAX, -0.5 };
	else
		job->verts = { -2.5, -0.5, -2, 0.5, 0.5 * rand() / RAND_MAX - 2, -0.5 };
}