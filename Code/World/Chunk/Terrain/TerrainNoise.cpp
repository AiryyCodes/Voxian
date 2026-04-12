#include "TerrainNoise.h"
#include "Engine.h"
#include "FastNoiseLite.h"
#include "Logger.h"
#include "NoiseFactory.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <glaze/json/generic.hpp>

void TerrainNoise::Init()
{
    m_Config = Load("Assets/Terrain.json");

    m_NoisePairs.clear();
    for (const auto &layerCfg : m_Config.NoiseLayers)
    {
        NoisePair pair;
        pair.noise = NoiseFactory::BuildNoise(layerCfg);
        pair.domainWarp = NoiseFactory::BuildDomainWarp(layerCfg);
        pair.useDomainWarp = layerCfg.UseDomainWarp;
        pair.weight = layerCfg.Weight;
        m_NoisePairs.push_back(pair);
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

    for (auto &layer : config->BlockLayers)
        layer.BlockIndex = EngineContext::GetBlockRegistry().GetBlockIndexById(layer.Block);

    std::sort(config->BlockLayers.begin(), config->BlockLayers.end(),
              [](const BlockLayerConfig &a, const BlockLayerConfig &b)
              {
                  return a.Priority < b.Priority;
              });

    return *config;
}

float TerrainNoise::GetNoise(float worldX, float worldZ) const
{
    float combined = 0.0f;
    float totalWeight = 0.0f;

    for (size_t i = 0; i < m_NoisePairs.size(); i++)
    {
        // Domain warp must operate on a local copy of coordinates
        float x = worldX, z = worldZ;

        if (m_NoisePairs[i].useDomainWarp)
        {
            // DomainWarp() is non-const, so we cache per-thread
            thread_local std::vector<FastNoiseLite> localWarps;
            if (localWarps.size() <= i)
                localWarps.resize(i + 1, m_NoisePairs[i].domainWarp);
            localWarps[i].DomainWarp(x, z);
        }

        float sample = m_NoisePairs[i].noise.GetNoise(x, z); // [-1, 1]
        combined += sample * m_NoisePairs[i].weight;
        totalWeight += m_NoisePairs[i].weight;
    }

    if (totalWeight == 0.0f)
        return 0.5f;

    float normalized = (combined / totalWeight) * 0.5f + 0.5f; // [0, 1]
    return std::clamp(normalized, 0.0f, 1.0f);
}

// Returns noise with height curve applied, still in [0, 1]
float TerrainNoise::GetNoiseCurved(float worldX, float worldZ) const
{
    return ApplyCurve(GetNoise(worldX, worldZ));
}

float TerrainNoise::ApplyCurve(float t) const
{
    const auto &curve = m_Config.HeightCurve;
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