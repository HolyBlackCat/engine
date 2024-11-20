#include "exception_analyzer.h"

namespace em
{
    AnalyzedException ExceptionAnalyzer::Analyze(std::exception_ptr e)
    {
        // Updates `e` to point to the nested exception, if any.
        auto AnalyzeLevel = [this](std::exception_ptr &e) -> AnalyzedException::Elem
        {
            for (const auto &handler : handlers)
            {
                try
                {
                    auto result = handler->Handle(e);
                    if (result)
                    {
                        e = result->nested_exception;
                        return {.type = result->type, .message = std::move(result->message)};
                    }
                }
                catch (...) {}
            }

            return {};
        };

        AnalyzedException ret;
        while (e)
            ret.elems.push_back(AnalyzeLevel(e));
        return ret;
    }

    ExceptionAnalyzer &DefaultExceptionAnalyzer()
    {
        static ExceptionAnalyzer ret = []{
            ExceptionAnalyzer ret;
            ret.handlers.push_back(std::make_shared<ExceptionAnalyzer::StdExceptionHandler>());
            return ret;
        }();
        return ret;
    }
}
