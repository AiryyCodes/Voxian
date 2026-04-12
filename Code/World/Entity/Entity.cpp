#include "World/Entity/Entity.h"
#include "Renderer/Renderer.h"

void Entity::Update(float delta)
{
    for (const auto &[id, component] : m_Components)
    {
        component->OnUpdate(delta);
    }

    std::erase_if(m_Components, [](const auto &item)
                  { return item.second->IsDead(); });
}

void Entity::Render(Renderer &renderer)
{
    for (const auto &[id, component] : m_Components)
    {
        component->OnRender(renderer);
    }
}
