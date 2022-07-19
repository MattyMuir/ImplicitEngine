#include "pow4.h"

int64_t Pow2(int64_t x)
{
	return ((int64_t)1 << x);
}

int64_t Pow4(int64_t x)
{
	return ((int64_t)1 << (x << (int64_t)1));
}