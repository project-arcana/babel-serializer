#include "escape.hh"

cc::string babel::escape_json_string(cc::string_view s)
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
    return r;
}

cc::string babel::unescape_json_string(cc::string_view s)
{
    CC_ASSERT(s.size() >= 2 && s.starts_with('"') && s.ends_with('"'));

    // strip '"'
    auto sv = s.subview(1, s.size() - 2);

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
