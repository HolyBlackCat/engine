#include "app.h"

using namespace em;

namespace States
{
    struct Game : BasicGameState
    {
        EM_REFL()

        void Tick() override
        {

        }

        void Render(Graphics::Renderer2d &r) override
        {
            Graphics::Renderer2d::Vertex verts[3] = {
                Graphics::Renderer2d::Vertex(fvec2(), fvec3(1, 0, 0)),
                Graphics::Renderer2d::Vertex(fvec2(32, 0), fvec3(0, 1, 0)),
                Graphics::Renderer2d::Vertex(fvec2(0, 32), fvec3(0, 0, 1)),
            };
            r.DrawVertices(verts);
        }
    };
}
