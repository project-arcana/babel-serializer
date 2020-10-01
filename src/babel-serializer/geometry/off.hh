#pragma once

#include <cstddef> // std::byte

#include <clean-core/span.hh>
#include <clean-core/typedefs.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <babel-serializer/errors.hh>

namespace babel::off
{
struct read_config
{
    bool has_colors = false;
    bool has_normals = false;
    bool has_tex_coords = false;
    int dimension = 3;
};
struct geometry
{
    struct face_entry
    {
        int vertex_index = -1;
    };
    struct face
    {
        int entries_start = -1;
        int entries_count = -1;
    };

    cc::vector<tg::pos4> vertices;
    cc::vector<tg::vec3> normals;
    cc::vector<tg::pos2> tex_coords;
    cc::vector<face_entry> face_entries;
    cc::vector<face> faces;
    cc::vector<tg::color4> colors;
    cc::vector<int> colormap_indices;
};
geometry read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);
}
