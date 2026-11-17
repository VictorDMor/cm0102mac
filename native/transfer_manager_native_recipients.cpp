#include "transfer_manager_native_recipients.hpp"
#include "transfer_manager_native_recipients_held.hpp"
#include "guest_fpu.hpp"
#include "transfer_manager_native_detail.hpp"
#include <algorithm>
#include <array>
#include <bit>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20;   // (contract manager, person) -> contract or 0
constexpr std::uint32_t club_takes_news = 0x102cdbf0; // (news manager, club) -> byte: the club may be given the item
constexpr std::uint32_t player_value = 0x10303560;    // (stats, profile, 2, 0x12; f1) -> f1
constexpr std::uint32_t iterator_begin = 0x10738c00, iterator_end = 0x10738c20, iterator_next = 0x10738e00;
constexpr std::uint32_t shortlist_clubs = 0x10362ef0;

// Kinds of recipient a caller asks for (bits of the third argument).
constexpr std::uint32_t kind_own_club = 0x2, kind_listed = 0x8, kind_derived = 0x10, kind_high_tier = 0x200,
                        kind_by_value = 0x80, kind_by_class = 0x40;
constexpr std::uint32_t manager_shortlist = 0x988;
constexpr std::uint32_t iterator_at = 0x48;
constexpr std::int32_t news_cell = -0x7bf0, contracts_cell = -0x7b7c, found_cell = -0x7b78, clubs_cell = -0x7be8,
                       people_cell = -0x7be0, people_count_cell = -0x7be4, stats_cell = -0x7b3c, float_pool_cell = -0x4ba4,
                       double_pool_cell = -0x4ba0, strings_cell = -0x4b9c, region_a_cell = -0x7a10, region_b_cell = -0x7a1c;

// The contract's kind: a signed six-bit field at byte +0x38 (bits 2-7), compared with 5 (a staff contract?).
bool contract_kind_is(const Cpu& c, std::uint32_t contract, std::int8_t kind) {
    const auto shifted = std::int32_t(std::uint32_t(c.u8(contract + 0x38)) << 24 & 0xfe000000u) >> 26;
    return std::int8_t(shifted) == kind;
}
}

std::uint32_t news_recipients(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t kinds,
                              std::uint32_t out, std::uint32_t capacity) {
    constexpr std::uint32_t club_contract = 4, person_nation = 0xc, person_profile = 0x14, person_staff = 0x1c,
                            person_record = 0x24, nation_region = 0x74, nation_group = 0x68, club_league = 0x30,
                            club_rival = 0x2c, club_class = 0x18;
    Frame frame(c, 0x103fc710, 0xf0, {.first_gpr = 13});
    RecipientsHeld r(c);
    guest::Fpu fpu(c);
    // The caller's parameter slot receives the kinds word.
    c.put32(frame.caller() + 0x20, kinds);
    const auto clubs = c.toc(clubs_cell), found = c.toc(found_cell);
    r[13] = c.toc(news_cell);
    r[14] = manager;
    r[15] = person;
    r[16] = out;
    r[17] = capacity;
    r[26] = 0;
    r[27] = c.toc(region_b_cell);
    r[28] = c.toc(region_a_cell);
    r[29] = found;
    r[30] = clubs;
    r[31] = c.toc(strings_cell);
    const auto strings = r[31];
    const auto club_at = [&](std::uint32_t contract) { return c.u32(clubs) + c.u32(contract + club_contract) * club_size; };
    const auto look_up_contract = [&](std::uint32_t who, std::uint32_t back) {
        r.publish(c);
        const auto contract = invoke(c, find_contract, back, {c.u32(c.toc(contracts_cell)), who});
        c.put32(found, contract);
        return contract;
    };
    // The club of the contract found for `who` (0 without one).
    const auto contract_club = [&](std::uint32_t who, std::uint32_t back) {
        const auto contract = look_up_contract(who, back);
        return contract ? club_at(contract) : 0;
    };
    // The same, 0 too when the contract's kind is 5; the original keeps the verdict in register `flag`
    // (cleared before the call).
    const auto member_club = [&](std::uint32_t who, std::uint32_t back, unsigned flag) {
        r[flag] = 0;
        const auto contract = look_up_contract(who, back);
        if (contract && !contract_kind_is(c, contract, 5)) r[flag] = 1;
        return r[flag] ? club_at(contract) : 0;
    };
    const auto takes_news = [&](std::uint32_t club, std::uint32_t back) {
        r.publish(c);
        return std::int8_t(invoke(c, club_takes_news, back, {c.u32(c.toc(news_cell)), club})) != 0;
    };
    const auto add = [&](std::uint32_t club) {
        c.put32(out + (std::uint32_t(std::int32_t(std::int8_t(r[26]))) << 2), club);
        ++r[26];
    };
    const auto full_report = [&](std::uint32_t at, std::uint32_t name, std::uint32_t extension, unsigned line) {
        guest::report_assertion(c, {at, strings + 0x37, strings + 0x6ff, strings + 0x70c, frame.at(name), frame.at(extension), line},
                                [&](std::uint32_t target, std::uint32_t back) {
                                    r.publish(c);
                                    c.call(target, back);
                                });
    };

    r.publish(c);
    invoke(c, iterator_begin, 0x103fc758, {frame.at(iterator_at)});

    if (kinds & kind_own_club) {
        if (contract_club(person, 0x103fc774)) {
            const auto club = contract_club(person, 0x103fc7b0);
            if (takes_news(club, 0x103fc7dc)) {
                const auto first = contract_club(person, 0x103fc7f4);
                r[26] = 1;
                c.put32(out, first);
            }
        }
    }

    if (kinds & kind_listed) {
        r.publish(c);
        invoke(c, shortlist_clubs, 0x103fc850, {c.u32(manager + manager_shortlist), person, frame.at(iterator_at), 1, 1, 0});
        c.put32(frame.at(0x54), c.u32(frame.at(0x4c)));
        for (;;) {
            r.publish(c);
            const auto entry = invoke(c, iterator_next, 0x103fc90c, {frame.at(iterator_at)});
            r[14] = entry;
            if (!entry) break;
            const auto club = c.u32(clubs) + c.u32(entry + 4) * club_size;
            if (!takes_news(club, 0x103fc878)) continue;
            if (std::int8_t(r[26]) < std::int8_t(r[17])) {
                add(c.u32(clubs) + c.u32(entry + 4) * club_size);
            } else {
                full_report(0x103fc8c4, 0x78, 0x40, 0x3aa3);
            }
        }
    }

    if (kinds & kind_derived) {
        // The original keeps the derived selection in registers: r13 everybody, r14 the nation's region (r23/r22),
        // r25 the person's nation group (r21/r20), r24 the person's league (r19).
        for (const unsigned n : {13u, 14u, 25u, 24u, 23u, 22u, 21u, 20u, 19u, 18u}) r[n] = 0;
        if (member_club(person, 0x103fc958, 18)) {
            const auto club = member_club(person, 0x103fc9c0, 18);
            r[19] = c.u32(club + club_league);  // a null club reads guest address 0x30
        }
        const auto profile = c.u32(person + person_profile);
        std::int32_t home = 0, current = 0, world = 0;
        if (profile) {
            current = c.i16(profile + 0xa);
            home = c.i16(profile + 8);
            world = c.i16(profile + 0xc);
        } else if (const auto staff = c.u32(person + person_staff)) {
            current = c.i16(staff + 0x2a);
            home = c.i16(staff + 0x28);
            world = c.i16(staff + 0x2c);
        }
        r[18] = std::uint32_t(current);
        // The thresholds of the two tiers (kinds & 0x200 asks for the higher one).
        const bool high = kinds & kind_high_tier;
        const std::int32_t all_above = high ? 0x251c : 0x23f0, nation_above = high ? 0x2328 : 0x206c,
                           club_above = high ? 0x206c : 0x1c20;
        const std::uint32_t scale_at = high ? 0x198 : 0x188;
        const std::uint32_t site_first = high ? 0x103fcc04 : 0x103fcf48, site_second = high ? 0x103fcd8c : 0x103fd0c8;
        const std::uint32_t back_pair[] = {high ? 0x103fcaf8u : 0x103fce3cu, high ? 0x103fcb60u : 0x103fcea4u};
        const std::uint32_t back_value[] = {high ? 0x103fcbd8u : 0x103fcf1cu, high ? 0x103fcd60u : 0x103fd09cu};
        const std::uint32_t back_value_pair[] = {high ? 0x103fcc44u : 0x103fcf88u, high ? 0x103fccacu : 0x103fcff0u};

        const auto read_value = [&](std::uint32_t profile_record, std::uint32_t back) {
            r.publish(c);
            c.set_f(1, double(c.f32(c.toc(float_pool_cell) + 0x2c)));
            invoke(c, player_value, back, {c.u32(c.toc(stats_cell)), profile_record, 2, 0x12});
            return c.f(1);
        };
        // value > -(scale * (current - magic) - limit), as the original's fsub, fnmsub and fcmpo leave the FPSCR.
        const auto value_beats = [&](double value, std::uint32_t limit_at, std::uint32_t site) {
            const auto pool = c.toc(double_pool_cell);
            const auto converted = fpu.fsub(std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(current) ^ 0x80000000u)),
                                            c.f64(pool + 8));
            const auto bound = fpu.multiply_add(site, c.f64(pool + scale_at), converted, c.f64(pool + limit_at));
            fpu.fcmp(0, value, bound);
            return value > bound;
        };
        // The person's rival club's field (r20), through two contract lookups.
        const auto rival_of_club = [&](std::uint32_t back_first, std::uint32_t back_second) {
            if (!member_club(person, back_first, 18)) return;
            const auto club = member_club(person, back_second, 15);
            r[20] = c.u32(club + club_rival);
        };
        const auto nation_regions = [&] {
            if (const auto nation = c.u32(person + person_nation)) r[23] = c.u32(nation + nation_region);
            if (const auto staff = c.u32(person + person_record))
                if (const auto record = c.u32(staff + 0x2c)) r[22] = c.u32(record + nation_region);
            r[14] = 1;
        };
        const auto by_class = [&] {
            if (!(kinds & kind_by_class) || !r[19]) return;
            const std::int32_t class_value = std::int32_t(c.i16(r[19] + club_class));
            if (high) {
                const auto limit = std::max<std::int32_t>(5000, class_value * 0x2ee);
                if (current > limit) { r[24] = 1; return; }
            } else {
                if (current > class_value * 0x15e && current > 0x9c4) { r[24] = 1; return; }
            }
            const auto record = c.u32(person + person_profile);
            if (!record) return;
            if (value_beats(read_value(record, back_value[1]), 0x190, site_second)) r[24] = 1;
        };

        if (world > all_above) {
            r[13] = 1;
        } else if (world > nation_above) {
            nation_regions();
        } else if (home > club_above || current > club_above) {
            r[25] = 1;
            if (home > club_above) r[21] = c.u32(person + person_nation);
            if (current > club_above) rival_of_club(back_pair[0], back_pair[1]);
        } else {
            bool done = false;
            if (profile) {
                const auto value = read_value(profile, back_value[0]);
                if (value_beats(value, 0x180, site_first) && c.i16(c.u32(person + person_profile) + 0xa) > 3000 &&
                    (kinds & kind_by_value)) {
                    r[25] = 1;
                    rival_of_club(back_value_pair[0], back_value_pair[1]);
                    done = true;
                }
            }
            if (!done) by_class();
        }

        for (std::uint32_t i = 0; std::int8_t(i) < 16; ++i) {
            if (std::int8_t(r[26]) == std::int8_t(r[17])) {
                full_report(0x103fd108, 0x58, 0x38, 0x3b42);
                break;
            }
            const auto count = c.u32(c.toc(people_count_cell));
            const auto listed = c.u32(c.toc(people_cell)) + (count - std::uint32_t(std::int32_t(std::int8_t(i))) - 1) * 0x70;
            if (c.i8(listed + 0x6b) == 0) continue;
            const auto record = c.u32(listed + person_record);
            if (!record) continue;
            const auto nation = c.u32(listed + person_nation);
            const auto other = c.u32(record + 0x2c);
            const auto region = nation ? c.u32(nation + nation_region) : 0;
            const auto other_region = other ? c.u32(other + nation_region) : 0;
            const auto group_x = [&] { return c.i32(r[28]); };
            const auto group_y = [&] { return c.i32(r[27]); };
            const auto group_is = [&](std::uint32_t nation_record, std::int32_t group) {
                return nation_record && c.i32(nation_record + nation_group) == group;
            };
            if (r[13]) {
                add(record);
            } else if (r[14]) {
                if ((region && (region == r[23] || region == r[22])) || (other_region && (other_region == r[23] || other_region == r[22])))
                    add(record);
            } else if (r[25]) {
                if ((nation && (nation == r[21] || nation == r[20])) || (other && (other == r[21] || other == r[20]))) {
                    add(record);
                } else if (nation && c.i32(nation + nation_group) == group_x() && (group_is(r[21], group_y()) || group_is(r[20], group_y()))) {
                    add(record);
                } else if (nation && c.i32(nation + nation_group) == group_y() && (group_is(r[21], group_x()) || group_is(r[20], group_x()))) {
                    add(record);
                }
            } else if (r[24]) {
                if (c.u32(record + club_league) == r[19]) add(record);
            }
        }
    }

    // Each recipient once: a later duplicate is replaced by the last entry (the list is not re-sorted).
    for (std::int32_t i = 0; i < std::int32_t(std::int8_t(r[26])) - 1; ++i)
        for (std::int32_t j = i + 1; j < std::int32_t(std::int8_t(r[26])); ++j)
            if (c.u32(out + 4 * i) == c.u32(out + 4 * j)) {
                --r[26];
                c.put32(out + 4 * j, c.u32(out + 4 * std::uint32_t(std::int32_t(std::int8_t(r[26])))));
            }
    r.publish(c);
    invoke(c, iterator_end, 0x103fd3fc, {frame.at(iterator_at), 0xffffffff});
    return r[26];
}
}
