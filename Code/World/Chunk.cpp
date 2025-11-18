
#include "World/Chunk.h"

#include <glad/gl.h>
#include <FastNoiseLite.h>
#include <mutex>

Chunk::~Chunk()
{
    if (m_GpuReady)
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }
}

void Chunk::UploadMeshToGPU()
{
    if (m_GpuReady)
        return;

    std::scoped_lock lock(m_MeshMutex);

    if (m_Mesh.Vertices.empty() || m_Mesh.Indices.empty())
        return;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_Mesh.Vertices.size() * sizeof(float), m_Mesh.Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Mesh.Indices.size() * sizeof(unsigned int), m_Mesh.Indices.data(), GL_STATIC_DRAW);

    GLsizei stride = 9 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    m_GpuReady = true;
}

void Chunk::Draw(const Shader &shader)
{
    if (!m_GpuReady)
        return;

    glm::vec3 chunkWorldPos{m_Position.x * CHUNK_WIDTH, 0.0f, m_Position.y * CHUNK_WIDTH};
    glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkWorldPos);
    shader.SetUniform("u_Model", model);

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_Mesh.Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Chunk::SetBlockData(BlockData &data)
{
    m_Blocks = data.Blocks;

    m_BlocksReady = true;
}

void Chunk::SetMeshData(MeshData &data)
{
    m_Mesh.Vertices = data.Vertices;
    m_Mesh.Indices = data.Indices;

    m_MeshReady = true;
    m_GpuReady = false;
}
