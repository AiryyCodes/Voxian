#pragma once

#include "BlockRegistry.h"
#include <cstdint>

namespace Blocks
{
inline uint16_t AIR = 0;
inline uint16_t GRASS_BLOCK = 0;

void RegisterAll(BlockRegistry &registry);
} // namespace Blocks