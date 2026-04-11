#pragma once

#include "Chunk/ChunkManager.h"
#include "Util/Memory.h"
#include "Renderer/Renderer.h"
#include "World/Block/Block.h"
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

        auto entity = CreateRef<T>(std::forward<Args>(args)...);
        T &ref = *entity;
        m_Entities.push_back(std::move(entity));
        return ref;
    }

    template <typename T>
    void RegisterEntity(Ref<T> entity)
    {
        m_Entities.push_back(std::move(entity));
    }

    void DestroyEntity(Entity *entity);

    const Block *GetBlock(int x, int y, int z) const;

    void SetActiveCamera(Camera *camera) { m_ActiveCamera = camera; }
    Camera *GetActiveCamera() { return m_ActiveCamera; }

    Player *GetPlayer() { return m_Player; }

    ChunkManager &GetChunkManager() { return m_ChunkManager; }

private:
    Player *m_Player = nullptr;
    Camera *m_ActiveCamera = nullptr;

    std::vector<Ref<Entity>> m_Entities;

    ChunkManager m_ChunkManager;
};
