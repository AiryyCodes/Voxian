#pragma once

#include "World/Block/Blocks.h"

#include <glaze/glaze.hpp>

enum class LayerCondition
{
    AtSurface,
    BelowSurface,
    BelowSeaLevel,
    AtBottom
};

template <>
struct glz::meta<LayerCondition>
{
    using enum LayerCondition;
    static constexpr auto value = glz::enumerate(
        "AtSurface", AtSurface,
        "BelowSurface", BelowSurface,
        "BelowSeaLevel", BelowSeaLevel,
        "AtBottom", AtBottom);
};

struct BlockLayerConfig
{
    std::string Block = "";
    LayerCondition Condition = LayerCondition::AtSurface;
    int MaxDepth = -1;
    bool RequiresBelowSeaLevel = false;
    int Priority = 0;

    int MinHeight = -1; // world-space Y, -1 = disabled
    int MaxHeight = -1;

    uint16_t BlockIndex = Blocks::AIR;

    struct glaze
    {
        using T = BlockLayerConfig;
        static constexpr auto value = glz::object(
            &T::Block,
            &T::Condition,
            &T::MaxDepth,
            &T::RequiresBelowSeaLevel,
            &T::Priority,
            &T::MinHeight,
            &T::MaxHeight);
    };
};