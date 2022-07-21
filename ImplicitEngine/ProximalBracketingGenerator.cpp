#include "ProximalBracketingGenerator.h"

void ProximalBracketingGenerator::Generate(std::vector<Seed>* seeds, Function* funcPtr, Bounds bounds, int maxEval, int filterMeshRes, int seedNum)
{
	Function& func = *funcPtr;

	// Initialize random number generation
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<unsigned> distribution(0, UINT32_MAX);
	srand(distribution(mt));

	double w = bounds.w();
	double h = bounds.h();

	// Initialize seed lists
	std::vector<Seed> unBracketedSeeds;
	std::vector<std::pair<Seed, Seed>> bracketedSeeds;
	unBracketedSeeds.reserve(seedNum);

	int posNum = 0, negNum = 0;

	double expansion = 1.1;
	Bounds exBounds = bounds.Expand(expansion);

	// Randomly position seeds, evaluate, and add to vec
	for (int i = 0; i < seedNum; i++)
	{
		Seed s = { exBounds.xmin + w * expansion * (double)rand() / RAND_MAX,
			exBounds.ymin + h * expansion * (double)rand() / RAND_MAX };

		s.fs = func(s.x, s.y);
		if (std::isfinite(s.fs))
		{
			unBracketedSeeds.push_back(s);

			if (s.fs > 0) posNum++;
			else if (s.fs < 0) negNum++;
		}
	}

	// Find oppositely signed points
	int maxNewtIter = maxEval / 3;
	int newtIter = 0;
	while (std::min(posNum, negNum) < 1 && newtIter < maxNewtIter)
	{
		// No sign flips, performing newton
		newtIter++;
		posNum = 0; negNum = 0;

		// Newton iteration
		for (Seed& s : unBracketedSeeds)
		{
			if (!s.active) continue;
			static constexpr double q = 1e-10;
			double fxn = func(s.x + s.x * q, s.y);
			double fyn = func(s.x, s.y + s.y * q);

			double dx = (fxn - s.fs) / (s.x * q);
			double dy = (fyn - s.fs) / (s.y * q);

			s.x -= 1.1 * (s.fs * dx) / (dx * dx + dy * dy);
			s.y -= 1.1 * (s.fs * dy) / (dx * dx + dy * dy);

			s.fs = func(s.x, s.y);
			if (!std::isfinite(s.fs)) { s.active = false; continue; }

			if (s.fs > 0) posNum++;
			else if (s.fs < 0) negNum++;
		}
	}

	// If no sign changes found, generation failed
	if (std::min(posNum, negNum) == 0) return;

	// Proximity bracketing
	// Seperate seeds into pos and neg
	std::vector<int> seeds1, seeds2;
	for (int i = 0; i < unBracketedSeeds.size(); i++)
	{
		if (!unBracketedSeeds[i].active) continue;
		if (unBracketedSeeds[i].fs > 0) seeds1.push_back(i);
		else seeds2.push_back(i);
	}

	// Perform bracketing
	if (seeds2.size() > seeds1.size()) std::swap(seeds1, seeds2);

	bracketedSeeds.reserve(seeds1.size() + seeds2.size());
	for (int i = 0; i < seeds1.size(); i++)
	{
		Seed& s1 = unBracketedSeeds[seeds1[i]];
		Seed* s2 = &s1; // Set to a the location of s1 to avoid uninitialized warning

		double bestDist = DBL_MAX;
		for (int si = 0; si < SMPL_NUM; si++)
		{
			Seed& s = unBracketedSeeds[seeds2[(i + si) % seeds2.size()]];
			double dist = Distance(s1, s);

			if (dist < bestDist)
			{
				bestDist = dist;
				s2 = &s;
			}	
		}

		bracketedSeeds.push_back({ s1, *s2 });
	}

	for (int i = 0; i < seeds2.size(); i++)
	{
		Seed& s2 = unBracketedSeeds[seeds2[i]];
		Seed* s1 = &s2; // Set to a the location of s2 to avoid uninitialized warning

		double bestDist = DBL_MAX;
		for (int si = 0; si < SMPL_NUM; si++)
		{
			Seed& s = unBracketedSeeds[seeds1[(i + SMPL_NUM + si) % seeds1.size()]];
			double dist = Distance(s2, s);

			if (dist < bestDist)
			{
				bestDist = dist;
				s1 = &s;
			}
		}

		bracketedSeeds.push_back({ *s1, s2 });
	}

	double absTol = std::min(w, h) / 1000;

	// Perform bracketed refinement
	for (auto& [s1, s2] : bracketedSeeds)
	{
		uint64_t maxIter = 16;

		double dx = s2.x - s1.x;
		double dy = s2.y - s1.y;

		auto [at, bt] = boost::math::tools::toms748_solve([&](double t) { return func(s1.x + dx * t, s1.y + dy * t); },
			0.0, 1.0, s1.fs, s2.fs, StopCondition(s1.x, s1.y, dx, dy, absTol, filterMeshRes, &bounds), maxIter);

		double t = (at + bt) / 2;
		seeds->push_back({ s1.x + dx * t, s1.y + dy * t });
	}
}

double ProximalBracketingGenerator::Distance(const Seed& s1, const Seed& s2)
{
	double dx = s2.x - s1.x;
	double dy = s2.y - s1.y;
	return sqrt(dx * dx + dy * dy);
}

StopCondition::StopCondition(double x0, double y0, double dx, double dy, double absTol, int filterMeshRes, Bounds* bounds)
{
	double bracketLength = sqrt(dx * dx + dy * dy);
	relTol = absTol / bracketLength;

	double filterMeshDim = (double)Pow2(filterMeshRes);
	boxXScale = dx * filterMeshDim / bounds->w();
	boxXOffset = (x0 - bounds->xmin) * filterMeshDim / bounds->w();

	boxYScale = dy * filterMeshDim / bounds->h();
	boxYOffset = (y0 - bounds->ymin) * filterMeshDim / bounds->h();
}

bool StopCondition::operator()(double at, double bt)
{
	bool withinTol = (abs(at - bt) < relTol);
	if (withinTol) return true;

	int aBoxXI = (int)floor(boxXScale * at + boxXOffset);
	int aBoxYI = (int)floor(boxYScale * at + boxYOffset);
	int bBoxXI = (int)floor(boxXScale * bt + boxXOffset);
	int bBoxYI = (int)floor(boxYScale * bt + boxYOffset);

	bool sameBox = ((aBoxXI == bBoxXI) && (aBoxYI == bBoxYI));

	return sameBox;
}

static int sign(double x)
{
	return (x > 0) - (x < 0);
}