#pragma once
#include "glall.h"
#include <vector>

struct LayoutElement
{
	unsigned int type;
	unsigned int count;
	bool normalized;

	size_t GetElementSize() const
	{
		size_t valSize = 0;
		switch (type)
		{
		case GL_FLOAT:
			valSize = sizeof(GLfloat);
			break;
		case GL_INT:
			valSize = sizeof(GLint);
			break;
		case GL_DOUBLE:
			valSize = sizeof(GLdouble);
			break;
		}
		return valSize * count;
	}
};

class VertexBufferLayout
{
public:
	VertexBufferLayout()
	{
		stride = 0;
	}

	template <typename T>
	void Push(unsigned int count)
	{
		__debugbreak(); // Unsupported type, break
	}

	// Specializations of 'Push' function for certain types
	template<>
	void Push<float>(unsigned int count)
	{
		elements.push_back({ GL_FLOAT, count, false });
		stride += sizeof(GLfloat) * count;
	}
	template<>
	void Push<int>(unsigned int count)
	{
		elements.push_back({ GL_INT, count, false });
		stride += sizeof(GLint) * count;
	}
	template<>
	void Push<double>(unsigned int count)
	{
		elements.push_back({ GL_DOUBLE, count, false });
		stride += sizeof(GLdouble) * count;
	}

	inline size_t GetStride() const { return stride; }
	inline const std::vector<LayoutElement>& GetElements() const { return elements; }

private:
	std::vector<LayoutElement> elements;
	size_t stride;
};