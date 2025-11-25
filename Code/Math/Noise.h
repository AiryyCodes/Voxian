#pragma once

#include "FastNoiseLite.h"

class Noise
{
public:
    float GetNoise(float x, float z) const { return m_Noise.GetNoise(x, z); }

    float GetFractalNoise2D(float x, float z, int octaves = 4, float persistence = 0.5f, float frequency = 0.5f) const;

private:
    FastNoiseLite m_Noise;
};
