#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;
using guest::Fpu;

constexpr auto renewal_entry = 0x1037e3e0u, proposal_entry = 0x10374f50u, comparison_entry = 0x1037e660u;
constexpr auto frame_size = 0xf0u, comparison_draft = 0x38u;
constexpr auto signed_integer_high = 0x43300000u, club_stride = 0x248u;
constexpr auto floating_toc = -0x4dac;

void prepare_renewal_proposal(Cpu& c, const guest::Kept<28>& draft, const guest::Kept<29>& staff,
                              const guest::Kept<27>& clubs_slot) {
    block(c, 0x1037e450, 5);
    const auto manager_slot = c.toc(-0x7b7c);
    c.set({{3, manager_slot}, {4, staff}, {5, draft}});
    c.set(3, c.u32(manager_slot));
    c.call(0x1007c140, 0x1037e464);

    block(c, 0x1037e464, 5);
    const auto proposed_role = std::int8_t(c.r(3));
    const auto current_role = c.u8(draft + 0x60) & 15u;
    c.set({{4, std::uint32_t(proposed_role)}, {7, current_role}});
    bool choose_proposed = false;
    if (guest::compare(c, proposed_role, std::int32_t(current_role)) < 0) {
        block(c, 0x1037e478, 2);
        if (detail::record_nonzero(c, proposed_role)) {
            block(c, 0x1037e480, 2);
            choose_proposed = guest::compare(c, proposed_role, 7) != 0;
        }
    }
    block(c, choose_proposed ? 0x1037e488 : 0x1037e4bc, choose_proposed ? 12 : 11);
    const auto club = c.u32(draft + ContractRecord::club_id);
    c.set({{0, club}, {4, staff}});
    const auto clubs = c.u32(clubs_slot);
    c.set(5, clubs);
    if (choose_proposed) c.set(7, c.r(3));
    c.set({{0, club * club_stride}, {3, c.sp() + comparison_draft}, {6, UINT32_MAX}, {8, 11},
           {5, clubs + club * club_stride}, {9, 0}, {10, 0}});
    c.call(proposal_entry, choose_proposed ? 0x1037e4b8 : 0x1037e4e8);
    if (choose_proposed) block(c, 0x1037e4b8, 1);
}

void measure_relative_terms(Cpu& c, Fpu& fp, const guest::Kept<28>& draft, const guest::Kept<29>& staff,
                            const guest::Kept<30>& constants) {
    block(c, 0x1037e4e8, 4);
    const auto other = c.sp() + comparison_draft;
    c.set({{3, draft}, {4, staff}, {5, other}});
    c.call(comparison_entry, 0x1037e4f8);

    block(c, 0x1037e4f8, 12);
    const auto original = c.r(3) ^ 0x80000000;
    c.set({{4, original}, {0, signed_integer_high}});
    auto fp_constants = c.toc(floating_toc);
    c.set({{5, fp_constants}, {3, other}});
    c.put32(c.sp() + 0xb8, original);
    c.set(4, staff);
    c.set_f(1, c.f64(fp_constants + 0xc0));
    c.set(5, other);
    c.put32(c.sp() + 0xb4, signed_integer_high);
    c.set_f(0, c.f64(c.sp() + 0xb4));
    c.set_f(31, fp.fsubs(c.f(0), c.f(1)));
    c.call(comparison_entry, 0x1037e528);

    block(c, 0x1037e528, 11);
    const auto replacement = c.r(3) ^ 0x80000000;
    c.set({{3, replacement}, {0, signed_integer_high}});
    fp_constants = c.toc(floating_toc);
    c.set(4, fp_constants);
    c.put32(c.sp() + 0xb0, replacement);
    c.set_f(2, c.f64(fp_constants + 0xc0));
    c.put32(c.sp() + 0xac, signed_integer_high);
    c.set_f(0, c.f32(constants + 0xc0));
    c.set_f(1, c.f64(c.sp() + 0xac));
    const auto proposed = fp.fsubs(c.f(1), c.f(2));
    c.set_f(1, proposed);
    fp.fcmp(0, proposed, c.f(0));
    if (proposed == c.f(0)) {
        block(c, 0x1037e564, 1);
        c.set_f(1, c.f(0));
    } else {
        block(c, 0x1037e554, 4);
        c.set_f(0, c.f32(constants + 0x130));
        c.set_f(0, fp.fmuls(c.f(0), c.f(31)));
        c.set_f(1, fp.fdivs(c.f(0), proposed));
    }
}

std::int32_t elapsed_days(Cpu& c, std::uint32_t draft, std::uint32_t today) {
    block(c, 0x1037e568, 9);
    const auto start_year = c.u16(draft + ContractRecord::start_date + 2);
    c.set(4, start_year);
    const auto year = c.u16(today + 2);
    c.set(0, year);
    const auto start_day = c.i16(draft + ContractRecord::start_date);
    const auto year_difference = int(year) - start_year;
    c.set({{3, guest::extended(start_day)}, {4, std::uint32_t(year_difference)}});
    const auto day = c.i16(today);
    const auto day_difference = int(day) - start_day;
    const auto elapsed = year_difference * 365 + day_difference;
    c.set(0, std::uint32_t(day_difference));
    detail::record_nonzero(c, elapsed, 4);
    if (elapsed < 0) {
        block(c, 0x1037e58c, 2);
        c.set(6, 0);
        return 0;
    }
    block(c, 0x1037e594, 3);
    const auto capped = guest::compare(c, elapsed, 500) > 0;
    c.set(6, 500);
    if (!capped) {
        block(c, 0x1037e5a0, 1);
        c.set(6, std::uint32_t(elapsed));
    }
    return capped ? 500 : elapsed;
}

void adjust_for_recent_terms(Cpu& c, Fpu& fp, std::uint32_t staff, std::uint32_t constants, int elapsed) {
    block(c, 0x1037e5a4, 6);
    const auto availability = c.i8(staff + 0x65);
    const auto interval = availability * 15;
    c.set({{4, std::uint32_t(availability)}, {3, std::uint32_t(interval)}, {0, std::uint32_t(interval + 100)}});
    if (guest::compare(c, elapsed, interval + 100) >= 0) return;

    block(c, 0x1037e5bc, 22);
    const auto packed_availability = std::uint32_t(availability) ^ 0x80000000;
    c.set({{3, signed_integer_high}, {4, packed_availability}});
    const auto fp_constants = c.toc(floating_toc);
    c.set({{5, fp_constants}, {0, std::uint32_t(elapsed)}});
    const auto stack = c.sp();
    c.put32(stack + 0xa8, packed_availability);
    const auto half_days = std::uint32_t(elapsed / 2) ^ 0x80000000;
    c.set(0, std::uint32_t(elapsed / 2));
    c.s.wr<1>(guest::reg::xer_ca, 0);
    c.set_f(0, c.f32(constants + 0xcc));
    c.put32(stack + 0xa4, signed_integer_high);
    c.set(0, half_days);
    c.set_f(3, c.f64(fp_constants + 0xc0));
    c.set_f(1, fp.fadds(c.f(0), c.f(1)));
    c.set_f(0, c.f64(stack + 0xa4));
    c.put32(stack + 0xa0, half_days);
    c.set_f(2, fp.fsub(c.f(0), c.f(3)));
    c.set_f(4, c.f64(fp_constants + 0x208));
    c.put32(stack + 0x9c, signed_integer_high);
    c.set_f(0, c.f64(stack + 0x9c));
    c.set_f(1, fp.multiply_add(0x1037e604, c.f(4), c.f(2), c.f(1)));
    c.set_f(0, fp.fsub(c.f(0), c.f(3)));
    c.set_f(1, fp.fsub(c.f(1), c.f(0)));
    c.set_f(1, fp.frsp(c.f(1)));
}

void choose_renewal_class(Cpu& c, Fpu& fp, std::uint32_t constants) {
    block(c, 0x1037e614, 3);
    c.set_f(0, c.f32(constants + 0xcc));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) {
        block(c, 0x1037e620, 2);
        c.set(3, 1);
        return;
    }
    block(c, 0x1037e628, 3);
    c.set_f(0, c.f32(constants + 0x168));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) {
        block(c, 0x1037e634, 2);
        c.set(3, 3);
    } else {
        block(c, 0x1037e63c, 1);
        c.set(3, 2);
    }
}
}

void classify_renewal_offer(Cpu& c, std::uint32_t contract, std::uint32_t person) {
    block(c, renewal_entry, 12);
    GuestNativeScope scope(c.s, renewal_entry);
    const auto stack = c.sp(), link = c.lr();
    constexpr auto saved_ratio = guest::reg::fpr(31);
    c.set(0, link);
    c.put64(stack - 8, c.s.r<8>(saved_ratio));
    detail::transfer_saved_registers<27>(c, stack - 16);
    const guest::Kept<28> draft(c, contract);
    const guest::Kept<27> clubs_slot(c, c.toc(-0x7be8));
    const guest::Kept<29> staff(c, person);
    const guest::Kept<30> constants(c, c.toc(-0x4da8));
    const guest::Kept<31> today(c, c.toc(-0x7b80));
    c.put32(stack + 8, link);
    const auto frame = stack - frame_size;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, frame + 0x60);
    c.call(0x10715880, 0x1037e410);
    block(c, 0x1037e410, 2);
    c.set(3, frame + 0x68);
    c.call(0x10715880, 0x1037e418);

    block(c, 0x1037e418, 12);
    c.set({{0, 0}, {3, UINT32_MAX}});
    const bool has_staff = guest::nonzero_logical(c, staff);
    c.put32(frame + 0x3c, UINT32_MAX);
    c.put8(frame + 0x8c, 0);
    c.put32(frame + 0x90, UINT32_MAX);
    c.put16(frame + 0x7a, 0);
    c.put8(frame + 0x94, 0);
    c.put32(frame + 0x80, 0);
    c.put32(frame + 0x88, 0);
    c.put32(frame + 0x40, 0);
    if (!has_staff) {
        block(c, 0x1037e448, 2);
        c.set(3, 1);
    } else {
        Fpu fp(c);
        prepare_renewal_proposal(c, draft, staff, clubs_slot);
        measure_relative_terms(c, fp, draft, staff, constants);
        const auto elapsed = elapsed_days(c, draft, today);
        adjust_for_recent_terms(c, fp, staff, constants, elapsed);
        choose_renewal_class(c, fp, constants);
    }

    block(c, 0x1037e640, 6);
    const auto restored = c.sp() + frame_size;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.s.wr<8>(saved_ratio, c.u64(restored - 8));
    c.set_lr(caller);
    detail::transfer_saved_registers<27, true>(c, restored - 16);
}
}
