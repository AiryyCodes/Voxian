#include "BlockRegistry.h"
#include "BlockData.h"
#include "BlockProperties.h"
#include "Logger.h"
#include "Util/Memory.h"
#include "World/Block/Block.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glaze/core/opts.hpp>
#include <glaze/core/read.hpp>
#include <glaze/json/read.hpp>
#include <string>
#include <vector>

void BlockRegistry::Init()
{
    RegisterBlock("air", CreateScope<Block>(BlockProperties().SetAir(true)));

    std::vector<std::filesystem::path> paths;
    for (const auto &entry : std::filesystem::directory_iterator("Assets/Blocks"))
        if (entry.path().extension() == ".json" && entry.path().filename() != "air.json")
            paths.push_back(entry.path());
    std::sort(paths.begin(), paths.end());

    // Collect all textures and bake models
    for (const auto &path : paths)
    {
        LOG_INFO("Loading block data: {}", path.string());

        std::ifstream file(path);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open file: {}", path.string());
            continue;
        }

        std::string jsonContent((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());

        auto blockData = glz::read_json<BlockData>(jsonContent);
        if (!blockData)
        {
            auto err = glz::format_error(blockData.error(), jsonContent);
            LOG_ERROR("Failed to parse block data: {} - {}", path.string(), err);
            continue;
        }
        if (!blockData->Validate())
        {
            LOG_ERROR("Validation failed: {}", path.string());
            continue;
        }

        blockData->ResolveTextures();

        // Parse model
        const std::string modelPath = "Assets/Models/" + blockData->Model + ".json";
        std::ifstream modelFile(modelPath);
        if (!modelFile.is_open())
        {
            LOG_ERROR("Failed to open model: {}", modelPath);
            continue;
        }

        std::string modelJson((std::istreambuf_iterator<char>(modelFile)),
                              std::istreambuf_iterator<char>());
        Model model;
        auto modelErr = glz::read_json(model, modelJson);
        if (modelErr)
        {
            auto err = glz::format_error(modelErr, modelJson);
            LOG_ERROR("Failed to parse model: {} - {}", modelPath, err);
            continue;
        }

        // Resolve texture variables and register textures
        auto vars = blockData->ResolveTextureVariables(model);
        m_ModelRegistry.Load(blockData->Id, model, vars, m_TextureRegistry);

        uint16_t index = RegisterBlock(blockData->Id, CreateScope<Block>(blockData->Properties));
        m_BlockDataMap[blockData->Id] = *blockData;

        const std::string &id = blockData->Id;

        const BakedModel *bakedModel = m_ModelRegistry.Get(id);

        BlockRenderData render;
        render.Model = bakedModel;
        render.DefaultTextureLayer = 0;
        render.Properties = blockData->Properties;

        m_RenderData.resize(m_Blocks.size());
        m_RenderData[index] = render;
    }

    if (m_TextureRegistry.GetLayerCount() == 0)
    {
        LOG_ERROR("No textures registered - skipping texture array build");
        return;
    }

    // Build texture array once — all layers finalized
    m_TextureRegistry.Build(16, 16);

    LOG_INFO("Registered {} blocks, {} textures",
             m_BlockDataMap.size(),
             m_TextureRegistry.GetLayerCount());
}

uint16_t BlockRegistry::RegisterBlock(const std::string &id, Scope<Block> block)
{
    if (IsIdRegistered(id))
    {
        LOG_ERROR("Block ID '{}' is already registered. Skipping registration.", id);
        return GetBlockIndexById(id);
    }

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

int BlockRegistry::GetTextureLayer(uint16_t blockIndex, const Direction &direction) const
{
    auto nameIt = m_IdToName.find(blockIndex);
    if (nameIt == m_IdToName.end())
        return 0;

    const BakedModel *model = m_ModelRegistry.Get(nameIt->second);
    if (!model || model->Elements.empty())
        return 0;

    // Find the face matching this direction
    const std::string faceName = Direction::ToString(direction);
    for (const auto &elem : model->Elements)
        for (const auto &face : elem.Faces)
            if (face.NormalIndex == s_NormalIndexMap.at(direction))
                return face.TextureLayer;

    return 0;
}