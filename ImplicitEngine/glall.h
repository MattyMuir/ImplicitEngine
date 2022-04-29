#pragma once
#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>

#define ASSERT(x) if(!(x)) __debugbreak();

#define GlCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__));

static void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* func, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cerr << "[OpenGL Error] (" << error << "): " << func << " " << file << ":" << line << "\n";
		return false;
	}
	return true;
}