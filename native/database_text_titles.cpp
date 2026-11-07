// DATABASE.CPP's text block, part 3: the job titles and the position names (database_text.hpp).
#include "database_text.hpp"
#include "database_text_detail.hpp"
#include <array>

namespace cm::aot::database {
using namespace text;

namespace {
constexpr std::uint32_t text_copy = 0x10727890, text_kind = 0x10727880;  // (buffer, format, argument) and its follow-up
constexpr std::int32_t buffer_slot = -0x5660, title_buffer_slot = -0x5658;
constexpr std::uint32_t argument_offset = 0xd0;  // the pool text passed as third argument

// One case of 0x100c6bf0: the four texts (pool offsets) chosen by the two flags, in the order
// flag r4 set and r5 set, r4 set and r5 clear, r4 clear and r5 set, both clear, with whether the
// original loads the third argument into r5 for that call and the address after its call; and,
// for each r4 branch, the follow-up that reads the kind through 0x10727880 (address of its
// null test, 0 when the case has none) and the value it stores for kind 4.
struct Copy {
    std::uint32_t text;
    bool argument;
    std::uint32_t back;
};
struct Follow {
    std::uint32_t at;
    std::uint32_t value;
};
struct Position {
    std::array<Copy, 4> copies;
    std::array<Follow, 2> follows;
};
constexpr std::array<Position, 18> positions{{
    {{{{0x989, true, 0x100c6c50}, {0x9af, true, 0x100c6c64}, {0x9d5, false, 0x100c6c9c}, {0x9e0, false, 0x100c6cac}}}, {{{0x100c6c64, 1}, {0x100c6cac, 1}}}},  // 0
    {{{{0x9eb, true, 0x100c6cf0}, {0xa0f, true, 0x100c6d04}, {0xa33, false, 0x100c6d3c}, {0xa3c, false, 0x100c6d4c}}}, {{{0x100c6d04, 1}, {0x100c6d4c, 1}}}},  // 1
    {{{{0xa45, true, 0x100c6d90}, {0xa6d, true, 0x100c6da4}, {0xa95, false, 0x100c6ddc}, {0xaa7, false, 0x100c6dec}}}, {{{0x100c6da4, 4}, {0x100c6dec, 4}}}},  // 2
    {{{{0xcef, true, 0x100c71f0}, {0xd16, true, 0x100c7204}, {0xd3d, false, 0x100c723c}, {0xd4d, false, 0x100c724c}}}, {{{0x100c7204, 1}, {0x100c724c, 1}}}},  // 3
    {{{{0xab9, true, 0x100c6e30}, {0xae1, true, 0x100c6e44}, {0xb09, false, 0x100c6e7c}, {0xb1e, false, 0x100c6e8c}}}, {{{0x100c6e44, 1}, {0x100c6e8c, 1}}}},  // 4
    {{{{0xb33, true, 0x100c6ed0}, {0xb56, true, 0x100c6ee4}, {0x962, false, 0x100c6f1c}, {0xb79, false, 0x100c6f2c}}}, {{{0x100c6ee4, 4}, {0x100c6f2c, 4}}}},  // 5
    {{{{0xb81, true, 0x100c6f70}, {0xba8, true, 0x100c6f84}, {0x96a, false, 0x100c6fbc}, {0xbcf, false, 0x100c6fcc}}}, {{{0x100c6f84, 4}, {0x100c6fcc, 4}}}},  // 6
    {{{{0xbe1, true, 0x100c7010}, {0xc08, true, 0x100c7024}, {0xc2f, false, 0x100c705c}, {0xc44, false, 0x100c706c}}}, {{{0x100c7024, 4}, {0x100c706c, 4}}}},  // 7
    {{{{0xc59, true, 0x100c70b0}, {0xc7a, true, 0x100c70c4}, {0x95c, false, 0x100c70fc}, {0xc9b, false, 0x100c710c}}}, {{{0x100c70c4, 1}, {0x100c710c, 1}}}},  // 8
    {{{{0xca1, true, 0x100c7150}, {0xcc2, true, 0x100c7164}, {0xce3, false, 0x100c719c}, {0xce9, false, 0x100c71ac}}}, {{{0x100c7164, 4}, {0x100c71ac, 4}}}},  // 9
    {{{{0xd5d, true, 0x100c7290}, {0xd7f, true, 0x100c72a4}, {0xda1, false, 0x100c72dc}, {0xda8, false, 0x100c72ec}}}, {{{0x100c72a4, 1}, {0x100c72ec, 1}}}},  // 10
    {{{{0xdaf, true, 0x100c7330}, {0xdd5, true, 0x100c7344}, {0xdfb, true, 0x100c7380}, {0xe1b, true, 0x100c7394}}}, {{{0x100c7344, 1}, {0x100c7394, 1}}}},  // 11
    {{{{0xe3b, true, 0x100c73d8}, {0xe61, true, 0x100c73ec}, {0xe87, false, 0x100c7424}, {0xe96, false, 0x100c7434}}}, {{{0x100c73ec, 1}, {0x100c7434, 1}}}},  // 12
    {{{{0xea5, true, 0x100c7478}, {0xecf, true, 0x100c748c}, {0xef9, false, 0x100c74c4}, {0xf12, false, 0x100c74d4}}}, {{{0x100c748c, 1}, {0x100c74d4, 1}}}},  // 13
    {},  // 14: the default
    {{{{0xf2b, true, 0x100c7518}, {0xf4f, true, 0x100c752c}, {0xf73, false, 0x100c7564}, {0xf80, false, 0x100c7574}}}, {{{0x100c752c, 1}, {0x100c7574, 1}}}},  // 15
    {{{{0xf8d, true, 0x100c75b8}, {0xfaf, true, 0x100c75cc}, {0xfd1, false, 0x100c75e4}, {0xfe0, false, 0x100c75f4}}}, {{{0x0, 0}, {0x0, 0}}}},  // 16
    {{{{0xfef, true, 0x100c7618}, {0x1011, true, 0x100c762c}, {0x1033, false, 0x100c7664}, {0x1033, false, 0x100c7674}}}, {{{0x100c762c, 1}, {0x100c7674, 1}}}},  // 17
}};

// A follow-up: with an output byte, ask for the kind (0x10727880, no arguments: r3 is
// whatever the copy left) and store `value` there when it is 4.
void follow_up(Cpu& c, std::uint32_t out, const Follow& follow) {
    if (follow.at == 0) return;
    if (compare_logical(c, out) == 0) return;
    c.call(text_kind, follow.at + 0xc);
    c.set(0, std::uint32_t(extsb(c.r(3))));
    if (compare(c, extsb(c.r(3)), 4) != 0) return;
    c.set(0, follow.value);
    c.put8(out, follow.value);
}
}

// 0x100c6af0: the title of a staff role (5-8 and 12-15) copied to the shared title buffer;
// kind 4 (as 0x10727880 answers) stores 1 (coach) or 4 (manager, assistant manager) at `out`.
// Other roles clear the buffer's first byte. Answers the buffer.
std::uint32_t job_title(Cpu& c, std::int8_t role, std::uint32_t out) {
    constexpr std::uint32_t entry = 0x100c6af0;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 30});
    const auto buffer = c.toc(title_buffer_slot);
    const auto pool = c.toc(pool_slot);
    c.set(3, std::uint32_t(std::int32_t(role)));
    c.set(31, buffer);
    c.set(30, out);
    c.set(5, pool);
    const auto index = std::uint32_t(std::int32_t(role)) - 5;
    c.set(0, index);
    if (compare_logical(c, index, 0xa) > 0) {
        c.set(0, 0);
        c.put8(buffer, 0);
        return guest::result(c, buffer);
    }
    const auto table = c.toc(-0x565c);
    c.set(0, index * 4);
    const auto target = c.u32(table + index * 4);
    c.set(3, target);
    c.s.wr<4>(guest::reg::ctr, target);
    // Cases: 0 and 7 manager, 1 and 8 assistant manager, 3 and 10 coach; the rest default.
    struct Title {
        std::uint32_t text, back, follow, value;
    };
    static constexpr Title coach{0x95c, 0x100c6b3c + 4, 0x100c6b40, 1}, manager{0x962, 0x100c6b6c + 4, 0x100c6b70, 4},
        assistant{0x96a, 0x100c6b9c + 4, 0x100c6ba0, 4};
    const Title* title = nullptr;
    switch (index) {
        case 0: case 7: title = &manager; break;
        case 1: case 8: title = &assistant; break;
        case 3: case 10: title = &coach; break;
        default: break;
    }
    if (!title) {
        c.set(0, 0);
        c.put8(buffer, 0);
        return guest::result(c, buffer);
    }
    c.set({{3, buffer}, {4, pool + title->text}});
    c.call_in_place(text_copy, title->back);
    follow_up(c, out, {title->follow, title->value});
    return guest::result(c, buffer);
}

// 0x100c6bf0: a position's name (index 0-17: the sweeper through the attacker in the order
// of the game's tables; inferred) in the shared buffer: `plural` (r5, a byte) and `long_form`
// (r4, a byte) choose among four texts, the long form passing a pool text as an argument.
// Optionally stores a kind into `out`. Answers the buffer (0 for an index out of range).
std::uint32_t position_name(Cpu& c, std::int8_t index, std::uint32_t long_form, std::uint32_t plural, std::uint32_t out) {
    constexpr std::uint32_t entry = 0x100c6bf0;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 30});
    const auto pool = c.toc(pool_slot);
    c.set(7, pool);
    c.set(31, out);
    const auto buffer = c.toc(buffer_slot);
    c.set(30, buffer);
    const auto id = std::uint32_t(extsb(std::uint32_t(index)));
    c.set(0, id);
    if (compare_logical(c, id, 0x11) > 0) return guest::result(c, 0);
    const auto table = c.toc(-0x5664);
    c.set(0, id * 4);
    const auto target = c.u32(table + id * 4);
    c.set(3, target);
    c.s.wr<4>(guest::reg::ctr, target);
    if (id == 14) return guest::result(c, 0);  // the table's default entry
    const auto& entry_case = positions[id];
    unsigned choice;
    c.set(0, std::uint32_t(extsb(long_form)));
    if (compare(c, extsb(long_form)) != 0) {
        c.set(0, std::uint32_t(extsb(plural)));
        choice = compare(c, extsb(plural)) != 0 ? 0 : 1;
    } else {
        c.set(0, std::uint32_t(extsb(plural)));
        choice = compare(c, extsb(plural)) != 0 ? 2 : 3;
    }
    const auto& copy = entry_case.copies[choice];
    c.set({{3, buffer}, {4, pool + copy.text}});
    if (copy.argument) c.set(5, pool + argument_offset);
    c.call_in_place(text_copy, copy.back);
    follow_up(c, out, entry_case.follows[choice / 2]);
    return guest::result(c, buffer);
}

namespace {
// One text of 0x100d6300: its two pool offsets (strcpy for the unreachable kind-10 case, the
// formatter otherwise), the addresses after the original's calls, and the codes stored in r29 for
// the kinds 2, 5 and 4 that 0x10727880 reports.
struct LabelText {
    std::uint32_t copy, copy_back, format, format_back;
    std::uint32_t kind2, kind5, kind4;
};
constexpr std::array<LabelText, 15> label_texts{{
    {0x213d, 0x100d63cc, 0x2147, 0x100d63dc, 2, 1, 1},
    {0x2152, 0x100d6444, 0x215a, 0x100d6454, 2, 1, 1},
    {0x2162, 0x100d64d4, 0x216d, 0x100d64e4, 2, 1, 1},
    {0x217e, 0x100d654c, 0x218a, 0x100d655c, 5, 1, 1},
    {0x2194, 0x100d65bc, 0x21a0, 0x100d65cc, 2, 1, 1},
    {0x21a9, 0x100d6644, 0x21bf, 0x100d6654, 5, 1, 1},
    {0x21d3, 0x100d66bc, 0x21ea, 0x100d66cc, 2, 1, 1},
    {0x21ff, 0x100d6734, 0x220c, 0x100d6744, 2, 1, 1},
    {0x2217, 0x100d67bc, 0x222e, 0x100d67cc, 2, 1, 1},
    {0x2243, 0x100d683c, 0x224a, 0x100d684c, 5, 1, 4},
    {0x2251, 0x100d68b4, 0x225f, 0x100d68c4, 5, 1, 4},
    {0x226b, 0x100d6924, 0x2277, 0x100d6934, 5, 1, 4},
    {0x2284, 0x100d69a4, 0x228f, 0x100d69b4, 5, 1, 4},
    {0x2297, 0x100d6a1c, 0x22a3, 0x100d6a2c, 2, 1, 1},
    {0x22b2, 0x100d6a90, 0x22bb, 0x100d6aa0, 2, 1, 1}}};
}

// 0x100d6300: the label of a position mask (as 0x100db1b0 builds it) in the buffer, as a
// list of texts chosen by the mask's bits, and a code (0, 1, 2, 4 or 5) from the kind byte
// 0x10727880 reports. `limit` (r5) must be at least 30. The strcpy variant of each text needs
// r27 == 1, which the original zeroes at entry, so it never runs.
std::uint32_t position_mask_text(Cpu& c, std::uint32_t mask, std::uint32_t buffer, std::uint32_t limit) {
    constexpr std::uint32_t entry = 0x100d6300;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x80, {.first_gpr = 25});
    const auto message = c.toc(-0x7ba8), pool = c.toc(pool_slot);
    c.set(28, message);
    c.set(25, mask);
    c.set(31, pool);
    c.set(26, buffer);
    c.set(30, limit);
    c.set(29, 0);
    c.set(27, 0);
    c.call(text_kind, entry + 0x30);
    compare(c, std::int32_t(limit), 0x1e);
    const auto kind = c.r(3);
    c.set(30, kind);
    if (std::int32_t(limit) < 0x1e) {
        assertion_failure(c, frame, pool, 0x100d6350, 0x40, 0x38, 0x5246, true);
        return 0;
    }
    const auto bit = [&](std::uint32_t m) {  // rlwinm. r0,r25,0,b,b
        c.set(0, mask & m);
        return compare(c, std::int32_t(mask & m)) != 0;
    };
    const auto signed_compare = [&](std::uint32_t value, std::int32_t with) {
        c.set(0, std::uint32_t(extsb(value)));
        return compare(c, extsb(value), with);
    };
    const auto add_one = [&] { c.set(28, c.r(28) + 1); };
    const auto emit = [&](const LabelText& t) {
        if (signed_compare(kind, 0xa) == 0 && signed_compare(c.r(27), 1) == 0) {
            c.set({{3, buffer}, {4, pool + t.copy}});
            c.call(strcpy_entry, t.copy_back);
        } else {
            c.set({{3, buffer}, {4, pool + t.format}});
            c.call_in_place(text_copy, t.format_back);
        }
        if (signed_compare(kind, 2) == 0) c.set(29, t.kind2);
        else if (signed_compare(kind, 5) == 0) c.set(29, t.kind5);
        else if (signed_compare(kind, 4) == 0) c.set(29, t.kind4);
    };
    const auto& t = label_texts;
    const bool first = bit(0x1);
    c.set(28, 0);
    if (first) { add_one(); emit(t[0]); }
    if (bit(0x2)) { add_one(); emit(t[1]); }
    if (bit(0x4)) {
        const bool nine = bit(0x200);
        add_one();
        bool done = false;
        if (nine && !bit(0x80) && !bit(0x800)) {
            emit(t[2]);
            done = true;
        }
        if (!done) {
            if (bit(0x200)) emit(t[4]);
            else emit(t[3]);
        }
    }
    if (bit(0x8)) {
        const bool four = bit(0x4);
        add_one();
        if (!four && !bit(0x200)) emit(t[5]);
        else if (bit(0x200)) emit(t[6]);
    }
    if (bit(0x10)) { add_one(); emit(t[7]); }
    if (bit(0x20) && signed_compare(c.r(28), 0) == 0) {
        const bool nine = bit(0x200);
        add_one();
        if (nine) emit(t[8]);
        else if (bit(0x80) && bit(0x800)) emit(t[9]);
        else if (bit(0x80)) emit(t[10]);
        else emit(t[11]);
    }
    if (bit(0x40) && bit(0x200)) { add_one(); emit(t[12]); }
    if (signed_compare(c.r(28), 3) > 0) emit(t[13]);
    if (signed_compare(c.r(28), 0) == 0) emit(t[14]);
    c.set(3, c.r(29));
    return c.r(29);
}
}
