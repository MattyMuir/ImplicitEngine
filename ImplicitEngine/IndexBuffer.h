#pragma once
#include "glall.h"

class IndexBuffer
{
public:
	IndexBuffer(const unsigned int* data, size_t count_);
	~IndexBuffer();
	void Bind() const;
	void Unbind() const;

	inline unsigned int GetCount() const { return count; }

private:
	unsigned int dataID;
	unsigned int count;
};