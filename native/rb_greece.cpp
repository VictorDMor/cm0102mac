#include "rb_greece_adapter.hpp"
#include "guest_abi.hpp"
#include "guest_saved_registers.hpp"
#include "routine_check.hpp"

#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
bool declining = std::getenv("CM_RB_GREECE_ORIGINAL") != nullptr;
constexpr auto nation_text_entry = 0x106ab7d0u, next_threshold_entry = 0x106ab890u;
constexpr auto construct_entry = 0x106ab8f0u, apply_entry = 0x106ab970u;
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

struct MatchDiscipline {
    Cpu& c;
    std::uint32_t address;
    unsigned player_id() const { return c.u32(address); }
    unsigned cautions() const { return c.u8(address + 0x2c); }
    unsigned dismissals() const { return c.u8(address + 0x2d); }
    unsigned match_flag() const { return c.u8(address + 0x48); }
};

struct BookingRecord {
    Cpu& c;
    std::uint32_t address;
    unsigned total() const { return c.u8(address + 4); }
    unsigned history() const { return c.u8(address + 0xa); }
    void total(unsigned value) const { c.put8(address + 4, value); }
    void history(unsigned value) const { c.put8(address + 0xa, value); }
};

struct Suspension {
    unsigned matches = 0;
    unsigned reason = UINT32_MAX;
    unsigned threshold;
};

void compare_record(Cpu& c, std::int32_t value) {
    const auto high = c.cr(0) & 0xf0;
    c.compare(0, value, 0);
    c.set_cr(0, high | c.cr(0));
}

bool no_cards(Cpu& c, const MatchDiscipline& match, std::uint32_t dismissals_block) {
    const auto cautions = match.cautions();
    c.set(0, cautions);
    c.compare_logical(0, cautions, 0);
    if (cautions) return false;
    block(c, dismissals_block, 3);
    const auto dismissals = match.dismissals();
    c.set(0, dismissals);
    c.compare_logical(0, dismissals, 0);
    return dismissals == 0;
}

void update_history(Cpu& c, const MatchDiscipline& match, const BookingRecord& record) {
    block(c, 0x106ab9a8, 16);
    // Advance the four older result bits in place, retaining the unrelated bits.
    for (const unsigned mask : {0x80u, 0x40u, 0x20u, 0x10u}) {
        const auto old = record.history();
        const auto shifted = (old & ~mask) | ((old << 1) & mask);
        c.set(4, shifted);
        if (mask == 0x80) c.set(5, 0);
        record.history(shifted);
    }
    const bool recorded_card = !no_cards(c, match, 0x106ab9e8);
    if (recorded_card) {
        block(c, 0x106ab9f4, 1);
        c.set(5, 1);
    }
    block(c, 0x106ab9f8, 8);
    c.set(4, recorded_card ? UINT32_MAX : 0);
    const auto history = (record.history() & ~8u) | (recorded_card ? 8u : 0);
    c.set(0, history);
    record.history(history);

    // A long enough clean run reduces the cumulative caution count by one.
    if (!no_cards(c, match, 0x106aba18)) return;
    block(c, 0x106aba24, 3);
    const auto recent = record.history();
    c.set(4, recent);
    constexpr std::array check_pcs{0x106aba24u, 0x106aba30u, 0x106aba38u, 0x106aba40u, 0x106aba48u};
    for (unsigned i = 0; i < check_pcs.size(); ++i) {
        if (i) block(c, check_pcs[i], 2);
        const auto bit = (recent >> (7 - i)) & 1;
        c.set(0, bit);
        compare_record(c, bit);
        if (bit) return;
    }
    block(c, 0x106aba50, 3);
    const auto total = record.total();
    c.set(4, total);
    c.compare_logical(0, total, 0);
    if (total) {
        block(c, 0x106aba5c, 2);
        c.set(0, total - 1);
        record.total(total - 1);
    }
}

void accumulate_cautions(Cpu& c, const MatchDiscipline& match, const BookingRecord& record,
                         Suspension& suspension) {
    block(c, 0x106aba64, 2);
    unsigned processed = 0;
    c.set(7, processed);
    for (;;) {
        block(c, 0x106abaf0, 4);
        const auto cautions = match.cautions();
        const auto index = std::int8_t(processed);
        c.set({{0, cautions}, {4, std::uint32_t(index)}});
        c.compare(0, index, cautions);
        if (index >= int(cautions)) break;

        block(c, 0x106aba6c, 6);
        const auto previous = record.total();
        c.set({{4, previous}, {0, previous + 1}});
        record.total(previous + 1);
        const auto total = record.total();
        c.set(6, total);
        c.compare_logical(0, total, 5);
        unsigned additional_matches = 0;
        if (total != 5) {
            block(c, 0x106aba84, 2);
            c.compare_logical(0, total, 8);
        }
        if (total == 5 || total == 8) {
            block(c, 0x106aba8c, 4);
            additional_matches = 1;
        } else {
            block(c, 0x106aba9c, 2);
            c.compare_logical(0, total, 11);
            if (total == 11) {
                block(c, 0x106abaa4, 4);
                additional_matches = 2;
            } else {
                block(c, 0x106abab4, 2);
                c.compare_logical(0, total, 14);
                if (total >= 14) {
                    block(c, 0x106ababc, 9);
                    const auto beyond = total - 14;
                    const auto remainder = beyond % 3;
                    c.set({{4, beyond / 3}, {5, beyond}, {0, remainder}});
                    compare_record(c, remainder);
                    if (!remainder) {
                        block(c, 0x106abae0, 3);
                        additional_matches = 3;
                    }
                }
            }
        }
        if (additional_matches) {
            suspension.matches += additional_matches;
            suspension.reason = 1;
            suspension.threshold = std::uint32_t(std::int8_t(total));
            c.set({{27, suspension.threshold}, {28, suspension.reason}, {29, suspension.matches}});
        }
        block(c, 0x106abaec, 1);
        c.set(7, ++processed);
    }
}

constexpr auto construct_date = 0x10715880u, shift_date = 0x10717890u, weekday = 0x10715b20u;

void copy_adjusted_date(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    const auto leap = c.u8(source + 4);
    c.set(0, leap);
    const auto year = c.u16(source + 2);
    c.set(4, year);
    const auto day = c.i16(source);
    c.set(3, std::uint32_t(day));
    c.put16(destination + 2, year);
    c.put16(destination, day);
    c.put8(destination + 4, leap);
}

void queue_hearing(Cpu& c, const MatchDiscipline& match, std::uint32_t rulebook,
                   std::uint32_t hearing_details, std::uint32_t today) {
    const auto frame = c.sp();
    for (const auto [pc, offset] : std::array<std::pair<unsigned, unsigned>, 3>{{
             {0x106abb0cu, 0x74u}, {0x106abb14u, 0x88u}, {0x106abb1cu, 0x50u}}}) {
        block(c, pc, 2);
        c.set(3, frame + offset);
        c.call(construct_date, pc + 8);
    }
    block(c, 0x106abb24, 5);
    c.set({{3, rulebook}, {5, match.address}, {4, frame + 0x70}, {6, 1}});
    c.call(0x105bd010, 0x106abb38);
    block(c, 0x106abb38, 2);
    const auto valid = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(valid));
    compare_record(c, valid);
    if (!valid) return;

    // Hearings start at the first Thursday strictly after the current date.
    block(c, 0x106abb40, 4);
    c.set({{4, today}, {3, frame + 0x48}, {5, 1}});
    c.call(shift_date, 0x106abb50);
    block(c, 0x106abb50, 7);
    copy_adjusted_date(c, frame + 0x50, frame + 0x48);
    for (;;) {
        block(c, 0x106abb98, 2);
        c.set(3, frame + 0x50);
        c.call(weekday, 0x106abba0);
        block(c, 0x106abba0, 3);
        const auto day_of_week = std::int16_t(c.r(3));
        c.set(0, std::uint32_t(day_of_week));
        c.compare(0, day_of_week, 3);
        if (day_of_week == 3) break;
        block(c, 0x106abb70, 4);
        c.set({{3, frame + 0x40}, {4, frame + 0x50}, {5, 1}});
        c.call(shift_date, 0x106abb80);
        block(c, 0x106abb80, 6);
        copy_adjusted_date(c, frame + 0x50, frame + 0x40);
    }

    block(c, 0x106abbac, 98);
    const auto leap = c.u8(frame + 0x54);
    c.set({{6, leap}, {0, UINT32_MAX}});
    const auto year = c.u16(frame + 0x52);
    c.set({{4, year}, {3, 16}});
    const auto day = c.i16(frame + 0x50);
    c.set({{7, std::uint32_t(day)}, {5, 0x78}});
    c.put16(frame + 0x8a, year);
    c.set(4, frame + 0x70);
    c.put16(frame + 0x88, day);
    c.put8(frame + 0x8c, leap);
    c.put8(frame + 0x7c, 16);
    for (unsigned offset = 0x7d; offset <= 0x7f; ++offset)
        c.put8(frame + offset, UINT32_MAX);

    // Preserve the original field widths and ordered load/store pairs when
    // copying the hearing details into the queue record.
    const auto details = frame + 0x94;
    for (unsigned offset = 0; offset < 0x24; offset += 4) {
        const auto value = c.u32(hearing_details + offset);
        c.set(0, value);
        c.put32(details + offset, value);
    }
    for (unsigned offset = 0x24; offset < 0x3a; offset += 2) {
        const auto value = c.u16(hearing_details + offset);
        c.set(0, offset >= 0x2e ? std::uint32_t(std::int16_t(value)) : value);
        c.put16(details + offset, value);
    }
    for (unsigned offset = 0x3a; offset < 0x4e; ++offset) {
        const auto value = c.u8(hearing_details + offset);
        c.set(0, value);
        c.put8(details + offset, value);
    }
    const auto last = c.u16(hearing_details + 0x4e);
    c.set(0, last);
    c.put16(details + 0x4e, last);
    c.set(3, c.u32(rulebook + 8));
    c.call(0x10738d10, 0x106abd34);
}

void publish_suspension(Cpu& c, const MatchDiscipline& match, const Suspension& suspension,
                        std::uint32_t hearing_details, std::uint32_t discipline_slot, std::uint32_t today) {
    const auto frame = c.sp(), notice = frame + 0x58;
    block(c, 0x106abd48, 2);
    c.set(3, notice + 4);
    c.call(construct_date, 0x106abd50);
    block(c, 0x106abd50, 9);
    c.set({{5, UINT32_MAX}, {0, 0}});
    c.put16(notice + 2, UINT32_MAX);
    c.set({{4, today}, {3, frame + 0x38}, {5, 14}});
    c.put8(notice, suspension.matches);
    c.put8(notice + 1, 0);
    c.call(shift_date, 0x106abd74);
    block(c, 0x106abd74, 28);
    const auto leap = c.u8(frame + 0x3c);
    c.set({{8, leap}, {3, 0}, {4, UINT32_MAX}});
    const auto year = c.u16(frame + 0x3a);
    c.set(5, year);
    const auto day = c.i16(frame + 0x38);
    c.set({{9, std::uint32_t(day)}, {7, std::uint32_t(std::int8_t(suspension.threshold))}});
    const auto flags = c.u8(notice + 0x14) & ~0x40u;
    c.set({{0, flags}, {6, 16}});
    c.put16(notice + 6, year);
    c.set({{3, c.u32(discipline_slot)}, {5, notice}});
    c.put8(notice + 0x14, flags);
    c.put16(notice + 4, day);
    const auto live_flags = c.u8(notice + 0x14);
    c.set(0, live_flags);
    c.put8(notice + 8, leap);
    c.put8(notice + 0x10, suspension.reason);
    c.put16(notice + 0x12, c.r(7));
    c.put8(notice + 0xc, 16);
    for (unsigned offset = 0xd; offset <= 0xf; ++offset)
        c.put8(notice + offset, UINT32_MAX);
    const auto leading_zeros = std::countl_zero(match.match_flag());
    const auto final_flags = (live_flags & ~0x20u) | (leading_zeros & 0x20u);
    c.set({{4, std::uint32_t(leading_zeros)}, {0, final_flags}});
    c.put8(notice + 0x14, final_flags);
    c.set(4, match.player_id());
    c.call(0x100dfb30, 0x106abde4);
    block(c, 0x106abde4, 9);
    const auto player = match.player_id();
    c.set({{0, player}, {6, hearing_details}});
    const auto player_index = c.toc(-0x7be0);
    c.set({{4, player_index}, {5, notice}, {0, player * 0x70u}});
    c.set(3, c.u32(discipline_slot));
    c.set(4, c.u32(player_index) + player * 0x70u);
    c.call(0x100e0700, 0x106abe08);
}

void apply_discipline(Cpu& c) {
    GuestNativeScope scope(c.s, apply_entry);
    block(c, apply_entry, 14);
    const auto caller = c.sp(), frame = caller - 0x110;
    const auto rulebook = c.r(3), hearing_details = c.r(5);
    const MatchDiscipline match{c, c.r(4)};
    Suspension suspension{.threshold = c.r(27)};
    c.set(0, c.lr());
    guest::transfer_saved_registers<24>(c, caller);
    c.set({{26, hearing_details}, {25, match.address}});
    const auto discipline_slot = c.toc(-0x7aa4);
    c.set({{30, discipline_slot}, {24, rulebook}});
    const auto today = c.toc(-0x7b80);
    c.set({{31, today}, {29, 0}, {28, UINT32_MAX}, {5, 1}});
    push_frame(c, scope, caller, frame);
    c.set(4, match.player_id());
    c.call(0x105bc2b0, 0x106ab9a8);
    const BookingRecord booking{c, c.r(3)};
    update_history(c, match, booking);
    accumulate_cautions(c, match, booking, suspension);

    block(c, 0x106abb00, 3);
    const auto dismissals = match.dismissals();
    c.set(0, dismissals);
    c.compare_logical(0, dismissals, 1);
    if (dismissals == 1) {
        queue_hearing(c, match, rulebook, hearing_details, today);
        block(c, 0x106abd34, 3);
        suspension.reason = 2;
        suspension.threshold = 1;
        ++suspension.matches;
        c.set({{28, suspension.reason}, {27, suspension.threshold}, {29, suspension.matches}});
    }
    block(c, 0x106abd40, 2);
    const auto matches = std::int8_t(suspension.matches);
    c.set(0, std::uint32_t(matches));
    compare_record(c, matches);
    if (matches) publish_suspension(c, match, suspension, hearing_details, discipline_slot, today);
    block(c, 0x106abe08, 5);
    restore_link(c, caller);
    guest::transfer_saved_registers<24, true>(c, caller);
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
    case apply_entry: apply_discipline(c); break;
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
