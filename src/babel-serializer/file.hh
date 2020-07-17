#pragma once

#include <clean-core/range_ref.hh>
#include <clean-core/stream_ref.hh>
#include <clean-core/string_view.hh>

#include <babel-serializer/errors.hh>

namespace babel::file
{
/// reads a file writes all bytes in the provided stream
void read(cc::stream_ref<std::byte> out, cc::string_view filename, error_handler on_error = default_error_handler);

/// reads a file and returns the content as string
cc::string read_all_text(cc::string_view filename, error_handler on_error = default_error_handler);

/// reads a file and returns the content as array of bytes
cc::array<std::byte> read_all_bytes(cc::string_view filename, error_handler on_error = default_error_handler);

/// writes the given binary data to a file
void write(cc::string_view filename, cc::span<std::byte const> data, error_handler on_error = default_error_handler);

/// writes the given string data to a file
void write(cc::string_view filename, cc::string_view data, error_handler on_error = default_error_handler);

/// writes the given lines to a file
void write_lines(cc::string_view filename, cc::range_ref<cc::string_view> lines, cc::string_view line_ending = "\n", error_handler on_error = default_error_handler);
}
