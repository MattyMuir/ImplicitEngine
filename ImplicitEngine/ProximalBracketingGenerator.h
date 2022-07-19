#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <functional>

#include <boost/math/tools/toms748_solve.hpp>

#include "Function.h"
#include "Bounds.h"
#include "Seed.h"
#include "pow4.h"

constexpr int SMPL_NUM = 10;

class StopCondition
{
public:
	StopCondition(double x0, double y0, double dx, double dy, double absTol, int filterMeshRes_, Bounds* bounds);

	bool operator()(double at, double bt);

private:
	double relTol;
	double boxXScale, boxXOffset, boxYScale, boxYOffset;
};

class ProximalBracketingGenerator
{
public:
	ProximalBracketingGenerator() {};

	static void Generate(std::vector<Seed>* seeds, Function* funcPtr, Bounds bounds, int maxEval, int filterMeshRes, int seedNum);

protected:
	static double Distance(const Seed& s1, const Seed& s2);
};