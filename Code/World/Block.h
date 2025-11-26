#pragma once

#include "World/BlockState.h"
#include "World/BlockRegistry.h"

#include <cstdint>
#include <string>

class Block
{
public:
    Block(const std::string &name)
        : m_Name(name)
    {
    }

    const std::string &GetName() const { return m_Name; }

    const BlockState &GetDefaultState() const { return g_BlockRegistry.Get(m_DefaultStateId); }

private:
    std::string m_Name;
    uint16_t m_DefaultStateId;

    friend class BlockRegistry;
};
