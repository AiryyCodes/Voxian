#pragma once

#include <glaze/glaze.hpp>

struct BlockProperties
{
    bool IsAir = false;
    bool IsFullCube = true;

    BlockProperties SetAir(bool isAir)
    {
        IsAir = isAir;
        return *this;
    }

    bool Validate() const;

    struct glaze
    {
        using T = BlockProperties;
        static constexpr auto value = glz::object(
            &T::IsAir,
            &T::IsFullCube);
    };
};