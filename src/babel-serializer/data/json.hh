#pragma once

#include <clean-core/is_range.hh>
#include <clean-core/span.hh>
#include <clean-core/stream_ref.hh>
#include <clean-core/string.hh>
#include <clean-core/to_string.hh>

#include <reflector/introspect.hh>

#include <babel-serializer/errors.hh>

namespace babel::json
{
struct read_config
{
    // empty for now
};
struct write_config
{
    /// if indent >= 0, outputs multi-line json with given indentation increase per level
    int indent = -1;
};

/// writes json to the stream from a given object
/// (uses rf::introspect to serialize the object)
template <class Obj>
void write(cc::stream_ref<char> output, Obj const& obj, write_config const& cfg = {});

/// creates a json string from a given object
/// (uses rf::introspect to serialize the object)
template <class Obj>
cc::string to_string(Obj const& obj, write_config const& cfg = {})
{
    cc::string result;
    babel::json::write([&result](cc::span<char const> s) { result += s; }, obj, cfg);
    return result;
}


// ====== IMPLEMENTATION ======

namespace detail
{
// TODO: proper escaping
struct json_writer_base
{
    void write(cc::stream_ref<char> output, bool v) { output << (v ? "true" : "false"); }
    void write(cc::stream_ref<char> output, std::nullptr_t) { output << "null"; }
    void write(cc::stream_ref<char> output, char c)
    {
        char v[] = {'"', c, '"'};
        output << v;
    }
    void write(cc::stream_ref<char> output, std::byte v) { cc::to_string(output, int(v)); }
    void write(cc::stream_ref<char> output, signed char v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, unsigned char v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, signed int v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, unsigned int v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, signed long v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, unsigned long v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, signed long long v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, unsigned long long v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, float v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, double v) { cc::to_string(output, v); }
    void write(cc::stream_ref<char> output, char const* v) { output << '"' << cc::string_view(v) << '"'; }
    void write(cc::stream_ref<char> output, cc::string_view v) { output << '"' << v << '"'; }
};

struct json_writer_compact : json_writer_base
{
    using json_writer_base::write;

    template <class Obj>
    void write(cc::stream_ref<char> output, Obj const& obj)
    {
        if constexpr (rf::is_introspectable<Obj>)
        {
            output << '{';
            auto first = true;
            rf::do_introspect(
                [&](auto& v, cc::string_view name) {
                    if (first)
                        first = false;
                    else
                        output << ',';
                    // TODO: escape
                    output << '"' << name << "\":";
                    write(output, v);
                },
                const_cast<Obj&>(obj)); // introspector will not modify!
            output << '}';
        }
        else if constexpr (cc::is_any_range<Obj>)
        {
            output << '[';
            auto first = true;
            for (auto const& v : obj)
            {
                if (first)
                    first = false;
                else
                    output << ',';
                write(output, v);
            }
            output << ']';
        }
        else
            static_assert(cc::always_false<Obj>, "obj is neither a range nor introspectable nor a predefined json type");
    }
};
struct json_writer_pretty : json_writer_base
{
    cc::string indent;
    int indent_inc;
    explicit json_writer_pretty(int inc) : indent_inc(inc) {}

    using json_writer_base::write;

    template <class Obj>
    void write(cc::stream_ref<char> output, Obj const& obj)
    {
        if constexpr (rf::is_introspectable<Obj>)
        {
            indent.resize(indent.size() + indent_inc, ' ');
            output << "{\n";
            auto first = true;
            rf::do_introspect(
                [&](auto& v, cc::string_view name) {
                    if (first)
                        first = false;
                    else
                        output << ",\n";
                    output << indent;
                    // TODO: escape
                    output << '"' << name << "\": ";
                    write(output, v);
                },
                const_cast<Obj&>(obj)); // introspector will not modify!
            indent.resize(indent.size() - indent_inc);
            if (!first)
                output << '\n';
            output << indent << '}';
        }
        else if constexpr (cc::is_any_range<Obj>)
        {
            indent.resize(indent.size() + indent_inc, ' ');
            output << "[\n";
            auto first = true;
            for (auto const& v : obj)
            {
                if (first)
                    first = false;
                else
                    output << ",\n";
                output << indent;

                write(output, v);
            }
            indent.resize(indent.size() - indent_inc);
            if (!first)
                output << '\n';
            output << indent << ']';
        }
        else
            static_assert(cc::always_false<Obj>, "obj is neither a range nor introspectable nor a predefined json type");
    }
};
}

template <class Obj>
void write(cc::stream_ref<char> output, Obj const& obj, write_config const& cfg)
{
    if (cfg.indent >= 0)
        detail::json_writer_pretty{cfg.indent}.write(output, obj);
    else
        detail::json_writer_compact{}.write(output, obj);
}
}
