#pragma once
#include <cassert>
#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
#include "glerr.h"

#define GlCall(x) GLClearError();\
	x;\
	assert(GLLogCall(#x, __FILE__, __LINE__));