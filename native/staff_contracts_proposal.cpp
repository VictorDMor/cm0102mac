#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;

constexpr auto proposal_frame = 0x130u, double_header = 0x43300000u;
constexpr auto staff_profile = 0x14u, staff_nation = 0x1cu, related_club = 0x20u;
constexpr auto registered_club = 0x24u, staff_age = 0x5cu;
constexpr auto club_rating = 0x1ecu, club_type = 0x23du;

struct ContractProposal {
    Cpu& c;
    guest::Fpu fp;
    const guest::Kept<28> contract;
    const guest::Kept<29> person;
    guest::Kept<30> destination;
    guest::Kept<31> role;
    const guest::Kept<13> policy;
    const guest::Kept<16> conditions;
    const guest::Kept<17> incumbent;
    const guest::Kept<18> incumbent_club;
    const guest::Kept<19> age;
    const guest::Kept<20> profile;
    const guest::Kept<21> reputation;
    const guest::Kept<22> wage_band;
    const guest::Kept<23> primary_slot;
    const guest::Kept<26> manager_slot;
    const guest::Kept<27> today;

    explicit ContractProposal(Cpu& cpu)
        : c(cpu), fp(cpu), contract(c, c.r(28)), person(c, c.r(29)), destination(c, c.r(30)),
          role(c, c.r(31)), policy(c, c.r(13)), conditions(c, c.r(16)), incumbent(c, c.r(17)),
          incumbent_club(c, c.r(18)), age(c, c.r(19)), profile(c, c.r(20)),
          reputation(c, c.r(21)), wage_band(c, c.r(22)), primary_slot(c, c.r(23)),
          manager_slot(c, c.r(26)), today(c, c.r(27)) {}

    double number(unsigned offset) const { return c.f64(c.r(24) + offset); }
    double single(unsigned offset) const { return c.f32(c.r(25) + offset); }
};

void open_proposal_frame(Cpu& c, GuestNativeScope& scope) {
    block(c, proposal_entry, 3);
    c.set({{0, c.lr()}, {11, c.sp()}});
    c.call(0x10773808, 0x10374f5c);

    block(c, 0x10374f5c, 19);
    const auto stack = c.sp();
    detail::transfer_saved_registers<13>(c, stack - 0x20);
    c.put32(stack + 8, c.r(0));
    const auto frame = stack - proposal_frame;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(14, c.toc(-0x7b9c));
    c.set(28, c.r(3));
    c.set(23, c.toc(-0x7b78));
    c.set(29, c.r(4));
    c.put32(frame + 0x154, c.r(6));
    c.set(30, c.r(5));
    c.set(24, c.toc(-0x4dac));
    c.set(31, c.r(7));
    c.put32(frame + 0x160, c.r(9));
    c.set(13, c.r(8));
    c.set(25, c.toc(-0x4da8));
    c.set(3, frame + 0x5c);
    c.put32(frame + 0x164, c.r(10));
    c.set(26, c.toc(-0x7b7c));
    c.set(27, c.toc(-0x7b80));
    c.call(0x10715880, 0x10374fa8);
}

bool initialize_proposal_record(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10374fa8, 7);
    c.set({{0, 0}, {3, p.contract}});
    c.put8(c.sp() + 0xb4, 0);
    c.set({{17, 0}, {16, 0}, {4, 0}});
    detail::call_owned(c, 0x10380700, 0x10374fc4, set_release_amount, std::uint32_t(p.contract), 0u);
    block(c, 0x10374fc4, 4);
    c.set(15, 0);
    const auto specified = guest::nonzero_logical(c, p.destination);
    c.put8(p.contract + 0x20, 0);
    return specified;
}

void find_primary_proposal_club(ContractProposal& p, bool specified) {
    Cpu& c = p.c;
    const auto site = specified ? 0x10375068u : 0x10374fd4u;
    block(c, site, 3);
    c.set(3, c.u32(p.manager_slot));
    c.set(4, p.person);
    c.call(0x10082f20, site + 12);
    block(c, site + 12, 3);
    const auto record = c.r(3);
    const auto has_record = guest::nonzero_logical(c, record);
    c.put32(p.primary_slot, record);
    if (has_record) {
        block(c, site + 24, specified ? 6 : 5);
        const auto packed = c.u8(record + ContractRecord::packed_kind);
        const auto kind = detail::unpack_kind(packed);
        c.s.wr<1>(guest::reg::xer_ca, (packed & 0x82u) == 0x82u);
        c.set(0, std::uint32_t(kind));
        const auto active = guest::compare(c, kind, 5) != 0;
        if (!specified) block(c, 0x10375000, 1);
        if (active) {
            block(c, specified ? 0x10375098u : 0x10375004u, 1);
            c.set(15, 1);
        }
    }

    const auto select = specified ? 0x1037509cu : 0x10375008u;
    const auto result_reg = specified ? 0u : 3u;
    block(c, select, 2);
    if (guest::nonzero(c, std::int32_t(c.r(15)))) {
        block(c, select + 8, 7);
        const auto active = c.u32(p.primary_slot);
        c.set(4, active);
        const auto clubs_slot = c.toc(-0x7be8);
        c.set(3, clubs_slot);
        const auto id = c.u32(active + ContractRecord::club_id);
        c.set(0, id);
        const auto clubs = c.u32(clubs_slot);
        c.set({{3, clubs}, {0, id * 0x248u}, {result_reg, clubs + id * 0x248u}});
    } else {
        block(c, select + 36, 1);
        c.set(result_reg, 0);
    }
    if (specified) {
        block(c, 0x103750c4, 1);
        c.set(18, c.r(0));
    }
}

void adopt_proposal_destination(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375030, 4);
    const auto club = c.r(3);
    const auto has_club = guest::nonzero_logical(c, club);
    c.set(18, club);
    p.destination = club;
    if (!has_club) return;

    block(c, 0x10375040, 3);
    c.set({{4, c.sp() + 0x38}, {5, 1}});
    c.call(0x100ce320, 0x1037504c);
    block(c, 0x1037504c, 2);
    if (!guest::nonzero_logical(c, c.r(3))) return;
    block(c, 0x10375054, 3);
    if (!detail::record_nonzero(c, c.i8(c.sp() + 0x38))) {
        block(c, 0x10375060, 2);
        p.destination = c.r(3);
    }
}

void lookup_proposal_contract(ContractProposal& p, bool specified) {
    Cpu& c = p.c;
    find_primary_proposal_club(p, specified);
    if (!specified) adopt_proposal_destination(p);

    block(c, 0x103750c8, 2);
    if (!guest::nonzero_logical(c, p.incumbent_club)) return;
    block(c, 0x103750d0, 4);
    c.set(3, c.u32(p.manager_slot));
    c.set({{4, p.person}, {5, p.incumbent_club}});
    c.call(0x10082f90, 0x103750e0);
    block(c, 0x103750e0, 1);
    c.set(17, c.r(3));
}

bool seed_proposal_identity(ContractProposal& p) {
    Cpu& c = p.c;
    const auto frame = c.sp();
    block(c, 0x103750e4, 37);
    const auto packed = c.u8(p.contract + ContractRecord::packed_kind);
    c.set({{0, (packed & 3u) | 8u}, {3, 2}, {4, double_header}});
    c.put8(p.contract + ContractRecord::packed_kind, std::uint8_t(c.r(0)));
    const auto selection = std::int8_t(std::uint32_t(p.role));
    c.set({{0, std::uint32_t(selection)}, {7, 0}, {5, UINT32_MAX}});
    const auto clear = c.u8(p.contract + ContractRecord::packed_kind) & 0xfeu;
    c.set(6, clear);
    const auto population_slot = c.toc(-0x7be4);
    c.set(3, population_slot);
    const auto choose_role = guest::compare(c, selection, -1) == 0;
    c.put8(p.contract + ContractRecord::packed_kind, std::uint8_t(clear));
    c.set_f(2, p.number(0xc0));
    c.put8(p.contract + ContractRecord::terms_flag, 0);
    c.put32(p.contract + ContractRecord::terms_marker, UINT32_MAX);
    c.put16(p.contract + ContractRecord::review_marker, 0);
    const auto id = c.u32(p.person);
    c.set(0, id);
    c.put32(frame + 0xac, double_header);
    c.put32(p.contract, id);
    c.put8(p.contract + 0x40, 0);
    const auto current_id = c.u32(p.person);
    c.set(5, current_id);
    const auto population = c.u32(population_slot);
    c.set({{0, population}, {5, current_id ^ 0x80000000}});
    c.put32(frame + 0xa4, double_header);
    c.set(3, population ^ 0x80000000);
    const auto age = c.i16(p.person + staff_age);
    c.set(0, guest::extended(age));
    c.put32(frame + 0xb0, current_id ^ 0x80000000);
    c.set(20, c.u32(p.person + staff_profile));
    c.set(19, std::uint32_t(std::int8_t(age)));
    c.put32(frame + 0xa8, population ^ 0x80000000);
    c.set_f(1, c.f64(frame + 0xac));
    c.set_f(0, c.f64(frame + 0xa4));
    c.set_f(1, p.fp.fsubs(c.f(1), c.f(2)));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(2)));
    c.set_f(31, p.fp.fdivs(c.f(1), c.f(0)));
    return choose_role;
}

void select_proposal_role(ContractProposal& p, bool automatic) {
    Cpu& c = p.c;
    if (automatic) {
        block(c, 0x10375178, 2);
        if (guest::nonzero_logical(c, p.profile)) {
            block(c, 0x10375180, 2);
            const std::uint32_t current = p.incumbent;
            if (guest::nonzero_logical(c, current)) {
                block(c, 0x10375188, 4);
                c.set(3, c.u32(p.manager_slot));
                c.set({{4, p.person}, {5, current}});
                c.call(0x1007c140, 0x10375198);
                block(c, 0x10375198, 4);
                const auto status = c.u32(current + ContractRecord::review_conditions);
                c.set(4, status);
                p.role = c.r(3);
                if (detail::record_nonzero(c, status & 0x10)) {
                    block(c, 0x103751a8, 3);
                    const auto role = std::int8_t(std::uint32_t(p.role));
                    c.set(0, std::uint32_t(role));
                    if (guest::compare(c, role, 1) != 0) {
                        block(c, 0x103751b4, 1);
                        p.role = 2;
                    }
                }
                block(c, 0x103751b8, 2);
                c.set(16, status);
            } else {
                block(c, 0x103751c0, 7);
                const auto service_slot = c.toc(-0x7b40);
                c.set({{3, service_slot}, {4, p.person}, {5, p.destination}, {6, 0}});
                const auto service = c.u32(service_slot);
                c.set(3, service);
                c.set(3, c.u32(service + 0x988));
                c.call(0x1036d510, 0x103751dc);
                block(c, 0x103751dc, 2);
                p.role = c.r(3);
            }
            return;
        }
    }
    block(c, 0x103751e4, 2);
    if (guest::nonzero_logical(c, p.incumbent)) {
        block(c, 0x103751ec, 1);
        c.set(16, c.u32(p.incumbent + ContractRecord::review_conditions));
    }
}

void initialize_proposal_context(ContractProposal& p) {
    const auto specified = initialize_proposal_record(p);
    lookup_proposal_contract(p, specified);
    select_proposal_role(p, seed_proposal_identity(p));
}

std::int32_t reduce_proposal_rating(Cpu& c, std::int16_t reputation) {
    const auto high = (INT64_C(0x10624dd3) * reputation) >> 32;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 31));
    return std::int32_t(reputation / 500);
}

void set_proposal_destination_band(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375204, 13);
    c.set(3, 0x10620000);
    const auto reputation = c.i16(p.destination + club_rating);
    c.set(4, guest::extended(reputation));
    c.set(3, 0x10624dd3);
    const auto type = c.i8(p.destination + club_type);
    c.set(0, std::uint32_t(type));
    const auto special = guest::compare(c, type, 2) == 0;
    const auto rating = reduce_proposal_rating(c, reputation);
    auto band = std::int8_t(rating + 1);
    c.set({{0, std::uint32_t(rating + 1)}, {3, std::uint32_t(rating)}, {22, std::uint32_t(band)}});
    if (!special) return;

    block(c, 0x10375238, 4);
    c.set(22, p.wage_band - 1);
    band = std::int8_t(p.wage_band);
    c.set(0, std::uint32_t(band));
    if (guest::compare(c, band, 1) < 0) {
        block(c, 0x10375248, 1);
        c.set(22, 1);
        band = 1;
    }
    block(c, 0x1037524c, 3);
    c.set(0, std::uint32_t(band));
    if (guest::compare(c, band, 4) > 0) {
        block(c, 0x10375258, 2);
        c.set(22, 4);
    }
}

void set_proposal_fallback_band(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375260, 3);
    const auto nation = c.u32(p.person + staff_nation);
    c.set(0, nation);
    bool use_profile = false;
    const std::uint32_t profile = p.profile;
    if (!guest::nonzero_logical(c, nation)) {
        block(c, 0x1037526c, 2);
        use_profile = guest::nonzero_logical(c, profile);
    }
    if (use_profile) {
        block(c, 0x10375274, 9);
        c.set(3, 0x10620000);
        const auto reputation = c.i16(profile + 12);
        const auto rating = reduce_proposal_rating(c, reputation);
        c.set({{0, std::uint32_t(rating)}, {3, unsigned(rating < 0 || (rating == 0 && reputation < 0))},
               {22, std::uint32_t(std::int8_t(rating))}});
        return;
    }

    block(c, 0x10375298, 12);
    const auto club = c.u32(p.person + related_club);
    c.set({{4, club}, {3, 0x10624dd3}});
    const auto rating = reduce_proposal_rating(c, c.i16(club + club_rating));
    const auto band = std::int8_t(rating + 1);
    c.set({{0, std::uint32_t(rating + 1)}, {3, std::uint32_t(rating)}, {22, std::uint32_t(band)}});
    if (guest::compare(c, band, 3) < 0) {
        block(c, 0x103752c8, 1);
        c.set(22, 3);
    }
}

void choose_proposal_band(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x103751f0, 5);
    const auto bits = (c.u8(p.contract + 0x60) & 0xf0u) | (p.role & 0xfu);
    c.set(0, bits);
    const auto has_destination = guest::nonzero_logical(c, p.destination);
    c.put8(p.contract + 0x60, std::uint8_t(bits));
    if (has_destination) set_proposal_destination_band(p);
    else set_proposal_fallback_band(p);

    block(c, 0x103752cc, 2);
    if (guest::nonzero_logical(c, p.destination)) {
        block(c, 0x103752d4, 3);
        const auto id = c.u32(p.destination);
        c.set(0, id);
        c.put32(p.contract + ContractRecord::club_id, id);
    } else {
        block(c, 0x103752e0, 2);
        c.set(0, UINT32_MAX);
        c.put32(p.contract + ContractRecord::club_id, UINT32_MAX);
    }
}

void raise_proposal_band_for_experience(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x103752e8, 3);
    const auto experience = c.u8(p.person + 0x5e);
    c.set(4, experience);
    if (!guest::nonzero_logical(c, experience)) return;
    block(c, 0x103752f4, 3);
    const auto league = c.u32(p.person + 12);
    c.set(3, league);
    if (guest::nonzero_logical(c, league)) {
        block(c, 0x10375300, 23);
        const auto level = c.u8(league + 0x11f);
        const auto frame = c.sp();
        c.set({{3, level}, {0, double_header}});
        c.put32(frame + 0xa4, double_header);
        const auto signed_level = std::uint32_t(std::int8_t(level));
        c.set(3, signed_level);
        c.set_f(4, p.number(0xc0));
        const auto encoded = signed_level ^ 0x80000000;
        c.set(3, encoded);
        c.put32(frame + 0xb0, experience);
        c.set_f(2, p.number(0xc8));
        c.put32(frame + 0xa8, encoded);
        c.set_f(3, p.single(0));
        c.put32(frame + 0xac, double_header);
        c.set_f(0, c.f64(frame + 0xa4));
        c.set_f(1, c.f64(frame + 0xac));
        c.set_f(4, p.fp.fsubs(c.f(0), c.f(4)));
        c.set_f(0, p.single(4));
        c.set_f(1, p.fp.fsubs(c.f(1), c.f(2)));
        c.set_f(2, p.fp.fmuls(c.f(4), c.f(3)));
        c.set_f(0, p.fp.fmuls(c.f(1), c.f(0)));
        c.set_f(0, p.fp.fmuls(c.f(2), c.f(0)));
        c.put64(frame + 0x9c, p.fp.fctiwz(0, c.f(0)));
        c.set(3, c.u32(frame + 0xa0));
    } else {
        block(c, 0x1037535c, 1);
        c.set(3, 1);
    }
    block(c, 0x10375360, 3);
    const auto increase = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(increase));
    if (guest::compare(c, increase, 4) > 0) {
        block(c, 0x1037536c, 2);
        c.set(3, 4);
    } else {
        block(c, 0x10375374, 2);
        if (!detail::record_nonzero(c, increase)) {
            block(c, 0x1037537c, 1);
            c.set(3, 1);
        }
    }
    block(c, 0x10375380, 1);
    c.set(22, p.wage_band + c.r(3));
}

bool proposal_profile_allowed(ContractProposal& p, std::uint32_t pc) {
    Cpu& c = p.c;
    block(c, pc, 2);
    if (!guest::nonzero_logical(c, p.profile)) return false;
    block(c, pc + 8, 2);
    c.set(3, p.policy);
    c.call(0x100c7a20, pc + 16);
    block(c, pc + 16, 2);
    return detail::record_nonzero(c, std::int8_t(c.r(3)));
}

void load_proposal_reputation(ContractProposal& p) {
    Cpu& c = p.c;
    if (proposal_profile_allowed(p, 0x10375384)) {
        block(c, 0x1037539c, 2);
        c.set(21, guest::extended(c.i16(p.profile + 10)));
    } else {
        block(c, 0x103753a4, 3);
        const auto nation = c.u32(p.person + staff_nation);
        c.set(3, nation);
        if (guest::nonzero_logical(c, nation)) {
            block(c, 0x103753b0, 1);
            c.set(21, guest::extended(c.i16(nation + 0x2a)));
        }
    }
    block(c, 0x103753b4, 2);
    if (!detail::record_nonzero(c, std::int16_t(p.reputation))) {
        block(c, 0x103753bc, 1);
        c.set(21, 1);
    }
}

int proposal_signed_short(ContractProposal& p, std::uint32_t pc, std::uint32_t source, unsigned field, int boundary) {
    Cpu& c = p.c;
    block(c, pc, 3);
    const auto value = c.i16(source + field);
    c.set(0, guest::extended(value));
    return guest::compare(c, value, boundary);
}

int proposal_signed_byte(ContractProposal& p, std::uint32_t pc, std::uint32_t source, unsigned field, int boundary) {
    Cpu& c = p.c;
    block(c, pc, 4);
    const auto value = c.i8(source + field);
    c.set(0, std::uint32_t(value));
    return guest::compare(c, value, boundary);
}

void scale_proposal_value(ContractProposal& p, std::uint32_t pc, unsigned factor, bool jump = true) {
    Cpu& c = p.c;
    block(c, pc, jump ? 15 : 14);
    const auto address = p.contract + ContractRecord::draft_auxiliary;
    const auto amount = c.u32(address);
    c.set({{3, amount}, {0, double_header}});
    const auto frame = c.sp();
    c.put32(frame + 0x9c, double_header);
    const auto encoded = amount ^ 0x80000000;
    c.set(0, encoded);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0xa0, encoded);
    c.set_f(2, p.number(factor));
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, p.fp.fmul(c.f(2), c.f(0)));
    c.put64(frame + 0xa4, p.fp.fctiwz(0, c.f(0)));
    const auto result = c.u32(frame + 0xa8);
    c.set(0, result);
    c.put32(address, result);
}

void adjust_proposal_value_for_reputation(ContractProposal& p) {
    Cpu& c = p.c;
    const auto registered = c.u32(p.person + registered_club);
    c.set(0, registered);
    const std::uint32_t profile = p.profile;
    if (!guest::nonzero_logical(c, registered)
        && proposal_signed_short(p, 0x10375420, profile, 10, 2750) < 0
        && proposal_signed_short(p, 0x1037542c, p.destination, club_rating, 2000) < 0) {
        scale_proposal_value(p, 0x10375438, 0x48);
        return;
    }
    if (proposal_signed_short(p, 0x10375474, profile, 10, 4250) < 0) {
        scale_proposal_value(p, 0x10375480, 0x28);
        return;
    }
    block(c, 0x103754bc, 2);
    if (guest::compare(c, std::int32_t(c.r(0)), 4750) < 0) scale_proposal_value(p, 0x103754c4, 0x30, false);
}

void adjust_proposal_value_for_incumbent(ContractProposal& p) {
    Cpu& c = p.c;
    const std::uint32_t profile = p.profile;
    const auto age = std::int32_t(p.age);
    block(c, 0x10375504, 2);
    if (guest::compare(c, age, 19) < 0 && proposal_signed_short(p, 0x1037550c, profile, 10, 3750) < 0) {
        block(c, 0x10375518, 3);
        c.set(0, 0);
        c.put32(p.contract + ContractRecord::draft_auxiliary, 0);
        return;
    }
    block(c, 0x10375524, 2);
    if (guest::compare(c, age, 21) < 0 && proposal_signed_short(p, 0x1037552c, profile, 10, 4250) < 0) {
        block(c, 0x10375538, 2);
        if (guest::compare(c, std::int32_t(c.r(0)), 2750) < 0
            && proposal_signed_byte(p, 0x10375540, p.person, 0x66, 15) < 0) {
            scale_proposal_value(p, 0x10375550, 0x88);
        } else if (proposal_signed_byte(p, 0x1037558c, p.person, 0x66, 15) < 0) {
            scale_proposal_value(p, 0x1037559c, 0x48);
        } else {
            scale_proposal_value(p, 0x103755d8, 0x28);
        }
        return;
    }
    block(c, 0x10375614, 2);
    if (guest::nonzero_logical(c, p.destination)
        && proposal_signed_short(p, 0x1037561c, profile, 10, 3500) < 0
        && proposal_signed_short(p, 0x10375628, p.destination, club_rating, 3500) < 0) {
        scale_proposal_value(p, 0x10375634, 0x28, false);
    }
}

void adjust_proposal_value_for_role(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x1037566c, 4);
    c.set(3, c.u32(p.manager_slot));
    c.set({{4, p.person}, {5, p.incumbent}});
    c.call(0x1007c140, 0x1037567c);
    block(c, 0x1037567c, 3);
    const auto role = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(role));
    if (guest::compare(c, role, 4) == 0) {
        scale_proposal_value(p, 0x10375718, 0x28);
        return;
    }
    block(c, 0x10375688, 1);
    if (role < 4) {
        block(c, 0x1037568c, 2);
        if (guest::compare(c, role, 1) == 0) {
            scale_proposal_value(p, 0x103756a0, 0xd0);
            return;
        }
        block(c, 0x10375694, 1);
        if (role > 1) {
            scale_proposal_value(p, 0x103756dc, 0x30);
            return;
        }
        block(c, 0x10375698, 1);
    }
    scale_proposal_value(p, 0x10375754, 0x48);
}

void adjust_proposal_value_for_previous_club(ContractProposal& p) {
    Cpu& c = p.c;
    const std::uint32_t previous = p.incumbent_club;
    block(c, 0x10375790, 2);
    if (guest::nonzero_logical(c, previous)
        && proposal_signed_short(p, 0x10375798, previous, club_rating, 4250) < 0)
        scale_proposal_value(p, 0x103757a4, 0x28, false);

    block(c, 0x103757dc, 2);
    if (!guest::nonzero_logical(c, previous)) return;
    block(c, 0x103757e4, 3);
    const auto league = c.u32(previous + 0x2c);
    c.set(3, league);
    if (guest::nonzero_logical(c, league) && proposal_signed_byte(p, 0x103757f0, league, 0x11f, 10) < 0)
        scale_proposal_value(p, 0x10375800, 0x28);
}

void calculate_proposal_value(ContractProposal& p) {
    Cpu& c = p.c;
    if (!proposal_profile_allowed(p, 0x103753c0)) {
        block(c, 0x1037583c, 2);
        c.set(0, 0);
        c.put32(p.contract + ContractRecord::draft_auxiliary, 0);
        return;
    }
    block(c, 0x103753d8, 7);
    c.set(3, c.u32(p.manager_slot));
    c.set({{4, p.person}, {5, p.destination}, {6, 1}, {7, UINT32_MAX}, {8, 0}});
    c.call(0x10084be0, 0x103753f4);
    block(c, 0x103753f4, 11);
    c.set(4, 0x66660000);
    const auto doubled = std::int32_t(c.r(3) * 2);
    const auto high = (INT64_C(0x66666667) * doubled) >> 32;
    const auto valuation = std::uint32_t(doubled / 10);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 3));
    c.set({{3, unsigned(high < 0)}, {0, valuation}});
    c.put32(p.contract + ContractRecord::draft_auxiliary, valuation);
    // The value store and the first reputation gate are one original block.
    adjust_proposal_value_for_reputation(p);

    block(c, 0x103754fc, 2);
    if (guest::compare_logical(c, p.destination, p.incumbent_club) == 0) {
        adjust_proposal_value_for_incumbent(p);
        adjust_proposal_value_for_role(p);
    } else {
        adjust_proposal_value_for_previous_club(p);
    }
}

void ensure_proposal_staff_role(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375844, 3);
    if (detail::record_nonzero(c, c.i8(p.person + 0x62))) return;

    block(c, 0x10375850, 5);
    c.set(4, c.toc(-0x4da4));
    c.set(3, c.toc(-0x7bb8));
    c.set(5, c.u32(p.person));
    c.set(4, c.r(4) + 0x1ea);
    c.call(0x1077b720, 0x10375864);
    block(c, 0x10375864, 2);
    const auto has_profile = guest::nonzero_logical(c, p.profile);
    block(c, has_profile ? 0x1037586cu : 0x10375878u, has_profile ? 3 : 2);
    const auto role = has_profile ? 11 : 9;
    c.set(0, role);
    c.put8(p.person + 0x62, role);
}

void calculate_reference_proposal_wage(ContractProposal& p, std::uint32_t pc, bool based_on_incumbent,
                                       unsigned adjustment, bool promoted = false, bool jump = true) {
    Cpu& c = p.c;
    block(c, pc, 8);
    c.set({{3, p.contract}, {4, p.person}, {5, p.destination}, {6, based_on_incumbent},
           {7, p.role}, {8, 11}, {9, adjustment}});
    c.call(reference_quote_entry, pc + 32);
    block(c, pc + 32, promoted ? 4 : jump ? 2 : 1);
    if (promoted) c.set(0, 1);
    c.set_f(29, c.f(1));
    if (promoted) c.put8(c.sp() + 0xb4, 1);
}

bool proposal_staff_status(ContractProposal& p, std::uint32_t pc, bool live_id) {
    Cpu& c = p.c;
    block(c, pc, live_id ? 6 : 7);
    const auto id = live_id ? c.r(0) : c.u32(p.person);
    c.set(0, id);
    const auto metadata = c.u32(c.r(14));
    const auto row = id * 100;
    c.set({{3, row}, {4, metadata}, {0, row + 12}});
    return detail::record_nonzero(c, (c.u8(metadata + row + 12) >> 4) & 0xf);
}

void read_proposal_days_remaining(ContractProposal& p, std::uint32_t pc) {
    Cpu& c = p.c;
    block(c, pc, 4);
    c.set(4, c.u32(p.today));
    c.set(3, p.incumbent + ContractRecord::end_date);
    c.set(5, c.u32(p.today + 4));
    c.call(0x10718130, pc + 16);
}

bool proposal_prefers_ordinary_renewal(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x103758b8, 3);
    const auto clean = !guest::nonzero(c, std::int32_t(p.conditions));
    c.set(13, c.r(3));
    if (!clean || proposal_staff_status(p, 0x103758c4, false)) return false;
    if (proposal_signed_short(p, 0x103758e0, p.profile, 10, 7250) >= 0) return false;
    block(c, 0x103758ec, 6);
    const auto experience = c.i8(p.person + 0x65);
    c.set(0, std::uint32_t(experience));
    const auto threshold = experience * 12 + 182;
    c.set({{3, std::uint32_t(experience * 12)}, {0, std::uint32_t(threshold)}});
    if (guest::compare(c, std::int32_t(c.r(13)), threshold) >= 0) return false;
    block(c, 0x10375904, 3);
    return !detail::record_nonzero(c, c.i8(p.incumbent + ContractRecord::terms_flag));
}

bool proposal_prefers_youth_renewal(ContractProposal& p) {
    Cpu& c = p.c;
    if (proposal_signed_byte(p, 0x10375938, p.person, 0x66, 6) >= 0) return false;
    block(c, 0x10375948, 4);
    const auto age = std::int8_t(c.i16(p.person + staff_age));
    c.set(0, std::uint32_t(age));
    if (guest::compare(c, age, 24) >= 0) return false;
    block(c, 0x10375958, 5);
    const auto slot = c.toc(-0x7b40);
    c.set({{3, slot}, {4, p.person}, {5, 0}});
    c.set(3, c.u32(slot));
    c.call(0x103f43b0, 0x1037596c);
    block(c, 0x1037596c, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(3)))) return false;
    block(c, 0x10375974, 3);
    c.set({{3, p.person}, {4, p.destination}});
    c.call(0x100d3150, 0x10375980);
    block(c, 0x10375980, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(3)))) return false;

    block(c, 0x10375988, 4);
    const auto profile = c.u32(p.person + staff_profile);
    c.set(3, profile);
    const auto reputation = c.i16(profile + 10);
    c.set(4, guest::extended(reputation));
    if (guest::compare(c, reputation, 3250) >= 0) return false;
    block(c, 0x10375998, 5);
    const auto club_reputation = c.i16(p.destination + club_rating);
    const auto margin = std::int16_t(club_reputation + 250);
    c.set({{3, guest::extended(club_reputation)}, {0, guest::extended(margin)}});
    if (guest::compare(c, reputation, margin) >= 0) return false;

    read_proposal_days_remaining(p, 0x103759ac);
    block(c, 0x103759bc, 5);
    const auto ambition = c.i8(p.person + 0x66);
    const auto margin_days = 200 - ambition;
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(ambition) <= 200);
    c.set(0, std::uint32_t(margin_days));
    return guest::compare(c, std::int32_t(c.r(3)), margin_days) < 0;
}

bool proposal_prefers_short_renewal(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x103759f8, 2);
    const std::uint32_t conditions = p.conditions;
    if (!guest::nonzero(c, std::int32_t(conditions))) return false;
    block(c, 0x10375a00, 4);
    c.set(3, 0x10000);
    if (detail::record_nonzero(c, conditions & 0x161a8)) return false;
    block(c, 0x10375a10, 13);
    c.set(3, 0xea0f0000);
    const auto id = c.i32(p.person);
    c.set(0, std::uint32_t(id));
    constexpr auto reciprocal = std::int32_t(0xea0ea0ebu);
    const auto high = (std::int64_t(reciprocal) * id) >> 32;
    const auto wrapped = std::int32_t(std::uint32_t(high) + std::uint32_t(id));
    const auto limit = id % 35 + 20;
    c.s.wr<1>(guest::reg::xer_ca, wrapped < 0 && (wrapped & 31));
    c.set({{3, std::uint32_t(limit)}, {4, unsigned(wrapped < 0)}});
    if (guest::compare(c, std::int32_t(c.r(13)), limit) >= 0) return false;
    if (proposal_staff_status(p, 0x10375a44, true)) return false;

    block(c, 0x10375a5c, 11);
    const auto professionalism = c.i8(p.person + 0x63);
    c.set(3, std::uint32_t(professionalism));
    const auto experience = c.i8(p.person + 0x65);
    c.set(0, std::uint32_t(experience));
    const auto reputation = c.i16(p.profile + 10);
    const auto threshold = experience * 25 + 4250;
    const auto score = reputation + professionalism * 20;
    c.set({{3, std::uint32_t(experience * 25)}, {4, std::uint32_t(score)},
           {5, guest::extended(reputation)}, {0, std::uint32_t(threshold)}});
    return guest::compare(c, score, threshold) < 0;
}

void quote_proposal_for_incumbent(ContractProposal& p) {
    const std::uint32_t profile = p.profile;
    read_proposal_days_remaining(p, 0x103758a8);
    if (proposal_prefers_ordinary_renewal(p)) {
        calculate_reference_proposal_wage(p, 0x10375910, true, 0);
        return;
    }
    if (proposal_prefers_youth_renewal(p)) {
        calculate_reference_proposal_wage(p, 0x103759d0, true, 0);
        return;
    }
    if (proposal_prefers_short_renewal(p)) {
        calculate_reference_proposal_wage(p, 0x10375a88, true, 0);
        return;
    }
    if ((proposal_signed_byte(p, 0x10375ab0, p.person, 0x65, 18) > 0
         || (proposal_signed_short(p, 0x10375ac0, profile, 10, 5250) < 0 && !proposal_staff_status(p, 0x10375acc, false)))
        && proposal_signed_short(p, 0x10375ae8, profile, 12, 3500) < 0) {
        calculate_reference_proposal_wage(p, 0x10375af4, true, 0);
        return;
    }
    calculate_reference_proposal_wage(p, 0x10375b1c, false, 0);
}

void quote_proposal_for_new_club(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375b44, 14);
    const auto professional_byte = c.u8(p.person + 0x63);
    c.set({{5, professional_byte}, {4, 0x20000}});
    const auto experience_byte = c.u8(p.person + 0x65);
    const auto professionalism = std::int8_t(professional_byte);
    const auto experience = std::int8_t(experience_byte);
    const auto combined = professionalism + experience;
    const auto score = combined * 125000;
    c.set({{6, std::uint32_t(professionalism)}, {3, std::uint32_t(combined)}, {5, 0},
           {4, std::uint32_t(score)}, {0, std::uint32_t(12500000 - score)}});
    if (guest::compare(c, 0, 12500000 - score) > 0) {
        block(c, 0x10375b7c, 5);
        const auto ambition = c.i8(p.person + 0x66);
        p.role = 1;
        c.set(0, std::uint32_t(ambition));
        auto promoted = guest::compare(c, ambition, 5) > 0;
        if (!promoted) {
            block(c, 0x10375b90, 4);
            const auto profile = c.u32(p.person + staff_profile);
            c.set(3, profile);
            const auto reputation = c.i16(profile + 12);
            c.set(0, guest::extended(reputation));
            promoted = guest::compare(c, reputation, 6250) > 0;
        }
        calculate_reference_proposal_wage(p, promoted ? 0x10375ba0u : 0x10375bd0u, false, promoted ? 1 : 2, true);
        return;
    }

    block(c, 0x10375c00, 5);
    c.set({{3, 0x990000}, {0, std::uint32_t(10000000 - score)}});
    if (guest::compare(c, 0, 10000000 - score) > 0) {
        block(c, 0x10375c14, 3);
        const auto role = std::int8_t(std::uint32_t(p.role));
        c.set(0, std::uint32_t(role));
        if (guest::compare(c, role, 1) != 0) {
            block(c, 0x10375c20, 1);
            p.role = 2;
        }
        calculate_reference_proposal_wage(p, 0x10375c24, false, 2);
        return;
    }
    block(c, 0x10375c4c, 2);
    if (!guest::nonzero(c, std::int32_t(c.r(5)))
        && proposal_signed_short(p, 0x10375c54, p.profile, 10, 6750) > 0)
        calculate_reference_proposal_wage(p, 0x10375c60, false, 3);
    else
        calculate_reference_proposal_wage(p, 0x10375c88, false, 0, false, false);
}

bool quote_proposal_for_profile(ContractProposal& p) {
    Cpu& c = p.c;
    ensure_proposal_staff_role(p);
    if (!proposal_profile_allowed(p, 0x10375880)) return false;
    block(c, 0x10375898, 2);
    const std::uint32_t previous = p.incumbent_club;
    if (guest::nonzero_logical(c, previous)) {
        block(c, 0x103758a0, 2);
        if (guest::compare_logical(c, previous, p.destination) == 0) {
            quote_proposal_for_incumbent(p);
            return true;
        }
    }
    quote_proposal_for_new_club(p);
    return true;
}

double signed_proposal_scratch(ContractProposal& p, unsigned input, unsigned scratch, bool single) {
    Cpu& c = p.c;
    const auto frame = c.sp() + scratch;
    const auto encoded = c.r(input) ^ 0x80000000;
    c.set({{input, encoded}, {0, double_header}});
    c.put32(frame + 4, encoded);
    const auto bias = p.number(0xc0);
    c.set_f(1, bias);
    c.put32(frame, double_header);
    c.set_f(0, c.f64(frame));
    const auto value = single ? p.fp.fsubs(c.f(0), bias) : p.fp.fsub(c.f(0), bias);
    c.set_f(0, value);
    return value;
}

unsigned proposal_growth_floor(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375d30, 2);
    const auto elevated = [&] {
        if (!guest::nonzero_logical(c, p.destination)) return false;
        block(c, 0x10375d38, 3);
        const auto league = c.u32(p.destination + 0x2c);
        c.set(4, league);
        if (!guest::nonzero_logical(c, league)) return false;
        block(c, 0x10375d44, 5);
        const auto region = c.u32(league + 0x68);
        c.set(5, region);
        const auto matches = [&](int toc_slot) {
            const auto slot = c.toc(toc_slot);
            c.set(4, slot);
            const auto region_id = c.u32(slot);
            c.set(0, region_id);
            return guest::compare(c, std::int32_t(region), std::int32_t(region_id)) == 0;
        };
        if (matches(-0x7a10)) return true;
        block(c, 0x10375d58, 4);
        if (matches(-0x7a18)) return true;
        block(c, 0x10375d68, 4);
        return matches(-0x7a14);
    }();
    block(c, elevated ? 0x10375d78 : 0x10375d84, elevated ? 3 : 2);
    const auto limit = elevated ? 75000 : 50000;
    c.set({{4, 0x10000}, {0, unsigned(limit)}});
    return limit;
}

void taper_proposal_growth(ContractProposal& p, std::uint32_t pc, unsigned limit, bool jump) {
    Cpu& c = p.c;
    block(c, pc, jump ? 18 : 17);
    const auto frame = c.sp();
    const auto encoded = limit ^ 0x80000000;
    c.set({{3, encoded}, {0, double_header}});
    c.put32(frame + 0xa0, encoded);
    const auto bias = p.number(0xc0);
    c.set_f(2, bias);
    c.put32(frame + 0x9c, double_header);
    c.set_f(3, p.number(0x48));
    c.set_f(0, c.f64(frame + 0x9c));
    c.put32(frame + 0xa8, encoded);
    c.set_f(1, p.fp.fsubs(c.f(0), bias));
    c.put32(frame + 0xa4, double_header);
    c.set_f(0, c.f64(frame + 0xa4));
    c.set_f(1, p.fp.fsubs(c.f(29), c.f(1)));
    c.set_f(0, p.fp.fsub(c.f(0), bias));
    c.set_f(0, p.fp.multiply_add(pc + 52, c.f(3), c.f(1), c.f(0)));
    c.put64(frame + 0xac, p.fp.fctiwz(0, c.f(0)));
    c.set(3, c.u32(frame + 0xb0));
}

void limit_proposal_growth(ContractProposal& p, std::uint32_t pc, unsigned minimum, bool incumbent) {
    Cpu& c = p.c;
    block(c, pc, 2);
    if (guest::compare(c, std::int32_t(c.r(3)), std::int32_t(minimum)) < 0) {
        block(c, pc + 8, 2);
        c.set(3, minimum);
        return;
    }
    taper_proposal_growth(p, pc + 16, minimum, incumbent);
}

void scale_proposal_growth_rating(ContractProposal& p, int rating, int level) {
    Cpu& c = p.c;
    const auto scratch = c.sp() + 0x9c;
    c.put32(scratch, double_header);
    const auto product = std::uint32_t(rating * level);
    c.set(4, product);
    c.set_f(1, p.number(0xc0));
    c.set_f(2, p.number(0xd8));
    const auto encoded = product ^ 0x80000000;
    c.set(4, encoded);
    c.put32(scratch + 4, encoded);
    c.set_f(0, c.f64(scratch));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, p.fp.fmul(c.f(2), c.f(0)));
    c.put64(scratch + 8, p.fp.fctiwz(0, c.f(0)));
    c.set(4, c.u32(scratch + 12));
}

void adjust_proposal_growth_for_destination(ContractProposal& p, unsigned minimum) {
    Cpu& c = p.c;
    block(c, 0x10375dec, 2);
    if (!guest::nonzero_logical(c, p.destination)) return;
    block(c, 0x10375df4, 3);
    const auto competition = c.u32(p.destination + 0x30);
    c.set(5, competition);
    if (guest::nonzero_logical(c, competition)) {
        block(c, 0x10375e00, 16);
        const auto rating = c.i16(p.destination + club_rating);
        c.set({{6, guest::extended(rating)}, {4, double_header}});
        const auto level = c.i16(competition + 0x18);
        c.set(5, guest::extended(level));
        scale_proposal_growth_rating(p, rating, level);
    } else {
        block(c, 0x10375e40, 3);
        const auto league = c.u32(p.destination + 0x2c);
        c.set(4, league);
        if (guest::nonzero_logical(c, league)) {
            block(c, 0x10375e4c, 17);
            const auto raw_level = c.u8(league + 0x11f);
            c.set({{5, raw_level}, {4, double_header}});
            const auto rating = c.i16(p.destination + club_rating);
            const auto level = std::int8_t(raw_level);
            c.set({{6, guest::extended(rating)}, {5, std::uint32_t(level)}});
            scale_proposal_growth_rating(p, rating, level);
        } else {
            block(c, 0x10375e90, 1);
            c.set(4, guest::extended(c.i16(p.destination + club_rating)));
        }
    }
    block(c, 0x10375e94, 3);
    const auto rating = std::int16_t(c.r(4));
    c.set(4, guest::extended(rating));
    if (guest::compare(c, rating, 5750) > 0) limit_proposal_growth(p, 0x10375ea0, minimum, false);
}

void cap_proposal_growth(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375cac, 2);
    const std::uint32_t existing = p.incumbent;
    if (!guest::nonzero_logical(c, existing)) return;
    block(c, 0x10375cb4, 10);
    const auto old_wage = c.u32(existing + 12);
    c.set({{3, old_wage}, {0, double_header}});
    const auto frame = c.sp();
    c.put32(frame + 0x9c, double_header);
    const auto encoded = old_wage ^ 0x80000000;
    c.set(0, encoded);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0xa0, encoded);
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(1)));
    p.fp.fcmp(0, c.f(0), c.f(29));
    if (!(c.cr(0) & 8)) return;

    block(c, 0x10375cdc, 4);
    auto ceiling = old_wage + 8000;
    const auto doubled = old_wage * 2;
    c.set({{4, ceiling}, {0, doubled}});
    if (guest::compare(c, std::int32_t(doubled), std::int32_t(ceiling)) > 0) {
        block(c, 0x10375cec, 1);
        ceiling = doubled;
        c.set(4, doubled);
    }
    block(c, 0x10375cf0, 4);
    const auto reputation = c.i16(p.profile + 12);
    c.set({{0, guest::extended(reputation)}, {3, ceiling}});
    const auto needs_taper = [&] {
        if (guest::compare(c, reputation, 6750) <= 0) return false;
        block(c, 0x10375d00, 3);
        c.set_f(0, p.single(0x14));
        p.fp.fcmp(0, c.f(29), c.f(0));
        if (!(c.cr(0) & 4)) return false;
        block(c, 0x10375d0c, 9);
        signed_proposal_scratch(p, 4, 0x9c, true);
        p.fp.fcmp(0, c.f(29), c.f(0));
        return (c.cr(0) & 4) != 0;
    };
    if (needs_taper()) {
        const auto minimum = proposal_growth_floor(p);
        block(c, 0x10375d8c, 2);
        if (guest::compare_logical(c, p.incumbent_club, p.destination) == 0) limit_proposal_growth(p, 0x10375d94, minimum, true);
        else adjust_proposal_growth_for_destination(p, minimum);
    }

    block(c, 0x10375ef4, 9);
    signed_proposal_scratch(p, 3, 0x9c, true);
    p.fp.fcmp(0, c.f(29), c.f(0));
    if (c.cr(0) & 4) {
        block(c, 0x10375f18, 4);
        c.put32(frame + 0xa0, c.r(3));
        c.put32(frame + 0x9c, c.r(0));
        c.set_f(0, c.f64(frame + 0x9c));
        c.set_f(29, p.fp.fsubs(c.f(0), c.f(1)));
    }
}

void lookup_proposal_wage_service(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375f28, 4);
    const auto slot = c.toc(-0x7ab8);
    c.set({{3, slot}, {4, p.destination}});
    c.set(3, c.u32(slot));
    c.call(0x10103510, 0x10375f38);
    block(c, 0x10375f38, 4);
    const auto service = c.r(3);
    const auto has_service = detail::record_nonzero(c, std::int32_t(service));
    c.put32(c.sp() + 0xb8, service);
    c.set(13, 0);
    if (has_service) {
        block(c, 0x10375f48, 1);
        c.set(13, c.u32(service + 0x10));
    }
}

bool senior_proposal_incumbent(ContractProposal& p) {
    Cpu& c = p.c;
    const std::uint32_t incumbent = p.incumbent;
    block(c, 0x1037600c, 2);
    if (guest::compare_logical(c, p.incumbent_club, p.destination) != 0) return false;
    block(c, 0x10376014, 2);
    if (!guest::nonzero_logical(c, incumbent)) return false;
    if (proposal_signed_byte(p, 0x1037601c, incumbent, 0x44, 50) <= 0) return false;
    block(c, 0x1037602c, 4);
    c.set(3, c.u32(p.manager_slot));
    c.set({{4, p.person}, {5, incumbent}});
    c.call(0x1007c140, 0x1037603c);
    block(c, 0x1037603c, 3);
    const auto current_role = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(current_role));
    if (guest::compare(c, current_role, 1) != 0) return false;
    if (proposal_signed_byte(p, 0x10376048, p.person, 0x65, 15) >= 0
        || proposal_signed_byte(p, 0x10376058, p.person, 0x63, 10) <= 0
        || proposal_signed_byte(p, 0x10376068, p.person, 0x66, 10) <= 0)
        return false;
    block(c, 0x10376078, 2);
    return guest::compare(c, std::int32_t(c.r(15)), 6750) > 0
        && proposal_signed_short(p, 0x10376080, p.profile, 12, 4250) > 0;
}

bool proposal_merits_wage_uplift(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10375f4c, 18);
    const auto professional_byte = c.u8(p.person + 0x63);
    c.set(4, professional_byte);
    const auto role = std::int8_t(std::uint32_t(p.role));
    c.set(0, std::uint32_t(role));
    const auto ambition_byte = c.u8(p.person + 0x66);
    c.set(3, ambition_byte);
    const auto top_role = guest::compare(c, role, 1) == 0;
    const auto professionalism = std::int8_t(professional_byte);
    c.set(4, std::uint32_t(professionalism));
    const auto experience_byte = c.u8(p.person + 0x65);
    c.set({{5, experience_byte}, {3, std::uint32_t(std::int8_t(ambition_byte))}});
    const auto reputation = c.i16(p.profile + 12);
    const auto experience = std::int8_t(experience_byte);
    const auto full_score = reputation + professionalism * 25 + std::int8_t(ambition_byte) * 10 - experience * 25;
    const auto score = std::int16_t(full_score);
    c.set({{0, guest::extended(reputation)}, {5, std::uint32_t(experience)}, {14, 0},
           {4, std::uint32_t(experience * 25)}, {3, std::uint32_t(full_score)}, {15, guest::extended(score)}});
    const std::uint32_t incumbent = p.incumbent;
    if (top_role) {
        block(c, 0x10375f94, 2);
        if (guest::compare(c, std::int32_t(p.age), 18) > 0) {
            block(c, 0x10375f9c, 2);
            if (guest::nonzero_logical(c, incumbent)) {
                block(c, 0x10375fa4, 4);
                const auto status = c.i8(incumbent + 0x44);
                c.set(3, std::uint32_t(status));
                if (guest::compare(c, status, 50) > 0) {
                    block(c, 0x10375fb4, 2);
                    return true;
                }
            }
        }
    }

    block(c, 0x10375fbc, 2);
    if (guest::compare(c, std::int32_t(c.r(0)), 4250) > 0) {
        block(c, 0x10375fc4, 3);
        const auto rating = c.i16(p.destination + club_rating);
        c.set(0, guest::extended(rating));
        bool merit = guest::compare(c, score, rating) > 0;
        if (!merit) {
            block(c, 0x10375fd0, 2);
            if (guest::compare(c, score, 6750) > 0) {
                block(c, 0x10375fd8, 3);
                const auto known = c.u8(p.person + 0x5e);
                c.set(0, known);
                if (guest::nonzero_logical(c, known)) {
                    block(c, 0x10375fe4, 3);
                    c.set(0, std::uint32_t(role));
                    if (guest::compare(c, role, 3) < 0) {
                        block(c, 0x10375ff0, 2);
                        merit = detail::record_nonzero(c, role);
                    }
                }
            }
        }
        if (merit) {
            block(c, 0x10375ff8, 2);
            c.set(0, std::uint32_t(role));
            const auto supported = guest::compare(c, role, 7) != 0;
            block(c, 0x10376000, 1);
            if (supported) {
                block(c, 0x10376004, 2);
                return true;
            }
        }
    }
    if (!senior_proposal_incumbent(p)) return false;
    block(c, 0x1037608c, 1);
    return true;
}

void check_proposal_uplift_duration(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376090, 2);
    if (!detail::record_nonzero(c, std::int8_t(c.r(14)))) return;
    block(c, 0x10376098, 2);
    const std::uint32_t incumbent = p.incumbent;
    if (!guest::nonzero_logical(c, incumbent)) return;

    read_proposal_days_remaining(p, 0x103760a0);
    block(c, 0x103760b0, 6);
    const auto ambition = c.i8(p.person + 0x66);
    const auto threshold = ambition * 10 + 525;
    c.set({{4, std::uint32_t(ambition * 10)}, {0, std::uint32_t(threshold)}});
    if (guest::compare(c, std::int32_t(c.r(3)), threshold) > 0) {
        block(c, 0x103760c8, 2);
        c.set(14, 0);
        return;
    }
    if (proposal_signed_byte(p, 0x103760d0, incumbent, 0x46, 2) < 0
        && proposal_signed_short(p, 0x103760e0, p.profile, 12, 5750) < 0) {
        block(c, 0x103760ec, 1);
        c.set(14, 0);
    }
}

bool check_proposal_uplift_club(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x103760f0, 2);
    if (!detail::record_nonzero(c, std::int8_t(c.r(14)))) return false;
    block(c, 0x103760f8, 2);
    c.set(3, p.destination);
    c.call(0x100de260, 0x10376100);
    block(c, 0x10376100, 2);
    if (!detail::record_nonzero(c, std::int8_t(c.r(3)))) return false;
    block(c, 0x10376108, 2);
    const auto age = std::int32_t(p.age);
    if (guest::compare(c, age, 18) <= 0) return false;
    block(c, 0x10376110, 2);
    return guest::compare(c, age, 32) < 0;
}

bool prepare_proposal_wage_uplift(ContractProposal& p) {
    lookup_proposal_wage_service(p);
    p.c.set(14, unsigned(proposal_merits_wage_uplift(p)));
    check_proposal_uplift_duration(p);
    return check_proposal_uplift_club(p);
}

bool proposal_region_matches(ContractProposal& p, int toc_offset, std::uint32_t region) {
    Cpu& c = p.c;
    const auto slot = c.toc(toc_offset);
    c.set(3, slot);
    const auto expected = c.u32(slot);
    c.set(0, expected);
    return guest::compare(c, std::int32_t(region), std::int32_t(expected)) == 0;
}

void raise_proposal_reference_amount(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376118, 3);
    const auto score = std::int32_t(c.r(15));
    const bool high_score = guest::compare(c, score, 7250) > 0;
    c.set(4, c.r(13));
    if (!high_score) return;

    block(c, 0x10376124, 3);
    const auto profile = c.u32(p.person + staff_profile);
    c.set(3, profile);
    if (!guest::nonzero_logical(c, profile)) return;
    block(c, 0x10376130, 3);
    const auto prospect = c.u8(profile + 15);
    c.set(0, prospect);
    if (guest::compare(c, prospect, 20) == 0) return;
    block(c, 0x1037613c, 3);
    const auto experience = c.u8(p.person + 0x65);
    c.set(0, experience);
    if (guest::compare(c, experience, 20) == 0) return;

    block(c, 0x10376148, 16);
    const auto frame = c.sp();
    const auto encoded = std::uint32_t(score) ^ 0x80000000;
    c.set({{3, encoded}, {0, double_header}});
    c.put32(frame + 0xa0, encoded);
    const auto league = c.u32(p.destination + 0x2c);
    c.set(3, league);
    c.put32(frame + 0x9c, double_header);
    const auto bias = p.number(0xc0);
    c.set_f(1, bias);
    c.set_f(0, c.f64(frame + 0x9c));
    const auto factor = p.number(0xe0);
    c.set_f(2, factor);
    c.set_f(0, p.fp.fsub(c.f(0), bias));
    const auto region = c.u32(league + 0x68);
    c.set(5, region);
    const auto slot = c.toc(-0x7a10);
    c.set(3, slot);
    c.set_f(1, p.fp.fmul(factor, c.f(0)));
    const auto primary_region = c.u32(slot);
    c.set(0, primary_region);
    const bool preferred = guest::compare(c, std::int32_t(region), std::int32_t(primary_region)) == 0;
    c.set_f(1, p.fp.frsp(c.f(1)));

    const auto apply_region_floor = [&](std::uint32_t pc, unsigned rate, bool jump) {
        block(c, pc, 7);
        c.set_f(0, p.single(rate));
        c.set_f(0, p.fp.fmuls(c.f(0), c.f(1)));
        c.put64(frame + 0x9c, p.fp.fctiwz(0, c.f(0)));
        const auto floor = c.u32(frame + 0xa0);
        c.set(0, floor);
        if (guest::compare(c, std::int32_t(c.r(13)), std::int32_t(floor)) < 0) {
            block(c, pc + 28, jump ? 2 : 1);
            c.set(4, floor);
        }
    };
    if (preferred) {
        apply_region_floor(0x10376188, 0x18, true);
        return;
    }
    block(c, 0x103761ac, 4);
    if (!proposal_region_matches(p, -0x7a18, region)) {
        block(c, 0x103761bc, 4);
        if (!proposal_region_matches(p, -0x7a14, region)) return;
    }
    apply_region_floor(0x103761cc, 0x1c, false);
}

void cap_proposal_prospect_reference(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x103761ec, 3);
    const auto profile = c.u32(p.person + staff_profile);
    c.set(3, profile);
    if (!guest::nonzero_logical(c, profile)) return;
    block(c, 0x103761f8, 3);
    const auto prospect = c.u8(profile + 15);
    c.set(0, prospect);
    if (guest::compare(c, prospect, 20) != 0) return;
    block(c, 0x10376204, 4);
    constexpr int limit = 60000;
    c.set({{3, 0x10000}, {0, limit}});
    if (guest::compare(c, std::int32_t(c.r(4)), limit) > 0) {
        block(c, 0x10376214, 1);
        c.set(4, limit);
    }
}

void proposal_uplift_trait_rate(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376218, 22);
    const auto raw_professionalism = c.u8(p.person + 0x63);
    c.set({{3, raw_professionalism}, {5, double_header}});
    const auto raw_experience = c.u8(p.person + 0x65);
    c.set(0, raw_experience);
    const auto professionalism = std::int8_t(raw_professionalism);
    c.set(3, std::uint32_t(professionalism));
    const auto raw_ambition = c.u8(p.person + 0x67);
    const auto experience = std::int8_t(raw_experience);
    c.set({{7, raw_ambition}, {6, std::uint32_t(experience)}});
    const auto frame = c.sp();
    c.put32(frame + 0x9c, double_header);
    const auto ambition = std::int8_t(raw_ambition);
    c.set({{5, std::uint32_t(professionalism + 4 - experience)}, {7, std::uint32_t(ambition)}});
    const auto bias = p.number(0xc0);
    c.set_f(2, bias);
    const auto merit = professionalism + 4 - experience - ambition;
    c.set(5, std::uint32_t(merit));
    const auto coefficient = p.number(0xe8);
    c.set_f(3, coefficient);
    const auto encoded = std::uint32_t(merit) ^ 0x80000000;
    c.set(5, encoded);
    c.set_f(0, p.number(0xf0));
    c.put32(frame + 0xa0, encoded);
    c.set_f(1, c.f64(frame + 0x9c));
    c.set_f(1, p.fp.fsubs(c.f(1), bias));
    c.set_f(1, p.fp.fmul(c.f(1), coefficient));
    c.set_f(1, p.fp.frsp(c.f(1)));
    p.fp.fcmp(0, c.f(1), c.f(0));
    if ((c.cr(0) & 8) != 0) {
        block(c, 0x10376270, 2);
        c.set_f(1, p.single(0x20));
    } else {
        block(c, 0x10376278, 3);
        c.set_f(0, p.number(0x78));
        p.fp.fcmp(0, c.f(1), c.f(0));
        if ((c.cr(0) & 4) != 0) {
            block(c, 0x10376284, 1);
            c.set_f(1, p.single(0x24));
        }
    }
}

void reduce_proposal_uplift_rate(ContractProposal& p) {
    Cpu& c = p.c;
    const auto subtract = [&](std::uint32_t pc, unsigned offset, bool jump) {
        block(c, pc, jump ? 4 : 3);
        c.set_f(0, p.number(offset));
        c.set_f(1, p.fp.fsub(c.f(1), c.f(0)));
        c.set_f(1, p.fp.frsp(c.f(1)));
    };
    const auto low_ambition = [&](std::uint32_t pc) {
        block(c, pc, 4);
        const auto ambition = c.i8(p.person + 0x66);
        c.set(5, std::uint32_t(ambition));
        return guest::compare(c, ambition, 15) < 0;
    };
    block(c, 0x10376288, 2);
    const auto score = std::int32_t(c.r(15));
    if (guest::compare(c, std::int32_t(p.age), 29) > 0) {
        block(c, 0x10376290, 2);
        if (guest::compare(c, score, 5250) < 0) subtract(0x10376298, 0x88, true);
        else if (low_ambition(0x103762a8)) subtract(0x103762b8, 0xd8, true);
    } else {
        block(c, 0x103762c8, 2);
        if (guest::compare(c, score, 4250) < 0 && low_ambition(0x103762d0))
            subtract(0x103762e0, 0xd8, false);
    }

    block(c, 0x103762ec, 8);
    const auto rating = c.i16(p.destination + club_rating);
    const auto professionalism = std::int32_t(c.r(3)) * 25;
    c.set({{5, guest::extended(rating)}, {6, std::uint32_t(professionalism)}});
    const auto reputation = c.i16(p.profile + 10);
    c.set(7, guest::extended(reputation));
    const auto threshold = std::int16_t(rating - 1250);
    const auto weighted = reputation + professionalism;
    c.set({{3, guest::extended(threshold)}, {5, std::uint32_t(weighted)}});
    if (guest::compare(c, weighted, threshold) < 0) subtract(0x1037630c, 0xd8, false);
}

void apply_proposal_uplift_floor(ContractProposal& p, std::uint32_t pc, unsigned base, bool jump = true) {
    Cpu& c = p.c;
    block(c, pc, 12);
    const auto encoded = c.r(4) ^ 0x80000000;
    const auto frame = c.sp();
    c.set({{3, encoded}, {0, double_header}});
    c.put32(frame + 0xa0, encoded);
    c.set_f(0, p.number(base));
    c.put32(frame + 0x9c, double_header);
    c.set_f(2, p.number(0xc0));
    c.set_f(0, p.fp.fadd(c.f(0), c.f(1)));
    c.set_f(1, c.f64(frame + 0x9c));
    c.set_f(1, p.fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, p.fp.fmul(c.f(1), c.f(0)));
    p.fp.fcmp(0, c.f(29), c.f(0));
    if ((c.cr(0) & 8) == 0) return;
    block(c, pc + 48, jump ? 2 : 1);
    c.set_f(29, p.fp.frsp(c.f(0)));
}

void raise_proposal_wage(ContractProposal& p) {
    raise_proposal_reference_amount(p);
    cap_proposal_prospect_reference(p);
    proposal_uplift_trait_rate(p);
    reduce_proposal_uplift_rate(p);

    Cpu& c = p.c;
    const auto max_trait = [&](std::uint32_t pc, unsigned field) {
        block(c, pc, 3);
        const auto trait = c.u8(p.profile + field);
        c.set(3, trait);
        return guest::compare(c, trait, 20) == 0;
    };
    if (max_trait(0x10376318, 15)) {
        apply_proposal_uplift_floor(p, 0x10376324, 0xf8);
        return;
    }
    if (max_trait(0x1037635c, 17)) {
        apply_proposal_uplift_floor(p, 0x10376368, 0x100);
        return;
    }
    const auto score = std::int32_t(c.r(15));
    if (max_trait(0x103763a0, 21)) {
        block(c, 0x103763ac, 2);
        if (guest::compare(c, score, 7250) > 0) {
            block(c, 0x103763b4, 3);
            const auto experience = std::int8_t(c.r(0));
            c.set(0, std::uint32_t(experience));
            if (guest::compare(c, experience, 6) < 0) {
                apply_proposal_uplift_floor(p, 0x103763c0, 0x108);
                return;
            }
        }
    }
    block(c, 0x103763f8, 2);
    if (guest::compare(c, score, 7250) > 0) apply_proposal_uplift_floor(p, 0x10376400, 0x10);
    else apply_proposal_uplift_floor(p, 0x10376438, 0xd0, false);
}

void read_proposal_interest(ContractProposal& p) {
    Cpu& c = p.c;
    const auto frame = c.sp();
    block(c, 0x1037646c, 10);
    const auto slot = c.toc(-0x7b40);
    c.set({{3, slot}, {4, p.person}});
    const auto purpose = c.u32(frame + 0x160);
    c.set({{9, purpose}, {5, p.destination}});
    const auto manager = c.u32(slot);
    c.set({{3, manager}, {8, p.role}, {6, 0}, {7, 0}});
    c.set(3, c.u32(manager + 0x988));
    c.call(0x10369080, 0x10376494);

    block(c, 0x10376494, 4);
    const auto output = c.u32(frame + 0x164);
    c.set(0, output);
    c.set_f(2, p.fp.frsp(c.f(1)));
    if (guest::nonzero_logical(c, output)) {
        block(c, 0x103764a4, 2);
        c.set(3, output);
        c.put_f32(output, float(c.f(2)));
    }
    block(c, 0x103764ac, 4);
    c.set_f(0, p.number(0x88));
    c.set_f(1, p.number(0x110));
    p.fp.fcmp(0, c.f(0), c.f(2));
    if ((c.cr(0) & 4) != 0) block(c, 0x103764bc, 1);
    else {
        block(c, 0x103764c0, 1);
        c.set_f(0, c.f(2));
    }
    block(c, 0x103764c4, 5);
    c.set_f(0, p.fp.frsp(c.f(0)));
    guest::nonzero_logical(c, p.incumbent);
    c.set_f(30, p.fp.fdiv(c.f(1), c.f(0)));
    c.set_f(30, p.fp.frsp(c.f(30)));
}

void cap_proposal_multiplier(ContractProposal& p, std::uint32_t pc, unsigned limit, unsigned replacement, bool jump = true) {
    Cpu& c = p.c;
    block(c, pc, 3);
    c.set_f(0, p.number(limit));
    p.fp.fcmp(0, c.f(30), c.f(0));
    if ((c.cr(0) & 4) != 0) {
        block(c, pc + 12, jump ? 2 : 1);
        c.set_f(30, p.single(replacement));
    }
}

bool has_proposal_retention_discount(ContractProposal& p) {
    Cpu& c = p.c;
    if (proposal_signed_byte(p, 0x10376508, p.person, 0x66, 6) > 0) return false;
    block(c, 0x10376518, 4);
    const auto age = std::int8_t(c.i16(p.person + staff_age));
    c.set(0, std::uint32_t(age));
    if (guest::compare(c, age, 23) > 0) return false;
    block(c, 0x10376528, 5);
    const auto slot = c.toc(-0x7b40);
    c.set({{3, slot}, {4, p.person}, {5, 0}});
    c.set(3, c.u32(slot));
    c.call(0x103f43b0, 0x1037653c);
    block(c, 0x1037653c, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(3)))) return false;
    block(c, 0x10376544, 3);
    c.set({{3, p.person}, {4, p.destination}});
    c.call(0x100d3150, 0x10376550);
    block(c, 0x10376550, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(3)))) return false;
    block(c, 0x10376558, 4);
    const auto profile = c.u32(p.person + staff_profile);
    c.set(3, profile);
    const auto reputation = c.i16(profile + 10);
    c.set(4, guest::extended(reputation));
    if (guest::compare(c, reputation, 3249) > 0) return false;
    block(c, 0x10376568, 5);
    const auto rating = c.i16(p.destination + club_rating);
    const auto margin = std::int16_t(rating + 250);
    c.set({{3, guest::extended(rating)}, {0, guest::extended(margin)}});
    if (guest::compare(c, reputation, margin) > 0) return false;
    read_proposal_days_remaining(p, 0x1037657c);
    block(c, 0x1037658c, 5);
    const auto ambition = c.i8(p.person + 0x66);
    const auto days = 200 - ambition;
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(ambition) <= 200);
    c.set(0, std::uint32_t(days));
    return guest::compare(c, std::int32_t(c.r(3)), days) <= 0;
}

void adjust_proposal_wage_for_interest(ContractProposal& p) {
    Cpu& c = p.c;
    const auto low_interest = [&](std::uint32_t pc) {
        block(c, pc, 3);
        c.set_f(0, p.number(0x120));
        p.fp.fcmp(0, c.f(2), c.f(0));
        return (c.cr(0) & 8) != 0;
    };
    const auto add_supplement = [&](std::uint32_t pc, bool jump) {
        block(c, pc, jump ? 3 : 2);
        c.set_f(0, p.single(0x2c));
        c.set_f(29, p.fp.fadds(c.f(29), c.f(0)));
    };
    if (p.incumbent == 0) {
        if (low_interest(0x103765ac)) add_supplement(0x103765b8, false);
        return;
    }

    block(c, 0x103764d8, 2);
    const std::uint32_t conditions = p.conditions;
    if (detail::record_nonzero(c, conditions & 2)) {
        cap_proposal_multiplier(p, 0x103764e0, 0x118, 0x28);
        return;
    }
    block(c, 0x103764f4, 2);
    if (guest::nonzero(c, std::int32_t(conditions)) && low_interest(0x103764fc) && !has_proposal_retention_discount(p))
        add_supplement(0x103765a0, true);
}

void cap_proposal_multiplier_for_conditions(ContractProposal& p) {
    Cpu& c = p.c;
    const std::uint32_t conditions = p.conditions;
    block(c, 0x103765ec, 2);
    if (detail::record_nonzero(c, conditions & 2)) {
        cap_proposal_multiplier(p, 0x103765f4, 0x128, 0x34);
        return;
    }
    block(c, 0x10376608, 2);
    if (guest::compare(c, std::int32_t(p.age), 21) < 0
        && proposal_signed_short(p, 0x10376610, p.profile, 10, 4250) < 0) {
        block(c, 0x1037661c, 2);
        if (guest::compare(c, std::int32_t(conditions), 0x10) == 0) {
            cap_proposal_multiplier(p, 0x10376624, 0x128, 0x34);
            return;
        }
    }
    block(c, 0x10376638, 2);
    if (detail::record_nonzero(c, conditions & 1)) {
        cap_proposal_multiplier(p, 0x10376640, 0x120, 0x38);
        return;
    }
    block(c, 0x10376654, 2);
    if (detail::record_nonzero(c, conditions & 0x8400))
        cap_proposal_multiplier(p, 0x1037665c, 0x130, 0x3c);
    else
        cap_proposal_multiplier(p, 0x10376670, 0x138, 0x40);
}

bool proposal_contract_expires_soon(ContractProposal& p, std::uint32_t pc) {
    Cpu& c = p.c;
    block(c, pc, 16);
    c.set(3, 0x66660000);
    const auto id = c.i32(p.person);
    c.set({{4, std::uint32_t(id)}, {0, 0x66666667}});
    const auto raw_experience = c.u8(p.person + 0x65);
    c.set(3, raw_experience);
    const auto experience = std::int8_t(raw_experience);
    const auto high = (INT64_C(0x66666667) * id) >> 32;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 1));
    const auto remainder = std::int8_t(id % 5);
    const auto minimum = int(remainder) + 10;
    c.set({{5, std::uint32_t(experience)}, {3, std::uint32_t(remainder)}, {0, std::uint32_t(minimum)}});
    if (guest::compare(c, experience, minimum) <= 0) return false;
    read_proposal_days_remaining(p, pc + 64);
    block(c, pc + 80, 2);
    return guest::compare(c, std::int32_t(c.r(3)), 30) < 0;
}

void cap_proposal_multiplier_for_role(ContractProposal& p, std::uint32_t start, bool narrow_age_range) {
    Cpu& c = p.c;
    block(c, start, 4);
    c.set(3, c.u32(p.manager_slot));
    c.set({{4, p.person}, {5, p.incumbent}});
    c.call(0x1007c140, start + 16);
    block(c, start + 16, 3);
    const auto role = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(role));
    const auto cap_role = [&](unsigned category) {
        if (category == 1) cap_proposal_multiplier(p, start + 64, 0x140, 0x44);
        else if (category == 2 && !narrow_age_range) cap_proposal_multiplier(p, start + 104, 0x148, 0x48);
        else if (category == 2) cap_proposal_multiplier(p, start + 84, 0x130, 0x3c);
        else if (category == 3 && !narrow_age_range) cap_proposal_multiplier(p, start + 84, 0x118, 0x28);
        else if (category == 3) cap_proposal_multiplier(p, start + 104, 0x148, 0x48);
        else cap_proposal_multiplier(p, start + 124, 0x150, 0x4c);
    };
    if (guest::compare(c, role, 2) == 0) {
        cap_role(2);
        return;
    }
    block(c, start + 28, 1);
    if (role < 2) {
        block(c, start + 32, 2);
        if (guest::compare(c, role, 1) >= 0) {
            cap_role(1);
            return;
        }
        block(c, start + 40, 1);
    } else {
        block(c, start + 44, 2);
        if (guest::compare(c, role, 8) < 0) {
            block(c, start + 52, 2);
            if (guest::compare(c, role, 6) < 0) {
                block(c, start + 60, 1);
                cap_role(3);
                return;
            }
        }
    }
    cap_role(0);
}

void cap_proposal_multiplier_for_age(ContractProposal& p) {
    Cpu& c = p.c;
    const auto age = std::int32_t(p.age);
    block(c, 0x10376684, 1);
    // The preceding conditions comparison is still live on this entry.
    if ((c.cr(0) & 2) != 0 && proposal_contract_expires_soon(p, 0x10376688)) {
        cap_proposal_multiplier(p, 0x103766e0, 0x118, 0x28);
        return;
    }
    const auto outside = [&](std::uint32_t pc, int first, int last) {
        block(c, pc, 2);
        if (guest::compare(c, age, first) < 0) return true;
        block(c, pc + 8, 2);
        return guest::compare(c, age, last) > 0;
    };
    if (outside(0x103766f4, 19, 34)) {
        cap_proposal_multiplier_for_role(p, 0x10376704, false);
        return;
    }
    if (outside(0x10376794, 21, 31)) {
        cap_proposal_multiplier_for_role(p, 0x103767a4, true);
        return;
    }
    block(c, 0x10376834, 2);
    if (!guest::nonzero(c, std::int32_t(p.conditions)) && proposal_contract_expires_soon(p, 0x1037683c))
        cap_proposal_multiplier(p, 0x10376894, 0x118, 0x28);
    else
        cap_proposal_multiplier(p, 0x103768a8, 0x140, 0x44, false);
}

void cap_young_proposal_multiplier(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x103768b8, 2);
    if (guest::nonzero(c, std::int32_t(p.conditions))) return;
    block(c, 0x103768c0, 3);
    c.set_f(0, p.number(0x158));
    p.fp.fcmp(0, c.f(30), c.f(0));
    if ((c.cr(0) & 4) == 0) return;
    block(c, 0x103768cc, 2);
    if (guest::compare(c, std::int32_t(p.age), 20) >= 0) return;

    block(c, 0x103768d4, 9);
    const auto professionalism = c.i8(p.person + 0x63);
    c.set(0, std::uint32_t(professionalism));
    const auto raw_ambition = c.u8(p.person + 0x66);
    c.set(3, raw_ambition);
    const auto starts = c.i16(p.profile + 4);
    const auto ambition = std::int8_t(raw_ambition);
    const auto maximum = 125 - professionalism - ambition;
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(professionalism) <= 125);
    c.set({{4, guest::extended(starts)}, {3, std::uint32_t(ambition)}, {0, std::uint32_t(maximum)}});
    if (guest::compare(c, starts, maximum) >= 0
        || proposal_signed_short(p, 0x103768f8, p.profile, 10, 4250) >= 0) return;

    read_proposal_days_remaining(p, 0x10376904);
    block(c, 0x10376914, 8);
    const auto professionalism_now = c.i8(p.person + 0x63);
    c.set(0, std::uint32_t(professionalism_now));
    const auto ambition_now = c.i8(p.person + 0x66);
    const auto deadline = 120 - professionalism_now - ambition_now;
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(professionalism_now) <= 120);
    c.set({{4, std::uint32_t(ambition_now)}, {0, std::uint32_t(deadline)}});
    if (guest::compare(c, std::int32_t(c.r(3)), deadline) < 0) {
        block(c, 0x10376934, 2);
        c.set_f(30, p.single(0x50));
    }
}

void compute_proposal_wage_multiplier(ContractProposal& p) {
    read_proposal_interest(p);
    adjust_proposal_wage_for_interest(p);

    Cpu& c = p.c;
    block(c, 0x103765c0, 3);
    c.set_f(0, p.number(0xd0));
    p.fp.fcmp(0, c.f(30), c.f(0));
    if ((c.cr(0) & 8) != 0) {
        block(c, 0x103765cc, 2);
        c.set_f(30, p.single(0x30));
        return;
    }
    block(c, 0x103765d4, 2);
    if (guest::compare_logical(c, p.incumbent_club, p.destination) != 0) return;
    block(c, 0x103765dc, 2);
    if (!guest::nonzero_logical(c, p.incumbent)) {
        cap_proposal_multiplier(p, 0x1037693c, 0x160, 0x54, false);
        return;
    }
    block(c, 0x103765e4, 2);
    if (guest::nonzero(c, std::int32_t(p.conditions))) cap_proposal_multiplier_for_conditions(p);
    else cap_proposal_multiplier_for_age(p);
    cap_young_proposal_multiplier(p);
}

void scale_proposal_incumbent_floor(ContractProposal& p, std::int16_t offered_rating, std::int16_t existing_rating) {
    Cpu& c = p.c;
    const auto frame = c.sp();
    block(c, 0x103769a0, 8);
    const auto encoded = guest::extended(offered_rating) ^ 0x80000000;
    c.set(3, encoded);
    c.put32(frame + 0x9c, c.r(0));
    const bool use_existing = guest::compare(c, existing_rating, 100) > 0;
    c.set(0, 100);
    c.put32(frame + 0xa0, encoded);
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(2, p.fp.fsubs(c.f(0), c.f(1)));
    if (use_existing) {
        block(c, 0x103769c0, 1);
        c.set(0, guest::extended(existing_rating));
    }
    block(c, 0x103769c4, 11);
    const auto divisor = c.r(0) ^ 0x80000000;
    c.set({{3, divisor}, {0, double_header}});
    c.put32(frame + 0xa8, divisor);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0xa4, double_header);
    c.set_f(3, p.number(0x28));
    c.set_f(0, c.f64(frame + 0xa4));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(1)));
    c.set_f(0, p.fp.fdivs(c.f(2), c.f(0)));
    p.fp.fcmp(0, c.f(0), c.f(3));
    if ((c.cr(0) & 4) != 0) {
        block(c, 0x103769f0, 1);
        c.set_f(3, c.f(0));
    }
    block(c, 0x103769f4, 1);
    c.set_f(3, p.fp.frsp(c.f(3)));
}

void floor_proposal_wage_at_existing_contract(ContractProposal& p) {
    Cpu& c = p.c;
    const std::uint32_t existing = p.incumbent;
    block(c, 0x1037694c, 4);
    c.set_f(30, p.fp.fmuls(c.f(30), c.f(29)));
    const bool has_existing = guest::nonzero_logical(c, existing);
    c.set_f(29, c.f(30));
    if (!has_existing) return;
    block(c, 0x1037695c, 2);
    if (guest::compare_logical(c, existing, p.contract) == 0) return;

    block(c, 0x10376964, 10);
    const auto salary = c.u32(existing + 12);
    const auto frame = c.sp();
    c.set({{5, salary}, {0, double_header}});
    c.put32(frame + 0x9c, double_header);
    const auto encoded = salary ^ 0x80000000;
    c.set(3, encoded);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0xa0, encoded);
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(1)));
    p.fp.fcmp(0, c.f(0), c.f(30));
    if ((c.cr(0) & 4) == 0) return;

    block(c, 0x1037698c, 5);
    const auto rating = c.i16(p.destination + club_rating);
    c.set(3, guest::extended(rating));
    const auto previous_rating = c.i16(p.incumbent_club + club_rating);
    c.set(4, guest::extended(previous_rating));
    c.set_f(3, p.single(0x58));
    if (guest::compare(c, rating, previous_rating) < 0) scale_proposal_incumbent_floor(p, rating, previous_rating);

    block(c, 0x103769f8, 10);
    c.set({{3, encoded}, {0, double_header}});
    c.put32(frame + 0xa0, encoded);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0x9c, double_header);
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(1)));
    c.set_f(0, p.fp.fmuls(c.f(0), c.f(3)));
    p.fp.fcmp(0, c.f(0), c.f(30));
    if ((c.cr(0) & 4) != 0) block(c, 0x10376a20, 1);
    else {
        block(c, 0x10376a24, 1);
        c.set_f(0, c.f(30));
    }
    block(c, 0x10376a28, 1);
    c.set_f(30, c.f(0));
}

void floor_proposal_wage_at_club(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376a2c, 2);
    if (!detail::record_nonzero(c, p.conditions & 0x01000000)) return;
    block(c, 0x10376a34, 11);
    const auto frame = c.sp();
    const auto encoded = c.r(13) ^ 0x80000000;
    c.set({{0, encoded}, {4, double_header}});
    c.put32(frame + 0xa0, encoded);
    c.set_f(2, p.number(0xc0));
    c.put32(frame + 0x9c, double_header);
    c.set_f(1, p.number(0x10));
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(2)));
    c.set_f(0, p.fp.fmul(c.f(1), c.f(0)));
    p.fp.fcmp(0, c.f(30), c.f(0));
    if ((c.cr(0) & 8) == 0) return;

    block(c, 0x10376a60, 9);
    const auto service = c.u32(frame + 0xb8);
    c.set(3, service);
    c.put32(frame + 0x9c, double_header);
    const auto salary = c.u32(service + 0x10);
    c.set(0, salary);
    c.set_f(1, p.single(0x5c));
    const auto salary_word = salary ^ 0x80000000;
    c.set(0, salary_word);
    c.put32(frame + 0xa0, salary_word);
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(2)));
    c.set_f(30, p.fp.fmuls(c.f(1), c.f(0)));
}

bool cap_proposal_wage(ContractProposal& p, std::uint32_t pc, unsigned threshold, bool jump = true) {
    Cpu& c = p.c;
    block(c, pc, 3);
    c.set_f(0, p.single(threshold));
    p.fp.fcmp(0, c.f(30), c.f(0));
    if ((c.cr(0) & 4) == 0) return false;
    block(c, pc + 12, jump ? 2 : 1);
    c.set_f(30, p.single(0x64));
    return true;
}

void cap_proposal_wage_for_age(ContractProposal& p) {
    Cpu& c = p.c;
    if (cap_proposal_wage(p, 0x10376a84, 0x60)) return;

    const auto age = std::int32_t(p.age);
    block(c, 0x10376a98, 2);
    if (guest::compare(c, age, 34) > 0 && cap_proposal_wage(p, 0x10376aa0, 0x68)) return;
    block(c, 0x10376ab4, 2);
    if (guest::compare(c, age, 31) > 0 && cap_proposal_wage(p, 0x10376abc, 0x6c)) return;
    block(c, 0x10376ad0, 3);
    const auto prospect = c.u8(p.profile + 15);
    c.set(0, prospect);
    if (guest::compare(c, prospect, 20) == 0) cap_proposal_wage(p, 0x10376adc, 0x1c, false);
}

bool proposal_wage_above(ContractProposal& p, std::uint32_t pc, unsigned threshold) {
    block(p.c, pc, 3);
    p.c.set_f(0, p.single(threshold));
    p.fp.fcmp(0, p.c.f(30), p.c.f(0));
    return (p.c.cr(0) & 4) != 0;
}

bool proposal_wage_role(ContractProposal& p, std::uint32_t pc, int expected) {
    block(p.c, pc, 3);
    const auto role = std::int8_t(std::uint32_t(p.role));
    p.c.set(0, std::uint32_t(role));
    return guest::compare(p.c, role, expected) == 0;
}

void enforce_proposal_wage_ceiling(ContractProposal& p, std::uint32_t pc, bool jump = true) {
    block(p.c, pc, jump ? 2 : 1);
    p.c.set_f(30, p.single(0x64));
}

void cap_role_three_wage(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376b90, 6);
    c.set({{3, p.destination}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x10376ba8);
    block(c, 0x10376ba8, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(3)))) {
        enforce_proposal_wage_ceiling(p, 0x10376bb0);
        return;
    }

    block(c, 0x10376bb8, 3);
    const auto profile = c.u32(p.person + staff_profile);
    c.set(3, profile);
    if (guest::nonzero_logical(c, profile)) {
        block(c, 0x10376bc4, 3);
        const auto reputation = c.i16(profile + 10);
        c.set(4, guest::extended(reputation));
        if (guest::compare(c, reputation, 6250) > 0) {
            const auto maximum = [&](std::uint32_t pc, unsigned field) {
                block(c, pc, 3);
                const auto attribute = c.u8(profile + field);
                c.set(0, attribute);
                return guest::compare(c, attribute, 20) == 0;
            };
            const bool gifted = maximum(0x10376bd0, 21) || maximum(0x10376bdc, 20);
            const auto pc = gifted ? 0x10376be8u : 0x10376c18u;
            block(c, pc, 2);
            const bool elite = guest::compare(c, reputation, 7250) > 0;
            const auto threshold = gifted ? (elite ? 0x7c : 0x1c) : (elite ? 0x1c : 0x18);
            cap_proposal_wage(p, pc + (elite ? 8 : 28), threshold);
            return;
        }
    }
    enforce_proposal_wage_ceiling(p, 0x10376c48);
}

bool cap_proposal_wage_for_role(ContractProposal& p) {
    Cpu& c = p.c;
    struct RoleCeiling {
        std::uint32_t pc;
        int role;
        unsigned threshold;
    };
    constexpr RoleCeiling ceilings[] = {
        {0x10376aec, 7, 0x2c}, {0x10376b0c, 6, 0x70},
        {0x10376b2c, 5, 0x74}, {0x10376b4c, 4, 0x68},
    };
    for (const auto [pc, role, threshold] : ceilings) {
        if (proposal_wage_role(p, pc, role) && cap_proposal_wage(p, pc + 12, threshold)) return true;
    }
    if (proposal_wage_role(p, 0x10376b6c, 3)
        && proposal_wage_above(p, 0x10376b78, 0x78)
        && proposal_signed_short(p, 0x10376b84, p.profile, 12, 6750) < 0) {
        cap_role_three_wage(p);
        return true;
    }
    if (proposal_wage_role(p, 0x10376c50, 2)
        && proposal_wage_above(p, 0x10376c5c, 0x7c)
        && proposal_signed_short(p, 0x10376c68, p.profile, 12, 7250) < 0) {
        enforce_proposal_wage_ceiling(p, 0x10376c74);
        return true;
    }
    if (proposal_wage_above(p, 0x10376c7c, 0x64)) {
        block(c, 0x10376c88, 2);
        c.set_f(30, c.f(0));
        return true;
    }
    return false;
}

bool proposal_can_blend_wages(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376c90, 2);
    const std::uint32_t existing = p.incumbent;
    if (!guest::nonzero_logical(c, existing)) return false;

    block(c, 0x10376c98, 10);
    const auto salary = c.u32(existing + 12);
    const auto frame = c.sp();
    c.set({{3, salary}, {0, double_header}});
    c.put32(frame + 0x9c, double_header);
    const auto encoded = salary ^ 0x80000000;
    c.set(0, encoded);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0xa0, encoded);
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(1)));
    p.fp.fcmp(0, c.f(0), c.f(29));
    if ((c.cr(0) & 4) == 0) return false;
    block(c, 0x10376cc0, 2);
    if (!guest::nonzero(c, std::int32_t(salary))) return false;
    block(c, 0x10376cc8, 2);
    return guest::compare_logical(c, p.incumbent_club, p.destination) != 0;
}

void seed_proposal_incumbent_weight(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376cd0, 4);
    const auto conditions = c.u32(p.incumbent + ContractRecord::review_conditions);
    c.set(0, conditions);
    c.set_f(28, p.number(0x168));
    const auto add_bonus = [&](std::uint32_t pc) {
        block(c, pc, 2);
        c.set_f(0, p.number(0x170));
        c.set_f(28, p.fp.fadd(c.f(28), c.f(0)));
    };
    if (detail::record_nonzero(c, conditions & 0x100)) add_bonus(0x10376ce0);
    block(c, 0x10376ce8, 3);
    c.set({{3, p.person}, {4, p.incumbent_club}});
    c.call(0x100d3150, 0x10376cf4);
    block(c, 0x10376cf4, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(3)))) add_bonus(0x10376cfc);
}

void add_proposal_salary_weight(ContractProposal& p) {
    Cpu& c = p.c;
    const auto frame = c.sp();
    block(c, 0x10376d04, 5);
    c.set(3, 0x10000);
    const auto salary = c.u32(p.incumbent + 12);
    constexpr int baseline = 40000;
    c.set({{4, salary}, {0, baseline}});
    if (guest::compare(c, std::int32_t(salary), baseline) <= 0) return;

    block(c, 0x10376d18, 12);
    const auto premium = salary - baseline;
    c.set({{3, premium}, {0, double_header}});
    c.put32(frame + 0x9c, double_header);
    const auto encoded = premium ^ 0x80000000;
    c.set(0, encoded);
    c.set_f(2, p.number(0xc0));
    c.put32(frame + 0xa0, encoded);
    c.set_f(0, p.number(0x178));
    c.set_f(1, c.f64(frame + 0x9c));
    c.set_f(1, p.fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, p.fp.fdiv(c.f(1), c.f(0)));
    c.set_f(28, p.fp.fadd(c.f(28), c.f(0)));
}

void add_proposal_age_weight(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376d48, 2);
    const auto age = std::int32_t(p.age);
    if (guest::compare(c, age, 33) <= 0) return;
    block(c, 0x10376d50, 10);
    const auto years = std::uint32_t(38) - std::uint32_t(age);
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(age) <= 38);
    const auto encoded = years ^ 0x80000000;
    const auto frame = c.sp();
    c.set({{3, encoded}, {0, double_header}});
    c.put32(frame + 0xa4, double_header);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0xa8, encoded);
    c.set_f(2, p.number(0x140));
    c.set_f(0, c.f64(frame + 0xa4));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
    c.set_f(28, p.fp.multiply_add(0x10376d74, c.f(2), c.f(0), c.f(28)));
}

void add_proposal_contract_weight(ContractProposal& p) {
    Cpu& c = p.c;
    const auto frame = c.sp();
    const std::uint32_t existing = p.incumbent;
    const auto accumulate = [&](unsigned scratch) {
        c.set_f(1, c.f64(frame + scratch));
        c.set_f(1, p.fp.fsub(c.f(1), c.f(2)));
        c.set_f(0, p.fp.fdiv(c.f(1), c.f(0)));
        c.set_f(28, p.fp.fadd(c.f(28), c.f(0)));
    };
    block(c, 0x10376d78, 3);
    const auto raw = c.u8(existing + 0x44);
    c.set(3, raw);
    const auto loyalty = std::int8_t(raw);
    detail::record_nonzero(c, loyalty);
    if (loyalty < 0) {
        block(c, 0x10376d84, 11);
        const auto encoded = std::uint32_t(loyalty) ^ 0x80000000;
        c.set({{3, encoded}, {0, double_header}});
        c.put32(frame + 0xac, double_header);
        c.set_f(2, p.number(0xc0));
        c.put32(frame + 0xb0, encoded);
        c.set_f(0, p.number(0x180));
        accumulate(0xac);
    }

    block(c, 0x10376db0, 3);
    const auto conditions = c.u32(existing + ContractRecord::review_conditions);
    c.set(0, conditions);
    if (!guest::nonzero(c, std::int32_t(conditions))) return;
    block(c, 0x10376dbc, 12);
    const auto pressure = c.u8(existing + 0x4c);
    c.set({{3, pressure}, {0, double_header}});
    c.put32(frame + 0x94, double_header);
    const auto signed_pressure = std::uint32_t(std::int8_t(pressure));
    c.set(0, signed_pressure);
    c.set_f(2, p.number(0xc0));
    const auto encoded = signed_pressure ^ 0x80000000;
    c.set(0, encoded);
    c.set_f(0, p.number(0x180));
    c.put32(frame + 0x98, encoded);
    accumulate(0x94);
}

void clamp_proposal_incumbent_weight(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376dec, 3);
    c.set_f(30, p.number(0x168));
    p.fp.fcmp(0, c.f(30), c.f(28));
    if ((c.cr(0) & 4) != 0) {
        block(c, 0x10376df8, 1);
        return;
    }
    block(c, 0x10376dfc, 3);
    c.set_f(30, p.number(0x188));
    p.fp.fcmp(0, c.f(30), c.f(28));
    if ((c.cr(0) & 8) != 0) block(c, 0x10376e08, 1);
    else {
        block(c, 0x10376e0c, 1);
        c.set_f(30, c.f(28));
    }
}

void lookup_proposal_role_wage(ContractProposal& p, std::uint32_t pc) {
    Cpu& c = p.c;
    block(c, pc, 4);
    const auto slot = c.toc(-0x7ab8);
    c.set({{3, slot}, {4, p.destination}});
    c.set(3, c.u32(slot));
    c.call(0x10103510, pc + 16);
    block(c, pc + 16, 3);
    c.set({{4, p.person}, {5, p.role}});
    c.call(0x100f2680, pc + 28);
}

void choose_proposal_wage_for_blend(ContractProposal& p) {
    Cpu& c = p.c;
    lookup_proposal_role_wage(p, 0x10376e10);
    block(c, 0x10376e2c, 11);
    const auto frame = c.sp();
    const auto encoded = c.r(3) ^ 0x80000000;
    c.set({{3, encoded}, {0, double_header}});
    c.put32(frame + 0x90, encoded);
    c.set_f(0, p.number(0x28));
    c.put32(frame + 0x8c, double_header);
    c.set_f(1, p.number(0xc0));
    c.set_f(28, p.fp.fmul(c.f(29), c.f(0)));
    c.set_f(0, c.f64(frame + 0x8c));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
    p.fp.fcmp(0, c.f(28), c.f(0));
    if ((c.cr(0) & 4) != 0) {
        block(c, 0x10376e58, 2);
        c.set_f(0, c.f(28));
    } else {
        lookup_proposal_role_wage(p, 0x10376e60);
        block(c, 0x10376e7c, 7);
        signed_proposal_scratch(p, 3, 0x84, false);
    }
    block(c, 0x10376e98, 2);
    p.fp.fcmp(0, c.f(29), c.f(0));
    if ((c.cr(0) & 8) != 0) {
        block(c, 0x10376ea0, 2);
        c.set_f(28, c.f(29));
        return;
    }

    lookup_proposal_role_wage(p, 0x10376ea8);
    block(c, 0x10376ec4, 9);
    signed_proposal_scratch(p, 3, 0x84, false);
    p.fp.fcmp(0, c.f(28), c.f(0));
    if ((c.cr(0) & 4) != 0) {
        block(c, 0x10376ee8, 1);
        return;
    }
    lookup_proposal_role_wage(p, 0x10376eec);
    block(c, 0x10376f08, 7);
    const auto word = c.r(3) ^ 0x80000000;
    c.set({{3, word}, {0, double_header}});
    c.put32(frame + 0x90, word);
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0x8c, double_header);
    c.set_f(0, c.f64(frame + 0x8c));
    c.set_f(28, p.fp.fsub(c.f(0), c.f(1)));
}

void blend_proposal_incumbent_wage(ContractProposal& p) {
    if (!proposal_can_blend_wages(p)) return;
    seed_proposal_incumbent_weight(p);
    add_proposal_salary_weight(p);
    add_proposal_age_weight(p);
    add_proposal_contract_weight(p);
    clamp_proposal_incumbent_weight(p);
    choose_proposal_wage_for_blend(p);

    Cpu& c = p.c;
    block(c, 0x10376f24, 16);
    const auto salary = c.u32(p.incumbent + 12);
    const auto frame = c.sp();
    c.set({{3, salary}, {0, double_header}});
    c.set_f(3, p.number(0x188));
    c.set_f(4, p.fp.frsp(c.f(28)));
    const auto encoded = salary ^ 0x80000000;
    c.set(3, encoded);
    c.put32(frame + 0x84, double_header);
    c.set_f(1, p.number(0xc0));
    c.set_f(2, p.fp.fsub(c.f(3), c.f(30)));
    c.put32(frame + 0x88, encoded);
    c.set_f(0, c.f64(frame + 0x84));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, p.fp.fmul(c.f(2), c.f(0)));
    c.set_f(0, p.fp.multiply_add(0x10376f54, c.f(4), c.f(30), c.f(0)));
    c.set_f(30, p.fp.fdiv(c.f(0), c.f(3)));
    c.set_f(30, p.fp.frsp(c.f(30)));
}

void quote_proposal_for_staff(ContractProposal& p) {
    Cpu& c = p.c;
    block(c, 0x10376f64, 8);
    c.set({{3, p.contract}, {4, p.person}, {5, p.destination}, {7, p.role}, {8, p.policy}, {6, 0}, {9, 0}});
    c.call(reference_quote_entry, 0x10376f84);

    const auto frame = c.sp();
    block(c, 0x10376f84, 10);
    const auto slot = c.toc(-0x7b40);
    c.set(3, slot);
    c.set_f(28, c.f(1));
    const auto purpose = c.u32(frame + 0x160);
    c.set({{8, purpose}, {4, p.person}});
    const auto manager = c.u32(slot);
    c.set({{3, manager}, {5, p.destination}, {7, p.policy}});
    c.set(3, c.u32(manager + 0x988));
    c.set(6, 0);
    c.call(0x10366470, 0x10376fac);

    block(c, 0x10376fac, 4);
    const auto output = c.u32(frame + 0x164);
    c.set(0, output);
    c.set_f(3, p.fp.frsp(c.f(1)));
    if (guest::nonzero_logical(c, output)) {
        block(c, 0x10376fbc, 2);
        c.set(3, output);
        c.put_f32(output, float(c.f(3)));
    }
    block(c, 0x10376fc4, 4);
    c.set_f(0, p.number(0x88));
    c.set_f(2, p.number(0x110));
    p.fp.fcmp(0, c.f(0), c.f(3));
    if ((c.cr(0) & 4) != 0) block(c, 0x10376fd4, 1);
    else {
        block(c, 0x10376fd8, 1);
        c.set_f(0, c.f(3));
    }
    block(c, 0x10376fdc, 6);
    c.set_f(1, p.fp.frsp(c.f(0)));
    c.set_f(0, p.number(0x120));
    p.fp.fcmp(0, c.f(3), c.f(0));
    c.set_f(1, p.fp.fdiv(c.f(2), c.f(1)));
    c.set_f(1, p.fp.frsp(c.f(1)));
    if ((c.cr(0) & 8) != 0) {
        block(c, 0x10376ff4, 2);
        c.set_f(0, p.single(0x80));
        c.set_f(28, p.fp.fadds(c.f(28), c.f(0)));
    }
    block(c, 0x10376ffc, 1);
    c.set_f(0, p.number(0xd0));
    block(c, 0x10377000, 2);
    p.fp.fcmp(0, c.f(1), c.f(0));
    if ((c.cr(0) & 8) != 0) {
        block(c, 0x10377008, 1);
        c.set_f(1, p.single(0x30));
    }
}

bool staff_proposal_wage_exceeds_limit(ContractProposal& p, std::uint32_t pc, int scale, int increment,
                                      unsigned absolute_cap, bool affine) {
    Cpu& c = p.c;
    const auto frame = c.sp();
    const bool has_scale = !affine && scale != 1;
    block(c, pc, affine ? 13 : has_scale ? 12 : 11);
    const auto rating = c.i16(p.destination + club_rating);
    c.set({{3, guest::extended(rating)}, {0, double_header}});
    c.put32(frame + 0x84, double_header);

    if (affine) {
        const auto encoded = guest::extended(rating) ^ 0x80000000;
        c.set(0, encoded);
        c.set_f(2, p.number(0xc0));
        c.put32(frame + 0x88, encoded);
        c.set_f(3, p.number(0x30));
        c.set_f(1, c.f64(frame + 0x84));
        c.set_f(0, p.number(0x68));
        c.set_f(1, p.fp.fsub(c.f(1), c.f(2)));
        c.set_f(0, p.fp.multiply_add(pc + 40, c.f(3), c.f(1), c.f(0)));
    } else {
        if (has_scale) c.set(3, std::uint32_t(rating * scale));
        c.set(0, std::uint32_t(rating * scale + increment));
        c.set_f(1, p.number(0xc0));
        const auto encoded = c.r(0) ^ 0x80000000;
        c.set(0, encoded);
        c.put32(frame + 0x88, encoded);
        c.set_f(0, c.f64(frame + 0x84));
        c.set_f(0, p.fp.fsubs(c.f(0), c.f(1)));
    }
    p.fp.fcmp(0, c.f(30), c.f(0));
    return (c.cr(0) & 4) != 0
        && proposal_wage_above(p, pc + (affine ? 52 : has_scale ? 48 : 44), absolute_cap);
}

void cap_proposal_staff_wage(ContractProposal& p) {
    Cpu& c = p.c;
    const auto category = std::int8_t(std::uint32_t(p.policy));
    block(c, 0x1037700c, 4);
    c.set(0, std::uint32_t(category));
    c.set_f(30, p.fp.fmuls(c.f(1), c.f(28)));
    if (guest::compare(c, category, 6) == 0
        && staff_proposal_wage_exceeds_limit(p, 0x1037701c, 2, 1200, 0x84, false)) {
        enforce_proposal_wage_ceiling(p, 0x10377058);
        return;
    }
    const auto is_category = [&](std::uint32_t pc, int expected) {
        block(c, pc, 3);
        c.set(0, std::uint32_t(category));
        return guest::compare(c, category, expected) == 0;
    };
    if (is_category(0x10377060, 8)
        && staff_proposal_wage_exceeds_limit(p, 0x1037706c, 1, 500, 0x88, false)) {
        enforce_proposal_wage_ceiling(p, 0x103770a4);
        return;
    }
    if (is_category(0x103770ac, 9)
        && staff_proposal_wage_exceeds_limit(p, 0x103770b8, 1, 0, 0x8c, true)) {
        enforce_proposal_wage_ceiling(p, 0x103770f8);
        return;
    }
    if (is_category(0x10377100, 10)
        && staff_proposal_wage_exceeds_limit(p, 0x1037710c, 1, 0, 0x90, true)) {
        enforce_proposal_wage_ceiling(p, 0x1037714c);
        return;
    }
    if (proposal_wage_above(p, 0x10377154, 0x64)) {
        block(c, 0x10377160, 1);
        c.set_f(30, c.f(0));
    }
}

void set_proposal_release_amount(ContractProposal& p, std::uint32_t resume) {
    auto& c = p.c;
    detail::call_owned(c, 0x10380700, resume, set_release_amount, std::uint32_t(p.contract), c.r(4));
}

void set_proposal_alternate_option(ContractProposal& p, std::uint32_t pc, bool jump) {
    auto& c = p.c;
    block(c, pc, jump ? 5 : 4);
    c.set({{3, 0}, {0, 1}});
    c.put8(p.contract + 0x1d, 0);
    c.put8(p.contract + 0x20, 1);
}

void lookup_release_club(ContractProposal& p, std::uint32_t pc) {
    auto& c = p.c;
    block(c, pc, 4);
    c.set(3, c.u32(p.manager_slot));
    c.set({{4, p.person}, {13, 0}});
    c.call(0x10082f20, pc + 16);

    block(c, pc + 16, 3);
    const auto record = c.r(3);
    const auto has_record = guest::nonzero_logical(c, record);
    c.put32(p.primary_slot, record);
    if (has_record) {
        block(c, pc + 28, 6);
        const auto packed = c.u8(record + ContractRecord::packed_kind);
        const auto kind = detail::unpack_kind(packed);
        c.s.wr<1>(guest::reg::xer_ca, (packed & 0x82u) == 0x82u);
        c.set(0, std::uint32_t(kind));
        if (guest::compare(c, kind, 5) != 0) {
            block(c, pc + 52, 1);
            c.set(13, 1);
        }
    }

    block(c, pc + 56, 2);
    if (guest::nonzero(c, std::int32_t(c.r(13)))) {
        block(c, pc + 64, 7);
        const auto active = c.u32(p.primary_slot);
        c.set(4, active);
        const auto clubs_slot = c.toc(-0x7be8);
        c.set(3, clubs_slot);
        const auto id = c.u32(active + ContractRecord::club_id);
        c.set(0, id);
        const auto clubs = c.u32(clubs_slot);
        c.set({{3, clubs}, {0, clubs + id * 0x248u}});
    } else {
        block(c, pc + 92, 1);
        c.set(0, 0);
    }
}

void remove_proposal_release(ContractProposal& p, std::uint32_t pc, unsigned field, unsigned wage_factor) {
    auto& c = p.c;
    block(c, pc, 2);
    if (guest::nonzero_logical(c, p.incumbent)) {
        block(c, pc + 8, 3);
        if (detail::record_nonzero(c, c.i8(p.incumbent + field))) {
            lookup_release_club(p, pc + 20);
            block(c, pc + 116, 2);
            if (guest::compare_logical(c, c.r(0), p.destination) == 0) {
                block(c, pc + 124, 3);
                c.set_f(0, p.number(wage_factor));
                c.set_f(30, p.fp.fmul(c.f(0), c.f(30)));
                c.set_f(30, p.fp.frsp(c.f(30)));
            }
        }
    }
    block(c, pc + 136, 2);
    c.set(0, 0);
    c.put8(p.contract + field, 0);
}

bool release_competition_approves(ContractProposal& p, std::uint32_t pc, unsigned callback) {
    auto& c = p.c;
    block(c, pc, 2);
    if (!guest::nonzero_logical(c, p.destination)) return false;
    block(c, pc + 8, 3);
    const auto competition = c.u32(p.destination + 0x30);
    c.set(3, competition);
    if (!guest::nonzero_logical(c, competition)) return false;
    block(c, pc + 20, 7);
    const auto index = c.u32(competition);
    c.set(0, index);
    const auto slot = c.toc(-0x7b5c);
    c.set({{3, slot}, {0, index * 4u}});
    const auto table = c.u32(slot);
    c.set(3, table);
    const auto entry = c.u32(table + index * 4u);
    c.set(3, entry);
    if (!guest::nonzero_logical(c, entry)) return false;
    block(c, pc + 48, 3);
    const auto kind = c.u8(entry + 0x43);
    c.set(0, kind);
    if (guest::compare(c, kind, 2) != 0) return false;
    block(c, pc + 60, 3);
    const auto vtable = c.u32(entry);
    c.set(12, vtable);
    c.set(12, c.u32(vtable + callback));
    c.call(0x10781320, pc + 72);
    block(c, pc + 72, 3);
    c.set(2, c.u32(c.sp() + 0x14));
    return detail::record_nonzero(c, std::int8_t(c.r(3)));
}

void appraise_proposal_release(ContractProposal& p, std::uint32_t pc) {
    auto& c = p.c;
    block(c, pc, 7);
    c.set(3, c.u32(p.manager_slot));
    c.set({{4, p.person}, {5, p.destination}, {6, 1}, {7, UINT32_MAX}, {8, 0}});
    c.call(0x10084be0, pc + 28);
}

void release_value_fraction(ContractProposal& p, unsigned bias_reg, unsigned coefficient) {
    auto& c = p.c;
    const auto frame = c.sp();
    c.put32(frame + 0x84, c.r(5));
    c.set_f(0, p.single(coefficient));
    c.set_f(1, c.f64(frame + 0x84));
    c.set_f(1, p.fp.fsubs(c.f(1), c.f(bias_reg)));
    c.set_f(1, p.fp.fmuls(c.f(31), c.f(1)));
    c.set_f(0, p.fp.fmuls(c.f(1), c.f(0)));
    c.put64(frame + 0x8c, p.fp.fctiwz(0, c.f(0)));
}

bool propose_release_value_with_conditions(ContractProposal& p, std::uint32_t pc, bool first_release) {
    auto& c = p.c;
    appraise_proposal_release(p, pc);
    block(c, pc + 28, 16);
    c.set({{0, c.r(3) ^ 0x80000000u}, {5, double_header}});
    c.put32(c.sp() + 0x88, c.r(0));
    const auto add_premium = first_release ? guest::nonzero_logical(c, p.incumbent)
                                          : !guest::nonzero(c, std::int32_t(p.conditions));
    c.set_f(4, p.number(0xc0));
    release_value_fraction(p, 4, first_release ? 0 : 0x98);
    c.set(0, c.u32(c.sp() + 0x90));
    c.set(4, c.r(3) + c.r(0));
    if (!first_release || !add_premium) return add_premium;
    block(c, pc + 92, 3);
    return !detail::record_nonzero(c, c.u32(p.incumbent + ContractRecord::review_conditions) & 1u);
}

void add_proposal_release_premium(ContractProposal& p, std::uint32_t pc) {
    auto& c = p.c;
    const auto frame = c.sp();
    block(c, pc, 16);
    c.set({{0, c.r(4) ^ 0x80000000u}, {3, c.r(4) * 2u}});
    c.put32(frame + 0x88, c.r(0));
    c.set(0, c.r(3) ^ 0x80000000u);
    c.set_f(3, p.number(0x10));
    c.put32(frame + 0x84, c.r(5));
    c.set_f(1, p.number(0x8));
    c.set_f(0, c.f64(frame + 0x84));
    c.put32(frame + 0x90, c.r(0));
    c.set_f(2, p.fp.fsub(c.f(0), c.f(4)));
    c.put32(frame + 0x8c, c.r(5));
    c.set_f(0, c.f64(frame + 0x8c));
    c.set_f(1, p.fp.multiply_add(pc + 48, c.f(3), c.f(2), c.f(1)));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(4)));
    p.fp.fcmp(0, c.f(1), c.f(0));
    if ((c.cr(0) & 4) != 0) {
        block(c, pc + 64, 1);
    } else {
        block(c, pc + 68, 4);
        c.put32(frame + 0x98, c.r(0));
        c.put32(frame + 0x94, c.r(5));
        c.set_f(0, c.f64(frame + 0x94));
        c.set_f(1, p.fp.fsub(c.f(0), c.f(4)));
    }
    block(c, pc + 84, 7);
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(1));
    c.set({{3, 0x260000}, {0, 2500000}});
    c.put64(frame + 0x9c, c.s.r<8>(guest::reg::fpr(0)));
    c.set(3, c.u32(frame + 0xa0));
    if (guest::compare(c, std::int32_t(c.r(3)), 2500000) > 0) {
        block(c, pc + 112, 14);
        c.set({{3, c.r(3) ^ 0x80000000u}, {0, double_header}});
        c.put32(frame + 0xa8, c.r(3));
        c.set_f(1, p.number(0xc0));
        c.put32(frame + 0xa4, c.r(0));
        c.set_f(3, p.number(0x190));
        c.set_f(0, c.f64(frame + 0xa4));
        c.set_f(2, p.number(0x20));
        c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
        c.set_f(0, p.fp.multiply_add(pc + 148, c.f(2), c.f(0), c.f(3)));
        c.set_f(0, p.fp.fadd(c.f(3), c.f(0)));
        c.put64(frame + 0xac, p.fp.fctiwz(0, c.f(0)));
        c.set(3, c.u32(frame + 0xb0));
    }
    block(c, pc + 168, 1);
    c.set(4, c.r(4) + c.r(3));
}

void apply_release_floor(ContractProposal& p, std::uint32_t pc, std::uint32_t minimum) {
    auto& c = p.c;
    block(c, pc, 4);
    c.set({{3, (minimum + 0x8000u) & 0xffff0000u}, {0, minimum}});
    if (guest::compare(c, std::int32_t(c.r(4)), minimum) < 0) {
        block(c, pc + 16, 1);
        c.set(4, minimum);
    }
}

void write_proposal_release_clause(ContractProposal& p, std::uint32_t pc, unsigned field) {
    auto& c = p.c;
    block(c, pc, 6);
    c.set({{3, 0}, {0, 1}});
    c.put8(p.contract + 0x1d, 0);
    c.set(3, p.contract);
    c.put8(p.contract + field, 1);
    set_proposal_release_amount(p, pc + 24);
    block(c, pc + 24, 1);
}

void write_exclusive_proposal_release(ContractProposal& p, std::uint32_t pc) {
    auto& c = p.c;
    block(c, pc, 12);
    c.set({{5, 0}, {0, 1}});
    c.put8(p.contract + 0x1c, 0);
    c.set(3, p.contract);
    for (auto field = 0x1du; field <= 0x20; ++field) c.put8(p.contract + field, 0);
    c.put8(p.contract + 0x1d, 1);
    c.put8(p.contract + 0x1c, 0);
    c.put8(p.contract + 0x1f, 0);
    set_proposal_release_amount(p, pc + 48);
    block(c, pc + 48, 1);
}

int clause_reputation_base(Cpu& c, int reputation) {
    const auto high = int((INT64_C(0x51eb851f) * reputation) >> 32);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 15));
    return high >> 4;
}

bool first_proposal_release_score(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x1037724c, 3);
    const auto reputation = std::int16_t(p.reputation);
    c.set(5, guest::extended(reputation));
    if (guest::compare(c, reputation, 2750) <= 0) return false;
    block(c, 0x10377258, 42);
    const auto frame = c.sp();
    c.set({{3, 0x51ec0000}, {0, double_header}, {4, 0x51eb851f}});
    const auto experience = c.i8(p.person + 0x65) * 2;
    const auto professionalism = c.i8(p.person + 0x63) * 2;
    const auto rating = clause_reputation_base(c, reputation);
    c.put32(frame + 0x8c, double_header);
    c.set_f(3, p.number(0xc0));
    const auto threshold = std::int8_t(p.wage_band) * 13;
    c.set({{4, std::uint32_t(experience) ^ 0x80000000u}, {5, std::uint32_t(rating) ^ 0x80000000u},
           {6, std::uint32_t(professionalism)}});
    c.put32(frame + 0x90, c.r(5));
    c.set(5, std::uint32_t(professionalism) ^ 0x80000000u);
    c.set_f(2, p.single(0x94));
    c.set_f(0, c.f64(frame + 0x8c));
    c.put32(frame + 0x98, c.r(4));
    c.set_f(1, p.fp.fsubs(c.f(0), c.f(3)));
    c.set(3, std::uint32_t(threshold) ^ 0x80000000u);
    c.put32(frame + 0x94, double_header);
    c.set_f(0, c.f64(frame + 0x94));
    c.set_f(1, p.fp.multiply_add(0x103772c8, c.f(2), c.f(31), c.f(1)));
    c.put32(frame + 0x88, c.r(5));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(3)));
    c.put32(frame + 0x84, double_header);
    c.set_f(2, c.f64(frame + 0x84));
    c.set_f(1, p.fp.fsubs(c.f(1), c.f(0)));
    c.put32(frame + 0xa0, c.r(3));
    c.set_f(2, p.fp.fsubs(c.f(2), c.f(3)));
    c.put32(frame + 0x9c, double_header);
    c.set_f(0, c.f64(frame + 0x9c));
    c.set_f(1, p.fp.fadds(c.f(2), c.f(1)));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(3)));
    p.fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

bool second_proposal_release_score(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10377528, 3);
    const auto reputation = std::int16_t(p.reputation);
    c.set(6, guest::extended(reputation));
    if (guest::compare(c, reputation, 2250) <= 0) return false;
    block(c, 0x10377534, 38);
    const auto frame = c.sp();
    c.set({{3, 0x51ec0000}, {4, double_header}, {0, 0x51eb851f}});
    const auto experience = c.i8(p.person + 0x65) * 3;
    const auto professionalism = c.i8(p.person + 0x63) * 3;
    c.put32(frame + 0x84, double_header);
    c.set_f(3, p.number(0xc0));
    const auto rating = clause_reputation_base(c, reputation);
    const std::uint32_t age = p.age;
    const auto threshold = std::int8_t(p.wage_band) * 10;
    c.set({{7, 0}, {8, std::uint32_t(experience)}, {3, age ^ 0x80000000u}});
    c.put32(frame + 0x90, c.r(3));
    c.set(6, std::uint32_t(rating));
    c.set_f(4, p.single(0x94));
    c.put32(frame + 0x8c, double_header);
    c.set(5, std::uint32_t(rating - experience));
    c.set_f(0, c.f64(frame + 0x8c));
    c.put32(frame + 0x94, double_header);
    c.set_f(1, p.fp.fsubs(c.f(0), c.f(3)));
    c.set(3, std::uint32_t(rating - experience + professionalism) ^ 0x80000000u);
    c.put32(frame + 0x88, c.r(3));
    c.set_f(0, c.f64(frame + 0x84));
    c.set(0, std::uint32_t(threshold) ^ 0x80000000u);
    c.set_f(2, p.fp.fsubs(c.f(0), c.f(3)));
    c.put32(frame + 0x98, c.r(0));
    c.set_f(0, c.f64(frame + 0x94));
    c.set_f(2, p.fp.multiply_add(0x103775b8, c.f(4), c.f(31), c.f(2)));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(3)));
    c.set_f(1, p.fp.fsubs(c.f(2), c.f(1)));
    p.fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

void scale_proposal_release(ContractProposal& p, std::uint32_t pc, unsigned factor, bool jump = true) {
    auto& c = p.c;
    const auto frame = c.sp();
    block(c, pc, jump ? 13 : 12);
    c.set({{3, c.r(4) ^ 0x80000000u}, {0, double_header}});
    c.put32(frame + 0x88, c.r(3));
    c.set_f(1, p.number(0xc0));
    c.put32(frame + 0x84, double_header);
    c.set_f(2, p.number(factor));
    c.set_f(0, c.f64(frame + 0x84));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, p.fp.fmul(c.f(2), c.f(0)));
    c.put64(frame + 0x8c, p.fp.fctiwz(0, c.f(0)));
    c.set(4, c.u32(frame + 0x90));
}

void set_second_proposal_release(ContractProposal& p) {
    auto& c = p.c;
    appraise_proposal_release(p, 0x10377620);
    block(c, 0x1037763c, 18);
    c.set({{0, c.r(3) ^ 0x80000000u}, {5, double_header}});
    c.put32(c.sp() + 0x88, c.r(0));
    c.set(4, 0x10000);
    c.set_f(2, p.number(0xc0));
    c.set(0, 50000);
    release_value_fraction(p, 2, 0x98);
    c.set(4, c.r(3) - c.u32(c.sp() + 0x90));
    if (guest::compare(c, std::int32_t(c.r(4)), 50000) < 0) {
        block(c, 0x10377684, 1);
        c.set(4, 50000);
    }
    const auto professionalism_above_ten = [&](std::uint32_t pc) {
        block(c, pc, 4);
        const auto professionalism = c.i8(p.person + 0x63);
        c.set(0, guest::extended(professionalism));
        return guest::compare(c, professionalism, 10) > 0;
    };
    block(c, 0x10377688, 4);
    const auto experience = c.u8(p.person + 0x65);
    c.set({{3, experience}, {0, guest::extended(std::int8_t(experience))}});
    if (guest::compare(c, std::int8_t(experience), 5) < 0 && professionalism_above_ten(0x10377698)) {
        scale_proposal_release(p, 0x103776a8, 0x28);
    } else if (professionalism_above_ten(0x103776dc)) {
        scale_proposal_release(p, 0x103776ec, 0x30);
    } else {
        block(c, 0x10377720, 3);
        c.set(0, guest::extended(std::int8_t(experience)));
        if (guest::compare(c, std::int8_t(experience), 15) < 0) {
            scale_proposal_release(p, 0x1037772c, 0x38, false);
        }
    }
    write_proposal_release_clause(p, 0x1037775c, 0x1f);
}

bool offers_release_for_unattached_player(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x103778b4, 3);
    const auto reputation = std::int16_t(p.reputation);
    c.set(0, guest::extended(reputation));
    if (guest::compare(c, reputation, 1250) <= 0) return false;
    lookup_release_club(p, 0x103778c0);
    block(c, 0x10377920, 2);
    if (guest::nonzero_logical(c, c.r(0))) {
        block(c, 0x10377928, 3);
        const auto band = std::int8_t(p.wage_band);
        c.set(0, guest::extended(band));
        if (guest::compare(c, band, 18) >= 0) return false;
    }
    block(c, 0x10377934, 34);
    const auto frame = c.sp();
    c.set({{4, 0x51ec0000}, {3, double_header}, {5, guest::extended(reputation)}});
    const auto experience = c.i8(p.person + 0x65) * 3;
    c.set(4, 0x51eb851f);
    const auto professionalism = c.i8(p.person + 0x63) * 4;
    c.set({{7, std::uint32_t(experience)}, {4, std::uint32_t(professionalism)}});
    c.put32(frame + 0x84, double_header);
    c.set_f(2, p.number(0xc0));
    const auto threshold = std::int8_t(p.wage_band) * 10;
    const auto rating = clause_reputation_base(c, reputation);
    c.set({{5, std::uint32_t(rating)}, {6, 0}});
    c.put32(frame + 0x8c, double_header);
    c.set_f(3, p.single(0x9c));
    const auto score = rating - experience + professionalism - std::int32_t(p.age);
    c.set(3, std::uint32_t(score) ^ 0x80000000u);
    c.put32(frame + 0x88, c.r(3));
    c.set_f(0, c.f64(frame + 0x84));
    c.set(0, std::uint32_t(threshold) ^ 0x80000000u);
    c.put32(frame + 0x90, c.r(0));
    c.set_f(1, p.fp.fsubs(c.f(0), c.f(2)));
    c.set_f(0, c.f64(frame + 0x8c));
    c.set_f(1, p.fp.multiply_add(0x103779ac, c.f(3), c.f(31), c.f(1)));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(2)));
    p.fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

bool offers_release_for_young_player(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10377b0c, 2);
    if (guest::compare(c, std::int32_t(p.age), 24) >= 0) return false;
    block(c, 0x10377b14, 30);
    const auto frame = c.sp();
    const std::uint32_t record = p.profile;
    const auto reserve_starts = c.i16(record + 6);
    c.set({{5, guest::extended(reserve_starts)}, {3, double_header}});
    const auto first_team_starts = c.i16(record + 4);
    c.set(4, guest::extended(first_team_starts));
    const auto threshold = std::int8_t(p.wage_band) * 15;
    c.set(0, std::uint32_t(threshold));
    const auto experience = c.u8(p.person + 0x65);
    c.set(6, experience);
    const auto professionalism = c.u8(p.person + 0x63);
    c.set(7, professionalism);
    c.put32(frame + 0x84, double_header);
    c.set(3, std::uint32_t(first_team_starts + reserve_starts) ^ 0x80000000u);
    c.put32(frame + 0x88, c.r(3));
    c.set_f(1, p.number(0xc0));
    c.set_f(0, c.f64(frame + 0x84));
    c.set_f(2, p.number(0x28));
    c.set_f(0, p.fp.fsub(c.f(0), c.f(1)));
    const auto experience_cost = std::int8_t(experience) * 3;
    const auto professionalism_bonus = std::int8_t(professionalism) * 2;
    c.set({{4, std::uint32_t(experience_cost)}, {5, std::uint32_t(professionalism_bonus)}});
    c.set_f(0, p.fp.fmul(c.f(2), c.f(0)));
    c.put64(frame + 0x8c, p.fp.fctiwz(0, c.f(0)));
    const auto starts_score = std::int16_t(c.u32(frame + 0x90));
    const auto score = starts_score - experience_cost + professionalism_bonus;
    c.set(3, std::uint32_t(score));
    return guest::compare(c, score, threshold) > 0;
}

void propose_exclusive_release(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10377808, 2);
    if (guest::compare_logical(c, p.destination, p.incumbent_club) == 0) {
        block(c, 0x10377810, 2);
        if (detail::record_nonzero(c, p.conditions & 1)) {
            appraise_proposal_release(p, 0x10377818);
            block(c, 0x10377834, 18);
            c.set({{0, c.r(3) ^ 0x80000000u}, {5, double_header}});
            c.put32(c.sp() + 0x88, c.r(0));
            c.set(4, 0x20000);
            c.set_f(2, p.number(0xc0));
            c.set(0, 150000);
            release_value_fraction(p, 2, 0);
            c.set(4, c.r(3) + c.u32(c.sp() + 0x90));
            if (guest::compare(c, std::int32_t(c.r(4)), 150000) < 0) {
                block(c, 0x1037787c, 1);
                c.set(4, 150000);
            }
            write_exclusive_proposal_release(p, 0x10377880);
            return;
        }
    }

    std::uint32_t value_site;
    if (offers_release_for_unattached_player(p)) {
        value_site = 0x103779bc;
    } else if (offers_release_for_young_player(p)) {
        value_site = 0x10377b8c;
    } else {
        remove_proposal_release(p, 0x10377cdc, 0x1d, 0x150);
        return;
    }
    if (propose_release_value_with_conditions(p, value_site, false)) {
        add_proposal_release_premium(p, value_site + 92);
    }
    apply_release_floor(p, value_site + 264, 150000);
    write_exclusive_proposal_release(p, value_site + 284);
}

bool premium_proposal_release_score(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10377d6c, 3);
    const auto reputation = std::int16_t(p.reputation);
    c.set(6, guest::extended(reputation));
    if (guest::compare(c, reputation, 6500) <= 0) return false;
    block(c, 0x10377d78, 28);
    const auto frame = c.sp();
    c.set({{4, 0x51ec0000}, {3, double_header}, {0, 0x51eb851f}});
    const auto experience = c.i8(p.person + 0x65) * 4;
    c.set(5, std::uint32_t(experience));
    c.put32(frame + 0x84, double_header);
    c.set_f(2, p.number(0xc0));
    const auto rating = clause_reputation_base(c, reputation);
    c.set(4, std::uint32_t(rating));
    c.put32(frame + 0x8c, double_header);
    c.set_f(3, p.single(0x94));
    c.set(3, std::uint32_t(rating - experience) ^ 0x80000000u);
    c.put32(frame + 0x88, c.r(3));
    c.set_f(0, c.f64(frame + 0x84));
    const auto threshold = std::int8_t(p.wage_band) * 14;
    c.set(0, std::uint32_t(threshold) ^ 0x80000000u);
    c.put32(frame + 0x90, c.r(0));
    c.set_f(1, p.fp.fsubs(c.f(0), c.f(2)));
    c.set_f(0, c.f64(frame + 0x8c));
    c.set_f(1, p.fp.multiply_add(0x10377dd8, c.f(3), c.f(31), c.f(1)));
    c.set_f(0, p.fp.fsubs(c.f(0), c.f(2)));
    p.fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

void propose_release_clauses(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10377164, 3);
    const auto role = c.u8(p.person + 0x62);
    c.set(0, role);
    bool player = guest::compare(c, role, 11) == 0;
    if (player) {
        block(c, 0x10377170, 3);
        const auto profile = c.u32(p.person + staff_profile);
        c.set(0, profile);
        player = guest::nonzero_logical(c, profile);
    }
    if (!player) {
        block(c, 0x10377f2c, 6);
        c.set(0, 0);
        for (auto field = 0x1cu; field <= 0x20; ++field) c.put8(p.contract + field, 0);
        return;
    }
    if (detail::qualifies_for_veteran_option(c, p.person, 0x1037717c, 21)) {
        set_proposal_alternate_option(p, 0x10377210, true);
    } else {
        block(c, 0x10377224, 3);
        const auto nation = c.u32(p.person + staff_nation);
        c.set(0, nation);
        if (guest::nonzero_logical(c, nation)) set_proposal_alternate_option(p, 0x10377230, false);
    }
    block(c, 0x10377240, 3);
    c.set({{3, p.contract}, {4, 0}});
    set_proposal_release_amount(p, 0x1037724c);

    if (first_proposal_release_score(p) && release_competition_approves(p, 0x10377300, 0xa0)) {
        if (propose_release_value_with_conditions(p, 0x10377354, true)) {
            add_proposal_release_premium(p, 0x103773bc);
        }
        apply_release_floor(p, 0x10377468, 150000);
        write_proposal_release_clause(p, 0x1037747c, 0x1c);
    } else {
        remove_proposal_release(p, 0x10377498, 0x1c, 0x198);
    }

    if (second_proposal_release_score(p) && release_competition_approves(p, 0x103775cc, 0xa4)) {
        set_second_proposal_release(p);
    } else {
        remove_proposal_release(p, 0x10377778, 0x1f, 0x198);
    }

    propose_exclusive_release(p);
    if (premium_proposal_release_score(p)) {
        if (propose_release_value_with_conditions(p, 0x10377de8, false)) {
            add_proposal_release_premium(p, 0x10377e44);
        }
        apply_release_floor(p, 0x10377ef0, 1500000);
        write_proposal_release_clause(p, 0x10377f04, 0x1e);
    } else {
        block(c, 0x10377f20, 3);
        c.set(0, 0);
        c.put8(p.contract + 0x1e, 0);
    }
}

void copy_proposal_date(Cpu& c, std::uint32_t from, std::uint32_t to) {
    c.set(0, c.u16(from + 2));
    c.put16(to + 2, c.r(0));
    c.set(0, guest::extended(c.i16(from)));
    c.put16(to, c.r(0));
    c.set(0, c.u8(from + 4));
    c.put8(to + 4, c.r(0));
}

void log_missing_proposal_transfer(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10377f8c, 6);
    const auto frame = c.sp();
    c.set(3, c.toc(-0x4da4));
    c.set({{6, frame + 0x64}, {7, frame + 0x54}, {4, 0}, {5, 0}});
    c.call(0x10739890, 0x10377fa4);
    block(c, 0x10377fa4, 1);
    // The generated call inlines this diagnostic getter and charges its
    // leaf body only after the live TOC read succeeds.
    c.link(0x10377fa8);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, 0x10377fa8, 8);
    c.set(4, c.toc(-0x4da4));
    c.set(5, c.r(3));
    c.set(3, c.toc(-0x7ba8));
    c.set({{6, frame + 0x64}, {4, c.r(4) + 0x14}, {7, frame + 0x54}, {8, 0xc31}});
    c.call(0x1077b720, 0x10377fc8);
    block(c, 0x10377fc8, 5);
    c.set(3, c.toc(-0x4da4));
    c.set(5, 0);
    c.set(4, c.toc(-0x7ba8));
    c.set(3, c.r(3) + 0x21);
    c.call(0x10769b60, 0x10377fdc);
    block(c, 0x10377fdc, 9);
    c.set(3, c.toc(-0x7bac));
    c.set(4, 0);
    c.set(0, c.u16(p.today + 2));
    c.put8(c.r(3), 0);
    c.put16(p.contract + ContractRecord::start_date + 2, c.r(0));
    c.set(0, guest::extended(c.i16(p.today)));
    c.put16(p.contract + ContractRecord::start_date, c.r(0));
    c.set(0, c.u8(p.today + 4));
    c.put8(p.contract + ContractRecord::start_date + 4, c.r(0));
    block(c, 0x10378000, 1);
}

void apply_proposal_transfer_notice(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10377f4c, 2);
    if (!guest::nonzero_logical(c, p.profile)) return;
    block(c, 0x10377f54, 2);
    if (guest::compare_logical(c, p.incumbent_club, p.destination) == 0) return;
    block(c, 0x10377f5c, 3);
    const auto requested = c.i32(c.sp() + 0x154);
    c.set(0, std::uint32_t(requested));
    if (guest::compare(c, requested, -1) == 0) {
        block(c, 0x1037801c, 5);
        c.set(3, c.toc(-0x7b40));
        c.set({{4, p.destination}, {5, p.contract + ContractRecord::start_date}});
        c.set(3, c.u32(c.r(3)));
        c.call(0x103f9200, 0x10378030);
        return;
    }

    block(c, 0x10377f68, 4);
    c.set(3, c.toc(-0x7b40));
    c.set(4, c.r(0));
    c.set(3, c.u32(c.r(3)));
    c.call(0x103eb440, 0x10377f78);
    block(c, 0x10377f78, 2);
    if (!detail::record_nonzero(c, std::int32_t(c.r(3)), 4)) {
        block(c, 0x10377f80, 3);
        const auto entry = c.u32(c.r(4));
        c.set(0, entry);
        if (!guest::nonzero_logical(c, entry)) {
            log_missing_proposal_transfer(p);
            return;
        }
    }
    block(c, 0x10378004, 5);
    c.set(3, c.toc(-0x7b40));
    c.set(5, p.contract + ContractRecord::start_date);
    c.set(4, c.u32(c.r(4)));
    c.set(3, c.u32(c.r(3)));
    c.call(0x103f8cc0, 0x10378018);
    block(c, 0x10378018, 1);
}

void draw_proposal_renewal_date(ContractProposal& p) {
    auto& c = p.c;
    const auto frame = c.sp();
    const auto start = p.contract + ContractRecord::start_date;
    block(c, 0x10378030, 16);
    c.set(0, c.u16(p.today + 2));
    c.set(14, p.today + 2);
    c.set(3, c.toc(-0x7b84));
    c.put16(start + 2, c.r(0));
    c.set(4, c.toc(-0x7b88));
    c.set(0, guest::extended(c.i16(p.today)));
    c.put16(start, c.r(0));
    c.set(0, c.u8(p.today + 4));
    c.put8(start + 4, c.r(0));
    const auto current = c.u32(c.r(3));
    c.set(3, current);
    const auto last = c.u32(c.r(4));
    c.set(0, last);
    const auto next = current + 2;
    c.set(4, next);
    const auto cursor = c.toc(-0x7b84);
    c.set(3, cursor);
    const auto needs_reseed = guest::compare_logical(c, next, last) > 0;
    c.put32(cursor, next);
    if (needs_reseed) {
        block(c, 0x10378070, 3);
        c.set(0, c.toc(-0x7b8c));
        c.put32(cursor, c.r(0));
        c.call(0x1077b770, 0x1037807c);
        block(c, 0x1037807c, 3);
        c.set(0, std::uint16_t(c.r(3)));
        c.set(3, c.toc(-0x7b90));
        c.put32(c.r(3), c.r(0));
    }

    block(c, 0x10378088, 18);
    c.set(3, c.toc(-0x7b84));
    constexpr auto reciprocal = std::int32_t(0x92492493);
    c.set({{4, 0x92490000}, {0, std::uint32_t(reciprocal)}});
    const auto draw = c.u32(c.r(3));
    c.set({{6, draw}, {4, p.destination}});
    c.set(3, c.toc(-0x7b90));
    const auto word = c.u16(draw);
    c.set(6, word);
    const auto salt = c.u32(c.r(3));
    c.set(5, salt);
    c.set(3, c.u32(p.manager_slot));
    const auto mixed = std::int32_t(word + salt);
    const auto high = std::int32_t((std::int64_t(reciprocal) * mixed) >> 32);
    const auto intermediate = std::int32_t(std::uint32_t(high) + std::uint32_t(mixed));
    c.s.wr<1>(guest::reg::xer_ca, intermediate < 0 && (intermediate & 15));
    const auto multiple = (mixed / 28) * 28;
    c.set({{6, std::uint32_t(mixed)}, {5, intermediate < 0 ? 1u : 0u},
           {0, std::uint32_t(multiple)}, {13, std::uint32_t(mixed - multiple)}});
    c.call(0x10089e40, 0x103780d0);

    block(c, 0x103780d0, 7);
    c.set({{0, c.r(13) + 1}, {5, c.r(3)}, {3, frame + 0x44}, {6, 1998}});
    c.set({{4, guest::extended(std::int16_t(c.r(0)))}, {7, UINT32_MAX}});
    c.call(0x107156f0, 0x103780ec);
    block(c, 0x103780ec, 3);
    c.set(13, guest::extended(c.i16(c.r(3))));
    c.set(3, p.destination);
    c.call(0x10191fd0, 0x103780f8);
    block(c, 0x103780f8, 7);
    c.set(0, guest::extended(std::int8_t(c.r(3))));
    c.set(3, c.toc(-0x7b44));
    c.set(0, c.r(0) * 0x4cu);
    c.set(3, c.r(3) + c.r(0));
    if (detail::record_nonzero(c, c.i8(c.r(3) + 0x18))) {
        block(c, 0x10378114, 4);
        c.set(3, c.u16(c.r(14)));
        c.set(0, c.r(3) + 1);
        c.set(5, std::uint16_t(c.r(0)));
    } else {
        block(c, 0x10378124, 1);
        c.set(5, c.u16(c.r(14)));
    }
}

bool proposal_age_below(ContractProposal& p, std::uint32_t pc, int limit) {
    block(p.c, pc, 2);
    return guest::compare(p.c, std::int32_t(p.age), limit) < 0;
}

bool proposal_profile_trait_at_least(ContractProposal& p, std::uint32_t pc, unsigned field, int threshold) {
    auto& c = p.c;
    block(c, pc, 4);
    const auto trait = c.i8(p.profile + field);
    c.set(0, guest::extended(trait));
    return guest::compare(c, trait, threshold) >= 0;
}

void proposal_duration_constant(ContractProposal& p, std::uint32_t pc, unsigned value, bool jump = true) {
    block(p.c, pc, jump ? 2 : 1);
    p.c.set_f(0, p.single(value));
}

void base_proposal_duration(ContractProposal& p) {
    auto& c = p.c;
    if (proposal_age_below(p, 0x10378128, 19)) {
        proposal_duration_constant(p, 0x10378130, 0x54);
    } else if (proposal_age_below(p, 0x10378138, 29)) {
        proposal_duration_constant(p, 0x10378140, 0x40);
    } else if (proposal_age_below(p, 0x10378148, 34)) {
        block(c, 0x10378150, 3);
        const auto profile = c.u32(p.person + staff_profile);
        c.set(3, profile);
        const auto trait_above_fifteen = [&](std::uint32_t pc, unsigned field) {
            block(c, pc, 4);
            const auto value = c.i8(profile + field);
            c.set(0, guest::extended(value));
            return guest::compare(c, value, 15) > 0;
        };
        if (guest::nonzero_logical(c, profile) && trait_above_fifteen(0x1037815c, 0xf)) {
            proposal_duration_constant(p, 0x1037816c, 0x54);
            return;
        }
        block(c, 0x10378174, 2);
        if (guest::nonzero_logical(c, profile) && trait_above_fifteen(0x1037817c, 0x11)) {
            proposal_duration_constant(p, 0x1037818c, 0x44);
            return;
        }
        proposal_duration_constant(p, 0x10378194, 0x3c);
    } else {
        block(c, 0x1037819c, 3);
        c.set(0, c.u32(p.person + staff_nation));
        if (guest::nonzero_logical(c, c.r(0))) {
            block(c, 0x103781a8, 3);
            c.set(0, c.u32(p.person + staff_profile));
            if (!guest::nonzero_logical(c, c.r(0))) {
                block(c, 0x103781b4, 3);
                c.set(0, c.u8(p.person + 0x62));
                if (guest::compare(c, c.r(0), 11) != 0) {
                    proposal_duration_constant(p, 0x103781c0, 0x40);
                    return;
                }
            }
        }
        proposal_duration_constant(p, 0x103781c8, 0x58, false);
    }
}

int proposal_flag(ContractProposal& p, std::uint32_t pc, unsigned base, unsigned field) {
    auto& c = p.c;
    block(c, pc, 3);
    const auto flag = c.i8(c.r(base) + field);
    detail::record_nonzero(c, flag);
    return flag;
}

void scale_proposal_duration(ContractProposal& p, std::uint32_t pc, unsigned value, unsigned instructions) {
    auto& c = p.c;
    block(c, pc, instructions);
    c.set_f(1, p.number(value));
    c.set_f(0, p.fp.fmul(c.f(0), c.f(1)));
    c.set_f(0, p.fp.frsp(c.f(0)));
}

void adjust_duration_for_clauses(ContractProposal& p) {
    auto& c = p.c;
    if (proposal_flag(p, 0x103781cc, 28, 0x1f) > 0 || proposal_flag(p, 0x103781d8, 28, 0x1c) > 0) {
        scale_proposal_duration(p, 0x103781e4, 0x30, 3);
    }
    if (proposal_flag(p, 0x103781f0, 28, 0x1d) > 0) {
        proposal_duration_constant(p, 0x103781fc, 0x58);
        return;
    }
    block(c, 0x10378204, 6);
    const auto band = std::int8_t(p.wage_band);
    const auto reputation = std::int16_t(p.reputation);
    const auto expectation = band * 500;
    c.set({{0, std::uint32_t(band)}, {3, guest::extended(reputation)}, {4, std::uint32_t(expectation)}});
    const auto above = guest::compare(c, reputation, expectation) > 0;
    c.set(0, std::uint32_t(expectation));
    if (above) {
        block(c, 0x1037821c, 1);
        c.set(0, guest::extended(reputation));
    }
    block(c, 0x10378220, 16);
    const auto frame = c.sp();
    c.set({{3, double_header}, {4, c.r(4) ^ 0x80000000u}, {0, c.r(0) ^ 0x80000000u}});
    c.put32(frame + 0x88, c.r(4));
    c.set_f(4, p.number(0xc0));
    const auto young = guest::compare(c, std::int32_t(p.age), 32) < 0;
    c.put32(frame + 0x84, double_header);
    c.set_f(1, c.f64(frame + 0x84));
    c.put32(frame + 0x90, c.r(0));
    c.set_f(2, p.fp.fsubs(c.f(1), c.f(4)));
    c.put32(frame + 0x8c, double_header);
    c.set_f(1, c.f64(frame + 0x8c));
    c.set_f(1, p.fp.fsubs(c.f(1), c.f(4)));
    c.set_f(1, p.fp.fdivs(c.f(2), c.f(1)));
    c.set_f(0, p.fp.fmuls(c.f(0), c.f(1)));
    if (!young) return;
    block(c, 0x10378260, 14);
    const auto experience = c.u8(p.person + 0x65);
    c.set(0, experience);
    c.put32(frame + 0x84, double_header);
    c.set(0, guest::extended(std::int8_t(experience)));
    c.set_f(1, p.number(0x188));
    c.set(0, c.r(0) ^ 0x80000000u);
    c.set_f(3, p.number(0x28));
    c.put32(frame + 0x88, c.r(0));
    c.set_f(2, c.f64(frame + 0x84));
    c.set_f(2, p.fp.fsub(c.f(2), c.f(4)));
    c.set_f(1, p.fp.fdiv(c.f(2), c.f(1)));
    c.set_f(1, p.fp.fadd(c.f(3), c.f(1)));
    c.set_f(1, p.fp.fmul(c.f(31), c.f(1)));
    c.set_f(1, p.fp.frsp(c.f(1)));
    c.set_f(0, p.fp.fadds(c.f(0), c.f(1)));
}

bool proposal_duration_float_less(ContractProposal& p, std::uint32_t pc, unsigned value, bool single) {
    auto& c = p.c;
    block(c, pc, 3);
    c.set_f(1, single ? p.single(value) : p.number(value));
    p.fp.fcmp(0, c.f(0), c.f(1));
    return (c.cr(0) & 8) != 0;
}

void offset_proposal_duration(ContractProposal& p, std::uint32_t pc, unsigned value, bool add, bool jump = true) {
    auto& c = p.c;
    block(c, pc, jump ? 4 : 3);
    c.set_f(1, p.number(value));
    const auto duration = c.f(0);
    c.set_f(0, add ? p.fp.fadd(duration, c.f(1)) : p.fp.fsub(duration, c.f(1)));
    c.set_f(0, p.fp.frsp(c.f(0)));
}

bool adjust_duration_for_role(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10378298, 2);
    if (!guest::nonzero_logical(c, p.profile)) return true;
    block(c, 0x103782a0, 3);
    const auto role = guest::extended(std::int8_t(p.role));
    c.set(0, role);
    if (guest::compare_logical(c, role, 7) > 0) return true;

    block(c, 0x103782ac, 5);
    c.set(3, c.toc(-0x4db4));
    c.set(0, role * 4);
    const auto destination = c.u32(c.r(3) + c.r(0));
    c.set(3, destination);
    c.s.wr<4>(guest::reg::ctr, destination);
    switch (destination) {
        case 0x103782c0:
            scale_proposal_duration(p, 0x103782c0, 0x118, 4);
            break;
        case 0x103782d0:
            scale_proposal_duration(p, 0x103782d0, 0x28, 4);
            break;
        case 0x103782e0: {
            block(c, 0x103782e0, 5);
            c.set_f(1, p.number(0x30));
            const auto young = guest::compare(c, std::int32_t(p.age), 24) < 0;
            c.set_f(0, p.fp.fmul(c.f(0), c.f(1)));
            c.set_f(0, p.fp.frsp(c.f(0)));
            if (!young) break;
            block(c, 0x103782f4, 3);
            const auto band = std::int8_t(p.wage_band);
            c.set(0, guest::extended(band));
            if (guest::compare(c, band, 10) > 0 && proposal_duration_float_less(p, 0x10378300, 0x140, false))
                proposal_duration_constant(p, 0x1037830c, 0x44);
            break;
        }
        case 0x10378314:
            block(c, 0x10378314, 6);
            c.set_f(2, p.number(0x28));
            c.set_f(1, p.number(0x140));
            c.set_f(0, p.fp.fmul(c.f(0), c.f(2)));
            c.set_f(0, p.fp.frsp(c.f(0)));
            p.fp.fcmp(0, c.f(0), c.f(1));
            if (c.cr(0) & 4) proposal_duration_constant(p, 0x1037832c, 0x44);
            break;
        case 0x10378334:
            proposal_duration_constant(p, 0x10378334, 0x58, false);
            break;
        case 0x10378338:
            break;
        default:
            // Only the case labels are reachable in the audited routine.
            // An external target still receives the original live frame.
            if (destination >= proposal_entry && destination < 0x10378d28)
                throw std::logic_error("Unclassified staff-contract proposal continuation");
            c.s.pc = destination;
            return false;
    }
    return true;
}

int compare_proposal_experience(ContractProposal& p, std::uint32_t pc, int limit) {
    auto& c = p.c;
    block(c, pc, 4);
    const auto experience = c.i8(p.person + 0x65);
    c.set(0, guest::extended(experience));
    return guest::compare(c, experience, limit);
}

void adjust_duration_for_reputation(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10378338, 2);
    const std::uint32_t profile = p.profile;
    if (!guest::nonzero_logical(c, profile)) return;
    block(c, 0x10378340, 3);
    const auto reputation = c.i16(profile + 10);
    c.set(4, guest::extended(reputation));
    if (guest::compare(c, reputation, 2250) <= 0) return;
    block(c, 0x1037834c, 4);
    const auto band = std::int8_t(p.wage_band);
    c.set({{3, guest::extended(band)}, {0, std::uint32_t(band * 675)}});
    if (guest::compare(c, reputation, band * 675) <= 0) return;
    block(c, 0x1037835c, 2);
    bool high = guest::compare(c, reputation, 3250) > 0;
    if (high) {
        block(c, 0x10378364, 3);
        c.set(0, std::uint32_t(band * 875));
        high = guest::compare(c, reputation, band * 875) > 0;
    }
    if (high) {
        if (!proposal_age_below(p, 0x10378370, 31)) return;
        block(c, 0x10378378, 2);
        if (guest::compare(c, std::int32_t(p.age), 20) <= 0) return;
        if (compare_proposal_experience(p, 0x10378380, 15) < 0) {
            proposal_duration_constant(p, 0x10378390, 0x58);
        } else {
            offset_proposal_duration(p, 0x10378398, 0x140, false);
        }
        return;
    }
    block(c, 0x103783a8, 2);
    if (guest::compare(c, std::int32_t(p.age), 19) <= 0) {
        offset_proposal_duration(p, 0x10378430, 0x108, false, false);
    } else if (proposal_age_below(p, 0x103783b0, 21)) {
        offset_proposal_duration(p, 0x103783b8, 0x108, false);
    } else if (proposal_age_below(p, 0x103783c8, 27)) {
        if (compare_proposal_experience(p, 0x103783d0, 5) < 0) {
            proposal_duration_constant(p, 0x103783e0, 0x58);
        } else {
            block(c, 0x103783e8, 3);
            c.set_f(1, p.single(0x54));
            p.fp.fcmp(0, c.f(0), c.f(1));
            if (c.cr(0) & 4) offset_proposal_duration(p, 0x103783f4, 0x108, false);
        }
    } else {
        block(c, 0x10378404, 2);
        if (guest::compare(c, std::int32_t(p.age), 29) > 0 && proposal_age_below(p, 0x1037840c, 32)
            && proposal_duration_float_less(p, 0x10378414, 0x140, false)) {
            offset_proposal_duration(p, 0x10378420, 0x108, true);
        }
    }
}

void extend_requested_proposal_duration(ContractProposal& p) {
    if (!proposal_flag(p, 0x1037843c, 1, 0xb4)) return;
    if (!proposal_duration_float_less(p, 0x10378448, 0x160, false)) {
        offset_proposal_duration(p, 0x1037847c, 0x108, true, false);
    } else if (proposal_age_below(p, 0x10378454, 27)) {
        proposal_duration_constant(p, 0x1037845c, 0xa0);
    } else if (proposal_age_below(p, 0x10378464, 30)) {
        proposal_duration_constant(p, 0x1037846c, 0x40);
    } else {
        proposal_duration_constant(p, 0x10378474, 0x54);
    }
}

void adjust_duration_for_league(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10378488, 3);
    const auto league = c.u32(p.destination + 0x2c);
    c.set(4, league);
    if (!guest::nonzero_logical(c, league)) return;
    block(c, 0x10378494, 5);
    c.set(3, c.toc(-0x7594));
    const auto region = c.i32(league + 0x68);
    c.set(4, std::uint32_t(region));
    c.set(0, c.u32(c.r(3)));
    if (guest::compare(c, region, std::int32_t(c.r(0))) != 0) {
        block(c, 0x103784a8, 4);
        c.set(3, c.toc(-0x75b0));
        c.set(0, c.u32(c.r(3)));
        if (guest::compare(c, region, std::int32_t(c.r(0))) != 0) return;
    }
    block(c, 0x103784b8, 6);
    c.set_f(2, p.number(0x28));
    c.set_f(1, p.number(0x160));
    c.set_f(0, p.fp.fmul(c.f(2), c.f(0)));
    c.set_f(0, p.fp.frsp(c.f(0)));
    p.fp.fcmp(0, c.f(0), c.f(1));
    if (c.cr(0) & 4) proposal_duration_constant(p, 0x103784d0, 0x54, false);
}

void cap_proposal_duration_for_wage(ContractProposal& p) {
    auto& c = p.c;
    const auto ensure = [&](std::uint32_t pc, unsigned wage, unsigned duration, bool jump) {
        block(c, pc, 3);
        c.set_f(1, p.single(wage));
        p.fp.fcmp(0, c.f(30), c.f(1));
        if (!(c.cr(0) & 8)) return false;
        const auto lower = proposal_duration_float_less(p, pc + 12, duration, false);
        block(c, pc + (lower ? 24 : 28), 1);
        if (!lower) c.set_f(0, c.f(1));
        block(c, pc + 32, jump ? 2 : 1);
        c.set_f(0, p.fp.frsp(c.f(0)));
        return true;
    };
    if (ensure(0x103784d4, 0xa4, 0x140, true)) return;
    if (ensure(0x103784fc, 0xa8, 0x160, true)) return;
    ensure(0x10378524, 0xac, 0x138, false);
}

void bound_proposal_duration(ContractProposal& p) {
    auto& c = p.c;
    const auto frame = c.sp();
    block(c, 0x10378548, 13);
    c.set({{4, c.toc(-0x7990)}, {3, double_header}});
    c.put32(frame + 0x84, double_header);
    const auto maximum = c.i8(c.r(4));
    c.set_f(2, p.number(0xc0));
    c.set({{4, guest::extended(maximum)}, {0, std::uint32_t(maximum - 1) ^ 0x80000000u}});
    c.put32(frame + 0x88, c.r(0));
    c.set_f(1, c.f64(frame + 0x84));
    c.set_f(1, p.fp.fsubs(c.f(1), c.f(2)));
    p.fp.fcmp(0, c.f(0), c.f(1));
    if (c.cr(0) & 4) {
        block(c, 0x1037857c, 8);
        c.set(0, std::uint32_t(maximum) ^ 0x80000000u);
        c.put32(frame + 0x84, double_header);
        c.set_f(0, p.single(0x58));
        c.put32(frame + 0x88, c.r(0));
        c.set_f(1, c.f64(frame + 0x84));
        c.set_f(1, p.fp.fsubs(c.f(1), c.f(2)));
        c.set_f(0, p.fp.fsubs(c.f(1), c.f(0)));
    } else if (proposal_duration_float_less(p, 0x1037859c, 0x108, false)) {
        proposal_duration_constant(p, 0x103785a8, 0x58, false);
    }
}

bool proposal_short_duration_exception(ContractProposal& p, std::uint32_t pc, unsigned factor) {
    auto& c = p.c;
    block(c, pc, 3);
    c.set(0, c.u32(p.person + registered_club));
    if (guest::nonzero_logical(c, c.r(0))) return false;
    block(c, pc + 12, 2);
    if (guest::compare(c, std::int32_t(c.r(4)), 2) >= 0) return false;
    block(c, pc + 20, 5);
    c.set_f(2, p.number(factor));
    c.set_f(1, p.number(0x168));
    c.set_f(2, p.fp.fmul(c.f(2), c.f(31)));
    p.fp.fcmp(0, c.f(2), c.f(1));
    return (c.cr(0) & 2) != 0;
}

void add_proposal_years(ContractProposal& p, std::uint32_t pc, bool short_term) {
    auto& c = p.c;
    block(c, pc, short_term ? 15 : 11);
    c.set_f(1, p.number(0x108));
    c.set(3, 2);
    const auto packed = (c.u8(p.contract + ContractRecord::packed_kind) & 3u) | 8u;
    c.set(0, packed);
    c.set_f(0, p.fp.fadd(c.f(1), c.f(0)));
    if (short_term) c.set(3, 1);
    c.put8(p.contract + ContractRecord::packed_kind, packed);
    const auto years = p.fp.fctiwz(0, c.f(0));
    if (short_term) {
        c.set(0, c.u8(p.contract + ContractRecord::packed_kind) | 1);
        c.put8(p.contract + ContractRecord::packed_kind, c.r(0));
    }
    c.put64(c.sp() + 0x84, years);
    c.set(0, c.u32(c.sp() + 0x88));
    c.set(5, c.r(5) + c.r(0));
}

void set_proposal_duration_kind(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x103785ac, 7);
    const auto band = std::int8_t(p.wage_band);
    const auto reputation = std::int16_t(p.reputation);
    const auto limit = std::int16_t(band * 500 + 1250);
    c.set({{4, guest::extended(band)}, {6, guest::extended(reputation)},
           {3, std::uint32_t(band * 500)}, {0, guest::extended(limit)}});
    if (guest::compare(c, reputation, limit) > 0) {
        const auto short_term = proposal_short_duration_exception(p, 0x103785c8, 0x138);
        add_proposal_years(p, short_term ? 0x1037861c : 0x103785f0, short_term);
    } else if (!proposal_short_duration_exception(p, 0x10378658, 0x1a0)) {
        add_proposal_years(p, 0x10378680, false);
    } else {
        block(c, 0x103786ac, 2);
        if (guest::compare(c, std::int32_t(p.age), 20) > 0) {
            add_proposal_years(p, 0x103786b4, true);
        } else {
            block(c, 0x103786f0, 4);
            const auto packed = (c.u8(p.contract + ContractRecord::packed_kind) & 3u) | 4u;
            c.set({{3, 1}, {0, packed}});
            c.put8(p.contract + ContractRecord::packed_kind, packed);
        }
    }
}

void set_proposal_expiry_date(ContractProposal& p) {
    auto& c = p.c;
    const auto frame = c.sp();
    block(c, 0x10378700, 3);
    c.set({{4, c.r(13)}, {3, frame + 0x4c}});
    c.call(0x107156b0, 0x1037870c);
    block(c, 0x1037870c, 8);
    c.set(3, c.u32(p.person + 0x54));
    c.set(0, c.u32(p.person + 0x58));
    c.put32(frame + 0x3c, c.r(3));
    c.set(3, c.u16(frame + 0x4e));
    c.set(4, c.u16(frame + 0x3e));
    c.put32(frame + 0x40, c.r(0));
    const auto year = guest::compare_logical(c, c.r(4), c.r(3));
    if (year > 0) {
        block(c, 0x1037872c, 2);
        c.set(0, 1);
    } else {
        block(c, 0x10378734, 1);
        if (year < 0) {
            block(c, 0x10378738, 2);
            c.set(0, 0);
        } else {
            block(c, 0x10378740, 8);
            const auto recorded = c.i16(frame + 0x3c);
            c.set(4, guest::extended(recorded));
            const auto offered = c.i16(frame + 0x4c);
            const auto difference = std::int32_t(recorded ^ offered);
            c.s.wr<1>(guest::reg::xer_ca, difference < 0 && (difference & 1));
            c.set({{3, std::uint32_t(difference >> 1)}, {0, recorded > offered ? 1u : 0u}});
        }
    }
    block(c, 0x10378760, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(0)))) {
        lookup_release_club(p, 0x10378768);
        block(c, 0x103787c8, 2);
        if (guest::compare_logical(c, p.destination, c.r(0)) == 0) {
            block(c, 0x103787d0, 6);
            copy_proposal_date(c, p.person + 0x54, frame + 0x4c);
        }
    }
    block(c, 0x103787e8, 8);
    copy_proposal_date(c, frame + 0x4c, p.contract + ContractRecord::end_date);
}

bool set_proposal_dates(ContractProposal& p) {
    block(p.c, 0x10377f44, 2);
    if (!guest::nonzero_logical(p.c, p.destination)) return true;
    apply_proposal_transfer_notice(p);
    draw_proposal_renewal_date(p);
    base_proposal_duration(p);
    adjust_duration_for_clauses(p);
    if (!adjust_duration_for_role(p)) return false;
    adjust_duration_for_reputation(p);
    extend_requested_proposal_duration(p);
    adjust_duration_for_league(p);
    cap_proposal_duration_for_wage(p);
    bound_proposal_duration(p);
    set_proposal_duration_kind(p);
    set_proposal_expiry_date(p);
    return true;
}

void write_proposal_bonus(ContractProposal& p, std::uint32_t pc, unsigned field, bool offered) {
    auto& c = p.c;
    block(c, pc, offered ? 12 : 2);
    if (offered) {
        c.set_f(0, p.fp.fmuls(c.f(31), c.f(30)));
        c.set_f(2, p.single(0xb4));
        c.set_f(3, p.single(0xb0));
        c.set_f(1, p.fp.fdivs(c.f(30), c.f(2)));
        c.set_f(0, p.fp.fdivs(c.f(0), c.f(2)));
        c.set_f(0, p.fp.fadds(c.f(1), c.f(0)));
        c.set_f(0, p.fp.fadds(c.f(3), c.f(0)));
        c.put64(c.sp() + 0x84, p.fp.fctiwz(0, c.f(0)));
        c.set(0, c.u32(c.sp() + 0x88));
    } else {
        c.set(0, UINT32_MAX);
    }
    c.put32(p.contract + field, c.r(0));
}

bool has_performance_bonus_trait(ContractProposal& p, std::uint32_t first_check) {
    return proposal_profile_trait_at_least(p, first_check, 0x13, 15)
        || proposal_profile_trait_at_least(p, first_check + 16, 0x14, 15)
        || proposal_profile_trait_at_least(p, first_check + 32, 0x15, 15);
}

bool has_potential_bonus_trait(ContractProposal& p, std::uint32_t pc) {
    auto& c = p.c;
    block(c, pc, 3);
    const auto potential = c.u8(p.profile + 0xf);
    c.set(0, potential);
    return guest::compare(c, potential, 20) == 0;
}

void propose_performance_bonuses(ContractProposal& p, bool came_from_dates) {
    auto& c = p.c;
    // The dated path already charged this test in the preceding block.
    if (!came_from_dates) block(c, 0x10378800, 2);
    if (!guest::nonzero_logical(c, p.profile)) return;
    if (proposal_profile_trait_at_least(p, 0x10378808, 0x15, 15)) {
        write_proposal_bonus(p, 0x10378818, 0x10, true);
    } else {
        write_proposal_bonus(p, 0x10378848, 0x10, false);
    }
    if (has_performance_bonus_trait(p, 0x10378850)) {
        write_proposal_bonus(p, 0x10378880, 0x14, true);
    } else {
        write_proposal_bonus(p, 0x103788b0, 0x14, false);
    }
    if (has_potential_bonus_trait(p, 0x103788b8)) {
        write_proposal_bonus(p, 0x103788c4, 0x18, true);
    } else {
        write_proposal_bonus(p, 0x103788f4, 0x18, false);
    }
}

void adjust_proposal_contract_flag(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x103788fc, 3);
    c.set_f(0, p.single(0xb8));
    p.fp.fcmp(0, c.f(30), c.f(0));
    if (c.cr(0) & 8) {
        block(c, 0x10378908, 2);
        if (guest::nonzero_logical(c, p.destination)) {
            block(c, 0x10378910, 3);
            const auto club_type = c.u8(p.destination + 0x23d);
            c.set(0, club_type);
            if (guest::compare(c, club_type, 2) == 0) {
                block(c, 0x1037891c, 5);
                const auto packed = c.u8(p.contract + ContractRecord::packed_kind) | 1u;
                c.set({{0, packed}, {3, 1}});
                c.put8(p.contract + ContractRecord::packed_kind, packed);
                return;
            }
        }
    }
    block(c, 0x10378930, 3);
    c.set_f(0, p.single(0xbc));
    p.fp.fcmp(0, c.f(30), c.f(0));
    if (!(c.cr(0) & 4)) return;
    block(c, 0x1037893c, 5);
    const auto packed = c.u8(p.contract + ContractRecord::packed_kind);
    c.set(3, packed);
    c.s.wr<1>(guest::reg::xer_ca, 0);
    if (detail::record_nonzero(c, -(packed & 1u))) {
        block(c, 0x10378950, 3);
        c.set({{0, 0}, {3, packed & ~1u}});
        c.put8(p.contract + ContractRecord::packed_kind, c.r(3));
    }
}

void set_proposal_weekly_wage(ContractProposal& p) {
    auto& c = p.c;
    const auto frame = c.sp();
    block(c, 0x1037895c, 17);
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(30));
    c.set(0, double_header);
    c.put32(frame + 0x8c, double_header);
    c.set_f(1, p.number(0xc0));
    c.put64(frame + 0x84, c.s.r<8>(guest::reg::fpr(0)));
    c.set(0, c.u32(frame + 0x88));
    c.put32(p.contract + 12, c.r(0));
    c.set(3, c.u32(p.contract + 12));
    const auto packed = c.u8(p.contract + ContractRecord::packed_kind);
    c.set(0, packed);
    c.set(3, c.r(3) ^ 0x80000000u);
    c.put32(frame + 0x90, c.r(3));
    const auto kind = detail::unpack_kind(packed);
    c.set(0, std::uint32_t(kind));
    c.s.wr<1>(guest::reg::xer_ca, (packed & 0x82u) == 0x82u);
    c.set_f(0, c.f64(frame + 0x8c));
    const auto compared = guest::compare(c, kind, 2);
    c.set_f(1, p.fp.fsubs(c.f(0), c.f(1)));
    if (compared != 0) {
        block(c, 0x103789a0, 1);
        if (compared < 0) {
            block(c, 0x103789a4, 2);
            if (guest::compare(c, kind, 1) >= 0) {
                block(c, 0x103789b0, 3);
                c.set_f(0, p.number(0x28));
                c.set_f(1, p.fp.fmul(c.f(1), c.f(0)));
                c.set_f(1, p.fp.frsp(c.f(1)));
            } else {
                block(c, 0x103789ac, 1);
            }
        }
    }
    block(c, 0x103789bc, 5);
    const auto flags = c.u8(p.contract + ContractRecord::packed_kind);
    c.s.wr<1>(guest::reg::xer_ca, 0);
    if (detail::record_nonzero(c, -(flags & 1u))) {
        block(c, 0x103789d0, 3);
        c.set_f(0, p.number(0x198));
        c.set_f(1, p.fp.fmul(c.f(0), c.f(1)));
        c.set_f(1, p.fp.frsp(c.f(1)));
    }
}

void factor_proposal_bonus(ContractProposal& p, std::uint32_t pc, bool read_again, bool add, bool jump = true) {
    auto& c = p.c;
    block(c, pc, read_again && jump ? 16 : 15);
    const auto frame = c.sp();
    c.set(5, 0x66660000);
    if (read_again) c.set(6, c.u32(p.contract + c.r(7)));
    c.set({{5, 0x66666667}, {0, double_header}});
    const auto bonus = std::int32_t(c.r(6));
    const auto high = std::int32_t((INT64_C(0x66666667) * bonus) >> 32);
    if (read_again) c.set(5, std::uint32_t(high));
    c.put32(frame + 0x84, double_header);
    c.set_f(3, p.number(0xc0));
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 3));
    c.set({{5, high < 0 ? 1u : 0u}, {0, std::uint32_t(bonus / 10) ^ 0x80000000u}});
    c.put32(frame + 0x88, c.r(0));
    c.set_f(2, c.f64(frame + 0x84));
    c.set_f(2, p.fp.fsubs(c.f(2), c.f(3)));
    const auto cost = c.f(0);
    c.set_f(0, add ? p.fp.fadds(cost, c.f(2)) : p.fp.fsubs(cost, c.f(2)));
}

void adjust_proposal_bonus_value(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x103789dc, 3);
    const auto profile = guest::nonzero_logical(c, p.profile);
    c.set_f(0, p.single(0xc0));
    if (!profile) return;
    block(c, 0x103789e8, 5);
    c.set({{0, 3}, {4, 0}, {3, 0}});
    c.s.wr<4>(guest::reg::ctr, 3);
    for (unsigned index = 0; index < 3; ++index) {
        block(c, 0x103789fc, 4);
        c.set(7, c.r(3) + 0x10);
        const auto amount = c.i32(p.contract + c.r(7));
        c.set(6, std::uint32_t(amount));
        if (guest::compare(c, amount, -1) > 0) {
            block(c, 0x10378a0c, 2);
            if (guest::compare(c, index, 2) == 0) {
                const auto desired = has_potential_bonus_trait(p, 0x10378adc);
                factor_proposal_bonus(p, desired ? 0x10378ae8 : 0x10378b24, false, desired);
            } else {
                block(c, 0x10378a14, 1);
                block(c, 0x10378a18, 2);
                guest::compare(c, index, 0);
                if (has_performance_bonus_trait(p, 0x10378a24)) {
                    factor_proposal_bonus(p, 0x10378a54, true, true);
                } else if (has_potential_bonus_trait(p, 0x10378a94)) {
                    factor_proposal_bonus(p, 0x10378aa0, false, false);
                }
            }
        }
        block(c, 0x10378b9c, 3);
        c.set({{3, c.r(3) + 4}, {4, c.r(4) + 1}});
        c.s.wr<4>(guest::reg::ctr, 2 - index);
    }
}

void add_proposal_release_value(ContractProposal& p, std::uint32_t pc, unsigned coefficient, bool jump = true) {
    auto& c = p.c;
    block(c, pc, jump ? 4 : 3);
    c.set_f(2, p.single(coefficient));
    c.set_f(2, p.fp.fdivs(c.f(30), c.f(2)));
    c.set_f(0, p.fp.fadds(c.f(0), c.f(2)));
}

int compare_proposal_release_value(ContractProposal& p, std::uint32_t pc, bool reread, unsigned minimum) {
    auto& c = p.c;
    block(c, pc, reread ? 5 : 4);
    c.set(3, (minimum + 0x8000u) & 0xffff0000u);
    if (reread) c.set(4, c.u32(p.contract + ContractRecord::release_amount));
    c.set(0, minimum);
    return guest::compare(c, std::int32_t(c.r(4)), minimum);
}

void adjust_proposal_release_value(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10378ba8, 3);
    c.set({{0, 5}, {5, 0}});
    c.s.wr<4>(guest::reg::ctr, 5);
    for (unsigned index = 0; index < 5; ++index) {
        block(c, 0x10378bb4, 4);
        c.set(0, index + 0x1c);
        if (detail::record_nonzero(c, c.i8(p.contract + c.r(0)))) {
            block(c, 0x10378bc4, 2);
            const auto compared = guest::compare(c, index, 2);
            if (compared == 0) {
                if (compare_proposal_release_value(p, 0x10378c30, true, 2500000) < 0)
                    add_proposal_release_value(p, 0x10378c44, 0x9c);
                else
                    add_proposal_release_value(p, 0x10378c54, 0xc8);
            } else {
                block(c, 0x10378bcc, 1);
                if (compared < 0) {
                    block(c, 0x10378bd0, 2);
                    guest::compare(c, index, 1);
                    if (index == 0) block(c, 0x10378bd8, 1);
                }
                if (index == 1) {
                    if (compare_proposal_release_value(p, 0x10378bdc, true, 500000) < 0)
                        add_proposal_release_value(p, 0x10378bf0, 0x9c);
                    else if (compare_proposal_release_value(p, 0x10378c00, false, 2500000) < 0)
                        add_proposal_release_value(p, 0x10378c10, 0x94);
                    else
                        add_proposal_release_value(p, 0x10378c20, 0xc4);
                } else {
                    if (compare_proposal_release_value(p, 0x10378c64, true, 500000) < 0)
                        add_proposal_release_value(p, 0x10378c78, 0xc8);
                    else if (compare_proposal_release_value(p, 0x10378c88, false, 2500000) < 0)
                        add_proposal_release_value(p, 0x10378c98, 0xb0);
                    else
                        add_proposal_release_value(p, 0x10378ca8, 0xcc, false);
                }
            }
        }
        block(c, 0x10378cb4, 2);
        c.set(5, index + 1);
        c.s.wr<4>(guest::reg::ctr, 4 - index);
    }
}

void close_proposal_frame(ContractProposal& p) {
    auto& c = p.c;
    block(c, 0x10378cbc, 2);
    if (!guest::nonzero_logical(c, p.profile)) {
        block(c, 0x10378cc4, 9);
        c.set({{3, 0}, {0, UINT32_MAX}});
        for (unsigned field = 0x1d; field <= 0x20; ++field) c.put8(p.contract + field, 0);
        for (unsigned field = 0x10; field <= 0x18; field += 4) c.put32(p.contract + field, UINT32_MAX);
    }
    block(c, 0x10378ce8, 4);
    c.set_f(2, p.single(0x9c));
    c.set_f(2, p.fp.fdivs(c.f(30), c.f(2)));
    p.fp.fcmp(0, c.f(0), c.f(2));
    if (c.cr(0) & 4) {
        block(c, 0x10378cf8, 1);
        c.set_f(0, c.f(2));
    }
    block(c, 0x10378cfc, 8);
    c.set_f(1, p.fp.fadds(c.f(1), c.f(0)));
    const auto frame = c.sp();
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(1));
    c.put64(frame + 0x84, c.s.r<8>(guest::reg::fpr(0)));
    c.set(3, c.u32(frame + 0x88));
    c.set(0, c.u32(frame + 0x138));
    const auto stack = frame + proposal_frame;
    c.set({{1, stack}, {11, stack}});
    c.call(0x10773858, 0x10378d1c);
    block(c, 0x10378d1c, 3);
    c.set_lr(c.r(0));
    detail::transfer_saved_registers<13, true>(c, stack - 0x20);
    c.finish();
}
}

void propose_staff_contract(Cpu& c) {
    GuestNativeScope scope(c.s, proposal_entry);
    open_proposal_frame(c, scope);
    ContractProposal proposal(c);
    initialize_proposal_context(proposal);
    choose_proposal_band(proposal);
    raise_proposal_band_for_experience(proposal);
    load_proposal_reputation(proposal);
    calculate_proposal_value(proposal);
    if (quote_proposal_for_profile(proposal)) {
        cap_proposal_growth(proposal);
        if (prepare_proposal_wage_uplift(proposal)) raise_proposal_wage(proposal);
        compute_proposal_wage_multiplier(proposal);
        floor_proposal_wage_at_existing_contract(proposal);
        floor_proposal_wage_at_club(proposal);
        cap_proposal_wage_for_age(proposal);
        if (!cap_proposal_wage_for_role(proposal)) blend_proposal_incumbent_wage(proposal);
    } else {
        quote_proposal_for_staff(proposal);
        cap_proposal_staff_wage(proposal);
    }
    propose_release_clauses(proposal);
    if (!set_proposal_dates(proposal)) return;
    propose_performance_bonuses(proposal, proposal.destination != 0);
    adjust_proposal_contract_flag(proposal);
    set_proposal_weekly_wage(proposal);
    adjust_proposal_bonus_value(proposal);
    adjust_proposal_release_value(proposal);
    close_proposal_frame(proposal);
}
}
