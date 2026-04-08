#include "World/Entity/Component/Transform.h"
#include "Math/Matrix.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

Matrix4 Transform::GetMatrix()
{
    return const_cast<const Transform *>(this)->GetMatrix();
}

Matrix4 Transform::GetMatrix() const
{
    Matrix4 matrix(1.0f);

    matrix = glm::translate(matrix, Position);
    matrix = glm::rotate(matrix, glm::radians(Rotation.x), Vector3f(1.0f, 0.0f, 0.0f));
    matrix = glm::rotate(matrix, glm::radians(Rotation.y), Vector3f(0.0f, 1.0f, 0.0f));
    matrix = glm::rotate(matrix, glm::radians(Rotation.z), Vector3f(0.0f, 0.0f, 1.0f));
    matrix = glm::scale(matrix, Scale);

    return matrix;
}