#include "Blocks.h"
#include "Block.h"
#include "Memory.h"

void Blocks::RegisterAll(BlockRegistry &registry)
{
    AIR = registry.RegisterBlock("Air", CreateScope<Block>(Properties().SetAir(true)));
    GRASS_BLOCK = registry.RegisterBlock("Grass Block", CreateScope<Block>(Properties()));
}