#pragma once
#include "glall.h"

class IndexBuffer
{
public:
	IndexBuffer(const unsigned int* data, size_t count_);
	~IndexBuffer();
	void Bind() const;
	void Unbind() const;

	inline size_t GetCount() const { return count; }

private:
	unsigned int dataID;
	size_t count;
};