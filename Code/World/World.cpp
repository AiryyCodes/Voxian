#include "World/World.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Player.h"

void World::Init()
{
    m_Player = &SpawnEntity<Player>();
    SetActiveCamera(&m_Player->GetComponent<Camera>());

    m_ChunkManager.Init();
}

void World::Update(float delta)
{
    m_ChunkManager.Update(delta);

    for (const auto &entity : m_Entities)
    {
        entity->Update(delta);
    }
}

void World::Render(Renderer &renderer)
{
    renderer.SetCamera(m_ActiveCamera);

    m_ChunkManager.Render(renderer);

    for (const auto &entity : m_Entities)
    {
        entity->Render(renderer);
    }
}

void World::DestroyEntity(Entity *entity)
{
    auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
                           [&](const auto &e)
                           { return e.get() == entity; });

    if (it != m_Entities.end())
    {
        *it = std::move(m_Entities.back());
        m_Entities.pop_back();
    }
}

const Block *World::GetBlock(int x, int y, int z) const
{
    return m_ChunkManager.GetBlock(x, y, z);
}
