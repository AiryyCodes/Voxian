#pragma once

#include "World/Chunk/Terrain/NoiseLayerConfig.h"

#include <FastNoiseLite.h>

namespace NoiseFactory
{
FastNoiseLite BuildNoise(const NoiseLayerConfig &config);
FastNoiseLite BuildDomainWarp(const NoiseLayerConfig &config);
} // namespace NoiseFactory