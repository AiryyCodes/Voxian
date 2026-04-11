#pragma once

#include "Renderer/Renderer.h"

class Entity;

class Component
{
public:
    virtual ~Component() = default;

    // called when added to entity
    virtual void OnAttach() {}

    // called when removed from entity
    virtual void OnDetach() {}

    virtual void OnUpdate(float delta) {}
    virtual void OnRender(Renderer &renderer) {}

    Entity &GetOwner() { return *m_Owner; }
    const Entity &GetOwner() const { return *m_Owner; }

    bool IsDead() const { return m_IsDead; }
    void Destroy() { m_IsDead = true; }

private:
    friend class Entity;

    Entity *m_Owner = nullptr;

    bool m_IsDead = false;
};
