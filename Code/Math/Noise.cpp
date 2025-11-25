#include "Math/Noise.h"

float Noise::GetFractalNoise2D(float x, float z, int octaves, float persistence, float frequency) const
{
    float total = 0.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i)
    {
        total += m_Noise.GetNoise(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}
