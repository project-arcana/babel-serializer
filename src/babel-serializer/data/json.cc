#include "json.hh"

#include <rich-log/log.hh>

#include <clean-core/from_string.hh>

cc::string babel::json::json_ref::node::get_string() const
{
    CC_ASSERT(is_string());

    // strip '"'
    auto sv = token.subview(1, token.size() - 2);

    // unescape
    cc::string r;
    r.reserve(sv.size());
    for (size_t i = 0; i < sv.size(); ++i)
    {
        auto c = sv[i];
        if (c == '\\' && i + 1 < sv.size())
        {
            switch (sv[i + 1])
            {
            case 'b':
                r += '\b';
                break;
            case 'f':
                r += '\f';
                break;
            case 'n':
                r += '\n';
                break;
            case 'r':
                r += '\r';
                break;
            case 't':
                r += '\t';
                break;
            case '\"':
                r += '\"';
                break;
            case '\\':
                r += '\\';
                break;
            default:
                // TODO: better error handling
                CC_UNREACHABLE("unknown escape");
            }
            ++i;
        }
        else
            r += c;
    }
    return r;
}

int babel::json::json_ref::node::get_int()
{
    CC_ASSERT(is_number());

    int v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

float babel::json::json_ref::node::get_float()
{
    CC_ASSERT(is_number());

    float v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

double babel::json::json_ref::node::get_double()
{
    CC_ASSERT(is_number());

    double v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

cc::int64 babel::json::json_ref::node::get_int64()
{
    CC_ASSERT(is_number());

    cc::int64 v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

cc::uint64 babel::json::json_ref::node::get_uint64()
{
    CC_ASSERT(is_number());

    cc::uint64 v;
    auto ok = cc::from_string(token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

void babel::json::detail::write_escaped_string(cc::stream_ref<char> output, cc::string_view s)
{
    // no realloc if nothing has to be escaped
    // TODO: for even better performance it might be faster to check if s needs escaping at all
    cc::string r;
    r.reserve(s.size() + 2);

    r += '"';
    for (auto c : s)
    {
        switch (c)
        {
        case '\b':
            r += "\\b";
            break;
        case '\f':
            r += "\\f";
            break;
        case '\r':
            r += "\\r";
            break;
        case '\n':
            r += "\\n";
            break;
        case '\t':
            r += "\\t";
            break;
        case '\"':
            r += "\\\"";
            break;
        case '\\':
            r += "\\\\";
            break;
        default:
            r += c;
            break;
        }
    }
    r += '"';

    output << r;
}

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
            auto& n = json.nodes.emplace_back();
            n.type = node_type::array;
            n.first_child = json.nodes.size();
            ++curr;

            skip_whitespace();
            if (err_on_end())
                return 0;

            if (*curr != ']')
            {
                size_t prev_idx = 0;

                while (true)
                {
                    auto ni = parse_json();
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
        }
        else if (c == '{')
        {
            auto& n = json.nodes.emplace_back();
            n.type = node_type::object;
            n.first_child = json.nodes.size();
            ++curr;

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
}
}

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
