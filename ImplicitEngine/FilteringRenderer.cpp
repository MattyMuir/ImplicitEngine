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
	TIMER(meshConstruct);
	mesh.boxes.resize(Pow4(filterMeshRes));
	memset(mesh.boxes.data(), false, mesh.boxes.size());

	mesh.bounds = job->bounds;
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