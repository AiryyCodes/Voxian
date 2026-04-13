#include "BlockData.h"
#include "BlockRegistry.h"
#include "Engine.h"
#include "Logger.h"
#include "Renderer/Model/Model.h"

void BlockData::ResolveTextures()
{
    BlockRegistry &registry = EngineContext::GetBlockRegistry();

    for (auto &[face, texturePath] : Textures)
    {
        if (registry.IsIdRegistered(texturePath))
            continue; // already a resolved/registered path

        std::string resolvedPath = "Assets/Textures/" + texturePath + ".png";
        if (std::filesystem::exists(resolvedPath))
        {
            texturePath = resolvedPath;
        }
        else
        {
            LOG_ERROR("Texture file not found for block '{}' face '{}': {}", Id, face, resolvedPath);
            texturePath = "Assets/Textures/null.png";
        }
    }
}

std::unordered_map<std::string, std::string> BlockData::ResolveTextureVariables(const struct Model &model) const
{
    std::unordered_map<std::string, std::string> vars;

    for (const auto &elem : model.elements)
        for (const auto &[faceName, face] : elem.Faces)
            vars[face.Texture] = "";

    for (auto &[variable, path] : vars)
    {
        const std::string key = variable.substr(1); // strip "#"
        auto it = Textures.find(key);
        if (it != Textures.end())
        {
            path = it->second; // already resolved by ResolveTextures()
        }
        else
        {
            LOG_ERROR("No texture entry for variable '{}' in block '{}'", variable, Id);
            path = "Assets/Textures/null.png";
        }
    }

    return vars;
}

bool BlockData::Validate() const
{
    if (Name.empty())
    {
        LOG_ERROR("Block has no Name");
        return false;
    }

    if (Id.empty())
    {
        LOG_ERROR("Block has no Id");
        return false;
    }

    if (Model.empty())
    {
        LOG_ERROR("Block {} has no Model", Id);
    }

    if (!Properties.Validate())
    {
        LOG_ERROR("Properties are invalid");
        return false;
    }

    return true;
}