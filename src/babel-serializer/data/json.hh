#pragma once

#include <cstdint>

#include <clean-core/collection_traits.hh>
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

/**
 * JSON serialization and deserialization
 *
 * Missing features:
 * - enums via string values
 * - non-introspect customization
 * - full unicode
 * - writing custom json (not via introspect)
 */

namespace babel::json
{
struct read_config
{
    /// produces warnings if the json contains more data than needed for the serialization
    /// (e.g. if an object has extra fields)
    bool warn_on_extra_data = true;

    /// produces warnings if the json is missing some data
    /// (e.g. if an object has too few fields)
    /// (behavior on missing data can be controlled via init_missing_data)
    bool warn_on_missing_data = false;

    /// if true, missing data is default-initialized
    /// otherwise, missing data is left as-is (relevant for read_to with prefilled object)
    bool init_missing_data = false;

    // TODO: comments
    // TODO: enums via strings
};
struct write_config
{
    /// if indent >= 0, outputs multi-line json with given indentation increase per level
    int indent = -1;
};

/// writes json to the stream from a given object
/// (uses rf::introspect to serialize the object)
template <class Obj>
void write(cc::string_stream_ref output, Obj const& obj, write_config const& cfg = {});

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
    struct child_range
    {
        size_t first_idx = 0;
        size_t count = 0;
    };

    /// a single node in a json tree
    /// NOTE: objects are have twice as many children as could be expected
    ///       they are actually pairs of key (string) -> value (json)
    /// IMPORTANT: children are NOT contiguous, one need to use next_sibling
    struct node
    {
        // TODO: type in a separate array would save some space
        //      (or merged with next_sibling)
        node_type type;
        size_t next_sibling = 0; ///< if > 0 points to the next node of the same parent
        cc::string_view token;
        size_t first_child = 0; ///< only valid for composite nodes
        size_t child_count = 0; ///< only valid for composite nodes (for object: number of keys)

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
        int32_t get_int() const;
        float get_float() const;
        double get_double() const;
        int64_t get_int64() const;
        uint64_t get_uint64() const;

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

/// parses the given json and deserializes it into the object
/// (uses rf::introspect for introspectable objects)
/// NOTE: read_config can be used to tweak behavior
template <class Obj>
void read_to(Obj& obj, cc::string_view json, read_config const& cfg = {}, error_handler on_error = default_error_handler);

/// same as read_to but returns the object instead
/// NOTE: Obj must be default-constructible
template <class Obj>
Obj read(cc::string_view json, read_config const& cfg = {}, error_handler on_error = default_error_handler)
{
    Obj obj;
    babel::json::read_to(obj, json, cfg, on_error);
    return obj;
}

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
void write_escaped_string(cc::string_stream_ref output, cc::string_view s);

// TODO: maybe use template specialization to customize json read/write
struct json_writer_base
{
    void write(cc::string_stream_ref output, bool v) { output << (v ? "true" : "false"); }
    void write(cc::string_stream_ref output, cc::nullopt_t const&) { output << "null"; }
    void write(cc::string_stream_ref output, char c) { write_escaped_string(output, cc::string_view(&c, 1)); }
    void write(cc::string_stream_ref output, std::byte v) { cc::to_string(output, int(v)); }
    void write(cc::string_stream_ref output, int8_t v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, uint8_t v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, int32_t v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, uint32_t v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, int64_t v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, uint64_t v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, float v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, double v) { cc::to_string(output, v); }
    void write(cc::string_stream_ref output, char const* v) { write_escaped_string(output, v); }
    void write(cc::string_stream_ref output, cc::string_view v) { write_escaped_string(output, v); }

protected:
    template <class T>
    void write_optional(cc::string_stream_ref output, T const& v)
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
    void write(cc::string_stream_ref output, Obj const& obj)
    {
        if constexpr (std::is_enum_v<Obj>)
        {
            cc::to_string(output, std::underlying_type_t<Obj>(obj));
        }
        else if constexpr (std::is_constructible_v<cc::string_view, Obj const&>)
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
                output << '[';
                auto first = true;
                for (auto&& [key, value] : obj)
                {
                    if (first)
                        first = false;
                    else
                        output << ',';
                    output << '[';
                    write(output, key);
                    output << ',';
                    write(output, value);
                    output << ']';
                }
                output << ']';
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
    void write(cc::string_stream_ref output, Obj const& obj)
    {
        if constexpr (std::is_enum_v<Obj>)
        {
            cc::to_string(output, std::underlying_type_t<Obj>(obj));
        }
        else if constexpr (std::is_constructible_v<cc::string_view, Obj const&>)
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
                indent.resize(indent.size() + indent_inc, ' ');
                output << "[\n";
                auto first = true;
                for (auto&& [key, value] : obj)
                {
                    if (first)
                        first = false;
                    else
                        output << ",\n";
                    output << indent;
                    output << '[';
                    write(output, key);
                    output << ", ";
                    write(output, value);
                    output << ']';
                }
                indent.resize(indent.size() - indent_inc);
                if (!first)
                    output << '\n';
                output << indent << ']';
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

// TODO: more error handling, especially for primitive parsing
struct json_deserializer
{
    cc::span<std::byte const> all_data;
    read_config const& cfg;
    error_handler on_error;
    json_ref const& jref;

    void deserialize(json_ref::node const& n, bool& v);
    void deserialize(json_ref::node const& n, char& v);
    void deserialize(json_ref::node const& n, std::byte& v);
    void deserialize(json_ref::node const& n, int8_t& v);
    void deserialize(json_ref::node const& n, uint8_t& v);
    void deserialize(json_ref::node const& n, int16_t& v);
    void deserialize(json_ref::node const& n, uint16_t& v);
    void deserialize(json_ref::node const& n, int32_t& v);
    void deserialize(json_ref::node const& n, uint32_t& v);
    void deserialize(json_ref::node const& n, int64_t& v);
    void deserialize(json_ref::node const& n, uint64_t& v);
    void deserialize(json_ref::node const& n, float& v);
    void deserialize(json_ref::node const& n, double& v);
    void deserialize(json_ref::node const& n, cc::string& v);
    template <class Obj>
    void deserialize(json_ref::node const& n, Obj& v)
    {
        if constexpr (std::is_enum_v<Obj>)
        {
            using int_t = std::underlying_type_t<Obj>;
            int_t nr = 0;
            this->deserialize(n, nr);
            v = Obj(nr);
        }
        else if constexpr (is_optional_t<Obj>::value)
        {
            if (n.is_null())
                v = {};
            else
            {
                // TODO: option to partially preserve previous data?
                if (!v.has_value())
                    v.emplace(); // make space for value
                deserialize(n, v.value());
            }
        }
        else if constexpr (is_map_t<Obj>::value)
        {
            using key_t = typename Obj::key_t;
            if constexpr (std::is_convertible_v<key_t, cc::string>)
            {
                if (!n.is_object())
                    on_error(all_data, cc::as_byte_span(n.token), "expected 'object' node for map-like type with string-like keys", severity::error);
                else
                {
                    // TODO: option to partially preserve previous data?
                    v = {};
                    // TODO: maybe .reserve? or via container_traits?
                    auto ci = n.first_child;
                    while (ci > 0)
                    {
                        auto const& cname = jref.nodes[ci];
                        CC_ASSERT(cname.next_sibling > 0 && "corrupted deserialization?");
                        CC_ASSERT(cname.is_string() && "corrupted deserialization?");
                        ci = cname.next_sibling;
                        auto const& cvalue = jref.nodes[ci];
                        ci = cvalue.next_sibling;

                        // TODO: map access via container_traits?
                        deserialize(cvalue, v[cname.get_string()]);
                    }
                }
            }
            else
            {
                if (!n.is_array())
                    on_error(all_data, cc::as_byte_span(n.token), "expected array of 2-arrays for map-like type with non-string-like keys", severity::error);
                else
                {
                    // TODO: option to partially preserve previous data?
                    v = {};
                    // TODO: maybe .reserve? or via container_traits?
                    auto ci = n.first_child;
                    while (ci > 0)
                    {
                        auto const& centry = jref.nodes[ci];
                        if (!centry.is_array() || centry.child_count != 2)
                        {
                            on_error(all_data, cc::as_byte_span(centry.token), "expected 2-array [key, value]", severity::error);
                            break;
                        }
                        ci = centry.next_sibling;

                        auto const& ckey = jref.nodes[centry.first_child];
                        CC_ASSERT(ckey.next_sibling > 0 && "corrupted deserialization");
                        auto const& cvalue = jref.nodes[ckey.next_sibling];

                        // TODO: map access via container_traits?
                        key_t key;
                        deserialize(ckey, key);
                        deserialize(cvalue, v[key]);
                    }
                }
            }
        }
        else if constexpr (rf::is_introspectable<Obj>)
        {
            if (!n.is_object())
                on_error(all_data, cc::as_byte_span(n.token), "expected 'object' node for rf::introspect based deserialization", severity::error);
            else
            {
                // introspect members are not too many
                // thus it might be faster to always search linearly for the correct key
                // TODO: benchmark this. if slower, construct a map or something
                size_t cnt = 0;
                rf::do_introspect(
                    [&](auto& member, cc::string_view name) {
                        auto found = false;
                        auto ci = n.first_child;
                        while (ci > 0)
                        {
                            auto const& cname = jref.nodes[ci];
                            CC_ASSERT(cname.next_sibling > 0 && "corrupted deserialization?");
                            CC_ASSERT(cname.is_string() && "corrupted deserialization?");
                            ci = cname.next_sibling;
                            auto const& cvalue = jref.nodes[ci];
                            ci = cvalue.next_sibling;

                            if (cname.get_string() == name)
                            {
                                deserialize(cvalue, member);
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            ++cnt;
                        else
                        {
                            if (cfg.warn_on_missing_data)
                                on_error(all_data, cc::as_byte_span(n.token), "missing data for field '" + cc::string(name) + "'", severity::warning);
                            if (cfg.init_missing_data)
                                member = {};
                        }
                    },
                    v);

                if (cnt != n.child_count && cfg.warn_on_extra_data)
                    on_error(all_data, cc::as_byte_span(n.token), "object contains extra data that could not be assigned", severity::warning);
            }
        }
        else if constexpr (cc::collection_traits<Obj>::is_range)
        {
            using traits = cc::collection_traits<Obj>;
            using element_t = typename traits::element_t;

            // if elements can be added, collection is built from the ground up
            if constexpr (traits::can_add)
            {
                // TODO: cc::collection_clear?
                // TODO: option to preserve prev data?
                v = {};
                auto ci = n.first_child;
                while (ci > 0)
                {
                    auto const& cvalue = jref.nodes[ci];
                    // TODO: trait for emplace?
                    element_t e;
                    deserialize(cvalue, e);
                    cc::collection_add(v, cc::move(e));
                    ci = cvalue.next_sibling;
                }
            }
            // otherwise it is assumed to have been pre-sized
            // TODO: support resizeable collections that cannot add, like cc::array
            else
            {
                auto it = traits::begin(v);
                auto end = traits::end(v);
                auto ci = n.first_child;
                while (ci > 0)
                {
                    if (!(it != end))
                    {
                        if (cfg.warn_on_extra_data)
                            on_error(all_data, cc::as_byte_span(n.token), "array contains extra data that could not be assigned", severity::warning);
                        break;
                    }

                    auto const& cvalue = jref.nodes[ci];
                    deserialize(cvalue, *it);
                    ci = cvalue.next_sibling;
                    ++it;
                }

                if (it != end)
                {
                    if (cfg.warn_on_missing_data)
                        on_error(all_data, cc::as_byte_span(n.token), "array contains not enough data", severity::warning);

                    if (cfg.init_missing_data)
                    {
                        while (it != end)
                        {
                            *it = {};
                            ++it;
                        }
                    }
                }
            }
        }
        else
            static_assert(cc::always_false<Obj>, "obj type is not supported for deserialization");
    }
};
}

template <class Obj>
void write(cc::string_stream_ref output, Obj const& obj, write_config const& cfg)
{
    if (cfg.indent >= 0)
        detail::json_writer_pretty{cfg.indent}.write(output, obj);
    else
        detail::json_writer_compact{}.write(output, obj);
}

template <class Obj>
void read_to(Obj& obj, cc::string_view json, read_config const& cfg, error_handler on_error)
{
    auto const jref = read_ref(json, cfg, on_error);

    if (jref.nodes.empty())
        return; // error in read_ref

    detail::json_deserializer{cc::as_byte_span(json), cfg, on_error, jref}.deserialize(jref.root(), obj);
}

}
