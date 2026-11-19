// LEAGUE.CPP, block `storage`: cases for the league's copies, its per-club arrays and the save writer, from the
// real career (--world): the league objects, their standings rows (with head-to-head blocks and history blocks),
// the clubs. What the career lacks is built by writing real values into a case's own memory (the writes are undone
// after each case): a league's count, history size, tie-break bytes, a row's block pointers (pointing at other real
// allocations and at the club table), a stage array of real stage objects, the writer's row arrays (scratch).
// The writer's file calls are answered (open: a handle in the scratch page, fwrite: 1, close: 0), so what the
// original and the C++ pass to each fwrite is compared by the call trace (pointer, size, count, file, r7-r10).
#include "league_base_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
constexpr std::uint32_t copy_base = 0x1049e820, clone = 0x1049e680, place_club = 0x104a60e0, copy_standings = 0x104a6440,
                        free_standings = 0x104a6660, club_list = 0x104a6760, save = 0x104a8530;
constexpr std::uint32_t new_array = 0x10775130, open_file = 0x10739de0, write_file = 0x10776360, close_file = 0x107766f0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, clubs_slot = -0x7be8, club_count_slot = -0x7b50;
constexpr std::uint32_t row_size = 0x44, club_size = 0x248;

std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write w16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
std::vector<Write> pattern(std::uint32_t at, std::uint32_t bytes, std::uint32_t seed = 0xa5a5a5a5u) {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < bytes; offset += 4) writes.push_back(w32(at + offset, seed + offset));
    return writes;
}

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> leagues, kind_one, other_kind;
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) {
            if (!object || !seen.insert(object).second) return;
            if (is_league(object)) leagues.push_back(object);
            else if (u8(object + 0x43) == 1) kind_one.push_back(object);
            else if (u8(object + 0x43) != 2 && s.r32(object) >= 0x108b0000 && s.r32(object) < 0x10a00000) other_kind.push_back(object);
        };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    std::uint32_t u8(std::uint32_t at) const { return (s.r32(at & ~3u) >> (24 - 8 * (at & 3))) & 0xff; }
    std::int32_t i16(std::uint32_t at) const { return std::int16_t(u8(at) << 8 | u8(at + 1)); }
    bool is_league(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || u8(object + 0x43) != 2) return false;
        for (std::uint32_t slot = 0x88; slot <= 0xb8; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= 0x1049e4f0 && code < 0x104aac14) return true;
        }
        return false;
    }
    std::int32_t clubs(std::uint32_t league) const { return i16(league + 0x3e); }
    std::uint32_t rows(std::uint32_t league) const { return s.r32(league + 0xb0); }
    bool has_opponents(std::uint32_t league) const { return rows(league) && s.r32(rows(league) + 8) != 0; }
    bool has_history(std::uint32_t league) const { return rows(league) && s.r32(rows(league) + 0xc) != 0; }
    bool active(std::uint32_t league) const { return u8(league + 0x45) != 0; }
    std::uint32_t club_table() const { return s.global(clubs_slot); }
    std::uint32_t club(unsigned index) const { return club_table() + index * club_size; }
    std::uint32_t club_count() const { return s.global(club_count_slot); }
};

// Leagues of the career told apart by what the routines branch on, a few of each.
struct Picks {
    std::vector<std::uint32_t> opponents, plain, inactive, small, stages, all;
    explicit Picks(const World& w) {
        for (const auto league : w.leagues) {
            if (!w.rows(league) || w.clubs(league) <= 0) continue;
            if (!w.active(league)) { if (inactive.size() < 2) inactive.push_back(league); continue; }
            if (w.has_opponents(league)) { if (opponents.size() < 4) opponents.push_back(league); }
            else if (w.has_history(league)) { if (plain.size() < 3) plain.push_back(league); }
            if (w.clubs(league) <= 4 && small.size() < 2) small.push_back(league);
            if (w.s.r32(league + 0xc) && std::int32_t(w.s.r32(league + 0x30)) >= 0 && stages.size() < 1) stages.push_back(league);
        }
        for (const auto* list : {&opponents, &plain, &inactive, &small, &stages})
            for (const auto league : *list) if (std::find(all.begin(), all.end(), league) == all.end()) all.push_back(league);
    }
};

// A row's pointers into real memory where the career has none: the club table stands in for a block.
std::vector<Write> block_pointers(const World& w, std::uint32_t rows, int count, std::uint32_t field, std::uint32_t area, std::uint32_t stride) {
    std::vector<Write> writes;
    for (int i = 0; i < count; ++i) writes.push_back(w32(rows + std::uint32_t(i) * row_size + field, w.club_table() + area + std::uint32_t(i) * stride));
    return writes;
}

LEAGUE_CASES("storage") {
    if (scenario.match) return;
    const World w(scenario);
    const Picks p(w);
    if (p.all.empty()) throw std::runtime_error("The career has no league with standings");
    std::fprintf(stderr, "storage: %zu leagues (%zu with head-to-head blocks, %zu plain, %zu inactive, %zu with stages), %zu kind-1 objects, %zu of other kinds\n",
                 w.leagues.size(), p.opponents.size(), p.plain.size(), p.inactive.size(), p.stages.size(), w.kind_one.size(), w.other_kind.size());
    const auto fresh = scratch_base + 0x100, work = scratch_base + 0x400, cells = scratch_base + 0x800;
    const auto lab = [&](const char* what, std::uint32_t league) { return std::string("storage: ") + what + " " + hex(league); };

    // The base copy and the match screens' copy.
    for (const auto league : p.all) {
        cases.push_back({.entry = copy_base, .registers = {{3, fresh}, {4, league}}, .memory = pattern(fresh, 0xc0), .label = lab("copy_base", league)});
        cases.push_back({.entry = clone, .registers = {{3, fresh}, {4, work}, {5, league}}, .memory = joined(pattern(fresh, 0x100), pattern(work, 0x100)),
                         .label = lab("clone", league)});
    }
    const auto league0 = p.opponents.empty() ? p.all.front() : p.opponents.front();
    const auto clone_with = [&](const char* what, std::uint32_t league, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {},
                                std::vector<Case::NthAnswer> nth = {}) {
        cases.push_back({.entry = clone, .registers = {{3, fresh}, {4, work}, {5, league}}, .memory = joined(joined(pattern(fresh, 0x100), pattern(work, 0x100)), memory),
                         .answers = std::move(answers), .nth_answers = std::move(nth), .label = lab(what, league)});
    };
    clone_with("clone, standings allocation fails", league0, {}, {}, {{new_array, 2, 0}});
    clone_with("clone, history allocation fails", p.plain.empty() ? league0 : p.plain.front(), {}, {}, {{new_array, 3, 0}});
    clone_with("clone, no standings", league0, {w32(league0 + 0xb0, 0)});
    clone_with("clone, no clubs", league0, {w16(league0 + 0x3e, 0)});
    clone_with("clone, 0x33 clubs", league0, {w16(league0 + 0x3e, 0x33), w16(league0 + 0xa4, 0xffff)});
    clone_with("clone, 0x33 clubs, no blocks", league0,
               {w16(league0 + 0x3e, 0x33), w16(league0 + 0xa4, 0xffff), w32(w.rows(league0) + 8, 0), w32(w.rows(league0) + 0xc, 0)});

    // copy_standings: the whole matrix of blocks.
    for (const auto league : p.all) cases.push_back({.entry = copy_standings, .registers = {{3, league}}, .label = lab("copy_standings", league)});
    const auto copy_with = [&](const char* what, std::uint32_t league, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {},
                               std::vector<Case::NthAnswer> nth = {}) {
        cases.push_back({.entry = copy_standings, .registers = {{3, league}}, .memory = std::move(memory), .answers = std::move(answers),
                         .nth_answers = std::move(nth), .label = lab(what, league)});
    };
    const auto rows0 = w.rows(league0);
    copy_with("copy_standings, no standings", league0, {w32(league0 + 0xb0, 0)});
    copy_with("copy_standings, no clubs", league0, {w16(league0 + 0x3e, 0)});
    copy_with("copy_standings, allocation fails", league0, {}, {{new_array, 0}});
    copy_with("copy_standings, history unset", league0, {w16(league0 + 0xa4, 0xffff)});
    copy_with("copy_standings, history unset, 0x33 clubs", league0, {w16(league0 + 0xa4, 0xffff), w16(league0 + 0x3e, 0x33)});
    copy_with("copy_standings, history unset, one club", league0, {w16(league0 + 0xa4, 0xffff), w16(league0 + 0x3e, 1)});
    // The 0x33-club assertion with no blocks to walk (the loops would follow a row past the array otherwise).
    copy_with("copy_standings, history unset, 0x33 clubs, no blocks", league0,
              {w16(league0 + 0xa4, 0xffff), w16(league0 + 0x3e, 0x33), w32(rows0 + 8, 0), w32(rows0 + 0xc, 0)});
    copy_with("copy_standings, history unset, 0x32 clubs, no blocks", league0,
              {w16(league0 + 0xa4, 0xffff), w16(league0 + 0x3e, 0x32), w32(rows0 + 8, 0), w32(rows0 + 0xc, 0)});
    copy_with("copy_standings, history unset, negative clubs", league0, {w16(league0 + 0xa4, 0xffff), w16(league0 + 0x3e, 0xffff)});
    copy_with("copy_standings, no head-to-head in row 0", league0, {w32(rows0 + 8, 0)});
    copy_with("copy_standings, no history in row 0", league0, {w32(rows0 + 0xc, 0)});
    copy_with("copy_standings, head-to-head blocks first allocation fails", league0, {}, {}, {{new_array, 2, 0}});
    if (!p.plain.empty()) {
        const auto plain = p.plain.front();
        const auto n = w.clubs(plain);
        copy_with("copy_standings, blocks made up on a plain league", plain, block_pointers(w, w.rows(plain), n, 8, 0x30000, 0x400));
        copy_with("copy_standings, history allocation fails", plain, {}, {}, {{new_array, 2, 0}});
    }

    // place_club: where the career's rows are real (every row has its blocks), and with the tie-break bytes, counts and
    // history size that steer each branch.
    const auto club_a = w.club(3), club_b = w.club(5);
    for (const auto league : p.all) {
        const auto n = w.clubs(league);
        for (const std::uint32_t slot : {0u, std::uint32_t(n - 1), std::uint32_t(n / 2), std::uint32_t(n)})
            cases.push_back({.entry = place_club, .registers = {{3, league}, {4, slot}, {5, club_a}, {6, 1}, {7, 0}}, .label = lab(("place_club slot " + std::to_string(slot) + " of").c_str(), league)});
        cases.push_back({.entry = place_club, .registers = {{3, league}, {4, std::uint32_t(n - 1)}, {5, 0}, {6, 0xff}, {7, 0}},
                         .label = lab("place_club, no club, last slot", league)});
    }
    const auto place_with = [&](const char* what, std::uint32_t league, std::uint32_t slot, std::uint32_t club, std::uint32_t flag, std::uint32_t rows,
                                std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}, std::vector<Case::NthAnswer> nth = {}) {
        cases.push_back({.entry = place_club, .registers = {{3, league}, {4, slot}, {5, club}, {6, flag}, {7, rows}}, .memory = std::move(memory),
                         .answers = std::move(answers), .nth_answers = std::move(nth), .label = lab(what, league)});
    };
    const auto plain0 = p.plain.empty() ? league0 : p.plain.front();
    const int plain_n = w.clubs(plain0), n0 = w.clubs(league0);
    const auto tie = [&](std::uint32_t league, std::uint32_t b0, std::uint32_t b1, std::uint32_t b2, std::uint32_t b3) {
        return std::vector<Write>{w8(league + 0xc7, b0), w8(league + 0xc8, b1), w8(league + 0xc9, b2), w8(league + 0xca, b3)};
    };
    // A club in state 3 goes to 2; any other state stays.
    place_with("place_club, club state 3", league0, 1, club_a, 1, 0, {w8(club_a + 0x23d, 3)});
    place_with("place_club, club state 2", league0, 1, club_a, 1, 0, {w8(club_a + 0x23d, 2)});
    place_with("place_club, history unset", league0, 1, club_a, 1, 0, {w16(league0 + 0xa4, 0xffff)});
    place_with("place_club, history unset, 0x33 clubs", league0, 1, club_a, 1, 0, {w16(league0 + 0xa4, 0xffff), w16(league0 + 0x3e, 0x33)});
    place_with("place_club, history unset, no clubs", league0, 1, club_a, 1, 0, {w16(league0 + 0xa4, 0xffff), w16(league0 + 0x3e, 0)});
    place_with("place_club, no rounds", league0, 1, club_a, 1, 0, {w16(league0 + 0x3c, 0)});
    place_with("place_club, negative rounds", league0, 1, club_a, 1, 0, {w16(league0 + 0x3c, 0xffff)});
    place_with("place_club, history allocation fails", league0, 1, club_a, 1, 0, {}, {{new_array, 0}});
    // The tie-break bytes (+0xc7..+0xca): the first zero ends the search; a 4 asks for the opponent block.
    for (const auto& bytes : std::vector<std::array<std::uint32_t, 4>>{{0, 4, 0, 0}, {1, 2, 3, 4}, {1, 2, 3, 3}, {4, 0, 0, 0}, {1, 4, 0, 0}, {0x84, 4, 0, 0}, {3, 0xfc, 4, 0}}) {
        const auto name = std::string("place_club, tie ") + hex(bytes[0]) + hex(bytes[1]) + hex(bytes[2]) + hex(bytes[3]);
        // Not the last slot: only the opponent block of this row is allocated and cleared.
        place_with(name.c_str(), plain0, 0, club_a, 1, 0, tie(plain0, bytes[0], bytes[1], bytes[2], bytes[3]));
        // The last slot with every row's block real (made up from the club table): the others' blocks are filled.
        place_with((name + ", last slot").c_str(), plain0, std::uint32_t(plain_n - 1), club_a, 1, 0,
                   joined(tie(plain0, bytes[0], bytes[1], bytes[2], bytes[3]), block_pointers(w, w.rows(plain0), plain_n, 8, 0x30000, 0x400)));
    }
    place_with("place_club, last slot of a head-to-head league", league0, std::uint32_t(n0 - 1), club_a, 1, 0, {});
    place_with("place_club, last slot, row 0 without blocks", league0, std::uint32_t(n0 - 1), club_a, 1, 0, {});
    place_with("place_club, head-to-head allocation fails", plain0, 0, club_a, 1, 0, tie(plain0, 4, 0, 0, 0), {}, {{new_array, 2, 0}});
    // One club: the last slot is slot 0 and the fill loops have no pair; two clubs.
    for (const int clubs : {1, 2, 3}) {
        auto memory = joined(tie(plain0, 4, 0, 0, 0), block_pointers(w, w.rows(plain0), clubs, 8, 0x30000, 0x400));
        memory.push_back(w16(plain0 + 0x3e, std::uint32_t(clubs)));
        place_with(("place_club, " + std::to_string(clubs) + " clubs, last slot").c_str(), plain0, std::uint32_t(clubs - 1), club_a, 0, 0, memory);
    }
    // No clubs and slot -1: the opponent block's size wraps; the allocator answers with a real address.
    {
        auto memory = tie(plain0, 4, 0, 0, 0);
        memory.push_back(w16(plain0 + 0x3e, 0));
        place_with("place_club, no clubs, slot -1", plain0, 0xff, club_a, 0, 0, memory, {{new_array, w.club_table() + 0x30000}});
    }
    // Rows of the case's own (scratch), the league's untouched.
    const auto scratch_rows = scratch_base + 0x1000;
    place_with("place_club, own rows", plain0, 2, club_a, 3, scratch_rows, {});
    place_with("place_club, own rows, head-to-head last slot", plain0, std::uint32_t(plain_n - 1), club_b, 3, scratch_rows,
               joined(tie(plain0, 4, 0, 0, 0), block_pointers(w, scratch_rows, plain_n, 8, 0x30000, 0x400)));
    place_with("place_club, no club", plain0, 2, 0, 0, scratch_rows, {});

    // free_standings: the league's own (cleared) and given rows (kept), with and without blocks.
    for (const auto league : p.all) {
        cases.push_back({.entry = free_standings, .registers = {{3, league}, {4, 0}}, .label = lab("free_standings own", league)});
        cases.push_back({.entry = free_standings, .registers = {{3, league}, {4, w.rows(league)}}, .label = lab("free_standings given", league)});
    }
    const auto free_with = [&](const char* what, std::uint32_t league, std::uint32_t rows, std::vector<Write> memory) {
        cases.push_back({.entry = free_standings, .registers = {{3, league}, {4, rows}}, .memory = std::move(memory), .label = lab(what, league)});
    };
    free_with("free_standings, no standings", league0, 0, {w32(league0 + 0xb0, 0)});
    free_with("free_standings, no clubs", league0, 0, {w16(league0 + 0x3e, 0)});
    free_with("free_standings, negative clubs", league0, 0, {w16(league0 + 0x3e, 0xffff)});
    free_with("free_standings, no history in row 0", league0, 0, {w32(rows0 + 0xc, 0)});
    free_with("free_standings, no head-to-head in row 0", league0, 0, {w32(rows0 + 8, 0)});
    free_with("free_standings, row 1 without blocks", league0, 0, {w32(rows0 + row_size + 8, 0), w32(rows0 + row_size + 0xc, 0)});
    free_with("free_standings, given rows of nothing", league0, 0, {});
    free_with("free_standings, given null rows and none own", league0, 0, {w32(league0 + 0xb0, 0), w16(league0 + 0x3e, 3)});
    free_with("free_standings, given rows, no clubs", league0, rows0, {w16(league0 + 0x3e, 0)});

    // club_list: the club words into a new array; the count; the allocation failure.
    for (const auto league : p.all) cases.push_back({.entry = club_list, .registers = {{3, league}, {4, cells}, {5, cells + 4}}, .memory = pattern(cells, 8), .label = lab("club_list", league)});
    cases.push_back({.entry = club_list, .registers = {{3, league0}, {4, cells}, {5, cells + 4}}, .memory = pattern(cells, 8),
                     .answers = {{new_array, 0}}, .label = lab("club_list, allocation fails", league0)});
    cases.push_back({.entry = club_list, .registers = {{3, league0}, {4, cells}, {5, cells + 4}}, .memory = joined(pattern(cells, 8), {w16(league0 + 0x3e, 0)}),
                     .label = lab("club_list, no clubs", league0)});
    cases.push_back({.entry = club_list, .registers = {{3, league0}, {4, cells}, {5, cells + 4}}, .memory = joined(pattern(cells, 8), {w16(league0 + 0x3e, 0xffff)}),
                     .label = lab("club_list, negative clubs", league0)});

    // The save writer.
    const auto handle = scratch_base + 0x8000;
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> io = {{open_file, handle}, {write_file, 1}, {close_file, 0}};
    const auto save_with = [&](const char* what, std::uint32_t league, std::uint32_t temporary, std::vector<Write> memory = {},
                               std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
        if (answers.empty()) answers = io;
        cases.push_back({.entry = save, .registers = {{3, league}, {4, temporary}}, .memory = std::move(memory), .answers = std::move(answers), .label = lab(what, league)});
    };
    for (const auto league : p.all) {
        save_with("save temporary", league, 1);
        save_with("save by year", league, 0);
        save_with("save, temporary flag bytes", league, 0x100);
    }
    save_with("save, file does not open", league0, 1, {}, {{open_file, 0}, {write_file, 1}, {close_file, 0}});
    save_with("save, by year, does not open", league0, 0, {}, {{open_file, 0}, {write_file, 1}, {close_file, 0}});
    save_with("save, a slash in the name", league0, 1, {w32(w.s.r32(league0 + 4) + 0x1a, 0x412f4200u)});
    save_with("save, only slashes in the name", league0, 1, {w32(w.s.r32(league0 + 4) + 0x1a, 0x2f2f2f00u)});
    // An inactive league (+0x45): nothing is written by year, but a temporary save still is.
    save_with("save, inactive league by year", league0, 0, {w8(league0 + 0x45, 0)});
    save_with("save, inactive league temporary", league0, 1, {w8(league0 + 0x45, 0)});
    save_with("save, inactive league, flag byte only", league0, 0x100, {w8(league0 + 0x45, 0)});
    save_with("save, negative year flag", league0, 0xff, {});
    save_with("save, no stats", league0, 1, {w32(league0 + 8, 0)});
    save_with("save, no record", league0, 1, {w32(league0 + 4, 0)});
    save_with("save, no clubs", league0, 1, {w16(league0 + 0x3e, 0)});
    save_with("save, negative clubs", league0, 1, {w16(league0 + 0x3e, 0xffff)});
    save_with("save, one club", league0, 1, {w16(league0 + 0x3e, 1)});
    save_with("save, history unset", league0, 1, {w16(league0 + 0xa4, 0xffff)});
    save_with("save, no history in row 0", league0, 1, {w32(rows0 + 0xc, 0)});
    save_with("save, no head-to-head in row 0", league0, 1, {w32(rows0 + 8, 0)});
    save_with("save, no club in row 0", league0, 1, {w32(rows0, 0)});
    if (!p.plain.empty()) {
        const auto plain = p.plain.front();
        const auto n = w.clubs(plain);
        auto blocks = block_pointers(w, w.rows(plain), n, 8, 0x30000, 0x400);
        blocks.push_back(w32(w.rows(plain) + row_size, 0));  // a null club in the second row's own block
        save_with("save, head-to-head blocks made up", plain, 1, blocks);
        save_with("save, history unset, plain", plain, 1, {w16(plain + 0xa4, 0xffff)});
    }
    // Round rows (+0x14: 8-byte rows, a club pointer first), the 0x18-byte rows (+0x18) and the stage list.
    const auto round_rows = scratch_base + 0x4000, rows_18 = scratch_base + 0x5000, stage_list = scratch_base + 0x6000;
    const auto row_memory = [&](std::uint32_t count, bool null_club) {
        std::vector<Write> writes{w32(league0 + 0x14, round_rows), w16(league0 + 0x36, count), w16(league0 + 0x38, count)};
        for (std::uint32_t i = 0; i < count; ++i) {
            writes.push_back(w32(round_rows + i * 8, null_club && i == 1 ? 0 : w.club(i + 2)));
            writes.push_back(w32(round_rows + i * 8 + 4, 0x11111111u * (i + 1)));
        }
        return writes;
    };
    save_with("save, 3 round rows", league0, 1, row_memory(3, false));
    save_with("save, 3 round rows, one without club", league0, 1, row_memory(3, true));
    save_with("save, 1 round row", league0, 1, row_memory(1, false));
    {
        auto memory = row_memory(3, false);
        memory.push_back(w16(league0 + 0x38, 2));
        save_with("save, 3 round rows, 2 saved", league0, 1, memory);
        auto none = row_memory(0, false);
        save_with("save, no round rows", league0, 1, none);
        auto zero_pointer = row_memory(0, false);
        zero_pointer.push_back(w32(league0 + 0x14, 0));
        zero_pointer.push_back(w16(league0 + 0x38, 4));
        save_with("save, round rows count without block", league0, 1, zero_pointer);
        auto negative = row_memory(0, false);
        negative.push_back(w16(league0 + 0x36, 0xffff));
        save_with("save, negative round row count", league0, 1, negative);
    }
    save_with("save, 0x18-byte rows", league0, 1, {w32(league0 + 0x18, rows_18), w16(league0 + 0x34, 3)});
    save_with("save, 0x18-byte rows, none counted", league0, 1, {w32(league0 + 0x18, rows_18), w16(league0 + 0x34, 0)});
    // Stages: the league's own with real stage objects (kind 2), a cup's (kind 1), and objects of another kind.
    const auto with_stages = [&](const char* what, std::vector<std::uint32_t> objects, std::int32_t last, std::vector<Write> more = {}) {
        std::vector<Write> memory{w32(league0 + 0xc, stage_list), w32(league0 + 0x30, std::uint32_t(last))};
        for (std::size_t i = 0; i < objects.size(); ++i) memory.push_back(w32(stage_list + std::uint32_t(i) * 4, objects[i]));
        save_with(what, league0, 1, joined(memory, more));
    };
    const auto kind_two = p.opponents.size() > 1 ? p.opponents[1] : p.all.back();
    with_stages("save, one stage of kind 2", {kind_two}, 0);
    if (!w.kind_one.empty()) {
        with_stages("save, one stage of kind 1", {w.kind_one.front()}, 0);
        with_stages("save, stages of kinds 1 and 2", {w.kind_one.front(), kind_two}, 1);
        if (w.kind_one.size() > 1) with_stages("save, two stages of kind 1", {w.kind_one[0], w.kind_one[1]}, 1);
        with_stages("save, a stage of kind 1 without rounds", {w.kind_one.front()}, 0, {w16(w.kind_one.front() + 0x3c, 0)});
        with_stages("save, a stage of kind 1 with negative rounds", {w.kind_one.front()}, 0, {w16(w.kind_one.front() + 0x3c, 0xffff)});
    }
    if (!w.other_kind.empty()) with_stages("save, a stage of another kind", {w.other_kind.front()}, 0);
    with_stages("save, last stage -1", {kind_two}, -1);
    with_stages("save, last stage -2", {kind_two}, -2);
    for (const auto league : p.stages)
        cases.push_back({.entry = save, .registers = {{3, league}, {4, 1}}, .answers = io, .label = lab("save, the career's stages of", league)});
}
}
}
