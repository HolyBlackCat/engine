#pragma once

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo> // IWYU pragma: keep, we use `typeid()` in this file.
#include <vector>

// The typical usecase of this file is `em::DefaultExceptionAnalyzer().Analyze(std::current_exception()).CombinedMessage()`.

namespace em
{
    // Stores all information about an exception, such its type, message, and the same information for all nested exceptions.
    struct AnalyzedException
    {
        // Stores information about a single nested exception, or the top-level exception.
        struct Elem
        {
            std::type_index type = typeid(void); // Void means the type isn't known.
            std::string message;
        };
        // Should only be empty for default-constructed instances.
        // Otherwise will have at least one element, even if the type is unknown.
        // Only the last element should contain an unknown type.
        std::vector<Elem> elems;

        [[nodiscard]] explicit operator bool() const {return !elems.empty();}

        // Concats all messages together.
        [[nodiscard]] std::string CombinedMessage(std::string_view separator = "\n") const
        {
            std::string ret;
            bool first = true;
            for (const Elem &elem : elems)
            {
                if (first)
                    first = false;
                else
                    ret += separator;
                ret += elem.message;
            }
            return ret;
        }
    };

    // Analyzes exception types to extract messages and more from them.
    struct ExceptionAnalyzer
    {
        // The return type of `BasicHandler::Handle`.
        struct HandlerResult
        {
            // Void means the type isn't known.
            // Then the message will be empty and you must use a placeholder.
            std::type_index type = typeid(void);

            std::string message;

            // Null if none.
            std::exception_ptr nested_exception = nullptr;

            [[nodiscard]] bool TypeIsKnown() const {return type != typeid(void);}
        };

        // Classes derived from this one know about some exception types and can extract messages and other information from them.
        struct BasicHandler
        {
            virtual ~BasicHandler() = default;

            // Return null (or let the exception escape your function) if you don't know this exception type,
            // then some other handler will try to analyze it.
            virtual std::optional<HandlerResult> Handle(const std::exception_ptr &e) = 0;
        };

        // This implementation of `BasicHandler` handles exceptions derived from `std::exception`-like bases. Normally `T = std::exception`.
        template <typename T>
        struct SimpleHandler : BasicHandler
        {
            std::optional<HandlerResult> Handle(const std::exception_ptr &e) override
            {
                std::optional<HandlerResult> ret_opt;

                try
                {
                    std::rethrow_exception(e);
                }
                catch (T &base)
                {
                    HandlerResult &ret = ret_opt.emplace();
                    ret.type = typeid(base);
                    ret.message = base.what();

                    try
                    {
                        std::rethrow_if_nested(base);
                    }
                    catch (...)
                    {
                        ret.nested_exception = std::current_exception();
                    }
                }

                return ret_opt;
            }
        };
        // This handles exceptions derived from `std::exception`.
        using StdExceptionHandler = SimpleHandler<std::exception>;

        // A list of handlers that `Analyze()` will try to apply to the passed exception.
        std::vector<std::shared_ptr<BasicHandler>> handlers;

        // Extracts information about an exception.
        // If `e` is null, the result will have `elems.size() == 0`. Otherwise it will be at least 1, even for unknown types.
        [[nodiscard]] AnalyzedException Analyze(std::exception_ptr e);
    };

    // Returns the global instance of `ExceptionAnalyzer`.
    [[nodiscard]] ExceptionAnalyzer &DefaultExceptionAnalyzer();
}
