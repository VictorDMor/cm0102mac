#include "contract_manager_random.hpp"
#include <tuple>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
struct PositionValue { unsigned scale; std::uint32_t peak_age; };
PositionValue positional_value(Cpu& c, std::uint32_t attributes) {
    for (const auto& [offset, scale, peak] : {std::tuple{0x15u, 0x198u, 28u},
            {0x14u, 0x1a0u, 29u}, {0x13u, 0x1a8u, 29u}, {0x12u, 0x1b0u, 29u}}) {
        const auto proficiency = c.i8(attributes + offset); c.set(0, std::uint32_t(std::int32_t(proficiency)));
        if (compare(c, proficiency, 15) >= 0) return {scale, peak};
    }
    const auto goalkeeper = c.u8(attributes + 15); c.set(0, goalkeeper);
    if (compare(c, goalkeeper, 20) == 0) return {0x1b8, 33};
    const auto secondary = c.i8(attributes + 16); c.set(0, std::uint32_t(std::int32_t(secondary)));
    if (compare(c, secondary, 15) < 0) {
        const auto wide = c.i8(attributes + 17); c.set(0, std::uint32_t(std::int32_t(wide)));
        nonzero(c, wide);
    }
    return {0x1c0, 29};
}
double national_value(Cpu& c, guest::Fpu& fp, double amount, std::uint32_t constants, std::uint32_t fused_site) {
    const auto threshold = c.f64(constants + 0x1b8); c.set_f(0, threshold); fp.fcmp(0, amount, threshold);
    if (amount < threshold) {
        auto curved = fp.fdiv(amount, threshold); c.set_f(0, curved);
        const auto quadratic = c.f64(constants + 0x160), linear = c.f64(constants + 0x268), floor = c.f64(constants + 0x260);
        c.set_f(1, quadratic); c.set_f(2, linear); c.set_f(3, floor);
        curved = fp.fmul(quadratic, curved); c.set_f(0, curved);
        curved = fp.fmul(curved, amount); c.set_f(0, curved);
        curved = fp.multiply_add(fused_site, linear, amount, curved); c.set_f(0, curved);
        fp.fcmp(0, floor, curved);
        if (!(floor > curved)) c.set_f(3, curved);
        amount = floor > curved ? floor : curved; c.set_f(31, amount);
    } else {
        const auto reduction = c.f64(constants + 0x168); c.set_f(0, reduction);
        amount = fp.fmul(amount, reduction); c.set_f(31, amount);
    }
    return amount;
}
std::uint32_t finish_value(Cpu& c, Program& program, guest::Fpu& fp, const Frame& frame,
                            double amount, std::uint32_t constants, std::uint32_t contract, std::uint32_t ceiling_back) {
    if (nonzero_logical(c, contract)) {
        const auto capped = std::int8_t(program.call(0x10380520, ceiling_back, {contract}));
        amount = c.f(31); // Read the value restored through the guest callee's ABI.
        c.set(0, std::uint32_t(std::int32_t(capped)));
        if (nonzero(c, capped)) {
            const auto ceiling = c.u32(contract + 0x24); c.set(0, 0x43300000); c.put32(frame.at(0x38), 0x43300000);
            c.set(3, ceiling ^ 0x80000000u); c.set_f(1, c.f64(constants + 8)); c.put32(frame.at(0x3c), ceiling ^ 0x80000000u);
            c.set_f(0, c.f64(frame.at(0x38))); const auto limit = fp.fsub(c.f(0), c.f(1)); c.set_f(0, limit); fp.fcmp(0, amount, limit);
            if (!(amount < limit)) {
                c.put32(frame.at(0x44), ceiling ^ 0x80000000u); c.put32(frame.at(0x40), 0x43300000); c.set_f(0, c.f64(frame.at(0x40)));
                amount = fp.fsub(c.f(0), c.f(1)); c.set_f(31, amount);
            }
            c.put64(frame.at(0x48), fp.fctiwz(0, amount));
            return result(c, c.u32(frame.at(0x4c)));
        }
    }
    c.put64(frame.at(0x38), fp.fctiwz(0, amount));
    return result(c, c.u32(frame.at(0x3c)));
}
}

std::uint32_t base_player_value(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                                 std::uint32_t target_address, std::uint32_t adjusted_value,
                                 std::uint32_t supplied_score) {
    Frame frame(c, 0x10084be0, 0xc0, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30});
    Kept<29> person(c, person_address); const bool person_present = nonzero(c, std::int32_t(person_address));
    const auto manager_holder = c.toc(-0x7b7c); c.set(4, manager_holder);
    Kept<31> adjusted(c, adjusted_value); Kept<18> national_country(c, c.toc(-0x79fc));
    Kept<30> target(c, target_address); Kept<27> singles(c, c.toc(-0x5734));
    Kept<17> suitability(c, supplied_score); Kept<16> transfers(c, c.toc(-0x7b40));
    Kept<28> constants(c, c.toc(-0x571c)); Kept<14> salt_holder(c, c.toc(-0x7b90));
    Kept<22> cursor_holder(c, c.toc(-0x7b84)); Kept<13> today(c, c.toc(-0x7b80));
    Kept<26> employer(c, c.r(26)); Kept<23> contract(c, c.r(23)); Kept<20> attributes(c, c.r(20));
    Kept<19> peak(c, c.r(19)); Kept<25> peak_age(c, c.r(25)); Kept<24> age(c, c.r(24));
    Kept<21> age_gap(c, c.r(21)); Kept<15> reputation(c, c.r(15)); Program program(c); guest::Fpu fp(c);
    const auto global_manager = c.u32(manager_holder); c.set(6, global_manager);
    std::uint32_t primary = 0;
    if (person_present) {
        const auto count_holder = c.toc(slot::person_count); c.set(4, count_holder);
        const auto id = c.i32(person), count = c.i32(count_holder); c.set(5, std::uint32_t(id)); c.set(0, std::uint32_t(count));
        if (compare(c, id, count) < 0) {
            const auto repeated = c.u32(person); c.set(0, repeated);
            const auto indices_holder = c.toc(slot::contract_index); c.set(4, indices_holder);
            const auto indices = c.u32(indices_holder), offset = repeated * 100; c.set(4, indices); c.set(0, offset);
            const auto index = c.i32(indices + offset); c.set(0, std::uint32_t(index));
            if (compare(c, index) >= 0) {
                const auto row = std::uint32_t(index) * 100, records = c.u32(global_manager); c.set(0, row); c.set(4, records); primary = records + row;
            }
        }
    }
    c.set(5, primary); const auto current = c.toc(-0x7b78); c.set(4, current);
    const bool has_primary = nonzero_logical(c, primary); c.put32(current, primary);
    if (has_primary) {
        const auto club_id = c.u32(primary + 4); c.set(0, club_id);
        const auto clubs_holder = c.toc(-0x7be8); c.set(4, clubs_holder); c.set(0, club_id * 0x248);
        const auto clubs = c.u32(clubs_holder); c.set(4, clubs); employer = clubs + club_id * 0x248;
    } else employer = 0;
    const auto located = program.call(0x10082f90, 0x10084cc0, {manager_address, person, employer});
    attributes = c.u32(person + 0x14); contract = located;
    if (!nonzero_logical(c, attributes)) {
        if (nonzero_logical(c, employer)) return program.call(0x103fac70, 0x10085194, {c.u32(transfers), person, employer, 1});
        return result(c, 0);
    }

    const auto position = positional_value(c, attributes); auto position_scale = c.f64(constants + position.scale); c.set_f(31, position_scale); peak = position.peak_age;
    const auto stored_age = c.i16(person + 0x5c); c.set(3, extended(stored_age)); c.set(0, 0x43300000);
    peak_age = std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(peak)))); c.put32(frame.at(0x58), 0x43300000);
    age = std::uint32_t(std::int32_t(std::int8_t(stored_age))); const auto bias = c.f64(constants + 8); c.set_f(2, bias);
    age_gap = std::uint32_t(peak_age) - std::uint32_t(age); const auto fold = c.f64(constants + 0x188); c.set_f(0, fold);
    c.set(0, std::uint32_t(age_gap) ^ 0x80000000u); c.put32(frame.at(0x5c), std::uint32_t(age_gap) ^ 0x80000000u);
    c.set_f(1, c.f64(frame.at(0x58))); auto age_factor = fp.fsubs(c.f(1), bias); c.set_f(30, age_factor); fp.fcmp(0, age_factor, fold);
    if (age_factor > fold) {
        const auto half = c.f32(singles + 12); const auto intercept = c.f64(constants + 0x1c8); c.set_f(0, half); c.set_f(1, intercept);
        const auto reduction = fp.fmuls(age_factor, half); c.set_f(0, reduction);
        age_factor = fp.fsub(intercept, reduction); c.set_f(30, age_factor); age_factor = fp.frsp(age_factor); c.set_f(30, age_factor);
    }
    c.put64(frame.at(0x50), fp.fctiwz(0, age_factor)); const auto absolute = program.call(0x10775540, 0x10084de4, {c.u32(frame.at(0x54))});
    age_factor = c.f(30); // The original keeps this across the absolute-value call.
    c.set(3, absolute ^ 0x80000000u); c.set(0, 0x43300000); c.put32(frame.at(0x4c), absolute ^ 0x80000000u);
    const auto role = c.i8(attributes + 15); c.set(3, std::uint32_t(std::int32_t(role))); c.put32(frame.at(0x48), 0x43300000);
    const auto local_bias = c.f64(constants + 8); c.set_f(9, local_bias); c.set_f(0, c.f64(frame.at(0x48))); compare(c, role, 15);
    const auto thirds = c.f32(singles + 16); c.set_f(1, thirds); auto absolute_factor = fp.fsubs(c.f(0), local_bias); c.set_f(2, absolute_factor);
    const auto age_divisor = c.f64(constants + 0x1d0), one = c.f64(constants + 0x18); c.set_f(0, age_divisor); c.set_f(8, one);
    absolute_factor = fp.fmuls(age_factor, absolute_factor); c.set_f(2, absolute_factor);
    auto attribute_factor = fp.fdivs(absolute_factor, thirds); c.set_f(1, attribute_factor);
    attribute_factor = fp.fdiv(attribute_factor, age_divisor); c.set_f(0, attribute_factor);
    attribute_factor = fp.fadd(one, attribute_factor); c.set_f(0, attribute_factor); attribute_factor = fp.frsp(attribute_factor); c.set_f(0, attribute_factor);
    if (role < 15) {
        const auto first = c.i8(attributes + 0x27), second = c.i8(attributes + 0x28), third = c.i8(attributes + 0x43);
        c.set(3, std::uint32_t(std::int32_t(third))); c.set(4, std::uint32_t(std::int32_t(second))); c.set(5, std::uint32_t(std::int32_t(first)) ^ 0x80000000u);
        c.put32(frame.at(0x4c), std::uint32_t(std::int32_t(first)) ^ 0x80000000u);
        c.set(4, std::uint32_t(std::int32_t(second) - 15) ^ 0x80000000u); c.put32(frame.at(0x48), 0x43300000);
        c.set(5, std::uint32_t(std::int32_t(third)) ^ 0x80000000u); const auto fourth = c.i8(attributes + 0x36); c.set(3, std::uint32_t(std::int32_t(fourth)));
        c.set_f(1, c.f64(frame.at(0x48))); c.put32(frame.at(0x54), c.r(4));
        auto first_weight = fp.fsub(c.f(1), local_bias); c.set_f(3, first_weight); const auto two_hundred = c.f64(constants + 0x1d8); c.set_f(2, two_hundred);
        c.put32(frame.at(0x50), 0x43300000); c.set(4, std::uint32_t(std::int32_t(fourth) - 15) ^ 0x80000000u);
        const auto fifth = c.i8(attributes + 0x2f); c.set(3, std::uint32_t(std::int32_t(fifth))); c.set_f(1, c.f64(frame.at(0x50)));
        first_weight = fp.fdiv(first_weight, two_hundred); c.set_f(3, first_weight); c.put32(frame.at(0x5c), c.r(5));
        const auto divisor = c.f64(constants + 0x1e0); c.set_f(6, divisor); c.put32(frame.at(0x58), 0x43300000);
        auto second_weight = fp.fsubs(c.f(1), local_bias); c.set_f(2, second_weight); c.set_f(1, c.f64(frame.at(0x58)));
        c.set(3, std::uint32_t(std::int32_t(fifth) - 15) ^ 0x80000000u);
        first_weight = fp.fadd(one, first_weight); c.set_f(7, first_weight); c.put32(frame.at(0x44), c.r(4));
        auto third_weight = fp.fsub(c.f(1), local_bias); c.set_f(4, third_weight); c.put32(frame.at(0x40), 0x43300000);
        second_weight = fp.fdiv(second_weight, divisor); c.set_f(5, second_weight);
        const auto three_hundred = c.f64(constants + 0x148); c.set_f(3, three_hundred); c.set_f(2, c.f64(frame.at(0x40)));
        c.put32(frame.at(0x3c), c.r(3)); c.put32(frame.at(0x38), 0x43300000); c.set_f(1, c.f64(frame.at(0x38)));
        third_weight = fp.fdiv(third_weight, three_hundred); c.set_f(3, third_weight);
        auto fourth_weight = fp.fsubs(c.f(2), local_bias); c.set_f(2, fourth_weight);
        auto fifth_weight = fp.fsubs(c.f(1), local_bias); c.set_f(1, fifth_weight);
        first_weight = fp.frsp(first_weight); c.set_f(7, first_weight);
        fourth_weight = fp.fdiv(fourth_weight, divisor); c.set_f(2, fourth_weight); fifth_weight = fp.fdiv(fifth_weight, divisor); c.set_f(1, fifth_weight);
        second_weight = fp.fadd(one, second_weight); c.set_f(4, second_weight); third_weight = fp.fadd(one, third_weight); c.set_f(3, third_weight);
        fourth_weight = fp.fadd(one, fourth_weight); c.set_f(2, fourth_weight); fifth_weight = fp.fadd(one, fifth_weight); c.set_f(1, fifth_weight);
        attribute_factor = fp.fmuls(attribute_factor, first_weight); c.set_f(0, attribute_factor);
        second_weight = fp.frsp(second_weight); c.set_f(4, second_weight); third_weight = fp.frsp(third_weight); c.set_f(3, third_weight);
        fourth_weight = fp.frsp(fourth_weight); c.set_f(2, fourth_weight); attribute_factor = fp.fmuls(attribute_factor, second_weight); c.set_f(0, attribute_factor);
        fifth_weight = fp.frsp(fifth_weight); c.set_f(1, fifth_weight); attribute_factor = fp.fmuls(attribute_factor, third_weight); c.set_f(0, attribute_factor);
        attribute_factor = fp.fmuls(attribute_factor, fourth_weight); c.set_f(0, attribute_factor); attribute_factor = fp.fmuls(attribute_factor, fifth_weight); c.set_f(0, attribute_factor);
    }
    const auto ambition = c.i8(attributes + 0x2d); const auto ability = c.i16(attributes + 8);
    c.set(4, std::uint32_t(std::int32_t(ambition))); c.set(3, 0x43300000); c.put32(frame.at(0x38), 0x43300000);
    const auto ambition_square = std::uint32_t(std::int32_t(ambition) * std::int32_t(ambition)); c.set(4, ambition_square);
    c.set(0, std::uint32_t(std::int32_t(ability)) ^ 0x80000000u); const auto score_bias = c.f64(constants + 8); c.set_f(4, score_bias); c.put32(frame.at(0x44), c.r(0));
    const auto twenty = c.f64(constants + 0x1f8); c.set_f(5, twenty); c.put32(frame.at(0x40), 0x43300000); c.set(0, ambition_square ^ 0x80000000u);
    c.set_f(1, c.f64(frame.at(0x40))); c.put32(frame.at(0x3c), ambition_square ^ 0x80000000u);
    auto ability_weight = fp.fsub(c.f(1), score_bias); c.set_f(2, ability_weight); const auto ability_scale = c.f64(constants + 0x208); c.set_f(1, ability_scale);
    c.set_f(3, c.f64(frame.at(0x38))); const auto ambition_scale = c.f64(constants + 0x1f0); c.set_f(7, ambition_scale);
    auto ambition_weight = fp.fsubs(c.f(3), score_bias); c.set_f(6, ambition_weight);
    const auto ambition_intercept = c.f64(constants + 0x1e8); c.set_f(4, ambition_intercept);
    ability_weight = fp.fdiv(ability_weight, ability_scale); c.set_f(2, ability_weight); const auto ability_intercept = c.f64(constants + 0x200); c.set_f(3, ability_intercept);
    const auto multiplier_floor = c.f64(constants + 0x28); c.set_f(1, multiplier_floor); ambition_weight = fp.fdiv(ambition_weight, twenty); c.set_f(5, ambition_weight);
    ambition_weight = fp.multiply_add(0x10084f84, ambition_scale, ambition_weight, ambition_intercept); c.set_f(4, ambition_weight);
    ability_weight = fp.fadd(ability_intercept, ability_weight); c.set_f(3, ability_weight);
    ambition_weight = fp.frsp(ambition_weight); c.set_f(2, ambition_weight); fp.fcmp(0, ability_weight, multiplier_floor);
    attribute_factor = fp.fmuls(attribute_factor, ambition_weight); c.set_f(0, attribute_factor);
    if (ability_weight < multiplier_floor) { ability_weight = multiplier_floor; c.set_f(3, ability_weight); }
    auto profile = c.f64(constants + 0x90); c.set_f(30, profile); fp.fcmp(0, profile, attribute_factor);
    if (!(profile > attribute_factor)) {
        fp.fcmp(0, ability_weight, attribute_factor); profile = ability_weight < attribute_factor ? ability_weight : attribute_factor; c.set_f(30, profile);
    }
    const bool has_target = nonzero_logical(c, target); profile = fp.frsp(profile); c.set_f(30, profile);
    std::uint32_t base_score;
    if (!has_target) { base_score = extended(c.i16(attributes + 10)); c.set(0, base_score); }
    else {
        const auto assessed = program.call(0x100cdef0, 0x10084fe8, {person, c.u32(target + 0x90), 0});
        c.set(0, extended(std::int16_t(std::uint32_t(suitability)))); reputation = extended(std::int16_t(assessed));
        if (compare(c, std::int16_t(std::uint32_t(suitability)), -1) == 0)
            suitability = extended(std::int16_t(program.call(0x10219e40, 0x10085010, {c.u32(target + 0x90), person, target, 1, reputation})));
        const auto scaled = std::uint32_t(suitability) * 50; c.set(0, scaled); suitability = extended(std::int16_t(scaled));
        if (compare(c, std::int32_t(std::uint32_t(suitability))) < 0) suitability = 0;
        const auto reputation_order = compare_logical(c, reputation, 5000); const auto capped_reputation = reputation_order < 0 ? std::uint32_t(reputation) : 5000u; c.set(4, capped_reputation);
        const auto combined = std::uint32_t(suitability) + std::uint32_t(reputation); c.set(0, combined); c.set(3, 0x43300000);
        c.put32(frame.at(0x3c), capped_reputation); const auto unsigned_bias = c.f64(constants + 0x278); c.set_f(3, unsigned_bias);
        c.put32(frame.at(0x38), 0x43300000); const auto divisor = c.f32(singles + 16); c.set_f(0, divisor); c.set_f(1, c.f64(frame.at(0x38)));
        c.put32(frame.at(0x44), combined); const auto capped = fp.fsubs(c.f(1), unsigned_bias); c.set_f(2, capped); c.put32(frame.at(0x40), 0x43300000);
        c.set_f(1, c.f64(frame.at(0x40))); const auto total = fp.fsubs(c.f(1), unsigned_bias); c.set_f(1, total);
        const auto weighted = fp.multiply_add(0x10085068, profile, capped, total); c.set_f(1, weighted); c.set_f(1, fp.fdivs(weighted, divisor));
        const auto converted = program.call(0x10773780, 0x10085074); compare_logical(c, converted, 1);
        base_score = converted > 1 ? converted : 1; c.set(0, base_score);
        if (compare_logical(c, base_score, 12000) > 0) { base_score = 12000; c.set(0, base_score); }
    }
    const auto potential = c.i16(attributes + 12); c.set(5, extended(potential));
    if (compare(c, potential, 5000) > 0) {
        c.set(3, std::uint32_t(peak_age) + 4);
        if (compare(c, std::int32_t(std::uint32_t(age)), std::int32_t(std::uint32_t(peak_age) + 4)) < 0) {
            const auto half = base_score >> 1, candidate = half + std::uint32_t(std::int32_t(potential) - 2500);
            c.set(3, half); c.set(4, candidate); if (compare_logical(c, candidate, base_score) > 0) { base_score = candidate; c.set(0, base_score); }
            if (compare(c, potential, 8000) > 0) {
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(peak)))));
                if (compare(c, std::int32_t(std::uint32_t(age)), std::int8_t(std::uint32_t(peak))) <= 0) {
                    c.set(4, 0x43300000); c.set(5, extended(potential) ^ 0x80000000u); c.set(3, base_score >> 1);
                    c.put32(frame.at(0x3c), c.r(5)); c.set_f(1, c.f64(constants + 8)); c.put32(frame.at(0x38), 0x43300000);
                    const auto unsigned_bias = c.f64(constants + 0x278); c.set_f(3, unsigned_bias); c.set_f(0, c.f64(frame.at(0x38)));
                    c.put32(frame.at(0x44), base_score >> 1); const auto converted_potential = fp.fsub(c.f(0), c.f(1)); c.set_f(2, converted_potential);
                    const auto gain = c.f64(constants + 0x210), offset = c.f64(constants + 0x218); c.set_f(4, gain); c.put32(frame.at(0x40), 0x43300000); c.set_f(1, offset); c.set_f(0, c.f64(frame.at(0x40)));
                    c.put32(frame.at(0x4c), base_score); const auto converted_half = fp.fsub(c.f(0), unsigned_bias); c.set_f(0, converted_half); c.put32(frame.at(0x48), 0x43300000);
                    auto boosted = fp.multiply_add(0x10085118, gain, converted_potential, converted_half); c.set_f(2, boosted); c.set_f(0, c.f64(frame.at(0x48)));
                    const auto converted_base = fp.fsub(c.f(0), unsigned_bias); c.set_f(0, converted_base); boosted = fp.fsub(boosted, offset); c.set_f(1, boosted); fp.fcmp(0, boosted, converted_base);
                    if (!(boosted > converted_base)) { c.put32(frame.at(0x54), base_score); c.put32(frame.at(0x50), 0x43300000); c.set_f(0, c.f64(frame.at(0x50))); boosted = fp.fsub(c.f(0), unsigned_bias); c.set_f(1, boosted); }
                    base_score = program.call(0x10773780, 0x10085148); c.set(0, base_score);
                }
            }
        }
    }
    const auto score_divisor = c.f64(constants + 0x220); c.set_f(0, score_divisor); const auto square = base_score * base_score; c.set(3, square); c.set(0, 0x43300000);
    position_scale = c.f(31); // Recover the retained coefficient after the guest calls.
    const auto unsigned_bias = c.f64(constants + 0x278); c.set_f(1, unsigned_bias); const auto position_share = fp.fdiv(position_scale, score_divisor); c.set_f(2, position_share);
    c.put32(frame.at(0x38), 0x43300000); c.put32(frame.at(0x3c), square); c.set_f(0, c.f64(frame.at(0x38)));
    const auto score_square = fp.fsub(c.f(0), unsigned_bias); c.set_f(0, score_square); auto amount = fp.fmul(position_share, score_square); c.set_f(1, amount);
    const auto international = c.u8(person + 0x5e); c.set(5, international);
    if (nonzero_logical(c, international)) {
        const auto country = c.u32(person + 12); c.set(3, country);
        if (nonzero_logical(c, country)) {
            const auto factor = c.i16(country + 0xac); const auto product = std::int32_t(factor) * std::int32_t(international), high = multiply_high(0x38e38e39, product);
            c.set(3, 0x38e38e39); c.set(4, age_gap); c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 3));
            const auto rounded = std::uint32_t(high >> 2) + (std::uint32_t(high >> 2) >> 31); const auto years = std::int16_t(rounded); c.set(3, std::uint32_t(high >> 2) >> 31); c.set(0, extended(years));
            auto minimum = std::uint32_t(age_gap); if (compare(c, years, std::int32_t(std::uint32_t(age_gap))) < 0) minimum = extended(years); c.set(4, minimum);
            if (compare(c, std::int32_t(minimum)) < 0) age_gap = 0;
            else if (compare(c, years, std::int32_t(std::uint32_t(age_gap))) < 0) age_gap = extended(years);
            const auto increment = std::int8_t(std::int16_t(std::uint32_t(age_gap))); c.set(0, std::uint32_t(std::int32_t(increment))); age = std::uint32_t(age) + std::uint32_t(std::int32_t(increment));
        }
    }
    const auto delta = std::int8_t(std::uint32_t(age) - std::uint32_t(peak_age)); c.set(0, std::uint32_t(std::int32_t(delta)));
    c.s.wr<1>(guest::reg::xer_ca, 10u >= std::uint32_t(std::int32_t(delta)));
    auto bucket = std::int32_t(std::int8_t(10 - std::int32_t(delta))); c.set(5, std::uint32_t(bucket));
    if (compare(c, bucket) < 0) { bucket = 0; c.set(5, 0); }
    c.set(0, std::uint32_t(bucket)); if (compare(c, bucket, 20) >= 0) { bucket = 19; c.set(5, 19); }
    const auto hundred = c.f64(constants + 0x228); c.set_f(0, hundred); c.set(3, 0x43300000);
    const auto table = c.toc(-0x573c); c.set(4, table); c.set(0, std::uint32_t(bucket)); amount = fp.fdiv(amount, hundred); c.set_f(1, amount); c.put32(frame.at(0x38), 0x43300000);
    const auto age_percent = c.i8(table + std::uint32_t(bucket)); const bool club_present = nonzero_logical(c, target); c.set_f(2, c.f64(constants + 8));
    c.set(0, std::uint32_t(std::int32_t(age_percent)) ^ 0x80000000u); c.put32(frame.at(0x3c), c.r(0)); c.set_f(0, c.f64(frame.at(0x38)));
    const auto percentage = fp.fsub(c.f(0), c.f(2)); c.set_f(0, percentage); amount = fp.fmul(amount, percentage); c.set_f(31, amount);

    double market;
    if (club_present) {
        const auto competition = c.u32(target + 0x30); c.set(5, competition);
        if (nonzero_logical(c, competition)) {
            const auto division = c.u32(target + 0x2c); c.set(4, division);
            if (!nonzero_logical(c, division)) { market = c.f64(constants + 0x18); c.set_f(5, market); }
            else {
                const auto level = c.i8(division + 0x11c); c.set(0, std::uint32_t(std::int32_t(level))); c.put32(frame.at(0x38), 0x43300000);
                c.set_f(0, c.f64(constants + 0x18)); c.set(0, std::uint32_t(std::int32_t(level)) ^ 0x80000000u); c.put32(frame.at(0x3c), c.r(0)); c.set_f(1, c.f64(frame.at(0x38)));
                auto league = fp.fsub(c.f(1), c.f(2)); c.set_f(1, league); fp.fcmp(0, league, c.f(0));
                bool reset = league < c.f(0);
                if (!reset) { c.set_f(0, c.f64(constants + 0x100)); fp.fcmp(0, league, c.f(0)); reset = league > c.f(0); }
                if (reset) { league = c.f64(constants + 0x100); c.set_f(1, league); }
                const auto half = c.f64(constants + 0xb8); c.set_f(0, half); const auto kind = c.i8(target + 0x23d); c.set(0, std::uint32_t(std::int32_t(kind)));
                league = fp.multiply_add(0x100852d4, league, half, half); c.set_f(1, league);
                const auto kind_order = compare(c, kind, 3);
                if (kind_order == 0) { c.set_f(0, c.f64(constants + 0x230)); league = fp.fmul(league, c.f(0)); c.set_f(1, league); }
                else if (kind_order < 0 && compare(c, kind, 2) >= 0) { c.set_f(0, c.f64(constants + 0x98)); league = fp.fmul(league, c.f(0)); c.set_f(1, league); }
                const auto standing = c.i16(competition + 0x18); c.set(3, extended(standing)); c.set(0, 0x43300000); c.set_f(0, c.f64(constants + 0x238));
                const auto standing_square = std::uint32_t(std::int32_t(standing) * std::int32_t(standing)); c.set(3, standing_square); const auto club_score = c.i16(target + 0x1ec); c.set(4, extended(club_score)); c.put32(frame.at(0x38), 0x43300000);
                const auto denominator = fp.fmul(c.f(0), league); c.set_f(0, denominator); c.set_f(2, c.f64(constants + 8)); const auto score = standing_square * 25 + extended(club_score);
                c.set(0, score ^ 0x80000000u); c.put32(frame.at(0x3c), c.r(0)); c.set_f(1, c.f64(frame.at(0x38))); const auto numerator = fp.fsub(c.f(1), c.f(2)); c.set_f(1, numerator);
                market = fp.fdiv(numerator, denominator); c.set_f(5, market);
            }
        } else {
            const auto division = c.u32(target + 0x2c); c.set(4, division);
            if (!nonzero_logical(c, division)) { market = c.f64(constants + 0x18); c.set_f(5, market); }
            else {
                const auto level = c.i8(division + 0x11c); c.set(0, std::uint32_t(std::int32_t(level))); c.put32(frame.at(0x38), 0x43300000); c.set_f(0, c.f64(constants + 0x18));
                c.set(0, std::uint32_t(std::int32_t(level)) ^ 0x80000000u); c.put32(frame.at(0x3c), c.r(0)); c.set_f(1, c.f64(frame.at(0x38)));
                auto league = fp.fsub(c.f(1), c.f(2)); c.set_f(1, league); fp.fcmp(0, league, c.f(0)); bool reset = league < c.f(0);
                if (!reset) { c.set_f(0, c.f64(constants + 0x100)); fp.fcmp(0, league, c.f(0)); reset = league > c.f(0); }
                if (reset) { league = c.f64(constants + 0x100); c.set_f(1, league); }
                const auto kind = c.i8(target + 0x23d); c.set(0, std::uint32_t(std::int32_t(kind))); const auto kind_order = compare(c, kind, 3);
                if (kind_order == 0) { c.set_f(0, c.f64(constants + 0x230)); league = fp.fmul(league, c.f(0)); c.set_f(1, league); }
                else if (kind_order < 0 && compare(c, kind, 2) >= 0) { c.set_f(0, c.f64(constants + 0x98)); league = fp.fmul(league, c.f(0)); c.set_f(1, league); }
                const auto standing = c.i8(division + 0x11f); c.set(3, std::uint32_t(std::int32_t(standing))); c.set(0, 0x43300000); c.set_f(0, c.f64(constants + 0x238));
                const auto club_score = c.i16(target + 0x1ec); const auto standing_square = std::int32_t(standing) * std::int32_t(standing); c.set(4, extended(club_score)); c.set(3, std::uint32_t(standing_square)); c.put32(frame.at(0x38), 0x43300000);
                c.set_f(2, c.f64(constants + 8)); const auto denominator = fp.fmul(c.f(0), league); c.set_f(0, denominator);
                const auto score = std::uint32_t(standing_square) * 25 + extended(club_score); c.set(0, score ^ 0x80000000u); c.put32(frame.at(0x3c), c.r(0)); c.set_f(1, c.f64(frame.at(0x38)));
                const auto numerator = fp.fsub(c.f(1), c.f(2)); c.set_f(1, numerator); market = fp.fdiv(numerator, denominator); c.set_f(5, market);
            }
        }
    } else {
        const auto end_holder = c.toc(-0x7b88); c.set(3, end_holder); const auto cursor = c.u32(cursor_holder); c.set(4, cursor); const auto last = c.u32(end_holder); c.set(0, last);
        const auto next = cursor + 2; c.set(3, next); const bool wrap = compare_logical(c, next, last) > 0; c.put32(cursor_holder, next);
        if (wrap) { const auto base = c.toc(-0x7b8c); c.set(0, base); c.put32(cursor_holder, base); const auto entropy = program.call(0x1077b770, 0x1008543c); c.set(0, entropy & 0xffff); c.put32(salt_holder, entropy & 0xffff); }
        const auto position_address = c.u32(cursor_holder); c.set(5, position_address); c.set(3, 0x55550000); c.set(0, 0x43300000); const auto salt = c.u32(salt_holder); c.set(4, salt);
        const auto draw = c.u16(position_address); c.set(5, draw); c.set(3, 0x55555556); c.put32(frame.at(0x38), 0x43300000);
        const auto total = draw + salt; c.set(4, total); c.set_f(1, c.f64(constants + 8)); const auto high = multiply_high(0x55555556, std::int32_t(total));
        const auto quotient = std::uint32_t(high) + (std::uint32_t(high) >> 31), remainder = total - quotient * 3;
        c.set(3, remainder); c.set(0, (remainder + 1) ^ 0x80000000u); c.put32(frame.at(0x3c), c.r(0)); c.set_f(0, c.f64(frame.at(0x38))); market = fp.fsub(c.f(0), c.f(1)); c.set_f(5, market);
    }
    c.set_f(0, c.f64(constants + 0x1f8)); c.set_f(3, c.f64(constants + 0x188)); auto market_distance = fp.fsub(c.f(0), market); c.set_f(4, market_distance);
    const auto market_scale = c.f64(constants + 0x240); c.set_f(1, market_scale); c.set_f(2, c.f64(constants + 0x18)); c.set_f(0, c.f64(constants + 0xc0));
    market_distance = fp.fmul(market_distance, market_distance); c.set_f(4, market_distance); market_distance = fp.fdiv(market_distance, c.f(3)); c.set_f(3, market_distance);
    auto market_weight = fp.fdiv(market_distance, market_scale); c.set_f(1, market_weight); market_weight = fp.fsub(c.f(2), market_weight); c.set_f(1, market_weight); fp.fcmp(0, market_weight, c.f(0));
    if (market_weight < c.f(0)) { market_weight = c.f(0); c.set_f(1, market_weight); }
    c.set_f(0, c.f64(constants + 0x28)); fp.fcmp(0, market_weight, c.f(0)); if (market_weight > c.f(0)) { market_weight = c.f(0); c.set_f(1, market_weight); }
    const auto market_square = fp.fmul(market, market); c.set_f(2, market_square); const auto compression_threshold = c.f64(constants + 0x258); c.set_f(0, compression_threshold);
    amount = fp.fmul(amount, market_weight); c.set_f(31, amount); const auto threshold = fp.fmul(compression_threshold, market_square); c.set_f(0, threshold); fp.fcmp(0, amount, threshold);
    if (amount > threshold) {
        const auto fixed = c.f64(constants + 0x250), share = c.f64(constants + 0x248); c.set_f(0, fixed); c.set_f(1, share);
        const auto offset = fp.fmul(fixed, market_square); c.set_f(0, offset); amount = fp.multiply_add(0x100854f8, amount, share, offset); c.set_f(31, amount);
    }
    const auto minimum = c.f64(constants + 0x260); c.set_f(0, minimum); const auto floor = fp.fmul(minimum, market); c.set_f(0, floor); fp.fcmp(0, amount, floor);
    if (amount < floor) { amount = floor; c.set_f(31, amount); }
    const auto mode = std::int8_t(std::uint32_t(adjusted)); c.set(0, std::uint32_t(std::int32_t(mode)));
    if (nonzero(c, mode)) {
        if (nonzero_logical(c, target)) {
            const auto division = c.u32(target + 0x2c); c.set(3, division);
            if (nonzero_logical(c, division)) {
                const auto country = c.i32(division + 0x68), watched = c.i32(national_country); c.set(3, std::uint32_t(country)); c.set(0, std::uint32_t(watched));
                if (compare(c, country, watched) == 0) amount = national_value(c, fp, amount, constants, 0x10085560);
            }
        }
        return finish_value(c, program, fp, frame, amount, constants, contract, 0x10085594);
    }
    if (nonzero_logical(c, contract)) {
        const auto expired_policy = std::int8_t(program.call(0x103faff0, 0x1008560c, {c.u32(transfers), person})); c.set(0, std::uint32_t(std::int32_t(expired_policy)));
        amount = c.f(31); // A guest callee restores the retained monetary value.
        if (nonzero(c, expired_policy)) {
            const auto current_year = c.u16(today + 2), expiry_year = c.u16(contract + 0x32); c.set(4, current_year); c.set(0, expiry_year);
            const auto current_day = c.i16(today), expiry_day = c.i16(contract + 0x30); c.set(3, extended(current_day));
            auto days = std::int32_t(expiry_year) * 365 - std::int32_t(current_year) * 365 + std::int32_t(expiry_day) - current_day; c.set(4, std::uint32_t(days)); c.set(0, std::uint32_t(std::int32_t(expiry_day) - current_day));
            if (compare(c, days, 1) < 0) return result(c, 0);
            if (compare_logical(c, target, employer) == 0) {
                const auto mood_byte = c.u8(contract + 0x44); const auto mood = std::int8_t(mood_byte); c.set(3, mood_byte); c.set(0, std::uint32_t(std::int32_t(mood)));
                if (compare(c, mood) > 0) {
                    const auto flags = c.u8(contract + 0x38); const auto role_kind = std::int32_t(std::int8_t(flags)) >> 2; c.set(0, std::uint32_t(role_kind)); c.s.wr<1>(guest::reg::xer_ca, role_kind < 0 && (flags & 2));
                    if (compare(c, role_kind, 2) == 0) { days += mood * 7 + 200; c.set(0, std::uint32_t(mood * 7)); c.set(4, std::uint32_t(days)); }
                    else { const auto reduction = c.f64(constants + 0xb8); c.set_f(0, reduction); amount = fp.fmul(reduction, amount); c.set_f(31, amount); }
                }
            }
            const bool short_term = compare(c, days, 350) < 0; c.set(3, std::uint32_t(days) ^ 0x80000000u); c.set(0, 0x43300000); c.put32(frame.at(0x3c), c.r(3)); c.set_f(2, c.f64(constants + 8)); c.put32(frame.at(0x38), 0x43300000);
            if (short_term) {
                const auto horizon = c.f64(constants + 0x270); c.set_f(0, horizon); c.set_f(1, c.f64(frame.at(0x38)));
                const auto elapsed = fp.fsubs(c.f(1), c.f(2)); c.set_f(1, elapsed); auto scale = fp.fdiv(elapsed, horizon); c.set_f(0, scale); scale = fp.frsp(scale); c.set_f(0, scale); amount = fp.fmul(amount, scale); c.set_f(31, amount);
            } else {
                const auto horizon = c.f64(constants + 0x270); c.set_f(1, horizon); c.set_f(0, c.f64(frame.at(0x38))); const auto top = c.f64(constants + 0xf0); c.set_f(3, top);
                const auto elapsed = fp.fsubs(c.f(0), c.f(2)); c.set_f(2, elapsed); c.set_f(0, c.f64(constants + 0x18)); auto scale = fp.fdiv(elapsed, horizon); c.set_f(1, scale);
                scale = fp.fsub(top, scale); c.set_f(1, scale); scale = fp.frsp(scale); c.set_f(1, scale); fp.fcmp(0, scale, c.f(0));
                if (scale < c.f(0)) { scale = c.f32(singles + 20); c.set_f(1, scale); }
                c.set_f(0, c.f64(constants + 0x98)); fp.fcmp(0, scale, c.f(0)); if (scale > c.f(0)) { scale = c.f32(singles + 24); c.set_f(1, scale); }
                amount = fp.fdiv(amount, scale); c.set_f(31, amount);
            }
        }
    } else { amount = c.f64(constants + 0x60); c.set_f(31, amount); }
    const auto zero = c.f64(constants + 0x60); c.set_f(0, zero); fp.fcmp(0, amount, zero); bool national = false;
    if (amount != zero && nonzero_logical(c, target)) {
        const auto division = c.u32(target + 0x2c); c.set(3, division);
        if (nonzero_logical(c, division)) {
            const auto country = c.i32(division + 0x68), watched = c.i32(national_country); c.set(3, std::uint32_t(country)); c.set(0, std::uint32_t(watched)); national = compare(c, country, watched) == 0;
        }
    }
    if (national) amount = national_value(c, fp, amount, constants, 0x10085774);
    else if (nonzero_logical(c, target)) {
        const auto division = c.u32(target + 0x2c); c.set(4, division);
        if (nonzero_logical(c, division)) {
            const auto alternate = c.toc(-0x7a00); c.set(3, alternate); const auto country = c.i32(division + 0x68), watched = c.i32(alternate); c.set(4, std::uint32_t(country)); c.set(0, std::uint32_t(watched));
            if (compare(c, country, watched) == 0) { const auto reduction = c.f64(constants + 0x170); c.set_f(0, reduction); amount = fp.fmul(amount, reduction); c.set_f(31, amount); }
        }
    }
    return finish_value(c, program, fp, frame, amount, constants, contract, 0x100857dc);
}
}
