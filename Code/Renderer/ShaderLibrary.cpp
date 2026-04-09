#include "Renderer/ShaderLibrary.h"
#include "Util/Memory.h"
#include "Renderer/Shader.h"

#include <cassert>
#include <string>
#include <utility>

void ShaderLibrary::Init()
{
    AddShader(Shaders::Main, "Assets/Shaders/main");
    AddShader(Shaders::Chunk, "Assets/Shaders/chunk");
}

Shader &ShaderLibrary::Get(const std::string &name)
{
    auto it = m_Shaders.find(name);
    assert(it != m_Shaders.end() && "Shader not found!");
    return *it->second;
}

void ShaderLibrary::AddShader(const std::string &name, const std::string basePath)
{
    Scope<Shader> shader = CreateScope<Shader>(basePath);
    m_Shaders[name] = std::move(shader);
}
