#include "european_cup_detail.hpp"

namespace cm::aot::european_cup {
using namespace detail;

void announce(Cpu& c, std::uint32_t address, std::uint32_t club, std::uint32_t event, std::uint32_t stage,
              std::uint32_t scope, std::uint32_t match, std::uint32_t list) {
    Frame f(c, 0x104eb290, 0x90, {.first_gpr = 17});
    keep(c, {{19, stage}, {25, c.toc(slot::definitions)}, {26, c.toc(slot::uefa_cup)}, {30, address}, {27, c.toc(slot::text)},
             {17, club}, {28, c.toc(slot::clubs)}, {18, event}, {29, c.toc(slot::pool)}, {20, scope}, {21, match}, {22, list}});
    const auto id = std::int32_t(std::int8_t(stage));
    const auto outcome = std::int32_t(std::int8_t(event));
    compare(c, id, -1);
    keep(c, {{23, c.u8(f.at(0xcb))}, {24, c.u8(f.at(0xcf))}, {31, c.u32(f.at(0xd0))}});
    const auto importance = c.r(23), personal = c.r(24), news = c.r(31);
    const auto text = c.r(27), clubs = c.r(28), definitions = c.r(25), uefa = c.r(26), pool = c.r(29);
    // The base class's generic news for this result.
    const auto forward = [&](std::uint32_t back) {
        c.put32(f.at(0x38), importance); c.put32(f.at(0x3c), personal); c.put32(f.at(0x40), news);
        c.set({{3, address}, {4, club}, {5, event}, {6, stage}, {7, scope}, {8, match}, {9, list}, {10, 0}});
        c.call(0x10450030, back);
    };
    // Generic news about entering or leaving the cup itself.
    const auto add_text = [&](std::uint32_t back) { c.set({{3, news}, {4, text}}); c.call(0x1074b830, back); };
    const auto link_club = [&](std::uint32_t back) {
        const auto record = c.u32(clubs) + c.r(17);
        c.set({{3, news}, {5, 0x7d5}, {6, record}, {4, record + 0x221}});
        c.call(0x1074bab0, back);
    };
    // "{club} ... {this competition}", naming it by one of its two name forms.
    const auto about_this_cup = [&](unsigned format, unsigned form_at, unsigned name_at, std::uint32_t back_text, std::uint32_t back_add,
                                    std::uint32_t back_club, std::uint32_t back_cup) {
        keep(c, {{17, club * club_size}});
        const auto definition = c.u32(address + 4);
        const auto form = signed_byte(c.u8(definition + form_at));
        const auto club_form = signed_byte(c.u8(c.u32(clubs) + c.r(17) + 0x23b));
        c.set({{3, text}, {4, pool + format}, {5, club_form}, {6, club_form}, {7, form}, {8, form},
               {9, c.u32(clubs) + c.r(17) + 0x221}, {10, definition + name_at}});
        c.call(0x10727890, back_text);
        add_text(back_add);
        link_club(back_club);
        const auto again = c.u32(address + 4);
        c.set({{3, news}, {5, 0x7d0}, {6, again}, {4, again + name_at}});
        c.call(0x1074bab0, back_cup);
    };
    // "{club} are entered for the {UEFA Cup}": the name comes from the definitions table.
    const auto about_uefa_cup = [&](std::uint32_t back_text, std::uint32_t back_add, std::uint32_t back_club) {
        const auto offset = c.u32(uefa) * definition_size;
        keep(c, {{17, club * club_size}});
        const auto table = c.u32(definitions);
        const auto form = signed_byte(c.u8(table + offset + 0x67));
        const auto club_form = signed_byte(c.u8(c.u32(clubs) + c.r(17) + 0x23b));
        c.set({{3, text}, {4, pool + pool_text::entered_uefa}, {5, club_form}, {6, club_form}, {7, form}, {8, form},
               {9, c.u32(clubs) + c.r(17) + 0x221}, {10, table + offset + 0x4d}});
        c.call(0x10727890, back_text);
        add_text(back_add);
        link_club(back_club);
    };
    const auto transferred = [&](std::uint32_t back) {
        c.set({{5, c.u32(address + 4)}, {3, club}, {4, scope}, {6, event}, {7, personal}, {8, news}});
        c.call(0x104698c0, back);
    };
    if (id == -1) {
        c.set(3, extended(c.i16(match + 0x30)));
        c.call(0x104648c0, 0x104eb2f0);
        bool base = compare(c, std::int16_t(c.r(3)), 0xfa) == 0;
        if (!base) {
            c.set(3, extended(c.i16(match + 0x30)));
            c.call(0x104648c0, 0x104eb304);
            base = compare(c, std::int16_t(c.r(3)), 0x104) == 0;
        }
        if (base) { forward(0x104eb340); return; }
        if (compare(c, std::int8_t(personal)) != 0) { forward(0x104eb37c); return; }
        if (compare(c, outcome, 1) == 0) {
            about_this_cup(pool_text::qualified, 0x67, 0x4d, 0x104eb3cc, 0x104eb3d8, 0x104eb3f0, 0x104eb404);
            return;
        }
        about_uefa_cup(0x104eb458, 0x104eb464, 0x104eb47c);
        const auto offset = c.u32(uefa) * definition_size;
        const auto table = c.u32(definitions);
        c.set({{3, news}, {5, 0x7d0}, {6, table + offset}, {4, table + offset + 0x4d}});
        c.call(0x1074bab0, 0x104eb49c);
        return;
    }
    if (compare(c, id, 8) < 0) {
        if (compare(c, outcome, 6) == 0) { transferred(0x104eb4d0); return; }
        if (compare(c, outcome, -2) != 0) return;
        if (compare(c, std::int8_t(personal)) != 0) {
            about_this_cup(pool_text::out_phase_1, 0x6a, 0x1a, 0x104eb524, 0x104eb530, 0x104eb548, 0x104eb55c);
            return;
        }
        about_uefa_cup(0x104eb5b0, 0x104eb5bc, 0x104eb5d4);
        const auto offset = c.u32(uefa) * definition_size;
        const auto table = c.u32(definitions);
        c.set({{3, news}, {7, table}, {5, 0x7d0}, {6, c.u32(address + 4)}, {4, table + offset + 0x4d}});
        c.call(0x1074bab0, 0x104eb5f8);
        return;
    }
    if (compare(c, id, 12) < 0) {
        if (compare(c, outcome, 6) == 0) { transferred(0x104eb62c); return; }
        if (compare(c, outcome, -2) != 0) return;
        if (compare(c, std::int8_t(personal)) != 0)
            about_this_cup(pool_text::out_phase_2, 0x6a, 0x1a, 0x104eb680, 0x104eb68c, 0x104eb6a4, 0x104eb6b8);
        else
            about_this_cup(pool_text::knocked_out, 0x67, 0x4d, 0x104eb6fc, 0x104eb708, 0x104eb720, 0x104eb734);
        return;
    }
    if (id == 12) {
        c.put32(f.at(0x38), importance); c.set(10, 0);
        c.put32(f.at(0x3c), personal); c.put32(f.at(0x40), news);
        c.call(0x10450030, 0x104eb750);
    }
}

std::uint32_t record_result(Cpu& c, std::uint32_t address, std::uint32_t club, std::uint32_t event, std::uint32_t stage,
                            std::uint32_t match, std::uint32_t list) {
    Frame f(c, 0x104eb940, 0x50, {.first_gpr = 28});
    keep(c, {{31, c.toc(slot::history)}, {30, club}, {29, address}, {28, match}});
    const auto history = c.r(31);
    const Competition cup{c, address};
    const auto id = std::int32_t(std::int8_t(stage));
    const auto outcome = std::int32_t(std::int8_t(event));
    const auto sample = [&](std::uint32_t low, std::uint32_t high, std::uint32_t back) {
        c.set(5, extended(c.i16(match + 0x34)));
        c.set({{3, low}, {4, high}, {6, 2}});
        c.call(0x10476780, back);
        c.set(8, c.r(3));
    };
    const auto left_cup = [&] { c.put32(club + 0x14, 0xffffffff); };
    if (compare(c, id, -1) == 0) {
        bool won = compare(c, outcome) == 0;
        if (!won) won = compare(c, outcome, 1) == 0;
        if (won) {
            if (compare(c, c.i16(match + 0x34)) == 0) {
                sample(1, 0x14, 0x104eb9a8);
                const auto rounds = cup.rounds();
                c.set({{3, c.u32(history)}, {4, club}, {5, cup.definition()}, {6, extended(cup.extra_rounds())},
                       {7, extended(c.i16(rounds + 0x80))}});
                c.call(0x10384500, 0x104eb9c8);
            } else {
                sample(1, 0x14, 0x104eb9dc);
                c.set({{3, c.u32(history)}, {5, cup.definition()}, {4, club}, {6, 0x41c}, {7, 0}});
                c.call(0x10384500, 0x104eb9f8);
            }
        } else if (compare(c, outcome, -1) == 0 || compare(c, outcome, 2) == 0) {
            sample(1, 0x14, 0x104eba20);
            c.set({{3, c.u32(history)}, {5, cup.definition()}, {4, club}, {6, extended(c.i16(match + 0x32))}, {7, extended(c.i16(match + 0x30))}});
            c.call(0x103846e0, 0x104eba3c);
            left_cup();
        }
        return 0;
    }
    if (compare(c, id, 8) < 0) {
        if (compare(c, outcome, 6) == 0) {
            c.set({{3, c.u32(history)}, {6, 0x44a}, {5, cup.definition()}, {7, 0xffffffff}, {8, 0x32}});
            c.call(0x10384500, 0x104eba74);
        } else if (compare(c, outcome, -2) == 0) {
            c.set({{3, c.u32(history)}, {6, 0x449}, {5, cup.definition()}, {7, 0x32}});
            c.call(0x10384820, 0x104eba94);
            left_cup();
        }
        return 0;
    }
    if (compare(c, id, 12) < 0) {
        if (compare(c, outcome, 6) == 0) {
            c.set({{3, c.u32(history)}, {6, 0}, {5, cup.definition()}, {7, 0x78}, {8, 0x32}});
            c.call(0x10384500, 0x104ebacc);
        } else if (compare(c, outcome, -2) == 0) {
            c.set({{3, c.u32(history)}, {6, 0x44a}, {5, cup.definition()}, {7, 0x32}});
            c.call(0x10384820, 0x104ebaec);
            left_cup();
        }
        return 0;
    }
    if (id != 12) return 0;
    c.set(3, std::uint32_t(outcome));
    if (compare(c, outcome, 1) == 0) {
        c.set({{3, c.u32(history)}, {5, match}, {6, list}});
        c.call(0x10383410, 0x104ebb18);
        left_cup();
    } else if (compare(c, outcome, 2) == 0) {
        c.set({{3, c.u32(history)}, {5, match}, {6, list}});
        c.call(0x10383b70, 0x104ebb3c);
        left_cup();
    } else if (compare(c, outcome) == 0) {
        sample(0x4b, 0x64, 0x104ebb64);
        const auto last_stage = c.u32(cup.stages() + 0x30);
        const auto rounds = c.u32(last_stage + 0xa0);
        const auto label = c.i16(rounds + std::uint32_t(std::int32_t(c.i16(match + 0x34))) * round_size + 0x80);
        c.set({{4, club}, {6, 0}, {3, c.u32(history)}, {5, cup.definition()}, {7, extended(label)}});
        c.call(0x10384500, 0x104ebb98);
    } else if (compare(c, outcome, -1) == 0) {
        sample(0x4b, 0x64, 0x104ebbb8);
        c.set({{3, c.u32(history)}, {5, cup.definition()}, {4, club}, {7, extended(c.i16(match + 0x30))}, {6, 0}});
        c.call(0x103846e0, 0x104ebbd4);
        left_cup();
    }
    return 0;
}

std::uint32_t entered_clubs(Cpu& c, std::uint32_t address, std::uint32_t out, std::uint32_t selector) {
    Frame f(c, 0x104ee7f0, 0xd0, {.first_gpr = 26});
    keep(c, {{31, c.toc(slot::flag)}, {26, c.toc(slot::message)}, {27, c.toc(slot::pool)}, {28, address}, {29, out}, {30, 0}});
    const Competition cup{c, address};
    Program p{c};
    const auto id = std::int32_t(std::int8_t(selector));
    // The clubs of this competition's zone; the first form leaves out the
    // clubs entering at route 7 (the champions' direct seats).
    const auto scan = [&](std::uint32_t back, std::uint32_t at, unsigned name, unsigned extension, unsigned line, bool routes) {
        const auto list = p.call(0x10775130, back, {0x11c});
        if (compare_logical(c, list) == 0) {
            assertion(c, f, at, name, extension, line);
            c.put32(out, 0);
            return false;
        }
        const unsigned offset_register = routes ? 5 : 6;
        std::uint32_t i = 0, offset = 0;
        c.set({{8, 0}, {offset_register, 0}});
        for (;;) {
            c.set(4, extended_half(i));
            if (compare(c, std::int32_t(c.r(4)), cup.entrant_scan()) >= 0) break;
            const auto entry = cup.entrants() + offset;
            const auto entrant = c.u32(entry);
            const auto definition = cup.definition();
            const auto zone = c.u32(definition);
            c.set({{4, zone}, {7, entrant}, {routes ? 6u : 5u, routes ? entry : definition}});
            if (compare(c, std::int32_t(zone), c.i32(entrant + 0x14)) == 0 &&
                (!routes || compare(c, std::int32_t(c.u8(entry + 4)), 7) != 0)) {
                c.put32(list + (extended_half(c.r(30)) << 2), entrant);
                c.set(30, c.r(30) + 1);
            }
            offset += 8; ++i;
            c.set({{8, i}, {offset_register, offset}});
        }
        c.put32(out, list);
        return true;
    };
    bool done = false;
    if (compare(c, id, 1) == 0) done = scan(0x104ee82c, 0x104ee848, 0x90, 0x48, 0x7b5, true);
    else if (compare(c, id, 2) == 0) done = scan(0x104ee904, 0x104ee920, 0x70, 0x40, 0x7cf, false);
    else if (compare(c, id, 3) == 0 && compare(c, cup.last_stage(), 11) == 0) done = scan(0x104ee9dc, 0x104ee9f8, 0x50, 0x38, 0x7e9, false);
    else {
        c.set({{3, address}, {4, out}, {5, 0}});
        c.call(0x10451d10, 0x104eeaa4);
        return c.r(3);
    }
    return done ? c.r(30) : 0;
}

void seed_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104eead0, 0xc0, {.first_gpr = 22});
    keep(c, {{30, c.toc(slot::flag)}, {24, address}, {25, c.toc(slot::message)}, {23, c.toc(slot::pool)}});
    Kept<28> dealt(c, 0);
    Kept<27> band_seed(c, 1);
    Kept<26> next_seed(c, 1);
    const Competition cup{c, address};
    Program p{c};
    if (compare_logical(c, cup.fixtures()) == 0) { assertion(c, f, 0x104eeb1c, 0x6c, 0x44, 0x81b); return; }
    Kept<29> order(c, p.call(0x10775130, 0x104eeb68, {std::uint32_t(std::int32_t(cup.entrant_count())) << 2}));
    if (compare(c, std::int32_t(std::uint32_t(order))) == 0) { assertion(c, f, 0x104eeb84, 0x4c, 0x3c, 0x823); return; }
    std::uint32_t copied = 0;
    for (;;) {
        c.set(4, extended(cup.entrant_count()));
        if (compare(c, std::int16_t(copied), std::int32_t(c.r(4))) >= 0) break;
        c.set(4, cup.entrants());
        c.put32(order + copied * 4, c.u32(c.r(4) + copied * 8));
        ++copied;
    }
    c.set({{3, order}, {5, copied * 8}, {6, copied}});
    c.call(0x10477b40, 0x104eec04);
    c.set({{3, address}, {5, f.at(0x3a)}, {6, f.at(0x38)}, {4, 0xc}, {7, 0}});
    Kept<25> schedule(c, p.virtual_call(address, 0x44, 0x104eec24));
    // Each club takes the next seed number; a band of clubs shares its first number.
    const auto assign = [&](std::uint32_t value, std::uint32_t back) {
        const auto at = extended_half(dealt);
        c.set({{3, cup.fixtures()}, {5, value}, {4, c.u32(order + (at << 2))}});
        dealt = dealt + 1;
        c.call(0x10457490, back);
        next_seed = next_seed + 1;
    };
    const auto deal_round = [&](std::uint32_t entrants_at, std::uint32_t value, std::uint32_t back) {
        for (Kept<22> seat(c, 0);; seat = seat + 1) {
            c.set(3, extended_half(seat));
            if (compare(c, std::int32_t(c.r(3)), c.i16(entrants_at)) >= 0) break;
            assign(value, back);
        }
    };
    // The calendar's rounds, last first; the last round's clubs are dealt twice.
    const auto rounds_count = c.i16(f.at(0x3a));
    c.set(4, extended(rounds_count));
    Kept<30> round(c, extended_half(extended(rounds_count) - 1));
    Kept<31> round_offset(c, round * round_size);
    while (compare(c, std::int16_t(std::uint32_t(round))) >= 0) {
        c.set(3, extended(c.i16(f.at(0x3a))));
        c.set(4, extended_half(round));
        if (compare(c, std::int32_t(c.r(4)), std::int32_t(c.r(3)) - 1) == 0) {
            Kept<23> entrants_at(c, round_offset + 0x1c);
            deal_round(schedule + entrants_at, band_seed, 0x104eec7c);
        }
        band_seed = next_seed;
        Kept<23> entrants_at(c, round_offset + 0x1c);
        deal_round(schedule + entrants_at, band_seed, 0x104eecc0);
        round_offset = round_offset - round_size;
        round = round - 1;
    }
    c.set(3, schedule);
    c.call(0x10775170, 0x104eecf0);
    // The two group phases seed in blocks of four, then of eight.
    const auto deal_blocks = [&](std::int32_t size, std::uint32_t back) {
        for (Kept<22> block(c, 2); compare(c, std::int8_t(std::uint32_t(block)), 4) < 0; block = block + 1) {
            Kept<23> block_seed(c, next_seed);
            for (Kept<25> seat(c, 0); compare(c, std::int8_t(std::uint32_t(seat)), size) < 0; seat = seat + 1) assign(block_seed, back);
        }
    };
    deal_blocks(4, 0x104eed20);
    deal_blocks(8, 0x104eed74);
    // The qualifying rounds, last first.
    c.set(3, extended(cup.round_count()));
    Kept<27> qualifying(c, extended_half(c.r(3) - 1));
    Kept<23> qualifying_offset(c, qualifying * round_size);
    while (compare(c, std::int16_t(std::uint32_t(qualifying))) >= 0) {
        Kept<25> round_seed(c, next_seed);
        for (Kept<30> seat(c, 0);; seat = seat + 1) {
            c.set(3, cup.rounds());
            c.set(4, extended_half(seat));
            if (compare(c, std::int32_t(c.r(4)), c.i16(qualifying_offset + c.r(3) + 0x1c)) >= 0) break;
            assign(round_seed, 0x104eedd4);
        }
        qualifying_offset = qualifying_offset - round_size;
        qualifying = qualifying - 1;
    }
    p.call(0x10775170, 0x104eee0c, {order});
}

// A comparator in 0x10768300's calling convention: clubs by qualifying tier,
// then descending reputation. CR0 keeps the tier difference's compare.
void compare_seeds(Cpu& c) {
    const auto b = c.u32(c.r(4)), a = c.u32(c.r(3));
    c.set({{5, b}, {4, a}});
    const Club first{c, a}, second{c, b};
    const auto tier = std::int32_t(first.tier()) - std::int32_t(second.tier());
    if (compare(c, tier) != 0) { c.set(3, std::uint32_t(tier)); return; }
    c.set(3, std::uint32_t(std::int32_t(second.reputation()) - std::int32_t(first.reputation())));
}

void stage_name(Cpu& c, std::uint32_t out, std::uint32_t size, std::uint32_t first, std::uint32_t second, std::uint32_t kind, std::uint32_t record) {
    Frame f(c, 0x104ef130, 0x80, {.first_gpr = 24});
    keep(c, {{24, size}, {29, c.toc(slot::message)}, {30, c.toc(slot::pool)}, {31, out}, {25, first}, {26, second}, {27, kind}, {28, record}});
    const auto pool = c.r(30);
    if (compare(c, std::int32_t(size), 0x1e) < 0) {
        assertion(c, f, 0x104ef17c, 0x40, 0x38, 0x950);
        c.set(3, c.toc(slot::flag));
        return;
    }
    keep(c, {{29, extended(std::int16_t(first))}});
    const auto id = c.r(29);
    const auto format = [&](unsigned text, std::uint32_t back) {
        c.set({{3, out}, {4, pool + text}});
        c.call(0x10727890, back);
    };
    const auto phase = [&](unsigned qualified, unsigned plain, std::uint32_t qualified_back, std::uint32_t plain_back) {
        const auto k = std::uint8_t(kind);
        bool qualifying = compare(c, std::int8_t(kind)) == 0;
        if (!qualifying) qualifying = compare_logical(c, std::uint8_t(k - 6), 1) <= 0;
        if (!qualifying) qualifying = compare(c, std::int8_t(kind), 8) == 0;
        if (qualifying) {
            c.set({{3, out}, {4, pool + qualified}, {6, pool + pool_text::empty}, {5, signed_byte(c.u8(record + 0x242))}});
            c.call(0x10727890, qualified_back);
        } else format(plain, plain_back);
    };
    c.set(3, id);
    c.call(0x10464010, 0x104ef1cc);
    if (compare(c, std::int32_t(c.r(3)), 0x449) == 0) {
        phase(pool_text::qualified_phase_1, pool_text::phase_1, 0x104ef210, 0x104ef220);
        return;
    }
    c.set(3, id);
    c.call(0x10464010, 0x104ef22c);
    if (compare(c, std::int32_t(c.r(3)), 0x44a) == 0) {
        phase(pool_text::qualified_phase_2, pool_text::phase_2, 0x104ef270, 0x104ef280);
        return;
    }
    const auto kind_id = signed_byte(kind);
    c.set(3, kind_id);
    if (compare(c, std::int32_t(kind_id), 1) == 0) { format(pool_text::winners, 0x104ef29c); return; }
    if (compare(c, std::int16_t(first)) == 0 && compare(c, std::int16_t(second)) == 0) {
        if (compare(c, std::int32_t(kind_id), 2) == 0) { format(pool_text::runners_up, 0x104ef2c4); return; }
        if (compare(c, std::int32_t(kind_id), 3) == 0) { format(pool_text::third, 0x104ef2dc); return; }
    }
    c.set({{3, id}, {5, out}, {6, size}, {4, extended(std::int16_t(second))}, {7, 0}});
    c.call(0x10476f40, 0x104ef2f8);
}

// A comparator for the second draw: clubs of the most represented nation
// first, then the higher nation coefficient, then winners before runners-up.
void compare_draw_order(Cpu& c) {
    const auto a = c.r(3), b = c.r(4);
    const auto nation_count = std::int32_t(c.i8(b + 4)) - std::int32_t(c.i8(a + 4));
    c.set(5, signed_byte(c.u8(a + 4)));
    if (compare(c, nation_count) != 0) { c.set(3, std::uint32_t(nation_count)); return; }
    const std::uint32_t first = Club{c, c.u32(a)}.nation(), second = Club{c, c.u32(b)}.nation();
    const auto own = extended(c.i16(first + 0xac));
    c.set({{6, own}, {5, second}});
    const auto coefficient = c.i16(second + 0xac) - std::int32_t(own);
    if (compare(c, coefficient) != 0) { c.set(3, std::uint32_t(coefficient)); return; }
    const auto second_place = c.u8(b + 6);
    c.set(4, second_place);
    c.set(3, std::uint32_t(std::int32_t(c.i8(a + 6))) - signed_byte(second_place));
}
}
