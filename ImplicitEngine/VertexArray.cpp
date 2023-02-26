#include "VertexArray.h"
VertexArray::VertexArray()
{
	GlCall(glGenVertexArrays(1, &dataID));
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &dataID);
}

void VertexArray::Bind() const
{
	GlCall(glBindVertexArray(dataID));
}

void VertexArray::Unbind() const
{
	GlCall(glBindVertexArray(0));
}

void VertexArray::AddVBuffer(const VertexBuffer& vb, const VertexBufferLayout& vbLayout)
{
	Bind();
	vb.Bind();
	const auto& layoutElements = vbLayout.GetElements();
	char* offset = (char*)0;
	for (size_t i = 0; i < layoutElements.size(); i++)
	{
		const LayoutElement& element = layoutElements[i];

		GlCall(glEnableVertexAttribArray(i));
		GlCall(glVertexAttribPointer(i, element.count, element.type,
			element.normalized ? GL_TRUE : GL_FALSE, (int)vbLayout.GetStride(), offset));

		offset += element.GetElementSize();
	}
}