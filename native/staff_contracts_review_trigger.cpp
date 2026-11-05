#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using detail::record_nonzero;
using guest::Cpu;

constexpr auto trigger_entry = 0x1037f110u, get_status_entry = 0x1037fa70u;
constexpr auto staff_stride = 0x70u, club_stride = 0x248u, review_counter = 0x4cu;
constexpr auto availability = 0x65u, morale = 0x66u, response = 0x67u, motivation = 0x69u;

void load_status(Cpu& c, std::uint32_t contract) {
    c.set(3, status_bits(c, contract));
}

bool has_status_flag(Cpu& c, std::uint32_t draft, std::uint32_t site) {
    block(c, site, 2);
    c.set(3, draft);
    detail::call_owned(c, get_status_entry, site + 8, load_status, draft);
    block(c, site + 8, 2);
    return record_nonzero(c, c.r(3) & 8);
}

int compare_signed_byte(Cpu& c, std::uint32_t value, int threshold, unsigned destination = 0) {
    const auto signed_value = std::int8_t(value);
    c.set(destination, std::uint32_t(signed_value));
    return guest::compare(c, signed_value, threshold);
}

void apply_trait_penalty(Cpu& c, guest::Kept<30>& score, std::uint8_t raw, std::uint32_t site) {
    block(c, site, 3);
    const auto value = std::int8_t(raw);
    c.set(3, std::uint32_t(value));
    if (guest::compare(c, value, 1) == 0) {
        block(c, site + 12, 2);
        score = std::uint32_t(score) - 3;
        return;
    }
    block(c, site + 20, 2);
    if (guest::compare(c, value, 6) < 0) {
        block(c, site + 28, 1);
        score = std::uint32_t(score) - 1;
    }
}

void apply_counter_bonus(Cpu& c, std::uint32_t draft, guest::Kept<30>& score) {
    block(c, 0x1037f248, 4);
    const auto raw = c.u8(draft + review_counter);
    const auto counter = std::int8_t(raw);
    c.set({{4, raw}, {3, std::uint32_t(counter)}});
    if (guest::compare(c, counter, 75) > 0) {
        block(c, 0x1037f258, 2);
        score = std::uint32_t(score) + 10;
        return;
    }
    block(c, 0x1037f260, 2);
    if (guest::compare(c, counter, 50) > 0) {
        block(c, 0x1037f268, 2);
        score = std::uint32_t(score) + 5;
        return;
    }
    block(c, 0x1037f270, 2);
    record_nonzero(c, counter, 3);
    if (counter > 0) {
        block(c, 0x1037f278, 1);
        score = std::uint32_t(score) + 2;
    }
}

void calculate_review_score(Cpu& c, std::uint32_t draft, std::uint32_t staff, guest::Kept<30>& score) {
    block(c, 0x1037f19c, 22);
    c.set(3, 0x92490000);
    const auto staff_id = c.i32(staff);
    c.set({{7, std::uint32_t(staff_id)}, {3, 0x92492493}});
    const auto raw_response = c.u8(staff + response);
    c.set(9, raw_response);
    // The score uses the signed ID remainder. The comparison still leaves
    // the original divide's carry and sign residue live at the next access.
    constexpr auto divisor_reciprocal = std::int64_t(std::int32_t(0x92492493u));
    const auto high = std::int32_t((divisor_reciprocal * staff_id) >> 32);
    c.set(6, std::uint32_t(high));
    const auto raw_availability = c.u8(staff + availability);
    c.set(8, raw_availability);
    const auto current_morale = c.i8(staff + morale);
    const auto shift_source = std::int32_t(std::uint32_t(high) + std::uint32_t(staff_id));
    c.s.wr<1>(guest::reg::xer_ca, shift_source < 0 && (shift_source & 3));
    const auto remainder = staff_id % 7;
    const auto total = std::int8_t(raw_availability) + std::int8_t(raw_response) + remainder + current_morale;
    c.set({{3, std::uint32_t(total)}, {4, std::uint32_t(current_morale)},
           {5, std::uint32_t(remainder)}, {6, staff_id < 0 ? 1 : 0}});
    score = std::uint32_t(std::int8_t(total));
    if (guest::compare(c, current_morale, 1) == 0) {
        block(c, 0x1037f1f4, 1);
        score = std::uint32_t(score) - 2;
    }
    apply_trait_penalty(c, score, raw_availability, 0x1037f1f8);
    apply_trait_penalty(c, score, raw_response, 0x1037f218);

    block(c, 0x1037f238, 3);
    const auto current_motivation = c.u8(staff + motivation);
    c.set(3, current_motivation);
    if (guest::compare(c, current_motivation, 1) == 0) {
        block(c, 0x1037f244, 1);
        score = std::uint32_t(score) - 2;
    }
    apply_counter_bonus(c, draft, score);

    block(c, 0x1037f27c, 3);
    const auto primary = c.u32(staff + 0x14);
    c.set(3, primary);
    if (guest::nonzero_logical(c, primary)) {
        block(c, 0x1037f288, 3);
        const auto service = c.i16(primary + 10);
        c.set(3, guest::extended(service));
        if (guest::compare(c, service, 7250) > 0) {
            block(c, 0x1037f294, 2);
            score = std::uint32_t(score) - 5;
        } else {
            block(c, 0x1037f29c, 2);
            if (guest::compare(c, service, 5750) > 0) {
                block(c, 0x1037f2a4, 1);
                score = std::uint32_t(score) - 3;
            }
        }
    }
}

void count_review_groups(Cpu& c, std::uint32_t draft, std::uint32_t flags, guest::Kept<29>& groups) {
    struct Group { std::uint32_t site, mask; unsigned instructions; };
    constexpr std::array order{
        Group{0x1037f2a8, 0x20000, 2}, Group{0x1037f2b4, 0x40000, 2}, Group{0x1037f2c4, 0x1000002, 4},
        Group{0x1037f2dc, 0x200600, 4}, Group{0x1037f2f4, 0x2840, 2}, Group{0x1037f304, 0x14, 2}};
    for (const auto& group : order) {
        const auto reg = group.site == 0x1037f304 ? 0u : 3u;
        block(c, group.site, group.instructions);
        if (record_nonzero(c, std::int32_t(flags & group.mask), reg)) {
            const auto first = group.site == order.front().site;
            block(c, group.site + group.instructions * 4, first ? 1 : 2);
            groups = std::uint32_t(groups) + 1;
            if (!first) c.set(reg, groups);
        }
    }
    auto extra = has_status_flag(c, draft, 0x1037f314);
    if (!extra) {
        block(c, 0x1037f324, 5);
        c.set(3, 0x4580000);
        const auto current = c.u32(draft + ContractRecord::review_conditions);
        c.set(4, current);
        extra = record_nonzero(c, current & 0x4582208);
    }
    if (extra) {
        block(c, 0x1037f338, 1);
        groups = std::uint32_t(groups) + 1;
    }
}

void apply_group_penalties(Cpu& c, std::uint32_t staff, guest::Kept<30>& score, std::uint32_t groups) {
    block(c, 0x1037f33c, 3);
    if (compare_signed_byte(c, groups, 1) <= 0) return;

    block(c, 0x1037f348, 5);
    const auto raw_response = c.u8(staff + response);
    score = std::uint32_t(score) - 3;
    if (compare_signed_byte(c, raw_response, 6) < 0) {
        block(c, 0x1037f35c, 2);
        const auto updated = std::uint32_t(score) - groups;
        c.set(0, updated);
        score = std::uint32_t(std::int8_t(updated));
    }
    block(c, 0x1037f364, 4);
    if (compare_signed_byte(c, c.u8(staff + motivation), 6) < 0) {
        block(c, 0x1037f374, 2);
        const auto updated = std::uint32_t(score) - groups;
        c.set(0, updated);
        score = std::uint32_t(std::int8_t(updated));
    }
    block(c, 0x1037f37c, 4);
    const auto raw_availability = c.u8(staff + availability);
    if (compare_signed_byte(c, raw_availability, 1) == 0) {
        block(c, 0x1037f38c, 5);
        const auto updated = std::uint32_t(score) - 2 * groups;
        c.set(0, updated);
        score = std::uint32_t(std::int8_t(updated));
    } else {
        block(c, 0x1037f3a0, 2);
        if (guest::compare(c, std::int8_t(raw_availability), 6) < 0) {
            block(c, 0x1037f3a8, 1);
            score = std::uint32_t(score) - groups;
        }
    }
    block(c, 0x1037f3ac, 4);
    if (compare_signed_byte(c, c.u8(staff + morale), 6) < 0) {
        block(c, 0x1037f3bc, 1);
        score = std::uint32_t(score) - groups;
    }
}

bool trigger_result(Cpu& c, std::uint32_t site) {
    block(c, site, 2);
    c.set(3, 1);
    return true;
}

bool below(Cpu& c, std::uint32_t site, std::uint32_t value, int threshold) {
    block(c, site, 3);
    return compare_signed_byte(c, value, threshold) < 0;
}

bool crosses_threshold(Cpu& c, const guest::Kept<28>& draft, const guest::Kept<31>& staff,
                       const guest::Kept<30>& score) {
    block(c, 0x1037f3c0, 4);
    const auto raw_availability = c.u8(staff + availability);
    c.set(4, raw_availability);
    if (compare_signed_byte(c, raw_availability, 15) < 0) {
        block(c, 0x1037f3d0, 3);
        if (record_nonzero(c, c.u8(draft + ContractRecord::review_flags) & 1))
            return trigger_result(c, 0x1037f3dc);
    }

    block(c, 0x1037f3e4, 5);
    c.set(3, 0x1000000);
    const auto flags = c.u32(draft + ContractRecord::review_conditions);
    c.set(5, flags);
    if (record_nonzero(c, flags & 0x1000002) && below(c, 0x1037f3f8, raw_availability, 10)
        && below(c, 0x1037f404, score, 25)) return trigger_result(c, 0x1037f410);

    block(c, 0x1037f418, 4);
    c.set(3, 0x20000);
    if (record_nonzero(c, flags & 0x20020) && below(c, 0x1037f428, score, 25))
        return trigger_result(c, 0x1037f434);
    block(c, 0x1037f43c, 2);
    if (record_nonzero(c, flags & 0x200) && below(c, 0x1037f444, score, 18))
        return trigger_result(c, 0x1037f450);

    block(c, 0x1037f458, 2);
    if (record_nonzero(c, flags & 4)) {
        block(c, 0x1037f460, 4);
        if (compare_signed_byte(c, c.u8(staff + 0x63), 15) > 0) {
            block(c, 0x1037f470, 4);
            if (compare_signed_byte(c, c.u8(staff + response), 10) < 0)
                return trigger_result(c, 0x1037f480);
        }
    }
    block(c, 0x1037f488, 2);
    if (record_nonzero(c, flags & 0x2808) && below(c, 0x1037f490, raw_availability, 10)) {
        block(c, 0x1037f49c, 4);
        if (compare_signed_byte(c, c.u8(staff + morale), 6) < 0 && below(c, 0x1037f4ac, score, 30))
            return trigger_result(c, 0x1037f4b8);
    }

    block(c, 0x1037f4c0, 4);
    c.set(3, 0x80000);
    if (record_nonzero(c, flags & 0x82200) || has_status_flag(c, draft, 0x1037f4d0)) {
        block(c, 0x1037f4e0, 4);
        if (compare_signed_byte(c, c.u8(staff + response), 10) < 0 && below(c, 0x1037f4f0, score, 20))
            return trigger_result(c, 0x1037f4fc);
    }

    block(c, 0x1037f504, 3);
    const auto live_flags = c.u32(draft + ContractRecord::review_conditions);
    c.set(3, live_flags);
    if (record_nonzero(c, live_flags & 1)) return trigger_result(c, 0x1037f510);
    block(c, 0x1037f518, 2);
    if (record_nonzero(c, live_flags & 0x10)) return trigger_result(c, 0x1037f520);
    block(c, 0x1037f528, 2);
    if (record_nonzero(c, live_flags & 0x100)) return trigger_result(c, 0x1037f530);

    block(c, 0x1037f538, 2);
    if (!guest::nonzero(c, std::int32_t(live_flags))) return false;
    block(c, 0x1037f540, 7);
    const auto raw = c.u8(staff + availability);
    c.set(0, raw);
    const auto counter = c.i8(draft + review_counter);
    const auto minimum = -70 - std::int8_t(raw);
    c.set({{0, std::uint32_t(minimum)}, {3, std::uint32_t(counter)}});
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(std::int8_t(raw)) <= std::uint32_t(-70));
    if (guest::compare(c, counter, minimum) < 0) return trigger_result(c, 0x1037f55c);
    return false;
}
}

bool has_review_trigger(Cpu& c, std::uint32_t contract) {
    block(c, trigger_entry, 22);
    GuestNativeScope scope(c.s, trigger_entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    c.put32(stack - 4, c.r(31));
    const auto staff_slot = c.toc(-0x7be0);
    c.set(5, staff_slot);
    c.put32(stack - 8, c.r(30));
    c.put32(stack - 12, c.r(29));
    guest::Kept<29> groups(c, 0);
    c.put32(stack - 16, c.r(28));
    const guest::Kept<28> draft(c, contract);
    const auto club_slot = c.toc(-0x7be8);
    c.set(3, club_slot);
    c.put32(stack + 8, link);
    const auto frame = stack - 0x50;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);

    const auto club_id = c.u32(draft + ContractRecord::club_id);
    c.set(0, club_id);
    const auto clubs = c.u32(club_slot);
    const auto club = clubs + club_id * club_stride;
    c.set({{7, clubs}, {6, club_id * club_stride}});
    const auto staff_id = c.u32(draft);
    c.set(4, staff_id);
    const auto staff_table = c.u32(staff_slot);
    c.set({{5, staff_table}, {3, club}});
    const auto first = c.u32(club + 0x90);
    c.set({{3, first}, {0, staff_id * staff_stride}});
    const auto has_first = guest::nonzero_logical(c, first);
    const guest::Kept<31> staff(c, staff_table + staff_id * staff_stride);
    bool exempt = false;
    if (has_first) {
        block(c, 0x1037f168, 2);
        exempt = guest::compare_logical(c, staff, first) == 0;
    }
    if (!exempt) {
        block(c, 0x1037f170, 4);
        c.set(3, club);
        const auto second = c.u32(club + 0x94);
        c.set(0, second);
        if (guest::nonzero_logical(c, second)) {
            block(c, 0x1037f180, 2);
            exempt = guest::compare_logical(c, staff, second) == 0;
        }
    }
    bool triggered = false;
    auto negative_site = 0x1037f564u;
    if (!exempt) {
        block(c, 0x1037f188, 3);
        const auto flags = c.u32(draft + ContractRecord::review_conditions);
        c.set(0, flags);
        if (guest::nonzero(c, std::int32_t(flags))) {
            // The score occupies r30 only after the first trait calculation.
            guest::Kept<30> score(c, c.r(30));
            calculate_review_score(c, draft, staff, score);
            count_review_groups(c, draft, flags, groups);
            apply_group_penalties(c, staff, score, groups);
            triggered = crosses_threshold(c, draft, staff, score);
        } else negative_site = 0x1037f194;
    }
    if (!triggered) {
        block(c, negative_site, negative_site == 0x1037f194 ? 2 : 1);
        c.set(3, 0);
    }
    block(c, 0x1037f568, 8);
    const auto restored = c.sp() + 0x50;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    c.set(31, c.u32(restored - 4));
    c.set(30, c.u32(restored - 8));
    c.set(29, c.u32(restored - 12));
    c.set(28, c.u32(restored - 16));
    return triggered;
}
}
