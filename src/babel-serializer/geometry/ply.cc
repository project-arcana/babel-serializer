#include "ply.hh"

#include <clean-core/string_view.hh>

babel::ply::geometry babel::ply::read(cc::span<const std::byte> data, const babel::ply::read_config& cfg, babel::error_handler on_error)
{
    auto const data_as_sv = cc::string_view(reinterpret_cast<char const*>(data.begin()), reinterpret_cast<char const*>(data.end()));

    auto pos = 0;
    auto const next_line = [&]() -> cc::string_view {
        auto start = pos;
        while (pos < int(data_as_sv.size()) && data_as_sv[pos] != '\n')
            ++pos;
        auto count = pos - start;
        if (pos < (int(data_as_sv.size()))) // skip '\n'
            ++pos;
        return data_as_sv.subview(start, count).trim();
    };
    auto const has_more_lines = [&]() { return pos < int(data_as_sv.size()); };

    enum class file_type
    {
        ascii,
        binary_big_endian,
        binary_little_endian
    };

    file_type file_type;


    //    name        type        number of bytes
    //    ---------------------------------------
    //    char       character                 1
    //    uchar      unsigned character        1
    //    short      short integer             2
    //    ushort     unsigned short integer    2
    //    int        integer                   4
    //    uint       unsigned integer          4
    //    float      single-precision float    4
    //    double     double-precision float    8


    // parse the header
    {
        auto const ply = next_line();
        if (ply != "ply")
        {
            // error, wrong file format
        }
        auto const format_line = next_line();
        if (!format_line.starts_with("format"))
        {
            // error, missing format line
        }
        auto const format = *format_line.subview(7).split().begin();
        if (format == "ascii")
            file_type = file_type::ascii;
        else if (format == "binary_little_endian")
            file_type = file_type::binary_little_endian;
        else if (format == "binary_big_endian")
            file_type = file_type::binary_big_endian;
        else
        {
            // error, unknown format
        }
        if (!format_line.ends_with("1.0"))
        {
            // error, unsupported version
        }

        { // parse elements and properties
            cc::string_view line = next_line();
            while (line != "end_header")
            {
                if (!has_more_lines())
                {
                    // error, incomplete header
                }

                if (line.starts_with("comment"))
                    continue;

                if (line.starts_with("element"))
                {
                }
                if (line.starts_with("property"))
                {
                }

                line = next_line();
            }
        }
    }
}

bool babel::ply::geometry::has_element(cc::string_view name) const
{
    for (auto const& e : elements)
        if (e.name == name)
            return true;
    return false;
}

babel::ply::geometry::element& babel::ply::geometry::get_element(cc::string_view name)
{
    for (auto& e : elements)
        if (e.name == name)
            return e;

    CC_UNREACHABLE("element does not exist");
}

const babel::ply::geometry::element& babel::ply::geometry::get_element(cc::string_view name) const
{
    for (auto const& e : elements)
        if (e.name == name)
            return e;

    CC_UNREACHABLE("element does not exist");
}

bool babel::ply::geometry::has_property(babel::ply::geometry::element const& element, cc::string_view name) const
{
    for (auto i = 0; i < element.properties_count; ++i)
        if (properties[i + element.properties_start].name == name)
            return true;
    return false;
}

cc::span<babel::ply::geometry::property> babel::ply::geometry::get_properties(cc::string_view element_name)
{
    auto const& e = get_element(element_name);
    return cc::span(&properties[e.properties_start], e.properties_count);
}

cc::span<const babel::ply::geometry::property> babel::ply::geometry::get_properties(cc::string_view element_name) const
{
    auto const& e = get_element(element_name);
    return cc::span(&properties[e.properties_start], e.properties_count);
}

babel::ply::geometry::property& babel::ply::geometry::get_property(babel::ply::geometry::element const& element, cc::string_view name)
{
    for (auto i = 0; i < element.properties_count; ++i)
        if (properties[i + element.properties_start].name == name)
            return properties[i + element.properties_start];

    CC_UNREACHABLE("property does not exist");
}

const babel::ply::geometry::property& babel::ply::geometry::get_property(const babel::ply::geometry::element& element, cc::string_view name) const
{
    for (auto i = 0; i < element.properties_count; ++i)
        if (properties[i + element.properties_start].name == name)
            return properties[i + element.properties_start];

    CC_UNREACHABLE("property does not exist");
}
