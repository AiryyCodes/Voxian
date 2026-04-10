#pragma once

#include "World/Chunk/Terrain/TerrainNoise.h"

#include <FastNoiseLite.h>

namespace NoiseFactory
{
FastNoiseLite BuildNoise(const TerrainConfig &config);
FastNoiseLite BuildDomainWarp(const TerrainConfig &config);
} // namespace NoiseFactory