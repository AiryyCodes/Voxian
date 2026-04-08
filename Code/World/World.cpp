#include "World/World.h"
#include "Entity/Chunk.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Player.h"
#include "World/Entity/Triangle.h"

void World::Init()
{
    m_Player = &SpawnEntity<Player>();
    SetActiveCamera(&m_Player->GetComponent<Camera>());

    // SpawnEntity<Triangle>();

    SpawnEntity<Chunk>(Vector2i(0, 0));
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
    renderer.SetCamera(m_ActiveCamera);

    for (const auto &entity : m_Entities)
    {
        entity->Render(renderer);
    }
}
