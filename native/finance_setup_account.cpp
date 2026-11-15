// FINANCE.CPP: a club's account as the game starts (0x100f1050).
//
// The opening balance comes from the club's funds (the word at +4 of the club record): for a club whose funds
// are below 1.5 times its reputation squared, or the club "Rushden", it is the funds with a random
// jitter (one draw below a tenth of the funds minus another). The other clubs get a balance from a table by
// reputation, scaled by the club's division and its status, with the same kind of jitter on a smaller scale.
// The ledger is cleared and the weekly attendance figure is set for clubs of the top divisions.
#include "finance_setup.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::finance {
using guest::Frame;
using setup::magic;

namespace {
constexpr CheckSite missing_club{0x100f10a8, 0x40, 0x38, 0x8e};
constexpr std::int32_t linked_table = -0x7b5c;  // the cell holding the table the club's two links index
constexpr std::int32_t reputation_table = -0x55a4;  // doubles by reputation band
namespace club_field {
constexpr std::uint32_t funds = 0x04, name_prefix = 0x1ee, division = 0x2c, first_link = 0x30, second_link = 0x38,
                        status = 0x23d, uses_wealth = 0x23e, second_scale = 0x241;
}
namespace division_field {
constexpr std::uint32_t rating = 0x11c, size_rating = 0x11f, kind = 0x68;
}
namespace account_field {
constexpr std::uint32_t club = 0x08, attendance = 0x14, factor = 0x160, state = 0x164, cleared = 0x165, interval = 0x166;
}
// The doubles of the unit's pool (offsets).
namespace constant {
constexpr std::uint32_t one_and_a_half = 0x00, wide_range = 0x08, zero = 0x10, one = 0x18, three = 0x20, twenty = 0x28,
                        nineteen = 0x30, half = 0x38, five = 0x40, fifty_thousand = 0x48, int_magic = 0x50;
}
constexpr std::uint32_t club_name_text = 0x1f;  // in the string pool
constexpr std::uint32_t compare_names = 0x1077dcd0, to_balance = 0x10773de0, wealth_band = 0x100f55b0;

// The three registers the draws reuse, as the original holds them (it keeps them in r22, r27 and r28): at a
// call they hold these words, and f30, f31 their values.
struct Held {
    std::uint32_t second_draw, range_or_first, first_draw;
    double f30, f31;
};
// Where each inline draw returns to after its calls (the address after the `bl`).
struct DrawSite { std::uint32_t redraw_back, nested_back; };
}

void start_account(Cpu& c, std::uint32_t account, std::uint32_t club) {
    Frame frame(c, 0x100f1050, 0xd0, {.first_gpr = 22, .first_fpr = 30});
    guest::Fpu fpu(c);
    const auto message = c.toc(toc::message), doubles_pool = c.toc(toc::doubles), text_pool = c.toc(toc::pool);
    const auto table_cell = c.toc(linked_table);
    Held held{message, table_cell, text_pool, c.f(30), c.f(31)};
    const auto hold = [&] {
        c.set({{22, held.second_draw}, {27, held.range_or_first}, {28, held.first_draw}});
        c.set_f(30, held.f30);
        c.set_f(31, held.f31);
    };
    c.set({{23, c.toc(sample_table::base)}, {24, c.toc(sample_table::first)}, {25, c.toc(sample_table::last)},
           {26, c.toc(sample_table::cursor)}, {29, account}, {30, club}, {31, doubles_pool}});
    hold();
    const auto constant_at = [&](std::uint32_t offset) { return c.f64(doubles_pool + offset); };
    const auto to_double = [&](std::int32_t x) { return fpu.fsub(magic(x), constant_at(constant::int_magic)); };

    if (club == 0) {
        report_check(c, frame, missing_club);
        c.set(3, 0xffffffff);
        c.call(0x1076f7d0, 0x100f10f0);  // ends the game
    }
    c.put32(account + account_field::club, c.u32(club));
    if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f1100, {club})) != 0) {
        for (const auto at : {0x04u, 0x00u, 0x10u, 0x0cu, 0x1cu}) c.put32(account + at, 0);
        return;
    }
    // A club in status 3 that is linked, through either of its links, to an entry of the table is in status 2.
    if (c.u8(club + club_field::status) == 3) {
        const auto linked = [&](std::uint32_t link) {
            if (link == 0) return false;
            const auto table = c.u32(table_cell);
            if (table == 0) return false;
            return c.u32(table + c.u32(link) * 4u) != 0;
        };
        if (linked(c.u32(club + club_field::first_link)) || linked(c.u32(club + club_field::second_link)))
            c.put8(club + club_field::status, 2);
    }
    c.put32(account + account_field::factor, c.u32(c.toc(toc::floats)));

    // The inline draw below `limit` (0x100f2100's body, expanded): `into` is the register the answer goes to,
    // The limit also sits in the range register. A limit past 0xffff adds a draw
    // below a tenth-scale limit, made by 0x100f2100 itself.
    const auto draw = [&](std::int32_t limit, std::uint32_t& into, DrawSite site) {
        held.range_or_first = std::uint32_t(limit);
        if (limit == 0) {
            into = 0;
            return;
        }
        const auto sample = setup::sample(c, site.redraw_back, hold);
        if (setup::fits_one_draw(limit)) {
            into = std::uint32_t(setup::remainder_in(sample, limit));
            return;
        }
        const double scale = fpu.fdiv(to_double(limit), constant_at(constant::wide_range));
        const std::int32_t rest = std::int32_t(sample) % 0xffff;
        const double part = fpu.fmul(scale, to_double(rest));
        const auto partial = std::int32_t(std::uint32_t(fpu.fctiwz(0, part)));
        const auto next = std::int32_t(std::uint32_t(limit) * 10u) / 0xffff;
        into = std::uint32_t(partial);
        hold();
        c.link(site.nested_back);
        const auto tenth = random_below(c, next);
        auto answer = std::int32_t(std::uint32_t(partial) + std::uint32_t(setup::word(std::uint32_t(tenth) + 5u) / 10));
        if (answer > std::int32_t(std::uint32_t(limit) - 1u)) answer = std::int32_t(std::uint32_t(limit) - 1u);
        into = std::uint32_t(answer);
    };
    // The funds with a jitter: two draws below a tenth of the funds, their difference added.
    const auto funds_with_jitter = [&](DrawSite first, DrawSite second) {
        held.f30 = to_double(c.i32(club + club_field::funds));
        draw(c.i32(club + club_field::funds) / 10, held.first_draw, first);
        draw(c.i32(club + club_field::funds) / 10, held.second_draw, second);
        const auto difference = std::int32_t(held.first_draw - held.second_draw);
        held.f30 = fpu.fadd(held.f30, to_double(difference));
    };

    const auto funds = c.i32(club + club_field::funds);
    bool jitter_small = false;
    if (funds != 0) {
        const auto reputation = std::int32_t(c.i16(club + 0x1ec));
        const double reputation_a = to_double(reputation), reputation_b = to_double(reputation);
        const double squared = fpu.fmul(reputation_a, reputation_b);
        const double funds_value = to_double(funds);
        const double limit = fpu.fmul(constant_at(constant::one_and_a_half), squared);
        fpu.fcmp(0, funds_value, limit);
        jitter_small = funds_value < limit;
    }
    if (jitter_small) {
        funds_with_jitter({0x100f1254, 0x100f131c}, {0x100f13c4, 0x100f148c});
    } else if (invoke(c, compare_names, 0x100f1524, {club + club_field::name_prefix, text_pool + club_name_text, 6}) == 0) {
        funds_with_jitter({0x100f1590, 0x100f1658}, {0x100f1700, 0x100f17c8});
    } else {
        // The balance from the table by reputation band, scaled by the club's division and status.
        const auto band = std::int8_t(std::int32_t(c.i16(club + 0x1ec)) / 500 - 1);
        const auto table = c.toc(reputation_table);
        held.f30 = c.f64(table + std::uint32_t(band < 0 ? 0 : band) * 8u);
        const auto division = c.u32(club + club_field::division);
        if (division == 0) {
            held.f30 = constant_at(constant::zero);
        } else {
            fpu.fcmp(0, held.f30, constant_at(constant::zero));
            if (held.f30 > constant_at(constant::zero)) {
                double rating = to_double(c.i8(division + division_field::rating));
                fpu.fcmp(0, rating, constant_at(constant::one));
                if (rating < constant_at(constant::one)) rating = constant_at(constant::one);
                fpu.fcmp(0, rating, constant_at(constant::three));
                if (rating > constant_at(constant::three)) rating = constant_at(constant::three);
                const std::int32_t size = c.i8(division + division_field::size_rating);
                const auto squared_size = std::int32_t(std::uint32_t(size) * std::uint32_t(size));
                const double over_twenty = fpu.fdiv(held.f30, constant_at(constant::twenty));
                const double size_value = to_double(squared_size);
                const auto club_rep = std::int32_t(c.i16(club_record(c, c.u32(account + account_field::club)) + 0x1ec));
                const double size_share = fpu.fdiv(size_value, constant_at(constant::nineteen));
                const auto squared_rep = std::int32_t(std::uint32_t(club_rep) * std::uint32_t(club_rep));
                const double scaled = fpu.fmul(over_twenty, size_share);
                const double rep_value = to_double(squared_rep);
                held.f30 = fpu.fdiv(scaled, rating);
                const double half_rep = fpu.fmul(constant_at(constant::half), rep_value);
                const auto ceiling = std::int32_t(std::uint32_t(fpu.fctiwz(0, half_rep)));
                const double ceiling_value = to_double(ceiling);
                fpu.fcmp(0, held.f30, ceiling_value);
                if (held.f30 > ceiling_value) held.f30 = to_double(ceiling);
                const auto status = c.i8(club + club_field::status);
                if (status == 3) held.f30 = fpu.fdiv(held.f30, constant_at(constant::five));
                else if (status == 2) held.f30 = fpu.fdiv(held.f30, constant_at(constant::three));
            }
        }
        fpu.fcmp(0, held.f30, constant_at(constant::zero));
        if (held.f30 < constant_at(constant::zero)) {
            const auto s = setup::sample(c, 0x100f1a00, hold);
            const std::uint32_t sign = s >> 31;
            if ((((s & 1u) ^ sign) - sign) != 0) held.f30 = constant_at(constant::fifty_thousand);
        }
        held.f31 = fpu.fdiv(held.f30, constant_at(constant::five));
        // The two draws below f31 taken as a whole number (not a limit in a register).
        const auto draw_float = [&](std::uint32_t& into, DrawSite site) {
            fpu.fcmp(0, held.f31, constant_at(constant::zero));
            if (held.f31 == constant_at(constant::zero)) {
                into = 0;
                return;
            }
            const auto sample = setup::sample(c, site.redraw_back, hold);
            const auto limit = std::int32_t(std::uint32_t(fpu.fctiwz(0, held.f31)));
            if (setup::fits_one_draw(limit)) {
                into = std::uint32_t(setup::remainder_in(sample, limit));
                return;
            }
            fpu.fctiwz(0, held.f31);
            const double scale = fpu.fdiv(to_double(limit), constant_at(constant::wide_range));
            const std::int32_t rest = std::int32_t(sample) % 0xffff;
            const double part = fpu.fmul(scale, to_double(rest));
            const auto partial = std::int32_t(std::uint32_t(fpu.fctiwz(0, part)));
            const auto next = std::int32_t(std::uint32_t(limit) * 10u) / 0xffff;
            into = std::uint32_t(partial);
            hold();
            c.link(site.nested_back);
            const auto tenth = random_below(c, next);
            const auto again = std::int32_t(std::uint32_t(fpu.fctiwz(0, held.f31)));
            auto answer = std::int32_t(std::uint32_t(partial) + std::uint32_t(setup::word(std::uint32_t(tenth) + 5u) / 10));
            if (answer > std::int32_t(std::uint32_t(again) - 1u)) answer = std::int32_t(std::uint32_t(again) - 1u);
            into = std::uint32_t(answer);
        };
        draw_float(held.range_or_first, {0x100f1a6c, 0x100f1b5c});
        draw_float(held.second_draw, {0x100f1c0c, 0x100f1cfc});
        const auto difference = std::int32_t(held.range_or_first - held.second_draw);
        held.f30 = fpu.fadd(held.f30, to_double(difference));
    }

    hold();
    c.set_f(1, held.f30);
    c.call(to_balance, 0x100f1da4);
    c.put32(account + 4, c.r(4));
    c.put32(account + 0, c.r(3));
    const auto band = std::int8_t(invoke_unit(c, wealth_band, 0x100f1db4, {account}));
    // The wealth band is -2..2 (0x100f55b0), so `band >= 3` below cannot hold.
    if (c.i8(club + club_field::uses_wealth) == 0) c.put8(account + account_field::interval, 10);
    else if (band == 0) c.put8(account + account_field::interval, 10);
    else if (band < 0 || band >= 3) c.put8(account + account_field::interval, 0xf);
    else c.put8(account + account_field::interval, 5);
    c.put8(account + account_field::cleared, 0);
    // The ledger, both copies of every line, and the words around it.
    for (std::uint32_t at = 0x18; at < account_field::factor; at += 4) c.put32(account + at, 0);
    c.put8(account + account_field::state, 3);

    // A club of one of the three divisions in the cells, above reputation 0x128e and in a rich band, has the weekly
    // attendance figure scaled up with its reputation.
    const auto division = c.u32(club + club_field::division);
    std::uint32_t attendance = 0;
    if (division != 0) {
        const auto kind = c.i32(division + division_field::kind);
        const bool of_three = kind == c.i32(c.toc(-0x7a10)) || kind == c.i32(c.toc(-0x7a14)) || kind == c.i32(c.toc(-0x7a18));
        std::int32_t reputation = c.i16(club + 0x1ec);
        if (of_three && reputation > 0x128e && (band == 2 || band == 1)) {
            if (band == 1) {
                reputation = reputation > 0x1482 ? std::int16_t((reputation - 0x1482) * 2 + 0x1482) : reputation + 0xfa;
            } else {
                // 0x100f1fdc..0x100f1ff8: the reputation is already above 0x128e here, so the other arm
                // (adding 500) is never taken.
                reputation = std::int16_t((reputation - 0x128e) * 2 + 0x128e);
            }
            // 0x100f2024..0x100f2038 (band 0) cannot run: the guard at 0x100f1fa4 lets bands 1 and 2 through.
            reputation = std::int16_t(reputation);
            const std::uint32_t offset = c.i8(club + club_field::second_scale) != 0 ? 0x4e2 : 0x2ee;
            const std::uint32_t above = std::uint32_t(reputation) - offset;
            attendance = reputation < 0x1676 ? above * 6u : band == 1 ? above * 8u : above * 10u;
        }
    }
    c.put32(account + account_field::attendance, attendance);
}
}
