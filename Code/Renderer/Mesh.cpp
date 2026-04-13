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
    if (m_EBO)
        glDeleteBuffers(1, &m_EBO);
}

void Mesh::Init(const void *data, size_t dataSize, std::initializer_list<AttribElement> layout, unsigned int usage)
{
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);

    int stride = 0;
    for (const auto &element : layout)
        stride += AttribInfo::FromType(element.Type).Size;

    unsigned int index = 0;
    size_t offset = 0;
    for (const auto &element : layout)
    {
        auto info = AttribInfo::FromType(element.Type);
        glEnableVertexAttribArray(index);
        switch (element.Type)
        {
        case AttribType::Float:
        case AttribType::Float2:
        case AttribType::Float3:
        case AttribType::Float4:
            glVertexAttribPointer(index, info.Components, info.GlType,
                                  element.Normalized, stride, (void *)offset);
            break;
        case AttribType::Int:
        case AttribType::UInt:
            glVertexAttribIPointer(index, info.Components, info.GlType, stride, (void *)offset);
            break;
        }
        offset += info.Size;
        index++;
    }

    m_NumVertices = (int)(dataSize / stride);
    glBindVertexArray(0);
}

void Mesh::Init(const void *data, size_t dataSize, const unsigned int *indices, size_t numIndices, std::initializer_list<AttribElement> layout, unsigned int usage)
{
    Init(data, dataSize, layout, usage);

    glBindVertexArray(m_VAO);

    // Create EBO for indices
    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(unsigned int), indices, usage);

    m_NumIndices = (int)numIndices;

    glBindVertexArray(0);
}

void Mesh::SetTexture(int width, int height, const void *data, int format)
{
    m_Texture = std::make_unique<Texture>();
    m_Texture->Init(width, height, data, format);
}

void Mesh::SetTexture(int width, int height, int layers, const std::vector<std::string> &fileNames, int format)
{
    m_Texture = std::make_unique<TextureArray2D>();
    m_Texture->Init(width, height, layers, fileNames, format);
}
