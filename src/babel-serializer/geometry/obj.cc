#include <cstdio>      // std::sscanf
#include <type_traits> // std::is_same_v

#include <clean-core/char_predicates.hh>

#include "obj.hh"

/*
 * TODO:
 *  - colors (unofficial extension)
 *  - free-form stuff
 *  - materials
 *  - merging groups
 *  - smoothing groups
 *  - disable MSVC warning that sscanf is deprecated
 *  - additional error checks (all indices must be valid)
 */

namespace
{
template <class ScalarT>
babel::obj::geometry<ScalarT> read_impl(cc::span<const std::byte> data, babel::obj::read_config const& cfg, babel::error_handler on_error)
{
    using namespace babel;

    obj::geometry<ScalarT> geometry;

    cc::vector<cc::string_view> active_groups = {"default"};
    int group_point_start = 0;
    int group_line_start = 0;
    int group_face_start = 0;

    size_t prev_pos = 0;
    size_t pos = 0;
    auto const next_line = [&]() -> cc::string_view {
        prev_pos = pos;
        if (pos >= data.size())
        {
            on_error(data, data.last(0), "expected line", severity::error);
        }

        // skip comment lines
        while (pos < data.size() && char(data[pos]) == '#')
        {
            while (pos < data.size() && char(data[pos]) != '\n')
                ++pos;
            if (pos < data.size() && char(data[pos]) == '\n')
                ++pos;
        }

        auto p_start = reinterpret_cast<char const*>(data.data() + pos);
        while (pos < data.size())
        {
            auto c = char(data[pos]);
            if (c == '\n' || c == '#')
                break;

            ++pos;
        }
        auto p_end = reinterpret_cast<char const*>(data.data() + pos);

        // ignore rest of comment
        while (pos < data.size())
        {
            auto c = char(data[pos]);
            if (c == '\n')
                break;

            ++pos;
        }

        // point behind newline
        if (pos < data.size() && char(data[pos]) == '\n')
            ++pos;

        // trim whitespaces
        while (pos < data.size() && cc::is_space(char(data[pos])))
            ++pos;

        return cc::string_view(p_start, p_end);
    };

    auto const has_more_lines = [&]() -> bool { return pos < data.size(); };

    auto const parse_int = [&](cc::string_view s) -> int {
        cc::string ss = s; // null-terminated
        int i;
        auto const r = std::sscanf(ss.c_str(), "%d", &i);
        if (r == 0)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse int", severity::error);
            return 0;
        }
        return i;
    };

    auto const parse_float = [&](cc::string_view s) -> ScalarT {
        cc::string ss = s; // null-terminated
        ScalarT f;
        if constexpr (std::is_same_v<ScalarT, float>)
        {
            auto const r = std::sscanf(ss.c_str(), "%f", &f);
            if (r == 0)
            {
                on_error(data, cc::as_byte_span(s), "unable to parse float", severity::error);
                return 0;
            }
        }
        else
        {
            auto const r = std::sscanf(ss.c_str(), "%lf", &f);
            if (r == 0)
            {
                on_error(data, cc::as_byte_span(s), "unable to parse double", severity::error);
                return 0;
            }
        }
        return f;
    };

    auto const parse_vertex = [&](cc::string_view line) {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(cc::is_space(line[1]));
        tg::pos<4, ScalarT> p;
        p.w = ScalarT(1);
        int i = 0;
        for (auto const s : line.subview(2).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 3)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse vertex", severity::error);
            }
            p[i++] = parse_float(s);
        }
        geometry.vertices.push_back(p);
    };

    auto const parse_texture_vertex = [&](cc::string_view line) {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(line[1] == 't');
        CC_ASSERT(cc::is_space(line[2]));
        tg::pos<3, ScalarT> p;
        p.z = ScalarT(0);
        int i = 0;
        for (auto const s : line.subview(3).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse texture vertex", severity::error);
            }
            p[i++] = parse_float(s);
        }
        geometry.tex_coords.push_back(p);
    };

    auto const parse_vertex_normal = [&](cc::string_view line) {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(line[1] == 'n');
        CC_ASSERT(cc::is_space(line[2]));
        tg::vec<3, ScalarT> p;
        int i = 0;
        for (auto const s : line.subview(3).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse vertex normal", severity::error);
            }
            p[i++] = parse_float(s);
        }
        geometry.normals.push_back(p);
    };

    auto const parse_parameter_space_vertex = [&](cc::string_view line) {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(line[1] == 'p');
        CC_ASSERT(cc::is_space(line[2]));
        tg::pos<3, ScalarT> p;
        p.z = ScalarT(1);
        int i = 0;
        for (auto const s : line.subview(3).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse free-form vertex", severity::error);
            }
            p[i++] = parse_float(s);
        }
        geometry.parameters.push_back(p);
    };

    auto const parse_face = [&](cc::string_view line) {
        auto const parse_entry = [&](cc::string_view s) {
            typename obj::geometry<ScalarT>::face_entry e;

            int i = 0;
            for (auto const seg : s.split('/'))
            {
                if (i == 0) // vertex index
                {
                    if (seg.empty())
                    {
                        on_error(data, cc::as_byte_span(line), "unable to parse face entry: missing vertex index", severity::error);
                    }
                    auto const idx = parse_int(seg);
                    if (idx < 0) // relative index
                        e.vertex_idx = int(geometry.vertices.size()) + idx;
                    else
                        e.vertex_idx = idx - 1;
                }
                if (i == 1) // tex_coord_idx
                {
                    if (!seg.empty())
                    {
                        auto const idx = parse_int(seg);
                        if (idx < 0) // relative index
                            e.tex_coord_idx = int(geometry.tex_coords.size()) + idx;
                        else
                            e.tex_coord_idx = idx - 1;
                    }
                }
                if (i == 2) // normal idx
                {
                    if (!seg.empty())
                    {
                        auto const idx = parse_int(seg);
                        if (idx < 0) // relative index
                            e.normal_idx = int(geometry.normals.size()) + idx;
                        else
                            e.normal_idx = idx - 1;
                    }
                }
                if (i >= 3)
                {
                    on_error(data, cc::as_byte_span(line), "unable to parse face entry: unknown format", severity::error);
                }

                ++i;
            }
            if (i == 0)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse face entry: unknown format", severity::error);
            }
            geometry.face_entries.push_back(e);
        };

        auto const entries_start = int(geometry.face_entries.size());
        for (auto const entry : line.subview(1).split(cc::is_space, cc::split_options::skip_empty))
        {
            parse_entry(entry);
        }

        auto const entries_count = int(geometry.face_entries.size()) - entries_start;
        if (entries_count == 0)
        {
            on_error(data, cc::as_byte_span(line), "unable to parse face: no face entries found", severity::error);
        }

        geometry.faces.push_back({entries_start, entries_count});
    };

    auto const parse_point = [&](cc::string_view s) {
        CC_ASSERT(s[0] == 'p');
        CC_ASSERT(cc::is_space(s[1]));
        auto const points = s.subview(2);
        for (auto const sp : points.split(cc::is_space, cc::split_options::skip_empty))
        {
            auto const idx = parse_int(sp);
            if (idx < 0) // relative index
                geometry.points.push_back({int(geometry.normals.size()) + idx});
            else
                geometry.points.push_back({idx - 1});
        }
    };

    auto const parse_line = [&](cc::string_view s) {
        CC_ASSERT(s[0] == 'l');
        CC_ASSERT(cc::is_space(s[1]));
        auto const segments = s.subview(2);
        auto const entries_start = int(geometry.line_entries.size());

        for (auto const seg : segments.split(cc::is_space, cc::split_options::skip_empty))
        {
            typename babel::obj::geometry<ScalarT>::line_entry e;
            int i = 0;
            for (auto const idx : seg.split('/'))
            {
                if (i == 0)
                {
                    auto const vertex_idx = parse_int(idx);
                    if (vertex_idx < 0)
                        e.vertex_idx = int(geometry.vertices.size()) + vertex_idx;
                    else
                        e.vertex_idx = vertex_idx - 1;
                }
                else if (i == 1)
                {
                    auto const texture_idx = parse_int(idx);
                    if (texture_idx < 0)
                        e.tex_coord_idx = int(geometry.vertices.size()) + texture_idx;
                    else
                        e.tex_coord_idx = texture_idx - 1;
                }
                else
                {
                    on_error(data, cc::as_byte_span(s), "unable to parse line: unknown line segment format", severity::error);
                }
                ++i;
            }
            geometry.line_entries.push_back(e);
        }

        auto const entries_count = int(geometry.line_entries.size()) - entries_start;
        if (entries_count < 2)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse line: a line segment must contain at least two points", severity::error);
        }
        geometry.lines.push_back({entries_start, entries_count});
    };

    auto const handle_previous_groups = [&]() {
        auto const points_count = int(geometry.points.size()) - group_point_start;
        auto const lines_count = int(geometry.lines.size()) - group_line_start;
        auto const faces_count = int(geometry.faces.size()) - group_face_start;

        typename babel::obj::geometry<ScalarT>::group g;
        if (points_count > 0)
        {
            g.points_start = group_point_start;
            g.points_count = points_count;
        }
        if (lines_count > 0)
        {
            g.lines_start = group_line_start;
            g.lines_count = lines_count;
        }
        if (faces_count > 0)
        {
            g.faces_start = group_face_start;
            g.faces_count = faces_count;
        }
        if (points_count > 0 || lines_count > 0 || faces_count > 0)
            for (auto const group : active_groups)
            {
                g.name = group;
                geometry.groups.push_back(g);
            }
    };

    auto const parse_groups = [&](cc::string_view s) {
        CC_ASSERT(s[0] == 'g');
        CC_ASSERT(cc::is_space(s[1]));

        handle_previous_groups();
        group_point_start = int(geometry.points.size());
        group_line_start = int(geometry.lines.size());
        group_face_start = int(geometry.faces.size());

        // now parse
        active_groups.clear();
        for (auto const g : s.subview(2).split(cc::is_space, cc::split_options::skip_empty))
            active_groups.push_back(g);
    };

    while (has_more_lines())
    {
        auto const line = next_line();
        if (line.empty())
            continue;

        if (line[0] == 'v')
        {
            if (line.size() < 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse line: starts with v but does not contain any vertex information", severity::error);
            }
            if (cc::is_blank(line[1]))
                parse_vertex(line);
            else if (line[1] == 't')
                parse_texture_vertex(line);
            else if (line[1] == 'n')
                parse_vertex_normal(line);
            else if (line[1] == 'p')
                parse_parameter_space_vertex(line);
            else if (cfg.add_unrecognized_lines)
                geometry.unrecognized_lines.push_back(line);
        }
        else if (line[0] == 'f')
            parse_face(line);
        else if (line[0] == 'p')
            parse_point(line);
        else if (line[0] == 'l')
            parse_line(line);
        else if (line[0] == 'g')
        {
            if (cfg.parse_groups)
                parse_groups(line);
        }
        else if (cfg.add_unrecognized_lines)
            geometry.unrecognized_lines.push_back(line);
    }

    if (cfg.parse_groups)
        handle_previous_groups(); // the last active groups

    return geometry;
}
}

babel::obj::geometry<float> babel::obj::read(cc::span<const std::byte> data, babel::obj::read_config const& cfg, babel::error_handler on_error)
{
    return read_impl<float>(data, cfg, on_error);
}

babel::obj::geometry<double> babel::obj::read_double(cc::span<const std::byte> data, babel::obj::read_config const& cfg, babel::error_handler on_error)
{
    return read_impl<double>(data, cfg, on_error);
}
