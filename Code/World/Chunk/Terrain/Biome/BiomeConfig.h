#pragma once

#include "World/Chunk/Terrain/CurvePoint.h"
#include "World/Chunk/Terrain/BlockLayerConfig.h"
#include "World/Chunk/Terrain/NoiseLayerConfig.h"

#include <FastNoiseLite.h>
#include <string>
#include <vector>

struct ClimateRange
{
    float Min = 0.0f;
    float Max = 1.0f;

    bool Contains(float v) const
    {
        return v >= Min && v <= Max;
    }

    struct glaze
    {
        using T = ClimateRange;
        static constexpr auto value = glz::object(&T::Min, &T::Max);
    };
};

struct BiomeConditions
{
    std::optional<ClimateRange> Temperature;
    std::optional<ClimateRange> Humidity;
    std::optional<ClimateRange> Continentalness;

    struct glaze
    {
        using T = BiomeConditions;
        static constexpr auto value = glz::object(
            &T::Temperature, &T::Humidity, &T::Continentalness);
    };
};

struct BiomeRule
{
    std::string Biome = "";
    BiomeConditions Conditions;

    struct glaze
    {
        using T = BiomeRule;
        static constexpr auto value = glz::object(&T::Biome, &T::Conditions);
    };
};

struct ClimateMapConfig
{
    std::string Name = "";
    float Frequency = 0.001f;
    int Octaves = 2;

    struct glaze
    {
        using T = ClimateMapConfig;
        static constexpr auto value = glz::object(
            &T::Name, &T::Frequency, &T::Octaves);
    };
};

struct NoisePair
{
    FastNoiseLite noise;
    FastNoiseLite domainWarp;
    bool useDomainWarp = false;
    float weight = 1.0f;
};

struct BiomeClimatePoint
{
    float Temperature = 0.5f;
    float Humidity = 0.5f;
    float Continentalness = 0.5f;

    struct glaze
    {
        using T = BiomeClimatePoint;
        static constexpr auto value = glz::object(
            &T::Temperature, &T::Humidity, &T::Continentalness);
    };
};

struct BiomeConfig
{
    std::string Id = "";
    BiomeClimatePoint ClimatePoint;
    std::vector<NoiseLayerConfig> NoiseLayers;
    std::vector<CurvePoint> OffsetCurve;
    std::vector<CurvePoint> ScaleCurve;
    std::vector<BlockLayerConfig> BlockLayers;
    std::vector<NoisePair> NoisePairs;

    struct glaze
    {
        using T = BiomeConfig;
        static constexpr auto value = glz::object(
            &T::Id, &T::ClimatePoint, &T::NoiseLayers,
            &T::OffsetCurve, &T::ScaleCurve, &T::BlockLayers);
    };
};