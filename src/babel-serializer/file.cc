#include "file.hh"

#include <fstream>

#if defined(CC_OS_WINDOWS)
#include <clean-core/native/win32_sanitized.hh>

#include <WinBase.h>
#include <fileapi.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <clean-core/array.hh>
#include <clean-core/assertf.hh>
#include <clean-core/format.hh>
#include <clean-core/macros.hh>
#include <clean-core/string.hh>
#include <clean-core/temp_cstr.hh>

void babel::file::read(cc::stream_ref<std::byte> out, cc::string_view filename, error_handler on_error)
{
    std::ifstream file(cc::temp_cstr(filename), std::ios_base::binary);
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
    std::ifstream file(cc::temp_cstr(filename), std::ios_base::binary);
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

cc::alloc_array<char> babel::file::read_all_text(cc::string_view filename, cc::allocator* alloc, babel::error_handler on_error)
{
#ifdef CC_OS_WINDOWS
    std::FILE* fp = nullptr;
    errno_t err = ::fopen_s(&fp, cc::temp_cstr(filename), "rb");
    if (err != 0)
        fp = nullptr;
#else
    std::FILE* fp = std::fopen(cc::temp_cstr(filename), "rb");
#endif

    if (!fp)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return {};
    }

    std::fseek(fp, 0, SEEK_END);
    auto res = cc::alloc_array<char>::uninitialized(std::ftell(fp) + 1, alloc);
    std::rewind(fp);
    std::fread(&res[0], 1, res.size() - 1, fp);
    std::fclose(fp);
    res[res.size() - 1] = '\0';
    return res;
}

cc::array<std::byte> babel::file::read_all_bytes(cc::string_view filename, babel::error_handler on_error)
{
    std::ifstream file(cc::temp_cstr(filename), std::ios_base::binary);
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

cc::alloc_array<std::byte> babel::file::read_all_bytes(cc::string_view filename, cc::allocator* alloc, babel::error_handler on_error)
{
#ifdef CC_OS_WINDOWS
    std::FILE* fp = nullptr;
    errno_t err = ::fopen_s(&fp, cc::temp_cstr(filename), "rb");
    if (err != 0)
        fp = nullptr;
#else
    std::FILE* fp = std::fopen(cc::temp_cstr(filename), "rb");
#endif

    if (!fp)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return {};
    }

    std::fseek(fp, 0, SEEK_END);
    auto res = cc::alloc_array<std::byte>::uninitialized(std::ftell(fp), alloc);
    std::rewind(fp);
    std::fread(&res[0], 1, res.size(), fp);
    std::fclose(fp);
    return res;
}

void babel::file::write(cc::string_view filename, cc::span<std::byte const> data, babel::error_handler on_error)
{
    std::ofstream file(cc::temp_cstr(filename), std::ios_base::binary);
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
    std::ofstream file(cc::temp_cstr(filename), std::ios_base::binary);
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
    return std::ifstream(cc::temp_cstr(filename)).good();
}

babel::file::file_output_stream::file_output_stream(cc::string_view filename)
{
#ifdef CC_OS_WINDOWS
    errno_t err = ::fopen_s(&_file, cc::temp_cstr(filename), "wb");
    if (err != 0)
        _file = nullptr;
#else
    _file = std::fopen(cc::temp_cstr(filename), "wb");
#endif
}

babel::file::memory_mapped_file::memory_mapped_file(cc::string_view filepath)
{
#if defined(CC_OS_WINDOWS)

    // todo: error reporting
    // todo: arbitrarily long paths, see: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
    _file_handle = CreateFileA(cc::string(filepath).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    CC_ASSERT(_file_handle != INVALID_HANDLE_VALUE && "failed to open file");

    DWORD high = 0;
    DWORD low = GetFileSize(_file_handle, &high);
    size_t size = (size_t(high) << 32) + size_t(low);

    _file_mapping_handle = CreateFileMappingA(_file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
    CC_ASSERT(_file_mapping_handle != INVALID_HANDLE_VALUE && "failed to create file mapping");

    auto const file_view = MapViewOfFile(_file_mapping_handle, FILE_MAP_READ, 0, 0, 0);
    CC_ASSERT(file_view != INVALID_HANDLE_VALUE && "failed to create file view");

    *static_cast<cc::span<std::byte>*>(this) = cc::span<std::byte>{static_cast<std::byte*>(file_view), size};
#else
    _file_descriptor = open(cc::string(filepath).c_str(), O_RDONLY);

    CC_ASSERT(_file_descriptor != -1 && "failed to open file");

    struct stat st;
    auto const stat_error = fstat(_file_descriptor, &st);
    CC_ASSERT(stat_error == 0 && "failed to read file size");
    size_t const size = st.st_size;

    void* const mmap_result = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, _file_descriptor, 0);
    CC_ASSERTF(mmap_result != MAP_FAILED, "Failed to map file to memory {}", filepath);

    *static_cast<cc::span<std::byte>*>(this) = cc::span<std::byte>{static_cast<std::byte*>(mmap_result), size};
#endif
}

babel::file::memory_mapped_file::~memory_mapped_file()
{
#if defined(CC_OS_WINDOWS)
    if (data() && data() != INVALID_HANDLE_VALUE)
        UnmapViewOfFile(data());
    if (_file_mapping_handle && _file_mapping_handle != INVALID_HANDLE_VALUE)
        CloseHandle(_file_mapping_handle);
    if (_file_handle && _file_handle != INVALID_HANDLE_VALUE)
        CloseHandle(_file_handle);
#else
    if (data())
        munmap(data(), size());
    if (_file_descriptor != -1)
        close(_file_descriptor);
#endif
}
