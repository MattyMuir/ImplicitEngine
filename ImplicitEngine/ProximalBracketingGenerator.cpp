#include "ProximalBracketingGenerator.h"

void ProximalBracketingGenerator::Generate(std::vector<Seed>* seeds, Function* funcPtr, Bounds bounds, int maxEval, int filterMeshRes, int seedNum)
{
	Function& func = *funcPtr;

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<unsigned> dist(0, UINT32_MAX);

	srand(dist(mt));

	double w = bounds.w();
	double h = bounds.h();

	std::vector<Seed> unBracketedSeeds, tempVec;
	std::vector<std::pair<Seed, Seed>> bracketedSeeds;
	unBracketedSeeds.reserve(seedNum);

	int posNum = 0, negNum = 0;

	// Randomly position seeds, evaluate, and add to vec
	for (int i = 0; i < seedNum; i++)
	{
		Seed s = { bounds.xmin + w * (double)rand() / RAND_MAX,
			bounds.ymin + h * (double)rand() / RAND_MAX };

		s.fs = func(s.x, s.y);
		if (std::isfinite(s.fs))
		{
			unBracketedSeeds.push_back(s);

			if (s.fs > 0) posNum++;
			else if (s.fs < 0) negNum++;
		}
	}

	// Find oppositely signed points
	int newtIter = maxEval / 3;
	int i = 0;
	while (std::min(posNum, negNum) < 1 && i < newtIter)
	{
		//std::cout << "No sign flips, performing newton\n";
		i++;
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

	// Remove inactive seeds
	for (Seed& s : unBracketedSeeds)
	{
		if (s.active) tempVec.push_back(s);
	}
	std::swap(unBracketedSeeds, tempVec);

	// Proximity bracketing
	// Seperate seeds into pos and neg
	std::vector<int> seeds1, seeds2;
	for (int i = 0; i < unBracketedSeeds.size(); i++)
	{
		if (unBracketedSeeds[i].fs > 0) seeds1.push_back(i);
		else seeds2.push_back(i);
	}

	// Perform bracketing
	if (seeds2.size() > seeds1.size()) std::swap(seeds1, seeds2);

	for (int i = 0; i < seeds1.size(); i++)
	{
		Seed& s1 = unBracketedSeeds[seeds1[i]];
		Seed& s2 = unBracketedSeeds[seeds2[i % seeds2.size()]];
		bracketedSeeds.push_back({ s1, s2 });
	}

	for (int i = 0; i < seeds2.size(); i++)
	{
		Seed& s2 = unBracketedSeeds[seeds2[i]];
		Seed& s1 = unBracketedSeeds[seeds1[(i + 1) % seeds1.size()]];
		bracketedSeeds.push_back({ s1, s2 });
	}

	double absTol = std::min(bounds.w(), bounds.h()) / 1000;

	// Perform bracketed refinement
	for (auto& [s1, s2] : bracketedSeeds)
	{
		uint64_t maxIter = 16;

		double dx = s2.x - s1.x;
		double dy = s2.y - s1.y;

		double bracketLength = sqrt(dx * dx + dy * dy);

		auto [at, bt] = boost::math::tools::toms748_solve([&](double t) { return func(s1.x + dx * t, s1.y + dy * t); },
			0.0, 1.0, s1.fs, s2.fs, StopCondition(s1.x, s1.y, dx, dy, absTol, filterMeshRes, &bounds), maxIter);
		double t = (at + bt) / 2;
		seeds->push_back({ s1.x + dx * t, s1.y + dy * t });
	}
}

StopCondition::StopCondition(double x0, double y0, double dx, double dy, double absTol, int filterMeshRes, Bounds* bounds)
{
	double bracketLength = sqrt(dx * dx + dy * dy);
	relTol = absTol / bracketLength;

	double filterMeshDim = Pow2(filterMeshRes);
	boxXScale = dx * filterMeshDim / bounds->w();
	boxXOffset = (x0 - bounds->xmin) * filterMeshDim / bounds->w();

	boxYScale = dy * filterMeshDim / bounds->h();
	boxYOffset = (y0 - bounds->ymin) * filterMeshDim / bounds->h();
}

bool StopCondition::operator()(double at, double bt)
{
	bool withinTol = (abs(at - bt) < relTol);

	int aBoxXI = boxXScale * at + boxXOffset;
	int aBoxYI = boxYScale * at + boxYOffset;
	int bBoxXI = boxXScale * bt + boxXOffset;
	int bBoxYI = boxYScale * bt + boxYOffset;

	bool sameBox = ((aBoxXI == bBoxXI) && (aBoxYI == bBoxYI));

	return (withinTol || sameBox);
}

static int sign(double x)
{
	return (x > 0) - (x < 0);
}

bool ProximalBracketingGenerator::ITPRefine(Seed& a, Seed b, Function* funcPtr, Bounds bounds, int maxIter)
{
	Function& func = *funcPtr;
	if (!bounds.In(a.x, a.y) && !bounds.In(b.x, b.y))
		return false;

	double at = 0, bt = 1;

	if (a.fs * b.fs > 0) return false;
	if (a.fs > b.fs) std::swap(a, b);

	double absEp = std::min(bounds.w(), bounds.h()) / 100;
	double ep = absEp / sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
	static constexpr double k1 = 0.1, k2 = 2, n0 = 1;

	int n12 = std::ceil(log2(0.5 / ep));
	int nmax = n12 + std::ceil(n0);
	int j = 0;

	double xitp;
	while (std::abs(bt - at) > ep && j < maxIter)
	{
		// Calculating parameters
		double x12 = (at + bt) / 2;
		double r = ep * ((uint64_t)1 << (nmax - j)) - (bt - at) / 2;
		double delta = k1 * std::pow((bt - at), k2);

		// Interpolation
		double xf = (b.fs * at - a.fs * bt) / (b.fs - a.fs);

		// Truncation
		double sigma = sign(x12 - xf);
		double xt = (delta <= std::abs(x12 - xf)) ? xf + sigma * delta : x12;

		// Projection
		xitp = (std::abs(xt - x12) <= r) ? xt : x12 - sigma * r;

		if (xitp == at || xitp == bt)
		{
			Seed res;
			res.x = xitp * (b.x - a.x) + a.x;
			res.y = xitp * (b.y - a.y) + a.y;
			a = res;
			goto evaluate;
		}

		// Updating Interval
		double zitp = func(xitp * (b.x - a.x) + a.x, xitp * (b.y - a.y) + a.y);
		if (!std::isfinite(zitp)) return false;
		if (zitp > 0)
		{
			bt = xitp;
			b.fs = zitp;
		}
		else if (zitp < 0)
		{
			at = xitp;
			a.fs = zitp;
		}
		else
		{
			Seed res;
			res.x = xitp * (b.x - a.x) + a.x;
			res.y = xitp * (b.y - a.y) + a.y;
			a = res;
			goto evaluate;
		}
		j++;
	}
	{
		Seed res;
		xitp = (at + bt) / 2;
		res.x = xitp * (b.x - a.x) + a.x;
		res.y = xitp * (b.y - a.y) + a.y;
		a = res;
	}

evaluate:
	a.fs = 0.0;
	return true;
}