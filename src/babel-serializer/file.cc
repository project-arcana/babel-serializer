#include "file.hh"

#include <fstream>

#include <clean-core/array.hh>
#include <clean-core/format.hh>
#include <clean-core/macros.hh>
#include <clean-core/string.hh>

void babel::file::read(cc::stream_ref<std::byte> out, cc::string_view filename, error_handler on_error)
{
    std::ifstream file(cc::string(filename).c_str(), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return;
    }

    std::byte buffer[1024 * 4];
    while (file)
    {
        auto n = file.readsome(reinterpret_cast<char*>(buffer), sizeof(buffer));
        out << cc::span(buffer, n);
    }
}

cc::string babel::file::read_all_text(cc::string_view filename, babel::error_handler on_error)
{
    std::ifstream file(cc::string(filename).c_str(), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return "";
    }

    file.seekg(0, std::ios_base::end);
    auto size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    auto result = cc::string::uninitialized(size);
    file.read(result.data(), size);
    return result;
}

cc::array<std::byte> babel::file::read_all_bytes(cc::string_view filename, babel::error_handler on_error)
{
    std::ifstream file(cc::string(filename).c_str(), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return {};
    }

    file.seekg(0, std::ios_base::end);
    auto size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    auto result = cc::array<std::byte>::uninitialized(size);
    file.read(reinterpret_cast<char*>(result.data()), size);
    return result;
}

void babel::file::write(cc::string_view filename, cc::span<std::byte const> data, babel::error_handler on_error)
{
    std::ofstream file(cc::string(filename).c_str(), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("cannot write to file '{}'", filename), severity::error);
        return;
    }

    file.write(reinterpret_cast<char const*>(data.data()), data.size());
}

void babel::file::write(cc::string_view filename, cc::string_view data, babel::error_handler on_error)
{
    write(filename, cc::as_byte_span(data), on_error);
}

void babel::file::write_lines(cc::string_view filename, cc::range_ref<cc::string_view> lines, cc::string_view line_ending, babel::error_handler on_error)
{
    std::ofstream file(cc::string(filename).c_str(), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("cannot write to file '{}'", filename), severity::error);
        return;
    }

    auto first = true;
    lines.for_each([&](cc::string_view line) {
        if (first)
            first = false;
        else if (!line_ending.empty())
            file.write(line_ending.data(), line_ending.size());

        if (!line.empty())
            file.write(line.data(), line.size());
    });
}

bool babel::file::exists(cc::string_view filename)
{
    // TODO: replace by filestream api
    return std::ifstream(cc::string(filename).c_str()).good();
}

babel::file::file_output_stream::file_output_stream(const char* filename)
{
#ifdef CC_OS_WINDOWS
    errno_t err = ::fopen_s(&_file, filename, "wb");
    if (err != 0)
        _file = nullptr;
#else
    _file = std::fopen(filename, "wb");
#endif
}

babel::file::file_output_stream::file_output_stream(cc::string_view filename) : file_output_stream(cc::string(filename).c_str()) {}
