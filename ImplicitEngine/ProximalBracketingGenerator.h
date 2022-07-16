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
	ProximalBracketingGenerator(Function& func_, const Bounds& bounds_, int maxEval_);

	void Generate(std::vector<Seed>& seeds, int num);
	bool ITPRefine(Seed& a, Seed b, int maxIter);

private:
	Function* funcPtr;
	Bounds bounds;

	int maxEval;
};