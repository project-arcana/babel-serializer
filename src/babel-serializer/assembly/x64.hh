#pragma once

#include <cstddef>
#include <cstdint>

#include <clean-core/optional.hh>
#include <clean-core/span.hh>
#include <clean-core/string.hh>

#include "x64.gen.hh"

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

// syntax is roughly that used in https://www.felixcloutier.com/x86/mov
enum class arg_format : uint8_t
{
    // 0 args
    none,

    opreg,
    opreg64, // always 64bit
    imm8,
    imm32,
    opreg_imm,

    // has modrm
    has_modm_start,
    modm = has_modm_start,
    modm_modr,
    modr_modm,
    modm_imm8,
    modm_imm32,
};
static_assert(int(arg_format::modm_imm32) <= 0b1111);
static constexpr bool has_modrm(arg_format f) { return f >= arg_format::has_modm_start; }

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

    /// we have 8 bytes here to use (so each instruction is 16B in memory)
    struct
    {
        // opcode with group encoded
        // as a single u16 to prevent accidental aliasing
        // lower u8 is the "normal" opcode
        // higher u8 is:
        //   0x00       - single-byte opcodes
        //   0x00..0x07 - single-byte opcodes with subresolve (e.g. 0x80 add/or/adc/...)
        //
        // NOTE: higher u8 is chosen so
        //       opcode -> mnemonic is actually a function
        //       in particular, SSE ops like 66 0F 3A 0C (blendps) must not alias with CMP
        // NOTE: opcode & 0b111 must keep op-encoded register
        uint16_t opcode = 0;

        // currently ~660 mnemonics
        // stored directly because it's accessed frequently
        uint16_t mnemonic_packed : 10 = 0;

        // 1-15 bytes for x64
        uint8_t size : 4 = 0;

        // packed version of arg_format
        uint8_t args_packed : 4 = 0;

        // lower 4 bits of rex
        uint8_t rex : 4 = 0;

        // is always valid, including 0
        uint8_t offset_op : 3 = 0;
        // 0 means "no ModR/M byte"
        uint8_t offset_modrm : 3 = 0;
        // SIB is always ModR/M + 1

        // 0 means "no displacement"
        uint8_t offset_displacement : 4 = 0;

        // 0 means "no immediate"
        uint8_t offset_immediate : 4 = 0;
        // size = 8/16/32/64
        uint8_t size_immediate : 2 = 0;
        uint8_t size_displacement : 2 = 0;

        // .. we are currently at exactly 64 bits
    };

    // convenience unpacking
    // NOTE: no mem access, just some casting
public:
    x64::mnemonic mnemonic() const { return x64::mnemonic(mnemonic_packed); }
    x64::arg_format arg_format() const { return x64::arg_format(args_packed); }

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
    CC_ASSERT(i.offset_immediate > 0 && "instruction has no immediate");
    CC_ASSERT(i.size_immediate == 2 && "instruction immediate has wrong size");
    return *(int32_t const*)(i.data + i.offset_immediate);
}

inline bool is_conditional_jump(instruction const& i) { return i.op_group == 1 && uint8_t(i.opcode) >= 0x80 && uint8_t(i.opcode) <= 0x8F; }
inline std::byte const* conditional_jump_target(instruction const& i)
{
    CC_ASSERT(is_conditional_jump(i));
    return i.data + i.size + int32_immediate_of(i);
}

inline bool is_unconditional_jump(instruction const& i) { return i.mnemonic == mnemonic::jmp; }
inline std::byte const* unconditional_jump_target(instruction const& i)
{
    CC_ASSERT(is_unconditional_jump(i));
    return i.data + i.size + int32_immediate_of(i);
}

inline bool is_relative_call(instruction const& i) { return i.op_group == 0 && i.opcode == std::byte(0xE8); }
inline std::byte const* relative_call_target(instruction const& i)
{
    CC_ASSERT(is_relative_call(i));
    return i.data + i.size + int32_immediate_of(i);
}

inline bool is_return(instruction const& i) { return i.mnemonic == mnemonic::retn; }

} // namespace babel::x64