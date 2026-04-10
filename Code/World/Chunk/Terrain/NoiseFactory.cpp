#include "NoiseFactory.h"

FastNoiseLite NoiseFactory::BuildNoise(const TerrainConfig &config)
{
    // TODO: Make the seed not hard-coded
    FastNoiseLite noise(1337);
    noise.SetFrequency(config.Frequency);
    noise.SetNoiseType(config.NoiseType);
    noise.SetFractalType(config.FractalType);
    noise.SetFractalOctaves(config.Octaves);
    noise.SetFractalLacunarity(config.Lacunarity);
    noise.SetFractalGain(config.Gain);
    noise.SetFractalWeightedStrength(config.WeightedStrength);

    return noise;
}

FastNoiseLite NoiseFactory::BuildDomainWarp(const TerrainConfig &cfg)
{
    FastNoiseLite warp(1337);

    warp.SetDomainWarpType(cfg.DomainWarpType);
    warp.SetDomainWarpAmp(cfg.DomainWarpAmp);
    warp.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);

    return warp;
}