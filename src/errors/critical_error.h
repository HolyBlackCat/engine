#pragma once

#include "em/zstring_view.h"

#include <cstddef>
#include <functional>
#include <list>
#include <optional>

namespace em
{
    // Terminates the program with a critical error.
    [[noreturn]] void CriticalError(zstring_view message);

    // While this is alive, it's going to be notified of all critical errors before the application is terminated. You can use this to e.g. log them.
    // Handlers added later have more priority.
    class CriticalErrorHandler
    {
      public:
        using Func = std::function<void(zstring_view)>;

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
        friend void CriticalError(zstring_view message);

      public:
        CriticalErrorHandler() {}
        CriticalErrorHandler(std::nullptr_t) {}

        // The `func` is never going to be moved around, you should store all your state in it.
        // `func` is `(em::zstring_view message) -> void`.
        // By default the handler is prepended to other handlers. Pass `after_other_handlers == true` to add it after the existing ones instead.
        CriticalErrorHandler(Func func, bool after_other_handlers = false);
        CriticalErrorHandler(CriticalErrorHandler &&other) noexcept;
        CriticalErrorHandler &operator=(CriticalErrorHandler other) noexcept;
        ~CriticalErrorHandler();

        [[nodiscard]] explicit operator bool() const {return bool(iter);}
    };
}
