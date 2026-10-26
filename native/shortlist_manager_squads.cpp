#include "shortlist_manager_squads_detail.hpp"

// 0x1035a9f0, the club squad assessment: its opening checks, the club's
// target reputation and the plan's layout. The later phases are in
// shortlist_manager_squads_{players,choices,values}.cpp.
namespace cm::aot::shortlist_manager {
namespace squads {
std::int32_t random_below(Assessment& a, std::int32_t range, const RandomSite& site, unsigned result_register) {
    Cpu& c = a.c;
    const DrawSite draw{site.rand_back, site.division, site.random_back, a.at(site.range_double),
                        a.at(site.value_double), a.at(site.result), false, Assessment::step};
    return draw_below(c, a.fpu, RandomTable{c}, draw, range, result_register, [&](const WideDraw& d) {
        // What the two copies' schedules leave in r4-r8 at the call.
        if (site.second)
            leave(c, {{4, d.coarse.sign}, {5, d.multiple}, {6, 0xffff},
                      {7, std::uint32_t(multiply_high(std::int32_t(0x80008001), std::int32_t(d.x)))}, {8, d.entry}});
        else
            leave(c, {{4, d.multiple}, {5, d.x}, {6, 0xffff}, {7, 0x80008001}, {8, d.entry}});
    });
}

bool opening(Assessment& a) {
    Cpu& c = a.c;
    auto& program = a.program;
    const auto plan = a.plan();
    const auto club = a.club();
    // The days from the game's start (passed by value) to today.
    const auto start = c.toc(slot::start_date);
    c.set(8, start);
    c.put32(a.at(local::days), program.call(call::days_between, 0x1035aa5c, {c.r(16), c.u32(start), c.u32(start + 4)}));
    c.put32(a.at(local::estimate_flags), a.human(0, 0x1035aa78) ? 0x800 : 0x1802);
    // A plan never laid out (the first place's index still -1) is laid out now.
    if (Row{c, a.slots()}.rank() == -1) keep(c, {{21, 1}});
    plan.changes() = 0;
    auto skip = [&] {
        plan.status() = 0;
        return false;
    };
    if (nonzero(c, std::int8_t(program.call(call::club_check_7d00, 0x1035aab8, {club})))) return skip();
    if (!nonzero_logical(c, c.u32(club + 0x2c))) return skip();
    if (nonzero(c, std::int8_t(program.call(call::human_club, 0x1035aae4, {club, 0, 1, 0, 0}))) &&
        nonzero(c, std::int8_t(c.r(15))))
        return skip();
    if (compare(c, c.u8(club + 0x23d), 3) == 0) return skip();
    if (!nonzero_logical(c, program.call(call::own_staff_query, 0x1035ab28, {club, a.at(local::staff_flag), 1})))
        c.put8(a.at(local::staff_flag), 0);
    else if (!nonzero(c, a.local8(local::staff_flag)))
        return skip();

    if (nonzero(c, std::int8_t(program.call(call::count_and_prune_rows, 0x1035ab60, {a.manager, club, 0xffffffff})))) {
        // A plan waiting for its refresh keeps waiting (and its state).
        return compare(c, c.u8(plan.address + 4), 2) != 0;
    }
    if (compare(c, c.u8(plan.address + 4), 2) == 0) {
        const auto human = std::int8_t(program.call(call::human_club, 0x1035ab8c, {club, 1, 1, 0, 0}));
        plan.status() = 1;
        keep(c, {{21, 1}});
        c.put8(a.at(local::refreshed), human == 0);
    }
    // A club in an established league (one whose competition is set up)
    // and big clubs (reputation 4000 or more) are always assessed; the others
    // only now and then: when random(reputation + 1000) is at least a random
    // number below 5000.
    const auto league = c.u32(club + 0x2c);
    if (nonzero(c, c.i8(league + 0x120))) {
        const auto competition = c.u32(club + 0x30);
        if (nonzero_logical(c, competition)) {
            const auto table = c.u32(c.toc(slot::competitions));
            if (nonzero_logical(c, c.u32(table + c.u32(competition) * 4))) return true;
        }
    }
    const std::int32_t reputation = a.reputation();
    if (compare(c, reputation, 4000) >= 0) return true;
    const Kept<15> range{c, std::uint32_t(reputation + 1000)};
    const std::int32_t chance =
        random_below(a, std::int32_t(std::uint32_t(range)), {0x1035ac30, 0x1035ad04, 0x1035ad54, 0x9d4, 0x9cc, 0x9c4, false}, 17);
    const std::uint32_t value = a.draw(0x1035ad9c);
    const std::int32_t threshold = std::int32_t(value) % 5000;
    if (compare(c, chance, threshold) < 0) {
        leave(c, {{3, 0}, {4, value}, {5, RandomTable{c}.position()}});
        return skip();
    }
    return true;
}

void clear_lists(Assessment& a) {
    struct List {
        std::int32_t offset;
        std::uint32_t size, back;
    };
    constexpr List lists[] = {{local::positions_a, 100, 0x1035ae10}, {local::positions_b, 100, 0x1035ae20},
                              {local::positions_c, 100, 0x1035ae30}, {local::picked_a, 200, 0x1035ae40},
                              {local::picked_c, 200, 0x1035ae50},    {local::picked_b, 200, 0x1035ae60},
                              {local::candidates, 400, 0x1035ae70},  {local::held_back, 200, 0x1035ae80}};
    for (const auto& list : lists) a.program.call(call::memset, list.back, {a.at(list.offset), 0, list.size});
    a.plan().status() = 1;
}

void set_target(Assessment& a) {
    Cpu& c = a.c;
    const auto club = a.club();
    // r18: the club's nation record (0x6c bytes; inferred), from the club's
    // nation, or for a big club of the home nation (and a club without one)
    // from the club's entry of toc::club_nations.
    const auto records = [&] { return c.u32(c.r(13)); };
    const std::int32_t nation = c.i32(club + 0x14);
    bool own = compare(c, nation, -1) > 0;
    if (own && compare(c, nation, c.i32(c.toc(toc::home_nation))) == 0) own = compare(c, a.reputation(), 5000) < 0;
    if (own) {
        keep(c, {{18, records() + std::uint32_t(nation) * 0x6c}});
    } else {
        const std::int32_t other = c.i32(c.u32(c.toc(toc::club_nations)) + c.u32(club) * 8);
        if (compare(c, other, -1) > 0) keep(c, {{18, records() + std::uint32_t(other) * 0x6c}});
    }
    c.put32(a.at(local::value), a.program.call(call::club_value, 0x1035aef8, {club, a.at(local::standard), 0}));

    // The target: a tenth of the club's +8 (inferred: its value) plus 4500,
    // scaled by the manager's and (twice) the chairman's sway (inferred), at
    // least the club's reputation and at most 10000.
    const std::uint32_t table = c.u32(a.manager + 0xdc);
    const std::uint32_t id_offset = c.u32(club) * 0xc;
    const Kept<13> entry{c, table + id_offset};
    const Field<std::int16_t> target{c, entry + 6};
    target = std::int16_t(c.i32(club + 8) / 10 + 0x1194);
    c.set(8, id_offset);
    c.set(9, table);
    if (!a.human(1, 0x1035af4c)) {
        const auto manager = c.u32(club + 0x90);
        if (nonzero_logical(c, manager)) {
            const std::int32_t sway = std::int32_t(c.i8(manager + 0x63)) - c.i8(c.u32(manager + 0x1c) + 0x3d);
            const double scale = a.fpu.fdiv(a.integer(0x9cc, sway), a.constant(0x88));
            const double current = a.integer(0x9c4, target);
            c.set_f(1, current);  // the original computes it into f1
            target = std::int16_t(a.truncated(0x9d4, a.fpu.fmul(current, a.fpu.fadd(a.constant(0x80), scale))));
        }
    }
    const auto chairman = c.u32(club + 0x80);
    if (nonzero_logical(c, chairman) && nonzero_logical(c, c.u32(chairman + 0x1c))) {
        const auto board = c.u32(chairman + 0x1c);
        const std::int32_t sway = std::int32_t(c.i8(chairman + 0x63)) - c.i8(board + 0x3d);
        const double scale = a.fpu.fdiv(a.integer(0x9cc, sway), a.constant(0x90));
        c.put32(a.at(0x9b4), 0x43300000);
        c.put32(a.at(0x9bc), 0x43300000);
        const double current = a.integer(0x9c4, target);
        target = std::int16_t(a.truncated(0x9d4, a.fpu.fmul(current, a.fpu.fadd(a.constant(0x80), scale))));
        const std::int32_t mood = std::int32_t(c.i8(c.u32(c.u32(club + 0x80) + 0x1c) + 0x40)) - 10;
        const double again = a.integer(0x9bc, target);
        const double moody = a.integer(0x9b4, mood);
        c.set_f(1, moody);  // the original computes it into f1
        const double ratio = a.fpu.fdiv(moody, a.constant(0x98));
        target = std::int16_t(a.truncated(0x9ac, a.fpu.fmul(again, a.fpu.fadd(a.constant(0x80), ratio))));
    }
    const std::int16_t reputation = a.reputation();
    const std::int16_t aimed = target;
    if (compare(c, reputation, aimed) > 0) target = reputation;
    else target = compare(c, aimed, 10000) > 0 ? std::int16_t(10000) : aimed;

    if (a.human(1, 0x1035b0f4)) {
        keep(c, {{26, a.program.call(call::human_manager, 0x1035b104, {club})}});
        const auto division = a.program.call(call::club_division, 0x1035b114, {club});
        c.put8(a.at(local::option), a.program.call(call::option_7e70, 0x1035b120, {c.u32(c.r(14)), division}));
    }
}

void lay_out_places(Assessment& a) {
    Cpu& c = a.c;
    const auto plan = a.plan();
    const auto club = a.club();
    if (!nonzero(c, std::int8_t(c.r(21)))) {
        // Keep the plan; sort it by the places' indices.
        a.program.qsort(a.slots(), slot_count, Row::size, c.toc(slot::compare_10358ff0_tv), 0x1035b4ac);
        return;
    }
    // The formation: the manager's preferred one (+0x43 of his staff data;
    // 3 without), 0 for a club in a human's hands, without a manager or
    // when the game option says so (inferred).
    [&] {
        const auto set = [&](std::int8_t formation) { plan.formation() = formation; };
        if (a.human(0, 0x1035b144)) return set(0);
        const auto manager = c.u32(club + 0x90);
        if (!nonzero_logical(c, manager)) return set(0);
        const auto data = c.u32(manager + 0x1c);
        if (nonzero_logical(c, data) && !nonzero(c, c.i8(data + 0x43))) return set(0);
        if (nonzero(c, a.local8(local::option))) {
            const auto division = a.program.call(call::club_division, 0x1035b190, {club});
            if (nonzero(c, std::int8_t(a.program.call(call::option_9df0, 0x1035b1a0, {c.u32(c.r(14)), 2, division}))))
                return set(0);
        }
        const auto preferred = c.u32(c.u32(club + 0x90) + 0x1c);
        if (!nonzero_logical(c, preferred)) return set(3);
        set(std::int8_t(c.u8(preferred + 0x43)));
    }();
    enter(c, 0x1035b1e4, {a.manager, plan.address});
    lay_out_formation(c, a.manager, plan.address);

    // Each place's index and date; the roles: place 0 and key places 1-23
    // (positions 1) first team; the rest by place, and for a club short of
    // money (inferred: standard 6000 or more, or 8500 with its nation's
    // record) fewer first-team places.
    Kept<14> index{c, 0};
    Kept<13> offset{c, 0};
    for (; std::int8_t(std::uint32_t(index)) < slot_count; index = index + 1, offset = offset + Row::size) {
        const std::int32_t i = std::int8_t(std::uint32_t(index));
        Row{c, a.slots() + offset}.rank() = std::int8_t(i);
        a.program.call(call::today_date, 0x1035b20c, {a.at(0x98), 1, 0x7cd});
        const Row slot{c, a.slots() + offset};
        c.put16(slot.date().address + 2, c.u16(a.at(0x9a)));
        c.put16(slot.date().address, c.u16(a.at(0x98)));
        c.put8(slot.date().address + 4, c.u8(a.at(0x9c)));
        if (i == 0) {
            slot.set_category(2);
            continue;
        }
        if (i > 0 && i < 24 && std::int16_t(slot.positions()) == 1) {
            slot.set_category(4);
            continue;
        }
        const std::int32_t standard = a.local32(local::standard);
        const bool flag = a.local8(local::staff_flag) != 0;
        auto by_place = [&] { slot.set_category(i < 11 ? 2 : i < 20 ? 4 : 5); };
        if (flag || a.human(1, 0x1035b2b4)) {
            if (standard < 6000 && !nonzero(c, a.local8(local::staff_flag))) {
                by_place();
                continue;
            }
            c.put8(a.at(local::over_budget), 1);
            if (i < 19) slot.set_category(3);
            else slot.set_category(a.local8(local::staff_flag) != 0 ? 4 : 5);
            continue;
        }
        if ((c.r(18) == 0 || standard < 6000) && standard < 8500) {
            by_place();
            continue;
        }
        c.put8(a.at(local::over_budget), 1);
        slot.set_category(i < 20 ? 3 : 5);
    }
}
}

void assess_squad(Cpu& c, std::uint32_t manager, std::uint32_t plan, std::uint32_t club, std::uint32_t skip_human,
                  std::uint32_t refresh) {
    using namespace squads;
    Frame frame(c, entry, 0xa50, {.first_gpr = 13});
    keep(c, {{19, plan},
             {16, c.toc(slot::today)},
             {20, club},
             {14, c.toc(toc::options)},
             {15, skip_human},
             {13, c.toc(toc::records)},
             {21, refresh},
             {29, c.toc(slot::contracts)},
             {26, 0},
             {30, c.toc(slot::constants)},
             {18, 0}});
    c.put8(frame.at(local::option), 0);
    c.put8(frame.at(local::refreshed), 0);
    c.put8(frame.at(local::over_budget), 0);
    c.put32(frame.caller() + 0x18, manager);
    Assessment a{c, frame, Program{c}, Fpu{c}, manager};
    if (!opening(a)) return;
    clear_lists(a);
    set_target(a);
    lay_out_places(a);
    collect_open_places(a);
    sort_squad(a);
    pick_for_places(a);
    review_candidates(a);
    rate_first_choices(a);
    place_choices(a);
    value_open_places(a);
    assign_staff(a);
}
}
