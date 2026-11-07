// DATABASE.CPP's text block, part 6: a record's name in four layouts chosen by the caller's
// selector (0x100d0940; database_text.hpp).
#include "database_text.hpp"
#include "database_text_detail.hpp"
#include <array>

namespace cm::aot::database {
using namespace text;

namespace {
constexpr std::uint32_t format_entry = 0x10727890;

// A text of a layout: the pool offset of its format, or 0 for a plain copy of the name field, and the
// address after the original's call.
struct Slot {
    std::uint32_t offset;
    std::uint32_t back;
};
struct Assertion {
    std::uint32_t at;
    std::int32_t name, extension;
    std::uint32_t line;
};
// One selector's layout. `field` is the record's name string (+0x1ee or +0x221) and `grammar` the byte
// passed twice to the formatter (+0x242 or +0x23b). The texts, in the order of the original's code: with
// the record in range (the first test, which doubles the margin) and the long form: the second flag
// selects `with` or `without`; with the short form (and the second test, which does not double): the
// answer of a table lookup selects `looked_up` or `plain`, and a failed second test `second_failed`;
// with the record out of range: `far_long` or `far_short`.
struct Layout {
    std::uint32_t field, grammar;
    Slot with, without, looked_up, plain, second_failed, far_long, far_short;
    Assertion first, second;
};
constexpr Layout selector3{0x1ee, 0x242, {0x1b20, 0x100d0ac4}, {0x1b3b, 0x100d0ae8}, {0x1b54, 0x100d0bb4}, {0x1b76, 0x100d0bd8},
                           {0, 0x100d0be8}, {0x1b98, 0x100d0c14}, {0, 0x100d0c24},
                           {0x100d0a20, 0x160, 0x70, 0x33ef}, {0x100d0b08, 0x140, 0x68, 0x42e9}};
constexpr Layout selector2{0x221, 0x23b, {0x1b20, 0x100d0ce8}, {0x1b3b, 0x100d0d0c}, {0x1bb8, 0x100d0dd8}, {0x1bd8, 0x100d0dfc},
                           {0, 0x100d0e0c}, {0x1bf8, 0x100d0e38}, {0, 0x100d0e48},
                           {0x100d0c44, 0x120, 0x60, 0x33ef}, {0x100d0d2c, 0x100, 0x58, 0x42e9}};
constexpr Layout selector1{0x221, 0x23b, {0x1b20, 0x100d0f0c}, {0x1b3b, 0x100d0f30}, {0x1c17, 0x100d0ffc}, {0x1c32, 0x100d1020},
                           {0, 0x100d1030}, {0x1b98, 0x100d105c}, {0, 0x100d106c},
                           {0x100d0e68, 0xe0, 0x50, 0x33ef}, {0x100d0f50, 0xc0, 0x48, 0x42e9}};
constexpr Layout selector4{0x221, 0x23b, {0x1c4d, 0x100d1130}, {0x1c6e, 0x100d1154}, {0x1c8d, 0x100d1220}, {0x1cb3, 0x100d1244},
                           {0x1cd9, 0x100d1268}, {0x1cf6, 0x100d1294}, {0x1d1b, 0x100d12b8},
                           {0x100d108c, 0xa0, 0x40, 0x33ef}, {0x100d1174, 0x80, 0x38, 0x42e9}};
}

// 0x100d0940: the name of a record in the layout of `selector` (1-4; others do nothing), formatted into
// `buffer`: whether the record lies in range (its word at +0 against two words the TOC points at), then
// the long or short form. `limit` (r6) must be at least 0x47. A null record reports an assertion and
// takes the "out of range" side. Always answers true unless the limit assertion fires.
bool club_name_text(Cpu& c, std::uint32_t record, std::uint32_t long_form, std::uint32_t buffer, std::uint32_t limit,
                    std::uint32_t selector, std::uint32_t second_flag) {
    constexpr std::uint32_t entry = 0x100d0940;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x1d0, {.first_gpr = 22});
    compare(c, std::int32_t(limit), 0x47);
    const auto pool = c.toc(pool_slot), message = c.toc(-0x7ba8), success_flag = c.toc(-0x7bac);
    const auto margin_word = c.toc(-0x7b04), count_word = c.toc(-0x7b50), lookup_word = c.toc(-0x7a64);
    c.set({{31, lookup_word}, {24, margin_word}, {29, record}, {25, success_flag}, {22, long_form}, {26, message},
           {30, buffer}, {27, pool}, {23, second_flag}, {28, count_word}});
    const auto assertion = [&](const Assertion& a) {
        assertion_failure(c, frame, pool, a.at, a.name, a.extension, a.line);
    };
    if (compare(c, std::int32_t(limit), 0x47) < 0) {
        assertion({0x100d0994, 0x180, 0x78, 0x4550});
        return false;
    }
    const auto sel = extsb(selector);
    c.set(0, std::uint32_t(sel));
    const Layout* layout = nullptr;
    if (compare(c, sel, 3) == 0) layout = &selector3;
    else if (compare(c, sel, 3) > 0) {
        if (compare(c, sel, 5) < 0) layout = &selector4;
    } else if (compare(c, sel, 1) == 0) layout = &selector1;
    else if (compare(c, sel, 1) > 0) layout = &selector2;
    if (!layout) return guest::result(c, 1);
    const auto& t = *layout;

    const auto in_range = [&](bool doubled, const Assertion& a) {
        if (compare_logical(c, record) == 0) {
            assertion(a);
            return false;
        }
        const auto margin = c.u32(margin_word);
        c.set(3, margin);
        const auto total = c.u32(count_word);
        c.set(0, total);
        const auto scaled = doubled ? margin << 1 : margin;
        if (doubled) c.set(3, scaled);
        const auto first = c.u32(record);
        c.set(4, first);
        c.set(0, total - scaled);
        const bool fits = compare(c, std::int32_t(first), std::int32_t(total - scaled)) >= 0;
        c.set(3, fits ? 1 : 0);
        return fits;
    };
    const auto test_r3 = [&](bool fits) {  // extsb. r0,r3
        c.set(0, std::uint32_t(extsb(fits ? 1 : 0)));
        return compare(c, extsb(fits ? 1 : 0)) != 0;
    };
    const auto nonzero_byte = [&](std::uint32_t value) {  // extsb. r0,rX
        c.set(0, std::uint32_t(extsb(value)));
        return compare(c, extsb(value)) != 0;
    };
    const auto emit = [&](const Slot& slot) {
        if (slot.offset == 0) {
            c.set({{3, buffer}, {4, record + t.field}});
            c.call(strcpy_entry, slot.back);
            return;
        }
        const auto grammar = std::uint32_t(extsb(c.u8(record + t.grammar)));
        c.set({{5, grammar}, {3, buffer}, {6, grammar}, {4, pool + slot.offset}, {7, record + t.field}});
        c.call_in_place(format_entry, slot.back);
    };
    const auto fits1 = in_range(true, t.first);
    if (test_r3(fits1)) {
        if (nonzero_byte(long_form)) {
            emit(nonzero_byte(second_flag) ? t.with : t.without);
        } else {
            const auto fits2 = in_range(false, t.second);
            if (test_r3(fits2)) {
                const auto table = c.u32(lookup_word);
                c.set(4, table);
                const auto owner = c.u32(record + 0x2c);
                c.set(3, owner);
                const auto flags_table = c.u32(table + 0x38);
                c.set(4, flags_table);
                const auto index = c.u32(owner + 0x68);
                c.set(0, index);
                c.set(0, c.u8(flags_table + index));
                emit(nonzero_byte(c.r(0)) ? t.looked_up : t.plain);
            } else {
                emit(t.second_failed);
            }
        }
    } else {
        emit(nonzero_byte(long_form) ? t.far_long : t.far_short);
    }
    return guest::result(c, 1) != 0;
}
}
