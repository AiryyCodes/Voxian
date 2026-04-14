#include "World/Entity/Component/MeshRenderer.h"
#include "World/Entity/Component/Camera.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"

void MeshRenderer::OnRender(Renderer &renderer)
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

                        if (m_Mesh.GetTexture())
                        {
                            shader.Set("u_Texture", 0);
                        }
                        // This comment will live here forever (for formattings sake until i fix the .clang-format file to not mess with my formatting in lambdas)
                    });
}