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
};