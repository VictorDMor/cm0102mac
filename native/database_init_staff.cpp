// Player Initialisation, the staff record (+0x1c) of a person and the age of a row without a player record.
#include "database_generation.hpp"
#include "database_init_detail.hpp"
#include "database_records.hpp"

namespace cm::aot::database::init {
namespace {
// Staff record fields (inferred): three 16-bit values scaled by 50, two ratings, and the trait bytes.
constexpr std::uint32_t field_24 = 0x24, field_26 = 0x26, field_28 = 0x28, field_2a = 0x2a, field_2c = 0x2c;
}

// 100c033c: a row without a player record: the age is mended like a player's, with a wider range (29 to 75).
void Gen::staff_only(std::uint32_t p) {
    auto& luck = g[13];
    auto& age = g[23];
    enter(0x100c0348, {p, date});
    age = date_difference(c, p, date);
    if (s8(age) < 0x1d || s8(age) > 0x4b) {
        if (s16(c.u16(p + 0x5c)) > 0x76c && s16(c.u16(p + 0x5c)) < 0x7af) {
            fix_contract_date(p, s16(c.u16(p + 0x5c)), true, 0, 0x100c044c, 0);
        } else {
            luck = u32(rem_s(next(), 27));
            g[15] = u32(rem_s(next(), 11));
            const auto year_sample = next();
            const auto year = u32(rem_s(year_sample, 26) + 0x794) & 0xffff;
            c.set(8, u32(year_sample));
            c.set({{3, p + person::contract_date}, {4, ext16(std::int32_t(luck) + 1)}, {5, u32(s8(g[15]))}, {6, year},
                   {7, 0xffffffff}});
            call(0x107158a0, 0x100c0578);
        }
    }
    enter(0x100c0584, {p, date});
    age = u32(s8(u32(date_difference(c, p, date))));
    c.put16(p + person::age, age);
}

// 100c058c to 100c2390 and the role at +0x62 after it.
void Gen::staff_record(std::uint32_t p) {
    auto& luck = g[13];
    auto& staff = g[24];
    staff = c.u32(p + person::staff);
    if (staff == 0) return;
    const auto field = [&](std::uint32_t offset) { return s16(c.u16(staff + offset)); };
    const auto set_field = [&](std::uint32_t offset, std::int32_t v) { c.put16(staff + offset, u32(v)); };
    const auto club = [&] { return c.u32(p + person::club); };
    const auto birth_club = [&] { return c.u32(p + person::birth_club); };
    const auto nation = [&] { return c.u32(p + person::nation); };
    const auto byte_at = [&](std::uint32_t offset) { return s8(c.u8(staff + offset)); };
    const auto fits = [&](std::int32_t v) { return v > 0 && v <= 0x2710; };
    const auto hold = [&](std::uint32_t offset) {  // 1 to 0x2710
        const auto v = field(offset);
        set_field(offset, v < 1 ? 1 : v > 0x2710 ? 0x2710 : v);
    };
    const auto reputation = [&](std::uint32_t holder) { return s16(c.u16(holder + 0x1ec)); };
    const auto scaled_reputation = [&](std::uint32_t holder, std::uint32_t k) {
        return scaled(reputation(holder), k, 0x2ac, 0x2b4);
    };
    set_field(field_2a, field(field_2a) * 0x32);
    set_field(field_2c, field(field_2c) * 0x32);
    set_field(field_28, field(field_28) * 0x32);
    if (!fits(field(field_2c))) {
        if (club() != 0 && reputation(club()) > 0x1676) set_field(field_2c, scaled_reputation(club(), 0x68));
        else if (birth_club() != 0) set_field(field_2c, scaled_reputation(birth_club(), 0x48));
        else set_field(field_2c, 1);
    }
    hold(field_2c);
    if (!fits(field(field_2a))) {
        if (fits(field(field_28))) set_field(field_2a, field(field_28));
        else if (fits(field(field_2c))) set_field(field_2a, field(field_2c));
        else if (club() != 0) set_field(field_2a, reputation(club()));
        else if (birth_club() != 0) set_field(field_2a, scaled_reputation(birth_club(), 0x70));
        else set_field(field_2a, 1);
    }
    hold(field_2a);
    if (!fits(field(field_28))) {
        if (fits(field(field_2a))) set_field(field_28, field(field_2a));
        else if (fits(field(field_2c))) set_field(field_28, field(field_2c));
        else if (club() != 0) set_field(field_28, scaled_reputation(club(), 0x78));
        else if (birth_club() != 0) set_field(field_28, scaled_reputation(birth_club(), 0x70));
        else set_field(field_28, 1);
    }
    hold(field_28);
    if (club() == 0 || c.u32(club() + 0x2c) == nation()) set_field(field_28, field(field_2a));
    if (!(field(field_24) > 0 && field(field_24) <= 0xc8)) {
        if (club() != 0) set_field(field_24, div_s(reputation(club()), 50));
        else if (birth_club() != 0) set_field(field_24, div_s(reputation(birth_club()), 50));
        else set_field(field_24, rem_s(next(), 100) + 1);
    }
    if (!(field(field_26) > 0 && field(field_26) <= 0xc8)) {
        const auto drawn = s16(u32(rem_s(next(), 200) + 1));
        set_field(field_26, field(field_24) > drawn ? field(field_24) : drawn);
    }
    // The nation's id against six well-known ones sets the byte +0x33.
    const auto nation_is = [&](std::int32_t toc_offset) {
        return c.u32(nation() + 0x68) == c.u32(c.toc(toc_offset));
    };
    if (nation() != 0 && byte_at(0x33) == 0) {
        bool listed = false;
        for (const auto offset : {-0x7574, -0x7a10, -0x7a1c, -0x7578, -0x757c, -0x7504}) {
            if (nation_is(offset)) {
                listed = true;
                break;
            }
        }
        if (listed) {
            luck = u32(rem_s(next(), 8));
            c.put8(staff + 0x33, (luck + 0xd) - u32(rem_s(next(), 14)));
        } else {
            luck = u32(rem_s(next(), 7));
            c.put8(staff + 0x33, (luck + 5) - u32(rem_s(next(), 5)));
        }
    }
    staff_slots(p);
    staff_traits(p);
    staff_names_and_role(p);
}
}
