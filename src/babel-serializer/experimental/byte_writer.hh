#pragma once

#include <clean-core/move.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>
#include <clean-core/typedefs.hh>

namespace babel::experimental
{
/// A lightweight, high-performance non-owning binary writer interface
/// (abstraction over a functor that can write cc::span<std::byte const>)
template <class ByteSpanWriter>
struct byte_writer
{
    static_assert(std::is_invocable_v<ByteSpanWriter, cc::span<std::byte const>>, "ByteSpanWriter must be callable with a byte span");
    using return_t = decltype(std::declval<ByteSpanWriter>()(cc::span<std::byte const>()));

    return_t write_raw(cc::span<std::byte const> bytes) { return _write(bytes); }

    //
    // generic, trivially copyable types
    //

    template <class T>
    return_t write_pod(T const& v)
    {
        static_assert(std::is_trivially_copyable_v<T>, "only works for trivially copyable types");
        return this->write_raw(cc::as_byte_span(v));
    }
    template <class T>
    return_t write_pod_span(cc::span<T const> v)
    {
        static_assert(std::is_trivially_copyable_v<T>, "only works for trivially copyable types");
        return this->write_raw(cc::as_byte_span(v));
    }

    //
    // primitives
    //

    return_t write_bool(bool v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_char(char v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_byte(std::byte v) { return write_raw(cc::span<std::byte>(v)); }

    return_t write_i8(cc::int8 v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_i16(cc::int16 v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_i32(cc::int32 v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_i64(cc::int64 v) { return write_raw(cc::as_byte_span(v)); }

    return_t write_u8(cc::uint8 v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_u16(cc::uint16 v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_u32(cc::uint32 v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_u64(cc::uint64 v) { return write_raw(cc::as_byte_span(v)); }

    return_t write_f32(float v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_f64(double v) { return write_raw(cc::as_byte_span(v)); }

    //
    // primitive spans
    // NOTE: uses the size of the given span!
    //

    return_t write_bool_span(cc::span<bool const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_char_span(cc::span<char const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_byte_span(cc::span<std::byte const> v) { return write_raw(v); }

    return_t write_i8_span(cc::span<cc::int8 const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_i16_span(cc::span<cc::int16 const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_i32_span(cc::span<cc::int32 const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_i64_span(cc::span<cc::int64 const> v) { return write_raw(cc::as_byte_span(v)); }

    return_t write_u8_span(cc::span<cc::uint8 const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_u16_span(cc::span<cc::uint16 const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_u32_span(cc::span<cc::uint32 const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_u64_span(cc::span<cc::uint64 const> v) { return write_raw(cc::as_byte_span(v)); }

    return_t write_f32_span(cc::span<float const> v) { return write_raw(cc::as_byte_span(v)); }
    return_t write_f64_span(cc::span<double const> v) { return write_raw(cc::as_byte_span(v)); }

    // string api
public:
    /// writes the bytes of the given string, WITHOUT any terminator at the end
    return_t write_string(cc::string_view s) { return write_raw(cc::as_byte_span(s)); }

    // ctor
public:
    byte_writer(ByteSpanWriter w) : _write(cc::move(w)) {}

private:
    ByteSpanWriter _write;
};
}
