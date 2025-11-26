#pragma once

#include "Graphics/Model.h"

#include <string>

class BlockState
{
public:
    BlockState(const std::string &name)
        : m_Name(name)
    {
    }

    const std::string &GetName() const { return m_Name; }
    int GetId() const { return m_Id; }

    bool IsAir() const { return m_IsAir; }
    bool IsSolid() const { return m_IsSolid; }
    bool IsTransparent() const { return m_IsTransparent; }

    BlockState &SetAir(bool value = true)
    {
        m_IsAir = value;
        return *this;
    }
    BlockState &SetSolid(bool value = true)
    {
        m_IsSolid = value;
        return *this;
    }
    BlockState &SetTransparent(bool value = true)
    {
        m_IsTransparent = value;
        return *this;
    }

    void SetIntProperty(const std::string &key, int value)
    {
        m_IntProperties[key] = value;
    }

    bool HasIntProperty(const std::string &key) const
    {
        return m_IntProperties.find(key) != m_IntProperties.end();
    }

    int GetIntProperty(const std::string &key, int defaultValue = 0) const
    {
        auto it = m_IntProperties.find(key);
        return it != m_IntProperties.end() ? it->second : defaultValue;
    }

    void SetStringProperty(const std::string &key, const std::string &value)
    {
        m_StringProperties[key] = value;
    }

    bool HasStringProperty(const std::string &key) const
    {
        return m_StringProperties.find(key) != m_StringProperties.end();
    }

    const std::string &GetStringProperty(const std::string &key,
                                         const std::string &defaultValue = "") const
    {
        auto it = m_StringProperties.find(key);
        return it != m_StringProperties.end() ? it->second : defaultValue;
    }

    int GetFrontLayer() const { return GetBaseOffset() + 0; }
    int GetBackLayer() const { return GetBaseOffset() + 1; }
    int GetLeftLayer() const { return GetBaseOffset() + 2; }
    int GetRightLayer() const { return GetBaseOffset() + 3; }
    int GetTopLayer() const { return GetBaseOffset() + 4; }
    int GetBottomLayer() const { return GetBaseOffset() + 5; }

    const Model &GetModel() const { return m_Model; }
    void SetModel(const Model &model)
    {
        m_Model = std::move(model);
    }

private:
    int GetBaseOffset() const
    {
        return ((m_Id + 1) == 1) ? 0 : ((m_Id + 1) == 2) ? 6
                                                         : ((m_Id + 1) - 1) * 6;
    }

private:
    friend class BlockRegistry;

    std::string m_Name;
    int m_Id;

    bool m_IsAir = false;
    bool m_IsSolid = true;
    bool m_IsTransparent = false;

    Model m_Model;

    std::unordered_map<std::string, int> m_IntProperties;
    std::unordered_map<std::string, std::string> m_StringProperties;
};
