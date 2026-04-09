#pragma once

#include "Block.h"
#include "Memory.h"

#include <cstdint>
#include <string>
#include <unordered_map>

class BlockRegistry
{
public:
    uint16_t RegisterBlock(const std::string &name, Scope<Block> block);

    const Block *GetBlockByName(const std::string &name) const;
    const Block *GetBlockById(uint16_t id) const;

private:
    std::unordered_map<std::string, Scope<Block>> m_Blocks;
    std::unordered_map<uint16_t, std::string> m_IdToName;
};