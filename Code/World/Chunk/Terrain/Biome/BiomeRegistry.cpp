#include "BiomeRegistry.h"
#include "Engine.h"
#include "World/Chunk/Terrain/NoiseFactory.h"

#include <algorithm>

void BiomeRegistry::LoadAll(const std::string &directory)
{
    for (auto &entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() != ".json")
            continue;

        std::ifstream file(entry.path());
        std::ostringstream ss;
        ss << file.rdbuf();

        auto biome = glz::read_json<BiomeConfig>(ss.str());
        if (!biome)
        {
            LOG_ERROR("Failed to parse biome: {}", entry.path().string());
            continue;
        }

        // resolve block indices same as TerrainNoise::Load does
        for (auto &layer : biome->BlockLayers)
            layer.BlockIndex = EngineContext::GetBlockRegistry()
                                   .GetBlockIndexById(layer.Block);

        std::sort(biome->BlockLayers.begin(), biome->BlockLayers.end(),
                  [](const BlockLayerConfig &a, const BlockLayerConfig &b)
                  {
                      return a.Priority < b.Priority;
                  });

        // Build NoisePairs from NoiseLayers
        for (auto &layer : biome->NoiseLayers)
        {
            NoisePair pair;
            pair.noise = NoiseFactory::BuildNoise(layer);
            pair.domainWarp = NoiseFactory::BuildDomainWarp(layer);
            pair.useDomainWarp = layer.UseDomainWarp;
            pair.weight = layer.Weight;
            biome->NoisePairs.push_back(pair);
        }

        if (biome->NoisePairs.empty())
            LOG_WARNING("Biome '{}' was loaded but has no NoiseLayers defined", biome->Id);
        else
            LOG_INFO("Loaded biome '{}' with {} noise layer(s)", biome->Id, biome->NoisePairs.size());

        m_Biomes[biome->Id] = std::move(*biome);
    }

    m_SortedIds.clear();
    for (const auto &[id, _] : m_Biomes)
    {
        m_SortedIds.push_back(id);
    }

    std::sort(m_SortedIds.begin(), m_SortedIds.end());
}

const BiomeConfig *BiomeRegistry::GetById(const std::string &id) const
{
    auto it = m_Biomes.find(id);
    return it != m_Biomes.end() ? &it->second : nullptr;
}