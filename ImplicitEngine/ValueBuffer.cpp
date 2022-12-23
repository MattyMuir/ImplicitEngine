#include "ValueBuffer.h"

ValueBuffer::ValueBuffer(ValueBuffer&& other) noexcept
{
	bufSize = other.bufSize;
	vals = std::move(other.vals);
	active = std::move(other.active);
}

void ValueBuffer::operator=(const ValueBuffer& other)
{
	bufSize = other.bufSize;
	vals = other.vals;
	active = other.active;
}

ValueBuffer::ValueBuffer(int64_t bufSize_)
	: bufSize(bufSize_), vals(bufSize), active(bufSize, false)
{}

double& ValueBuffer::operator[](int64_t index)
{
	active[index] = true;
	return vals[index];
}

void ValueBuffer::SetAllActive(bool val)
{
	std::fill(active.begin(), active.end(), val);
}