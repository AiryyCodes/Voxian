#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderLibrary.h"
#include "World/Entity/Component/Component.h"

#include <string>

struct ChunkMeshRenderer : public Component
{
    ChunkMeshRenderer() {}

    void RenderOpaque(Renderer &renderer);
    void RenderTransparent(Renderer &renderer);

    Mesh &GetOpaqueMesh() { return m_OpaqueMesh; }
    Mesh &GetTransparentMesh() { return m_TransparentMesh; }

private:
    Mesh m_OpaqueMesh;
    Mesh m_TransparentMesh;

    std::string m_Shader = Shaders::Chunk;
};
