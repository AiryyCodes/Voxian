#pragma once

#include "Block.h"
#include "BlockData.h"
#include "Util/Memory.h"
#include "Renderer/Model/ModelRegistry.h"

#include <cstdint>
#include <string>
#include <unordered_map>

class BlockRegistry
{
public:
    void Init();

    uint16_t RegisterBlock(const std::string &id, Scope<Block> block);

    const Block *GetBlockById(const std::string &id) const;
    const Block *GetBlockByIndex(uint16_t id) const;
    uint16_t GetBlockIndexById(const std::string &id) const;
    const std::string &GetBlockNameByIndex(uint16_t index) const;

    int GetNumBlocks() const { return m_Blocks.size(); }

    bool IsIdRegistered(const std::string &id) const;

    const BakedModel *GetBakedModel(const std::string &id) const;
    Ref<TextureArray2D> GetTextureArray() const { return m_TextureRegistry.GetArray(); }
    int GetTextureLayer(uint16_t blockIndex, const Direction &direction) const;

private:
    std::map<std::string, BlockData> m_BlockDataMap;
    std::unordered_map<std::string, Scope<Block>> m_Blocks;
    std::unordered_map<uint16_t, std::string> m_IdToName;

    TextureRegistry m_TextureRegistry;
    ModelRegistry m_ModelRegistry;
};
