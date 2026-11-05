#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"
#include "staff_contracts_review_followup.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using detail::record_nonzero;
using guest::Cpu;
using Draft = guest::Kept<27>;
using Club = guest::Kept<30>;
using StaffSlot = guest::Kept<31>;

constexpr auto consequence_entry = 0x1037fa80u, reseed = 0x1077b770u;
constexpr auto club_stride = 0x248u, staff_stride = 0x70u;
constexpr auto primary_staff = 0x90u, secondary_staff = 0x80u;
constexpr auto primary_conditions = 0x16288u, secondary_conditions = 0x20001u;

struct ReviewDraws {
    Cpu& c;

    void advance(std::uint32_t site) const {
        block(c, site, 6);
        const auto position = c.u32(c.r(26));
        c.set(3, position);
        const auto end = c.u32(c.r(25));
        const auto next = position + 2;
        c.set({{0, end}, {3, next}});
        const auto wrap = guest::compare_logical(c, next, end) > 0;
        c.put32(c.r(26), next);
        if (wrap) {
            block(c, site + 24, 2);
            c.put32(c.r(26), c.r(24));
            c.call(reseed, site + 32);
            block(c, site + 32, 2);
            const auto salt = c.r(3) & 0xffffu;
            c.set(0, salt);
            c.put32(c.r(23), salt);
        }
    }

    void draw_small(std::uint32_t advance_site, std::uint32_t check_site,
                    std::uint32_t draw_site, unsigned destination, std::int32_t bound) const {
        advance(advance_site);
        // The first two bounds are signed bytes and the last is a signed
        // byte divided by 15 plus 10. Reseeding only changes volatile
        // registers, so neither wide range branch can be reached.
        block(c, check_site, 4);
        c.set({{3, 0x10000}, {0, 65535}});
        guest::compare(c, bound, 65535);
        block(c, check_site + 16, 4);
        c.set({{3, 0xffff0000}, {0, std::uint32_t(-65535)}});
        guest::compare(c, bound, -65535);

        block(c, draw_site, 10);
        const auto position = c.u32(c.r(26));
        c.set(3, position);
        const auto sign = guest::compare(c, bound);
        const auto salt = c.u32(c.r(23));
        c.set(0, salt);
        const auto word = c.u16(position) + salt;
        c.set(3, word);
        const auto divisor = std::uint32_t(bound);
        const auto quotient = std::uint32_t(divide_signed<4>(word, divisor));
        const auto remainder = word - quotient * divisor;
        c.set({{0, remainder}, {destination, 0u - remainder}});
        if (sign > 0) {
            block(c, draw_site + 40, 1);
            c.set(destination, remainder);
        }
    }
};

bool first_staff_matches(Cpu& c, const Draft& draft, std::uint32_t staff, std::uint32_t site) {
    if (!guest::nonzero_logical(c, staff)) return false;
    block(c, site, 4);
    const auto wanted = c.u32(draft);
    c.set(3, wanted);
    const auto current = c.u32(staff);
    c.set(0, current);
    return guest::compare(c, std::int32_t(wanted), std::int32_t(current)) == 0;
}

void review_followup(Cpu& c, const Draft& draft, const Club& club, const StaffSlot& staff_slot,
                     const guest::Kept<21>& policy, std::uint32_t conditions) {
    block(c, 0x1037fafc, 6);
    const auto updated = c.u32(draft + ContractRecord::review_conditions) | conditions;
    c.set(0, updated);
    c.put32(draft + ContractRecord::review_conditions, updated);
    const auto primary = c.u32(club + primary_staff);
    c.set(4, primary);
    if (first_staff_matches(c, draft, primary, 0x1037fb14)) return;

    block(c, 0x1037fb24, 3);
    const auto deputy = c.u32(club + 0x94);
    c.set(4, deputy);
    if (first_staff_matches(c, draft, deputy, 0x1037fb30)) return;

    block(c, 0x1037fb40, 4);
    const auto counter = c.i8(draft + 0x4c);
    if (record_nonzero(c, counter < 0 ? 0x10000 : 0)) return;
    block(c, 0x1037fb50, 3);
    if (record_nonzero(c, c.i8(draft + ContractRecord::terms_flag))) return;
    detail::notify_review_followup<0x1037fb5c>(c, draft, staff_slot, policy);
}

bool team_can_handle_review(Cpu& c, const Club& club, unsigned side, std::uint32_t site) {
    block(c, site, 6);
    c.set({{3, club}, {4, side}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, site + 24);
    block(c, site + 24, 2);
    return record_nonzero(c, std::int8_t(c.r(3)));
}

bool wants_primary_consequence(Cpu& c, std::uint32_t conditions, std::uint32_t site) {
    block(c, site, 4);
    c.set(3, 0x10000);
    return record_nonzero(c, conditions & primary_conditions);
}

void reduce_availability(Cpu& c, const Club& club, ReviewDraws& draws) {
    block(c, 0x1037fc3c, 5);
    const auto primary = c.u32(club + primary_staff);
    c.set(3, primary);
    const auto availability = c.i8(primary + 0x65);
    c.set(0, std::uint32_t(availability));
    if (guest::compare(c, availability, 1) <= 0) return;

    draws.advance(0x1037fc50);
    block(c, 0x1037fc78, 13);
    const auto position = c.u32(c.r(26));
    c.set({{5, position}, {3, 0x66660000}});
    const auto salt = c.u32(c.r(23));
    c.set({{4, salt}, {0, 0x66666667}});
    const auto random = c.u16(position) + salt;
    const auto signed_random = std::int32_t(random);
    const auto high = (INT64_C(0x66666667) * signed_random) >> 32;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 1));
    c.set({{3, high < 0 ? 1u : 0u}, {4, random}});
    if (record_nonzero(c, signed_random % 5)) return;

    block(c, 0x1037fcac, 5);
    const auto live = c.u32(club + primary_staff);
    c.set(4, live);
    const auto current = c.u8(live + 0x65);
    c.set({{3, current}, {0, std::uint32_t(current) - 1}});
    c.put8(live + 0x65, current - 1);
}

void notify_staff(Cpu& c, const Draft& draft, const StaffSlot& staff_slot,
                  std::uint32_t site, const Club* club = nullptr) {
    block(c, site, club ? 7 : 6);
    const auto id = c.u32(draft);
    c.set({{0, id}, {5, 1}});
    const auto table = c.u32(staff_slot);
    c.set({{4, table}, {0, id * staff_stride}});
    if (club) c.set(3, c.u32(*club + primary_staff));
    c.set(4, table + id * staff_stride);
    c.call(0x100d9b80, site + (club ? 28 : 24));
}

void consider_primary_staff(Cpu& c, const Draft& draft, const Club& club, const StaffSlot& staff_slot,
                           ReviewDraws& draws) {
    block(c, 0x1037fcf0, 3);
    const auto primary = c.u32(club + primary_staff);
    c.set(3, primary);
    if (!guest::nonzero_logical(c, primary)) return;
    block(c, 0x1037fcfc, 3);
    const auto profile = c.u32(primary + 0x1c);
    c.set(0, profile);
    if (!guest::nonzero_logical(c, profile)) return;

    block(c, 0x1037fd08, 3);
    const auto motivation = c.i8(primary + 0x69);
    if (record_nonzero(c, motivation, 21))
        draws.draw_small(0x1037fd1c, 0x1037fd44, 0x1037fe38, 20, motivation);
    else {
        block(c, 0x1037fd14, 2);
        c.set(20, 0);
    }

    block(c, 0x1037fe64, 2);
    if (guest::nonzero(c, std::int32_t(c.r(20)))) {
        block(c, 0x1037fe6c, 5);
        const auto live = c.u32(club + primary_staff);
        c.set(3, live);
        const auto preferences = c.u32(live + 0x1c);
        c.set(3, preferences);
        const auto response = c.i8(preferences + 0x34);
        if (record_nonzero(c, response, 21))
            draws.draw_small(0x1037fe88, 0x1037feb0, 0x1037ffa4, 20, response);
        else {
            block(c, 0x1037fe80, 2);
            c.set(20, 0);
        }

        block(c, 0x1037ffd0, 11);
        const auto counter = c.i8(draft + 0x4c);
        const auto quotient = counter / 15;
        const auto bound = quotient + 10;
        c.set({{0, std::uint32_t(quotient - (counter < 0 ? 1 : 0))}, {3, std::uint32_t(quotient)}});
        c.s.wr<1>(guest::reg::xer_ca, quotient < 0);
        record_nonzero(c, bound, 21);
        // The signed-byte numerator constrains this bound to [2, 18].
        draws.draw_small(0x10380004, 0x1038002c, 0x10380120, 22, bound);

        block(c, 0x1038014c, 2);
        if (guest::compare(c, std::int32_t(c.r(20)), std::int32_t(c.r(22))) <= 0) return;
    }
    notify_staff(c, draft, staff_slot, 0x10380154, &club);
}

void consider_secondary_staff(Cpu& c, const Draft& draft, const Club& club, const StaffSlot& staff_slot,
                             ReviewDraws& draws, std::uint32_t conditions) {
    block(c, 0x10380170, 4);
    c.set(3, 0x20000);
    if (!record_nonzero(c, conditions & secondary_conditions)) return;
    block(c, 0x10380180, 3);
    const auto member = c.u32(club + secondary_staff);
    c.set(3, member);
    if (!guest::nonzero_logical(c, member)) return;
    block(c, 0x1038018c, 3);
    const auto profile = c.u32(member + 0x1c);
    c.set(0, profile);
    if (!guest::nonzero_logical(c, profile)) return;

    draws.advance(0x10380198);
    block(c, 0x103801c0, 19);
    const auto position = c.u32(c.r(26));
    c.set({{5, position}, {4, 0x88890000}});
    const auto live = c.u32(club + secondary_staff);
    c.set({{3, live}, {0, 0x88888889}});
    const auto word = c.u16(position);
    c.set(5, word);
    const auto salt = c.u32(c.r(23));
    c.set(4, salt);
    const auto live_profile = c.u32(live + 0x1c);
    const auto random = word + salt;
    const auto high = std::int32_t((std::int64_t(std::int32_t(0x88888889u)) * std::int32_t(random)) >> 32);
    c.set({{6, live_profile}, {5, random}, {0, std::uint32_t(high)}});
    const auto threshold = c.i8(live_profile + 0x36);
    const auto shift_source = std::int32_t(std::uint32_t(high) + random);
    c.s.wr<1>(guest::reg::xer_ca, shift_source < 0 && (shift_source & 15));
    const auto remainder = std::int32_t(random) % 30;
    c.set({{6, std::uint32_t(threshold)}, {4, shift_source < 0 ? 1u : 0u}, {0, std::uint32_t(remainder)}});
    if (guest::compare(c, threshold, remainder) > 0)
        notify_staff(c, draft, staff_slot, 0x1038020c);
}

void notify_context(Cpu& c, const Draft& draft, const StaffSlot& staff_slot, std::uint32_t context,
                    std::uint32_t conditions) {
    block(c, 0x10380224, 2);
    if (!guest::nonzero_logical(c, context)) return;
    block(c, 0x1038022c, 2);
    if (!record_nonzero(c, std::int32_t(conditions & 0xe0000000))) return;

    block(c, 0x10380234, 7);
    const auto id = c.u32(draft);
    c.set({{0, id}, {4, context}});
    const auto table = c.u32(staff_slot);
    c.set({{0, id * staff_stride}, {3, table + id * staff_stride}, {5, conditions}});
    c.call(0x100ddfc0, 0x10380250);
}
}

void apply_review_conditions(Cpu& c, std::uint32_t contract, std::uint32_t conditions, std::uint32_t argument) {
    block(c, consequence_entry, 27);
    GuestNativeScope scope(c.s, consequence_entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    detail::transfer_saved_registers<20>(c, stack);
    const guest::Kept<29> context(c, argument);
    const StaffSlot staff_slot(c, c.toc(-0x7be0));
    const Draft draft(c, contract);
    const guest::Kept<28> flags(c, conditions);
    const auto clubs_slot = c.toc(-0x7be8);
    c.set(5, clubs_slot);
    const guest::Kept<21> policy(c, c.toc(-0x7b40));
    c.set(22, c.toc(-0x4dac));
    c.set(23, c.toc(-0x7b90));
    c.set(24, c.toc(-0x7b8c));
    c.set(25, c.toc(-0x7b88));
    c.set(26, c.toc(-0x7b84));
    c.put32(stack + 8, link);
    const auto frame = stack - 0x80;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);

    const auto id = c.u32(contract);
    c.set(0, id);
    const auto club_id = c.u32(contract + ContractRecord::club_id);
    c.set({{4, club_id}, {0, id * staff_stride}});
    const auto staff = c.u32(staff_slot);
    c.set(3, staff);
    const auto clubs = c.u32(clubs_slot);
    c.set({{5, clubs}, {3, staff + id * staff_stride}});
    const auto status = c.i8(staff + id * staff_stride + 0x62);
    c.set({{3, club_id * club_stride}, {0, std::uint32_t(status)}});
    auto relevant = guest::compare(c, status, 11) == 0;
    const Club club(c, clubs + club_id * club_stride);
    if (!relevant) {
        block(c, 0x1037faec, 2);
        relevant = guest::compare(c, status, 15) == 0;
        if (!relevant) {
            block(c, 0x1037faf4, 2);
            relevant = guest::compare(c, status, 13) == 0;
        }
    }
    if (relevant) {
        review_followup(c, draft, club, staff_slot, policy, flags);
        ReviewDraws draws{c};
        if (team_can_handle_review(c, club, 0, 0x1037fc0c)) {
            if (wants_primary_consequence(c, flags, 0x1037fc2c)) reduce_availability(c, club, draws);
        } else if (!team_can_handle_review(c, club, 1, 0x1037fcc0)
                   && wants_primary_consequence(c, flags, 0x1037fce0)) {
            consider_primary_staff(c, draft, club, staff_slot, draws);
        }
        consider_secondary_staff(c, draft, club, staff_slot, draws, flags);
        notify_context(c, draft, staff_slot, context, flags);
    }

    block(c, 0x10380250, 5);
    const auto restored = c.sp() + 0x80;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    detail::transfer_saved_registers<20, true>(c, restored);
}
}
