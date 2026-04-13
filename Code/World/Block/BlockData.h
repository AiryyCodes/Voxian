#pragma once

#include "BlockProperties.h"
#include "Renderer/Model/Model.h"

#include <string>
#include <glaze/glaze.hpp>
#include <unordered_map>

struct BlockData
{
    std::string Name;
    std::string Id;

    std::string Model;
    std::unordered_map<std::string, std::string> Textures;

    BlockProperties Properties;

    void ResolveTextures();
    std::unordered_map<std::string, std::string> ResolveTextureVariables(const struct Model &model) const;

    bool Validate() const;

    struct glaze
    {
        using T = BlockData;
        static constexpr auto value = glz::object(
            "Name", &T::Name,
            "Id", &T::Id,
            "Model", &T::Model,
            "Textures", &T::Textures,
            "Properties", &T::Properties);
    };
};