#pragma once

#include "Attribute.h"

#include <cstddef>
#include <initializer_list>
#include <glad/gl.h>

class Mesh
{
public:
    Mesh();
    ~Mesh();

    void Init(const void *data, size_t dataSize, std::initializer_list<AttribElement> layout, GLenum usage = GL_STATIC_DRAW);

    unsigned int GetVAO() const { return m_VAO; }
    int GetNumVertices() const { return m_NumVertices; }

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;

    int m_NumVertices = 0;
};
