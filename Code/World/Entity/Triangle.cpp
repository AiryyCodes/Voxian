#include "World/Entity/Triangle.h"
#include "Math/Vector.h"
#include "Renderer/ShaderLibrary.h"
#include "World/Entity/Component/MeshRenderer.h"
#include "World/Entity/Component/Transform.h"

static const float TRIANGLE_VERTICES[] = {
    -0.5,
    -0.5f,
    0.0f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    0.5f,
    0.0f,
};

Triangle::Triangle()
    : Entity("Triangle")
{
    auto &transform = AddComponent<Transform>();
    transform.Position = Vector3f(0.0f, 0.5f, 0.0f);

    auto &meshRenderer = AddComponent<MeshRenderer>(Shaders::Main);
    meshRenderer.GetMesh().Init(TRIANGLE_VERTICES, sizeof(TRIANGLE_VERTICES), {{AttribType::Float3}});
}
