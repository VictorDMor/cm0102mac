#include "transfer_manager_native_status.hpp"
#include "transfer_manager_native_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t status_rows = 0x9a0, status_row_size = 10, row_history = 4, row_level = 6;
constexpr std::uint8_t status_flag_move = 0x20;
constexpr std::uint32_t key_at = 0x48, frame_toc_slot = 0x14;
constexpr std::uint32_t interest_rows = 0xa6c, interest_count = 0xa70;  // the manager's rows by person
constexpr std::uint32_t person_club = 0x0c, person_competition_row = 0x14;

// The original's signed comparison a < b of two halfword values, as xor/shift/and/subtract leave it.
std::uint32_t less_than(std::int32_t a, std::int32_t b) {
    const auto x = std::uint32_t(a ^ b);
    return (((std::uint32_t(std::int32_t(x) >> 1)) - (x & std::uint32_t(b))) >> 31) & 1;
}
// Whether the contract's term (the word at +0x30) ends before the offer's (halfwords +0x32, then +0x30).
bool term_ends_before(Cpu& c, std::uint32_t contract, std::uint32_t offer) {
    const auto term = c.u32(contract + 0x30);
    const std::uint16_t low = std::uint16_t(term), other = c.u16(offer + 0x32);
    if (low < other) return true;
    if (low > other) return false;
    return less_than(std::int16_t(term >> 16), c.i16(offer + 0x30)) != 0;
}
}

std::uint32_t person_refused(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t offer, std::uint32_t row,
                             std::uint32_t noisy) {
    Frame frame(c, 0x103ed520, 0xb0, {.first_gpr = 16});
    const auto fixed_date = c.toc(-0x7b80), rating_table = c.toc(-0x7a64), settings = c.toc(-0x4ba0),
               strings = c.toc(-0x4b9c), clubs_cell = c.toc(toc::clubs), limit_cell = c.toc(-0x7be4);
    const HalfwordDraw draw(c);
    keep(c, 21, rating_table);
    keep(c, 29, manager);
    keep(c, 22, fixed_date);
    keep(c, 30, person);
    keep(c, 23, draw.seed);
    keep(c, 20, offer);
    keep(c, 24, draw.start);
    keep(c, 16, row);
    keep(c, 25, draw.limit);
    keep(c, 31, noisy);
    keep(c, 26, draw.cursor);
    keep(c, 18, 0);
    keep(c, 27, settings);
    keep(c, 19, clubs_cell);
    keep(c, 28, strings);
    const auto history_row = [&] { return c.u32(manager + status_rows) + c.u32(person) * status_row_size; };
    const bool noisy_flag = std::int8_t(noisy) != 0;

    // 0x103ee1a0 is `li r3,1; blr`, so this never returns 1.
    if (std::uint8_t(invoke(c, 0x103ee1a0, 0x103ed570, {manager, person})) == 0) return 1;
    std::uint32_t term_contract = invoke(c, 0x10082f20, 0x103ed590, {c.u32(c.toc(toc::contracts)), person});
    keep(c, 17, term_contract);
    std::uint32_t other_contract = 0;
    if (term_contract) {
        // Bits 7..2 of the contract's byte +0x38 as a signed field: 2 marks the contract compared by its term.
        const auto kind = std::int32_t(std::uint32_t(c.u8(term_contract + 0x38) & 0xfe) << 24) >> 26;
        if (kind != 2) {
            other_contract = term_contract;
            keep(c, 18, other_contract);
            term_contract = 0;
            keep(c, 17, 0);
        }
    }
    // The club's interest row for the person.
    if (row == 0) {
        std::uint32_t found = 0;
        if (person == 0 || c.i32(person) < std::int32_t(c.u32(limit_cell))) {
            const auto index = c.u32(person);
            if (c.u8(history_row()) & status_flag_move) {
                c.put32(frame.at(key_at), index);
                found = invoke(c, 0x10775600, 0x103ed620,
                               {frame.at(key_at), c.u32(manager + interest_rows), c.u32(manager + interest_count), 0x10,
                                c.toc(-0x4ba8)});
            }
        }
        row = found;
        keep(c, 16, row);
    }
    if (row != 0) {
        const auto too_few_games = [&] {
            // More often than three times the games played, or fewer than four games.
            const std::uint32_t games = c.u16(row + 6), other = c.u16(row + 4);
            return std::int32_t(other) > std::int32_t(games * 3) || games < 4;
        };
        if (offer == 0) {
            if (too_few_games()) return 1;
        } else {
            bool check_games = term_contract != 0 && term_ends_before(c, term_contract, offer);
            if (!check_games && other_contract != 0 && c.i32(offer + 4) != c.i32(other_contract + 4)) check_games = true;
            if (check_games && too_few_games()) return 1;
            if (term_contract != 0 && !term_ends_before(c, term_contract, offer)) return 1;
        }
    }
    if (c.u32(person + 0x1c) != 0 && c.u32(person + person_competition_row) == 0)
        return less_than(0xa0, c.i16(c.u32(person + 0x1c) + 0x2c));

    const auto season_word = fixed_date + 2;
    keep(c, 18, season_word);
    for (const auto string_offset : {0x1a27u, 0x1a36u}) {
        if (c.u16(season_word) == 0x7d1 && c.u32(person + 0x28) != 0 &&
            invoke(c, 0x1077dda0, string_offset == 0x1a27u ? 0x103ed7c4 : 0x103ed820, {c.u32(person + 0x28), strings + string_offset}) != 0 &&
            offer != 0 &&
            invoke(c, 0x1077dc90, string_offset == 0x1a27u ? 0x103ed7f0 : 0x103ed84c,
                   {c.u32(clubs_cell) + c.u32(offer + 4) * club_size + 0x1ee, strings + 0x1a2e}) == 0)
            return 1;
    }
    if (noisy_flag) {
        if (c.u32(person + person_competition_row) != 0) {
            draw.advance(0x103ed890);
            const auto drawn = std::int32_t(draw.value());
            const auto margin = drawn - (drawn / 1000) * 1000 + 8000;
            if (std::int32_t(c.i16(c.u32(person + person_competition_row) + 0xc)) > margin) return 1;
        }
    } else if (c.u32(person + person_competition_row) != 0 && c.i16(c.u32(person + person_competition_row) + 0xc) > 0x2134) {
        return 1;
    }
    if (offer != 0 && c.i32(offer + 0xc) > 34999) return 1;
    const auto age = std::int8_t(c.i16(person + 0x5c));
    if (age < 0x18 && c.u32(person + person_competition_row) != 0 &&
        c.i16(c.u32(person + person_competition_row) + 6) > 0xbe)
        return 1;
    const auto potential = c.u8(person + 0x5e);
    if (potential < 5) return 0;
    if (potential > 0x14 && c.u32(person + person_club) != 0 && c.i16(c.u32(person + person_club) + 0xac) > 0xbb8 &&
        age < 0x20)
        return 1;

    // The person's level in the row's history: it moves up the history by the gap since the row's last level.
    const auto level = c.i16(c.u32(c.u32(rating_table) + 4) + c.u32(c.u32(person + person_club) + 0x68) * 0x74 + 2);
    const auto gap = std::int16_t(std::int32_t(level) - std::int32_t(c.i16(history_row() + row_level)));
    if (gap > 0) {
        const auto history = c.u16(history_row() + row_history);
        c.put16(history_row() + row_history, gap >= 16 ? 0 : std::uint32_t(history) << gap);
    }
    c.put16(history_row() + row_level, std::uint32_t(std::int32_t(level)));
    const auto year = c.u16(season_word);
    keep(c, 18, year);
    std::int32_t entries;
    if (std::int32_t(year) - 2 < 0x7d1) {
        entries = 0x10;
    } else {
        const auto league = c.u32(c.u32(rating_table) + 0x1c);
        keep(c, 17, league);
        const auto first = invoke(c, 0x10718360, 0x103edc44, {fixed_date});
        keep(c, 16, first);
        const auto second = invoke(c, 0x10718360, 0x103edc50, {fixed_date});
        entries = std::int16_t(invoke(c, 0x10487140, 0x103edc70,
                                      {league, c.u32(person + person_club), second, (year - 2u) & 0xffff, first, year}));
    }
    const std::int16_t sample = entries > 0x10 ? 0x10 : std::int16_t(entries);
    keep(c, 20, std::uint32_t(std::int32_t(sample)));
    keep(c, 19, 0);
    keep(c, 21, 0);
    keep(c, 18, 0x43300000);
    guest::Fpu fpu(c);
    std::int16_t hits = 0;
    // `i < 0x10` (0x103edd0c) always holds once `i < sample`: sample was capped to 16 at 0x103edc7c.
    for (std::int16_t i = 0; i < sample && i < 0x10; ++i) {
        const auto magic = std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(std::int32_t(i)) ^ 0x80000000u));
        const auto exponent = fpu.fsub(magic, c.f64(settings + 8));
        c.set_f(1, c.f64(settings + 0x30));
        c.set_f(2, exponent);
        invoke(c, 0x10780fc0, 0x103edcc4);
        c.set(2, c.u32(frame.at(frame_toc_slot)));
        const auto mask = std::int16_t(std::int32_t(fpu.fctiwz(0, c.f(1))));
        if ((std::int32_t(c.i16(history_row() + row_history)) & std::int32_t(mask)) != 0) {
            hits = std::int16_t(hits + 1);
            keep(c, 21, std::uint32_t(std::int32_t(hits)));
        }
        keep(c, 19, std::uint32_t(i) + 1);
    }
    if (!noisy_flag) {
        if (hits > 0xb) return 1;
        if (sample == 0) return 0;
        return (std::int32_t(hits) * 100) / std::int32_t(sample) > 0x46 ? 1 : 0;
    }
    draw.advance(0x103edd3c);
    const auto first_draw = draw.value();
    const auto first_third = std::int32_t(first_draw) % 3;
    keep(c, 16, std::uint32_t(first_third));
    draw.advance(0x103edd8c);
    const auto second_draw = std::int32_t(draw.value());
    keep(c, 18, std::uint32_t(std::int32_t(hits)));
    if (std::int32_t(hits) > 11 + first_third - second_draw % 3) return 1;
    if (sample == 0) return 0;
    draw.advance(0x103eddf8);
    const auto third_draw = std::int32_t(draw.value());
    const auto third_tenth = third_draw % 10;
    keep(c, 16, std::uint32_t(third_tenth));
    draw.advance(0x103ede4c);
    const auto fourth_draw = std::int32_t(draw.value());
    return (std::int32_t(hits) * 100) / std::int32_t(sample) > third_tenth + 0x46 - fourth_draw % 10 ? 1 : 0;
}
}
