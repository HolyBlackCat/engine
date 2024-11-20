#include "errors/critical_error.h"
#include "errors/exception_analyzer.h"

#include <exception>

// This file registers some error handlers during initialization.

namespace
{
    struct RegistrationHelper
    {
        RegistrationHelper()
        {
            std::set_terminate([]{
                if (auto e = std::current_exception())
                {
                    em::CriticalError("Uncaught exception:\n" + em::DefaultExceptionAnalyzer().Analyze(e).CombinedMessage());
                }
                else
                {
                    em::CriticalError("Unknown error: `std::terminate()` was called without an active exception.");
                }
            });
        }
    };

    // Can't use an immediately invoked lambda, because apparently `__init_priority__` can't be used with non-class types. Lame!
    #ifndef _MSC_VER
    __attribute__((__init_priority__(101))) // This is the smallest value (i.e. highest priority) that's not reserved for the standard library.
    #endif
    const RegistrationHelper registration_helper{};
}
