#pragma once

inline int FloorDiv(int a, int b)
{
    int d = a / b;
    if ((a ^ b) < 0 && a % b)
        --d;
    return d;
}

// positive modulo in [0, b-1] for possibly negative a
inline int PosMod(int a, int b)
{
    int m = a % b;
    if (m < 0)
        m += b;
    return m;
}
