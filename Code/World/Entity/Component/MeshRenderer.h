#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Component/Component.h"

#include <string>

struct MeshRenderer : public Component
{
    MeshRenderer(std::string shader)
        : m_Shader(std::move(shader)) {}

    void OnRender(Renderer &renderer) override;

    Mesh &GetMesh() { return m_Mesh; }

private:
    Mesh m_Mesh;
    std::string m_Shader;
};
