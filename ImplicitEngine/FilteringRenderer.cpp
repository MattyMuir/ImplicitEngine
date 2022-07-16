#include "FilteringRenderer.h"

FilteringRenderer::FilteringRenderer(CallbackFun refreshFun, int seedNum_, int filterMeshRes_, int finalMeshRes_)
	: Renderer(refreshFun), seedNum(seedNum_), filterMeshRes(filterMeshRes_), finalMeshRes(finalMeshRes_),
	pool(std::thread::hardware_concurrency() - 2) {}

void FilteringRenderer::ProcessJob(Job* job)
{
	ProximalBracketingGenerator gen(job->func, job->bounds, 16);

	std::vector<Seed> seeds;
	gen.Generate(seeds, 5000);

	job->verts.clear();
	for (const auto& seed : seeds)
	{
		job->verts.push_back(seed.x);
		job->verts.push_back(seed.y);
	}
}