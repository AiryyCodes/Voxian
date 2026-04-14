#include "ChunkMeshRenderer.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Component/Camera.h"
#include "World/Entity/Component/Transform.h"
#include "World/Entity/Entity.h"

void ChunkMeshRenderer::RenderOpaque(Renderer &renderer)
{
    auto &transform = GetOwner().GetComponent<Transform>();
    renderer.Submit(m_OpaqueMesh, m_Shader, [&](Shader &shader)
                    {
                        if (Camera *camera = renderer.GetCamera())
                        {
                            shader.Set("u_View", camera->GetViewMatrix());
                            shader.Set("u_Projection", camera->GetProjectionMatrix());
                        }
                        shader.Set("u_Transform", transform.GetMatrix());

                        if (m_OpaqueMesh.GetTexture())
                        {
                            shader.Set("u_Texture", 0);
                        }

                        glEnable(GL_DEPTH_TEST);
                        glDepthFunc(GL_LESS);
                        glDepthMask(GL_TRUE);
                        glDisable(GL_BLEND);
                        glEnable(GL_CULL_FACE);

                        // This comment will live here forever (for formattings sake until i fix the .clang-format file to not mess with my formatting in lambdas)
                    });
}

void ChunkMeshRenderer::RenderTransparent(Renderer &renderer)
{
    auto &transform = GetOwner().GetComponent<Transform>();

    renderer.Submit(m_TransparentMesh, m_Shader, [&](Shader &shader)
                    {
                        if (Camera *camera = renderer.GetCamera())
                        {
                            shader.Set("u_View", camera->GetViewMatrix());
                            shader.Set("u_Projection", camera->GetProjectionMatrix());
                        }
                        shader.Set("u_Transform", transform.GetMatrix());

                        if (m_TransparentMesh.GetTexture())
                        {
                            shader.Set("u_Texture", 0);
                        }

                        glEnable(GL_DEPTH_TEST);
                        glDepthFunc(GL_LESS);
                        glDepthMask(GL_TRUE);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glEnable(GL_CULL_FACE);

                        // This comment will live here forever (for formattings sake until i fix the .clang-format file to not mess with my formatting in lambdas)
                    });
}