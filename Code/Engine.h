#pragma once

#include "Memory.h"
#include "Renderer/Renderer.h"
#include "Window.h"
#include <cassert>

class Engine
{
public:
    bool Init();

    bool IsRunning() const;

    Renderer &GetRenderer() { return m_Renderer; }

    Window &GetWindow() { return *m_Window; }

private:
    Renderer m_Renderer;

    Scope<Window> m_Window;
};

class EngineContext
{
public:
    static void Init(Engine *engine)
    {
        assert(engine != nullptr && "Attempted to init with null engine!");
        assert(m_Engine == nullptr && "EngineContext already initialized!");
        m_Engine = engine;
    }

    static void Shutdown()
    {
        m_Engine = nullptr;
    }

    static Engine &Get()
    {
        assert(m_Engine && "EngineContext accessed after shutdown!");
        return *m_Engine;
    }

private:
    static inline Engine *m_Engine = nullptr;
};
