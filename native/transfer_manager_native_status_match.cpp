#include "transfer_manager_native_status.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t status_rows = 0x9a0, status_row_size = 10;
constexpr std::uint8_t status_flag_move = 0x20;  // set while a person is on the move through the manager
constexpr std::uint32_t side_stride = 0x664, entry_stride = 0x4c, entry_count = 20, first_substitute = 11;
constexpr std::uint32_t entry_person = 0x194, entry_marker = 0x1b1, entry_slot = 0x1c6;
constexpr std::uint32_t list_at = 0x38, chosen_at = 0x100, list_size = 50;
constexpr std::uint8_t kind_player = 2, kind_staff = 4;

// One pass over a side of the match: the club's people on the move are collected, the lined-up people have
// their status fees settled, and the collected people who did not play are charged an absence.
class SidePass {
public:
    SidePass(Cpu& c, Frame& frame, std::uint32_t manager, std::uint32_t match, std::uint32_t lineups)
        : c(c), frame(frame), manager(manager), match(match), lineups(lineups), contracts(c.toc(toc::contracts)),
          people_cell(c.toc(toc::people)), clubs_cell(c.toc(toc::clubs)), found_cell(c.toc(toc::found_contract)),
          limit_cell(c.toc(-0x7be4)), injury_cell(c.toc(-0x7ba0)), national_cell(c.toc(-0x7a64)),
          discipline_cell(c.toc(-0x7aa4)), fixture_word(c.toc(-0x7b80)) {}

    void run() {
        keep(c, 14, discipline_cell);
        keep(c, 13, fixture_word);
        keep(c, 15, manager);
        keep(c, 16, match);
        keep(c, 17, lineups);
        keep(c, 24, 0);
        keep(c, 25, 0);
        keep(c, 28, national_cell);
        keep(c, 29, injury_cell);
        keep(c, 30, contracts);
        keep(c, 31, limit_cell);
        for (std::int8_t side = 0; side < 2; ++side) {
            one_side(side, std::uint32_t(side) * side_stride);
            keep(c, 25, std::uint32_t(side + 1) * side_stride);
            keep(c, 24, std::uint32_t(side + 1));
        }
    }

private:
    std::uint8_t flags_of(std::uint32_t person) const {
        return c.u8(c.u32(manager + status_rows) + c.u32(person) * status_row_size);
    }
    bool tiny(std::uint32_t value) const { return std::uint8_t(value) != 0; }
    std::uint32_t club_of(std::uint32_t person, std::uint32_t back) {
        const auto contract = invoke(c, 0x10082f20, back, {c.u32(contracts), person});
        c.put32(found_cell, contract);
        return contract ? c.u32(clubs_cell) + c.u32(contract + 4) * club_size : 0;
    }

    void one_side(std::int8_t side, std::uint32_t side_offset) {
        keep(c, 19, 0);
        const auto club = c.u32(match + (side == 0 ? 0x1c : 0x20));
        keep(c, 27, club);
        bool moving = false;
        // `moving`: the side's people on the move are tracked (a fixture without a competition, a human club's
        // or a flagged side, or id -2).
        bool by_id = c.u32(match + 0x14) == 0 || tiny(invoke(c, 0x100c7d00, 0x103e8d9c, {c.u32(match + 0x1c)})) ||
                     tiny(invoke(c, 0x10494d20, 0x103e8db4, {c.u32(c.toc(-0x7b54)), c.u32(match + 0x14)}));
        if (!by_id) {
            if (c.u8(match + (side == 0 ? 0x40 : 0x41)) != 0) {
                moving = true;
                keep(c, 19, 1);
            } else {
                by_id = true;
            }
        }
        if (by_id && c.i32(match) == -2) {
            moving = true;
            keep(c, 19, 1);
        }
        std::uint8_t kind;
        if (tiny(invoke(c, 0x100c7d00, 0x103e8df8, {club}))) {
            kind = kind_staff;
            keep(c, 22, kind);
        } else {
            kind = kind_player;
            keep(c, 22, kind);
            invoke(c, 0x100d06f0, 0x103e8e1c, {club, frame.at(list_at), 0});
        }
        std::int8_t chosen = 0;
        keep(c, 20, 0);
        if (moving) {
            for (std::int8_t i = 0; i < std::int8_t(list_size); ++i) {
                const auto candidate = c.u32(frame.at(list_at + 4 * std::uint32_t(i)));
                if (candidate == 0) continue;
                // The kind 2 list (0x100d0708..0x100d0888) only holds ids below the limit; the other fillers are the raw
                // copy at 0x100d08fc (when 0x100ce320 fails) and a human club's stale frame bytes.
                if (c.i32(candidate) >= std::int32_t(c.u32(limit_cell))) continue;
                if (!(flags_of(candidate) & status_flag_move)) continue;
                c.put32(frame.at(chosen_at + 4 * std::uint32_t(std::int32_t(chosen))), candidate);
                ++chosen;
                keep(c, 20, std::uint32_t(chosen));
            }
        }
        keep(c, 23, 0);
        keep(c, 26, 0);
        for (std::int8_t i = 0; i < std::int8_t(entry_count);) {
            if (!lined_up(side_offset, i, kind, moving, chosen)) break;
            ++i;
            keep(c, 26, std::uint32_t(i) * entry_stride);
            keep(c, 23, std::uint32_t(i));
        }
        if (!moving) return;
        absences(club, chosen);
    }

    // One lineup entry; false when the entry names a person without a club (the original stops the side there).
    bool lined_up(std::uint32_t side_offset, std::int8_t i, std::uint8_t kind, bool moving,
                  std::int8_t& chosen) {
        const auto entry = lineups + side_offset + std::uint32_t(i) * entry_stride;
        if (c.i8(entry + entry_marker) == -5) return true;
        if (i >= std::int8_t(first_substitute) && c.i8(entry + entry_slot) == -1) return true;
        const auto person = c.u32(people_cell) + c.u32(entry + entry_person) * person_size;
        keep(c, 21, person);
        if (c.u32(person + 0x14) == 0) return false;
        if (person != 0 && c.i32(person) >= std::int32_t(c.u32(limit_cell))) return true;
        if (kind == kind_player && invoke(c, 0x10083110, 0x103e8f28, {c.u32(contracts), person}) != 0) return true;
        if (flags_of(person) & kind) {
            const auto term = std::int16_t(invoke_native(c, 0x103e9220, 0x103e8f60, {manager, person, kind}));
            const std::uint8_t code = kind == kind_staff ? 5 : 2;
            keep(c, 18, code);
            if ((term % 10 == 0 && term <= 0x32) || term == 1) {
                const auto posted = invoke_native(c, 0x103f2800, 0x103e8fd0,
                                                  {manager, person, std::uint32_t(std::int32_t(term)), code, 0});
                if (tiny(posted)) {
                    if (code == 5) invoke(c, 0x103f2b50, 0x103e8ff0, {manager, person});
                    else invoke(c, 0x103f2aa0, 0x103e9000, {manager, person});
                }
            }
        }
        if (!moving) return true;
        // 0x103ee1a0 is `li r3,1; blr`: it never answers 0.
        if (!tiny(invoke(c, 0x103ee1a0, 0x103e9014, {manager, person}))) return true;
        if (!(flags_of(person) & status_flag_move)) return true;
        const auto record = invoke(c, 0x103ecb00, 0x103e9040, {manager, person});
        c.put16(record + 4, std::uint32_t(c.u16(record + 4) + 1));
        for (std::int8_t j = 0; j < chosen; ++j) {
            const auto slot = frame.at(chosen_at + 4 * std::uint32_t(std::int32_t(j)));
            if (c.u32(slot) == person) {
                c.put32(slot, 0);
                break;
            }
        }
        return true;
    }

    // The collected people who were not lined up (their slot is still set): each who passes the status,
    // injury, discipline, national-team and club tests is charged an absence.
    void absences(std::uint32_t club, std::int8_t chosen) {
        keep(c, 20, std::uint32_t(std::int32_t(chosen)));
        for (std::int8_t j = 0;; ++j) {
            keep(c, 18, std::uint32_t(j));
            keep(c, 21, std::uint32_t(j) * 4);
            keep(c, 19, frame.at(chosen_at + 4 * std::uint32_t(j)));
            if (!(j < chosen)) return;
            const auto person = c.u32(frame.at(chosen_at + 4 * std::uint32_t(j)));
            keep(c, 22, person);
            if (person == 0) continue;
            // (always true: 0x103ee1a0 is `li r3,1; blr`)
            if (!tiny(invoke(c, 0x103ee1a0, 0x103e90e4, {manager, person}))) continue;
            if (tiny(invoke(c, 0x10187680, 0x103e90f8, {c.u32(injury_cell), person}))) continue;
            if (c.u32(match + 0x14) != 0 &&
                tiny(invoke(c, 0x100e7920, 0x103e9120, {c.u32(discipline_cell), person, c.u32(match + 0x14), 1, 0})))
                continue;
            if (invoke(c, 0x102b7c70, 0x103e9144, {c.u32(national_cell), person, fixture_word, club, 0, 0}) != 0)
                continue;
            if (c.u32(match + 0x14) != 0 && tiny(invoke(c, 0x103f93e0, 0x103e9168, {manager, person, match}))) continue;
            if (invoke(c, 0x10083110, 0x103e917c, {c.u32(contracts), person}) != 0) {
                const auto holder = club_of(person, 0x103e9190);
                if (holder == club) continue;
            }
            const auto record = invoke(c, 0x103ecb00, 0x103e91d0, {manager, person});
            c.put16(record + 6, std::uint32_t(c.u16(record + 6) + 1));
        }
    }

    Cpu& c;
    Frame& frame;
    std::uint32_t manager, match, lineups;
    std::uint32_t contracts, people_cell, clubs_cell, found_cell, limit_cell, injury_cell, national_cell,
        discipline_cell, fixture_word;
};
}

void match_transfer_statuses(Cpu& c, std::uint32_t manager, std::uint32_t match, std::uint32_t lineups) {
    Frame frame(c, 0x103e8d30, 0x220, {.first_gpr = 13});
    SidePass(c, frame, manager, match, lineups).run();
}
}
