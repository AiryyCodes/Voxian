#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "World/Entity/Component/Component.h"

struct Transform : public Component
{
    Vector3f Position = Vector3f(0.0f);

    Matrix4 GetMatrix();
};
