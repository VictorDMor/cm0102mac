// DATABASE.CPP's text block, part 8: a rating scaled by a record's table entry and a contract's
// effect (0x100d5de0; database_text.hpp).
#include "database_text.hpp"
#include "database_text_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::database {
using namespace text;

namespace {
constexpr std::uint32_t primary_contract = 0x10082f20;  // CONTRACT_MANAGER.CPP's lookup of a person's primary contract
constexpr std::uint32_t conversion_high = 0x43300000;
std::int32_t divide(std::int32_t a, std::int32_t b) { return b == 0 || (a == INT32_MIN && b == -1) ? 0 : a / b; }
}

// 0x100d5de0: a record's (a) 16-bit rating at +0x1ec adjusted by 500 per step of a table entry (the
// record's team's table at TOC -0x7b5c, indexed by the first word of the object at +0x30), by the pool's
// constants (TOC -0x5614) and a table of twenty doubles (TOC -0x56b8), and by +-500 when `b`'s primary
// contract is with a's club and a's role slot (b+0x18) names a. Answers the 16-bit result.
// Quirk: the original calls 0x10459b50 and, when it answers null, reports an assertion with a message
// built by sprintf.
std::int32_t scaled_rating(Cpu& c, std::uint32_t a, std::uint32_t b) {
    constexpr std::uint32_t entry = 0x100d5de0;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0xc0, {.first_gpr = 21, .gpr_offset = -0x3c, .first_fpr = 31, .fpr_offset = -8});
    guest::Fpu fpu(c);
    c.set({{21, a}, {22, b}});
    c.set(25, c.toc(-0x56b8));
    c.set(26, c.toc(-0x7b78));
    c.set(23, 0);
    c.set(27, c.toc(-0x7b7c));
    c.set(28, c.toc(-0x7ba8));
    const auto pool = c.toc(pool_slot);
    c.set(29, pool);
    c.set(30, c.toc(-0x7be8));
    const auto k = c.toc(-0x5614);
    c.set(31, k);
    const auto table = c.r(25);
    const auto constant = [&](std::uint32_t offset) { return c.f64(k + offset); };

    const auto result = [&](std::uint32_t value) {
        c.set(3, value);
        return std::int32_t(value);
    };
    const auto sixth = c.u32(a + 0x30);  // the object whose first word indexes the team table
    c.set(0, sixth);
    if (compare_logical(c, sixth) == 0) return result(std::uint32_t(c.i16(a + 0x1ec)));

    c.set(3, c.u32(a + 0x30));
    c.set(4, c.toc(-0x7b5c));
    c.set(0, c.u32(c.r(3)));
    c.set(3, c.u32(c.r(4)));
    c.set(0, c.r(0) << 2);
    const auto entry24 = c.u32(c.r(3) + c.r(0));
    c.set(24, entry24);
    bool big = false;
    if (compare_logical(c, entry24) != 0) {
        c.set(3, c.u32(a + 0x30));
        c.set(0, std::uint32_t(c.i16(a + 0x1ec)));
        c.set(3, std::uint32_t(c.i16(c.r(3) + 0x18)));
        c.set(3, c.r(3) * 0x1f4);
        big = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0;
    }

    // `b`'s primary contract's club, against a (the original's two copies of this test).
    const auto club_of_primary = [&](std::uint32_t back) {
        c.set(3, c.u32(c.r(27)));
        c.set(4, b);
        c.call_in_place(primary_contract, back);
        compare_logical(c, c.r(3), 0);
        c.put32(c.r(26), c.r(3));
        if (c.r(3) == 0) {
            c.set(0, 0);
            return;
        }
        c.set(3, c.u32(c.r(26)));
        c.set(4, c.u32(c.r(30)));
        c.set(0, c.u32(c.r(3) + 4));
        c.set(0, c.r(0) * 0x248);
        c.set(0, c.r(4) + c.r(0));
    };
    // The +-500 adjustment from b's role record (+0x18): a plus when it names a among its first three words,
    // a minus among the next three. Leaves r23.
    const auto adjust = [&](bool first_loaded) {
        std::uint32_t role = 0;
        if (!first_loaded) {
            role = c.u32(b + 0x18);
            c.set(0, role);
            compare_logical(c, role);
        } else {
            compare_logical(c, c.r(0));
            role = c.r(0);
        }
        if (role != 0) {
            bool plus = false;
            for (const std::uint32_t offset : {4u, 8u, 0xcu}) {
                c.set(3, c.u32(b + 0x18));
                c.set(0, c.u32(c.r(3) + offset));
                if (compare_logical(c, c.r(0), a) == 0) { plus = true; break; }
            }
            if (plus) {
                c.set(23, 0x1f4);
                return;
            }
        }
        c.set(0, c.u32(b + 0x18));
        if (compare_logical(c, c.r(0)) == 0) return;
        for (const std::uint32_t offset : {0x10u, 0x14u, 0x18u}) {
            c.set(3, c.u32(b + 0x18));
            c.set(0, c.u32(c.r(3) + offset));
            if (compare_logical(c, c.r(0), a) == 0) {
                c.set(23, 0xfffffe0c);
                return;
            }
        }
    };
    const auto adjusted = [&] {
        c.set(0, std::uint32_t(c.i16(a + 0x1ec)));
        c.set(0, c.r(0) + c.r(23));
        c.set(3, std::uint32_t(extsh(c.r(0))));
        return std::int32_t(c.r(3));
    };

    if (!big) {
        if (compare_logical(c, b) == 0) return adjusted();
        c.set(0, c.u32(b + 0x14));
        if (compare_logical(c, c.r(0)) == 0) return adjusted();
        club_of_primary(0x100d5e88);
        if (compare_logical(c, c.r(0), a) != 0) return adjusted();
        adjust(false);
        return adjusted();
    }

    // The big path: ask 0x10459b50 for the object of the table entry's word at +8.
    c.set(3, c.u32(entry24 + 8));
    c.set(4, a);
    c.call(0x10459b50, 0x100d5f58);
    if (compare_logical(c, c.r(3), 0) == 0) {
        c.set(6, c.u32(entry24 + 4));
        c.set(4, pool + 0x2121);
        c.set(3, c.toc(-0x7bb8));
        c.set(5, a + 0x221);
        c.set(6, c.r(6) + 0x4d);
        c.call(sprintf_entry, 0x100d5f78);
        assertion_failure(c, frame, pool, 0x100d5f8c, 0x40, 0x38, 0x51e2, true);
        return 0;
    }
    const auto object = c.r(3);
    const auto team_scale = std::int32_t(c.i16(entry24 + 0x3e));
    c.set(0, std::uint32_t(team_scale));
    c.set(4, c.u8(object + 0x76));
    compare(c, team_scale);
    c.set(4, c.r(4) - 1);
    // The clamped index below divides by the team's scale unless it is 0 or 20.
    if (team_scale != 0 && team_scale != 0x14) {
        c.set(0, std::uint32_t(c.i16(entry24 + 0x3e)));
        compare(c, team_scale, 0x14);
        c.set(4, c.r(4) * 0x14);
        c.set(0, std::uint32_t(c.i16(entry24 + 0x3e)));
        c.set(4, std::uint32_t(divide(std::int32_t(c.r(4)), std::int32_t(c.r(0)))));
    } else if (team_scale != 0) {
        c.set(0, std::uint32_t(c.i16(entry24 + 0x3e)));
        compare(c, team_scale, 0x14);
    }
    {
        const auto v = std::int32_t(c.r(4));
        compare(c, v);
        if (v < 0) {
            c.set(4, 0);
        } else {
            compare(c, v, 0x13);
            c.set(0, 0x13);
            if (!(v > 0x13)) c.set(0, c.r(4));
            c.set(4, c.r(0));
        }
    }
    c.set(0, c.u8(object + 4));
    c.set(4, c.r(4) << 3);
    const double top = c.f64(table + c.r(4));
    c.set_f(31, top);
    double f4;
    if (compare_logical(c, c.r(0), 0x14) < 0) {
        c.set(4, c.u32(a + 0x34));
        c.set(0, c.u32(a + 0x30));
        if (compare_logical(c, c.r(4), c.r(0)) != 0) {
            f4 = constant(0xd0);
        } else {
            const auto scale = std::int32_t(c.i16(entry24 + 0x3e));
            c.set(0, std::uint32_t(scale));
            bool plain = scale == 0;
            compare(c, scale);
            if (!plain) {
                c.set(0, std::uint32_t(c.i16(entry24 + 0x3e)));
                plain = compare(c, scale, 0x14) == 0;
            }
            if (!plain) {
                c.set(4, c.u8(a + 0x23c));
                c.set(0, std::uint32_t(c.i16(entry24 + 0x3e)));
                c.set(4, std::uint32_t(extsb(c.r(4))));
                c.set(4, c.r(4) - 1);
                c.set(4, c.r(4) * 0x14);
                const auto quotient = divide(std::int32_t(c.r(4)), std::int32_t(c.r(0)));
                c.set(4, std::uint32_t(quotient));
                compare(c, quotient);
                if (quotient < 0) {
                    c.set(4, 0);
                } else {
                    compare(c, quotient, 0x13);
                    c.set(0, 0x13);
                    if (!(quotient > 0x13)) c.set(0, c.r(4));
                    c.set(4, c.r(0));
                }
                c.set(0, c.r(4) << 3);
                f4 = c.f64(table + c.r(0));
            } else {
                const auto rating = extsb(c.u8(a + 0x23c));
                c.set(0, std::uint32_t(c.u8(a + 0x23c)));
                c.set(0, std::uint32_t(rating));
                bool outside = compare(c, rating) <= 0;
                if (!outside) {
                    c.set(0, std::uint32_t(c.u8(a + 0x23c)));
                    c.set(0, std::uint32_t(rating));
                    outside = compare(c, rating, 0x15) >= 0;
                }
                if (outside) {
                    f4 = constant(0xd0);
                } else {
                    c.set(0, std::uint32_t(c.u8(a + 0x23c)));
                    c.set(0, std::uint32_t(rating));
                    c.set(0, c.r(0) << 3);
                    c.set(4, table + c.r(0));
                    f4 = c.f64(c.r(4) - 8);
                }
            }
        }
        c.set_f(4, f4);
        // f31 = (f2 * f31 + (converted(20 - team) - c0) * f4) / c1
        const auto team = c.u8(object + 4);
        c.set(4, team);
        c.set(0, conversion_high);
        c.set(6, c.u8(object + 4));
        c.set(5, conversion_high);
        c.set(3, 0x14 - c.r(4));
        c.s.wr<1>(guest::reg::xer_ca, c.r(4) <= 0x14);
        c.put32(frame.at(0x68), c.r(0));
        c.set(0, c.r(3) ^ 0x80000000u);
        c.set_f(1, constant(0x18));
        c.put32(frame.at(0x6c), c.r(0));
        c.set_f(3, constant(0xe8));
        c.set_f(0, c.f64(frame.at(0x68)));
        c.put32(frame.at(0x74), c.r(6));
        const auto diff = fpu.fsub(c.f(0), c.f(1));
        c.set_f(1, diff);
        c.set_f(0, constant(0xb8));
        c.put32(frame.at(0x70), c.r(5));
        c.set_f(2, c.f64(frame.at(0x70)));
        c.set_f(1, fpu.fmul(c.f(1), f4));
        c.set_f(2, fpu.fsub(c.f(2), c.f(3)));
        c.set_f(1, fpu.multiply_add(0x100d612c, c.f(2), c.f(31), c.f(1)));
        c.set_f(31, fpu.fdiv(c.f(1), c.f(0)));
    }
    // The contract's effect.
    bool adjust_this = false;
    if (compare_logical(c, b) != 0) {
        c.set(0, c.u32(b + 0x14));
        if (compare_logical(c, c.r(0)) != 0) {
            club_of_primary(0x100d6154);
            adjust_this = compare_logical(c, c.r(0), a) == 0;
        }
    }
    if (adjust_this) {
        c.set(3, c.toc(-0x7b3c));
        c.set_f(1, fpu.frsp(c.f(31)));
        c.set(4, c.u32(b + 0x14));
        c.set(5, 6);
        c.set(3, c.u32(c.r(3)));
        c.set(6, 0x14);
        c.call(0x10303560, 0x100d61a0);
        c.set_f(0, constant(0x48));
        c.set(0, c.u32(b + 0x18));
        c.set_f(0, fpu.fmul(c.f(31), c.f(0)));
        c.set_f(2, constant(0x48));
        compare_logical(c, c.r(0));
        c.set_f(31, fpu.multiply_add(0x100d61b4, c.f(1), c.f(2), c.f(0)));
        adjust(true);
    }
    c.set_f(1, constant(0xd8));
    c.set_f(0, constant(0xe0));
    c.set(3, c.u32(a + 0x30));
    c.set_f(31, fpu.multiply_add(0x100d6240, c.f(1), c.f(31), c.f(0)));
    c.set_f(0, constant(0x40));
    c.set(0, std::uint32_t(c.i16(c.r(3) + 0x18)));
    c.set(0, c.r(0) * 0x1f4);
    fpu.fcmp(0, c.f(0), c.f(31));
    c.set(4, std::uint32_t(extsh(c.r(0))));
    // The original compares f0 (loaded before the rating's r0 was computed): f0 = constant(0x40).
    if (c.cr(0) & 4) {
        c.set_f(31, constant(0x40));
    } else {
        c.set_f(0, constant(0xb0));
        fpu.fcmp(0, c.f(0), c.f(31));
        if (c.cr(0) & 8) c.set_f(31, constant(0xb0));
    }
    c.set(3, std::uint32_t(c.i16(a + 0x1ec)));
    c.set(0, conversion_high);
    c.set(5, std::uint32_t(extsh(c.r(4))));
    c.set_f(0, constant(0xb0));
    c.set(3, c.r(3) ^ 0x80000000u);
    c.set(4, conversion_high);
    c.put32(frame.at(0x74), c.r(3));
    c.set(3, c.r(5) ^ 0x80000000u);
    c.set_f(1, constant(0x18));
    c.set_f(2, fpu.fsub(c.f(0), c.f(31)));
    c.put32(frame.at(0x70), c.r(0));
    c.set_f(4, constant(0x18));
    c.set_f(0, c.f64(frame.at(0x70)));
    c.put32(frame.at(0x6c), c.r(3));
    c.set_f(1, fpu.fsub(c.f(0), c.f(1)));
    c.set_f(0, constant(0xb0));
    c.put32(frame.at(0x68), c.r(4));
    c.set_f(3, c.f64(frame.at(0x68)));
    c.set_f(1, fpu.fmul(c.f(2), c.f(1)));
    c.set_f(2, fpu.fsub(c.f(3), c.f(4)));
    c.set_f(1, fpu.multiply_add(0x100d62c4, c.f(2), c.f(31), c.f(1)));
    c.set_f(0, fpu.fdiv(c.f(1), c.f(0)));
    const auto bits = fpu.fctiwz(0, c.f(0));
    c.put64(frame.at(0x60), bits);
    c.set(0, c.u32(frame.at(0x64)));
    c.set(0, c.r(0) + c.r(23));
    c.set(3, std::uint32_t(extsh(c.r(0))));
    return std::int32_t(c.r(3));
}
}
