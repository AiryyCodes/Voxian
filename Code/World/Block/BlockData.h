#pragma once

#include "BlockProperties.h"

#include <string>
#include <glaze/glaze.hpp>

struct BlockData
{
    std::string Name;
    std::string Id;

    BlockProperties Properties;

    bool Validate() const;

    struct glaze
    {
        using T = BlockData;
        static constexpr auto value = glz::object(
            "Name", &T::Name,
            "Id", &T::Id,
            "Properties", &T::Properties);
    };
};