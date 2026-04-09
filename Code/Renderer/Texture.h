#pragma once

#include <glad/gl.h>

class Texture
{
public:
    Texture();
    ~Texture();

    void Init(int width, int height, const void *data, int format = GL_RGBA, int internalFormat = GL_RGBA8, int type = GL_UNSIGNED_BYTE);
    void Bind(unsigned int slot = 0) const;

    unsigned int GetId() const { return m_Id; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    unsigned int m_Id = 0;

    int m_Width = 0;
    int m_Height = 0;
};