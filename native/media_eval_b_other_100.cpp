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
constexpr std::int32_t dressing_room_kind = 0x1e;

bool is_dressing_room_story(Reaction& r) { return compare(r.c, half_word(r.kind), dressing_room_kind) == 0; }

// How a person's morale moves for a dressing-room story and for the other kinds, with the return address after each
// call.
struct Move {
    std::int32_t dressing_room, other;
    std::uint32_t dressing_room_back, other_back;
};
struct Reprisal {
    std::uint32_t dislike_ask_back, like_ask_back;
    Move dislike, like;
};
// A person who dislikes the player (the query about the two) has his morale moved; failing that one who likes him.
void reprisal(Reaction& r, const Reprisal& t) {
    const auto move = [&](const Move& m) {
        const bool dressing_room = is_dressing_room_story(r);
        r.morale(r.person, dressing_room ? m.dressing_room : m.other,
                 dressing_room ? m.dressing_room_back : m.other_back);
    };
    if (r.asks(relations::disliked_by, t.dislike_ask_back, r.player, r.person)) return move(t.dislike);
    if (r.asks(relations::liked_by, t.like_ask_back, r.player, r.person)) move(t.like);
}

// Verdict above the band.
void high_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (is_dressing_room_story(r)) {
        if (!nonzero(c, r.roll_trait(r.person, &records::Person::temperament, 0x1066a9e0)) &&
            !nonzero(c, r.roll_trait_plus(r.person, &records::Person::sportsmanship, 5, 0x1066ab5c)) &&
            nonzero(c, r.roll_trait_plus(r.player, &records::Person::loyalty, 0xa, 0x1066acd8)))
            r.turn_on_manager(0x1066ae2c, 0x1066ae3c);
    }
    reprisal(r, {0x1066ae48, 0x1066ae90, {-0xf, -5, 0x1066ae6c, 0x1066ae80}, {-5, 5, 0x1066aeb4, 0x1066aec8}});
}

// Verdict below the band.
void low_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (is_dressing_room_story(r)) {
        if (!nonzero(c, r.roll_half_trait(r.person, &records::Person::temperament, 0x1066af28)) &&
            !nonzero(c, r.roll_half_trait(r.person, &records::Person::sportsmanship, 0x1066b0ac)))
            r.turn_on_manager(0x1066b200, 0x1066b210);
    }
    reprisal(r, {0x1066b21c, 0x1066b264, {-0x19, -0xf, 0x1066b240, 0x1066b254}, {-5, 0xa, 0x1066b288, 0x1066b29c}});
}

// In between: only a dressing-room story does anything, and it only rolls for the manager.
void middle_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (!is_dressing_room_story(r)) return;
    if (nonzero(c, r.roll_trait(r.person, &records::Person::temperament, 0x1066b2e0))) return;
    if (nonzero(c, r.roll_trait(r.person, &records::Person::sportsmanship, 0x1066b458))) return;
    r.turn_on_manager(0x1066b5ac, 0x1066b5bc);
}
}

void Reaction::other_100() {
    switch (band(0x78)) {
    case Band::above: return high_verdict(*this);
    case Band::below: return low_verdict(*this);
    default: return middle_verdict(*this);  // Band::between
    }
}
}
}
