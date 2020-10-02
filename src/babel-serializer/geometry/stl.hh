#pragma once

#include <cstddef> // std::byte

#include <clean-core/span.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <babel-serializer/errors.hh>

namespace babel::stl
{
enum class file_type
{
    autodetect,
    binary,
    ascii
};
struct read_config
{
    file_type file_type = file_type::autodetect;
};

/// See: https://en.wikipedia.org/wiki/STL_(file_format)
/// We accept colors in VisCAM and SolidView, as well as Materialise Magics format
struct geometry
{
    /// right-hand-rule triangles with normal information
    struct triangle
    {
        tg::pos3 v0;
        tg::pos3 v1;
        tg::pos3 v2;
        tg::vec3 normal;
    };

    struct color
    {
        std::byte r;
        std::byte g;
        std::byte b;
        std::byte a;
    };

    /// The name is optional and always emtpy for binary STL files
    cc::string name;
    cc::vector<triangle> triangles;
    cc::vector<color> triangle_color;
};

geometry read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);
}
