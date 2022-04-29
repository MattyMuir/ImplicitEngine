#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(const unsigned int* data, size_t count_)
	: count(count_)
{
	GlCall(glGenBuffers(1, &dataID));
	GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dataID));
	GlCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &dataID);
}

void IndexBuffer::Bind() const
{
	GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dataID));
}

void IndexBuffer::Unbind() const
{
	GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}