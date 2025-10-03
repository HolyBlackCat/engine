#include "utils/command_line_parser.h"

#include "em/minitest.hpp"

using namespace em;

#include <iostream>

EM_TEST( wrong_usage )
{
    CommandLine::Parser p;
    p.AddFlag("-a", "desc", [&]{});
    EM_MUST_THROW( p.AddFlag("a", "desc", [&]{}) )(std::logic_error("Bad usage of the command line parser: Each command line flag must start with a `-` or `--`, but got `a`."));
    EM_MUST_THROW( p.AddFlag("-a", "desc", [&]{}) )(std::logic_error("Bad usage of the command line parser: Duplicate command line flag: `a`."));
    EM_MUST_THROW( p.AddFlag("--a", "desc", [&]{}) )(std::logic_error("Bad usage of the command line parser: A flag starting with `--` must have a multi-letter name, but got `--a`."));
    EM_MUST_THROW( p.AddFlag("-aa", "desc", [&]{}) )(std::logic_error("Bad usage of the command line parser: A flag starting with `-` must have exactly one letter in the name, but got `-aa`."));
}

EM_TEST( parsing )
{
    std::string log;

    CommandLine::Parser p;
    p.AddFlag("--alpha,-A", "alpha desc", [&](){log += "{}\n";});
    p.AddFlag<std::string>("--beta,-B", "beta_arg1", "beta desc", [&](std::string a){log += "[" + a + "]\n";});
    p.AddFlag<std::string, std::string>("--gamma,-G", "gamma_arg1", "gamma_arg2", "gamma desc", [&](std::string a, std::string b){log += "[" + a + "|" + b + "]\n";});

    EM_MUST_THROW( p.Parse(std::array{"./app", "a", (const char *)nullptr}.data()) )(std::runtime_error("Positional arguments are not allowed, but got `a`."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "--a", (const char *)nullptr}.data()) )(std::runtime_error("No such flag: `--a`."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "--A", (const char *)nullptr}.data()) )(std::runtime_error("No such flag: `--A`, did you mean `-A`."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "--alphafoo", (const char *)nullptr}.data()) )(std::runtime_error("No such flag: `--alphafoo`."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "--alpha=foo", (const char *)nullptr}.data()) )(std::runtime_error("Flag `--alpha` doesn't accept arguments."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "--beta", (const char *)nullptr}.data()) )(std::runtime_error("Flag `--beta` needs 1 argument, but got 0."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "--gamma", (const char *)nullptr}.data()) )(std::runtime_error("Flag `--gamma` needs 2 arguments, but got 0."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "--gamma", "-x", (const char *)nullptr}.data()) )(std::runtime_error("Flag `--gamma` needs 2 arguments, but got 1."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "-", (const char *)nullptr}.data()) )(std::runtime_error("No such flag: `-`."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "-a", (const char *)nullptr}.data()) )(std::runtime_error("No such flag: `-a`."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "-Gx", (const char *)nullptr}.data()) )(std::runtime_error("Flag `-G` needs more than one argument, so they can't follow it immediately, and must be passed as separate arguments."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "-B", (const char *)nullptr}.data()) )(std::runtime_error("Flag `-B` needs 1 argument, but got 0."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "-G", (const char *)nullptr}.data()) )(std::runtime_error("Flag `-G` needs 2 arguments, but got 0."));
    EM_MUST_THROW( p.Parse(std::array{"./app", "-G", "-x", (const char *)nullptr}.data()) )(std::runtime_error("Flag `-G` needs 2 arguments, but got 1."));

    log.clear();

    // Some valid usages.
    p.Parse(std::array{"./app", "--alpha", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "{}\n");
    p.Parse(std::array{"./app", "--alpha", "--alpha", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "{}\n{}\n");
    p.Parse(std::array{"./app", "-A", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "{}\n");
    p.Parse(std::array{"./app", "-AA", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "{}\n{}\n");
    p.Parse(std::array{"./app", "-A", "-A", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "{}\n{}\n");

    p.Parse(std::array{"./app", "--beta", "-x", "--beta", "-y", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[-x]\n[-y]\n");
    p.Parse(std::array{"./app", "--beta", "-x", "--beta", "-y", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[-x]\n[-y]\n");
    p.Parse(std::array{"./app", "--beta=-x", "--beta=-y", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[-x]\n[-y]\n");
    p.Parse(std::array{"./app", "--beta=", "--beta=", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[]\n[]\n");
    p.Parse(std::array{"./app", "--gamma", "-x", "-y", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[-x|-y]\n");

    p.Parse(std::array{"./app", "-BA", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[A]\n");
    p.Parse(std::array{"./app", "-B", "A", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[A]\n");
    p.Parse(std::array{"./app", "-G", "A", "B", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "[A|B]\n");

    p.Parse(std::array{"./app", "-AABAA", (const char *)nullptr}.data());
    EM_CHECK(std::exchange(log, {}) == "{}\n{}\n[AA]\n");
}
