#include "Texture.h"
#include "Logger.h"

#include <stb/stb_image.h>

Texture::Texture()
{
    glGenTextures(1, &m_Id);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_Id);
}

void Texture::Init(int width, int height, const void *data, int format, int internalFormat, int type)
{
    m_Width = width;
    m_Height = height;

    glBindTexture(GL_TEXTURE_2D, m_Id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);

    // Set default filtering and wrapping modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_Id);
}

TextureArray2D::TextureArray2D()
{
    glGenTextures(1, &m_Id);
}

TextureArray2D::~TextureArray2D()
{
    glDeleteTextures(1, &m_Id);
}

void TextureArray2D::Init(int width, int height, int layers, const std::vector<std::string> &fileNames, int format, int internalFormat, int type)
{
    m_Width = width;
    m_Height = height;
    m_Layers = layers;

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Id);

    // Allocate storage for the texture array
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, layers, 0, format, type, nullptr);

    for (int i = 0; i < layers; ++i)
    {
        // Load texture from file
        int texWidth, texHeight, texChannels;
        unsigned char *data = stbi_load(fileNames[i].c_str(), &texWidth, &texHeight, &texChannels, 0);

        if (data)
        {
            LOG_INFO("Loaded texture '{}' with width: {}, height: {}, channels: {}", fileNames[i], texWidth, texHeight, texChannels);

            int texFormat = (texChannels == 4) ? GL_RGBA : GL_RGB;

            // Upload texture data to the appropriate layer
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, texWidth, texHeight, 1, texFormat, type, data);

            stbi_image_free(data);
        }
        else
        {
            LOG_ERROR("Failed to load texture '{}'", fileNames[i]);
        }
    }

    // Set default filtering and wrapping modes
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void TextureArray2D::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Id);
}