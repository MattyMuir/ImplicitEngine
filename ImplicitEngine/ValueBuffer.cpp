#include "ValueBuffer.h"

ValueBuffer::ValueBuffer(ValueBuffer&& other)
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

ValueBuffer::ValueBuffer(int bufSize_)
	: bufSize(bufSize_), vals(bufSize), active(bufSize, false)
{}

double& ValueBuffer::operator[](int index)
{
	active[index] = true;
	return vals[index];
}

void ValueBuffer::SetActive(bool val)
{
	std::fill(active.begin(), active.end(), val);
}