#pragma once

#include <cstddef>

namespace babel
{
// TODO: improve me
//       - maybe span instead of pos?
struct deserialization_error
{
    deserialization_error() = default;
    deserialization_error(size_t pos, char const* msg) : _pos(pos), _message(msg) {}

    size_t pos() const { return _pos; }
    char const* message() const { return _message; }

private:
    size_t _pos = 0; /// position in the data stream
    char const* _message = nullptr;
};
}
