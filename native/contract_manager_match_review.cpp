#include "contract_manager_random.hpp"
#include <tuple>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
bool signed_return(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value); c.set(0, std::uint32_t(std::int32_t(byte))); return nonzero(c, byte);
}
// Four copies of the same reputation rule use different local slots.
// Preserve those stores, including the conditional third conversion.
bool reputation_mismatch(Cpu& c, const Frame& frame, std::uint32_t constants,
                         std::uint32_t club, std::uint32_t opponent, bool first_home) {
    const unsigned initial = first_home ? 0x1e8 : 0x1d0;
    const unsigned raised = first_home ? 0x1e0 : 0x1d8;
    const unsigned repeated = first_home ? 0x1d8 : 0x1e0;
    const unsigned other = first_home ? 0x1d0 : 0x1e8;
    const auto reputation = c.i16(club + 0x1ec); c.set(4, extended(reputation)); c.set(3, 0x43300000);
    c.put32(frame.at(initial), 0x43300000); c.put32(frame.at(initial + 4), extended(reputation) ^ 0x80000000u);
    const auto increased = std::int32_t(reputation) + 1500; c.set(4, std::uint32_t(increased)); c.set(0, std::uint32_t(increased) ^ 0x80000000u);
    guest::Fpu fpu(c); const auto bias = c.f64(constants + 8); c.set_f(2, bias);
    const auto encoded = c.f64(frame.at(initial)); c.set_f(0, encoded);
    c.put32(frame.at(raised + 4), std::uint32_t(increased) ^ 0x80000000u);
    const auto converted = fpu.fsub(encoded, bias); c.set_f(1, converted);
    const auto scale = c.f64(constants + 0x40); c.set_f(3, scale); c.put32(frame.at(raised), 0x43300000);
    const auto raised_encoded = c.f64(frame.at(raised)); c.set_f(0, raised_encoded);
    auto cutoff = fpu.fmul(scale, converted); c.set_f(3, cutoff);
    const auto raised_value = fpu.fsub(raised_encoded, bias); c.set_f(0, raised_value); fpu.fcmp(0, cutoff, raised_value);
    if (!(cutoff > raised_value)) {
        c.put32(frame.at(repeated + 4), std::uint32_t(increased) ^ 0x80000000u); c.put32(frame.at(repeated), 0x43300000);
        const auto again = c.f64(frame.at(repeated)); c.set_f(0, again); cutoff = fpu.fsub(again, bias); c.set_f(3, cutoff);
    }
    c.set(3, opponent); c.set(0, 0x43300000); c.put32(frame.at(other), 0x43300000);
    const auto opponent_reputation = c.i16(opponent + 0x1ec); c.set(0, extended(opponent_reputation) ^ 0x80000000u);
    const auto other_bias = c.f64(constants + 8); c.set_f(1, other_bias);
    c.put32(frame.at(other + 4), extended(opponent_reputation) ^ 0x80000000u);
    const auto other_encoded = c.f64(frame.at(other)); c.set_f(0, other_encoded);
    const auto other_value = fpu.fsub(other_encoded, other_bias); c.set_f(0, other_value); fpu.fcmp(0, cutoff, other_value);
    return cutoff > other_value;
}
std::uint32_t match_sample(Cpu& c, std::uint32_t salt, std::uint32_t table, std::uint32_t cursor,
                            std::int32_t bound, std::uint32_t back) {
    const auto end_holder = c.toc(-0x7b88); c.set(3, end_holder);
    const auto old = c.u32(cursor); c.set(4, old);
    const RandomTable random{c, salt, table, end_holder, cursor}; random.advance(back);
    // 21 minus a signed byte is in [-106,149]: wide sampling is unreachable.
    c.set(3, 0x10000); c.set(0, 0xffff); compare(c, bound, 0xffff);
    c.set(3, 0xffff0000); c.set(0, 0xffff0001); compare(c, bound, -0xffff);
    const auto position = c.u32(cursor); c.set(3, position); const bool positive = compare(c, bound) > 0;
    const auto seed = c.u32(salt); c.set(0, seed);
    const auto draw = c.u16(position) + seed; c.set(3, draw);
    const auto quotient = std::uint32_t(divide_signed<4>(draw, std::uint32_t(bound)));
    const auto remainder = draw - quotient * std::uint32_t(bound); c.set(0, remainder);
    return positive ? remainder : 0u - remainder;
}
struct MinuteShare { std::int32_t scaled, high, floor; std::uint32_t sign; std::int8_t share; };
MinuteShare minute_share(std::int8_t minutes) {
    const auto scaled = std::int32_t(minutes) * 10;
    const auto high = multiply_high(std::int32_t(0xb60b60b7), scaled);
    const auto floor = std::int32_t(std::uint32_t(high) + std::uint32_t(scaled)) >> 6;
    const auto sign = std::uint32_t(floor) >> 31;
    return {scaled, high, floor, sign, std::int8_t(std::uint32_t(floor) + sign)};
}
void change_interest(Cpu& c, std::uint32_t contract, std::int32_t change) {
    const auto raw = c.u8(contract + 0x4c); c.set(4, raw); c.set(3, std::uint32_t(std::int32_t(std::int8_t(raw))));
    const auto value = std::int32_t(std::int8_t(raw)) + change; c.set(0, std::uint32_t(value));
    if (compare(c, value, 100) > 0) { c.set(0, 100); c.put8(contract + 0x4c, 100); }
    else if (compare(c, value, -100) < 0) { c.set(0, 0xffffff9c); c.put8(contract + 0x4c, 0xffffff9c); }
    else { c.set(0, std::uint32_t(raw) + std::uint32_t(change)); c.put8(contract + 0x4c, std::uint32_t(raw) + std::uint32_t(change)); }
}
void decrease_score(Cpu& c, std::uint32_t contract, std::int32_t amount) {
    const auto raw = c.u8(contract + 0x44); c.set(4, raw); const auto score = std::int32_t(std::int8_t(raw)); c.set(3, std::uint32_t(score));
    const auto value = score - amount; c.set(0, std::uint32_t(value));
    if (compare(c, value, 100) > 0) { c.set(0, 100); c.put8(contract + 0x44, 100); }
    else if (compare(c, value, -100) < 0) { c.set(0, 0xffffff9c); c.put8(contract + 0x44, 0xffffff9c); }
    else { c.set(0, std::uint32_t(raw) - std::uint32_t(amount)); c.put8(contract + 0x44, std::uint32_t(raw) - std::uint32_t(amount)); }
}
}

void review_match_contracts(Cpu& c, std::uint32_t manager_address, std::uint32_t fixture_address, std::uint32_t data_address) {
    Frame frame(c, 0x1007c4e0, 0x250, {.first_gpr = 13});
    Kept<21> constants(c, c.toc(-0x571c)); Kept<25> fixture(c, fixture_address);
    Kept<22> salt(c, c.toc(-0x7b90)); Kept<24> object(c, manager_address);
    Kept<14> table(c, c.toc(-0x7b8c)); Kept<23> cursor(c, c.toc(-0x7b84)); Program program(c);
    const auto played = c.i8(fixture + 0x40); c.set(0, std::uint32_t(std::int32_t(played))); c.put32(frame.at(0x270), data_address);
    if (!nonzero(c, played)) return;
    const auto complete = c.i8(fixture + 0x41); c.set(0, std::uint32_t(std::int32_t(complete))); if (!nonzero(c, complete)) return;
    const auto penalty_home = c.i8(fixture + 0x47); c.set(3, std::uint32_t(std::int32_t(penalty_home))); c.set(0, std::uint32_t(std::int32_t(penalty_home)));
    const bool penalties = compare(c, penalty_home, -1) != 0;
    std::int8_t home_score, away_score;
    if (penalties) { home_score = penalty_home; away_score = c.i8(fixture + 0x48); c.set(0, std::uint32_t(std::int32_t(away_score))); }
    else { home_score = c.i8(fixture + 0x43); away_score = c.i8(fixture + 0x44); c.set(3, std::uint32_t(std::int32_t(home_score))); c.set(0, std::uint32_t(std::int32_t(away_score))); }
    const auto outcome = compare(c, home_score, away_score);
    if (outcome) {
        c.set(3, outcome > 0); c.set(0, outcome < 0); c.put8(frame.at(0x3c), outcome > 0); c.put8(frame.at(0x3d), outcome < 0);
    } else { c.set(0, 0); c.put8(frame.at(0x3d), 0); c.put8(frame.at(0x3c), 0); }
    program.call(0x1077fc28, 0x1007c5e8, {c.u32(fixture + 0x1c) + 0x98, frame.at(0x40), 200}); c.set(2, c.u32(c.sp() + 0x14));
    program.call(0x1077fc28, 0x1007c600, {c.u32(fixture + 0x20) + 0x98, frame.at(0x108), 200}); c.set(2, c.u32(c.sp() + 0x14));
    c.set(0, 0); c.put32(frame.at(0x1f4), 0); c.put8(frame.at(0x1f0), 0);
    {
        Kept<17> roster_offset(c, c.u32(frame.at(0x1f4)));
        Kept<31> club(c, c.r(31)); Kept<26> finances(c, c.r(26)); Kept<19> score_offset(c, c.r(19));
        Kept<30> player_index(c, c.r(30)); Kept<18> stat_offset(c, c.r(18)); Kept<16> stats(c, c.r(16));
        Kept<29> row(c, c.r(29)); Kept<28> person(c, c.r(28)); Kept<27> contract(c, c.r(27));
        Kept<15> minutes(c, c.r(15)); Kept<20> sample(c, c.r(20)); Kept<13> interest(c, c.r(13));
        for (;;) {
            const auto side = c.i8(frame.at(0x1f0)); c.set(0, std::uint32_t(std::int32_t(side))); if (compare(c, side, 2) >= 0) break;
            c.set(0, std::uint32_t(std::int32_t(side))); nonzero(c, side); club = c.u32(fixture + (side == 0 ? 0x1c : 0x20));
            const auto alternate = program.call(0x100ce320, 0x1007c644, {club, frame.at(0x38), 1});
            bool skipped = false;
            if (nonzero_logical(c, alternate)) { const auto flag = c.i8(frame.at(0x38)); c.set(0, std::uint32_t(std::int32_t(flag))); skipped = !nonzero(c, flag); }
            if (!skipped) {
                const auto type = c.u8(fixture + 0x3a); c.set(0, type);
                if (compare(c, type, 1) == 0) {
                    const auto current_side = c.i8(frame.at(0x1f0)); c.set(0, std::uint32_t(std::int32_t(current_side))); nonzero(c, current_side);
                    if (current_side == 0) skipped = reputation_mismatch(c, frame, constants, club, c.u32(fixture + 0x20), true);
                    if (!skipped) {
                        const auto again = c.u8(frame.at(0x1f0)); c.set(0, again);
                        if (compare(c, again, 1) == 0) skipped = reputation_mismatch(c, frame, constants, club, c.u32(fixture + 0x1c), false);
                    }
                }
            }
            if (!skipped) {
                finances = program.call(0x10103510, 0x1007c79c, {c.u32(c.toc(-0x7ab8)), club});
                const auto data = c.u32(frame.at(0x270)); c.set(4, data); score_offset = 0;
                const auto offset = c.u32(frame.at(0x1f4)); c.set(0, offset); player_index = 0; stat_offset = 0;
                stats = data + offset; row = stats + 0x194;
                for (;;) {
                    const auto index = std::int8_t(std::uint32_t(player_index)); c.set(0, std::uint32_t(std::int32_t(index))); if (compare(c, index, 20) >= 0) break;
                    const auto id = c.i32(row); c.set(3, std::uint32_t(id)); bool valid = compare(c, id, -1) != 0;
                    if (valid) { const auto recorded_club = c.i32(row + 4); c.set(0, std::uint32_t(recorded_club)); valid = compare(c, recorded_club, -1) != 0; }
                    if (valid) {
                        const auto offset_person = std::uint32_t(id) * Staff::size; c.set(0, offset_person);
                        const auto holder = c.toc(-0x7be0); c.set(3, holder); const auto base = c.u32(holder); c.set(3, base); person = base + offset_person;
                        if (nonzero(c, std::int32_t(std::uint32_t(person)))) {
                            const auto count_holder = c.toc(slot::person_count); c.set(3, count_holder);
                            const auto person_id = c.i32(person), count = c.i32(count_holder); c.set(4, std::uint32_t(person_id)); c.set(0, std::uint32_t(count)); valid = compare(c, person_id, count) < 0;
                        }
                    }
                    if (valid) {
                        const bool national = signed_return(c, program.call(0x100c7d00, 0x1007c808, {club}));
                        if (national) contract = 0;
                        else { contract = program.call(0x10082f90, 0x1007c828, {object, person, club}); valid = nonzero(c, std::int32_t(std::uint32_t(contract))); }
                        if (valid && !national) {
                            const auto competition = c.u32(fixture + 0x14); c.set(4, competition); bool no_bonus = false;
                            if (nonzero_logical(c, competition)) {
                                no_bonus = signed_return(c, program.call(0x10494d20, 0x1007c848, {c.u32(c.toc(-0x7b54))}));
                                if (!no_bonus) { const auto holder = c.toc(-0x7aa8); c.set(3, holder); const auto actual = c.i32(fixture), special = c.i32(holder); c.set(4, std::uint32_t(actual)); c.set(0, std::uint32_t(special)); no_bonus = compare(c, actual, special) == 0; }
                            }
                            if (!no_bonus) {
                                const auto packed = c.u8(contract + 0x38); const auto role = std::int32_t(std::int8_t(packed)) >> 2; c.set(0, std::uint32_t(role)); c.s.wr<1>(guest::reg::xer_ca, role < 0 && (packed & 2));
                                no_bonus = compare(c, role, 6) == 0; if (!no_bonus) no_bonus = compare(c, role, 5) == 0;
                            }
                            if (!no_bonus) {
                                for (const auto [count_offset, value_offset, back] : {std::tuple{0x21u, 0x10u, 0x1007c8a8u}, {0x27u, 0x14u, 0x1007c8ccu}}) {
                                    const auto count = c.u8(row + count_offset); c.set(3, count);
                                    if (nonzero_logical(c, count)) { const auto amount = c.i32(contract + value_offset); c.set(0, std::uint32_t(amount)); if (compare(c, amount) > 0) program.call(0x100f71b0, back, {finances, std::uint32_t(count) * std::uint32_t(amount)}); }
                                }
                                c.set(0, score_offset + 0x156); const auto decision = c.i16(stats + score_offset + 0x156); c.set(0, extended(decision));
                                if (compare(c, decision, 1) == 0) {
                                    const auto flag = c.u8(row + 0x22); c.set(0, flag);
                                    if (nonzero_logical(c, flag)) { const auto amount = c.i32(contract + 0x18); c.set(4, std::uint32_t(amount)); if (compare(c, amount) > 0) { const auto count = c.u8(row + 0x27); c.set(0, count); program.call(0x100f71b0, 0x1007c904, {finances, std::uint32_t(count) * std::uint32_t(amount)}); } }
                                }
                            }
                        }
                        if (valid) {
                            c.set(0, std::uint32_t(std::int32_t(index)));
                            if (compare(c, index, 11) < 0) { const auto played_minutes = c.i8(row + 0x30); c.set(0, std::uint32_t(std::int32_t(played_minutes))); c.set(3, 90); if (compare(c, played_minutes, -1) > 0) c.set(3, std::uint32_t(std::int32_t(played_minutes))); minutes = std::uint32_t(std::int32_t(played_minutes > -1 ? played_minutes : 90)); }
                            else {
                                const auto introduced = c.i8(row + 0x32); c.set(0, std::uint32_t(std::int32_t(introduced)));
                                if (compare(c, introduced, -1) > 0) { const auto left = 90 - std::int32_t(introduced); c.set(3, std::uint32_t(left)); c.set(0, std::uint32_t(left > 0 ? left : 0)); c.s.wr<1>(guest::reg::xer_ca, left > 0 && (std::uint32_t(0u - std::uint32_t(left)) & 0x7fffffffu)); minutes = std::uint32_t(std::int32_t(std::int8_t(left > 0 ? left : 0))); }
                                else { c.set(0, 0); minutes = 0; }
                            }
                            c.set(0, std::uint32_t(std::int32_t(index))); bool credit = true;
                            if (compare(c, index, 11) >= 0) { c.set(0, stat_offset + 0x1c6); const auto listed = c.i8(stats + stat_offset + 0x1c6); c.set(0, std::uint32_t(std::int32_t(listed))); credit = compare(c, listed, -1) != 0; }
                            if (credit) {
                                const bool national_again = signed_return(c, program.call(0x100c7d00, 0x1007c988, {club}));
                                const auto share = minute_share(std::int8_t(std::uint32_t(minutes)));
                                c.s.wr<1>(guest::reg::xer_ca, std::int32_t(std::uint32_t(share.high) + std::uint32_t(share.scaled)) < 0 && ((std::uint32_t(share.high) + std::uint32_t(share.scaled)) & 63));
                                if (national_again) {
                                    c.set(0, std::uint32_t(share.scaled)); c.set(3, 0xb60b0000); const auto id = c.u32(person); c.set(5, id); c.set(4, std::uint32_t(share.high));
                                    const auto holder = c.toc(slot::contract_index); c.set(3, holder); const auto indices = c.u32(holder); c.set(3, indices); c.set(5, id * Contract::size);
                                    const auto offset = id * Contract::size + 9; c.set(6, offset); c.set(4, share.sign); const auto old = c.u8(indices + offset); c.set(5, old); c.set(0, std::uint32_t(old) + std::uint32_t(std::int32_t(share.share))); c.put8(indices + offset, std::uint32_t(old) + std::uint32_t(std::int32_t(share.share)));
                                    const auto current_holder = c.toc(slot::contract_index); c.set(3, current_holder); const auto current_id = c.u32(person); c.set(0, current_id); const auto current_indices = c.u32(current_holder); c.set(4, current_indices); c.set(3, current_id * Contract::size + 9);
                                    const auto current = c.i8(current_indices + current_id * Contract::size + 9); c.set(0, std::uint32_t(std::int32_t(current))); if (compare(c, current, 100) > 0) { c.set(0, 100); c.put8(current_indices + current_id * Contract::size + 9, 100); }
                                } else {
                                    const auto raw = c.u8(contract + 0x44); const auto score = std::int32_t(std::int8_t(raw)); c.set(0, std::uint32_t(score)); c.set(5, raw); const auto score_order = compare(c, score, 50);
                                    c.set(3, std::uint32_t(std::int32_t(share.share))); c.set(4, share.sign); auto updated = score + share.share;
                                    if (score_order > 0) { c.set(0, std::uint32_t(updated)); if (compare(c, updated, 100) > 0) { c.set(0, 100); c.put8(contract + 0x44, 100); } else if (compare(c, updated, -100) < 0) { c.set(0, 0xffffff9c); c.put8(contract + 0x44, 0xffffff9c); } else { c.set(0, std::uint32_t(raw) + std::uint32_t(std::int32_t(share.share))); c.put8(contract + 0x44, std::uint32_t(raw) + std::uint32_t(std::int32_t(share.share))); } }
                                    else { c.set(3, std::uint32_t(updated)); const auto above = compare(c, updated, 50); updated = std::int8_t(above > 0 ? 50 : updated); c.set(0, std::uint32_t(updated)); if (compare(c, updated, 100) > 0) { c.set(0, 100); c.put8(contract + 0x44, 100); } else if (compare(c, updated, -100) < 0) { c.set(0, 0xffffff9c); c.put8(contract + 0x44, 0xffffff9c); } else c.put8(contract + 0x44, std::uint32_t(updated)); }
                                }
                            }
                            bool skip_mood = signed_return(c, program.call(0x100c7d00, 0x1007cac4, {club}));
                            if (!skip_mood) { const auto id = c.i32(fixture); c.set(0, std::uint32_t(id)); if (compare(c, id) >= 0) skip_mood = signed_return(c, program.call(0x10494d20, 0x1007cae8, {c.u32(c.toc(-0x7b54)), c.u32(fixture + 0x14)})); }
                            if (!skip_mood) {
                                c.set(0, std::uint32_t(std::int32_t(index))); bool decide = compare(c, index, 11) >= 0;
                                if (!decide) { c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(minutes))))); decide = compare(c, std::int8_t(std::uint32_t(minutes)), 30) <= 0; if (decide) { const auto starter = c.i8(row + 0x1f); c.set(0, std::uint32_t(std::int32_t(starter))); decide = !nonzero(c, starter); } if (decide) { const auto fitness = c.i8(row + 0x20); c.set(0, std::uint32_t(std::int32_t(fitness))); decide = compare(c, fitness, 70) >= 0; } }
                                bool satisfied = true;
                                if (decide) {
                                    auto end_holder = c.toc(-0x7b88); c.set(3, end_holder); const auto old = c.u32(cursor); c.set(4, old); const auto last = c.u32(end_holder); c.set(0, last); const auto next = old + 2; c.set(3, next); interest = c.u8(contract + 0x4c);
                                    const bool wrap = compare_logical(c, next, last) > 0; c.put32(cursor, next);
                                    if (wrap) { c.put32(cursor, table); const auto entropy = program.call(0x1077b770, 0x1007cb4c); c.set(0, entropy & 0xffff); c.put32(salt, entropy & 0xffff); }
                                    const auto position = c.u32(cursor); c.set(4, position); c.set(3, 0x88890000); c.set(5, 0x88888889); end_holder = c.toc(-0x7b88); c.set(3, end_holder);
                                    const auto entry = c.u16(position); c.set(7, entry); const auto seed = c.u32(salt); c.set(6, seed); const auto last_again = c.u32(end_holder); c.set(0, last_again); const auto draw = std::uint32_t(entry) + seed; c.set(6, draw); c.set(4, position + 2); c.put32(cursor, position + 2);
                                    const auto high = multiply_high(std::int32_t(0x88888889), std::int32_t(draw)); c.set(3, std::uint32_t(high)); const bool wraps_again = compare_logical(c, position + 2, last_again) > 0;
                                    const auto floor = std::int32_t(std::uint32_t(high) + draw) >> 4; const auto quotient = std::uint32_t(floor) + (std::uint32_t(floor) >> 31); c.set(0, quotient * 30); c.set(3, std::uint32_t(floor) >> 31); sample = draw - quotient * 30;
                                    c.s.wr<1>(guest::reg::xer_ca, std::int32_t(std::uint32_t(high) + draw) < 0 && ((std::uint32_t(high) + draw) & 15));
                                    if (wraps_again) { c.put32(cursor, table); const auto entropy = program.call(0x1077b770, 0x1007cba8); c.set(0, entropy & 0xffff); c.put32(salt, entropy & 0xffff); }
                                    const auto signed_interest = std::int32_t(std::int8_t(std::uint32_t(interest))); c.set(3, std::uint32_t(signed_interest) ^ 0x80000000u); c.set(0, 0x43300000);
                                    const auto second_position = c.u32(cursor); c.set(5, second_position); c.put32(frame.at(0x1dc), std::uint32_t(signed_interest) ^ 0x80000000u); c.set(4, 0x88888889); c.set(7, std::uint32_t(sample) ^ 0x80000000u); c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(minutes)))) ^ 0x80000000u); c.put32(frame.at(0x1d8), 0x43300000);
                                    guest::Fpu fpu(c); const auto bias = c.f64(constants + 8); c.set_f(3, bias); const auto encoded_interest = c.f64(frame.at(0x1d8)); const auto divisor = c.f64(constants + 0x50); c.set_f(0, divisor);
                                    auto allowance = fpu.fsub(encoded_interest, bias); const auto second_entry = c.u16(second_position); c.set(6, second_entry); const auto second_seed = c.u32(salt); c.set(5, second_seed); c.put32(frame.at(0x1e4), std::uint32_t(sample) ^ 0x80000000u);
                                    allowance = fpu.fdiv(allowance, divisor); const auto second_draw = std::uint32_t(second_entry) + second_seed; c.set(6, second_draw); const auto second_high = multiply_high(std::int32_t(0x88888889), std::int32_t(second_draw)); const auto second_floor = std::int32_t(std::uint32_t(second_high) + second_draw) >> 4; const auto second_q = std::uint32_t(second_floor) + (std::uint32_t(second_floor) >> 31); c.set(4, second_q); c.set(5, std::uint32_t(second_floor) >> 31);
                                    c.put32(frame.at(0x1e0), 0x43300000); const auto base = c.f64(constants + 0x48); c.set_f(2, base); const auto encoded_first = c.f64(frame.at(0x1e0)); c.set(3, second_q * 30); c.put32(frame.at(0x1e8), 0x43300000);
                                    allowance = fpu.fsub(base, allowance); const auto first_random = fpu.fsub(encoded_first, bias); c.put32(frame.at(0x1d4), std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(minutes)))) ^ 0x80000000u); c.put32(frame.at(0x1d0), 0x43300000);
                                    allowance = fpu.fadd(allowance, first_random); c.set_f(1, allowance); c.set(0, (second_draw - second_q * 30) ^ 0x80000000u);
                                    const auto encoded_minutes = c.f64(frame.at(0x1d0)); c.put32(frame.at(0x1ec), (second_draw - second_q * 30) ^ 0x80000000u); const auto converted_minutes = fpu.fsub(encoded_minutes, bias); c.set_f(2, converted_minutes);
                                    const auto second_random = fpu.fsub(c.f64(frame.at(0x1e8)), bias); const auto required = fpu.fsub(allowance, second_random); c.set_f(0, required); fpu.fcmp(0, converted_minutes, required);
                                    c.set_cr(0, (c.cr(0) & ~2u) | ((c.cr(0) & 6u) ? 2u : 0u)); satisfied = converted_minutes >= required;
                                    c.s.wr<1>(guest::reg::xer_ca, std::int32_t(std::uint32_t(second_high) + second_draw) < 0 && ((std::uint32_t(second_high) + second_draw) & 15));
                                }
                                c.set(0, 0);
                                if (satisfied) {
                                    c.put8(contract + 0x45, 0); const auto old = c.u8(contract + 0x46); c.set(3, old); if (compare_logical(c, old, 100) != 0) { c.set(0, std::uint32_t(old) + 1); c.put8(contract + 0x46, std::uint32_t(old) + 1); }
                                    const auto streak = c.i8(contract + 0x46); c.set(0, std::uint32_t(std::int32_t(streak)));
                                    if (compare(c, streak, 5) >= 0) { const auto active = program.call(0x1014c2e0, 0x1007ccac, {club, 0, 1, 0, 0}); if (!signed_return(c, active)) program.call(0x100ce200, 0x1007ccc0, {person, 1}); }
                                    program.call(0x1007d530, 0x1007ccd4, {object, person, club, contract});
                                } else {
                                    c.put8(contract + 0x46, 0); const auto old = c.u8(contract + 0x45); c.set(3, old); if (compare_logical(c, old, 100) != 0) { c.set(0, std::uint32_t(old) + 1); c.put8(contract + 0x45, std::uint32_t(old) + 1); }
                                    decrease_score(c, contract, 4); program.call(0x1007d800, 0x1007cd44, {object, person, club, contract});
                                }
                            }
                            const auto current_side = c.i8(frame.at(0x1f0)); c.set(0, std::uint32_t(std::int32_t(current_side))); c.set(3, frame.at(0x3c)); c.set(4, std::uint32_t(std::int32_t(current_side)));
                            const auto won = c.i8(frame.at(0x3c) + std::uint32_t(std::int32_t(current_side))); c.set(0, std::uint32_t(std::int32_t(won))); const bool winner = nonzero(c, won);
                            bool loser = false; if (!winner) { c.set(0, 1u - std::uint32_t(std::int32_t(current_side))); c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(std::int32_t(current_side)) <= 1); const auto other = c.i8(frame.at(0x3c) + 1u - std::uint32_t(std::int32_t(current_side))); c.set(0, std::uint32_t(std::int32_t(other))); loser = nonzero(c, other); }
                            if (winner || loser) {
                                const auto adaptation = c.i8(person + 0x63); c.set(0, std::uint32_t(std::int32_t(adaptation))); sample = std::uint32_t(21 - std::int32_t(adaptation)); c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(std::int32_t(adaptation)) <= 21);
                                if (nonzero(c, std::int32_t(std::uint32_t(sample)))) minutes = match_sample(c, salt, table, cursor, std::int32_t(std::uint32_t(sample)), winner ? 0x1007cd9c : 0x1007cf64); else minutes = 0;
                                if (compare(c, std::int32_t(std::uint32_t(minutes)), 4) < 0 && nonzero_logical(c, contract)) change_interest(c, contract, winner ? 3 : -3);
                            }
                            c.set(5, 0); c.set(3, 0);
                            for (std::uint32_t slot = 0;; ++slot) {
                                c.set(0, slot); if (compare(c, std::int8_t(slot), 50) >= 0) break;
                                const auto base = frame.at(0x40) + std::uint32_t(roster_offset); c.set(4, base);
                                const auto saved = c.u32(base + slot * 4); c.set(0, saved);
                                if (compare_logical(c, saved, person) == 0) { c.set(0, slot * 4); c.set(3, 0); c.put32(base + slot * 4, 0); break; }
                                c.set(3, (slot + 1) * 4); c.set(5, slot + 1);
                            }
                        }
                    }
                    score_offset = std::uint32_t(score_offset) + 2; stat_offset = std::uint32_t(stat_offset) + 76; player_index = std::uint32_t(player_index) + 1; row = std::uint32_t(row) + 76;
                }
            }
            const auto current_offset = c.u32(frame.at(0x1f4)); c.set(3, current_offset); roster_offset = std::uint32_t(roster_offset) + 200;
            c.set(3, current_offset + 0x664); c.put32(frame.at(0x1f4), current_offset + 0x664);
            const auto current_side = c.u8(frame.at(0x1f0)); c.set(3, std::uint32_t(current_side) + 1); c.put8(frame.at(0x1f0), std::uint32_t(current_side) + 1);
        }
    }
    {
        Kept<17> side(c, 0); Kept<16> roster_offset(c, 0); Kept<18> club(c, c.r(18)); Kept<14> limit(c, c.r(14));
        Kept<19> index(c, c.r(19)); Kept<13> roster(c, c.r(13)); Kept<15> offset(c, c.r(15));
        Kept<20> person(c, c.r(20)); Kept<22> contract(c, c.r(22));
        for (;;) {
            c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(side))))); if (compare(c, std::int8_t(std::uint32_t(side)), 2) >= 0) break;
            nonzero(c, std::int8_t(std::uint32_t(side))); club = c.u32(fixture + (std::uint32_t(side) == 0 ? 0x1c : 0x20));
            const auto alternate = program.call(0x100ce320, 0x1007d198, {club, 0, 1}); const bool linked = nonzero_logical(c, alternate); limit = linked ? 14 : 8;
            const auto type = c.u8(fixture + 0x3a); c.set(0, type); bool skipped = false;
            if (compare(c, type, 1) == 0) {
                c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(side))))); nonzero(c, std::int8_t(std::uint32_t(side)));
                if (std::uint32_t(side) == 0) skipped = reputation_mismatch(c, frame, constants, club, c.u32(fixture + 0x20), false);
                if (!skipped) { c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(side))))); if (compare(c, std::int8_t(std::uint32_t(side)), 1) == 0) skipped = reputation_mismatch(c, frame, constants, club, c.u32(fixture + 0x1c), false); }
            }
            if (!skipped) {
                index = 0; roster = frame.at(0x40); offset = 0;
                for (;;) {
                    c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index))))); if (compare(c, std::int8_t(std::uint32_t(index)), 50) >= 0) break;
                    c.set(0, roster_offset + offset); person = c.u32(roster + roster_offset + offset); bool valid = nonzero_logical(c, person);
                    if (valid) { const auto holder = c.toc(slot::person_count); c.set(3, holder); const auto id = c.i32(person), count = c.i32(holder); c.set(4, std::uint32_t(id)); c.set(0, std::uint32_t(count)); valid = compare(c, id, count) < 0; }
                    if (valid) { const auto attributes = c.u32(person + 0x14); c.set(0, attributes); valid = nonzero_logical(c, attributes); }
                    if (valid) valid = !signed_return(c, program.call(0x100c7d00, 0x1007d324, {club}));
                    if (valid) { contract = program.call(0x10082f90, 0x1007d33c, {object, person, club}); valid = nonzero(c, std::int32_t(std::uint32_t(contract))); }
                    if (valid) { const auto id = c.i32(fixture); c.set(0, std::uint32_t(id)); if (compare(c, id, -2) != 0) valid = !signed_return(c, program.call(0x100e7920, 0x1007d36c, {c.u32(c.toc(-0x7aa4)), person, c.u32(fixture + 0x14), 1, 0})); }
                    if (valid) valid = !signed_return(c, program.call(0x10187680, 0x1007d384, {c.u32(c.toc(-0x7ba0)), person}));
                    if (valid) { const auto unavailable = program.call(0x102b7c70, 0x1007d3ac, {c.u32(c.toc(-0x7a64)), person, c.toc(-0x7b80), club, 0, 0}); valid = !nonzero_logical(c, unavailable); }
                    if (valid) { const auto secondary = program.call(0x10083110, 0x1007d3c0, {object, person}); valid = !nonzero_logical(c, secondary); }
                    if (valid) {
                        c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(side))))); nonzero(c, std::int8_t(std::uint32_t(side)));
                        const auto opponent = c.u32(fixture + (std::uint32_t(side) == 0 ? 0x20 : 0x1c)); c.set(6, opponent);
                        const auto holder = c.toc(-0x7b3c); c.set(3, holder); const auto league = c.u32(holder); c.set(8, league); c.set(3, 0);
                        guest::Fpu fpu(c); const auto zero = c.f64(constants + 0x60), denominator = c.f64(league + 0x18); c.set_f(0, zero); c.set_f(1, denominator); fpu.fcmp(0, denominator, zero);
                        double rating;
                        if (denominator == zero) rating = c.f64(constants + 0x58);
                        else { const auto numerator = c.f64(league + 0x10); c.set_f(0, numerator); rating = fpu.fdiv(numerator, denominator); }
                        const auto baseline = c.f64(constants + 0x68); c.set_f(0, baseline); c.set_f(1, fpu.frsp(fpu.fsub(rating, baseline)));
                        program.call(0x101f9470, 0x1007d42c, {0, person, club, opponent, fixture, league, 0, 0});
                        c.set(0, 0); c.put8(contract + 0x46, 0); const auto old = c.u8(contract + 0x45); c.set(3, old);
                        if (compare_logical(c, old, 100) != 0) { c.set(0, std::uint32_t(old) + 1); c.put8(contract + 0x45, std::uint32_t(old) + 1); }
                        const auto streak = c.i8(contract + 0x45); c.set(3, std::uint32_t(std::int32_t(streak))); c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(limit)))));
                        if (compare(c, streak, std::int8_t(std::uint32_t(limit))) >= 0) {
                            if (!signed_return(c, program.call(0x1014c2e0, 0x1007d474, {club, 0, 1, 0, 0}))) {
                                const auto alternate = program.call(0x100ce320, 0x1007d48c, {club, 0, 1});
                                if (!nonzero_logical(c, alternate)) program.call(0x1007ee50, 0x1007d4a4, {object, person, club});
                            }
                        }
                        decrease_score(c, contract, 8); program.call(0x1007d800, 0x1007d4f4, {object, person, club, contract});
                    }
                    offset = std::uint32_t(offset) + 4; index = std::uint32_t(index) + 1;
                }
            }
            roster_offset = std::uint32_t(roster_offset) + 200; side = std::uint32_t(side) + 1;
        }
    }
}
}
