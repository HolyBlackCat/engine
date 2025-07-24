#include "process.h"

#include "sdl/properties.h"

#include <cassert>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_timer.h>

#include <vector>

namespace em
{
    int Process::NumCpuCores()
    {
        static int ret = SDL_GetNumLogicalCPUCores();
        return ret;
    }

    void Process::WriteMoreInput()
    {
        if (!state.input_stream)
            return; // Nothing to do.

        // Repeat this a few times, in case the user callback doesn't write all it can in one sitting (which is unwise).
        // Here we don't flush the stream, because that seems to be unnecessary: https://github.com/libsdl-org/SDL/issues/13412

        while (true)
        {
            bool callback_had_data = false;
            bool stream_is_full = false;
            bool stream_error = false;

            bool eof = state.input_callback([&](std::string_view data) -> std::size_t
            {
                if (data.empty())
                    return 0; // Nothing to do. Don't want to update the variable below.

                callback_had_data = true;

                std::size_t bytes_written = 0;

                while (true)
                {
                    bytes_written += SDL_WriteIO(state.input_stream, data.data() + bytes_written, data.size() - bytes_written);

                    if (bytes_written < data.size() && SDL_GetIOStatus(state.input_stream) == SDL_IO_STATUS_READY)
                        continue; // Write in blocks if needed? I had to do this when writing in large chunks (100k bytes).

                    break;
                }

                if (bytes_written < data.size())
                {
                    if (SDL_GetIOStatus(state.input_stream) == SDL_IO_STATUS_NOT_READY)
                        stream_is_full = true; // Out of space? Exit for now and retry later.
                    else
                        stream_error = true; // Something else is wrong.
                }

                return bytes_written;
            });

            // Close the stream if the callback says it's done, or if the stream has errored.
            if (eof || stream_error)
            {
                // Must close the stream explicitly. The SDL tests do it like this too.
                SDL_CloseIO(state.input_stream);
                state.input_stream = nullptr;
                state.input_callback = {};
                return;
            }

            // Stop for now if the pipe doesn't accept any more data.
            if (stream_is_full)
                break;
        }
    }

    void Process::ReadMoreOutput()
    {
        if (!state.output_stream)
            return; // Nothing to do.

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
                SDL_IOStatus status = SDL_GetIOStatus(state.output_stream);
                if (status != SDL_IO_STATUS_NOT_READY)
                {
                    assert(status != SDL_IO_STATUS_READY); // Shouldn't be possible?

                    // Whatever happened, we'll not be needing the stream anymore.
                    // But don't close it, I guess? It shouldn't matter.

                    state.output_stream = nullptr;
                    state.output_callback = {};
                }

                // Either way, stop.
                return;
            }
        }
    }

    void Process::CheckOrWait(bool wait)
    {
        ThrowIfNull();
        if (KnownToBeFinished())
            return; // Nothing to do.

        // Poke the streams.
        if (state.input_stream || state.output_stream)
        {
            while (true)
            {
                WriteMoreInput();
                ReadMoreOutput();

                if (!wait)
                    break; // Only one iteration.

                // If the streams have died, stop now without adding the extra delay.
                if (!state.input_stream && !state.output_stream)
                    break;

                // Same as what `SDL_LoadFile_IO()` uses. Weird that we don't have proper blocking.
                SDL_Delay(1);
            }
        }

        int exit_code = 0;
        if (SDL_WaitProcess(state.handle, wait, &exit_code))
            state.exit_code = exit_code;
    }

    Process::Process(const char *const *argv, Params params)
        : Process() // Clean up on throw, just in case.
    {
        { // Fill the debug string `debug_cmdline` from `argv`.
            std::size_t num_args = 0;
            while (argv[num_args])
                num_args++;

            std::span<const char *const> argv_span(argv, num_args);

            state.debug_cmdline = fmt::format("{}", argv_span);
        }

        state.input_callback = std::move(params.input);
        state.output_callback = std::move(params.output);

        SdlProperties props;
        props.Set(SDL_PROP_PROCESS_CREATE_ARGS_POINTER, argv);
        if (state.input_callback)
        {
            // If no input callback, this defaults to null, instead of being inherited like the output stream, so we don't need to do anything in that case.

            props.Set(SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_APP); // Create new stream for input.
        }
        if (state.output_callback)
        {
            props.Set(SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP); // Create new stream for output.
            props.Set(SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true); // Redirect stderr to stdout.
        }

        state.handle = SDL_CreateProcessWithProperties(props.Handle());
        if (!state.handle)
            throw std::runtime_error("Failed to start process: " + state.debug_cmdline);

        // Get the output stream handle.
        if (state.input_callback)
        {
            state.input_stream = SDL_GetProcessInput(state.handle);
            if (!state.input_stream)
                throw std::runtime_error("Failed to get the process input stream handle.");
        }
        // Get the output stream handle.
        if (state.output_callback)
        {
            state.output_stream = SDL_GetProcessOutput(state.handle);
            if (!state.output_stream)
                throw std::runtime_error("Failed to get the process output stream handle.");
        }


        // Lastly, try to send some input immediately.
        // It seems to make sense to write input immediately, but don't touch the output yet.
        WriteMoreInput();
    }

    Process::Process(std::span<const zstring_view> args, Params params)
    {
        std::vector<const char *> argv;
        argv.reserve(args.size() + 1);

        for (const auto &arg : args)
            argv.push_back(arg.c_str());
        argv.push_back(nullptr);

        *this = Process(argv.data(), std::move(params));
    }

    Process::Process(std::span<const std::string_view> args, Params params)
    {
        std::vector<std::string> str_args;
        str_args.reserve(args.size());

        for (const auto &arg : args)
            str_args.emplace_back(arg);

        *this = Process(str_args, std::move(params));
    }

    Process::Process(std::span<const std::string> args, Params params)
    {
        std::vector<const char *> argv;
        argv.reserve(args.size() + 1);

        for (const auto &arg : args)
            argv.push_back(arg.c_str());
        argv.push_back(nullptr);

        *this = Process(argv.data(), std::move(params));
    }

    // Allow passing a braced list directly. Otherwise we get an ambiguity with the constructors above.
    Process::Process(std::initializer_list<const char *> args, Params params)
    {
        std::vector<const char *> argv;
        argv.reserve(args.size() + 1);
        argv.assign(args.begin(), args.end());
        argv.push_back(nullptr);

        *this = Process(argv.data(), std::move(params));
    }

    Process::~Process()
    {
        if (state.handle)
        {
            Kill(true); // I guess force this?
            Detach();
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

    void Process::Detach()
    {
        if (state.handle)
        {
            SDL_DestroyProcess(state.handle);
            state = {};
        }
    }
}
