#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <functional>

#include <boost/math/tools/toms748_solve.hpp>

#include "Function.h"
#include "Bounds.h"
#include "Seed.h"

class StopCondition
{
public:
	StopCondition(double tol_) { tol = tol_; }

	bool operator()(double at, double bt);

private:
	double tol;
};

class ProximalBracketingGenerator
{
public:
	ProximalBracketingGenerator() {};

	static void Generate(std::vector<Seed>* seeds, Function* funcPtr, Bounds bounds, int maxEval, int seedNum);

protected:
	static bool ITPRefine(Seed& a, Seed b, Function* funcPtr, Bounds bounds, int maxIter);
};