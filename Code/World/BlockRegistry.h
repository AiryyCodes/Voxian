#pragma once

#include "Graphics/Texture.h"
#include "Memory.h"
#include "World/BlockState.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

class Block;
class BlockRegistry
{
public:
    void Init();

    uint16_t Register(BlockState state);

    const BlockState &Get(uint16_t id) const { return m_States[id]; }

    const std::unordered_map<std::string, Ref<Block>> &GetBlocks() const { return m_Blocks; }
    const std::unordered_map<std::string, Model> &GetModels() const { return m_Models; }

    Ref<TextureArray2D> GetTexture() const { return m_Texture; }
    int GetTextureLayer(std::string_view texName, int fallback);

private:
    void LoadBlockStateJSON(BlockState &state);
    Model LoadModelJSON(const std::string &modelName);

private:
    std::vector<BlockState> m_States;
    std::unordered_map<std::string, Ref<Block>> m_Blocks;
    std::unordered_map<std::string, Model> m_Models;
    std::unordered_map<std::string, uint16_t> m_NameToId;
    std::unordered_map<std::string, int> m_TextureNameToLayer;

    Ref<TextureArray2D> m_Texture;
};

inline BlockRegistry g_BlockRegistry;

inline uint16_t BLOCK_AIR = g_BlockRegistry.Register(
    BlockState("air")
        .SetAir(true)
        .SetSolid(false)
        .SetTransparent(true));

inline uint16_t BLOCK_GRASS = g_BlockRegistry.Register(
    BlockState("grass_block"));

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
