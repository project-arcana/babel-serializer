// CAUTION: this file is auto-generated. DO NOT MODIFY!
#include "x64.hh"
#include "x64.gen.hh"

static char const* s_mnemonic_names[] = {
    "<invalid-mnemonic>",
    "adc",
    "add",
    "addpd",
    "addps",
    "addsd",
    "addss",
    "addsubpd",
    "addsubps",
    "and",
    "andnpd",
    "andnps",
    "andpd",
    "andps",
    "blendpd",
    "blendps",
    "bsf",
    "bsr",
    "bswap",
    "bt",
    "btc",
    "btr",
    "bts",
    "call",
    "cbw",
    "clc",
    "cld",
    "cli",
    "cmc",
    "cmovb",
    "cmovbe",
    "cmovl",
    "cmovle",
    "cmovnb",
    "cmovnbe",
    "cmovnl",
    "cmovnle",
    "cmovno",
    "cmovnp",
    "cmovns",
    "cmovnz",
    "cmovo",
    "cmovp",
    "cmovs",
    "cmovz",
    "cmp",
    "cmppd",
    "cmpps",
    "cmps",
    "cmpsd",
    "cmpss",
    "cmpxchg",
    "cmpxchg8b",
    "comisd",
    "comiss",
    "cpuid",
    "cvtdq2pd",
    "cvtdq2ps",
    "cvtpd2dq",
    "cvtpd2pi",
    "cvtpd2ps",
    "cvtpi2pd",
    "cvtpi2ps",
    "cvtps2dq",
    "cvtps2pd",
    "cvtps2pi",
    "cvtsd2si",
    "cvtsd2ss",
    "cvtsi2sd",
    "cvtsi2ss",
    "cvtss2sd",
    "cvtss2si",
    "cvttpd2dq",
    "cvttpd2pi",
    "cvttps2dq",
    "cvttps2pi",
    "cvttsd2si",
    "cvttss2si",
    "cwd",
    "dec",
    "div",
    "divpd",
    "divps",
    "divsd",
    "divss",
    "dppd",
    "dpps",
    "enter",
    "extractps",
    "fndisi",
    "fneni",
    "fnsetpm",
    "fxrstor",
    "fxsave",
    "getsec",
    "haddpd",
    "haddps",
    "hint_nop",
    "hsubpd",
    "hsubps",
    "idiv",
    "imul",
    "in",
    "inc",
    "ins",
    "insertps",
    "int",
    "int1",
    "into",
    "iret",
    "ja",
    "jae",
    "jb",
    "jbe",
    "je",
    "jecxz",
    "jg",
    "jge",
    "jl",
    "jle",
    "jmp",
    "jmpe",
    "jne",
    "jno",
    "jns",
    "jo",
    "jpe",
    "jpo",
    "js",
    "lahf",
    "lddqu",
    "lea",
    "leave",
    "lfs",
    "lgs",
    "lods",
    "loop",
    "loopnz",
    "loopz",
    "lss",
    "maskmovdqu",
    "maskmovq",
    "maxpd",
    "maxps",
    "maxsd",
    "maxss",
    "minpd",
    "minps",
    "minsd",
    "minss",
    "mov",
    "movapd",
    "movaps",
    "movd",
    "movddup",
    "movdq2q",
    "movdqa",
    "movdqu",
    "movhpd",
    "movhps",
    "movlpd",
    "movlps",
    "movmskpd",
    "movmskps",
    "movntdq",
    "movnti",
    "movntpd",
    "movntps",
    "movntq",
    "movq",
    "movq2dq",
    "movs",
    "movsd",
    "movshdup",
    "movsldup",
    "movss",
    "movsx",
    "movsxd",
    "movupd",
    "movups",
    "movzx",
    "mpsadbw",
    "mul",
    "mulpd",
    "mulps",
    "mulsd",
    "mulss",
    "neg",
    "nop",
    "not",
    "or",
    "orpd",
    "orps",
    "out",
    "outs",
    "packssdw",
    "packsswb",
    "packuswb",
    "paddb",
    "paddd",
    "paddq",
    "paddsb",
    "paddsw",
    "paddusb",
    "paddusw",
    "paddw",
    "palignr",
    "pand",
    "pandn",
    "pause",
    "pavgb",
    "pavgw",
    "pblendw",
    "pcmpeqb",
    "pcmpeqd",
    "pcmpeqw",
    "pcmpestri",
    "pcmpestrm",
    "pcmpgtb",
    "pcmpgtd",
    "pcmpgtw",
    "pcmpistri",
    "pcmpistrm",
    "pextrb",
    "pextrd",
    "pextrw",
    "pinsrb",
    "pinsrd",
    "pinsrw",
    "pmaddwd",
    "pmaxsw",
    "pmaxub",
    "pminsw",
    "pminub",
    "pmovmskb",
    "pmulhuw",
    "pmulhw",
    "pmullw",
    "pmuludq",
    "pop",
    "popcnt",
    "popf",
    "por",
    "prefetchnta",
    "prefetcht0",
    "prefetcht1",
    "prefetcht2",
    "psadbw",
    "pshufd",
    "pshufhw",
    "pshuflw",
    "pshufw",
    "pslld",
    "pslldq",
    "psllq",
    "psllw",
    "psrad",
    "psraw",
    "psrld",
    "psrldq",
    "psrlq",
    "psrlw",
    "psubb",
    "psubd",
    "psubq",
    "psubsb",
    "psubsw",
    "psubusb",
    "psubusw",
    "psubw",
    "punpckhbw",
    "punpckhdq",
    "punpckhqdq",
    "punpckhwd",
    "punpcklbw",
    "punpckldq",
    "punpcklqdq",
    "punpcklwd",
    "push",
    "pushf",
    "pxor",
    "rcl",
    "rcpps",
    "rcpss",
    "rcr",
    "rdpmc",
    "rdtsc",
    "repnz",
    "repz",
    "retf",
    "retn",
    "rol",
    "ror",
    "roundpd",
    "roundps",
    "roundsd",
    "roundss",
    "rsqrtps",
    "rsqrtss",
    "sahf",
    "sal",
    "sar",
    "sbb",
    "scas",
    "setb",
    "setbe",
    "setl",
    "setle",
    "setnb",
    "setnbe",
    "setnl",
    "setnle",
    "setno",
    "setnp",
    "setns",
    "setnz",
    "seto",
    "setp",
    "sets",
    "setz",
    "shld",
    "shr",
    "shrd",
    "shufpd",
    "shufps",
    "sqrtpd",
    "sqrtps",
    "sqrtsd",
    "sqrtss",
    "stc",
    "std",
    "sti",
    "stos",
    "sub",
    "subpd",
    "subps",
    "subsd",
    "subss",
    "syscall",
    "sysenter",
    "test",
    "ucomisd",
    "ucomiss",
    "ud",
    "ud2",
    "unpckhpd",
    "unpckhps",
    "unpcklpd",
    "unpcklps",
    "xadd",
    "xchg",
    "xlat",
    "xor",
    "xorpd",
    "xorps",
    "xsave",
};

char const* babel::x64::to_string(mnemonic m)
{
    if (m > mnemonic::xsave)
        return "<unknown-mnemonic>";
    return s_mnemonic_names[int(m)];
}

uint16_t const babel::x64::detail::decode_table[789] =
{
    1026, 1026, 1026, 1026, 2050, 6146, 0, 0, 1214, 1214, 1214, 1214, 2238, 6334, 0, 239,  //
    1025, 1025, 1025, 1025, 2049, 6145, 0, 0, 1326, 1326, 1326, 1326, 2350, 6446, 0, 0,  //
    1033, 1033, 1033, 1033, 2057, 6153, 0, 0, 1357, 1357, 1357, 1357, 2381, 6477, 0, 0,  //
    1376, 1376, 1376, 1376, 2400, 6496, 0, 0, 1069, 1069, 1069, 1069, 2093, 6189, 0, 0,  //
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
    278, 278, 278, 278, 278, 278, 278, 278, 239, 239, 239, 239, 239, 239, 239, 239,  //
    0, 0, 0, 1201, 0, 0, 0, 0, 6422, 7269, 2326, 3173, 104, 0, 194, 0,  //
    2173, 2171, 2160, 2159, 2162, 2170, 2161, 2158, 2176, 2172, 2174, 2175, 2166, 2165, 2167, 2164,  //
    34079, 34087, 0, 34095, 1364, 1364, 1374, 1374, 1174, 1174, 1174, 1174, 1174, 1155, 1174, 1263,  //
    33002, 350, 350, 350, 350, 350, 350, 350, 24, 78, 0, 0, 279, 241, 299, 129,  //
    2198, 8342, 2198, 8342, 171, 171, 48, 48, 2388, 6484, 332, 332, 135, 135, 303, 303,  //
    2198, 2198, 2198, 2198, 2198, 2198, 2198, 2198, 8342, 8342, 8342, 8342, 8342, 8342, 8342, 8342,  //
    34103, 34111, 4386, 290, 0, 0, 3222, 7318, 2135, 132, 4385, 289, 106, 2154, 108, 109,  //
    34119, 34127, 34135, 34143, 0, 0, 0, 351, 0, 0, 0, 0, 0, 0, 0, 0,  //
    2185, 2186, 2184, 2163, 2150, 2150, 2241, 2241, 6167, 6264, 0, 2168, 102, 102, 193, 193,  //
    0, 107, 287, 288, 0, 28, 34149, 34157, 25, 329, 27, 331, 26, 330, 33952, 33931,  //
    1145, 0, 0, 0, 0, 338, 0, 0, 0, 0, 0, 344, 0, 1212, 0, 0,  //
    32914, 32918, 33140, 32923, 32924, 32925, 33147, 32927, 34185, 1121, 1121, 1121, 1121, 1121, 1121, 33970,  //
    0, 0, 0, 0, 0, 0, 0, 0, 32930, 32934, 32943, 32940, 32952, 32955, 32941, 32950,  //
    0, 286, 0, 285, 339, 0, 0, 94, 0, 0, 34816, 0, 0, 0, 0, 0,  //
    1065, 1061, 1053, 1057, 1068, 1064, 1054, 1058, 1067, 1063, 1066, 1062, 1055, 1059, 1056, 1060,  //
    32959, 32964, 33051, 33175, 32962, 32973, 32974, 32975, 32976, 32985, 32988, 32992, 32997, 33016, 33025, 33028,  //
    1298, 1301, 1299, 1220, 1242, 1244, 1243, 1221, 1294, 1297, 1295, 1219, 1300, 1296, 1177, 33032,  //
    33037, 34188, 34189, 33930, 1237, 1239, 1238, 0, 0, 0, 0, 0, 33006, 33142, 1177, 33041,  //
    6269, 6267, 6256, 6255, 6258, 6266, 6257, 6254, 6272, 6268, 6270, 6271, 6262, 6261, 6263, 6260,  //
    1340, 1336, 1328, 1332, 1343, 1339, 1329, 1333, 1342, 1338, 1341, 1337, 1330, 1334, 1331, 1335,  //
    278, 239, 55, 1043, 3392, 1344, 0, 0, 278, 239, 0, 1046, 3394, 1346, 34196, 1125,  //
    1075, 1075, 1163, 1045, 1157, 1158, 1204, 1204, 33178, 1367, 34176, 1044, 1040, 1041, 1200, 1200,  //
    1373, 1373, 33046, 1189, 3300, 3297, 32995, 1076, 18, 18, 18, 18, 18, 18, 18, 18,  //
    33143, 1285, 1282, 1284, 1224, 1261, 33144, 1258, 1291, 1292, 1257, 1231, 1227, 1228, 1255, 1232,  //
    1234, 1281, 1280, 1235, 1259, 1260, 33152, 33042, 1289, 1290, 1256, 1266, 1225, 1226, 1254, 1304,  //
    1154, 1279, 1276, 1278, 1262, 1253, 1271, 33044, 1286, 1293, 1287, 1288, 1222, 1229, 1223, 0,  //
    3288, 3289, 3293, 3294, 3332, 1047, 0, 1144, 3326, 1302, 1203, 1196, 1199, 3331, 1203, 1196,  //
    1199, 3325, 1202, 1185, 1372, 1370, 1202, 1183, 1127, 1103, 1176, 1184, 1371, 1369, 1176, 1182,  //
    1109, 1110, 1175, 3253, 1191, 1366, 1175, 1086, 1092, 1093, 1212, 1121, 1190, 1365, 1078, 1085,  //
    1099, 1100, 1101, 1089, 1090, 1095, 1077, 1187, 1097, 0, 1037, 1083, 1350, 1351, 1352, 1186,  //
    3177, 3298, 1036, 3299, 1349, 1035, 1216, 1378, 1028, 1029, 1030, 0, 0, 1034, 1215, 1377,  //
    1027, 1208, 1209, 1210, 1088, 1091, 1094, 0, 1081, 1207, 1098, 3396, 1084, 1359, 1360, 1361,  //
    1087, 0, 350, 3395, 209, 1358, 1230, 1120, 3365, 3366, 3367, 3368, 3086, 3087, 1119, 3284,  //
    1171, 1172, 1173, 0, 3297, 3295, 3160, 3296, 1170, 1106, 1107, 1108, 1167, 1168, 1169, 0,  //
    1193, 1105, 1181, 0, 1166, 3323, 3322, 3321, 1180, 1193, 1192, 1181, 1165, 3320, 3119, 3121,  //
    3122, 1180, 1188, 1321, 1164, 1322, 3118, 3074, 3262, 3073, 3374, 3081, 3405, 3424, 3117, 7170,  //
    7358, 7169, 7470, 7177, 7501, 7520, 7213, 3074, 3262, 3073, 3374, 3081, 3405, 3424, 3117, 3363,  //
    3364, 3353, 3356, 3372, 3393, 0, 3373, 3363, 3364, 3353, 3356, 3372, 3393, 0, 3373, 1315,  //
    1316, 1305, 1308, 1324, 1345, 0, 1325, 1315, 1316, 1305, 1308, 1324, 1345, 0, 1325, 1315,  //
    1316, 1305, 1308, 1324, 1345, 0, 1325, 1315, 1316, 1305, 1308, 1324, 1345, 3412, 1325, 1213,  //
    1211, 1206, 1125, 1104, 1124, 9556, 0, 1213, 1211, 1206, 1125, 1104, 1124, 1178, 1198, 1123,  //
    1032, 1179, 1194, 0, 1184, 1197, 1122, 1031, 1193, 1082, 1080, 1182, 3091, 3094, 3093, 3092,  //
    1096, 1267, 1268, 1269, 1270, 1121, 3333, 3330, 3329, 3328, 3327, 3324, 1117, 1116, 0, 1306,  //
    1379, 1307, 121, 0, 1264, 
};
