#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glaze/glaze.hpp>

using Vector2 = glm::vec2;
using Vector2f = glm::fvec2;
using Vector2i = glm::ivec2;

using Vector3 = glm::vec3;
using Vector3f = glm::fvec3;
using Vector3i = glm::ivec3;

using Vector4 = glm::vec4;
using Vector4f = glm::fvec4;
using Vector4i = glm::ivec4;

template <>
struct glz::meta<Vector2f>
{
    static constexpr auto value = glz::array(&Vector2f::x, &Vector2f::y);
};

template <>
struct glz::meta<Vector3f>
{
    static constexpr auto value = glz::array(&Vector3f::x, &Vector3f::y, &Vector3f::z);
};

template <>
struct glz::meta<Vector3i>
{
    static constexpr auto value = glz::array(&Vector3i::x, &Vector3i::y, &Vector3i::z);
};

template <>
struct glz::meta<Vector4f>
{
    static constexpr auto value = glz::array(&Vector4f::x, &Vector4f::y, &Vector4f::z, &Vector4f::w);
};

template <>
struct glz::meta<Vector4i>
{
    static constexpr auto value = glz::array(&Vector4i::x, &Vector4i::y, &Vector4i::z, &Vector4i::w);
};