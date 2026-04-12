#include "TerrainNoise.h"
#include "Biome/BiomeConfig.h"
#include "Engine.h"
#include "Logger.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <glaze/json/generic.hpp>
#include <string>
#include <FastNoiseLite.h>

static FastNoiseLite BuildClimateNoise(const ClimateMapConfig &config)
{
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFrequency(config.Frequency);
    noise.SetFractalOctaves(config.Octaves);
    return noise;
}

void TerrainNoise::Init()
{
    m_Config = Load("Assets/Terrain.json");
    m_ClimateSamplers.clear();

    for (const auto &map : m_Config.ClimateMaps)
    {
        ClimateNoise sampler;
        sampler.Name = map.Name;
        sampler.Noise = BuildClimateNoise(map);
        m_ClimateSamplers.push_back(std::move(sampler));
    }
}

TerrainConfig TerrainNoise::Load(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR("Cannot open terrain config: {}", path);
        assert(false);
    }
    std::ostringstream ss;
    ss << file.rdbuf();

    auto config = glz::read_json<TerrainConfig>(ss.str());
    if (!config)
    {
        auto err = glz::format_error(config.error(), ss.str());
        LOG_ERROR("Failed to parse terrain config: {} — {}", path, err);
        assert(false);
    }

    return *config;
}

float TerrainNoise::GetNoise(float worldX, float worldZ, const BiomeConfig *biome) const
{
    if (!biome || biome->NoisePairs.empty())
    {
        LOG_WARNING("Biome '{}' has no noise layers", biome ? biome->Id : "null");
        return 0.5f;
    }

    float combined = 0.0f;
    float totalWeight = 0.0f;

    for (size_t i = 0; i < biome->NoisePairs.size(); i++)
    {
        float x = worldX, z = worldZ;
        if (biome->NoisePairs[i].useDomainWarp)
        {
            FastNoiseLite warper = biome->NoisePairs[i].domainWarp;
            warper.DomainWarp(x, z);
        }
        combined += biome->NoisePairs[i].noise.GetNoise(x, z) * biome->NoisePairs[i].weight;
        totalWeight += biome->NoisePairs[i].weight;
    }

    float normalized = (combined / totalWeight) * 0.5f + 0.5f;
    return std::clamp(normalized, 0.0f, 1.0f);
}

// Returns noise with height curve applied, still in [0, 1]
float TerrainNoise::GetNoiseCurved(float worldX, float worldZ) const
{
    const BiomeConfig *biome = SelectBiome(worldX, worldZ);
    return ApplyCurve(GetNoise(worldX, worldZ, biome), biome);
}

float TerrainNoise::ApplyCurve(float t, const BiomeConfig *biome) const
{
    assert(biome && !biome->HeightCurve.empty());
    const auto &curve = biome->HeightCurve;
    if (curve.size() < 2)
        return t; // linear passthrough

    // Find the two surrounding points and lerp between them
    for (size_t i = 0; i + 1 < curve.size(); i++)
    {
        const auto &p0 = curve[i];
        const auto &p1 = curve[i + 1];
        if (t >= p0.In && t <= p1.In)
        {
            float localT = (t - p0.In) / (p1.In - p0.In);
            return p0.Out + localT * (p1.Out - p0.Out);
        }
    }

    return curve.back().Out; // past the end
}

const BiomeConfig *TerrainNoise::SelectBiome(float x, float z) const
{
    auto climate = SampleClimate(x, z);
    const BiomeConfig *bestMatch = nullptr;
    float bestFitness = -1.0f; // Higher is better

    const auto &registry = EngineContext::GetBiomeRegistry();

    for (const std::string &id : EngineContext::GetBiomeRegistry().GetAllSorted())
    {
        const BiomeConfig *biome = registry.GetById(id);
        auto &c = biome->Conditions;

        // check if it's within the required bounds
        if ((c.Temperature && !c.Temperature->Contains(climate["temperature"])) ||
            (c.Humidity && !c.Humidity->Contains(climate["humidity"])) ||
            (c.Continentalness && !c.Continentalness->Contains(climate["continentalness"])))
        {
            continue;
        }

        float currentFitness = 0.0f;
        if (c.Temperature)
            currentFitness += (1.0f - (c.Temperature->Max - c.Temperature->Min));
        if (c.Humidity)
            currentFitness += (1.0f - (c.Humidity->Max - c.Humidity->Min));

        if (currentFitness > bestFitness)
        {
            bestFitness = currentFitness;
            bestMatch = biome;
        }
    }

    return bestMatch ? bestMatch : EngineContext::GetBiomeRegistry().GetById(m_Config.FallbackBiome);
}

std::unordered_map<std::string, float> TerrainNoise::SampleClimate(float x, float z) const
{
    std::unordered_map<std::string, float> values;
    for (auto &c : m_ClimateSamplers)
    {
        float v = c.Noise.GetNoise(x, z); // [-1, 1]
        values[c.Name] = v * 0.5f + 0.5f; // normalize to [0, 1]
    }
    return values;
}