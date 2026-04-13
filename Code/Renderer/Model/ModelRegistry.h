#pragma once

#include "BakedModel.h"
#include "Model.h"
#include "Renderer/TextureRegistry.h"
#include "Util/Direction.h"

class ModelRegistry
{
public:
    // Pass in textureVariables per block: "#North" -> "textures/stone.png"
    const BakedModel &Load(
        const std::string &name,
        const Model &model,
        const std::unordered_map<std::string, std::string> &textureVariables,
        TextureRegistry &textures)
    {
        auto it = m_Models.find(name);
        if (it != m_Models.end())
            return it->second;

        BakedModel baked = Bake(model, textureVariables, textures);
        baked.Name = name;

        auto [ins, _] = m_Models.emplace(name, std::move(baked));
        return ins->second;
    }

    const BakedModel *Get(const std::string &name) const
    {
        auto it = m_Models.find(name);
        return it != m_Models.end() ? &it->second : nullptr;
    }

private:
    BakedModel Bake(
        const Model &model,
        const std::unordered_map<std::string, std::string> &textureVariables,
        TextureRegistry &textures)
    {
        BakedModel baked;

        for (const auto &elem : model.elements)
        {
            BakedElement bakedElem;
            bakedElem.Name = elem.Name;

            const Vector3f origin = Vector3f(elem.From) / 16.0f;
            const Vector3f size = Vector3f(elem.To - elem.From) / 16.0f;

            for (const auto &[faceName, face] : elem.Faces)
            {
                BakedFace bakedFace;

                const Direction dir = FaceNameToDirection(faceName);
                const FaceIndices &fi = s_FaceIndicesMap.at(dir);
                bakedFace.NormalIndex = s_NormalIndexMap.at(dir);

                for (int i = 0; i < 4; ++i)
                {
                    const int vi = (&fi.v0)[i];
                    bakedFace.Positions[i] = origin + Vector3f(s_Corners[vi]) * size;
                }

                bakedFace.UVs = {
                    face.UV.x / 16.0f, // uMin
                    face.UV.y / 16.0f, // vMin
                    face.UV.z / 16.0f, // uMax
                    face.UV.w / 16.0f, // vMax
                };

                auto varIt = textureVariables.find(face.Texture);
                if (varIt == textureVariables.end())
                    throw std::runtime_error("Unresolved texture variable: " + face.Texture);

                bakedFace.TextureLayer = textures.Register(varIt->second);
                bakedElem.Faces.push_back(std::move(bakedFace));
            }

            baked.Elements.push_back(std::move(bakedElem));
        }

        return baked;
    }

    Direction FaceNameToDirection(const std::string &name)
    {
        static const std::unordered_map<std::string, Direction::Value> TABLE = {
            {"Up", Direction::Value::Up},
            {"Down", Direction::Value::Down},
            {"North", Direction::Value::Forward},
            {"South", Direction::Value::Backward},
            {"West", Direction::Value::Left},
            {"East", Direction::Value::Right},
        };
        auto it = TABLE.find(name);
        if (it == TABLE.end())
            throw std::runtime_error("Unknown face: " + name);
        return Direction(it->second);
    }

    std::unordered_map<std::string, BakedModel> m_Models;
};