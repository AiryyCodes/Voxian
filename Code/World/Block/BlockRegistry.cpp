#include "BlockRegistry.h"
#include "Block.h"

uint16_t BlockRegistry::RegisterBlock(const std::string &name, Scope<Block> block)
{
    uint16_t id = static_cast<uint16_t>(m_Blocks.size());
    m_Blocks[name] = std::move(block);
    m_IdToName[id] = name;
    return id;
}

const Block *BlockRegistry::GetBlockByName(const std::string &name) const
{
    auto it = m_Blocks.find(name);
    if (it != m_Blocks.end())
        return it->second.get();
    return nullptr; // Block not found
}

const Block *BlockRegistry::GetBlockById(uint16_t id) const
{
    auto it = m_IdToName.find(id);
    if (it != m_IdToName.end())
        return GetBlockByName(it->second);
    return nullptr; // Block not found
}