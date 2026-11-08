// MATCH_MAN.CPP lineup_b block (match_man_lineup_b.hpp): the computer's lineup choice, 0x102065f0.
#include "match_man_lineup_b.hpp"
#include "guest_values.hpp"
#include "match_man_adapter.hpp"
#include "match_man_detail.hpp"
#include "match_man_lineup_b_detail.hpp"

namespace cm::aot::match_man {
using namespace guest;
namespace {
constexpr std::uint32_t entry = 0x102065f0;

// One of the six asserts: the report as the game's macro expands it, then exit(-1). The
// original goes on where exit returns. `at` is the first call, `back` the exit's return.
struct Assert {
    std::uint32_t at, back, name, extension, line;
};
constexpr Assert argument_asserts[] = {
    {0x10206654, 0x10206698, 0x108, 0x60, 0x69}, {0x102066b4, 0x102066f8, 0xe8, 0x58, 0x6b},
    {0x10206714, 0x10206758, 0xc8, 0x50, 0x6d},  {0x10206774, 0x102067b8, 0xa8, 0x48, 0x6f},
    {0x102067d4, 0x10206818, 0x88, 0x40, 0x71},
};
constexpr Assert copy_assert = {0x102069c8, 0x10206a0c, 0x68, 0x38, 0xbc};

void report(Cpu& c, const Frame& frame, const Assert& a) {
    const auto pool = c.r(25);
    report_assertion(c, {.at = a.at, .file = pool, .format = pool + 0xe, .title = pool + 0x1b,
                         .name = frame.at(a.name), .extension = frame.at(a.extension), .line = a.line});
    c.set({{0, 0}, {3, 0xffffffff}});
    c.call(lineup_entries::exit_game, a.back);
}

// The people's id test the choice uses twice: an id is a real one when below the people count.
bool known_person(Cpu& c, std::uint32_t person) {
    const auto id = c.i32(person + person_id);
    return compare(c, id, c.i32(c.toc(-0x7be4))) < 0;
}
}

std::uint32_t choose_lineup(Cpu& c, std::uint32_t side, std::uint32_t match, std::uint32_t manager,
                            std::uint32_t squad_list, std::uint32_t players, std::uint32_t slot,
                            std::uint32_t club, std::uint32_t rival) {
    namespace ls = lineup_side;
    using namespace lineup_entries;
    Frame frame(c, entry, 0x170, {.first_gpr = 17});
    // The two stack parameters sit in the caller's frame (sp+0x38, sp+0x3c).
    const auto fixture = c.u32(frame.caller() + 0x38);
    const auto bench_out = c.u32(frame.caller() + 0x3c);
    const auto people_count_at = c.toc(-0x7be4);
    const auto registry = c.toc(-0x7a54);  // r17 until it takes the result
    keep(c, {{23, people_count_at}, {17, registry}, {31, side}, {19, c.toc(-0x7bac)}, {20, manager},
             {24, c.toc(-0x7ba8)}, {21, squad_list}, {25, c.toc(slots::pool)}, {22, players}, {18, slot},
             {27, club}, {28, rival}, {29, fixture}, {30, bench_out}, {26, match}});
    Kept<17> chosen(c, registry);

    // A missing argument reports an assertion and exits; where exit returns, the choice goes on.
    if (compare(c, std::int32_t(match)) == 0) report(c, frame, argument_asserts[0]);
    if (compare_logical(c, squad_list) == 0) report(c, frame, argument_asserts[1]);
    if (compare_logical(c, players) == 0) report(c, frame, argument_asserts[2]);
    if (compare_logical(c, club) == 0) report(c, frame, argument_asserts[3]);
    if (compare_logical(c, rival) == 0) report(c, frame, argument_asserts[4]);

    // The manager's record, or none.
    if (compare_logical(c, manager) != 0) {
        c.put32(side + ls::manager, manager);
        c.put32(side + ls::manager_extra, c.u32(c.u32(side + ls::manager) + 0x1c));
    } else {
        c.put32(side + ls::manager, 0);
        c.put32(side + ls::manager_extra, 0);
    }
    c.put32(side + ls::club, club);
    c.put32(side + ls::match_object, match);
    c.put32(side + ls::squad_list, squad_list);
    c.put32(side + ls::players, players);
    c.put8(side + ls::slot, slot);
    c.set(4, c.u32(side + ls::match_object));
    c.put32(side + ls::team_slot, c.u32(side + ls::match_object) +
                                  std::uint32_t(std::int32_t(std::int8_t(c.u8(side + ls::slot)))) * ls::team_slot_size +
                                  ls::team_slot_base);
    c.put32(side + ls::fixture, fixture);
    c.put32(side + ls::rival, rival);
    c.put32(side + ls::fixture_state, 0);
    c.put32(side + 0x10, 0);
    c.put32(side + 0xc, 0);
    c.put32(side + 0x8, 0);

    // The club is the fixture's first or second side with its hold byte clear (kind 1, mode 0),
    // else kind 0, mode 3.
    bool picks = false;
    if (compare_logical(c, club, c.u32(fixture + lineup_fixture::home)) == 0)
        picks = compare(c, c.i8(fixture + lineup_fixture::home_hold)) == 0;
    if (!picks && compare_logical(c, club, c.u32(fixture + lineup_fixture::away)) == 0)
        picks = compare(c, c.i8(fixture + lineup_fixture::away_hold)) == 0;
    c.set(3, picks ? 1 : 0);
    c.put8(side + ls::kind, picks ? 1 : 0);
    c.put8(side + ls::mode, picks ? 0 : 3);

    // The manager's three halfwords (lha: sign-extended, stored as halfwords).
    if (compare_logical(c, c.u32(side + ls::manager_extra)) != 0) {
        c.put16(side + ls::ratings, extended(c.i16(c.u32(side + ls::manager_extra) + 0x2a)));
        c.put16(side + ls::ratings + 2, extended(c.i16(c.u32(side + ls::manager_extra) + 0x28)));
        c.put16(side + ls::ratings + 4, extended(c.i16(c.u32(side + ls::manager_extra) + 0x2c)));
    }

    // The manager's successor or replacement from the club's squad: stored with its extra record.
    const auto take_manager = [&](std::uint32_t back) {
        c.set(3, c.u32(side + ls::club));
        c.call(next_manager, back);
        c.put32(side + ls::manager, c.r(3));
        const auto taken = c.u32(side + ls::manager);
        c.set(3, taken);
        c.put32(side + ls::manager_extra, compare_logical(c, taken) != 0 ? c.u32(taken + 0x1c) : 0u);
    };
    const auto computer_pick = [&](std::uint32_t back, std::uint32_t mode_argument) {
        c.set({{3, side}, {4, rival}, {5, mode_argument}});
        match_man::call(c, computer_lineup, back);
        chosen = c.r(3);
    };

    // Whether the club's own side chooses: the match has not started, the club is a computer
    // one and the state query allows it (0x10206b38 otherwise).
    bool own = false;
    if (compare(c, c.i8(c.u32(side + ls::match_object) + match_started)) == 0 && compare_logical(c, club) != 0) {
        c.set(3, club);
        c.call(club_is_computer, 0x1020692c);
        if (compare_logical(c, c.r(3)) != 0) {
            c.set({{3, club}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
            c.call(club_state, 0x1020694c);
            own = compare(c, std::int8_t(c.r(3))) != 0;
        }
    }
    if (own) {
        c.set(3, club);
        c.call(club_flag, 0x1020695c);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            c.set(3, c.u32(club + lineup_club::squad_head));
            c.call(club_squad_head, 0x1020696c);
            c.put32(side + ls::manager, c.u32(c.r(3) + lineup_club::head_of_staff));
        }
        c.set({{3, c.u32(registry)}, {5, club}, {4, c.u8(side + ls::kind)},
               {6, c.u32(club + lineup_club::head_of_staff)}});
        c.call(registry_a, 0x10206988);
        c.set({{3, c.u32(registry)}, {4, c.u32(club + lineup_club::head_of_staff)}});
        c.call(registry_b, 0x10206994);
        const auto found = c.r(3);
        c.set(4, found);
        bool copied = false;
        if (compare(c, std::int32_t(found)) != 0) {
            const auto destination = c.u32(side + ls::team_slot);
            c.set(3, destination);
            if (compare_logical(c, destination) != 0) {
                c.set(5, ls::team_slot_size);
                c.call(copy, 0x102069b0);
                copied = true;
            }
        }
        if (!copied) report(c, frame, copy_assert);  // where exit returns, the choice goes on

        if (compare(c, c.i8(side + ls::kind)) != 0) {
            c.set(3, club);
            c.call(club_check_a, 0x10206a20);
            if (compare(c, std::int8_t(c.r(3))) == 0) {
                c.set({{3, c.u32(registry)}, {4, c.u32(club + lineup_club::head_of_staff)}});
                c.call(registry_c, 0x10206a34);
                if (compare(c, std::int8_t(c.r(3))) != 0) {
                    c.set(3, side);
                    match_man::call(c, prepare_squad_actors, 0x10206a44);
                    chosen = c.r(3);
                } else {
                    take_manager(0x10206a54);
                    computer_pick(0x10206a88, 1);
                }
                goto picked;
            }
        }
        if (compare(c, c.i8(side + ls::kind)) != 0 && (c.set(3, club), c.call(club_check_a, 0x10206aa4),
                                                                compare(c, std::int8_t(c.r(3))) != 0)) {
            c.set({{3, c.u32(registry)}, {5, 0}, {4, c.u32(club + lineup_club::head_of_staff)}});
            c.call(registry_d, 0x10206abc);
            if (compare(c, std::int8_t(c.r(3))) != 0) {
                c.set(3, side);
                match_man::call(c, prepare_squad_actors, 0x10206acc);
                chosen = c.r(3);
            } else {
                take_manager(0x10206adc);
                computer_pick(0x10206b10, 1);
            }
        } else {
            c.set(3, side);
            match_man::call(c, prepare_squad_actors, 0x10206b20);
            chosen = c.r(3);
        }
    picked:
        // The bench size: the fixture's high nibble plus eleven.
        c.put8(bench_out, ((c.u8(fixture + lineup_fixture::bench) >> 4) & 0xf) + 0xb);
    } else {
        // Not the club's own pick: a manager that is a known human takes over, then the lineup is
        // chosen in mode 2 and the eligible players of the side are counted.
        const auto taken = c.u32(side + ls::manager);
        bool human = false;
        if (compare_logical(c, taken) != 0) {
            const auto count = c.u32(people_count_at);
            const auto id = c.u32(taken);
            c.set({{3, taken}, {4, count}, {5, id}});
            if (compare(c, std::int32_t(id), std::int32_t(count - 0x10)) >= 0 &&
                compare(c, std::int32_t(id), std::int32_t(count)) < 0)
                human = compare(c, c.i8(taken + lineup_manager::human_flag)) != 0;
        }
        compare(c, human);
        if (human) take_manager(0x10206b90);
        computer_pick(0x10206bc4, 2);
        auto record = c.u32(side + ls::players);
        std::uint32_t index = 0;
        c.put8(bench_out, 0);
        for (;; ++index, record += match_player::size) {
            const auto bench = c.u8(fixture + lineup_fixture::bench);
            const auto limit = std::int32_t(std::int8_t(((bench >> 4) & 0xf) + 0xb));
            if (compare(c, std::int32_t(std::int8_t(index)), limit) >= 0) break;
            const auto person = c.u32(record + match_player::person);
            if (compare_logical(c, person) == 0) continue;
            if (known_person(c, person)) c.put8(bench_out, c.u8(bench_out) + 1u);
        }
    }

    // The substitutes' count: what the last pick returned less eleven, within the fixture's low nibble.
    const auto bench = c.u8(fixture + lineup_fixture::bench);
    const auto taken_picks = std::int32_t(std::int8_t(chosen)) - 0xb;
    const auto available = std::int32_t(std::int8_t(taken_picks));
    const auto limit = std::int32_t(bench & 0xf);
    const auto smaller = limit < available ? limit : available;
    std::int32_t count = 0;
    if (compare(c, std::int32_t(std::int8_t(smaller))) > 0) {
        compare(c, limit, available);
        count = std::int32_t(std::int8_t(limit < available ? limit : available));
    }
    c.put8(side + 0x19, std::uint32_t(count));
    c.set({{5, side + ls::squad_text}, {3, c.toc(-0x7a6c)}, {4, c.u32(side + ls::club)}});
    c.set(3, c.u32(c.r(3)));
    c.set(6, c.u8(side + ls::kind));
    c.call(register_squad, 0x10206c94);
    return result(c, chosen);
}
}
