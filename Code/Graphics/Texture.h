#pragma once

#include "Graphics/Shader.h"
#include "Math/Vector.h"

#include <string>
#include <vector>

class TextureArray2D
{
public:
    TextureArray2D(std::vector<std::string> paths, int maxWidth, int maxHeight);

    void Bind(const Shader &shader);
    void Unbind();

    int GetWidth(int layer);
    int GetHeight(int layer);

    int GetMaxWidth() { return m_MaxWidth; }
    int GetMaxHeight() { return m_MaxHeight; }

private:
    unsigned int m_Id = 0;

    int m_NumLayers = 0;

    int m_MaxWidth = 0;
    int m_MaxHeight = 0;

    std::vector<Vector2i> m_Sizes;
};
