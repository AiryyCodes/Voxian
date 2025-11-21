#include "World/Block.h"
#include "Logger.h"
#include <filesystem>

void BlockRegistry::Init()
{
    std::vector<std::string> paths;
    paths.reserve(m_States.size() * 6);

    const std::string fallbackTexture = "Assets/Textures/null.png";

    for (const auto &state : m_States)
    {
        std::string basePath = "Assets/Textures/" + state.GetName() + ".png";
        std::string texPath = std::filesystem::exists(basePath) ? basePath : fallbackTexture;

        LOG_INFO("Loading block texture '{}'", texPath);

        for (int i = 0; i < 6; ++i)
            paths.emplace_back(texPath);
    }

    m_Texture = CreateRef<TextureArray2D>(paths, 1024, 1024);
}
