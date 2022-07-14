#pragma once
#include "Renderer.h"

class FilteringRenderer : public Renderer
{
public:
	FilteringRenderer(CallbackFun refreshFun, int seedNum_ = 2048, int filterMeshRes_ = 5, int finalMeshRes_ = 9);

protected:
	void ProcessJob(Job* job);

	int seedNum;
	int filterMeshRes;
	int finalMeshRes;
};