// 0x103f6240: moves a contract to a new stage (the low nibble of its byte +0x60), with the mood changes and the
// draws that decide whether the person's contract talks end.
#include "transfer_manager_native_lists.hpp"
#include "transfer_manager_native_lists_detail.hpp"
#include "transfer_manager_native_status.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
// Offsets in the contract record found for the person and club.
namespace contract {
inline constexpr std::uint32_t kind = 0x38, since = 0x28, flags = 0x50, stage = 0x60, mood_a = 0x44, mood_b = 0x4c,
                               note = 0x5c;
}
// Offsets in the person record.
namespace person {
inline constexpr std::uint32_t amount = 8, profile = 0x14, patience = 0x5d, leniency = 0x63, pride = 0x65, greed = 0x67;
}
constexpr std::uint32_t find_contract_for_club = 0x10082f90, contract_value = 0x10084be0, set_flag = 0x1037f7d0, clear_flag = 0x1037fa80,
                        fixture_days = 0x10718130, injury_of = 0x1018c340, contract_answer = 0x1007c140,
                        shortlist_a = 0x10363b80, shortlist_b = 0x10363ca0, leave_club = 0x100eaf30;

// A byte that moves by `delta` and stays within -100..100 (a sum past either bound stores the bound).
void move_clamped(Cpu& c, std::uint32_t at, std::int32_t delta) {
    const auto value = c.u8(at);
    const auto sum = std::int32_t(std::int8_t(value)) + delta;
    if (sum > 100) c.put8(at, 100);
    else if (sum < -100) c.put8(at, 0x9c);
    else c.put8(at, value + delta);
}

// The draws of this routine, which holds them in r13 (the range) and r14 (the result) while the date and table
// words it started with are no longer needed.
struct Rolls {
    Cpu& c;
    HalfwordDraw draw;
    explicit Rolls(Cpu& cpu) : c(cpu), draw(cpu) {}
    // A raw sample (the refill call, when the table runs out, returns to `back`).
    std::int32_t sample(std::uint32_t back) const {
        draw.advance(back);
        return std::int32_t(draw.value());
    }
    // A draw below the byte `range` (negative ranges give negative results, 0 gives 0 without a sample). The
    // game's own wider branch for ranges past 0xffff cannot be reached from a byte.
    std::int32_t below(std::int8_t range, std::uint32_t back) const {
        keep_signed(c, 13, range);
        std::int32_t result = 0;
        if (range != 0) {
            const auto value = std::int32_t(sample(back));
            const auto rest = value - (value / range) * range;
            result = range > 0 ? rest : -rest;
        }
        keep_signed(c, 14, result);
        return result;
    }
};
}

void advance_contract_stage(Cpu& c, std::uint32_t manager, std::uint32_t person_record, std::uint32_t club, std::uint32_t amount,
                            std::uint32_t flag_out, std::uint32_t stage_word, std::uint32_t reasons, std::uint32_t clear_note) {
    Frame frame(c, 0x103f6240, 0xd0, {.first_gpr = 13});
    const auto date_cell = c.toc(-0x7b80), table = c.toc(-0x7bb0), seed = c.toc(-0x7b90), start = c.toc(-0x7b8c),
               limit = c.toc(-0x7b88), cursor = c.toc(-0x7b84), floats = c.toc(-0x4ba0), contracts = c.toc(-0x7b7c),
               texts = c.toc(-0x4b9c);
    keep(c, 17, club);
    keep(c, 14, table);
    keep(c, 13, date_cell);
    keep(c, 15, manager);
    keep(c, 25, seed);
    keep(c, 16, person_record);
    keep(c, 26, start);
    keep(c, 18, amount);
    keep(c, 27, limit);
    keep(c, 19, flag_out);
    keep(c, 28, cursor);
    keep(c, 20, stage_word);
    keep(c, 29, floats);
    keep(c, 21, reasons);
    keep(c, 30, contracts);
    keep(c, 22, clear_note);
    keep(c, 31, texts);
    keep(c, 24, 0);
    const auto stage = std::int8_t(stage_word), reason_bits = std::int8_t(reasons);
    const Rolls roll(c);
    const auto p = person_record;
    const auto set_stage_flag = [&] { c.put8(flag_out, 1); };

    bool club_is_flagged = false;
    if (club != 0) {
        const auto finance = invoke(c, 0x10103510, 0x103f62ac, {c.u32(c.toc(-0x7ab8)), club});
        if (c.u8(finance + 0x165) == 1) { club_is_flagged = true; keep(c, 24, 1); }
    }
    if (c.u32(p + person::amount) != 0) invoke(c, 0x10775550, 0x103f62d0, {c.u32(p + person::amount) - amount});
    const auto record = invoke(c, find_contract_for_club, 0x103f62e0, {c.u32(contracts), p, club});
    keep(c, 23, record);
    if (record == 0) {
        // No contract: reported, and the error flag cleared; the shortlist calls are skipped.
        invoke(c, 0x10739890, 0x103f6300, {texts + 0x37, 0, 0, frame.at(0x40), frame.at(0x38)});
        const auto line = invoke(c, 0x10768810, 0x103f6304);
        invoke(c, 0x1077b720, 0x103f6320, {c.toc(-0x7ba8), texts + 0x6ff, line, frame.at(0x40), frame.at(0x38), 0x2f0b});
        invoke(c, 0x10769b60, 0x103f6330, {texts + 0x70c, c.toc(-0x7ba8), 0});
        c.put8(c.toc(-0x7bac), 0);
        return;
    }
    // The amount is the person's own: kept when the contract is the kind that can wait (kind 2) and the person has
    // a profile, unless the club is flagged and what the contract is worth is already above it.
    if ((c.i8(record + contract::kind) >> 2) == 2 && c.u32(p + person::profile) != 0) {
        bool take = true;
        if (club_is_flagged) {
            const auto row = c.u32(table + std::uint32_t(c.i16(table + 0x2c58)) * 0x2c0 + 4);
            const auto worth = std::int32_t(invoke(c, contract_value, 0x103f6398, {c.u32(contracts), p, c.u32(row + 0x24), 0, 0xffffffff, 1}));
            take = std::int32_t(amount) <= worth;
        }
        if (take) c.put32(p + person::amount, amount);
    }
    if (stage == 7 && (c.u8(record + contract::stage) & 0xf) != 7) move_clamped(c, record + contract::mood_a, -100);

    const auto finish = [&] {
        // The stage nibble is the last thing stored; the note flag is cleared unless the club is flagged.
        c.put8(record + contract::stage, (stage_word & 0xf) | (c.u8(record + contract::stage) & 0xf0));
    };
    const auto end = [&] {
        if (std::int8_t(clear_note) != 0 && !club_is_flagged) c.put8(record + contract::note, c.u8(record + contract::note) & 0xfb);
        invoke(c, shortlist_a, 0x103f80c0, {c.u32(manager + 0x988), p, 0});
        invoke(c, shortlist_b, 0x103f80d0, {c.u32(manager + 0x988), p, 0});
    };
    if (stage == -1) { end(); return; }

    const auto nibble = std::int32_t(c.u8(record + contract::stage) & 0xf);
    if (nibble == 0) {
        // From no stage: the mood of each stage 1..7 (reason bits 1 and 2 raise the flag for the person).
        if (std::uint32_t(std::int32_t(stage)) < 8) {
            const bool talks = (reason_bits & 3) != 0;
            switch (stage) {
            case 1:
                if (talks) { move_clamped(c, record + contract::mood_b, -100); set_stage_flag(); }
                else move_clamped(c, record + contract::mood_b, 100);
                break;
            case 2:
                if (talks) { move_clamped(c, record + contract::mood_b, -100); set_stage_flag(); }
                else move_clamped(c, record + contract::mood_b, 50);
                break;
            case 3:
                if (talks) { move_clamped(c, record + contract::mood_b, -100); set_stage_flag(); }
                else move_clamped(c, record + contract::mood_b, 25);
                break;
            case 4:
                if (talks) { move_clamped(c, record + contract::mood_b, -100); set_stage_flag(); }
                break;
            case 5:
                if (reasons & 1) { move_clamped(c, record + contract::mood_b, -50); set_stage_flag(); }
                break;
            case 6: move_clamped(c, record + contract::mood_b, -20); break;
            case 7: move_clamped(c, record + contract::mood_b, -100); break;
            default: break;
            }
        }
    } else if (std::int8_t(nibble) < stage) {
        auto step = std::int16_t((nibble - stage) * 0x19);
        if (nibble == 3 && stage == 4) step = -0x32;
        if (step < -0x7d) step = -0x7d;
        move_clamped(c, record + contract::mood_b, std::int8_t(step));
    }

    const auto days = std::int32_t(invoke(c, fixture_days, 0x103f67c8, {date_cell, c.u32(c.toc(-0x7a9c)), c.u32(c.toc(-0x7a9c) + 4)}));
    if (days >= 7) {
        // Near a fixture, the person acts on the new stage.
        const auto settle_without_roll = [&] { invoke(c, clear_flag, 0x103f6ad4, {record, 0x200, 0}); };
        if (stage == 1) {
            if (c.u32(record + contract::flags) & 0x4000) {
                invoke(c, set_flag, 0x103f67fc, {record, 0x4000});
                if (!(roll.below(c.i8(p + person::greed), 0x103f6830) != 0 &&
                      roll.below(c.i8(p + person::pride), 0x103f6994) != 0))
                    settle_without_roll();
            }
        } else if ((c.u8(record + contract::stage) & 0xf) != 0 && std::int8_t(c.u8(record + contract::stage) & 0xf) < stage) {
            // The person's mood towards the stage (the original keeps it in r14 as a number around 32).
            std::int32_t mood = 0x20;
            keep_signed(c, 14, mood);
            if (roll.sample(0x103f6b14) % 3 + 0xf < c.i8(p + person::pride)) mood = 0x1f;
            else if (c.i8(p + person::pride) < roll.sample(0x103f6b80) % 3 + 5) mood = 0x21;
            keep_signed(c, 14, mood);
            if (roll.sample(0x103f6bec) % 3 + 0xf < c.i8(p + person::greed)) mood -= 1;
            else if (c.i8(p + person::greed) < roll.sample(0x103f6c58) % 3 + 5) mood += 1;
            keep_signed(c, 14, mood);
            const auto profile = c.u32(p + person::profile);
            if (profile != 0 && c.i8(profile + 0xf) > 0xf) mood += 1;
            if (profile != 0 && c.i8(profile + 0x15) > 0xf) mood -= 1;
            keep_signed(c, 14, mood);
            const auto injury = invoke(c, injury_of, 0x103f6cf0, {c.u32(c.toc(-0x7ba0)), p});
            if (injury != 0) {
                const auto weeks = c.i16(injury + 0x34);
                if (weeks > 0xb4) mood -= 8;
                else if (weeks > 0x5a && c.i8(p + person::greed) > 5) mood -= 5;
                if (weeks > 0x1e) mood -= 2;
                keep_signed(c, 14, mood);
            }
            const auto record_stage = std::int32_t(c.u8(record + contract::stage) & 0xf);
            const bool patient = c.i8(p + person::patience) < std::int8_t(mood);
            // Greed and pride both rolled nonzero.
            const auto both = [&](std::uint32_t greed_back, std::uint32_t pride_back) {
                return roll.below(c.i8(p + person::greed), greed_back) != 0 && roll.below(c.i8(p + person::pride), pride_back) != 0;
            };
            if (record_stage == 1 && patient) {
                // 0x103f70e4: a one-in-five roll, then a roll on leniency that may clear the flag.
                const auto five_in_ten = [&] {
                    if (roll.sample(0x103f7104) % 5 == 0 && roll.below(c.i8(p + person::leniency), 0x103f7170) != 0)
                        invoke(c, clear_flag, 0x103f72b0, {record, 0x200, 0});
                };
                bool to_five_in_ten = stage == 2;
                if (!to_five_in_ten) {
                    if (both(0x103f6d98, 0x103f6efc)) to_five_in_ten = roll.sample(0x103f704c) % 5 != 0;
                    if (!to_five_in_ten) {
                        invoke(c, leave_club, 0x103f7094, {p, club});
                        invoke(c, clear_flag, 0x103f70a4, {record, 0x4000, 0});
                        move_clamped(c, record + contract::mood_b, -5);
                    }
                }
                if (to_five_in_ten) five_in_ten();
            } else if (!(stage == 3 && record_stage == 2 && c.i16(club + 0x1ec) >= 0x1676)) {
                const auto waited = std::int32_t(invoke(c, fixture_days, 0x103f72e8,
                                                        {date_cell, c.u32(record + contract::since), c.u32(record + contract::since + 4)}));
                if (waited < c.i8(p + person::leniency) * 5 + 0x1e) {
                    invoke(c, clear_flag, 0x103f7310, {record, 0x200, 0});
                } else {
                    const auto answer = std::int8_t(invoke(c, contract_answer, 0x103f7324, {c.u32(contracts), p, record}));
                    if (answer == 2) {
                        if (roll.sample(0x103f76e0) % 10 != 0 && !both(0x103f774c, 0x103f78b0))
                            invoke(c, clear_flag, 0x103f79f0, {record, 0x200, 0});
                    } else if (answer == 1) {
                        if (!(both(0x103f7384, 0x103f74e8) && roll.sample(0x103f7638) % 3 != 0)) {
                            invoke(c, clear_flag, 0x103f7680, {record, 0x200, 0});
                            move_clamped(c, record + contract::mood_b, -10);
                        }
                    } else if (answer == 5) {
                        if (c.i8(p + person::leniency) > 0xf && roll.sample(0x103f7d7c) % 3 != 0 && !both(0x103f7de4, 0x103f7f48))
                            invoke(c, clear_flag, 0x103f8088, {record, 0x200, 0});
                    } else if (answer > 2 && answer < 5 && patient) {
                        if (roll.sample(0x103f7a28) % 5 != 0 && c.i8(p + person::leniency) > 10 && !both(0x103f7aa4, 0x103f7c08))
                            invoke(c, clear_flag, 0x103f7d48, {record, 0x200, 0});
                    }
                }
            }
        }
    }
    finish();
    end();
}
}
