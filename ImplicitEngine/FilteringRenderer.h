#pragma once
#include "Renderer.h"

#include <BS_thread_pool.hpp>

class FilteringRenderer : public Renderer
{
public:
	FilteringRenderer(CallbackFun refreshFun, int seedNum_ = 2048, int filterMeshRes_ = 5, int finalMeshRes_ = 9);

protected:
	void ProcessJob(Job* job);

	BS::thread_pool pool;

	int seedNum;
	int filterMeshRes;
	int finalMeshRes;
};