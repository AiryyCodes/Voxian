#pragma once

#include <string>

class Shader
{
public:
    Shader(const std::string &basePath);
    ~Shader();

    void Bind();
    static void Unbind();

private:
    unsigned int CompileStage(unsigned int type, const std::string &source, const std::string &name);

    std::string ReadFile(const std::string &path);

private:
    unsigned int m_ProgramId = 0;
};
