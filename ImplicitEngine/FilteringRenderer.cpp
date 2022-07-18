#include "FilteringRenderer.h"

FilteringRenderer::FilteringRenderer(CallbackFun refreshFun, int seedNum_, int filterMeshRes_, int finalMeshRes_)
	: Renderer(refreshFun), seedNum(seedNum_), filterMeshRes(filterMeshRes_), finalMeshRes(finalMeshRes_),
	pool(std::thread::hardware_concurrency() - 1), seeds(pool.get_thread_count()), filterMesh(Pow4(filterMeshRes), false) {}

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

	// ===== Seed Generation =====
	for (auto& vec : seeds)
		vec.clear();

	int seedsPerThread = seedNum / pool.get_thread_count();
	std::vector<std::future<void>> futs;
	for (int ti = 0; ti < pool.get_thread_count(); ti++)
		futs.push_back(pool.submit(ProximalBracketingGenerator::Generate, &seeds[ti], job->funcs[ti], job->bounds, 16, filterMeshRes, seedsPerThread));

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
	TIMER(mesh);
	filterMesh.resize(Pow4(filterMeshRes));
	memset(filterMesh.data(), false, filterMesh.size());

	const Bounds bounds = job->bounds;
	size_t filterMeshDim = Pow2(filterMeshRes);

	// Enable mesh boxes containing or neigbouring seeds
	for (const auto& seedVec : seeds)
	{
		for (const Seed& s : seedVec)
		{
			if (!bounds.In(s.x, s.y)) continue;

			int boxXI = (s.x - bounds.xmin) / bounds.w() * filterMeshDim;
			int boxYI = (s.y - bounds.ymin) / bounds.h() * filterMeshDim;

			filterMesh[boxXI + boxYI * filterMeshDim] = true;
			boxXI++;
			if (boxXI < filterMeshDim) filterMesh[boxXI + boxYI * filterMeshDim] = true;
			boxXI -= 2;
			if (boxXI >= 0) filterMesh[boxXI + boxYI * filterMeshDim] = true;
			boxXI++; boxYI++;
			if (boxYI < filterMeshDim) filterMesh[boxXI + boxYI * filterMeshDim] = true;
			boxYI -= 2;
			if (boxYI >= 0) filterMesh[boxXI + boxYI * filterMeshDim] = true;
		}
	}

	STOP_LOG(mesh);

	if (keepMesh)
	{
		if (jobMeshes.contains(job->id))
		{
			Mesh& jobMesh = *jobMeshes[job->id];

			jobMesh.boxes = filterMesh;
			jobMesh.bounds = bounds;
			jobMesh.dim = Pow2(filterMeshRes);
		}
		else
			jobMeshes[job->id] = std::make_shared<Mesh>(filterMesh, bounds, Pow2(filterMeshRes));
	}

	// ===== Data Output =====
	job->verts.clear();
}