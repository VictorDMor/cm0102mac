// COMP_UTIL.CPP, block `rating_sort`: how the unit ranks clubs for seeding and qualification. 0x10477870 rates one
// club; 0x10477b40 sorts an array of club pointers by that rating, best first.
//
// The rating is the club's money (the finance manager's account balance in hundreds-and-threes: balance / 300,
// or without a manager the club's word +4 / 400) plus 16 times its reputation plus the worth of the players the
// computer would pick for it: the first fifteen places of 0x10210c50 over the club's members that have player
// data, each rated by 0x10216830. A missing club rates -10,000,000. Every callee is the original's, called at the
// original's frame with the arguments and non-volatile registers it sees there (a callee's prologue saves them;
// the checker's call trace compares them). The place-chooser reads seven stack words at +0x38.. of this frame
// (all zero here) and the arrays this routine passes by address live at the original's offsets.
//
// 0x10477b40 sorts a copy of the ratings alongside the clubs with a selection sort (swap while the earlier rating
// is lower, so equal ratings keep their order) and frees the copy. Its callers' registers: after the sort the
// original leaves r4-r10 as its loops ended (a caller's virtual call spills r8-r10), which is reproduced here.
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
// Offsets in 0x10477870's frame.
constexpr std::int32_t flags_at = 0x58, chosen_at = 0x78, members_at = 0xb4, picked_at = 0x17c, stack_words_at = 0x38;
// The places the computer fills: the halfword flags it passes (1 first, then 0x804 ...), the original's constants.
constexpr std::uint16_t place_flags[15] = {1, 0x804, 0x84, 0x204, 0x204, 0x204, 0x810, 0x90, 0x210, 0x210, 0x210, 0x40, 0x40, 0x40, 0x40};
constexpr std::int32_t absent_rating = -10000000;
}

std::int32_t club_strength(Cpu& c, std::uint32_t club) {
    Frame frame(c, 0x10477870, 0x260, {.first_gpr = 27});
    c.set({{29, club}, {31, 0}});
    if (club == 0) return absent_rating;
    std::uint32_t value = 0;  // r31
    if (const auto manager = c.u32(c.toc(toc::finance_manager)); manager == 0) {
        value = std::uint32_t(c.i32(club + club::value) / 400);
    } else {
        const auto account = invoke(c, callee::finance_account, 0x104778d0, {manager, club});
        if (account != 0) {
            // The balance (a 64-bit word pair) divided by 300 through the compiler's signed 64-bit quotient: its low word.
            const auto low = c.u32(account + 4), high = c.u32(account);  // the original loads the low word first
            invoke(c, callee::divide64, 0x104778ec, {high, low, 0, 300});
            value = c.r(4);
        }
    }
    value += std::uint32_t(std::int32_t(c.i16(club + club::reputation)) * 16);
    c.set(31, value);
    invoke(c, callee::fill_memory, 0x1047790c, {frame.at(picked_at), 0, 200});
    invoke(c, callee::fill_memory, 0x1047791c, {frame.at(members_at), 0, 200});
    invoke(c, callee::fill_memory, 0x1047792c, {frame.at(chosen_at), 0, 0x3c});
    for (unsigned n = 0; n < 15; ++n) c.put16(frame.at(flags_at + std::int32_t(n) * 2), place_flags[n]);
    // The original's registers when it calls 0x100d06f0 (r6-r10 are leftovers of its constant setup).
    c.set({{30, 0}, {6, 0x210}, {7, 0x90}, {8, 0x810}, {9, 0x204}, {10, 0x84}});
    invoke(c, callee::member_ids, 0x1047799c, {club, frame.at(members_at), 0});
    // The members that have player data (+0x14 of the record), in order, up to 50.
    std::uint32_t picked = 0;
    for (unsigned n = 0; n < 50; ++n) {
        const auto member = c.u32(frame.at(members_at + std::int32_t(n) * 4));
        if (member != 0 && c.u32(member + 0x14) != 0) c.put32(frame.at(picked_at + std::int32_t(picked++) * 4), member);
    }
    c.set(30, picked);
    for (unsigned n = 0; n < 7; ++n) c.put32(frame.at(stack_words_at + std::int32_t(n) * 4), 0);
    invoke(c, callee::choose_places, 0x10477ad0, {frame.at(picked_at), picked, frame.at(flags_at), 15, 0, frame.at(chosen_at), 0x840, club});
    // The worth of each place's pick, judged by the club (no floor, no separate judge).
    for (std::uint32_t place = 0; place < 15; ++place) {
        c.set({{27, place}, {28, frame.at(flags_at + std::int32_t(place) * 2)}, {30, frame.at(chosen_at + std::int32_t(place) * 4)}, {31, value}});
        const auto person = c.u32(frame.at(chosen_at + std::int32_t(place) * 4));
        if (person == 0) continue;
        const auto role = sext16(c.u16(frame.at(flags_at + std::int32_t(place) * 2)));
        value += invoke(c, callee::rate_person, 0x10477b04, {person, role, 0x840, club, 0xffffffffu, 0});
    }
    c.set(31, value);
    return std::int32_t(value);
}

void sort_by_strength(Cpu& c, std::uint32_t clubs, std::int32_t count) {
    Frame frame(c, 0x10477b40, 0x80, {.first_gpr = 25});
    const auto pool = c.toc(toc::pool), buffer = c.toc(toc::message_buffer);
    const auto bytes = std::uint32_t(count) << 2;
    c.set({{27, buffer}, {28, bytes}, {29, clubs}, {26, pool}, {30, std::uint32_t(count)}});
    const auto ratings = invoke(c, callee::allocate, 0x10477b6c, {bytes});
    c.set(31, ratings);
    if (ratings == 0) {
        guest::report_assertion(c, {0x10477b88, pool, pool + 0xe, pool + 0x1b, frame.at(0x40), frame.at(0x38), 0x292b});
        return;
    }
    invoke(c, callee::fill_memory, 0x10477bdc, {ratings, 0xfffe7960u, bytes});
    // Rate every club (an empty slot rates 0).
    c.set({{26, 0}});
    std::uint32_t done = 0;
    for (; std::int32_t(done) < count; ++done) {
        c.set({{25, done}, {27, clubs + done * 4}, {28, ratings + done * 4}});
        const auto club = c.u32(clubs + done * 4);
        std::uint32_t rating = 0;
        if (club != 0) {
            c.set(3, club);
            c.link(0x10477c00);
            rating = std::uint32_t(club_strength(c, club));
        }
        c.put32(ratings + done * 4, rating);
    }
    c.set({{25, done}, {27, clubs + done * 4}, {28, ratings + done * 4}});
    // Selection sort: the best rating to the front, swapping the club along with it.
    const std::int32_t last = count - 1;
    std::uint32_t loaded_second = 0;  // r9: what the last comparison (or swap) loaded
    for (std::int32_t first = 0; first < last; ++first)
        for (std::int32_t second = first + 1; second < count; ++second) {
            const auto first_at = std::uint32_t(first) * 4, second_at = std::uint32_t(second) * 4;
            const auto first_rating = c.i32(ratings + first_at);
            const auto second_rating = c.i32(ratings + second_at);
            loaded_second = std::uint32_t(second_rating);
            if (first_rating < second_rating) {
                c.put32(ratings + first_at, std::uint32_t(second_rating));
                c.put32(ratings + second_at, std::uint32_t(first_rating));
                const auto first_club = c.u32(clubs + first_at);
                loaded_second = first_club;
                c.put32(clubs + first_at, c.u32(clubs + second_at));
                c.put32(clubs + second_at, first_club);
            }
        }
    // The registers the loops leave for the release's caller to see: r4-r6 the cursors, r7 the last index, r8 the
    // byte offset, r9 the last word loaded, r10 the outer count (only when a comparison ran).
    c.set(7, std::uint32_t(last));
    if (last >= 1) {
        const auto end = std::uint32_t(count) * 4;
        c.set({{4, ratings + std::uint32_t(last) * 4}, {5, clubs + end}, {6, ratings + end}, {8, end}, {9, loaded_second}, {10, std::uint32_t(last)}});
    } else {
        c.set({{4, ratings}, {10, 0}});
    }
    invoke(c, callee::release, 0x10477cb0, {ratings});
}
}
