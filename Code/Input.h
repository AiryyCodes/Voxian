#pragma once

#include <GLFW/glfw3.h>
#include <array>
#include <cstddef>

enum class CursorMode
{
    Locked,
    Unlocked,
    Hidden
};

class Input
{
public:
    static void OnKey(int key, int action);

    static bool IsKeyDown(int key);
    static bool IsKeyHeld(int key);

    static void OnMouseMove(float mouseX, float mouseY);

    static float GetMouseX();
    static float GetMouseY();
    static float GetMouseDeltaX();
    static float GetMouseDeltaY();

    static void SetCursorMode(CursorMode mode);
    static bool IsCursorLocked();

private:
    static std::array<bool, static_cast<size_t>(GLFW_KEY_LAST)> m_Keys;
    static std::array<bool, static_cast<size_t>(GLFW_KEY_LAST)> m_KeysHeld;

    static float m_MouseX;
    static float m_MouseY;
    static float m_MouseLastX;
    static float m_MouseLastY;

    static bool m_FirstMouse;

    static CursorMode m_CursorMode;
};
