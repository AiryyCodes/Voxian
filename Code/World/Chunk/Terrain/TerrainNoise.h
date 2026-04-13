#pragma once

#include "Biome/BiomeConfig.h"

#include <string>
#include <glaze/glaze.hpp>
#include <FastNoiseLite.h>
#include <vector>

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
    struct HeightRange HeightRange;
    std::vector<ClimateMapConfig> ClimateMaps;
    std::vector<BiomeRule> BiomeRules;
    std::string FallbackBiome = "plains";

    struct glaze
    {
        using T = TerrainConfig;
        static constexpr auto value = glz::object(
            &T::SeaLevel, &T::HeightRange, &T::ClimateMaps,
            &T::BiomeRules, &T::FallbackBiome);
    };
};

struct ClimateNoise
{
    std::string Name;
    FastNoiseLite Noise;
};

struct BiomeWeight
{
    const BiomeConfig *Biome = nullptr;
    float Weight = 0.0f;
};

struct ClimateValues
{
    float Temperature;
    float Humidity;
    float Continentalness;
};

class TerrainNoise
{
public:
    void Init();

    TerrainConfig Load(const std::string &path);

    float GetDensity(float x, float y, float z) const;
    float GetNoise(float worldX, float worldZ, const BiomeConfig *biome) const;

    float ApplyOffset(float noise, const BiomeConfig *biome) const;
    float ApplyScale(float noise, const BiomeConfig *biome) const;
    float InterpolateCurve(const std::vector<CurvePoint> &curve, float t) const;

    const BiomeConfig *SelectBiome(float x, float z) const;
    int SampleBiomeWeights(float x, float z, BiomeWeight *outWeights, int maxN = 3) const;

    ClimateValues SampleClimate(float x, float z) const;

    const TerrainConfig &GetConfig() const { return m_Config; }

private:
    TerrainConfig m_Config;
    std::vector<ClimateNoise> m_ClimateSamplers;
};
