#pragma once
#include <map>
#include <optional>

#include <BS_thread_pool.hpp>

#include "Renderer.h"
#include "ProximalBracketingGenerator.h"
#include "pow4.h"
#include "ValueBuffer.h"
#include "Lines.h"

typedef BS::thread_pool ThreadPool;
typedef std::vector<std::vector<Seed>> Seeds;

struct Mesh
{
	Mesh(int res)
		: dim((int)Pow2(res)), boxes((size_t)dim * dim) {}
	Mesh(const std::vector<uint8_t>& boxes_, const Bounds& bounds_, int dim_)
		: boxes(boxes_), bounds(bounds_), dim(dim_) {}

	int dim;
	std::vector<uint8_t> boxes;
	Bounds bounds = { 0.0, 0.0, 0.0, 0.0 };
};

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

	void KeepSeeds(bool keep);
	void KeepMesh(bool keep);

	std::optional<std::shared_ptr<Seeds>> GetSeeds(size_t id);
	std::optional<std::shared_ptr<Mesh>> GetMesh(size_t id);

protected:
	void UpdateJobs();
	void ProcessJob(Job* job);
	void InsertSeed(const Seed& s);
	void ContourMesh(std::vector<double>& lineVerts, FunctionPack& funcs);
	void ContourRows(std::vector<double>* lineVerts, Function* funcPtr, int startRow, int endRow, const ValueBuffer* top, const ValueBuffer* bottom) const;
	void FillBuffer(ValueBuffer* bufPtr, Function* funcPtr, int y) const;
	Lines GetTileLines(double* xs, double* ys, double* vals) const;

	ThreadPool pool;

	int seedNum;
	int filterMeshRes;
	int finalMeshRes;

	Seeds seeds;
	Mesh mesh;

	bool keepSeeds = false;
	std::map<size_t, std::shared_ptr<Seeds>> jobSeeds;

	bool keepMesh = false;
	std::map<size_t, std::shared_ptr<Mesh>> jobMeshes;
};