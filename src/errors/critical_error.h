#pragma once

#include <functional>
#include <list>
#include <optional>
#include <string_view>

namespace em
{
    // Terminates the program with a critical error.
    [[noreturn]] void CriticalError(std::string_view message);

    // While this is alive, it's going to be notified of all critical errors before the application is terminated. You can use this to e.g. log them.
    // Handlers added later have more priority.
    class CriticalErrorHandler
    {
      public:
        using Func = std::function<void(std::string_view)>;

      private:
        struct Entry
        {
            Func func;

            Entry(Func func) : func(std::move(func)) {}
            Entry(const Entry &) = delete;
            Entry &operator=(const Entry &) = delete;
        };
        std::optional<std::list<Entry>::iterator> iter;

        static std::list<Entry> list;
        friend void CriticalError(std::string_view message);

      public:
        CriticalErrorHandler() {}

        // The `func` is never going to be moved around, you should store all your state in it.
        CriticalErrorHandler(Func func);
        CriticalErrorHandler(CriticalErrorHandler &&other) noexcept;
        CriticalErrorHandler &operator=(CriticalErrorHandler other) noexcept;
        ~CriticalErrorHandler();

        [[nodiscard]] explicit operator bool() const {return bool(iter);}
    };
}
