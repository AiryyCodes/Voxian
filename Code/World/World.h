#pragma once

#include "Memory.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Entity.h"

#include <type_traits>
#include <utility>
#include <vector>

class World
{
public:
    void Init();
    void Update(float delta);
    void Render(Renderer &renderer);

    template <typename T, typename... Args>
    T &SpawnEntity(Args &&...args)
    {
        static_assert(std::is_base_of_v<Entity, T>, "T must derive from Entity");

        auto entity = CreateScope<T>(std::forward<Args>(args)...);
        T &ref = *entity;
        m_Entities.push_back(std::move(entity));
        return ref;
    }

private:
    std::vector<Scope<Entity>> m_Entities;
};
