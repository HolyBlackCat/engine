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
                    .output = Process::OutputToString(str, params.max_output_bytes),
                }),
            .output = std::move(str),
        };
    }

    ProcessQueue::Status ProcessQueue::MakeStatus() const
    {
        return {
            .exit_code = first_nonzero_exit_code,
            .num_finished = int(next_task_index - jobs.size()),
            .num_failed = num_failed_tasks,
            .num_running = int(jobs.size()),
            .num_total = int(tasks.size()),
        };
    }

    void ProcessQueue::CheckOrWait(bool wait)
    {
        if (LastKnownStatus().IsFinished())
            return; // Nothing to do.

        while (true)
        {
            for (std::size_t i = 0; i < jobs.size();)
            {
                Job &job = jobs[i];
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
                    num_failed_tasks++;
                    if (!first_nonzero_exit_code)
                        first_nonzero_exit_code = job.process.ExitCode();
                }

                bool stop_queue = params.stop_on_failure && job.process.ExitCode();

                // Run the callback:

                Status this_status = MakeStatus();

                // Adjust the status a bit.
                {
                    this_status.num_finished++;
                    if (stop_queue)
                        this_status.num_running = 0;
                    else if (next_task_index == tasks.size())
                        this_status.num_running--;
                }
                params.status_callback(job, this_status);

                // Stop all jobs on failure, if enabled. But only after the callback, for nicer numbers.
                if (params.stop_on_failure && job.process.ExitCode())
                {
                    Kill(false); // I guess false?
                    break;
                }

                // Try to start a new job.
                if (next_task_index < tasks.size())
                {
                    job = StartJob(std::move(tasks[next_task_index++]));
                    i++;
                    continue;
                }
                else
                {
                    // If no more tasks, remove the job entirely.

                    // Swap with the last job.
                    if (i + 1 != jobs.size())
                        std::swap(job, jobs.back());

                    // Pop the last job.
                    jobs.pop_back();
                }
            }

            // Stop or try again.
            if (wait)
            {
                if (jobs.empty())
                    break;

                SDL_Delay(1);
            }
            else
                break;
        }
    }

    ProcessQueue::ProcessQueue(std::vector<Task> new_tasks, Params new_params)
        : tasks(std::move(new_tasks)), params(std::move(new_params))
    {
        // Adjust the params:

        // Default the number of jobs to the number of CPU cores, and in any case cap it to the number of tasks.
        if (params.num_jobs == 0)
            params.num_jobs = Process::NumCpuCores();
        if (std::size_t(params.num_jobs) > tasks.size())
            params.num_jobs = int(tasks.size());

        // Default the status callback to printing to `stderr`.
        if (!params.status_callback)
        {
            params.status_callback = [stop_on_failure = params.stop_on_failure](const Job &job, const Status &status)
            {
                // Header.
                if (job.process.ExitCode() == 0)
                    fmt::print(stderr, "[Done] {}\n", job.name.c_str());
                else
                    fmt::print(stderr, "[Failed] {} (exit code: {}, command: {})\n", job.name.c_str(), job.process.ExitCode(), job.process.GetDebugCommandLine().c_str());

                // Output, if any.
                if (!job.output->empty())
                {
                    std::fwrite(job.output->data(), job.output->size(), 1, stderr);
                    Terminal::SendAnsiResetSequence(stderr); // Reset the syling after the output.
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
                    fmt::print(stderr, "-- All {} job{} done --", status.num_total, status.num_total != 1 ? "s" : "");
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
        jobs.reserve(std::size_t(params.num_jobs));
        for (std::size_t i = 0; i < std::size_t(params.num_jobs); i++)
            jobs.push_back(StartJob(std::move(tasks[i])));
        next_task_index = std::size_t(params.num_jobs);
    }

    // Kills all running processes, if any. Will not start any new ones after this.
    void ProcessQueue::Kill(bool force)
    {
        for (Job &jobs : jobs)
        {
            jobs.process.Kill(force);
            jobs.process.Detach();
        }
        jobs = decltype(jobs){};

        if (!first_nonzero_exit_code)
            first_nonzero_exit_code = -255; // Just some exit code. This corresponds to "unknown reason" as reported by `SDL_WaitProcess()`.
    }
}
