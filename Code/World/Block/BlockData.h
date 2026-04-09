#pragma once

#include "BlockProperties.h"

#include <string>
#include <glaze/glaze.hpp>

struct BlockData
{
    std::string Name;
    std::string Id;

    std::string Texture = "null";

    BlockProperties Properties;

    void ResolveTextures();
    bool Validate() const;

    struct glaze
    {
        using T = BlockData;
        static constexpr auto value = glz::object(
            "Name", &T::Name,
            "Id", &T::Id,
            "Texture", &T::Texture,
            "Properties", &T::Properties);
    };
};