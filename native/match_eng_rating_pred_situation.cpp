// MATCH_ENG.CPP, rating predicates: the situation predicate (0x101fdc10), thirteen cases over a
// side's competition rules, the minute and the random draws of the match's table.
#include "match_eng_rating_pred.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// The match clock's minute as the compiler's divide by 11 computes it: the high word of the multiply,
// halved, plus its sign bit. The registers it passes through are part of what some cases leave.
struct Minute {
    std::int32_t high, half, value;
};
Minute minute_of(std::int16_t clock) {
    const auto high = std::int32_t((std::int64_t(0x2e8ba2e9) * clock) >> 32);
    const auto half = high >> 1;
    return {high, half, std::int32_t(std::uint32_t(half) + (std::uint32_t(half) >> 31))};
}

// Which registers hold what at the `bl 0x10452ed0` (rules check) of each case.
enum class Held { index, fixture, fixture_and_index, table };
}  // namespace

std::uint32_t situation_met(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t kind, std::uint32_t out) {
    Frame frame(c, 0x101fdc10, 0x90, {.first_gpr = 20});
    const auto pool = c.toc(-0x7b94);   // the object the trait query asks
    const auto rules_table = c.toc(-0x7b5c);  // table of rule entries, by the fixture's first word
    keep(c, {{31, match}, {30, side}, {24, kind}, {26, out}, {27, pool}, {25, rules_table}});

    // Two five-byte records built from the pool's text, kept for the trait queries.
    c.set({{3, frame.at(0x48)}, {4, c.toc(-0x7b80)}, {5, 10}});
    c.call(0x10717b20, 0x101fdc4c);
    c.set({{3, frame.at(0x58)}, {4, frame.at(0x48)}});
    copy_cell_record(c, frame.at(0x58), frame.at(0x48));
    c.set({{3, frame.at(0x40)}, {4, c.toc(-0x7b80)}, {5, 0x1e}});
    c.call(0x10717b20, 0x101fdc70);
    c.set({{3, frame.at(0x50)}, {4, frame.at(0x40)}});
    copy_cell_record(c, frame.at(0x50), frame.at(0x40));

    Kept<29> rules(c, 0);
    Kept<23> in_range(c, 0);
    const auto kind16 = std::int16_t(kind);
    if (compare(c, kind16, 0x1f40) >= 0) {
        c.set(5, extended(kind16));
        if (compare(c, kind16, 0x21e4) <= 0) in_range = 1;
    }
    const bool usable = compare(c, in_range) != 0;
    Kept<22> usable_flag(c, usable);
    if (compare(c, usable_flag) == 0) return result(c, 0);

    // The kind's flag byte for this side must be clear, and the fixture needs an id.
    auto flag_at = match + u(s8(side)) * 1000;
    c.set(4, flag_at);
    flag_at += extended(kind16);
    c.set(4, flag_at);
    const auto flag = c.u8(flag_at - 0x1d08);
    c.set(4, u(s8(flag)));
    if (compare(c, s8(flag)) != 0) return result(c, 0);
    if (compare_logical(c, c.u32(c.u32(match + fixture_at) + 0x14)) == 0) return result(c, 0);

    c.set({{3, match}, {4, side}, {5, frame.at(0x3c)}, {6, frame.at(0x3b)}, {7, frame.at(0x3a)}, {8, frame.at(0x39)},
           {9, frame.at(0x38)}, {10, 0}});
    call(c, 0x101f21d0, 0x101fdd1c);
    Kept<28> aggregate(c, c.r(3));  // the tie's running result (a signed byte)

    const auto fixture = [&] { return c.u32(match + fixture_at); };
    const auto team_of_side = [&] {
        c.set({{3, match}, {4, side}});
        return fixture_team(c, match, side);
    };
    const auto rules_entry = [&] { return c.u32(c.u32(rules_table) + (c.u32(fixture()) << 2)); };

    // The fixture's rules record (0x10459b50), when its table entry and that entry's object exist.
    c.set(5, c.u32(rules_table));
    c.set(4, c.u32(fixture()) << 2);
    if (compare_logical(c, c.u32(c.r(5) + c.r(4))) != 0) {
        c.set(4, c.u32(rules_table));
        if (compare_logical(c, c.u32(rules_entry() + 8)) != 0) {
            const auto team = team_of_side();
            c.set({{4, team}, {5, c.u32(rules_table)}});
            c.set(5, rules_entry());
            c.set(3, c.u32(c.r(5) + 8));
            c.call(0x10459b50, 0x101fdd88);
            rules = c.r(3);
        }
    }

    const auto fail = [&] { return result(c, 0); };
    const auto pass = [&] { return result(c, 1); };
    const auto draw = [&](std::uint32_t bound, std::uint32_t back) {
        c.set(3, bound);
        call(c, 0x101d3c90, back);
        return std::int32_t(c.r(3));
    };
    const auto rating = [&](std::uint32_t back) {
        enter(c, back, {match, side});
        return std::int16_t(bench_average(c, match, side));
    };
    // 0x1069e700: whether the side's team has trait `code`, answered into the record at sp+local.
    const auto trait = [&](std::uint32_t code, std::uint32_t local, std::uint32_t back, bool into_r4 = false) {
        const auto team = team_of_side();
        c.set({{5, team}, {3, c.u32(pool)}, {4, 0}, {6, u(-1)}, {7, code}, {8, frame.at(local)}});
        c.call(0x1069e700, back);
        if (into_r4) c.set(4, u(s8(c.r(3))));
        return compare(c, s8(into_r4 ? c.r(4) : c.r(3))) != 0;
    };
    // 0x10452ed0: whether the fixture's rule entry applies to the team.
    const auto applies = [&](std::uint32_t team, std::uint32_t back, Held held) {
        const auto base = c.u32(rules_table);
        const auto index = c.u32(fixture()) << 2;
        c.set({{3, c.u32(base + index)}, {4, team}});
        switch (held) {
        case Held::index: c.set(5, index); break;
        case Held::fixture: c.set(5, fixture()); break;
        case Held::fixture_and_index: c.set({{5, fixture()}, {6, index}}); break;
        case Held::table: c.set(5, base); break;
        }
        c.call(0x10452ed0, back);
        return compare(c, s8(c.r(3))) != 0;
    };
    // The side's team plays for the fixture's team: its +0x30 field is the fixture's +0x14 (cases that
    // keep it in r4).
    const auto own_team = [&] {
        const auto team = team_of_side();
        c.set(4, c.u32(team + 0x30));
        return compare_logical(c, c.r(4), c.u32(fixture() + 0x14)) == 0;
    };
    const auto clock = [&] { return c.i16(match + 0x7948); };
    // The out value, when asked for.
    const auto store_out = [&](std::uint32_t at) {
        if (compare_logical(c, out) != 0) c.put32(out, c.u32(at));
    };

    const auto n = extended(kind16) - 0x21a3;
    if (compare_logical(c, n, 0xc) > 0) return pass();
    c.set(4, c.u32(c.toc(-0x52ec) + n * 4));  // the jump table's entry
    switch (n) {
    case 0: {
        if (!own_team()) return fail();
        const auto drawn = draw(3, 0x101fddd8);
        if (compare(c, s8(aggregate), drawn) <= 0) return fail();
        if (!applies(team_of_side(), 0x101fde0c, Held::index)) return fail();
        if (trait(0x61, 0x58, 0x101fde40, true) && compare(c, rating(0x101fde58), 0x1f40) > 0) return pass();
        if (trait(0x62, 0x58, 0x101fde8c) && compare(c, rating(0x101fdea4), 0x1e46) > 0) return pass();
        if (!trait(0x63, 0x58, 0x101fded8)) return fail();
        return compare(c, rating(0x101fdef0), 0x1d4c) > 0 ? pass() : fail();
    }
    case 1: {
        const auto team = team_of_side();
        if (compare(c, c.i32(fixture()), c.i32(team + 0x14)) != 0) return fail();
        Kept<21> bound(c, compare(c, s8(c.u8(frame.at(0x38))) + 3) != 0 ? 3 : 0);
        const auto drawn = draw(bound, 0x101fdf4c);
        c.set(4, u(s8(aggregate)));
        if (compare(c, s8(aggregate), drawn) <= 0) return fail();
        if (!applies(team_of_side(), 0x101fdf80, Held::fixture_and_index)) return fail();
        if (!trait(0x5c, 0x50, 0x101fdfb4)) return fail();
        return compare(c, rating(0x101fdfd0), 0x1d4c) > 0 ? pass() : fail();
    }
    case 2: {
        if (!own_team()) return fail();
        const auto drawn = draw(3, 0x101fe00c);
        if (compare(c, s8(aggregate), drawn) <= 0) return fail();
        if (!applies(team_of_side(), 0x101fe040, Held::fixture)) return fail();
        if (!trait(0x5f, 0x58, 0x101fe074)) return fail();
        const auto average = rating(0x101fe08c);
        c.set(4, extended(average));
        return compare(c, average, 0x1d4c) > 0 ? pass() : fail();
    }
    case 3: {
        if (!own_team()) return fail();
        const auto drawn = draw(u(-3), 0x101fe0c8);
        c.set(4, u(s8(aggregate)));
        if (compare(c, s8(aggregate), drawn) >= 0) return fail();
        const auto minute = minute_of(clock());
        c.set(5, u(minute.high));
        if (compare(c, minute.value, 0x1e) <= 0) return fail();
        if (!applies(team_of_side(), 0x101fe120, Held::index)) return fail();
        if (!trait(0x3f, 0x58, 0x101fe154)) return fail();
        return compare(c, rating(0x101fe16c), 0x1a2c) < 0 ? pass() : fail();
    }
    case 4: {
        if (!own_team()) return fail();
        const auto drawn = draw(u(-3), 0x101fe1a8);
        if (compare(c, s8(aggregate), drawn) >= 0) return fail();
        if (compare(c, minute_of(clock()).value, 0x1e) <= 0) return fail();
        if (!applies(team_of_side(), 0x101fe200, Held::table)) return fail();
        if (!trait(0x61, 0x58, 0x101fe234) && !trait(0x62, 0x58, 0x101fe268) && !trait(0x41, 0x58, 0x101fe29c) &&
            !trait(0x63, 0x58, 0x101fe2d0))
            return fail();
        return compare(c, rating(0x101fe2e8), 0x1a2c) < 0 ? pass() : fail();
    }
    case 5: {
        const auto team = team_of_side();
        c.set(4, c.u32(team + 0x14));
        if (compare(c, c.i32(fixture()), std::int32_t(c.r(4))) != 0) return fail();
        // -3 unless the tie's knockout byte is already -3.
        keep(c, {{20, compare(c, -3 - s8(c.u8(frame.at(0x38)))) != 0 ? u(-3) : 0}});
        const auto drawn = draw(c.r(20), 0x101fe344);
        if (compare(c, s8(aggregate), drawn) >= 0) return fail();
        if (!applies(team_of_side(), 0x101fe378, Held::table)) return fail();
        if (!trait(0x5c, 0x50, 0x101fe3ac)) return fail();
        return compare(c, rating(0x101fe3c4), 0x1a2c) < 0 ? pass() : fail();
    }
    case 6: {
        if (!own_team()) return fail();
        const auto drawn = draw(u(-3), 0x101fe400);
        if (compare(c, s8(aggregate), drawn) >= 0) return fail();
        if (compare(c, minute_of(clock()).value, 0x1e) <= 0) return fail();
        if (!applies(team_of_side(), 0x101fe458, Held::table)) return fail();
        if (!trait(0x5f, 0x58, 0x101fe48c) && !trait(0x40, 0x58, 0x101fe4c0)) return fail();
        return compare(c, rating(0x101fe4d8), 0x1a2c) < 0 ? pass() : fail();
    }
    case 7: {
        if (!own_team()) return fail();
        const auto drawn = draw(3, 0x101fe514);
        if (compare(c, s8(aggregate), drawn) <= 0) return fail();
        if (compare(c, minute_of(clock()).value, 0x1e) <= 0) return fail();
        if (!applies(team_of_side(), 0x101fe56c, Held::table)) return fail();
        if (!trait(0x3f, 0x58, 0x101fe5a0) && !trait(0x60, 0x58, 0x101fe5d4)) return fail();
        const auto average = rating(0x101fe5ec);
        c.set(4, extended(average));
        return compare(c, average, 0x1b58) > 0 ? pass() : fail();
    }
    case 8: {
        if (!own_team()) return fail();
        const auto drawn = draw(2, 0x101fe628);
        c.set(5, u(drawn + 1));
        if (compare(c, s8(aggregate), drawn + 1) <= 0) return fail();
        const auto minute = minute_of(clock());
        c.set(4, u(minute.value));
        if (compare(c, minute.value, 0x1e) <= 0) return fail();
        if (compare_logical(c, rules) == 0) return fail();
        auto next = draw(3, 0x101fe66c);
        if (compare(c, c.i32(rules + 0x1c), next + 1) <= 0) return fail();
        next = draw(5, 0x101fe684);
        c.set({{4, u(next)}, {5, u(next + 8)}});
        if (compare(c, c.i32(rules + 0x44), next + 8) <= 0) return fail();
        store_out(rules + 0x44);
        return pass();
    }
    case 9: {
        const auto team = team_of_side();
        c.set(5, c.u32(team + 0x30));
        if (compare_logical(c, c.r(5), c.u32(fixture() + 0x14)) != 0) return fail();
        const auto drawn = draw(2, 0x101fe6d8);
        c.set(4, u(-1 - drawn));
        if (compare(c, s8(aggregate), -1 - drawn) >= 0) return fail();
        const auto minute = minute_of(clock());
        c.set(4, u(minute.half));
        if (compare(c, minute.value, 0x1e) <= 0) return fail();
        if (compare_logical(c, rules) == 0) return fail();
        auto next = draw(3, 0x101fe71c);
        c.set(5, u(next + 1));
        if (compare(c, c.i32(rules + 0x30), next + 1) <= 0) return fail();
        next = draw(5, 0x101fe734);
        c.set(5, u(next));
        if (compare(c, c.i32(rules + 0x58), next + 8) <= 0) return fail();
        if (compare_logical(c, out) != 0) {
            c.set(5, c.u32(rules + 0x58));
            c.put32(out, c.r(5));
        }
        return pass();
    }
    case 10: {
        if (!own_team()) return fail();
        const auto drawn = draw(3, 0x101fe788);
        c.set(5, u(drawn));
        if (compare(c, s8(aggregate), drawn + 1) <= 0) return fail();
        const auto minute = minute_of(clock());
        c.set(5, std::uint32_t(minute.half) >> 31);
        if (compare(c, minute.value, 0x1e) <= 0) return fail();
        if (compare_logical(c, rules) == 0) return fail();
        auto next = draw(3, 0x101fe7d0);
        if (compare(c, c.i32(rules + 0x30), next + 1) <= 0) return fail();
        next = draw(5, 0x101fe7e8);
        if (compare(c, c.i32(rules + 0x58), next + 8) <= 0) return fail();
        store_out(rules + 0x58);
        return pass();
    }
    case 11: {
        const auto team = team_of_side();
        c.set(4, c.u32(fixture() + 0x14));
        if (compare_logical(c, c.u32(team + 0x30), c.r(4)) != 0) return fail();
        const auto drawn = draw(u(-3), 0x101fe838);
        c.set(5, u(s8(aggregate)));
        if (compare(c, s8(aggregate), drawn) >= 0) return fail();
        c.set({{5, extended(clock())}, {6, 0x2e8ba2e9}});
        if (compare(c, minute_of(clock()).value, 0x1e) <= 0) return fail();
        c.set(4, extended(clock()));
        // After the 75th minute the tie must not be lost by more than one.
        if (compare(c, minute_of(clock()).value, 0x4b) > 0 && compare(c, s8(aggregate), -1) < 0) return fail();
        if (compare_logical(c, rules) == 0) return fail();
        auto next = draw(3, 0x101fe8a8);
        c.set(4, c.u32(rules + 0x1c));
        if (compare(c, c.i32(rules + 0x1c), next + 1) <= 0) return fail();
        next = draw(5, 0x101fe8c0);
        c.set(5, u(next + 8));
        if (compare(c, c.i32(rules + 0x44), next + 8) <= 0) return fail();
        store_out(rules + 0x44);
        return pass();
    }
    default: {  // 12
        const auto team = team_of_side();
        c.set(5, fixture());
        if (compare_logical(c, c.u32(team + 0x30), c.u32(c.r(5) + 0x14)) != 0) return fail();
        bool skip_minute = false;
        if (compare(c, s8(aggregate), -1) == 0) {
            const auto minute = minute_of(clock());
            c.set(4, u(minute.value));
            skip_minute = compare(c, minute.value, 0x50) > 0;
        }
        if (!skip_minute) {
            if (compare(c, s8(aggregate), -2) != 0) return fail();
            const auto drawn = draw(10, 0x101fe94c);
            c.set({{4, u(drawn)}, {5, u(drawn + 0x46)}});
            const auto minute = minute_of(clock());
            c.set(4, u(minute.half));
            if (compare(c, minute.value, drawn + 0x46) <= 0) return fail();
        }
        if (compare_logical(c, rules) == 0) return fail();
        auto next = draw(3, 0x101fe988);
        c.set(5, u(next + 1));
        if (compare(c, c.i32(rules + 0x1c), next + 1) <= 0) return fail();
        next = draw(5, 0x101fe9a0);
        c.set(5, u(next));
        if (compare(c, c.i32(rules + 0x44), next + 8) <= 0) return fail();
        if (compare_logical(c, out) != 0) {
            c.set(5, c.u32(rules + 0x44));
            c.put32(out, c.r(5));
        }
        return pass();
    }
    }
}
}
