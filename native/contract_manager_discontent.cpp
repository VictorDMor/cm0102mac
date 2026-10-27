#include "contract_manager_random.hpp"
#include <algorithm>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
// Each of the three inlined samplers takes a signed byte, optionally +5.
// Those bounds are in [-128,132], so the source's wide recursive arms
// cannot run. Keep the two range comparisons and their ABI residue.
std::uint32_t sample_review(Cpu& c, const RandomTable& random, std::int32_t bound, std::uint32_t back) {
    random.advance(back);
    c.set(3, 0x10000); c.set(0, 0xffff); compare(c, bound, 0xffff);
    c.set(3, 0xffff0000); c.set(0, 0xffff0001); compare(c, bound, -0xffff);
    const auto position = random.position(); c.set(3, position);
    const bool positive = compare(c, bound) > 0;
    const auto salt = c.u32(random.salt); c.set(0, salt);
    const auto draw = std::uint32_t(c.u16(position)) + salt; c.set(3, draw);
    const auto quotient = std::uint32_t(divide_signed<4>(draw, std::uint32_t(bound)));
    const auto remainder = draw - quotient * std::uint32_t(bound); c.set(0, remainder);
    return positive ? remainder : 0u - remainder;
}

std::uint32_t current_employer(Cpu& c, Program& program, std::uint32_t person,
                                std::uint32_t managers, std::uint32_t scratch,
                                std::uint32_t clubs, std::uint32_t back) {
    const auto contract = program.call(0x10082f20, back, {c.u32(managers), person});
    const bool present = nonzero_logical(c, contract); c.put32(scratch, contract);
    if (!present) return result(c, 0);
    const auto id = c.u32(contract + 4); c.set(0, id);
    const auto base = c.u32(clubs); c.set(3, base); c.set(0, id * 0x248);
    return result(c, base + id * 0x248);
}
}

void review_contract_discontent(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                                 std::uint32_t club_address, std::uint32_t contract_address) {
    Frame frame(c, 0x1007d800, 0xa0, {.first_gpr = 14});
    Kept<31> contract(c, contract_address); const bool has_contract = nonzero(c, std::int32_t(contract_address));
    Kept<26> windows(c, c.toc(-0x7b44)); const auto modifiers = c.toc(-0x5734); c.set(6, modifiers);
    Kept<17> object(c, manager_address); Kept<27> managers(c, c.toc(-0x7b7c));
    Kept<30> person(c, person_address); Kept<28> scratch(c, c.toc(-0x7b78));
    Kept<22> club(c, club_address); Kept<29> clubs(c, c.toc(-0x7be8));
    Kept<19> constants(c, c.toc(-0x571c)); Kept<20> salt(c, c.toc(-0x7b90));
    Kept<15> table(c, c.toc(-0x7b8c)); Kept<14> end(c, c.toc(-0x7b88));
    Kept<21> cursor(c, c.toc(-0x7b84)); Kept<16> today(c, c.toc(-0x7b80));
    Kept<18> age(c, c.r(18)); Kept<23> nation(c, c.r(23));
    Kept<24> decline(c, c.r(24)); Kept<25> today_year(c, c.r(25)); Program program(c);
    if (!has_contract || !nonzero_logical(c, club) || !nonzero_logical(c, person)) return;
    const auto manager = c.u32(club + 0x90); c.set(0, manager);
    if (compare_logical(c, person, manager) == 0) return;
    const auto assistant = c.u32(club + 0x94); c.set(0, assistant);
    if (compare_logical(c, person, assistant) == 0) return;
    const auto role = c.u8(person + 0x62); c.set(0, role);
    if (compare(c, role, 11) != 0) return;
    today_year = today + 2;
    const auto raw_age = c.i16(person + 0x5c); c.set(4, extended(raw_age));
    const auto joined_year = c.u16(person + 0x4e), year = c.u16(today + 2);
    c.set(3, joined_year); c.set(0, year); age = std::uint32_t(std::int32_t(std::int8_t(raw_age)));
    const auto joined_day = c.i16(person + 0x4c), day = c.i16(today);
    c.set(4, extended(joined_day)); c.set(3, extended(day));
    const auto years_since = std::uint32_t(year) - joined_year;
    const auto days_since = std::int32_t(years_since * 365 + std::uint32_t(std::int32_t(day) - joined_day)); c.set(5, std::uint32_t(days_since));
    const auto since_order = compare(c, days_since);
    std::int32_t tenure = 0;
    if (since_order >= 0) { const auto order = compare(c, days_since, 400); tenure = order > 0 ? 400 : days_since; }
    c.set(4, std::uint32_t(tenure));
    const auto expiry_year = c.u16(contract + 0x32);
    const auto expiry_day = c.i16(contract + 0x30);
    c.set(7, expiry_year); c.set(5, extended(expiry_day));
    const auto years_expired = std::uint32_t(year) - expiry_year; c.set(0, years_expired);
    const auto expired_days = std::int32_t(years_expired * 365 + std::uint32_t(std::int32_t(day) - expiry_day));
    c.set(7, std::uint32_t(expired_days)); c.set(0, std::uint32_t(std::int32_t(day) - expiry_day));
    const auto expiry_order = compare(c, expired_days);
    std::int32_t expired = 0;
    if (expiry_order >= 0) { const auto order = compare(c, expired_days, 400); expired = order > 0 ? 400 : expired_days; }
    c.set(9, std::uint32_t(expired));
    const auto attributes = c.u32(person + 0x14); c.set(3, attributes); c.set(0, 0x43300000);
    c.set(8, std::uint32_t(age) ^ 0x80000000u);
    const auto reputation = c.i16(club + 0x1ec), ability = c.i16(attributes + 10);
    c.set(7, extended(reputation)); c.set(5, extended(ability)); c.set(3, std::uint32_t(tenure) * 50);
    c.put32(frame.at(0x4c), std::uint32_t(age) ^ 0x80000000u); compare(c, expired, 400);
    const auto relative_ability = std::int32_t(ability) - reputation; c.set(4, std::uint32_t(relative_ability) ^ 0x80000000u);
    guest::Fpu fpu(c);
    const auto bias = c.f64(constants + 8), age_weight = c.f64(constants + 0x70);
    c.set_f(4, bias); c.set_f(5, age_weight); c.put32(frame.at(0x48), 0x43300000);
    c.set(3, std::uint32_t(tenure * 50) ^ 0x80000000u);
    const auto converted_age = fpu.fsub(c.f64(frame.at(0x48)), bias); c.set_f(3, converted_age);
    c.put32(frame.at(0x44), std::uint32_t(relative_ability) ^ 0x80000000u);
    const auto baseline = c.f64(constants + 0xa0); c.set_f(0, baseline); c.put32(frame.at(0x40), 0x43300000);
    const auto ability_encoded = c.f64(frame.at(0x40)); c.set_f(1, ability_encoded);
    c.put32(frame.at(0x3c), std::uint32_t(tenure * 50) ^ 0x80000000u);
    const auto converted_ability = fpu.fsub(ability_encoded, bias); c.set_f(2, converted_ability); c.put32(frame.at(0x38), 0x43300000);
    const auto tenure_encoded = c.f64(frame.at(0x38)); c.set_f(1, tenure_encoded);
    const auto weighted = fpu.multiply_add(0x1007d968, age_weight, converted_age, converted_ability); c.set_f(2, weighted);
    const auto converted_tenure = fpu.fsub(tenure_encoded, bias);
    const auto accumulated = fpu.fadd(weighted, converted_tenure); c.set_f(1, accumulated);
    double discontent = fpu.frsp(fpu.fsub(accumulated, baseline)); c.set_f(5, discontent);
    // expired is clamped to [0,400], so the source's >400 modifier is dead.
    const auto appearances = c.u8(person + 0x5e); c.set(4, appearances);
    bool appearance_bonus = false;
    if (nonzero_logical(c, appearances)) {
        const auto history = c.u32(person + 12); c.set(3, history);
        const auto standing = c.i16(history + 0xac); c.set(0, extended(standing));
        if (compare(c, standing, 3500) > 0) appearance_bonus = compare(c, std::int32_t(std::uint32_t(age)), 32) < 0;
    }
    if (appearance_bonus) {
        const auto scaled_appearances = std::uint32_t(appearances) * 25;
        const auto adaptability = c.i8(person + 0x63); c.set(0, std::uint32_t(std::int32_t(adaptability)) * 250);
        c.set(3, 0x43300000); c.set(4, scaled_appearances ^ 0x80000000u); c.put32(frame.at(0x38), 0x43300000);
        const auto increment = c.f32(modifiers + 8); c.set_f(0, increment);
        const auto local_bias = c.f64(constants + 8); c.set_f(3, local_bias);
        c.put32(frame.at(0x3c), scaled_appearances ^ 0x80000000u);
        auto adjusted = fpu.fadds(increment, discontent); c.set_f(2, adjusted);
        const auto maximum = c.f32(modifiers); c.set_f(0, maximum);
        const auto attendance = fpu.fsubs(c.f64(frame.at(0x38)), local_bias); c.set_f(1, attendance);
        c.put32(frame.at(0x44), std::uint32_t(std::int32_t(adaptability) * 250) ^ 0x80000000u); c.put32(frame.at(0x40), 0x43300000);
        adjusted = fpu.fadds(adjusted, attendance); c.set_f(2, adjusted);
        const auto adapted = fpu.fsubs(c.f64(frame.at(0x40)), local_bias);
        auto candidate = fpu.fadds(adjusted, adapted); c.set_f(1, candidate); fpu.fcmp(0, candidate, maximum);
        if (!(candidate < maximum)) candidate = maximum;
        discontent = candidate; c.set_f(1, candidate); c.set_f(5, discontent);
    } else {
        const auto squad_role = c.u8(contract + 0x60) & 15u; c.set(0, squad_role);
        const auto order = compare(c, squad_role, 2);
        bool modifier = order == 0;
        if (order < 0) modifier = compare(c, squad_role, 1) >= 0;
        else if (order > 0) modifier = compare(c, squad_role, 4) < 0;
        if (modifier) {
            double candidate;
            if (squad_role == 3) {
                const auto raw = c.u8(contract + 0x45); c.set(3, raw); c.set(0, 0x43300000); c.put32(frame.at(0x38), 0x43300000);
                const auto score = std::int32_t(std::int8_t(raw)) * 1000;
                c.set(0, std::uint32_t(score) ^ 0x80000000u); const auto local_bias = c.f64(constants + 8); c.set_f(2, local_bias);
                const auto maximum = c.f32(modifiers); c.set_f(0, maximum);
                c.put32(frame.at(0x3c), std::uint32_t(score) ^ 0x80000000u);
                const auto converted = fpu.fsubs(c.f64(frame.at(0x38)), local_bias);
                candidate = fpu.fadds(discontent, converted); c.set_f(1, candidate); fpu.fcmp(0, candidate, maximum);
                if (!(candidate < maximum)) candidate = maximum;
            } else {
                const auto adaptability = c.i8(person + 0x63); c.set(3, std::uint32_t(std::int32_t(adaptability))); c.set(0, 0x43300000);
                c.put32(frame.at(0x38), 0x43300000);
                const auto score = std::int32_t(adaptability) * (squad_role == 1 ? 250 : 125);
                const auto increment = c.f32(modifiers + (squad_role == 1 ? 4 : 8)); c.set_f(0, increment);
                const auto local_bias = c.f64(constants + 8); c.set_f(2, local_bias);
                const auto subtotal = fpu.fadds(increment, discontent); c.set_f(3, subtotal);
                const auto maximum = c.f32(modifiers); c.set_f(0, maximum);
                c.set(0, std::uint32_t(score) ^ 0x80000000u); c.put32(frame.at(0x3c), std::uint32_t(score) ^ 0x80000000u);
                const auto converted = fpu.fsubs(c.f64(frame.at(0x38)), local_bias);
                candidate = fpu.fadds(subtotal, converted); c.set_f(1, candidate); fpu.fcmp(0, candidate, maximum);
                if (!(candidate < maximum)) candidate = maximum;
            }
            discontent = candidate; c.set_f(1, candidate); c.set_f(5, discontent);
        }
    }
    const auto zero = c.f64(constants + 0x60); c.set_f(0, zero); fpu.fcmp(0, discontent, zero);
    double pressure;
    if (discontent < zero) {
        const auto divisor = c.f64(constants + 0xa8), base = c.f64(constants + 0x18); c.set_f(0, divisor); c.set_f(1, base);
        const auto share = fpu.fdiv(discontent, divisor); c.set_f(0, share); pressure = fpu.frsp(fpu.fsub(base, share));
    } else {
        const auto slope = c.f64(constants + 0xb0), base = c.f64(constants + 0x18); c.set_f(1, slope); c.set_f(0, base);
        pressure = fpu.frsp(fpu.multiply_add(0x1007dba4, slope, discontent, base));
    }
    c.set_f(4, pressure);
    const auto mood = c.i8(contract + 0x45); c.set(3, std::uint32_t(std::int32_t(mood))); c.set(0, 0x43300000);
    c.put32(frame.at(0x38), 0x43300000); const auto lower = c.f64(constants + 0xc0); c.set_f(0, lower);
    const auto numerator_bias = c.f64(constants + 8); c.set_f(2, numerator_bias); c.set(0, std::uint32_t(std::int32_t(mood) + 4) ^ 0x80000000u);
    fpu.fcmp(0, lower, pressure); c.put32(frame.at(0x3c), std::uint32_t(std::int32_t(mood) + 4) ^ 0x80000000u);
    const auto rounding = c.f64(constants + 0xb8); c.set_f(3, rounding);
    const auto numerator = fpu.fsubs(c.f64(frame.at(0x38)), numerator_bias); c.set_f(1, numerator);
    double bounded = lower;
    if (!(lower > pressure)) {
        const auto upper = c.f64(constants + 0xc8); c.set_f(0, upper); fpu.fcmp(0, upper, pressure); bounded = !(upper < pressure) ? pressure : upper;
    }
    const auto divisor = fpu.frsp(bounded);
    const auto ratio = fpu.fdivs(numerator, divisor);
    const auto integer = fpu.fctiwz(0, fpu.fadd(rounding, ratio)); c.put64(frame.at(0x40), integer);
    decline = c.u32(frame.at(0x44));
    const auto unavailable = std::int8_t(program.call(0x10187680, 0x1007dc24, {c.u32(c.toc(-0x7ba0)), person}));
    c.set(0, std::uint32_t(std::int32_t(unavailable))); bool blocked = nonzero(c, unavailable);
    if (!blocked) {
        const auto secondary = program.call(0x10083110, 0x1007dc38, {c.u32(managers), person}); blocked = nonzero_logical(c, secondary);
    }
    if (!blocked) {
        const auto news = std::int8_t(program.call(0x102de3c0, 0x1007dc50, {c.u32(c.toc(-0x7b28)), person}));
        c.set(0, std::uint32_t(std::int32_t(news))); blocked = nonzero(c, news);
    }
    if (!blocked && compare(c, std::int32_t(std::uint32_t(age)), 18) >= 0) {
        const auto locked = c.i8(contract + 0x54); c.set(0, std::uint32_t(std::int32_t(locked))); blocked = nonzero(c, locked);
    } else blocked = true;

    auto mark_request = [&](std::uint32_t flag, std::uint32_t clear_back, std::uint32_t set_back) {
        program.call(0x1037f7d0, clear_back, {contract, 4});
        program.call(0x1037fa80, set_back, {contract, flag, 0});
    };
    auto in_window = [&](std::uint32_t first_back, std::uint32_t repeated_back, std::uint32_t nation_back) {
        const auto employer = current_employer(c, program, person, managers, scratch, clubs, first_back);
        const auto competition = c.u32(employer + 0x2c); c.set(0, competition);
        if (nonzero_logical(c, competition)) {
            const auto repeated = current_employer(c, program, person, managers, scratch, clubs, repeated_back);
            nation = program.call(0x10191f80, nation_back, {c.u32(repeated + 0x2c)});
        }
        const auto id = std::int8_t(std::uint32_t(nation)); c.set(0, std::uint32_t(std::int32_t(id)));
        if (compare(c, id) < 0) return false;
        if (compare(c, id, 34) >= 0) return false;
        const auto offset = std::uint32_t(std::int32_t(id)) * 76; c.set(5, offset);
        const auto current_day = c.i16(today); c.set(6, extended(current_day)); c.set(4, windows + offset);
        const auto first_day = c.i16(windows + offset + 0x2c); c.set(0, extended(first_day));
        bool after = false;
        if (compare(c, current_day, first_day) > 0) {
            const auto current_year = c.u16(today_year), first_year = c.u16(windows + offset + 0x2e);
            c.set(3, current_year); c.set(0, first_year); after = compare_logical(c, current_year, first_year) == 0;
        }
        if (!after) {
            c.set(3, windows + offset); const auto current_year = c.u16(today_year), first_year = c.u16(windows + offset + 0x2e);
            c.set(4, current_year); c.set(0, first_year); after = compare_logical(c, current_year, first_year) > 0;
        }
        if (!after) return false;
        c.set(4, windows + offset); const auto last_day = c.i16(windows + offset + 0x3c); c.set(0, extended(last_day));
        bool before = false;
        if (compare(c, current_day, last_day) < 0) {
            const auto current_year = c.u16(today_year), last_year = c.u16(windows + offset + 0x3e);
            c.set(3, current_year); c.set(0, last_year); before = compare_logical(c, current_year, last_year) == 0;
        }
        if (!before) {
            c.set(3, windows + offset); const auto current_year = c.u16(today_year), last_year = c.u16(windows + offset + 0x3e);
            c.set(4, current_year); c.set(0, last_year); before = compare_logical(c, current_year, last_year) < 0;
        }
        return before;
    };

    if (blocked) {
        const auto first = c.u32(contract + 0x50) & 4u; c.set(0, first);
        if (nonzero(c, std::int32_t(first))) program.call(0x1037f7d0, 0x1007dc84, {contract, 4});
        const auto second = c.u32(contract + 0x50) & 16u; c.set(0, second);
        if (nonzero(c, std::int32_t(second))) program.call(0x1037f7d0, 0x1007dc9c, {contract, 16});
    } else {
        const auto delta = std::int8_t(std::uint32_t(decline)); c.set(4, std::uint32_t(std::int32_t(delta)));
        if (compare(c, delta, 5) > 0) {
            const auto interest = c.i8(contract + 0x4c), loyalty = c.i8(person + 0x65);
            c.set(3, std::uint32_t(std::int32_t(interest) - delta)); c.set(0, std::uint32_t(-30 - std::int32_t(loyalty)));
            c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(std::int32_t(loyalty)) <= std::uint32_t(-30));
            if (compare(c, std::int32_t(interest) - delta, -30 - std::int32_t(loyalty)) < 0) {
                if (compare(c, std::int32_t(std::uint32_t(age)), 21) > 0) {
                    const auto current_attributes = c.u32(person + 0x14); c.set(3, current_attributes);
                    const auto club_reputation = c.i16(club + 0x1ec), current_ability = c.i16(current_attributes + 10);
                    c.set(0, extended(club_reputation)); c.set(3, extended(current_ability));
                    bool strong = compare(c, current_ability, club_reputation) > 0;
                    if (strong) strong = compare(c, current_ability, 2250) > 0;
                    if (strong) {
                        if (in_window(0x1007dcfc, 0x1007dd38, 0x1007dd64)) mark_request(4, 0x1007ddf8, 0x1007de08);
                    } else {
                        bool loyal = false;
                        if (nonzero_logical(c, club)) {
                            const auto relation = std::int8_t(program.call(0x1007e820, 0x1007de24, {object, person, club}));
                            c.set(0, std::uint32_t(std::int32_t(relation))); loyal = compare(c, relation, 1) == 0;
                        }
                        if (loyal) {
                            const auto attributes_now = c.u32(person + 0x14); c.set(3, attributes_now);
                            const auto skill = c.i16(attributes_now + 10); c.set(0, extended(skill));
                            loyal = compare(c, skill, 1250) > 0;
                            if (loyal) loyal = compare(c, std::int32_t(std::uint32_t(age)), 23) > 0;
                        }
                        bool requested = false;
                        if (loyal) {
                            const auto loyalty_now = c.i8(person + 0x65); c.set(3, std::uint32_t(std::int32_t(loyalty_now)));
                            object = std::uint32_t(std::int32_t(loyalty_now) + 5);
                            c.s.wr<1>(guest::reg::xer_ca, (std::uint64_t(std::uint32_t(std::int32_t(loyalty_now))) + 5) >> 32);
                            if (nonzero(c, std::int32_t(std::uint32_t(object)))) age = sample_review(c, {c, salt, table, end, cursor}, std::int32_t(std::uint32_t(object)), 0x1007de80);
                            else age = 0;
                            requested = compare(c, std::int32_t(std::uint32_t(age))) == 0;
                            if (requested) mark_request(16, 0x1007dfbc, 0x1007dfcc);
                        }
                        if (!requested) {
                            const auto attributes_now = c.u32(person + 0x14); c.set(4, attributes_now);
                            const auto club_reputation_now = c.i16(club + 0x1ec); c.set(3, extended(club_reputation_now));
                            const auto potential = c.i16(attributes_now + 12); c.set(4, extended(potential)); c.set(0, std::uint32_t(std::int32_t(club_reputation_now) - 1500));
                            bool ambitious = compare(c, potential, std::int32_t(club_reputation_now) - 1500) > 0;
                            if (ambitious) ambitious = compare(c, potential, 500) > 0;
                            if (ambitious) {
                                object = std::uint32_t(std::int32_t(c.i8(person + 0x63)));
                                if (nonzero(c, std::int32_t(std::uint32_t(object)))) end = sample_review(c, {c, salt, table, end, cursor}, std::int32_t(std::uint32_t(object)), 0x1007e024);
                                else end = 0;
                                if (nonzero(c, std::int32_t(std::uint32_t(end)))) {
                                    const auto score = c.i8(contract + 0x44); c.set(0, std::uint32_t(std::int32_t(score)));
                                    if (compare(c, score) < 0 && in_window(0x1007e16c, 0x1007e1a8, 0x1007e1d4)) mark_request(4, 0x1007e268, 0x1007e278);
                                }
                            }
                        }
                    }
                } else if (nonzero_logical(c, club)) {
                    const auto relation = std::int8_t(program.call(0x1007e820, 0x1007e294, {object, person, club}));
                    c.set(0, std::uint32_t(std::int32_t(relation)));
                    if (compare(c, relation, 1) == 0) {
                        today = std::uint32_t(std::int32_t(c.i8(person + 0x63)));
                        if (nonzero(c, std::int32_t(std::uint32_t(today)))) end = sample_review(c, {c, salt, table, end, cursor}, std::int32_t(std::uint32_t(today)), 0x1007e2d4);
                        else end = 0;
                        if (compare(c, std::int32_t(std::uint32_t(end)), 5) > 0) {
                            const auto attributes_now = c.u32(person + 0x14); c.set(3, attributes_now);
                            const auto club_reputation_now = c.i16(club + 0x1ec), skill = c.i16(attributes_now + 10);
                            c.set(0, extended(club_reputation_now)); c.set(3, extended(skill));
                            if (compare(c, skill, club_reputation_now) > 0 && compare(c, skill, 1250) > 0) mark_request(16, 0x1007e42c, 0x1007e43c);
                        }
                    }
                }
            }
        }
    }
    const auto delta = std::int8_t(0u - std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(decline)))));
    c.set(0, std::uint32_t(std::int32_t(delta)));
    const auto old = c.u8(contract + 0x4c); c.set(4, old);
    const auto updated = std::int32_t(std::int8_t(old)) + delta; c.set(3, std::uint32_t(updated));
    if (compare(c, updated, 100) > 0) { c.set(0, 100); c.put8(contract + 0x4c, 100); }
    else if (compare(c, updated, -100) < 0) { c.set(0, 0xffffff9c); c.put8(contract + 0x4c, 0xffffff9c); }
    else { c.set(0, std::uint32_t(old) + std::uint32_t(std::int32_t(delta))); c.put8(contract + 0x4c, std::uint32_t(old) + std::uint32_t(std::int32_t(delta))); }
}
}
