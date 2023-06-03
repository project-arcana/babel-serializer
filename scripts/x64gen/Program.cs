
using System.Net.Http;
using System.Xml.Linq;

var xml_local_path = "x64asm-ref.xml";
var xml_url = "http://ref.x86asm.net/x86reference.xml";
var gen_file_cc = "../../src/babel-serializer/assembly/x64.gen.cc";
var gen_file_hh = "../../src/babel-serializer/assembly/x64.gen.hh";

//
// fetch reference
//
if (!File.Exists(xml_local_path))
{
    System.Console.WriteLine("Local x64 reference does not exist, downloading it ...");
    using (var client = new HttpClient())
    {
        var response = await client.GetAsync(xml_url);

        if (!response.IsSuccessStatusCode)
        {
            System.Console.WriteLine("Error while downloading");
            return;
        }

        using (var content = response.Content)
        {
            var data = await content.ReadAsStringAsync();
            File.WriteAllText(xml_local_path, data);
            System.Console.WriteLine($"Written ref to {xml_url}");
        }
    }
}
if (!File.Exists(xml_local_path))
{
    System.Console.WriteLine("Still no ref data...");
    return;
}
var refdoc = XDocument.Load(xml_local_path);

//
// parse reference
//
var mnemonics_set = new HashSet<string>();
string NormalizeMnemonic(string s)
{
    s = s.Trim().ToLower();
    // some rewrites
    switch (s)
    {
        case "shl": s = "sal"; break;
        case "jnae": s = "jb"; break;
        case "jc": s = "jb"; break;
        case "jnb": s = "jae"; break;
        case "jnc": s = "jae"; break;
        case "jz": s = "je"; break;
        case "jnz": s = "jne"; break;
        case "jna": s = "jbe"; break;
        case "jnbe": s = "ja"; break;
        case "jp": s = "jpe"; break;
        case "jnp": s = "jpo"; break;
        case "jnge": s = "jl"; break;
        case "jnl": s = "jge"; break;
        case "jng": s = "jle"; break;
        case "jnle": s = "jg"; break;
    }
    return s;
}
string SafeMnemonic(string s)
{
    switch (s)
    {
        case "and": return "and_";
        case "or": return "or_";
        case "xor": return "xor_";
        case "int": return "int_";
        case "not": return "not_";
        default:
            return s;
    }
}
string AddrOfOp(XElement? e) => e?.Element("a")?.Value ?? e?.Attribute("address")?.Value ?? e?.Value ?? "";
string TypeOfOp(XElement? e) => e?.Element("t")?.Value ?? e?.Attribute("type")?.Value ?? "";
var entries = new List<OpEntry>();
void ProcessEntry(XElement entry, XElement syntax, string mnemonic)
{
    if (entry.Parent!.Name != "pri_opcd")
        throw new InvalidOperationException("expected pri_opcd as parent");

    // any mnemonic seens is recorded
    mnemonics_set.Add(mnemonic);

    // FIXME: 
    //  syntax:
    //     mnem
    //     op0
    //     op1
    // NOT ALWAYS dst src or src dst but also src src
    
    var src = AddrOfOp(syntax.Element("src"));
    var dst = AddrOfOp(syntax.Element("dst"));
    var src_t = TypeOfOp(syntax.Element("src"));
    var dst_t = TypeOfOp(syntax.Element("dst"));
    var pri_opcd = entry.Parent.Attribute("value")!.Value;
    var pri_opcode = int.Parse(pri_opcd, System.Globalization.NumberStyles.HexNumber);

    var op_cat = entry.Parent!.Parent!.Name;
    var is_one_byte = op_cat == "one-byte";
    var is_two_byte = op_cat == "two-byte"; // 0x0F
    var cat = is_one_byte ? "one" :
              is_two_byte ? "two" :
              throw new InvalidOperationException();

    // TODO: print hints
    // SC/AL/... are fixed implicit args and not encoded
    bool isEmptyArg(string s) => s == "" || s == "SC" || s == "AL" || s == "eAX" || s == "rAX" || s == "rCX" || s == "rBP" || s == "X" || s == "Y";
    bool isOpReg(string s) => s == "Z";
    // O is some kind of offset, treated as immediate for decoding
    bool isImmediate(string s) => s == "J" || s == "I" || s == "A" || s == "O";
    bool isModR(string s) => s == "G" || s == "V" || s == "P";
    bool isModM(string s) => s == "E" || s == "W" || s == "Q" || s == "M" || s == "U";
    string ImmTypeOf(string t)
    {
        switch (t)
        {
            case "b":
            case "bs": // sign extended
            case "bss": // sign extended to stack ptr size
                return "8";

            // "The most important part is ds code, which means doubleword, sign-extended to 64 bits for 64-bit operand size."
            // TODO: is 16 bit with 0x66 size prefix
            case "vds":
                return "32";
            case "vs":
                return "32";

            // promoted by REX.W
            case "vqp":
                return "32_64";

            case "w":
                return "16";
        }
        System.Console.WriteLine($"unknown immediate type '{t}' for {mnemonic} {pri_opcd}");
        return "";
    }
    var has_dst = !isEmptyArg(dst);
    var has_src = !isEmptyArg(src);

    var extended_op = entry.Element("opcd_ext")?.Value;
    var is_extended = !string.IsNullOrEmpty(extended_op);

    // int3 is weirdly defined in the doc
    if (is_one_byte && pri_opcd == "CC")
    {
        has_src = false;
        has_dst = false;
    }
    // this mov is treated as opcode extended for some reason...
    if (is_one_byte && pri_opcd == "C7")
    {
        is_extended = false;
    }

    // invalid ops
    if (!is_extended)
    {
        var is_non_64bit = entry.Attribute("mode")?.Value != "e";
        var has_64bit_siblings = entry.Parent!.Elements("entry").Any(e => e.Attribute("mode")?.Value == "e");

        // ignore non-64bit ops if we are not extended and have 64bit siblings
        if (is_non_64bit && has_64bit_siblings)
            return;
    }

    // TODO: they require special entries
    if (is_extended)
        return;

    // no args
    if (!has_src && !has_dst)
    {
        entries.Add(new OpEntry
        {
            category = cat,
            mnemonic = mnemonic,
            primary_opcode = pri_opcode,
            arg_format = "none",
        });
    }
    // opreg, like push/pop
    else if ((isOpReg(src) && !has_dst) || (isOpReg(dst) && !has_src))
    {
        for (var i = 0; i < 8; ++i)
            entries.Add(new OpEntry
            {
                category = cat,
                mnemonic = mnemonic,
                primary_opcode = pri_opcode + i,
                arg_format = src_t == "vq" || dst_t == "vq" ? "opreg64" : "opreg",
            });
    }
    // only immediate
    else if ((isImmediate(src) && !has_dst) || (isImmediate(dst) && !has_src))
    {
        var immtype = has_src ? ImmTypeOf(src_t) : ImmTypeOf(dst_t);
        entries.Add(new OpEntry
        {
            category = cat,
            mnemonic = mnemonic,
            primary_opcode = pri_opcode,
            arg_format = "imm" + immtype,
        });
    }
    // pure ModR/M
    else if (isModR(dst) && isModM(src))
    {
        // if (src_t == "vqp" && dst_t == "vqp")
        entries.Add(new OpEntry
        {
            category = cat,
            mnemonic = mnemonic,
            primary_opcode = pri_opcode,
            arg_format = "modr_modm",
        });
        // TODO: type hints for printing
    }
    else if (isModM(dst) && isModR(src))
    {
        // if (src_t == "vqp" && dst_t == "vqp")
        entries.Add(new OpEntry
        {
            category = cat,
            mnemonic = mnemonic,
            primary_opcode = pri_opcode,
            arg_format = "modm_modr",
        });
        // TODO: type hints for printing
    }
    else if (src == "I" && isModM(dst))
    {
        // if (src_t == "vds" && dst_t == "vqp")
        entries.Add(new OpEntry
        {
            category = cat,
            mnemonic = mnemonic,
            primary_opcode = pri_opcode,
            arg_format = "modm_imm32",
        });
        // TODO: type hints for printing
    }
    else
    {
        System.Console.WriteLine($"TODO: decode entry for {cat} {pri_opcd} {mnemonic} | {dst}:{dst_t} {src}:{src_t}");
    }
}
foreach (var entry in refdoc.Descendants("entry"))
{
    var amode = entry.Attribute("mode");
    if (amode != null && amode.Value != "e")
        continue; // not 64bit mode

    if (entry.Attribute("ring")?.Value == "0")
        continue; // ring 0

    foreach (var syntax in entry.Elements("syntax"))
    {
        var mnem = syntax.Element("mnem")?.Value;
        if (string.IsNullOrEmpty(mnem))
            continue;

        var s = NormalizeMnemonic(mnem);

        // rex prefix
        if (s == "rex" || s.StartsWith("rex."))
            continue;

        // segment / branch prefix
        if (s == "alter" || s == "fs" || s == "gs")
            continue;

        // lock prefix
        if (s == "lock")
            continue;

        ProcessEntry(entry, syntax, s);

        // only process the first valid syntax node per entry
        break;
    }
}
var mnemonics = mnemonics_set.OrderBy(s => s).ToList();
System.Console.WriteLine($"found {mnemonics.Count} mnemonics");
if (mnemonics.Count > 4 * 256 - 2)
{
    System.Console.WriteLine("ERROR: too many mnemonics! 10 bit assumption violated!");
    return;
}

// DEBUG
foreach (var e in entries)
    System.Console.WriteLine($"info_{e.category}.set_op(0x{e.primary_opcode.ToString("X").PadLeft(2, '0')}, mnemonic::{SafeMnemonic(e.mnemonic)}, arg_format::{e.arg_format});");


//
// generate file
//

var hh = @$"#pragma once
// CAUTION: this file is auto-generated. DO NOT MODIFY!

#include <cstdint>

namespace babel::x64
{{

enum class mnemonic : uint16_t 
{{
    _invalid,
    _subresolve,

    {string.Join(",\n    ", mnemonics.Select(s => SafeMnemonic(s)))}
}};
char const* to_string(mnemonic m);

}} // babel::x64

";

var cc = @$"// CAUTION: this file is auto-generated. DO NOT MODIFY!
#include ""x64.hh""
#include ""x64.gen.hh""

static char const* s_mnemonic_names[] = {{
    ""<invalid-mnemonic>"",
    ""<unresolved-mnemonic>"",
    {string.Join("\n    ", mnemonics.Select(s => $@"""{s}"","))}
}};

char const* babel::x64::to_string(mnemonic m)
{{
    if (m > mnemonic::{mnemonics.Last()})
        return ""<unknown-mnemonic>"";
    return s_mnemonic_names[int(m)];
}}

";

// mnemonic to-string

File.WriteAllText(gen_file_cc, cc);
System.Console.WriteLine("written generated source to " + gen_file_cc);
File.WriteAllText(gen_file_hh, hh);
System.Console.WriteLine("written generated source to " + gen_file_hh);
