#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct Face
{
    std::vector<float> UV;
    std::string Texture;
};

struct Element
{
    std::vector<float> From;
    std::vector<float> To;
    std::unordered_map<std::string, Face> Faces;
    std::vector<float> Rotation;
};

struct Model
{
    std::vector<Element> Elements;
    std::unordered_map<std::string, std::string> Textures;
};
