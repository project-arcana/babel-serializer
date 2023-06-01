#pragma once

#include <cstddef>
#include <cstdint>

#include <clean-core/optional.hh>
#include <clean-core/string.hh>

namespace babel::x64
{
enum class reg64 : uint8_t
{
    rax,
    rcx,
    rdx,
    rbx,
    rsp,
    rbp,
    rsi,
    rdi,

    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15,
};
constexpr char const* to_string(reg64 r)
{
    switch (r)
    {
    case reg64::rax:
        return "rax";
    case reg64::rcx:
        return "rcx";
    case reg64::rdx:
        return "rdx";
    case reg64::rbx:
        return "rbx";
    case reg64::rsp:
        return "rsp";
    case reg64::rbp:
        return "rbp";
    case reg64::rsi:
        return "rsi";
    case reg64::rdi:
        return "rdi";
    case reg64::r8:
        return "r8";
    case reg64::r9:
        return "r9";
    case reg64::r10:
        return "r10";
    case reg64::r11:
        return "r11";
    case reg64::r12:
        return "r12";
    case reg64::r13:
        return "r13";
    case reg64::r14:
        return "r14";
    case reg64::r15:
        return "r15";
    }
    return "r<unknown>";
}

enum class mnemonic : uint8_t
{
    invalid,
    push,
};
char const* to_string(mnemonic m);

// NOTE: rex versions are always non-rex + 1
enum class instruction_format : uint8_t
{
    unknown,

    b1_opreg,
    b2_rex_opreg,
};

/// a decoded instruction
/// is guaranteed to be valid if data is not nullptr (no out-of-bound reads)
/// NOTE: this is only a "base" decoding
///       i.e. everything needed to inspect the next instruction
///            and to decide if more info is needed
struct instruction
{
    // location of the first instruction byte
    std::byte const* data = nullptr;

    // 1-15 bytes for x64
    uint8_t size = 0;
    x64::mnemonic mnemonic;
    instruction_format format;

    bool is_valid() const { return data != nullptr; }

    cc::string to_string() const;
};
static_assert(sizeof(instruction) <= 2 * sizeof(void*));

/// tries to decode a single instruction
/// returns an invalid instruction if unknown op or if decoding would read beyond end
instruction decode_one(std::byte const* data, std::byte const* end);
} // namespace babel::x64
