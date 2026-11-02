// 0x1066a990..0x1066b5bc of 0x106620b0: flag 0x100 (the story is about the dressing room's pecking order) for a
// person who is not the player. The verdict against the pool's threshold (0x78 from the lean, both ways) picks one of
// three shapes. Kind 0x1e stories roll the person's traits for a quarrel with the club's manager; the rest of the
// shapes move the person's morale when he is at odds with, or a friend of, the player. Every path ends the person (the
// code falls into the loop's increment at 0x1066b5bc).
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
namespace {
constexpr std::uint32_t against_manager = 0x100d9b80;  // (player, the club's manager, 0); (inferred) the player turns on the manager
constexpr unsigned sour_bit = 0x8;
constexpr std::int32_t dressing_room_kind = 0x1e;

using TraitOf = Field<std::int8_t> (records::Person::*)() const;

bool is_dressing_room_story(Reaction& r) { return compare(r.c, half_word(r.kind), dressing_room_kind) == 0; }

// random(trait of `holder`) in line.
std::int32_t roll_trait(Reaction& r, std::uint32_t holder, TraitOf trait, std::uint32_t back) {
    return r.draw((r.who(holder).*trait)().get(), back);
}
// random(trait of `holder` + bias) in line. The trait is in r3 until the draw replaces it (a zero range keeps it).
std::int32_t roll_trait_plus(Reaction& r, std::uint32_t holder, TraitOf trait, std::int32_t bias, std::uint32_t back) {
    const std::int32_t value = (r.who(holder).*trait)().get();
    r.c.set(3, std::uint32_t(value));
    return r.draw(value + bias, back);
}
// random(trait / 2) in line, the halving rounding toward zero ((x + sign) >> 1).
std::int32_t roll_half_trait(Reaction& r, std::uint32_t holder, TraitOf trait, std::uint32_t back) {
    const std::int32_t value = (r.who(holder).*trait)().get();
    r.c.set(3, std::uint32_t(value));
    return r.draw((value + std::int32_t(std::uint32_t(value) >> 31)) >> 1, back);
}

// The player turns on the club's manager: he loses the contract's 8 bit.
void turn_on_manager(Reaction& r, std::uint32_t call_back, std::uint32_t clear_back) {
    r.program.call(against_manager, call_back, {r.player, r.c.u32(r.club + 0x90), 0});
    r.program.call(clear_flag, clear_back, {r.standing, sour_bit, 0});
}

// How a person's morale moves for a dressing-room story and for the other kinds, with the return address after each call.
struct Move {
    std::int32_t dressing_room, other;
    std::uint32_t dressing_room_back, other_back;
};
struct Reprisal {
    std::uint32_t quarrel_ask_back, friend_ask_back;
    Move quarrel, friend_;
};
// A person at odds with the player (the query about the two) has his morale moved; failing that a friend of his does.
void reprisal(Reaction& r, const Reprisal& t) {
    const auto move = [&](const Move& m) {
        const bool dressing_room = is_dressing_room_story(r);
        r.morale(r.person, dressing_room ? m.dressing_room : m.other, dressing_room ? m.dressing_room_back : m.other_back);
    };
    if (r.asks(query_rival_bad, t.quarrel_ask_back, r.player, r.person)) return move(t.quarrel);
    if (r.asks(query_rival_good, t.friend_ask_back, r.player, r.person)) move(t.friend_);
}

// Verdict above the threshold.
void high_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (is_dressing_room_story(r)) {
        if (!nonzero(c, roll_trait(r, r.person, &records::Person::trait_69, 0x1066a9e0)) &&
            !nonzero(c, roll_trait_plus(r, r.person, &records::Person::sportsmanship, 5, 0x1066ab5c)) &&
            nonzero(c, roll_trait_plus(r, r.player, &records::Person::loyalty, 0xa, 0x1066acd8)))
            turn_on_manager(r, 0x1066ae2c, 0x1066ae3c);
    }
    reprisal(r, {0x1066ae48, 0x1066ae90, {-0xf, -5, 0x1066ae6c, 0x1066ae80}, {-5, 5, 0x1066aeb4, 0x1066aec8}});
}

// Verdict below the lean less the threshold.
void low_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (is_dressing_room_story(r)) {
        if (!nonzero(c, roll_half_trait(r, r.person, &records::Person::trait_69, 0x1066af28)) &&
            !nonzero(c, roll_half_trait(r, r.person, &records::Person::sportsmanship, 0x1066b0ac)))
            turn_on_manager(r, 0x1066b200, 0x1066b210);
    }
    reprisal(r, {0x1066b21c, 0x1066b264, {-0x19, -0xf, 0x1066b240, 0x1066b254}, {-5, 0xa, 0x1066b288, 0x1066b29c}});
}

// In between: only a dressing-room story does anything, and it only rolls for the manager.
void middle_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (!is_dressing_room_story(r)) return;
    if (nonzero(c, roll_trait(r, r.person, &records::Person::trait_69, 0x1066b2e0))) return;
    if (nonzero(c, roll_trait(r, r.person, &records::Person::sportsmanship, 0x1066b458))) return;
    turn_on_manager(r, 0x1066b5ac, 0x1066b5bc);
}
}

void Reaction::other_100() {
    if (compare_verdict(0x78).greater) return high_verdict(*this);
    const double floor = fpu.fsub(lean(), dbl(0x78));
    c.set_f(0, floor);
    fpu.fcmp(0, verdict(), floor);
    if (verdict() < floor) low_verdict(*this);
    else middle_verdict(*this);
}
}
}
