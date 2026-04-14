#pragma once

#include "BlockRegistry.h"

#include <cstdint>

namespace Blocks
{
inline uint16_t AIR = 0;
inline uint16_t GRASS_BLOCK = 0;
inline uint16_t DIRT = 0;
inline uint16_t DIRT_SLAB = 0;
inline uint16_t STONE = 0;
inline uint16_t LEAVES = 0;

void AssignAll(BlockRegistry &registry);
} // namespace Blocks