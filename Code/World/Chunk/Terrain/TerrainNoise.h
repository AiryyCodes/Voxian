#pragma once

#include "Biome/BiomeConfig.h"

#include <string>
#include <glaze/glaze.hpp>
#include <FastNoiseLite.h>
#include <unordered_map>
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
    HeightRange HeightRange;
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
    const BiomeConfig *biome = nullptr;
    float weight = 0.0f;
};

class TerrainNoise
{
public:
    void Init();

    TerrainConfig Load(const std::string &path);

    float GetNoise(float worldX, float worldZ, const BiomeConfig *biome) const;
    float GetNoiseCurved(float worldX, float worldZ) const;

    float ApplyCurve(float t, const BiomeConfig *biome) const;

    const BiomeConfig *SelectBiome(float x, float z) const;
    std::vector<BiomeWeight> SelectBiomes(float x, float z) const;
    std::unordered_map<std::string, float> SampleClimate(float x, float z) const;

    const TerrainConfig &GetConfig() const { return m_Config; }

private:
    TerrainConfig m_Config;
    std::vector<ClimateNoise> m_ClimateSamplers;
};
