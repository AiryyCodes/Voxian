#include "Texture.h"
#include "Logger.h"
#include "Renderer/Mesh.h"

Texture::Texture()
{
    glGenTextures(1, &m_Id);
}

Texture::~Texture()
{
    LOG_INFO("Deleting texture with ID: {}", m_Id);

    glDeleteTextures(1, &m_Id);
}

void Texture::Init(int width, int height, const void *data, int format, int internalFormat, int type)
{
    m_Width = width;
    m_Height = height;

    glGenTextures(1, &m_Id);
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