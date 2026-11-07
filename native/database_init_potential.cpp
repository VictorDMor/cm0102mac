// Player Initialisation, the player record: the age and its date, the potential ability (r28), the current
// ability (r15) and the store of the three abilities, then the call that copies the attributes to slots.
#include "database_generation.hpp"
#include "database_init_detail.hpp"
#include "database_lookups.hpp"
#include "database_records.hpp"

namespace cm::aot::database::init {
namespace {
}

// The date-of-birth fix shared by the player and the staff paths: a row whose age is out of range gets a
// contract date drawn (day below 27 plus one, month below 11) in the year of its +0x5c field, or in a fixed
// year, through 0x107158a0.
void Gen::fix_contract_date(std::uint32_t p, std::int32_t year_field, bool use_field, std::uint32_t fixed_year,
                            std::uint32_t back_field, std::uint32_t back_fixed) {
    g[13] = u32(rem_s(next(), 27));
    const auto month_sample = next();
    const auto month = rem_s(month_sample, 11);
    c.set(8, u32(month_sample));  // add r8,r8,r7: the sample sum stays in r8 across the call
    c.set({{3, p + person::contract_date}, {4, ext16(std::int32_t(g[13]) + 1)}, {5, u32(s8(u32(month)))},
           {6, use_field ? (u32(year_field) & 0xffff) : fixed_year}, {7, 0xffffffff}});
    call(0x107158a0, use_field ? back_field : back_fixed);
}

// 100bb9f0 to 100bd3a8.
void Gen::player_age_and_dates(std::uint32_t p, std::uint32_t player) {
    auto& current = g[15];
    auto& potential = g[28];
    auto& luck = g[13];
    auto& scale = g[16];
    auto& age = g[23];
    const auto club = [&] { return c.u32(p + person::club); };
    const auto valid = [&](std::uint32_t r) { return in_rating_range(r, 1, 200); };

    enter(0x100bb9fc, {p, date});
    age = date_difference(c, p, date);
    if (s8(age) < 0xe || s8(age) > 0x32) {
        const auto year_field = s16(c.u16(p + 0x5c));
        if (year_field > 0x76c) {
            fix_contract_date(p, year_field, true, 0, 0x100bbaf4, 0);
        } else if (c.u32(p + person::staff) != 0) {
            fix_contract_date(p, year_field, false, 0x7af, 0, 0x100bbbd0);
        } else {
            fix_contract_date(p, year_field, false, 0x7bc, 0, 0x100bbca0);
        }
        enter(0x100bbcac, {p, date});
        age = date_difference(c, p, date);
    }
    c.put16(p + person::age, u32(s8(age)));
    // 100bbcb8..100bbdd0: a block guarded by "a club and no club" (dead: both branches skip it).

    // The potential: after the settling (100bc030) it is the larger of the current ability and the candidate,
    // held to 10-200.
    const auto settle = [&](std::uint32_t candidate) {
        potential = ext16(s16(current) > s16(candidate) ? s16(current) : s16(candidate));
        potential = ext16(s16(potential) >= 10 ? s16(potential) : 10);
        potential = ext16(s16(potential) > 200 ? 200 : s16(potential));
    };
    if (s16(potential) == -2) {
        if (rem_s(next(), 10) != 0) potential = ext16(rem_s(next(), 81) + 0x78);
        else potential = ext16(rem_s(next(), 41) + 0xa0);
        if (rem_s(next(), 20) == 0) potential = potential - ext16(rem_s(next(), 50));
        else if (s16(potential) > 0xa5) potential = potential - ext16(rem_s(next(), 30));
        settle(potential);
    } else {
        bool minus_one = s16(potential) == -1;
        if (!minus_one && s16(potential) > 0x8c) {
            if (s8(age) < rem_s(next(), 30) && s16(current) < 0xaa) {
                const auto bound = 0x271a - s8(g[27]);
                scale = bound == 0 ? 0 : u32(below(bound));
                minus_one = scale == 0;
            }
        }
        if (minus_one) {
            if (rem_s(next(), 10) != 0) potential = ext16(rem_s(next(), 81) + 0x50);
            else potential = ext16(rem_s(next(), 81) + 0x78);
            if (rem_s(next(), 20) == 0) potential = potential - ext16(rem_s(next(), 50));
            else if (s16(potential) > 0x8c) potential = potential - ext16(rem_s(next(), 30));
            settle(potential);
        } else {
            bool recompute = true;
            if (valid(potential)) {
                recompute = false;
                if (s8(age) < 0x19 && rem_s(next(), 25) == 0) recompute = true;
            }
            if (recompute) {
                if (rem_s(next(), 100) == 0) {
                    luck = u32(s8(u32(rem_s(next(), 21))));
                } else if (rem_s(next(), 2) == 0) {
                    luck = u32(s8(u32(rem_s(next(), 6) + 6)));
                } else if (rem_s(next(), 2000) == 0) {
                    luck = u32(s8(u32(rem_s(next(), 20) + 1)));
                } else {
                    luck = u32(s8(u32(rem_s(next(), 7) + 4)));
                }
                if (!(s8(luck) >= 1 && s8(luck) <= 20)) luck = u32(s8(u32(rem_s(next(), 10) + 3)));
                const auto sample = next();
                const auto table = c.toc(-0x5634);
                const auto base = s16(c.u16(table + 2 * u32(s8(luck))));
                scale = ext16(base + rem_s(sample, 60) - 0x1e);
                settle(scale);
            }
        }
    }
    // 100bc998: the current ability when outside 1-200.
    if (!valid(current)) {
        luck = u32(s8(u32(rem_s(next(), 10) + 0x14)));
        current = s16(potential) == 0 ? 0 : u32(below(s16(potential)));
        scale = s16(current) >= 5 ? u32(s16(current)) : 5;
        const auto bound = div_s((s16(potential) >> 2) * std::int32_t(scale), 200)  /* 0x51eb851f >> 6 */;
        current = bound == 0 ? 0 : u32(below(bound));
        const auto quotient = div_s(0x2710 - s8(g[27]), 200);
        const auto difference = s8(luck) - s8(age);
        scale = ext16((s16(ext16(quotient)) << 1) + s16(current) + 3);
        c.set(3, u32(difference));
        call(0x10775540, 0x100bce9c);
        auto distance = std::int32_t(c.r(3));
        if (distance >= 3) {
            c.set(3, u32(s8(luck) - s8(age)));
            call(0x10775540, 0x100bcebc);
            distance = std::int32_t(c.r(3));
        } else {
            distance = 3;
        }
        const auto step = s16(ext16(div_s(s16(scale) * distance, 5)));
        const auto lowered = s16(potential) - step;
        current = ext16(lowered > 200 ? 200 : s16(potential) - step);
        current = ext16(s16(current) >= 1 ? s16(current) : 1);
        if (s16(current) == 1 && rem_s(next(), 10) != 0) {
            const auto fifth = div_s(s16(potential), 5);
            luck = fifth == 0 ? 0 : u32(below(fifth));
            current = ext16(std::int32_t(luck) + 1);
        }
    }
    // 100bd1e0: a row without a club loses a tenth of the current ability, up to five times over.
    if (club() == 0) {
        bool skip = false;
        if (c.u8(p + person::role_flag) != 0) {
            enter(0x100bd204, {p, date});
            date_difference(c, p, date);
            skip = s8(c.r(3)) <= 0x20;
        }
        if (!skip) {
            const auto sample = next();
            const auto tenth = scaled(s16(current), 0x60, 0x2ac, 0x2b4, true);
            current = ext16(s16(current) - s16(u32(rem_s(sample, 6) * tenth)));
            const auto held = s16(current) < 1 ? 1 : (s16(current) > 200 ? 200 : s16(current));
            current = ext16(held);
        }
    }
    // 100bd2d0
    if (s16(current) > s16(potential)) {
        current = ext16(s16(potential) - rem_s(next(), 40));
        if (s16(current) < 1) current = ext16(rem_s(next(), 5) + 1);
    }
    // 100bd3a8: the abilities go back to the record (scaled by 50), the table row's byte is marked and
    // the attributes are copied into slots.
    c.put16(player + 0x4, current);
    c.put16(player + 0x6, potential);
    c.put16(player + 0xa, g[24] * 0x32);
    c.put16(player + 0x8, g[25] * 0x32);
    c.put16(player + 0xc, g[26] * 0x32);
    c.set(10, u32(-0x64));
    const auto table = c.u32(c.toc(-0x7b9c));
    c.put8(table + c.u32(p + person::id) * 100 + 9, 0x9c);
    enter(0x100bd400, {player, p, at(slot_array), at(0x39), at(0x3a), at(0x38)});
    copy_all_attributes_to_slots(c, player, p, at(slot_array), at(0x39), at(0x3a), at(0x38));
    player_attributes(p, player);
}
}
