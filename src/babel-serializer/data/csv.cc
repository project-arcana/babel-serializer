#include "csv.hh"

#include <clean-core/from_string.hh>

namespace
{
cc::string csv_to_string(cc::string_view sv)
{
    if (sv.starts_with('"'))
    {
        CC_ASSERT(sv.ends_with('"'));
        sv = sv.subview(1, sv.size() - 2);
    }

    cc::string s;
    for (size_t i = 0; i < sv.size(); ++i)
    {
        s += sv[i];
        if (sv[i] == '"') // remove duplicate <">
            ++i;
    }
    return s;
}
}

babel::csv::csv_ref babel::csv::read(cc::string_view csv_string, read_config const& config, error_handler on_error)
{
    csv_ref csv;

    auto const end = csv_string.end();

    auto p = csv_string.begin();

    auto parse_token = [&]() -> cc::string_view
    {
        auto is_escaped = false;

        auto const start = p;

        while (p != end)
        {
            if (*p == '"')
                is_escaped = !is_escaped;

            else if (!is_escaped && (*p == config.separator || *p == '\n'))
                break;

            ++p;
        }

        if (is_escaped)
            on_error(cc::as_byte_span(csv_string), cc::as_byte_span(cc::string_view(start, p)), "unmatched escape character <\">", severity::error);

        return cc::string_view(start, p).trim();
    };

    if (config.has_header)
    {
        csv.header = cc::vector<cc::string>();
        while (p != end && *p != '\n')
        {
            auto name = csv_to_string(parse_token());

            if (name.empty())
                on_error(cc::as_byte_span(csv_string), cc::as_byte_span(name), "header has empty token", severity::warning);

            csv.header.push_back(name);

            if (p != end && *p == config.separator) // no data lines, just a header
                ++p;
        }

        if (p != end)
            ++p;

        csv.column_count = csv.header.size();
    }


    // the workflow here is as follows:
    // if there was a header, we expect at most as many tokens per row, as header entries, otherwise it's an error.
    // if there was no header, we restart parsing, each time we encounter a row that has more tokens than the previous row.
    // this technically has a terrible worst-case runtime (i.e. if every row has one more entry, than the last), but should perform fine for
    // real-world examples since we expect one of the first lines to have the correct number of entries.
    // The alternative is parsing the entire csv twice to figure out the maximal tokens per line. Note we only have to do that, if we encounter a mismatch.
    auto const data_start = p;

    while (p != end)
    {
        // parse line
        auto const line_start = p;
        auto token_count = 0;
        while (p != end && *p != '\n')
        {
            auto token = parse_token();
            csv.entries.push_back({token});
            ++token_count;
            if (p != end && *p == config.separator)
                ++p;
        }

        if (p != end)
            ++p;

        // some csv files do not explicitly create trailing empty tokens.
        // instead of              <values, followed, by, empty,,,,>
        // they will only contain  <values, followed, by, empty>
        // we now make sure, we have the correct amount of tokens
        for (; token_count < csv.column_count; ++token_count)
            csv.entries.emplace_back();

        if (csv.column_count == 0) // first time the column width is set
            csv.column_count = token_count;

        if (token_count > csv.column_count)
        {
            if (config.has_header)
            {
                on_error(cc::as_byte_span(csv_string), cc::as_byte_span(cc::string_view(line_start, p)),
                         "line and header have mismatching number of tokens", severity::error);
            }
            else
            {
                // restart
                csv.entries.clear();
                csv.column_count = token_count;
                p = data_start;
            }
        }
    }
    return csv;
}

cc::string babel::csv::csv_ref::entry::get_string() const { return csv_to_string(raw_token); }

int32_t babel::csv::csv_ref::entry::get_int() const
{
    int32_t v;
    auto ok = cc::from_string(raw_token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

uint32_t babel::csv::csv_ref::entry::get_uint() const
{
    uint32_t v;
    auto ok = cc::from_string(raw_token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

float babel::csv::csv_ref::entry::get_float() const
{
    float v;
    auto ok = cc::from_string(raw_token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

double babel::csv::csv_ref::entry::get_double() const
{
    double v;
    auto ok = cc::from_string(raw_token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

int64_t babel::csv::csv_ref::entry::get_int64() const
{
    int64_t v;
    auto ok = cc::from_string(raw_token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}

uint64_t babel::csv::csv_ref::entry::get_uint64() const
{
    uint64_t v;
    auto ok = cc::from_string(raw_token, v);
    CC_ASSERT(ok); // TODO: proper error handling
    return v;
}
