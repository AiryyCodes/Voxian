#pragma once

#include "Window.h"

#include <GLFW/glfw3.h>
#include <array>

class Input
{
public:
    Input() = default;
    explicit Input(Window &window);
    ~Input();

    void Update();

    bool IsKeyDown(int key) const;
    bool IsKeyJustDown(int key) const;
    bool IsKeyJustReleased(int key) const;
    bool IsKeyDoubleTapped(int key) const;

    float GetMouseX() const;
    float GetMouseY() const;

    bool IsMouseButtonPressed(int button);
    bool IsMouseButtonJustPressed(int button);

    void SetCursorMode(int mode) const;
    bool IsCursorLocked() const;

private:
    static Input *Get(GLFWwindow *window);

    static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

private:
    Window *m_Window = nullptr;

    struct KeyState
    {
        bool Current = false;
        bool Previous = false;
        float LastPressTime = -100.0f;
        bool IsDoubleTapped = false;
    };

    struct MouseButtonState
    {
        bool Current = false;
        bool Previous = false;
    };

    std::array<KeyState, GLFW_KEY_LAST + 1> m_Keys = {};
    std::array<MouseButtonState, GLFW_MOUSE_BUTTON_LAST + 1> m_MouseButtons = {};
};
