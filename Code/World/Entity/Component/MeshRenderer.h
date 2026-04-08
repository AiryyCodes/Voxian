#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Component/Camera.h"
#include "World/Entity/Entity.h"
#include "World/Entity/Component/Component.h"
#include <string>
#include <utility>

struct MeshRenderer : public Component
{
    MeshRenderer(std::string shader)
        : m_Shader(std::move(shader)) {}

    void OnRender(Renderer &renderer) override
    {
        auto &transform = GetOwner().GetComponent<Transform>();
        renderer.Submit(m_Mesh, m_Shader, [&](Shader &shader)
                        {
                            if (Camera *camera = renderer.GetCamera())
                            {
                                shader.Set("u_View", camera->GetViewMatrix());
                                shader.Set("u_Projection", camera->GetProjectionMatrix());
                            }
                            shader.Set("u_Transform", transform.GetMatrix());
                            // This comment will live here until i add more uniforms (for formattings sake)
                        });
    }

    Mesh &GetMesh() { return m_Mesh; }

private:
    Mesh m_Mesh;
    std::string m_Shader;
};
