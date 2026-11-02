// Reading a logged press item back into the press's working record (0x1067b920).
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"
#include <span>

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

namespace {
constexpr std::uint32_t open_record = 0x1073e4b0;
constexpr std::uint32_t item_at = 0x88;

// One register the original fills for a kind of item: from value `source` of the item (called at
// `back`), or with a constant.
struct Step {
    unsigned reg;
    bool constant;
    std::uint32_t source;
    std::uint32_t back;
};
constexpr Step value(unsigned reg, std::uint32_t index, std::uint32_t back) { return {reg, false, index, back}; }
constexpr Step fixed(unsigned reg, std::uint32_t constant) { return {reg, true, constant, 0}; }

// What each kind of item puts in r26, r25, r24, r23 (the first four fields), r29, r28 and r27. r22 stays -1
// unless a kind sets it.
constexpr Step kind_0fbc[] = {value(26, 0, 0x1067baa8), value(25, 1, 0x1067babc), value(24, 2, 0x1067bad0), value(23, 3, 0x1067bae4),
                              value(29, 4, 0x1067baf8), fixed(28, 0x1f)};
constexpr Step kind_0bdd[] = {value(26, 0, 0x1067bb54), value(25, 1, 0x1067bb68), value(24, 2, 0x1067bb7c), value(23, 3, 0x1067bb90),
                              value(29, 4, 0x1067bba4), value(28, 5, 0x1067bbb8)};
constexpr Step kind_0bdf[] = {value(26, 0xf, 0x1067bbd0), value(25, 0x10, 0x1067bbe4), value(24, 0x11, 0x1067bbf8), value(23, 0x12, 0x1067bc0c),
                              value(29, 4, 0x1067bc20), value(28, 9, 0x1067bc34), value(27, 1, 0x1067bc48)};
constexpr Step kind_0be6[] = {value(26, 0, 0x1067bc60), value(25, 1, 0x1067bc74), value(24, 2, 0x1067bc88), value(23, 3, 0x1067bc9c),
                              value(29, 9, 0x1067bcb0), fixed(28, 0x82), fixed(27, 0), value(22, 0xa, 0x1067bccc)};
constexpr Step kind_0be8[] = {value(26, 0, 0x1067bce0), value(25, 1, 0x1067bcf4), value(24, 2, 0x1067bd08), value(23, 3, 0x1067bd1c),
                              value(29, 8, 0x1067bd30), fixed(28, 0), fixed(27, 0)};
constexpr Step other_kinds[] = {value(26, 0, 0x1067bd50), value(25, 1, 0x1067bd64), value(24, 2, 0x1067bd78), value(23, 3, 0x1067bd8c),
                                value(29, 4, 0x1067bda0), value(28, 9, 0x1067bdb4)};
}

// `date` points at the day the item was logged (its first word), `id` names the item.
void replay_logged_item(Cpu& c, std::uint32_t date, std::uint32_t id) {
    Frame frame(c, 0x1067b920, 0x1a0, {.first_gpr = 22});
    keep(c, {{31, id}, {30, c.toc(-0x7bb0)}, {25, c.toc(slot::flag)}, {26, date}, {24, c.toc(slot::message)}, {23, c.toc(slot::pool)}});
    const auto item = frame.at(item_at);
    c.set({{3, item}, {4, 0}, {5, 0}});
    c.call(library::begin_item, 0x1067b958);
    c.set(22, 0xffffffffu);
    if (compare_logical(c, date) == 0) {
        assertion(c, frame, 0x1067b978, 0x68, 0x40, 0x3b60);
        return;
    }
    c.set({{3, c.u32(c.toc(-0x7bf0))}, {4, date}, {5, id}, {6, item}});
    c.call(library::fetch_item, 0x1067b9d0);
    if (compare(c, std::int8_t(c.r(3))) == 0) {
        assertion(c, frame, 0x1067b9ec, 0x48, 0x38, 0x3b67);
        return;
    }
    c.set({{3, c.r(30)}, {4, c.toc(-0x5dfc)}, {5, c.toc(-0x5e00)}, {6, 1}, {7, c.toc(-0x5e04)}, {8, 0}});
    c.call(open_record, 0x1067ba48);
    if (compare(c, std::int8_t(c.r(3))) == 0) return;
    std::span<const Step> steps;
    switch (c.i32(item)) {
    case 0xfbc: steps = kind_0fbc; break;
    case 0xbdd: steps = kind_0bdd; break;
    case 0xbdf: steps = kind_0bdf; break;
    case 0xbe6: steps = kind_0be6; break;
    case 0xbe8: steps = kind_0be8; break;
    default: steps = other_kinds;
    }
    for (const auto& step : steps)
        c.set(step.reg, step.constant ? step.source : item_value(c, item, step.source, step.back));
    if (c.i32(item) == 0xfbc) {
        // A fifth party, read only when value 15 is set; it counts only as an index below the table's size.
        if (compare(c, std::int32_t(item_value(c, item, 0xf, 0x1067bb10))) != 0) {
            c.set(22, item_value(c, item, 0x10, 0x1067bb24));
            compare(c, std::int32_t(c.r(22)));
            if (std::int32_t(c.r(22)) < 0) c.set(22, 0xffffffffu);
            else if (compare(c, std::int32_t(c.r(22)), std::int32_t(c.u32(c.toc(-0x7b50)) - 1)) > 0) c.set(22, 0xffffffffu);
        }
    }
    // The record's ten fields.
    const std::uint32_t fields[10] = {c.r(31), c.r(29), c.r(28), 3, c.r(26), c.r(25), c.r(24), c.r(23), c.r(27), c.r(22)};
    static constexpr std::uint32_t backs[10] = {0x1067bdd0, 0x1067bde4, 0x1067bdf8, 0x1067be0c, 0x1067be20,
                                                0x1067be34, 0x1067be48, 0x1067be5c, 0x1067be70, 0x1067be84};
    for (unsigned index = 0; index < 10; ++index) {
        c.set({{3, c.r(30)}, {4, index}, {5, fields[index]}, {6, 0}});
        c.call(library::set_field, backs[index]);
    }
}
}
