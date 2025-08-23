#pragma once

#include "em/macros/utils/finally.h"
#include "em/macros/utils/forward.h"
#include "em/meta/common.h"
#include "em/meta/const_string.h"
#include "em/zstring_view.h"
#include "strings/split.h"

#include <fmt/format.h>
#include <parallel_hashmap/phmap.h>

#include <cassert>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace em::CommandLine
{
    template <typename T>
    concept StringLike = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, zstring_view>;

    // Valid argument types for the command-line parser.
    template <typename T>
    concept ArgumentType = (Meta::cvref_unqualified<T> && std::is_integral_v<T>) || StringLike<T>;

    // Describes a single argument of a command-line flag.
    template <ArgumentType T, Meta::ConstString Name>
    struct Arg {};

    class Parser
    {
      public:
        struct BasicFlag
        {
            virtual ~BasicFlag() = default;

            virtual int NumArgs() const {return 0;}

            // Returns the help page names for each argument. `i` will be smaller than `NumArgs()`.
            virtual std::string_view ArgName(int i) const {(void)i; return {};}

            // Here you'll get the amount of arguments returned by `NumArgs()`.
            virtual void ConsumeArgs(std::span<const char *const> args) = 0;
        };

        struct FlagDesc
        {
            // A comma-separated list of flag names, with the leading dashes.
            std::string names;

            std::shared_ptr<BasicFlag> flag;
            std::string help_text;
        };

      private:
        phmap::flat_hash_map<std::string, std::shared_ptr<BasicFlag>> name_to_flag;
        std::vector<FlagDesc> flag_descriptions; // This is primarily for the help page.

      public:
        // Inserts a new flag of type `T` derived from `BasicFlag`.
        // This is a low-level function, prefer `AddFlag()`.
        // `flag_names` is a comma-separated list of equivalent names for this flag, each starting with either `-` or `--` and having exactly one or more than one (respectively) symbols in the name.
        // `help_text` is the description of this flag to be displayed in `--help`.
        // `params...` are forwarded to the constructor of `T`.
        template <typename T, Meta::Deduce..., typename ...P>
        requires Meta::cvref_unqualified<T> && std::derived_from<T, BasicFlag> && std::constructible_from<T, P &&...>
        Parser &AddFlagLow(std::string flag_names, std::string help_text, P &&... params)
        {
            FlagDesc &new_flag = flag_descriptions.emplace_back(std::move(flag_names), std::make_shared<T>(EM_FWD(params)...), std::move(help_text));

            Strings::Split(new_flag.names, ",", [&](std::string_view name)
            {
                if (!name.starts_with('-'))
                    throw std::logic_error(fmt::format("Bad usage of the command line parser: Each command line flag must start with a `-` or `--`, but got `{}`.", name));

                if (name.starts_with("--"))
                {
                    // Want more than one letter after `--`.
                    if (name.size() <= 3)
                        throw std::logic_error(fmt::format("Bad usage of the command line parser: A flag starting with `--` must have a multi-letter name, but got `{}`.", name));

                    name.remove_prefix(2);
                }
                else
                {
                    if (name.size() != 2)
                        throw std::logic_error(fmt::format("Bad usage of the command line parser: A flag starting with `-` must have exactly one letter in the name, but got `{}`.", name));

                    name.remove_prefix(1); // We already checked that it starts with at least one `-`.
                }

                if (!name_to_flag.try_emplace(name, new_flag.flag).second)
                    throw std::logic_error(fmt::format("Bad usage of the command line parser: Duplicate command line flag: `{}`.", name));
            });

            return *this;
        }

        // Inserts a new flag.
        // `ArgTypes...` is the argument types, possibly zero.
        // `flag_names` is a comma-separated list of equivalent names for this flag, each starting with either `-` or `--` and having exactly one or more than one (respectively) symbols in the name.
        // `arg_names...` has the same length as `ArgTypes...`, those are the names of the arguments to be displayed in `--help`.
        // `help_text` is the description of this flag to be displayed in `--help`.
        // `func()` is the callback called when this flag is received.
        template <ArgumentType ...ArgTypes>
        Parser &AddFlag(
            std::string flag_names,
            Meta::first_type<std::string, ArgTypes> ...arg_names,
            std::string help_text,
            std::type_identity_t<std::function<void(const ArgTypes &...)>> func
        )
        {
            using ArgNameArray = std::array<std::string, sizeof...(ArgTypes)>;
            struct FlagImpl : BasicFlag
            {
                ArgNameArray arg_names;
                std::function<void(const ArgTypes &...)> func;

                FlagImpl(ArgNameArray arg_names, std::function<void(const ArgTypes &...)> func)
                    : arg_names(std::move(arg_names)), func(std::move(func))
                {}

                int NumArgs() const override {return sizeof...(ArgTypes);}

                std::string_view ArgName(int i) const override
                {
                    return arg_names.at(std::size_t(i));
                }

                void ConsumeArgs(std::span<const char *const> args) override
                {
                    std::size_t i = 0;
                    // Since we only allow string-like types for now, we can just cast to `ArgTypes`.
                    std::apply(func, std::tuple{ArgTypes(args[i++])...}); // Using a tuple to force evaluation order.
                }
            };
            return AddFlagLow<FlagImpl>(std::move(flag_names), std::move(help_text), ArgNameArray{std::move(arg_names)...}, std::move(func));
        }

        // This throws on failure. `argv` is expected to be terminated by a null pointer.
        void Parse(const char *const *argv)
        {
            if (!*argv)
                return; // Zero arguments, not even the program name. Nothing to do.

            // Skip the program name.
            argv++;

            while (*argv)
            {
                const char *this_flag = *argv;

                if (this_flag[0] == '-')
                {
                    if (this_flag[1] == '-')
                    {
                        std::string_view flag_view = this_flag + 2;

                        std::optional<std::string_view> first_arg;

                        // Got an argument?
                        if (auto pos = flag_view.find('='); pos != std::string_view::npos)
                        {
                            first_arg = flag_view.substr(pos + 1);
                            flag_view = flag_view.substr(0, pos);
                        }

                        auto iter = name_to_flag.find(flag_view);

                        // Complain if no such flag.
                        if (iter == name_to_flag.end())
                            throw std::runtime_error(fmt::format("No such flag: `--{}`.", flag_view));
                        // Complain if we got `--` with a single letter. Those should only be usable with a single `-`.
                        if (iter->first.size() == 1)
                            throw std::runtime_error(fmt::format("No such flag: `--{}`, did you mean `-{}`.", flag_view, flag_view));

                        const int num_args = iter->second->NumArgs();
                        if (first_arg)
                        {
                            // The `=...` argument is embedded in this flag.

                            // Complain if `=` argument was specified, but the flag doesn't accept arguments.
                            if (num_args == 0)
                                throw std::runtime_error(fmt::format("Flag `--{}` doesn't accept arguments.", flag_view));
                            // Complain if `=` argument was specified, but the flag accepts more than one argument.
                            if (num_args > 1)
                                throw std::runtime_error(fmt::format("Flag `--{}` needs more than one argument, so `=` can't be used with it. Pass the arguments as separate arguments.", flag_view));

                            iter->second->ConsumeArgs({first_arg->data()});
                        }
                        else
                        {
                            // The arguments (possibly 0) are expected to follow this flag.

                            // Check that we got enough arguments after this one.
                            for (int i = 0; i < num_args; i++)
                            {
                                if (!argv[i + 1])
                                    throw std::runtime_error(fmt::format("Flag `--{}` needs {} argument{}, but got {}.", flag_view, num_args, num_args == 1 ? "" : "s", i));
                            }

                            // Special-casing `num_args == 0` is a bit pointless here, but it ensures that flags accepting zero arguments
                            //   don't accidentally read the arguments array out of bounds, I guess?
                            if (num_args == 0)
                                iter->second->ConsumeArgs({});
                            else
                                iter->second->ConsumeArgs({argv + 1, std::size_t(num_args)});

                            argv += num_args;
                        }
                    }
                    else // Starting with a single `-`.
                    {
                        this_flag++; // Skip `-`.

                        if (!*this_flag)
                            throw std::runtime_error("No such flag: `-`.");

                        do
                        {
                            auto iter = name_to_flag.find(std::string_view(this_flag, 1));

                            // Complain if no such flag.
                            if (iter == name_to_flag.end())
                                throw std::runtime_error(fmt::format("No such flag: `-{}`.", *this_flag));

                            const int num_args = iter->second->NumArgs();

                            // No arguments?
                            if (num_args == 0)
                            {
                                iter->second->ConsumeArgs({});
                                this_flag++;
                                continue;
                            }

                            // One argument embedded in the flag?
                            if (num_args == 1 && this_flag[1])
                            {
                                // Pass the rest of the flag as the argument.
                                iter->second->ConsumeArgs({this_flag + 1});
                                break; // Nothing else to do here, go to the next argument.
                            }

                            // If we have more than one argument, make sure the first one isn't embedded in the flag.
                            if (this_flag[1])
                                throw std::runtime_error(fmt::format("Flag `-{}` needs more than one argument, so they can't follow it immediately, and must be passed as separate arguments.", *this_flag));

                            // Check that we got enough arguments after this one.
                            for (int i = 0; i < num_args; i++)
                            {
                                if (!argv[i + 1])
                                    throw std::runtime_error(fmt::format("Flag `-{}` needs {} argument{}, but got {}.", *this_flag, num_args, num_args == 1 ? "" : "s", i));
                            }

                            iter->second->ConsumeArgs({argv + 1, std::size_t(num_args)});

                            argv += num_args;
                            break;
                        }
                        while (*this_flag);
                    }
                }
                else
                {
                    throw std::runtime_error(fmt::format("Positional arguments are not allowed, but got `{}`", this_flag));
                }

                argv++;
            }
        }
    };
}
