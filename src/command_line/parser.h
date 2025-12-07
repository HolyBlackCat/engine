#pragma once

#include "em/macros/utils/forward.h"
#include "em/macros/utils/flag_enum.h"
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

    // Those are used by `Parser::AddFlag()`.
    enum class Flags
    {
        allow_repeat = 1 << 0,
    };
    EM_FLAG_ENUM(Flags)

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
            virtual void ConsumeArgs(const Parser &parser, std::span<const char *const> args) = 0;

            // If this is false, repeating this flag will automatically error.
            virtual bool AllowRepeat() const {return false;}

            // This is called once for every flag at the end of parsing, even if this flag wasn't passed.
            virtual void OnPostParse(const Parser &parser) {(void)parser;}
        };

        struct FlagDesc
        {
            // A comma-separated list of flag names, with the leading dashes.
            std::string names;

            std::shared_ptr<BasicFlag> flag;
            std::string help_text;
        };

        using NameToFlagMap = phmap::flat_hash_map<std::string, std::shared_ptr<BasicFlag>>;

      private:
        NameToFlagMap name_to_flag;
        std::vector<FlagDesc> flag_descriptions; // This is primarily for the help page.

      public:
        // Inserts the default implementation of the `--help` flag.
        // Note that the flag insertion order is preserved in `--help` by default.
        void AddDefaultHelpFlag();

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
        // `ArgTypes...` is the argument types, possibly none.
        // `flag_names` is a comma-separated list of equivalent names for this flag, each starting with either `-` or `--` and having exactly one or more than one (respectively) symbols in the name.
        //   The order of flags in `flag_names` is propagated to `--help` but otherwise doesn't matter.
        // `arg_names...` has the same length as `ArgTypes...`, those are the names of the arguments to be displayed in `--help`.
        // `help_text` is the description of this flag to be displayed in `--help`.
        // `func(ArgTypes...)` is the callback called when this flag is received.
        // `on_post_parse()` is the callback called when we're done parsing arguments. It is always called once per registered flag, even if it wasn't actually passed.
        template <ArgumentType ...ArgTypes>
        Parser &AddFlag(
            std::string flag_names,
            Flags flags,
            Meta::first_type<std::string, ArgTypes> ...arg_names,
            std::string help_text,
            std::type_identity_t<std::function<void(const ArgTypes &...)>> func,
            std::function<void()> on_post_parse = nullptr
        )
        {
            using ArgNameArray = std::array<std::string, sizeof...(ArgTypes)>;
            struct FlagImpl : BasicFlag
            {
                Flags flags;
                ArgNameArray arg_names;
                std::function<void(const ArgTypes &...)> func;
                std::function<void()> on_post_parse;

                FlagImpl(Flags flags, ArgNameArray arg_names, std::function<void(const ArgTypes &...)> func, std::function<void()> on_post_parse)
                    : flags(flags), arg_names(std::move(arg_names)), func(std::move(func)), on_post_parse(std::move(on_post_parse))
                {}

                int NumArgs() const override {return sizeof...(ArgTypes);}

                std::string_view ArgName(int i) const override
                {
                    return arg_names.at(std::size_t(i));
                }

                void ConsumeArgs(const Parser &parser, std::span<const char *const> args) override
                {
                    (void)parser; // Only custom flags use this. Realistically, only `--help` needs it.

                    std::size_t i = 0;
                    // Since we only allow string-like types for now, we can just cast to `ArgTypes`.
                    // Using a tuple to force evaluation order. Using `ArgTypes...` just in case, to avoid CTAD jank.
                    std::apply(func, std::tuple<ArgTypes...>{ArgTypes(args[i++])...});
                }

                bool AllowRepeat() const override
                {
                    return bool(flags & Flags::allow_repeat);
                }

                void OnPostParse(const Parser &parser) override
                {
                    (void)parser;
                    if (on_post_parse)
                        on_post_parse();
                }
            };
            return AddFlagLow<FlagImpl>(std::move(flag_names), std::move(help_text), flags, ArgNameArray{std::move(arg_names)...}, std::move(func), std::move(on_post_parse));
        }

        // This throws on failure. `argv` is expected to be terminated by a null pointer.
        void Parse(const char *const *argv);
        // If `argc` is specified, it's validated against `argv` and that's all.
        void Parse(int argc, const char *const *argv);

        // Some getters, you usually don't need those.
        [[nodiscard]] const NameToFlagMap &GetNameToFlagMap() const {return name_to_flag;}
        [[nodiscard]] const std::vector<FlagDesc> &GetFlagDescriptions() const {return flag_descriptions;}
    };
}
