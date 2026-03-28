#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include <string>
#include <unordered_map>

class Shader
{
public:
    Shader(const std::string &basePath);
    ~Shader();

    void Bind();
    static void Unbind();

    void Set(const char *name, const Vector3f &value);
    void Set(const char *name, const Matrix4 &value);

private:
    unsigned int CompileStage(unsigned int type, const std::string &source, const std::string &name);

    std::string ReadFile(const std::string &path);

    unsigned int GetUniformLocation(const char *name);

private:
    unsigned int m_ProgramId = 0;

    std::unordered_map<std::string, unsigned int> m_UniformCache;
};
