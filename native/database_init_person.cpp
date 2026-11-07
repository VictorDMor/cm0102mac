// Player Initialisation, per-row phases for every person: names, dates, progress, the 1-20 ratings, the nation.
#include "database_generation.hpp"
#include "database_init_detail.hpp"
#include "database_lookups.hpp"
#include "database_records.hpp"

namespace cm::aot::database::init {
namespace {
// A date's fields as the rows keep them: day (16-bit), year (16-bit), one byte.
constexpr std::uint32_t date_day = 0, date_year = 2, date_byte = 4;
}

// 100b8e60: name records the database left as the "unset" sentinel are picked.
void Gen::person_names(std::uint32_t p) {
    const auto unset = [&](std::uint32_t offset) { return c.u32(p + offset) + 0x10000u == 0xfffe; };
    if (unset(person::common_name)) {
        enter(0x100b8e78, {c.u32(p + person::nation)});
        c.put32(p + person::common_name, pick_unmarked_record(c));
    }
    if (unset(person::first_name)) {
        enter(0x100b8e94, {c.u32(p + person::nation)});
        c.put32(p + person::first_name, pick_name_for_person_filtered(c, c.r(3)));
    }
    if (unset(person::second_name)) {
        enter(0x100b8eb0, {c.u32(p + person::nation)});
        c.put32(p + person::second_name, pick_name_for_person(c, c.r(3)));
    }
}

// 100b8eb4: the row's date at +0x4c (the reference date for a row without a club), brought to a plausible
// birth date, and the two dates at +0x54 and +0x44 reset to the default one when their years are out of range.
void Gen::person_dates(std::uint32_t p) {
    const auto club = [&] { return c.u32(p + person::club); };
    const auto copy_date = [&](std::uint32_t from, std::uint32_t to) {  // lha/sth, lhz/sth, lbz/stb
        c.put16(to, c.u16(from + date_day));
        c.put16(to + 2, c.u16(from + date_year));
        c.put8(to + 4, c.u8(from + date_byte));
    };
    if (club() == 0) copy_date(date, p + person::born);
    if (c.u16(p + person::born + 2) < 0x7b2) {
        if (club() != 0) {
            c.set({{3, p + person::born}, {4, 1}, {5, 0}, {6, 0x7cc}, {7, 0xffffffff}});
            call(0x107158a0, 0x100b8f08);
        } else {
            c.put16(p + person::born, c.u16(date));
            c.put16(p + person::born + 2, c.u16(date + 2));
        }
    } else if (c.u16(p + person::born + 2) == 0x7cc && s16(c.u16(p + person::born)) == 1 && club() != 0 &&
               c.u32(club() + 0x2c) != 0 &&
               c.u32(c.u32(c.u32(club() + 0x2c) + 0x74) + 8) == c.u32(c.toc(-0x7a70))) {
        enter(0x100b8f84, {p, c.u32(club() + 0x2c)});
        person_exempt_from_club_rule(c, p, c.u32(club() + 0x2c));
        if (s8(c.r(3)) == 0) {
            g[13] = c.u16(date + 2);
            const auto sample = next();
            const auto year = (g[13] - u32(rem_s(sample, 2) + 1)) & 0xffff;
            c.set({{3, at(0x84)}, {4, 1}, {5, year}});
            call(0x107156b0, 0x100b8ff0);
            c.put16(p + person::born + 2, c.u16(at(0x86)));
            c.put16(p + person::born, c.u16(at(0x84)));
            c.put8(p + person::born + 4, c.u8(at(0x88)));
        }
    }
    // 100b9008: a club without a nation is forgotten.
    if (club() != 0 && c.u32(club() + 0x2c) == 0) c.put32(p + person::club, 0);
    // 100b902c: the latest allowed birth date (a table date); a later date is replaced by it.
    c.set({{3, at(0x7c)}, {4, c.toc(-0x7a9c)}, {5, 5}});
    call(0x10717cb0, 0x100b9040);
    c.put32(at(0x4c), c.u32(at(0x7c)));
    c.put32(at(0x50), c.u32(at(0x80)));
    const auto limit_year = c.u16(at(0x4e)), born_year = c.u16(p + person::born + 2);
    bool later;
    if (limit_year < born_year) later = true;
    else if (limit_year > born_year) later = false;
    else later = s16(c.u16(p + person::born)) > s16(c.u16(at(0x4c)));
    if (later) {
        c.set({{3, at(0x74)}, {4, c.toc(-0x7a9c)}, {5, 5}});
        call(0x10717cb0, 0x100b90bc);
        c.put16(p + person::born + 2, c.u16(at(0x76)));
        c.put16(p + person::born, c.u16(at(0x74)));
        c.put8(p + person::born + 4, c.u8(at(0x78)));
    }
    // 100b90d4 and 100b9164: the other two dates, replaced by the 31st day of the default year unless equal to it.
    struct DefaultDate {
        std::uint32_t year, day, byte;                 // offsets in the row
        std::uint32_t built, copy, again;              // stack offsets: first build, its copy, second build
        std::uint32_t built_back, again_back;
    };
    for (const auto& d : {DefaultDate{0x56, 0x54, 0x58, 0x6c, 0x44, 0x64, 0x100b90fc, 0x100b914c},
                          DefaultDate{0x46, 0x44, 0x48, 0x5c, 0x3c, 0x54, 0x100b918c, 0x100b91dc}}) {
        if (c.u16(p + d.year) >= 0x7cf && c.u16(p + d.year) <= 0x7da) continue;
        c.set({{3, at(d.built)}, {4, 0x1f}, {5, 0x76c}});
        call(0x107156b0, d.built_back);
        const auto result = c.r(3);
        c.set({{6, c.u32(result)}, {5, c.u32(result + 4)}});  // lwz r6,0(r3); lwz r5,4(r3)
        c.put32(at(d.copy), c.u32(result));
        c.put32(at(d.copy) + 4, c.u32(result + 4));
        if (!(c.u16(at(d.copy + 2)) == c.u16(p + d.year) && s16(c.u16(at(d.copy))) == s16(c.u16(p + d.day)))) {
            c.set({{3, at(d.again)}, {4, 0x1f}, {5, 0x76c}});
            call(0x107156b0, d.again_back);
            c.put16(p + d.year, c.u16(at(d.again + 2)));
            c.put16(p + d.day, c.u16(at(d.again)));
            c.put8(p + d.byte, c.u8(at(d.again + 4)));
        }
    }
}

// 100b91f4: a progress message each time the counter reaches count / 100.
void Gen::person_progress() {
    if (std::int32_t(c.u32(at(0x308))) >= std::int32_t(c.u32(at(0x30c)))) {
        const auto percent = s8(c.u8(at(0x310)) + 1u);
        c.put8(at(0x310), u32(percent));
        c.put32(at(0x308), 0);
        c.set({{3, c.toc(-0x7bb8)}, {4, c.toc(-0x5618) + 0x895}});
        call(0x10727890, 0x100b922c);
        c.set({{3, c.toc(-0x7bb0)}, {4, c.toc(-0x7bb8)}, {5, u32(s8(c.u8(at(0x310))))}, {6, 0x64}, {7, 0xffffffff}});
        call(0x10743120, 0x100b9248);
    } else {
        c.put32(at(0x308), c.u32(at(0x308)) + 1);
    }
}

// 100b9258: the seven ratings at +0x61, +0x63..+0x68 that are outside 1-20: -1 means "unset" (5 to 20),
// anything else is drawn twice (1-20) and the lower or the average kept; +0x69 is drawn from 10-20 likewise.
void Gen::person_ratings(std::uint32_t p) {
    struct Rating {
        std::uint32_t offset;
        bool lower;  // otherwise the average
    };
    for (const auto& rating : {Rating{0x61, true}, Rating{0x63, false}, Rating{0x64, false}, Rating{0x65, true},
                               Rating{0x66, false}, Rating{0x67, false}, Rating{0x68, true}}) {
        const auto value = s8(c.u8(p + rating.offset));
        if (value >= 1 && value <= 20) continue;
        if (value == -1) {
            c.put8(p + rating.offset, u32(rem_s(next(), 16) + 5));
            continue;
        }
        g[13] = u32(s8(u32(rem_s(next(), 20) + 1)));
        const auto second = s8(u32(rem_s(next(), 20) + 1));
        const auto first = s8(g[13]);
        if (rating.lower) c.put8(p + rating.offset, u32(first > second ? second : first));
        else {
            const auto sum = first + second;
            c.put8(p + rating.offset, u32((sum + (sum < 0 ? 1 : 0)) >> 1));
        }
    }
    const auto value = s8(c.u8(p + 0x69));
    if (!(value >= 1 && value <= 20)) {
        if (value == -1) {
            c.put8(p + 0x69, u32(rem_s(next(), 11) + 10));
        } else {
            g[13] = u32(s8(u32(rem_s(next(), 11) + 5)));
            const auto second = s8(u32(rem_s(next(), 11) + 5));
            const auto sum = s8(g[13]) + second;
            c.put8(p + 0x69, u32((sum + (sum < 0 ? 1 : 0)) >> 1));
        }
    }
}

// 100b9d64: the nation's category (+0x118, 4-13) shifts the ratings at +0x68 and +0x69 by draws; then both
// are held to 1-20. Then (100ba0e0) the index table's flag nibble is cleared and a row without a nation takes
// its club's, or one drawn from the nation table.
void Gen::person_nation(std::uint32_t p) {
    if (const auto nation = c.u32(p + person::nation); nation != 0) {
        const auto category = u32(s8(c.u8(nation + 0x118)) - 4);
        const auto shift = [&](std::uint32_t offset, std::int32_t modulus, bool add) {
            const auto delta = s8(u32(rem_s(next(), modulus)));
            c.put8(p + offset, add ? c.u8(p + offset) + u32(delta) : c.u8(p + offset) - u32(delta));
        };
        if (category <= 9) {
            static constexpr std::uint32_t targets[10]{0x100b9e08, 0x100ba068, 0x100b9d9c, 0x100ba068, 0x100b9d9c,
                                                       0x100ba068, 0x100ba068, 0x100b9ee0, 0x100b9e08, 0x100b9f98};
            c.s.wr<4>(guest::reg::ctr, targets[category]);  // mtspr CTR,r3; bctr
            // The blocks fall through: only the first ends with a jump to the clamp.
            if (category == 2 || category == 4) {  // 100b9d9c
                shift(0x68, 10, false);
            } else {
                if (category == 0 || category == 8) {  // 100b9e08
                    shift(0x68, 15, false);
                    shift(0x69, 15, false);
                }
                if (category == 0 || category == 8 || category == 7) {  // 100b9ee0
                    shift(0x68, 8, true);
                    shift(0x69, 8, true);
                }
                if (category == 0 || category == 8 || category == 7 || category == 9) {  // 100b9f98
                    shift(0x68, 5, false);
                    shift(0x69, 5, false);
                }
            }
        }
        for (const auto offset : {0x68u, 0x69u}) {
            const auto v = s8(c.u8(p + offset));
            c.put8(p + offset, u32(v < 1 ? 1 : v > 20 ? 20 : v));
        }
    }
    const auto table = c.u32(c.toc(-0x7b9c));
    const auto flag = table + c.u32(at(0x314)) * 100 + 0xc;
    c.put8(flag, c.u8(flag) & 0x0f);
    c.set(6, 0);
    if (c.u32(p + person::nation) != 0) return;
    if (c.u32(p + person::birth_club) != 0 && c.u32(c.u32(p + person::birth_club) + 0x2c) != 0) {
        c.put32(p + person::nation, c.u32(c.u32(p + person::birth_club) + 0x2c));
    } else if (c.u32(p + person::club) != 0 && c.u32(c.u32(p + person::club) + 0x2c) != 0) {
        c.put32(p + person::nation, c.u32(c.u32(p + person::club) + 0x2c));
    } else {
        g[15] = c.u32(c.toc(-0x7b04));
        g[13] = u32(below_wide(std::int32_t(g[15]), {0x2fc, 0x2f4, 0x2ec}, 0x100ba278));
        c.put32(p + person::nation, c.u32(c.toc(-0x7b98)) + g[13] * 0x128);
    }
}
}
