// TRANSFER_MANAGER.CPP, block "plan": one player moves to the buyer (0x103efec0), third part: what the
// move does to the clubs' reputations and the person's own ratings, and the closing steps.
#include "transfer_manager_native_plan.hpp"
#include "transfer_manager_native_plan_move.hpp"
#include <bit>
#include <stdexcept>
#include <tuple>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
namespace {
constexpr std::uint32_t slot_news_made = 0x119, slot_negotiation = 0x11c,
                        slot_manager_flag = 0x120, slot_free_flag = 0x118;
constexpr std::uint32_t club_reputation = 0x1ec;

// The original's int-to-double: the magic exponent word with the integer's bits flipped at the top.
double magic_signed(std::int32_t x) {
    return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u));
}
double magic_unsigned(std::uint32_t x) { return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | x); }
std::int32_t low_word(std::uint64_t bits) { return std::int32_t(std::uint32_t(bits)); }
// `sample` taken as a remainder of the limit `n` (the original's divw, mullw, subf; the sign of the limit sets the sign of the answer).
std::int32_t remainder_in(std::uint32_t sample, std::int32_t n) {
    const auto quotient = n == 0 ? 0 : std::int32_t(std::int64_t(std::int32_t(sample)) / n);
    const auto rest = std::int32_t(sample - std::uint32_t(quotient) * std::uint32_t(n));
    return n > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
}

void PlayerMove::adjust_values() {
    std::int32_t held14 = 0;  // r14 as the original holds it: the person's value in 16 bits, sign extended
    const auto set14 = [&](std::int32_t value) { held14 = value; keep_signed(c, 14, value); };
    const auto record = [&] { return c.u32(person + 0x14); };
    const auto buyer = [&] { return club_at(buyer_id); };
    if (record() != 0 && std::int8_t(invoke(c, 0x100c7a20, 0x103f0e74, {c.u8(person + 0x62)})) != 0) {
        std::uint32_t paid;
        if (c.u32(person) == c.u32(offer + 0xc))
            paid = invoke(c, 0x10416710, 0x103f0ea4, {offer, 0, 0, 0, 1});
        else
            paid = invoke(c, 0x10084be0, 0x103f0ec4, {contracts(), person, club, 0, 0xffffffff, 0});
        // The buyer's reputation grows by a hundredth of the person's, the seller's shrinks by it.
        c.put16(buyer() + club_reputation, std::uint16_t(c.i16(buyer() + club_reputation) + std::int16_t(c.i16(record() + 0xc) / 100)));
        if (c.i16(buyer() + club_reputation) > 10000) c.put16(buyer() + club_reputation, 10000);
        if (std::int32_t(seller_id) > -1) {
            const auto seller = club_at(seller_id);
            c.put16(seller + club_reputation, std::uint16_t(c.i16(seller + club_reputation) - std::int16_t(c.i16(record() + 0xc) / 100)));
        }
        // The fee decides a bonus (r13) in three bands.
        const auto fee = std::int32_t(paid);
        std::int32_t bonus;
        if (fee > 5000000) {
            const auto scaled = fpu.fsub(magic_signed(fee), c.f64(pool + 8));
            bonus = low_word(fpu.fctiwz(0, fpu.multiply_add(0x103f0fa4, c.f64(pool + 0xe0), scaled, c.f64(pool + 0xd8))));
        } else if (fee > 2500000) {
            const auto scaled = fpu.fsub(magic_signed(fee), c.f64(pool + 8));
            bonus = low_word(fpu.fctiwz(0, fpu.fmul(c.f64(pool + 0xe8), scaled)));
        } else {
            const auto scaled = fpu.fsub(magic_signed(fee), c.f64(pool + 8));
            bonus = low_word(fpu.fctiwz(0, fpu.multiply_add(0x103f1020, c.f64(pool + 0xf0), scaled, c.f64(pool + 0xf8))));
        }
        keep_signed(c, 13, bonus);
        const auto bonus16 = std::int32_t(std::int16_t(bonus));
        if (std::int8_t(invoke(c, 0x1014c2e0, 0x103f1054, {buyer(), 0, 1, 0, 0})) != 0) {
            const auto third = std::int32_t(c.i16(buyer() + club_reputation)) / 3 - 0x2ee;
            set14(std::int16_t(third > bonus16 ? third : bonus16));
        } else {
            const auto half = std::int32_t(c.i16(buyer() + club_reputation)) / 2 - 0x3e8;
            set14(std::int16_t(half > bonus16 ? half : bonus16));
            if (c.i8(frame.at(slot_news_made)) != 0) {
                // A negotiation was held: the person's value is also bounded by what it came to.
                const auto staff = c.u32(club + 0x90);
                std::int32_t weight;
                if (staff != 0 && c.u32(staff + 0x1c) != 0)
                    weight = std::int32_t(c.i16(club + club_reputation)) / 200 + std::int32_t(c.i16(c.u32(staff + 0x1c) + 0x2a)) / 200;
                else
                    weight = std::int32_t(c.i16(club + club_reputation)) / 150;
                const auto negotiation = std::int32_t(c.u32(frame.at(slot_negotiation)));
                const auto left = std::int32_t(std::uint32_t(weight) * std::uint32_t(negotiation));
                const auto right = std::int32_t(std::uint32_t(c.i16(record() + 0xa)) * std::uint32_t(150 - weight));
                auto share = fpu.fsub(magic_signed(left), c.f64(pool + 8));
                share = fpu.fdiv(share, c.f64(pool + 0x100));
                auto other = fpu.fsub(magic_signed(right), c.f64(pool + 8));
                other = fpu.fadd(share, other);
                const auto bound = low_word(fpu.fctiwz(0, fpu.fdiv(other, c.f64(pool + 0x108))));
                set14(std::int16_t(held14 > bound ? held14 : bound));
            }
        }
        if (std::int16_t(held14) > 0x251c) set14(0x251c);
        // The reputation difference between the clubs, divided by the person's weight, is rolled out in three draws.
        std::int32_t difference;
        if (seller_id != 0xffffffff)
            difference = std::int32_t(c.i16(club + club_reputation)) - std::int32_t(c.i16(club_at(seller_id) + club_reputation));
        else
            difference = std::int32_t(c.i16(club + club_reputation)) - std::int32_t(c.i16(record() + 0xa));
        const auto positive = std::int16_t(difference > 0 ? difference : 0);
        const auto scale = std::int32_t(c.i8(person + 0x66)) * 0x14;
        const auto divisor = scale >= 1 ? scale : 1;
        std::int32_t share_a = std::int32_t(positive) / divisor;
        keep_signed(c, 13, share_a);
        std::uint32_t first_roll = 0;
        if (share_a != 0) {
            draw_advance(0x103f12a0);
            if (share_a > 0xffff || share_a < -0xffff)
                throw std::logic_error("0x103f12c8: a share past 16 bits cannot occur (|share| <= 32768)");
            first_roll = std::uint32_t(remainder_in(draw_value(), share_a));
        }
        keep(c, 21, first_roll);
        const auto roll = [&](std::int32_t limit, std::uint32_t back) {
            keep_signed(c, 13, limit);
            if (limit == 0) return std::uint32_t(0);
            draw_advance(back);
            if (limit > 0xffff || limit < -0xffff)
                throw std::logic_error("0x103f1434: a limit past 16 bits cannot occur (20 less a byte)");
            return std::uint32_t(remainder_in(draw_value(), limit));
        };
        const auto second_roll = roll(20 - std::int32_t(c.i8(person + 0x61)), 0x103f140c);
        keep(c, 22, second_roll);
        const auto third_roll = roll(20 - std::int32_t(c.i8(record() + 0x2f)), 0x103f157c);
        keep(c, 17, third_roll);
        const auto total = (third_roll + first_roll + second_roll) & 0xff;
        keep(c, 13, total);
        const auto owner_flag = c.u32(club + 0x2c) != 0 ? std::int32_t(c.i8(c.u32(club + 0x2c) + 0x119)) : 0;
        const auto form = std::int8_t(invoke(c, 0x103092f0, 0x103f16e8, {cell_value(-0x7b3c), person, club, std::uint32_t(owner_flag), 0}));
        // The person's form: a scaled mix of the draws and the form lookup, capped at 100, added to the history byte.
        auto base = fpu.fsub(magic_signed(form), c.f64(pool + 8));
        const auto divided = fpu.fdiv(base, c.f64(pool + 0x78));
        auto drawn = fpu.fsub(magic_unsigned(total), c.f64(pool + 0x128));
        const auto complement = fpu.fsub(c.f64(pool), divided);
        const auto mixed = fpu.fmul(drawn, complement);
        std::int32_t added = low_word(fpu.fctiwz(0, mixed));
        if (std::uint32_t(added & 0xff) > 0x64) added = 0x64;
        const auto history = c.u32(shortlists_cell) + c.u32(person) * 0x64;
        c.put8(history + 0xa, std::uint8_t(c.u8(history + 0xa) + added));
        // The record's value is pulled halfway towards the buyer's figure (at +8 if the person is the offer's own, else +0xc).
        const auto pull_from = c.u32(club + 0x2c) == c.u32(person + 0xc) ? 8u : 0xcu;
        c.put16(record() + 0xa, std::uint16_t(c.i16(record() + pull_from) / 2 + c.i16(record() + 0xa) / 2));
        // Then raised to at least what the person's value allows.
        const auto value16 = std::int32_t(std::int16_t(held14));
        const auto current = c.i16(record() + 0xa);
        c.put16(record() + 0xa, std::uint16_t(current > value16 ? current : std::int16_t(held14)));
        // Two floors of the person's own ratings, from the value and the buyer's reputation.
        for (const auto& [pool_scale, offset, minus, site] :
             {std::tuple{0x118u, 8u, 0x7d0, 0x103f1838u}, std::tuple{0x120u, 0xcu, 0x1f40, 0x103f18b4u}}) {
            const auto reputation_part = fpu.fsub(magic_signed(c.i16(club + club_reputation)), c.f64(pool + 8));
            const auto value_part = fpu.fsub(magic_signed(value16 - minus), c.f64(pool + 8));
            const auto limit = fpu.multiply_add(site, c.f64(pool + pool_scale), reputation_part, value_part);
            const auto ceiling = c.f64(pool + 0x110);
            fpu.fcmp(0, ceiling, limit);
            const auto chosen = ceiling > limit ? ceiling : limit;
            const auto converted = low_word(fpu.fctiwz(0, chosen));
            const auto old = std::int32_t(c.i16(record() + offset));
            c.put16(record() + offset, std::uint16_t(old > std::int32_t(std::int16_t(converted)) ? old : converted));
        }
    }
}

void PlayerMove::finish() {
    // The contract is marked as moved (flag 4 of its byte +0x5c) and the other managers are told.
    c.put8(contract + 0x5c, 4);
    c.put8(contract + 0x5c, std::uint8_t(c.u8(contract + 0x5c) | 4));
    invoke(c, 0x10083180, 0x103f1914, {contracts(), contract, c.u8(person + 0x62)});
    invoke(c, 0x10311bc0, 0x103f1928, {cell_value(-0x7b1c), person, club});
    invoke(c, 0x1007bd80, 0x103f193c, {contracts(), person, 0, 0});
    const auto account = invoke(c, 0x10103510, 0x103f194c, {cell_value(-0x7ab8), club});
    invoke(c, 0x100f7130, 0x103f1954, {account, c.u32(contract + 8)});
    if (c.u32(person + 0x14) != 0) {
        const bool club_open = std::int8_t(invoke(c, 0x1014c2e0, 0x103f1978, {club, 0, 1, 0, 0})) != 0;
        const auto value = invoke(c, 0x10084be0, club_open ? 0x103f199c : 0x103f19c0,
                                  {contracts(), person, club, 0, 0xffffffff, club_open ? 1u : 0u});
        c.put32(person + 8, value);
        invoke(c, 0x10363b80, 0x103f19d4, {manager_table(), person, 1});
    }
    if (std::int8_t(invoke(c, 0x100c7a20, 0x103f19dc, {c.u8(person + 0x62)})) != 0 &&
        std::int8_t(invoke(c, 0x104ae570, 0x103f19f8, {cell_value(-0x7a98), club, 0})) != 0)
        invoke(c, 0x104ade40, 0x103f1a18, {cell_value(-0x7a98), person, club, c.u8(frame.at(slot_manager_flag))});
    // Where the person now stands in the club's lists.
    const auto list_kind = [&](std::uint32_t kind, std::uint32_t back) { invoke(c, 0x100ce200, back, {person, kind}); };
    if (std::int8_t(invoke(c, 0x1014c2e0, 0x103f1a30, {club, 1, 1, 0, 0})) != 0 &&
        invoke(c, 0x100ce320, 0x103f1a48, {club, 0, 1}) == 0 &&
        std::int8_t(invoke(c, 0x100d12d0, 0x103f1a58, {club})) != 0) {
        const auto grade = std::int32_t(c.u8(contract + 0x60) & 0xf);
        if (grade >= 5 || grade < 1) list_kind(2, 0x103f1a98);
        else list_kind(1, 0x103f1a88);
    } else {
        list_kind(1, 0x103f1aa8);
    }
    invoke_native(c, 0x103edef0, 0x103f1ab8, {manager, person, club});
    const auto history = c.u32(shortlists_cell) + c.u32(person) * 0x64 + 0xc;
    c.put8(history, std::uint8_t(c.u8(history) & ~0x0c));
    c.put8(c.u32(manager + 0x9a0) + c.u32(person) * 10 + 8, c.u8(person + 0x5e));
    invoke(c, 0x10006600, 0x103f1b00, {cell_value(-0x7b24), person});
    invoke(c, 0x1069f270, 0x103f1b1c, {cell_value(-0x7b94), person, club, person_club, c.u8(frame.at(slot_free_flag))});
}
}
