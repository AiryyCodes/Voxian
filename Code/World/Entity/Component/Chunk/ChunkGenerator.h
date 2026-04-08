#pragma once

#include "World/Entity/Component/Component.h"

class ChunkGenerator : public Component
{
public:
    void Generate();
};