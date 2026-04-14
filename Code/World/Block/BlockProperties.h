#pragma once

#include <glaze/glaze.hpp>

enum class TransparencyType
{
    Opaque,     // Stone, Dirt, Wood (Writes to depth, No alpha)
    Cutout,     // Leaves, Tall Grass, Saplings (Writes to depth, Alpha discard)
    Transparent // Glass, Water (Reads depth, No depth write, Alpha blend)
};

template <>
struct glz::meta<TransparencyType>
{
    using enum TransparencyType;
    static constexpr auto value = glz::enumerate(
        "Opaque", Opaque,
        "Cutout", Cutout,
        "Transparent", Transparent);
};

struct BlockProperties
{
    bool IsAir = false;
    bool IsFullCube = true;
    bool IsSolid = true;
    TransparencyType Transparency = TransparencyType::Opaque;

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
            &T::IsFullCube,
            &T::IsSolid,
            &T::Transparency);
    };
};