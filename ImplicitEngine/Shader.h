#pragma once
#include "glall.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

enum class ShaderType { NONE = -1, VERTEX = 0, FRAGMENT = 1 };

class Shader
{
public:
	~Shader();

	static Shader Compile(const std::string& filepath);

	void Bind() const;
	void Unbind() const;

	unsigned int GetUniformLocation(const std::string& varName);
	// Uniform setting is done using 'glUniform[TYPE](location, ...)'
	// e.g.		glUniform4f(location, 0.1f, 0.2f, 0.3f, 0.4f);
private:
	unsigned int dataID;

	Shader(unsigned int id);
	static void SplitShaderFile(const std::string& filepath, std::string& vShader, std::string& fShader);
	static unsigned int CompileIndividual(unsigned int shaderType, const std::string& source);
	static unsigned int LinkShaders(unsigned int vShader, unsigned int fShader);
};