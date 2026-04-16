#pragma once

#include "Block.h"
#include "BlockData.h"
#include "Util/Memory.h"
#include "Renderer/Model/ModelRegistry.h"
#include "World/Block/BlockProperties.h"

#include <cstdint>
#include <string>
#include <unordered_map>

struct BlockRenderData
{
    const BakedModel *Model;
    uint16_t DefaultTextureLayer;
    uint8_t RenderFlags;

    BlockProperties Properties;
};

class BlockRegistry
{
public:
    void Init();

    uint16_t RegisterBlock(const std::string &id, Scope<Block> block);

    const Block *GetBlockById(const std::string &id) const;
    const Block *GetBlockByIndex(uint16_t id) const;
    uint16_t GetBlockIndexById(const std::string &id) const;

    const BlockRenderData &GetRenderData(uint16_t index) const { return m_RenderData[index]; }

    int GetNumBlocks() const { return m_Blocks.size(); }

    bool IsIdRegistered(const std::string &id) const;

    Ref<TextureArray2D> GetTextureArray() const { return m_TextureRegistry.GetArray(); }
    int GetTextureLayer(uint16_t blockIndex, const Direction &direction) const;

private:
    std::map<std::string, BlockData> m_BlockDataMap;
    std::unordered_map<std::string, Scope<Block>> m_Blocks;
    std::unordered_map<uint16_t, std::string> m_IdToName;

    std::vector<BlockRenderData> m_RenderData;

    TextureRegistry m_TextureRegistry;
    ModelRegistry m_ModelRegistry;
};
