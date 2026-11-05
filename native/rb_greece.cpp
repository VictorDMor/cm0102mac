#include "rb_greece_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"

#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
bool declining = std::getenv("CM_RB_GREECE_ORIGINAL") != nullptr;
constexpr auto nation_text_entry = 0x106ab7d0u, next_threshold_entry = 0x106ab890u;
constexpr auto construct_entry = 0x106ab8f0u;
constexpr auto nation_text = 0x10168a00u, construct_base_rulebook = 0x105bb3e0u;
constexpr auto split_source_name = 0x10739890u, version_text = 0x10768810u;
constexpr auto format_message = 0x1077b720u, show_error = 0x10769b60u;

struct DisciplineRulebook {
    Cpu& c;
    std::uint32_t address;
    void set_initial_policy() const {
        c.put8(address + 0x20, 16);
        c.put8(address + 0x21, UINT32_MAX);
        c.put8(address + 0x22, 1);
        c.put8(address + 0x23, 0);
    }
};

void block(Cpu& c, std::uint32_t pc, unsigned instructions) {
    c.s.step(pc);
    c.s.charge(instructions - 1);
}

void push_frame(Cpu& c, GuestNativeScope& scope, std::uint32_t caller, std::uint32_t frame) {
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
}

void restore_link(Cpu& c, std::uint32_t caller) {
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
}

void format_nation_text(Cpu& c) {
    GuestNativeScope scope(c.s, nation_text_entry);
    block(c, nation_text_entry, 9);
    const auto caller = c.sp(), frame = caller - 0x70;
    const auto text_width = std::int32_t(c.r(5));
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.compare(0, text_width, 20);
    const auto source = c.toc(-0x41d4);
    c.set(31, source);
    c.put32(caller - 8, c.r(30));
    const auto message = c.toc(-0x7ba8);
    c.set(30, message);
    push_frame(c, scope, caller, frame);
    if (text_width < 20) {
        block(c, 0x106ab7f4, 6);
        c.set({{3, source}, {4, 0}, {5, 0}, {6, frame + 0x40}, {7, frame + 0x38}});
        c.call(split_source_name, 0x106ab80c);
        block(c, 0x106ab80c, 1);
        c.call(version_text, 0x106ab810);
        block(c, 0x106ab810, 7);
        c.set({{5, c.r(3)}, {3, message}, {4, source + 0xe},
               {6, frame + 0x40}, {7, frame + 0x38}, {8, 0xbc}});
        c.call(format_message, 0x106ab82c);
        block(c, 0x106ab82c, 4);
        c.set({{4, message}, {3, source + 0x1b}, {5, 0}});
        c.call(show_error, 0x106ab83c);
        block(c, 0x106ab83c, 4);
        const auto success = c.toc(-0x7bac);
        c.set({{3, success}, {0, 0}});
        c.put8(success, 0);
    } else {
        block(c, 0x106ab84c, 8);
        const auto nation_index = c.toc(-0x7598);
        c.set({{3, nation_index}, {5, std::uint32_t(std::int16_t(text_width))}});
        const auto nation_registry = c.toc(-0x7b98);
        c.set(7, nation_registry);
        const auto index = c.u32(nation_index);
        c.set(0, index);
        const auto nations = c.u32(nation_registry);
        c.set(3, nations);
        const auto offset = index * 0x128u;
        c.set({{0, offset}, {3, nations + offset}});
        c.call(nation_text, 0x106ab86c);
    }
    block(c, 0x106ab86c, 6);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.finish();
}

// The optional outputs can alias each other or the card tally. Read the
// tally only after the original ordered writes.
void next_threshold(Cpu& c) {
    const auto tally = c.r(4), match_ban = c.r(5), dismissal = c.r(6);
    block(c, next_threshold_entry, 2);
    c.compare_logical(0, match_ban, 0);
    if (match_ban) {
        block(c, 0x106ab898, 2);
        c.set(0, 1);
        c.put8(match_ban, 1);
    }
    block(c, 0x106ab8a0, 2);
    c.compare_logical(0, dismissal, 0);
    if (dismissal) {
        block(c, 0x106ab8a8, 2);
        c.set(0, UINT32_MAX);
        c.put8(dismissal, UINT32_MAX);
    }
    block(c, 0x106ab8b0, 10);
    const unsigned next_card = c.u8(tally + 4) + 1u;
    const unsigned remainder = next_card % 3;
    c.set({{0, remainder}, {4, next_card}});
    c.compare(0, remainder, 0);
    block(c, remainder ? 0x106ab8e0 : 0x106ab8d8, 2);
    c.set(3, remainder == 0);
    c.finish();
}

void construct(Cpu& c) {
    GuestNativeScope scope(c.s, construct_entry);
    block(c, construct_entry, 6);
    const auto caller = c.sp(), frame = caller - 0x40;
    const DisciplineRulebook rulebook{c, c.r(3)};
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, rulebook.address);
    push_frame(c, scope, caller, frame);
    c.call(construct_base_rulebook, 0x106ab908);

    block(c, 0x106ab908, 26);
    const auto vtable = c.toc(-0x5ddc);
    c.set({{0, vtable}, {9, 0}, {8, 16}, {7, 10}});
    c.put32(rulebook.address, vtable);
    c.set(4, UINT32_MAX);
    c.set(6, c.toc(-0x7574));
    c.set(0, 1);
    c.put8(rulebook.address + 4, 0);
    c.set(3, rulebook.address);
    c.set(5, c.toc(-0x7a70));
    c.put8(rulebook.address + 5, 16);
    c.put8(rulebook.address + 6, 10);
    const auto national_league = c.u32(c.r(6));
    c.set(6, national_league);
    c.put32(rulebook.address + 0x18, national_league);
    const auto rule_group = c.u32(c.r(5));
    c.set(5, rule_group);
    c.put32(rulebook.address + 0x1c, rule_group);
    rulebook.set_initial_policy();
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.finish();
}

}

void rb_greece_run_original(State& state, std::uint32_t entry) {
    run_original_body(state, entry, declining);
}

void rb_greece_run_rewrite(State& state, std::uint32_t entry) {
    Cpu c(state);
    switch (entry) {
    case nation_text_entry: format_nation_text(c); break;
    case next_threshold_entry: next_threshold(c); break;
    case construct_entry: construct(c); break;
    default: throw std::logic_error("Unknown rb_greece entry " + hex_address(entry));
    }
}

bool try_modern_rb_greece(State& state, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(state, entry, rb_greece_run_original, rb_greece_run_rewrite, RoutineChecker::result_r3);
    rb_greece_run_rewrite(state, entry);
    return true;
}
}
