// FINANCE.CPP, block board: the owner's bail-out of a club in trouble (0x101006a0).
//
// By the wealth band and the league's standing a draw decides whether the owner puts money in: a sum by reputation less
// the balance's low word (cut back to nothing now and then when it is large), booked as income on the donations line
// and announced as news kind 2 (names inferred). When the club has an owner record the bail-out may bring in a new owner
// from the table of owners and give the owner's person new traits. Returns 1 when booked.
#include "finance_board.hpp"
#include "finance_board_engine.hpp"

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;
using namespace board;

namespace {
constexpr std::uint32_t wealth_of_account = 0x100f55b0;
namespace club_field {
constexpr std::uint32_t division = 0x2c, league = 0x30, owner = 0x80;
}
namespace owner_field {
constexpr std::uint32_t taken_over = 0x0c,     // the record the owner was taken from
    first = 0x28, second = 0x2c, third = 0x30, // three words computed from it by 0x100c4400, 0x100c4cf0, 0x100c53e0
    person = 0x1c;
}
namespace pool_at {
constexpr std::uint32_t half = 0x38, quarter = 0x60, three_quarters = 0x70, fifteen_thousand = 0x338, two_thousandths = 0x238;
}

class Bailout {
public:
    Bailout(Cpu& cpu, std::uint32_t manager, std::uint32_t club, std::uint32_t account)
        : c(cpu), h(cpu), fpu(cpu), manager(manager), club(club), account(account) {
        h.set(24, c.toc(sample_table::base));
        h.set(25, c.toc(sample_table::first));
        h.set(26, c.toc(sample_table::last));
        h.set(27, c.toc(sample_table::cursor));
        h.set(28, c.toc(toc::doubles));
        h.set(29, manager);
        h.set(30, club);
        h.set(31, account);
        f31 = c.f(31);
        f30 = c.f(30);
    }

    std::uint32_t run() {
        h.sync();
        const auto band = std::int8_t(invoke_unit(c, wealth_of_account, 0x101006e0, {account}));
        // 0x100f55b0 answers only -2..2 (finance_ledger.cpp wealth: returns 2, 1, 0, -1, -2), so every test of a band beyond them is dead.
        if (band < -2 || band > 2) throw std::logic_error("finance board: a wealth band outside -2..2");
        const auto rep = reputation();
        if (band == -1) f31 = k(rep > 0x109a ? 0x118 : 0x200);
        else if (band == -2) f31 = k(rep > 0x109a ? 0x300 : pool_at::quarter);
        else f31 = k(0x180);
        h.set_f(31, f31);
        if (const auto league = c.u32(club + club_field::league); league != 0) {
            const auto scaled = fpu.fmul(k(pool_at::two_thousandths), to_double(fpu, c, rep));
            f30 = fpu.fdiv(scaled, to_double(fpu, c, c.i16(league + 0x18)));
            fpu.fcmp(0, f30, k(pool_at::three_quarters));
            if (f30 < k(pool_at::three_quarters)) f30 = k(pool_at::three_quarters);
        } else {
            f30 = k(pool_at::half);
        }
        h.set_f(30, f30);

        const auto owner = c.u32(club + club_field::owner);
        if (owner != 0 && c.u32(owner + owner_field::person) != 0) return with_owner_person();
        return without_owner_person();
    }

private:
    double k(std::uint32_t offset) const { return pool_double(c, offset); }
    std::int32_t reputation() const { return c.i16(club + club::reputation); }
    std::uint32_t owner() const { return c.u32(club + club_field::owner); }
    std::uint32_t person() const { return c.u32(owner() + owner_field::person); }

    // f31 becomes 15000 / f31; the draw below it, 0 when that is 0.
    std::int32_t limited_draw(const Below& site) {
        h.set_f(31, f31 = fpu.fdiv(k(pool_at::fifteen_thousand), f31));
        fpu.fcmp(0, f31, k(pool::zero));
        if (f31 == k(pool::zero)) {
            h.set(site.dest, 0);
            return 0;
        }
        return draw_below_real(h, fpu, site, f31);
    }

    // The sum, if the draw is below the reputation scaled by the league's standing: by reputation less the balance's
    // low word, now and then (above 40 million) forgiven down to the balance's low word negated; booked, refreshed.
    // Returns false when no money goes in.
    bool settle(std::int32_t drawn, unsigned amount_reg, std::uint32_t sample_back, std::uint32_t refresh_back) {
        const auto rep = reputation();
        const auto scaled = fpu.fmul(to_double(fpu, c, rep), f30);
        const auto against = to_double(fpu, c, drawn);
        fpu.fcmp(0, scaled, against);
        if (!(scaled > against)) return false;
        const auto low = c.u32(account + 4);
        auto amount = i(u(rep) * 1000u + 1'000'000u - low);
        h.set(amount_reg, u(amount));
        if (amount > 40'000'000) {
            if (reputation() < std::int32_t(sample(h, sample_back)) % 2500 + 2000) {
                amount = i(0u - c.u32(account + 4));
                h.set(amount_reg, u(amount));
            }
        }
        amount_ = u(amount);
        book_donation(c, account, amount_);
        refreshed_ = refresh_after_booking(h, account, refresh_back);
        return true;
    }

    // A club with an owner person: the person's byte at +0x36 against a draw, then the draw below 15000 over the divisor
    // against the league's standing; a new owner and new traits for the person; news kind 2.
    std::uint32_t with_owner_person() {
        const auto frequency = std::int32_t(c.i8(person() + 0x36));
        h.set(23, u(frequency));
        std::int32_t first = 0;
        if (frequency != 0) first = draw_below(h, {22, 0x101007ec, 0}, frequency);
        else h.set(22, 0);
        if (first != 0) return 0;
        const auto drawn = limited_draw({22, 0x10100958, 0x10100a48});
        if (!settle(drawn, 21, 0x10100b48, 0x10100bf0)) return 0;
        h.set(20, refreshed_);
        if (owner() != 0) {
            new_owner();
            if (person_of_new_owner()) new_traits();
        }
        announce(h, manager, club, amount_, refreshed_, 2, 0x10101074);
        c.put8(account + 0x165, 0);
        return 1;
    }

    // The owner is taken over from the club's division record, or (now and then, or without one) from the table of owners.
    void new_owner() {
        const auto division = c.u32(club + club_field::division);
        bool from_division = false;
        if (division != 0) from_division = std::int32_t(sample(h, 0x10100c2c)) % 1000 != 0;
        if (from_division) {
            c.put32(owner() + owner_field::taken_over, c.u32(club + club_field::division));
        } else {
            const auto count = c.i32(c.toc(-0x7b04));
            h.set(23, u(count));
            std::int32_t index = 0;
            if (count != 0) index = draw_below(h, {22, 0x10100cb0, 0x10100d78}, count);
            else h.set(22, 0);
            c.put32(owner() + owner_field::taken_over, c.u32(c.toc(-0x7b98)) + u(index) * 0x128u);
        }
        const auto pass = [&](std::uint32_t routine, std::uint32_t back, std::uint32_t field) {
            h.sync();
            const auto result = invoke(c, routine, back, {c.u32(owner() + owner_field::taken_over)});
            c.put32(owner() + field, result);
        };
        pass(0x100c4400, 0x10100dfc, owner_field::first);
        pass(0x100c53e0, 0x10100e10, owner_field::third);
        pass(0x100c4cf0, 0x10100e24, owner_field::second);
    }
    bool person_of_new_owner() const { return c.u32(owner() + owner_field::person) != 0; }

    // Four traits of the person are redrawn, each 1 to 20, the last again while below 5.
    void new_traits() {
        const auto trait = [&](std::uint32_t back, std::uint32_t field) {
            const auto drawn = std::int32_t(sample(h, back)) % 20 + 1;
            c.put8(person() + field, u(drawn));
        };
        trait(0x10100e5c, 0x3d);
        trait(0x10100ec4, 0x2f);
        trait(0x10100f2c, 0x36);
        trait(0x10100f94, 0x40);
        if (c.i8(person() + 0x40) < 5) trait(0x10101014, 0x40);
    }

    // A club without an owner person: one in twenty, then the same draw and the same sum; the amount is held in r22.
    std::uint32_t without_owner_person() {
        if (std::int32_t(sample(h, 0x101010a4)) % 20 != 0) return 0;
        const auto drawn = limited_draw({22, 0x10101118, 0x10101208});
        if (!settle(drawn, 22, 0x10101308, 0x101013b0)) return 0;
        announce(h, manager, club, amount_, refreshed_, 2, 0x101013c8);
        c.put8(account + 0x165, 0);
        return 1;
    }

    Cpu& c;
    Held h;
    Fpu fpu;
    std::uint32_t manager, club, account;
    double f31 = 0, f30 = 0;
    std::uint32_t amount_ = 0, refreshed_ = 0;
};
}

std::uint32_t owner_bailout(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t account) {
    Frame frame(c, 0x101006a0, 0xb0, {.first_gpr = 20, .first_fpr = 30});
    return Bailout(c, manager, club, account).run();
}
}
