// COMP.CPP, block `stages`: the stage walk and the club queries (comp_stages.hpp). Each routine keeps the
// original's order of reads, calls and stores; where a routine's registers reach a call that can reach an
// import (the assertion, the allocator, the virtual calls) the original's non-volatile registers are set first.
#include "comp_detail.hpp"
#include "comp_stages.hpp"

namespace cm::aot::comp {
namespace {
// TOC cells this block reads (comp_detail.hpp has the shared ones).
namespace cell {
inline constexpr std::int32_t
    reserved_clubs = -0x7b04,  // cell: subtracted from the club count (0x10451570 tests a club's id against the difference) (inferred)
    stats_owner = -0x7b34;     // cell: the object 0x104abf40 asks about a competition record (inferred)
}
// Offsets of a stage's rows and fields used here (comp_detail.hpp's competition:: names the rest).
constexpr std::uint32_t round_count = 0x1c, round_number = 0x08;  // inside a 0x78-byte round row: matches (halfword), the round (halfword)
constexpr std::uint32_t round_kind_back = 0x70;                    // the round row's halfword 0x70 bytes before the end of the table (kind 0x8c)
constexpr std::uint32_t entry_status = 0x40, entry_a = 0x42, entry_b = 0x43;  // a knockout row's bytes (inferred: a status the routines test, and two bytes that are compared)

bool low_byte_set(std::uint32_t result) { return std::int8_t(result) != 0; }
}

void club_finish(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t code_out, std::uint32_t round_out,
                 std::uint32_t flag_out) {
    const Frame frame(c, 0x10450710, 0x60, {.first_gpr = 22});
    c.set({{25, object}, {26, club}, {27, code_out}, {28, round_out}, {29, flag_out}});
    for (std::int32_t index = c.i32(object + competition::last_stage); index >= -1; --index) {
        c.set({{30, std::uint32_t(index)}, {31, std::uint32_t(index) << 2}});
        const auto stage = stage_unchecked(c, object, index);
        if (c.i16(stage + competition::code) == 0x3fc) continue;
        if (c.u8(stage + competition::kind) == competition::knockout) {
            // The counter is a char: r22 counts on and r24 is its sign-extended byte.
            c.set(23, stage);
            for (std::int32_t counter = 0;; ++counter) {
                const std::int32_t row = std::int8_t(counter);
                c.set({{22, std::uint32_t(counter)}, {24, std::uint32_t(row)}});
                if (row >= c.i16(stage + competition::club_count)) break;
                const auto entry = invoke(c, 0x1049ea00, 0x1045078c, {stage, std::uint32_t(row)});
                if (c.u32(entry) == club) {
                    c.put16(code_out, c.u16(stage + competition::code));
                    const auto again = invoke(c, 0x1049ea00, 0x104507ac, {stage, std::uint32_t(row)});
                    c.put8(flag_out, c.u8(again + entry_status));
                    return;
                }
            }
            continue;
        }
        // A league stage: every row of the club writes the code and its byte (the last one stays), then the
        // played rounds are searched from the current one back for a match row that names the club's row.
        std::int32_t found = -1;
        for (std::int32_t n = 0; std::int16_t(n) < c.i16(stage + competition::club_count); ++n) {
            const auto offset = std::uint32_t(n) * competition::league_row;
            if (c.u32(c.u32(stage + competition::table) + offset) != club) continue;
            c.put16(code_out, c.u16(stage + competition::code));
            c.put8(flag_out, c.u8(c.u32(stage + competition::table) + offset + 5));
            found = n;
        }
        found = std::int16_t(found);
        if (found == -1) continue;
        for (std::int32_t round = std::int16_t(std::int8_t(c.u8(stage + competition::current_round)) - 1); round >= 0; --round) {
            // r11 (the round) counts down as an int; every use takes its low halfword.
            const auto row = std::uint32_t(round) * competition::round_row;
            std::uint32_t at = 0;
            for (std::int32_t m = 0;; ++m, at += competition::match_row) {
                const auto rounds = c.u32(stage + competition::round_table);
                if (std::int16_t(m) >= c.i16(rounds + row + round_count)) break;
                const auto matches = c.u32(c.u32(stage + competition::round_matches) + (std::uint32_t(round) << 2)) + at;
                if (found == c.i16(matches) || found == c.i16(matches + 2)) {
                    c.put16(round_out, c.u16(rounds + row + round_number));
                    return;
                }
            }
        }
    }
}

bool all_done(Cpu& c, std::uint32_t object) {
    const Frame frame(c, 0x10450900, 0x40, {.first_gpr = 30});
    c.set({{30, object}, {31, c.toc(toc::competitions)}});
    if (c.i32(object + competition::parent) > -1) return false;
    if (!low_byte_set(virtual_call(c, 0x18, 0x10450944, {object, 1, 1}))) return false;
    for (auto next = c.i32(object + competition::next); next >= 0; next = c.i32(object + competition::next)) {
        object = competition_object(c, std::uint32_t(next));
        c.set(30, object);
        if (!low_byte_set(virtual_call(c, 0x18, 0x10450984, {object, 1, 1}))) return false;
    }
    return true;
}

std::uint32_t current_stage(Cpu& c, std::uint32_t object) {
    const Frame frame(c, 0x10450a50, 0x50, {.first_gpr = 28});
    std::uint32_t found = 0;
    std::int32_t index = -1;
    c.set({{28, object}, {31, found}});
    // r30 holds the caller's value until the first stage is fetched.
    for (; index <= c.i32(object + competition::last_stage); ++index) {
        c.set({{28, object}, {29, std::uint32_t(index)}, {31, found}});
        c.link(0x10450a8c);
        const auto stage = comp::stage(c, object, index);
        c.set(30, stage);
        if (!low_byte_set(virtual_call(c, 0x18, 0x10450aa4, {stage, 0, 1}))) {
            found = stage;
            break;
        }
    }
    c.set({{28, object}, {29, std::uint32_t(index)}, {31, found}});
    if (found == 0) {
        c.link(0x10450ae0);
        const auto first = first_stage_of_group(c, object, c.i32(object + competition::last_stage));
        c.set({{28, object}, {29, std::uint32_t(index)}, {31, found}});
        c.link(0x10450aec);
        found = comp::stage(c, object, first);
    } else if (c.u8(found + competition::kind) == competition::league && c.i8(found + competition::current_round) == 0 && index > -1) {
        c.link(0x10450b20);
        const auto first = first_stage_of_group(c, object, index - 1);
        c.set({{28, object}, {29, std::uint32_t(index)}, {31, found}});
        c.link(0x10450b2c);
        found = comp::stage(c, object, first);
    }
    return found ? found : object;
}

std::int32_t first_stage_of_group(Cpu& c, std::uint32_t object, std::int32_t index) {
    const Frame frame(c, 0x10450c20, 0xb0, {.first_gpr = 25});
    const auto success = c.toc(toc::success), message = c.toc(toc::message), pool = c.toc(toc::pool);
    // r26: the success flag's address, r27: the message buffer, r28: the pool, r30: the competition, r31: the index.
    c.set({{26, success}, {27, message}, {28, pool}, {30, object}, {31, std::uint32_t(index)}});
    const auto first = checked_stage(c, frame, object, index, {0x10450c6c, 0x68, 0x40, 0x36e});
    const auto group = std::int32_t(invoke(c, 0x10464010, 0x10450ccc, {std::uint32_t(std::int32_t(c.i16(first + competition::code)))}));
    c.set(29, std::uint32_t(group));
    if (group > -1) {
        for (; index > -1; --index) {
            c.set({{25, std::uint32_t(index) << 2}, {29, std::uint32_t(group)}, {31, std::uint32_t(index)}});
            const auto previous = checked_stage(c, frame, object, index - 1, {0x10450d20, 0x48, 0x38, 0x36e});
            const auto other = std::int32_t(invoke(c, 0x10464010, 0x10450d84, {std::uint32_t(std::int32_t(c.i16(previous + competition::code)))}));
            if (group != other) break;
        }
    }
    return index;
}

std::uint32_t round_played(Cpu& c, std::uint32_t object, std::int32_t index, std::int32_t round) {
    const Frame frame(c, 0x10450db0, 0x70, {.first_gpr = 29});
    c.set({{29, std::uint32_t(round)}, {30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    if (index > c.i32(object + competition::last_stage)) return 0;
    // The first test makes the second's "index > last" dead; only an index below -1 asserts (the stage is then 0).
    const auto stage = checked_stage(c, frame, object, index, {0x10450e04, 0x40, 0x38, 0x36e});
    if (c.u8(stage + competition::kind) == competition::knockout) return 1;
    const std::int32_t current = c.i8(stage + competition::current_round);
    if (round < current) return 1;
    if (round != current) return 0;
    const std::int32_t rounds = c.i16(stage + competition::rounds);
    if (current != rounds - 1) return 0;
    const auto last_round = c.u32(stage + competition::round_table) + std::uint32_t(rounds) * competition::round_row;
    return c.i16(last_round - round_kind_back) == 0x8c ? 1 : 0;
}

bool takes_part(Cpu& c, std::uint32_t object, std::uint32_t club) {
    const auto record = c.u32(object + competition::record);
    const auto nation = c.u32(club + 0x2c);  // the club's nation record (inferred)
    const std::uint32_t known = c.u32(c.toc(toc::club_count)) - c.u32(c.toc(cell::reserved_clubs));
    if (c.i32(club) < std::int32_t(known)) {
        if (nation && nation == c.u32(record + record::nation) && c.u32(club + 0x30) && c.u32(club + 0x30) == record) return true;
        if (nation && c.u32(nation + 0x74) != c.u32(record + record::continent)) return false;
    } else {
        const auto continent = c.u32(record + record::continent);
        if (!(continent && nation && continent == c.u32(nation + 0x74)) && continent) return false;
    }
    // r9: the latest league stage seen, r10: the latest knockout stage seen. Once a knockout stage has been
    // seen, only it is searched in every later stage.
    std::uint32_t league = 0, knockout = 0;
    for (std::int32_t index = -1; index <= c.i32(object + competition::last_stage); ++index) {
        const auto stage = stage_unchecked(c, object, index);
        (c.u8(stage + competition::kind) == competition::knockout ? knockout : league) = stage;
        if (knockout) {
            for (std::int32_t n = 0, at = 0; std::int16_t(n) < c.i16(knockout + competition::club_count); ++n, at += competition::entry_row)
                if (c.u32(c.u32(knockout + competition::entries) + std::uint32_t(at)) == club) return true;
        } else {
            for (std::int32_t n = 0, at = 0; std::int16_t(n) < c.i16(league + competition::club_count); ++n, at += competition::league_row)
                if (c.u32(c.u32(league + competition::table) + std::uint32_t(at)) == club) return true;
        }
    }
    return false;
}

std::int32_t all_clubs(Cpu& c, std::uint32_t object, std::uint32_t out) {
    const Frame frame(c, 0x10451d10, 0x80, {.first_gpr = 27});
    // r27: the message buffer, r28: the pool, r29: the competition, r30: the output address, r31: the count.
    c.set({{27, c.toc(toc::message)}, {28, c.toc(toc::pool)}, {29, object}, {30, out}, {31, 0}});
    const auto clubs = invoke(c, callee::allocate_block, 0x10451d3c, {0x320});
    if (!clubs) {
        report(c, frame, {0x10451d58, 0x40, 0x38, 0x67f});
        c.put32(out, 0);
        return 0;
    }
    std::int32_t count = 0;
    // The stage counter is a char (r6 counts on, its low byte is tested); the stage offset is the int's.
    for (std::int32_t n = -1; std::int8_t(n) <= c.i32(object + competition::last_stage); ++n) {
        const auto stage = std::int8_t(n) == -1 ? object : c.u32(c.u32(object + competition::stages) + (std::uint32_t(n) << 2));
        const bool knockout = c.u8(stage + competition::kind) == competition::knockout;
        const auto rows = knockout ? competition::entries : competition::table;
        const auto step = knockout ? competition::entry_row : competition::league_row;
        for (std::int32_t row = 0, at = 0; std::int16_t(row) < c.i16(stage + competition::club_count); ++row, at += step) {
            const auto club = c.u32(c.u32(stage + rows) + std::uint32_t(at));
            if (!club) continue;
            bool seen = false;
            for (std::int32_t k = 0; std::int16_t(k) < std::int16_t(count); ++k)
                if (c.u32(clubs + std::uint32_t(k) * 4) == club) { seen = true; break; }
            if (seen || std::int16_t(count) >= 200) continue;
            c.put32(clubs + (std::uint32_t(std::int16_t(count)) << 2), club);
            ++count;
        }
    }
    c.put32(out, clubs);
    return count;
}

std::uint32_t club_still_in(Cpu& c, std::uint32_t object, std::uint32_t club) {
    // The match engine calls this (CLAUDE.md): the frame's image, r30 = the competition, r31 = the club.
    const Frame frame(c, 0x10452ed0, 0x40, {.first_gpr = 30});
    c.set({{30, object}, {31, club}});
    const auto asked = c.u32(c.toc(cell::stats_owner));
    if (!asked) return 0;  // the competition is not read before this test (0x10452ef0..0x10452ef8)
    if (low_byte_set(invoke(c, 0x104abf40, 0x10452f04, {asked, c.u32(object + competition::record)}))) return 0;
    const auto rules = c.u32(c.toc(toc::fixture_rules));
    if (!rules) return 0;
    if (low_byte_set(invoke(c, 0x10494d20, 0x10452f24, {rules, c.u32(object + competition::record)}))) return 0;
    if (c.i8(object + competition::owner) != -1) return 1;
    if (const auto stats = c.u32(object + competition::stats)) {
        // r7..r10 are what 0x10494d20 left (0x10459b50 reads them); nothing here touches them.
        const auto row = invoke(c, 0x10459b50, 0x10452f60, {stats, club});
        if (row && c.u8(row + 0x77) == c.u8(row + 0x78) && c.u8(row + 0x77) != 0) return 0;
    }
    // The stage counter's low byte is tested (r7 counts down as an int); the offset (r5) counts with it.
    std::int32_t counter = std::int8_t(c.u32(object + competition::last_stage));
    std::uint32_t offset = std::uint32_t(counter) << 2;
    for (; std::int8_t(counter) >= -1; --counter, offset -= 4) {
        const auto stage = std::int8_t(counter) == -1 ? object : c.u32(c.u32(object + competition::stages) + offset);
        const std::int32_t kind = c.i8(stage + competition::kind);
        if (kind == competition::league) {
            for (std::int32_t n = 0, at = 0; std::int16_t(n) < c.i16(stage + competition::club_count); ++n, at += 8) {
                const auto table = c.u32(stage + competition::table);
                if (table && c.u32(table + std::uint32_t(at)) == club) return 1;
            }
        } else if (kind == competition::knockout) {
            for (std::int32_t n = 0, at = 0; std::int16_t(n) < c.i16(stage + competition::club_count); ++n, at += competition::entry_row) {
                const auto entries = c.u32(stage + competition::entries);
                if (!entries || c.u32(entries + std::uint32_t(at)) != club) continue;
                const auto entry = entries + std::uint32_t(std::int32_t(std::int16_t(n))) * competition::entry_row;
                const auto round = c.u8(entry + entry_status);
                if (std::int8_t(round) == -2 || std::uint8_t(round - 6) <= 1 || std::int8_t(round) == 5) return 0;
                // ((a - b) | (b - a)) >> 31: the two bytes (signed) differ.
                return c.i8(entry + entry_b) != c.i8(entry + entry_a) ? 1 : 0;
            }
        }
    }
    return 1;
}

std::uint32_t lacks_property(Cpu& c, std::uint32_t record) {
    const Frame frame(c, 0x10451f50, 0x40, {});
    const auto answer = invoke(c, 0x10464880, 0x10451f64, {std::uint32_t(std::int32_t(c.i16(record + 0x30)))});
    return std::int8_t(answer) == 0 ? 1 : 0;
}

void set_club_bytes(Cpu& c, std::uint32_t object, std::uint32_t first, std::uint32_t second, std::uint32_t third) {
    const Frame frame(c, 0x10451f80, 0x50, {.first_gpr = 29});
    c.set({{29, first}, {30, second}, {31, third}});
    if (c.i8(object + competition::owner) == -1) {
        // r4 (and r7..r10) reach 0x10459b50 as the caller left them.
        c.set(3, c.u32(object + competition::stats));
        c.call(0x10459b50, 0x10451fe8);
        const auto row = c.r(3);
        c.put8(row + 0x76, first);
        c.put8(row + 0x77, second);
        c.put8(row + 0x78, third);
        return;
    }
    const auto id = c.u32(c.u32(object + competition::record));
    c.set(9, c.toc(toc::competitions));
    virtual_call(c, 0x70, 0x10451fd8, {c.u32(c.u32(c.toc(toc::competitions)) + (id << 2))});
}

void pack_fixture_rules(Cpu& c, std::uint32_t object, std::uint32_t fixture) {
    const Frame frame(c, 0x104530f0, 0x40, {});
    if (c.i8(object + competition::owner) != -1) {
        const auto cell = c.toc(toc::competitions), table = c.u32(cell);
        if (table) {
            const auto other = c.u32(table + (c.u32(c.u32(object + competition::record)) << 2));
            if (other) {
                c.set({{5, c.u32(object + competition::record)}, {6, table}});
                virtual_call(c, 0x84, 0x10453144, {other});
                return;
            }
        }
    }
    // Bench size to the high nibble, then the substitutes to the low one: two stores, as the original.
    auto packed = c.u8(fixture + 0x4e);
    packed = std::uint8_t((packed & 0x0f) | (std::uint32_t(std::int8_t(c.u8(object + competition::bench))) << 4 & 0xf0));
    c.put8(fixture + 0x4e, packed);
    packed = std::uint8_t((c.u8(fixture + 0x4e) & 0xf0) | (std::uint32_t(std::int8_t(c.u8(object + competition::substitutes))) & 0x0f));
    c.put8(fixture + 0x4e, packed);
}

std::uint32_t stage_virtual_5c(Cpu& c, std::uint32_t object, std::uint32_t other) {
    const Frame frame(c, 0x10451ca0, 0x40, {});
    const std::int32_t index = c.i8(other + 0x42);
    if (index == -1) return virtual_call(c, 0x5c, 0x10451cc8, {object});
    return virtual_call(c, 0x5c, 0x10451ce8, {c.u32(c.u32(object + competition::stages) + (std::uint32_t(index) << 2))});
}

}
