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
    noise.SetFractalOctaves(config.Octaves);

    noise.SetFrequency(config.Frequency);
    return noise;
}

// Quintic falloff: flat in the center, smooth zero crossing at radius edge.
// Much cleaner than raw inverse-distance which over-weights the winner.
static float BiomeWeight_Kernel(float distSq, float radiusSq)
{
    if (distSq >= radiusSq)
        return 0.0f;
    float t = 1.0f - (distSq / radiusSq);                // 1 at center, 0 at edge
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); // smoothstep6
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

float TerrainNoise::GetDensity(float x, float y, float z) const
{
    auto weights = SampleBiomeWeights(x, z, 3);

    float blendedOffset = 0.0f;
    float blendedScale = 0.0f;

    for (auto &bw : weights)
    {
        float n = GetNoise(x, z, bw.Biome);
        blendedOffset += ApplyOffset(n, bw.Biome) * bw.Weight;
        blendedScale += ApplyScale(n, bw.Biome) * bw.Weight;
    }

    float yFrac = (y - m_Config.HeightRange.Min) /
                  (float)(m_Config.HeightRange.Max - m_Config.HeightRange.Min);

    return (blendedOffset - yFrac) / std::max(blendedScale, 0.001f);
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
            auto warper = biome->NoisePairs[i].domainWarp;
            warper.DomainWarp(x, z);
        }
        combined += biome->NoisePairs[i].noise.GetNoise(x, z) * biome->NoisePairs[i].weight;
        totalWeight += biome->NoisePairs[i].weight;
    }

    float normalized = (combined / totalWeight) * 0.5f + 0.5f;
    return std::clamp(normalized, 0.0f, 1.0f);
}

float TerrainNoise::ApplyOffset(float noise, const BiomeConfig *biome) const
{
    return InterpolateCurve(biome->OffsetCurve, noise);
}

float TerrainNoise::ApplyScale(float noise, const BiomeConfig *biome) const
{
    return InterpolateCurve(biome->ScaleCurve, noise);
}

float TerrainNoise::InterpolateCurve(const std::vector<CurvePoint> &curve, float t) const
{
    if (curve.size() < 2)
        return t;
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
    return curve.back().Out;
}

const BiomeConfig *TerrainNoise::SelectBiome(float x, float z) const
{
    auto weights = SampleBiomeWeights(x, z, 3);
    if (weights.empty())
        return EngineContext::GetBiomeRegistry().GetById(m_Config.FallbackBiome);
    // weights[0] is always the nearest after partial_sort
    return weights[0].Biome;
}

std::vector<BiomeWeight> TerrainNoise::SampleBiomeWeights(float x, float z, int topN) const
{
    auto climate = SampleClimate(x, z);
    float t = climate.Temperature;
    float h = climate.Humidity;
    float c = climate.Continentalness;

    // Collect all biomes with their squared climate-space distances
    struct Candidate
    {
        const BiomeConfig *Biome;
        float DistSq;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(16);

    for (auto &[id, biome] : EngineContext::GetBiomeRegistry().GetAll())
    {
        auto &p = biome.ClimatePoint;
        float dt = t - p.Temperature;
        float dh = h - p.Humidity;
        float dc = c - p.Continentalness;
        // Continentalness weighted less so it drives macro shape, not borders
        float distSq = dt * dt + dh * dh + 0.4f * dc * dc;
        candidates.push_back({&biome, distSq});
    }

    // Partial sort: cheapest way to get the topN closest
    int clampedN = std::min(topN, (int)candidates.size());
    std::partial_sort(candidates.begin(), candidates.begin() + clampedN, candidates.end(),
                      [](const Candidate &a, const Candidate &b)
                      { return a.DistSq < b.DistSq; });
    candidates.resize(clampedN);

    // The blend radius is set to 2× the nearest biome's distance so the
    // winner always dominates the center but neighbors bleed in at borders.
    float blendRadius = candidates[0].DistSq * 4.0f + 0.0001f; // in distSq space

    std::vector<BiomeWeight> result;
    result.reserve(clampedN);
    float totalW = 0.0f;

    for (auto &cand : candidates)
    {
        float w = BiomeWeight_Kernel(cand.DistSq, blendRadius);
        if (w > 1e-5f)
        {
            result.push_back({cand.Biome, w});
            totalW += w;
        }
    }

    // Always guarantee at least the nearest biome
    if (result.empty())
    {
        result.push_back({candidates[0].Biome, 1.0f});
        return result;
    }

    // Normalize
    for (auto &bw : result)
        bw.Weight /= totalW;

    return result;
}

ClimateValues TerrainNoise::SampleClimate(float x, float z) const
{
    ClimateValues v{};
    for (auto &c : m_ClimateSamplers)
    {
        float val = c.Noise.GetNoise(x, z) * 0.5f + 0.5f;
        if (c.Name == "temperature")
            v.Temperature = val;
        else if (c.Name == "humidity")
            v.Humidity = val;
        else if (c.Name == "continentalness")
            v.Continentalness = val;
    }
    return v;
}