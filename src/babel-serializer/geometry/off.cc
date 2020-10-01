#include "off.hh"

#include <clean-core/from_string.hh>
#include <clean-core/string_view.hh>

/*
 * TODO:
 *  - Do we want to preserve the color format?
 *  - Do we want to support face colors? (Not even assimp supports them)
 *  - Do we want to support binary OFF? (Not even assimp supports them)
 */

babel::off::geometry babel::off::read(cc::span<const std::byte> data, babel::off::read_config const& cfg, babel::error_handler on_error)
{
    babel::off::geometry geometry;

    auto const data_as_string_view = cc::string_view(reinterpret_cast<char const*>(data.data()), reinterpret_cast<char const*>(data.data() + data.size()));

    auto has_header = false;
    auto has_tex_coords = cfg.has_tex_coords;
    auto has_normals = cfg.has_normals;
    auto has_colors = cfg.has_colors;
    auto has_dimension = false;
    auto has_homogeneous = false;
    auto dimension = cfg.dimension;

    auto curr = data_as_string_view.begin();
    auto const end = data_as_string_view.end();

    auto const advance_to = [&](char c) {
        while (curr != end && *curr != c)
            ++curr;
    };
    auto const skip_spaces = [&]() {
        while (curr != end && cc::is_space(*curr))
            ++curr;
    };
    auto const next_line = [&]() {
        advance_to('\n');
        ++curr;
    };
    auto const skip_spaces_and_comments = [&]() {
        while (curr != end && (*curr == '#' || cc::is_space(*curr)))
        {
            if (*curr == '#')
                next_line();
            skip_spaces();
        }
    };
    auto const next_token = [&]() -> cc::string_view {
        skip_spaces();
        if (*curr == '#' || cc::is_space(*curr))
        {
            // error, unexpected end of line
            return {};
        }
        auto const start = curr;
        while (curr != end && !cc::is_space(*curr) && *curr != '#')
            ++curr;
        return cc::string_view(start, curr);
    };

    if (curr == end)
    {
        on_error(data, data, "Faile to parse OFF: File empty", severity::error);
        return {};
    }

    // parse header, if there is one
    {
        skip_spaces_and_comments();
        if (curr < end - 1 && curr[0] == 'S' && curr[1] == 'T')
        {
            has_tex_coords = true;
            curr += 2;
        }
        if (curr != end && curr[0] == 'C')
        {
            has_colors = true;
            ++curr;
        }
        if (curr != end && curr[0] == 'N')
        {
            has_normals = true;
            ++curr;
        }
        if (curr != end && curr[0] == '4')
        {
            has_homogeneous = true;
            ++curr;
        }
        if (curr != end && curr[0] == 'n')
        {
            has_dimension = true;
            ++curr;
        }
        if (curr < end - 2 && curr[0] == 'O' && curr[1] == 'F' && curr[2] == 'F')
        {
            has_header = true;
            curr += 3;
        }
        if (has_header)
        {
            if (has_dimension) // must be explicityly given if there is no header
            {
                next_line();
                if (!cc::from_string(next_token(), dimension))
                {
                    on_error(data, data.subspan(curr - data_as_string_view.begin()),
                             "Failed to parse OFF file: Header contained dimension hint 'n' but no dimension information was present", severity::error);
                    dimension = 3;
                }
            }
            else
            {
                dimension = 3;
            }
            if (has_homogeneous)
            {
                ++dimension;
            }
            if (dimension > 4)
            {
                on_error(data, data.subspan(curr - data_as_string_view.begin()), "Failed to parse OFF file: At most 4 dimensional points supported",
                         severity::error);
            }
        }
        else
        {
            // no header given, reset
            curr = data_as_string_view.begin();
            has_tex_coords = cfg.has_tex_coords;
            has_normals = cfg.has_normals;
            has_colors = cfg.has_colors;
            has_dimension = false;
            dimension = cfg.dimension;
        }
    }

    int n_vertices = 0;
    int n_faces = 0;
    int n_edges = 0;
    {
        next_line();
        if (!cc::from_string(next_token(), n_vertices))
        {
            on_error(data, data.subspan(curr - data_as_string_view.begin()), "Failed to parse OFF file: Failed to read vertex count", severity::error);
            return {};
        }
        if (!cc::from_string(next_token(), n_faces))
        {
            on_error(data, data.subspan(curr - data_as_string_view.begin()), "Failed to parse OFF file: Failed to read face count", severity::error);
            return {};
        }
        if (!cc::from_string(next_token(), n_edges))
        {
            on_error(data, data.subspan(curr - data_as_string_view.begin()), "Failed to parse OFF file: Failed to read edge count", severity::error);
            return {};
        }
        skip_spaces_and_comments();

        geometry.vertices.resize(n_vertices);
        if (has_normals)
            geometry.normals.resize(n_vertices);
        if (has_tex_coords)
            geometry.tex_coords.resize(n_vertices);
        if (has_colors)
        {
            geometry.colors.resize(n_vertices, tg::color4{0.6f, 0.6f, 0.6f, 1.0f});
            geometry.colormap_indices.resize(n_vertices, -1);
        }

        for (auto i = 0; i < n_vertices; ++i)
        {
            for (auto d = 0; d < dimension; ++d)
                if (!cc::from_string(next_token(), geometry.vertices[i][d]))
                {
                    on_error(data, data.subspan(curr - data_as_string_view.begin()), "Failed to parse OFF file: Corrupt vertex information", severity::error);
                    continue;
                }

            if (has_normals)
                for (auto d = 0; d < 3; ++d)
                    if (!cc::from_string(next_token(), geometry.normals[i][d]))
                    {
                        on_error(data, data.subspan(curr - data_as_string_view.begin()), "Failed to parse OFF file: Corrupt normal information", severity::error);
                        continue;
                    }

            if (has_colors)
            {
                // colors are cancer, there can be 0, 1, 3, or 4 integers, or 3 to 4 floats
                // the color format ends with a '\n' or '#'
                int color_count = 0;
                auto const colors_start = curr;
                while (*curr != '\n' && *curr != '#')
                    ++curr;

                bool is_float_color = false;

                int int_colors[4] = {};
                float float_colors[4] = {};

                auto const color_string = cc::string_view(colors_start, curr);
                for (auto const c : color_string.split(cc::is_space, cc::split_options::skip_empty))
                {
                    if (color_count > 4)
                    {
                        on_error(data, cc::as_byte_span(c), "Failed to parse OFF file: Color information can at most have four channels", severity::error);
                        break;
                    }
                    if (c.contains('.'))
                    {
                        if (!is_float_color && color_count > 0)
                        {
                            on_error(data, cc::as_byte_span(c), "Failed to parse OFF file: A single color entry cannot mix float and integer values",
                                     severity::error);
                            break;
                        }
                        else
                        {
                            is_float_color = true;
                        }

                        // float
                        if (!cc::from_string(c, float_colors[color_count]))
                        {
                            on_error(data, cc::as_byte_span(c), "Failed to parse OFF file: Corrupt color entry", severity::error);
                            break;
                        }
                    }
                    else
                    {
                        if (is_float_color)
                        {
                            on_error(data, cc::as_byte_span(c), "Failed to parse OFF file: Color information can at most have four channels", severity::error);
                            break;
                        }
                        // int
                        if (!cc::from_string(c, int_colors[color_count]))
                        {
                            on_error(data, cc::as_byte_span(c), "Failed to parse OFF file: Corrupt color entry", severity::error);
                            break;
                        }
                    }
                    ++color_count;
                }

                if (is_float_color)
                {
                    if (color_count < 3)
                    {
                        on_error(data, data, "Failed to parse OFF file: Float colors must contain RGB or RGBA values", severity::error);
                        break;
                    }
                    // check values between 0 and 1
                    for (auto d = 0; d < color_count; ++d)
                    {
                        if (float_colors[d] < 0.0f || float_colors[d] > 1.0f)
                        {
                            on_error(data, data, "Failed to parse OFF file: Float colors must be in the range [0.0, 1.0]", severity::error);
                            break;
                        }
                        geometry.colors[i][d] = float_colors[d];
                    }
                }
                else
                {
                    if (color_count == 1)
                    {
                        geometry.colormap_indices[i] = int_colors[0];
                    }
                    else if (3 <= color_count && color_count <= 4)
                    {
                        // check values between 0 and 255
                        for (auto d = 0; d < color_count; ++d)
                        {
                            if (int_colors[d] < 0 || int_colors[d] > 255)
                            {
                                on_error(data, data, "Failed to parse OFF file: Integer RGB or RGBA colors must be in the range [0, 255]", severity::error);
                                break;
                            }
                            geometry.colors[i][d] = float(int_colors[d]) / 255;
                        }
                    }
                    else
                    {
                        on_error(data, data, "Failed to parse OFF file: Integer colors must have 3 or 4 entries", severity::error);
                        break;
                    }
                }

                next_line();
            }

            if (has_tex_coords)
                for (auto d = 0; d < 2; ++d)
                    if (!cc::from_string(next_token(), geometry.tex_coords[i][d]))
                    {
                        on_error(data, data, "Failed to parse OFF file: Failed to parse texture coordinates", severity::error);
                        break;
                    }

            next_line();
        }

        skip_spaces_and_comments();

        geometry.faces.resize(n_faces);
        for (auto i = 0; i < n_faces; ++i)
        {
            auto const face_start = int(geometry.face_entries.size());

            int face_vertices = 0;
            if (!cc::from_string(next_token(), face_vertices))
            {
                on_error(data, data, "Failed to parse OFF file: Failed to parse vertex count of face", severity::error);
                break;
            }
            if (face_vertices == 0)
            {
                on_error(data, data, "Failed to parse OFF file: A facec cannot have zero vertices", severity::error);
                break;
            }
            for (auto v = 0; v < face_vertices; ++v)
            {
                int idx;
                if (!cc::from_string(next_token(), idx))
                {
                    on_error(data, data, "Failed to parse OFF file: Failed to parse vertex index of face", severity::error);
                    break;
                }
                if (idx < 0 || n_vertices <= idx)
                {
                    on_error(data, data, "Failed to parse OFF file: Vertex index out of bounds", severity::error);
                    break;
                }
                geometry.face_entries.push_back({idx});
            }
            geometry.faces[i].entries_start = face_start;
            geometry.faces[i].entries_count = int(geometry.face_entries.size()) - face_start;
        }
    }
    return geometry;
}
