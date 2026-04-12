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

    int MinHeight = -1; // world-space Y, -1 = disabled
    int MaxHeight = -1;

    uint16_t BlockIndex = Blocks::AIR;

    struct glaze
    {
        using T = BlockLayerConfig;
        static constexpr auto value = glz::object(
            &T::Block,
            &T::Condition,
            &T::MaxDepth,
            &T::RequiresBelowSeaLevel,
            &T::Priority,
            &T::MinHeight,
            &T::MaxHeight);
    };
};

struct NoiseLayerConfig
{
    std::string Name = "";
    float Weight = 1.0f;
    FastNoiseLite::NoiseType NoiseType = FastNoiseLite::NoiseType_OpenSimplex2;
    FastNoiseLite::FractalType FractalType = FastNoiseLite::FractalType_FBm;
    float Frequency = 0.004f;
    float Amplitude = 1.0f;
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
            &T::Frequency, &T::Amplitude, &T::Octaves, &T::Lacunarity,
            &T::Gain, &T::WeightedStrength, &T::UseDomainWarp,
            &T::DomainWarpType, &T::DomainWarpAmp);
    };
};

struct CurvePoint
{
    float In = 0.0f;
    float Out = 0.0f;

    struct glaze
    {
        using T = CurvePoint;
        static constexpr auto value = glz::object(&T::In, &T::Out);
    };
};

struct HeightRange
{
    int Min = 0;
    int Max = 256;

    struct glaze
    {
        using T = HeightRange;
        static constexpr auto value = glz::object(&T::Min, &T::Max);
    };
};

struct TerrainConfig
{
    int SeaLevel = 128;
    HeightRange HeightRange;
    std::vector<NoiseLayerConfig> NoiseLayers;
    std::vector<CurvePoint> HeightCurve; // empty = linear passthrough
    std::vector<BlockLayerConfig> BlockLayers;

    struct glaze
    {
        using T = TerrainConfig;
        static constexpr auto value = glz::object(
            &T::SeaLevel, &T::HeightRange, &T::NoiseLayers,
            &T::HeightCurve, &T::BlockLayers);
    };
};

struct NoisePair
{
    FastNoiseLite noise;
    FastNoiseLite domainWarp;
    bool useDomainWarp = false;
    float weight = 1.0f;
};

class TerrainNoise
{
public:
    void Init();

    TerrainConfig Load(const std::string &path);

    float GetNoise(float worldX, float worldZ) const;
    float GetNoiseCurved(float worldX, float worldZ) const;

    const TerrainConfig &GetConfig() const { return m_Config; }

private:
    float ApplyCurve(float t) const;

private:
    TerrainConfig m_Config;
    std::vector<NoisePair> m_NoisePairs;
};
