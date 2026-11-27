#include "croatia_first_rollover.hpp"
#include "cm0001_cup_order.hpp"
#include "comp_util.hpp"
#include "croatia_second_rules.hpp"
#include "league_base_detail.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace cm::aot::croatia_first {
namespace {
using guest::Cpu;
using guest::Frame;
using comp::invoke;
using comp::virtual_call;
using u32 = std::uint32_t;
namespace object = comp::competition;
namespace league = league_base::league;

constexpr int lower_slot = -0x7a0c;
constexpr std::array lower_divisions{lower_slot, -0x6a28, -0x6a30, -0x6a2c, -0x6a34};
constexpr unsigned lower_places = 5;

void fail(Cpu& c, const char* message) {
    std::fprintf(stderr, "CM 00/01 Croatian First Division: %s\n", message);
    c.put8(c.toc(comp::toc::success), 0);
}

// 3.89 links these competitions in both directions. A promoted or relegated
// club cannot be assigned to one of the two 01/02 regional competitions.
u32 second_division(Cpu& c, u32 first) {
    const auto id = c.i32(first + object::next);
    const auto objects = c.u32(c.toc(comp::toc::competitions));
    if (!objects || id < 0 || id >= c.i32(c.toc(comp::toc::competition_count)) ||
        id != c.i32(c.toc(croatia_second::national_division_slot))) return 0;
    const auto second = c.u32(objects + u32(id) * 4);
    if (!second) return 0;
    const auto vtable = c.u32(second);
    if (!vtable || vtable != c.toc(-0x5cdc) ||
        !croatia_second::uses_single_table(c.s, c.i16(first + object::year))) return 0;
    const auto first_record = c.u32(first + object::record), second_record = c.u32(second + object::record);
    if (!first_record || !second_record || c.i32(second_record) != id ||
        c.i32(second + object::parent) != c.i32(first_record) || c.i32(second + object::next) != -1 ||
        c.i16(second + object::club_count) != 18 || !c.u32(second + league::standings) ||
        c.u16(second + object::year) != c.u16(first + object::year)) return 0;
    return second;
}

// 3.89 CRO_FIRST chooses five clubs from the lower divisions and moves the
// clubs marked relegated by CRO_SECOND into the common lower division.
bool exchange_lower_clubs(Cpu& c, u32 first, u32 second, u32 candidates) {
    const auto count = c.i32(c.toc(comp::toc::club_count));
    const auto clubs = c.u32(c.toc(comp::toc::clubs));
    const auto croatia = c.u32(c.toc(-0x7590));
    std::array<u32, lower_divisions.size()> division_ids{};
    std::transform(lower_divisions.begin(), lower_divisions.end(), division_ids.begin(),
                   [&](int slot) { return c.u32(c.toc(slot)); });
    std::vector<cm0001_cups::Candidate> eligible;
    for (int index = 0; index < count; ++index) {
        const auto club = clubs + u32(index) * comp::club::size;
        const auto nation = c.u32(club + 0x2c), division = c.u32(club + 0x30);
        if (nation && division && c.u32(nation + 0x68) == croatia &&
            std::find(division_ids.begin(), division_ids.end(), c.u32(division)) != division_ids.end()) {
            eligible.push_back({club, c.i16(club + 0x1ec)});
        }
    }
    if (eligible.size() < lower_places) return false;
    c.link(0x106dcbb8);
    cm0001_cups::choose_front(eligible, lower_places, [&](unsigned bound) {
        return comp_util::pooled_draw(c, bound);
    });
    for (std::size_t i = 0; i < eligible.size(); ++i)
        c.put32(candidates + 4 * i, eligible[i].club);
    const auto second_record = c.u32(second + object::record);
    for (unsigned i = 0; i < lower_places; ++i)
        invoke(c, 0x1049f780, 0x106dcbe0, {first, c.u32(candidates + i * 4), second_record, 1});
    const auto lower = c.u32(c.toc(comp::toc::records)) + division_ids.front() * comp::record::size;
    for (int i = 0; i < c.i16(second + object::club_count); ++i) {
        const auto row = invoke(c, 0x1049ea00, 0x106dcc0c, {second, u32(i)});
        if (c.i8(row + 0x40) == 3)
            invoke(c, 0x1049f8e0, 0x106dcc48, {first, c.u32(row), lower, 1});
    }
    return true;
}

void dispose_season(Cpu& c, u32 first) {
    invoke(c, 0x104a6660, 0x106dbf74, {first, 0});
    if (const auto schedule = c.u32(first + league::schedule)) {
        invoke(c, comp::callee::free_block, 0x106dbf84, {schedule});
        c.put32(first + league::schedule, 0);
    }
    const auto stages = c.u32(first + object::stages);
    for (int i = 0; i <= c.i32(first + object::last_stage); ++i) {
        if (const auto child = c.u32(stages + u32(i) * 4))
            virtual_call(c, 0x08, 0x1057e02c, {child, 1});
    }
    c.put32(first + object::last_stage, UINT32_MAX);
    if (stages) invoke(c, comp::callee::free_block, 0x1057e058, {stages});
    c.put32(first + object::stages, 0);
    c.put32(first + object::word_2c, 0);
    // The statistics object is reset in place; record_positions reuses it
    // once the next season's table exists.
    if (const auto stats = c.u32(first + object::stats))
        invoke(c, 0x104564a0, 0x1057e06c, {stats, 1});
}
}

void exchange_divisions(Cpu& c, u32 first) {
    if (!second_division(c, first)) {
        fail(c, "the 18-club second division is unavailable");
        return;
    }
    const Frame frame(c, 0x106dccc0, 0x50, {.first_gpr = 27});
    invoke(c, 0x104a9160, 0x106dbf54, {first, 0});
}

void rollover(Cpu& c, u32 first) {
    const auto second = second_division(c, first);
    if (!second) {
        fail(c, "the 18-club second division is unavailable");
        return;
    }
    const Frame frame(c, 0x106dbf10, 0x100, {.first_gpr = 25});
    const auto count = c.u32(c.toc(comp::toc::club_count));
    const auto candidates = invoke(c, comp::callee::allocate_block, 0x106dca94, {count * 4});
    if (!candidates) {
        fail(c, "could not allocate the lower-division candidate list");
        return;
    }
    c.put8(first + object::byte_49, 0);
    invoke(c, 0x104a9160, 0x106dbf54, {first, 0});
    const bool exchanged = exchange_lower_clubs(c, first, second, candidates);
    invoke(c, comp::callee::free_block, 0x106dcca4, {candidates});
    if (!exchanged) {
        // Without five lower-division clubs the relegated clubs cannot move
        // either; starting the next season would leave stale memberships.
        fail(c, "fewer than five lower-division clubs to promote");
        return;
    }
    dispose_season(c, first);
    c.put16(first + object::year, c.u16(first + object::year) + 1u);
    if (!virtual_call(c, 0x94, 0x106dbfe8, {first}) ||
        !invoke(c, 0x106dbcb0, 0x106dc050, {first}) ||
        !invoke(c, 0x104a00a0, 0x106dc0b4, {first}) ||
        !invoke(c, 0x1049eb30, 0x106dc11c, {first, 0})) {
        fail(c, "could not initialize the next season");
        return;
    }
    virtual_call(c, 0x10, 0x106dc1dc, {second});
    virtual_call(c, 0x64, 0x106dc210, {first});
    invoke(c, 0x104aa3a0, 0x106dc21c, {first});
    invoke(c, 0x102d46c0, 0x106dc22c, {c.u32(c.toc(-0x7428)), c.u32(first + object::record)});
}
}
