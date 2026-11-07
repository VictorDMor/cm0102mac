// Player Initialisation, the staff record's attribute slots, its trait bytes and its role.
#include "database_generation.hpp"
#include "database_init_detail.hpp"
#include "database_records.hpp"

namespace cm::aot::database::init {
namespace {
}

// 100c0c9c to 100c1970.
void Gen::staff_slots(std::uint32_t p) {
    auto& luck = g[13];
    auto& index = g[16];
    auto& staff = g[24];
    auto& candidate = g[23];
    const auto slot16 = [&](std::uint32_t base) { return s16(c.u16(at(base + 4 * index))); };
    const auto set_slot = [&](std::int32_t v) { c.put16(at(0x1b0 + 4 * index), u32(v)); };
    const auto club = [&] { return c.u32(p + person::club); };
    // The bound on a drawn slot value: below 1 a draw of 1-8, above 20 twenty.
    const auto store_bounded = [&] {
        if (s8(luck) < 1) set_slot(s8(u32(rem_s(next(), 8) + 1)));
        else if (s8(luck) > 0x14) set_slot(0x14);
        else set_slot(s8(luck));
    };
    const auto in_slot_range = [&](std::int32_t v) { return v >= 1 && v <= 0x14; };

    enter(0x100c0cb0, {staff, p, at(0x1b0), 0});
    copy_attributes_to_slots(c, staff, at(0x1b0), 0);
    bool all_valid = true;
    for (index = 0; slot16(0x1b0) != -0x7f; index = index + 1) {
        if (!in_slot_range(slot16(0x1b0))) {
            all_valid = false;
            break;
        }
    }
    if (all_valid) {
        enter(0x100c1970, {staff, p, at(0x1b0)});
        copy_attributes_from_slots(c, staff, at(0x1b0));
        return;
    }
    candidate = 0;
    const auto reputation = [&] { return s16(c.u16(club() + 0x1ec)); };
    const auto nation_is = [&](std::int32_t toc_offset) {
        return c.u32(p + person::nation) != 0 &&
               c.u32(c.u32(p + person::nation) + 0x68) == c.u32(c.toc(toc_offset));
    };
    // 100c0d18: a role-5 staff member of a club with a reputation above 6250 gets its trait bytes drawn.
    if (c.u8(p + person::role) == 5 && club() != 0 && reputation() > 0x186a) {
        const auto byte = [&](std::uint32_t offset) { return s8(c.u8(staff + offset)); };
        const auto bad = [&](std::uint32_t offset) { return !(byte(offset) >= 1 && byte(offset) <= 20); };
        if (bad(0x33)) {
            if (nation_is(-0x7a14)) c.put8(staff + 0x33, u32(rem_s(next(), 16) + 1));
            else c.put8(staff + 0x33, u32(rem_s(next(), 14) + 7));
        }
        if (bad(0x2e)) {
            if (reputation() > 0x1e46) {
                if (nation_is(-0x7a14)) c.put8(staff + 0x2e, u32(rem_s(next(), 4) + 0xd));
                else c.put8(staff + 0x2e, u32(rem_s(next(), 8) + 0xd));
            } else {
                c.put8(staff + 0x2e, u32(rem_s(next(), 9) + 0xc));
            }
        }
        if (bad(0x3b)) c.put8(staff + 0x3b, u32(rem_s(next(), 13) + 8));
        if (bad(0x3f)) c.put8(staff + 0x3f, u32(rem_s(next(), 13) + 8));
        if (bad(0x41)) {
            if (reputation() > 0x1e46) {
                if (nation_is(-0x7a14)) c.put8(staff + 0x41, u32(rem_s(next(), 7) + 0xe));
                else c.put8(staff + 0x41, u32(rem_s(next(), 9) + 0xc));
            } else {
                c.put8(staff + 0x41, u32(rem_s(next(), 11) + 0xa));
            }
        }
    }
    // 100c1230: the closest other staff record over 250 draws of a row.
    for (index = 0; s16(index) < 0xfa; index = index + 1) {
        g[15] = c.u32(count_word);
        luck = u32(below_wide(std::int32_t(g[15]), {0x2ac, 0x2b4, 0x2bc}, 0x100c1344));
        const auto row = c.u32(c.toc(-0x7be0)) + luck * 0x70;
        luck = c.u32(row + person::staff);
        if (luck != 0 && s16(c.u16(luck + 0x24)) >= 1 && s16(c.u16(luck + 0x24)) <= 0xc8) {
            enter(0x100c13fc, {staff, p, luck});
            c.set(6, row);
            const auto distance = attribute_distance(c, staff, luck, row);
            if (distance < std::int32_t(g[29]) || candidate == 0) {
                candidate = luck;
                g[29] = u32(distance);
            }
        }
    }
    if (candidate != 0) {
        enter(0x100c1444, {candidate, p, at(slot_array_b), 0});
        copy_attributes_to_slots(c, candidate, at(slot_array_b), 0);
        g[15] = u32(s16(c.u16(staff + 0x24)));
        if (!(s16(g[15]) >= 1 && s16(g[15]) <= 200)) g[15] = ext16(rem_s(next(), 100) + 1);
        candidate = u32(s16(c.u16(candidate + 0x24)));
        if (!(s16(candidate) >= 1 && s16(candidate) <= 200)) candidate = g[15];
        for (index = 0; slot16(0x1b0) != -0x7f; index = index + 1) {
            if (in_slot_range(slot16(0x1b0))) continue;
            const auto other_value = slot16(slot_array_b);
            if (in_slot_range(other_value)) {
                const auto delta = s8(u32(div_s(s16(g[15]) - s16(candidate), 10)));
                luck = u32(s8(u32(delta + other_value)));
                luck = luck + u32(s8(u32(rem_s(next(), 6) - 3)));
            } else {
                const auto sample = next();
                luck = u32(s8(u32((rem_s(sample, 6) + div_s(s16(c.u16(staff + 0x24)), 10)) - 3)));
            }
            store_bounded();
        }
        enter(0x100c17dc, {staff, p, at(0x1b0)});
        copy_attributes_from_slots(c, staff, at(0x1b0));
    } else {
        for (index = 0; slot16(0x1b0) != -0x7f; index = index + 1) {
            if (in_slot_range(slot16(0x1b0))) continue;
            const auto sample = next();
            luck = u32(s8(u32((rem_s(sample, 7) + div_s(s16(c.u16(staff + 0x24)), 10)) - 3)));
            store_bounded();
        }
        enter(0x100c195c, {staff, p, at(0x1b0)});
        copy_attributes_from_slots(c, staff, at(0x1b0));
    }
}

// 100c1970: the role at +0x62 (9, 8 or 10) sets the staff's trait bytes, each lowered to a draw when above its limit.
void Gen::staff_traits(std::uint32_t p) {
    auto& staff = g[24];
    const auto byte = [&](std::uint32_t offset) { return s8(c.u8(staff + offset)); };
    // A byte above `limit` is redrawn as sample % modulus + 1 (the other traits' usual 1-5 or 1-8).
    const auto lower = [&](std::uint32_t offset, std::int32_t limit, std::int32_t modulus) {
        if (byte(offset) > limit) c.put8(staff + offset, u32(rem_s(next(), modulus) + 1));
    };
    const auto lower_all_to_five = [&](std::initializer_list<std::uint32_t> offsets) {
        for (const auto offset : offsets) lower(offset, 5, 5);
    };
    const auto role = c.u8(p + person::role);
    if (role == 9) {
        if (rem_s(next(), 100) != 0) {
            c.put8(staff + 0x2f, 1);
            c.put8(staff + 0x3e, 1);
            if (byte(0x37) < 5) c.put8(staff + 0x37, u32(rem_s(next(), 5) + 4));
            if (byte(0x38) < 4) c.put8(staff + 0x38, u32(rem_s(next(), 5) + 3));
            lower(0x30, 8, 8);
            lower(0x31, 8, 8);
            lower_all_to_five({0x32, 0x33, 0x3a});
            return;
        }
    }
    if (c.u8(p + person::role) == 8) {
        if (rem_s(next(), 100) != 0) {
            c.put8(staff + 0x2f, 1);
            c.put8(staff + 0x3e, 1);
            return;
        }
    }
    if (c.u8(p + person::role) == 10) {
        if (rem_s(next(), 100) != 0) {
            c.put8(staff + 0x2f, 1);
            if (byte(0x3e) < 5) c.put8(staff + 0x3e, u32(rem_s(next(), 10) + 4));
            lower_all_to_five({0x30, 0x31, 0x32, 0x33, 0x37, 0x38, 0x3a});
        }
    }
}

// 100c2150: well-known names fix the staff's type byte (+0x43); otherwise a type is drawn when unset or
// large. The staff record's words +0x4..+0x20 are cleared and a row without a role gets one from its bytes.
void Gen::staff_names_and_role(std::uint32_t p) {
    auto& staff = g[24];
    const auto byte = [&](std::uint32_t offset) { return s8(c.u8(staff + offset)); };
    const auto pool = c.toc(-0x5618);
    // 0x1036ff00 compares the row's name with a string of the pool.
    const auto named = [&](std::uint32_t offset, std::uint32_t back) {
        c.set({{3, p}, {4, pool + offset}});
        call(0x1036ff00, back);
        return s8(c.r(3)) != 0;
    };
    const auto finish = [&] {
        // 100c2390: li r0,0; li r3..r7,0 (r7 = 0 reaches the next row's assert sprintf spill).
        c.set({{0, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}});
        for (std::uint32_t offset = 4; offset <= 0x20; offset += 4) c.put32(staff + offset, 0);
        if (s8(c.u8(p + person::role)) != 0 || s8(c.u8(p + person::role_byte)) != 0) return;
        if (c.u32(p + person::player) != 0) {
            c.put8(p + person::role, 0xb);
        } else if (byte(0x41) > 10 && byte(0x3b) > 10) {
            c.put8(p + person::role, 5);
        } else if (byte(0x30) <= 6 && byte(0x31) <= 6) {
            if (byte(0x30) > 6 || byte(0x31) > 6) c.put8(p + person::role, 8);
            else c.put8(p + person::role, byte(0x3e) > 10 ? 0xa : 9);
        } else {
            // (dead: byte +0x3b and +0x41 above 10 were taken above)
            if (byte(0x3b) > 10 && byte(0x41) > 10) c.put8(p + person::role, 6);
            else c.put8(p + person::role, 8);
        }
    };
    if (named(0x8cd, 0x100c2160)) {
        c.put8(staff + 0x43, 3);
        c.put8(staff + 0x33, 7);
        c.put8(staff + 0x2e, 0x11);
        return finish();
    }
    if (named(0x8db, 0x100c2194) || named(0x8e9, 0x100c21ac) || named(0x8f5, 0x100c21c4) || named(0x903, 0x100c21dc)) {
        c.put8(staff + 0x43, 3);
        return finish();
    }
    if (named(0x90f, 0x100c2200) || named(0x91c, 0x100c2218) || named(0x92b, 0x100c2230) || named(0x938, 0x100c2248) ||
        named(0x945, 0x100c2260)) {
        c.put8(staff + 0x43, 2);
        return finish();
    }
    if (named(0x94f, 0x100c2284)) {
        c.put8(staff + 0x43, 6);
        return finish();
    }
    if (byte(0x43) == 0 || byte(0x43) >= 10) {
        // The type from a draw of 0-25 through the compiler's switch table (9 types).
        static constexpr std::uint8_t types[26] = {0, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 8};
        const auto drawn = rem_s(next(), 26);
        static constexpr std::uint32_t targets[26]{0x100c2328, 0x100c2334, 0x100c2334, 0x100c2340, 0x100c2340, 0x100c2340,
                                                   0x100c2340, 0x100c2340, 0x100c2340, 0x100c234c, 0x100c234c, 0x100c234c,
                                                   0x100c234c, 0x100c234c, 0x100c234c, 0x100c2358, 0x100c2358, 0x100c2364,
                                                   0x100c2364, 0x100c2364, 0x100c2370, 0x100c2370, 0x100c2370, 0x100c237c,
                                                   0x100c237c, 0x100c2388};
        if (std::uint32_t(drawn) <= 25) {
            c.s.wr<4>(guest::reg::ctr, targets[drawn]);  // mtspr CTR,r3; bctr
            c.put8(staff + 0x43, types[drawn]);
        }
    }
    finish();
}
}
