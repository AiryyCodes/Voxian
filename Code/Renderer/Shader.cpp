#include "Renderer/Shader.h"
#include "Logger.h"

#include <cassert>
#include <fstream>
#include <glad/gl.h>
#include <string>

Shader::Shader(const std::string &basePath)
{
    std::string vertexSource = ReadFile(basePath + ".vert");
    std::string fragmentSource = ReadFile(basePath + ".frag");

    unsigned int vertexId = CompileStage(GL_VERTEX_SHADER, vertexSource, "Vertex");
    unsigned int fragmentId = CompileStage(GL_FRAGMENT_SHADER, fragmentSource, "Fragment");

    m_ProgramId = glCreateProgram();
    glAttachShader(m_ProgramId, vertexId);
    glAttachShader(m_ProgramId, fragmentId);
    glLinkProgram(m_ProgramId);

    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);
}

Shader::~Shader()
{
    if (!m_ProgramId)
        return;

    glDeleteProgram(m_ProgramId);
}

void Shader::Bind()
{
    glUseProgram(m_ProgramId);
}

void Shader::Unbind()
{
    glUseProgram(0);
}

unsigned int Shader::CompileStage(unsigned int type, const std::string &source, const std::string &name)
{
    unsigned int id = glCreateShader(type);

    const char *cstr = source.c_str();
    glShaderSource(id, 1, &cstr, nullptr);
    glCompileShader(id);

    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(id, 512, nullptr, log);
        LOG_FATAL("{} shader compilation failed: {}", name, log);
        assert(false);
    }

    return id;
}

std::string Shader::ReadFile(const std::string &path)
{
    std::ifstream file(path);
    assert(file.is_open() && "Shader file not found!");
    return {std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
}
