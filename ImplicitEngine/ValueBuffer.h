#pragma once
#include <vector>

struct ValueBuffer
{
	int bufSize = 0;
	std::vector<double> vals;
	std::vector<bool> active;

	ValueBuffer() = delete;
	ValueBuffer(const ValueBuffer& other) = delete;
	ValueBuffer(ValueBuffer&& other);
	void operator=(const ValueBuffer& other);

	// Constructor
	ValueBuffer(int bufSize_);

	double& operator[](int index);
	void SetActive(bool val);
};