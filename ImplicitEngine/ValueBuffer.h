#pragma once
#include <vector>

struct ValueBuffer
{
	int64_t bufSize = 0;
	std::vector<double> vals;
	std::vector<uint8_t> active;

	ValueBuffer() = delete;
	ValueBuffer(const ValueBuffer& other) = delete;
	ValueBuffer(ValueBuffer&& other) noexcept;
	void operator=(const ValueBuffer& other);

	// Constructor
	ValueBuffer(int64_t bufSize_);

	double& operator[](int64_t index);
	void SetAllActive(bool val);
};