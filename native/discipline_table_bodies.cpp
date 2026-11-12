// DISCIPLINE.CPP table block (discipline_table.hpp): what the manager asks of the ruling bodies
// (RULING_BODY.CPP objects) on behalf of other modules, native-first.
#include "discipline_table.hpp"
#include "discipline_table_detail.hpp"

namespace cm::aot::discipline {
namespace {
namespace callee {
constexpr std::uint32_t virtual_call = 0x10781320, ban_check = 0x105bc600, human_manager_check = 0x1014c2e0,
                        competition_query = 0x105bcef0, slot_query = 0x105bca10, fixture_hook = 0x105bbee0,
                        competition_filter = 0x10494d20;
}
namespace vtable {
constexpr std::uint32_t competition_method = 8, all_bodies_method = 0xc;
}

// The ruling body object governing a competition object: null when the competition has none (index -1) or its
// slot holds none.
std::uint32_t governing_body(const guest::Cpu& c, std::uint32_t manager_object, std::uint32_t competition_object) {
    const auto index = c.i8(competition_object + competition::ruling_body);
    if (index == no_ruling_body) return 0;
    return body_object(c, manager_object, std::uint32_t(std::int32_t(index)));
}

// `lwz r12,0(r3); lwz r12,vtable_offset(r12); bl 0x10781320; lwz r2,0x14(r1)`: the glue switches the TOC for the
// method and leaves the caller's in the frame's linkage word. The caller sets r4... as the method reads them.
void call_virtual(guest::Cpu& c, std::uint32_t object, std::uint32_t vtable_offset, std::uint32_t return_address) {
    c.set(3, object);
    c.set(12, c.u32(c.u32(object) + vtable_offset));
    c.call(callee::virtual_call, return_address);
    c.set(2, c.u32(c.sp() + 0x14));
}
}

void notify_competition_body(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t competition_id_at) {
    guest::Frame frame(c, 0x100df260, 0x40, {});
    const auto competition_at = competition_object(c, c.u32(competition_id_at));
    if (competition_at == 0) return;
    const auto body = governing_body(c, manager_object, competition_at);
    if (body == 0) return;
    call_virtual(c, body, vtable::competition_method, 0x100df2b8);
}

void notify_all_bodies(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t argument) {
    guest::Frame frame(c, 0x100df2d0, 0x50, {.first_gpr = 28});
    for (std::int8_t index = 0; index < manager::body_count; ++index) {
        const auto body = body_object(c, manager_object, std::uint32_t(index));
        if (body == 0) continue;
        c.set(4, argument);
        call_virtual(c, body, vtable::all_bodies_method, 0x100df320);
    }
}

std::uint32_t is_suspended(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person,
                           std::uint32_t competition_id_at, std::uint32_t rule_argument_a, std::uint32_t rule_argument_b) {
    guest::Frame frame(c, 0x100e7920, 0x50, {.first_gpr = 27});
    if (!person_in_table(c, person)) return 0;
    if (c.u32(c.toc(table_toc::competitions)) == 0) return 0;
    const auto competition_at = competition_object(c, c.u32(competition_id_at));
    if (competition_at == 0) return 0;
    const auto ask = [&](std::uint32_t body, std::uint32_t first_level, std::uint32_t return_address) {
        c.set({{3, body}, {4, person}, {5, first_level}, {6, rule_argument_a}, {7, rule_argument_b}});
        c.call(callee::ban_check, return_address);
        return std::int8_t(c.r(3)) != 0;
    };
    // The general body (index 0) first, then the competition's own.
    if (ask(body_object(c, manager_object, 0), 1, 0x100e79b0)) return 1;
    const auto body = governing_body(c, manager_object, competition_at);
    if (body == 0) return 0;
    if (!ask(body, 0, 0x100e79f8)) return 0;
    const auto human_manager = c.u32(person + person_record::human_manager);
    if (human_manager != 0) {
        c.set({{3, human_manager}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(callee::human_manager_check, 0x100e7a20);
        if (std::int8_t(c.r(3)) != 0) {
            // The human manager's list of 20 pointers: the first entry that is this person is cleared.
            auto list_at = c.u32(person + person_record::human_manager) + person_record::human_manager_list;
            for (std::int8_t n = 0; n < person_record::human_manager_list_size; ++n, list_at += 4)
                if (c.u32(list_at) == person) {
                    c.put32(list_at, 0);
                    break;
                }
        }
    }
    return 1;
}

std::uint32_t body_query_for_competition(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person,
                                         std::uint32_t competition_id_at) {
    guest::Frame frame(c, 0x100e7a90, 0x40, {});
    if (!person_in_table(c, person)) return 0;
    const auto competition_at = competition_object(c, c.u32(competition_id_at));
    if (competition_at == 0 || person == 0) return 0;
    const auto body = governing_body(c, manager_object, competition_at);
    if (body == 0) return 0;
    c.set({{3, body},
           {4, c.u32(person + person_record::id)},
           {5, 0},
           {6, 0},
           {7, c.u32(competition_at + competition::ruling_argument)}});
    c.call(callee::competition_query, 0x100e7b24);
    return c.r(3);
}

std::uint32_t body_query_for_slot(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot,
                                  std::uint32_t argument_a, std::uint32_t argument_b) {
    guest::Frame frame(c, 0x100e7b40, 0x40, {});
    if (!person_in_table(c, person)) return 0;
    // A null person passes the test above and is read here (the original's address 0).
    const auto index = c.i8(slot_address(row_address(c, manager_object, c.u32(person + person_record::id)), slot));
    if (index == no_ruling_body) return 0;
    c.set({{3, body_object(c, manager_object, std::uint32_t(std::int32_t(index)))},
           {4, person},
           {5, argument_a},
           {6, argument_b}});
    c.call(callee::slot_query, 0x100e7bb0);
    return c.r(3);
}

void match_day_fixture(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t fixture_at, std::uint32_t argument) {
    guest::Frame frame(c, 0x100e6e90, 0x50, {.first_gpr = 28});
    const auto send = [&](std::uint32_t body, bool a, bool b, bool d, std::uint32_t return_address) {
        c.set({{3, body}, {4, fixture_at}, {5, argument}, {6, a}, {7, b}, {8, d}});
        c.call(callee::fixture_hook, return_address);
    };
    const auto competition_id = c.i32(fixture_at + fixture::competition_id);
    std::uint32_t competition_at = 0;
    if (competition_id > -1) competition_at = competition_object(c, std::uint32_t(competition_id));
    if (competition_at != 0) {
        if (const auto body = governing_body(c, manager_object, competition_at); body != 0) {
            send(body, true, true, true, 0x100e6f1c);
            return;
        }
    }
    c.set({{3, c.u32(c.toc(table_toc::fixture_filter))}, {4, c.u32(fixture_at + fixture::filter_word)}});
    c.call(callee::competition_filter, 0x100e6f30);
    if (std::int8_t(c.r(3)) == 0 && c.i32(fixture_at + fixture::competition_id) != c.i32(c.toc(table_toc::friendly_competition)))
        return;
    // Each side's league (a pointer at +0x30 of the side) has its own ruling body.
    const auto side_body = [&](std::uint32_t side_field) -> std::uint32_t {
        const auto league = c.u32(c.u32(fixture_at + side_field) + fixture::side_league);
        if (league == 0) return 0;
        const auto league_competition = competition_object(c, c.u32(league));
        return league_competition == 0 ? 0 : governing_body(c, manager_object, league_competition);
    };
    if (const auto body = side_body(fixture::home_side); body != 0) send(body, true, false, false, 0x100e6fb4);
    if (const auto body = side_body(fixture::away_side); body != 0) send(body, false, true, false, 0x100e701c);
}
}
