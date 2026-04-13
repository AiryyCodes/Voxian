#pragma once

#include "Math/Vector.h"
#include "Util/Direction.h"

#include <glaze/glaze.hpp>
#include <string>

struct Face
{
    Vector4i UV;
    std::string Texture;
};

struct FaceIndices
{
    int v0, v1, v2, v3;
    Vector2f UVs[4];
};

struct Element
{
    std::string Name;
    Vector3i From;
    Vector3i To;
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
        "Texture", &T::Texture);
};

template <>
struct glz::meta<Element>
{
    using T = Element;
    static constexpr auto value = glz::object(
        "Name", &T::Name,
        "From", &T::From,
        "To", &T::To,
        "Faces", &T::Faces);
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
