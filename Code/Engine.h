#pragma once

#include "Renderer/Renderer.h"
#include "Input.h"
#include "Util/Memory.h"
#include "Window.h"
#include "World/Block/BlockRegistry.h"
#include "World/World.h"

#include <cassert>

class Engine
{
public:
    bool Init();

    bool IsRunning() const;

    Renderer &GetRenderer() { return m_Renderer; }
    Window &GetWindow() { return *m_Window; }
    Input &GetInput() { return *m_Input; }

    BlockRegistry &GetBlockRegistry() { return m_BlockRegistry; }
    World &GetWorld() { return m_World; }

private:
    Renderer m_Renderer;

    Scope<Window> m_Window;
    Scope<Input> m_Input;

    BlockRegistry m_BlockRegistry;
    World m_World;
};

class EngineContext
{
public:
    static void Init(Engine *engine)
    {
        assert(engine != nullptr && "Attempted to init with null engine!");
        assert(s_Engine == nullptr && "EngineContext already initialized!");
        s_Engine = engine;
    }

    static void Shutdown()
    {
        s_Engine = nullptr;
    }

    static Renderer &GetRenderer() { return Get().GetRenderer(); }
    static Window &GetWindow() { return Get().GetWindow(); }
    static Input &GetInput() { return Get().GetInput(); }

    static BlockRegistry &GetBlockRegistry() { return Get().GetBlockRegistry(); }
    static World &GetWorld() { return Get().GetWorld(); }

    static Engine &Get()
    {
        assert(s_Engine && "EngineContext accessed after shutdown!");
        return *s_Engine;
    }

private:
    static inline Engine *s_Engine = nullptr;
};
