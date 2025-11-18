#pragma once

#include "Input.h"

#include <GLFW/glfw3.h>
#include <string>

class Window
{
public:
    Window(int width, int height, const std::string &title);

    const GLFWwindow *GetGLFWWindow() const { return m_Window; }
    GLFWwindow *GetGLFWWindow() { return m_Window; }

    void SwapBuffers() const;

    void SetCursorMode(CursorMode mode);

    bool IsClosing() const;

    static void PollEvents();

    const std::string &GetTitle() const { return m_Title; }
    void SetTitle(const std::string &title);

    int GetWidth() const { return m_Width; }
    void SetWidth(int width);

    int GetHeight() const { return m_Height; }
    void SetHeight(int height);

    int GetFrameBufferWidth() const { return m_FrameBufferWidth; }
    int GetFrameBufferHeight() const { return m_FrameBufferHeight; }

    static Window &GetMain() { return *m_MainInstance; }

private:
    static Window *m_MainInstance;

    GLFWwindow *m_Window;

    std::string m_Title;

    int m_Width;
    int m_Height;

    int m_FrameBufferWidth;
    int m_FrameBufferHeight;
};
