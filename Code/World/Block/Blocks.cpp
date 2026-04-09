#include "Blocks.h"

void Blocks::AssignAll(BlockRegistry &registry)
{
    AIR = registry.GetBlockIndexById("air");
    GRASS_BLOCK = registry.GetBlockIndexById("grass_block");
}