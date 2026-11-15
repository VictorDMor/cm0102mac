// FINANCE.CPP, block season: the donation to a club in need (0x100fe640).
//
// A club whose wealth band is short or in trouble (and a sound one below a small balance) may get a
// donation: some chance by its reputation, a second chance by its owner, then a sum by reputation
// booked as income on one ledger line with a finance news item of kind 4 (names inferred).
#include "finance_season.hpp"
#include "finance_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;

namespace {
constexpr std::uint32_t wealth_of_account = 0x100f55b0, draw_below = 0x100f2100, announce = 0x10101ff0;
// The income line the donation is booked on (word, both copies).
constexpr std::uint32_t donations_line = 0xb4;
// Offsets in the double pool.
namespace pool {
constexpr std::uint32_t scale = 0x08, zero = 0x10, short_divisor = 0x2a0, sound_divisor = 0x300, numerator = 0x308;
}
namespace club_field {
constexpr std::uint32_t owner = 0x80;  // the club's owner record; its word +0x1c is a person with a byte at +0x40
}

std::int32_t divide(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }
std::int32_t remainder(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(a) - std::uint32_t(divide(a, b)) * std::uint32_t(b));
}

constexpr std::uint32_t owner_sites[3] = {0x100feb3c, 0x100febbc, 0x100fec10}, plain_sites[3] = {0x100feea8, 0x100fef28, 0x100fef7c};

class Donation {
public:
    Donation(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t account)
        : c(c), fpu(c), object(object), club(club), account(account), doubles(c.toc(toc::doubles)),
          base_cell(c.toc(sample_table::base)), first(c.toc(sample_table::first)), last(c.toc(sample_table::last)),
          cursor(c.toc(sample_table::cursor)), chance_result(c.r(23)), second_chance(base_cell), f31(c.f(31)) {}

    bool run() {
        sync();
        const auto band = std::int8_t(invoke_unit(c, wealth_of_account, 0x100fe67c, {account}));
        if (band == -1) {
            f31 = c.f64(doubles + (reputation() < 0xdac ? pool::short_divisor : pool::sound_divisor));
        } else if (band < -1) {
            // Dead (0x100fe694): 0x100f55b0 returns only 2, 1, 0, -1 or -2 (its returns at 0x100f55e8..0x100f5724).
            if (band < -2) throw std::logic_error("0x100fe640: wealth band below -2 (dead at 0x100fe694)");
            f31 = c.f64(doubles + pool::short_divisor);
        } else {
            if (band > 0) return false;
            if (reputation() > 0xdac) return false;
            // A sound club only below 25 a point of reputation (signed 64-bit compare with the sign-extended limit).
            if (balance() >= std::int64_t(std::int32_t(std::uint32_t(reputation()) * 25u))) return false;
            f31 = c.f64(doubles + pool::sound_divisor);
        }
        // The first chance: reputation against 0xdac plus a draw below 7000.
        const auto sample = std::int32_t(drawn(0x100fe740));
        if (reputation() >= remainder(sample, 7000) + 0xdac) return false;

        const auto owner = c.u32(club + club_field::owner);
        const bool with_owner = owner != 0 && c.u32(owner + 0x1c) != 0;
        if (with_owner) {
            if (chance(0x100fe7d8, 0x100fe8c8) != 0) return false;
            // The owner's person decides how often it happens (a signed byte: 0 never, and never past 0xffff).
            const auto frequency = std::int32_t(c.i8(c.u32(c.u32(club + club_field::owner) + 0x1c) + 0x40));
            chance_result = std::uint32_t(frequency);
            if (frequency == 0) return false;
            const auto draw_sample = std::int32_t(drawn(0x100fe988));
            // 0x100fe9a0..0x100fea84 is the large-limit branch of the below-a-limit draw: a signed byte is within
            // +-0xffff, so it is never taken.
            if (frequency > 0xffff || frequency < -0xffff)
                throw std::logic_error("0x100fe640: the owner's frequency is past 0xffff (dead branch at 0x100fe9b0)");
            const auto rest = remainder(draw_sample, frequency);
            second_chance = std::uint32_t(frequency > 0 ? rest : std::int32_t(0u - std::uint32_t(rest)));
            if (second_chance == 0) return false;
        } else {
            if (chance(0x100fec54, 0x100fed44) != 0) return false;
            const auto draw_sample = std::int32_t(drawn(0x100fede8));
            if (remainder(draw_sample, 10) == 0) return false;
        }
        donate(with_owner ? owner_sites : plain_sites);
        return true;
    }

private:
    std::int32_t reputation() const { return c.i16(club + club::reputation); }
    std::int64_t balance() const { return balance_of(c, account); }
    // The non-volatile registers as the original holds them at a call.
    void sync() {
        c.set({{23, chance_result}, {24, second_chance}, {25, first}, {26, last}, {27, cursor}, {28, doubles}, {29, object}, {30, club}, {31, account}});
        c.set_f(31, f31);
    }
    std::uint32_t drawn(std::uint32_t back) {
        sync();
        return draw(c, back);
    }
    // int to double through the magic word, as the original does with `xoris` and the stack.
    double to_double(std::int32_t value) {
        const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(value) ^ 0x80000000u);
        return fpu.fsub(std::bit_cast<double>(bits), c.f64(doubles + doubles::int_magic));
    }

    // The second chance of the club's own draw: 0 means "the event happens". f31 becomes numerator / divisor and
    // that quotient, as a limit, is drawn below (one draw, with the inner draw of 0x100f2100 for a limit past
    // 0xffff). `back` is the address after the first draw's call.
    std::int32_t chance(std::uint32_t back, std::uint32_t inner_back) {
        f31 = fpu.fdiv(c.f64(doubles + pool::numerator), f31);
        fpu.fcmp(0, f31, c.f64(doubles + pool::zero));
        if (f31 == c.f64(doubles + pool::zero)) {
            chance_result = 0;
            return 0;
        }
        const auto sample = std::int32_t(drawn(back));
        auto limit = std::int32_t(fpu.fctiwz(0, f31));
        const bool small = limit <= 0xffff && (fpu.fctiwz(0, f31), limit >= -0xffff);
        if (small) {
            // divw by 0 leaves the remainder as the sample itself (the product with the quotient is 0).
            const auto rest = limit == 0 ? sample : remainder(sample, limit);
            chance_result = std::uint32_t(limit > 0 ? rest : std::int32_t(0u - std::uint32_t(rest)));
            return std::int32_t(chance_result);
        }
        limit = std::int32_t(fpu.fctiwz(0, f31));
        const auto scale = fpu.fdiv(to_double(limit), c.f64(doubles + pool::scale));
        const auto place = remainder(sample, 0xffff);
        const auto share = std::int32_t(fpu.fctiwz(0, fpu.fmul(scale, to_double(place))));
        const auto tenth = divide(std::int32_t(std::uint32_t(limit) * 10u), 0xffff);
        chance_result = std::uint32_t(share);
        sync();
        const auto inner = std::int32_t(invoke_unit(c, draw_below, inner_back, {std::uint32_t(tenth)}));
        const auto top = std::int32_t(std::uint32_t(std::int32_t(fpu.fctiwz(0, f31))) - 1u);
        auto result = std::int32_t(std::uint32_t(share) + std::uint32_t(divide(std::int32_t(std::uint32_t(inner) + 5u), 10)));
        if (result > top) result = top;
        chance_result = std::uint32_t(result);
        return result;
    }

    // The sum by reputation, in the first of three sizes the balance allows, booked as income; then the news.
    // `back` holds the addresses after the three calls of the news routine (one per size).
    void donate(const std::uint32_t (&back)[3]) {
        const auto grant = [&](std::uint32_t amount, std::uint32_t after) {
            c.put32(account + donations_line, c.u32(account + donations_line) + amount);
            c.put32(account + donations_line + account::second_copy, c.u32(account + donations_line + account::second_copy) + amount);
            c.put64(account, c.u64(account) + std::uint64_t(std::int64_t(std::int32_t(amount))));
            sync();
            invoke_unit(c, announce, after, {object, club, amount, 0, 4});
        };
        const auto owed = [&](std::uint32_t amount) { return balance() < std::int64_t(std::int32_t(0u - amount)); };
        if (reputation() < 0x4e2) {
            const auto amount = std::uint32_t(reputation()) * 0x1c2u;
            if (owed(amount)) return grant(amount, back[0]);
        }
        {
            const auto amount = std::uint32_t(reputation()) * 300u;
            if (owed(amount)) return grant(amount, back[1]);
        }
        grant(std::uint32_t(reputation()) * 0x96u, back[2]);
    }

    Cpu& c;
    Fpu fpu;
    std::uint32_t object, club, account, doubles, base_cell, first, last, cursor;
    std::uint32_t chance_result, second_chance;
    double f31;
};
}

std::uint32_t donation(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t account) {
    Frame frame(c, 0x100fe640, 0xa0, {.first_gpr = 23, .gpr_offset = -0x34, .first_fpr = 31});
    return Donation(c, object, club, account).run() ? 1 : 0;
}
}
