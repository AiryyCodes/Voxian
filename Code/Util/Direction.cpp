#include "Direction.h"
#include "Math/Vector.h"
#include <string>

const Direction Direction::Up = Direction(Direction::Value::Up);
const Direction Direction::Down = Direction(Direction::Value::Down);
const Direction Direction::Left = Direction(Direction::Value::Left);
const Direction Direction::Right = Direction(Direction::Value::Right);
const Direction Direction::Forward = Direction(Direction::Value::Forward);
const Direction Direction::Backward = Direction(Direction::Value::Backward);

std::vector<Direction> Direction::s_AllDirections = {Up, Down, Left, Right, Forward, Backward};

Direction Direction::FromIndex(int index)
{
    switch (index)
    {
    case 0:
        return Up;
    case 1:
        return Down;
    case 2:
        return Left;
    case 3:
        return Right;
    case 4:
        return Forward;
    case 5:
        return Backward;
    default:
        static_assert("Invalid direction index");
        return Up; // Default case to silence warnings
    }
}

Vector3i Direction::ToVector() const
{
    switch (m_Value)
    {
    case Value::Up:
        return Vector3i(0, 1, 0);
    case Value::Down:
        return Vector3i(0, -1, 0);
    case Value::Left:
        return Vector3i(-1, 0, 0);
    case Value::Right:
        return Vector3i(1, 0, 0);
    case Value::Forward:
        return Vector3i(0, 0, 1);
    case Value::Backward:
        return Vector3i(0, 0, -1);
    default:
        static_assert("Invalid direction value");
        return Vector3i(0, 0, 0); // Default case to silence warnings
    }
}

std::string Direction::ToString(const Direction &direction)
{
    switch (direction.GetValue())
    {
    case Value::Up:
        return "Up";
    case Value::Down:
        return "Down";
    case Value::Left:
        return "Left";
    case Value::Right:
        return "Right";
    case Value::Forward:
        return "Forward";
    case Value::Backward:
        return "Backward";
    default:
        static_assert("Invalid direction value");
        return "Unknown"; // Default case to silence warnings
    }
}