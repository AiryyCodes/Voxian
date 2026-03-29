#include "World/Entity/Component/Transform.h"
#include "Math/Matrix.h"
#include <glm/ext/matrix_transform.hpp>

Matrix4 Transform::GetMatrix()
{
    Matrix4 matrix(1.0f);

    matrix = glm::translate(matrix, Position);

    return matrix;
}
