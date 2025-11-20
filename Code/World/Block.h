#pragma once

#include "Memory.h"

#include <string>
#include <unordered_map>

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
    BlockState(Ref<Block> block) : m_Block(block) {}

    void SetProperty(const std::string &name, Ref<BlockProperty> prop)
    {
        m_Properties[name] = prop;
    }

    Ref<BlockProperty> GetProperty(const std::string &name) const
    {
        auto it = m_Properties.find(name);
        return it != m_Properties.end() ? it->second : nullptr;
    }

    std::string ToString() const
    {
        std::string result = m_Block->GetName();
        for (auto &[k, v] : m_Properties)
            result += " " + k + "=" + v->ToString();
        return result;
    }

private:
    Ref<Block> m_Block;
    std::unordered_map<std::string, Ref<BlockProperty>> m_Properties;
};

inline Ref<BlockState> BLOCK_AIR = nullptr;
inline Ref<BlockState> BLOCK_STONE = CreateRef<BlockState>(CreateRef<Block>("stone"));
inline Ref<BlockState> BLOCK_DIRT = CreateRef<BlockState>(CreateRef<Block>("dirt"));
