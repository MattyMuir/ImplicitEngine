#pragma once
#include "glall.h"

class VertexBuffer
{
public:
	VertexBuffer();
	VertexBuffer(const void* data, size_t size_);
	~VertexBuffer();
	void Bind() const;
	void Unbind() const;
	void SetData(void* newData);
	void SetData(void* newData, size_t newSize);
	size_t Size();

private:
	unsigned int dataID;
	size_t size;
	size_t capacity;
};