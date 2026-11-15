// FINANCE.CPP, block board: the board's cover of a club's shortfall (0x100ffec0).
//
// Only a club of reputation 0x128e or more, that has not been refused, and not in the three competition kinds the board
// keeps out of. By the wealth band a limit is drawn against (one draw below 30 over the divisor with an owner
// person, whose bytes at +0x2f and +0x40 decide it, or below 4 over the divisor without), and the sum is by
// reputation, replaced by a tenth of the debt when that is larger, capped by reputation, booked as income on the
// donations line and announced as news kind 3 (names inferred). Returns 1 when booked.
#include "finance_board.hpp"
#include "finance_board_engine.hpp"

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;
using namespace board;

namespace {
constexpr std::uint32_t wealth_of_account = 0x100f55b0, to_double_routine = 0x10773c80;
namespace club_field {
constexpr std::uint32_t division = 0x2c, owner = 0x80, refused = 0x241;
}
namespace pool_at {
constexpr std::uint32_t one = 0x18, four = 0x230, thirty = 0x330, tenth = 0xe0, quarter = 0x60, thousandth = 0x180;
}

class Cover {
public:
    Cover(Cpu& cpu, std::uint32_t manager, std::uint32_t club, std::uint32_t account)
        : c(cpu), h(cpu), fpu(cpu), manager(manager), club(club), account(account) {
        h.set(24, c.toc(sample_table::base));
        h.set(25, c.toc(sample_table::first));
        h.set(26, c.toc(sample_table::last));
        h.set(27, c.toc(sample_table::cursor));
        h.set(28, c.toc(toc::doubles));
        h.set(29, manager);
        h.set(30, club);
        h.set(31, account);
        h.set_f(31, f31 = k(pool_at::one));
    }

    std::uint32_t run() {
        if (c.i16(club + club::reputation) < 0x128e || flag() != 0) return 0;
        if (club != 0) {
            if (const auto division = c.u32(club + club_field::division); division != 0) {
                const auto kind = c.i32(division + 0x68);
                for (const auto slot : {-0x757c, -0x7574, -0x7578})
                    if (kind == c.i32(c.toc(slot))) return 0;
            }
        }
        h.sync();
        const auto band = std::int8_t(invoke_unit(c, wealth_of_account, 0x100fff70, {account}));
        // 0x100f55b0 answers only -2..2 (finance_ledger.cpp wealth: returns 2, 1, 0, -1, -2), so every test of a band beyond them is dead.
        if (band < -2 || band > 2) throw std::logic_error("finance board: a wealth band outside -2..2");
        const auto share = band == -1 ? pool_at::tenth : band == -2 ? pool_at::quarter : pool_at::thousandth;
        h.set_f(31, f31 = fpu.fmul(f31, k(share)));

        const auto owner = c.u32(club + club_field::owner);
        // 0x100fffc4..0x100fffcc: the flag was 0 at 0x100fff08 and nothing between sets it.
        if (flag() != 0) throw std::logic_error("0x100ffec0: the refused flag appeared during the routine");
        if (owner != 0 && c.u32(owner + 0x1c) != 0) return with_owner_person();
        return without_owner_person();
    }

private:
    double k(std::uint32_t offset) const { return pool_double(c, offset); }
    std::int32_t flag() const { return c.i8(club + club_field::refused); }
    std::int32_t reputation() const { return c.i16(club + club::reputation); }
    std::uint32_t person() const { return c.u32(c.u32(club + club_field::owner) + 0x1c); }

    // f31 becomes numerator / divisor; the draw below it, 0 when that quotient is 0.
    std::int32_t limited_draw(std::uint32_t numerator, const Below& site) {
        h.set_f(31, f31 = fpu.fdiv(k(numerator), f31));
        fpu.fcmp(0, f31, k(pool::zero));
        if (f31 == k(pool::zero)) {
            h.set(23, 0);
            return 0;
        }
        return draw_below_real(h, fpu, site, f31);
    }

    // The owner's person must have a byte at +0x2f above the draw, and a byte at +0x40 above a draw mod 5 plus 10.
    std::uint32_t with_owner_person() {
        const auto drawn = limited_draw(pool_at::thirty, {23, 0x1010000c, 0x101000fc});
        if (c.i8(person() + 0x2f) <= drawn) return 0;
        const auto sample_word = std::int32_t(sample(h, 0x101001b0));
        const auto skill = std::int32_t(c.i8(person() + 0x40));
        if (skill <= sample_word % 5 + 10) return 0;
        const auto rep = reputation();
        h.set(24, u(rep));
        const auto amount = i(u(rep) * u(i(u(skill) * 20u + 0x96u)) + 750'000u);
        return pay(amount, 0x10100254, 0x1010033c);
    }

    std::uint32_t without_owner_person() {
        const auto drawn = limited_draw(pool_at::four, {23, 0x101003a0, 0x10100490});
        if (drawn != 0) return 0;
        // 0x10100514..0x10100518: the flag was tested at 0x100fff08 and nothing between sets it.
        if (flag() != 0) throw std::logic_error("0x100ffec0: the refused flag appeared during the routine");
        const auto rep = reputation();
        h.set(24, u(rep));
        const auto amount = i(u(rep) * 0xfau + 750'000u);
        return pay(amount, 0x10100568, 0x10100650);
    }

    // The sum: a tenth of the debt where ten times the sum is below the debt, capped by reputation; then booked.
    std::uint32_t pay(std::int32_t amount, std::uint32_t to_double_back, std::uint32_t refresh_back) {
        h.set(23, u(amount));
        const auto high = c.u32(account), low = c.u32(account + 4);
        const auto debt = std::int64_t(0 - ((std::uint64_t(high) << 32) | low));
        if (std::int64_t(i(u(amount) * 10u)) < debt) {
            h.sync();
            invoke(c, to_double_routine, to_double_back, {high, low});
            const auto tenth_of_debt = fpu.fmul(k(pool_at::tenth), c.f(1));
            amount = i(std::uint32_t(fpu.fctiwz(0, -tenth_of_debt)));
            h.set(23, u(amount));
        }
        const auto rep = std::int32_t(h.get(24));
        // 0x10100288..0x101002ec (and 0x1010059c..0x10100600): the cap for a reputation below 2000 is dead, the routine
        // returned at 0x100fff00 for a reputation below 0x128e and nothing between changes it.
        if (rep < 2000) throw std::logic_error("0x100ffec0: a reputation below 2000 reaches the sum");
        {
            const auto sum = to_double(fpu, c, amount);
            const auto limit = fpu.fmul(k(pool_at::quarter), to_double(fpu, c, i(u(rep) * u(rep))));
            fpu.fcmp(0, sum, limit);
            const auto cap = i(4'000'000u + (sum > limit ? 1u : 0u));
            if (amount > cap) amount = cap;
        }
        h.set(23, u(amount));
        book_donation(c, account, u(amount));
        const auto refreshed = refresh_after_booking(h, account, refresh_back);
        announce(h, manager, club, u(amount), refreshed, 3, refresh_back == 0x1010033c ? 0x10100354 : 0x10100668);
        c.put8(account + 0x165, 0);
        return 1;
    }

    Cpu& c;
    Held h;
    Fpu fpu;
    std::uint32_t manager, club, account;
    double f31 = 0;
};
}

std::uint32_t board_cover(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t account) {
    Frame frame(c, 0x100ffec0, 0xa0, {.first_gpr = 23, .gpr_offset = -0x34, .first_fpr = 31});
    return Cover(c, manager, club, account).run();
}
}
