#pragma once

#include "Math/Vector.h"

#include <string>
#include <vector>

class Direction
{
public:
    enum class Value
    {
        Up,
        Down,
        Left,
        Right,
        Forward,
        Backward
    };

    Direction(Value value)
        : m_Value(value) {}

    Direction FromIndex(int index);
    Vector3i ToVector() const;
    static std::string ToString(const Direction &direction);

    Value GetValue() const { return m_Value; }
    static std::vector<Direction> GetAllDirections()
    {
        return s_AllDirections;
    }

    bool operator==(const Direction &other) const
    {
        return m_Value == other.m_Value;
    }

public:
    static const Direction Up;
    static const Direction Down;
    static const Direction Left;
    static const Direction Right;
    static const Direction Forward;
    static const Direction Backward;

private:
    Value m_Value;

    static std::vector<Direction> s_AllDirections;
};

namespace std
{
template <>
struct hash<Direction>
{
    size_t operator()(const Direction &dir) const
    {
        return std::hash<int>()(static_cast<int>(dir.GetValue()));
    }
};
} // namespace std