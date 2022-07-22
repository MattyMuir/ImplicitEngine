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

	static Shader* CompileFile(const std::string& filepath);
	static Shader* CompileString(std::string_view shaderStr);

	void Bind() const;
	void Unbind() const;

	unsigned int GetUniformLocation(const std::string& varName);
	// Uniform setting is done using 'glUniform[TYPE](location, ...)'
	// e.g.		glUniform4f(location, 0.1f, 0.2f, 0.3f, 0.4f);
private:
	unsigned int dataID;

	Shader(unsigned int id);
	static void SplitShader(const std::string& filepath, std::string& vShader, std::string& fShader);
	static void SplitShader(std::stringstream& shaderStream, std::string& vShader, std::string& fShader);
	static unsigned int CompileIndividual(unsigned int shaderType, const std::string& source);
	static unsigned int LinkShaders(unsigned int vShader, unsigned int fShader);
};