// TRANSFER_MANAGER.CPP: the transfer window dates and the nation-rule checks over them (block "queries").
#include "transfer_manager_native_queries.hpp"
#include "transfer_manager_native_queries_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace queries_detail;
namespace {
constexpr std::uint32_t make_date = 0x107156f0, make_date_in_month = 0x107158a0, default_date = 0x10715880, add_days = 0x10717890,
                        day_number = 0x107182f0, print = 0x10727890, describe_person = 0x100caeb0, describe_club = 0x10168a00,
                        check_contract_end = 0x10083110, contract_of_blocked = 0x1007f7b0, offer_allowed_entry = 0x103f83c0,
                        narrow_entry = 0x103fa8e0, season_window_entry = 0x103fb130;
constexpr std::int32_t people_cell = toc::people, contracts_cell = toc::contracts, found_cell = toc::found_contract, clubs_cell = toc::clubs;
constexpr std::uint32_t no_day = 0xffffffffu;

// The text explaining why 0x103f83c0 refused (its reason code at frame+0x3a), printed into the sink the original
// chooses: the text argument, or the shared sink when the sentence names the club.
void explain_refusal(Cpu& c, const Frame& frame, std::uint32_t club, std::uint32_t person, std::uint32_t text, std::uint32_t contracts,
                     std::uint32_t clubs, std::uint32_t competitions, std::uint32_t sink, std::uint32_t pool) {
    const auto buffer = frame.at(0x188);
    const auto reason = std::int32_t(c.i16(frame.at(0x3a)));
    const auto club_name = [&](std::uint32_t at) { return std::uint32_t(std::int32_t(c.i8(at + 0x23b))); };
    if (reason == 0x21) {
        const auto manager_object = c.u32(c.toc(-0x7b40));
        keep(c, 20, manager_object);
        const auto id = c.u32(person);
        if (c.u8(c.u32(manager_object + 0x9a0) + id * 10) & 0x40) {
            const auto holder = invoke(c, contract_of_blocked, 0x103fa414, {c.u32(contracts), id});
            if (holder != 0) {
                c.put32(frame.at(0x40), c.u32(clubs) + c.u32(holder + 4) * club_size);
            } else {
                const auto row = c.u32(manager_object + 0x9a0) + c.u32(person) * 10;
                c.put8(row, c.u8(row) & 0xbf);
            }
        }
        // The club named is read from the frame slot at 0x40, which only the branch above writes.
        const auto named = c.u32(frame.at(0x40));
        invoke(c, print, 0x103fa470, {sink, pool + 0x21a6, club_name(named), club_name(named), buffer, named + 0x221});
    } else if (reason == 0x27) {
        invoke(c, print, 0x103fa488, {text, pool + 0x2218});
    } else if (reason == 7) {
        invoke(c, print, 0x103fa4a4, {text, pool + 0x225e, buffer});
    } else if (reason == 1) {
        invoke(c, print, 0x103fa4c0, {text, pool + 0x22ba, buffer});
    } else if (reason == 0x12 || reason == 0x13) {  // 0x13 is never stored: 0x103f83c0's reason stores are li 0x27, 0x21, 7, 1, 0x12, 0x20, 0x19, 0x28, 0x1a, 0 only (0x103f8444 ... 0x103f87e4)
        invoke(c, print, 0x103fa4e4, {sink, pool + 0x2306, buffer});
    } else if (reason == 0x19 || reason == 0x28 || reason == 0x1a) {
        // The club the person's rival rule names: its name goes to a 100-byte buffer, then into the sentence.
        const auto table = c.u32(competitions);
        const auto at = reason == 0x19 ? frame.at(0x124) : reason == 0x28 ? frame.at(0xc0) : frame.at(0x5c);
        const auto source = reason == 0x28 ? c.u32(club + 0x2c)
                                           : table + c.u32(c.toc(reason == 0x19 ? -0x6678 : -0x7a18)) * 0x128;
        invoke(c, describe_club, reason == 0x19 ? 0x103fa514 : reason == 0x28 ? 0x103fa55c : 0x103fa5b4, {source, at, 0x64, 0xd});
        invoke(c, print, reason == 0x19 ? 0x103fa53c : reason == 0x28 ? 0x103fa584 : 0x103fa5dc,
               {sink, pool + 0x2354, club_name(club), club_name(club), 0x64, 0x64, club + 0x221, at});
    }
}
}

void season_window(Cpu& c, std::uint32_t out, std::uint32_t out_end) {
    Frame frame(c, 0x103fb130, 0x80, {.first_gpr = 28});
    const auto now = c.toc(now_cell);
    keep(c, 31, out_end);
    keep(c, 30, out);
    keep(c, 29, now);
    keep(c, 28, now + 2);
    // Four dates of the clock's year: (1, 6), (31, 6), (15, 11) and (15, 0) as day and month arguments.
    const auto first = frame.at(0x60), second = frame.at(0x58), third = frame.at(0x50), fourth = frame.at(0x48);
    invoke(c, make_date, 0x103fb174, {first, 1, 6, c.u16(now + 2), no_day});
    invoke(c, make_date, 0x103fb18c, {second, 0x1f, 6, c.u16(now + 2), no_day});
    invoke(c, make_date, 0x103fb1a4, {third, 0xf, 0xb, c.u16(now + 2), no_day});
    invoke(c, make_date, 0x103fb1bc, {fourth, 0xf, 0, c.u16(now + 2), no_day});
    const auto clock = c.i16(now);
    const auto mark = [&](std::uint32_t at) { return std::int32_t(c.i16(at)); };
    const auto copy_clock = [&] { write_date(c, out, {c.i16(now), c.u16(now + 2), c.u8(now + 4)}); };
    const auto copy_end = [&](std::uint32_t at) { if (out_end != 0) write_date(c, out_end, read_date(c, at)); };
    // The end date of the next year's (15, 0): built into `at` first.
    const auto copy_next_year_end = [&](std::uint32_t at, std::uint32_t back) {
        if (out_end == 0) return;
        invoke(c, make_date, back, {at, 0xf, 0, (c.u16(now + 2) + 1) & 0xffffu, no_day});
        write_date(c, out_end, read_date(c, at));
    };
    if (clock >= mark(first) && clock <= mark(second)) {
        copy_clock();
        copy_end(second);
    } else if (clock >= mark(third)) {
        copy_clock();
        copy_next_year_end(frame.at(0x40), 0x103fb260);
    } else if (clock <= mark(fourth)) {
        copy_clock();
        copy_end(fourth);
    } else if (clock < mark(first)) {
        write_date(c, out, read_date(c, first));
        copy_end(second);
    } else {
        write_date(c, out, read_date(c, third));
        copy_next_year_end(frame.at(0x38), 0x103fb34c);
    }
}

std::int32_t earliest_offer_date(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t out) {
    Frame frame(c, 0x103f8cc0, 0xb0, {.first_gpr = 18});
    const auto now = c.toc(now_cell), people = c.toc(people_cell), contracts = c.toc(contracts_cell), found_at = c.toc(found_cell),
               clubs = c.toc(clubs_cell);
    keep(c, 22, now);
    keep(c, 28, manager);
    keep(c, 23, people);
    keep(c, 29, offer);
    keep(c, 24, contracts);
    keep(c, 30, out);
    keep(c, 25, found_at);
    keep(c, 19, 0);
    keep(c, 26, clubs);
    const auto cursor = frame.at(0x6c);
    invoke(c, default_date, 0x103f8cfc, {cursor});
    invoke(c, add_days, 0x103f8d0c, {frame.at(0x54), now, 0x16d});
    // The date a year ahead; the last byte of its copy is the one byte of the word at 0x68 that is written.
    const auto horizon = read_date(c, frame.at(0x54));
    c.put8(frame.at(0x68), horizon.extra);
    keep(c, 31, 0);
    invoke(c, default_date, 0x103f8d30, {frame.at(0x5c)});
    keep(c, 21, now + 2);
    keep(c, 20, now + 4);
    const Date today = read_date(c, now);
    write_date(c, out, today);
    const auto offer_kind_nibble = c.u8(offer + 0x38) >> 4;
    const auto person = c.u32(people) + c.u32(offer + 0xc) * person_size;
    // The earliest date the offer can start: today, or the day the person's contract with the club runs out.
    Date start = today;
    if (offer_kind_nibble == 4) {
        const auto contract = invoke(c, find_contract, 0x103f8d80, {c.u32(contracts), person});
        if (contract != 0) {
            const Date ends = read_date(c, contract + 0x30);
            if (!later_than(today.year, today.day, ends.year, ends.day)) start = ends;
        }
    }
    keep(c, 27, 0);
    keep(c, 18, person);
    const auto buyer = c.u32(clubs) + c.u32(offer + 8) * club_size;
    keep(c, 24, buyer);
    const auto found = invoke(c, find_contract, 0x103f8e7c, {c.u32(contracts), person});
    c.put32(found_at, found);
    keep(c, 27, found != 0 && (c.u8(found + 0x38) >> 2) != 5 ? 1 : 0);
    const auto seller = contract_club_of(c, found);
    keep(c, 23, seller);
    std::int32_t reason = 0;
    if (offer_kind_nibble == 5) {
        invoke_native(c, season_window_entry, 0x103f8ef0, {manager, out, 0});
        const auto rule = nation_rule(c, manager, c.u32(buyer + club_nation));
        if (rule != 0 && std::int8_t(call_rule(c, frame, rule, 0x18, 0x103f8f54, {rule, offer, buyer, out, frame.at(0x38)})) == 0) {
            const auto code = std::int32_t(c.i16(frame.at(0x38)));
            if (code != 9) {
                reason = code;
                keep_signed(c, 31, reason);
            }
        }
        return reason;
    }
    if (c.u32(person + 0x14) == 0) {
        write_date(c, out, {c.i16(now), c.u16(now + 2), c.u8(now + 4)});
        return 0;
    }
    const auto buyer_rule = nation_rule(c, manager, c.u32(buyer + club_nation));
    keep(c, 20, buyer_rule);
    std::uint32_t seller_rule = 0;
    if (seller != 0 && c.u32(offer + 4) != 0) seller_rule = nation_rule(c, manager, c.u32(seller + club_nation));
    keep(c, 19, seller_rule);
    // The stale read of the frame word at 0x68 (stored to 0x40 by the original, where nothing reads it).
    read_stale(c, frame.at(0x68));
    keep(c, 18, (std::uint32_t(horizon.day) & 0xffffu) << 16 | horizon.year);
    write_date(c, cursor, start);
    const auto codes_end_search = [](std::int32_t code) { return code == 0x14 || code == 0x15 || code == 0x1f || code == 0x11; };
    // The first rule that refuses sets the reason, unless it is 9 or a reason is already set.
    const auto record_refusal = [&] {
        const auto code = std::int32_t(c.i16(frame.at(0x38)));
        if (code != 9 && std::int16_t(reason) == 0) {
            reason = code;
            keep_signed(c, 31, reason);
        }
        return codes_end_search(std::int32_t(std::int16_t(reason)));
    };
    for (;;) {
        // Continue while the cursor is before the horizon.
        const auto at = read_date(c, cursor);
        if (!later_than(horizon.year, horizon.day, at.year, at.day)) return reason;
        bool give_date = false;
        bool refused_by_buyer = false;
        if (buyer_rule != 0 && std::int8_t(call_rule(c, frame, buyer_rule, 0x18, 0x103f9098, {buyer_rule, offer, buyer, cursor, frame.at(0x38)})) == 0)
            refused_by_buyer = true;
        if (!refused_by_buyer || record_refusal()) {
            if (seller_rule == 0 ||
                std::int8_t(call_rule(c, frame, seller_rule, 0x18, 0x103f9108, {seller_rule, offer, seller, cursor, frame.at(0x38)})) != 0 ||
                record_refusal())
                give_date = true;
        }
        if (give_date) {
            write_date(c, out, read_date(c, cursor));
            return 0;
        }
        invoke(c, add_days, 0x103f9180, {frame.at(0x4c), cursor, 1});
        write_date(c, cursor, read_date(c, frame.at(0x4c)));
    }
}

void narrow_window_to_contract(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t club_b, std::uint32_t person,
                               std::uint32_t month_a, std::uint32_t year_a, std::uint32_t month_b, std::uint32_t year_b) {
    Frame frame(c, 0x103fa8e0, 0x90, {.first_gpr = 25});
    const auto now = c.toc(now_cell);
    keep(c, 31, person);
    keep(c, 25, month_a);
    keep(c, 30, now);
    keep(c, 26, year_a);
    keep(c, 27, month_b);
    keep(c, 28, year_b);
    // The nation's rule narrows the window first (a table's first entry stands in when no rule names the nation).
    if (const auto rule = nation_rule(c, manager, c.u32(club + club_nation)); rule != 0)
        call_rule(c, frame, rule, 0x30, 0x103fa974, {rule, club, club_b, month_a, year_a, month_b, year_b, person});
    else {
        const auto first_rule = c.u32(manager + nation_rules);
        call_rule(c, frame, first_rule, 0x30, 0x103fa9a0, {first_rule, club, club_b, month_a, year_a, month_b, year_b, person});
    }
    const auto days = invoke(c, day_number, 0x103fa9ac, {now});
    keep(c, 29, days);
    const auto contract = invoke(c, find_contract, 0x103fa9c0, {c.u32(c.toc(toc::contracts)), person});
    keep(c, 31, contract);
    const auto set_year_from = [&](std::uint32_t buffer, std::uint32_t back) {
        const auto date = invoke(c, default_date, back, {buffer});
        c.put16(year_b, c.u16(date + 2));
    };
    if (contract == 0 || (c.u8(contract + 0x38) >> 2) != 2) return set_year_from(frame.at(0x68), 0x103fa9e8);
    const auto today = read_date(c, now);
    const auto ends_year = c.u16(contract + 0x32);
    const auto ends_day = std::int32_t(c.i16(contract + 0x30));
    if (later_than(today.year, today.day, ends_year, ends_day)) return set_year_from(frame.at(0x60), 0x103faa58);
    keep(c, 30, now + 2);
    if (std::int32_t(c.u16(year_b)) > std::int32_t(c.u16(now + 2) + 5)) return set_year_from(frame.at(0x58), 0x103faa80);
    for (;;) {
        // The month before the window's end, as a date: stop once it is not after the contract's end.
        const auto date = invoke(c, make_date, 0x103fab30, {frame.at(0x48), days, c.u8(month_b), c.u16(year_b), no_day});
        const auto built = c.u32(date);
        read_stale(c, date + 4);
        if (!later_than(built & 0xffffu, std::int32_t(std::int16_t(built >> 16)), ends_year, ends_day)) return;
        c.put8(month_b, c.u8(month_b) - 1);
        if (c.i8(month_b) < 0) {
            c.put16(year_b, c.u16(year_b) - 1u);
            c.put8(month_b, 11);
        }
        // Keep the earliest (year, month) seen in the first pair.
        const auto year_first = c.u16(year_a), year_now = c.u16(year_b);
        if (year_first > year_now || (year_first == year_now && c.i8(month_a) > c.i8(month_b))) {
            c.put8(month_a, c.u8(month_b));
            c.put16(year_a, c.u16(year_b));
        }
        if (c.u16(year_b) < c.u16(now + 2)) return set_year_from(frame.at(0x50), 0x103fab0c);
    }
}

bool transfer_possible(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t person, std::uint32_t text) {
    Frame frame(c, 0x103fa290, 0x2f0, {.first_gpr = 20});
    const auto contracts = c.toc(contracts_cell), found_at = c.toc(found_cell), clubs = c.toc(clubs_cell);
    const auto competitions = c.toc(-0x7b98), sink = c.toc(-0x7bb8), pool = c.toc(-0x4b9c);
    keep(c, 20, 0);
    keep(c, 21, competitions);
    keep(c, 22, sink);
    keep(c, 23, contracts);
    keep(c, 24, found_at);
    keep(c, 25, clubs);
    keep(c, 26, pool);
    keep(c, 27, manager);
    keep(c, 28, club);
    keep(c, 29, person);
    keep(c, 30, text);
    const auto found = invoke(c, find_contract, 0x103fa2d8, {c.u32(contracts), person});
    c.put32(found_at, found);
    const auto employer = contract_club_of(c, found);
    keep(c, 20, employer);
    invoke(c, default_date, 0x103fa32c, {frame.at(0x54)});
    const auto days = invoke(c, day_number, 0x103fa334, {c.toc(now_cell)});
    keep(c, 31, days);
    if (employer == 0) return false;
    if (invoke(c, check_contract_end, 0x103fa354, {c.u32(contracts), person}) != 0) return false;
    if (c.u8(person + 0x62) != 0xb) {
        if (text != 0) {
            invoke(c, describe_person, 0x103fa38c, {person, frame.at(0x188), 0x12c, 1});
            invoke(c, print, 0x103fa39c, {sink, pool + 0x2153, frame.at(0x188)});
        }
        return false;
    }
    const auto reason_at = frame.at(0x3a);
    if (std::int8_t(invoke_native(c, offer_allowed_entry, 0x103fa3bc, {manager, person, club, reason_at, 0})) == 0) {
        if (text != 0) {
            invoke(c, describe_person, 0x103fa3e0, {person, frame.at(0x188), 0x12c, 1});
            explain_refusal(c, frame, club, person, text, contracts, clubs, competitions, sink, pool);
        }
        return false;
    }
    // The buyer's nation must accept the person, then the employer's nation must have a month to spare.
    if (const auto rule = nation_rule(c, manager, c.u32(club + club_nation)); rule != 0) {
        if (std::int8_t(call_rule(c, frame, rule, 0x28, 0x103fa648, {rule, club, person, text, 0})) == 0) return false;
    }
    const auto employer_rule = nation_rule(c, manager, c.u32(employer + club_nation));
    keep(c, 21, employer_rule);
    if (employer_rule == 0) return true;
    const auto lookup = invoke(c, find_contract, 0x103fa6b0, {c.u32(contracts), person});
    c.put32(found_at, lookup);
    const auto lookup_club = lookup != 0 ? c.u32(clubs) + c.u32(lookup + 4) * club_size : 0;
    invoke_native(c, narrow_entry, 0x103fa6f4, {manager, club, lookup_club, person, frame.at(0x39), frame.at(0x3e), frame.at(0x38), frame.at(0x3c)});
    if (c.i8(frame.at(0x39)) == c.i8(frame.at(0x38)) && c.u16(frame.at(0x3e)) == c.u16(frame.at(0x3c))) return true;
    std::uint32_t month = c.u8(frame.at(0x39)), year = c.u16(frame.at(0x3e));
    keep(c, 22, month);
    keep(c, 23, year);
    for (;;) {
        invoke(c, make_date_in_month, 0x103fa744, {frame.at(0x54), days, month, year, no_day});
        if (std::int8_t(call_rule(c, frame, employer_rule, 0x2c, 0x103fa768, {employer_rule, employer, person, 0, frame.at(0x54), 1})) != 0)
            return true;
        month += 1;
        keep(c, 22, month);
        if (std::int8_t(month) > 0xb) {
            month = 0;
            keep(c, 22, 0);
            year += 1;
            keep(c, 23, year);
        }
        const auto date = invoke(c, make_date, 0x103fa7b0, {frame.at(0x4c), days, c.u8(frame.at(0x38)), c.u16(frame.at(0x3c)), no_day});
        const auto built = c.u32(date);
        read_stale(c, date + 4);
        if (!later_than(built & 0xffffu, std::int32_t(std::int16_t(built >> 16)), c.u16(frame.at(0x56)), c.i16(frame.at(0x54)))) break;
    }
    if (text != 0) invoke(c, print, 0x103fa820, {text, pool + 0x23be});
    return false;
}
}
