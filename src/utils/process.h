#pragma once

#include "em/zstring_view.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_process.h>

#include <cstddef>
#include <functional>
#include <initializer_list>
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

        // See `InputCallback` below.
        // Writes `data`, returns the number of bytes successfully written. Never blocks.
        using WriteFunc = std::function<std::size_t(std::string_view data)>;
        // Emits process input.
        // This might be called multiple times.
        // Call `write()` as many times as you like (makes more sense to do less calls with larger buffers).
        // `write` never blocks, and returns the number of bytes successfully written. If it returns less than what you passed,
        //   it's a good sign that you should return for now, and try writing those bytes again the next time your callback is called.
        // Your callback should return `true` when it doesn't want to write anything else, that closes the stream.
        using InputCallback = std::function<bool(WriteFunc write)>;

        // Receives process output.
        // This might be called multiple times with parts of the output.
        using OutputCallback = std::function<void(std::string_view data)>;

        struct Params
        {
            InputCallback input = nullptr;
            OutputCallback output = nullptr;
        };

        [[nodiscard]] static InputCallback InputFromString(std::string input)
        {
            return [input = std::move(input), pos = 0zu](WriteFunc write) mutable -> bool
            {
                std::string_view substr = std::string_view(input).substr(pos);
                std::size_t bytes_written = write(substr);
                pos += bytes_written;
                return pos == input.size();
            };
        }

        [[nodiscard]] static OutputCallback OutputToString(std::shared_ptr<std::string> target, std::size_t max_bytes)
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

            // Those two are null if we didn't redirect input.
            SDL_IOStream *input_stream = nullptr;
            InputCallback input_callback;

            // Those two are null if we didn't redirect output.
            SDL_IOStream *output_stream = nullptr;
            OutputCallback output_callback;
        };
        State state;

        void WriteMoreInput();
        void ReadMoreOutput();
        void CheckOrWait(bool wait);

        [[nodiscard]] static Params DefaultConstructParams() {return {};} // Stupid Clang bugs!

      public:
        constexpr Process() {}

        // If a non-null `output_callback` is passed, it will receive all stdout and stderr output from the process.
        // `output_callback` is always called synchronously in the same thread, when some of the functions below are called to check the process status.
        // If `output_callback` is not specified, the process stdout and stderr are not redirected.
        // In this overload, `argv` is required to have a null element at the end.
        explicit Process(const char *const *               argv, Params params = DefaultConstructParams());
        explicit Process(std::span<const zstring_view    > args, Params params = DefaultConstructParams());
        explicit Process(std::span<const std::string_view> args, Params params = DefaultConstructParams());
        explicit Process(std::span<const std::string     > args, Params params = DefaultConstructParams());
        // Allow passing a braced list directly. Otherwise we get an ambiguity with the constructors above.
        explicit Process(std::initializer_list<const char *> args, Params params = DefaultConstructParams());

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
