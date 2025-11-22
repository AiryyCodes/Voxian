
#include "World/Chunk.h"
#include "World/ChunkManager.h"
#include "Graphics/Texture.h"
#include "Graphics/Vertex.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Memory.h"
#include "World/Block.h"

#include <cstddef>
#include <glad/gl.h>
#include <FastNoiseLite.h>
#include <mutex>

Chunk::~Chunk()
{
    if (m_State == State::Done)
        DeleteGPUData();
}

void Chunk::UploadMeshToGPU()
{
    if (m_State != State::MeshReady)
        return;

    std::scoped_lock lock(m_MeshMutex);

    if (m_Mesh.Vertices.empty() || m_Mesh.Indices.empty())
        return;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_Mesh.Vertices.size() * sizeof(BlockVertex), m_Mesh.Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Mesh.Indices.size() * sizeof(unsigned int), m_Mesh.Indices.data(), GL_STATIC_DRAW);

    GLsizei stride = sizeof(BlockVertex);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)(offsetof(BlockVertex, Position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(offsetof(BlockVertex, Normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(offsetof(BlockVertex, UV)));
    glEnableVertexAttribArray(2);

    glVertexAttribIPointer(3, 2, GL_INT, stride, (void *)(offsetof(BlockVertex, TextureSize)));
    glEnableVertexAttribArray(3);

    glVertexAttribIPointer(4, 1, GL_INT, stride, (void *)(offsetof(BlockVertex, Layer)));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void *)(offsetof(BlockVertex, AO)));
    glEnableVertexAttribArray(5);

    glBindVertexArray(0);

    // Update state
    SetState(State::Done);
}

void Chunk::DeleteGPUData()
{
    std::scoped_lock lock(m_MeshMutex);

    if (m_VAO)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO)
    {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_EBO)
    {
        glDeleteBuffers(1, &m_EBO);
        m_EBO = 0;
    }

    if (m_State == State::Done)
        SetState(State::MeshReady); // fallback to indicate mesh exists but not on GPU
}

void Chunk::Draw(const Shader &shader)
{
    if (m_State != State::Done)
        return;

    Vector3f chunkWorldPos{m_Position.x * CHUNK_WIDTH, 0.0f, m_Position.y * CHUNK_WIDTH};
    Matrix4 model = glm::translate(Matrix4(1.0f), chunkWorldPos);
    shader.SetUniform("u_Model", model);

    Ref<TextureArray2D> texture = g_BlockRegistry.GetTexture();
    if (!texture)
        return;

    texture->Bind(shader);

    shader.SetUniform("u_Block.Diffuse", 0);
    shader.SetUniform("u_Block.MaxTexSize", Vector2(texture->GetMaxWidth(), texture->GetMaxHeight()));

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_Mesh.Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Chunk::SetBlockData(const BlockData &data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Blocks = data;
    m_NeedsRebuild = true;
    SetState(State::BlocksReady);
}

void Chunk::SetMeshData(MeshData &data)
{
    std::lock_guard<std::mutex> lock(m_MeshMutex);
    m_Mesh.Vertices = data.Vertices;
    m_Mesh.Indices = data.Indices;
    SetState(State::MeshReady);
}

void Chunk::MarkMeshDirty()
{
    // Set the dirty flag
    m_NeedsRebuild.store(true, std::memory_order_release);

    // Optionally, if chunk is already meshed, mark it as ready to regenerate
    State currentState = GetState();
    if (currentState == State::MeshReady || currentState == State::Done)
    {
        SetState(State::BlocksReady);
    }
}
