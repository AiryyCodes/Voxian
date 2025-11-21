#include "Graphics/Texture.h"
#include "Logger.h"

#include <glad/gl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

TextureArray2D::TextureArray2D(std::vector<std::string> paths, int maxWidth, int maxHeight)
    : m_NumLayers(paths.size()), m_MaxWidth(maxWidth), m_MaxHeight(maxHeight)
{
    glGenTextures(1, &m_Id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Id);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16, maxWidth, maxHeight, m_NumLayers);

    for (int i = 0; i < paths.size(); i++)
    {
        int width;
        int height;
        int numChannels;
        unsigned char *data = stbi_load(paths[i].c_str(), &width, &height, &numChannels, 0);

        if (data)
        {
            GLenum format;
            if (numChannels == 1)
                format = GL_RED;
            else if (numChannels == 3)
                format = GL_RGB;
            else if (numChannels == 4)
                format = GL_RGBA;

            m_Sizes.push_back(Vector2i(width, height));

            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            LOG_ERROR("Failed to load texture: {}", paths[i]);
        }
    }
}

void TextureArray2D::Bind(const Shader &shader)
{
    shader.SetUniform("u_TexArray", 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Id);
}

void TextureArray2D::Unbind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

int TextureArray2D::GetWidth(int layer)
{
    if (layer >= m_Sizes.size())
        return 0;

    return m_Sizes[layer].x;
}

int TextureArray2D::GetHeight(int layer)
{
    if (layer >= m_Sizes.size())
        return 0;

    return m_Sizes[layer].y;
}
