#pragma once

#include "Math/Vector.h"
#include "Physics/AABB.h"
#include "Util/Memory.h"
#include "Renderer/Renderer.h"
#include "World/Entity/Component/Component.h"

#include <cstddef>
#include <string>
#include <unordered_map>

class Entity
{
public:
    Entity(std::string name)
        : m_Name(std::move(name)) {}

    void Update(float delta);
    void Render(Renderer &renderer);

    template <typename T, typename... Args>
    T &AddComponent(Args &&...args)
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        assert(!HasComponent<T>() && "Entity already has this component!");

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->m_Owner = this;
        component->OnAttach();

        T &ref = *component;
        m_Components[typeid(T).hash_code()] = std::move(component);
        return ref;
    }

    template <typename T>
    T &GetComponent()
    {
        assert(HasComponent<T>() && "Entity does not have this component!");
        return static_cast<T &>(*m_Components.at(typeid(T).hash_code()));
    }

    template <typename T>
    const T &GetComponent() const
    {
        assert(HasComponent<T>() && "Entity does not have this component!");
        return static_cast<T &>(*m_Components.at(typeid(T).hash_code()));
    }

    template <typename T>
    bool HasComponent() const
    {
        return m_Components.contains(typeid(T).hash_code());
    }

    template <typename T>
    void RemoveComponent()
    {
        assert(HasComponent<T>() && "Entity does not have this component!");
        m_Components.at(typeid(T).hash_code())->OnDetach();
        m_Components.erase(typeid(T).hash_code());
    }

    virtual AABB GetAABB() const
    {
        return {Vector3f(0, 0, 0), Vector3f(0, 0, 0)};
    }

    const std::string &GetName() const { return m_Name; }

private:
    std::string m_Name;

    std::unordered_map<size_t, Scope<Component>> m_Components;
};
