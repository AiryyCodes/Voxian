#include "Time.h"

#include <GLFW/glfw3.h>

void Time::Init()
{
    s_LastFrameTime = static_cast<float>(glfwGetTime());
}

void Time::Update()
{
    s_CurrentFrameTime = static_cast<float>(glfwGetTime());
    s_DeltaTime = s_CurrentFrameTime - s_LastFrameTime;
    s_LastFrameTime = s_CurrentFrameTime;
}

float Time::GetDeltaTime()
{
    return s_DeltaTime;
}