#include "TerrainNoise.h"
#include "Engine.h"
#include "FastNoiseLite.h"
#include "Logger.h"
#include "NoiseFactory.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <glaze/json/generic.hpp>
#include <threads.h>

void TerrainNoise::Init()
{
    m_Config = Load("Assets/Terrain.json");

    m_Noise = NoiseFactory::BuildNoise(m_Config);
    m_DomainWarp = NoiseFactory::BuildDomainWarp(m_Config);
}

TerrainConfig TerrainNoise::Load(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {

        LOG_ERROR("Cannot open terrain config: " + path);
        assert(false);
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    auto config = glz::read_json<TerrainConfig>(ss.str());
    if (!config)
    {
        LOG_ERROR("Failed to parse terrain config from file: {}", path);
        assert(false);
    }

    for (auto &layer : config->Layers)
    {
        layer.BlockIndex = EngineContext::GetBlockRegistry().GetBlockIndexById(layer.Block);
    }

    // TODO: Sort layers after priority
    std::sort(config->Layers.begin(), config->Layers.end(),
              [](const BlockLayerConfig &a, const BlockLayerConfig &b)
              {
                  return a.Priority < b.Priority;
              });

    return *config;
}

float TerrainNoise::GetNoise(float worldX, float worldZ) const
{
    if (m_Config.UseDomainWarp)
    {
        thread_local FastNoiseLite localWarp = NoiseFactory::BuildDomainWarp(m_Config);
        localWarp.DomainWarp(worldX, worldZ);
    }

    return m_Noise.GetNoise(worldX, worldZ) * m_Config.Amplitude;
}