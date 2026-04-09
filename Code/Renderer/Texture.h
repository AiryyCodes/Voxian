#pragma once

#include <glad/gl.h>
#include <string>
#include <vector>

class Texture
{
public:
    Texture();
    ~Texture();

    void Init(int width, int height, const void *data, int format = GL_RGBA, int internalFormat = GL_RGBA8, int type = GL_UNSIGNED_BYTE);
    virtual void Init(int width, int height, int layers, const std::vector<std::string> &fileNames, int format = GL_RGBA, int internalFormat = GL_RGBA8, int type = GL_UNSIGNED_BYTE) {}

    virtual void Bind(unsigned int slot = 0) const;

    unsigned int GetId() const { return m_Id; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    unsigned int m_Id = 0;

    int m_Width = 0;
    int m_Height = 0;
};

class TextureArray2D : public Texture
{
public:
    TextureArray2D();
    ~TextureArray2D();

    void Init(int width, int height, int layers, const std::vector<std::string> &fileNames, int format = GL_RGBA, int internalFormat = GL_RGBA8, int type = GL_UNSIGNED_BYTE) override;
    void Bind(unsigned int slot = 0) const override;

    unsigned int GetId() const { return m_Id; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetLayers() const { return m_Layers; }

private:
    unsigned int m_Id = 0;

    int m_Width = 0;
    int m_Height = 0;
    int m_Layers = 0;
};