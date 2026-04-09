#pragma once

#include "Util/Memory.h"
#include "Renderer/Shader.h"

#include <string>
#include <unordered_map>

namespace Shaders
{
constexpr const char *Main = "main";
constexpr const char *Chunk = "chunk";
} // namespace Shaders

class ShaderLibrary
{
public:
    void Init();

    Shader &Get(const std::string &name);

private:
    void AddShader(const std::string &name, const std::string basePath);

private:
    std::unordered_map<std::string, Scope<Shader>> m_Shaders;
};
