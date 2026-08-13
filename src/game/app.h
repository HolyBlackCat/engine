#pragma once

#include "graphics/renderer_2d.h"
#include "mainloop/app_state.h"

using namespace em;

struct BasicGameState : App::BasicState<BasicGameState>
{
    virtual ~BasicGameState() = default;
    virtual void Tick() = 0;
    virtual void Render(Graphics::Renderer2d &r) = 0;
};
