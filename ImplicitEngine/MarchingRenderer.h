#pragma once
#include "FilteringRenderer.h"

class MarchingRenderer : public Renderer
{
public:
	MarchingRenderer(CallbackFun refreshFun, int finalMeshRes_ = 9);

	void SetFinalMeshRes(int value);
	int GetFinalMeshRes();

	// For parity with filtering renderer
	void KeepMesh(bool) {}
	void KeepSeeds(bool) {}
	int GetSeedNum() { return 0; };
	int GetFilterMeshRes() { return 0; };
	void SetSeedNum(int) {};
	void SetFilterMeshRes(int) {};
	std::optional<std::shared_ptr<Seeds>> GetSeeds(size_t) { return {}; };
	std::optional<std::shared_ptr<Mesh>> GetMesh(size_t) { return {}; };

protected:
	void ProcessJob(Job* job);
	void FillBuffer(std::vector<double>* buf, size_t y, const Bounds* boundsPtr, size_t finalMeshDim, Function* funcPtr);
	void ContourRows(std::vector<double>* verts, size_t startY, size_t endY, const Bounds* boundsPtr, Function* funcPtr, const std::vector<double>* bottom, const std::vector<double>* top);
	Lines GetTileLines(double* xs, double* ys, double* vals) const;

	int finalMeshRes;
	BS::thread_pool pool;
};