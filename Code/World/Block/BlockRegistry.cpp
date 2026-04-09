#include "BlockRegistry.h"
#include "BlockData.h"
#include "BlockProperties.h"
#include "Logger.h"
#include "Memory.h"
#include "World/Block/Block.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glaze/core/opts.hpp>
#include <glaze/core/read.hpp>
#include <glaze/json/read.hpp>
#include <string>

void BlockRegistry::Init()
{
    // Manually register air block to ensure it always has ID 0
    RegisterBlock("air", CreateScope<Block>(BlockProperties().SetAir(true)));

    // Read all blocks from JSON file and register them
    // Iterate through Blocks folder
    for (const auto &entry : std::filesystem::directory_iterator("Assets/Blocks"))
    {
        if (entry.path().filename() == "air.json")
            continue; // Skip air block since it's already registered

        if (entry.path().extension() == ".json")
        {
            std::ifstream file(entry.path());
            if (!file.is_open())
            {
                LOG_ERROR("Failed to open file: {}", entry.path().string());
                continue;
            }

            std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            auto blockData = glz::read_json<BlockData>(jsonContent);
            if (!blockData)
            {
                LOG_ERROR("Failed to parse block data from file: {}", entry.path().string());
                continue;
            }

            if (!blockData->Validate())
            {
                LOG_ERROR("Validation failed for block data in file: {}", entry.path().string());
                continue;
            }

            if (IsIdRegistered(blockData->Id))
            {
                LOG_ERROR("Duplicate block ID '{}' found in file: {}. Skipping registration.", blockData->Id, entry.path().string());
                continue;
            }

            RegisterBlock(blockData->Id, CreateScope<Block>(blockData->Properties));
        }
    }
}

uint16_t BlockRegistry::RegisterBlock(const std::string &id, Scope<Block> block)
{
    uint16_t index = static_cast<uint16_t>(m_Blocks.size());
    m_Blocks[id] = std::move(block);
    m_IdToName[index] = id;
    return index;
}

const Block *BlockRegistry::GetBlockById(const std::string &id) const
{
    auto it = m_Blocks.find(id);
    if (it != m_Blocks.end())
        return it->second.get();
    return nullptr; // Block not found
}

const Block *BlockRegistry::GetBlockByIndex(uint16_t id) const
{
    auto it = m_IdToName.find(id);
    if (it != m_IdToName.end())
        return GetBlockById(it->second);
    return nullptr; // Block not found
}

uint16_t BlockRegistry::GetBlockIndexById(const std::string &id) const
{
    // Convert Id to lowercase for case-insensitive lookup
    std::string lowerId = id;
    std::transform(lowerId.begin(), lowerId.end(), lowerId.begin(), ::tolower);

    auto it = m_Blocks.find(lowerId);
    if (it != m_Blocks.end())
    {
        // Find the index of the block by its name
        auto indexIt = std::find_if(m_IdToName.begin(), m_IdToName.end(),
                                    [&lowerId](const auto &pair)
                                    { return pair.second == lowerId; });
        if (indexIt != m_IdToName.end())
            return indexIt->first;
    }
    return 0; // Default to air block index if not found
}

bool BlockRegistry::IsIdRegistered(const std::string &id) const
{
    return m_Blocks.contains(id);
}