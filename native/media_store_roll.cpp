// Two small draws from the shared stream, by how well known the subject is (a club's
// reputation at +0x1ec, or the value at +0xa of the record at +0x14 of a player): above
// 6250 a roll of 100 sorted into seven outcomes, otherwise two outcomes.
#include "media_store.hpp"
#include "media_store_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::store;
namespace {
constexpr std::int32_t well_known = 0x186a;
struct Sites { std::uint32_t refill_high, refill_low; };

std::uint32_t roll(Cpu& c, std::int32_t fame, const Sites& sites) {
    // The refill is called with the stream's own words still in r4 and r5.
    leave(c, {{4, c.toc(slot::rng_end)}, {5, c.toc(slot::rng_start)}});
    const auto stream = Stream::world(c);
    if (compare(c, fame, well_known) > 0) {
        stream.advance(sites.refill_high);
        const auto sum = std::uint32_t(stream.sample());
        c.set(4, sum);
        const auto roll = std::int32_t(sum) % 100;
        if (compare(c, std::int8_t(roll)) == 0) return result(c, 6);
        const auto outcome = std::int8_t(roll);
        if (compare(c, outcome, 8) < 0) return result(c, 3);
        if (compare(c, outcome, 0xf) < 0) return result(c, 5);
        if (compare(c, outcome, 0x28) < 0) return result(c, 2);
        if (compare(c, outcome, 0x3c) < 0) return result(c, 0);
        return result(c, 1);
    }
    stream.advance(sites.refill_low);
    const auto sum = std::uint32_t(stream.sample());
    const auto roll = std::int32_t(sum) % 4;
    if (compare(c, roll, 3) >= 0) return result(c, 4);
    if (compare(c, roll) >= 0) return result(c, 2);
    return result(c, 4);
}
}

// The draw for a club (reputation at +0x1ec).
std::uint32_t story_roll_for_club(Cpu& c, std::uint32_t club) {
    Frame frame(c, 0x1069e990, 0x40, {.first_gpr = 30});
    const auto stream = Stream::world(c);
    keep(c, {{31, stream.seed}, {30, stream.cursor}});
    return roll(c, c.i16(club + 0x1ec), {0x1069e9e0, 0x1069eaa0});
}

// The draw for a player (reputation at +0xa of his record at +0x14).
std::uint32_t story_roll_for_player(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x1069eb10, 0x40, {.first_gpr = 30});
    const auto stream = Stream::world(c);
    keep(c, {{31, stream.seed}, {30, stream.cursor}});
    c.set(3, c.u32(player + 0x14));
    return roll(c, c.i16(c.r(3) + 0xa), {0x1069eb64, 0x1069ec24});
}
}
