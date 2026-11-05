#include "national_teams_adapter.hpp"
#include "guest_abi.hpp"
#include "guest_saved_registers.hpp"
#include "routine_check.hpp"

#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
bool declining = std::getenv("CM_NATIONAL_TEAMS_ORIGINAL") != nullptr;
constexpr auto selection_context_entry = 0x102ae9c0u;
constexpr auto selection_flag_entry = 0x102aea20u, clear_selection_flag_entry = 0x102aea50u;
constexpr auto selection_value_entry = 0x102af620u, selection_slot_entry = 0x102af650u;
constexpr auto selection_tier_entry = 0x102aea80u, selection_prefix_entry = 0x102aeb40u;
constexpr auto international_competition_entry = 0x102aed80u;
constexpr auto assigned_staff_entry = 0x102b7450u;
constexpr unsigned nation_record_size = 0x74, selection_slot_size = 0x14, staff_record_size = 0x6c;
constexpr unsigned nation_index_offset = 0x68, selection_slots_offset = 0x40;
constexpr unsigned selection_value_offset = 0xa, selection_flag_offset = 0xb;
constexpr int unassigned = -1;

void block(Cpu& c, std::uint32_t pc, unsigned count) {
    c.s.step(pc);
    c.s.charge(count - 1);
}

bool record_nonzero_byte(Cpu& c, std::uint32_t raw, unsigned output = 0) {
    const auto value = std::int8_t(raw);
    c.set(output, std::uint32_t(value));
    const auto high = c.cr(0) & 0xf0;
    c.compare(0, value, 0);
    c.set_cr(0, high | c.cr(0));
    return value != 0;
}

void selection_context(Cpu& c) {
    GuestNativeScope scope(c.s, selection_context_entry);
    block(c, selection_context_entry, 7);
    const auto caller = c.sp(), frame = caller - 0x40, mode = c.r(5);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set({{31, mode}, {3, c.r(4)}});
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.call(0x100ce250, 0x102ae9dc);

    block(c, 0x102ae9dc, 2);
    const bool national = record_nonzero_byte(c, c.r(3));
    bool alternate = true;
    if (!national) {
        block(c, 0x102ae9e4, 2);
        alternate = record_nonzero_byte(c, mode);
    }
    if (national || alternate) {
        block(c, 0x102ae9f4, 2);
        if (record_nonzero_byte(c, mode)) {
            block(c, 0x102ae9fc, 2);
            c.set(3, 1);
        } else {
            block(c, 0x102aea04, 1);
            c.set(3, 2);
        }
    } else {
        block(c, 0x102ae9ec, 2);
        c.set(3, 0);
    }

    block(c, 0x102aea08, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

// The nation pool unlocks the first tier at 100 (50 in alternate mode).
// The assessment bytes are signed, and each tier stops at the first score
// that decides it. The guest can observe the final comparison and load.
void selection_tier(Cpu& c) {
    block(c, selection_tier_entry, 2);
    const bool alternate = record_nonzero_byte(c, c.r(6));
    block(c, alternate ? 0x102aea9c : 0x102aea88, 3);
    const auto pool_size = c.i16(c.r(5));
    const auto minimum = alternate ? 50 : 100;
    c.set(0, std::uint32_t(pool_size));
    c.compare(0, pool_size, minimum);
    if (pool_size < minimum) {
        block(c, alternate ? 0x102aeaa8 : 0x102aea94, 2);
        c.set(3, 0);
        c.finish();
        return;
    }

    block(c, 0x102aeab0, 5);
    const auto assessment = c.u32(c.r(4) + 0x14);
    c.set(3, assessment);
    const auto first_score = c.i8(assessment + 0xf);
    c.set(0, std::uint32_t(first_score));
    c.compare(0, first_score, 15);
    if (first_score >= 15) {
        block(c, 0x102aeac4, 2);
        c.set(3, 0);
        c.finish();
        return;
    }
    constexpr std::array score_blocks{0x102aeaccu, 0x102aeadcu, 0x102aeaf4u, 0x102aeb04u, 0x102aeb14u};
    for (unsigned score = 0; score < score_blocks.size(); ++score) {
        block(c, score_blocks[score], 4);
        const auto value = c.i8(assessment + 0x10 + score);
        c.set(0, std::uint32_t(value));
        c.compare(0, value, 15);
        if (value >= 15) {
            const bool second_tier = score >= 2;
            block(c, second_tier ? 0x102aeb24 : 0x102aeaec, 2);
            c.set(3, second_tier ? 2 : 1);
            c.finish();
            return;
        }
    }
    block(c, 0x102aeb2c, 2);
    c.set(3, 3);
    c.finish();
}

void selection_prefix(Cpu& c) {
    block(c, selection_prefix_entry, 7);
    const auto records = c.r(4), raw_count = c.r(6);
    const auto group = std::int8_t(c.r(5)), count = std::int8_t(raw_count);
    c.set(3, std::uint32_t(group));
    record_nonzero_byte(c, raw_count);
    const auto group_offset = std::uint32_t(group) * 0x18;
    c.set({{9, group_offset}, {11, 0}});
    const auto row = records + group_offset;
    c.set(3, row);
    auto total = c.u32(row + 8);
    c.set(3, total);
    if (count <= 0) {
        c.finish();
        return;
    }

    block(c, 0x102aeb5c, 4);
    constexpr unsigned batch_size = 8, stride = 4, first_weight = 0xf;
    c.set(5, raw_count - batch_size);
    c.compare(0, count, batch_size);
    const auto weights_to_sum = unsigned(count);
    unsigned used = 0;
    if (weights_to_sum > batch_size) {
        block(c, 0x102aeb6c, 3);
        const auto batch_limit = weights_to_sum - batch_size;
        c.set({{0, batch_limit}, {8, 0}});
        while (true) {
            block(c, 0x102aebcc, 3);
            c.set(5, used);
            c.compare(0, used, batch_limit);
            if (used >= batch_limit) break;
            block(c, 0x102aeb78, 21);
            const auto offset = used * stride;
            const auto weights = row + offset + first_weight;
            used += batch_size;
            c.set({{5, group_offset + offset}, {8, used * stride}, {10, weights}, {11, used}});
            auto previous = c.u8(weights);
            c.set(7, previous);
            for (unsigned j = 1; j < batch_size; ++j) {
                const auto next = c.u8(weights + j * stride);
                c.set(j % 2 ? 5 : 7, next);
                total += previous;
                c.set(3, total);
                previous = next;
            }
            total += previous;
            c.set(3, total);
        }
    }
    block(c, 0x102aebd8, 4);
    c.set({{5, used}, {0, std::uint32_t(count)}, {6, used * stride}});
    while (true) {
        block(c, 0x102aec00, 3);
        c.set(5, used);
        c.compare(0, used, count);
        if (used >= weights_to_sum) break;
        block(c, 0x102aebe8, 6);
        const auto offset = group_offset + used * stride;
        c.set({{5, offset + first_weight}, {6, (used + 1) * stride}, {11, used + 1}});
        const auto weight = c.u8(records + offset + first_weight);
        total += weight;
        c.set({{5, weight}, {3, total}});
        ++used;
    }
    block(c, 0x102aec0c, 1);
    c.finish();
}

void international_competition(Cpu& c) {
    block(c, international_competition_entry, 2);
    const auto competition = c.r(4);
    c.compare_logical(0, competition, 0);
    if (competition) {
        constexpr std::array international_ids{-0x7bc0, -0x7a40, -0x6948, -0x694c, -0x6950,
                                                -0x693c, -0x6a3c, -0x6938, -0x6b00};
        block(c, 0x102aed88, 5);
        auto slot = c.toc(international_ids.front());
        c.set(3, slot);
        const auto id = c.u32(competition);
        c.set(4, id);
        for (unsigned i = 0; i < international_ids.size(); ++i) {
            if (i != 0) {
                block(c, 0x102aed9c + 16 * (i - 1), 4);
                slot = c.toc(international_ids[i]);
                c.set(3, slot);
            }
            const auto international_id = c.u32(slot);
            c.set(0, international_id);
            c.compare(0, id, international_id);
            if (id == international_id) {
                block(c, 0x102aee1c, 2);
                c.set(3, 1);
                c.finish();
                return;
            }
        }
    }
    block(c, 0x102aee24, 2);
    c.set(3, 0);
    c.finish();
}

enum class SelectionAccess { flag, clear_flag, value, address };

template<SelectionAccess Access>
void selection_slot(Cpu& c) {
    constexpr auto entry = Access == SelectionAccess::flag ? selection_flag_entry :
        Access == SelectionAccess::clear_flag ? clear_selection_flag_entry :
        Access == SelectionAccess::value ? selection_value_entry : selection_slot_entry;
    block(c, entry, Access == SelectionAccess::clear_flag ? 10 : 9);
    const auto nation = c.u32(c.r(4) + nation_index_offset);
    c.set(4, nation);
    const auto displacement = std::uint32_t(std::int8_t(c.r(5))) * selection_slot_size;
    c.set(0, displacement);
    const auto records = c.u32(c.r(3) + 4);
    c.set(5, records);
    const auto nation_offset = nation * nation_record_size;
    if constexpr (Access == SelectionAccess::address) {
        c.set(3, records + nation_offset + displacement + selection_slots_offset);
    } else {
        const auto slot = records + nation_offset + displacement;
        c.set(3, slot);
        if constexpr (Access == SelectionAccess::clear_flag) {
            c.set(6, 0);
            c.put8(slot + selection_slots_offset + selection_flag_offset, 0);
        } else {
            constexpr auto field = Access == SelectionAccess::flag ? selection_flag_offset : selection_value_offset;
            c.set(3, c.u8(slot + selection_slots_offset + field));
        }
    }
    c.finish();
}

void assigned_staff(Cpu& c) {
    block(c, assigned_staff_entry, 2);
    const auto nation = c.r(4);
    c.compare_logical(0, nation, 0);
    if (nation) {
        block(c, 0x102b7458, 7);
        const auto nation_id = c.u32(nation + nation_index_offset);
        c.set(0, nation_id);
        const auto records = c.u32(c.r(3) + 4);
        c.set(3, records);
        const auto nation_offset = nation_id * nation_record_size;
        c.set(0, nation_offset);
        c.set(3, records + nation_offset);
        const auto staff = c.u32(records + nation_offset + 4);
        c.set(0, staff);
        c.compare(0, std::int32_t(staff), unassigned);
        if (std::int32_t(staff) > unassigned) {
            block(c, 0x102b7474, 5);
            const auto slot = c.toc(-0x7bbc);
            c.set(3, slot);
            c.set(0, staff * staff_record_size);
            c.set(3, c.u32(slot) + c.r(0));
            c.finish();
            return;
        }
    }
    block(c, 0x102b7488, 2);
    c.set(3, 0);
    c.finish();
}
constexpr auto international_club_entry = 0x102b7c00u, duty_conflict_entry = 0x102b7c70u;
constexpr auto remove_selection_entry = 0x102b8180u, selected_for_nation_entry = 0x102aefb0u;
constexpr auto deduplicate_rosters_entry = 0x102ace30u, drop_roster_player_entry = 0x102b8d60u;
constexpr auto classify_club = 0x100c7d00u;
constexpr int first_all_star_id = -0x7208, second_all_star_id = -0x7204;
constexpr unsigned club_size = 0x248, club_roster = 0x98, club_roster_size = 50;
constexpr unsigned player_nation = 0xc, player_assessment = 0x14;
constexpr unsigned player_national_team = 0x20, player_duty_flags = 0x6a;
constexpr unsigned player_call_up_status = 0x60;

void international_club(Cpu& c) {
    GuestNativeScope scope(c.s, international_club_entry);
    block(c, international_club_entry, 7);
    const auto caller = c.sp(), club = c.r(4), frame = caller - 0x40;
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, club);
    c.put32(caller + 8, c.r(0));
    c.set(3, club);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.call(classify_club, 0x102b7c1c);
    block(c, 0x102b7c1c, 2);
    if (record_nonzero_byte(c, c.r(3))) {
        block(c, 0x102b7c24, 2);
        c.set(3, 1);
    } else {
        block(c, 0x102b7c2c, 5);
        c.set(3, c.toc(second_all_star_id));
        const auto id = c.u32(club);
        c.set(4, id);
        auto reserved_id = c.u32(c.r(3));
        c.set(0, reserved_id);
        c.compare(0, id, reserved_id);
        if (id != reserved_id) {
            block(c, 0x102b7c40, 4);
            c.set(3, c.toc(first_all_star_id));
            reserved_id = c.u32(c.r(3));
            c.set(0, reserved_id);
            c.compare(0, id, reserved_id);
        }
        const bool reserved = id == reserved_id;
        block(c, reserved ? 0x102b7c50 : 0x102b7c58, reserved ? 2 : 1);
        c.set(3, reserved);
    }
    block(c, 0x102b7c5c, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

// The tier query returns at most three, so these inlined prefix scans never
// reach the original's generic eight-weight batches. Keep the two scans'
// register effects and accounting at their original boundaries.
std::uint32_t selection_queue_start(Cpu& c, std::uint32_t nation, unsigned tier,
                                    std::uint32_t sum, bool second_pass) {
    const auto entry = second_pass ? 0x102b8340u : 0x102b8280u;
    const unsigned index_reg = second_pass ? 10 : 8;
    const unsigned tier_reg = second_pass ? 3 : 5;
    const unsigned sum_reg = second_pass ? 4 : 9;
    block(c, entry, 4);
    c.set({{6, 0}, {tier_reg, tier}, {7, 0}});
    for (unsigned i = 0; ; ++i) {
        block(c, entry + 0x24, 3);
        c.set(6, i);
        c.compare(0, i, tier);
        if (i == tier) return sum;
        block(c, entry + 0x10, 5);
        const auto weight_offset = 0xf + i * 4;
        c.set({{6, weight_offset}, {7, (i + 1) * 4}});
        const auto weight = c.u8(nation + weight_offset);
        c.set({{6, weight}, {index_reg, i + 1}});
        sum += weight;
        c.set(sum_reg, sum);
    }
}

void remove_from_selection_queue(Cpu& c, std::uint32_t manager, std::uint32_t player,
                                  std::uint32_t nation, std::uint32_t bucket, unsigned tier) {
    block(c, 0x102b81f8, 5);
    const auto base = c.u32(nation + 8);
    c.set(4, base);
    record_nonzero_byte(c, tier, 5);
    c.set({{8, 0}, {9, base}});
    auto offset = base;
    if (tier) {
        block(c, 0x102b820c, 3);
        c.compare(0, tier, 8);
        c.set(5, tier - 8);
        offset = selection_queue_start(c, nation, tier, base, false);
    }

    block(c, 0x102b82b0, 6);
    c.set(5, bucket);
    record_nonzero_byte(c, tier);
    const auto length = c.u8(bucket + 0xe);
    c.set({{0, length}, {10, 0}});
    const auto end = offset + length;
    c.set(0, end);
    auto start = base;
    if (tier) {
        block(c, 0x102b82c8, 4);
        c.set({{6, tier}, {7, tier - 8}});
        c.compare(0, tier, 8);
        start = selection_queue_start(c, nation, tier, base, true);
    }

    block(c, 0x102b8370, 6);
    auto remaining = end - start;
    c.set({{3, remaining}, {7, start}, {6, start * 4}});
    c.s.wr<4>(guest::reg::ctr, remaining);
    c.compare(0, start, end);
    if (std::int32_t(start) >= std::int32_t(end)) return;
    auto index = start;
    do {
        block(c, 0x102b8388, 4);
        auto table = c.u32(manager + 0xc);
        c.set(3, table);
        const auto candidate = c.u32(table + index * 4);
        c.set(3, candidate);
        c.compare_logical(0, candidate, player);
        if (candidate == player) {
            block(c, 0x102b8398, 10);
            c.set(4, c.u8(bucket + 0xe));
            c.set({{3, end * 4}, {0, index * 4}, {4, c.r(4) - 1}});
            c.put8(bucket + 0xe, c.r(4));
            table = c.u32(manager + 0xc);
            c.set({{4, table}, {3, table + end * 4}});
            const auto last_player = c.u32(table + (end - 1) * 4);
            c.set(3, last_player);
            c.put32(table + index * 4, last_player);
            return;
        }
        block(c, 0x102b83c0, 3);
        ++index;
        c.set({{6, index * 4}, {7, index}});
        c.s.wr<4>(guest::reg::ctr, --remaining);
    } while (remaining);
}

void unlink_national_roster(Cpu& c, std::uint32_t player) {
    block(c, 0x102b83cc, 3);
    const auto team = c.u32(player + player_national_team);
    c.set(5, team);
    c.compare_logical(0, team, 0);
    if (!team) return;

    block(c, 0x102b83d8, 3);
    c.set({{4, 0}, {3, 0}});
    for (unsigned slot = 0; ; ++slot) {
        block(c, 0x102b8418, 3);
        c.set(0, slot);
        c.compare(0, slot, club_roster_size);
        if (slot == club_roster_size) break;
        block(c, 0x102b83e8, 4);
        const auto offset = slot * 4;
        c.set(0, club_roster + offset);
        const auto listed = c.u32(team + club_roster + offset);
        c.set(0, listed);
        c.compare_logical(0, listed, player);
        if (listed == player) {
            block(c, 0x102b83f8, 6);
            c.set({{4, 0}, {0, offset}, {3, team + offset}});
            c.put32(team + club_roster + offset, 0);
            break;
        }
        block(c, 0x102b8410, 2);
        c.set({{3, offset + 4}, {4, slot + 1}});
    }
    block(c, 0x102b8424, 5);
    const auto retained = c.u8(player + player_duty_flags) & 3u;
    c.set({{0, 0}, {3, retained}});
    c.put8(player + player_duty_flags, retained);
    c.put32(player + player_national_team, 0);
}

void call_national_owner(Cpu& c, std::uint32_t entry, std::uint32_t resume, void (*body)(Cpu&)) {
    if (declining || routine_checker || dispatch_probe) {
        c.call(entry, resume);
    } else {
        c.link(resume);
        body(c);
    }
}

void remove_selection(Cpu& c) {
    GuestNativeScope scope(c.s, remove_selection_entry);
    block(c, remove_selection_entry, 11);
    const auto caller = c.sp(), frame = caller - 0x50;
    const auto manager = c.r(3), player = c.r(4);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.put32(caller - 8, c.r(30));
    c.set(30, player);
    c.put32(caller - 12, c.r(29));
    c.set(29, manager);
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);

    const auto nation = c.u32(player + player_nation);
    c.set(5, nation);
    c.compare_logical(0, nation, 0);
    if (nation) {
        block(c, 0x102b81ac, 3);
        const auto assessment = c.u32(player + player_assessment);
        c.set(0, assessment);
        c.compare_logical(0, assessment, 0);
        if (assessment) {
            block(c, 0x102b81bc, 7);
            const auto nation_index = c.u32(nation + nation_index_offset);
            c.set({{0, nation_index}, {6, 0}});
            const auto records = c.u32(manager + 4);
            c.set(5, records);
            const auto offset = nation_index * nation_record_size;
            const auto record = records + offset;
            c.set({{0, offset}, {31, record}, {5, record}});
            constexpr auto resume = 0x102b81d8;
            call_national_owner(c, selection_tier_entry, resume, selection_tier);

            block(c, resume, 8);
            const auto tier = unsigned(std::int8_t(c.r(3)));
            c.set(6, tier);
            const auto profile = c.u32(player + player_assessment);
            c.set(4, profile);
            const auto bucket = record + tier * 4;
            c.set({{0, tier * 4}, {5, bucket}});
            const auto player_score = c.i16(profile + 8);
            c.set(4, std::uint32_t(player_score));
            const auto threshold = c.i16(bucket + 0x10);
            c.set(5, std::uint32_t(threshold));
            c.compare(0, threshold, player_score);
            if (threshold < player_score) remove_from_selection_queue(c, manager, player, record, bucket, tier);
            unlink_national_roster(c, player);
        } else {
            block(c, 0x102b81b8, 1);
        }
    }

    block(c, 0x102b8438, 7);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.set(29, c.u32(caller - 12));
    c.finish();
}

void selected_for_nation(Cpu& c) {
    GuestNativeScope scope(c.s, selected_for_nation_entry);
    block(c, selected_for_nation_entry, 8);
    const auto caller = c.sp(), frame = caller - 0x40;
    const auto player = c.r(4), nation = c.r(5), raw_mode = c.r(6);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, player);
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    const auto team = c.u32(player + player_national_team);
    c.set(0, team);
    c.compare_logical(0, team, 0);
    if (!team) {
        block(c, 0x102aefd0, 2);
        c.set(3, 0);
    } else {
        block(c, 0x102aefd8, 2);
        const auto mode = std::int8_t(raw_mode);
        if (record_nonzero_byte(c, raw_mode)) {
            block(c, 0x102af008, 3);
            c.compare(0, mode, 1);
            if (mode != 1) {
                block(c, 0x102af03c, 2);
                c.compare(0, mode, 2);
            }
        }
        constexpr std::array call_blocks{0x102aefe0u, 0x102af014u, 0x102af044u};
        bool selected = false;
        if (mode >= 0 && mode < int(call_blocks.size())) {
            const auto pc = call_blocks[mode];
            block(c, pc, 2);
            c.set(3, nation);
            c.call(mode == 2 ? 0x100c7860 : 0x100c76c0, pc + 8);
            block(c, pc + 8, 3);
            const auto current = c.u32(player + player_national_team);
            c.set(0, current);
            c.compare_logical(0, current, c.r(3));
            if (current == c.r(3)) {
                block(c, pc + 20, 3);
                selected = record_nonzero_byte(c, c.u8(player + player_duty_flags) & (mode == 1 ? 8 : 4));
            }
            if (selected) block(c, pc + 32, 2);
        }
        if (!selected) block(c, 0x102af06c, 1);
        c.set(3, selected);
    }

    block(c, 0x102af070, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

void deduplicate_roster(Cpu& c, std::uint32_t team) {
    const auto roster = team + club_roster;
    block(c, 0x102ace6c, 3);
    c.set({{7, roster}, {6, 0}});
    for (unsigned slot = 0; ; ++slot) {
        block(c, 0x102acedc, 3);
        c.set(0, slot);
        c.compare(0, slot, club_roster_size);
        if (slot == club_roster_size) return;
        block(c, 0x102ace78, 3);
        const auto player = c.u32(roster + slot * 4);
        c.set(8, player);
        c.compare_logical(0, player, 0);
        if (player) {
            block(c, 0x102ace84, 3);
            c.set(4, slot);
            c.compare(0, slot, club_roster_size);
            block(c, 0x102ace90, 7);
            const auto first = slot + 1;
            c.set({{0, first}, {4, 0}, {5, first}, {9, roster + first * 4}});
            for (unsigned later = first; ; ++later) {
                block(c, 0x102acec8, 3);
                c.set(0, later);
                c.compare(0, later, club_roster_size);
                if (later == club_roster_size) break;
                block(c, 0x102aceb0, 3);
                const auto address = roster + later * 4;
                const auto candidate = c.u32(address);
                c.set(0, candidate);
                c.compare_logical(0, candidate, player);
                if (candidate == player) {
                    block(c, 0x102acebc, 1);
                    c.put32(address, 0);
                }
                block(c, 0x102acec0, 2);
                c.set({{5, later + 1}, {9, address + 4}});
            }
        }
        block(c, 0x102aced4, 2);
        c.set({{6, slot + 1}, {7, roster + (slot + 1) * 4}});
    }
}

void deduplicate_rosters(Cpu& c) {
    GuestNativeScope scope(c.s, deduplicate_rosters_entry);
    block(c, deduplicate_rosters_entry, 8);
    const auto caller = c.sp(), frame = caller - 0x40, nation = c.r(4);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, 0);
    c.put32(caller - 8, c.r(30));
    c.set(30, nation);
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);

    for (unsigned team_kind = 0; ; ++team_kind) {
        block(c, 0x102aceec, 3);
        c.set(0, team_kind);
        c.compare(0, team_kind, 2);
        if (team_kind == 2) break;
        block(c, 0x102ace50, 2);
        const bool alternate = record_nonzero_byte(c, team_kind);
        const auto pc = alternate ? 0x102ace64u : 0x102ace58u;
        block(c, pc, 2);
        c.set(3, nation);
        c.call(alternate ? 0x100c7860 : 0x100c76c0, pc + 8);
        if (!alternate) block(c, 0x102ace60, 1);
        deduplicate_roster(c, c.r(3));
        block(c, 0x102acee8, 1);
        c.set(31, team_kind + 1);
    }

    block(c, 0x102acef8, 6);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.finish();
}

void drop_roster_player(Cpu& c) {
    GuestNativeScope scope(c.s, drop_roster_player_entry);
    block(c, drop_roster_player_entry, 10);
    const auto caller = c.sp(), frame = caller - 0x50;
    const auto nation = c.r(4), raw_mode = c.r(5), player = c.r(6);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    const auto tactics_slot = c.toc(-0x7a6c);
    c.set(31, tactics_slot);
    c.put32(caller - 8, c.r(30));
    c.put32(caller - 12, c.r(29));
    c.set(29, player);
    c.put32(caller + 8, c.r(0));
    const auto mode = std::int8_t(raw_mode);
    const bool nonzero = record_nonzero_byte(c, raw_mode);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);

    if (nonzero) {
        block(c, 0x102b8d88, 3);
        c.compare(0, mode, 1);
    }
    const bool alternate = mode != 0 && mode != 1;
    const auto lookup = alternate ? 0x102b8da4u : 0x102b8d94u;
    block(c, lookup, 2);
    c.set(3, nation);
    c.call(alternate ? 0x100c7860 : 0x100c76c0, lookup + 8);
    block(c, lookup + 8, alternate ? 1 : 2);
    const auto team = c.r(3);
    c.set(30, team);

    for (const unsigned managed : {1u, 0u}) {
        const auto pc = managed ? 0x102b8db0u : 0x102b8dc8u;
        block(c, pc, 6);
        c.set(3, c.u32(tactics_slot));
        c.set({{4, team}, {5, player}, {6, managed}, {7, 1}});
        c.call(0x103afb90, pc + 24);
    }

    block(c, 0x102b8de0, 3);
    c.set({{4, 0}, {3, 0}});
    for (unsigned slot = 0; ; ++slot) {
        block(c, 0x102b8e38, 3);
        c.set(0, slot);
        c.compare(0, slot, club_roster_size);
        if (slot == club_roster_size) break;
        block(c, 0x102b8df0, 4);
        const auto offset = slot * 4;
        c.set(0, club_roster + offset);
        const auto listed = c.u32(team + club_roster + offset);
        c.set(0, listed);
        c.compare_logical(0, player, listed);
        if (player == listed) {
            block(c, 0x102b8e00, 11);
            c.set({{4, 0}, {0, offset}, {3, team + offset}});
            c.put32(team + club_roster + offset, 0);
            c.put32(player + player_national_team, 0);
            c.put8(player + player_call_up_status, 0);
            const auto flags = c.u8(player + player_duty_flags) & 3u;
            c.set(0, flags);
            c.put8(player + player_duty_flags, flags);
            break;
        }
        block(c, 0x102b8e30, 2);
        c.set({{3, offset + 4}, {4, slot + 1}});
    }

    block(c, 0x102b8e44, 7);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.set(29, c.u32(caller - 12));
    c.finish();
}

constexpr auto club_roster_notice_entry = 0x102b1130u, national_roster_notice_entry = 0x102b1be0u;
constexpr auto replacement_notice_entry = 0x102b16d0u;
constexpr auto add_replacement_entry = 0x102b8c50u, replacement_objection_entry = 0x102b8e60u;
constexpr auto create_news = 0x102bdf90u, set_news_field = 0x102cb4b0u, publish_news = 0x102cc140u;

void roster_notice_header(Cpu& c, std::uint32_t news, std::uint32_t player,
                          std::uint32_t pc, unsigned category) {
    block(c, pc, 4);
    c.set({{3, news}, {4, category}, {5, 0}});
    c.call(create_news, pc + 16);
    block(c, pc + 16, 4);
    c.set(5, c.u32(player));
    c.set({{3, news}, {4, 0}});
    c.call(set_news_field, pc + 32);
    for (unsigned field = 1; field <= 3; ++field) {
        const auto field_pc = pc + 32 + (field - 1) * 20;
        block(c, field_pc, 5);
        const auto name = c.u32(player + 0x24 + field * 4);
        c.set({{5, name}, {3, news}, {4, field}});
        c.set(5, c.u32(name + 0x20));
        c.call(set_news_field, field_pc + 20);
    }
}

void set_roster_injury_field(Cpu& c, std::uint32_t news, std::uint32_t player,
                             std::uint32_t injuries, std::uint32_t pc) {
    block(c, pc, 3);
    c.set({{3, c.u32(injuries)}, {4, player}});
    c.call(0x1018c340, pc + 12);
    block(c, pc + 12, 2);
    c.compare_logical(0, c.r(3), 0);
    if (!c.r(3)) {
        block(c, pc + 68, 4);
        c.set({{3, news}, {4, 6}, {5, 0}});
        c.call(set_news_field, pc + 84);
        return;
    }

    block(c, pc + 20, 3);
    c.set({{3, c.u32(injuries)}, {4, player}});
    c.call(0x1018c340, pc + 32);
    block(c, pc + 32, 4);
    c.set(4, c.r(3));
    c.set(3, c.u32(injuries));
    c.set(4, c.u8(c.r(4)));
    c.call(0x1018c180, pc + 48);
    block(c, pc + 48, 4);
    c.set({{5, std::uint32_t(std::int8_t(c.r(3)))}, {3, news}, {4, 6}});
    c.call(set_news_field, pc + 64);
    block(c, pc + 64, 1);
}

struct ClubNotice {
    std::uint32_t player, nation, mode, scratch, clubs, news_system, contracts, injuries;
};

std::uint32_t contract_club(Cpu& c, const ClubNotice& n, std::uint32_t pc, unsigned output) {
    block(c, pc, 3);
    c.set({{3, c.u32(n.contracts)}, {4, n.player}});
    c.call(0x10082f20, pc + 12);
    block(c, pc + 12, 3);
    const auto contract = c.r(3);
    c.compare_logical(0, contract, 0);
    c.put32(n.scratch, contract);
    if (!contract) {
        block(c, pc + 44, 1);
        c.set(output, 0);
        return 0;
    }
    block(c, pc + 24, 5);
    const auto index = c.u32(contract + 4);
    c.set(0, index);
    const auto clubs = c.u32(n.clubs);
    c.set({{3, clubs}, {0, index * club_size}});
    const auto club = clubs + index * club_size;
    c.set(output, club);
    return club;
}

bool club_receives_notice(Cpu& c, const ClubNotice& n, std::uint32_t pc) {
    block(c, pc, 2);
    c.set(3, c.u32(n.news_system));
    c.call(0x102cdbf0, pc + 8);
    block(c, pc + 8, 2);
    return record_nonzero_byte(c, c.r(3));
}

bool wants_club_notice(Cpu& c, const ClubNotice& n, std::uint32_t pc) {
    const auto current = c.u32(n.player + 0x24);
    c.set(4, current);
    c.compare_logical(0, current, 0);
    if (current && club_receives_notice(c, n, pc)) return true;
    const auto contracted = contract_club(c, n, pc + 16, 0);
    block(c, pc + 64, 2);
    c.compare_logical(0, contracted, 0);
    if (!contracted) return false;
    contract_club(c, n, pc + 72, 4);
    const bool interested = club_receives_notice(c, n, pc + 120);
    if (!interested) block(c, pc + 136, 1);
    return interested;
}

void club_notice_team_fields(Cpu& c, const ClubNotice& n, std::uint32_t message, std::uint32_t pc) {
    block(c, pc, 2);
    const auto mode = std::int8_t(n.mode);
    if (record_nonzero_byte(c, n.mode)) {
        block(c, pc + 56, 3);
        c.compare(0, mode, 1);
    }
    const auto resolve = mode == 0 ? (pc + 8) : mode == 1 ? (pc + 68) : (pc + 116);
    block(c, resolve, 2);
    c.set(3, n.nation);
    c.call(mode == 0 || mode == 1 ? 0x100c76c0 : 0x100c7860, resolve + 8);
    block(c, resolve + 8, 5);
    const auto team = c.r(3);
    c.set({{4, team}, {3, message}});
    c.set(5, c.u32(team));
    c.set(4, 4);
    c.call(set_news_field, resolve + 28);
    block(c, resolve + 28, 4);
    c.set({{3, message}, {4, 5}, {5, mode == 1}});
    c.call(set_news_field, resolve + 44);
    if (mode == 0 || mode == 1) block(c, resolve + 44, 1);
}

void deliver_club_notice(Cpu& c, const ClubNotice& n, std::uint32_t message, std::uint32_t pc) {
    block(c, pc, 3);
    const auto current = c.u32(n.player + 0x24);
    c.set(5, current);
    c.compare_logical(0, current, 0);
    if (current) {
        block(c, pc + 12, 3);
        c.set({{3, c.u32(n.news_system)}, {4, message}});
        c.call(publish_news, pc + 24);
    }
    const auto contracted = contract_club(c, n, pc + 24, 3);
    block(c, pc + 72, 3);
    const auto current_after_send = c.u32(n.player + 0x24);
    c.set(0, current_after_send);
    c.compare_logical(0, contracted, current_after_send);
    if (contracted == current_after_send) return;
    contract_club(c, n, pc + 84, 5);
    block(c, pc + 132, 3);
    c.set({{3, c.u32(n.news_system)}, {4, message}});
    c.call(publish_news, pc + 144);
}

void club_roster_notice(Cpu& c) {
    GuestNativeScope scope(c.s, club_roster_notice_entry);
    block(c, club_roster_notice_entry, 15);
    const auto caller = c.sp(), frame = caller - 0x140;
    const auto player = c.r(4), nation = c.r(5), mode = c.r(6);
    c.set(0, c.lr());
    guest::transfer_saved_registers<24>(c, caller);
    c.set(24, player);
    const auto scratch = c.toc(-0x7b78);
    c.set(27, scratch);
    const auto clubs = c.toc(-0x7be8);
    c.set({{28, clubs}, {25, nation}});
    const auto news_system = c.toc(-0x7bf0);
    c.set({{29, news_system}, {26, mode}});
    const auto contracts = c.toc(-0x7b7c);
    c.set(30, contracts);
    const auto injuries = c.toc(-0x7ba0);
    c.set(31, injuries);
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    const ClubNotice n{player, nation, mode, scratch, clubs, news_system, contracts, injuries};
    if (wants_club_notice(c, n, 0x102b116c)) {
        const auto message = frame + 0x38;
        roster_notice_header(c, message, player, 0x102b11f8, 0x20);
        club_notice_team_fields(c, n, message, 0x102b1254);
        set_roster_injury_field(c, message, player, injuries, 0x102b12f4);
        deliver_club_notice(c, n, message, 0x102b1348);
    }
    block(c, 0x102b13d8, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<24, true>(c, caller);
    c.finish();
}

void replacement_notice(Cpu& c) {
    GuestNativeScope scope(c.s, replacement_notice_entry);
    block(c, replacement_notice_entry, 14);
    const auto caller = c.sp(), frame = caller - 0x140;
    const auto player = c.r(4), nation = c.r(5), mode = c.r(6);
    c.set(0, c.lr());
    guest::transfer_saved_registers<25>(c, caller);
    c.set(25, player);
    const auto scratch = c.toc(-0x7b78);
    c.set(28, scratch);
    const auto clubs = c.toc(-0x7be8);
    c.set({{29, clubs}, {26, nation}});
    const auto news_system = c.toc(-0x7bf0);
    c.set({{30, news_system}, {27, mode}});
    const auto contracts = c.toc(-0x7b7c);
    c.set(31, contracts);
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);

    const ClubNotice n{player, nation, mode, scratch, clubs, news_system, contracts, 0};
    if (wants_club_notice(c, n, 0x102b1708)) {
        const auto message = frame + 0x38;
        roster_notice_header(c, message, player, 0x102b1794, 0x21);
        club_notice_team_fields(c, n, message, 0x102b17f0);
        deliver_club_notice(c, n, message, 0x102b1890);
    }
    block(c, 0x102b1920, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<25, true>(c, caller);
    c.finish();
}

void add_replacement_to_roster(Cpu& c) {
    GuestNativeScope scope(c.s, add_replacement_entry);
    block(c, add_replacement_entry, 11);
    const auto caller = c.sp(), frame = caller - 0x50;
    const auto nation = c.r(4), raw_mode = c.r(5), player = c.r(6);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, player);
    c.put32(caller - 8, c.r(30));
    const auto today = c.toc(-0x7b80);
    c.set(30, today);
    c.put32(caller - 12, c.r(29));
    c.set(29, raw_mode);
    c.put32(caller + 8, c.r(0));
    const auto mode = std::int8_t(raw_mode);
    const bool non_primary = record_nonzero_byte(c, raw_mode);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    if (non_primary) {
        block(c, 0x102b8c7c, 3);
        c.compare(0, mode, 1);
    }
    const bool alternate = mode != 0 && mode != 1;
    const auto resolve = alternate ? 0x102b8c98u : 0x102b8c88u;
    block(c, resolve, 2);
    c.set(3, nation);
    c.call(alternate ? 0x100c7860 : 0x100c76c0, resolve + 8);
    block(c, resolve + 8, alternate ? 1 : 2);
    const auto team = c.r(3);
    c.set(6, team);

    block(c, 0x102b8ca4, 2);
    if (record_nonzero_byte(c, raw_mode)) {
        block(c, 0x102b8cac, 3);
        c.compare(0, mode, 2);
    }
    const auto duty_flag = mode == 0 || mode == 2 ? 4u : 8u;
    block(c, duty_flag == 4 ? 0x102b8cb8 : 0x102b8cc0, duty_flag == 4 ? 2 : 1);
    c.set(4, duty_flag);
    block(c, 0x102b8cc4, 3);
    c.set({{5, 0}, {3, 0}});
    for (unsigned slot = 0; ; ++slot) {
        block(c, 0x102b8d2c, 3);
        c.set(0, slot);
        c.compare(0, slot, club_roster_size);
        if (slot == club_roster_size) break;
        block(c, 0x102b8cd0, 4);
        const auto offset = slot * 4;
        c.set(0, club_roster + offset);
        const auto occupant = c.u32(team + club_roster + offset);
        c.set(0, occupant);
        c.compare_logical(0, occupant, 0);
        if (!occupant) {
            block(c, 0x102b8ce0, 15);
            c.set({{0, 0xb}, {5, team + offset}, {3, player}});
            c.put32(team + club_roster + offset, player);
            c.put32(player + player_national_team, team);
            c.set(5, c.u16(today + 2));
            c.put16(player + 0x3e, c.r(5));
            c.set(5, std::uint32_t(c.i16(today)));
            c.put16(player + 0x3c, c.r(5));
            c.set(5, c.u8(today + 4));
            c.put8(player + 0x40, c.r(5));
            c.put8(player + player_call_up_status, 0xb);
            c.call(0x100ce200, 0x102b8d1c);
            block(c, 0x102b8d1c, 1);
            break;
        }
        block(c, 0x102b8d24, 2);
        c.set({{3, offset + 4}, {5, slot + 1}});
    }
    block(c, 0x102b8d38, 7);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.set(29, c.u32(caller - 12));
    c.finish();
}

void replacement_objection(Cpu& c) {
    GuestNativeScope scope(c.s, replacement_objection_entry);
    block(c, replacement_objection_entry, 7);
    const auto caller = c.sp(), frame = caller - 0x40, player = c.r(4);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, player);
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, c.u32(player + player_nation));
    c.call(0x100c76c0, 0x102b8e7c);
    block(c, 0x102b8e7c, 4);
    const auto team = c.r(3);
    c.set({{0, team}, {3, player}, {4, team}});
    c.call(0x100da050, 0x102b8e8c);
    block(c, 0x102b8e8c, 10);
    const bool objects = std::int8_t(c.r(3)) != 0;
    c.set({{0, objects}, {3, objects}});
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

bool managed_notice_team(Cpu& c, std::uint32_t pc) {
    block(c, pc, 5);
    c.set({{4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, pc + 20);
    block(c, pc + 20, 2);
    return record_nonzero_byte(c, c.r(3));
}

void national_roster_notice(Cpu& c) {
    GuestNativeScope scope(c.s, national_roster_notice_entry);
    block(c, national_roster_notice_entry, 12);
    const auto caller = c.sp(), frame = caller - 0x150, news = frame + 0x38;
    const auto player = c.r(4), nation = c.r(5), raw_mode = c.r(6), reason = c.r(7);
    c.set(0, c.lr());
    guest::transfer_saved_registers<23>(c, caller);
    c.set(30, raw_mode);
    const auto news_system = c.toc(-0x7bf0);
    c.set(31, news_system);
    const auto injuries = c.toc(-0x7ba0);
    c.set({{28, injuries}, {23, player}, {29, nation}, {24, reason}});
    c.put32(caller + 8, c.r(0));
    const auto mode = std::int8_t(raw_mode);
    const bool nonzero = record_nonzero_byte(c, raw_mode);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);

    if (nonzero) {
        block(c, 0x102b1c28, 3);
        c.compare(0, mode, 1);
    }
    const bool alternate = mode != 0 && mode != 1;
    const auto resolve = mode == 0 ? 0x102b1c10u : mode == 1 ? 0x102b1c34u : 0x102b1c4cu;
    block(c, resolve, 2);
    c.set(3, nation);
    c.call(alternate ? 0x100c7860 : 0x100c76c0, resolve + 8);
    block(c, resolve + 8, alternate ? 3 : 4);
    const auto team = c.r(3);
    const unsigned squad_mask = mode == 1 ? 8 : 4;
    c.set({{26, mode == 1}, {27, team}, {25, squad_mask}});
    roster_notice_header(c, news, player, 0x102b1c60, 0x22);
    block(c, 0x102b1cbc, 4);
    c.set(5, c.u32(team));
    c.set({{3, news}, {4, 4}});
    c.call(set_news_field, 0x102b1ccc);
    block(c, 0x102b1ccc, 4);
    c.set({{3, news}, {5, mode == 1}, {4, 5}});
    c.call(set_news_field, 0x102b1cdc);
    set_roster_injury_field(c, news, player, injuries, 0x102b1cdc);

    block(c, 0x102b1d30, 4);
    c.set({{3, news}, {4, 7}, {5, 0}});
    c.call(set_news_field, 0x102b1d40);
    block(c, 0x102b1d40, 4);
    c.set({{3, team}, {4, squad_mask}, {5, 0}});
    c.call(0x100c9070, 0x102b1d50);
    const auto squad_count = std::uint32_t(std::int8_t(c.r(3)));
    const std::array fields{squad_count, std::uint32_t(std::int8_t(reason)), std::uint32_t(mode)};
    for (unsigned field = 0; field < fields.size(); ++field) {
        const auto pc = 0x102b1d50 + field * 16;
        block(c, pc, 4);
        c.set({{5, fields[field]}, {3, news}, {4, 8 + field}});
        c.call(set_news_field, pc + 16);
    }
    block(c, 0x102b1d80, 4);
    c.set(5, c.u32(nation + nation_index_offset));
    c.set({{3, news}, {4, 11}});
    c.call(set_news_field, 0x102b1d90);
    block(c, 0x102b1d90, 6);
    c.set({{3, team}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x102b1da8);
    block(c, 0x102b1da8, 2);
    if (record_nonzero_byte(c, c.r(3))) {
        block(c, 0x102b1db0, 4);
        c.set({{3, c.u32(news_system)}, {5, team}, {4, news}});
        c.call(publish_news, 0x102b1dc0);
        block(c, 0x102b1dc0, 1);
    } else {
        block(c, 0x102b1dc4, 2);
        c.set(3, nation);
        c.call(0x100c76c0, 0x102b1dcc);
        if (managed_notice_team(c, 0x102b1dcc)) {
            block(c, 0x102b1de8, 2);
            c.set(3, nation);
            c.call(0x100c76c0, 0x102b1df0);
            block(c, 0x102b1df0, 4);
            c.set(5, c.r(3));
            c.set({{3, c.u32(news_system)}, {4, news}});
            c.call(publish_news, 0x102b1e00);
        }
    }
    block(c, 0x102b1e00, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<23, true>(c, caller);
    c.finish();
}

struct DutyQuery {
    Cpu& c;
    std::uint32_t frame, manager, player, date, requested_club, label, competition;
    std::uint32_t first_star, second_star, shared_label, clubs, source;
    std::uint32_t list() const { return frame + 0x80; }
};

enum class DutyLabel : unsigned { fixture = 0x7c62, roster = 0x7c8d, all_star = 0x7cb7 };

void label_duty(const DutyQuery& q, std::uint32_t pc, DutyLabel label) {
    auto& c = q.c;
    block(c, pc, 4);
    const auto output = label == DutyLabel::all_star ? q.shared_label : q.label;
    c.set({{3, output}, {4, q.source + unsigned(label)}, {5, q.source + 0x7c8c}});
    c.call(0x10727890, pc + 16);
}

std::uint32_t end_duty(const DutyQuery& q, std::uint32_t pc, bool team) {
    auto& c = q.c;
    block(c, pc, team ? 4 : 3);
    if (team) c.set(21, c.u32(q.player + player_national_team));
    c.set({{3, q.list()}, {4, UINT32_MAX}});
    const auto resume = pc + (team ? 16 : 12);
    c.call(0x10738c20, resume);
    block(c, resume, resume == 0x102b8160 ? 1 : 2);
    return team ? c.r(21) : 0;
}

enum class RequestDisposition { eligible, national, all_star };

RequestDisposition ignored_request(const DutyQuery& q) {
    auto& c = q.c;
    block(c, 0x102b7cb4, 2);
    c.compare_logical(0, q.requested_club, 0);
    if (q.requested_club) {
        block(c, 0x102b7cbc, 2);
        c.set(3, q.requested_club);
        c.call(classify_club, 0x102b7cc4);
        block(c, 0x102b7cc4, 2);
        if (record_nonzero_byte(c, c.r(3))) return RequestDisposition::national;
    }
    block(c, 0x102b7ce0, 2);
    c.compare_logical(0, q.requested_club, 0);
    if (!q.requested_club) return RequestDisposition::eligible;
    block(c, 0x102b7ce8, 4);
    const auto id = c.u32(q.requested_club);
    c.set(3, id);
    auto reserved = c.u32(q.second_star);
    c.set(0, reserved);
    c.compare(0, id, reserved);
    if (id != reserved) {
        block(c, 0x102b7cf8, 3);
        reserved = c.u32(q.first_star);
        c.set(0, reserved);
        c.compare(0, id, reserved);
    }
    return id == reserved ? RequestDisposition::all_star : RequestDisposition::eligible;
}

std::uint32_t build_duty_window(const DutyQuery& q) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7d18, 4);
    c.set({{4, q.date}, {3, frame + 0x60}, {5, 1}});
    c.call(0x10717b20, 0x102b7d28);
    block(c, 0x102b7d28, 10);
    c.set(5, std::uint32_t(c.i16(frame + 0x60)));
    c.set(4, q.date);
    c.set(6, c.u16(frame + 0x62));
    c.set(3, frame + 0x58);
    c.set(0, c.u8(frame + 0x64));
    c.put16(frame + 0x78, c.r(5));
    c.set(5, 1);
    c.put16(frame + 0x7a, c.r(6));
    c.put8(frame + 0x7c, c.r(0));
    c.call(0x10717890, 0x102b7d50);

    block(c, 0x102b7d50, 9);
    const auto national_team = c.u32(q.player + player_national_team);
    c.set(8, national_team);
    c.set(4, std::uint32_t(c.i16(frame + 0x58)));
    c.set(3, c.u16(frame + 0x5a));
    c.compare_logical(0, national_team, 0);
    c.set(0, c.u8(frame + 0x5c));
    c.put16(frame + 0x70, c.r(4));
    c.put16(frame + 0x72, c.r(3));
    c.put8(frame + 0x74, c.r(0));
    return national_team;
}

bool list_next(const DutyQuery& q, std::uint32_t pc, std::uint32_t list) {
    auto& c = q.c;
    block(c, pc, 2);
    c.set(3, list);
    c.call(0x10738e00, pc + 8);
    block(c, pc + 8, 2);
    c.compare_logical(0, c.r(3), 0);
    return c.r(3) != 0;
}

bool fixture_conflict(const DutyQuery& q, std::uint32_t national_team) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7d74, 23);
    const auto flags = c.u8(q.player + 0x6a);
    c.set({{0, flags}, {4, std::uint32_t(-2)}});
    const auto lower_day = std::uint32_t(c.i16(frame + 0x78));
    c.set({{3, lower_day}, {9, q.list()}, {5, std::uint32_t(std::int8_t(flags))}});
    c.put32(frame + 0x38, c.r(4));
    const bool reserve_team = (flags & 4) != 0;
    c.set(5, reserve_team);
    const auto fixture_slot = c.toc(-0x7a60);
    const unsigned check_roster = !reserve_team;
    c.set({{11, fixture_slot}, {26, check_roster}});
    const auto upper_day = std::uint32_t(c.i16(frame + 0x70));
    c.set({{0, upper_day}, {4, check_roster}});
    c.set(5, c.u16(frame + 0x7a));
    c.set(4, check_roster ? 31 : 32);
    c.set(7, c.u16(frame + 0x72));
    c.set({{4, reserve_team}, {10, UINT32_MAX}, {6, reserve_team}});
    c.put32(frame + 0x3c, c.r(6));
    c.set({{4, std::uint16_t(lower_day)}, {6, std::uint16_t(upper_day)}});
    c.set(3, c.u32(fixture_slot));
    c.set(8, c.u32(national_team));
    c.call(0x1048b180, 0x102b7dd0);

    block(c, 0x102b7dd0, 2);
    const auto count = std::uint16_t(c.r(3));
    c.set(0, count);
    const auto high = c.cr(0) & 0xf0;
    c.compare(0, count, 0);
    c.set_cr(0, high | c.cr(0));
    if (!count) return false;
    block(c, 0x102b7dd8, 2);
    c.compare_logical(0, q.competition, 0);
    if (!q.competition) {
        block(c, 0x102b7e40, 2);
        c.compare_logical(0, q.label, 0);
        if (q.label) label_duty(q, 0x102b7e48, DutyLabel::fixture);
        c.set(3, end_duty(q, 0x102b7e58, true));
        return true;
    }
    block(c, 0x102b7de0, 3);
    c.set(0, c.u32(q.list() + 4));
    c.put32(q.list() + 12, c.r(0));
    block(c, 0x102b7e28, 1);
    while (list_next(q, 0x102b7e2c, q.list())) {
        block(c, 0x102b7dec, 3);
        const auto competition = c.u32(c.r(3) + 0x14);
        c.set(0, competition);
        c.compare_logical(0, competition, q.competition);
        if (competition != q.competition) continue;
        block(c, 0x102b7df8, 2);
        c.compare_logical(0, q.label, 0);
        if (q.label) label_duty(q, 0x102b7e00, DutyLabel::fixture);
        c.set(3, end_duty(q, 0x102b7e10, true));
        return true;
    }
    block(c, 0x102b7e3c, 1);
    return false;
}

void publish_day_order(Cpu& c, std::int16_t earlier, std::int16_t later) {
    // The original's signed-day comparison leaves the xor half and carry
    // live, even though the actual decision is an ordinary signed comparison.
    const auto bits = std::int32_t(earlier) ^ std::int32_t(later);
    c.set(3, std::uint32_t(bits >> 1));
    c.s.wr<1>(guest::reg::xer_ca, bits < 0 && (bits & 1));
    c.set(0, earlier < later);
}

bool roster_date_precedes_query(const DutyQuery& q, std::uint32_t roster) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7ebc, 8);
    c.set(4, c.u32(roster + 8));
    c.set(0, c.u32(roster + 12));
    c.put32(frame + 0x50, c.r(4));
    const auto date_year = c.u16(q.date + 2);
    c.set(3, date_year);
    const auto roster_year = c.u16(frame + 0x52);
    c.set(4, roster_year);
    c.put32(frame + 0x54, c.r(0));
    c.compare_logical(0, roster_year, date_year);
    if (roster_year < date_year) {
        block(c, 0x102b7edc, 2);
        c.set(0, 1);
    } else {
        block(c, 0x102b7ee4, 1);
        if (roster_year > date_year) {
            block(c, 0x102b7ee8, 2);
            c.set(0, 0);
        } else {
            block(c, 0x102b7ef0, 8);
            const auto roster_day = c.i16(frame + 0x50);
            c.set(0, std::uint32_t(roster_day));
            const auto date_day = c.i16(q.date);
            c.set(4, std::uint32_t(date_day));
            publish_day_order(c, roster_day, date_day);
        }
    }
    block(c, 0x102b7f10, 2);
    return record_nonzero_byte(c, c.r(0));
}

bool roster_conflict(const DutyQuery& q) {
    auto& c = q.c;
    block(c, 0x102b7e70, 3);
    const auto roster_list = c.u32(q.manager);
    c.set(3, roster_list);
    c.compare_logical(0, roster_list, 0);
    if (!roster_list) return false;
    block(c, 0x102b7e7c, 2);
    if (record_nonzero_byte(c, c.r(26))) return false;
    block(c, 0x102b7e84, 3);
    c.set(0, c.u32(roster_list + 4));
    c.put32(roster_list + 12, c.r(0));
    while (true) {
        block(c, 0x102b7f48, 2);
        c.set(3, c.u32(q.manager));
        c.call(0x10738e00, 0x102b7f50);
        block(c, 0x102b7f50, 2);
        const auto roster = c.r(3);
        c.compare_logical(0, roster, 0);
        if (!roster) return false;

        block(c, 0x102b7e90, 5);
        const auto team = c.u32(q.player + player_national_team);
        c.set(4, team);
        const auto roster_id = c.u32(roster);
        c.set(5, roster_id);
        const auto team_id = c.u32(team);
        c.set(0, team_id);
        c.compare(0, roster_id, team_id);
        if (roster_id != team_id) continue;
        block(c, 0x102b7ea4, 2);
        c.compare_logical(0, q.competition, 0);
        if (q.competition) {
            block(c, 0x102b7eac, 4);
            const auto roster_competition = c.u32(roster + 4);
            c.set(4, roster_competition);
            const auto requested = c.u32(q.competition);
            c.set(0, requested);
            c.compare(0, roster_competition, requested);
            if (roster_competition != requested) continue;
        }
        if (!roster_date_precedes_query(q, roster)) continue;
        block(c, 0x102b7f18, 2);
        c.compare_logical(0, q.label, 0);
        if (q.label) label_duty(q, 0x102b7f20, DutyLabel::roster);
        c.set(3, end_duty(q, 0x102b7f30, true));
        return true;
    }
}

bool all_star_window(const DutyQuery& q) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7f58, 8);
    const auto calendar_index = c.toc(-0x6868);
    c.set(3, calendar_index);
    const auto calendars = c.toc(-0x7b5c);
    c.set(4, calendars);
    c.set(0, c.u32(calendar_index));
    c.set(3, c.u32(calendars));
    c.set(0, c.r(0) * 4);
    const auto calendar = c.u32(c.r(3) + c.r(0));
    c.set(4, calendar);
    c.compare_logical(0, calendar, 0);
    if (!calendar) return false;
    block(c, 0x102b7f78, 9);
    const auto calendar_date = c.u32(calendar + 0xa0);
    c.set({{6, calendar_date}, {3, frame + 0x68}});
    const auto year = c.i16(calendar + 0x40);
    c.set(0, std::uint32_t(year));
    const auto year_delta = c.i8(calendar_date + 6);
    c.set(5, std::uint8_t(year_delta));
    c.set(4, std::uint32_t(c.i16(calendar_date + 4)));
    c.set({{5, std::uint32_t(year_delta)}, {0, std::uint32_t(year + year_delta)}, {5, std::uint16_t(year + year_delta)}});
    c.call(0x107156b0, 0x102b7f9c);

    block(c, 0x102b7f9c, 3);
    const auto club = c.u32(q.player + 0x24);
    c.set(3, club);
    c.compare_logical(0, club, 0);
    if (!club) return false;
    block(c, 0x102b7fa8, 3);
    const auto nation = c.u32(club + 0x30);
    c.set(4, nation);
    c.compare_logical(0, nation, 0);
    if (!nation) return false;
    block(c, 0x102b7fb4, 5);
    c.set(3, c.toc(-0x7b70));
    const auto id = c.u32(nation);
    c.set(4, id);
    const auto selected_nation = c.u32(c.r(3));
    c.set(0, selected_nation);
    c.compare(0, id, selected_nation);
    if (id != selected_nation) return false;

    block(c, 0x102b7fc8, 8);
    c.set(3, c.u32(frame + 0x78));
    c.set(0, c.u32(frame + 0x7c));
    c.put32(frame + 0x48, c.r(3));
    const auto deadline_year = c.u16(frame + 0x6a);
    c.set(5, deadline_year);
    const auto lower_year = c.u16(frame + 0x4a);
    c.set(3, lower_year);
    c.put32(frame + 0x4c, c.r(0));
    c.compare_logical(0, lower_year, deadline_year);
    if (lower_year > deadline_year) {
        block(c, 0x102b7fe8, 2);
        c.set(0, 1);
    } else {
        block(c, 0x102b7ff0, 1);
        if (lower_year < deadline_year) {
            block(c, 0x102b7ff4, 2);
            c.set(0, 0);
        } else {
            block(c, 0x102b7ffc, 1);
            const auto lower_day = c.i16(frame + 0x48);
            c.set(4, std::uint32_t(lower_day));
            block(c, 0x102b8000, 9);
            const auto deadline_day = c.i16(frame + 0x68);
            c.set(0, std::uint32_t(deadline_day));
            publish_day_order(c, deadline_day, lower_day);
        }
    }
    if (lower_year != deadline_year) block(c, 0x102b801c, 2);
    if (record_nonzero_byte(c, c.r(0))) return false;

    block(c, 0x102b8024, 7);
    c.set(3, c.u32(frame + 0x70));
    c.set(0, c.u32(frame + 0x74));
    c.put32(frame + 0x40, c.r(3));
    const auto upper_year = c.u16(frame + 0x42);
    c.set(3, upper_year);
    c.put32(frame + 0x44, c.r(0));
    c.compare_logical(0, upper_year, deadline_year);
    if (upper_year < deadline_year) {
        block(c, 0x102b8040, 2);
        c.set(0, 1);
    } else {
        block(c, 0x102b8048, 1);
        if (upper_year > deadline_year) {
            block(c, 0x102b804c, 2);
            c.set(0, 0);
        } else {
            block(c, 0x102b8054, 8);
            const auto upper_day = c.i16(frame + 0x40);
            c.set(0, std::uint32_t(upper_day));
            const auto deadline_day = c.i16(frame + 0x68);
            c.set(4, std::uint32_t(deadline_day));
            publish_day_order(c, upper_day, deadline_day);
        }
    }
    block(c, 0x102b8074, 2);
    return !record_nonzero_byte(c, c.r(0));
}

std::uint32_t all_star_club(const DutyQuery& q) {
    auto& c = q.c;
    block(c, 0x102b807c, 10);
    const auto first = c.u32(q.first_star);
    c.set({{3, first}, {7, 0}});
    const auto second = c.u32(q.second_star);
    c.set({{0, second}, {4, 0}, {3, first * club_size}});
    const auto clubs = c.u32(q.clubs);
    c.set({{5, clubs}, {0, second * club_size}, {6, clubs + first * club_size}, {5, clubs + second * club_size}});
    for (unsigned row = 0; ; ++row) {
        block(c, 0x102b8148, 3);
        c.set(0, row);
        c.compare(0, row, 50);
        if (row == 50) return end_duty(q, 0x102b8154, false);
        block(c, 0x102b80a8, 4);
        const auto offset = club_roster + row * 4;
        c.set(3, offset);
        const auto first_player = c.u32(clubs + first * club_size + offset);
        c.set(0, first_player);
        c.compare_logical(0, first_player, q.player);
        const bool first_match = first_player == q.player;
        bool match = first_match;
        if (!match) {
            block(c, 0x102b80f8, 3);
            const auto second_player = c.u32(clubs + second * club_size + offset);
            c.set(0, second_player);
            c.compare_logical(0, second_player, q.player);
            match = second_player == q.player;
        }
        if (match) {
            const auto branch = first_match ? 0x102b80b8u : 0x102b8104u;
            block(c, branch, 2);
            c.compare_logical(0, q.label, 0);
            if (q.label) label_duty(q, branch + 8, DutyLabel::all_star);
            const auto dispose = branch + 24;
            block(c, dispose, 7);
            c.set(0, c.u32(first_match ? q.first_star : q.second_star));
            c.set(3, q.list());
            c.set(5, c.u32(q.clubs));
            c.set({{4, UINT32_MAX}, {0, c.r(0) * club_size}});
            c.set(21, c.r(5) + c.r(0));
            c.call(0x10738c20, dispose + 28);
            block(c, dispose + 28, 2);
            return c.r(21);
        }
        block(c, 0x102b8140, 2);
        c.set({{4, (row + 1) * 4}, {7, row + 1}});
    }
}

std::uint32_t resolve_duty(const DutyQuery& q) {
    auto& c = q.c;
    switch (ignored_request(q)) {
    case RequestDisposition::national: return end_duty(q, 0x102b7ccc, false);
    case RequestDisposition::all_star: return end_duty(q, 0x102b7d04, false);
    case RequestDisposition::eligible: break;
    }
    if (const auto national_team = build_duty_window(q)) {
        if (fixture_conflict(q, national_team) || roster_conflict(q)) return c.r(3);
    }
    if (all_star_window(q)) return all_star_club(q);
    return end_duty(q, 0x102b8154, false);
}

void duty_conflict(Cpu& c) {
    GuestNativeScope scope(c.s, duty_conflict_entry);
    block(c, duty_conflict_entry, 17);
    const auto caller = c.sp(), frame = caller - 0xc0;
    const auto manager = c.r(3), player = c.r(4), date = c.r(5), club = c.r(6), label = c.r(7), competition = c.r(8);
    c.set(0, c.lr());
    guest::transfer_saved_registers<21>(c, caller);
    const auto first_star = c.toc(first_all_star_id);
    c.set({{27, first_star}, {21, manager}});
    const auto second_star = c.toc(second_all_star_id);
    c.set({{28, second_star}, {22, player}});
    const auto shared_label = c.toc(-0x7bb8);
    c.set({{29, shared_label}, {23, date}});
    const auto clubs = c.toc(-0x7be8);
    c.set({{30, clubs}, {26, club}});
    const auto source = c.toc(-0x513c);
    c.set({{31, source}, {24, label}, {25, competition}});
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    const DutyQuery query{c, frame, manager, player, date, club, label, competition,
                         first_star, second_star, shared_label, clubs, source};
    c.set(3, query.list());
    c.link(0x102b7cb4);
    c.set(0, 0);
    for (unsigned offset : {4u, 8u, 12u}) c.put32(query.list() + offset, 0);
    c.put16(query.list(), 0);
    c.s.charge(6); // The original inlines its temporary list constructor.

    c.set(3, resolve_duty(query));
    block(c, 0x102b8164, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<21, true>(c, caller);
    c.finish();
}

constexpr auto injured_selection_entry = 0x102b8460u;

struct InjuredSelection {
    std::uint32_t frame, manager, player, team, nation, record, today, tactics;
    unsigned mode;
};

void report_missing_injury_team(Cpu& c, std::uint32_t frame, std::uint32_t source, std::uint32_t buffer) {
    block(c, 0x102b84a0, 6);
    c.set({{3, source + 0x37}, {6, frame + 0x50}, {7, frame + 0x48}, {4, 0}, {5, 0}});
    c.call(0x10739890, 0x102b84b8);
    block(c, 0x102b84b8, 1);
    c.call(0x10768810, 0x102b84bc);
    block(c, 0x102b84bc, 7);
    c.set({{5, c.r(3)}, {3, buffer}, {4, source + 0x4a}, {6, frame + 0x50}, {7, frame + 0x48}, {8, 0x1df8}});
    c.call(0x1077b720, 0x102b84d8);
    block(c, 0x102b84d8, 4);
    c.set({{4, buffer}, {3, source + 0x57}, {5, 0}});
    c.call(0x10769b60, 0x102b84e8);
    block(c, 0x102b84e8, 4);
    c.set(3, c.toc(-0x7bac));
    c.set(0, 0);
    c.put8(c.r(3), 0);
}

unsigned injured_team_mode(Cpu& c, std::uint32_t player, std::uint32_t team) {
    block(c, 0x102b8520, 2);
    c.set(3, team);
    c.call(0x100ce250, 0x102b8528);
    block(c, 0x102b8528, 2);
    if (record_nonzero_byte(c, c.r(3))) {
        block(c, 0x102b8530, 2);
        c.set(29, 2);
        return 2;
    }
    block(c, 0x102b8538, 3);
    const bool reserve = record_nonzero_byte(c, c.u8(player + player_duty_flags) & 8u);
    block(c, reserve ? 0x102b8544 : 0x102b854c, 2);
    c.set(29, reserve);
    return reserve;
}

bool injury_date_before(Cpu& c, std::uint32_t pc, std::uint16_t left_year, std::uint16_t right_year,
                        std::uint32_t left_date, std::uint32_t right_date) {
    if (left_year < right_year) {
        block(c, pc, 2);
        c.set(0, 1);
    } else {
        block(c, pc + 8, 1);
        if (left_year > right_year) {
            block(c, pc + 12, 2);
            c.set(0, 0);
        } else {
            block(c, pc + 20, 8);
            const auto left_day = c.i16(left_date);
            c.set(0, std::uint32_t(left_day));
            const auto right_day = c.i16(right_date);
            c.set(4, std::uint32_t(right_day));
            publish_day_order(c, left_day, right_day);
        }
    }
    block(c, pc + 52, 2);
    return record_nonzero_byte(c, c.r(0));
}

bool injured_selection_in_window(Cpu& c, const InjuredSelection& n, std::uint32_t slot) {
    block(c, 0x102b85c4, 11);
    c.set(0, n.mode);
    c.set(3, c.u32(n.today));
    c.set(26, n.mode * selection_slot_size);
    c.set(0, c.u32(n.today + 4));
    c.put32(n.frame + 0x40, c.r(3));
    c.set(3, slot);
    const auto today_year = c.u16(n.frame + 0x42);
    c.set(5, today_year);
    const auto deadline_year = c.u16(slot + 0x42);
    c.set(4, deadline_year);
    c.put32(n.frame + 0x44, c.r(0));
    c.compare_logical(0, today_year, deadline_year);
    return injury_date_before(c, 0x102b85f0, today_year, deadline_year, n.frame + 0x40, slot + 0x40);
}

bool retained_in_injury_list(Cpu& c, const InjuredSelection& n) {
    block(c, 0x102b862c, 4);
    const auto list = c.u32(n.manager);
    c.set(3, list);
    const auto team = c.u32(n.player + player_national_team);
    c.set(28, team);
    c.compare_logical(0, list, 0);
    bool retained = false;
    if (list) {
        block(c, 0x102b863c, 3);
        c.set(0, c.u32(list + 4));
        c.put32(list + 12, c.r(0));
        block(c, 0x102b86c0, 1);
        for (;;) {
            block(c, 0x102b86c4, 2);
            c.set(3, c.u32(n.manager));
            c.call(0x10738e00, 0x102b86cc);
            block(c, 0x102b86cc, 2);
            const auto item = c.r(3);
            c.compare_logical(0, item, 0);
            if (!item) break;

            block(c, 0x102b864c, 4);
            const auto listed = c.u32(item);
            c.set(4, listed);
            const auto team_id = c.u32(team);
            c.set(0, team_id);
            c.compare(0, listed, team_id);
            if (listed != team_id) continue;

            block(c, 0x102b865c, 8);
            c.set(4, c.u32(item + 8));
            c.set(0, c.u32(item + 12));
            c.put32(n.frame + 0x38, c.r(4));
            const auto today_year = c.u16(n.today + 2);
            c.set(3, today_year);
            const auto item_year = c.u16(n.frame + 0x3a);
            c.set(4, item_year);
            c.put32(n.frame + 0x3c, c.r(0));
            c.compare_logical(0, item_year, today_year);
            if (injury_date_before(c, 0x102b867c, item_year, today_year, n.frame + 0x38, n.today)) {
                block(c, 0x102b86b8, 2);
                retained = true;
                break;
            }
        }
    }
    if (!retained) block(c, 0x102b86d4, 1);
    c.set(0, retained);
    block(c, 0x102b86d8, 2);
    return record_nonzero_byte(c, c.r(0));
}

void remove_injured_player(Cpu& c, const InjuredSelection& n, std::uint32_t pc) {
    for (const unsigned managed : {1u, 0u}) {
        block(c, pc, 6);
        c.set({{3, c.u32(n.tactics)}, {4, n.team}, {5, n.player}, {6, managed}, {7, 1}});
        c.call(0x103afb90, pc + 24);
        pc += 24;
    }
    block(c, pc, 5);
    c.set({{3, n.manager}, {4, n.nation}, {5, n.mode}, {6, n.player}});
    call_national_owner(c, drop_roster_player_entry, pc + 20, drop_roster_player);
}

bool offer_managed_replacement(Cpu& c, const InjuredSelection& n, unsigned tier) {
    block(c, 0x102b876c, 8);
    c.set({{0, tier}, {3, n.team}, {25, tier}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x102b878c);
    block(c, 0x102b878c, 2);
    if (!record_nonzero_byte(c, c.r(3))) {
        block(c, 0x102b8794, 3);
        const auto manager = c.u32(n.team + 0x90);
        c.set(0, manager);
        c.compare_logical(0, manager, 0);
        if (manager) return false;

        block(c, 0x102b87a0, 2);
        c.set(3, n.nation);
        c.call(0x100c76c0, 0x102b87a8);
        if (!managed_notice_team(c, 0x102b87a8)) return false;
    }
    block(c, 0x102b87c4, 2);
    c.set(3, n.nation);
    c.call(0x100c76c0, 0x102b87cc);
    block(c, 0x102b87cc, 3);
    const bool non_primary = record_nonzero_byte(c, n.mode);
    c.set(4, c.u32(c.r(3) + 0x90));
    if (!non_primary) return true;

    block(c, 0x102b87d8, 4);
    c.set(3, c.toc(-0x7a54));
    c.set(5, 0);
    c.set(3, c.u32(c.r(3)));
    c.call(0x101484b0, 0x102b87e8);
    block(c, 0x102b87e8, 2);
    return record_nonzero_byte(c, c.r(3));
}

void replace_injured_selection(Cpu& c, const InjuredSelection& n, std::uint32_t slot) {
    block(c, 0x102b86e0, 5);
    c.set({{3, n.manager}, {4, n.player}, {5, n.nation}, {6, n.mode}});
    call_national_owner(c, club_roster_notice_entry, 0x102b86f4, club_roster_notice);
    remove_injured_player(c, n, 0x102b86f4);

    block(c, 0x102b8738, 13);
    c.set({{6, slot}, {0, n.mode}});
    const auto count = c.u8(slot + 0x4b);
    c.set(4, count);
    const bool alternate = n.mode == 2;
    c.s.wr<1>(guest::reg::xer_ca, 1); // subfic from 2, with mode constrained to 0–2.
    c.set({{3, n.manager}, {5, count + 1u}, {4, n.player}, {0, alternate}});
    c.put8(slot + 0x4b, c.r(5));
    c.set({{5, n.record}, {6, alternate}});
    call_national_owner(c, selection_tier_entry, 0x102b876c, selection_tier);
    const auto tier = c.r(3);
    if (offer_managed_replacement(c, n, tier)) {
        block(c, 0x102b87f0, 6);
        c.set({{3, n.manager}, {4, n.player}, {5, n.nation}, {6, n.mode}, {7, tier}});
        call_national_owner(c, national_roster_notice_entry, 0x102b8808, national_roster_notice);
        block(c, 0x102b8808, 1);
    } else {
        block(c, 0x102b880c, 6);
        c.set({{3, n.manager}, {4, n.nation}, {5, n.mode}, {6, tier}, {7, n.player}});
        c.call(0x102b8850, 0x102b8824);
        block(c, 0x102b8824, 3);
        c.set({{3, n.manager}, {4, n.nation}});
        call_national_owner(c, deduplicate_rosters_entry, 0x102b8830, deduplicate_rosters);
    }
}

void process_injured_selection(Cpu& c, std::uint32_t frame, std::uint32_t manager, std::uint32_t player,
                               std::uint32_t today, std::uint32_t tactics, std::uint32_t source, std::uint32_t buffer) {
    block(c, 0x102b8494, 3);
    const auto team = c.u32(player + player_national_team);
    c.set(27, team);
    c.compare_logical(0, team, 0);
    if (!team) return report_missing_injury_team(c, frame, source, buffer);
    block(c, 0x102b84f8, 3);
    const auto status = c.u8(player + player_call_up_status);
    c.set(0, status);
    c.compare(0, status, 0xb);
    if (status != 0xb) return;

    block(c, 0x102b8504, 7);
    const auto nation = c.u32(team + 0x2c);
    c.set(24, nation);
    c.compare_logical(0, team, 0);
    const auto records = c.u32(manager + 4);
    c.set(3, records);
    const auto nation_index = c.u32(nation + nation_index_offset);
    c.set(0, nation_index * nation_record_size);
    const auto record = records + nation_index * nation_record_size;
    c.set(25, record);
    const auto mode = injured_team_mode(c, player, team);
    const InjuredSelection n{frame, manager, player, team, nation, record, today, tactics, mode};

    block(c, 0x102b8558, 4);
    c.set(3, c.toc(-0x7ba0));
    c.set(4, player);
    c.set(3, c.u32(c.r(3)));
    c.call(0x1018c340, 0x102b8568);
    block(c, 0x102b8568, 2);
    const auto injury = c.r(3);
    c.compare_logical(0, injury, 0);
    if (injury) {
        block(c, 0x102b8570, 3);
        const auto type = c.u8(injury);
        c.set(0, type);
        c.compare(0, type, 0x51);
        if (type == 0x51) {
            remove_injured_player(c, n, 0x102b857c);
            block(c, 0x102b85c0, 1);
            return;
        }
    }
    const auto slot = record + mode * selection_slot_size;
    if (injured_selection_in_window(c, n, slot) && !retained_in_injury_list(c, n)) {
        replace_injured_selection(c, n, slot);
    }
}

void injured_selection(Cpu& c) {
    GuestNativeScope scope(c.s, injured_selection_entry);
    block(c, injured_selection_entry, 13);
    const auto caller = c.sp(), frame = caller - 0xa0;
    const auto manager = c.r(3), player = c.r(4);
    c.set(0, c.lr());
    guest::transfer_saved_registers<22>(c, caller);
    const auto tactics = c.toc(-0x7a6c);
    c.set({{30, tactics}, {22, manager}});
    const auto today = c.toc(-0x7b80);
    c.set({{31, today}, {23, player}});
    const auto buffer = c.toc(-0x7ba8);
    c.set(25, buffer);
    const auto source = c.toc(-0x513c);
    c.set(24, source);
    c.put32(caller + 8, c.r(0));
    const auto duration = std::int16_t(c.r(5));
    c.set(0, std::uint32_t(duration));
    c.compare(0, duration, 14);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    if (duration >= 14) process_injured_selection(c, frame, manager, player, today, tactics, source, buffer);

    block(c, 0x102b8830, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<22, true>(c, caller);
    c.finish();
}

}

void national_teams_run_original(State& state, std::uint32_t entry) {
    run_original_body(state, entry, declining);
}

void national_teams_run_rewrite(State& state, std::uint32_t entry) {
    Cpu c(state);
    switch (entry) {
    case selection_context_entry: selection_context(c); break;
    case selection_flag_entry: selection_slot<SelectionAccess::flag>(c); break;
    case clear_selection_flag_entry: selection_slot<SelectionAccess::clear_flag>(c); break;
    case selection_value_entry: selection_slot<SelectionAccess::value>(c); break;
    case selection_slot_entry: selection_slot<SelectionAccess::address>(c); break;
    case international_club_entry: international_club(c); break;
    case duty_conflict_entry: duty_conflict(c); break;
    case selection_tier_entry: selection_tier(c); break;
    case selection_prefix_entry: selection_prefix(c); break;
    case international_competition_entry: international_competition(c); break;
    case remove_selection_entry: remove_selection(c); break;
    case selected_for_nation_entry: selected_for_nation(c); break;
    case deduplicate_rosters_entry: deduplicate_rosters(c); break;
    case drop_roster_player_entry: drop_roster_player(c); break;
    case club_roster_notice_entry: club_roster_notice(c); break;
    case national_roster_notice_entry: national_roster_notice(c); break;
    case injured_selection_entry: injured_selection(c); break;
    case replacement_notice_entry: replacement_notice(c); break;
    case add_replacement_entry: add_replacement_to_roster(c); break;
    case replacement_objection_entry: replacement_objection(c); break;
    case assigned_staff_entry: assigned_staff(c); break;
    default: throw std::logic_error("Unknown NATIONAL_TEAMS entry " + hex_address(entry));
    }
}

bool try_modern_national_teams(State& state, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(state, entry, national_teams_run_original, national_teams_run_rewrite,
                               RoutineChecker::result_r3);
    national_teams_run_rewrite(state, entry);
    return true;
}
}
