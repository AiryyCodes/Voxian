#pragma once

#include <glaze/glaze.hpp>

struct CurvePoint
{
    float In = 0.0f;
    float Out = 0.0f;

    struct glaze
    {
        using T = CurvePoint;
        static constexpr auto value = glz::object(&T::In, &T::Out);
    };
};