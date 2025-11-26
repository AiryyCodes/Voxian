#include <algorithm>
#include <memory>
#include <random>
#include <stdexcept>
#include <glad/gl.h>

#include "World/ChunkManager.h"
#include "Graphics/Camera.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "Graphics/Window.h"
#include "Logger.h"
#include "Math/Vector.h"
#include "Memory.h"
#include "Queue.h"
#include "World/BlockRegistry.h"
#include "World/Chunk.h"
#include "World/Structure.h"

const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

ChunkManager::ChunkManager()
{
    shadowShader.Init("Assets/shaders/Shadow.vert", "Assets/shaders/Shadow.frag");

    glGenFramebuffers(1, &depthMapFBO);

    // Create 2D depth texture
    glGenTextures(1, &depthMap); // use member variable, not local
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE); // no color output
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LOG_ERROR("Shadow framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ChunkManager::RequestChunk(int x, int z)
{
    Vector2i key{x, z};
    std::shared_ptr<Chunk> chunk;

    {
        std::scoped_lock lock(m_ChunkMutex);

        // If chunk exists and isn't empty, skip
        auto it = m_Chunks.find(key);
        if (it != m_Chunks.end())
        {
            if (it->second->GetState() != Chunk::State::Empty)
                return;
            chunk = it->second;
        }
        else
        {
            // Create new chunk
            chunk = std::make_shared<Chunk>(this, x, z);
            m_Chunks[key] = chunk;
        }
        m_RequestedChunks.insert(key);
        chunk->SetState(Chunk::State::Generating);
    }

    std::weak_ptr<Chunk> weakChunk = chunk;

    // Async block generation
    m_ThreadPool.post([this, weakChunk]()
                      {
        if (auto chunk = weakChunk.lock())
        {
            BlockData blockData = GenerateBlocks(chunk->m_Position.x, chunk->m_Position.y);

            Vector2i chunkPos = chunk->GetPosition();
            ApplyStructures(chunkPos.x, chunkPos.y, blockData);

            // Push block result to main thread queue
            m_BlockQueue.push({
                .callback = [weakChunk](BlockData &data)
                {
                    if (auto chunk = weakChunk.lock())
                    {
                        chunk->SetBlockData(data);
                        chunk->SetState(Chunk::State::BlocksReady);
                    }
                },
                .result = std::move(blockData)
            });
        } });
}

void ChunkManager::UpdatePlayerPosition(const Vector3 &pos)
{
    m_PlayerPosition = pos;
    Vector2i center{
        int(std::floor(pos.x / CHUNK_WIDTH)),
        int(std::floor(pos.z / CHUNK_WIDTH))};

    // LOG_INFO("Chunk Pos: ({}, {})", center.x, center.y);

    // Calculate desired chunks
    std::vector<std::pair<int, Vector2i>> desired;
    for (int dz = -m_ViewDistance; dz <= m_ViewDistance; dz++)
    {
        for (int dx = -m_ViewDistance; dx <= m_ViewDistance; dx++)
        {
            Vector2i c{center.x + dx, center.y + dz};
            int dist2 = dx * dx + dz * dz;
            desired.emplace_back(dist2, c);
        }
    }

    std::sort(desired.begin(), desired.end(),
              [](auto &a, auto &b)
              { return a.first < b.first; });

    // Mark unloads
    {
        std::scoped_lock lock(m_ChunkMutex);
        for (auto &[pos, chunk] : m_Chunks)
        {
            bool keep = false;
            for (auto &d : desired)
                if (d.second == pos)
                    keep = true;

            chunk->m_ShouldUnload = !keep;
        }
    }

    int MAX_REQUESTS_PER_FRAME = 2;
    int made = 0;

    for (auto &[dist, c] : desired)
    {
        if (made >= MAX_REQUESTS_PER_FRAME)
            break;

        // Already exists?
        if (m_Chunks.find(c) != m_Chunks.end())
            continue;

        // Already requested?
        if (m_RequestedChunks.find(c) != m_RequestedChunks.end())
            continue;

        // Request it now
        RequestChunk(c.x, c.y);
        made++;
    }
}

void ChunkManager::Update(const Shader &shader, const Camera &camera)
{
    int px = static_cast<int>(std::floor(m_PlayerPosition.x / CHUNK_WIDTH));
    int pz = static_cast<int>(std::floor(m_PlayerPosition.z / CHUNK_WIDTH));
    Vector2i playerChunk{px, pz};

    // Apply block results
    ThreadTask<BlockData> blockTask;
    while (m_BlockQueue.pop(blockTask))
    {
        blockTask.callback(blockTask.result);
    }

    // Apply mesh results
    ThreadTask<MeshData> meshTask;
    while (m_MeshQueue.pop(meshTask))
    {
        meshTask.callback(meshTask.result);
    }

    // Collect chunks that need mesh rebuild
    std::vector<std::shared_ptr<Chunk>> dirtyChunks;
    {
        std::scoped_lock lock(m_ChunkMutex);
        for (auto &[pos, chunk] : m_Chunks)
        {
            Chunk::State state = chunk->GetState();

            // Only rebuild if blocks are ready but mesh not yet generated
            if ((state == Chunk::State::BlocksReady || state == Chunk::State::MeshReady) && chunk->m_NeedsRebuild)
            {
                dirtyChunks.push_back(chunk);
            }
        }
    }

    // Sort by distance to player so closest chunks rebuild first
    std::sort(dirtyChunks.begin(), dirtyChunks.end(),
              [&](const std::shared_ptr<Chunk> &a, const std::shared_ptr<Chunk> &b)
              {
                  int dxA = a->GetPosition().x - playerChunk.x;
                  int dzA = a->GetPosition().y - playerChunk.y;
                  int dxB = b->GetPosition().x - playerChunk.x;
                  int dzB = b->GetPosition().y - playerChunk.y;
                  return (dxA * dxA + dzA * dzA) < (dxB * dxB + dzB * dzB);
              });

    // Post mesh rebuild tasks
    int rebuildCount = 0;
    for (auto &chunk : dirtyChunks)
    {
        if (rebuildCount >= m_MaxRebuildsPerFrame)
            break;

        chunk->m_NeedsRebuild = false;

        m_ThreadPool.post([this, chunk]()
                          {
            // Only generate mesh if blocks are ready
            if (chunk->GetState() == Chunk::State::BlocksReady || chunk->GetState() == Chunk::State::MeshReady)
            {
                MeshData newMesh = GenerateMesh(chunk->m_Blocks, chunk->GetPosition().x, chunk->GetPosition().y);

                // Push result to mesh queue
                m_MeshQueue.push({.callback = [chunk](MeshData &mesh)
                {
                    chunk->SetMeshData(mesh);
                    chunk->SetState(Chunk::State::MeshReady); // transition to MeshReady
                },
                .result = std::move(newMesh)});
            } });

        rebuildCount++;
    }

    // Upload meshes, draw, unload
    const int renderDistance = m_ViewDistance;

    std::vector<std::shared_ptr<Chunk>> opaqueChunks;
    std::vector<std::shared_ptr<Chunk>> transparentChunks;

    {
        std::scoped_lock lock(m_ChunkMutex);

        for (auto it = m_Chunks.begin(); it != m_Chunks.end();)
        {
            auto &chunk = it->second;
            int dx = chunk->GetPosition().x - playerChunk.x;
            int dz = chunk->GetPosition().y - playerChunk.y;
            int dist2 = dx * dx + dz * dz;

            Chunk::State state = chunk->GetState();

            // Upload mesh if ready
            if (state == Chunk::State::MeshReady)
            {
                chunk->UploadMeshToGPU();
                chunk->SetState(Chunk::State::Done);
            }

            // Collect chunks in range
            if (state == Chunk::State::Done && dist2 <= renderDistance * renderDistance)
            {
                opaqueChunks.push_back(chunk); // opaque and transparent will be separated in Draw()
            }

            // Unload far-away chunks
            if (chunk->m_ShouldUnload)
            {
                chunk->DeleteGPUData();
                m_RequestedChunks.erase(it->first);
                it = m_Chunks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Separate opaque and transparent
    for (auto &chunk : opaqueChunks)
    {
        if (!chunk->m_Mesh.Transparent.Indices.empty())
            transparentChunks.push_back(chunk);
    }

    Camera::Frustum frustum = camera.GetFrustum();

    // Draw to the shadow map first
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, -1.0f, 0.5f));
    float orthoSize = 160.0f;
    float nearPlane = 0.1f;
    float farPlane = 300.0f;

    // Position the "sun camera" so the player is centered
    glm::vec3 lightPos = m_PlayerPosition - lightDir * 160.0f;

    glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(lightPos, m_PlayerPosition, glm::vec3(0, 1, 0));
    glm::mat4 lightSpaceMatrix = lightProj * lightView;

    // Bind shadow shader once
    shadowShader.Bind();
    shadowShader.SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);

    // Draw all opaque chunks into shadow map
    for (auto &chunk : opaqueChunks)
    {
        if (!camera.IsInsideFrustum(frustum, chunk->GetAABB()))
            continue;

        chunk->DrawOpaque(shadowShader);
    }

    for (auto &chunk : transparentChunks)
    {
        if (!camera.IsInsideFrustum(frustum, chunk->GetAABB()))
            continue;

        chunk->DrawTransparent(shadowShader); // Shadow shader discards fully transparent pixels
    }

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Window &window = Window::GetMain();
    glViewport(0, 0, window.GetFrameBufferWidth(), window.GetFrameBufferHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind main shader
    shader.Bind();
    shader.SetUniform("u_CameraPos", m_PlayerPosition);
    shader.SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
    shader.SetUniform("u_ShadowMap", 0); // Texture unit 0

    // Bind shadow map to texture unit 0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    shader.SetUniform("u_ShadowMap", 1);

    // Draw all opaque chunks with lighting + shadows
    for (auto &chunk : opaqueChunks)
    {
        if (!camera.IsInsideFrustum(frustum, chunk->GetAABB()))
            continue;

        chunk->DrawOpaque(shader);
    }

    // Sort transparent chunks back-to-front
    std::sort(transparentChunks.begin(), transparentChunks.end(),
              [&](const std::shared_ptr<Chunk> &a, const std::shared_ptr<Chunk> &b)
              {
                  float da = glm::length(m_PlayerPosition - Vector3(a->GetPosition().x * CHUNK_WIDTH, 0, a->GetPosition().y * CHUNK_WIDTH));
                  float db = glm::length(m_PlayerPosition - Vector3(b->GetPosition().x * CHUNK_WIDTH, 0, b->GetPosition().y * CHUNK_WIDTH));
                  return da > db; // farthest first
              });

    // Draw transparent
    for (auto &chunk : transparentChunks)
    {
        if (!camera.IsInsideFrustum(frustum, chunk->GetAABB()))
            continue;

        chunk->DrawTransparent(shader);
    }
}

std::shared_ptr<Chunk> ChunkManager::GetChunk(int x, int z)
{
    std::scoped_lock lock(m_ChunkMutex);
    auto it = m_Chunks.find(Vector2i(x, z));
    return it != m_Chunks.end() ? it->second : nullptr;
}

const BlockState &ChunkManager::GetBlock(
    int x, int y, int z,
    int chunkX, int chunkZ,
    const BlockData &localData)
{
    // Height check (padded supports y=-1 and y=H)
    if (y < -1 || y > CHUNK_HEIGHT)
        return g_BlockRegistry.Get(BLOCK_AIR);

    // Convert to padded coordinates
    int px = x + 1;
    int py = y + 1;
    int pz = z + 1;

    // Padded bounds:
    // px: 0 .. W+1
    // py: 0 .. H+1
    // pz: 0 .. W+1
    if (px < 0 || px >= BlockData::PW ||
        py < 0 || py >= BlockData::PH ||
        pz < 0 || pz >= BlockData::PW)
    {
        return g_BlockRegistry.Get(BLOCK_AIR);
    }

    // Direct sample from padded chunk
    return localData.Get(px, py, pz);
}

BlockData ChunkManager::GenerateBlocks(int chunkX, int chunkZ)
{
    GenerateTreesForChunk(chunkX, chunkZ);

    BlockData data;

    const int W = CHUNK_WIDTH;
    const int H = CHUNK_HEIGHT;
    const int PW = W + 2;
    const int PH = H + 2;

    const int baseHeight = CHUNK_BASE_HEIGHT;
    const float amplitude = 20.0f;
    const int minDirt = 1;
    const int maxDirt = 3;

    for (int px = 0; px < PW; ++px)
    {
        for (int pz = 0; pz < PW; ++pz)
        {
            int wx = chunkX * W + (px - 1);
            int wz = chunkZ * W + (pz - 1);

            // Smooth height using fractal noise
            float hNoise = m_Noise.GetFractalNoise2D(float(wx), float(wz));
            int terrainHeight = int(baseHeight + hNoise * amplitude);
            terrainHeight = std::clamp(terrainHeight, 0, H - 1);

            // Dirt thickness variation
            int dirtThickness = minDirt + int((m_Noise.GetFractalNoise2D(float(wx + 1000), float(wz + 1000), 2, 0.5f, 0.05f)) * (maxDirt - minDirt));
            dirtThickness = std::clamp(dirtThickness, minDirt, maxDirt);

            for (int py = 0; py < PH; ++py)
            {
                int wy = py - 1;
                uint16_t id = BLOCK_AIR;

                if (wy <= 0)
                {
                    id = BLOCK_BEDROCK;
                }
                else if (wy > terrainHeight)
                {
                    id = BLOCK_AIR;
                }
                else if (wy == terrainHeight)
                {
                    id = BLOCK_GRASS;
                }
                else if (wy >= terrainHeight - dirtThickness)
                {
                    id = BLOCK_DIRT;
                }
                else
                {
                    id = BLOCK_STONE;
                }

                data.SetID(px, py, pz, id);
            }
        }
    }

    return data;
}

MeshData ChunkManager::GenerateMesh(const BlockData &blockData, int chunkX, int chunkZ)
{
    MeshData data;

    // For a 16x256x16 chunk (65536 blocks)
    const size_t MAX_VERTICES = 1572864; // 65536 * 24
    const size_t MAX_INDICES = 2359296;  // 65536 * 36

    data.Opaque.Vertices.reserve(MAX_VERTICES / 2);
    data.Opaque.Indices.reserve(MAX_INDICES / 2);

    data.Transparent.Vertices.reserve(MAX_VERTICES / 2);
    data.Transparent.Indices.reserve(MAX_INDICES / 2);

    const float eps = 0.001f;

    struct FaceIndices
    {
        int v0, v1, v2, v3;
        Vector3i normal;
    };

    static const std::unordered_map<std::string, FaceIndices> faceMap = {
        {"up", {2, 6, 7, 3, Vector3i(0, 1, 0)}},
        {"down", {1, 5, 4, 0, Vector3i(0, -1, 0)}},  // **CORRECTED** from {0, 1, 5, 4, ...}
        {"north", {1, 0, 2, 3, Vector3i(-1, 0, 0)}}, // **CORRECTED** from {2, 6, 4, 0, ...}
        {"south", {4, 5, 7, 6, Vector3i(1, 0, 0)}},
        {"east", {5, 1, 3, 7, Vector3i(0, 0, 1)}},
        {"west", {0, 4, 6, 2, Vector3i(0, 0, -1)}}};

    for (int x = 0; x < CHUNK_WIDTH; ++x)
    {
        for (int y = 0; y < CHUNK_HEIGHT; ++y)
        {
            for (int z = 0; z < CHUNK_WIDTH; ++z)
            {
                const BlockState &state = GetBlock(x, y, z, chunkX, chunkZ, blockData);
                if (state.IsAir())
                    continue;

                const bool isTransparent = state.IsTransparent();
                Vector3 blockPos(x, y, z);
                const Model &model = state.GetModel();

                for (const Element &elem : model.Elements)
                {
                    // Not being called...
                    // LOG_INFO("Element: {}, {}, {}", elem.From[0], elem.From[1], elem.From[2]);

                    Vector3f from(elem.From[0] / 16.0f, elem.From[1] / 16.0f, elem.From[2] / 16.0f);
                    Vector3f to(elem.To[0] / 16.0f, elem.To[1] / 16.0f, elem.To[2] / 16.0f);

                    // Compute cube corners
                    Vector3f points[8] = {
                        blockPos + Vector3f(from.x, from.y, from.z),
                        blockPos + Vector3f(from.x, from.y, to.z),
                        blockPos + Vector3f(from.x, to.y, from.z),
                        blockPos + Vector3f(from.x, to.y, to.z),
                        blockPos + Vector3f(to.x, from.y, from.z),
                        blockPos + Vector3f(to.x, from.y, to.z),
                        blockPos + Vector3f(to.x, to.y, from.z),
                        blockPos + Vector3f(to.x, to.y, to.z)};

                    for (const auto &[faceNameRaw, face] : elem.Faces)
                    {
                        // Normalize face names from JSON
                        std::string faceName = faceNameRaw;
                        if (faceName == "top")
                            faceName = "up";
                        else if (faceName == "bottom")
                            faceName = "down";

                        auto fit = faceMap.find(faceName);
                        if (fit == faceMap.end())
                            continue; // Unknown face, skip

                        const FaceIndices &fi = fit->second;

                        int nx = x + fi.normal.x;
                        int ny = y + fi.normal.y;
                        int nz = z + fi.normal.z;

                        const BlockState &adj = GetBlock(nx, ny, nz, chunkX, chunkZ, blockData);

                        // Neighbor culling
                        bool skip = false;
                        if (!isTransparent)
                            skip = (!adj.IsAir() && !adj.IsTransparent());

                        // Always draw top/bottom for transparent blocks
                        if (isTransparent && fi.normal.y != 0)
                            skip = false;

                        if (skip)
                            continue;

                        auto &destMesh = isTransparent ? data.Transparent : data.Opaque;
                        auto &verts = destMesh.Vertices;
                        auto &inds = destMesh.Indices;
                        unsigned int baseIndex = verts.size();

                        auto ao = GetVertexAOs(blockData, Vector3i(x, y, z), fi.normal, Vector2i(chunkX, chunkZ));

                        auto push = [&](const Vector3f &pos, const Vector3i &normal, const Vector2f &uv, float aoVal, int layer)
                        {
                            Ref<TextureArray2D> texture = g_BlockRegistry.GetTexture();
                            if (!texture)
                                return;

                            BlockVertex v;
                            v.Position = pos;
                            v.Normal = normal;
                            v.UV = uv;
                            v.Layer = layer;
                            v.TextureSize = Vector2i(texture->GetWidth(layer),
                                                     texture->GetHeight(layer));

                            v.AO = isTransparent ? 1.0f : aoVal;
                            verts.push_back(v);
                        };

                        // Per-face texture layer
                        std::string texName = face.Texture; // might be "#all"
                        if (!texName.empty() && texName[0] == '#')
                        {
                            std::string key = texName.substr(1);
                            auto it = model.Textures.find(key);
                            if (it != model.Textures.end())
                                texName = it->second;
                            else
                                texName = "missing";
                        }

                        size_t slash = texName.find_last_of('/');
                        if (slash != std::string::npos)
                            texName = texName.substr(slash + 1);

                        int layer = g_BlockRegistry.GetTextureLayer(texName, (state.GetId() * 6) - 1);

                        // Map UVs
                        float u0 = face.UV[0] / 16.0f;
                        float u1 = face.UV[2] / 16.0f;
                        float v0 = 1.0f - face.UV[3] / 16.0f;
                        float v1 = 1.0f - face.UV[1] / 16.0f;

                        Vector2f uv0(u0, v1);
                        Vector2f uv1(u1, v1);
                        Vector2f uv2(u1, v0);
                        Vector2f uv3(u0, v0);

                        push(points[fi.v0], fi.normal, uv0, ao[0], layer);
                        push(points[fi.v1], fi.normal, uv1, ao[1], layer);
                        push(points[fi.v2], fi.normal, uv2, ao[2], layer);
                        push(points[fi.v3], fi.normal, uv3, ao[3], layer);

                        float diag1 = ao[0] + ao[2];
                        float diag2 = ao[1] + ao[3];

                        if (diag1 > diag2)
                        {
                            inds.insert(inds.end(),
                                        {baseIndex, baseIndex + 1, baseIndex + 2,
                                         baseIndex, baseIndex + 2, baseIndex + 3});
                        }
                        else
                        {
                            inds.insert(inds.end(),
                                        {baseIndex + 1, baseIndex + 2, baseIndex + 3,
                                         baseIndex + 1, baseIndex + 3, baseIndex});
                        }
                    }
                }
            }
        }
    }

    return data;
}

int ChunkManager::GetBlockIndex(int x, int y, int z) const
{
    return x + CHUNK_WIDTH * (z + CHUNK_WIDTH * y);
}

std::array<float, 4> ChunkManager::GetVertexAOs(const BlockData &localData, const Vector3i &blockPos, const Vector3i &faceNormal, const Vector2i chunkPos)
{
    auto getOcclusion = [](bool s1, bool s2, bool c) -> float
    {
        if (s1 && s2 && c)
            return 0.5f;
        if (s1 && s2)
            return 0.6f;
        if (s1 || s2 || c)
            return 0.6f;
        return 1.0f;
    };

    std::array<float, 4> ao;
    for (int i = 0; i < 4; i++)
    {
        auto neighbors = GetAONeighbors(i, faceNormal);

        const Vector3i &p1 = neighbors[0];
        const Vector3i &p2 = neighbors[1];
        const Vector3i &p3 = neighbors[2];

        bool s1 = GetBlock(blockPos.x + p1.x, blockPos.y + p1.y, blockPos.z + p1.z, chunkPos.x, chunkPos.y, localData).IsSolid();
        bool s2 = GetBlock(blockPos.x + p2.x, blockPos.y + p2.y, blockPos.z + p2.z, chunkPos.x, chunkPos.y, localData).IsSolid();
        bool c = GetBlock(blockPos.x + p3.x, blockPos.y + p3.y, blockPos.z + p3.z, chunkPos.x, chunkPos.y, localData).IsSolid();

        ao[i] = getOcclusion(s1, s2, c);
    }

    if (faceNormal == Vector3i(0, -1, 0))
        std::swap(ao[0], ao[2]);

    return ao;
}

std::array<Vector3i, 3> ChunkManager::GetAONeighbors(int vertexIndex, const Vector3i &face)
{
    Vector3i up(0, 1, 0), down(0, -1, 0);
    Vector3i left(-1, 0, 0), right(1, 0, 0);
    Vector3i front(0, 0, 1), back(0, 0, -1);

    using Triple = std::array<Vector3i, 3>;
    std::array<Triple, 4> neighbors;

    if (face == Vector3i(1, 0, 0))
    {
        neighbors = {Triple{back + right, down + right, back + down + right},
                     Triple{front + right, down + right, front + down + right},
                     Triple{front + right, up + right, front + up + right},
                     Triple{back + right, up + right, back + up + right}};
    }
    else if (face == Vector3i(-1, 0, 0))
    {
        neighbors = {Triple{front + left, down + left, front + down + left},
                     Triple{back + left, down + left, back + down + left},
                     Triple{back + left, up + left, back + up + left},
                     Triple{front + left, up + left, front + up + left}};
    }
    else if (face == Vector3i(0, 1, 0))
    {
        neighbors = {Triple{left + up, back + up, left + back + up},
                     Triple{right + up, back + up, right + back + up},
                     Triple{right + up, front + up, right + front + up},
                     Triple{left + up, front + up, left + front + up}};
    }
    else if (face == Vector3i(0, -1, 0))
    {
        neighbors = {Triple{left + down, front + down, left + front + down},
                     Triple{right + down, front + down, right + front + down},
                     Triple{right + down, back + down, right + back + down},
                     Triple{left + down, back + down, left + back + down}};
    }
    else if (face == Vector3i(0, 0, 1))
    {
        neighbors = {Triple{right + front, down + front, right + down + front},
                     Triple{left + front, down + front, left + down + front},
                     Triple{left + front, up + front, left + up + front},
                     Triple{right + front, up + front, right + up + front}};
    }
    else if (face == Vector3i(0, 0, -1))
    {
        neighbors = {Triple{left + back, down + back, left + down + back},
                     Triple{right + back, down + back, right + down + back},
                     Triple{right + back, up + back, right + up + back},
                     Triple{left + back, up + back, left + up + back}};
    }
    else
    {
        throw std::runtime_error("Invalid face normal");
    }

    return neighbors[vertexIndex];
}

void ChunkManager::GenerateTreesForChunk(int chunkX, int chunkZ)
{
    const int W = CHUNK_WIDTH;
    Vector2i key{chunkX, chunkZ};
    auto &list = m_Trees[key];

    uint32_t seed = static_cast<uint32_t>(int64_t(chunkX) * 73856093 + int64_t(chunkZ) * 19349663);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> distXZ(2.0f, W - 2.0f);

    std::uniform_int_distribution<int> treeCountDist(1, 3);

    // Determine number of trees per chunk using fractal noise
    float noiseVal = m_Noise.GetFractalNoise2D(float(chunkX * 5), float(chunkZ * 5), 3, 0.5f, 0.2f);
    int baseCount = int((noiseVal + 1.0f) * 2.0f);
    int extraCount = treeCountDist(rng);
    int treeCount = baseCount + extraCount;

    for (int i = 0; i < treeCount; ++i)
    {
        int wx = chunkX * W + int(distXZ(rng));
        int wz = chunkZ * W + int(distXZ(rng));

        float hNoise = m_Noise.GetFractalNoise2D(float(wx), float(wz), 4, 0.5f);
        int wy = int(CHUNK_BASE_HEIGHT + hNoise * 20.0f);
        wy = std::clamp(wy, 1, CHUNK_HEIGHT - 2);

        if (!CanPlaceTree(Vector3i(wx, wy, wz), chunkX, chunkZ))
            continue;

        Tree tree;
        tree.BasePos = Vector3i(wx, wy, wz);
        tree.Blocks = GenerateTreeBlocks(tree.BasePos);
        list.push_back(tree);
    }
}

bool ChunkManager::CanPlaceTree(const Vector3i &pos, int chunkX, int chunkZ)
{
    int localX = pos.x - chunkX * CHUNK_WIDTH;
    int localZ = pos.z - chunkZ * CHUNK_WIDTH;

    for (int dx = -1; dx <= 1; dx++)
        for (int dz = -1; dz <= 1; dz++)
        {
            Vector2i key{chunkX + dx, chunkZ + dz};

            auto it = m_Trees.find(key);
            if (it == m_Trees.end())
                continue;

            for (const Tree &other : it->second)
            {
                // compute world-space distance
                float dist = glm::distance(
                    glm::vec2(pos.x, pos.z),
                    glm::vec2(other.BasePos.x, other.BasePos.z));

                if (dist < 3.0f)
                    return false;
            }
        }

    return true;
}

void ChunkManager::PlaceTree(const Tree &tree, BlockData &data, const Vector2i &chunkPos)
{
    const int W = CHUNK_WIDTH;
    const int H = CHUNK_HEIGHT;
    const int PW = W + 2;
    const int PH = H + 2;

    // Iterate all tree blocks
    for (const auto &tb : tree.Blocks)
    {
        // World coordinates of the tree block
        int wx = tree.BasePos.x + tb.RelativePos.x;
        int wy = tree.BasePos.y + tb.RelativePos.y;
        int wz = tree.BasePos.z + tb.RelativePos.z;

        // Determine if this block belongs to this chunk
        int cx = (wx >= 0) ? wx / W : (wx - W + 1) / W;
        int cz = (wz >= 0) ? wz / W : (wz - W + 1) / W;

        if (cx != chunkPos.x || cz != chunkPos.y)
            continue; // Not in this chunk

        // Convert world coords → local padded coords
        int lx = (wx - chunkPos.x * W) + 1;
        int ly = wy + 1; // padded y
        int lz = (wz - chunkPos.y * W) + 1;

        if (lx < 0 || lx >= PW || ly < 0 || ly >= PH || lz < 0 || lz >= PW)
            continue; // Outside padded range

        data.SetID(lx, ly, lz, tb.BlockId);
    }
}

bool ChunkManager::TreeIntersectsChunk(const Tree &tree, const Vector2i &chunkPos) const
{
    const int W = CHUNK_WIDTH;

    // Compute the bounding box of the tree in world coordinates
    int minX = tree.BasePos.x;
    int maxX = tree.BasePos.x;
    int minZ = tree.BasePos.z;
    int maxZ = tree.BasePos.z;

    for (const auto &tb : tree.Blocks)
    {
        int wx = tree.BasePos.x + tb.RelativePos.x;
        int wz = tree.BasePos.z + tb.RelativePos.z;

        if (wx < minX)
            minX = wx;
        if (wx > maxX)
            maxX = wx;
        if (wz < minZ)
            minZ = wz;
        if (wz > maxZ)
            maxZ = wz;
    }

    // Chunk bounds in world coordinates
    int chunkMinX = chunkPos.x * W;
    int chunkMaxX = chunkMinX + W - 1;
    int chunkMinZ = chunkPos.y * W;
    int chunkMaxZ = chunkMinZ + W - 1;

    // Check if the tree bounding box intersects the chunk
    bool intersectsX = (maxX >= chunkMinX) && (minX <= chunkMaxX);
    bool intersectsZ = (maxZ >= chunkMinZ) && (minZ <= chunkMaxZ);

    return intersectsX && intersectsZ;
}

std::vector<TreeBlock> ChunkManager::GenerateTreeBlocks(const Vector3i &base)
{
    std::vector<TreeBlock> blocks;

    const int trunkHeight = 5;
    const int leafStart = trunkHeight - 1;
    const int leafRadius = 2;
    const int leafHeight = 4;

    // Trunk
    for (int y = 0; y < trunkHeight; ++y)
    {
        blocks.push_back({Vector3i(0, y, 0), BLOCK_WOOD_LOG});
    }

    // Leaves (simple cube)
    for (int x = -leafRadius; x <= leafRadius; ++x)
    {
        for (int y = leafStart; y <= leafStart + leafHeight; ++y)
        {
            for (int z = -leafRadius; z <= leafRadius; ++z)
            {
                if (x == 0 && y < trunkHeight && z == 0)
                    continue; // skip trunk space
                blocks.push_back({Vector3i(x, y, z), BLOCK_LEAVES});
            }
        }
    }

    return blocks;
}

void ChunkManager::ApplyStructures(int cx, int cz, BlockData &data)
{
    const Vector2i currentChunk{cx, cz};

    // Check this chunk and the 8 neighbors
    for (int dx = -1; dx <= 1; dx++)
        for (int dz = -1; dz <= 1; dz++)
        {
            Vector2i key{cx + dx, cz + dz};

            auto it = m_Trees.find(key);
            if (it == m_Trees.end())
                continue;

            for (const Tree &tree : it->second)
            {
                if (TreeIntersectsChunk(tree, currentChunk))
                {
                    PlaceTree(tree, data, currentChunk);
                }
            }
        }
}
