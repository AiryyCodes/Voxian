#pragma once

#include "Chunk/ChunkManager.h"
#include "Util/Memory.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Entity.h"
#include "World/Entity/Player.h"
#include "World/Entity/Component/Camera.h"

#include <type_traits>
#include <utility>
#include <vector>

class World
{
public:
    World() : m_ChunkManager(*this) {}

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

    template <typename T>
    void RegisterEntity(T *entity)
    {
        m_Entities.push_back(std::move(Scope<T>(entity)));
    }

    void DestroyEntity(Entity *entity);

    void SetActiveCamera(Camera *camera) { m_ActiveCamera = camera; }
    Camera *GetActiveCamera() { return m_ActiveCamera; }

    Player *GetPlayer() { return m_Player; }

    ChunkManager &GetChunkManager() { return m_ChunkManager; }

private:
    Player *m_Player = nullptr;
    Camera *m_ActiveCamera = nullptr;

    std::vector<Scope<Entity>> m_Entities;

    ChunkManager m_ChunkManager;
};
