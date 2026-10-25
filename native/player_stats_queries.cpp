#include "player_stats_detail.hpp"
#include <bit>

namespace cm::aot::player_stats {
using namespace detail;
namespace {
// The int-to-double idiom's high word, which the original also leaves in a register.
constexpr std::uint32_t magic = 0x43300000;
std::uint32_t flipped(std::int32_t value) { return signed_word(value); }
double temporary(Cpu& c, std::uint32_t at, std::uint32_t low) { return stacked_integer(c, at, low); }
// An int16 or int32 as a single (fsubs of the signed temporary at sp+at).
double signed_single(Cpu& c, Fpu& fpu, std::uint32_t at, std::int32_t value) {
    return fpu.fsubs(temporary(c, at, flipped(value)), signed_bias);
}
// An unsigned byte as a single (fsubs of the unsigned temporary at sp+at).
double unsigned_single(Cpu& c, Fpu& fpu, std::uint32_t at, std::uint32_t value) {
    return fpu.fsubs(temporary(c, at, value), unsigned_bias);
}

// The pool's constants the queries use (read-only; the original loads them
// with lfd from slot::constants and lfs from slot::constants2).
namespace k {
constexpr double two = 2.0, three = 3.0, four = 4.0, five = 5.0, six = 6.0, ten = 10.0, one_and_half = 1.5,
                 two_and_half = 2.5;
// constants2 (single precision): +0 none, +4, +8, +0xc, +0x10, +0x14, +0x18.
constexpr double none = 0.0f, unset_rating = 6.5f, twenty = 20.0f, eighth = 0.125f, five_s = 5.0f, hundred = 100.0f,
                 no_form = 1.5f;
}

// The assertion report of these routines: the success flag, message buffer,
// pool (and constants2) addresses the original holds in its non-volatile
// registers while it reports.
struct Reporting {
    unsigned flag, buffer, pool, constants2 = 0;  // register numbers (constants2: 0 if not held)
};
void report_failure(Cpu& c, const Frame& frame, Reporting held, std::uint32_t at, std::int32_t name,
                    std::int32_t extension, std::uint32_t line, bool flag_in_r4) {
    const auto pool = c.toc(slot::pool);
    c.set(held.flag, c.toc(slot::success));
    c.set(held.buffer, c.toc(slot::message));
    c.set(held.pool, pool);
    if (held.constants2) c.set(held.constants2, c.toc(slot::constants2));
    report(c, pool, at, frame.at(name), frame.at(extension), line, flag_in_r4);
}

PlayerEntry entry_of(Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t& players) {
    players = Stats{c, stats}.players();
    const std::int32_t id = Field<std::int32_t>{c, person};
    return {c, players + std::uint32_t(id) * PlayerEntry::size};
}
}

std::uint32_t season_record(Cpu& c, std::uint32_t stats, std::uint32_t person, std::int8_t category) {
    Frame frame(c, 0x103054e0, 0xa0, {.first_gpr = 29});
    const Reporting held{.flag = 29, .buffer = 30, .pool = 31};
    if (!nonzero_logical(c, person)) {
        report_failure(c, frame, held, 0x10305520, 0x68, 0x40, 0xa65, false);
        return result(c, 0);
    }
    std::uint32_t players;
    const auto entry = entry_of(c, stats, person, players);
    leave(c, {{4, entry.address}, {5, players}});
    if (category < 1 || category > 6) {
        // The original's compare chain ends on 6 above the range, on 1 below.
        compare(c, category, category > 6 ? 6 : 1);
        report_failure(c, frame, held, 0x10305698, 0x48, 0x38, 0xaa7, false);
        return result(c, 0);
    }
    const std::int32_t season = entry.season(category);
    if (compare(c, season, -1) <= 0) return result(c, 0);
    return result(c, Stats{c, stats}.seasons() + std::uint32_t(season) * SeasonRecord::size);
}

std::uint32_t packed_ratings_average(Cpu& c, std::uint32_t stats, std::uint32_t person) {
    Frame frame(c, 0x10303090, 0x70, {.first_gpr = 30});
    if (!nonzero_logical(c, person)) {
        report_failure(c, frame, {.flag = 4, .buffer = 30, .pool = 31}, 0x103030c8, 0x40, 0x38, 0x475, true);
        return result(c, 0);
    }
    std::uint32_t players;
    const auto entry = entry_of(c, stats, person, players);
    const std::uint32_t first = entry.ratings(0), second = entry.ratings(1), third = entry.ratings(2);
    // An unset rating (nibble 0) counts as 27, a set one as 4x its nibble.
    const auto term = [](std::uint32_t nibble) { return std::int32_t(nibble ? nibble * 4 : 27); };
    const std::int32_t two = term(first >> 4) + term(first & 0xf);
    const std::int32_t three = two + term(second >> 4);
    const std::int32_t four = three + term(second & 0xf);
    const std::uint32_t fifth = third >> 4;
    const std::int32_t sum = four + term(fifth);
    nonzero(c, std::int32_t(fifth));
    leave(c, {{4, std::uint32_t(four)}, {5, std::uint32_t(three)}, {6, entry.address}});
    // Quirk: five ratings summed, divided by 4 (srawi/addze: toward zero).
    return result(c, std::uint32_t(std::int32_t(std::int8_t(sum / 4))));
}

double fifth_rating(Cpu& c, std::uint32_t stats, std::uint32_t person) {
    Frame frame(c, 0x103031f0, 0x80, {.first_gpr = 29});
    if (!nonzero_logical(c, person)) {
        report_failure(c, frame, {.flag = 3, .buffer = 30, .pool = 31, .constants2 = 29}, 0x10303230, 0x40, 0x38,
                       0x4bd, false);
        c.set(3, c.toc(slot::success));
        return k::none;
    }
    const std::int32_t id = Field<std::int32_t>{c, person};
    const auto offset = std::uint32_t(id) * PlayerEntry::size;
    const PlayerEntry entry{c, Stats{c, stats}.players() + offset};
    const std::uint32_t nibble = entry.ratings(2) >> 4;
    if (!nonzero(c, std::int32_t(nibble))) {
        leave(c, {{3, offset}, {4, 0}});
        return k::unset_rating;
    }
    leave(c, {{3, c.toc(slot::constants)}, {4, nibble}});
    Fpu fpu{c};
    return unsigned_single(c, fpu, frame.at(0x60), nibble);
}

double entry_figure(Cpu& c, std::uint32_t stats, std::uint32_t person, std::int8_t kind) {
    Frame frame(c, 0x10306a80, 0xa0, {.first_gpr = 28});
    const Reporting held{.flag = 29, .buffer = 30, .pool = 31, .constants2 = 28};
    if (!nonzero_logical(c, person)) {
        report_failure(c, frame, held, 0x10306ac8, 0x68, 0x40, 0xcef, false);
        return k::none;
    }
    std::uint32_t players;
    const auto entry = entry_of(c, stats, person, players);
    c.set(5, players);
    if (kind < 1 || kind > 2) {
        // The original's compare chain ends on 2 above the range, on 1 below.
        compare(c, kind, kind > 2 ? 2 : 1);
        c.set(3, entry.address);
        report_failure(c, frame, held, 0x10306b9c, 0x48, 0x38, 0xd07, false);
        return k::none;
    }
    compare(c, kind, kind);  // the last compare matched: equal
    const std::int32_t figure = entry.figure(kind);
    leave(c, {{3, flipped(figure)}, {4, c.toc(slot::constants)}});
    Fpu fpu{c};
    return signed_single(c, fpu, frame.at(0x88), figure);
}

double season_value(Cpu& c, std::uint32_t record_address, std::uint32_t kind_register, double prior) {
    Frame frame(c, 0x10303980, 0xc0, {.first_gpr = 28});
    const Reporting held{.flag = 28, .buffer = 29, .pool = 30, .constants2 = 31};
    if (!nonzero_logical(c, record_address)) {
        report_failure(c, frame, held, 0x103039c8, 0x68, 0x40, 0x6d9, false);
        return k::none;
    }
    const std::int32_t kind = std::int8_t(kind_register);
    compare_logical(c, std::uint32_t(kind), 22);
    const SeasonRecord record{c, record_address};
    Fpu fpu{c};
    // The jump table's case label stays in r3 where a case does not replace it.
    const auto case_label = [&] { c.set(3, c.u32(c.toc(slot::value_cases) + std::uint32_t(kind) * 4)); };
    const auto constants = [&] { return c.toc(slot::constants); };
    // value / divisor (both as singles) when the divisor (unsigned byte) is
    // not 0, at the original's temporaries; the case's registers left.
    const auto per = [&](std::int16_t value, std::uint32_t divisor, std::int32_t value_at, std::int32_t divisor_at) {
        if (!nonzero_logical(c, divisor)) {
            case_label();
            c.set(5, 0);
            return k::none;
        }
        leave(c, {{3, flipped(value)}, {4, constants()}, {5, divisor}});
        const double top = signed_single(c, fpu, frame.at(value_at), value);
        const double bottom = unsigned_single(c, fpu, frame.at(divisor_at), divisor);
        return fpu.fdivs(top, bottom);
    };
    // part / whole * 100 (int16s) when whole > 0.
    const auto percent = [&](std::int16_t whole, std::int16_t part) {
        c.set(5, extended(whole));
        if (compare(c, whole) <= 0) {
            case_label();
            return k::none;
        }
        leave(c, {{3, magic}, {4, flipped(part)}, {5, constants()}});
        const double top = signed_single(c, fpu, frame.at(0x98), part);
        const double bottom = signed_single(c, fpu, frame.at(0xa0), whole);
        return fpu.fmuls(k::hundred, fpu.fdivs(top, bottom));
    };
    switch (kind) {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12: {
        const std::uint32_t tally = record.tally(kind);
        leave(c, {{3, constants()}, {4, tally}});
        return unsigned_single(c, fpu, frame.at(0xa0), tally);
    }
    case 13: return percent(record.passes(), record.passes_completed());
    case 14: return per(record.tackles(), record.appearances(), 0x98, 0xa0);
    case 15: return per(record.dribbles(), record.appearances(), 0x98, 0xa0);
    case 16: return percent(record.shots(), record.shots_on_target());
    case 17: return per(record.rating_total(), record.appearances(), 0xa0, 0x98);
    case 18: {
        // The average rating; under 20 appearances the missing ones count as `prior`.
        const std::uint32_t games = record.appearances();
        const std::int16_t total = record.rating_total();
        if (compare_logical(c, games, 20) >= 0) {
            leave(c, {{3, flipped(total)}, {4, constants()}, {5, games}});
            const double top = signed_single(c, fpu, frame.at(0x90), total);
            return fpu.fdivs(top, unsigned_single(c, fpu, frame.at(0x88), games));
        }
        const auto missing = std::int32_t(20 - games);
        leave(c, {{3, magic}, {4, flipped(missing)}, {5, constants()}});
        const double weight = signed_single(c, fpu, frame.at(0x98), missing);
        const double sum = signed_single(c, fpu, frame.at(0xa0), total);
        return fpu.fdivs(fpu.multiply_add(0x10303ddc, prior, weight, sum), k::twenty);
    }
    case 20: {
        // Appearances + 2 x won - lost per appearance; under 20 appearances
        // the missing ones count 1.5 each.
        const std::uint32_t games = record.appearances();
        c.set(8, games);
        if (!nonzero_logical(c, games)) {
            case_label();
            return k::no_form;
        }
        const std::uint32_t won = record.games_won(), lost = record.games_lost();
        const auto points = std::int32_t(games + won * 2 - lost);
        if (compare_logical(c, games, 20) >= 0) {
            leave(c, {{3, flipped(points)}, {4, lost}, {5, constants()}});
            const double top = signed_single(c, fpu, frame.at(0x98), points);
            return fpu.fdivs(top, unsigned_single(c, fpu, frame.at(0xa0), games));
        }
        const auto missing = std::int32_t(20 - games);
        leave(c, {{3, lost}, {4, flipped(missing)}, {5, magic}, {6, std::uint32_t(missing)}, {7, constants()}});
        const double weight = signed_single(c, fpu, frame.at(0x88), missing);
        const double sum = signed_single(c, fpu, frame.at(0x90), points);
        return fpu.fdivs(fpu.frsp(fpu.multiply_add(0x10303e98, k::one_and_half, weight, sum)), k::twenty);
    }
    case 21: return per(record.shots(), record.appearances(), 0x88, 0x90);
    case 22: {
        const std::int32_t figure = record.figure();
        leave(c, {{3, flipped(figure)}, {4, constants()}});
        return signed_single(c, fpu, frame.at(0x88), figure);
    }
    default:  // 0, 19 and out of range
        report_failure(c, frame, held, 0x10303f88, 0x48, 0x38, 0x7ab, false);
        return k::none;
    }
}

double player_value(Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t category_register,
                    std::uint32_t kind_register, double prior) {
    Frame frame(c, 0x10303560, 0xd0, {.first_gpr = 22, .gpr_offset = -0x48, .first_fpr = 29});
    const auto constants2 = c.toc(slot::constants2), constants = c.toc(slot::constants);
    c.set_f(31, prior);
    keep(c, {{22, kind_register}, {24, c.toc(slot::message)}, {25, c.toc(slot::pool)}, {26, stats}, {27, person},
             {28, category_register}, {30, constants2}, {31, constants}});
    Kept<23> games{c, 0};
    Kept<29> rating{c, 0};
    if (!nonzero(c, std::int32_t(person))) {
        report(c, c.toc(slot::pool), 0x103035c0, frame.at(0x40), frame.at(0x38), 0x659, false);
        c.set(3, c.toc(slot::success));
        return k::none;
    }
    const auto count = c.toc(slot::stats_players);
    const std::int32_t id = Field<std::int32_t>{c, person};
    leave(c, {{6, count}, {7, std::uint32_t(id)}});
    if (compare(c, id, Field<std::int32_t>{c, count}) >= 0) return k::none;
    const std::int32_t category = std::int8_t(category_register), kind = std::int8_t(kind_register);
    if (kind != 0x13 && category <= 6) {
        // One season record's value (the compares here are overwritten by the callee's).
        enter(c, 0x10303640, {stats, person, category_register});
        const auto record = season_record(c, stats, person, std::int8_t(category));
        c.set(4, record);
        if (nonzero(c, std::int32_t(record))) {
            enter(c, 0x10303658, {stats, record, kind_register});
            return season_value(c, record, kind_register, prior);
        }
        // No record: the averages fall back on `prior`, the rest on 0.
        if (compare(c, kind, 0x12) == 0 || compare(c, kind, 0x14) == 0) return prior;
        return k::none;
    }
    // The combined ratings. Categories 2 and 3 together: average rating,
    // with up to 20 missing appearances counted as 5.
    const auto lookup = [&](std::uint32_t back, std::int8_t which) {
        enter(c, back, {stats, person, std::uint32_t(which)});
        return season_record(c, stats, person, which);
    };
    if (const auto record = lookup(0x10303690, 2)) {
        const SeasonRecord season{c, record};
        games = season.appearances();
        rating = extended(season.rating_total());
    }
    if (const auto record = lookup(0x103036b0, 3)) {
        const SeasonRecord season{c, record};
        const std::uint32_t more = season.appearances();
        c.set(4, more);
        games = games + more;
        rating = rating + extended(season.rating_total());
    }
    // Quirk: the sums wrap to a byte (appearances) and an int16 (ratings).
    const std::uint32_t played = games & 0xff;
    const std::int32_t total = std::int16_t(rating);
    Fpu fpu{c};
    double combined;
    if (played < 20) {
        const double missing = signed_single(c, fpu, frame.at(0x78), std::int32_t(20 - played));
        const double sum = fpu.fsub(temporary(c, frame.at(0x70), flipped(total)), signed_bias);
        combined = fpu.fdivs(fpu.frsp(fpu.multiply_add(0x10303714, k::five, missing, sum)), k::twenty);
    } else {
        const double sum = signed_single(c, fpu, frame.at(0x68), total);
        combined = fpu.fdivs(sum, unsigned_single(c, fpu, frame.at(0x60), played));
    }
    c.set_f(31, combined);
    // Category 4: average rating, up to 8 missing appearances counted as 3.
    // Quirk: without a record f30 keeps the caller's value.
    double fourth = c.f(30);
    if (const auto record = lookup(0x1030376c, 4)) {
        const SeasonRecord season{c, record};
        const std::uint32_t played4 = season.appearances();
        const std::int32_t total4 = season.rating_total();
        if (played4 < 8) {
            const auto missing = std::int32_t(8 - played4);
            leave(c, {{3, magic}, {4, flipped(missing)}});
            const double weight = signed_single(c, fpu, frame.at(0x60), missing);
            const double sum = fpu.fsub(temporary(c, frame.at(0x68), flipped(total4)), signed_bias);
            fourth = fpu.fmuls(fpu.frsp(fpu.multiply_add(0x103037c0, k::three, weight, sum)), k::eighth);
        } else {
            leave(c, {{3, flipped(total4)}, {4, played4}});
            const double sum = signed_single(c, fpu, frame.at(0x60), total4);
            fourth = fpu.fdivs(sum, unsigned_single(c, fpu, frame.at(0x68), played4));
        }
        c.set_f(30, fourth);
    }
    // Category 5: average rating, up to 5 missing appearances counted as 1.
    // Quirk: without a record f29 keeps the caller's value.
    double fifth = c.f(29);
    if (const auto record = lookup(0x10303818, 5)) {
        const SeasonRecord season{c, record};
        const std::uint32_t played5 = season.appearances();
        const std::int32_t total5 = season.rating_total();
        if (played5 < 5) {
            const auto missing = std::int32_t(5 - played5);
            leave(c, {{3, magic}, {4, flipped(total5)}, {5, played5}});
            const double sum = fpu.fsub(temporary(c, frame.at(0x60), flipped(total5)), signed_bias);
            const double weight = signed_single(c, fpu, frame.at(0x68), missing);
            fifth = fpu.fdivs(fpu.frsp(fpu.fadd(sum, weight)), k::five_s);
        } else {
            leave(c, {{3, flipped(total5)}, {5, played5}});
            const double sum = signed_single(c, fpu, frame.at(0x60), total5);
            fifth = fpu.fdivs(sum, unsigned_single(c, fpu, frame.at(0x68), played5));
        }
        c.set_f(29, fifth);
    }
    // The blend: (a x f30 + b x f31 + c x f29) / 10 by category.
    compare(c, category, category == 0x65 ? 0x65 : category > 0x65 ? 0x67 : 0x64);
    switch (category) {
    case 0x64:
        return fpu.frsp(fpu.fdiv(
            fpu.frsp(fpu.multiply_add(0x103038f0, k::one_and_half, fifth,
                                              fpu.multiply_add(0x103038ec, k::six, combined, fpu.fmul(k::two_and_half, fourth)))),
            k::ten));
    case 0x65:
        return fpu.frsp(fpu.fdiv(
            fpu.frsp(fpu.multiply_add(0x1030391c, k::three, fifth,
                                              fpu.multiply_add(0x10303918, k::two, combined, fpu.fmul(k::five, fourth)))), k::ten));
    case 0x66:
        return fpu.frsp(fpu.fdiv(fpu.frsp(fpu.multiply_add(0x10303940, k::five, fifth, fpu.multiply_add(0x10303938, k::four, fourth, combined))), k::ten));
    default:
        return k::none;
    }
}
}
