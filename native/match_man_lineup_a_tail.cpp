// MATCH_MAN.CPP lineup_a block: the tactic's flag bits, set last from the kind of
// lineup 0x1020c5d0 finds and the assistant's ratings.
#include "match_man_lineup_a.hpp"
#include "match_man_lineup_a_detail.hpp"

namespace cm::aot::match_man {
using namespace guest;

namespace {
// Two tactic flag words are named by the bits they set (tactic object +0x30 of the side).
namespace bits {
constexpr std::uint32_t loose = 0x800, tight = 0x1000, pressing = 0x2000, direct = 0x4000, wide_low = 0x8000,
                        wide = 0x10000, wide_high = 0x20000;
}
}

// 0x10113b60 on the side's tactic with a flag word; `bl_address` is the call's address in
// the original (the return address is the next instruction).
void ComputerLineup::flag(std::uint32_t value, std::uint32_t bl_address) {
    c.set({{3, c.u32(side() + lineup_side::tactic)}, {4, value}});
    c.call(0x10113b60, bl_address + 4);
}

// The assistant's rating gap test: the value at assistant+0x33 against 10 (or 13
// when the side's squad rating byte at match+slot+0x1c6 is positive), with `base`
// in place of 10 where the original uses another. The original builds the
// positive test from neg, andc, srawi 31 and and.
bool ComputerLineup::assistant_below(std::uint32_t base) {
    const auto self = side();
    const auto rating = s8(c.u8(c.u32(self + lineup_side::match) + std::uint32_t(s8(c.u8(self + lineup_side::slot))) + 0x1c6));
    const auto threshold = std::int32_t(rating > 0 ? 3 : 0) + std::int32_t(base);
    const auto value = s8(c.u8(c.u32(self + lineup_side::assistant) + 0x33));
    c.set(5, std::uint32_t(value));
    return compare(c, value, threshold) < 0;
}

// 0x10209754-0x10209cec.
void ComputerLineup::tail_flags() {
    const auto self = side();
    // Is the tactic named like one of two strings of the module's pool?
    const auto name = [&] { return compare_logical(c, c.u32(self + lineup_side::tactic)) != 0 ? c.u32(self + lineup_side::tactic) : r23 + 0x21; };
    r14 = 0;
    c.set({{3, name()}, {4, r23 + 0x29}});
    c.call(0x1077dc90, 0x1020977c);
    if (compare(c, std::int32_t(c.r(3))) != 0) {
        c.set({{3, name()}, {4, r23 + 0x2f}});
        c.call(0x1077dc90, 0x102097a0);
        if (compare(c, std::int32_t(c.r(3))) != 0) r14 = 1;
    }
    c.set(3, self);
    r14 = std::uint32_t(s8(r14));
    match_man::call(c, lineup_entries::classifier, 0x102097b8);
    const auto kind = s8(c.r(3));
    const auto assistant = c.u32(self + lineup_side::assistant);
    // The float at the match's per-slot table (+0x30f0), by the side's number (or r13 below).
    const auto table_float = [&](std::uint32_t index) {
        const double value = c.f32(c.u32(self + lineup_side::match) + index * 4 + 0x30f0);
        c.set_f(1, value);
        return value;
    };
    const auto slot_index = [&] { return std::uint32_t(s8(c.u8(self + lineup_side::slot))); };
    if (compare(c, kind, 3) == 0) {
        if (compare_logical(c, assistant) != 0) {
            if (compare(c, s8(c.u8(assistant + 0x2e)), 0x10) > 0) flag(bits::wide_high, 0x102098f0);
            else flag(bits::wide_low, 0x10209904);
            if (assistant_below(10)) flag(bits::tight, 0x10209954);
            else flag(bits::loose, 0x10209964);
        } else {
            flag(bits::wide_low, 0x10209978);
            flag(bits::tight, 0x10209984);
        }
        flag(bits::pressing, 0x10209990);
    } else if (compare(c, kind, 1) == 0) {
        c.set(5, assistant);
        bool tested = false;
        if (compare_logical(c, assistant) != 0) {
            const double limit = c.f64(pool() + 0x80);
            fpu.fcmp(0, table_float(slot_index()), limit);
            if (c.cr(0) & 8) {
                tested = true;
                if (compare(c, s8(c.u8(assistant + 0x2e)), 6) > 0 && compare(c, s8(r14)) != 0) flag(bits::wide, 0x1020983c);
                else flag(bits::wide_high, 0x10209850);
            }
        }
        if (!tested) {
            bool above = compare(c, s8(r14)) != 0;
            if (!above) {
                fpu.fcmp(0, table_float(slot_index()), c.f64(pool() + 0x88));
                above = (c.cr(0) & 4) != 0;
            }
            if (above) flag(bits::wide, 0x10209894);
            else flag(bits::wide_high, 0x102098a8);
        }
        flag(bits::loose, 0x102098b4);
        flag(bits::pressing, 0x102098c0);
    } else if (compare(c, kind, 4) == 0) {
        if (compare_logical(c, assistant) != 0) {
            if (compare(c, s8(c.u8(assistant + 0x2e)), 0x12) > 0) flag(bits::wide_high, 0x102099c0);
            else flag(bits::wide_low, 0x102099d4);
            // Reloaded after the flag call, as the original does (0x102099d8).
            const auto assistant_now = c.u32(self + lineup_side::assistant);
            c.set(5, assistant_now);
            if (compare(c, s8(c.u8(assistant_now + 0x2e)), 0xc) > 0) {
                if (assistant_below(10)) flag(bits::tight, 0x10209a34);
                else flag(bits::loose, 0x10209a44);
                flag(bits::pressing, 0x10209a50);
            } else {
                flag(bits::loose, 0x10209a60);
                flag(bits::direct, 0x10209a6c);
            }
        } else {
            flag(bits::wide_low, 0x10209a80);
            flag(bits::loose, 0x10209a8c);
            flag(bits::direct, 0x10209a98);
        }
    } else if (compare(c, kind, 5) == 0) {
        if (compare_logical(c, assistant) != 0) {
            const auto level = s8(c.u8(assistant + 0x2e));
            if (compare(c, level, 0x10) > 0) flag(bits::wide, 0x10209ac8);
            else if (compare(c, level, 0xc) > 0) flag(bits::wide_high, 0x10209ae4);
            else flag(bits::wide_low, 0x10209af8);
            if (assistant_below(10)) flag(bits::tight, 0x10209b48);
            else flag(bits::loose, 0x10209b58);
        } else {
            flag(bits::wide_high, 0x10209b6c);
            flag(bits::tight, 0x10209b78);
        }
        flag(bits::pressing, 0x10209b84);
    } else {
        if (compare_logical(c, assistant) != 0) {
            const auto raw_level = c.u8(assistant + 0x2e);
            const auto raw_rating = c.u8(c.u32(self + lineup_side::match) + slot_index() + 0x1c6);
            const auto threshold = std::int32_t(s8(raw_rating) > 0 ? 3 : 0) + 0xf;
            c.set({{6, raw_level}, {7, raw_rating}, {8, c.u32(self + lineup_side::match)}, {5, std::uint32_t(s8(raw_rating))}});
            bool strong = compare(c, s8(raw_level), threshold) > 0 && compare(c, s8(r14)) != 0;
            if (strong) {
                flag(bits::wide, 0x10209bec);
                flag(bits::loose, 0x10209bf8);
            } else {
                bool low = compare(c, s8(raw_level), 5) <= 0 && compare(c, s8(raw_rating)) != 0;
                if (low) {
                    fpu.fcmp(0, table_float(c.r(13)), c.f64(pool() + 0x0));
                    low = (c.cr(0) & 4) == 0;
                }
                if (!low) {
                    flag(bits::wide_high, 0x10209c3c);
                    flag(bits::loose, 0x10209c48);
                } else {
                    flag(bits::wide_low, 0x10209c5c);
                    if (assistant_below(4)) flag(bits::tight, 0x10209cac);
                    else flag(bits::loose, 0x10209cbc);
                }
            }
        } else {
            flag(bits::wide_high, 0x10209cd0);
            flag(bits::loose, 0x10209cdc);
        }
        flag(bits::pressing, 0x10209ce8);
    }
}
}
