#include "Shader.h"

Shader::~Shader()
{
    glDeleteProgram(dataID);
}

Shader* Shader::CompileFile(const std::string& filepath)
{
    std::string vShaderSource, fShaderSource;
    Shader::SplitShader(filepath, vShaderSource, fShaderSource);

    unsigned int vShader, fShader;
    vShader = Shader::CompileIndividual(GL_VERTEX_SHADER, vShaderSource);
    fShader = Shader::CompileIndividual(GL_FRAGMENT_SHADER, fShaderSource);

    return new Shader(Shader::LinkShaders(vShader, fShader));
}

Shader* Shader::CompileString(std::string_view shaderStr)
{
    std::string vShaderSource, fShaderSource;
    std::stringstream shaderSS;
    shaderSS << shaderStr;
    Shader::SplitShader(shaderSS, vShaderSource, fShaderSource);

    unsigned int vShader, fShader;
    vShader = Shader::CompileIndividual(GL_VERTEX_SHADER, vShaderSource);
    fShader = Shader::CompileIndividual(GL_FRAGMENT_SHADER, fShaderSource);

    return new Shader(Shader::LinkShaders(vShader, fShader));
}

void Shader::Bind() const
{
    GlCall(glUseProgram(dataID));
}

void Shader::Unbind() const
{
    GlCall(glUseProgram(0));
}

GLint Shader::GetUniformLocation(const std::string& varName)
{
    GlCall(GLint location = glGetUniformLocation(dataID, varName.c_str()));
    assert(location != -1);
    return location;
}

Shader::Shader(unsigned int id)
    : dataID(id) {}

void Shader::SplitShader(const std::string& filepath, std::string& vShader, std::string& fShader)
{
    std::ifstream file(filepath);
    std::stringstream fileSS;
    fileSS << file.rdbuf();
    file.close();

    SplitShader(fileSS, vShader, fShader);
}

void Shader::SplitShader(std::stringstream& shaderStream, std::string& vShader, std::string& fShader)
{
    std::stringstream ss[2];

    std::string line;
    ShaderType currentlyReading = ShaderType::VERTEX;
    while (std::getline(shaderStream, line, '\n'))
    {
        if (line == "#shader vertex")
            currentlyReading = ShaderType::VERTEX;
        else if (line == "#shader fragment")
            currentlyReading = ShaderType::FRAGMENT;
        else
        {
            ss[(int)currentlyReading] << line << "\n";
        }
    }
    vShader = ss[0].str();
    fShader = ss[1].str();
}

unsigned int Shader::CompileIndividual(unsigned int shaderType, const std::string& source)
{
    unsigned int id = glCreateShader(shaderType);
    const char* source_c = source.c_str();

    glShaderSource(id, 1, &source_c, nullptr);
    glCompileShader(id);

    // Error checking
    int compileResult;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compileResult);
    if (compileResult == GL_FALSE)
    {
        // Compilation failed
        int msgLength;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &msgLength);

        char* msg = new char[msgLength];
        glGetShaderInfoLog(id, msgLength, &msgLength, msg);

        std::cerr << "Failed to compile " << (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!\n";
        std::cerr << msg << "\n";

        delete[] msg;
        return 0;
    }

    return id;
}

unsigned int Shader::LinkShaders(unsigned int vShader, unsigned int fShader)
{
    unsigned int id = glCreateProgram();
    glAttachShader(id, vShader);
    glAttachShader(id, fShader);

    glLinkProgram(id);

    // Error checking
    int linkStatus;
    glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE)
    {
        // Linking failed
        int msgLength;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &msgLength);

        char* msg = new char[msgLength];
        glGetProgramInfoLog(id, msgLength, &msgLength, msg);

        std::cerr << "Failed to link shaders!\n";
        std::cerr << msg << "\n";

        delete[] msg;
        return 0;
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return id;
}