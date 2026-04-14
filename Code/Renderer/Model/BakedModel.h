#include "Math/Vector.h"

#include <glaze/glaze.hpp>
#include <array>
#include <string>

struct BakedFace
{
    std::array<Vector3f, 4> Positions;
    Vector4f UVs; // x=uMin, y=vMin, z=uMax, w=vMax
    Vector3f RotatedNormal;
    int NormalIndex;
    int TextureLayer;
};

struct BakedElement
{
    std::string Name;
    std::vector<BakedFace> Faces;
    bool NoAmbientOcclusion;
};

struct BakedModel
{
    std::string Name;
    std::vector<BakedElement> Elements;
};

template <>
struct glz::meta<BakedFace>
{
    using T = BakedFace;
    static constexpr auto value = glz::object(
        "Positions", &T::Positions,
        "UVs", &T::UVs,
        "NormalIndex", &T::NormalIndex,
        "TextureLayer", &T::TextureLayer);
};

template <>
struct glz::meta<BakedElement>
{
    using T = BakedElement;
    static constexpr auto value = glz::object(
        "Name", &T::Name,
        "Faces", &T::Faces);
};

template <>
struct glz::meta<BakedModel>
{
    using T = BakedModel;
    static constexpr auto value = glz::object(
        "Name", &T::Name,
        "Elements", &T::Elements);
};