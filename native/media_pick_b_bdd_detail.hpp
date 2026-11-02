#pragma once
// 0x1067e210: the press item 0xbdd about a player (`person`) and the club he is with. The routine picks the
// story kind (0x29-0x32, or the one it was given for 0x33/0x34), a roll for it, builds the numbered item on its
// stack, posts it and then lets the story move the player's ratings. The struct is its register and stack
// state; a method per phase.
#include "media_pick_a_detail.hpp"

namespace cm::aot::media::detail::pick_b {
using namespace guest;
using pick::Club;
using pick::Fields;
using pick::Person;

struct Bdd {
    Cpu& c;
    const Program program;
    Frame frame;
    Fpu fpu;
    Floats pool;
    Stream rng;
    std::uint32_t query, person, club, extra;
    // Registers the original reuses: r14/r15 the tail's values, r20 the date pointer and then the draws,
    // r23 the standing record (an entry value when the first guard is bypassed) and then the owner's id,
    // r25 the story kind, r27 its roll.
    Kept<14> r14;
    Kept<15> r15;
    Kept<20> r20;
    Kept<23> standing;
    Kept<25> kind;
    Kept<27> roll;
    Item item;
    Fields numbered;  // the item's fields past 0x10, numbered by the counter byte at sp+0x38
    const Person subject;  // the player
    const Club team;       // his club

    Bdd(Cpu& cpu, std::uint32_t query, std::uint32_t person, std::uint32_t club, std::uint32_t kind, std::uint32_t extra);
    // The player's record (the one behind the person's +0x14).
    std::uint32_t player_record() const { return subject.player(); }
    std::int32_t age() const { return subject.age(); }
    // The player's standing with the club (the contract record `standing` holds once the guards have passed).
    pick::Contract standing_record() const { return {c, standing}; }
    std::uint32_t slot_value(int slot) const { return c.u32(c.toc(slot)); }
    // The next entry of the random table plus the seed, as a signed word.
    std::int32_t sample(std::uint32_t refill_back) const {
        rng.advance(refill_back);
        return std::int32_t(rng.sample());
    }
    bool admits();
    void choose_kind();
    bool dislikes();
    bool settled_rules();
    bool ambitions();
    bool posts();
    void put_standing_fields();
    void put_dates();
    void put_staff();
    void deliver();
    void raise_and_notify();
    bool scaled_exceeds(std::uint32_t a_at, std::int32_t a, std::uint32_t b_at, std::int32_t b, std::uint32_t scale_offset);
    void pick_roll(std::uint32_t kind_value, std::uint32_t refill_back, std::int32_t modulus) {
        kind = kind_value;
        roll = std::uint32_t(sample(refill_back) % modulus);
    }
};
}
