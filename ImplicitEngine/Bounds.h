#pragma once
struct Bounds
{
	double xmin, ymin, xmax, ymax;

	double w() const { return xmax - xmin; }
	double h() const { return ymax - ymin; }

	bool In(double x, double y) const
	{
		return x > xmin && x < xmax&& y > ymin && y < ymax;
	}

	Bounds Expand(double fac) const
	{
		fac -= 1;
		double bw = w();
		double bh = h();
		return {	xmin - bw * fac / 2,
					xmax + bw * fac / 2,
					ymin - bh * fac / 2,
					ymax + bh * fac / 2, };
	}
};