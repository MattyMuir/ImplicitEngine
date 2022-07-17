#pragma once
#include <map>
#include <optional>

#include <BS_thread_pool.hpp>

#include "Renderer.h"
#include "ProximalBracketingGenerator.h"
#include "pow4.h"

typedef BS::thread_pool ThreadPool;
typedef std::vector<std::vector<Seed>> Seeds;

struct Mesh
{
	Mesh(const std::vector<uint8_t>& boxes_, const Bounds& bounds_, int dim_)
		: boxes(boxes_), bounds(bounds_), dim(dim_) {}

	std::vector<uint8_t> boxes;
	Bounds bounds;
	int dim;
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

	ThreadPool pool;

	int seedNum;
	int filterMeshRes;
	int finalMeshRes;

	Seeds seeds;
	std::vector<uint8_t> filterMesh;

	bool keepSeeds = false;
	std::map<size_t, std::shared_ptr<Seeds>> jobSeeds;

	bool keepMesh = false;
	std::map<size_t, std::shared_ptr<Mesh>> jobMeshes;
};