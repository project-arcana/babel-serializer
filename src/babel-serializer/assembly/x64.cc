#include "x64.hh"

#include <babel-serializer/detail/log.hh>

#include <clean-core/flags.hh>
#include <clean-core/format.hh>

#include <rich-log/log.hh>

// explanations:
// - https://www-user.tu-chemnitz.de/~heha/hsn/chm/x86.chm/x64.htm
// - https://www.systutorials.com/beginners-guide-x86-64-instruction-encoding/
// - https://pyokagan.name/blog/2019-09-20-x86encoding/

// references:
// - https://wiki.osdev.org/X86-64_Instruction_Encoding
//   (contains register mapping at the beginning)
//   (contains parts of the instruction)
// - http://ref.x86asm.net/geek64.html
//   (field description in http://ref.x86asm.net/#column_flds )
// - https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html (waaay too long)
// - https://www.felixcloutier.com/x86/index.html

namespace babel::x64
{
namespace
{
constexpr reg64 reg64_from_op(uint32_t op, bool extended) { return reg64((op & 0b111) + (extended ? 8 : 0)); }
constexpr reg32 reg32_from_op(uint32_t op) { return reg32((op & 0b111)); }

//
// REX
//

// see https://wiki.osdev.org/X86-64_Instruction_Encoding#Encoding
constexpr bool is_rex(std::byte b)
{
    auto v = uint8_t(b);
    return (v & 0b11110000) == 0b01000000;
}
// precondition: is rex
// aka is REX.B set
constexpr bool is_rex_b(uint8_t v) { return v & 0b0001; }
// precondition: is rex
// aka is REX.X set
constexpr bool is_rex_x(uint8_t v) { return v & 0b0010; }
// precondition: is rex
// aka is REX.R set
constexpr bool is_rex_r(uint8_t v) { return v & 0b0100; }
// precondition: is rex
// aka is REX.W set
constexpr bool is_rex_w(uint8_t v) { return v & 0b1000; }

//
// ModR/M
//

enum class modrm_mode : uint8_t
{
    register_indirect = 0b00,
    memory_disp8 = 0b01,
    memory_disp32_64 = 0b10,
    register_direct = 0b11,
};
modrm_mode modrm_mode_of(std::byte b) { return modrm_mode(uint8_t(b) >> 6); }
uint8_t modrm_reg_of(std::byte b) { return (uint8_t(b) >> 3) & 0b111; }
uint8_t modrm_rm_of(std::byte b) { return uint8_t(b) & 0b111; }

//
// ModR/M
//

uint8_t sib_scale_of(std::byte b) { return 1 << (uint8_t(b) >> 6); }
uint8_t sib_index_of(std::byte b, uint8_t rex) { return ((uint8_t(b) >> 3) & 0b111) + (is_rex_x(rex) ? 8 : 0); }
uint8_t sib_base_of(std::byte b, uint8_t rex) { return (uint8_t(b) & 0b111) + (is_rex_b(rex) ? 8 : 0); }

//
// Tables
//

enum class subres_entry
{
    prim_81,
    prim_83,
    prim_C1,
    prim_FF,
};

struct x64_op_info_t
{
    // if mnemonic is sub_resolve
    // then args contains offset into _sub table
    // which then serves +0..7
    x64::mnemonic mnemonic[256] = {};
    arg_format args[256] = {};

    constexpr void set_op(uint8_t code, x64::mnemonic m, arg_format args)
    {
        CC_ASSERT(this->mnemonic[code] == x64::mnemonic::_invalid && "already set");
        this->mnemonic[code] = m;
        this->args[code] = args;
    }
    constexpr void set_subres(uint8_t code, subres_entry target)
    {
        CC_ASSERT(this->mnemonic[code] == x64::mnemonic::_invalid && "already set");
        this->mnemonic[code] = x64::mnemonic::_subresolve;
        this->args[code] = arg_format(uint8_t(target) * 8);
    }
};

// first byte opcode
/*static constexpr x64_op_info_t x64_op_info = []
{
    // TODO: generate me
    x64_op_info_t info;

    // NOTE: rex is handled separately

    // push / pop
    for (auto r = 0; r < 8; ++r)
        info.set_op(0x50 + r, mnemonic::push, arg_format::opreg64);
    for (auto r = 0; r < 8; ++r)
        info.set_op(0x58 + r, mnemonic::pop, arg_format::opreg64);

    // move
    info.set_op(0xC7, mnemonic::mov, arg_format::modm_imm32);
    info.set_op(0x89, mnemonic::mov, arg_format::modm_modr);
    info.set_op(0x8B, mnemonic::mov, arg_format::modr_modm);
    info.set_op(0x8D, mnemonic::lea, arg_format::modr_modm);

    // shift / rot
    info.set_subres(0xC1, subres_entry::prim_C1);

    // add/or/...
    {
        // extended
        info.set_subres(0x81, subres_entry::prim_81);
        info.set_subres(0x83, subres_entry::prim_83);

        // non-extended
        info.set_op(0x01, mnemonic::add, arg_format::modm_modr);
        info.set_op(0x03, mnemonic::add, arg_format::modr_modm);
        info.set_op(0x29, mnemonic::sub, arg_format::modm_modr);
        info.set_op(0x2B, mnemonic::sub, arg_format::modr_modm);
        info.set_op(0x31, mnemonic::xor_, arg_format::modm_modr);
        info.set_op(0x33, mnemonic::xor_, arg_format::modr_modm);
        info.set_op(0x39, mnemonic::cmp, arg_format::modm_modr);
        info.set_op(0x3B, mnemonic::cmp, arg_format::modr_modm);
    }

    // test
    info.set_op(0x85, mnemonic::test, arg_format::modm_modr);

    // call
    info.set_op(0xE8, mnemonic::call, arg_format::imm32);

    // jmp
    info.set_op(0x70, mnemonic::jo, arg_format::imm8);
    info.set_op(0x71, mnemonic::jno, arg_format::imm8);
    info.set_op(0x72, mnemonic::jb, arg_format::imm8);
    info.set_op(0x73, mnemonic::jae, arg_format::imm8);
    info.set_op(0x74, mnemonic::je, arg_format::imm8);
    info.set_op(0x75, mnemonic::jne, arg_format::imm8);
    info.set_op(0x76, mnemonic::jbe, arg_format::imm8);
    info.set_op(0x77, mnemonic::ja, arg_format::imm8);
    info.set_op(0x78, mnemonic::js, arg_format::imm8);
    info.set_op(0x79, mnemonic::jns, arg_format::imm8);
    info.set_op(0x7A, mnemonic::jpe, arg_format::imm8);
    info.set_op(0x7B, mnemonic::jpo, arg_format::imm8);
    info.set_op(0x7C, mnemonic::jl, arg_format::imm8);
    info.set_op(0x7D, mnemonic::jge, arg_format::imm8);
    info.set_op(0x7E, mnemonic::jle, arg_format::imm8);
    info.set_op(0x7F, mnemonic::jg, arg_format::imm8);
    info.set_op(0xE9, mnemonic::jmp, arg_format::imm32);

    // ret
    info.set_op(0xC3, mnemonic::retn, arg_format::none);

    // various
    info.set_subres(0xFF, subres_entry::prim_FF);

    return info;
}();

// 0F + opcode
static constexpr x64_op_info_t x64_op_info_0F = []
{
    // TODO: generate me
    x64_op_info_t info;

    // cmovs
    info.set_op(0x40, mnemonic::cmovo, arg_format::modr_modm);
    info.set_op(0x41, mnemonic::cmovno, arg_format::modr_modm);
    info.set_op(0x42, mnemonic::cmovb, arg_format::modr_modm);
    info.set_op(0x43, mnemonic::cmovnb, arg_format::modr_modm);
    info.set_op(0x44, mnemonic::cmovz, arg_format::modr_modm);
    info.set_op(0x45, mnemonic::cmovnz, arg_format::modr_modm);
    info.set_op(0x46, mnemonic::cmovbe, arg_format::modr_modm);
    info.set_op(0x47, mnemonic::cmova, arg_format::modr_modm);
    info.set_op(0x48, mnemonic::cmovs, arg_format::modr_modm);
    info.set_op(0x49, mnemonic::cmovns, arg_format::modr_modm);
    info.set_op(0x4A, mnemonic::cmovp, arg_format::modr_modm);
    info.set_op(0x4B, mnemonic::cmovnp, arg_format::modr_modm);
    info.set_op(0x4C, mnemonic::cmovl, arg_format::modr_modm);
    info.set_op(0x4D, mnemonic::cmovge, arg_format::modr_modm);
    info.set_op(0x4E, mnemonic::cmovle, arg_format::modr_modm);
    info.set_op(0x4F, mnemonic::cmovg, arg_format::modr_modm);

    // jumps
    info.set_op(0x80, mnemonic::jo, arg_format::imm32);
    info.set_op(0x81, mnemonic::jno, arg_format::imm32);
    info.set_op(0x82, mnemonic::jb, arg_format::imm32);
    info.set_op(0x83, mnemonic::jae, arg_format::imm32);
    info.set_op(0x84, mnemonic::je, arg_format::imm32);
    info.set_op(0x85, mnemonic::jne, arg_format::imm32);
    info.set_op(0x86, mnemonic::jbe, arg_format::imm32);
    info.set_op(0x87, mnemonic::ja, arg_format::imm32);
    info.set_op(0x88, mnemonic::js, arg_format::imm32);
    info.set_op(0x89, mnemonic::jns, arg_format::imm32);
    info.set_op(0x8A, mnemonic::jpe, arg_format::imm32);
    info.set_op(0x8B, mnemonic::jpo, arg_format::imm32);
    info.set_op(0x8C, mnemonic::jl, arg_format::imm32);
    info.set_op(0x8D, mnemonic::jge, arg_format::imm32);
    info.set_op(0x8E, mnemonic::jle, arg_format::imm32);
    info.set_op(0x8F, mnemonic::jg, arg_format::imm32);

    return info;
}();*/

// subop
static constexpr x64_op_info_t x64_op_info_sub = []
{
    // TODO: generate me
    x64_op_info_t info;

    // shift / rot
    {
        auto const args = arg_format::modm_imm8;
        auto const base = uint8_t(subres_entry::prim_C1) * 8;
        info.set_op(base + 0, mnemonic::rol, args);
        info.set_op(base + 1, mnemonic::ror, args);
        info.set_op(base + 2, mnemonic::rcl, args);
        info.set_op(base + 3, mnemonic::rcr, args);
        info.set_op(base + 4, mnemonic::sal, args);
        info.set_op(base + 5, mnemonic::shr, args);
        info.set_op(base + 6, mnemonic::sal, args);
        info.set_op(base + 7, mnemonic::sar, args);
    }

    // add / and / or / ...
    for (auto code : {subres_entry::prim_81, subres_entry::prim_83})
    {
        auto const args = code == subres_entry::prim_81 ? arg_format::modm_imm32 : arg_format::modm_imm8;
        auto const base = uint8_t(code) * 8;
        info.set_op(base + 0, mnemonic::add, args);
        info.set_op(base + 1, mnemonic::or_, args);
        info.set_op(base + 2, mnemonic::adc, args);
        info.set_op(base + 3, mnemonic::sbb, args);
        info.set_op(base + 4, mnemonic::and_, args);
        info.set_op(base + 5, mnemonic::sub, args);
        info.set_op(base + 6, mnemonic::xor_, args);
        info.set_op(base + 7, mnemonic::cmp, args);
    }

    // 0xFF
    {
        // auto const base = uint8_t(subres_entry::prim_FF) * 8;
        // TODO
        // info.set_op(base + 3, mnemonic::call, )
    }

    return info;
}();

void add_opreg_to_string(cc::string& s, babel::x64::instruction const& in)
{
    if (is_rex_w(in.rex))
    {
        LOG_WARN("TODO: which rex byte is used here?");
        CC_UNREACHABLE("TODO");
        // s += x64::to_string(reg64_from_op(opcode, is_rex_b(rex)));
    }
    else
        s += x64::to_string(reg32_from_op(in.opcode));
}

void add_opreg64_to_string(cc::string& s, babel::x64::instruction const& in)
{
    if (is_rex_w(in.rex))
    {
        LOG_WARN("TODO: which rex byte is used here?");
        CC_UNREACHABLE("TODO");
        // s += x64::to_string(reg64_from_op(opcode, is_rex_b(rex)));
    }
    else
        s += x64::to_string(reg64_from_op(in.opcode, false));
}

void add_modr_to_string(cc::string& s, babel::x64::instruction const& in)
{
    auto const modrm = in.data[in.offset_modrm];
    // auto const mode = modrm_mode_of(modrm);
    // CC_ASSERT(mode == modrm_mode::register_direct && "TODO"); -- is always register, right?

    auto regi = modrm_reg_of(modrm);

    // 64bit
    if (is_rex_w(in.rex))
    {
        if (is_rex_r(in.rex))
            regi += 8;

        s += x64::to_string(reg64(regi));
    }
    else // 32bit
    {
        s += x64::to_string(reg32(regi));
    }
}

void add_disp8_to_string(cc::string& s, std::byte d)
{
    auto v = int8_t(d);
    if (v >= 0)
    {
        s += " + 0x";
        s += cc::to_string(d);
    }
    else
    {
        s += " - 0x";
        s += cc::to_string(std::byte(-v));
    }
}

void add_modm_to_string(cc::string& s, babel::x64::instruction const& in)
{
    auto const modrm = in.data[in.offset_modrm];
    auto const mode = modrm_mode_of(modrm);

    if (mode != modrm_mode::register_direct)
        s += '[';

    auto regi = modrm_rm_of(modrm);
    if (is_rex_b(in.rex))
        regi += 8;

    auto skip_plus = false;

    // SIB
    auto has_special_sip_disp32 = false;
    if (mode != modrm_mode::register_direct && regi == 0b100)
    {
        CC_ASSERT(in.offset_modrm > 0 && "no mod/rm sib");
        auto const sib = in.data[in.offset_modrm + 1];
        auto const scale = sib_scale_of(sib);
        auto const index = sib_index_of(sib, in.rex);
        auto const base = sib_base_of(sib, in.rex);

        auto const no_base_reg = mode == modrm_mode::register_indirect && (base & 0b111) == 0b101;
        auto const no_index_reg = mode != modrm_mode::register_indirect && index == 0b0100;

        if (no_base_reg)
            has_special_sip_disp32 = true;

        if (no_index_reg && no_base_reg)
        {
            // only disp
            skip_plus = true;
        }
        else if (no_index_reg)
        {
            s += x64::to_string(reg64(base));
        }
        else if (no_base_reg)
        {
            s += x64::to_string(reg64(index));
            s += " * ";
            s += cc::to_string(scale);
        }
        else
        {
            s += x64::to_string(reg64(base));
            s += " + ";
            s += x64::to_string(reg64(index));
            s += " * ";
            s += cc::to_string(scale);
        }
    }
    else if (mode == modrm_mode::register_indirect && regi == 0b101)
    {
        CC_UNREACHABLE("TODO: impl RIP+disp");
    }
    // r/m register
    else
    {
        if (mode != modrm_mode::register_direct || is_rex_w(in.rex))
            s += x64::to_string(reg64(regi));
        else
            s += x64::to_string(reg32(regi));
    }

    // displacement
    if (mode == modrm_mode::memory_disp8)
    {
        CC_ASSERT(in.offset_displacement > 0 && "instruction has no disp set");
        add_disp8_to_string(s, in.data[in.offset_displacement]);
    }
    else if (mode == modrm_mode::memory_disp32_64 || has_special_sip_disp32)
    {
        CC_ASSERT(in.offset_displacement > 0 && "instruction has no disp set");
        if (!skip_plus)
            s += " + ";
        s += "0x";
        for (auto i = 3; i >= 0; --i)
            s += cc::to_string(in.data[in.offset_displacement + i]);
    }

    if (mode != modrm_mode::register_direct)
        s += ']';
}

void add_imm8_to_string(cc::string& s, babel::x64::instruction const& in)
{
    CC_ASSERT(in.offset_immediate > 0 && "no immediate available");
    s += "0x";
    s += cc::to_string(in.data[in.offset_immediate]);
}

void add_imm32_to_string(cc::string& s, babel::x64::instruction const& in)
{
    CC_ASSERT(in.offset_immediate > 0 && "no immediate available");
    s += "0x";
    for (auto i = 3; i >= 0; --i)
        s += cc::to_string(in.data[in.offset_immediate + i]);
}

} // namespace
} // namespace babel::x64

babel::x64::instruction babel::x64::decode_one(std::byte const* data, std::byte const* end)
{
    if (data >= end)
        return {};

    instruction instr;
    arg_format args = arg_format::none;
    mnemonic mnem = mnemonic::_invalid;
    std::byte rex = std::byte(0);
    instr.data = data;
    auto op = *data++;

    // parse prefixes
    auto pref_0F = false;  // two-byte opcodes
    auto pref_66 = false;  // size prefix | op sel
    auto pref_67 = false;  // addr size prefix
    auto pref_F2 = false;  // rep
    auto pref_F3 = false;  // rep
    auto pref_F0 = false;  // lock

    auto check_for_prefixes = true;
    while (check_for_prefixes)
    {
        switch (uint8_t(op))
        {
        case 0x0F:
            pref_0F = true;
            break;
        case 0x66:
            pref_66 = true;
            break;
        case 0x67:
            pref_67 = true;
            break;
        case 0xF2:
            pref_F2 = true;
            break;
        case 0xF3:
            pref_F3 = true;
            break;
        case 0xF0:
            pref_F0 = true;
            break;

        case 0x9B:
            LOG_WARN("TODO: support x87 instruction for byte 0x%s (in %s)", op, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
            return {};

        default:
            if (is_rex(op))
            {
                rex = op;
            }
            else
            {
                // no prefix anymore
                check_for_prefixes = false;

                // no next data
                continue;
            }
        }

        // next
        if (data >= end)
            return {};

        op = *data++;
    }
    instr.offset_op = data - instr.data;

    // first round of decode
    // two tables can be directly addressed
    //   one-byte ops from 0x000..0x0FF
    //   two-byte ops from 0x100..0x1FF
    {
        auto dec_idx = int(op);
        if (pref_0F)
            dec_idx += 0xFF;

        // 10 bit mnemonic
        //  4 bit args
        //  2 bit status
        auto dec = detail::decode_table[dec_idx];
        mnem = mnemonic(dec & 0b11'1111'1111);
        args = arg_format((dec >> 10) & 0b1111);
    }

    // unknown op?
    if (mnem == mnemonic::_invalid)
    {
        LOG_WARN("unknown instruction for byte 0x%s (in %s)", op, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
        return {};
    }

    // subresolve:
    // - some op codes like 0x80 (add/or/...) have 8 "subcodes" in their ModR/M byte
    // - those are marked as subresolve in the 1st phase lookup

    // ModR/M
    if (mnem == mnemonic::_subresolve || has_modrm(args))
    {
        if (data >= end)
            return {};

        instr.offset_modrm = data - instr.data;
        auto modrm = *data++;
        auto mod = modrm_mode_of(modrm);

        // extended mnemonic
        if (mnem == mnemonic::_subresolve)
        {
            auto subcode = int(args) + modrm_reg_of(modrm);
            mnem = x64_op_info_sub.mnemonic[subcode];
            args = x64_op_info_sub.args[subcode];
        }
        if (mnem == mnemonic::_invalid)
        {
            LOG_WARN("unknown instruction for byte 0x%s (in %s)", op, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
            return {};
        }

        // see https://wiki.osdev.org/X86-64_Instruction_Encoding#32.2F64-bit_addressing

        // SIB
        auto has_special_sip_disp32 = false;
        if (mod != modrm_mode::register_direct && modrm_rm_of(modrm) == 0b100)
        {
            if (data >= end)
                return {};

            auto const sib = *data++;

            if (mod == modrm_mode::register_indirect && sib_base_of(sib, 0) == 0b101)
                has_special_sip_disp32 = true;
        }

        // disp8
        if (mod == modrm_mode::memory_disp8)
        {
            if (data >= end)
                return {};

            instr.offset_displacement = data - instr.data;
            instr.size_displacement = 0;
            data++;
        }
        // disp32
        else if (mod == modrm_mode::memory_disp32_64 || has_special_sip_disp32)
        {
            if (data + 3 >= end)
                return {};

            // TODO: is there 64 bit displacement?
            instr.offset_displacement = data - instr.data;
            instr.size_displacement = 2;
            data += 4;
        }

        // special
        CC_ASSERT(!(mod == modrm_mode::register_indirect && modrm_rm_of(modrm) == 0b101) && "TODO: implement RIP/EIP+disp32");
    }

    // immediate args
    switch (args)
    {
    case arg_format::opreg_imm:
        CC_UNREACHABLE("TODO");
        break;

    case arg_format::imm32:
    case arg_format::modm_imm32:
        if (data + 3 >= end)
            return {};
        instr.offset_immediate = data - instr.data;
        instr.size_immediate = 2;
        data += 4;
        break;

    case arg_format::imm8:
    case arg_format::modm_imm8:
    case arg_format::modm_modr_imm8:
    case arg_format::modr_modm_imm8:
        if (data >= end)
            return {};
        instr.offset_immediate = data - instr.data;
        instr.size_immediate = 0;
        data++;
        break;

    case arg_format::modm:
    case arg_format::modm_modr:
    case arg_format::modr_modm:
    case arg_format::opreg:
    case arg_format::opreg64:
    case arg_format::none:
        // no immediate
        break;
    }

    // finalize
    instr.mnemonic_packed = uint16_t(mnem);
    instr.args_packed = uint8_t(args);
    instr.rex = uint8_t(rex) & 0b1111;
    instr.size = data - instr.data;
    return instr;
}

cc::string babel::x64::instruction::to_string() const
{
    if (!is_valid())
        return "<invalid instruction>";

    cc::string s;
    s += x64::to_string(mnemonic());

    switch (arg_format())
    {
        // 0 args
    case arg_format::none:
        break;

        // 1 arg
    case arg_format::opreg:
        s += ' '; // TODO: pad?
        add_opreg_to_string(s, *this);
        break;
    case arg_format::opreg64:
        s += ' '; // TODO: pad?
        add_opreg64_to_string(s, *this);
        break;
    case arg_format::imm8:
        s += ' '; // TODO: pad?
        add_imm8_to_string(s, *this);
        break;
    case arg_format::imm32:
        s += ' '; // TODO: pad?
        add_imm32_to_string(s, *this);
        break;
    case arg_format::modm:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        break;

        // 2 args
    case arg_format::modm_modr:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_modr_to_string(s, *this);
        break;
    case arg_format::modr_modm:
        s += ' '; // TODO: pad?
        add_modr_to_string(s, *this);
        s += ", ";
        add_modm_to_string(s, *this);
        break;
    case arg_format::opreg_imm:
        CC_UNREACHABLE("TODO");
        break;
    case arg_format::modm_imm8:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm8_to_string(s, *this);
        break;
    case arg_format::modm_imm32:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm32_to_string(s, *this);
        break;

        // 3 args
    case arg_format::modm_modr_imm8:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_modr_to_string(s, *this);
        s += ", ";
        add_imm8_to_string(s, *this);
        break;
    case arg_format::modr_modm_imm8:
        s += ' '; // TODO: pad?
        add_modr_to_string(s, *this);
        s += ", ";
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm8_to_string(s, *this);
        break;
    }

    return s;
}
