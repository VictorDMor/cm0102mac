// DATABASE.CPP's position mask (database.hpp).
#include "database.hpp"
#include "database_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;

// The original loads each rating into r4 and compares it with cmpwi into
// cr0: unsigned (lbz) for "natural" (== 20), sign-extended (extsb) for
// "above 10" and "at least 15". Each test leaves r4 and cr0 as it did.
struct Ratings {
    Cpu& c;
    std::uint32_t player;
    bool natural(std::uint32_t position) const {
        const auto value = std::uint32_t(c.u8(player + position));
        c.set(4, value);
        return compare(c, std::int32_t(value), 20) == 0;
    }
    std::int32_t signed_rating(std::uint32_t position) const {
        const auto value = std::int32_t(c.i8(player + position));
        c.set(4, std::uint32_t(value));
        return value;
    }
    bool above_ten(std::uint32_t position) const { return compare(c, signed_rating(position), 10) > 0; }
    bool at_least_fifteen(std::uint32_t position) const { return compare(c, signed_rating(position), 15) >= 0; }
    // An attacker counts as central unless a position from the attacking
    // midfielder down to the sweeper is natural too. In the "rated 15 or
    // more" pass the original loads the sweeper's rating into r3, not r4.
    bool attacker_only(unsigned sweeper_register) const {
        for (const auto position : {player::attacking_midfielder, player::midfielder, player::defensive_midfielder,
                                    player::defender})
            if (natural(position)) return false;
        const auto value = std::uint32_t(c.u8(player + player::sweeper));
        c.set(sweeper_register, value);
        return compare(c, std::int32_t(value), 20) != 0;
    }
};
}

std::uint32_t position_mask(Cpu& c, std::uint32_t person) {
    std::uint32_t mask = 0;
    c.set(0, mask);
    const auto answer = [&](std::uint32_t value) {
        c.set(0, value);
        c.set(3, value);
        return value;
    };
    if (compare_logical(c, person) == 0) {
        c.set(3, 0);
        return 0;
    }
    const auto player_address = c.u32(person + person::player);
    c.set(3, player_address);
    if (compare_logical(c, player_address) == 0) {
        c.set(3, 0);
        return 0;
    }
    const Ratings ratings{c, player_address};
    if (ratings.natural(player::goalkeeper)) return answer(0x1);
    if (ratings.natural(player::left)) {
        mask |= 0x80;
        if (ratings.above_ten(player::central)) mask |= 0x100;
    }
    if (ratings.natural(player::right)) {
        mask |= 0x800;
        if (ratings.above_ten(player::central)) mask |= 0x400;
    }
    if (ratings.natural(player::central)) mask |= 0x700;
    if (ratings.natural(player::sweeper)) mask |= 0x2;
    if (ratings.natural(player::defender)) mask |= 0x4;
    if (ratings.natural(player::defensive_midfielder)) mask |= 0x18;
    if (ratings.natural(player::midfielder)) mask |= 0x38;
    if (ratings.natural(player::attacking_midfielder)) mask |= 0x30;
    if (ratings.natural(player::attacker)) {
        mask |= 0x40;
        if (ratings.attacker_only(4)) mask |= 0x700;
    }
    if (compare(c, std::int32_t(mask)) != 0) return answer(mask);
    // Nothing natural: the first rating of 15 or more, from the sweeper on.
    if (ratings.at_least_fifteen(player::sweeper)) return answer(0x2);
    if (ratings.at_least_fifteen(player::defender)) return answer(0x4);
    if (ratings.at_least_fifteen(player::defensive_midfielder)) return answer(0x18);
    if (ratings.at_least_fifteen(player::midfielder)) return answer(0x38);
    if (ratings.at_least_fifteen(player::attacking_midfielder)) return answer(0x30);
    if (ratings.at_least_fifteen(player::attacker)) {
        mask = 0x40;
        if (ratings.attacker_only(3)) mask |= 0x700;
        return answer(mask);
    }
    return answer(0x10);
}
}
