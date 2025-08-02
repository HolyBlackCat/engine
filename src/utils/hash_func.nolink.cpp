#include "utils/hash_func.h"

static_assert(em::Hash(std::string_view(""), 0) == 0);
static_assert(em::Hash(std::string_view("abcd"), 42) == 3898664396);
static_assert(em::Hash(std::string_view("abcde"), 42) == 2933533680);
static_assert(em::Hash(std::string_view("abcdef"), 42) == 2449278475);
static_assert(em::Hash(std::string_view("abcdefg"), 42) == 1781200409);
