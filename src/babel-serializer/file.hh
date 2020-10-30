#pragma once

#include <cstdio>

#include <clean-core/alloc_array.hh>
#include <clean-core/range_ref.hh>
#include <clean-core/stream_ref.hh>
#include <clean-core/string_view.hh>

#include <babel-serializer/errors.hh>

namespace babel::file
{
/// returns true if the file exists and can be read
bool exists(cc::string_view filename);

/// reads a file and writes all bytes in the provided stream (buffered)
void read(cc::stream_ref<std::byte> out, cc::string_view filename, error_handler on_error = default_error_handler);

/// reads a file and returns the content as a string (null terminated)
cc::string read_all_text(cc::string_view filename, error_handler on_error = default_error_handler);
cc::alloc_array<char> read_all_text(char const* filename, cc::allocator* alloc, error_handler on_error = default_error_handler);
cc::alloc_array<char> read_all_text(cc::string_view filename, cc::allocator* alloc, error_handler on_error = default_error_handler);

/// reads a file and returns the content as an array of bytes
cc::array<std::byte> read_all_bytes(cc::string_view filename, error_handler on_error = default_error_handler);
/// reads a file and returns the content as an allocator-backed array of chars (null terminated)
cc::alloc_array<std::byte> read_all_bytes(char const* filename, cc::allocator* alloc, error_handler on_error = default_error_handler);
cc::alloc_array<std::byte> read_all_bytes(cc::string_view filename, cc::allocator* alloc, error_handler on_error = default_error_handler);

/// writes the given binary data to a file
void write(cc::string_view filename, cc::span<std::byte const> data, error_handler on_error = default_error_handler);

/// writes the given string data to a file
void write(cc::string_view filename, cc::string_view data, error_handler on_error = default_error_handler);

/// writes the given lines to a file
void write_lines(cc::string_view filename, cc::range_ref<cc::string_view> lines, cc::string_view line_ending = "\n", error_handler on_error = default_error_handler);

/// an output file stream that can be used as cc::stream_ref<char>, cc::stream_ref<std::byte>, cc::string_stream_ref
/// NOTE: overwrites existing files
struct file_output_stream
{
    file_output_stream(char const* filename);
    file_output_stream(cc::string_view filename);

    ~file_output_stream()
    {
        if (_file)
        {
            std::fclose(_file);
        }
    }

    void operator()(cc::string_view content) { fwrite(content.data(), 1, content.size(), _file); }
    void operator()(cc::span<char const> content) { fwrite(content.data(), 1, content.size(), _file); }
    void operator()(cc::span<std::byte const> content) { fwrite(content.data(), 1, content.size(), _file); }

    bool valid() const { return _file != nullptr; }

private:
    FILE* _file = nullptr;
};
}
