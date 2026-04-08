#include "Direction.h"
#include "Math/Vector.h"

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

Vector3f Direction::ToVector() const
{
    switch (m_Value)
    {
    case Value::Up:
        return Vector3f(0.0f, 1.0f, 0.0f);
    case Value::Down:
        return Vector3f(0.0f, -1.0f, 0.0f);
    case Value::Left:
        return Vector3f(-1.0f, 0.0f, 0.0f);
    case Value::Right:
        return Vector3f(1.0f, 0.0f, 0.0f);
    case Value::Forward:
        return Vector3f(0.0f, 0.0f, 1.0f);
    case Value::Backward:
        return Vector3f(0.0f, 0.0f, -1.0f);
    default:
        static_assert("Invalid direction value");
        return Vector3f(0.0f); // Default case to silence warnings
    }
}