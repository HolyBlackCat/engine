#pragma once

#include <SDL3/SDL_timer.h>

#include <cstdint>

namespace em::Time
{
    using Ticks = std::uint64_t;
    using Seconds = float;

    [[nodiscard]] inline Ticks Time()
    {
        // I had stuttering issues with the performance counter
        // return SDL_GetPerformanceCounter();
        return SDL_GetTicksNS();
    }

    [[nodiscard]] inline Ticks TicksPerSecond()
    {
        // static Ticks ret = SDL_GetPerformanceFrequency();
        // return ret;
        return 1'000'000'000;
    }

    [[nodiscard]] inline Ticks SecondsToTicks(Seconds secs)
    {
        return Ticks(secs * TicksPerSecond());
    }
    [[nodiscard]] inline Seconds TicksToSeconds(Ticks ticks)
    {
        return ticks / Seconds(TicksPerSecond());
    }

    // This achieves more precision by adding a busy loop after `SDL_DelayNS()`, from what I understand.
    inline void WaitTicks(Ticks ticks)
    {
        SDL_DelayPrecise(ticks);
    }
    inline void WaitTicksApprox(Ticks ticks)
    {
        SDL_DelayNS(ticks);
    }

    inline void WaitSeconds(Seconds secs)
    {
        WaitTicks(SecondsToTicks(secs));
    }
    inline void WaitSecondsApprox(Seconds secs)
    {
        WaitTicksApprox(SecondsToTicks(secs));
    }
}
