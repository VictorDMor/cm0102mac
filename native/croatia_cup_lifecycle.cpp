#include "croatia_cup_lifecycle.hpp"
#include "croatia_cup_entrants.hpp"
#include "cup_base_detail.hpp"

namespace cm::aot::croatia_cup {
namespace {
using guest::Cpu;
using guest::Frame;
using u32 = std::uint32_t;
namespace object = comp::competition;
namespace field = cup_base::cup;
constexpr int vtable_slot = -0x5d3c, strings_slot = -0x4120;
constexpr u32 stats_size = 0x634, create_stats = 0x104533a0, destroy_stats = 0x104536c0, reset_stats = 0x104564a0;

void report(Cpu& c, const Frame& frame, u32 call, u32 name, u32 extension, u32 line) {
    const auto pool = c.r(31);
    guest::report_assertion(c, {call, pool, pool + 0xc, pool + 0x19, frame.at(name), frame.at(extension), line});
    c.set(0, 0);
}

u32 configure_rounds(Cpu& c, u32 cup, u32 back) {
    return cup_method(c, cup, 0x94, 0x106bfe30, back, [&] {
        c.set(3, configure(c, cup, season_profile(c, cup)));
    });
}

std::int32_t draw_tree(Cpu& c, u32 cup, u32 back) {
    return std::int32_t(cup_method(c, cup, 0x9c, 0x10479100, back, [&] {
        c.set(3, u32(cup_base::tree::map_fixture_tree(c, cup)));
    }));
}

void rank_clubs(Cpu& c, u32 cup, u32 back) {
    cup_method(c, cup, 0x64, 0x10485b10, back, [&] { cup_base::queries::rank_teams(c, cup); });
}

// The destructor keeps the old pointers until base destruction. Rollover must
// clear them as it goes: a failed next-season setup is itself destructible.
void release_rounds(Cpu& c, u32 cup, bool rollover) {
    const auto release = [&](u32 slot, u32 back) {
        const auto block = c.u32(slot);
        if (block) {
            comp::invoke(c, comp::callee::free_block, back, {block});
            if (rollover) c.put32(slot, 0);
        }
    };
    release(cup + field::entrants, rollover ? 0x106bfc18 : 0x106bf410);
    release(cup + field::rounds, rollover ? 0x106bfc30 : 0x106bf420);
    const auto matches = cup + field::round_matches;
    if (!c.u32(matches)) return;
    if (rollover) c.set(27, 0);
    for (int i = 0;; ++i) {
        c.set(rollover ? 25 : 30, u32(i));
        c.set(rollover ? 26 : 31, u32(i) * 4);
        if (i >= c.i16(cup + object::rounds)) break;
        release(c.u32(matches) + u32(i) * 4, rollover ? 0x106bfc6c : 0x106bf44c);
    }
    release(matches, rollover ? 0x106bfc90 : 0x106bf468);
}
}

u32 construct(Cpu& c, u32 cup, u32 year, u32 definition) {
    const Frame frame(c, 0x106bf0f0, 0xf0, {.first_gpr = 27});
    c.set({{27, year}, {28, definition}, {29, c.toc(comp::toc::success)},
           {30, c.toc(comp::toc::message)}, {31, c.toc(strings_slot)}});
    c.put32(frame.at(0x108), cup);
    c.link(0x106bf120);
    cup_base::construct(c, cup);
    c.put32(cup + object::vtable, c.toc(vtable_slot));
    c.put16(cup + object::year, year);
    c.put8(cup + field::current_round, 0);
    c.put8(cup + object::owner, 0xff);
    c.put32(cup + object::last_stage, UINT32_MAX);
    c.put32(cup + object::word_2c, 0);
    c.put32(cup + object::record, definition);
    c.put8(cup + object::type, 1);
    c.put8(cup + object::bench, 7);
    c.put8(cup + object::substitutes, 3);
    c.put8(cup + object::ruling_body, 0x21);
    c.put8(cup + field::byte_b0, 0);

    c.set({{4, 1}, {5, 3}, {6, 7}, {7, 1}, {8, UINT32_MAX}, {9, 0}});
    const auto loaded = cup_method(c, cup, 0x20, 0x104831a0, 0x106bf1b0, [&] {
        c.set(3, cup_base::load_from_disk(c, cup, 1));
    });
    if (loaded) return cup;
    c.link(0x106bf1cc);
    c.set(3, cup);
    if (select_entrants(c, cup, season_profile(c, cup)) <= 0) {
        report(c, frame, 0x106bf1e8, 0xb8, 0x50, 0x40);
    } else if (!configure_rounds(c, cup, 0x106bf23c)) {
        report(c, frame, 0x106bf25c, 0x98, 0x48, 0x47);
    } else if (!draw_tree(c, cup, 0x106bf2b0)) {
        report(c, frame, 0x106bf2d0, 0x78, 0x40, 0x4e);
    } else {
        const auto stats = comp::invoke(c, comp::callee::allocate_object, 0x106bf31c, {stats_size});
        c.set(27, stats);
        if (stats) comp::invoke(c, create_stats, 0x106bf32c, {stats, cup});
        c.put32(cup + object::stats, stats);
        if (stats) rank_clubs(c, cup, 0x106bf3a8);
        else report(c, frame, 0x106bf358, 0x58, 0x38, 0x55);
    }
    return cup;
}

u32 destroy(Cpu& c, u32 cup, u32 deleting) {
    const Frame frame(c, 0x106bf3d0, 0x50, {.first_gpr = 28});
    c.set({{28, cup}, {29, deleting}});
    if (!cup) return cup;
    c.put32(cup + object::vtable, c.toc(vtable_slot));
    release_rounds(c, cup, false);
    if (const auto stats = c.u32(cup + object::stats))
        comp::invoke(c, destroy_stats, 0x106bf47c, {stats, 1});
    c.set({{3, cup}, {4, 0}});
    c.link(0x106bf488);
    cup_base::destroy(c, cup, 0);
    if (std::int16_t(deleting) > 0)
        comp::invoke(c, comp::callee::free_object, 0x106bf498, {cup});
    return cup;
}

void next_season(Cpu& c, u32 cup) {
    const Frame frame(c, 0x106bfbe0, 0xd0, {.first_gpr = 25});
    c.set({{28, cup}, {29, c.toc(comp::toc::success)}, {30, c.toc(comp::toc::message)}, {31, c.toc(strings_slot)}});
    c.put8(cup + object::byte_49, 0);
    release_rounds(c, cup, true);
    if (const auto stats = c.u32(cup + object::stats))
        comp::invoke(c, reset_stats, 0x106bfcac, {stats, 1});
    const auto year = u32(c.i16(cup + object::year)) + 1;
    c.put16(cup + object::year, year);
    c.put8(cup + field::current_round, 0);
    c.put8(cup + field::byte_b0, 0);
    c.set({{3, cup}, {4, year}});
    c.link(0x106bfccc);
    if (select_entrants(c, cup, season_profile(c, cup)) < 0)
        report(c, frame, 0x106bfce8, 0x90, 0x48, 0x1a5);
    else if (std::int32_t(configure_rounds(c, cup, 0x106bfd38)) < 0)
        report(c, frame, 0x106bfd58, 0x70, 0x40, 0x1ac);
    else if (draw_tree(c, cup, 0x106bfda8) < 0)
        report(c, frame, 0x106bfdc8, 0x50, 0x38, 0x1b3);
    else rank_clubs(c, cup, 0x106bfe18);
}
}
