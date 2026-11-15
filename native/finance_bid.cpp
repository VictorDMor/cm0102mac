// FINANCE.CPP, block bid: the facilities upgrade (0x100f9f40) and the sampled income (0x100fa1f0).
// Neither has a real call in any capture: every branch is covered by fixtures (finance_checks_bid.cpp).
#include "finance_bid.hpp"
#include "finance_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;

namespace {
namespace club_field {
constexpr std::uint32_t index = 0x00, income_scale = 0x08, reputation = 0x1ec, board_flag = 0x23e,
                        facilities = 0x240, competition = 0x2c, owner = 0x80;
}
namespace ledger_line {  // word lines, each kept twice (account::second_copy apart)
constexpr std::uint32_t facilities_expense = 0x8c, sampled_income = 0xbc;
}

// The facilities expense: booked on the ledger line (both copies) and taken off the balance.
void pay(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t amount) {
    const auto account = c.u32(manager) + c.u32(club + club_field::index) * account::size;
    for (const auto at : {account + ledger_line::facilities_expense,
                          account + ledger_line::facilities_expense + account::second_copy})
        c.put32(at, c.u32(at) + amount);
    const auto low = c.u32(account + 4), high = c.u32(account);
    c.put32(account + 4, low - amount);
    c.put32(account, high - (low < amount ? 1u : 0u));
}
}

void upgrade_facilities(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    const auto standing_at = club + club_field::facilities;
    const auto reputation = [&] { return std::int32_t(c.i16(club + club_field::reputation)); };
    const auto standing = [&] { return std::int32_t(c.i8(standing_at)); };

    // The step up, and its cost: 75000 plus 50000 for each level.
    const auto before = standing();
    std::uint32_t amount = std::uint32_t(before) * 50000u + 75000u;
    c.put8(standing_at, c.u8(standing_at) + (before < 0x13 ? 2u : 1u));
    pay(c, manager, club, amount);

    // A reputation well ahead of the standing catches it up in one or two extra payments.
    const auto now = standing();
    const auto raw = c.u8(standing_at);
    if (now < 0x14 && reputation() > 0x203a) {
        amount = std::uint32_t(0x14 - now) * 250000u + 500000u;
        if (std::int32_t(amount) < 2000000) amount = 2000000;
        c.put8(standing_at, 0x14);
        pay(c, manager, club, amount);
    } else if (now < 0xe && reputation() > 0x1482) {
        amount = std::uint32_t(0x14 - now) * 150000u + 500000u;
        // The floor is dead (0x100f9ff8..0x100f9ffc is the same compare for the 2,000,000 floor; here 0x100fa08c):
        // standing < 14 makes 20 - standing at least 7, so the amount is at least 7 * 150000 + 500000.
        if (std::int32_t(amount) < 1000000) amount = 1000000;
        c.put8(standing_at, c.u8(standing_at) + 4u);
        if (standing() < 0x10) c.put8(standing_at, 0x10);
        pay(c, manager, club, amount);
    } else if ((now < 0x10 && reputation() > 0x109a) || (now < 0x14 && reputation() > 0x1c52)) {
        if (reputation() > 0x1c52) {
            c.put8(standing_at, 0x14);
        } else {
            c.put8(standing_at, raw + 2u);
            if (standing() < 0xc) c.put8(standing_at, 0xc);
        }
        pay(c, manager, club, std::uint32_t(standing()) * 50000u + 75000u);
    }
    if (standing() > 0x14) c.put8(standing_at, 0x14);
}

namespace {
namespace pool {  // offsets in the double pool
constexpr std::uint32_t scale = 0x78, magic = 0x50, millis = 0x170;
// The factors by income class, by the club's pull (higher first).
constexpr std::uint32_t rich = 0x258, middle = 0x260, poor = 0x268, rich_in_league = 0x270;
}
// The words of a stored int-to-double: the high word 0x43300000 at `at`, the biased integer after it.
double stored_integer(Cpu& c, const Frame& frame, std::int32_t at, std::int32_t value) {
    c.put32(frame.at(at), 0x43300000);
    c.put32(frame.at(at + 4), std::uint32_t(value) ^ 0x80000000u);
    return c.f64(frame.at(at));
}
// fctiwz through the Fpu, stored to the frame and read back as the original does (stfd, lwz of the low word).
std::int32_t stored_truncation(Cpu& c, Fpu& fpu, const Frame& frame, std::int32_t at, double value) {
    c.put64(frame.at(at), fpu.fctiwz(0, value));
    return std::int32_t(c.u32(frame.at(at + 4)));
}
}

void book_sampled_income(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    Frame frame(c, 0x100fa1f0, 0x70, {.first_gpr = 26});
    Fpu fpu{c};
    const auto doubles = c.toc(toc::doubles);
    const auto base_cell = c.toc(sample_table::base), cursor_cell = c.toc(sample_table::cursor);
    const auto competition_cell = c.toc(-0x7a24);
    c.set({{26, manager}, {27, club}, {29, base_cell}, {30, cursor_cell}, {31, doubles}});
    const auto magic = [&] { return c.f64(doubles + pool::magic); };
    const std::int32_t income_scale = std::int32_t(c.u32(club + club_field::income_scale));

    // sum = trunc(factor * (reputation - m) * (income scale - m)), the factor by the club's pull.
    const auto sum_with = [&](std::uint32_t factor, std::int32_t reputation, bool first_layout) {
        const std::int32_t a_at = first_layout ? 0x40 : 0x38, b_at = first_layout ? 0x48 : 0x40, out_at = first_layout ? 0x38 : 0x48;
        const double a = stored_integer(c, frame, a_at, reputation);
        const double b = stored_integer(c, frame, b_at, income_scale);
        const double scaled_a = fpu.fsub(a, magic());
        const double scaled_b = fpu.fsub(b, magic());
        const double product = fpu.fmul(scaled_a, scaled_b);
        const double result = fpu.fmul(c.f64(doubles + factor), product);
        return stored_truncation(c, fpu, frame, out_at, result);
    };

    std::int32_t sum;
    const auto owner = c.u32(club + club_field::owner);
    const auto profile = owner ? c.u32(owner + 0x1c) : 0u;
    if (owner && profile) {
        // The owner's rating and the club's reputation pick the class.
        const std::int32_t trait = c.i8(profile + 0x2f);
        std::int32_t class_value;
        if (c.i8(club + club_field::board_flag) != 0) {
            class_value = std::int32_t(std::uint32_t(c.i16(club + club_field::reputation)) + std::uint32_t(trait) * 25u);
        } else {
            const std::int32_t reputation = c.i16(club + club_field::reputation);
            const auto value = std::int32_t(std::uint32_t(reputation) + std::uint32_t(trait) * 25u);
            const double stored = stored_integer(c, frame, 0x48, value);
            const double single = fpu.fsubs(stored, magic());
            class_value = stored_truncation(c, fpu, frame, 0x40, fpu.fmul(c.f64(doubles + pool::scale), single));
        }
        const std::int32_t reputation = c.i16(club + club_field::reputation);
        if (class_value > 0x157c) sum = sum_with(pool::rich, reputation, true);
        else sum = sum_with(class_value > 0xfa0 ? pool::middle : pool::poor, reputation, false);
    } else {
        const std::int32_t reputation = c.i16(club + club_field::reputation);
        const auto competition = c.u32(club + club_field::competition);
        const bool same = competition && c.u32(competition + 0x68) == c.u32(competition_cell);
        std::uint32_t factor;
        if (reputation > 0x1482) factor = same ? pool::rich_in_league : pool::rich;
        else if (reputation > 0xea6) factor = same ? pool::rich : pool::middle;
        else factor = same ? pool::middle : pool::poor;
        sum = sum_with(factor, reputation, false);
    }

    // The sample table's next draw, 0..10 above 95 percent.
    c.set(28, std::uint32_t(sum));
    const auto sample = std::int32_t(draw(c, 0x100fa610));
    const auto account = c.u32(manager) + c.u32(club + club_field::index) * account::size;
    const std::uint32_t before = c.u32(account + ledger_line::sampled_income);
    const std::int32_t quotient = sample / 11;
    const auto weight = std::uint32_t(sample) - std::uint32_t(quotient) * 11u + 0x5fu;
    const auto weighted = std::int32_t(std::uint32_t(sum) * weight);
    const double stored = stored_integer(c, frame, 0x38, weighted);
    const double scaled = fpu.fsub(stored, magic());
    const auto amount = std::uint32_t(stored_truncation(c, fpu, frame, 0x40, fpu.fmul(c.f64(doubles + pool::millis), scaled)));
    c.put32(account + ledger_line::sampled_income, before + amount);
    c.put32(account + ledger_line::sampled_income + account::second_copy,
            c.u32(account + ledger_line::sampled_income + account::second_copy) + amount);
    const std::uint32_t sign = std::int32_t(amount) < 0 ? 0xffffffffu : 0u;
    const auto low = c.u32(account + 4), high = c.u32(account);
    c.put32(account + 4, low + amount);
    c.put32(account, high + sign + (low + amount < low ? 1u : 0u));
}
}
