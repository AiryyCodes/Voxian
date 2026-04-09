#include "BlockData.h"
#include "BlockRegistry.h"
#include "Engine.h"
#include "Logger.h"

void BlockData::ResolveTextures()
{
    // Get the texture path from the current Texture field, or default to "null" if it's empty
    std::string texturePath = Texture.empty() ? "null" : Texture;

    // Check if the texture path is already in the registry
    BlockRegistry &registry = EngineContext::GetBlockRegistry();
    if (registry.IsIdRegistered(texturePath))
    {
        // If it is, we can just use the existing texture
        Texture = texturePath;
    }
    else
    {
        // Otherwise, we need to resolve it to a valid texture path
        std::string resolvedPath = "Assets/Textures/" + texturePath + ".png";
        if (std::filesystem::exists(resolvedPath))
        {
            Texture = resolvedPath;
        }
        else
        {
            LOG_ERROR("Texture file not found for block '{}': {}", Id, resolvedPath);
            Texture = "Assets/Textures/null.png"; // Fallback to null texture if not found
        }
    }
}

bool BlockData::Validate() const
{
    if (Name.empty())
    {
        LOG_ERROR("BlockData validation failed: Name is empty");
        return false;
    }

    if (Id.empty())
    {
        LOG_ERROR("BlockData validation failed: Id is empty");
        return false;
    }

    if (!Properties.Validate())
    {
        LOG_ERROR("BlockData validation failed: Properties are invalid");
        return false;
    }

    return true;
}