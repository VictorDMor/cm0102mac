#include "contract_manager_random.hpp"
#include <utility>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
bool preparation_byte(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value); c.set(0, std::uint32_t(std::int32_t(byte)));
    return nonzero(c, byte);
}
double encoded_amount(Cpu& c, const Frame& frame, unsigned slot, std::uint32_t value) {
    c.put32(frame.at(slot), 0x43300000); c.put32(frame.at(slot + 4), value ^ 0x80000000u);
    return c.f64(frame.at(slot));
}
std::uint32_t truncated_amount(Cpu& c, guest::Fpu& fp, const Frame& frame,
                                unsigned destination, double value, unsigned slot) {
    c.put64(frame.at(slot), fp.fctiwz(destination, value));
    return c.u32(frame.at(slot + 4));
}
void prepared_date(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    const auto year = c.u16(source + 2); c.set(0, year); c.put16(destination + 2, year);
    const auto day = c.i16(source); c.set(0, extended(day)); c.put16(destination, extended(day));
    const auto leap = c.u8(source + 4); c.set(0, leap); c.put8(destination + 4, leap);
}
}

std::uint32_t prepare_contract_offer(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                                     std::uint32_t club_address, std::uint32_t output_address,
                                     std::uint32_t origin_value, std::uint32_t mode_value, double scale) {
    Frame frame(c, 0x10080d30, 0x130, {.first_gpr = 19, .gpr_offset = -0x54, .first_fpr = 29});
    c.set_f(30, scale);
    Kept<27> object(c, manager_address); Kept<22> injuries(c, c.toc(-0x7ba0));
    Kept<28> person(c, person_address); Kept<23> national_or_total(c, c.toc(-0x7ab8));
    Kept<20> club(c, club_address); Kept<26> context_or_contract(c, c.toc(-0x7b40));
    Kept<29> output(c, output_address); Kept<19> role_or_scratch(c, c.toc(-0x7b78));
    Kept<30> origin(c, origin_value); Kept<24> constants(c, c.toc(-0x571c));
    Kept<31> mode(c, mode_value); Kept<25> today(c, c.toc(-0x7b80));
    Kept<21> offer_or_weeks(c, c.r(21)); Program program(c); guest::Fpu fp(c);
    const auto proposal = frame.at(0x4c);
    program.call(0x103721f0, 0x10080d88, {proposal});
    const bool specified_club = nonzero_logical(c, club); offer_or_weeks = 0;
    if (!specified_club) {
        const auto contract = program.call(0x10082f20, 0x10080da4, {c.u32(c.toc(-0x7b7c)), person});
        const bool present = nonzero_logical(c, contract); c.put32(role_or_scratch, contract);
        std::uint32_t employer = 0;
        if (present) {
            const auto id = c.u32(contract + 4); c.set(0, id * 0x248);
            const auto clubs = c.u32(c.toc(-0x7be8)); c.set(3, clubs); employer = clubs + id * 0x248;
        }
        c.set(0, employer); const bool has_employer = nonzero_logical(c, employer); club = employer;
        if (!has_employer) return result(c, 0);
    }
    if (preparation_byte(c, program.call(0x100c7d00, 0x10080de8, {club}))) return result(c, 0);
    const auto person_id = c.u32(person), index_rows = c.u32(c.toc(slot::contract_index));
    const auto status = (c.u8(index_rows + person_id * 100 + 12) >> 2) & 3u;
    c.set(3, person_id * 100); c.set(4, index_rows); c.set(0, status);
    if (nonzero(c, std::int32_t(status))) return result(c, 0);
    if (!preparation_byte(c, program.call(0x100c7a20, 0x10080e28, {c.u8(person + 0x62)}))) {
        const auto employer = c.u32(person + 0x24); c.set(4, employer);
        if (compare_logical(c, club, employer) == 0) {
            const auto transfers = c.u32(context_or_contract); c.set(3, transfers);
            const auto id = c.u32(employer); c.set(0, id);
            const auto shortlist = c.u32(transfers + 0x988); c.set(4, shortlist); c.set(3, id * 12);
            const auto entries = c.u32(shortlist + 0xdc); c.set(4, entries); c.set(0, id * 12 + 4);
            const auto state = c.i8(entries + id * 12 + 4); c.set(0, std::uint32_t(std::int32_t(state)));
            if (compare(c, state, 2) == 0) return result(c, 0);
            const auto owner = c.u32(club + 0x90); c.set(0, owner);
            if (!nonzero_logical(c, owner) && preparation_byte(c, program.call(0x100d3c40, 0x10080e78, {club}))) return result(c, 0);
        }
    }
    if (preparation_byte(c, program.call(0x1014c2e0, 0x10080ea0, {club, 0, 1, 0, 0}))) return result(c, 0);
    if (preparation_byte(c, program.call(0x103f43b0, 0x10080ec0, {c.u32(context_or_contract), person, 0}))) return result(c, 0);
    const auto club_id = c.u32(club); c.set(0, club_id); compare(c, std::int32_t(std::uint32_t(origin)), -1); c.put32(frame.at(0x50), club_id);
    if (std::int32_t(std::uint32_t(origin)) != -1) {
        const auto offer = program.call(0x103eb440, 0x10080eec, {c.u32(context_or_contract), origin});
        if (nonzero_logical(c, offer)) offer_or_weeks = c.u32(offer);
    }
    auto role = program.call(0x1036d510, 0x10080f10, {c.u32(c.u32(context_or_contract) + 0x988), person, club, frame.at(0x38)});
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(role))));
    if (compare(c, std::int8_t(role), 3) == 0) {
        const auto attributes = c.u32(person + 0x14); c.set(4, attributes);
        const auto ability = c.i16(attributes + 10); c.set(0, extended(ability));
        if (compare(c, ability, 5250) > 0) role = 2;
    }
    const auto proposal_result = program.call(0x10374f50, 0x10080f54,
        {proposal, person, club, origin, role, c.u8(frame.at(0x38)), 0, 0});
    c.set(0, proposal_result ^ 0x80000000u); c.set(5, 0x43300000);
    c.put32(frame.at(0xd4), proposal_result ^ 0x80000000u);
    const auto bias = c.f64(constants + 8); c.set_f(2, bias); c.put32(frame.at(0xd0), 0x43300000);
    const auto zero = c.f64(constants + 0x60); c.set_f(0, zero);
    const auto prepared_value = fp.fsubs(c.f64(frame.at(0xd0)), bias); c.set_f(1, prepared_value); fp.fcmp(0, zero, prepared_value);
    if (zero == prepared_value) return result(c, 0);

    // Scale the two monetary amounts using double intermediates, then
    // truncate through the original conversion's full 64-bit stack image.
    const auto initial = c.u32(frame.at(0x54)), weekly = c.u32(frame.at(0x58));
    const auto guaranteed = c.i8(frame.at(0x69)); c.set(4, initial ^ 0x80000000u); c.set(3, weekly ^ 0x80000000u); c.set(0, std::uint32_t(std::int32_t(guaranteed)));
    const bool has_guarantee = nonzero(c, guaranteed);
    const auto encoded_initial = encoded_amount(c, frame, 0xd0, initial); c.set_f(0, encoded_initial);
    c.put32(frame.at(0xc4), weekly ^ 0x80000000u);
    const auto initial_value = fp.fsub(encoded_initial, bias); c.set_f(1, initial_value); c.put32(frame.at(0xc0), 0x43300000);
    const auto encoded_weekly = c.f64(frame.at(0xc0)); c.set_f(0, encoded_weekly);
    const auto scaled_initial = fp.fmul(initial_value, c.f(30)); c.set_f(1, scaled_initial);
    const auto weekly_value = fp.fsub(encoded_weekly, bias); c.set_f(0, weekly_value);
    const auto initial_integer = truncated_amount(c, fp, frame, 1, scaled_initial, 0xc8);
    const auto scaled_weekly = fp.fmul(weekly_value, c.f(30)); c.set_f(0, scaled_weekly);
    c.set(0, initial_integer); c.put32(frame.at(0x54), initial_integer);
    const auto weekly_integer = truncated_amount(c, fp, frame, 0, scaled_weekly, 0xb8);
    c.set(0, weekly_integer); c.put32(frame.at(0x58), weekly_integer);
    if (has_guarantee) {
        const bool linked_offer = nonzero_logical(c, offer_or_weeks); context_or_contract = 0;
        if (linked_offer) {
            const auto valuation = program.call(0x10416840, 0x10081010, {offer_or_weeks, 0, 0, 0, 0, 1});
            unsigned slope = 0, base = 0; std::uint32_t excess = 0;
            c.set(4, 0x4c0000); c.set(0, 5000000);
            if (compare(c, std::int32_t(valuation), 5000000) > 0) { slope = 0x40; base = 0xd0; excess = valuation - 5000000; }
            else {
                c.set(4, 0xf0000); c.set(0, 1000000);
                if (compare(c, std::int32_t(valuation), 1000000) > 0) { slope = 0xe0; base = 0xd8; excess = valuation - 1000000; }
                else {
                    c.set(4, 0x10000); c.set(0, 50000);
                    if (compare(c, std::int32_t(valuation), 50000) > 0) { slope = 0xf0; base = 0xe8; excess = valuation - 50000; }
                }
            }
            if (slope) {
                c.set(3, excess); c.set(0, excess ^ 0x80000000u);
                c.put32(frame.at(0xb8), 0x43300000); c.put32(frame.at(0xbc), excess ^ 0x80000000u);
                const auto local_bias = c.f64(constants + 8), multiplier = c.f64(constants + slope), fixed = c.f64(constants + base);
                c.set_f(2, local_bias); c.set_f(3, multiplier); c.set_f(0, fixed);
                const auto converted = fp.fsubs(c.f64(frame.at(0xb8)), local_bias); c.set_f(1, converted);
                const auto site = slope == 0x40 ? 0x1008104c : slope == 0xe0 ? 0x1008109c : 0x100810ec;
                const auto minimum = fp.multiply_add(site, multiplier, converted, fixed); c.set_f(0, minimum);
                context_or_contract = truncated_amount(c, fp, frame, 0, minimum, 0xc0);
            } else {
                c.set(3, 0x20000); const auto current = c.i32(frame.at(0x70)); c.set(4, std::uint32_t(current)); c.set(0, 100000);
                if (compare(c, current, 100000) < 0) context_or_contract = 100000;
            }
        } else {
            c.set(3, 0x10000); const auto current = c.i32(frame.at(0x70)); c.set(4, std::uint32_t(current)); c.set(0, 50000);
            if (compare(c, current, 50000) < 0) context_or_contract = 50000;
        }
        const auto current = c.i32(frame.at(0x70)); c.set(0, std::uint32_t(current));
        if (compare(c, current, std::int32_t(std::uint32_t(context_or_contract))) < 0) c.put32(frame.at(0x70), context_or_contract);
    } else {
        const auto alternative = c.i8(frame.at(0x6b)); c.set(0, std::uint32_t(std::int32_t(alternative)));
        bool floor = nonzero(c, alternative);
        if (!floor) { const auto flag = c.i8(frame.at(0x68)); c.set(0, std::uint32_t(std::int32_t(flag))); floor = nonzero(c, flag); }
        if (floor) {
            c.set(3, 0x10000); const auto current = c.i32(frame.at(0x70)); c.set(4, std::uint32_t(current)); c.set(0, 50000);
            if (compare(c, current, 50000) < 0) c.put32(frame.at(0x70), 50000);
        }
    }

    role_or_scratch = c.u8(frame.at(0x38));
    const bool player_role = compare(c, std::int32_t(std::uint32_t(role_or_scratch)), 11) == 0;
    if (player_role) {
        const auto flags = c.u8(frame.at(0xac)); c.set(0, flags); role_or_scratch = flags & 15u;
        program.call(0x10103510, 0x10081198, {c.u32(national_or_total), club});
        const auto assessed = program.call(0x100f2680, 0x100811a4, {c.r(3), person, role_or_scratch});
        c.set(3, assessed ^ 0x80000000u); c.set(0, 0x43300000);
        c.put32(frame.at(0xbc), assessed ^ 0x80000000u); const auto increment = c.f64(constants + 0xb8); c.set_f(0, increment);
        c.put32(frame.at(0xb8), 0x43300000); const auto local_bias = c.f64(constants + 8); c.set_f(2, local_bias);
        const auto weighted = fp.multiply_add(0x100811bc, c.f(30), increment, increment); c.set_f(0, weighted);
        const auto converted = fp.fsub(c.f64(frame.at(0xb8)), local_bias); c.set_f(1, converted); c.set_f(31, fp.fmul(weighted, converted));
    } else {
        program.call(0x10103510, 0x100811dc, {c.u32(national_or_total), club});
        const auto assessed = program.call(0x100f4790, 0x100811e4, {c.r(3), role_or_scratch});
        c.set(3, assessed ^ 0x80000000u); c.set(0, 0x43300000); c.put32(frame.at(0xbc), assessed ^ 0x80000000u);
        const auto local_bias = c.f64(constants + 8); c.set_f(1, local_bias); c.put32(frame.at(0xb8), 0x43300000);
        c.set_f(0, c.f64(frame.at(0xb8))); c.set_f(31, fp.fsub(c.f(0), local_bias));
    }
    context_or_contract = program.call(0x10082f90, 0x10081210, {object, person, club});
    const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
    if (nonzero_logical(c, attributes)) {
        const auto elapsed = program.call(0x10718130, 0x10081230, {today, c.u32(person + 0x34), c.u32(person + 0x38)});
        c.set(0, elapsed ^ 0x80000000u); c.set(5, 0x43300000);
        c.put32(frame.at(0xbc), elapsed ^ 0x80000000u); const auto current_attributes = c.u32(person + 0x14); c.set(6, current_attributes);
        c.put32(frame.at(0xb8), 0x43300000); const auto local_bias = c.f64(constants + 8); c.set_f(4, local_bias);
        const auto encoded = c.f64(frame.at(0xb8)); c.set_f(1, encoded); const auto year_days = c.f64(constants + 0xf8); c.set_f(0, year_days);
        const auto days = fp.fsub(encoded, local_bias); c.set_f(1, days);
        const auto position = c.u8(current_attributes + 15); c.set(0, position); const bool special_position = compare(c, position, 20) == 0;
        double age = fp.fdiv(days, year_days); c.set_f(30, age);
        if (special_position) {
            const auto adjustment = c.f64(constants + 0x98); c.set_f(0, adjustment); age = fp.fsub(age, adjustment); c.set_f(30, age);
        } else {
            const auto adaptation = c.i8(person + 0x67), first = c.i8(current_attributes + 0x34), second = c.i8(current_attributes + 0x3c);
            c.set(4, std::uint32_t(std::int32_t(adaptation))); c.set(3, std::uint32_t(std::int32_t(second)));
            const auto total = std::uint32_t(std::int32_t(first) + adaptation + second); c.set(0, total ^ 0x80000000u);
            const auto encoded_traits = encoded_amount(c, frame, 0xc0, total); c.set_f(3, encoded_traits);
            const auto divisor = c.f64(constants + 0x100), baseline = c.f64(constants + 0x108), years = c.f64(constants + 0xf0);
            c.set_f(2, divisor); c.set_f(1, baseline); c.set_f(0, years);
            const auto traits = fp.fsub(encoded_traits, local_bias); c.set_f(3, traits);
            const auto share = fp.fdiv(traits, divisor); c.set_f(2, share);
            const auto relative = fp.fsub(share, baseline); c.set_f(1, relative);
            const auto adjustment = fp.fdiv(relative, years); c.set_f(0, adjustment);
            age = fp.fsub(age, adjustment); c.set_f(30, age);
        }
        // Older players receive shorter terms and adjusted positive bonuses.
        // Each branch has its own scratch layout and rounding sequence.
        unsigned bonus_multiplier = 0;
        auto cap_years = [&](std::uint32_t limit, bool final_band) {
            const auto old_year = c.u16(frame.at(0x7e)); c.set(4, limit); c.set(0, old_year);
            if (compare(c, old_year, std::int32_t(limit)) <= 0) return;
            const auto wage = c.u32(frame.at(0x58)); c.set(3, wage); c.set(0, wage ^ 0x80000000u);
            const auto encoded_wage = encoded_amount(c, frame, 0xb8, wage); c.put16(frame.at(0x7e), limit);
            const auto conversion_bias = final_band ? local_bias : c.f64(constants + 8);
            const auto multiplier = c.f64(constants + 0x28);
            if (final_band) c.set_f(1, multiplier); else { c.set_f(1, conversion_bias); c.set_f(2, multiplier); }
            const auto converted = fp.fsub(encoded_wage, conversion_bias); c.set_f(0, converted);
            const auto adjusted = fp.fmul(final_band ? converted : multiplier, final_band ? multiplier : converted); c.set_f(0, adjusted);
            const auto value = truncated_amount(c, fp, frame, 0, adjusted, 0xc0); c.set(0, value); c.put32(frame.at(0x58), value);
        };
        const auto oldest = c.f64(constants + 0x110); c.set_f(0, oldest); fp.fcmp(0, age, oldest);
        bool oldest_band = age > oldest;
        if (!oldest_band && nonzero_logical(c, context_or_contract)) {
            const auto squad_role = c.u8(context_or_contract + 0x60) & 15u; c.set(0, squad_role); oldest_band = compare(c, squad_role, 6) == 0;
        }
        if (oldest_band) {
            c.set(3, c.u16(today + 2)); cap_years(std::uint32_t(c.u16(today + 2)) + 1, false); bonus_multiplier = 0x40;
        } else {
            const auto older = c.f64(constants + 0x118); c.set_f(0, older); fp.fcmp(0, age, older);
            if (age > older) { c.set(3, c.u16(today + 2)); cap_years(std::uint32_t(c.u16(today + 2)) + 2, false); bonus_multiplier = 0x30; }
            else {
                const auto mature = c.f64(constants + 0x120); c.set_f(0, mature); fp.fcmp(0, age, mature);
                if (age > mature) {
                    const auto ability = c.i16(current_attributes + 12), reputation = c.i16(club + 0x1ec);
                    c.set(3, extended(ability) ^ 0x80000000u); c.set(0, extended(reputation) ^ 0x80000000u); c.set(5, 0x43300000);
                    const auto encoded_ability = encoded_amount(c, frame, 0xb8, extended(ability));
                    const auto conversion_bias = c.f64(constants + 8), weight = c.f64(constants + 0x20); c.set_f(2, conversion_bias); c.set_f(3, weight);
                    const auto converted_ability = fp.fsub(encoded_ability, conversion_bias); c.set_f(1, converted_ability);
                    const auto encoded_reputation = encoded_amount(c, frame, 0xc0, extended(reputation)); c.set_f(0, encoded_reputation);
                    const auto weighted_ability = fp.fmul(weight, converted_ability); c.set_f(1, weighted_ability);
                    const auto converted_reputation = fp.fsub(encoded_reputation, conversion_bias); c.set_f(0, converted_reputation); fp.fcmp(0, weighted_ability, converted_reputation);
                    if (weighted_ability < converted_reputation) {
                        const auto rounding = c.f64(constants + 0xb8); c.set_f(0, rounding); const auto rounded = fp.fadd(rounding, age); c.set_f(0, rounded);
                        const auto raw = truncated_amount(c, fp, frame, 0, rounded, 0xb8); const auto years = std::int32_t(std::int8_t(raw));
                        const auto limit = std::uint32_t(c.u16(today + 2)) + 5 - std::uint32_t(years - 29);
                        c.set(0, std::uint32_t(c.u16(today + 2)) + 5); c.set(3, limit);
                        cap_years(limit, true); bonus_multiplier = 0xe0;
                    }
                }
            }
        }
        if (bonus_multiplier) {
            c.set(4, proposal); c.set(3, 0x43300000); c.set(7, 0); c.set(5, 0);
            for (unsigned n = 0; n < 3; ++n) {
                const auto offset = n * 4 + 0x10; c.set(6, offset);
                const auto bonus = c.i32(proposal + offset); c.set(0, std::uint32_t(bonus));
                if (compare(c, bonus, -1) > 0) {
                    const auto converted = encoded_amount(c, frame, 0xb8, std::uint32_t(bonus));
                    const auto conversion_bias = c.f64(constants + 8), multiplier = c.f64(constants + bonus_multiplier);
                    c.set_f(1, conversion_bias); c.set_f(2, multiplier); c.set_f(0, converted);
                    const auto value = fp.fsub(converted, conversion_bias); c.set_f(0, value);
                    const auto adjusted = fp.fmul(multiplier, value); c.set_f(0, adjusted);
                    const auto integer = truncated_amount(c, fp, frame, 0, adjusted, 0xc0); c.set(0, integer); c.put32(proposal + offset, integer);
                }
                c.set(5, (n + 1) * 4); c.set(7, n + 1);
            }
            c.set(0, 3); compare(c, 3, 3);
        }

        const auto wage = c.i32(frame.at(0x58)); c.set(0, std::uint32_t(wage));
        if (compare(c, wage, 10000) > 0) {
            const auto discount_age = c.f64(constants + 0x128); c.set_f(0, discount_age); fp.fcmp(0, age, discount_age);
            if (age > discount_age) {
                const auto duration = program.call(0x10718130, 0x100815b8, {frame.at(0x7c), c.u32(today), c.u32(today + 4)});
                const auto high = multiply_high(std::int32_t(0x92492493), std::int32_t(duration));
                const auto sum = std::int32_t(std::uint32_t(high) + duration), floor = sum >> 2;
                offer_or_weeks = std::uint32_t(floor) + (std::uint32_t(floor) >> 31);
                c.s.wr<1>(guest::reg::xer_ca, sum < 0 && (std::uint32_t(sum) & 3));
                c.set(3, std::uint32_t(floor)); c.set(4, std::uint32_t(floor) >> 31);
                const auto current_wage = c.u32(frame.at(0x58)); c.set(0, current_wage); national_or_total = std::uint32_t(offer_or_weeks) * current_wage;
                const auto powered = [&](double base, double exponent, std::uint32_t back) {
                    c.set_f(1, base); c.set_f(2, exponent); program.call(0x10780fc0, back); c.set(2, c.u32(frame.at(0x14))); return c.f(1);
                };
                const auto threshold = c.f64(constants + 0x128), exponent = c.f64(constants + 0x98); c.set_f(0, threshold);
                auto base = fp.fsub(age, threshold); c.set_f(1, base); auto power = powered(base, exponent, 0x100815ec);
                double discount = c.f64(constants + 0x18); c.set_f(29, discount); fp.fcmp(0, power, discount);
                if (power > discount) { c.set_f(0, c.f64(constants + 0x128)); base = fp.fsub(age, c.f(0)); power = powered(base, c.f64(constants + 0x98), 0x1008160c); discount = power; c.set_f(29, discount); }
                const auto powered_ability = [&](unsigned slot, std::uint32_t back) {
                    const auto data = c.u32(person + 0x14); c.set(3, data); const auto ability = c.i16(data + 10); c.set(0, extended(ability) ^ 0x80000000u);
                    const auto raw = encoded_amount(c, frame, slot, extended(ability)), conversion_bias = c.f64(constants + 8);
                    c.set_f(0, raw); c.set_f(1, conversion_bias); const auto converted = fp.fsub(raw, conversion_bias);
                    return powered(converted, c.f64(constants + 0x98), back);
                };
                power = powered_ability(0xb8, 0x10081640);
                const auto divisor = c.f64(constants + 0x130), ceiling = c.f64(constants + 0x138); c.set_f(2, divisor); c.set_f(0, ceiling);
                const auto ratio = fp.fdiv(power, divisor); c.set_f(1, ratio); fp.fcmp(0, ratio, ceiling);
                double capped = ceiling;
                if (ratio < ceiling) { power = powered_ability(0xc0, 0x10081684); c.set_f(0, c.f64(constants + 0x130)); capped = fp.fdiv(power, c.f(0)); c.set_f(0, capped); }
                power = powered(capped, c.f64(constants + 0x98), 0x1008169c);
                const auto discounted = fp.fdiv(power, discount); c.set_f(0, discounted);
                const auto encoded_total = encoded_amount(c, frame, 0xd0, national_or_total), conversion_bias = c.f64(constants + 8);
                c.set(3, 0x43300000); c.set(0, std::uint32_t(national_or_total) ^ 0x80000000u); c.set_f(3, conversion_bias);
                const auto allowance = truncated_amount(c, fp, frame, 2, discounted, 0xc8); c.set(4, allowance);
                const auto total = fp.fsub(encoded_total, conversion_bias); c.set_f(2, total);
                const auto encoded_allowance = encoded_amount(c, frame, 0xb0, allowance); c.set(0, allowance ^ 0x80000000u); c.set_f(0, encoded_allowance);
                c.set_f(1, c.f64(constants + 0xe0)); const auto amount = fp.fsub(encoded_allowance, conversion_bias); c.set_f(0, amount);
                const auto tolerated = fp.fmul(c.f(1), amount); c.set_f(0, tolerated); fp.fcmp(0, total, tolerated);
                if (total > tolerated) {
                    while (compare(c, std::int32_t(std::uint32_t(offer_or_weeks)), 104) > 0) {
                        offer_or_weeks = std::uint32_t(offer_or_weeks) - 52;
                        const auto old_wage = c.u32(frame.at(0x58)); c.set(0, old_wage ^ 0x80000000u);
                        const auto raw = encoded_amount(c, frame, 0xb0, old_wage), local_conversion_bias = c.f64(constants + 8), multiplier = c.f64(constants + 0x28);
                        c.set_f(1, local_conversion_bias); c.set_f(2, multiplier); c.set_f(0, raw);
                        const auto converted = fp.fsub(raw, local_conversion_bias); c.set_f(0, converted);
                        const auto adjusted = fp.fmul(multiplier, converted); c.set_f(0, adjusted);
                        const auto value = truncated_amount(c, fp, frame, 0, adjusted, 0xb8); c.set(0, value); c.put32(frame.at(0x58), value);
                        national_or_total = std::uint32_t(offer_or_weeks) * value;
                        if (compare(c, std::int32_t(allowance), std::int32_t(std::uint32_t(national_or_total))) >= 0) break;
                    }
                }
                if (compare(c, std::int32_t(std::uint32_t(national_or_total)), std::int32_t(allowance)) > 0) {
                    const auto per_week = std::uint32_t(divide_signed<4>(allowance, offer_or_weeks)); c.set(3, per_week ^ 0x80000000u); c.set(0, 0x43300000);
                    const auto raw = encoded_amount(c, frame, 0xb0, per_week), local_conversion_bias = c.f64(constants + 8); c.set_f(1, local_conversion_bias);
                    const auto weekly_limit = fp.fsub(raw, local_conversion_bias); c.set_f(0, weekly_limit); fp.fcmp(0, weekly_limit, c.f(31));
                    if (weekly_limit < c.f(31)) { const auto repeated = encoded_amount(c, frame, 0xb8, per_week); c.set_f(0, repeated); c.set_f(0, fp.fsub(repeated, local_conversion_bias)); }
                    else c.set_f(0, c.f(31));
                    c.set_f(31, c.f(0));
                }
            }
        }
    }

    if (nonzero_logical(c, context_or_contract)) {
        const auto current_year = c.u16(context_or_contract + 0x32), proposed_year = c.u16(frame.at(0x7e)); c.set(3, current_year); c.set(0, proposed_year);
        if (compare_logical(c, current_year, proposed_year) >= 0) return result(c, 0);
        const auto service = c.u32(injuries); c.set(3, service);
        if (nonzero_logical(c, service)) {
            const auto found = program.call(0x1018c340, 0x100817bc, {service, person});
            if (nonzero_logical(c, found)) {
                const auto again = program.call(0x1018c340, 0x100817d0, {c.u32(injuries), person});
                const auto days = c.i16(again + 0x34); c.set(0, extended(days)); if (compare(c, days, 70) > 0) return result(c, 0);
            }
        }
    }
    const auto config = c.toc(-0x7a68); c.set(3, config); const auto limited = c.u8(config); c.set(0, limited);
    if (compare(c, limited, 1) == 0) {
        const auto maximum_year = std::uint32_t(c.u16(today + 2)) + 5;
        const auto proposed_year = c.u16(frame.at(0x7e)); c.set(3, maximum_year); c.set(0, proposed_year);
        if (compare(c, proposed_year, std::int32_t(maximum_year)) > 0) c.put16(frame.at(0x7e), maximum_year);
    }
    const auto proposed_wage = c.u32(frame.at(0x58)); c.set(3, proposed_wage ^ 0x80000000u); c.set(0, 0x43300000);
    const auto wage_encoded = encoded_amount(c, frame, 0xb0, proposed_wage), ceiling_factor = c.f64(constants + 0x38), final_bias = c.f64(constants + 8);
    c.set_f(0, ceiling_factor); c.set_f(2, final_bias); const auto ceiling = fp.fmul(ceiling_factor, c.f(31)); c.set_f(0, ceiling);
    const auto proposed = fp.fsub(wage_encoded, final_bias); c.set_f(1, proposed); fp.fcmp(0, proposed, ceiling);
    if (proposed > ceiling) return result(c, 0);
    c.put32(frame.at(0xb4), proposed_wage ^ 0x80000000u); c.put32(frame.at(0xb0), 0x43300000); c.set_f(0, c.f64(frame.at(0xb0)));
    const auto repeated_wage = fp.fsub(c.f(0), final_bias); c.set_f(0, repeated_wage); fp.fcmp(0, repeated_wage, c.f(31));
    if (repeated_wage > c.f(31)) {
        c.put32(frame.at(0xb4), proposed_wage ^ 0x80000000u); const auto minimum = c.f64(constants + 0x18); c.set_f(0, minimum); c.put32(frame.at(0xb0), 0x43300000);
        fp.fcmp(0, minimum, c.f(31)); const auto loaded = c.f64(frame.at(0xb0)); c.set_f(1, loaded); const auto amount = fp.fsub(loaded, final_bias); c.set_f(1, amount);
        const auto divisor = minimum > c.f(31) ? minimum : c.f(31); c.set_f(0, divisor); const auto ratio = fp.fdiv(amount, divisor); c.set_f(2, ratio);
        const auto initial_amount = c.u32(frame.at(0x54)); c.set(0, initial_amount ^ 0x80000000u); c.set(4, 0x43300000); c.set(7, 0); c.set(5, 0); c.set(3, proposal);
        const auto encoded = encoded_amount(c, frame, 0xb8, initial_amount); c.set_f(1, c.f64(constants + 8)); c.set_f(0, encoded);
        const auto converted = fp.fsub(encoded, c.f(1)); c.set_f(0, converted); const auto scaled = fp.fmul(converted, ratio); c.set_f(0, scaled);
        const auto integer = truncated_amount(c, fp, frame, 0, scaled, 0xc0); c.set(0, integer); c.put32(frame.at(0x54), integer);
        for (unsigned n = 0; n < 3; ++n) {
            const auto offset = n * 4 + 0x10; c.set(6, offset); const auto bonus = c.i32(proposal + offset); c.set(0, std::uint32_t(bonus));
            if (compare(c, bonus, -1) > 0) {
                const auto raw = encoded_amount(c, frame, 0xb0, std::uint32_t(bonus)); c.set_f(1, c.f64(constants + 8)); c.set_f(0, raw);
                const auto value = fp.fsub(raw, c.f(1)); c.set_f(0, value); const auto scaled_bonus = fp.fmul(value, ratio); c.set_f(0, scaled_bonus);
                const auto result = truncated_amount(c, fp, frame, 0, scaled_bonus, 0xb8); c.set(0, result); c.put32(proposal + offset, result);
            }
            c.set(5, (n + 1) * 4); c.set(7, n + 1);
        }
        c.set(0, 3); compare(c, 3, 3); const auto value = truncated_amount(c, fp, frame, 0, c.f(31), 0xb0); c.set(0, value); c.put32(frame.at(0x58), value);
    }
    program.call(0x1037f830, 0x10081934, {proposal, 0});
    if (nonzero_logical(c, output)) {
        for (unsigned offset : {0u,4u,8u,12u}) { const auto value=c.u32(proposal+offset); c.set(0,value); c.put32(output+offset,value); }
        const auto first=c.u32(proposal+0x10), second=c.u32(proposal+0x14); c.set(6,first); c.set(0,second); c.put32(output+0x10,first); c.put32(output+0x14,second);
        for (unsigned offset : {0x18u,0x1au}) { const auto value=c.u16(proposal+offset); c.set(0,value); c.put16(output+offset,value); }
        program.call(0x10771110,0x1008198c,{output+0x1c,proposal+0x1c,5});
        const auto value=c.u32(proposal+0x24); c.set(0,value); c.set(3,1); c.put32(output+0x24,value);
        prepared_date(c,output+0x28,proposal+0x28); prepared_date(c,output+0x30,proposal+0x30);
        for (auto [offset,width] : {std::pair{0x38u,1u},{0x3cu,4u},{0x40u,1u},{0x42u,2u},{0x44u,1u},{0x45u,1u},{0x46u,1u},{0x48u,4u},{0x4cu,1u},{0x50u,4u},{0x54u,1u},{0x58u,4u},{0x5cu,1u},{0x60u,4u}}) {
            const auto field=width==4?c.u32(proposal+offset):width==2?extended(c.i16(proposal+offset)):c.u8(proposal+offset); c.set(0,field);
            if(width==4)c.put32(output+offset,field);else if(width==2)c.put16(output+offset,field);else c.put8(output+offset,field);
        }
        return 1;
    }
    const auto action = std::int8_t(std::uint32_t(mode)); c.set(0,std::uint32_t(std::int32_t(action)));
    bool publish = compare(c,action,2)==0;
    if (!publish && nonzero_logical(c,context_or_contract) && compare(c,action,1)==0) {
        program.call(0x10717890,0x10081a68,{frame.at(0x44),c.toc(-0x79b8),150});
        const auto date=c.u32(frame.at(0x44)),flags=c.u32(frame.at(0x48)); c.set(3,date);c.set(0,flags);c.put32(frame.at(0x3c),date);
        const auto current_year=c.u16(context_or_contract+0x32),cutoff_year=c.u16(frame.at(0x3e)); c.set(3,current_year);c.set(4,cutoff_year);c.put32(frame.at(0x40),flags);
        const auto order=compare_logical(c,cutoff_year,current_year); publish=order<0;
        if(!order){const auto cutoff_day=c.i16(frame.at(0x3c)),current_day=c.i16(context_or_contract+0x30);const auto bits=extended(current_day)^extended(cutoff_day);c.set(4,extended(current_day));c.set(3,std::uint32_t(std::int32_t(bits)>>1));c.s.wr<1>(guest::reg::xer_ca,std::int32_t(bits)<0&&(bits&1));publish=cutoff_day<current_day;}
        c.set(0,publish); nonzero(c,publish);
    }
    if(publish)program.call(0x1007f200,0x10081ae8,{object,proposal,origin,0xffffffff,c.u8(person+0x62),0,1,0});
    return result(c,1);
}
}
