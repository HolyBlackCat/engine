#pragma once

#include "time/clock.h"

namespace em::Time
{
    // Measures time intervals.
    class DeltaTimer
    {
        Ticks time = 0;

      public:
        // This does nothing. We don't always want to record the time here.
        constexpr DeltaTimer() {}

        // The delta since the last call in ticks.
        [[nodiscard]] Ticks CountTicks()
        {
            Ticks new_time = Time();
            Ticks delta = new_time - time;
            time = new_time;
            return delta;
        }

        // The delta since the last call in seconds.
        [[nodiscard]] Seconds CountSeconds()
        {
            return TicksToSeconds(CountTicks());
        }

        // The absolute time point of the last call, measured in ticks.
        [[nodiscard]] Ticks LastTimePointTicks() const
        {
            return time;
        }
    };
}
