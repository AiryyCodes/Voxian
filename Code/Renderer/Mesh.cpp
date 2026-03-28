#include "Renderer/Mesh.h"

#include <glad/gl.h>

Mesh::Mesh()
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
}

void Mesh::Init(const void *data, size_t dataSize, std::initializer_list<AttribElement> layout, unsigned int usage)
{
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, dataSize, data, usage);

    int stride = 0;
    for (const auto &element : layout)
        stride += AttribInfo::FromType(element.Type).Size;

    unsigned int index = 0;
    size_t offset = 0;
    for (const auto &element : layout)
    {
        auto info = AttribInfo::FromType(element.Type);
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, info.Components, info.GlType,
                              element.Normalized, stride, (void *)offset);
        offset += info.Size;
        index++;
    }

    m_NumVertices = (int)(dataSize / stride);
    glBindVertexArray(0);
}
