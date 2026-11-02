// 0x106667e0..0x10667df4 of 0x106620b0: flag 0x80 (the story's verdict moved the dressing room) for a person who is
// not the player. The verdict against the pool's threshold (0x78 from the lean, both ways) picks one of three
// shapes; each asks whether the person is at odds with the player (his morale then moves by 5), and otherwise
// rolls the person's traits to decide whether the player's contract loses a flag. Every exit ends the person.
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
namespace {
constexpr unsigned loyalty_bit = 0x100, story_bit = 0x400;

// The rival's kind (0x13) decides what a successful roll does to the player's contract: sets 0x100, or else clears
// 0x400.
void mark_contract(Reaction& r, std::uint32_t set_back, std::uint32_t clear_back) {
    if (compare(r.c, half_word(r.rival_kind), 0x13) == 0)
        r.program.call(set_flag_b, set_back, {r.contract, loyalty_bit});
    else
        r.program.call(clear_flag, clear_back, {r.contract, story_bit, 0});
}

// The person dislikes the player (the query about the two): his morale moves by 5.
bool dislikes_player(Reaction& r, std::uint32_t ask_back, std::uint32_t morale_back) {
    if (!r.asks(relations::disliked_by, ask_back, r.player, r.person)) return false;
    r.morale(r.person, 5, morale_back);
    return true;
}

// Verdict above the band.
void high_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (dislikes_player(r, 0x106667fc, 0x10666814)) return;
    if (r.asks(relations::liked_by, 0x10666824, r.player, r.person)) {
        // The person's loyalty decides whether the contract is marked.
        if (nonzero(c, r.roll_trait(r.person, &records::Person::loyalty, 0x10666860)))
            mark_contract(r, 0x106669bc, 0x106669d0);
        if (nonzero(c, r.roll_trait(r.person, &records::Person::temperament, 0x10666a04))) return;
        if (nonzero(c, r.roll_trait(r.person, &records::Person::sportsmanship, 0x10666b7c))) return;
        r.turn_on_manager(0x10666cd0, 0x10666ce0);
        return;
    }
    // Not a friend: only a person whose reputation tops the player's own (+ 50 per point of his influence) and the
    // 0x1482 line is rolled.
    const auto mine = r.who(r.player).player_record();
    const auto reputation = std::int32_t(mine.reputation().get()), bonus = mine.influence().get() * 0x32;
    c.set(3, std::uint32_t(reputation));
    c.set(4, mine.at);
    if (compare(c, reputation + bonus, 0x1482) <= 0) return;
    const auto theirs = std::int32_t(r.who(r.person).player_record().reputation().get());
    leave(c, {{3, std::uint32_t(reputation + bonus)}, {4, std::uint32_t(bonus)}, {5, std::uint32_t(reputation)}});
    if (compare(c, reputation + bonus, theirs) <= 0) return;
    if (nonzero(c, r.roll_trait(r.person, &records::Person::temperament, 0x10666d68))) return;
    if (!nonzero(c, r.roll_trait(r.person, &records::Person::ambition, 0x10666ee0))) return;
    mark_contract(r, 0x1066703c, 0x10667050);
}

// The person's roll against a sample mod `by` of the table (his loyalty roll beating it marks the contract).
void marked_by_loyalty(Reaction& r, const Reciprocal& by, std::uint32_t loyalty_back, std::uint32_t sample_back,
                       std::uint32_t set_back, std::uint32_t clear_back) {
    const auto loyal = r.roll_trait(r.person, &records::Person::loyalty, loyalty_back);
    if (compare(r.c, loyal, r.beaten_by(by, sample_back)) > 0) mark_contract(r, set_back, clear_back);
}

// Verdict below the band.
void low_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (dislikes_player(r, 0x10667070, 0x10667088)) return;
    if (!r.asks(relations::liked_by, 0x10667098, r.player, r.person)) return;
    marked_by_loyalty(r, divisors::by_20, 0x106670d4, 0x10667230, 0x10667288, 0x1066729c);
    if (nonzero(c, r.roll_trait(r.person, &records::Person::temperament, 0x106672d0))) return;
    if (nonzero(c, r.roll_trait(r.person, &records::Person::sportsmanship, 0x10667448))) return;
    if (nonzero(c, r.roll_trait(r.player, &records::Person::loyalty, 0x106675c0))) return;
    r.turn_on_manager(0x10667714, 0x10667724);
}

// In between.
void middle_verdict(Reaction& r) {
    Cpu& c = r.c;
    if (dislikes_player(r, 0x10667734, 0x1066774c)) return;
    if (!r.asks(relations::liked_by, 0x1066775c, r.player, r.person)) return;
    marked_by_loyalty(r, divisors::by_10, 0x10667798, 0x106678f4, 0x1066794c, 0x10667960);
    if (nonzero(c, r.roll_trait(r.person, &records::Person::temperament, 0x10667994))) return;
    if (nonzero(c, r.roll_trait_plus(r.person, &records::Person::sportsmanship, 5, 0x10667b10))) return;
    if (!nonzero(c, r.roll_trait_plus(r.player, &records::Person::loyalty, 0xa, 0x10667c8c))) return;
    r.turn_on_manager(0x10667de0, 0x10667df0);
}
}

void Reaction::other_80() {
    switch (band(0x78)) {
    case Band::above: return high_verdict(*this);
    case Band::below: return low_verdict(*this);
    case Band::between: return middle_verdict(*this);
    }
}
}
}
