#pragma once
// What the pick blocks share: the views of the records the story routines read, the byte counter
// that numbers an item's fields, the person and match blocks every item repeats.
#include "media_records.hpp"
#include "media_values_detail.hpp"
#include <array>

namespace cm::aot::media::detail::pick {
using records::Club;
using records::Contract;
using records::Division;
using records::Person;
constexpr std::uint32_t player_size = Person::size;
constexpr std::uint32_t none = 0xffffffff;
// A signed byte field as the original loads it when it does not sign-extend.
inline std::int32_t unsigned_byte(const Field<std::int8_t>& field) { return std::uint8_t(field.get()); }
// A request for story 0xbd9 (0x18 bytes, built by the caller): who, for which club, and why.
struct Request {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint32_t> player() const { return {c, at}; }        // index into the persons table
    Field<std::int32_t> club() const { return {c, at + 4}; }       // index into the clubs table
    Field<std::int8_t> amount() const { return {c, at + 0xc}; }
    Field<std::int8_t> flag() const { return {c, at + 0xe}; }
    Field<std::int32_t> second() const { return {c, at + 0x10}; }  // -1: none
    Field<std::int16_t> kind() const { return {c, at + 0x14}; }
};
// `bl` into a routine of the module that has a typed function: its arguments in r3 on and the link, then the
// C++ call. Returns r3.
template <class Function, class... Arguments>
std::uint32_t typed_call(Cpu& c, std::uint32_t back, Function function, Arguments&&... arguments) {
    enter(c, back, {std::uint32_t(arguments)...});
    function(c, std::uint32_t(arguments)...);
    return c.r(3);
}
// The match record the press reads (inferred from the result block): the goals of the two sides
// (-1: not played yet) and the clubs they belong to.
struct Fixture {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint32_t> first_club() const { return {c, at + 0x1c}; }
    Field<std::uint32_t> second_club() const { return {c, at + 0x20}; }
    Field<std::int8_t> first_goals() const { return {c, at + 0x49}; }
    Field<std::int8_t> second_goals() const { return {c, at + 0x4a}; }
};

// The item field numbers of some routines live in a byte of their frame: a field takes the
// byte's value, then the byte counts up.
struct Counter {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t take() const {
        const auto value = c.u8(at);
        c.put8(at, value + 1);
        return value;
    }
};
// An item numbered by such a counter.
struct Fields {
    Item item;
    Counter counter;
    // A field whose value the original has computed before the counter steps.
    void put(std::uint32_t value, std::uint32_t back) const {
        const auto index = counter.take();
        item.put(index, value, back);
    }
    // A field the original loads after stepping the counter (a fault then leaves the stepped byte).
    template <class Value>
    void put_loaded(Value&& value, std::uint32_t back) const {
        const auto index = counter.take();
        item.put(index, value(), back);
    }
};
// An item's fields in a literal run, from `next` on.
struct Run {
    Item item;
    mutable std::uint32_t next;
    void put(std::uint32_t value, std::uint32_t back) const { item.put(next++, value, back); }
};

// The return addresses after the puts of a person's block (his index, then the ids behind his three name links).
using Backs = std::array<std::uint32_t, 4>;
using NameBacks = std::array<std::uint32_t, 3>;
// The ids behind a person's three name links, as the next three fields.
template <class Numbered>
void put_names(Cpu& c, Numbered&& fields, std::uint32_t person, const NameBacks& backs) {
    const Person record{c, person};
    fields.put(record.first_name(), backs[0]);
    fields.put(record.second_name(), backs[1]);
    fields.put(record.common_name(), backs[2]);
}
// A person's block: his id and the ids behind his name links, as the next four fields.
template <class Numbered>
void put_person(Cpu& c, Numbered&& fields, std::uint32_t person, const Backs& backs) {
    fields.put(Person{c, person}.id(), backs[0]);
    put_names(c, fields, person, {backs[1], backs[2], backs[3]});
}
// The same at fixed field numbers.
inline void put_person(Cpu& c, const Item& item, std::uint32_t first_field, std::uint32_t person, const Backs& backs) {
    put_person(c, Run{item, first_field}, person, backs);
}
// The block of no person: four -1.
template <class Numbered>
void put_no_person(Numbered&& fields, const Backs& backs) {
    for (const auto back : backs) fields.put(none, back);
}
inline void put_no_person(const Item& item, std::uint32_t first_field, const Backs& backs) {
    put_no_person(Run{item, first_field}, backs);
}
// The same for fields the original loads after stepping the counter (a fault then leaves the stepped byte).
template <class Numbered>
void put_names_loaded(Cpu& c, Numbered&& fields, std::uint32_t person, const NameBacks& backs) {
    const Person record{c, person};
    fields.put_loaded([&] { return record.first_name(); }, backs[0]);
    fields.put_loaded([&] { return record.second_name(); }, backs[1]);
    fields.put_loaded([&] { return record.common_name(); }, backs[2]);
}
template <class Numbered>
void put_person_loaded(Cpu& c, Numbered&& fields, std::uint32_t person, const Backs& backs) {
    const Person record{c, person};
    fields.put_loaded([&] { return record.id().get(); }, backs[0]);
    put_names_loaded(c, fields, person, {backs[1], backs[2], backs[3]});
}

// The return addresses of a result block: after the put for a match not played, for the first side
// ahead (the club is that side, or the other), for the second side ahead, and for a level score
// (0: the original puts nothing).
struct ResultBacks {
    std::uint32_t unplayed, first_here, first_other, second_here, second_other, level;
};
// How the match stands for `club`: 0 not played, 1 the club's side is ahead, 2 the other side.
template <class Numbered>
void put_result(Cpu& c, Numbered&& fields, const Fixture& match, std::uint32_t club, const ResultBacks& backs) {
    const std::int32_t first = match.first_goals(), second = match.second_goals();
    const auto side = [&](std::uint32_t at, std::uint32_t here_back, std::uint32_t other_back) {
        const bool here = compare_logical(c, c.u32(match.at + at), club) == 0;
        fields.put(here ? 1 : 2, here ? here_back : other_back);
    };
    if (compare(c, first, -1) == 0) fields.put(0, backs.unplayed);
    else if (compare(c, first, second) > 0) side(0x1c, backs.first_here, backs.first_other);
    else if (compare(c, first, second) < 0) side(0x20, backs.second_here, backs.second_other);
    else if (backs.level != 0) fields.put(0, backs.level);
}
}
