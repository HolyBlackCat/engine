#pragma once

#include "utils/process.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace em
{
    // Runs several process, at most N at a time.
    class ProcessQueue
    {
      public:
        // The input task to run.
        struct Task
        {
            std::string name;
            std::vector<std::string> command;

            // If not null, we send this string to the standard input.
            std::optional<std::string> input{};
        };

        // A job that the queue runs internally.
        struct Job
        {
            std::string name;
            Process process;

            // The process output goes here.
            // This is shared to keep the address stable, in case the queue is moved, since a pointer to it is saved in process callbacks.
            // And also because the jobs can be reshuffled.
            std::shared_ptr<std::string> output;
        };

        // The status of the entire queue.
        struct Status
        {
            [[nodiscard]] bool IsFinished() const {return num_running == 0;}

            // The exit code of the first failed task if any. If `stop_on_failure == false`, this can be set even before we're finished.
            int exit_code = 0;

            int num_finished = 0; // Finished tasks, both failed and successful.
            int num_failed = 0; // Out of those, the number of tasks that failed.
            int num_running = 0; // How many tasks are running now.
            int num_total = 0; // The total number of tasks. This doesn't change.
        };

        // This is called after every process finishes.
        // The `queue` is passed so that you can call `LastKnownStatus()` on it.
        using StatusCallback = std::function<void(const Job &job, const Status &status)>;

        struct Params
        {
            // If zero, uses the number of CPU cores.
            int num_jobs = 0;

            // Whether to stop if any process fails.
            bool stop_on_failure = true;

            // How many bytes of output to preserve for each job.
            // Since we buffer it in RAM to avoid overlap between parallel jobs, making this arbitrarily large could consume all your RAM if a command
            //   continues to print things indefinitely.
            std::size_t max_output_bytes = 20000; // A few screens worth of text.

            // This is called after every process finishes.
            // If null, we default to print status to stderr.
            // No multithreading is involved, this is called by some of the status checking functions below.
            StatusCallback status_callback = nullptr;
        };

      private:
        // All of our tasks, including the finished ones, in the order they will be executed.
        std::vector<Task> tasks;

        Params params;

        // The next task we'll run.
        std::size_t next_task_index = 0;

        int first_nonzero_exit_code = 0;

        int num_failed_tasks = 0;

        std::vector<Job> jobs;

        [[nodiscard]] Job StartJob(Task &&task);
        [[nodiscard]] Status MakeStatus() const;
        void CheckOrWait(bool wait);

        [[nodiscard]] static Params DefaultConstructParams() {return {};} // Stupid Clang bugs!

      public:
        ProcessQueue() {}

        ProcessQueue(std::vector<Task> new_tasks, Params new_params = DefaultConstructParams());

        // Kills all running processes, if any. Will not start any new ones after this.
        void Kill(bool force);


        // Blocks until all processes finish.
        [[nodiscard]] Status WaitUntilFinished() {CheckOrWait(true); return MakeStatus();}

        // Checks the current state of the processes.
        [[nodiscard]] Status CheckStatus() {CheckOrWait(false); return MakeStatus();}

        // Returns the last known status from `WaitUntilFinished()` or `CheckStatus()`. Doesn't actually query the underlying processes.
        [[nodiscard]] Status LastKnownStatus() const {return MakeStatus();}

        // Returns the current jobs.
        [[nodiscard]] std::span<const Job> Jobs() const {return jobs;}
    };
}
