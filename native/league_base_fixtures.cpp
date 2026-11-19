// LEAGUE.CPP, block `fixtures`: 0x104a4970, the order of the standings rows for the fixture list. The original
// works on three allocations: `used[n]` (a row of the standings has been placed), `taken[n]` (a slot of the
// scratch copy is filled) and `copy[n]` (0x44-byte rows). It shuffles the standings in place with two draws per
// row, reseeds, then fills `copy` by one of two rules and writes it back over the standings.
//
// The draws: the buffered stream (cursor/limit/start/seed cells, TOC -0x7b84/-0x7b88/-0x7b8c/-0x7b90) whose
// refill calls rand, and rand itself twice per shuffled row. srand is called with the season's seed and again
// with a seed taken from the stream before the shuffle.
//
// Registers: the original keeps its state in r13-r31 and the allocator, the RNG, the virtuals and the assertion
// can reach imports, where a replay compares r13-r31. Each call below is preceded by the registers the original
// holds there; the virtual calls also read r5-r10 (the glue's mask), so the ones the original builds by its own
// loops are set as it leaves them.
#include "league_base_detail.hpp"
#include "league_base_fixtures.hpp"
#include <array>
#include <optional>

namespace cm::aot::league_base {
namespace {
namespace fixture {
// The shared stream with the leftover this routine's inline step leaves: r4 holds the stepped cursor.
void advance_stream(Cpu& c, std::uint32_t back) {
    const auto pool = Pool::at(c);
    c.set(4, c.u32(pool.cursor) + 2);
    pool.advance(back);
}

// TOC cells of the game's globals this routine reads (word = address of the value, unless noted).
inline constexpr std::int32_t
    stream_cursor = -0x7b84, season_seed = -0x7450, derby_competition = -0x67c0, derby_club_a = -0x7a04,
    derby_club_b = -0x7a08;
inline constexpr std::uint32_t rand_entry = 0x1077b770, srand_entry = 0x1077b790, allocate_entry = 0x107735f0,
                               release_entry = 0x10773610;
inline constexpr std::uint32_t club_region = 0x3c;      // a club's pointer to its region record (inferred)
inline constexpr std::uint32_t region_parent = 0x14;    // a region's pointer to its parent region (inferred)
inline constexpr std::uint32_t slot_for_club = 0xb0, slot_final_place = 0xb4;  // the league's virtual slots
inline constexpr std::uint32_t preset_flag = 0x100;     // bit of the flag word (+0xdc): the league presets its rows

// rand * count / 32768, truncated toward zero, as the original's mulhw/rlwimi/srawi/addze sequence (low 32 bits).
std::uint32_t scale(std::uint32_t draw, std::int32_t count) {
    const std::int64_t product = std::int64_t(std::int32_t(draw)) * count;
    return std::uint32_t(product / 32768);
}
}
}

void draw_fixture_order(Cpu& c, std::uint32_t league, std::int32_t pairs) {
    using namespace fixture;
    const Frame frame(c, 0x104a4970, 0x130, {.first_gpr = 13});
    const auto message = c.toc(toc::message), pool = c.toc(toc::pool);
    const auto count = [&] { return std::int32_t(c.i16(league + competition::club_count)); };
    const auto standings = [&] { return c.u32(league + league::standings); };
    const auto row_at = [](std::uint32_t base, std::uint32_t index) { return base + index * league::standing_row; };
    const auto positive = [&] { return std::uint32_t(count() > 0 ? count() : 0); };  // a counted loop's end value
    const std::int32_t half = count() / 2;
    // r22 the message buffer, r23 the pool, r24 the league, r25 the argument, r30 half the clubs.
    c.set({{22, message}, {23, pool}, {24, league}, {25, std::uint32_t(pairs)}, {30, std::uint32_t(half)}});
    const auto used = invoke(c, allocate_entry, 0x104a49a4, {std::uint32_t(count())});
    c.set(29, used);
    const auto taken = invoke(c, allocate_entry, 0x104a49b4, {std::uint32_t(count())});
    c.set(28, taken);
    const auto copy = invoke(c, allocate_entry, 0x104a49c4, {std::uint32_t(count()) * league::standing_row});
    c.set({{27, copy}, {26, 0}});
    if (!copy || !used || !taken) {
        report(c, frame, {0x104a49f4, 0x90, 0x48, 0xed3});
        c.set(4, c.toc(toc::success));  // the original loads the flag's address into r4 to clear it
        invoke(c, release_entry, 0x104a4a3c, {taken});
        invoke(c, release_entry, 0x104a4a44, {used});
        invoke(c, release_entry, 0x104a4a4c, {copy});
        return;
    }
    // r4-r10 are scratch in the original, but its generated callers read r4-r10 after it returns and the callees
    // it calls pass them through: the code's own definitions are mirrored below where it makes them.
    for (std::int32_t n = 0; n < count(); ++n) {
        c.put8(used + std::uint32_t(n), 0);
        c.put8(taken + std::uint32_t(n), 0);
    }
    c.set({{4, used + positive()}, {5, 0}, {6, positive()}});
    // The seed to restore after the shuffle: the stream's sample reduced to 1..30000.
    advance_stream(c, 0x104a4ab8);
    const auto sample = std::int32_t(Pool::at(c).sample());
    const auto quotient = sample / 30000;
    const auto remainder = std::uint32_t(sample) - std::uint32_t(quotient) * 30000u;
    const auto restored = remainder + 1;
    const auto season = c.u32(c.toc(season_seed)) + std::uint32_t(std::int32_t(c.i16(league + competition::year)));
    c.set({{4, remainder}, {5, 0x45e7b273u}, {6, std::uint32_t(sample)}});
    invoke(c, srand_entry, 0x104a4b18, {season});
    // The shuffle: each row swaps with a row drawn at random (two draws per row). The registers hold the
    // swap's leftovers at the next draw: the first row's bytes 0x27, 0x26 and 0x40..0x43, its halfword at 0x28,
    // the second row's offset and the first row's address, and its halfwords/bytes 0x2c..0x38 in r4-r10.
    c.set(31, 0);
    for (std::int32_t i = 0; i < count(); c.set(31, std::uint32_t(++i))) {
        const auto first_draw = invoke(c, rand_entry, 0x104a4b24);
        const auto first_product = std::int64_t(std::int32_t(first_draw)) * count();
        const auto first_index = scale(first_draw, count());
        c.set({{13, first_index}, {4, std::uint32_t(first_product)}});
        const auto second_draw = invoke(c, rand_entry, 0x104a4b4c);
        const auto first_row = row_at(standings(), first_index);
        const auto second_offset = scale(second_draw, count()) * league::standing_row;
        const auto saved = read_row(c, first_row);
        const auto second_row = standings() + second_offset;
        copy_row(c, second_row, first_row);
        write_row(c, standings() + second_offset, saved);
        c.set({{13, saved[16]}, {14, std::uint32_t(std::int32_t(std::int16_t(saved[17])))}, {15, saved[15]},
               {16, second_offset}, {17, first_row}, {18, saved[31]}, {19, saved[30]}, {20, saved[29]}, {21, saved[28]}});
        c.set({{4, std::uint32_t(std::int32_t(std::int16_t(saved[19])))}, {5, saved[20]}, {6, saved[21]}, {7, saved[22]},
               {8, saved[23]}, {9, saved[24]}, {10, std::uint32_t(std::int32_t(std::int16_t(saved[25])))}});
    }
    invoke(c, srand_entry, 0x104a4e20, {restored});
    const auto half_pairs = pairs / 2;  // the original's argument halved, rounding toward zero
    std::uint32_t placed = 0;           // r26: pairs placed by the first rules
    if (c.u16(league + league::flags) & preset_flag) {
        // The derived league presets each club's place.
        for (std::int32_t n = 0; n < count(); ++n) c.put32(copy + std::uint32_t(n) * league::standing_row, 0);
        c.set({{4, 0}, {5, positive()}});
        for (std::int32_t i = 0; i < count(); ++i) {
            const auto offset = std::uint32_t(i) * league::standing_row;
            c.set({{13, offset}, {14, std::uint32_t(i)}});
            const auto club = c.u32(standings() + offset);
            const auto place = sext8(virtual_call(c, slot_for_club, 0x104a4e7c, {league, club}));
            if (std::int32_t(place) >= 0 && std::int32_t(place) < count() &&
                c.u32(row_at(copy, place)) == 0) {
                const auto source = standings() + offset;
                copy_row(c, source, row_at(copy, place));
                c.put32(standings() + offset, 0);
                c.set(5, c.u8(source + 0x42));
            }
        }
        // The rows the virtual did not place go into the first free slots. r6-r9 keep the search's leftovers.
        std::optional<std::uint32_t> search_r7, search_r9;
        std::int32_t row = 0;
        for (; row < count(); ++row) {
            const auto source = standings() + std::uint32_t(row) * league::standing_row;
            if (c.u32(source) == 0) continue;
            const auto limit = count();
            search_r7 = 0;
            search_r9 = 0;
            if (limit <= 0) continue;
            for (std::int32_t slot = 0; slot < limit; ++slot) {
                if (c.u32(row_at(copy, std::uint32_t(slot))) == 0) {
                    search_r7 = c.u32(source);
                    search_r9 = std::uint32_t(slot);
                    copy_row(c, source, row_at(copy, std::uint32_t(slot)));
                    break;
                }
                *search_r7 += league::standing_row;
                *search_r9 += 1;
            }
        }
        c.set({{6, std::uint32_t(row)}, {8, std::uint32_t(count())}});
        if (search_r7) c.set({{7, *search_r7}, {9, *search_r9}});
        c.set({{13, copy}, {14, 0}});
        for (std::int32_t i = 0; i < count(); ++i) {
            const auto at = copy + std::uint32_t(i) * league::standing_row;
            c.set({{13, at}, {14, std::uint32_t(i)}});
            virtual_call(c, slot_final_place, 0x104a516c, {league, c.u32(at), sext8(std::uint32_t(i))});
        }
        c.set({{13, copy + positive() * league::standing_row}, {14, positive()}});
    } else {
        c.set(4, c.u32(c.u32(league + competition::record)));
        if (c.u32(c.u32(league + competition::record)) == c.u32(c.toc(derby_competition))) {
            const auto id_a = c.u32(c.toc(derby_club_a)), id_b = c.u32(c.toc(derby_club_b));
            std::uint32_t found_a = 0xffffffffu, found_b = 0xffffffffu;  // r13, r14
            std::int32_t index = 0;
            std::uint32_t offset = 0;
            for (; std::int8_t(index) < count(); ++index, offset += league::standing_row) {
                const auto id = c.u32(c.u32(standings() + offset));
                if (id_a == id) found_a = std::uint32_t(index);
                if (id_b == id) found_b = std::uint32_t(index);
            }
            c.set({{13, found_a}, {14, found_b}, {4, id_b}, {5, id_a}, {6, offset}, {7, std::uint32_t(index)}});
            if (std::int8_t(found_b) != -1 && std::int8_t(found_a) != -1) {
                advance_stream(c, 0x104a5248);
                c.set(4, c.u32(c.toc(stream_cursor)));
                const auto draw = std::int32_t(Pool::at(c).sample());
                const bool even = (draw & 1) == 0;
                // Even: the second club leads the pair (slot 0), the first goes to slot pairs/2; odd: reverse.
                const auto lead = sext8(even ? found_b : found_a), follow = sext8(even ? found_a : found_b);
                const auto lead_row = row_at(standings(), lead);
                copy_row(c, lead_row, copy);
                copy_row(c, row_at(standings(), follow), row_at(copy, std::uint32_t(half_pairs)));
                c.put8(taken + std::uint32_t(half_pairs), 1);
                placed = 1;
                c.put8(taken, 1);
                c.put8(used + sext8(found_a), 1);
                c.put8(used + sext8(found_b), 1);
                c.set({{4, std::uint32_t(half_pairs)}, {5, 1}, {6, c.u8(lead_row + 0x42)}});
            }
        }
        // Two passes over the rows not yet placed, each pairing a row with the first later unplaced row that fits
        // the rule: the same region record (pass 1), or one region the other's parent (pass 2). r13-r17 and r4-r9
        // follow the original's loops (pass 1 keeps the offset in r8, pass 2 in r7 and the standings in r8).
        for (int pass = 1; pass <= 2; ++pass) {
            const CheckSite site = pass == 1 ? CheckSite{0x104a57b0, 0x70, 0x40, 0xf76} : CheckSite{0x104a5b18, 0x50, 0x38, 0xfa3};
            const auto offset_register = pass == 1 ? 8u : 7u;
            const auto loop_registers = [&](std::int32_t index) {
                c.set({{13, std::uint32_t(index)}, {14, taken + placed}, {15, used + std::uint32_t(index)},
                       {16, std::uint32_t(index) * league::standing_row}, {17, placed * league::standing_row}, {26, placed}});
            };
            const auto cursor_registers = [&](std::int32_t j) {
                c.set({{6, std::uint32_t(j)}, {offset_register, std::uint32_t(j) * league::standing_row}, {4, used + std::uint32_t(j)}});
            };
            std::int32_t i = 0;
            for (; i < count() - 1; ++i) {
                if (c.u8(used + std::uint32_t(i)) != 0) continue;
                const std::int32_t length = count();
                bool left = false;
                std::int32_t j = i + 1;
                for (; j < length; ++j) {
                    cursor_registers(j);
                    if (c.u8(used + std::uint32_t(j)) != 0) continue;
                    const auto base = standings();
                    const auto club_i = c.u32(base + std::uint32_t(i) * league::standing_row);
                    const auto key_i = c.u32(club_i + club_region);
                    if (pass == 1) c.set({{5, base}, {7, club_i}, {9, key_i}});
                    else c.set({{8, base}, {5, club_i}, {9, key_i}});
                    if (key_i == 0) { if (pass == 1) continue; left = true; break; }
                    const auto club_j = c.u32(base + std::uint32_t(j) * league::standing_row);
                    const auto key_j = c.u32(club_j + club_region);
                    c.set(5, pass == 1 ? club_j : key_j);
                    if (key_j == 0) { if (pass == 1) continue; left = true; break; }
                    if (pass == 1 ? key_i != key_j : (c.u32(key_i + region_parent) != key_j && c.u32(key_j + region_parent) != key_i))
                        continue;
                    left = true;
                    if (std::int32_t(placed) < half) {
                        const auto slot = placed + std::uint32_t(half_pairs);
                        const auto row_i = base + std::uint32_t(i) * league::standing_row;
                        copy_row(c, row_i, row_at(copy, placed));
                        const auto row_j = standings() + std::uint32_t(j) * league::standing_row;
                        copy_row(c, row_j, row_at(copy, slot));
                        c.put8(taken + slot, 1);
                        c.put8(taken + placed, 1);
                        ++placed;
                        c.put8(used + std::uint32_t(j), 1);
                        c.put8(used + std::uint32_t(i), 1);
                        c.set({{4, 1}, {5, row_at(copy, slot)}, {6, std::uint32_t(j)}, {7, c.u8(row_j + 0x42)}, {8, c.u8(row_i + 0x42)}});
                    } else {
                        loop_registers(i);
                        report(c, frame, {site.at, site.name, site.extension, site.line});
                    }
                    break;
                }
                if (!left) cursor_registers(length);  // the counted loop ran out: its cursors stepped once more
            }
            loop_registers(i);
        }
        // The rows still unplaced go to the first free slots, in order.
        c.set({{6, 0}, {8, used}, {4, 0}});
        std::int32_t i = 0;
        for (; (c.set(5, std::uint32_t(count())), i < count()); ++i) {
            if (c.u8(used + std::uint32_t(i)) == 0) {
                const auto slots = count();
                c.set(7, 0);
                std::int32_t slot = 0;
                for (; slot < slots; ++slot) {
                    if (c.u8(taken + std::uint32_t(slot)) != 0) continue;
                    const auto source = standings() + std::uint32_t(i) * league::standing_row;
                    copy_row(c, source, row_at(copy, std::uint32_t(slot)));
                    c.put8(taken + std::uint32_t(slot), 1);
                    c.put8(used + std::uint32_t(i), 1);
                    c.set({{7, std::uint32_t(slot)}, {9, c.u8(source + 0x42)}});
                    break;
                }
                if (slot == slots && slots > 0) c.set(7, std::uint32_t(slots));
            }
            c.set({{4, std::uint32_t(i + 1) * league::standing_row}, {6, std::uint32_t(i + 1)}, {8, used + std::uint32_t(i + 1)}});
        }
    }
    c.set({{6, 0}, {4, 0}});
    for (std::int32_t i = 0; i < count(); ++i) {
        const auto at = row_at(copy, std::uint32_t(i));
        c.set({{5, at}, {6, std::uint32_t(i + 1)}, {4, std::uint32_t(i + 1) * league::standing_row}});
        copy_row(c, at, row_at(standings(), std::uint32_t(i)));
    }
    c.set(26, placed);
    invoke(c, release_entry, 0x104a6090, {taken});
    invoke(c, release_entry, 0x104a6098, {used});
    invoke(c, release_entry, 0x104a60a0, {copy});
}

}
