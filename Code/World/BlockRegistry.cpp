#include "World/BlockRegistry.h"
#include "Graphics/Model.h"
#include "World/Block.h"
#include "Logger.h"
#include "Memory.h"
#include <cstdint>
#include <filesystem>

#include <fstream>
#include <nlohmann/json.hpp>
#include <string_view>
#include <unordered_set>

void BlockRegistry::Init()
{
    LOG_INFO("Initializing BlockRegistry...");

    // 1. Load blockstate JSONs
    for (auto &state : m_States)
    {
        LoadBlockStateJSON(state);
    }

    // 2. Collect all unique texture paths
    std::unordered_set<std::string> uniqueTextures;
    for (const auto &pair : m_Models)
    {
        const Model &model = pair.second;
        for (const auto &texPair : model.Textures)
        {
            uniqueTextures.insert("Assets/textures/" + texPair.second + ".png");
        }
    }

    // 3. Create texture array
    std::vector<std::string> texturePaths(uniqueTextures.begin(), uniqueTextures.end());

    int layerIndex = 0;
    for (const auto &path : texturePaths)
    {
        std::string texName = std::filesystem::path(path).stem().string();

        m_TextureNameToLayer[texName] = layerIndex++;
    }
    m_Texture = CreateRef<TextureArray2D>(texturePaths, 1024, 1024);

    LOG_INFO("BlockRegistry initialized: {} blocks, {} models, {} textures",
             m_States.size(), m_Models.size(), texturePaths.size());
}

void BlockRegistry::LoadBlockStateJSON(BlockState &state)
{
    if (state.GetName() == "air")
    {
        LOG_INFO("Skipping model loading for air block");
        return;
    }

    std::string path = "Assets/blockstates/" + state.GetName() + ".json";

    if (!std::filesystem::exists(path))
    {
        // No JSON -> assume default model name = block name
        Model model = LoadModelJSON("block/" + state.GetName());
        state.SetModel(model);
        return;
    }

    std::ifstream file(path);
    nlohmann::json json;
    file >> json;

    std::string modelName = json.value("model", state.GetName());
    Model model = LoadModelJSON(modelName);
    state.SetModel(model);
}

Model BlockRegistry::LoadModelJSON(const std::string &modelName)
{
    // Already loaded?
    auto it = m_Models.find(modelName);
    if (it != m_Models.end())
        return it->second;

    std::string path = "Assets/models/" + modelName + ".json";

    if (!std::filesystem::exists(path))
    {
        LOG_WARN("Model JSON not found: {}", path);
        return {};
    }

    std::ifstream file(path);
    nlohmann::json json;
    file >> json;

    Model model;

    // 1️⃣ Load parent/extends first
    std::string parentName;
    if (json.contains("parent"))
        parentName = json["parent"];
    else if (json.contains("extends"))
        parentName = json["extends"];

    if (!parentName.empty())
    {
        Model parentModel = LoadModelJSON(parentName);
        model.Elements = parentModel.Elements;
        model.Textures = parentModel.Textures;
    }

    // 2️⃣ Load/override child textures
    if (json.contains("textures"))
    {
        for (auto &texPair : json["textures"].items())
        {
            model.Textures[texPair.key()] = texPair.value();
        }
    }

    // 3️⃣ Load child elements
    if (json.contains("elements"))
    {
        for (auto &elemJson : json["elements"])
        {
            Element elem;
            elem.From = elemJson["from"].get<std::vector<float>>();
            elem.To = elemJson["to"].get<std::vector<float>>();

            if (elemJson.contains("rotation"))
            {
                auto rot = elemJson["rotation"];
                elem.Rotation = {
                    rot.value("angle", 0.0f),
                    rot["origin"][0].get<float>(),
                    rot["origin"][1].get<float>(),
                    rot["origin"][2].get<float>()};
            }

            if (elemJson.contains("faces"))
            {
                for (auto &facePair : elemJson["faces"].items())
                {
                    Face f;
                    f.UV = facePair.value().value("uv", std::vector<float>{0, 0, 16, 16});

                    // Store raw texture reference (#key or plain name)
                    f.Texture = facePair.value().value("texture", "#missing");

                    // Handle "all" key
                    if (facePair.key() == "all")
                    {
                        static const std::vector<std::string> faces = {"up", "down", "north", "south", "east", "west"};
                        for (const std::string &name : faces)
                            elem.Faces[name] = f;
                    }
                    else
                    {
                        elem.Faces[facePair.key()] = f;
                    }
                }
            }

            model.Elements.push_back(elem);
        }
    }

    m_Models[modelName] = model;
    return model;
}

uint16_t BlockRegistry::Register(BlockState state)
{
    uint16_t id = m_States.size();
    state.m_Id = id;

    m_States.push_back(state);
    m_NameToId[state.GetName()] = id;

    // Create Block object pointing to default state
    Ref<Block> block = CreateRef<Block>(state.GetName());
    block->m_DefaultStateId = id;
    m_Blocks[state.GetName()] = block;

    return id;
}

int BlockRegistry::GetTextureLayer(std::string_view texName, int fallback)
{
    if (!texName.empty() && texName.front() == '#')
    {
        texName = texName.substr(1);
    }

    std::string lookupKey(texName);

    int layer = fallback;
    auto it = g_BlockRegistry.m_TextureNameToLayer.find(lookupKey);
    if (it != g_BlockRegistry.m_TextureNameToLayer.end())
        layer = it->second;

    return layer;
}
