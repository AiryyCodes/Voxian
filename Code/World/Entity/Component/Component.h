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

private:
    friend class Entity;

    Entity *m_Owner = nullptr;
};
