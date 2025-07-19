#include "process.h"

#include "sdl/properties.h"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <SDL3/SDL_timer.h>

#include <vector>

namespace em
{
    void Process::CheckOrWait(bool wait)
    {
        ThrowIfNull();
        if (KnownToBeFinished())
            return; // Nothing to do.

        // If the output is redirected, consume it.
        if (state.output_stream)
        {
            while (true)
            {
                // SDL seems to use this chunk size in `SDL_LoadFile_IO()`, so we do too.
                char buffer[1024];
                std::size_t bytes_read = SDL_ReadIO(state.output_stream, buffer, sizeof buffer);
                if (bytes_read > 0)
                {
                    state.output_callback(std::string_view(buffer, bytes_read));
                }
                else
                {
                    if (SDL_GetIOStatus(state.output_stream) == SDL_IO_STATUS_NOT_READY)
                    {
                        if (wait)
                            SDL_Delay(1); // Same as what `SDL_LoadFile_IO()` uses. Weird that we don't have proper blocking.
                        else
                            break;
                    }
                    else
                    {
                        // Whatever happened, we'll not be needing the stream anymore.
                        state.output_stream = nullptr;
                        state.output_callback = {};
                        break;
                    }
                }
            }
        }

        int exit_code = 0;
        if (SDL_WaitProcess(state.handle, wait, &exit_code))
            state.exit_code = exit_code;
    }

    Process::Process(const char *const *argv, OutputCallback output_callback)
        : Process() // Clean up on throw, just in case.
    {
        { // Fill the debug string `debug_cmdline` from `argv`.
            std::size_t num_args = 0;
            while (argv[num_args])
                num_args++;

            std::span<const char *const> argv_span(argv, num_args);

            state.debug_cmdline = fmt::format("{}", argv_span);
        }

        state.output_callback = std::move(output_callback);

        SdlProperties props;
        props.Set(SDL_PROP_PROCESS_CREATE_ARGS_POINTER, argv);
        if (state.output_callback)
        {
            props.Set(SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP); // Create new stream for output.
            props.Set(SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true); // Redirect stderr to stdout.
        }

        state.handle = SDL_CreateProcessWithProperties(props.Handle());
        if (!state.handle)
            throw std::runtime_error("Failed to start process: " + state.debug_cmdline);

        // Get the output stream handle.
        if (state.output_callback)
        {
            state.output_stream = SDL_GetProcessOutput(state.handle);
            if (!state.output_stream)
                throw std::runtime_error("Failed to the the process output stream handle.");
        }
    }

    Process::Process(std::span<const zstring_view> args, OutputCallback output_callback)
    {
        std::vector<const char *> argv;
        argv.reserve(args.size() + 1);

        for (zstring_view arg : args)
            argv.push_back(arg.c_str());
        argv.push_back(nullptr);

        *this = Process(argv.data(), std::move(output_callback));
    }

    Process::~Process()
    {
        if (state.handle)
        {
            Kill(true); // I guess force this?
            SDL_DestroyProcess(state.handle);
        }
    }

    void Process::Kill(bool force)
    {
        // Silently doing nothing if this instance is null.

        // SDL errors if this is called on a known finished process, which is why we check `!state.exit_code`.
        // But either way it seems to not be a big deal, we could just ignore the error. But checking is a bit cleaner.

        if (state.handle && !state.exit_code)
        {
            SDL_KillProcess(state.handle, force);
        }
    }
}
