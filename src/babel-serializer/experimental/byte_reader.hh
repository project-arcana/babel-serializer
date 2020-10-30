#pragma once

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>
#include <clean-core/typedefs.hh>

namespace babel::experimental
{
/// A lightweight, high-performance non-owning binary reader interface
/// (abstraction over byte span and cursor position)
///
/// NOTE: - this object can be cheaply copied (does not touch the underlying data)
///       - as a view-type object, it MUST NOT outlive the data it views
///       - reading changes internal state, thus copies can be used for "snapshots" or rollback
///       - read functions are uniquely named to make accidental type errors harder
///
/// Error handling:
///   - bool read_xyz(T& v)
///     tries to read the type and returns false if not possible (without touching v)
///     NOTE: the _span versions copy data into the provided span
///
///   - T read_xyz()
///     asserts that read was successful
///     NOTE: the _span versions return a view into the byte_reader data
///
/// TODO: some version with a babel error handler?
/// TODO: peek_ api?
/// TODO: read_or api?
/// TODO: non-asserting non-copying api
struct byte_reader
{
    // error-returning reading API
public:
    /// tries to read as many raw bytes as specified by the given span
    /// on success, returns true and writes through the given span
    bool read_raw(cc::span<std::byte> target)
    {
        if (remaining_bytes() < target.size())
            return false;

        std::memcpy(target.data(), _curr, target.size());

        _curr += target.size();
        return true;
    }

    //
    // generic, trivially copyable types
    //

    template <class T>
    bool read_pod(T& v)
    {
        static_assert(!std::is_const_v<T>, "cannot copy into const object");
        static_assert(std::is_trivially_copyable_v<T>, "only works for trivially copyable types");
        return this->read_raw(cc::as_byte_span(v));
    }
    template <class T>
    bool read_pod_span(cc::span<T> v)
    {
        static_assert(!std::is_const_v<T>, "cannot copy into const object");
        static_assert(std::is_trivially_copyable_v<T>, "only works for trivially copyable types");
        return this->read_raw(cc::as_byte_span(v));
    }

    //
    // primitives
    //

    bool read_bool(bool& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_char(char& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_byte(std::byte& v) { return read_raw(cc::span<std::byte>(v)); }

    bool read_i8(cc::int8& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_i16(cc::int16& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_i32(cc::int32& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_i64(cc::int64& v) { return read_raw(cc::as_byte_span(v)); }

    bool read_u8(cc::uint8& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_u16(cc::uint16& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_u32(cc::uint32& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_u64(cc::uint64& v) { return read_raw(cc::as_byte_span(v)); }

    bool read_f32(float& v) { return read_raw(cc::as_byte_span(v)); }
    bool read_f64(double& v) { return read_raw(cc::as_byte_span(v)); }

    //
    // primitive spans
    // NOTE: uses the size of the given span!
    //

    bool read_bool_span(cc::span<bool> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_char_span(cc::span<char> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_byte_span(cc::span<std::byte> v) { return read_raw(v); }

    bool read_i8_span(cc::span<cc::int8> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_i16_span(cc::span<cc::int16> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_i32_span(cc::span<cc::int32> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_i64_span(cc::span<cc::int64> v) { return read_raw(cc::as_byte_span(v)); }

    bool read_u8_span(cc::span<cc::uint8> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_u16_span(cc::span<cc::uint16> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_u32_span(cc::span<cc::uint32> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_u64_span(cc::span<cc::uint64> v) { return read_raw(cc::as_byte_span(v)); }

    bool read_f32_span(cc::span<float> v) { return read_raw(cc::as_byte_span(v)); }
    bool read_f64_span(cc::span<double> v) { return read_raw(cc::as_byte_span(v)); }

    // asserting reading API
public:
    /// returns a given number of bytes
    cc::span<std::byte const> read_raw(size_t bytes)
    {
        CC_ASSERT(remaining_bytes() >= bytes);
        auto d = cc::span<std::byte const>(_curr, bytes);
        _curr += bytes;
        return d;
    }

    //
    // generic, trivially copyable types
    //

    template <class T>
    T const& read_pod()
    {
        static_assert(std::is_trivially_copyable_v<T>, "only works for trivially copyable types");
        auto byte_size = sizeof(T);
        CC_ASSERT(remaining_bytes() >= byte_size);
        auto d = _curr;
        _curr += byte_size;
        return *reinterpret_cast<T const*>(d);
    }
    template <class T>
    cc::span<T const> read_pod_span(size_t size)
    {
        static_assert(std::is_trivially_copyable_v<T>, "only works for trivially copyable types");
        auto byte_size = sizeof(T) * size;
        CC_ASSERT(remaining_bytes() >= byte_size);
        auto d = _curr;
        _curr += byte_size;
        return {reinterpret_cast<T const*>(d), size};
    }

    //
    // primitives
    //

    bool read_bool() { return read_pod<bool>(); }
    char read_char() { return read_pod<char>(); }
    std::byte read_byte() { return read_pod<std::byte>(); }

    cc::int8 read_i8() { return read_pod<cc::int8>(); }
    cc::int16 read_i16() { return read_pod<cc::int16>(); }
    cc::int32 read_i32() { return read_pod<cc::int32>(); }
    cc::int64 read_i64() { return read_pod<cc::int64>(); }

    cc::uint8 read_u8() { return read_pod<cc::uint8>(); }
    cc::uint16 read_u16() { return read_pod<cc::uint16>(); }
    cc::uint32 read_u32() { return read_pod<cc::uint32>(); }
    cc::uint64 read_u64() { return read_pod<cc::uint64>(); }

    float read_f32() { return read_pod<float>(); }
    double read_f64() { return read_pod<double>(); }

    //
    // primitive spans
    // NOTE: requires an externally provided size!
    //

    cc::span<bool const> read_bool_span(size_t count) { return read_pod_span<bool>(count); }
    cc::span<char const> read_char_span(size_t count) { return read_pod_span<char>(count); }
    cc::span<std::byte const> read_byte_span(size_t count) { return read_pod_span<std::byte>(count); }

    cc::span<cc::int8 const> read_i8_span(size_t count) { return read_pod_span<cc::int8>(count); }
    cc::span<cc::int16 const> read_i16_span(size_t count) { return read_pod_span<cc::int16>(count); }
    cc::span<cc::int32 const> read_i32_span(size_t count) { return read_pod_span<cc::int32>(count); }
    cc::span<cc::int64 const> read_i64_span(size_t count) { return read_pod_span<cc::int64>(count); }

    cc::span<cc::uint8 const> read_u8_span(size_t count) { return read_pod_span<cc::uint8>(count); }
    cc::span<cc::uint16 const> read_u16_span(size_t count) { return read_pod_span<cc::uint16>(count); }
    cc::span<cc::uint32 const> read_u32_span(size_t count) { return read_pod_span<cc::uint32>(count); }
    cc::span<cc::uint64 const> read_u64_span(size_t count) { return read_pod_span<cc::uint64>(count); }

    cc::span<float const> read_f32_span(size_t count) { return read_pod_span<float>(count); }
    cc::span<double const> read_f64_span(size_t count) { return read_pod_span<double>(count); }

    // string reading API
public:
    cc::string_view read_string_view(size_t chars)
    {
        CC_ASSERT(remaining_bytes() >= chars);
        auto d = _curr;
        _curr += chars;
        return cc::string_view(reinterpret_cast<char const*>(d), chars);
    }

    /// reads until \0, \n, \r or end of bytes and returns a view to it
    /// NOTE: does not include the end in the return type
    ///       BUT consumes them (so they are not read subsequently)
    ///           (consumes \r\n together)
    ///       works if no bytes remaining (returns empty string)
    cc::string_view read_line()
    {
        // find matching string
        auto start = _curr;
        while (_curr != _end)
        {
            auto c = char(*_curr);
            if (c == '\0' || c == '\r' || c == '\n')
                break;

            ++_curr;
        }
        auto sv = cc::string_view(reinterpret_cast<char const*>(start), reinterpret_cast<char const*>(_curr));

        if (_curr != _end) // stopped by \0, \r, \n
        {
            // consume first
            auto c = char(*_curr);
            ++_curr;

            // consume second if \r\n
            if (_curr != _end && c == '\r' && char(*_curr) == '\n')
                ++_curr;
        }

        return sv;
    }

    /// reads until after \0 and returns a pointer to the start (a C string)
    /// NOTE: does not include the end in the return type
    ///       BUT consumes them (so they are not read subsequently)
    ///           (consumes \r\n together)
    ///       works if no bytes remaining (returns empty string)
    char const* read_c_str()
    {
        // find matching string
        auto start = _curr;
        while (_curr != _end)
        {
            auto c = char(*_curr);
            ++_curr;

            if (c == '\0')
                break; // breaks after ++_curr so we consume the null terminator
        }

        CC_ASSERT(start != _end && "not null terminated");

        return reinterpret_cast<char const*>(start);
    }

    // other API
public:
    /// returns a view on the full data viewed by this reader
    cc::span<std::byte const> complete_data() const { return {_begin, _end}; }

    /// returns a view on the non-read data viewed by this reader
    cc::span<std::byte const> remaining_data() const { return {_curr, _end}; }

    /// returns the number of bytes that viewed by this reader
    size_t complete_bytes() const { return _end - _begin; }

    /// returns the number of bytes that can be read
    size_t remaining_bytes() const { return _end - _curr; }

    /// returns true if at least one byte can be read
    bool has_remaining_bytes() const { return _curr != _end; }

    /// returns a pointer to the current "head"
    /// i.e. the first byte of the remaining data
    /// NOTE: do not dereference the pointer if no bytes are remaining
    std::byte const* current_ptr() const { return _curr; }

    /// returns the current reader position as offset into complete_data()
    /// (complete_data()[position()] is the first UNREAD byte)
    /// NOTE: position can be complete_data().size() in which case no bytes can be read
    size_t position() const { return _curr - _begin; }

    /// resets the reader position to the beginning
    void reset_position() { _curr = _begin; }

    /// sets the next byte to be read by the reader
    void set_position(size_t p)
    {
        CC_ASSERT(p <= complete_bytes());
        _curr = _begin + p;
    }

    /// skips a certain number of bytes
    /// NOTE: asserts that enough bytes are remaining
    void skip(size_t bytes)
    {
        CC_ASSERT(bytes <= remaining_bytes());
        _curr += bytes;
    }

    // ctor
public:
    byte_reader() = default;
    byte_reader(cc::span<std::byte const> data) : _begin(data.data()), _curr(data.data()), _end(data.data() + data.size()) {}

private:
    std::byte const* _begin = nullptr;
    std::byte const* _curr = nullptr;
    std::byte const* _end = nullptr; // NOT included
};
}
