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

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    Mesh(Mesh &&) = default; // allow moving
    Mesh &operator=(Mesh &&) = default;

    void Init(const void *data, size_t dataSize, std::initializer_list<AttribElement> layout, GLenum usage = GL_STATIC_DRAW);
    void Init(const void *data, size_t dataSize, const unsigned int *indices, size_t numIndices, std::initializer_list<AttribElement> layout, GLenum usage = GL_STATIC_DRAW);

    unsigned int GetVAO() const { return m_VAO; }
    int GetNumVertices() const { return m_NumVertices; }

    int GetNumIndices() const { return m_NumIndices; }
    bool HasIndices() const { return m_EBO != 0; }

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;

    int m_NumVertices = 0;
    int m_NumIndices = 0;
};
