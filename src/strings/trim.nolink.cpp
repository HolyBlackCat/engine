#include "strings/trim.h"

using em::operator""_compact; // NOLINT(misc-unused-using-decls) - A clang-tidy bug?

static_assert(""_compact == "");
static_assert("\n"_compact == "");
static_assert("\n\n"_compact == "");
static_assert("\n\n\n"_compact == "");
static_assert("a"_compact == "a");
static_assert("a\n"_compact == "a\n");
static_assert("\n\n\na\n\n\n"_compact == "a\n");
static_assert("   a   \t  \r   "_compact == "a");
static_assert("\t\t\ta   \t  \r   "_compact == "a");
static_assert("a\n  b"_compact == "a\n  b");
static_assert("    \t  \r   \n      \n  a  \n    b   \n   \n       \r \t"_compact == "a\n  b\n");


static_assert(
    R"(
        int main()
        {
            std::cout << "Hello!\n";
        }
    )"_compact
    ==
R"(int main()
{
    std::cout << "Hello!\n";
}
)"
    );
