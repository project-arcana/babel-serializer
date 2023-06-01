#include "x64.hh"

#include <babel-serializer/detail/log.hh>

#include <clean-core/format.hh>

#include <rich-log/log.hh>

// explanations:
// - https://wiki.osdev.org/X86-64_Instruction_Encoding
// - https://www-user.tu-chemnitz.de/~heha/hsn/chm/x86.chm/x64.htm
// - https://www.systutorials.com/beginners-guide-x86-64-instruction-encoding/
// - https://pyokagan.name/blog/2019-09-20-x86encoding/

// references:
// - http://ref.x86asm.net/geek64.html
//   (field description in http://ref.x86asm.net/#column_flds )
// - https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html (waaay too long)
// - https://www.felixcloutier.com/x86/index.html

namespace babel::x64
{
namespace
{
reg64 reg64_from_op(std::byte op, bool extended) { return reg64((uint8_t(op) & 0b111) + (extended ? 8 : 0)); }

bool is_rex(std::byte b)
{
    auto v = uint8_t(b);
    return 0x40 <= v && v < 0x50;
}
// aka is rex and is REX.B set
bool is_rex_b_set(std::byte b)
{
    auto v = uint8_t(b);
    return v & 1;
}

struct x64_op_info_t
{
    mnemonic primary_op_to_mnemonic[256] = {};
    instruction_format primary_op_to_fmt[256] = {};
};

static constexpr x64_op_info_t x64_op_info = []
{
    // TODO: generate me
    x64_op_info_t info;

    // push
    for (auto r = 0; r < 8; ++r)
    {
        info.primary_op_to_mnemonic[0x50 + r] = mnemonic::push;
        info.primary_op_to_fmt[0x50 + r] = instruction_format::b1_opreg;
    }

    // rex
    for (auto f = 0; f < 16; ++f)
    {
        // info.primary_op_to_mnemonic[0x40 + f] = mnemonic::invalid;
        // info.primary_op_to_fmt[0x40 + f] = instruction_format::b1_opreg;
    }

    return info;
}();
} // namespace
// namespace
} // namespace babel::x64

babel::x64::instruction babel::x64::decode_one(std::byte const* data, std::byte const* end)
{
    if (data >= end)
        return {};

    instruction instr;
    instr.data = data;

    auto prim_op = *data++;
    auto rex_byte = std::byte(0);

    // rex prefix
    if (is_rex(prim_op))
    {
        if (data >= end)
            return {};

        rex_byte = prim_op;
        prim_op = *data++;
    }

    // look up primary op
    instr.mnemonic = x64_op_info.primary_op_to_mnemonic[int(prim_op)];
    instr.format = x64_op_info.primary_op_to_fmt[int(prim_op)];
    if (instr.format == instruction_format::unknown)
    {
        LOG_WARN("unknown instruction format for byte 0x%s", prim_op);
        return {};
    }

    // rex formats are always format+1
    if (rex_byte != std::byte(0))
        instr.format = instruction_format(uint8_t(instr.format) + 1);

    // special per format decoding
    switch (instr.format)
    {
    case instruction_format::b1_opreg:
    case instruction_format::b2_rex_opreg:
        // none
        break;

    case instruction_format::unknown:
        CC_UNREACHABLE();
    }

    instr.size = data - instr.data;
    return instr;
}

char const* babel::x64::to_string(mnemonic m)
{
    switch (m)
    {
    case mnemonic::invalid:
        return "<invalid-op>";
    case mnemonic::push:
        return "push";
    }
    return "<unknown-op>";
}

cc::string babel::x64::instruction::to_string() const
{
    if (!is_valid())
        return "<invalid instruction>";

    switch (format)
    {
    case instruction_format::b1_opreg:
        return cc::format("%s %s", mnemonic, reg64_from_op(data[0], false));
    case instruction_format::b2_rex_opreg:
        return cc::format("%s %s", mnemonic, reg64_from_op(data[1], is_rex_b_set(data[0])));
    case instruction_format::unknown:
        return cc::format("<unknown instruction format for '%s'>", mnemonic);
    }
}
