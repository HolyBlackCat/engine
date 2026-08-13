#pragma once

#include <cmath>

#include "time/clock.h"

namespace em::Time
{
    // Maintains a fixed tick rate.
    // Use together with `DeltaTimer`, and each frame do this:
    //   metronome.BeginFrame(delta_timer.CountTicks());
    //   while (metronome.Tick())
    //       MyTick();
    class Metronome
    {
        // Desired tick length, measured in clock ticks.
        Ticks tick_len = 0;
        // Max ticks per frame. <1 = no limit.
        int max_ticks = 0;

        // Accumulates clock ticks, which we then spend on our own ticks.
        Ticks accumulator = 0;
        // If this is true, we're between frames now.
        bool new_frame = false;
        // This is set to true whenever the `max_ticks` limit is reached.
        bool lag = false;

        // Compensation logic. This makes the clock run more smoothly.
        // When `accumulator` is very close to `tick_len` (absolute difference less than `comp_th * tick_len`),
        // it's adjusted by offsetting it by `+-comp_amount * tick_len`.
        // The adjustement direction is determined by `comp_dir`.
        float comp_th = 0;
        float comp_amount = 0;
        int comp_dir = 0; // 1 = forward, -1 = backward, 0 = away from `tick_len`.

      public:
        // Tick counter.
        Ticks ticks = 0;

        Metronome() {} // For any other constructor to work, the clock has to be initialized first.

        Metronome(float freq, int max_ticks_per_frame = 8, float compensation_threshold = 0.01f, float compensation_amount = 0.5f)
        {
            SetFrequency(freq);
            SetMaxTicksPerFrame(max_ticks_per_frame);
            SetCompensation(compensation_threshold, compensation_amount);
            Reset();
        }

        void SetFrequency(float freq)
        {
            tick_len = Ticks(TicksPerSecond() / freq);
        }
        // Set to 0 negative to disable the limit.
        void SetMaxTicksPerFrame(int n)
        {
            max_ticks = n;
        }

        // Threshold should be positive and small, at least less than 1.
        // Amount should be at least two times larger (by some margin) than threshold, otherwise it will break. 0.5 should give best results, but don't make it much larger.
        // When `abs(accumulator - tick_len) / tick_len < threshold`, the compensator kicks in and adds or subtracts `amount * tick_len` from the time.
        void SetCompensation(float threshold, float amount)
        {
            comp_th = threshold;
            comp_amount = amount;
        }
        void Reset()
        {
            accumulator = 0;
            new_frame = true;
            lag = false;
            comp_dir = 0;
            ticks = 0;
        }

        [[nodiscard]] bool Lag() // Flag resets after this function is called. The flag is set to 1 if the amount of ticks per last frame is at maximum value.
        {
            if (lag)
            {
                lag = false;
                return true;
            }
            return false;
        }

        [[nodiscard]] float Frequency() const
        {
            return float(TicksPerSecond() / double(tick_len));
        }

        [[nodiscard]] Ticks ClockTicksPerTick() const
        {
            return tick_len;
        }

        [[nodiscard]] int MaxTicksPerFrame() const
        {
            return max_ticks;
        }

        void BeginFrame(Ticks delta)
        {
            accumulator += delta;
        }

        [[nodiscard]] bool Tick()
        {
            // Compensate.
            if (std::abs(std::make_signed_t<Ticks>(accumulator - tick_len)) < tick_len * comp_th)
            {
                int dir;
                if (comp_dir)
                    dir = -comp_dir;
                else
                    dir = (accumulator < tick_len ? -1 : 1); // Away from `tick_len`.

                comp_dir += dir;
                accumulator += Ticks(tick_len * comp_amount * dir);
            }

            // Decide whether to tick.
            if (accumulator >= tick_len)
            {
                // Do tick.

                // Check for lag first.
                if (max_ticks > 0)
                {
                    Ticks max_allowed_accum = tick_len * Ticks(max_ticks);
                    if (accumulator > max_allowed_accum)
                    {
                        accumulator = max_allowed_accum;
                        lag = true;
                    }
                }

                accumulator -= tick_len;
                new_frame = false;
                ticks++;
                return true;
            }
            else
            {
                // Don't need more ticks.
                new_frame = true;
                return false;
            }
        }

        // The fractional time point inside of the current frame. Use this after `Tick()` returns true.
        [[nodiscard]] float TimeFrac() const
        {
            return float(accumulator / double(tick_len));
        }
    };

}
