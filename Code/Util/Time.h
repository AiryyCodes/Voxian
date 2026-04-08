#pragma once

class Time
{
public:
    static void Init();
    static void Update();

    static float GetDeltaTime();

private:
    static inline float s_DeltaTime = 0.0f;
    static inline float s_LastFrameTime = 0.0f;
    static inline float s_CurrentFrameTime = 0.0f;
};