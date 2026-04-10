#pragma once

#include "World/Block/Blocks.h"
#include <string>
#include <glaze/glaze.hpp>
#include <FastNoiseLite.h>
#include <vector>

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

enum class LayerCondition
{
    AtSurface,
    BelowSurface,
    BelowSeaLevel,
    AtBottom
};

template <>
struct glz::meta<LayerCondition>
{
    using enum LayerCondition;
    static constexpr auto value = glz::enumerate(
        "AtSurface", AtSurface,
        "BelowSurface", BelowSurface,
        "BelowSeaLevel", BelowSeaLevel,
        "AtBottom", AtBottom);
};

struct BlockLayerConfig
{
    std::string Block = "";
    LayerCondition Condition = LayerCondition::AtSurface;
    int MaxDepth = -1;
    bool RequiresBelowSeaLevel = false;
    int Priority = 0;

    uint16_t BlockIndex = Blocks::AIR;

    struct glaze
    {
        using T = BlockLayerConfig;
        static constexpr auto value = glz::object(
            &T::Block,
            &T::Condition,
            &T::MaxDepth,
            &T::RequiresBelowSeaLevel,
            &T::Priority);
    };
};

struct TerrainConfig
{
    float Frequency = 0.004f;
    float Amplitude = 1.0f;
    FastNoiseLite::NoiseType NoiseType = FastNoiseLite::NoiseType_OpenSimplex2;
    FastNoiseLite::FractalType FractalType = FastNoiseLite::FractalType_FBm;
    int Octaves = 5;
    float Lacunarity = 2.0f;
    float Gain = 0.5f;
    float WeightedStrength = 0.0f;
    bool UseDomainWarp = false;
    FastNoiseLite::DomainWarpType DomainWarpType = FastNoiseLite::DomainWarpType_OpenSimplex2;
    float DomainWarpAmp = 30.0f;

    int SeaLevel = 128;

    std::vector<BlockLayerConfig> Layers;

    struct glaze
    {
        using T = TerrainConfig;
        static constexpr auto value = glz::object(&T::Frequency,
                                                  &T::Amplitude,
                                                  &T::NoiseType,
                                                  &T::FractalType,
                                                  &T::Octaves,
                                                  &T::Lacunarity,
                                                  &T::Gain,
                                                  &T::WeightedStrength,
                                                  &T::UseDomainWarp,
                                                  &T::DomainWarpType,
                                                  &T::DomainWarpAmp,
                                                  &T::SeaLevel,
                                                  &T::Layers);
    };
};

class TerrainNoise
{
public:
    void Init();

    TerrainConfig Load(const std::string &path);

    float GetNoise(float worldX, float worldZ) const;

    const TerrainConfig &GetConfig() const { return m_Config; }

private:
    TerrainConfig m_Config;
    FastNoiseLite m_Noise;
    FastNoiseLite m_DomainWarp;
};