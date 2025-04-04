#pragma once

typedef struct SDL_GPUCopyPass SDL_GPUCopyPass;

namespace em::Gpu
{
    class CommandBuffer;

    class CopyPass
    {
        struct State
        {
            SDL_GPUCopyPass *pass = nullptr;
        };
        State state;

      public:
        constexpr CopyPass() {}

        CopyPass(CommandBuffer &command_buffer);

        CopyPass(CopyPass &&other) noexcept;
        CopyPass &operator=(CopyPass other) noexcept;
        ~CopyPass();

        [[nodiscard]] explicit operator bool() const {return bool(state.pass);}
        [[nodiscard]] SDL_GPUCopyPass *Handle() {return state.pass;}
    };
}
