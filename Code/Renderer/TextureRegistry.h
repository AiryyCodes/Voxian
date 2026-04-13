#pragma once

#include "Util/Memory.h"
#include "Renderer/Texture.h"

#include <stdexcept>
#include <string>
#include <unordered_map>

class TextureRegistry
{
public:
    // Register a texture name and assign it the next layer index.
    int Register(const std::string &name)
    {
        auto it = m_NameToLayer.find(name);
        if (it != m_NameToLayer.end())
            return it->second;

        int layer = static_cast<int>(m_FileNames.size());
        m_NameToLayer[name] = layer;
        m_FileNames.push_back(name);
        return layer;
    }

    // Call once after all models have been registered.
    // width/height must match all textures in the array.
    void Build(int width, int height)
    {
        m_Array = CreateRef<TextureArray2D>();
        m_Array->Init(width, height, static_cast<int>(m_FileNames.size()), m_FileNames);
    }

    int GetLayer(const std::string &name) const
    {
        auto it = m_NameToLayer.find(name);
        if (it == m_NameToLayer.end())
            throw std::runtime_error("Texture not registered: " + name);
        return it->second;
    }

    void Bind(unsigned int slot = 0) const { m_Array->Bind(slot); }

    Ref<TextureArray2D> GetArray() const { return m_Array; }

    int GetLayerCount() const { return static_cast<int>(m_FileNames.size()); }

private:
    Ref<TextureArray2D> m_Array;
    std::vector<std::string> m_FileNames;
    std::unordered_map<std::string, int> m_NameToLayer;
};