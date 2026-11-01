// MATCH_ENG.CPP, rating predicates: stage lookup, fixture grade and the side's bench average.
#include "match_eng_rating_pred.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
std::uint32_t draw(Cpu& c, std::uint32_t bound, std::uint32_t back) {
    c.set(3, bound);
    call(c, 0x101d3c90, back);
    return c.r(3);
}
}  // namespace

// ---------------------------------------------------------------------------
// 0x101fd6b0
std::uint32_t stage_number(Cpu& c, std::uint32_t pair, std::uint32_t record, std::uint32_t mode) {
    Frame frame(c, 0x101fd6b0, 0xb0, {.first_gpr = 21});
    const auto pool = c.toc(-0x52cc);
    keep(c, {{31, pair}, {26, record}, {21, mode}, {25, c.toc(-0x7ba8)}, {28, pool}});
    if (compare_logical(c, pair) == 0) {
        report_assertion(c, {.at = 0x101fd6f0, .file = pool + 0x45, .format = pool + 0x53, .title = pool + 0x60,
                             .name = frame.at(0x61), .extension = frame.at(0x3d), .line = 0x49c5});
        return result(c, 0);
    }
    if (compare_logical(c, record) == 0) {
        report_assertion(c, {.at = 0x101fd754, .file = pool + 0x45, .format = pool + 0x53, .title = pool + 0x60,
                             .name = frame.at(0x42), .extension = frame.at(0x38), .line = 0x49cb});
        return result(c, 0);
    }
    const bool probing = compare(c, s8(mode)) == 0;
    std::uint32_t first_object = 0, second_object = 0, limit = 0;  // r29, r30 and r24
    if (probing) {
        // The record's own objects, or the shared table's entry when they are empty.
        const auto fallback = [&](std::uint32_t slot, std::uint32_t count_slot) {
            c.set(4, c.u32(c.toc(slot)));
            return c.r(4) + c.u32(c.toc(count_slot)) * 0x3c;
        };
        std::uint32_t home = c.u32(record + 0x44);
        keep(c, {{23, home}});
        if (compare_logical(c, home) == 0) { home = fallback(-0x7544, -0x7670); keep(c, {{23, home}}); }
        limit = c.u32(record + 0x48);
        keep(c, {{24, limit}});
        if (compare_logical(c, limit) == 0) { limit = fallback(-0x7544, -0x7678); keep(c, {{24, limit}}); }
    }
    Kept<27> stage(c, 0);
    Kept<22> found(c, 0);
    for (;;) {
        const auto order = compare(c, s8(stage), 1);
        std::uint32_t at;
        bool must_exist = true;
        if (order == 0) at = 0x4c;
        else if (order > 0) {
            if (compare(c, s8(stage), 3) >= 0) return result(c, 1);
            at = 0x54;
        } else {
            if (compare(c, s8(stage), 0) < 0) return result(c, 1);
            at = 0x44;
            must_exist = false;
        }
        first_object = c.u32(pair + at);
        second_object = c.u32(pair + at + 4);
        keep(c, {{29, first_object}, {30, second_object}});
        if (must_exist && (compare_logical(c, first_object) == 0 || compare_logical(c, second_object) == 0)) return result(c, 2);
        stage = stage + 1;
        if (compare(c, s8(mode)) == 0 && compare_logical(c, second_object) != 0 && compare_logical(c, first_object) != 0) {
            c.set({{3, second_object}, {4, limit}});
            c.call(0x100cd980, 0x101fd8a0);
            found = s8(c.r(3)) == 0;
        }
        if (compare(c, s8(mode)) != 0) break;
        if (compare(c, s8(found)) != 0) break;
    }
    if (compare_logical(c, first_object) == 0) return result(c, 1);
    if (compare_logical(c, second_object) == 0) return result(c, 1);
    return result(c, stage);
}

// ---------------------------------------------------------------------------
// 0x101fd900
void post_grade(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101fd900, 0x50, {.first_gpr = 31});
    keep(c, {{31, match}});
    const auto list = match + 0xe030;  // the fixture's list object (+0x10000 - 0x1fd0)
    c.set(3, list);
    const auto head = std::int32_t(first_of_list(c, list));
    if (compare(c, head, -1) == 0) return;
    if (compare_logical(c, c.u32(match + fixture_at)) == 0) return;
    c.set(3, c.u32(match + fixture_at));
    if (compare_logical(c, c.u32(c.r(3) + 0x14)) == 0) return;
    const auto global = c.toc(-0x7b54);
    c.set(3, global);
    if (compare_logical(c, c.u32(global)) != 0) {
        c.set({{3, c.u32(global)}, {4, c.u32(c.u32(match + fixture_at) + 0x14)}});
        c.call(0x10494d20, 0x101fd96c);
        if (compare(c, s8(c.r(3))) != 0) return;
    }
    // The two locals are byte sums of the fixture's counters; the grade starts at its year / 1000.
    const Field<std::uint8_t> sum_a(c, frame.at(0x3c)), sum_b(c, frame.at(0x3d)), grade(c, frame.at(0x3e));
    sum_a = std::uint8_t(c.u8(match + 0xe04b) + c.u8(match + 0xe081));
    sum_b = std::uint8_t(c.u8(match + 0xe04c) + c.u8(match + 0xe082));
    c.set(3, list);
    grade = std::uint8_t(thousands(c, list));
    if (compare(c, s8(sum_a), 2) > 0) grade = std::uint8_t(grade - 2);
    else if (compare(c, s8(sum_a), 1) > 0) grade = std::uint8_t(grade - 1);
    else if (compare(c, s8(sum_a), 0) == 0) grade = std::uint8_t(grade + 1);
    if (compare(c, s8(sum_b), 5) > 0) grade = std::uint8_t(grade - 1);
    else if (compare(c, s8(sum_b), 3) < 0) grade = std::uint8_t(grade + 1);
    bool raised = false;
    if (compare(c, s8(grade), 4) < 0) {
        if (compare(c, std::int32_t(draw(c, 500, 0x101fda74))) != 0) {
            grade = 4;
            raised = true;
        }
    }
    if (!raised && compare(c, s8(grade), 8) > 0) {
        if (compare(c, std::int32_t(draw(c, 25, 0x101fdaa0))) != 0) grade = 8;
    }
    if (compare(c, s8(grade), 1) < 0) grade = 1;
    else if (compare(c, s8(grade), 10) > 0) grade = 10;
    c.set(3, list);
    c.put32(frame.at(0x38), first_of_list(c, list));
    c.set({{3, c.u32(c.toc(-0x7428))}, {4, frame.at(0x38)}});
    c.call(0x102d4590, 0x101fdb04);
}

// ---------------------------------------------------------------------------
// 0x101fdb70
std::uint32_t bench_average(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101fdb70, 0x50, {.first_gpr = 27});
    c.put32(frame.at(0x68), match);  // the arguments spilled into the caller's parameter area
    c.put8(frame.at(0x6f), side);
    const auto row = u(s8(side)) * side_size;
    leave(c, {{4, match}});
    auto player = match + row + players_at;
    std::int32_t total = 0, count = 0;
    for (int n = 0; n < 20; ++n, player += player_size) {
        if (compare(c, s8(position(c, player))) < 0) {
            total += c.i16(player + 0x36);
            ++count;
        }
    }
    if (compare(c, count) == 0) return result(c, 0);
    return result(c, u(std::int16_t(total / count)));
}
}
