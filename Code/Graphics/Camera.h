#pragma once

#include "Graphics/Shader.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "World/Chunk.h"

class Camera
{
public:
    struct Matrices
    {
        Matrix4 view;
        Matrix4 projection;
    };

public:
    void Update();
    void Draw(const Shader &shader, const Matrices &matrices) const;

    Matrix4 GetViewMatrix();
    Matrix4 GetProjectionMatrix();

    const Vector3f &GetPosition() const { return m_Position; }
    const Vector3f &GetRotation() const { return m_Rotation; }

    const Matrices &GetMatrices() const { return m_Matrices; }

private:
    Vector3f m_Position = Vector3f(CHUNK_WIDTH / 2.0f, CHUNK_BASE_HEIGHT + 16.0f, CHUNK_WIDTH / 2.0f);
    // Vector3f m_Position = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f m_Rotation = Vector3f(0.0f, 0.0f, 0.0f);

    Vector3f m_Right;
    Vector3f m_Forward;

    float m_Speed = 25.0f;
    float m_Sensitivity = 0.2f;

    float m_Fov = 70.0f;
    float m_Near = 0.05f;
    float m_Far = 500.0f;

    Matrices m_Matrices;
};
