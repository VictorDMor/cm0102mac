// MATCH_ENG.CPP, contact: the injury a contact causes to a player.
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

// 0x101f6940: a contact of `severity` (kind -1: none) hurts `player`; `culprit` (may be 0) pays ten points of morale.
// The counter at +0xe0ba counts the injuries this period.
void apply_contact_injury(Cpu& c, std::uint32_t match, std::uint32_t kind, std::uint32_t severity, std::uint32_t player,
                          std::uint32_t culprit) {
    Frame frame(c, 0x101f6940, 0xa0, {.first_gpr = 27, .gpr_offset = -0x34, .first_fpr = 29});
    c.put8(frame.at(0xbf), kind);
    c.put8(frame.at(0xc3), severity);
    keep(c, {{30, match}, {31, player}, {27, culprit}});
    const auto pool = c.toc(-0x52c8), table = c.toc(-0x52c4);
    keep(c, {{28, pool}, {29, table}});
    Fpu fp(c);
    c.set_f(31, c.f32(pool + 0xc));
    const auto counter_at = match + 0xe0ba;
    const auto magic = [&] { const double value = c.f64(table + 0x60); c.set_f(1, value); return value; };
    c.set(3, match + 0x10000);
    if (compare(c, c.i16(counter_at), 3) > 0) {
        c.set(3, extended(c.i16(counter_at)));
        // Beyond the third injury of a period, most contacts do nothing more.
        if (compare(c, std::int32_t(draw(c, c.r(3), 0x101f6998))) != 0) {
            if (compare(c, std::int32_t(draw(c, 500, 0x101f69a8))) != 0) return;
        }
    }
    c.set(3, match + 0x10000);
    if (compare(c, c.i16(counter_at), 6) > 0) return;
    if (compare(c, s8(c.u8(frame.at(0xbf))), -1) > 0) {
        c.set({{3, c.u32(c.toc(-0x7ba0))}, {4, c.u32(player + 0x6c)}, {5, frame.at(0xbf)}, {6, frame.at(0xc3)}});
        c.call(0x10186cd0, 0x101f69e8);
    }
    c.put32(player + 0x58, culprit);
    if (compare_logical(c, culprit) != 0) {
        c.set(3, extended(c.i16(culprit + 0x32)));
        c.put16(culprit + 0x32, c.r(3) - 10);
    }
    const auto amount = s8(c.u8(frame.at(0xc3)));
    const auto squared = u(amount * amount);
    if (compare(c, amount, 0x32) > 0) {
        c.set(3, u(amount));
        const double lost = from_int_single(c, fp, frame.at(0x58), squared, magic());
        c.put_f32(player + 0x50, float(fp.fsubs(c.f32(player + 0x50), lost)));
        c.put8(player + 0x1c, 1);
    } else {
        c.set(3, u(amount));
        const double scale = c.f64(table + 0x90);
        const double points = from_int(c, fp, frame.at(0x58), squared, magic());
        c.put64(frame.at(0x50), fp.fctiwz(0, fp.fmul(scale, points)));
        c.set(3, c.u32(frame.at(0x54)));
        const auto loss = draw(c, c.r(3), 0x101f6aa0);
        c.set_f(31, from_int_single(c, fp, frame.at(0x48), loss, magic()));
        c.put_f32(player + 0x50, float(fp.fsubs(c.f32(player + 0x50), c.f(31))));
        c.put64(frame.at(0x40), fp.fctiwz(0, fp.fmuls(c.f(31), c.f32(pool + 0x54))));
        c.set(3, c.u32(frame.at(0x44)));
        const auto stamina = draw(c, c.r(3), 0x101f6ae0);
        c.put_f32(player + 0x5c, float(from_int_single(c, fp, frame.at(0x38), stamina, magic())));
        c.set_f(1, c.f32(player + 0xd8));
        c.put_f32(player + 0xd8, float(fp.fmuls(c.f32(player + 0xd8), c.f32(pool + 0x54))));
    }
    const auto health = [&] { return c.f32(player + 0x50); };
    const auto hurt = [&] { c.put8(match + 0x792b, 1); c.put8(player + 0x1d, 1); };
    c.set_f(1, health());
    fp.fcmp(0, health(), c.f64(table + 0x2e8));
    if (c.cr(0) & 8) {
        hurt();
        c.put8(player + 0x1c, 1);
    } else {
        const double limit_base = c.f64(table + 0x2f0);
        const auto record = c.u32(player + 0x70);
        c.set(3, record);
        const auto grade = s8(c.u8(record + 0x2d)) * 25;
        c.set_f(1, magic());
        const double limit = fp.fadd(limit_base, from_int(c, fp, frame.at(0x38), u(grade), magic()));
        fp.fcmp(0, health(), limit);
        if (c.cr(0) & 8) {
            c.set({{3, player}, {4, 5}});
            c.call(0x10285ee0, 0x101f6b88);
            hurt();
        } else {
            c.set({{3, player}, {4, 3}});
            c.call(0x10285ee0, 0x101f6ba8);
            fp.fcmp(0, c.f(31), c.f32(pool + 0xf0));
            // cror eq,gt,eq: greater counts as equal.
            if (c.cr(0) & 4) c.set_cr(0, c.cr(0) | 2);
            if ((c.cr(0) & 2) && compare(c, s8(c.u8(player + 0x1c)), 0) != 0) hurt();
        }
    }
    // The health is kept between the table's floor and ceiling.
    c.set_f(1, c.f64(table + 0x58));
    fp.fcmp(0, c.f(1), health());
    if (c.cr(0) & 4) c.set_f(30, c.f64(table + 0x58));
    else {
        c.set_f(1, c.f64(table + 0x38));
        fp.fcmp(0, c.f(1), health());
        c.set_f(29, (c.cr(0) & 8) ? c.f64(table + 0x38) : health());
        c.set_f(30, c.f(29));
    }
    c.put_f32(player + 0x50, float(fp.frsp(c.f(30))));
    if (compare(c, s8(c.u8(match + 0x792b)), 0) != 0) {
        c.set(4, match + 0x10000);
        c.set(3, extended(c.i16(counter_at)));
        c.put16(counter_at, c.r(3) + 1);
    }
}
}
