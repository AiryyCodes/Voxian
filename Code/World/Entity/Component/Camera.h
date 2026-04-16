#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "World/Entity/Component/Component.h"

class Camera : public Component
{
public:
    Camera(float fov = 45.0f, float aspect = 16.0f / 9.0f, float near = 0.1f, float far = 1000.0f)
        : m_FOV(fov), m_BaseFOV(fov), m_AspectRatio(aspect), m_NearPlane(near), m_FarPlane(far)
    {
        UpdateProjectionMatrix();
    }

    void SetFOV(float fov)
    {
        m_FOV = fov;
        UpdateProjectionMatrix();
    }
    void SetBaseFOV(float fov)
    {
        m_BaseFOV = fov;
        UpdateProjectionMatrix();
    }

    void SetAspectRatio(float aspect)
    {
        m_AspectRatio = aspect;
        UpdateProjectionMatrix();
    }

    void SetNearPlane(float near)
    {
        m_NearPlane = near;
        UpdateProjectionMatrix();
    }

    void SetFarPlane(float far)
    {
        m_FarPlane = far;
        UpdateProjectionMatrix();
    }

    float GetFOV() const { return m_FOV; }
    float GetBaseFOV() const { return m_BaseFOV; }

    float GetAspectRatio() const { return m_AspectRatio; }
    float GetNearPlane() const { return m_NearPlane; }
    float GetFarPlane() const { return m_FarPlane; }

    void SetPitch(float pitch) { m_Pitch = pitch; }
    float GetPitch() const { return m_Pitch; }

    Matrix4 GetViewMatrix() const;
    Matrix4 GetProjectionMatrix() const { return m_ProjectionMatrix; }

    Vector3f GetForward() const;
    Vector3f GetRight() const;

private:
    void UpdateProjectionMatrix();

private:
    float m_FOV;
    float m_BaseFOV = 70.0f;

    float m_AspectRatio;
    float m_NearPlane;
    float m_FarPlane;
    float m_Pitch = 0.0f; // Vertical rotation in degrees

    Matrix4 m_ProjectionMatrix;
};
