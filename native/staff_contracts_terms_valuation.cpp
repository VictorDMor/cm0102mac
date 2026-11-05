#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using detail::record_nonzero;
using guest::Cpu;
using guest::Fpu;

constexpr auto valuation_entry = 0x1037e660u, proposal_entry = 0x10374f50u;
constexpr auto power = 0x10780fc0u, high_word = 0x43300000u;
constexpr auto frame_size = 0x100u, temporary_contract = 0x38u;
constexpr auto wage = 0x0cu, employment = 0x14u;

struct Valuation {
    Cpu& c;
    Fpu fp;
    const guest::Kept<23> draft;
    const guest::Kept<30> doubles;
    const guest::Kept<24> staff;
    const guest::Kept<31> singles;
    const guest::Kept<25> alternative;

    Valuation(Cpu& cpu, std::uint32_t record, std::uint32_t person, std::uint32_t comparison)
        : c(cpu), fp(cpu), draft(cpu, record), doubles(cpu, cpu.toc(-0x4dac)),
          staff(cpu, person), singles(cpu, cpu.toc(-0x4da8)), alternative(cpu, comparison) {}

    double double_constant(unsigned offset) const { return c.f64(doubles + offset); }
    double single_constant(unsigned offset) const { return c.f32(singles + offset); }

    void scale(unsigned offset) {
        c.set_f(0, single_constant(offset));
        c.set_f(31, fp.fmuls(c.f(31), c.f(0)));
    }

    void scale_double(unsigned dest, unsigned offset) {
        c.set_f(0, double_constant(offset));
        c.set_f(dest, fp.fmul(c.f(dest), c.f(0)));
        c.set_f(dest, fp.frsp(c.f(dest)));
    }

    void scale_at(std::uint32_t site, unsigned offset) {
        block(c, site, 3);
        scale(offset);
    }

    bool alternative_option(unsigned field) {
        return record_nonzero(c, c.i8(alternative + field)) && (c.cr(0) & 4);
    }
};

template<class Value>
void copy_valuation_field(Cpu& c, std::uint32_t source, std::uint32_t destination, unsigned offset) {
    const auto value = guest::Field<Value>(c, source + offset).get();
    c.set(0, std::uint32_t(value));
    const guest::Field<Value> field(c, destination + offset);
    field = value;
}

void copy_alternative(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037e6dc, 20);
    const auto source = std::uint32_t(v.alternative);
    const auto destination = c.sp() + temporary_contract;
    const auto id = c.u32(source);
    c.set({{0, id}, {3, destination + 0x1c}, {4, source + 0x1c}, {5, 5}});
    c.put32(destination, id);
    for (auto offset : {4u, 8u, 12u}) copy_valuation_field<std::uint32_t>(c, source, destination, offset);
    const auto appearance = c.u32(source + 0x10);
    c.set(6, appearance);
    const auto scoring = c.u32(source + 0x14);
    c.set(0, scoring);
    c.put32(destination + 0x10, appearance);
    c.put32(destination + 0x14, scoring);
    copy_valuation_field<std::uint16_t>(c, source, destination, 0x18);
    copy_valuation_field<std::uint16_t>(c, source, destination, 0x1a);
    c.call(0x10771110, 0x1037e72c);

    block(c, 0x1037e72c, 43);
    copy_valuation_field<std::uint32_t>(c, v.alternative, destination, ContractRecord::release_amount);
    for (auto date : {ContractRecord::start_date, ContractRecord::end_date}) {
        copy_valuation_field<std::uint16_t>(c, v.alternative, destination, date + 2);
        copy_valuation_field<std::int16_t>(c, v.alternative, destination, date);
        copy_valuation_field<std::uint8_t>(c, v.alternative, destination, date + 4);
    }
    copy_valuation_field<std::uint8_t>(c, v.alternative, destination, ContractRecord::packed_kind);
    copy_valuation_field<std::uint32_t>(c, v.alternative, destination, 0x3c);
    copy_valuation_field<std::uint8_t>(c, v.alternative, destination, 0x40);
    copy_valuation_field<std::int16_t>(c, v.alternative, destination, ContractRecord::review_marker);
    for (auto offset : {0x44u, 0x45u, 0x46u})
        copy_valuation_field<std::uint8_t>(c, v.alternative, destination, offset);
    copy_valuation_field<std::uint32_t>(c, v.alternative, destination, ContractRecord::status);
    copy_valuation_field<std::uint8_t>(c, v.alternative, destination, 0x4c);
    copy_valuation_field<std::uint32_t>(c, v.alternative, destination, ContractRecord::review_conditions);
    copy_valuation_field<std::uint8_t>(c, v.alternative, destination, ContractRecord::terms_flag);
    copy_valuation_field<std::uint32_t>(c, v.alternative, destination, ContractRecord::terms_marker);
    copy_valuation_field<std::uint8_t>(c, v.alternative, destination, ContractRecord::review_flags);
    copy_valuation_field<std::uint32_t>(c, v.alternative, destination, 0x60);
}

void make_alternative(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037e7d8, 13);
    const auto club = c.u32(v.draft + ContractRecord::club_id);
    c.set({{0, club}, {4, v.staff}});
    const auto clubs_slot = c.toc(-0x7be8);
    c.set({{5, clubs_slot}, {3, c.sp() + temporary_contract}, {0, club * 0x248}, {6, UINT32_MAX}});
    const auto clubs = c.u32(clubs_slot);
    c.set({{5, clubs + club * 0x248}, {7, UINT32_MAX}, {8, 11}, {9, 0}, {10, 0}});
    c.call(proposal_entry, 0x1037e80c);
}

std::int16_t remaining_term(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037e80c, 5);
    const auto today = c.toc(-0x7b80);
    c.set({{5, today}, {3, v.draft + ContractRecord::end_date}});
    c.set(4, c.u32(today));
    c.set(5, c.u32(today + 4));
    c.call(0x10718130, 0x1037e820);
    block(c, 0x1037e820, 3);
    const auto remaining = std::int16_t(c.r(3));
    c.set(29, guest::extended(remaining));
    guest::compare(c, remaining, 7);
    return remaining;
}

void add_bonus(Valuation& v, std::uint32_t site, unsigned field, unsigned multiplier, unsigned scratch) {
    auto& c = v.c;
    block(c, site, 3);
    const auto bonus = c.i32(v.draft + field);
    c.set(0, std::uint32_t(bonus));
    if (guest::compare(c, bonus, -1) <= 0) return;

    block(c, site + 12, 11);
    const auto encoded = std::uint32_t(bonus) ^ 0x80000000;
    c.set({{3, encoded}, {0, high_word}});
    c.put32(c.sp() + scratch + 4, encoded);
    c.set_f(1, v.double_constant(0xc0));
    c.put32(c.sp() + scratch, high_word);
    c.set_f(2, v.double_constant(multiplier));
    c.set_f(0, c.f64(c.sp() + scratch));
    c.set_f(0, v.fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, v.fp.fmul(c.f(2), c.f(0)));
    c.set_f(0, v.fp.frsp(c.f(0)));
    c.set_f(31, v.fp.fadds(c.f(31), c.f(0)));
}

void add_role_bonuses(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037e834, 13);
    const auto wage_word = c.u32(v.draft + wage);
    c.set({{4, wage_word}, {0, high_word}});
    const auto staff_profile = c.u32(v.staff + employment);
    c.set({{3, staff_profile}, {4, wage_word ^ 0x80000000}});
    const auto age = c.i16(v.staff + 0x5c);
    c.set(5, guest::extended(age));
    c.put32(c.sp() + 0xa8, wage_word ^ 0x80000000);
    const auto has_profile = guest::nonzero_logical(c, staff_profile);
    c.set_f(1, v.double_constant(0xc0));
    c.set(28, std::uint32_t(std::int8_t(age)));
    c.put32(c.sp() + 0xa4, high_word);
    c.set_f(0, c.f64(c.sp() + 0xa4));
    c.set_f(31, v.fp.fsubs(c.f(0), c.f(1)));
    if (!has_profile) return;

    block(c, 0x1037e868, 1);
    c.call(0x10117490, 0x1037e86c);
    const auto roles = guest::extended(std::int16_t(c.r(3)));
    block(c, 0x1037e86c, 3);
    c.set(4, roles);
    if (record_nonzero(c, roles & 1)) {
        add_bonus(v, 0x1037e878, 0x18, 0x248, 0xa4);
        block(c, 0x1037e8b0, 1);
        c.set(27, 33);
    }
    block(c, 0x1037e8b4, 2);
    if (record_nonzero(c, roles & 6)) {
        add_bonus(v, 0x1037e8bc, 0x10, 0x1e8, 0xa4);
        add_bonus(v, 0x1037e8f4, 0x14, 0xa0, 0x9c);
        block(c, 0x1037e92c, 1);
        c.set(27, 31);
    }
    block(c, 0x1037e930, 2);
    if (record_nonzero(c, roles & 0x38)) {
        add_bonus(v, 0x1037e938, 0x10, 0x250, 0x9c);
        add_bonus(v, 0x1037e970, 0x14, 0x100, 0xa4);
        block(c, 0x1037e9a8, 1);
        c.set(27, 29);
    }
    block(c, 0x1037e9ac, 2);
    if (record_nonzero(c, roles & 0x40)) {
        add_bonus(v, 0x1037e9b4, 0x10, 0x248, 0x9c);
        add_bonus(v, 0x1037e9ec, 0x14, 0x28, 0xa4);
        block(c, 0x1037ea24, 1);
        c.set(27, 28);
    }
}

void add_signing_value(Valuation& v, std::int16_t duration) {
    auto& c = v.c;
    block(c, 0x1037ea28, 20);
    const auto encoded_days = std::uint32_t(duration) ^ 0x80000000;
    c.set({{3, high_word}, {0, encoded_days}});
    c.put32(c.sp() + 0xa8, encoded_days);
    const auto amount = c.u32(v.draft + 8);
    c.set(0, amount);
    c.put32(c.sp() + 0xa4, high_word);
    c.set_f(3, v.double_constant(0xc0));
    const auto encoded_amount = amount ^ 0x80000000;
    c.set(0, encoded_amount);
    c.set_f(1, c.f64(c.sp() + 0xa4));
    c.set_f(0, v.single_constant(0x16c));
    c.set_f(1, v.fp.fsubs(c.f(1), c.f(3)));
    const auto staff_profile = c.u32(v.staff + employment);
    c.set(4, staff_profile);
    c.put32(c.sp() + 0xa0, encoded_amount);
    const auto has_profile = guest::nonzero_logical(c, staff_profile);
    c.set_f(0, v.fp.fdivs(c.f(1), c.f(0)));
    c.put32(c.sp() + 0x9c, high_word);
    c.set_f(1, c.f64(c.sp() + 0x9c));
    c.set_f(1, v.fp.fsubs(c.f(1), c.f(3)));
    c.set_f(0, v.fp.fdivs(c.f(1), c.f(0)));
    c.set_f(31, v.fp.fadds(c.f(31), c.f(0)));
    if (!has_profile) return;

    block(c, 0x1037ea78, 3);
    const auto reputation = c.i16(staff_profile + 12);
    c.set(0, guest::extended(reputation));
    if (guest::compare(c, reputation, 4000) <= 0) return;
    block(c, 0x1037ea84, 7);
    const auto encoded = std::uint32_t(reputation) ^ 0x80000000;
    c.set(0, encoded);
    c.put32(c.sp() + 0x9c, c.r(3));
    c.set_f(2, v.double_constant(0x140));
    c.put32(c.sp() + 0xa0, encoded);
    c.set_f(0, c.f64(c.sp() + 0x9c));
    c.set_f(1, v.fp.fsub(c.f(0), c.f(3)));
    c.call(power, 0x1037eaa0);

    block(c, 0x1037eaa0, 13);
    c.set(2, c.u32(c.sp() + 0x14));
    c.set_f(0, v.double_constant(0x228));
    c.set(0, high_word);
    const auto live_amount = c.u32(v.draft + 8);
    c.set(3, live_amount);
    c.set_f(0, v.fp.fdiv(c.f(1), c.f(0)));
    c.put32(c.sp() + 0xa4, high_word);
    c.set(0, live_amount ^ 0x80000000);
    c.set_f(2, v.double_constant(0xc0));
    c.put32(c.sp() + 0xa8, live_amount ^ 0x80000000);
    c.set_f(1, c.f64(c.sp() + 0xa4));
    c.set_f(1, v.fp.fsub(c.f(1), c.f(2)));
    v.fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) {
        block(c, 0x1037ead4, 2);
        v.scale(0x98);
    }
}

std::int32_t read_fee(Valuation& v, std::uint32_t contract, unsigned reg) {
    const auto fee = v.c.i32(contract + ContractRecord::release_amount);
    v.c.set(reg, std::uint32_t(fee));
    return fee;
}

bool both_fees_below(Valuation& v, std::uint32_t site, int limit, bool alternative_first, unsigned first_reg) {
    auto& c = v.c;
    block(c, site, 5);
    c.set(4, std::uint32_t(limit) & 0xffff0000u);
    const auto first = read_fee(v, alternative_first ? v.alternative : v.draft, first_reg);
    c.set(4, std::uint32_t(limit));
    if (guest::compare(c, first, limit) >= 0) return false;
    block(c, site + 20, 3);
    const auto second = read_fee(v, alternative_first ? v.draft : v.alternative, 0);
    return guest::compare(c, second, limit) < 0;
}

void scale_by_fee_comparison(Valuation& v, bool cheaper, std::uint32_t site,
                             unsigned smaller_factor, unsigned other_factor) {
    v.scale_at(cheaper ? site : site + 12, cheaper ? smaller_factor : other_factor);
}

void minimum_fee_weight(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037eb24, 3);
    if (!v.alternative_option(0x1d)) {
        v.scale_at(0x1037ebd8, 0x34);
        return;
    }
    if (both_fees_below(v, 0x1037eb30, 1'000'000, true, 6)) {
        block(c, 0x1037eb50, 2);
        const auto cheaper = guest::compare(c, std::int32_t(c.r(6)), std::int32_t(c.r(0))) < 0;
        scale_by_fee_comparison(v, cheaper, 0x1037eb58, 0x170, 0x38);
        return;
    }
    block(c, 0x1037eb70, 5);
    c.set(4, 0x720000);
    const auto current = read_fee(v, v.draft, 5);
    c.set(0, 7'500'000);
    if (guest::compare(c, current, 7'500'000) < 0) {
        block(c, 0x1037eb84, 2);
        if (guest::compare(c, std::int32_t(c.r(6)), 7'500'000) < 0) {
            block(c, 0x1037eb8c, 3);
            const auto cheaper = guest::compare(c, read_fee(v, v.alternative, 0), current) < 0;
            scale_by_fee_comparison(v, cheaper, 0x1037eb98, 0x174, 0x3c);
            return;
        }
    }
    block(c, 0x1037ebb0, 4);
    const auto rival = read_fee(v, v.alternative, 4);
    const auto cheaper = guest::compare(c, rival, read_fee(v, v.draft, 0)) < 0;
    scale_by_fee_comparison(v, cheaper, 0x1037ebc0, 0x4c, 0x34);
}

void comparative_clause_weight(Valuation& v, std::uint32_t site, bool younger) {
    auto& c = v.c;
    if (both_fees_below(v, site, 1'000'000, false, 5)) {
        block(c, site + 0x20, 2);
        const auto cheaper = guest::compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(5))) < 0;
        scale_by_fee_comparison(v, cheaper, site + 0x28, younger ? 0x17c : 0x170, younger ? 0x44 : 0x38);
        return;
    }

    block(c, site + 0x40, 4);
    c.set({{4, 0x4c0000}, {0, 5'000'000}});
    if (guest::compare(c, std::int32_t(c.r(5)), 5'000'000) < 0) {
        block(c, site + 0x50, 3);
        const auto rival = read_fee(v, v.alternative, 4);
        if (guest::compare(c, rival, 5'000'000) < 0) {
            block(c, site + 0x5c, 3);
            const auto cheaper = guest::compare(c, rival, read_fee(v, v.draft, 0)) < 0;
            scale_by_fee_comparison(v, cheaper, site + 0x68, 0x174, 0x3c);
            return;
        }
    }
    if (younger) {
        v.scale_at(site + 0x80, 0x28);
        return;
    }

    block(c, site + 0x80, 4);
    const auto rival = read_fee(v, v.alternative, 4);
    const auto cheaper = guest::compare(c, rival, read_fee(v, v.draft, 0)) < 0;
    scale_by_fee_comparison(v, cheaper, site + 0x90, 0x4c, 0x34);
}

void release_clause_weight(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037ebe4, 3);
    if (v.alternative_option(0x1d)) {
        block(c, 0x1037ebf0, 2);
        const auto age = std::int32_t(c.r(28));
        if (guest::compare(c, age, 28) > 0) {
            const auto low = both_fees_below(v, 0x1037ebf8, 1'000'000, false, 0);
            scale_by_fee_comparison(v, low, 0x1037ec18, 0x50, 0x178);
            return;
        }
        block(c, 0x1037ec30, 2);
        const auto younger = guest::compare(c, age, 24) < 0;
        comparative_clause_weight(v, younger ? 0x1037ec38 : 0x1037ecc4, younger);
        return;
    }
    if (both_fees_below(v, 0x1037ed6c, 1'000'000, false, 5)) {
        v.scale_at(0x1037ed8c, 0x34);
        return;
    }
    block(c, 0x1037ed98, 4);
    c.set(4, 5'000'000);
    if (guest::compare(c, std::int32_t(c.r(5)), 5'000'000) < 0) {
        block(c, 0x1037eda8, 3);
        if (guest::compare(c, read_fee(v, v.alternative, 0), 5'000'000) < 0) {
            v.scale_at(0x1037edb4, 0x178);
            return;
        }
    }
    v.scale_at(0x1037edc0, 0x180);
}

void weigh_clause(Valuation& v, unsigned clause) {
    auto& c = v.c;
    block(c, 0x1037eaf8, 2);
    if (guest::compare(c, std::int32_t(clause), 2) == 0) {
        const auto low = both_fees_below(v, 0x1037edcc, 5'000'000, false, 0);
        scale_by_fee_comparison(v, low, 0x1037edec, 0x184, 0x34);
        return;
    }

    block(c, 0x1037eb00, 1);
    if (clause < 2) {
        block(c, 0x1037eb04, 2);
        if (guest::compare(c, std::int32_t(clause), 0) == 0) minimum_fee_weight(v);
        else {
            block(c, 0x1037eb0c, 1);
            release_clause_weight(v);
        }
        return;
    }

    block(c, 0x1037eb14, 2);
    if (guest::compare(c, std::int32_t(clause), 4) == 0) {
        block(c, 0x1037eec4, 3);
        v.scale_double(31, 0x118);
    } else {
        block(c, 0x1037eb1c, 1);
        block(c, 0x1037eb20, 1);
        block(c, 0x1037ee04, 3);
        if (v.alternative_option(0x1f)) comparative_clause_weight(v, 0x1037ee10, false);
        else v.scale_at(0x1037eeb8, 0x50);
    }
}

void weigh_release_clauses(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037eadc, 2);
    c.set(3, 0);
    for (unsigned clause = 0;; ++clause) {
        block(c, 0x1037eed4, 3);
        c.set(0, clause);
        if (guest::compare(c, std::int32_t(clause), 5) >= 0) break;
        block(c, 0x1037eae4, 5);
        c.set({{4, clause}, {0, ContractRecord::option + clause}});
        const auto active = c.i8(v.draft + ContractRecord::option + clause);
        record_nonzero(c, active);
        if (active > 0) weigh_clause(v, clause);
        block(c, 0x1037eed0, 1);
        c.set(3, clause + 1);
    }
}

std::int8_t role_year_adjustment(Valuation& v) {
    auto& c = v.c;
    block(c, 0x1037eee0, 13);
    const auto alternative_year = c.u16(c.sp() + 0x6a);
    c.set({{5, alternative_year}, {3, high_word}});
    const auto draft_year = c.u16(v.draft + ContractRecord::end_date + 2);
    const auto expected_age = std::int8_t(c.r(27));
    c.set({{4, draft_year}, {0, std::uint32_t(expected_age)}});
    c.put32(c.sp() + 0x9c, high_word);
    guest::compare(c, expected_age, -1);
    const auto difference = int(draft_year) - alternative_year;
    c.set(0, std::uint32_t(difference));
    c.set_f(1, v.double_constant(0xc0));
    const auto encoded = std::uint32_t(difference) ^ 0x80000000;
    c.set(0, encoded);
    c.put32(c.sp() + 0xa0, encoded);
    c.set_f(0, c.f64(c.sp() + 0x9c));
    c.set_f(29, v.fp.fsubs(c.f(0), c.f(1)));
    if (expected_age != -1) {
        block(c, 0x1037ef14, 2);
        const auto delta = std::uint32_t(c.r(27) - c.r(28));
        c.set({{0, delta}, {26, std::uint32_t(std::int8_t(delta))}});
    }

    block(c, 0x1037ef1c, 2);
    // With no role category the original consumes the incoming r26.
    const auto role_delta = std::int32_t(c.r(26));
    if (guest::compare(c, role_delta, -5) < 0) {
        block(c, 0x1037ef24, 2);
        c.set(3, std::uint32_t(-5));
        return -5;
    }
    block(c, 0x1037ef2c, 3);
    const auto capped = guest::compare(c, role_delta, 5) > 0;
    c.set(3, 5);
    if (!capped) {
        block(c, 0x1037ef38, 1);
        c.set(3, std::uint32_t(role_delta));
    }
    return capped ? 5 : std::int8_t(role_delta);
}

void apply_valuation_years(Valuation& v) {
    auto& c = v.c;
    const auto age_delta = role_year_adjustment(v);
    block(c, 0x1037ef3c, 4);
    const auto minimum_fee = c.i8(v.alternative + 0x1d);
    c.set(26, std::uint32_t(age_delta));
    record_nonzero(c, minimum_fee);
    if (minimum_fee > 0) {
        block(c, 0x1037ef4c, 4);
        v.scale_double(30, 0x30);
    } else {
        block(c, 0x1037ef5c, 2);
        record_nonzero(c, age_delta);
        if (age_delta > 0) {
            block(c, 0x1037ef64, 3);
            auto rival = v.alternative_option(0x1f);
            if (!rival) {
                block(c, 0x1037ef70, 3);
                rival = v.alternative_option(0x1c);
            }
            if (rival) {
                block(c, 0x1037ef7c, 3);
                v.scale_double(30, 0x258);
            }
        }
    }

    block(c, 0x1037ef88, 2);
    record_nonzero(c, age_delta);
    if (age_delta < 0) {
        block(c, 0x1037ef90, 3);
        const auto staff_profile = c.u32(v.staff + employment);
        c.set(0, staff_profile);
        if (guest::nonzero_logical(c, staff_profile)) {
            block(c, 0x1037ef9c, 13);
            const auto encoded = std::uint32_t(age_delta) ^ 0x80000000;
            c.set({{3, encoded}, {0, high_word}});
            c.put32(c.sp() + 0xa0, encoded);
            c.set_f(2, v.double_constant(0xc0));
            c.put32(c.sp() + 0x9c, high_word);
            c.set_f(0, v.double_constant(0x228));
            c.set_f(1, c.f64(c.sp() + 0x9c));
            c.set_f(3, v.double_constant(0x108));
            c.set_f(1, v.fp.fsub(c.f(1), c.f(2)));
            c.set_f(0, v.fp.fdiv(c.f(1), c.f(0)));
            c.set_f(0, v.fp.fsub(c.f(3), c.f(0)));
            c.set_f(0, v.fp.frsp(c.f(0)));
            c.set_f(30, v.fp.fmuls(c.f(30), c.f(0)));
        }
    }

    block(c, 0x1037efd0, 3);
    c.set_f(2, std::abs(c.f(29)));
    c.set_f(1, c.f(30));
    c.call(power, 0x1037efdc);
    block(c, 0x1037efdc, 5);
    c.set(2, c.u32(c.sp() + 0x14));
    c.set_f(0, v.fp.frsp(c.f(1)));
    c.set_f(1, v.double_constant(0x28));
    v.fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) block(c, 0x1037eff0, 1);
    else {
        block(c, 0x1037eff4, 3);
        c.set_f(1, v.double_constant(0x140));
        v.fp.fcmp(0, c.f(1), c.f(0));
        if (c.cr(0) & 8) block(c, 0x1037f000, 1);
        else {
            block(c, 0x1037f004, 1);
            c.set_f(1, c.f(0));
        }
    }
    block(c, 0x1037f008, 4);
    c.set_f(0, v.single_constant(0xc0));
    c.set_f(1, v.fp.frsp(c.f(1)));
    v.fp.fcmp(0, c.f(29), c.f(0));
    if (c.cr(0) & 8) {
        block(c, 0x1037f018, 2);
        c.set_f(31, v.fp.fdivs(c.f(31), c.f(1)));
    } else {
        block(c, 0x1037f020, 1);
        c.set_f(31, v.fp.fmuls(c.f(31), c.f(1)));
    }
}

void restore_valuation_frame(Cpu& c) {
    const auto stack = c.sp() + frame_size;
    const auto caller = c.u32(stack + 8);
    c.set({{0, caller}, {1, stack}});
    for (unsigned n = 31; n >= 29; --n)
        c.s.wr<8>(guest::reg::fpr(n), c.u64(stack - 8 * (32 - n)));
    c.set_lr(caller);
    detail::transfer_saved_registers<23, true>(c, stack - 32);
}
}

void value_contract_terms(Cpu& c, std::uint32_t contract, std::uint32_t staff, std::uint32_t alternative) {
    block(c, valuation_entry, 15);
    GuestNativeScope scope(c.s, valuation_entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    for (unsigned n = 31; n >= 29; --n)
        c.put64(stack - 8 * (32 - n), c.s.r<8>(guest::reg::fpr(n)));
    detail::transfer_saved_registers<23>(c, stack - 32);
    Valuation valuation(c, contract, staff, alternative);
    c.put32(stack + 8, link);
    const auto frame = stack - frame_size;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);

    c.set({{0, frame + 0x60}, {3, frame + 0x60}});
    c.call(0x10715880, 0x1037e69c);
    block(c, 0x1037e69c, 2);
    c.set(3, frame + 0x68);
    c.call(0x10715880, 0x1037e6a4);

    block(c, 0x1037e6a4, 14);
    c.set({{0, 0}, {3, UINT32_MAX}});
    const auto has_alternative = guest::nonzero_logical(c, valuation.alternative);
    c.put32(frame + 0x3c, UINT32_MAX);
    c.set_f(30, valuation.single_constant(0x58));
    c.set(27, UINT32_MAX);
    c.put8(frame + 0x8c, 0);
    c.put32(frame + 0x90, UINT32_MAX);
    c.put16(frame + 0x7a, 0);
    c.put8(frame + 0x94, 0);
    c.put32(frame + 0x80, 0);
    c.put32(frame + 0x88, 0);
    c.put32(frame + 0x40, 0);

    if (has_alternative) copy_alternative(valuation);
    else make_alternative(valuation);
    const auto days = remaining_term(valuation);
    if (days < 7) {
        block(c, 0x1037e82c, 2);
        c.set(3, 0);
        block(c, 0x1037f030, 8);
    } else {
        add_role_bonuses(valuation);
        add_signing_value(valuation, days);
        weigh_release_clauses(valuation);
        apply_valuation_years(valuation);

        block(c, 0x1037f024, 11);
        constexpr auto converted = guest::reg::fpr(0);
        original_fctiwz(c.s, converted, guest::reg::fpr(31));
        c.put64(frame + 0x9c, c.s.r<8>(converted));
        c.set(3, c.u32(frame + 0xa0));
    }
    restore_valuation_frame(c);
}
}
