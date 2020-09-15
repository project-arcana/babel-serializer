#pragma once

#include <clean-core/array.hh>

#include <babel-serializer/errors.hh>

// general-purpose image loading
// unified but simplified interface for many image types
namespace babel::image
{
enum class channels
{
    invalid = 0,

    grey = 1,
    grey_alpha = 2,
    rgb = 3,
    rgb_alpha = 4,
};

enum class bit_depth
{
    invalid,

    u8,
    u16,
    f32
};

struct read_config
{
    // TODO: format hint?

    // if not invalid, converts the result to the given channel number
    channels desired_channels = channels::invalid;

    // if not invalid, converts the result to the given bit depth
    bit_depth desired_bit_depth = bit_depth::invalid;
};

struct data
{
    channels channels = channels::invalid;
    bit_depth bit_depth = bit_depth::invalid;
    int width = 0;
    int height = 0;
    int depth = 0;
    cc::array<std::byte> data;

    bool is_valid() const { return channels != channels::invalid; }
};

/// reads an image from memory
data read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);
}
