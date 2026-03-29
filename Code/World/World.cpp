#include "World/World.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Player.h"
#include "World/Entity/Triangle.h"

void World::Init()
{
    m_Player = &SpawnEntity<Player>();

    SpawnEntity<Triangle>();
}

void World::Update(float delta)
{
    for (const auto &entity : m_Entities)
    {
        entity->Update(delta);
    }
}

void World::Render(Renderer &renderer)
{
    for (const auto &entity : m_Entities)
    {
        entity->Render(renderer);
    }
}
