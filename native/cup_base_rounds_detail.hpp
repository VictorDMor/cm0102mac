#pragma once
// CUP.CPP, block `rounds`: names and helpers private to the block (the shared names are in cup_base_detail.hpp).
// "(inferred)" marks a meaning no reader has confirmed.
#include "cup_base_detail.hpp"
#include "cup_base_rounds.hpp"
#include "date.hpp"
#include <array>

namespace cm::aot::cup_base::rounds {
// A tie, a 0x36-byte row of cup::round_matches[round].
namespace tie {
inline constexpr std::uint32_t
    first = 0x00,          // halfword: the first side's entrant index, -1: not known yet
    second = 0x02,         // halfword: the second side's
    side_flag = 0x08,      // halfword: set by 0x10485d70 in a feeder tie (1: ...) (inferred)
    first_feeder_match = 0x0a,   // halfword: the earlier tie whose winner fills the first side (inferred)
    second_feeder_match = 0x0c,  // halfword: ... the second side
    first_feeder_round = 0x0e,   // halfword: the round of that tie, -1: none (inferred)
    second_feeder_round = 0x10,  // halfword
    state = 0x34,          // byte: 0 undrawn; 1/2 waiting; 3 scheduled (0x1047f3a0)
    size = 0x36;
}
// A row of cup::rounds (0x78 bytes): the fields these routines read.
namespace round_row {
inline constexpr std::uint32_t
    start_day = 0x04, year_offset = 0x06,  // the round's first date: halfword day, signed byte added to the year
    kind_07 = 0x07,
    label = 0x08,          // halfword: 0x8c is the final
    half_0a = 0x0a,
    rule_flags = 0x0c,     // halfword: 1 pair by virtual +0x98, 4 nation check, 8 by entrant state (inferred)
    flags = 0x0e,          // halfword: 0x100 fixtures by FIX_MAN, 0x200 no draw, 0x400 sort, 0x800 stronger side (inferred)
    byte_18 = 0x18,
    fixture_day = 0x04,    // halfword: the day of the round's fixtures (inferred)
    fixture_kind = 0x07,   // byte: the day kind of its fixtures (inferred)
    rules_id = 0x14,       // word handed to 0x104aff20 (inferred)
    ties = 0x1c,           // halfword: the number of ties
    entrants = 0x1e,       // halfword: clubs of the round, summed over the rounds so far by 0x1047e980 (inferred)
    pending_14 = 0x14,     // word: the rules' id handed to 0x104aff20 (inferred)
    leg_rule = 0x23,       // signed byte: 2 a replay-style round (inferred)
    byte_22 = 0x22,        // signed byte: above 0 sets the fixture's flag 0x20 (inferred)
    byte_24 = 0x24,        // byte: days between a tie's legs (inferred)
    size = 0x78;
}
// The registers r4-r10 as the original holds them, for the routines whose callers read them after the return
// (`registers_after.txt`) and whose callees read them: `v[n]` is the original's rN, written where it writes it.
struct Registers {
    Cpu& c;
    std::array<std::uint32_t, 11> v{};
    explicit Registers(Cpu& cpu) : c(cpu) { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }
    void publish() const { for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]); }
    void take() { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }
};
// The registers r14-r31 as the original holds them at its calls (the news manager and the date routines can
// reach imports, where a replay compares them).
struct Held {
    Cpu& c;
    std::array<std::uint32_t, 32> v{};
    explicit Held(Cpu& cpu) : c(cpu) { for (unsigned n = 14; n < 32; ++n) v[n] = c.r(n); }
    void publish() const { for (unsigned n = 14; n < 32; ++n) c.set(n, v[n]); }
};
// Every register a routine of the block holds at its calls, as the original holds it: r3-r10 (what the last callee
// left and what the routine set after it) and r14-r31 (the caller's, until the routine writes them). `v[n]` is
// the original's rN, written where the original writes it; a call publishes them first and takes r3-r10 back.
struct Original {
    Cpu& c;
    std::array<std::uint32_t, 32> v{};
    explicit Original(Cpu& cpu) : c(cpu) {
        for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n);
        for (unsigned n = 14; n < 32; ++n) v[n] = c.r(n);
    }
    void publish() const {
        for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]);
        for (unsigned n = 14; n < 32; ++n) c.set(n, v[n]);
    }
    void take() { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }
    // `bl entry` (dispatcher) with the arguments in r3 onwards; the callee's r3.
    std::uint32_t call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
        unsigned n = 3;
        for (const auto argument : arguments) v[n++] = argument;
        publish();
        c.call(entry, back);
        take();
        return v[3];
    }
    // The same for one of DATE.CPP's routines (its C++ owner, or the dispatcher while a check watches the call).
    void date(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        unsigned n = 3;
        for (const auto argument : arguments) v[n++] = argument;
        publish();
        ::cm::aot::date::call(c, entry, back);
        take();
    }
    std::uint32_t virt(std::uint32_t slot, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        unsigned n = 3;
        for (const auto argument : arguments) v[n++] = argument;
        publish();
        virtual_call(c, slot, back, arguments);
        take();
        return v[3];
    }
    // The return: r4-r10 as the original leaves them (r3 is the result the hook sets).
    void leave() const { for (unsigned n = 4; n <= 10; ++n) c.set(n, v[n]); }
};
// The loop of 0x1047e980 and 0x1047ef50 that moves a round's draw date a day on when a club is missing (cup_base_rounds_draw.cpp).
void move_round_dates(Cpu&, Original&, const guest::Frame&, std::uint32_t cup, std::uint32_t round, unsigned counter,
                      const std::array<std::uint32_t, 3>& back);
namespace item {
// NEWS.CPP: start an item of a kind (item, kind, 0), set field n (item, n, value), file an item under a club.
inline constexpr std::uint32_t start = 0x102bdf90, set_field = 0x102cb4b0, file_under_club = 0x102cc140,
                               enabled = 0x102cdbf0;
}
}
