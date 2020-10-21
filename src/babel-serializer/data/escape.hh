#pragma once

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>

namespace babel
{
/// escapes a string so that it can be written as json,
/// e.g. hello -> "hello"
///      ha"s\ -> "ha\"s\\"
[[nodiscard]] cc::string escape_json_string(cc::string_view s);

/// takes a json-escaped string and returns the original, unescaped version
/// (inverted version of escape_json_string)
/// NOTE: s starts and ends with "
[[nodiscard]] cc::string unescape_json_string(cc::string_view s);
}
