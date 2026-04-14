#include "Blocks.h"

void Blocks::AssignAll(BlockRegistry &registry)
{
    AIR = registry.GetBlockIndexById("air");
    GRASS_BLOCK = registry.GetBlockIndexById("grass_block");
    DIRT = registry.GetBlockIndexById("dirt");
    DIRT_SLAB = registry.GetBlockIndexById("dirt_slab");
    STONE = registry.GetBlockIndexById("stone");
    LEAVES = registry.GetBlockIndexById("leaves");
}