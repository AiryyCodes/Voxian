#pragma once

#include "Memory.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class BlockProperty
{
public:
    virtual ~BlockProperty() = default;
    virtual std::string ToString() const = 0;
};

class Block
{
public:
    Block(const std::string &name) : m_Name(name) {}

    const std::string &GetName() const { return m_Name; }

private:
    std::string m_Name;
};

class BlockState
{
public:
    BlockState(const std::string &name)
        : m_Name(name)
    {
    }

    // --- Basic data getters ---
    const std::string &GetName() const { return m_Name; }

    bool IsAir() const { return m_IsAir; }
    bool IsSolid() const { return m_IsSolid; }
    bool IsTransparent() const { return m_IsTransparent; }

    // --- Basic data setters (used during registration only) ---
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

    // --- Integer properties ---
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

    // --- String properties ---
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

private:
    std::string m_Name;

    // - Flags describing block behavior -
    bool m_IsAir = false;
    bool m_IsSolid = true;
    bool m_IsTransparent = false;

    // - Moddable per-block properties -
    std::unordered_map<std::string, int> m_IntProperties;
    std::unordered_map<std::string, std::string> m_StringProperties;
};

class BlockRegistry
{
public:
    uint16_t Register(const BlockState &state)
    {
        uint16_t id = m_States.size();
        m_States.push_back(state);
        NameToID[state.GetName()] = id;
        return id;
    }

    const BlockState &Get(uint16_t id) const { return m_States[id]; }

private:
    std::vector<BlockState> m_States;
    std::unordered_map<std::string, uint16_t> NameToID;
};

inline BlockRegistry g_BlockRegistry;

inline uint16_t BLOCK_AIR = g_BlockRegistry.Register(
    BlockState("air")
        .SetAir(true)
        .SetSolid(false)
        .SetTransparent(true));

inline uint16_t BLOCK_STONE = g_BlockRegistry.Register(
    BlockState("stone"));

inline uint16_t BLOCK_DIRT = g_BlockRegistry.Register(
    BlockState("dirt"));
