#include "Physics/AABB.h"
#include <glm/ext/quaternion_geometric.hpp>

bool AABB::IsValid() const
{
    // TODO: May need to change this to something safer
    return glm::length(Min) != 0.0f && glm::length(Max) != 0.0f;
}
