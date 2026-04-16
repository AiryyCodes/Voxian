#pragma once

#include "Math/Vector.h"
#include "Util/Direction.h"

#include <glaze/core/common.hpp>
#include <glaze/glaze.hpp>
#include <string>

struct Face
{
    Vector4i UV;
    std::string Texture;
    Direction::Value CullFace;
};

struct FaceIndices
{
    int v0, v1, v2, v3;
    Vector2f UVs[4];
};

enum class RotationAxis
{
    X,
    Y,
    Z
};

struct ElementRotation
{
    Vector3f Origin = Vector3f(8, 8, 8);
    RotationAxis Axis = RotationAxis::Y;
    float Angle = 0.0f;
};

struct Element
{
    std::string Name;
    Vector3f From;
    Vector3f To;
    bool NoAmbientOcclusion = false;
    std::vector<ElementRotation> Rotations;
    std::unordered_map<std::string, Face> Faces;
};

struct Model
{
    std::vector<Element> elements;
};

template <>
struct glz::meta<Face>
{
    using T = Face;
    static constexpr auto value = glz::object(
        "UV", &T::UV,
        "Texture", &T::Texture,
        "CullFace", &T::CullFace);
};

template <>
struct glz::meta<RotationAxis>
{
    using enum RotationAxis;
    static constexpr auto value = glz::enumerate(
        "X", X,
        "Y", Y,
        "Z", Z);
};

template <>
struct glz::meta<ElementRotation>
{
    using T = ElementRotation;
    static constexpr auto value = glz::object(
        "Origin", &T::Origin,
        "Axis", &T::Axis,
        "Angle", &T::Angle);
};

template <>
struct glz::meta<Element>
{
    using T = Element;
    static constexpr auto value = glz::object(
        "Name", &T::Name,
        "From", &T::From,
        "To", &T::To,
        "Rotations", &T::Rotations,
        "Faces", &T::Faces,
        "NoAmbientOcclusion", &T::NoAmbientOcclusion);
};

template <>
struct glz::meta<Model>
{
    using T = Model;
    static constexpr auto value = glz::object(
        "Elements", &T::elements);
};

static inline const std::unordered_map<Direction, FaceIndices> s_FaceIndicesMap = {
    {Direction::Up, {2, 6, 7, 3, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
    {Direction::Down, {1, 5, 4, 0, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
    {Direction::Left, {1, 0, 2, 3, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
    {Direction::Right, {4, 5, 7, 6, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
    {Direction::Forward, {6, 2, 0, 4, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
    {Direction::Backward, {3, 7, 5, 1, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}}},
};

static inline const std::unordered_map<Direction, int> s_NormalIndexMap = {
    {Direction::Right, 0},
    {Direction::Left, 1},
    {Direction::Up, 2},
    {Direction::Down, 3},
    {Direction::Forward, 4},
    {Direction::Backward, 5},
};

static inline const Vector3i s_Corners[8] = {
    {0, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 1},
    {1, 0, 0},
    {1, 0, 1},
    {1, 1, 0},
    {1, 1, 1},
};