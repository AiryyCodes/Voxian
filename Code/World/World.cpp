#include "World/World.h"
#include "Logger.h"
#include "Renderer/Renderer.h"
#include "Util/Memory.h"
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

    for (const auto &entity : m_Entities)
    {
        entity->Render(renderer);
    }
}

void World::DestroyEntity(Entity *entity)
{
    auto it = std::remove_if(m_Entities.begin(), m_Entities.end(),
                             [entity](const Scope<Entity> &e)
                             { return e.get() == entity; });
    if (it != m_Entities.end())
    {
        m_Entities.erase(it, m_Entities.end());
    }
    else
    {
        LOG_WARNING("Attempted to destroy entity that doesn't exist in the world!");
    }
}