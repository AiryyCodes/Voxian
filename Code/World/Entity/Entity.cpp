#include "World/Entity/Entity.h"
#include "Physics/AABB.h"
#include "Renderer/Renderer.h"

void Entity::Update(float delta)
{
    for (const auto &[id, component] : m_Components)
    {
        component->OnUpdate(delta);
    }
}

void Entity::Render(Renderer &renderer)
{
    for (const auto &[id, component] : m_Components)
    {
        component->OnRender(renderer);
    }
}
