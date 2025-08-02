#include "utils/byte_view.h"

static_assert(std::is_constructible_v<em::mut_byte_view, std::string>);
static_assert(std::is_constructible_v<em::const_byte_view, std::string>);
static_assert(!std::is_constructible_v<em::mut_byte_view, std::string_view>);
static_assert(std::is_constructible_v<em::const_byte_view, std::string_view>);

static_assert(!std::is_constructible_v<em::mut_byte_view, em::const_byte_view>);
static_assert(std::is_constructible_v<em::const_byte_view, em::mut_byte_view>);

static_assert(!em::const_byte_view_reinterpretable_as_range_of<char>);
static_assert(em::const_byte_view_reinterpretable_as_range_of<const char>);
static_assert(em::mut_byte_view_reinterpretable_as_range_of<char>);
static_assert(em::mut_byte_view_reinterpretable_as_range_of<const char>);
