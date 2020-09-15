#include "image.hh"

#include <babel-serializer/detail/stb_image.hh>

babel::image::data babel::image::read(cc::span<const std::byte> data, read_config const& cfg, error_handler on_error)
{
    babel::image::data d;

    // stb types
    auto buffer_data = (babel_stbi_uc*)(data.data());
    auto buffer_size = int(data.size());

    // fixed bit depth
    if (cfg.desired_bit_depth != bit_depth::invalid)
        d.bit_depth = cfg.desired_bit_depth;
    else if (babel_stbi_is_hdr_from_memory(buffer_data, buffer_size))
        d.bit_depth = bit_depth::f32;
    else if (babel_stbi_is_16_bit_from_memory(buffer_data, buffer_size))
        d.bit_depth = bit_depth::u16;
    else
        d.bit_depth = bit_depth::u8;

    // load
    int w, h;
    int stored_channels;
    int bit_depth_size;
    std::byte const* ptr;
    switch (d.bit_depth)
    {
    case bit_depth::u8:
        ptr = reinterpret_cast<std::byte const*>(babel_stbi_load_from_memory(buffer_data, buffer_size, &w, &h, &stored_channels, int(cfg.desired_channels)));
        bit_depth_size = 1;
        break;
    case bit_depth::u16:
        ptr = reinterpret_cast<std::byte const*>(babel_stbi_load_16_from_memory(buffer_data, buffer_size, &w, &h, &stored_channels, int(cfg.desired_channels)));
        bit_depth_size = 1;
        break;
    case bit_depth::f32:
        ptr = reinterpret_cast<std::byte const*>(babel_stbi_loadf_from_memory(buffer_data, buffer_size, &w, &h, &stored_channels, int(cfg.desired_channels)));
        bit_depth_size = 1;
        break;
    default:
        CC_UNREACHABLE("unsupported bit depth");
    }

    // channels
    d.channels = cfg.desired_channels != channels::invalid ? cfg.desired_channels : channels(stored_channels);

    if (ptr)
    {
        d.width = w;
        d.height = h;
        d.depth = 1;
        d.data = cc::array<std::byte>::uninitialized(w * h * int(d.channels) * bit_depth_size);
        std::memcpy(d.data.data(), ptr, d.data.size());
    }
    else // error
    {
        on_error(data, data, babel_stbi_failure_reason(), severity::error);
    }

    return d;
}
