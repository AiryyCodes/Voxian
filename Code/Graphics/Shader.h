#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include <string>

class Shader
{
public:
    ~Shader();

    void Init(const std::string &vertexPath, const std::string &fragmentPath);

    void Bind() const;
    void Unbind() const;

    void SetUniform(const std::string &name, const Matrix4 &value) const;
    void SetUniform(const std::string &name, const Vector2 &value) const;
    void SetUniform(const std::string &name, bool value) const;
    void SetUniform(const std::string &name, int value) const;

private:
    unsigned int CompileShader(const std::string &path, const std::string &name, unsigned int type);

private:
    unsigned int m_Id = 0;
};
