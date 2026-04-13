#pragma once

#include <FastNoiseLite.h>
#include <glaze/glaze.hpp>

template <>
struct glz::meta<FastNoiseLite::NoiseType>
{
    using enum FastNoiseLite::NoiseType;
    static constexpr auto value = glz::enumerate(
        "OpenSimplex2", NoiseType_OpenSimplex2,
        "OpenSimplex2S", NoiseType_OpenSimplex2S,
        "Cellular", NoiseType_Cellular,
        "Perlin", NoiseType_Perlin,
        "ValueCubic", NoiseType_ValueCubic,
        "Value", NoiseType_Value);
};

template <>
struct glz::meta<FastNoiseLite::FractalType>
{
    using enum FastNoiseLite::FractalType;
    static constexpr auto value = glz::enumerate(
        "None", FractalType_None,
        "FBm", FractalType_FBm,
        "Ridged", FractalType_Ridged,
        "PingPong", FractalType_PingPong);
};

template <>
struct glz::meta<FastNoiseLite::DomainWarpType>
{
    using enum FastNoiseLite::DomainWarpType;
    static constexpr auto value = glz::enumerate(
        "OpenSimplex2", DomainWarpType_OpenSimplex2,
        "OpenSimplex2Reduced", DomainWarpType_OpenSimplex2Reduced,
        "BasicGrid", DomainWarpType_BasicGrid);
};

struct NoiseLayerConfig
{
    std::string Name = "";
    float Weight = 1.0f;
    FastNoiseLite::NoiseType NoiseType = FastNoiseLite::NoiseType_OpenSimplex2;
    FastNoiseLite::FractalType FractalType = FastNoiseLite::FractalType_FBm;
    float Frequency = 0.004f;
    int Octaves = 5;
    float Lacunarity = 2.0f;
    float Gain = 0.5f;
    float WeightedStrength = 0.0f;
    bool UseDomainWarp = false;
    FastNoiseLite::DomainWarpType DomainWarpType = FastNoiseLite::DomainWarpType_OpenSimplex2;
    float DomainWarpAmp = 30.0f;

    struct glaze
    {
        using T = NoiseLayerConfig;
        static constexpr auto value = glz::object(
            &T::Name, &T::Weight, &T::NoiseType, &T::FractalType,
            &T::Frequency, &T::Octaves, &T::Lacunarity,
            &T::Gain, &T::WeightedStrength, &T::UseDomainWarp,
            &T::DomainWarpType, &T::DomainWarpAmp);
    };
};