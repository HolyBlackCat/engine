#include "process_queue.h"
#include "SDL3/SDL_timer.h"
#include "utils/terminal.h"

#include <fmt/format.h>

#include <cstdio>

namespace em
{
    ProcessQueue::Job ProcessQueue::StartJob(Task &&task)
    {
        auto str = std::make_shared<std::string>();
        return {
            .name = task.name,
            .process = Process(task.command,
                {
                    .input = task.input ? Process::InputFromString(std::move(*task.input)) : nullptr,
                    .output = Process::OutputToString(str, state.params.max_output_bytes),
                }),
            .output = std::move(str),
        };
    }

    ProcessQueue::Status ProcessQueue::MakeStatus() const
    {
        return {
            .exit_code = state.first_nonzero_exit_code,
            .num_finished = int(state.next_task_index - state.jobs.size()),
            .num_failed = state.num_failed_tasks,
            .num_running = int(state.jobs.size()),
            .num_total = int(state.tasks.size()),
        };
    }

    void ProcessQueue::CheckOrWait(bool wait)
    {
        if (LastKnownStatus().IsFinished())
            return; // Nothing to do.

        while (true)
        {
            for (std::size_t i = 0; i < state.jobs.size();)
            {
                Job &job = state.jobs[i];
                if (!job.process.CheckIfFinished())
                {
                    // This job isn't finished yet, continue.
                    i++;
                    continue;
                }

                // At this point we know the process has finished.

                // Update state on job failure.
                if (job.process.ExitCode())
                {
                    state.num_failed_tasks++;
                    if (!state.first_nonzero_exit_code)
                        state.first_nonzero_exit_code = job.process.ExitCode();
                }

                bool stop_queue = state.params.stop_on_failure && job.process.ExitCode();

                // Run the callback:

                Status this_status = MakeStatus();

                // Adjust the status a bit.
                {
                    this_status.num_finished++;
                    if (stop_queue)
                        this_status.num_running = 0;
                    else if (state.next_task_index == state.tasks.size())
                        this_status.num_running--;
                }
                state.params.status_callback(job, this_status);

                // Stop all jobs on failure, if enabled. But only after the callback, for nicer numbers.
                if (state.params.stop_on_failure && job.process.ExitCode())
                {
                    Kill(false); // I guess false?
                    break;
                }

                // Try to start a new job.
                if (state.next_task_index < state.tasks.size())
                {
                    job = StartJob(std::move(state.tasks[state.next_task_index++]));
                    i++;
                    continue;
                }
                else
                {
                    // If no more tasks, remove the job entirely.

                    // Swap with the last job.
                    if (i + 1 != state.jobs.size())
                        std::swap(job, state.jobs.back());

                    // Pop the last job.
                    state.jobs.pop_back();
                }
            }

            // Stop or try again.
            if (wait)
            {
                if (state.jobs.empty())
                    break;

                SDL_Delay(1);
            }
            else
                break;
        }
    }

    ProcessQueue::ProcessQueue(std::vector<Task> new_tasks, Params new_params)
    {
        state.tasks = std::move(new_tasks);
        state.params = std::move(new_params);

        // Adjust the params:

        // Default the number of jobs to the number of CPU cores, and in any case cap it to the number of tasks.
        if (state.params.num_jobs == 0)
            state.params.num_jobs = Process::NumCpuCores();
        if (std::size_t(state.params.num_jobs) > state.tasks.size())
            state.params.num_jobs = int(state.tasks.size());

        // Default the status callback to printing to `stderr`.
        if (!state.params.status_callback)
        {
            state.params.status_callback = [first = true, stop_on_failure = state.params.stop_on_failure](const Job &job, const Status &status) mutable
            {
                // Enable the console on the first message.
                if (first)
                {
                    first = false;
                    Terminal::DefaultToConsole(stderr);
                }

                // Header.
                if (job.process.ExitCode() == 0)
                    fmt::print(stderr, "[Done] {}\n", job.name.c_str());
                else
                    fmt::print(stderr, "[Failed] {} (exit code: {}, command: {})\n", job.name.c_str(), job.process.ExitCode(), job.process.GetDebugCommandLine().c_str());

                // Output, if any.
                if (!job.output->empty())
                {
                    std::fwrite(job.output->data(), job.output->size(), 1, stderr);
                    Terminal::SendAnsiResetSequence(stderr); // Reset the styling after the output.
                    if (!job.output->ends_with('\n'))
                        std::fputs("\n(missing newline at the end of output)\n", stderr); // Unlike `puts`, `fputs` doesn't append a newline.

                    // Report the error again.
                    if (job.process.ExitCode() != 0)
                        fmt::print(stderr, "[Failed] The job above has failed\n");
                }

                if (!status.IsFinished())
                {
                    fmt::print(
                        stderr,
                        "-- {}/{} done{}, {} still running --\n",
                        status.num_finished,
                        status.num_total,
                        status.num_failed > 0 ? fmt::format(" (including {} failed)", status.num_failed) : "",
                        status.num_running
                    );
                }
                else if (status.exit_code == 0 && status.num_finished == status.num_total) // The second condition seems redundant, but just in case.
                {
                    fmt::print(stderr, "-- All {} job{} done --\n", status.num_total, status.num_total != 1 ? "s" : "");
                }
                else
                {
                    fmt::print(
                        stderr,
                        "-- {}/{} done, {} failed!{} --\n",
                        status.num_finished,
                        status.num_total,
                        status.num_failed,
                        stop_on_failure ? " Stopping." : ""
                    );
                }
            };
        }

        // Start the first jobs.
        state.jobs.reserve(std::size_t(state.params.num_jobs));
        for (std::size_t i = 0; i < std::size_t(state.params.num_jobs); i++)
            state.jobs.push_back(StartJob(std::move(state.tasks[i])));
        state.next_task_index = std::size_t(state.params.num_jobs);
    }

    ProcessQueue::ProcessQueue(ProcessQueue &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }
    ProcessQueue &ProcessQueue::operator=(ProcessQueue other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    void ProcessQueue::Kill(bool force)
    {
        for (Job &jobs : state.jobs)
        {
            jobs.process.Kill(force);
            jobs.process.Detach();
        }
        state.jobs = decltype(state.jobs){};

        if (!state.first_nonzero_exit_code)
            state.first_nonzero_exit_code = -255; // Just some exit code. This corresponds to "unknown reason" as reported by `SDL_WaitProcess()`.
    }
}
