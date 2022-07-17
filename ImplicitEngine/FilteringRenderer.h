#pragma once
#include <BS_thread_pool.hpp>

#include "Renderer.h"
#include "ProximalBracketingGenerator.h"
#include "pow4.h"

typedef BS::thread_pool ThreadPool;

class FilteringRenderer : public Renderer
{
public:
	FilteringRenderer(CallbackFun refreshFun, int seedNum_ = 2048, int filterMeshRes_ = 5, int finalMeshRes_ = 9);

	~FilteringRenderer();

	int GetSeedNum();
	int GetFilterMeshRes();
	int GetFinalMeshRes();

	void SetSeedNum(int value);
	void SetFilterMeshRes(int value);
	void SetFinalMeshRes(int value);

protected:
	void UpdateJobs();
	void ProcessJob(Job* job);

	ThreadPool pool;

	int seedNum;
	int filterMeshRes;
	int finalMeshRes;

	std::vector<std::vector<Seed>> seeds;
	std::vector<uint8_t> filterMesh;
};