
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
        if (s == "rex" || s.StartsWith("rex."))
            continue;

        mnemonics_set.Add(s);
    }
}
var mnemonics = mnemonics_set.OrderBy(s => s).ToList();
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
System.Console.WriteLine($"found {mnemonics.Count} mnemonics");
if (mnemonics.Count > 4 * 256 - 2)
{
    System.Console.WriteLine("ERROR: too many mnemonics! 10 bit assumption violated!");
    return;
}

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
