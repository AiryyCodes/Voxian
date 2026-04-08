#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "World/Entity/Component/Component.h"

struct Transform : public Component
{
    Vector3f Position = Vector3f(0.0f);
    Vector3f Rotation = Vector3f(0.0f);
    Vector3f Scale = Vector3f(1.0f);

    Matrix4 GetMatrix();
    Matrix4 GetMatrix() const;
};
