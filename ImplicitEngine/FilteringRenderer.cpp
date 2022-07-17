#include "FilteringRenderer.h"

FilteringRenderer::FilteringRenderer(CallbackFun refreshFun, int seedNum_, int filterMeshRes_, int finalMeshRes_)
	: Renderer(refreshFun), seedNum(seedNum_), filterMeshRes(filterMeshRes_), finalMeshRes(finalMeshRes_),
	pool(std::thread::hardware_concurrency() - 1), seeds(pool.get_thread_count()) {}

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

void FilteringRenderer::UpdateJobs()
{
	for (auto job : jobs)
		job->status = JobStatus::OUTDATED;
}

void FilteringRenderer::ProcessJob(Job* job)
{
	TIMER(processing);
	job->funcs.Resize(pool.get_thread_count());

	// ===== Seed Generation =====
	for (auto& vec : seeds)
		vec.clear();

	int seedsPerThread = seedNum / pool.get_thread_count();
	std::vector<std::future<void>> futs;
	for (int ti = 0; ti < pool.get_thread_count(); ti++)
		futs.push_back(pool.submit(ProximalBracketingGenerator::Generate, &seeds[ti], job->funcs[ti], job->bounds, 16, seedsPerThread));

	for (auto& fut : futs)
		fut.wait();

	// ===== Mesh Generation =====

	STOP_LOG(processing);

	// ===== Data Output =====
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