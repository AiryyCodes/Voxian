#pragma once

#include "Math/Vector.h"

#include <array>

enum class Direction
{
    Up,
    Down,
    Front,
    Back,
    Left,
    Right,
};

static inline const std::array<Direction, 6> DIRECTIONS = {
    Direction::Right,
    Direction::Left,
    Direction::Up,
    Direction::Down,
    Direction::Front,
    Direction::Back,
};

static inline Vector3f GetDirectionOffset(const Direction &direction)
{
    switch (direction)
    {
    case Direction::Up:
        return Vector3f(0.0f, 1.0f, 0.0f);
    case Direction::Down:
        return Vector3f(0.0f, -1.0f, 0.0f);
    case Direction::Front:
        return Vector3f(0.0f, 0.0f, -1.0f);
    case Direction::Back:
        return Vector3f(0.0f, 0.0f, 1.0f);
    case Direction::Left:
        return Vector3f(-1.0f, 0.0f, 0.0f);
    case Direction::Right:
        return Vector3f(1.0f, 0.0f, 0.0f);
    }

    // Should not happen
    return Vector3f(0.0f, 0.0f, 0.0f);
}
