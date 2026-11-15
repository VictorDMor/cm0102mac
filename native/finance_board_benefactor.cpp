// FINANCE.CPP, block board: a benefactor's gift to a club that is short of money (0x100fefa0).
//
// A club in a poor wealth band may get a gift once the dice allow it: a limit by reputation and wealth, one
// draw (or two, when the club's owner has a person record) against it, and a sum by reputation booked as
// income on the donations line, then a finance news item of kind 3, 5, 6, 7 or 1 (names inferred from the
// callers and the news kinds; the routine returns 1 when a gift was booked).
#include "finance_board.hpp"
#include "finance_board_engine.hpp"

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;
using namespace board;

namespace {
constexpr std::uint32_t wealth_of_account = 0x100f55b0, pow_routine = 0x10780fc0;
namespace club_field {
constexpr std::uint32_t division = 0x2c,     // the club's division record: its word +0x68 is the kind of competition
    owner = 0x80,                            // the owner record; its word +0x1c is a person with bytes at +0x2f, +0x40
    national = 0x23d,                        // a byte: 2 and 3 mark clubs the board never helps
    refused = 0x241;                         // a byte: the gift was already made once (set with news kind 7)
}
namespace pool_at {
constexpr std::uint32_t scale = 0x20, balance_scale = 0x168, million = 0x1d0, share = 0x98, exponent = 0x2a0, thirty = 0x330,
                        fifteen_thousand = 0x338;
}

class Gift {
public:
    Gift(Cpu& cpu, std::uint32_t manager, std::uint32_t club, std::uint32_t account)
        : c(cpu), h(cpu), fpu(cpu), manager(manager), club(club), account(account) {
        h.set(21, c.toc(-0x7514));
        h.set(22, c.toc(toc::doubles));
        h.set(23, c.toc(-0x750c));
        h.set(24, c.toc(-0x7504));
        h.set(25, c.toc(sample_table::base));
        h.set(26, c.toc(sample_table::first));
        h.set(27, c.toc(sample_table::last));
        h.set(28, c.toc(sample_table::cursor));
        h.set(29, manager);
        h.set(30, club);
        h.set(31, account);
    }

    std::uint32_t run() {
        if (refused_club()) return 0;
        h.sync();
        const auto band = std::int8_t(invoke_unit(c, wealth_of_account, 0x100ff080, {account}));
        // 0x100f55b0 answers only -2..2 (finance_ledger.cpp wealth: returns 2, 1, 0, -1, -2), so every test of a band beyond them is dead.
        if (band < -2 || band > 2) throw std::logic_error("finance board: a wealth band outside -2..2");
        h.set_f(31, f31 = divisor(band));
        const auto owner = c.u32(club + club_field::owner);
        if (owner != 0 && c.u32(owner + 0x1c) != 0) return with_owner_person();
        return without_owner_person();
    }

private:
    std::int32_t reputation() const { return c.i16(club + club::reputation); }
    double k(std::uint32_t offset) const { return pool_double(c, offset); }
    std::int32_t global(std::int32_t slot) const { return c.i32(c.toc(slot)); }
    bool in_kinds(std::initializer_list<std::int32_t> slots) const {
        const auto division = c.u32(club + club_field::division);
        if (division == 0) return false;
        const auto kind = c.i32(division + 0x68);
        for (const auto slot : slots)
            if (kind == global(slot)) return true;
        return false;
    }

    // A club of a kind the board does not help (the cells name seven competition kinds), or a national side.
    bool refused_club() const {
        if (club == 0 || c.u32(club + club_field::division) == 0) return false;
        if (in_kinds({-0x757c, -0x7574, -0x7578, -0x7504, -0x750c, -0x7514, -0x7510})) return true;
        const auto flag = c.i8(club + club_field::national);
        return flag == 2 || flag == 3;
    }

    // The divisor of the limits, by the wealth band and the reputation (and, for a sound club, its owner's person).
    double divisor(std::int8_t band) const {
        const auto rep = reputation();
        if (band == -1) return rep > 0x1482 ? k(0xe0) : rep > 0xcb2 ? k(0x310) : k(0x318);
        if (band < -1) {
            return rep > 0xea6 ? k(0x210) : k(0x310);
        }
        if (band > 0) return k(0x180);
        const auto owner = c.u32(club + club_field::owner);
        const auto person = owner != 0 ? c.u32(owner + 0x1c) : 0u;
        if (rep <= 0x109a) {
            if (owner != 0 && person != 0 && c.i8(owner + 0x63) > 0xf && c.i8(person + 0x40) > 0xf) return k(0x320);
            return k(0x328);
        }
        if (rep < 0x186a && owner != 0 && person != 0 && c.i8(person + 0x40) > 0xf && c.i8(owner + 0x63) > 0xf) return k(0x200);
        return k(0x318);
    }

    // The sum is a third in the club's three kinds of competition that hold back their money.
    std::int32_t third_in_those_kinds(std::int32_t amount) const {
        // 0x100ff750..0x100ff76c and 0x100ffc48..0x100ffc6c: the three cells are among the seven the refusal at 0x100ff024..0x100ff048
        // tested the same kind against (0x100ff0.. returned 0), and nothing between changes the kind: the third never applies.
        if (in_kinds({-0x7514, -0x7504, -0x750c})) throw std::logic_error("0x100fefa0: a kind the refusal let through is cut to a third");
        return amount;
    }

    void book(std::uint32_t amount) {
        h.set(17, amount);
        book_donation(c, account, amount);
    }

    // A club with a person for its owner: the person's byte at +0x2f against a draw, then a draw below 30 over the
    // divisor, and the club's debts against a draw below the person's byte at +0x40 million.
    std::uint32_t with_owner_person() {
        const auto frame_at = [&](std::int32_t offset) { return c.sp() + std::uint32_t(offset); };
        auto person = [&] { return c.u32(c.u32(club + club_field::owner) + 0x1c); };
        const auto first_limit = std::int32_t(c.i8(person() + 0x2f));
        h.set(20, u(first_limit));
        std::int32_t first = 0;
        if (first_limit != 0) first = draw_below(h, {18, 0x100ff1e4, 0}, first_limit);
        else h.set(18, 0);
        h.set_f(31, f31 = fpu.fdiv(k(pool_at::thirty), f31));
        fpu.fcmp(0, f31, k(pool::zero));
        std::int32_t second = 0;
        if (f31 == k(pool::zero)) h.set(19, 0);
        else second = draw_below_real(h, fpu, {19, 0x100ff348, 0x100ff438}, f31);
        if (first <= second) return 0;

        const auto high = c.u32(account), low = c.u32(account + 4);
        h.set(20, high);
        h.set(19, low);
        const auto stake = i(u(c.i8(c.u32(c.u32(club + club_field::owner) + 0x1c) + 0x40)) * 1'000'000u);
        h.set(18, u(stake));
        std::int32_t drawn = 0;
        if (stake != 0) drawn = draw_below(h, {17, 0x100ff50c, 0x100ff5d4}, stake);
        else h.set(17, 0);
        // Proceeds only while the debt (the balance negated, 64 bits) is below the draw.
        const auto debt = std::int64_t(0 - ((std::uint64_t(high) << 32) | low));
        if (!(debt < std::int64_t(drawn))) return 0;

        const auto skill = std::int32_t(c.i8(c.u32(c.u32(club + club_field::owner) + 0x1c) + 0x40));
        const auto rep = reputation();
        if (rep >= i(u(skill) * 750u)) return 0;
        h.sync();
        c.set_f(1, to_double(fpu, c, skill));
        c.set_f(2, k(pool_at::exponent));
        c.call(pow_routine, 0x100ff6a0);
        c.set(2, c.u32(frame_at(0x14)));
        const auto squared = c.f(1);
        const auto per_point = fpu.multiply_add(0x100ff6d4, k(pool_at::scale), squared, k(pool_at::balance_scale));
        const auto points = to_double(fpu, c, rep);
        const auto base = fpu.multiply_add(0x100ff6ec, points, per_point, k(pool_at::million));
        auto amount = i(std::uint32_t(fpu.fctiwz(0, base)));
        const std::int32_t balance_high = c.i32(account);
        const std::uint32_t balance_low = c.u32(account + 4);
        amount = balance_high < 0 ? i(u(amount) - balance_low) : i(u(amount) + balance_low);
        if (rep < 2000) {
            const auto cap = i(u(rep) * u(rep) + 1'500'000u);
            if (amount > cap) amount = cap;
        }
        amount = third_in_those_kinds(amount);
        book(u(amount));
        const auto refreshed = refresh_after_booking(h, account, 0x100ff7d0);
        h.set(18, refreshed);
        notify_with_person(u(amount), refreshed);
        // The person's byte at +0x40 wears down toward 5.
        const auto owner_person = c.u32(c.u32(club + club_field::owner) + 0x1c);
        const auto byte = c.u8(owner_person + 0x40);
        if (std::int8_t(byte) > 5) c.put8(owner_person + 0x40, byte - 1u);
        c.put8(account + 0x165, 0);
        return 1;
    }

    // The news kind, by the refresh's second word and the club's flag.
    void notify_with_person(std::uint32_t amount, std::uint32_t refreshed) {
        const auto flag = [&] { return c.i8(club + club_field::refused); };
        if (!(i(refreshed) > 0 || flag() != 0)) return announce(h, manager, club, amount, refreshed, 3, 0x100ff9f8);
        if (flag() != 0 && multiple_of(h, 0x100ff810, 4)) return announce(h, manager, club, amount, refreshed, 5, 0x100ff858);
        if (multiple_of(h, 0x100ff87c, 4)) {
            const auto rep = reputation();
            const auto threshold = i(std::uint32_t(fpu.fctiwz(0, fpu.fmul(k(pool_at::share), to_double(fpu, c, i(u(rep) * u(rep)))))));
            if (i(amount) < threshold && i(amount) > 50000) return announce(h, manager, club, amount, refreshed, 6, 0x100ff914);
        }
        if (flag() == 0 && reputation() > 0x128e && multiple_of(h, 0x100ff950, 4)) {
            const auto division = c.u32(club + club_field::division);
            if (division != 0 && c.i32(division + 0x68) != global(-0x7a14)) {
                announce(h, manager, club, amount, refreshed, 7, 0x100ff9b8);
                c.put8(club + club_field::refused, 1);
                return;
            }
        }
        announce(h, manager, club, amount, refreshed, 1, 0x100ff9dc);
    }

    // A club without an owner person: one draw below 15000 over the divisor against the reputation, one in eight.
    std::uint32_t without_owner_person() {
        h.set_f(31, f31 = fpu.fdiv(k(pool_at::fifteen_thousand), f31));
        fpu.fcmp(0, f31, k(pool::zero));
        std::int32_t drawn = 0;
        if (f31 == k(pool::zero)) h.set(17, 0);
        else drawn = draw_below_real(h, fpu, {17, 0x100ffa64, 0x100ffb54}, f31);
        if (reputation() <= drawn) return 0;
        if (!multiple_of(h, 0x100ffbfc, 8)) return 0;
        auto amount = i(u(reputation()) * 1000u + 1'000'000u);
        h.set(17, u(amount));
        amount = third_in_those_kinds(amount);
        book(u(amount));
        const auto refreshed = refresh_after_booking(h, account, 0x100ffcd4);
        h.set(18, refreshed);
        const auto flag = [&] { return c.i8(club + club_field::refused); };
        const auto done = [&] { c.put8(account + 0x165, 0); return 1u; };
        if (!(i(refreshed) > 0 || flag() != 0)) {
            announce(h, manager, club, u(amount), refreshed, 3, 0x100ffe88);
            return done();
        }
        if (flag() != 0 && multiple_of(h, 0x100ffd14, 4)) {
            announce(h, manager, club, u(amount), refreshed, 5, 0x100ffd5c);
            return done();
        }
        if (multiple_of(h, 0x100ffd80, 4)) {
            announce(h, manager, club, u(amount), refreshed, 6, 0x100ffdc8);
            return done();
        }
        if (flag() == 0 && reputation() > 0x128e && multiple_of(h, 0x100ffe00, 4)) {
            announce(h, manager, club, u(amount), refreshed, 7, 0x100ffe48);
            c.put8(club + club_field::refused, 1);
            return done();
        }
        announce(h, manager, club, u(amount), refreshed, 1, 0x100ffe6c);
        return done();
    }

    Cpu& c;
    Held h;
    Fpu fpu;
    std::uint32_t manager, club, account;
    double f31 = 0;
};
}

std::uint32_t benefactor_gift(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t account) {
    Frame frame(c, 0x100fefa0, 0xc0, {.first_gpr = 17, .gpr_offset = -0x4c, .first_fpr = 31});
    return Gift(c, manager, club, account).run();
}
}
