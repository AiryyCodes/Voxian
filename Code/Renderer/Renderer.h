#pragma once

#include "Window.h"

class Renderer
{
public:
    ~Renderer();

    bool Init(const Window &window);

    void SwapBuffers() const;
};
