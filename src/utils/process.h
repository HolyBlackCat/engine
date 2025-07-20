#pragma once

#include "em/zstring_view.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_process.h>

#include <functional>
#include <memory>
#include <string_view>
#include <string>

namespace em
{
    // Runs a process.
    // This can optionally redirect process output to a callback, which does NOT run in a separate thread, and instead runs in response to calling
    //   some of the status-checking functions below.
    // This has to be designed in a particular way, because the running process can block if we don't periodically consume its output. So we have to do that,
    //   without waiting for the process to finish first.
    class Process
    {
      public:
        // This is here just for completeness. This counts the logical cores (as it should).
        [[nodiscard]] static int NumCpuCores();

        using OutputCallback = std::function<void(std::string_view data)>;

        [[nodiscard]] static OutputCallback WriteOutputToString(std::shared_ptr<std::string> target, std::size_t max_bytes)
        {
            return [target = std::move(target), remaining_bytes = max_bytes](std::string_view data) mutable
            {
                if (remaining_bytes == 0)
                    return;
                if (remaining_bytes < data.size())
                    data.remove_suffix(data.size() - remaining_bytes);
                *target += data;
                remaining_bytes -= data.size();
            };
        }

      private:
        struct State
        {
            SDL_Process *handle = nullptr;

            // The command line, escaped and prepared for debug printing.
            std::string debug_cmdline;

            // If not null, the process has finished running and we known about it.
            std::optional<int> exit_code;

            // The output stream. If null, it means we didn't redirect the output.
            SDL_IOStream *output_stream = nullptr;

            // The output callback. If null, it means we didn't redirect the output.
            OutputCallback output_callback;
        };
        State state;

        void CheckOrWait(bool wait);

      public:
        constexpr Process() {}

        // If a non-null `output_callback` is passed, it will receive all stdout and stderr output from the process.
        // `output_callback` is always called synchronously in the same thread, when some of the functions below are called to check the process status.
        // If `output_callback` is not specified, the process stdout and stderr are not redirected.
        // In this overload, `argv` is required to have a null element at the end.
        explicit Process(const char *const *argv, OutputCallback output_callback = nullptr);
        explicit Process(std::span<const zstring_view> args, OutputCallback output_callback = nullptr);
        explicit Process(std::span<const std::string_view> args, OutputCallback output_callback = nullptr);
        explicit Process(std::span<const std::string> args, OutputCallback output_callback = nullptr);

        Process(Process &&other) noexcept
            : state(std::move(other.state))
        {
            other.state = {};
        }
        Process &operator=(Process other) noexcept
        {
            std::swap(state, other.state);
            return *this;
        }

        // This calls `Kill(true)`, and then does NOT block to wait for the process to complete.
        ~Process();


        [[nodiscard]] explicit operator bool() const {return bool(state.handle);}

        // Throws if this is a null instance.
        void ThrowIfNull() const
        {
            if (!*this)
                throw std::runtime_error("This process wrapper is null.");
        }


        // Returns the command line used to start this process, in a debug format not suitable for actually running it.
        // Returns empty if this is a null instance.
        [[nodiscard]] zstring_view GetDebugCommandLine() const
        {
            return state.debug_cmdline;
        }


        // Calling this on a process that's already finished (or is just null) is a no-op.
        // Doesn't zero this instance, you can still query the information about the process.
        void Kill(bool force);

        // Zeroes this instance without stopping the process. Does nothing if already null.
        void Detach();


        // Blocks until the process finishes.
        void WaitUntilFinished() {CheckOrWait(true);}

        // Checks the current process state, returns true if it has finished.
        [[nodiscard]] bool CheckIfFinished() {CheckOrWait(false); return KnownToBeFinished();}

        // Returns true if we know the process has finished. Doesn't actually check the process,
        //   but simply returns true if we have called `WaitUntilFinished()` before, or if `CheckIfFinished()` returned true before.
        [[nodiscard]] bool KnownToBeFinished() const {return bool(state.exit_code);}

        // Throws if not `KnownToBeFinished()`.
        void ThrowIfNotKnownToBeFinished() const
        {
            ThrowIfNull();
            if (!KnownToBeFinished())
                throw std::runtime_error("This process hasn't finished running yet, or its status wasn't updated.");
        }


        // The process exit code, or throws if not `KnownToBeFinished()`.
        // Non-negative code if terminated normally, a negative signal if it terminated due to a signal, or -255 if terminated for another reason.
        // This is how `SDL_WaitProcess()` reports things.
        [[nodiscard]] int ExitCode() const
        {
            ThrowIfNotKnownToBeFinished();
            return state.exit_code.value(); // This should never throw.
        }
    };
}
