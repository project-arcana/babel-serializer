#pragma once

#include <cstddef>
#include <cstdint>

#include <clean-core/optional.hh>
#include <clean-core/span.hh>
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
enum class reg32 : uint8_t
{
    eax,
    ecx,
    edx,
    ebx,
    esp,
    ebp,
    esi,
    edi,
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
constexpr char const* to_string(reg32 r)
{
    switch (r)
    {
    case reg32::eax:
        return "eax";
    case reg32::ecx:
        return "ecx";
    case reg32::edx:
        return "edx";
    case reg32::ebx:
        return "ebx";
    case reg32::esp:
        return "esp";
    case reg32::ebp:
        return "ebp";
    case reg32::esi:
        return "esi";
    case reg32::edi:
        return "edi";
    }
    return "r<unknown>";
}

enum class mnemonic : uint8_t
{
    invalid,
    extended_resolve,

    push,
    pop,

    mov,
    lea,

    call,
    ret,

    add,
    or_,
    adc,
    sbb,
    and_,
    sub,
    xor_,
    cmp,
};
char const* to_string(mnemonic m);

// syntax is roughly that used in https://www.felixcloutier.com/x86/mov
enum class arg_format : uint8_t
{
    // 0 args
    none,

    opreg,
    imm32,
    opreg_imm,

    // has modrm
    modm = 0b1000'0000,
    modm_modr,
    modr_modm,
    modm_imm8,
};
static constexpr bool has_modrm(arg_format f) { return uint8_t(f) >= 0b1000'0000; }

/// NOTES:
/// - offset_op is always valid, including 0
/// - offsets for modrm/sib/displ/imm cannot be 0 when present, so 0 means absent
/// - size of imm/disp is 1/2/4/8
struct instruction_format
{
    uint8_t offset_op : 4 = 0;
    uint8_t offset_modrm : 4 = 0;

    uint8_t offset_sib : 4 = 0;
    uint8_t offset_displacement : 4 = 0;

    uint8_t offset_immediate : 4 = 0;
    // size = 8/16/32/64
    uint8_t size_immediate : 2 = 0;
    uint8_t size_displacement : 2 = 0;

    arg_format args;
};
static_assert(sizeof(instruction_format) <= 4);

/// a decoded instruction
/// is guaranteed to be valid if data is not nullptr (no out-of-bound reads)
/// NOTE: this is only a "base" decoding
///       i.e. everything needed to inspect the next instruction
///            and to decide if more info is needed
///
/// x64 instructions consist of:
/// - legacy prefixes (1-4 bytes, optional)
/// - opcode with prefixes (1-4 bytes, required)
/// - ModR/M (1 byte, if required)
/// - SIB (1 byte, if required)
/// - displacement (1/2/4/8 byte, if required)
/// - immediate (1/2/4/8 byte, if required)
///
/// TODO: rename to instruction_ref?
///       we could have a "write" version that is 16B, aka up-to-15 B for instruction + 1 B for size
struct instruction
{
    // location of the first instruction byte
    std::byte const* data = nullptr;

    // we have 8 bytes here to use (so each instruction is 16B in memory)

    // 1-15 bytes for x64
    uint8_t size = 0;
    x64::mnemonic mnemonic = x64::mnemonic::invalid;
    std::byte rex = std::byte(0);    // 0 if not present
    std::byte opcode = std::byte(0); // primary opcode byte

    // 4B encoding of format
    instruction_format format;

    // properties
public:
    bool is_valid() const { return data != nullptr; }

    cc::span<std::byte const> as_span() const { return {data, data + size}; }

    cc::string to_string() const;
};
static_assert(sizeof(instruction) <= 2 * sizeof(void*));

/// tries to decode a single instruction
/// returns an invalid instruction if unknown op or if decoding would read beyond end
instruction decode_one(std::byte const* data, std::byte const* end);

//
// properties
//

inline int32_t int32_immediate_of(instruction const& i)
{
    CC_ASSERT(i.format.offset_immediate > 0 && "instruction has no immediate");
    CC_ASSERT(i.format.size_immediate == 2 && "instruction immediate has wrong size");
    return *(int32_t const*)(i.data + i.format.offset_immediate);
}

inline bool is_relative_call(instruction const& i) { return i.opcode == std::byte(0xE8); }
inline std::byte const* relative_call_target(instruction const& i)
{
    CC_ASSERT(is_relative_call(i));
    return i.data + i.size + int32_immediate_of(i);
}

} // namespace babel::x64
