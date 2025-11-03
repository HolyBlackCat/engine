#include "command_line_parser.h"

#include "utils/terminal.h"

#include <cstdlib>

namespace em::CommandLine
{
    namespace
    {
        // This is a non-member to ensure that we're being fair by only using the public API.
        struct HelpFlag : Parser::BasicFlag
        {
            void ConsumeArgs(const Parser &parser, std::span<const char *const> args) override
            {
                (void)args;

                const auto &flag_descs = parser.GetFlagDescriptions();

                // Assemble parameter names with arguments.
                std::vector<std::string> param_strings;
                param_strings.reserve(flag_descs.size());

                std::size_t max_param_string_len = 0;

                for (const Parser::FlagDesc &desc : flag_descs)
                {
                    std::string &new_str = param_strings.emplace_back(desc.names);
                    int num_args = desc.flag->NumArgs();
                    for (int i = 0; i < num_args; i++)
                    {
                        new_str += ' ';
                        new_str += desc.flag->ArgName(i);
                    }

                    if (new_str.size() > max_param_string_len)
                        max_param_string_len = new_str.size();
                }

                // Now print everything.

                Terminal::DefaultToConsole(stdout);

                std::printf("Usage:\n");
                for (std::size_t i = 0; i < param_strings.size(); i++)
                    std::printf("  %-*s  - %s\n", (int)max_param_string_len, param_strings[i].c_str(), flag_descs[i].help_text.c_str());

                // Exit immediately.
                // We could delay this until we finish parsing all flags, but it probably doesn't matter.
                std::exit(0);
            }
        };
    }

    void Parser::AddDefaultHelpFlag()
    {
        AddFlagLow<HelpFlag>("-h,--help", "Show this page.");
    }

    void Parser::Parse(int argc, const char *const *argv)
    {
        (void)argc; // This is unused, we rely on the null terminator in `argv`.
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

                        iter->second->ConsumeArgs(*this, {first_arg->data()});
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
                            iter->second->ConsumeArgs(*this, {});
                        else
                            iter->second->ConsumeArgs(*this, {argv + 1, std::size_t(num_args)});

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
                            iter->second->ConsumeArgs(*this, {});
                            this_flag++;
                            continue;
                        }

                        // One argument embedded in the flag?
                        if (num_args == 1 && this_flag[1])
                        {
                            // Pass the rest of the flag as the argument.
                            iter->second->ConsumeArgs(*this, {this_flag + 1});
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

                        iter->second->ConsumeArgs(*this, {argv + 1, std::size_t(num_args)});

                        argv += num_args;
                        break;
                    }
                    while (*this_flag);
                }
            }
            else
            {
                throw std::runtime_error(fmt::format("Positional arguments are not allowed, but got `{}`.", this_flag));
            }

            argv++;
        }
    }
}
