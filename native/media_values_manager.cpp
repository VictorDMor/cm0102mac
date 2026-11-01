// 0x106487d0: the manager the press finds for a club.
#include "media_records.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::records;

namespace {
// One of the club's three affiliated clubs: notable when its manager's +0x68 byte draws 0 and +0x63 draws
// above 5, in the club's division and a rating 750 above the club's, once a draw modulo 3 is not 0. The
// registers: the first range is kept in rA and its draw in rB (20 and 26, or 26 and 20); the second range is
// always in r26 and its draw in r20. Leaves the manager's id in r3 when it takes the club.
template <unsigned A, unsigned B>
bool affiliate_manager(Cpu& c, const Stream& rng, const Club& home, unsigned n, std::uint32_t back1, std::uint32_t back2, std::uint32_t back3) {
    const Kept<30> affiliate_at(c, home.affiliate(n));
    const Club affiliate{c, affiliate_at};
    if (!nonzero_logical(c, affiliate_at)) return false;
    if (!nonzero_logical(c, affiliate.manager())) return false;
    const auto manager = affiliate.manager_record();
    const Kept<A> first_range(c, std::uint32_t(std::int32_t(manager.trait_68())));
    const auto first_draw = draw_byte<B>(c, rng, back1, std::int32_t(first_range));
    if (compare(c, first_draw) != 0) return false;
    const Kept<26> second_range(c, std::uint32_t(std::int32_t(manager.ambition())));
    const auto second_draw = draw_byte<20>(c, rng, back2, std::int32_t(second_range));
    if (compare(c, second_draw, 5) <= 0) return false;
    if (compare_logical(c, affiliate.division(), home.division()) != 0) return false;
    if (compare(c, home.reputation() + 0x2ee, affiliate.reputation()) >= 0) return false;
    rng.advance(back3);
    const auto x = std::int32_t(rng.sample());
    leave(c, {{3, std::uint32_t(multiply_high(0x55555556, x))}, {4, std::uint32_t(x)}});
    if (compare(c, x % 3) == 0) return false;
    result(c, manager.id());
    return true;
}
}

// Returns the id of a manager (the first word of a +0x90 record): the manager of one of the club's affiliated
// clubs (+0x74, +0x78 or +0x7c) who is notable (a byte draw at +0x68 comes out 0 and one at +0x63 above 5), in
// the club's division and a rating 750 over the club's, once a draw modulo 3 is not 0; failing that, one of up
// to 50 clubs drawn at random that 0x10464160 accepts, with the same test of the manager. -1 when there is none,
// the club has no manager (+0x90), no division (+0x30) that 0x100c7d00 can supply, or the manager has no staff record (+0x1c).
std::uint32_t find_manager(Cpu& c, std::uint32_t club) {
    Frame frame(c, 0x106487d0, 0xe0, {.first_gpr = 19});
    Fpu fpu(c);
    const auto rng = Stream::world(c);
    const Kept<21> club_total_slot(c, c.toc(tables::club_total));
    const Kept<22> excluded_slot(c, c.toc(tables::excluded_clubs));
    const Kept<23> clubs_slot(c, c.toc(tables::clubs));
    const Kept<28> floats_slot(c, c.toc(slot::floats));
    keep(c, {{24, club}, {25, rng.end}, {27, rng.start}, {29, rng.seed}, {31, rng.cursor}});
    const Club home{c, club};
    const auto none = [&] { return result(c, 0xffffffffu); };
    if (!nonzero_logical(c, home.manager())) return none();
    if (!nonzero_logical(c, home.division()) && !nonzero(c, byte_word(Program{c}.call(0x100c7d00, 0x1064882c, {club})))) return none();
    const auto manager = home.manager_record();
    if (!nonzero_logical(c, manager.non_player())) return none();
    // The members of the club's managers' list: it is walked and never matches (both ratings it compares are read
    // from the same record, so the original's draw after the comparison cannot run).
    Program{c}.call(0x100db060, 0x10648860, {manager.at, frame.at(0x3c), frame.at(0x38)});
    const auto entry = [&](std::uint32_t index) { return Person{c, c.u32(frame.at(0x3c) + (index << 2))}; };
    for (Kept<26> member(c, 0); compare(c, byte_word(member), byte_word(c.u8(frame.at(0x38)))) < 0; member = member + 1) {
        const auto listed = entry(std::uint32_t(byte_word(member)));
        if (nonzero_logical(c, listed.at) && nonzero_logical(c, listed.non_player())) {
            const auto rating = std::int32_t(listed.staff_record().reputation());
            leave(c, {{4, std::uint32_t(rating)}});
            compare(c, rating, rating + 0x2ee);
        }
    }
    // The three affiliated clubs, then the draw over the other clubs.
    if (affiliate_manager<20, 26>(c, rng, home, 0, 0x106489b0, 0x10648b2c, 0x10648cb4)) return c.r(3);
    if (affiliate_manager<26, 20>(c, rng, home, 1, 0x10648d48, 0x10648ec4, 0x1064904c)) return c.r(3);
    if (affiliate_manager<26, 20>(c, rng, home, 2, 0x106490e0, 0x1064925c, 0x106493e4)) return c.r(3);
    constexpr DrawSite wide{0x1064946c, 0x10649544, 0x8c, 0x94, 0x9c, 20};
    for (Kept<26> attempt(c, 0); compare(c, byte_word(attempt), 0x32) < 0; attempt = attempt + 1) {
        const auto count = std::int32_t(c.u32(club_total_slot) - (c.u32(excluded_slot) << 1));
        const Kept<30> range(c, std::uint32_t(count));
        const auto drawn = draw_below(c, frame, fpu, rng, std::uint32_t(floats_slot), wide, count);
        const Kept<30> candidate_at(c, c.u32(clubs_slot) + std::uint32_t(drawn) * club_size);
        const Club candidate{c, candidate_at};
        const auto accepted = Program{c}.call(0x10464160, 0x106495c0, {club, candidate.at});
        if (!nonzero(c, byte_word(accepted))) continue;
        if (!nonzero_logical(c, candidate.manager())) continue;
        const auto candidate_manager = candidate.manager_record();
        const Kept<19> first_range(c, std::uint32_t(std::int32_t(candidate_manager.trait_68())));
        const auto first_draw = draw_byte<20>(c, rng, 0x1064960c, std::int32_t(first_range));
        if (compare(c, first_draw) != 0) continue;
        const Kept<20> second_range(c, std::uint32_t(std::int32_t(candidate_manager.ambition())));
        const auto second_draw = draw_byte<19>(c, rng, 0x10649788, std::int32_t(second_range));
        if (compare(c, second_draw, 5) <= 0) continue;
        return result(c, candidate_manager.id());
    }
    return none();
}
}
