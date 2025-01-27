#include "json.hh"

#include <rich-log/log.hh>

#include <clean-core/from_string.hh>

#include <babel-serializer/data/escape.hh>

cc::string babel::json::json_ref::node::get_string() const
{
    CC_ASSERT(is_string());
    return babel::unescape_json_string(token);
}

int32_t babel::json::json_ref::node::get_int() const
{
    CC_ASSERT(is_number());

    int32_t v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

float babel::json::json_ref::node::get_float() const
{
    CC_ASSERT(is_number());

    float v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

double babel::json::json_ref::node::get_double() const
{
    CC_ASSERT(is_number());

    double v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

int64_t babel::json::json_ref::node::get_int64() const
{
    CC_ASSERT(is_number());

    int64_t v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

uint64_t babel::json::json_ref::node::get_uint64() const
{
    CC_ASSERT(is_number());

    uint64_t v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

bool babel::json::json_cursor::has_child(cc::string_view name) const
{
    if (!is_object())
        return false;

    auto ci = first_child;
    while (ci > 0)
    {
        auto const& cname = ref.nodes[ci];
        CC_ASSERT(cname.next_sibling > 0 && "corrupted deserialization?");
        CC_ASSERT(cname.is_string() && "corrupted deserialization?");

        if (cname.get_string() == name) // TODO: replace me with an "escape-aware compare"
            return true;

        ci = cname.next_sibling;
        auto const& cvalue = ref.nodes[ci];
        ci = cvalue.next_sibling;
    }

    return false;
}

babel::json::json_cursor babel::json::json_cursor::operator[](cc::string_view name) const
{
    CC_ASSERT(is_object() && "only works on objects");

    auto ci = first_child;
    while (ci > 0)
    {
        auto const& cname = ref.nodes[ci];
        CC_ASSERT(cname.next_sibling > 0 && "corrupted deserialization?");
        CC_ASSERT(cname.is_string() && "corrupted deserialization?");

        ci = cname.next_sibling;
        auto const& cvalue = ref.nodes[ci];

        if (cname.get_string() == name) // TODO: replace me with an "escape-aware compare"
            return json_cursor(ref, cvalue);

        ci = cvalue.next_sibling;
    }

    CC_UNREACHABLE("could not find child");
}

void babel::json::detail::write_escaped_string(cc::string_stream_ref output, cc::string_view s) { output << babel::escape_json_string(s); }

namespace babel::json
{
namespace
{
struct json_parser
{
    error_handler on_error;
    char const* start;
    char const* curr;
    char const* end;
    json::json_ref json;

    json_parser(error_handler on_error, cc::string_view json) : on_error(on_error)
    {
        CC_ASSERT(!json.empty());

        start = json.data();
        curr = json.data();
        end = json.data() + json.size();
    }

    void parse()
    {
        parse_json();
        skip_whitespace();
        if (curr != end)
            on_error(data_span(), rest_data_span(), "extra data after json", severity::warning);
    }

private:
    cc::string_view curr_string() const { return cc::string_view(curr, end); }
    cc::span<std::byte const> data_span() const { return cc::as_byte_span(cc::string_view(start, end)); }
    cc::span<std::byte const> rest_data_span() const { return cc::as_byte_span(cc::string_view(curr, end)); }
    cc::span<std::byte const> curr_data_span() const { return cc::as_byte_span(cc::string_view(curr, curr == end ? curr : curr + 1)); }

    void skip_whitespace()
    {
        while (curr != end && cc::is_space(*curr))
            ++curr;
    }

    bool err_on_end()
    {
        if (curr != end)
            return false;

        on_error(data_span(), curr_data_span(), "unexpected end of data", severity::error);
        return true;
    }

    size_t parse_json()
    {
        skip_whitespace();
        if (err_on_end())
            return 0;

        auto c = *curr;

        auto node_idx = json.nodes.size();

        if (c == '[')
        {
            auto s = curr;
            {
                auto& n = json.nodes.emplace_back();
                n.type = node_type::array;
                n.first_child = json.nodes.size();
            }
            ++curr;
            size_t child_cnt = 0;

            skip_whitespace();
            if (err_on_end())
                return 0;

            if (*curr != ']')
            {
                size_t prev_idx = 0;

                while (true)
                {
                    auto ni = parse_json();
                    ++child_cnt;
                    if (prev_idx > 0)
                        json.nodes[prev_idx].next_sibling = ni;
                    prev_idx = ni;

                    skip_whitespace();
                    if (err_on_end())
                        return 0;

                    if (*curr == ']')
                        break;

                    if (*curr != ',')
                    {
                        ++curr;
                        on_error(data_span(), curr_data_span(), "expected ',' or ']'", severity::error);
                        return node_idx;
                    }

                    ++curr;
                }
            }

            ++curr;
            auto& n = json.nodes[node_idx];
            n.token = {s, curr};
            n.child_count = child_cnt;
            if (child_cnt == 0)
                n.first_child = 0;
        }
        else if (c == '{')
        {
            auto s = curr;
            {
                auto& n = json.nodes.emplace_back();
                n.type = node_type::object;
                n.first_child = json.nodes.size();
            }
            ++curr;
            size_t child_cnt = 0;

            skip_whitespace();
            if (err_on_end())
                return 0;

            if (*curr != '}')
            {
                size_t prev_idx = 0;

                while (true)
                {
                    skip_whitespace();
                    if (err_on_end())
                        return 0;

                    // parse key
                    if (*curr != '"')
                    {
                        on_error(data_span(), curr_data_span(), "expected '\"' (objects keys must be strings)", severity::error);
                        return node_idx;
                    }

                    auto s = curr;
                    ++curr;

                    while (curr < end)
                    {
                        auto c = *curr;
                        if (c == '"')
                            break;

                        if (c == '\\')
                            ++curr;

                        ++curr;
                    }

                    if (curr < end)
                        ++curr;
                    else
                    {
                        if (curr > end)
                            curr = end; // can happen if an unfinished escape is last
                        on_error(data_span(), curr_data_span(), "expected '\"'", severity::error);
                        return 0;
                    }

                    auto key_idx = json.nodes.size();
                    auto& n = json.nodes.emplace_back();
                    n.type = node_type::string;
                    n.token = {s, curr};

                    // skip ':'
                    skip_whitespace();
                    if (err_on_end())
                        return 0;
                    if (*curr != ':')
                    {
                        on_error(data_span(), curr_data_span(), "expected ':'", severity::error);
                        return 0;
                    }
                    ++curr;

                    // parse value
                    auto ni = parse_json();
                    ++child_cnt;

                    // connect
                    if (prev_idx > 0)
                        json.nodes[prev_idx].next_sibling = key_idx;
                    json.nodes[key_idx].next_sibling = ni;
                    prev_idx = ni;

                    skip_whitespace();
                    if (err_on_end())
                        return 0;

                    if (*curr == '}')
                        break;

                    if (*curr != ',')
                    {
                        ++curr;
                        on_error(data_span(), curr_data_span(), "expected ',' or '}'", severity::error);
                        return node_idx;
                    }

                    ++curr;
                }
            }

            ++curr;
            auto& n = json.nodes[node_idx];
            n.token = {s, curr};
            n.child_count = child_cnt;
        }
        else if (c == '"')
        {
            auto s = curr;
            ++curr;

            while (curr < end)
            {
                auto c = *curr;
                if (c == '"')
                    break;

                if (c == '\\')
                    ++curr;

                ++curr;
            }

            if (curr < end)
                ++curr;
            else
            {
                if (curr > end)
                    curr = end; // can happen if an unfinished escape is last
                on_error(data_span(), curr_data_span(), "expected '\"'", severity::error);
                return 0;
            }

            auto& n = json.nodes.emplace_back();
            n.type = node_type::string;
            n.token = {s, curr};
        }
        else if (c == 't')
        {
            if (!curr_string().starts_with("true"))
            {
                on_error(data_span(), curr_data_span(), "expected 'true'.", severity::error);
                return 0;
            }

            auto& n = json.nodes.emplace_back();
            n.type = node_type::boolean;
            n.token = {curr, curr + 4};
            curr += 4;
        }
        else if (c == 'f')
        {
            if (!curr_string().starts_with("false"))
            {
                on_error(data_span(), curr_data_span(), "expected 'false'.", severity::error);
                return 0;
            }

            auto& n = json.nodes.emplace_back();
            n.type = node_type::boolean;
            n.token = {curr, curr + 5};
            curr += 5;
        }
        else if (c == 'n')
        {
            if (!curr_string().starts_with("null"))
            {
                on_error(data_span(), curr_data_span(), "expected 'true'.", severity::error);
                return 0;
            }

            auto& n = json.nodes.emplace_back();
            n.type = node_type::null;
            n.token = {curr, curr + 4};
            curr += 4;
        }
        else if (c == '-' || c == '+' || cc::is_digit(c))
        {
            auto s = curr;
            ++curr;

            auto is_number = [](char c) { return cc::is_alphanumeric(c) || c == '.' || c == '-' || c == '+'; };
            while (curr < end && is_number(*curr))
                ++curr;

            auto& n = json.nodes.emplace_back();
            n.type = node_type::number;
            n.token = {s, curr};
        }
        else
        {
            on_error(data_span(), curr_data_span(), "unknown json token, expected list, object, string, boolean, null, or number.", severity::error);
            return 0;
        }

        return node_idx;
    }
};
} // namespace
} // namespace babel::json

babel::json::json_ref babel::json::read_ref(cc::string_view json, read_config const&, error_handler on_error)
{
    if (json.empty())
    {
        on_error(cc::as_byte_span(json), cc::as_byte_span(json), "empty string is not valid json", severity::error);
        return {};
    }

    auto parser = json_parser{on_error, json};
    parser.parse();
    return parser.json;
}

void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, bool& v)
{
    if (!n.is_boolean())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'bool' node", severity::error);
    else
        v = n.get_boolean();
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, char& v)
{
    if (!n.is_string())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'string' node", severity::error);
    else if (auto s = n.get_string(); s.size() != 1)
        on_error(all_data, cc::as_byte_span(n.token), "expected 'string' node of length 1", severity::error);
    else
        v = s[0];
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, std::byte& v)
{
    if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = std::byte(n.get_int());
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, int8_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (int8_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = (int8_t)(n.get_int());
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, uint8_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (uint8_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = (uint8_t)(n.get_int());
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, int16_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (int16_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = (int16_t)(n.get_int());
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, uint16_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (uint16_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = (uint16_t)(n.get_int());
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, int32_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (int32_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = n.get_int();
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, uint32_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (uint32_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = (uint32_t)(n.get_uint64());
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, int64_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (int64_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = n.get_int64();
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, uint64_t& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (uint64_t)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = n.get_uint64();
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, float& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (float)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = n.get_float();
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, double& v)
{
    if (cfg.allow_bool_number_conversion && n.is_boolean())
        v = (double)(n.get_boolean());
    else if (!n.is_number())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'number' node", severity::error);
    else
        v = n.get_double();
}
void babel::json::detail::json_deserializer::deserialize(json_ref::node const& n, cc::string& v)
{
    // TODO: performance can be increased by reusing storage of v

    if (!n.is_string())
        on_error(all_data, cc::as_byte_span(n.token), "expected 'string' node", severity::error);
    else
        v = n.get_string();
}
