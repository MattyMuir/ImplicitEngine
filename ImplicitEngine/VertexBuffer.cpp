#include "VertexBuffer.h"

VertexBuffer::VertexBuffer()
{
	dataID = 0;
	size = 0;
	capacity = 0;

	GlCall(glGenBuffers(1, &dataID));
	GlCall(glBindBuffer(GL_ARRAY_BUFFER, dataID));
}

VertexBuffer::VertexBuffer(const void* data, size_t size_)
{
	dataID = 0;
	size = size_;
	capacity = size;

	GlCall(glGenBuffers(1, &dataID));
	GlCall(glBindBuffer(GL_ARRAY_BUFFER, dataID));
	GlCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &dataID);
}

void VertexBuffer::Bind() const
{
	GlCall(glBindBuffer(GL_ARRAY_BUFFER, dataID));
}

void VertexBuffer::Unbind() const
{
	GlCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBuffer::SetData(void* newData)
{
	Bind();
	GlCall(glBufferSubData(GL_ARRAY_BUFFER, 0, size, newData));
}

void VertexBuffer::SetData(void* newData, size_t newSize)
{
	Bind();
	if (capacity >= newSize)
	{
		GlCall(glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, newData));
	}
	else
	{
		GlCall(glBufferData(GL_ARRAY_BUFFER, newSize, newData, GL_DYNAMIC_DRAW));
		capacity = newSize;
	}
	size = newSize;
}

size_t VertexBuffer::Size()
{
	return size;
}