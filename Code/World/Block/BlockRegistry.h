#pragma once

#include "Block.h"
#include "BlockData.h"
#include "Util/Memory.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class BlockRegistry
{
public:
    void Init();

    uint16_t RegisterBlock(const std::string &id, Scope<Block> block);

    const Block *GetBlockById(const std::string &id) const;
    const Block *GetBlockByIndex(uint16_t id) const;
    uint16_t GetBlockIndexById(const std::string &id) const;

    bool IsIdRegistered(const std::string &id) const;

    std::vector<std::string> GetAllBlockTextures() const;

private:
    std::unordered_map<std::string, BlockData> m_BlockDataMap;
    std::unordered_map<std::string, Scope<Block>> m_Blocks;
    std::unordered_map<uint16_t, std::string> m_IdToName;
};