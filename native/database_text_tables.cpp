// DATABASE.CPP's text block, part 4: attribute names (short and long forms) and language names
// (database_text.hpp).
#include "database_text.hpp"
#include "database_text_detail.hpp"
#include <array>

namespace cm::aot::database {
using namespace text;

namespace {
constexpr std::uint32_t format_entry = 0x10727890;  // localization_format's owner: (buffer, format, argument)
constexpr std::uint32_t empty_argument = 0xd0;      // the empty pool text passed as the %s argument

// A text of a table: the pool offset of its format, whether the original loads the empty argument
// into r5 for it, and the address after the original's call.
struct Text {
    std::uint32_t offset = 0;
    bool argument = false;
    std::uint32_t back = 0;
};
// One case of an attribute table: the text for a nonzero flag (long form), then for a zero flag.
struct Pair {
    Text long_form, short_form;
};

// Common body of 0x100d2720 and 0x100d15f0: the value is 1..`last`, the buffer non-null and the
// limit (16-bit) at least 100; the buffer gets the text of the case, whose index is the value.
struct Attribute {
    std::uint32_t entry, first_assertion, first_line;
    std::int32_t value_max;
    std::int32_t table_slot;
};
std::uint32_t attribute_text(Cpu& c, const Attribute& t, std::int8_t value, std::uint32_t buffer, std::uint32_t limit,
                             std::uint32_t flag, const Pair* pairs) {
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, t.entry, 0xa0, {.first_gpr = 29});
    const auto pool = c.toc(pool_slot);
    c.set(31, pool);
    const auto success_flag = c.toc(-0x7bac);
    c.set(30, success_flag);
    const auto message = c.toc(-0x7ba8);
    c.set(29, message);
    const auto fail = [&](std::uint32_t at, std::int32_t name_at, std::int32_t extension_at, std::uint32_t line) {
        return std::uint32_t(assertion_failure(c, frame, pool, at, name_at, extension_at, line));
    };
    const auto v = extsb(std::uint32_t(std::int32_t(value)));
    c.set(0, std::uint32_t(v));
    bool valid = compare(c, v, 1) >= 0;
    if (valid) {
        c.set(0, std::uint32_t(v));
        valid = compare(c, v, t.value_max) <= 0;
    }
    if (valid) valid = compare_logical(c, buffer) != 0;
    if (valid) {
        c.set(0, std::uint32_t(extsh(limit)));
        valid = compare(c, extsh(limit), 0x64) >= 0;
    }
    if (!valid) return fail(t.first_assertion, 0x68, 0x40, t.first_line);
    c.set(0, std::uint32_t(v));
    c.set(3, 0);
    compare_logical(c, std::uint32_t(v), std::uint32_t(t.value_max));
    c.put8(buffer, 0);
    // v is 1..value_max here, so the original's two further assertion paths (above the maximum, and the
    // table's entry 0) are unreachable; the table is still read for its jump.
    const auto table = c.toc(t.table_slot);
    c.set(0, std::uint32_t(v) * 4);
    const auto target = c.u32(table + std::uint32_t(v) * 4);
    c.set(3, target);
    c.s.wr<4>(guest::reg::ctr, target);
    c.set(0, std::uint32_t(extsb(flag)));
    const auto long_form = compare(c, extsb(flag)) != 0;
    const auto& text = long_form ? pairs[v].long_form : pairs[v].short_form;
    c.set({{3, buffer}, {4, pool + text.offset}});
    if (text.argument) c.set(5, pool + empty_argument);
    c.call_in_place(format_entry, text.back);
    c.set(3, 1);
    return 1;
}
// Cases 1-12 of 0x100d2720 (the index is the month's attribute number; index 0 asserts).
constexpr std::array<Pair, 13> short_attributes{{
    {},  // 0: outside
    {{0x1ffa, false, 0x100d2804}, {0x1ffe, false, 0x100d2814}},  // 1 -> 100d27f0
    {{0x200b, false, 0x100d2830}, {0x200f, false, 0x100d2840}},  // 2 -> 100d281c
    {{0x2024, false, 0x100d285c}, {0x2028, false, 0x100d286c}},  // 3 -> 100d2848
    {{0x1e23, false, 0x100d2888}, {0x1e27, false, 0x100d2898}},  // 4 -> 100d2874
    {{0x2042, false, 0x100d28b4}, {0x2046, false, 0x100d28c4}},  // 5 -> 100d28a0
    {{0x205d, false, 0x100d28e0}, {0x2061, false, 0x100d28f0}},  // 6 -> 100d28cc
    {{0x207a, false, 0x100d290c}, {0x207e, false, 0x100d291c}},  // 7 -> 100d28f8
    {{0x2092, true, 0x100d293c}, {0x20b5, false, 0x100d294c}},  // 8 -> 100d2924
    {{0x20c4, false, 0x100d2968}, {0x20c8, false, 0x100d2978}},  // 9 -> 100d2954
    {{0x20d3, false, 0x100d2994}, {0x20d7, false, 0x100d29a4}},  // 10 -> 100d2980
    {{0x20e5, false, 0x100d29c0}, {0x20e9, false, 0x100d29d0}},  // 11 -> 100d29ac
    {{0x20fc, false, 0x100d29ec}, {0x2100, false, 0x100d29fc}},  // 12 -> 100d29d8
}};
constexpr std::array<Pair, 32> long_attributes{{
    {},  // 0: outside
    {{0x1d3d, false, 0x100d16d4}, {0x1d41, false, 0x100d16e4}},  // 1 -> 100d16c0
    {{0x1da0, false, 0x100d1760}, {0x1da4, false, 0x100d1770}},  // 2 -> 100d174c
    {{0x1daf, false, 0x100d178c}, {0x1db3, false, 0x100d179c}},  // 3 -> 100d1778
    {{0x1dbb, false, 0x100d17b8}, {0x1dbf, false, 0x100d17c8}},  // 4 -> 100d17a4
    {{0x1dcc, false, 0x100d17e4}, {0x1dd0, true, 0x100d17f8}},  // 5 -> 100d17d0
    {{0x1dfb, false, 0x100d1814}, {0x1dff, false, 0x100d1824}},  // 6 -> 100d1800
    {{0x1e07, false, 0x100d1840}, {0x1e0b, false, 0x100d1850}},  // 7 -> 100d182c
    {{0x1e16, false, 0x100d186c}, {0x1e1a, false, 0x100d187c}},  // 8 -> 100d1858
    {{0x1d4e, true, 0x100d1704}, {0x1d6c, false, 0x100d1714}},  // 9 -> 100d16ec
    {{0x1e23, false, 0x100d1898}, {0x1e27, false, 0x100d18a8}},  // 10 -> 100d1884
    {{0x1e35, false, 0x100d18c4}, {0x1e39, false, 0x100d18d4}},  // 11 -> 100d18b0
    {{0x1f83, true, 0x100d1b18}, {0x1fa1, false, 0x100d1b28}},  // 12 -> 100d1b00
    {{0x1e43, false, 0x100d18f0}, {0x1e47, false, 0x100d1900}},  // 13 -> 100d18dc
    {{0x1e4d, false, 0x100d191c}, {0x1e51, false, 0x100d192c}},  // 14 -> 100d1908
    {{0x1e5a, false, 0x100d1948}, {0x1e5e, false, 0x100d1958}},  // 15 -> 100d1934
    {{0x1e66, false, 0x100d1974}, {0x1e6a, false, 0x100d1984}},  // 16 -> 100d1960
    {{0x1e74, false, 0x100d19a0}, {0x1e78, false, 0x100d19b0}},  // 17 -> 100d198c
    {{0x1d76, true, 0x100d1734}, {0x1d95, false, 0x100d1744}},  // 18 -> 100d171c
    {{0x1e80, true, 0x100d19d0}, {0x1eaf, true, 0x100d19e4}},  // 19 -> 100d19b8
    {{0x1ed7, true, 0x100d1a04}, {0x1f01, false, 0x100d1a14}},  // 20 -> 100d19ec
    {{0x1f0e, false, 0x100d1a30}, {0x1f12, false, 0x100d1a40}},  // 21 -> 100d1a1c
    {{0x1f17, false, 0x100d1a5c}, {0x1f1b, false, 0x100d1a6c}},  // 22 -> 100d1a48
    {{0x1f23, false, 0x100d1a88}, {0x1f27, false, 0x100d1a98}},  // 23 -> 100d1a74
    {{0x1f33, true, 0x100d1ab8}, {0x1f50, false, 0x100d1ac8}},  // 24 -> 100d1aa0
    {{0x1f59, true, 0x100d1ae8}, {0x1f78, false, 0x100d1af8}},  // 25 -> 100d1ad0
    {{0x1fab, false, 0x100d1b44}, {0x1faf, false, 0x100d1b54}},  // 26 -> 100d1b30
    {{0x1fb7, false, 0x100d1b70}, {0x1fbb, false, 0x100d1b80}},  // 27 -> 100d1b5c
    {{0x1fc4, false, 0x100d1b9c}, {0x1fc8, false, 0x100d1bac}},  // 28 -> 100d1b88
    {{0x1fd1, false, 0x100d1bc8}, {0x1fd5, false, 0x100d1bd8}},  // 29 -> 100d1bb4
    {{0x1fde, false, 0x100d1bf4}, {0x1fe2, false, 0x100d1c04}},  // 30 -> 100d1be0
    {{0x1fec, false, 0x100d1c20}, {0x1ff0, false, 0x100d1c30}},  // 31 -> 100d1c0c
}};
// 0x100ca140's cases: the format offset and the address after the call (index 0 is the default, shared by every index above 0x40).
constexpr std::array<Text, 65> languages{{
{0x18e9, true, 0x100ca688}, {0x10ca, true, 0x100ca188}, {0x10ec, true, 0x100ca19c}, {0x110c, true, 0x100ca1b0}, {0x112f, true, 0x100ca1c4}, {0x1150, true, 0x100ca1d8}, {0x1172, true, 0x100ca1ec}, {0x1191, true, 0x100ca200}, {0x11b0, true, 0x100ca214}, {0x11d1, true, 0x100ca228}, {0x11f2, true, 0x100ca23c}, {0x1212, true, 0x100ca250}, {0x1232, true, 0x100ca264}, {0x1251, true, 0x100ca278}, {0x1271, true, 0x100ca28c}, {0x1294, true, 0x100ca2a0}, {0x12b5, true, 0x100ca2b4}, {0x12d7, true, 0x100ca2c8}, {0x12f7, true, 0x100ca2dc}, {0x1317, true, 0x100ca2f0}, {0x133b, true, 0x100ca304}, {0x135d, true, 0x100ca318}, {0x137e, true, 0x100ca32c}, {0x13a1, true, 0x100ca340}, {0x13c2, true, 0x100ca354}, {0x13e3, true, 0x100ca368}, {0x1404, true, 0x100ca37c}, {0x1425, true, 0x100ca390}, {0x1444, true, 0x100ca3a4}, {0x1465, true, 0x100ca3b8}, {0x14c8, true, 0x100ca3f4}, {0x14a5, true, 0x100ca3e0}, {0x1485, true, 0x100ca3cc}, {0x14e9, true, 0x100ca408}, {0x10ca, true, 0x100ca41c}, {0x150c, true, 0x100ca430}, {0x152d, true, 0x100ca444}, {0x154f, true, 0x100ca458}, {0x156e, true, 0x100ca46c}, {0x158e, true, 0x100ca480}, {0x15b0, true, 0x100ca494}, {0x15d2, true, 0x100ca4a8}, {0x15f1, true, 0x100ca4bc}, {0x1613, true, 0x100ca4d0}, {0x1632, true, 0x100ca4e4}, {0x1652, true, 0x100ca4f8}, {0x166f, true, 0x100ca50c}, {0x1690, true, 0x100ca520}, {0x16b6, true, 0x100ca534}, {0x16da, true, 0x100ca548}, {0x16fe, true, 0x100ca55c}, {0x1720, true, 0x100ca570}, {0x1741, true, 0x100ca584}, {0x1764, true, 0x100ca598}, {0x1787, true, 0x100ca5ac}, {0x17a7, true, 0x100ca5c0}, {0x17c8, true, 0x100ca5d4}, {0x17e7, true, 0x100ca5e8}, {0x1806, true, 0x100ca5fc}, {0x1824, true, 0x100ca610}, {0x1843, true, 0x100ca624}, {0x1867, true, 0x100ca638}, {0x1887, true, 0x100ca64c}, {0x18a8, true, 0x100ca660}, {0x18cb, true, 0x100ca674}
}};

constexpr Attribute short_table{0x100d2720, 0x100d2784, 0x48f9, 12, -0x568c};
constexpr Attribute long_table{0x100d15f0, 0x100d1654, 0x467e, 31, -0x5684};
}

// 0x100d2720 and 0x100d15f0: the name of attribute 1-12 or 1-31 (flag nonzero: the long form with
// its comment key, zero: the short form), copied into `buffer` through the localization formatter.
bool attribute_text_12(Cpu& c, std::int8_t value, std::uint32_t buffer, std::uint32_t limit, std::uint32_t flag) {
    return attribute_text(c, short_table, value, buffer, limit, flag, short_attributes.data()) != 0;
}
bool attribute_text_31(Cpu& c, std::int8_t value, std::uint32_t buffer, std::uint32_t limit, std::uint32_t flag) {
    return attribute_text(c, long_table, value, buffer, limit, flag, long_attributes.data()) != 0;
}

// 0x100ca140: a language's name (index 0-64; anything else is the "Unknown" text of index 0)
// formatted into the shared buffer (TOC -0x5674) with the empty argument. Answers the buffer.
std::uint32_t language_name(Cpu& c, std::int8_t index) {
    constexpr std::uint32_t entry = 0x100ca140;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 31});
    const auto buffer = c.toc(-0x5674);
    c.set(31, buffer);
    const auto id = std::uint32_t(extsb(std::uint32_t(std::int32_t(index))));
    c.set(0, id);
    const auto pool = c.toc(pool_slot);
    c.set(5, pool);
    unsigned selected = 0;
    if (compare_logical(c, id, 0x40) > 0) {
        selected = 0;
    } else {
        const auto table = c.toc(-0x5678);
        c.set(0, id * 4);
        const auto target = c.u32(table + id * 4);
        c.set(3, target);
        c.s.wr<4>(guest::reg::ctr, target);
        selected = id;
    }
    const auto& text = languages[selected];
    c.set({{3, buffer}, {4, pool + text.offset}, {5, pool + empty_argument}});
    c.call_in_place(format_entry, text.back);
    return guest::result(c, buffer);
}
}
