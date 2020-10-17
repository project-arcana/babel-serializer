#include "stl.hh"

#include <cstring> // std::memcpy
#include <limits>

#include <clean-core/from_string.hh>
#include <clean-core/typedefs.hh>

babel::stl::geometry babel::stl::read(cc::span<const std::byte> data, babel::stl::read_config const& cfg, babel::error_handler on_error)
{
    using namespace babel;

    stl::geometry geometry;

    auto const data_as_string_view = cc::string_view(reinterpret_cast<char const*>(data.data()), reinterpret_cast<char const*>(data.end()));

    auto const is_ascii = [&]() {
        if (data.size() < 6)
        {
            on_error(data, data, "STL-file too short to be valid", severity::error);
        }
        if (!data_as_string_view.trim().starts_with("solid"))
            return false;

        // unfortunately many binary stl also start with "solid"
        // also check for '\n' followed by "facet"
        auto const idx = data_as_string_view.index_of('\n');
        if (idx < 0)
            return false;

        auto s = data_as_string_view.subview(idx);
        s = s.trim();
        return s.starts_with("facet");
    };

    auto file_type = cfg.file_type;
    if (file_type == stl::file_type::autodetect)
    {
        if (is_ascii())
            file_type = babel::stl::file_type::ascii;
        else
            file_type = babel::stl::file_type::binary;
    }

    auto const parse_binary = [&]() {
        size_t pos = 0;
        auto const get_next = [&](size_t count) -> cc::span<const std::byte> {
            auto const last_pos = pos;
            if (pos + count > data.size())
                on_error(data, data.first(last_pos), "Failed to parse binary stl-file: unexpected eof", severity::error);
            pos += count;
            return data.subspan(last_pos, count);
        };

        stl::geometry::color default_color = {cc::uint8(0.6 * 255), cc::uint8(0.6 * 255), cc::uint8(0.6 * 255), cc::uint8(255)};

        auto const header = get_next(80);
        // check if this is Materialise Magics software style
        auto is_materialize = false;
        // the 10 comes from the bytes "COLOR=" and 4 RGBA bytes
        for (size_t i = 0; i < header.size() - 10; ++i)
        {
            auto const sv = header.subspan(i, 10);
            auto const s = cc::string_view(reinterpret_cast<char const*>(sv.begin()), reinterpret_cast<char const*>(sv.end()));
            if (s.starts_with("COLOR="))
            {
                is_materialize = true;
                default_color.r = cc::uint8(sv[6]);
                default_color.g = cc::uint8(sv[7]);
                default_color.b = cc::uint8(sv[8]);
                default_color.a = cc::uint8(sv[9]);
                break;
            }
        }

        auto const parse_uint32 = [&]() {
            cc::uint32 ui;
            auto const s = get_next(sizeof(cc::uint32));
            std::memcpy(&ui, s.data(), sizeof(ui));
            return ui;
        };

        auto const parse_uint16 = [&]() {
            cc::uint16 ui;
            auto const s = get_next(sizeof(cc::uint16));
            std::memcpy(&ui, s.data(), sizeof(ui));
            return ui;
        };

        auto const parse_float = [&]() {
            cc::float32 f;
            auto const s = get_next(sizeof(cc::float32));
            std::memcpy(&f, s.data(), sizeof(f));
            return f;
        };

        auto const n_triangles = parse_uint32();

        if (data.size() < 84 + n_triangles * 50)
        {
            on_error(data, data, "Failed to parse binary STL-file: File too short", severity::error);
            return;
        }

        geometry.triangles.resize(n_triangles);
        geometry.triangle_color.resize(n_triangles);
        for (size_t i = 0; i < n_triangles; ++i)
        {
            auto& t = geometry.triangles[i];
            t.normal.x = parse_float();
            t.normal.y = parse_float();
            t.normal.z = parse_float();
            t.v0.x = parse_float();
            t.v0.y = parse_float();
            t.v0.z = parse_float();
            t.v1.x = parse_float();
            t.v1.y = parse_float();
            t.v1.z = parse_float();
            t.v2.x = parse_float();
            t.v2.y = parse_float();
            t.v2.z = parse_float();

            // this is the "attribute_byte_count" but in practice is always a color
            // see https://en.wikipedia.org/wiki/STL_(file_format)#Color%20in%20binary%20STL
            auto const color = parse_uint16();
            if (color & (1 << 15))
            {
                auto& c = geometry.triangle_color[i];
                if (is_materialize)
                {
                    c.r = cc::uint8((0x0F & (color >> 8)) << 4);
                    c.g = cc::uint8((0x0F & (color >> 4)) << 4);
                    c.b = cc::uint8((0x0F & (color >> 0)) << 4);
                }
                else
                {
                    c.r = cc::uint8((0x0F & (color >> 0)) << 4);
                    c.g = cc::uint8((0x0F & (color >> 4)) << 4);
                    c.b = cc::uint8((0x0F & (color >> 8)) << 4);
                }
                c.a = cc::uint8(255);
            }
            else
            {
                geometry.triangle_color[i] = default_color;
            }
        }
        if (pos != data.size())
        {
            on_error(data, data, "Binary STL-file contains excess data", severity::warning);
        }
    };

    auto const parse_ascii = [&]() {
        tg::vec3 normal;
        tg::pos3 vertices[3];
        int current_vertex_idx = 0;

        auto const is_space = [](char c) { return cc::is_space(c) || (static_cast<unsigned char>(c) >= static_cast<unsigned char>(128)); }; // treat non-ascii chars as space

        auto const from_string = [](cc::string_view s, float& f) {
            if (cc::from_string(s, f))
                return true;
            if (s.equals_ignore_case("nan"))
            {
                f = std::numeric_limits<float>::quiet_NaN();
                return true;
            }
            else if (s.equals_ignore_case("inf") || s.equals_ignore_case("infinity"))
            {
                f = std::numeric_limits<float>::infinity();
                return true;
            }
            else if (s.equals_ignore_case("-inf") || s.equals_ignore_case("-infinity"))
            {
                f = -std::numeric_limits<float>::infinity();
                return true;
            }
            else
                return false;
        };

        for (auto line : data_as_string_view.split('\n', cc::split_options::skip_empty))
        {
            line = line.trim(is_space);
            if (line.empty())
                continue;

            if (line.starts_with("solid"))
            {
                // note: do not use the local version of is_space here, because some files have unicode names
                geometry.name = line.subview(5).trim(); // skip "solid"
            }
            else if (line.starts_with("facet normal"))
            {
                current_vertex_idx = 0;
                auto const normal_entries = line.subview(12); // skip "facet_normal"
                int i = 0;
                for (auto normal_entry : normal_entries.split(is_space, cc::split_options::skip_empty))
                {
                    if (!from_string(normal_entry, normal[i++]))
                    {
                        on_error(data, cc::as_byte_span(normal_entry), "Failed to parse STL: Failed to read normal", severity::error);
                        continue;
                    }
                }
            }
            else if (line.starts_with("outer loop"))
            {
                continue;
            }
            else if (line.starts_with("vertex"))
            {
                auto const vertex_entries = line.subview(6); // skip "vertex"
                auto i = 0;
                for (auto const vertex_entry : vertex_entries.split(is_space, cc::split_options::skip_empty))
                {
                    if (!from_string(vertex_entry, vertices[current_vertex_idx][i++]))
                    {
                        on_error(data, cc::as_byte_span(vertex_entry), "Failed to parse STL: Failed to read vertex", severity::error);
                        continue;
                    }
                }
                current_vertex_idx++;
            }
            else if (line.starts_with("endloop"))
            {
                continue;
            }
            else if (line.starts_with("endfacet"))
            {
                auto& triangle = geometry.triangles.emplace_back();
                triangle.normal = normal;
                triangle.v0 = vertices[0];
                triangle.v1 = vertices[1];
                triangle.v2 = vertices[2];
            }
            else if (line.starts_with("endsolid"))
            {
                // TODO: error handling, if anything comes after "endsolid <name>"
                continue;
            }
            else
            {
                on_error(data, cc::as_byte_span(line), "Failed to parse STL-file: unknown line", severity::error);
                continue;
            }
        }
    };

    switch (file_type)
    {
    case stl::file_type::binary:
        parse_binary();
        break;
    case stl::file_type::ascii:
        parse_ascii();
        break;
    case stl::file_type::autodetect:
        CC_UNREACHABLE("Autodetect should have already happened at this point!");
    }

    return geometry;
}
