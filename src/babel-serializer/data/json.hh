#pragma once

#include <clean-core/fwd.hh>
#include <clean-core/is_range.hh>
#include <clean-core/span.hh>
#include <clean-core/stream_ref.hh>
#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/to_string.hh>
#include <clean-core/vector.hh>

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

enum class node_type
{
    null,
    number,
    string,
    boolean,
    array,
    object,
};

/// a non-owning read-only view on a json string
struct json_ref
{
    /// a single node in a json tree
    /// NOTE: objects are have twice as many children as could be expected
    ///       they are actually pairs of key (string) -> value (json)
    struct node
    {
        // TODO: type in a separate array would save some space
        //      (or merged with next_sibling)
        node_type type;
        size_t next_sibling = 0; ///< if > 0 points to the next node of the same parent
        union {
            cc::string_view token; ///< only valid for leaf nodes
            size_t first_child;    ///< only valid for composite nodes
        };

        bool is_null() const { return type == node_type::null; }
        bool is_number() const { return type == node_type::number; }
        bool is_string() const { return type == node_type::string; }
        bool is_boolean() const { return type == node_type::boolean; }
        bool is_array() const { return type == node_type::array; }
        bool is_object() const { return type == node_type::object; }

        bool is_composite() const { return type >= node_type::array; }
        bool is_leaf() const { return type < node_type::array; }

        bool get_boolean() const
        {
            CC_ASSERT(is_boolean());
            return token[0] == 't';
        }
        cc::string get_string() const; ///< returns the unescaped string content
        int get_int();
        float get_float();
        double get_double();
        cc::int64 get_int64();
        cc::uint64 get_uint64();

        node() {}
    };

    /// flat list of all nodes
    cc::vector<node> nodes;

    node const& root() const { return nodes.front(); }
};

/// parses the given json string and returns a json reference,
/// a read-only non-owning view on the json
/// NOTE: - does not convert numbers, only deduces types and structure
///       - the reference points into the json string (which must outlive the json_ref)
json_ref read_ref(cc::string_view json, read_config const& cfg = {}, error_handler on_error = default_error_handler);

// ====== IMPLEMENTATION ======

namespace detail
{
template <class T, class = void>
struct is_optional_t : std::false_type
{
};
template <class T>
struct is_optional_t<cc::optional<T>> : std::true_type
{
};

template <class T, class = void>
struct is_map_t : std::false_type
{
};
template <class A, class B, class HashT, class EqualT>
struct is_map_t<cc::map<A, B, HashT, EqualT>> : std::true_type
{
};

/// writes a string as "abc" to the output
/// escapes reserved json character using backslash
void write_escaped_string(cc::stream_ref<char> output, cc::string_view s);

// TODO: maybe use template specialization to customize json read/write
struct json_writer_base
{
    void write(cc::stream_ref<char> output, bool v) { output << (v ? "true" : "false"); }
    void write(cc::stream_ref<char> output, cc::nullopt_t const&) { output << "null"; }
    void write(cc::stream_ref<char> output, char c) { write_escaped_string(output, cc::string_view(&c, 1)); }
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
    void write(cc::stream_ref<char> output, char const* v) { write_escaped_string(output, v); }
    void write(cc::stream_ref<char> output, cc::string_view v) { write_escaped_string(output, v); }

protected:
    template <class T>
    void write_optional(cc::stream_ref<char> output, T const& v)
    {
        if (v.has_value())
            this->write(output, v.value());
        else
            output << "null";
    }
};

struct json_writer_compact : json_writer_base
{
    using json_writer_base::write;

    template <class Obj>
    void write(cc::stream_ref<char> output, Obj const& obj)
    {
        if constexpr (std::is_constructible_v<cc::string_view, Obj const&>)
        {
            write_escaped_string(output, cc::string_view(obj));
        }
        else if constexpr (is_optional_t<Obj>::value)
        {
            write_optional(output, obj);
        }
        else if constexpr (is_map_t<Obj>::value)
        {
            using key_t = typename Obj::key_t;
            if constexpr (std::is_convertible_v<key_t, cc::string_view>)
            {
                output << '{';
                auto first = true;
                for (auto&& [key, value] : obj)
                {
                    if (first)
                        first = false;
                    else
                        output << ',';
                    write_escaped_string(output, key);
                    output << ':';
                    write(output, value);
                }
                output << '}';
            }
            else
            {
                static_assert(cc::always_false<Obj>, "only map<string-like, Value> supported for now");
            }
        }
        else if constexpr (rf::is_introspectable<Obj>)
        {
            output << '{';
            auto first = true;
            rf::do_introspect(
                [&](auto& v, cc::string_view name) {
                    if (first)
                        first = false;
                    else
                        output << ',';
                    write_escaped_string(output, name);
                    output << ':';
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
        if constexpr (std::is_constructible_v<cc::string_view, Obj const&>)
        {
            write_escaped_string(output, cc::string_view(obj));
        }
        else if constexpr (is_optional_t<Obj>::value)
        {
            write_optional(output, obj);
        }
        else if constexpr (is_map_t<Obj>::value)
        {
            using key_t = typename Obj::key_t;
            if constexpr (std::is_convertible_v<key_t, cc::string_view>)
            {
                indent.resize(indent.size() + indent_inc, ' ');
                output << "{\n";
                auto first = true;
                for (auto&& [key, value] : obj)
                {
                    if (first)
                        first = false;
                    else
                        output << ",\n";
                    output << indent;
                    write_escaped_string(output, key);
                    output << ": ";
                    write(output, value);
                }
                indent.resize(indent.size() - indent_inc);
                if (!first)
                    output << '\n';
                output << indent << '}';
            }
            else
            {
                static_assert(cc::always_false<Obj>, "only map<string-like, Value> supported for now");
            }
        }
        else if constexpr (rf::is_introspectable<Obj>)
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
                    write_escaped_string(output, name);
                    output << ": ";
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
