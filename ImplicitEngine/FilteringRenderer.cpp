#include "FilteringRenderer.h"

FilteringRenderer::FilteringRenderer(CallbackFun refreshFun, int seedNum_, int filterMeshRes_, int finalMeshRes_)
	: Renderer(refreshFun), seedNum(seedNum_), filterMeshRes(filterMeshRes_), finalMeshRes(finalMeshRes_),
	pool(std::thread::hardware_concurrency() - 2), seeds(pool.get_thread_count()) {}

FilteringRenderer::~FilteringRenderer()
{
	pool.wait_for_tasks();
}

void FilteringRenderer::ProcessJob(Job* job)
{
	job->funcs.Resize(pool.get_thread_count());

	int seedsPerThread = seedNum / pool.get_thread_count();

	for (auto& vec : seeds)
		vec.clear();

	std::vector<std::future<void>> futs;
	for (int ti = 0; ti < pool.get_thread_count(); ti++)
	{
		std::vector<Seed>* outPtr = &seeds[ti];
		futs.push_back(pool.submit([=]()
			{
				ProximalBracketingGenerator::Generate(outPtr, job->funcs[ti], job->bounds, 16, seedsPerThread);
			}));
	}

	for (auto& fut : futs)
		fut.wait();

	job->verts.clear();
	for (const auto& seedVec : seeds)
	{
		for (const auto& seed : seedVec)
		{
			job->verts.push_back(seed.x);
			job->verts.push_back(seed.y);
		}
	}
}