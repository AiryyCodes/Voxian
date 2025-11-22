#pragma once

#include "Graphics/Texture.h"
#include "Logger.h"
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

    std::unordered_map<std::string, int> m_IntProperties;
    std::unordered_map<std::string, std::string> m_StringProperties;
};

class BlockRegistry
{
public:
    void Init();

    uint16_t Register(BlockState state)
    {
        uint16_t id = m_States.size();

        state.m_Id = id;

        m_States.push_back(state);
        m_NameToId[state.GetName()] = id;

        return id;
    }

    const BlockState &Get(uint16_t id) const { return m_States[id]; }

    Ref<TextureArray2D> GetTexture() const
    {
        return m_Texture;
    }

private:
    std::vector<BlockState> m_States;
    std::unordered_map<std::string, uint16_t> m_NameToId;

    Ref<TextureArray2D> m_Texture;
};

inline BlockRegistry g_BlockRegistry;

inline uint16_t BLOCK_AIR = g_BlockRegistry.Register(
    BlockState("air")
        .SetAir(true)
        .SetSolid(false)
        .SetTransparent(true));

inline uint16_t BLOCK_GRASS = g_BlockRegistry.Register(
    BlockState("grass"));

inline uint16_t BLOCK_DIRT = g_BlockRegistry.Register(
    BlockState("dirt"));

inline uint16_t BLOCK_STONE = g_BlockRegistry.Register(
    BlockState("stone"));

inline uint16_t BLOCK_BEDROCK = g_BlockRegistry.Register(
    BlockState("bedrock"));

inline uint16_t BLOCK_WOOD_LOG = g_BlockRegistry.Register(
    BlockState("wood_log"));

inline uint16_t BLOCK_LEAVES = g_BlockRegistry.Register(
    BlockState("leaves")
        .SetSolid(false)
        .SetTransparent(true));
