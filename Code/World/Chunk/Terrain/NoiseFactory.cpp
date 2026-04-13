#include "NoiseFactory.h"

FastNoiseLite NoiseFactory::BuildNoise(const NoiseLayerConfig &layer)
{
    FastNoiseLite noise;
    noise.SetSeed(42); // add this
    noise.SetNoiseType(layer.NoiseType);
    noise.SetFractalType(layer.FractalType);
    noise.SetFrequency(layer.Frequency);
    noise.SetFractalOctaves(layer.Octaves);
    noise.SetFractalLacunarity(layer.Lacunarity);
    noise.SetFractalGain(layer.Gain);
    noise.SetFractalWeightedStrength(layer.WeightedStrength);
    return noise;
}

FastNoiseLite NoiseFactory::BuildDomainWarp(const NoiseLayerConfig &layer)
{
    FastNoiseLite warp;
    warp.SetSeed(42); // add this
    warp.SetDomainWarpType(layer.DomainWarpType);
    warp.SetDomainWarpAmp(layer.DomainWarpAmp);
    warp.SetFrequency(layer.Frequency);
    return warp;
}