#include "pow4.h"

uint64_t Pow2(uint64_t x)
{
	return ((uint64_t)1 << x);
}

uint64_t Pow4(uint64_t x)
{
	return ((uint64_t)1 << (x << (uint64_t)1));
}