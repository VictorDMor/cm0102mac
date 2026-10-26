#include "shortlist_manager_candidates_scan.hpp"

// The candidate scan (0x1035e9f0): for one of a club's shortlist rows, walk
// the shortlist manager's candidate lists for the row's categories, score
// each eligible person and keep the best; then link the pick to the club
// and register the club's interest, or, finding none, run a relaxed second
// pass (row flag bit 7) or mark the row as unfilled. This file holds the
// set-up, the walk and the conclusion; the per-candidate filters and the
// valuation are in shortlist_manager_candidates_filter.cpp and _value.cpp.
namespace cm::aot::shortlist_manager {
namespace candidates {
namespace {
// Whether day/year a is before b, as the original's inline compare: years as
// uint16 first, then days as int16 (its branchless signed less-than).
bool before(Cpu& c, std::int16_t a_day, std::uint16_t a_year, std::int16_t b_day, std::uint16_t b_year) {
    const auto by_year = compare_logical(c, a_year, b_year);
    if (by_year != 0) return by_year < 0;
    return a_day < b_day;
}
}

std::int32_t Scan::random_below(std::int32_t base, Copy copy, std::uint32_t rand_back, std::uint32_t division,
                                std::uint32_t call_back) {
    const DrawSite site{rand_back, division, call_back, at(base), at(base + 8), at(base + 0x10)};
    return draw_below(c, fpu, random(), site, r13.value(), 14, [&](const WideDraw& d) {
        if (copy == Copy::a) d.leave_a(c);
        else d.leave_b(c);
    });
}

std::uint32_t Scan::contract_club(std::uint32_t person_address, std::uint32_t back) {
    const auto contract = program.call(0x10082f20, back, {c.u32(contracts_slot), person_address});
    const auto global = c.toc(slot::current_contract);
    c.set(4, global);
    const bool found = compare_logical(c, contract) != 0;
    c.put32(global, contract);
    return found ? club_of(contract) : 0;
}

// 0x11a stays set only for a player row when today lies after the region's
// first calendar date and before its second, each 28 days early (inferred:
// the transfer window, with four weeks' notice).
void Scan::check_window(std::uint32_t region) {
    if (compare(c, row.kind(), 11) != 0) {
        c.put8(at(local::in_window), 0);
        return;
    }
    const std::int32_t index = std::int8_t(region);
    if (compare(c, index, 0) < 0 || compare(c, index, 0x22) >= 0) return;
    r14 = c.toc(slot::calendars) + std::uint32_t(index) * 0x4c;
    program.call(0x10717b20, 0x1035eabc, {at(local::window_start_result), r14 + 0x2c, 0x1c});
    const auto today = c.toc(slot::today);
    c.put32(at(local::window_start), c.u32(at(local::window_start_result)));
    r18 = today + 2;
    c.put32(at(local::window_start) + 4, c.u32(at(local::window_start_result) + 4));
    const std::int16_t start_day = c.i16(at(local::window_start)), today_day = c.i16(today);
    const std::uint16_t start_year = c.u16(at(local::window_start) + 2), today_year = c.u16(today + 2);
    c.set(4, start_year == today_year ? extended(today_day) : start_year);
    if (!nonzero(c, before(c, start_day, start_year, today_day, today_year))) {
        c.put8(at(local::in_window), 0);
        return;
    }
    program.call(0x10717b20, 0x1035eb34, {at(local::window_end_result), r14 + 0x3c, 0x1c});
    c.put32(at(local::window_end), c.u32(at(local::window_end_result)));
    c.put32(at(local::window_end) + 4, c.u32(at(local::window_end_result) + 4));
    const std::uint16_t now_year = c.u16(r18), end_year = c.u16(at(local::window_end) + 2);
    // The years compared the other way round here (cmplw end, today).
    const auto by_year = compare_logical(c, end_year, now_year);
    const std::int16_t end_day = c.i16(at(local::window_end)), now_day = c.i16(today);
    c.set(4, by_year != 0 ? end_year : extended(end_day));
    if (!nonzero(c, by_year != 0 ? by_year > 0 : now_day < end_day)) c.put8(at(local::in_window), 0);
}

// The sampling divisor (int16 at 0x118): a club takes a full look at one
// candidate in `divisor` (by person id plus club id). Clubs below 3500
// reputation start from 24100 - 4 x reputation, the others from 100 (50 in
// the regional and small-divisor nations); each of the seven staff slots
// adds 30 - (2 x attribute 6 + attribute 5) of its member, an empty slot 10
// with a competition (5 without); then a tenth of that, at least 2.
void Scan::set_divisor() {
    const auto divisor = at(local::divisor);
    const std::int32_t reputation = club.reputation();
    if (compare(c, reputation, 0xdac) < 0) {
        c.put16(at(local::divisor), std::uint32_t(std::int16_t(24000 - std::uint32_t(reputation) * 4) + 100));
    } else {
        const std::int32_t nation = c.i32(club.nation() + 0x68);
        c.set(4, std::uint32_t(nation));
        for (const auto nation_slot : {slot::regional_nation_4, slot::small_divisor_nation, slot::regional_nation_1,
                                       slot::regional_nation_3, slot::regional_nation_2}) {
            if (compare(c, nation, c.i32(c.toc(nation_slot))) == 0) {
                c.put16(divisor, 50);
                break;
            }
        }
    }
    r18 = 0;
    r14 = 0;
    for (;;) {
        r20 = r14 + 0x174;
        const auto member = c.u32(club_address + r20);
        if (nonzero_logical(c, member)) {
            const auto record = c.u32(member + 0x1c);
            if (nonzero_logical(c, record)) {
                const std::int8_t first = std::int8_t(program.call(0x100d2f50, 0x1035ec4c, {record, 6}));
                const auto again = c.u32(club_address + r20);
                r20 = extended(std::int16_t(std::uint32_t(std::int32_t(first)) << 1));
                c.set(5, again);
                const std::int8_t second = std::int8_t(program.call(0x100d2f50, 0x1035ec68, {c.u32(again + 0x1c), 5}));
                r20 = r20 + std::uint32_t(std::int32_t(second));
                const auto share = std::int32_t(std::int16_t(30 - r20));
                c.put16(divisor, std::uint32_t(c.i16(divisor) + share));
            }
        } else {
            const auto competition = club.division().get();
            std::int32_t share = 5;
            if (nonzero_logical(c, competition)) {
                const auto table = c.u32(r13);
                if (nonzero_logical(c, c.u32(table + c.u32(competition) * 4))) share = 10;
            }
            c.put16(divisor, std::uint32_t(c.i16(divisor) + share));
        }
        r18 = r18 + 1;
        r14 = r14 + 4;
        if (compare(c, r18.value(), 7) >= 0) break;
    }
    c.put16(divisor, extended(std::int16_t(c.i16(divisor) / 10)));
    if (compare(c, c.i16(divisor), 2) < 0) c.put16(divisor, 2);
}

// The early refusals: a club in human hands (0x1014c2e0 mode 0) never scans;
// in mode 1, 0x1014c5f0's answer (r19) and the human table's view (0x10149df0
// kind 4 answering 1) can stop it; a linked row (+0x18) and 0x100c7d00 stop
// it too.
bool Scan::open() {
    if (human(club_address, 0, 0x1035ed28)) return false;
    if (human(club_address, 1, 0x1035ed48)) {
        player_row = program.call(0x1014c5f0, 0x1035ed58, {club_address});
        const auto manager_record = program.call(0x100c96e0, 0x1035ed68, {club_address});
        if (nonzero(c, std::int8_t(program.call(0x10147e70, 0x1035ed78, {c.u32(c.toc(slot::humans)), manager_record})))) {
            const auto again = program.call(0x100c96e0, 0x1035ed88, {club_address});
            const auto answer = std::int8_t(program.call(0x10149df0, 0x1035ed9c, {c.u32(c.toc(slot::humans)), 4, again}));
            if (compare(c, answer, 1) == 0) return false;
        }
    }
    if (compare(c, row.approach(), -1) != 0) return false;
    return !nonzero(c, std::int8_t(program.call(0x100c7d00, 0x1035edbc, {club_address})));
}

// The minimum score (0x110, then r24) and the club's reputation scale (f31):
// reputation over 1050-1250 by the club's finance class (0x100f55b0 of its
// finance record, for clubs of five nations), at least 1. A row asking for
// one person scores him first (0x10216830 with the mode's flags), capped by
// his contract with the club unless that contract has a year or more to
// run, and takes 1.05 x that when it is above the row's minimum. False when
// such a row is not a player row (the scan stops).
bool Scan::set_minimum() {
    const std::uint8_t flags = row.flags();
    c.put32(at(local::minimum), std::uint32_t(row.level()));
    c.put32(at(local::chosen), 0);
    c.put32(at(local::limit), program.call(0x1036f6f0, 0x1035edf0, {manager, club_address, extended(row.positions()),
                                                                     std::uint32_t(std::int32_t(std::int8_t(Row::category(flags))))}));
    const auto finances_slot = c.toc(slot::finances);
    const auto finances = c.u32(finances_slot);
    c.set(4, finances_slot);
    c.set(5, finances);
    double scale = k::reputation_1250;
    if (nonzero_logical(c, finances)) {
        const auto nation = club.nation().get();
        if (nonzero_logical(c, nation)) {
            const std::int32_t id = c.i32(nation + 0x68);
            c.set(4, std::uint32_t(id));
            bool classed = false;
            for (const auto nation_slot : {slot::finance_nation_1, slot::finance_nation_2, slot::finance_nation_3,
                                           slot::finance_nation_4, slot::finance_nation_5}) {
                if (compare(c, id, c.i32(c.toc(nation_slot))) == 0) {
                    classed = true;
                    break;
                }
            }
            if (classed) {
                program.call(0x10103510, 0x1035ee70, {finances, club_address});
                c.call(0x100f55b0, 0x1035ee74);
                const std::int32_t finance_class = std::int8_t(c.r(3));
                const std::int32_t reputation = club.reputation();
                const auto order = compare(c, finance_class, 2);
                if (order == 0)
                    scale = compare(c, reputation, 0x128e) > 0 ? k::reputation_1050 : k::reputation_1125;
                else if (order < 0 && compare(c, finance_class, 1) >= 0)
                    scale = compare(c, reputation, 0x186a) > 0 ? k::reputation_1125 : k::reputation_1175;
            }
        }
    }
    // f1 is left as the original leaves it (a returning operation's caller sees it).
    const double reputation_value = integer(0x104, club.reputation());
    c.set_f(1, reputation_value);
    const double ratio = fpu.fdiv(reputation_value, scale);
    f31 = k::one;
    if (fcompare(ratio, k::one) & 4) f31 = ratio;

    const std::int32_t target = row.previous();
    if (compare(c, target, -1) <= 0) return true;
    const std::uint32_t offset = std::uint32_t(target) * Person::size;
    c.set(4, offset);
    r14 = c.u32(c.toc(slot::persons)) + offset;
    if (compare(c, row.kind(), 11) != 0) return false;
    const auto mode = Row::category(row.flags());
    const auto order = compare(c, std::int32_t(mode), 4);
    if (order == 0) {
        r13 = 0x19c2;
    } else if (order < 0 && compare(c, std::int32_t(mode), 2) >= 0) {
        r13 = 0x18c2;
        if (human(club_address, 1, 0x1035f03c)) r13 = r13 | 0x810000;
    } else {
        r13 = 0x1902;
    }
    r13 = program.call(0x10216830, 0x1035f074, {r14, extended(row.positions()), r13, club_address, std::uint32_t(-1), player_row});
    const auto contract = program.call(0x10082f90, 0x1035f08c, {c.u32(contracts_slot), r14, club_address});
    bool capped = true;
    if (nonzero_logical(c, contract)) {
        const std::uint32_t terms = c.u32(contract + 0x50);
        c.set(4, terms);
        if (!nonzero(c, std::int32_t(terms))) {
            capped = false;
        } else if (!nonzero(c, std::int32_t(terms & 0x100)) && !nonzero(c, std::int32_t(terms & 1))) {
            c.call(0x10374d10, 0x1035f0b4);
            const std::int32_t days = std::int16_t(c.r(3));
            if (compare(c, days, 365) >= 0) capped = false;
        }
    }
    const std::int32_t minimum = c.i32(at(local::minimum));
    if (capped && compare(c, r13.value(), minimum) >= 0) r13 = std::uint32_t(minimum);
    const double estimate = integer(0xfc, r13.value());
    const double wanted = fpu.fmul(k::one_point_oh_five, estimate);
    const double floor = integer(0x104, minimum);
    double taken = wanted;
    if (fcompare(floor, wanted) & 4) taken = integer(0xf4, minimum);
    c.set_f(1, taken);
    c.put32(at(local::minimum), std::uint32_t(truncated(0xec, taken)));
    return true;
}

// The categories the row's position covers: 0x10359980 gives the last for a
// player row (15 otherwise); 17-21 stand for ranges of their own.
void Scan::set_categories() {
    std::uint32_t last;
    if (compare(c, row.kind(), 11) != 0) {
        last = 15;
    } else {
        last = program.call(0x10359980, 0x1035f15c, {manager, extended(row.positions())});
    }
    r13 = last;
    const std::int32_t budget = row.budget();
    c.put32(at(local::fee_floor), std::uint32_t(compare(c, budget, 1) > 0 ? budget : 1));
    const std::uint8_t flags = row.flags();
    const std::int32_t kind = std::int8_t(row.kind());
    c.set(4, std::uint32_t(kind));
    player_row = kind == 11 ? 1 : 0;
    bool relaxed_player = false;
    if (nonzero(c, std::int32_t(Row::loan(flags)))) relaxed_player = compare(c, kind, 11) == 0;
    r14 = relaxed_player ? 1 : 0;

    const std::uint32_t excluded = c.u32(at(local::excluded));
    if (nonzero_logical(c, excluded) &&
        nonzero(c, std::int8_t(program.call(0x10362d70, 0x1035f1d8, {manager, excluded, club_address})))) {
        program.call(0x10365590, 0x1035f200, {manager, c.u32(at(local::excluded)), club_address, 3,
                                              std::uint32_t(player_row == 0)});
    }
    std::uint32_t first = r13, final_category = r13;
    const std::int32_t category = std::int8_t(r13);
    if (compare(c, category, 0x11) == 0) first = 1, final_category = 4;
    else if (compare(c, category, 0x12) == 0) first = 8, final_category = 0xd;
    else if (compare(c, category, 0x13) == 0) first = 5, final_category = 7;
    else if (compare(c, category, 0x15) == 0) first = 0xb, final_category = 0xd;
    else if (compare(c, category, 0x14) == 0) first = 0xe, final_category = 0xe;
    const auto first_category = std::uint32_t(std::int32_t(std::int8_t(first)));
    c.put8(at(local::category), first);
    c.put32(at(local::last_category), std::uint32_t(std::int32_t(std::int8_t(final_category))));
    c.put32(at(local::category_offset), first_category << 2);
    c.put32(at(local::lists_offset), std::uint32_t(r14) << 6);
    leave(c, {{4, first_category}, {5, final_category}});
}

// The walk: each category's list (manager +0x18, the relaxed pass's from
// +0x58) holds 12-byte entries, the first person id -1 ending the walk;
// the counts are at manager +0x98.
void Scan::walk() {
    for (;;) {
        const std::int32_t category = std::int8_t(c.u8(at(local::category)));
        if (compare(c, category, c.i32(at(local::last_category))) > 0) return;
        const std::uint32_t offset = c.u32(at(local::category_offset));
        const std::uint32_t list = c.u32(manager + c.u32(at(local::lists_offset)) + offset + 0x18);
        c.put32(at(local::list), list);
        if (compare(c, c.i32(list), -1) == 0) return;
        const std::int32_t count = c.i32(manager + offset + 0x98);
        c.put32(at(local::index), 0);
        c.put32(at(local::count), std::uint32_t(count));
        c.put32(at(local::entry_offset), 0);
        std::uint32_t entry_offset = 0;
        for (std::int32_t index = 0; compare(c, index, c.i32(at(local::count))) < 0;) {
            consider(c.u32(at(local::list)) + entry_offset);
            entry_offset = c.u32(at(local::entry_offset)) + 12;
            c.put32(at(local::entry_offset), entry_offset);
            index = c.i32(at(local::index)) + 1;
            c.put32(at(local::index), std::uint32_t(index));
        }
        c.put32(at(local::category_offset), offset + 4);
        c.put8(at(local::category), c.u8(at(local::category)) + 1);
    }
}

// The pick: link it to the club (0x103632f0), and for a first-pass player
// row, now and then (one in 37 - ability/300 draws) register the club's
// interest at once: unless his ability exceeds 5000 plus a draw below 2000,
// his club must be human; then the offer is 0x10089a40's (his contract's
// club human) or 0x10085860's price, capped by 0x1036fbf0, and the interest
// goes to 0x10698bb0 with 0x10369080's weight.
void Scan::take(std::uint32_t chosen) {
    row.target() = c.i32(chosen);
    program.call(0x103632f0, 0x10360d7c, {manager, c.u32(at(local::chosen)), club_address});
    if (nonzero(c, std::int32_t(Row::loan(row.flags())))) return;
    if (!nonzero(c, std::int8_t(player_row))) return;
    const auto profile = c.u32(c.u32(at(local::chosen)) + 0x14);
    c.set(4, profile);
    r13 = std::uint32_t(37 - Player{c, profile}.reputation() / 300);
    random_below(0xdc, Copy::b, 0x10360de8, 0x10360ef8, 0x10360eb0);
    if (compare(c, r14.value(), 0) != 0) return;
    advance(0x10360f38);
    const auto pick = c.u32(at(local::chosen));
    const auto pick_profile = c.u32(pick + 0x14);
    const std::int32_t value = draw();
    const std::int32_t ability = Player{c, pick_profile}.reputation();
    leave(c, {{4, std::uint32_t(value)}, {5, std::uint32_t(ability)}, {6, pick_profile}});
    if (compare(c, ability, value % 2000 + 5000) <= 0) {
        const auto pick_club = c.u32(c.u32(at(local::chosen)) + 0x24);
        if (!nonzero_logical(c, pick_club)) return;
        if (!human(pick_club, 0, 0x10360fac)) return;
    }
    r13 = 0;
    const auto contract = program.call(0x10082f20, 0x10360fc4, {c.u32(contracts_slot), c.u32(at(local::chosen))});
    const auto global = c.toc(slot::current_contract);
    c.set(4, global);
    const bool found = compare_logical(c, contract) != 0;
    c.put32(global, contract);
    if (found) {
        const std::int32_t standing = std::int8_t(std::int32_t(std::uint32_t(c.u8(contract + 0x38)) << 24 & 0xfe000000) >> 26);
        if (compare(c, standing, 5) != 0) r13 = 1;
    }
    if (compare(c, r13.value(), 0) == 0) {
        r14 = 0;
    } else {
        const auto current = c.u32(c.toc(slot::current_contract));
        c.set(4, current);
        r14 = club_of(current);
    }
    const auto contract_again = program.call(0x10082f20, 0x10361028, {c.u32(contracts_slot), c.u32(at(local::chosen))});
    r13 = program.call(0x1036fbf0, 0x10361040, {manager, row_address, contract_again, club_address, at(local::offer_cap)});
    if (compare_logical(c, r14) == 0 || nonzero(c, std::int8_t(r13))) {
        r14 = 0;
    } else if (human(r14, 0, 0x10361074)) {
        r14 = program.call(0x10089a40, 0x10361090, {c.u32(contracts_slot), r14, club_address, c.u32(at(local::chosen))});
    } else {
        r14 = program.call(0x10085860, 0x103610b8, {c.u32(contracts_slot), r14, club_address, c.u32(at(local::chosen)), 0, 1, 1});
    }
    const std::int32_t cap = c.i32(at(local::offer_cap));
    if (nonzero(c, cap) && nonzero(c, std::int8_t(r13)) && compare(c, cap, r14.value()) < 0) r14 = std::uint32_t(cap);
    const std::uint8_t flags = row.flags();
    program.call(0x10369080, 0x10361110, {manager, c.u32(at(local::chosen)), club_address, 1,
                                          std::uint32_t(std::int32_t(std::int8_t(Row::loan(flags)))),
                                          std::uint32_t(std::int32_t(std::int8_t(Row::category(flags)))), 0});
    program.call(0x10698bb0, 0x10361128, {c.u32(c.toc(slot::interest)), c.u32(at(local::chosen)), club_address, r14});
}

// No pick: a first-pass row in the window with nothing found tries the
// relaxed pass when the club can afford it, or now and then anyway; else
// the row is marked unfilled (+0x10 = -1).
void Scan::conclude_empty() {
    const auto unfilled = [&] { row.target() = -1; };
    if (!nonzero(c, std::int8_t(c.u8(at(local::in_window))))) return unfilled();
    const std::uint8_t flags = row.flags();
    if (nonzero(c, std::int32_t(Row::loan(flags)))) return unfilled();
    if (compare(c, row.target(), -1) != 0) return unfilled();
    const std::uint32_t mode = Row::category(flags);
    if (compare_logical(c, mode, 2) != 0 && compare_logical(c, mode, 3) != 0) return unfilled();

    // The wage ceiling against the minimum x (0.6 + reputation^2 / 2.5e8).
    const std::int32_t reputation = club.reputation();
    const std::int32_t ceiling_value = row.weight();
    const double share = fpu.fdiv(integer(0xf4, reputation), k::quarter_billion);
    const double reputation_value = integer(0xec, reputation);
    const double minimum = integer(0xe4, row.level());
    const double scaled = fpu.fmul(reputation_value, share);
    const double ceiling = integer(0xdc, ceiling_value);
    c.set_f(1, ceiling);
    leave(c, {{4, 0x43300000}, {5, signed_word(ceiling_value)}});
    const double limit = fpu.fmul(minimum, fpu.fadd(k::six_tenths, scaled));
    bool relax = (fcompare(ceiling, limit) & 4) != 0;
    if (!relax) {
        const std::int32_t budget = row.budget();
        c.set(4, std::uint32_t(budget));
        if (compare(c, budget, 50000) < 0) {
            relax = true;
        } else {
            const auto nation = club.nation().get();
            c.set(4, nation);
            if (compare(c, c.i32(nation + 0x68), c.i32(c.toc(slot::finance_nation_1))) != 0) return unfilled();
            advance(0x10361230);
            if (!nonzero(c, draw() % 2)) return unfilled();
            if (compare(c, reputation, 0xbb8) <= 0) return unfilled();
            if (compare(c, reputation, 0x1770) >= 0) return unfilled();
            relax = true;
        }
    }
    if (!nonzero(c, std::int8_t(c.u8(club.nation() + 0x120)))) {
        r13 = std::uint32_t(club.reputation());
        random_below(0xdc, Copy::b, 0x103612b4, 0x103613c4, 0x1036137c);
        advance(0x103613fc);
        const std::int32_t value = draw();
        leave(c, {{4, std::uint32_t(value)}, {5, cursor()}});
        if (compare(c, r14.value(), value % 20000) <= 0) return unfilled();
    }
    const auto marked = std::uint8_t(row.flags() | 0x80);
    const auto excluded = c.u32(at(local::excluded));
    c.put8(row_address + 0x28, marked);
    enter(c, 0x10361460, {manager, row_address, club_address, excluded});
    scan_candidates(c, manager, row_address, club_address, excluded);
}

void Scan::run() {
    program.call(0x103721f0, 0x1035ea4c, {at(local::quote)});
    const auto nation = club.nation().get();
    c.put8(at(local::in_window), 1);
    c.put16(at(local::divisor), 100);
    if (!nonzero_logical(c, nation)) return;
    check_window(program.call(0x10191f80, 0x1035ea6c, {nation}));
    set_divisor();
    if (!open()) return;
    if (!set_minimum()) return;
    best = c.u32(at(local::minimum));
    set_categories();
    walk();
    const auto chosen = c.u32(at(local::chosen));
    if (nonzero_logical(c, chosen)) {
        take(chosen);
        return;
    }
    conclude_empty();
}
}

void scan_candidates(Cpu& c, std::uint32_t manager, std::uint32_t row, std::uint32_t club, std::uint32_t excluded) {
    namespace slot = candidates::slot;
    namespace local = candidates::local;
    guest::Frame frame(c, candidates::entry, 0x1b0, {.first_gpr = 13, .gpr_offset = -0x6c, .first_fpr = 29});
    candidates::Scan s(c, frame, row, club);
    s.manager = manager;
    s.r13 = c.toc(slot::competitions);
    s.row_address = row;
    s.contracts_slot = c.toc(slot::contracts);
    s.club_address = club;
    s.constants = c.toc(slot::constants);
    s.player_row = 0;
    s.salt_slot = c.toc(slot::random_salt);
    s.table = c.toc(slot::random_table);
    s.end_slot = c.toc(slot::random_end);
    s.cursor_slot = c.toc(slot::random_cursor);
    c.put32(frame.at(local::excluded), excluded);
    c.put8(frame.at(local::human_talks), 0);
    s.run();
}
}
