#pragma once
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray
{
public:
	VertexArray();
	~VertexArray();

	void Bind() const;
	void Unbind() const;
	void AddVBuffer(const VertexBuffer& vb, const VertexBufferLayout& vbLayout);

private:
	unsigned int dataID;
};