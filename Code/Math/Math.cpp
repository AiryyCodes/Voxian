#include "Math.h"

float Math::Remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (value - inMin) / (inMax - inMin) * (outMax - outMin);
}