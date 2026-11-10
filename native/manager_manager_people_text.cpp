// MANAGER_MANAGER.CPP people text block (manager_manager_people_text.hpp): the text chooser 0x101980a0 and
// its three helpers 0x10197e00, 0x10197e80 and 0x10197f00.
//
// The two with a frame (0x10197f00, 0x101980a0) keep the original's stack image (the prologue stores, the
// helper's two bytes at sp+0x38 / 0x39, the nation's name at sp+0x3c, the sentences in the buffers at
// sp+0x488 and sp+0xa0), which this module's checker compares, and all four the registers the original
// holds at each call and leaves at its end. The formatter is varargs and spills r3-r10 into its frame, so
// each of r3-r10 the original sets for a call is set here and the others stay as the callee before left them.
//
// A leftover-stack reader no capture reaches: for a club 0x100c7d00 flags whose nation record (+0x2c) is
// null, the original formats the bytes at sp+0x3c as a name without having written them (0x1019c28c on),
// so that text holds whatever earlier code left there. Kept as it is; see `flagged_text`.
#include "manager_manager_people_text.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"
#include "manager_manager_people_text_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Frame;
using guest::compare;
using guest::compare_logical;
using namespace people_text;
namespace {
namespace callee {
constexpr std::uint32_t formatter = 0x10727890, text_copy = 0x1077b720, club_flag = 0x100c7d00,
    employer_record = 0x10103510, level_of = 0x100f55b0, days_between = 0x10718130, nation_name = 0x10168a00,
    league_entry = 0x10459b50;
}
namespace toc {
constexpr std::int32_t today = -0x7b80, leagues = -0x7b5c, employers = -0x7ab8;
}
// The module's row per club (object[0] + club[0] * 0x4c) and the club record's fields the texts read.
constexpr std::uint32_t row_size = 0x4c;
namespace row {
constexpr std::uint32_t first_rating = 0x6, second_rating = 0x12, third_rating = 0x16, date = 0x44;  // date: two words
}
namespace club {
constexpr std::uint32_t nation = 0x2c, league = 0x30, name = 0x1ee, name_class = 0x242;
}
namespace frame_slot {
constexpr std::uint32_t count = 0x38, kind = 0x39, nation_name = 0x3c, sentence = 0x488, rest = 0xa0;
}

std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
std::uint32_t row_of(Cpu& c, std::uint32_t object, std::uint32_t club) {
    return c.u32(object) + c.u32(club) * row_size;
}

// The cuts the two band helpers share: the band is the count of cuts the rating reached, the first four
// compares in order; past the last, the top two bands are told apart by a sixth cut (and r4 holds it).
struct Bands {
    std::uint32_t rating_at;
    std::int32_t cuts[5];
    std::int32_t last;
};
std::uint32_t band_of(Cpu& c, std::uint32_t object, std::uint32_t club, const Bands& bands) {
    const auto rating = std::int32_t(std::int16_t(c.u16(row_of(c, object, club) + bands.rating_at)));
    for (unsigned band = 0; band < 5; ++band) {
        compare(c, rating, bands.cuts[band]);
        if (rating < bands.cuts[band]) return guest::result(c, band);
    }
    // `subfc` / `subfe`: 5 below the last cut, 6 at or above it (the rating is positive here).
    c.set(4, word(bands.last));
    return guest::result(c, rating >= bands.last ? 6 : 5);
}
}  // namespace

// 0x10197e00. No frame, no calls.
std::uint32_t club_first_band(Cpu& c, std::uint32_t object, std::uint32_t club) {
    return band_of(c, object, club, {row::first_rating, {0xbb8, 0xdac, 0x10b3, 0x1964, 0x2134}, 0x251c});
}
// 0x10197e80. No frame, no calls.
std::uint32_t club_second_band(Cpu& c, std::uint32_t object, std::uint32_t club) {
    return band_of(c, object, club, {row::second_rating, {-0x7d0, -0x3e8, -0x1f4, 0x1f4, 0x3e8}, 0x7d0});
}

// 0x10197f00. Frame 0x50 (r28-r31 saved, LR at +8).
void club_upcoming_entry(Cpu& c, std::uint32_t, std::uint32_t club, std::uint32_t kind_at, std::uint32_t count_at) {
    Frame frame(c, 0x10197f00, 0x50, {.first_gpr = 28});
    const auto today = c.toc(toc::today);
    guest::keep(c, {{31, count_at}, {30, kind_at}, {28, today}});
    // The four entries of the league's record in the order the original tries them: where the count
    // is, the date's two words, and the kind it answers; the call after each is a return address.
    struct Entry {
        std::uint32_t count, date_a, date_b;
        std::uint8_t kind;
        std::uint32_t back;
    };
    static constexpr Entry entries[] = {{0x1c, 0x28, 0x2c, 1, 0x10197f84}, {0x30, 0x3c, 0x40, 3, 0x10197fbc},
                                        {0x58, 0x64, 0x68, 4, 0x10197ff4}, {0x44, 0x50, 0x54, 2, 0x1019802c}};
    const auto nothing = [&] {
        c.set(3, 5);
        c.put8(kind_at, 5);
        c.put8(count_at, 0);
    };
    const auto league = c.u32(club + club::league);
    c.set(7, league);
    if (!guest::nonzero_logical(c, league)) return nothing();
    const auto table = c.u32(c.toc(toc::leagues));
    c.set(3, c.u32(table + c.u32(league) * 4));
    if (!guest::nonzero_logical(c, c.r(3))) return nothing();
    c.set(3, c.u32(c.r(3) + 8));
    if (!guest::nonzero_logical(c, c.r(3))) return nothing();
    c.call(callee::league_entry, 0x10197f60);
    const auto record = c.r(3);
    c.set(29, record);
    if (!guest::nonzero(c, std::int32_t(record))) return nothing();
    for (const auto& entry : entries) {
        const auto count = c.i32(record + entry.count);
        compare(c, count, 1);
        if (count <= 1) continue;
        c.set({{4, c.u32(record + entry.date_a)}, {3, today}, {5, c.u32(record + entry.date_b)}});
        c.call(callee::days_between, entry.back);
        compare(c, std::int32_t(c.r(3)), 10);
        if (std::int32_t(c.r(3)) >= 10) continue;
        c.put8(kind_at, entry.kind);
        c.put8(count_at, c.u32(record + entry.count));
        return;
    }
    nothing();
}

namespace {
constexpr std::span<const Node> direct{};  // a cell with no test: its first leaf
#include "manager_manager_people_text_tables.inc"

// The relation of 0x100f55b0 (-2..2) as three-way nodes: the exit is 0, 1, 2, 3 for -1 or 4 for -2; 5 outside.
enum Relation { zero, one, two, minus_one, minus_two, other_relation };
constexpr Node relation_nodes[] = {{0, node(2), leaf(zero), node(1)},
                                   {2, leaf(one), leaf(two), leaf(other_relation)},
                                   {-2, leaf(other_relation), leaf(minus_two), leaf(minus_one)}};
// The second band (0..6) as groups: its own tables for 0, 1 and 2, one for 3..6; 4 outside 0..6.
constexpr Node group_nodes[] = {{2, node(2), leaf(2), node(1)}, {7, leaf(3), leaf(4), leaf(4)}, {0, leaf(4), leaf(0), leaf(1)}};
constexpr int outside_group = 4;
// The text for a relation the groups do not cover (the second band outside 0..6, the relation outside
// -2..2): copied, not formatted.
struct Fallback {
    std::uint16_t format;
    std::uint32_t back;
};
constexpr Fallback group_fallback[5] = {{0x4d30, 0x1019aff8}, {0x4d30, 0x1019a1f4}, {0x4d30, 0x10199710},
                                        {0x4d30, 0x1019b8fc}, {0xf7ab, 0x1019c274}};
constexpr Fallback relation_fallback = {0xf7cb, 0x1019c288};
// The club flagged by 0x100c7d00: a text by the first band's rating (cuts at 0x251c, 0x2134, 0x1964,
// 0x10b3, 0xdac; the sixth text below all), formatted with the second text (sp+0x3c) and 100 twice.
constexpr Node flagged_nodes[] = {{0x251c, node(1), node(1), leaf(0)}, {0x2134, node(2), node(2), leaf(1)},
                                  {0x1964, node(3), node(3), leaf(2)}, {0x10b3, node(4), node(4), leaf(3)},
                                  {0xdac, leaf(5), leaf(5), leaf(4)}};
constexpr Leaf flagged_leaves[6] = {{0x10000 - 0x818, 0x1019c2e0}, {0x10000 - 0x7aa, 0x1019c308},
                                    {0x10000 - 0x739, 0x1019c330}, {0x10000 - 0x6cd, 0x1019c358},
                                    {0x10000 - 0x65f, 0x1019c380}, {0x10000 - 0x5ef, 0x1019c3a0}};

class Chooser {
public:
    Chooser(Cpu& c, Frame& frame, std::uint32_t object, std::uint32_t club, std::uint32_t out)
        : c(c), frame(frame), object(object), club(club), out(out), pool(c.toc(slots::pool)),
          sentence(frame.at(frame_slot::sentence)), rest(frame.at(frame_slot::rest)) {}

    // The days, the two bands and the helper's bytes are known: the first band and the second band are
    // r27 and r26 of the original, kept in the registers the callees save.
    void choose(std::uint32_t days) {
        if (flagged()) {
            flagged_text();
            tail();
            return;
        }
        c.set(3, c.u32(c.toc(toc::employers)));
        c.set(4, club);
        c.call(callee::employer_record, 0x1019811c);
        c.call(callee::level_of, 0x10198120);
        compare(c, std::int32_t(days), 0x1e);
        if (std::int32_t(days) < 0x1e) return short_text();
        compare(c, std::int32_t(days), 0x3c);
        if (std::int32_t(days) < 0x3c) return middle_text();
        long_text();
    }

private:
    bool flagged() {
        c.set(3, club);
        c.call(callee::club_flag, 0x10198104);
        return compare(c, sbyte(c.r(3))) != 0;
    }
    // One sentence about the club: `buffer`, the format, the club's name class twice and its name; the
    // count byte at sp+0x38 as a fifth argument where the leaf says so.
    void say(std::uint32_t buffer, const Leaf& leaf) {
        const auto name_class = sbyte(c.u8(club + club::name_class));
        c.set({{3, buffer}, {4, pool + leaf.format}, {5, word(name_class)}, {6, word(name_class)}, {7, club + club::name}});
        if (leaf.with_count) c.set(8, word(sbyte(c.u8(frame.at(frame_slot::count)))));
        c.call(callee::formatter, leaf.back);
    }
    // A formatted text with only a buffer and a format (the extra texts of the tail).
    void say_plain(std::uint32_t buffer, std::uint16_t format, std::uint32_t back) {
        c.set({{3, buffer}, {4, pool + format}});
        c.call(callee::formatter, back);
    }
    void copy(std::uint32_t buffer, const Fallback& text) {
        c.set({{3, buffer}, {4, pool + text.format}});
        c.call(callee::text_copy, text.back);
    }
    std::int32_t relation() const { return sbyte(c.r(3)); }

    // Days under 30: one sentence by the relation, into the caller's buffer, and the routine is done.
    void short_text() {
        const auto exit = run(c, relation_nodes, relation());
        if (exit != other_relation) say(out, low_leaves[exit]);
    }

    // The jump-table case of a first band `band` in a table: r3 holds the table's target, and the cell's
    // script over the byte at sp+0x39 picks the sentence. Returns false when there is none.
    bool cell_text(const Cell& cell, std::uint32_t buffer) {
        c.set(3, cell.entry);
        int exit = 0;
        if (!cell.script.empty()) exit = run(c, cell.script, sbyte(c.u8(frame.at(frame_slot::kind))));
        if (exit == none) return false;
        say(buffer, cell.leaves[exit]);
        return true;
    }
    // Is the first band outside 0..6 (an unsigned compare of the extended byte)?
    bool band_outside(std::uint32_t band) {
        return compare_logical(c, word(sbyte(band)), 6) > 0;
    }

    // Days 30..59: by the relation, the table of the first band, into the caller's buffer.
    void middle_text() {
        const auto exit = run(c, relation_nodes, relation());
        if (exit == other_relation) return;
        const auto band = c.r(27);
        if (band_outside(band)) return;
        cell_text(band_tables[exit][sbyte(band)], out);
    }

    // Days 60 and more: the same by the second band too, into the sentence buffer, then the tail.
    void long_text() {
        const auto exit = run(c, relation_nodes, relation());
        if (exit == other_relation) {
            copy(sentence, relation_fallback);
            return tail();
        }
        const auto group = run(c, group_nodes, sbyte(c.r(26)));
        if (group == outside_group) {
            copy(sentence, group_fallback[exit]);
            return tail();
        }
        const auto band = c.r(27);
        if (!band_outside(band)) cell_text(pair_tables[exit][group][sbyte(band)], sentence);
        tail();
    }

    // The flagged club's text (from 0x1019c28c): the name of its nation record (+0x2c) into sp+0x3c,
    // then a sentence by the first band's rating with that name. Without a nation record the original
    // never writes sp+0x3c and still formats it: the bytes there are whatever was left on the stack.
    void flagged_text() {
        const auto nation = c.u32(club + club::nation);
        c.set(3, nation);
        if (guest::nonzero_logical(c, nation)) {
            c.set({{4, frame.at(frame_slot::nation_name)}, {5, 0x64}, {6, 9}});
            c.call(callee::nation_name, 0x1019c2a8);
        }
        const auto rating = std::int32_t(std::int16_t(c.u16(row_of(c, object, club) + row::first_rating)));
        const auto exit = run(c, flagged_nodes, rating);
        const auto& leaf = flagged_leaves[exit];
        c.set({{3, sentence}, {4, pool + leaf.format}, {7, frame.at(frame_slot::nation_name)}, {5, 0x64}, {6, 0x64}});
        c.call(callee::formatter, leaf.back);
    }

    // 0x1019c3a0: the sentence in the buffer at sp+0x488 gets a second one at sp+0xa0 (from the club's
    // ratings and how soon its date is) and both go to the output buffer.
    void tail() {
        auto row = row_of(c, object, club);
        c.set({{3, c.toc(toc::today)}, {4, c.u32(row + row::date)}, {5, c.u32(row + row::date + 4)}});
        c.call(callee::days_between, 0x1019c3c0);
        const auto soon = std::int32_t(c.r(3));
        compare(c, soon, 0x78);
        bool by_rating = soon >= 0x78;
        if (!by_rating) {
            const auto rating = rating_at(row, row::first_rating);
            compare(c, rating, 0x1964);
            if (rating >= 0x1964) by_rating = true;
            else {
                compare(c, rating, 0x10b3);
                if (rating > 0x10b3) {
                    const auto third = compare_third(row, 0x5dc);
                    say(sentence, third > 0 ? Leaf{0x10000 - 0x579, 0x1019c418} : Leaf{0x10000 - 0x453, 0x1019c43c});
                } else {
                    compare(c, rating, 0xdac);
                    if (rating > 0xdac) {
                        const auto third = compare_third(row, 0x5dc);
                        say(sentence, third > 0 ? Leaf{0x10000 - 0x3ab, 0x1019c474} : Leaf{0x10000 - 0x27f, 0x1019c498});
                    }
                }
            }
        }
        if (by_rating) rest_text();
        join();
    }
    std::int32_t rating_at(std::uint32_t row, std::uint32_t field) { return std::int32_t(std::int16_t(c.u16(row + field))); }
    // The third rating against `limit` (cr0 as the compare leaves it); its order.
    std::int32_t compare_third(std::uint32_t row, std::int32_t limit) {
        return compare(c, rating_at(row, row::third_rating), limit);
    }
    // The extra text (sp+0xa0, empty until now) by the third rating, and the first's where it is low.
    void rest_text() {
        if (compare(c, std::int32_t(std::int8_t(c.u8(rest)))) != 0) return;
        const auto row = row_of(c, object, club);
        const auto third = rating_at(row, row::third_rating);
        compare(c, third, -0x2ee);
        if (third < -0x2ee) {
            const auto first = rating_at(row, row::first_rating);
            compare(c, first, 0xea6);
            if (first < 0xea6) say_plain(rest, 0x10000 - 0x1d6, 0x1019c4e0);
            else {
                compare(c, first, 0x157c);
                if (first < 0x157c) say_plain(rest, 0x10000 - 0x15a, 0x1019c4fc);
            }
            return;
        }
        compare(c, third, -0x1f4);
        if (third < -0x1f4) return say_plain(rest, 0x10000 - 0xe3, 0x1019c518);
        compare(c, third, 0x5dc);
        if (third > 0x5dc) say_plain(rest, 0x10000 - 0x8a, 0x1019c534);
    }
    // The output: the sentence, and the extra text after it when there is one.
    void join() {
        const auto has_rest = compare(c, std::int32_t(std::int8_t(c.u8(rest)))) != 0;
        if (has_rest) {
            c.set({{3, out}, {4, pool + 0x10000 - 0x47}, {5, sentence}, {6, rest}});
            c.call(callee::text_copy, 0x1019c558);
        } else {
            c.set({{3, out}, {4, pool + 0x10000 - 0x40}, {5, sentence}});
            c.call(callee::text_copy, 0x1019c570);
        }
    }

    Cpu& c;
    Frame& frame;
    std::uint32_t object, club, out, pool, sentence, rest;
};
}  // namespace

// 0x101980a0. Frame 0xa90 (r25-r31 saved with stmw, LR at +8).
void club_people_text(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t days, std::uint32_t out) {
    Frame frame(c, 0x101980a0, 0xa90, {.first_gpr = 25});
    const auto pool = c.toc(slots::pool);
    guest::keep(c, {{25, days}, {28, object}, {29, out}, {30, pool}, {31, club}});
    c.put8(out, 0);
    c.put8(frame.at(frame_slot::rest), 0);
    guest::enter(c, 0x101980d4, {object, club});
    c.set(26, club_second_band(c, object, club));
    guest::enter(c, 0x101980e4, {object, club});
    c.set(27, club_first_band(c, object, club));
    guest::enter(c, 0x101980fc, {object, club, frame.at(frame_slot::kind), frame.at(frame_slot::count)});
    club_upcoming_entry(c, object, club, frame.at(frame_slot::kind), frame.at(frame_slot::count));
    Chooser(c, frame, object, club, out).choose(days);
    lmw_leaves(c, frame.caller());
}
}
