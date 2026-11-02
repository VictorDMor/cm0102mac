#pragma once
// What the press reactions share: the numbered fields of an item, the draws against a byte of a person's
// traits, and the person views.
#include "media_detail.hpp"
#include "media_records.hpp"
#include "media_values_detail.hpp"
#include <type_traits>

namespace cm::aot::media::detail::pick_c {
using records::Club;
using records::Person;
// A number of the random stream after one step: the table's entry plus the seed.
inline std::uint32_t entry_of(Cpu& c, const Stream& rng) { return c.u16(c.u32(rng.cursor)); }
// A trait byte as the original holds it before `extsb`: the unsigned byte.
inline std::uint32_t raw_byte(const Field<std::int8_t>& trait) { return std::uint8_t(trait.get()); }

// An inlined random(n) site for a signed byte (or a small sum), into register `result` (see draw_narrow).
inline std::int32_t roll(Cpu& c, const Stream& rng, std::uint32_t refill_back, unsigned result, std::int32_t n) {
    return draw_narrow(c, rng, n, refill_back, result);
}

// `lbz rN; extsb. rN` and the draw that follows: the byte, as n, in register N and the draw into `result`.
template <unsigned N>
std::int32_t draw_byte(Cpu& c, const Stream& rng, std::uint32_t byte, std::uint32_t refill_back, unsigned result) {
    c.set(N, extended(std::int16_t(std::int8_t(byte))));
    return roll(c, rng, refill_back, result, byte_word(byte));
}
// A signed score byte (a standing record's mood) moved by `delta` and held within -100..100: past either limit it
// is set to the limit. The unsigned byte is left in r3 on the unclamped path, the signed one otherwise.
inline void shift_score(Cpu& c, std::uint32_t at, std::int32_t delta) {
    const auto signed_score = [&] { return std::int32_t(c.i8(at)); };
    c.set(3, std::uint32_t(signed_score()));
    if (compare(c, signed_score() + delta, 100) > 0) {
        c.put8(at, 100);
        return;
    }
    c.set(3, std::uint32_t(signed_score()));
    if (compare(c, signed_score() + delta, -100) < 0) {
        c.put8(at, std::uint32_t(-100));
        return;
    }
    c.set(3, c.u8(at));
    c.put8(at, c.u8(at) + std::uint32_t(delta));
}

// A byte of the person's contract moved by `delta`: `lwz r4,0x14(r30); lbz r3,off(r4); ...; stb r0,off(r4)`.
inline void step_contract_byte(Cpu& c, std::uint32_t person, std::uint32_t offset, std::uint32_t delta) {
    const auto contract = c.u32(person + 0x14);
    c.set(4, contract);
    const auto old = c.u8(contract + offset);
    c.set(3, old);
    c.put8(contract + offset, old + delta);
}

// The press item a routine builds on its frame, its fields numbered by a counter in register R
// (`or r4,R,R; addi R,R,1` before each put) or in a stack byte.
// A field's value: a number, or a function that reads it from memory after the counter has moved on (where the
// original loads after `addi`; this matters when the read faults).
template <class V>
std::uint32_t value_of(V&& v) {
    if constexpr (std::is_invocable_v<V>) return std::uint32_t(v());
    else return std::uint32_t(v);
}
template <unsigned R>
struct RegisterFields {
    Cpu& c;
    Item item;
    Kept<R>& counter;
    // `or r4,R,R; addi R,R,1` around the load of a value already in a register.
    void put(std::uint32_t value, std::uint32_t back) const {
        const auto index = std::uint32_t(counter);
        counter = index + 1;
        item.put(index, value, back);
    }
    template <class Fn>
    void put_after(Fn&& value, std::uint32_t back) const {
        const auto index = std::uint32_t(counter);
        counter = index + 1;
        item.put(index, value(), back);
    }
    // A link of the person: the pointer is loaded first, the id behind it after the counter moved on.
    void link(std::uint32_t person, std::uint32_t offset, std::uint32_t back) const {
        const auto pointer = c.u32(person + offset);
        put_after([&] { return c.u32(pointer + 0x20); }, back);
    }
    // The four fields that name a person: his id and three links.
    void name(std::uint32_t person, std::uint32_t back0, std::uint32_t back1, std::uint32_t back2, std::uint32_t back3) const {
        put(c.u32(person), back0);
        link(person, 0x28, back1);
        link(person, 0x2c, back2);
        link(person, 0x30, back3);
    }
};
// The same with the counter in a byte of the frame: here every load follows the store of the counter.
struct StackFields {
    Cpu& c;
    Item item;
    std::uint32_t counter_at;
    template <class V>
    void put(V&& value, std::uint32_t back) const {
        const auto index = c.u8(counter_at);
        c.put8(counter_at, index + 1);
        item.put(index, value_of(value), back);
    }
    void name(std::uint32_t person, std::uint32_t back0, std::uint32_t back1, std::uint32_t back2, std::uint32_t back3) const {
        put([&] { return c.u32(person); }, back0);
        put([&] { return linked(c, person, 0x28); }, back1);
        put([&] { return linked(c, person, 0x2c); }, back2);
        put([&] { return linked(c, person, 0x30); }, back3);
    }
};
// `bl` into another routine of this module: the registers, then the C++ when it runs here.
inline void module_call(Cpu& c, std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    unsigned n = 3;
    for (const auto value : arguments) c.set(n++, value);
    media::call(c, entry, back);
}
namespace callee {
constexpr std::uint32_t standing_of = 0x10082f90,   // (standings, person, club): the person's standing record
    club_flag = 0x1014c2e0, region_of = 0x100db1b0, region_tier = 0x100db120, club_level = 0x100c9070, since = 0x10718130,
    stance = 0x1007c140, rival_check = 0x100dbc20, person_flag = 0x100d3150,
    fame = 0x100d71a0, grade = 0x10117490, flags_of = 0x1037fa70, set_flag = 0x1037f8a0, clear_flag = 0x1037fa50,
    modify = 0x1037fa80, clear_bits = 0x1037f7d0, today = 0x10715880, is_national_team = 0x100c7d00;
// This module's.
constexpr std::uint32_t verdict = 0x106584d0, label = 0x106a95c0;
}
namespace slots {
constexpr int standings = -0x7b7c, queue = -0x7bf0, labels = -0x7570, press = -0x7b94;
}
}
