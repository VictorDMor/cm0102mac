// cm-database-checks fixtures for DATABASE.CPP's records block
// (database_checks.hpp): every branch of the record accessors, the assert
// paths included. The assert macro's four callees are stubbed (the alert
// must not open) with record_call, so each call's arguments, stack buffers
// and registers are compared.
#include "database_checks.hpp"
#include "database_records_ce320.hpp"
#include "database_records_detail.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <tuple>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;
using namespace cm::aot::database::records;

constexpr std::uint32_t record = data + 0x100, other = data + 0x200, owner = data + 0x300,
                        first_link = data + 0x500, second_link = data + 0x600, out_first = data + 0x700,
                        out_second = data + 0x704, table_word = data + 0x800, ready_word = data + 0x804,
                        low_word = data + 0x808, high_word = data + 0x80c, pointer_a = data + 0x900,
                        pointer_b = data + 0x904, number_pool = data + 0x3000, short_a_word = data + 0xa00,
                        short_b_word = data + 0xa02, short_c_word = data + 0xa04, count_word = data + 0xa08,
                        manager_word = data + 0xa0c;
// The library glue at 0x1077fc28 jumps through a descriptor in TOC slot -0x7e64; the fixtures point it at an
// address the stub answers (the glue itself saves the TOC at 0x14(r1)).
constexpr std::uint32_t memcpy_code = 0x10fe0000;
constexpr std::array assert_calls{0x10739890u, 0x10768810u, 0x1077b720u, 0x10769b60u};

struct Spec {
    std::uint32_t entry = 0;
    std::string text;
    std::function<void(Cpu&)> setup;
    bool overflow = false;
    std::uint32_t lookup_answer = 0;  // CONTRACT_MANAGER's club contract lookup, when called
};

// The base state of every fixture: the TOC words the routines share point
// into fixture memory. A case's setup overrides what it varies.
void base(Cpu& c) {
    const auto put = [&](std::int32_t at, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(at), value); };
    put(slot::club_table, table_word);
    put(slot::club_table_ready, ready_word);
    put(slot::range_low, low_word);
    put(slot::range_high, high_word);
    put(slot::number_pool, number_pool);
    put(slot::short_a, short_a_word);
    put(slot::short_b, short_b_word);
    put(slot::short_c, short_c_word);
    put(slot::club_count, count_word);
    put(slot::contract_manager, manager_word);
    put(-0x7e64, data + 0xc00);
    c.put32(data + 0xc00, memcpy_code);
    c.put32(data + 0xc04, c.r(2) + 0x1000);  // the glue loads r2 from here; the caller restores it from 0x14(r1)
    c.put32(manager_word, data + 0xb00);
    c.put16(short_a_word, 0x1111);
    c.put16(short_b_word, 0x2222);
    c.put16(short_c_word, 0x3333);
    c.put32(table_word, data + 0x4000);
    c.put32(ready_word, 1);
    c.put32(low_word, 3);
    c.put32(high_word, 20);
    c.put32(count_word, 10);
    c.put64(number_pool + 0x40, std::bit_cast<std::uint64_t>(7.0));
    // The rating's constants (database_ratings.cpp).
    for (const auto& [offset, value] : std::array<std::pair<std::uint32_t, double>, 8>{{
             {0x18, 4503601774854144.0}, {0x48, 0.5}, {0x60, 0.1}, {0xa0, 2.0}, {0xb0, 10.0}, {0xb8, 20.0},
             {0xc0, 3.0}, {0xc8, 1.0}}})
        c.put64(number_pool + offset, std::bit_cast<std::uint64_t>(value));
}

bool stub(State& s, const Spec& spec) {
    const bool assertion = std::ranges::find(assert_calls, s.pc) != assert_calls.end();
    // Library glue (memcpy, memset, qsort) and CONTRACT_MANAGER's lookups: recorded, answered, not run.
    const bool others = s.pc == memcpy_code || s.pc == 0x10768300;
    if (!assertion && !others && s.pc != club_contract_lookup && s.pc != 0x1014c2e0 && s.pc != 0x10775130 &&
        s.pc != 0x1076f7d0)
        return false;
    Cpu c(s);
    record_call(c);
    // The version text's address, as the original's callee answers.
    c.set(3, assertion ? (s.pc == 0x10768810 ? data + 0x7000 : data + 0x7100)
                       : spec.lookup_answer);
    s.pc = c.lr();
    return true;
}

std::vector<Fixture> fixtures;
void add(const Spec& spec) {
    Fixture f;
    f.entry = spec.entry;
    f.description = spec.text;
    f.setup = [spec](Cpu& c) {
        base(c);
        spec.setup(c);
    };
    f.stub = [spec](State& s) { return stub(s, spec); };
    f.summary_overflow = spec.overflow;
    fixtures.push_back(std::move(f));
}
std::string hex(std::uint32_t value) { return hex_address(value); }

constexpr std::array<std::uint32_t, 7> words{0, 1, 3, 20, 0x7fffffff, 0x80000000, 0xffffffff};

// 0x100c6a10: every sign and size of the two dates' fields.
void date_difference() {
    constexpr std::array<std::uint32_t, 7> years{0, 1, 5, 0x7fff, 0x8000, 0xffff, 0xfffe};
    constexpr std::array<std::uint32_t, 4> days{0, 1, 5, 0xffff};
    add({0x100c6a10, "null first", [](Cpu& c) { c.set(3, 0); c.set(4, other); }});
    for (const auto a : years)
        for (const auto b : years)
            for (const auto day_a : days)
                for (const auto day_b : {0u, 3u, 0xffffu}) {
                    add({0x100c6a10, "years " + hex(a) + "," + hex(b) + " days " + hex(day_a) + "," + hex(day_b),
                         [=](Cpu& c) {
                             c.set(3, record);
                             c.set(4, other);
                             c.put16(record + 0x34, a);
                             c.put16(record + 0x36, day_a);
                             c.put16(other, b);
                             c.put16(other + 2, day_b);
                         },
                         ((a ^ b) & 1) != 0});
                }
    add({0x100c6a10, "first in high memory", [](Cpu& c) { c.set(3, high); c.set(4, high + 0x10); }});
}

// 0x100c76c0, 0x100c7860: each of the three asserts and the index arithmetic.
void club_at() {
    for (const std::uint32_t entry : {0x100c76c0u, 0x100c7860u}) {
        add({entry, "null record", [](Cpu& c) { c.set(3, 0); }});
        add({entry, "no club table", [](Cpu& c) { c.set(3, record); c.put32(table_word, 0); }});
        add({entry, "table not ready", [](Cpu& c) { c.set(3, record); c.put32(ready_word, 0); }, true});
        for (const auto table : {data + 0x4000, 0x7ffffff0u, 0xfffffff0u})
            for (const auto index : words)
                for (const auto low : {0u, 3u, 0x80000000u})
                    for (const auto high : {0u, 20u, 0xffffffffu}) {
                        add({entry,
                             "table " + hex(table) + " index " + hex(index) + " low " + hex(low) + " high " + hex(high),
                             [=](Cpu& c) {
                                 c.set(3, record);
                                 c.put32(record + 0x68, index);
                                 c.put32(table_word, table);
                                 c.put32(low_word, low);
                                 c.put32(high_word, high);
                             },
                             index == 1});
                    }
        add({entry, "record in high memory", [](Cpu& c) { c.set(3, high + 0x40); c.put32(high + 0x40 + 0x68, 2); }});
    }
}

// 0x100c7d00, 0x100ce250: the sign/carry idiom around the range limit.
void range_position() {
    for (const std::uint32_t entry : {0x100c7d00u, 0x100ce250u}) {
        add({entry, "null record", [](Cpu& c) { c.set(3, 0); }});
        for (const auto low : {0u, 1u, 3u, 0x40000000u, 0x80000000u, 0xffffffffu})
            for (const auto high : {0u, 5u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
                const auto t = entry == 0x100c7d00 ? high - (low << 1) : high - low;
                for (const auto word : {0u, 1u, 3u, 20u, 0x7fffffffu, 0x80000000u, 0xffffffffu, t - 1, t, t + 1, ~t, t >> 1}) {
                    add({entry, "low " + hex(low) + " high " + hex(high) + " word " + hex(word),
                         [=](Cpu& c) {
                             c.set(3, record);
                             c.put32(record, word);
                             c.put32(low_word, low);
                             c.put32(high_word, high);
                         },
                         ((low ^ word) & 1) != 0});
                }
            }
    }
}

// 0x100c7bc0: owner and direct links, present or absent.
void record_pair() {
    add({0x100c7bc0, "null record", [](Cpu& c) { c.set({{3, 0}, {4, out_first}, {5, out_second}}); }});
    for (const bool with_owner : {false, true})
        for (const auto first : {0u, first_link, high + 0x500})
            for (const auto second : {0u, second_link, high + 0x600}) {
                add({0x100c7bc0, std::string(with_owner ? "owner" : "direct") + " links " + hex(first) + "," + hex(second),
                     [=](Cpu& c) {
                         c.set({{3, record}, {4, out_first}, {5, out_second}, {7, 0x70707070}, {8, 0x80808080}});
                         c.put32(record + 8, with_owner ? owner : 0);
                         c.put32(owner + 0x80, first);
                         c.put32(owner + 0x84, second);
                         c.put32(record + 0xc, first);
                         c.put32(record + 0x10, second);
                         for (const auto link : {first_link, second_link, high + 0x500, high + 0x600}) {
                             c.put8(link + 0x37, 0x12 + (link & 0xff));
                             c.put8(link + 0x38, 0x9c);
                             c.put8(link + 0x39, 0xe7);
                         }
                     },
                     with_owner && first == 0});
            }
}

// 0x100c7a50: every kind and every combination of the six links.
void record_kind() {
    add({0x100c7a50, "null record", [](Cpu& c) { c.set({{3, 0}, {4, 1}, {5, out_first}, {6, out_second}}); }});
    for (const auto kind : {0u, 1u, 2u, 3u, 4u, 0x101u, 0xffffff02u, 0x300u, 0x103u, 0xffu})
        for (unsigned mask = 0; mask < 64; ++mask) {
            add({0x100c7a50, "kind " + hex(kind) + " links " + std::to_string(mask),
                 [=](Cpu& c) {
                     c.set({{3, record}, {4, kind}, {5, out_first}, {6, out_second}, {8, 0x80808080}});
                     const std::array<std::uint32_t, 6> offsets{0x44, 0x48, 0x4c, 0x50, 0x54, 0x58};
                     for (unsigned i = 0; i < 6; ++i)
                         c.put32(record + offsets[i], mask >> i & 1 ? (i & 1 ? second_link : first_link) : 0);
                     for (const auto link : {first_link, second_link}) {
                         c.put8(link + 0x37, 0x12 + (link & 0xff));
                         c.put8(link + 0x38, 0x9c);
                         c.put8(link + 0x39, 0xe7);
                     }
                 },
                 mask == 5});
        }
}

// 0x100ca010: index limits on both sides, the lookup's answer and the flags.
void contract_rating() {
    const auto setup = [](std::uint32_t person_index, std::uint32_t club_value, std::uint32_t low, std::uint32_t high,
                          std::uint32_t count, bool no_club) {
        return [=](Cpu& c) {
            c.set(3, record);
            c.set(4, no_club ? 0 : other);
            c.put32(record, person_index);
            c.put32(other, club_value);
            c.put32(low_word, low);
            c.put32(high_word, high);
            c.put32(count_word, count);
        };
    };
    add({0x100ca010, "no club", setup(0, 0, 3, 20, 10, true)});
    add({0x100ca010, "no club, overflow", setup(0, 0, 3, 20, 10, true), true});
    for (const auto club : {0u, 13u, 14u, 15u, 0x7fffffffu, 0x80000000u, 0xffffffffu})
        for (const auto limit_words : {std::pair{3u, 20u}, std::pair{0x80000000u, 14u}, std::pair{0u, 0u}})
            for (const auto person : {0u, 9u, 10u, 11u, 0x80000000u, 0xffffffffu})
                for (const auto count : {10u, 0u, 0x80000000u})
                    add({0x100ca010, "club " + hex(club) + " person " + hex(person) + " count " + hex(count),
                         setup(person, club, limit_words.first, limit_words.second, count, false), club == 14 && count == 0});
    // Past the limits, CONTRACT_MANAGER finds nothing or a contract.
    for (const auto answer : {0u, data + 0x1000, high + 0x1000})
        for (const auto flags : {0x00u, 0x0fu, 0x10u, 0x7fu, 0x90u, 0xa0u, 0xf0u, 0xffu}) {
            add({0x100ca010, "contract flags " + hex(flags), [=](Cpu& c) {
                     setup(2, 1, 3, 20, 10, false)(c);
                     if (answer) c.put8(answer + 0x60, flags);
                 }, flags == 0x10, answer});
        }
}

// 0x100cd060, 0x100cd180: absent records, links and values; doubles of every kind.
void compare_rating() {
    const double quiet_nan = std::numeric_limits<double>::quiet_NaN();
    const std::array<double, 7> values{-1.0, 0.0, -0.0, 1.0, 2.5, quiet_nan, 7.0};
    for (const std::uint32_t entry : {0x100cd060u, 0x100cd180u}) {
        const std::uint32_t at = entry == 0x100cd060 ? 0 : 8;
        // Shapes: each pointer's record, link and value present or not.
        for (unsigned shape = 0; shape < 64; ++shape) {
            add({entry, "shape " + std::to_string(shape),
                 [=](Cpu& c) {
                     c.set({{3, pointer_a}, {4, pointer_b}, {6, 0x66666666}});
                     c.put32(pointer_a, shape & 1 ? data + 0x1100 : 0);
                     c.put32(pointer_b, shape & 2 ? data + 0x1200 : 0);
                     c.put32(data + 0x1100 + 0x3c, shape & 4 ? data + 0x1300 : 0);
                     c.put32(data + 0x1200 + 0x3c, shape & 8 ? data + 0x1400 : 0);
                     c.put32(data + 0x1300 + 0x10, shape & 16 ? data + 0x1500 : 0);
                     c.put32(data + 0x1400 + 0x10, shape & 32 ? data + 0x1600 : 0);
                     c.put64(data + 0x1500 + at, std::bit_cast<std::uint64_t>(2.5));
                     c.put64(data + 0x1600 + at, std::bit_cast<std::uint64_t>(-1.0));
                 },
                 shape == 3});
        }
        for (const auto x : values)
            for (const auto y : values)
                add({entry, "values " + std::to_string(x) + " " + std::to_string(y),
                     [=](Cpu& c) {
                         c.set({{3, pointer_a}, {4, pointer_b}});
                         c.put32(pointer_a, data + 0x1100);
                         c.put32(pointer_b, data + 0x1200);
                         c.put32(data + 0x1100 + 0x3c, data + 0x1300);
                         c.put32(data + 0x1200 + 0x3c, data + 0x1400);
                         c.put32(data + 0x1300 + 0x10, data + 0x1500);
                         c.put32(data + 0x1400 + 0x10, data + 0x1600);
                         c.put64(data + 0x1500 + at, std::bit_cast<std::uint64_t>(x));
                         c.put64(data + 0x1600 + at, std::bit_cast<std::uint64_t>(y));
                     },
                     std::isnan(x)});
        // Records in high memory: the pointers' signs do not matter.
        add({entry, "high memory", [=](Cpu& c) {
                 c.set({{3, high + 0x10}, {4, high + 0x14}});
                 c.put32(high + 0x10, high + 0x100);
                 c.put32(high + 0x14, high + 0x200);
                 c.put32(high + 0x100 + 0x3c, 0);
                 c.put32(high + 0x200 + 0x3c, 0);
             }});
    }
}

// 0x100cbde0: each link absent, matching or different, the two guards, and a date.
void record_matches() {
    constexpr std::uint32_t links[3] = {data + 0x1100, data + 0x1200, data + 0x1300}, date = data + 0x1400;
    add({0x100cbde0, "null record", [=](Cpu& c) { c.set({{3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, date}}); }});
    // The date: absent, equal, low half different, high half different, both.
    struct Date {
        std::uint32_t pointer, word, next;
        std::uint16_t year, day;
    };
    constexpr Date dates[] = {{0, 0x00020003, 9, 2, 3},     {date, 0x00020003, 9, 2, 3},
                              {date, 0x00020003, 9, 2, 4},  {date, 0x00020003, 9, 3, 3},
                              {date, 0x00020003, 9, 3, 4},  {date, 0x8000ffff, 9, 0x8000, 0xffff},
                              {date, 0xffff0000, 1, 0xffff, 0}, {high + 0x800, 0x00020003, 9, 2, 3}};
    for (unsigned guard = 0; guard < 4; ++guard)
        for (unsigned shape = 0; shape < 27; ++shape)
            for (const auto& d : dates) {
                add({0x100cbde0, "guard " + std::to_string(guard) + " links " + std::to_string(shape),
                     [=](Cpu& c) {
                         c.set({{3, record}, {4, 5}, {5, 6}, {6, 7}, {7, d.pointer}, {8, 0x88888888}});
                         c.put32(record + 0x14, guard & 1 ? 1 : 0);
                         c.put32(record + 0x1c, guard & 2 ? 1 : 0);
                         const std::uint32_t ids[3] = {5, 6, 7};
                         for (unsigned i = 0, rest = shape; i < 3; ++i, rest /= 3) {
                             const auto kind = rest % 3;  // absent, matching, different
                             c.put32(record + 0x28 + 4 * i, kind == 0 ? 0 : links[i]);
                             c.put32(links[i] + 0x20, kind == 1 ? ids[i] : ids[i] + 0x80000000u);
                         }
                         c.put32(record + 0x34, d.word);
                         c.put32(record + 0x38, d.next);
                         for (const auto where : {date, high + 0x800}) {
                             c.put16(where, d.year);
                             c.put16(where + 2, d.day);
                         }
                     },
                     shape == 4});
            }
}

// 0x100cd7c0: the guards, then the gap between dates of every sign.
void format_date_gap() {
    const std::uint32_t buffer = data + 0x1800;
    const auto call = [&](const std::string& text, std::uint32_t a, std::uint32_t b, std::uint32_t out,
                          std::uint32_t capacity, std::function<void(Cpu&)> more = {}) {
        static unsigned counter = 0;
        add({0x100cd7c0, text,
             [=](Cpu& c) {
                 c.set({{3, a}, {4, b}, {5, out}, {6, capacity}, {7, 0x77777777}, {8, 0x88888888}});
                 c.put32(buffer, 0x55667788);
                 if (more) more(c);
             },
             ++counter % 3 == 0});
    };
    call("no buffer", record, other, 0, 10);
    for (const std::uint32_t capacity : {0u, 4u, 5u, 6u, 100u, 0xffffffffu, 0x80000000u, 0x7fffffffu}) {
        call("capacity " + hex(capacity) + " null a", 0, other, buffer, capacity);
        call("capacity " + hex(capacity) + " null b", record, 0, buffer, capacity);
    }
    constexpr std::array<std::uint16_t, 5> years{0xfffe, 0, 1, 0x8000, 0x7fff};
    constexpr std::array<std::uint16_t, 4> days{0, 1, 0xffff, 5};
    for (const auto year_a : years)
        for (const auto year_b : years)
            for (const auto day_a : days)
                for (const auto day_b : days)
                    call("years " + hex(year_a) + "," + hex(year_b) + " days " + hex(day_a) + "," + hex(day_b), record, other,
                         buffer, 5, [=](Cpu& c) {
                             c.put16(record, year_a);
                             c.put16(record + 2, day_a);
                             c.put16(other, year_b);
                             c.put16(other + 2, day_b);
                         });
}

// 0x100cdae0: every class's slot, and values that match none or only a later slot.
void classify_by_slots() {
    constexpr std::array<std::int32_t, 34> slots{-0x7668, -0x766c, -0x7670, -0x7674, -0x7678, -0x767c, -0x7680, -0x7684,
                                                 -0x7688, -0x768c, -0x7690, -0x7694, -0x7698, -0x769c, -0x76a0, -0x76a4,
                                                 -0x76a8, -0x76ac, -0x76b0, -0x76b4, -0x76b8, -0x76bc, -0x76c0, -0x76c4,
                                                 -0x76c8, -0x76cc, -0x76d0, -0x76d4, -0x76d8, -0x76dc, -0x76e0, -0x7664,
                                                 -0x76e4, -0x76e8};
    const auto setup = [=](std::uint32_t value, std::uint32_t shared) {
        return [=](Cpu& c) {
            c.set(3, other);
            c.put32(other, value);
            for (unsigned i = 0; i < slots.size(); ++i) {
                c.put32(c.r(2) + std::uint32_t(slots[i]), data + 0x2000 + 4 * i);
                c.put32(data + 0x2000 + 4 * i, shared ? shared : 100 + i);
            }
        };
    };
    for (unsigned i = 0; i < slots.size(); ++i) add({0x100cdae0, "class of slot " + std::to_string(i), setup(100 + i, 0)});
    for (const auto value : {0u, 99u, 134u, 0x7fffffffu, 0x80000000u, 0xffffffffu})
        add({0x100cdae0, "no class " + hex(value), setup(value, 0), value == 134});
    for (const auto shared : {77u, 0x80000000u, 0xffffffffu}) {
        add({0x100cdae0, "all words " + hex(shared), setup(shared, shared)});
        add({0x100cdae0, "all words " + hex(shared) + " but the value", setup(shared + 1, shared), true});
    }
}

// 0x100d2b40: every number (including those outside 1-12), the byte read and
// the rating field it scales by, and the two null records.
void attribute_by_number() {
    const auto setup = [](std::uint32_t first, std::uint32_t second, std::uint32_t number, std::uint8_t byte,
                          std::int16_t field) {
        return [=](Cpu& c) {
            c.set({{3, first}, {4, second}, {5, number}, {7, 0x77777777}});
            for (const std::uint32_t at : {0x61u, 0x64u}) c.put8(record + at, byte + at);
            for (const std::uint32_t at : {0x30u, 0x31u, 0x34u, 0x37u, 0x38u, 0x39u, 0x3bu, 0x3eu, 0x41u, 0x42u})
                c.put8(other + at, byte ^ at);
            c.put16(other + 0x24, std::uint16_t(field));
            c.put16(other + 0x4, std::uint16_t(field));
        };
    };
    add({0x100d2b40, "null first", setup(0, other, 3, 0, 0)});
    add({0x100d2b40, "null second", setup(record, 0, 3, 0, 0)});
    add({0x100d2b40, "null second, number 0", setup(record, 0, 0, 0, 0), true});
    for (int number = -128; number < 128; ++number) {
        add({0x100d2b40, "number " + std::to_string(number), setup(record, other, std::uint32_t(number) & 0xff, 0x40, 50)});
        add({0x100d2b40, "number with junk " + std::to_string(number), setup(record, other, 0x12345600u | (number & 0xff), 0x21, -7),
             number % 5 == 0});
    }
    for (unsigned number = 1; number <= 12; ++number)
        for (const std::uint8_t byte : {0, 1, 0x7f, 0x80, 0xff, 10, 0xa5})
            for (const std::int16_t field : {0, 100, -100, 32767, -32768, 200})
                add({0x100d2b40, "number " + std::to_string(number) + " byte " + hex(byte) + " field " + std::to_string(field),
                     setup(record, other, number, byte, field)});
}

// 0x100d2330: every number, with the position byte at both sides of its two tests.
void attribute_by_number_31() {
    const auto setup = [](std::uint32_t first, std::uint32_t second, std::uint32_t number, std::uint8_t byte,
                          std::uint8_t position, std::int16_t field) {
        return [=](Cpu& c) {
            c.set({{3, first}, {4, second}, {5, number}, {7, 0x77777777}});
            c.put8(record + 0x64, byte + 0x64);
            for (std::uint32_t at = 0x10; at <= 0x44; ++at) c.put8(other + at, byte ^ at);
            c.put8(other + 0xf, position);
            c.put16(other + 0x24, std::uint16_t(field));
            c.put16(other + 0x4, std::uint16_t(field));
        };
    };
    add({0x100d2330, "null first", setup(0, other, 3, 0, 0, 0)});
    add({0x100d2330, "null second", setup(record, 0, 3, 0, 0, 0)});
    add({0x100d2330, "null second, number 0", setup(record, 0, 0, 0, 0, 0), true});
    for (int number = -128; number < 128; ++number) {
        add({0x100d2330, "number " + std::to_string(number),
             setup(record, other, std::uint32_t(number) & 0xff, 0x40, 20, 50)});
        add({0x100d2330, "number with junk " + std::to_string(number),
             setup(record, other, 0x12345600u | (number & 0xff), 0x21, 3, -7), number % 5 == 0});
    }
    for (unsigned number = 1; number <= 31; ++number)
        for (const std::uint8_t position : {0, 14, 15, 16, 19, 20, 21, 0x94, 0xff})
            for (const std::uint8_t byte : {0, 1, 0x7f, 0x80, 0xff, 10})
                for (const std::int16_t field : {100, -100, 32767})
                    add({0x100d2330, "number " + std::to_string(number) + " position " + hex(position) + " byte " +
                                         hex(byte) + " field " + std::to_string(field),
                         setup(record, other, number, byte, position, field), position == 15});
}

// 0x100c7dd0: every kind against singles and lists that are empty, matching
// at a position, or past a full scan.
void record_accepts_id() {
    constexpr std::uint32_t list_base = data + 0x1000;
    struct List {
        std::uint32_t offset;
        unsigned count;
    };
    constexpr List lists[] = {{0x84, 3}, {0x160, 5}, {0x174, 7}, {0x190, 3}, {0x98, 0x32}};
    const auto setup = [=](std::uint32_t at, std::int32_t kind, std::uint32_t id, std::uint32_t single, int mode,
                           unsigned position, bool only_98) {
        return [=](Cpu& c) {
            c.set({{3, at}, {4, std::uint32_t(kind) | 0x12345600u}, {5, id}, {7, 0x77777777}});
            if (!at) return;
            for (std::uint32_t offset = 0x70; offset < 0x1a0; offset += 4) c.put32(at + offset, 0x55);
            for (const auto offset : {0x80u, 0x90u, 0x94u}) c.put32(at + offset, single);
            for (const auto& list : lists) {
                if (only_98 && list.offset != 0x98) continue;
                if (mode == 1 && position < list.count) c.put32(at + list.offset + 4 * position, 0);
                if (mode == 2 && position < list.count) c.put32(at + list.offset + 4 * position, id);
            }
        };
    };
    add({0x100c7dd0, "null record", setup(0, 1, 0x1234, 0, 0, 0, false)});
    add({0x100c7dd0, "null id", setup(list_base, 1, 0, 0, 0, 0, false)});
    for (int kind = -128; kind < 128; ++kind)
        add({0x100c7dd0, "kind " + std::to_string(kind), setup(list_base, kind, 0x1234, 0, 0, 0, false), kind % 7 == 0});
    for (int kind = 1; kind <= 15; ++kind)
        for (const auto id : {0x1234u, 0x80000000u})
            for (const auto single : {0u, id, 0x55u})
                for (const bool only_98 : {false, true}) {
                    add({0x100c7dd0, "kind " + std::to_string(kind) + " scan full", setup(list_base, kind, id, single, 0, 0, only_98)});
                    for (const int mode : {1, 2})
                        for (const unsigned position : {0u, 1u, 2u, 4u, 6u, 0x31u})
                            add({0x100c7dd0,
                                 "kind " + std::to_string(kind) + " id " + hex(id) + " single " + hex(single) +
                                     (only_98 ? " only 0x98" : " all lists") + (mode == 1 ? " empty at " : " match at ") +
                                     std::to_string(position),
                                 setup(high + 0x100, kind, id, single, mode, position, only_98), position == 2});
                }
}

// 0x100c82a0: every kind against empty, full and partly filled records, the
// date, the two flags and the link's answer. The club count is 100, so an id
// record's index passes below 84 and needs kind 5 from there; flag A makes the
// kind byte (+0x60 or +0x62, by the first word against the limit 14) visible, so
// each kind promotion, reduction and list-full fallback shows in memory.
void add_to_record() {
    constexpr std::uint32_t list_record = data + 0x1000, id_record = data + 0x1800, date = data + 0x1e00,
                            linked = data + 0x1f00;
    struct List {
        std::uint32_t offset;
        unsigned count;
    };
    constexpr List lists[] = {{0x84, 3}, {0x160, 5}, {0x174, 7}, {0x190, 3}, {0x98, 0x32}};
    struct Case {
        std::uint32_t record = list_record, id = id_record, kind = 5, date_pointer = 0, flag_a = 0, flag_b = 0;
        unsigned fill = 0;           // 0 empty, 1 full, 2 first entry only, 3 all but the last
        std::uint32_t single = 0;
        std::uint32_t first_word = 0, index = 0;  // the record's first word, the id record's first word
        std::uint32_t link = 0, link_value = 0x56;
        std::uint32_t answer = 0;
        std::uint32_t count = 100;
        bool overflow = false;
    };
    unsigned emitted = 0;
    const auto describe = [](const Case& t) {
        return "kind " + hex(t.kind) + " fill " + std::to_string(t.fill) + " single " + hex(t.single) + " date " +
               hex(t.date_pointer) + " flags " + hex(t.flag_a) + "," + hex(t.flag_b) + " first " + hex(t.first_word) +
               " index " + hex(t.index) + " link " + hex(t.link) + " value " + hex(t.link_value) + " answer " + hex(t.answer) +
               " count " + hex(t.count);
    };
    const auto emit = [&](Case t) {
        t.overflow = t.overflow || ++emitted % 5 == 0;  // XER[SO] on every fifth case
        add({0x100c82a0, describe(t),
             [=](Cpu& c) {
                 c.set({{3, t.record}, {4, t.id}, {5, t.kind}, {6, t.date_pointer}, {7, t.flag_a}, {8, t.flag_b}});
                 c.put32(count_word, t.count);
                 if (!t.record || !t.id) return;
                 for (std::uint32_t offset = 0x70; offset < 0x1a0; offset += 4) c.put32(t.record + offset, 0);
                 c.put32(t.record, t.first_word);
                 for (const auto offset : {0x80u, 0x90u, 0x94u}) c.put32(t.record + offset, t.single);
                 for (const auto& list : lists)
                     for (unsigned i = 0; i < list.count; ++i) {
                         const bool filled = t.fill == 1 || (t.fill == 2 && i == 0) || (t.fill == 3 && i + 1 < list.count);
                         c.put32(t.record + list.offset + 4 * i, filled ? 0x4000 + i : 0);
                     }
                 c.put32(t.id, t.index);
                 c.put32(t.id + 0x14, t.link);
                 c.put16(linked + 4, std::uint16_t(t.link_value));
                 c.put16(date, 0xfffe);
                 c.put16(date + 2, 0x1234);
                 c.put8(date + 4, 0x9a);
             },
             t.overflow, t.answer});
    };
    Case t;
    t.record = 0;
    emit(t);
    t = {};
    t.id = 0;
    emit(t);
    // The id record's index against the count less 16 (84), and kind 5; also a
    // count so small that the limit is negative.
    for (const auto count : {100u, 16u, 0u, 0x80000000u})
        for (const auto index : {0u, 83u, 84u, 85u, 0x7fffffffu, 0x80000000u, 0xfffffff0u})
            for (const std::uint32_t kind : {5u, 4u, 0x105u, 0u, 1u}) {
                t = {};
                t.count = count;
                t.index = index;
                t.kind = kind;
                emit(t);
            }
    // Every kind byte, junk above it included; with flag B the reduction.
    for (int kind = -128; kind < 128; ++kind) {
        t = {};
        t.kind = std::uint32_t(kind);
        t.flag_a = 1;
        emit(t);
        t.kind |= 0x12345600u;
        t.flag_b = 1;
        emit(t);
    }
    // Each kind 0-15 into every fill, single word and side of the limit; flag A
    // stores the final kind byte.
    for (std::uint32_t kind = 0; kind <= 15; ++kind)
        for (unsigned fill = 0; fill < 4; ++fill)
            for (const std::uint32_t single : {0u, 0x7777u})
                for (const std::uint32_t first : {0u, 100u})
                    for (const std::uint32_t flag_a : {0u, 1u}) {
                        t = {};
                        t.kind = kind;
                        t.fill = fill;
                        t.single = single;
                        t.first_word = first;
                        t.flag_a = flag_a;
                        emit(t);
                    }
    // Flag B's reduction (kinds 11-13 and 15 become 11, the rest 0) into the
    // slots the new kind names, with every fill.
    for (std::uint32_t kind = 0; kind <= 15; ++kind)
        for (const std::uint32_t flag_b : {1u, 0x80u, 0x100u, 0xffu})
            for (const unsigned fill : {0u, 1u, 3u}) {
                t = {};
                t.kind = kind;
                t.flag_b = flag_b;
                t.fill = fill;
                t.flag_a = 1;
                t.first_word = kind & 1 ? 100 : 0;
                emit(t);
                t.kind |= 0xabcdef00u;
                emit(t);
            }
    // The link: no link, no answer from 0x1014c2e0 with its first value above or
    // not above 0x55 (signed 16-bit: 0x8000 and 0xffff are negative), or an answer.
    for (const std::uint32_t kind : {5u, 6u, 7u, 8u, 9u, 0u})
        for (const std::uint32_t link_value : {0x54u, 0x55u, 0x56u, 0x7fffu, 0x8000u, 0x8056u, 0xffffu})
            for (const std::uint32_t answer : {0u, 1u, 0x100u, 0x80u})
                for (const unsigned fill : {0u, 1u, 3u})
                    for (const std::uint32_t single : {0u, 0x7777u}) {
                        t = {};
                        t.kind = kind;
                        t.link = linked;
                        t.link_value = link_value;
                        t.answer = answer;
                        t.fill = fill;
                        t.single = single;
                        t.flag_a = 1;
                        t.flag_b = kind == 9 ? 1 : 0;
                        emit(t);
                    }
    // The date and flag A against the range limit (high 20 less twice low 3 = 14).
    for (const std::uint32_t kind : {0u, 1u, 5u, 11u})
        for (const std::uint32_t first : {0u, 13u, 14u, 15u, 0x80000000u, 0xffffffffu})
            for (const std::uint32_t date_pointer : {0u, date, high + 0x40})
                for (const std::uint32_t flag_a : {0u, 1u, 0x100u, 0x80u}) {
                    t = {};
                    t.kind = kind;
                    t.first_word = first;
                    t.date_pointer = date_pointer;
                    t.flag_a = flag_a;
                    emit(t);
                }
    // Records in high memory.
    t = {};
    t.record = high + 0x100;
    t.id = high + 0x800;
    t.kind = 13;
    t.fill = 3;
    t.flag_a = 1;
    emit(t);
}

// 0x100c3cb0: the allocation (answered, or failing), the slot count, and
// three tables of entries with every club-index class and weight sign.
void build_slot_summaries() {
    constexpr std::uint32_t summaries_cell = data + 0x2800, block = data + 0x2900, clubs = data + 0x6000,
                            tables = data + 0x8000;
    struct Table {
        std::uint32_t cell, count_word, entries;
        std::int32_t toc_cell, toc_count;
    };
    // The TOC words: r23 -0x7580, r24 -0x7554, r25 -0x7550, r26 -0x754c, r27 -0x7534, r28 -0x7530, r29 -0x752c.
    const Table tables_in_order[3] = {{data + 0x2a00, data + 0x2a10, tables, -0x7530, -0x7550},
                                      {data + 0x2a20, data + 0x2a30, tables + 0x1000, -0x752c, -0x754c},
                                      {data + 0x2a40, data + 0x2a50, tables + 0x2000, -0x7534, -0x7554}};
    struct Case {
        std::uint32_t slots = 3, allocation = block;
        std::array<std::uint32_t, 3> counts{0, 0, 0};
        std::uint32_t seed = 0;
    };
    const auto emit = [&](const Case& t, const std::string& text) {
        add({0x100c3cb0, text,
             [=](Cpu& c) {
                 c.put32(c.r(2) - 0x7580, summaries_cell);
                 c.put32(summaries_cell, 0x11111111);
                 for (const auto& table : tables_in_order) {
                     c.put32(c.r(2) + std::uint32_t(table.toc_cell), table.cell);
                     c.put32(c.r(2) + std::uint32_t(table.toc_count), table.count_word);
                 }
                 for (unsigned i = 0; i < 3; ++i) {
                     c.put32(tables_in_order[i].count_word, t.counts[i]);
                     c.put32(tables_in_order[i].cell, tables_in_order[i].entries);
                 }
                 c.put32(low_word, t.slots);
                 c.put32(ready_word, clubs);
                 // Clubs: each names its slot (+0x68).
                 for (unsigned club = 0; club < 6; ++club) c.put32(clubs + club * 0x128 + 0x68, club % 4);
                 // Entries: club indexes of every class, runs and changes, weights of both signs.
                 auto seed = t.seed * 2654435761u + 12345u;
                 const std::array<std::uint32_t, 10> indexes{0, 1, 2, 3, 0xffffffff, 5, 0x80000000, 2, 1, 0};
                 for (const auto& table : tables_in_order)
                     for (unsigned i = 0; i < 8; ++i) {
                         seed = seed * 1664525u + 1013904223u;
                         const auto entry = table.entries + i * 0x2c;
                         c.put32(entry + 0x24, indexes[(seed >> 16) % indexes.size()]);
                         c.put8(entry + 0x28, (seed >> 8) & 0xff);
                     }
                 c.set({{3, 0x33}, {4, 0x44}, {5, 0x55}, {6, 0x66}, {7, 0x77}, {8, 0x88}, {9, 0x99}, {10, 0xaa}});
             },
             t.slots == 1, t.allocation});
    };
    Case t;
    // A failed allocation: the assert's buffers and registers into 0x1076f7d0
    // (stubbed, returning to 0x100c3d64). No slot is positive, so no loop stores.
    for (const std::uint32_t slots : {0u, 0x80000000u, 0xffffffffu, 0xfffffff0u})
        for (const auto counts : {std::array<std::uint32_t, 3>{0, 0, 0}, {2, 3, 4}}) {
            t = {};
            t.slots = slots;
            t.allocation = 0;
            t.counts = counts;
            emit(t, "allocation fails, slots " + hex(slots) + " counts " + hex(counts[0]) + "," + hex(counts[1]) + "," +
                        hex(counts[2]));
        }
    // A block whose last store (+0x20 of the first slot) lands on the summaries
    // cell: the cell is read again before every store, and the registers keep the
    // values read before. (Earlier stores into the cell make the next read a
    // fault on both sides, which the checker rejects as a difference.)
    t = {};
    t.slots = 1;
    t.allocation = summaries_cell - 0x20;
    emit(t, "block overlaps the cell at +0x20");
    for (const std::uint32_t slots : {0u, 1u, 2u, 3u, 4u, 0x80000000u, 0xffffffffu})
        for (const std::uint32_t seed : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u})
            for (const auto counts : {std::array<std::uint32_t, 3>{0, 0, 0}, {1, 1, 1}, {2, 3, 4}, {8, 8, 8}, {5, 0, 7},
                                      {0, 6, 2}, {0x80000000, 3, 0xffffffff}}) {
                t = {};
                t.slots = slots;
                t.counts = counts;
                t.seed = seed;
                emit(t, "slots " + hex(slots) + " counts " + hex(counts[0]) + "," + hex(counts[1]) + "," + hex(counts[2]) +
                            " seed " + std::to_string(seed));
            }
}

// 0x100ce320: the record's gates (owner, flag byte, owner type), then every
// code of both stages and the index it maps to, with the index overridden by
// -1, other negatives and 0 (the stages end differently on them), the found
// byte absent or aliasing the record's code word (so the stored byte changes
// the word the map reads), and codes that match a stage but miss its map.
void club_of_code() {
    namespace code = cm::aot::database::records::club_code;
    constexpr std::uint32_t cells = data + 0x10000, owner = data + 0x1200, out = data + 0x1400,
                            record_address = data + 0x1000;
    // One cell per TOC offset the table uses, each with a distinct value.
    std::map<std::int32_t, std::uint32_t> cell_of;
    const auto note = [&](std::int32_t at) { cell_of.try_emplace(at, std::uint32_t(cell_of.size())); };
    for (const auto& pair : code::pairs) {
        note(pair.code);
        note(pair.index);
    }
    const auto code_of = [&](std::int32_t at) { return 0x1000u + cell_of.at(at); };
    constexpr std::int32_t owner_types[3] = {-0x7a18, -0x7af4, -0x79fc};
    struct Case {
        std::uint32_t record = record_address, found = out, word = 0, owner_type = 7;
        std::uint32_t flag = 0, flag_byte = 1, owner_pointer = owner;
        std::int32_t override_at = 0;
        std::uint32_t override_value = 0;
        bool overflow = false;
    };
    const auto emit = [&](const Case& t, const std::string& text) {
        add({0x100ce320, text,
             [=](Cpu& c) {
                 c.set({{3, t.record}, {4, t.found}, {5, t.flag}, {7, 0x77777777}, {8, 0x88888888}});
                 for (const auto& [at, index] : cell_of) {
                     c.put32(c.r(2) + std::uint32_t(at), cells + index * 4);
                     c.put32(cells + index * 4, 0x1000 + index);
                 }
                 if (t.override_at) c.put32(cells + cell_of.at(t.override_at) * 4, t.override_value);
                 for (unsigned k = 0; k < 3; ++k) {
                     c.put32(c.r(2) + std::uint32_t(owner_types[k]), data + 0x1600 + 4 * k);
                     c.put32(data + 0x1600 + 4 * k, 7 + k);
                 }
                 c.put32(owner + 0x68, t.owner_type);
                 if (!t.record) return;
                 c.put32(t.record, t.word);
                 c.put32(t.record + 0x2c, t.owner_pointer);
                 c.put8(t.record + 0x244, t.flag_byte);
                 c.put8(out, 0x5a);
             },
             t.overflow});
    };
    Case t;
    t.record = 0;
    emit(t, "null record");
    for (const std::uint32_t owner_pointer : {0u, owner})
        for (const std::uint32_t type : {6u, 7u, 8u, 9u, 10u})
            for (const std::uint32_t flag : {0u, 1u, 0x80u, 0x100u})
                for (const std::uint32_t flag_byte : {0u, 1u, 0x80u, 0xffu})
                    for (const std::uint32_t found : {0u, out}) {
                        t = {};
                        t.owner_pointer = owner_pointer;
                        t.owner_type = type;
                        t.flag = flag;
                        t.flag_byte = flag_byte;
                        t.found = found;
                        t.word = code_of(code::pairs[3].code);
                        t.overflow = (type + flag + flag_byte) % 3 == 0;
                        emit(t, "owner " + hex(owner_pointer) + " type " + std::to_string(type) + " flag " + hex(flag) +
                                    " byte " + hex(flag_byte) + " found " + hex(found));
                    }
    // Words that are no code, with and without the found pointer.
    for (const std::uint32_t word : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 0x1000u + 0x3000u})
        for (const std::uint32_t found : {out, 0u}) {
            t = {};
            t.word = word;
            t.found = found;
            emit(t, "word " + hex(word) + " found " + hex(found));
        }
    // Every code of each stage, with the found pointer present and absent (the
    // second stage's store of 0 is skipped without it).
    for (unsigned i = 0; i < code::pair_count; ++i)
        for (const bool second : {false, true})
            for (const std::uint32_t found : {out, 0u}) {
                t = {};
                t.word = code_of(second ? code::pairs[i].index : code::pairs[i].code);
                t.flag = i % 2;
                t.found = found;
                emit(t, std::string(second ? "second" : "first") + " code " + std::to_string(i) + " found " + hex(found));
            }
    // The index overridden, for every pair of both stages.
    for (unsigned i = 0; i < code::pair_count; ++i)
        for (const bool second : {false, true})
            for (const std::uint32_t value : {0xffffffffu, 0xfffffffbu, 0u, 3u, 0x80000000u}) {
                const auto& pair = code::pairs[i];
                t = {};
                t.word = code_of(second ? pair.index : pair.code);
                t.override_at = second ? pair.code : pair.index;
                t.override_value = value;
                t.overflow = value == 3;
                emit(t, std::string(second ? "second" : "first") + " map " + std::to_string(i) + " index " + hex(value));
            }
    // A code that matches its stage but misses the map: `found` is the record's
    // own word, so the stored byte (1 in the first stage, 0 in the second)
    // replaces the word's top byte before the map compares it. The code's cell
    // takes the word, with a top byte that the store keeps or changes.
    for (const unsigned i : {0u, 1u, 57u, 58u, 59u, 70u, 81u})
        for (const bool second : {false, true})
            for (const std::uint32_t top : {0x00u, 0x01u, 0x7fu}) {
                const auto& pair = code::pairs[i];
                t = {};
                t.found = record_address;
                t.word = top << 24 | 0x1234;
                t.override_at = second ? pair.index : pair.code;
                t.override_value = t.word;
                t.flag_byte = 1;
                t.overflow = top == 1;
                emit(t, std::string(second ? "second" : "first") + " alias code " + std::to_string(i) + " top " + hex(top));
            }
}

// The world of 0x100ce320's fixtures for the routines that call it: one pair's
// code word is 5 and its index word 6, so a record whose first word is 5 has
// club 6 (first stage, found 1) and one whose word is 6 has club 5 (second
// stage, found 0); any other word has no club. Clubs are table entries of 0x248
// bytes at data+0x4000.
namespace world {
constexpr std::uint32_t cells = data + 0x10000, owner = data + 0x1800, table = data + 0x4000,
                        record = data + 0x1000;
constexpr std::uint32_t club(unsigned index) { return table + index * 0x248; }
const std::map<std::int32_t, std::uint32_t>& cell_of() {
    static const auto map = [] {
        std::map<std::int32_t, std::uint32_t> m;
        for (const auto& pair : cm::aot::database::records::club_code::pairs) {
            m.try_emplace(pair.code, std::uint32_t(m.size()));
            m.try_emplace(pair.index, std::uint32_t(m.size()));
        }
        return m;
    }();
    return map;
}
// Places the code cells, the owner types (7, 8, 9) and the table; the first pair's words.
void place(Cpu& c) {
    namespace code = cm::aot::database::records::club_code;
    for (const auto& [at, index] : cell_of()) {
        c.put32(c.r(2) + std::uint32_t(at), cells + index * 4);
        c.put32(cells + index * 4, 0x1000 + index);
    }
    c.put32(cells + cell_of().at(code::pairs[0].code) * 4, 5);
    c.put32(cells + cell_of().at(code::pairs[0].index) * 4, 6);
    for (unsigned k = 0; k < 3; ++k) {
        constexpr std::int32_t owner_types[3] = {-0x7a18, -0x7af4, -0x79fc};
        c.put32(c.r(2) + std::uint32_t(owner_types[k]), data + 0x1600 + 4 * k);
        c.put32(data + 0x1600 + 4 * k, 7 + k);
    }
    c.put32(owner + 0x68, 7);
}
// A record that club_of_code accepts, with the given code word.
void make_record(Cpu& c, std::uint32_t address, std::uint32_t word) {
    c.put32(address, word);
    c.put32(address + 0x2c, owner);
    c.put8(address + 0x244, 1);
}
}

// The pseudo-random generator of the list fixtures.
std::uint32_t next_random(std::uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return seed >> 8;
}

// Lists of pointers to people (0x100c9440, 0x100c9590): each entry empty or a
// person whose first word is below, at or above the limit (10), as signed values.
void count_valid() {
    constexpr std::uint32_t people = data + 0x2000;
    const std::array<std::uint32_t, 8> firsts{0, 5, 9, 10, 11, 0x7fffffff, 0x80000000, 0xffffffff};
    for (const auto& [entry, offset, entries] : {std::tuple{0x100c9440u, 0x160u, 5u}, std::tuple{0x100c9590u, 0x190u, 3u}}) {
        add({entry, "null record", [](Cpu& c) { c.set(3, 0); }});
        for (const std::uint32_t word : {5u, 6u, 0x77u})
            for (unsigned seed = 1; seed <= 24; ++seed) {
                add({entry, "word " + hex(word) + " seed " + std::to_string(seed),
                     [=](Cpu& c) {
                         world::place(c);
                         world::make_record(c, world::record, word);
                         c.set(3, world::record);
                         auto state = seed * 7919u;
                         for (unsigned i = 0; i < 16; ++i) c.put32(people + i * 16, firsts[next_random(state) % firsts.size()]);
                         for (const auto base : {world::record, world::club(5), world::club(6)})
                             for (unsigned i = 0; i < entries; ++i) {
                                 const auto pick = next_random(state);
                                 c.put32(base + offset + 4 * i, pick % 4 == 0 ? 0 : people + (pick >> 4) % 16 * 16);
                             }
                     },
                     seed % 5 == 0});
            }
    }
}

// 0x100c9bb0: the +0x94 word of the record or its club.
void id_word_94() {
    add({0x100c9bb0, "null record", [](Cpu& c) { c.set(3, 0); }});
    for (const std::uint32_t word : {5u, 6u, 0x77u})
        for (const std::uint32_t own : {0u, 7u, 0x80000000u})
            for (const std::uint32_t other_word : {0u, 9u, 0xffffffffu})
                add({0x100c9bb0, "word " + hex(word) + " own " + hex(own) + " club's " + hex(other_word),
                     [=](Cpu& c) {
                         world::place(c);
                         world::make_record(c, world::record, word);
                         c.set(3, world::record);
                         c.put32(world::record + 0x94, own);
                         c.put32(world::club(5) + 0x94, other_word);
                         c.put32(world::club(6) + 0x94, other_word);
                     },
                     own == 7 && other_word == 9});
}

// 0x100d1cb0, 0x100d2a80: every number (including junk above the byte) on a
// person whose link holds attribute bytes and the rating field.
void person_attributes() {
    constexpr std::uint32_t person = data + 0x1000, link = data + 0x1200;
    for (const auto& [entry, offset] : {std::pair{0x100d1cb0u, 0x14u}, std::pair{0x100d2a80u, 0x1cu}}) {
        const auto setup = [=](std::uint32_t who, std::uint32_t number, std::uint8_t byte) {
            return [=](Cpu& c) {
                c.set({{3, who}, {4, number}, {7, 0x77777777}});
                c.put32(person + offset, link);
                c.put8(person + 0x61, byte + 1);
                c.put8(person + 0x64, byte + 4);
                for (std::uint32_t at = 0x10; at <= 0x44; ++at) c.put8(link + at, byte ^ at);
                c.put8(link + 0xf, byte & 1 ? 20 : 14);
                c.put16(link + 0x24, 60);
                c.put16(link + 0x4, 0xff9c);
            };
        };
        add({entry, "null person", setup(0, 3, 0)});
        for (int number = -128; number < 128; ++number) {
            add({entry, "number " + std::to_string(number), setup(person, std::uint32_t(number) & 0xff, 0x40)});
            add({entry, "junk number " + std::to_string(number), setup(person, 0x12345600u | (number & 0xff), 0x21),
                 number % 7 == 0});
        }
    }
}

// 0x100d3e60: which stage club_of_code reports, and the gates.
void in_second_set() {
    add({0x100d3e60, "null record", [](Cpu& c) { c.set(3, 0); }});
    for (const std::uint32_t word : {5u, 6u, 0x77u})
        for (const std::uint32_t owner_pointer : {0u, world::owner})
            for (const std::uint32_t flag_byte : {0u, 1u})
                for (const std::uint32_t type : {7u, 9u, 12u})
                    add({0x100d3e60, "word " + hex(word) + " owner " + hex(owner_pointer) + " byte " + hex(flag_byte) +
                                         " type " + hex(type),
                         [=](Cpu& c) {
                             world::place(c);
                             world::make_record(c, world::record, word);
                             c.put32(world::record + 0x2c, owner_pointer);
                             c.put8(world::record + 0x244, flag_byte);
                             c.put32(world::owner + 0x68, type);
                             c.set(3, world::record);
                         },
                         type == 9});
}

// 0x100cd980: every pair of classes, and the shared word.
void classes_compatible() {
    const std::array<std::int32_t, 34> slots{-0x7668, -0x766c, -0x7670, -0x7674, -0x7678, -0x767c, -0x7680, -0x7684,
                                             -0x7688, -0x768c, -0x7690, -0x7694, -0x7698, -0x769c, -0x76a0, -0x76a4,
                                             -0x76a8, -0x76ac, -0x76b0, -0x76b4, -0x76b8, -0x76bc, -0x76c0, -0x76c4,
                                             -0x76c8, -0x76cc, -0x76d0, -0x76d4, -0x76d8, -0x76dc, -0x76e0, -0x7664,
                                             -0x76e4, -0x76e8};
    // The first slot of each class 1-10, and a value that is no class's.
    const std::array<unsigned, 10> first_of_class{2, 4, 5, 14, 18, 23, 29, 8, 12, 0};
    for (const bool alias : {false, true})
        for (unsigned class_a = 0; class_a < 11; ++class_a)
            for (unsigned class_b = 0; class_b < 11; ++class_b) {
                const auto value_of = [&](unsigned k) { return k == 10 ? 0x5555u : 100u + first_of_class[k]; };
                add({0x100cd980, std::string(alias ? "aliased " : "") + "classes " + std::to_string(class_a + 1) + "," +
                                     std::to_string(class_b + 1),
                     [=](Cpu& c) {
                         for (unsigned i = 0; i < slots.size(); ++i) {
                             c.put32(c.r(2) + std::uint32_t(slots[i]), data + 0x2000 + 4 * i);
                             c.put32(data + 0x2000 + 4 * i, 100 + i);
                         }
                         // The class-6 word equal to the shared word (slot -0x7664).
                         if (alias) c.put32(data + 0x2000 + 4 * 23, 100 + 31);
                         c.put32(data + 0x2400, value_of(class_a));
                         c.put32(data + 0x2404, alias && class_b == 5 ? 100u + 31 : value_of(class_b));
                         c.set({{3, data + 0x2400}, {4, data + 0x2404}});
                     },
                     (class_a + class_b) % 7 == 0});
            }
    // The shared word (the cell of slot -0x7664, value 131, a class 7 word): a class 6 record
    // against a class 7 record holding it, either way round (the incompatible exits).
    for (const bool swapped : {false, true})
        for (const bool overflow : {false, true})
            add({0x100cd980, std::string("class 6 against the shared word") + (swapped ? " swapped" : ""),
                 [=](Cpu& c) {
                     for (unsigned i = 0; i < slots.size(); ++i) {
                         c.put32(c.r(2) + std::uint32_t(slots[i]), data + 0x2000 + 4 * i);
                         c.put32(data + 0x2000 + 4 * i, 100 + i);
                     }
                     c.put32(data + 0x2400, swapped ? 131u : 123u);  // 123: slot 23, class 6
                     c.put32(data + 0x2404, swapped ? 123u : 131u);
                     c.set({{3, data + 0x2400}, {4, data + 0x2404}});
                 },
                 overflow});
}

// 0x100c9c90, 0x100c9e50: the id list of a record, with and without a club.
void copy_ids() {
    constexpr std::uint32_t out = data + 0x3000;
    for (const auto& [entry, offset, entries] : {std::tuple{0x100c9c90u, 0x160u, 5u}, std::tuple{0x100c9e50u, 0x174u, 7u}}) {
        add({entry, "null record", [=](Cpu& c) { c.set({{3, 0}, {4, out}}); }});
        for (const std::uint32_t word : {5u, 6u, 0x77u})
            for (unsigned seed = 1; seed <= 30; ++seed)
                add({entry, "word " + hex(word) + " seed " + std::to_string(seed),
                     [=](Cpu& c) {
                         world::place(c);
                         world::make_record(c, world::record, word);
                         c.set({{3, world::record}, {4, out}});
                         auto state = seed * 104729u;
                         for (const auto base : {world::record, world::club(5), world::club(6)})
                             for (unsigned i = 0; i < entries; ++i) {
                                 const auto pick = next_random(state);
                                 c.put32(base + offset + 4 * i, pick % 3 == 0 ? 0 : 0x4000 + pick % 100);
                             }
                     },
                     seed % 4 == 0});
    }
}

// 0x100d06f0: members (fifty ids at +0x98) kept by their first word against the limit.
void copy_member_ids() {
    constexpr std::uint32_t out = data + 0x3000, people = data + 0x2000;
    const std::array<std::uint32_t, 6> firsts{0, 5, 9, 10, 0x7fffffff, 0xffffffff};
    add({0x100d06f0, "null record", [=](Cpu& c) { c.set({{3, 0}, {4, out}, {5, 1}}); }});
    for (const std::uint32_t word : {5u, 6u, 0x77u})
        for (const std::uint32_t flag : {0u, 1u, 0x100u, 0x80u})
            for (unsigned seed = 1; seed <= 10; ++seed)
                add({0x100d06f0, "word " + hex(word) + " flag " + hex(flag) + " seed " + std::to_string(seed),
                     [=](Cpu& c) {
                         world::place(c);
                         world::make_record(c, world::record, word);
                         c.set({{3, world::record}, {4, out}, {5, flag}});
                         auto state = seed * 15485863u;
                         for (unsigned i = 0; i < 16; ++i) c.put32(people + i * 16, firsts[next_random(state) % firsts.size()]);
                         for (const auto base : {world::record, world::club(5), world::club(6)}) {
                             // Mostly full lists, so the second pass runs out of room.
                             const unsigned fill = seed % 3 == 0 ? 20 : 50;
                             for (unsigned i = 0; i < 50; ++i) {
                                 const auto pick = next_random(state);
                                 c.put32(base + 0x98 + 4 * i, i >= fill || pick % 5 == 0 ? 0 : people + (pick >> 4) % 16 * 16);
                             }
                         }
                     },
                     seed % 4 == 0});
}

// 0x100d4370: the slots collected into `out`, against the limit byte.
void collect_ids() {
    constexpr std::uint32_t out = data + 0x3000;
    add({0x100d4370, "null record", [=](Cpu& c) { c.set({{3, 0}, {4, out}, {5, 3}}); }});
    for (const std::uint32_t word : {5u, 6u, 0x77u})
        for (const std::uint32_t limit : {0u, 1u, 3u, 10u, 19u, 20u, 100u, 127u, 0x80u, 0xffu, 0x105u, 0x12345603u})
            for (unsigned seed = 1; seed <= 4; ++seed)
                add({0x100d4370, "word " + hex(word) + " limit " + hex(limit) + " seed " + std::to_string(seed),
                     [=](Cpu& c) {
                         world::place(c);
                         world::make_record(c, world::record, word);
                         c.set({{3, world::record}, {4, out}, {5, limit}});
                         auto state = seed * 32452843u;
                         for (const auto base : {world::record, world::club(5), world::club(6)}) {
                             for (const auto at : {0x80u, 0x90u, 0x94u})
                                 c.put32(base + at, next_random(state) % 3 == 0 ? 0 : 0x5000 + at);
                             for (const auto& [offset, entries] : {std::pair{0x84u, 3u}, std::pair{0x160u, 5u},
                                                                    std::pair{0x174u, 7u}, std::pair{0x190u, 3u}})
                                 for (unsigned i = 0; i < entries; ++i)
                                     c.put32(base + offset + 4 * i, next_random(state) % 3 == 0 ? 0 : 0x6000 + offset + i);
                         }
                     },
                     limit == 3});
}

// 0x100c96e0: a chain of records whose owners lead to the next (range
// position 1 needs a first word of 17 or more), with the +0x90 word set at one
// step, a null owner at one step, a step that is not in range, and the club case.
void id_word_90_chain() {
    constexpr std::uint32_t owners = data + 0x1800;
    const auto node = [](unsigned i) { return world::club(20 + i); };
    const auto build = [=](Cpu& c, unsigned words_at, unsigned null_owner_at, unsigned out_of_range_at) {
        world::place(c);
        for (unsigned i = 0; i < 9; ++i) {
            const auto owner = owners + i * 0x100;
            c.put32(node(i), 0x4000 + i);
            c.put32(node(i) + 0x2c, i == null_owner_at ? 0 : owner);
            c.put32(node(i) + 0x90, i == words_at ? 0x7000 + i : 0);
            c.put8(node(i) + 0x244, 1);
            c.put32(owner + 0x68, 7 + i);  // club_at_doubled_bias: the next node, 21 + i
            if (i == out_of_range_at) c.put32(node(i), 3);
        }
    };
    add({0x100c96e0, "null record", [](Cpu& c) { c.set(3, 0); }});
    for (unsigned step = 0; step < 9; ++step) {
        add({0x100c96e0, "word at " + std::to_string(step), [=](Cpu& c) { build(c, step, 99, 99); c.set(3, node(0)); }, step % 3 == 1});
        add({0x100c96e0, "null owner at " + std::to_string(step), [=](Cpu& c) { build(c, 99, step, 99); c.set(3, node(0)); }, step % 3 == 2});
        add({0x100c96e0, "out of range at " + std::to_string(step), [=](Cpu& c) { build(c, 99, 99, step); c.set(3, node(0)); }});
    }
    add({0x100c96e0, "no word, no end", [=](Cpu& c) { build(c, 99, 99, 99); c.set(3, node(0)); }});
    // The owners do not lead anywhere: the club table is not ready.
    add({0x100c96e0, "table not ready", [=](Cpu& c) { build(c, 99, 99, 99); c.put32(ready_word, 0); c.set(3, node(0)); }});
    add({0x100c96e0, "no table", [=](Cpu& c) { build(c, 99, 99, 99); c.put32(table_word, 0); c.set(3, node(0)); }});
    // Club cases: the record's code names a club.
    for (const std::uint32_t word : {5u, 6u})
        for (const std::uint32_t own : {0u, 0x7001u})
            for (const std::uint32_t club_word : {0u, 0x7002u})
                add({0x100c96e0, "club word " + hex(word) + " own " + hex(own) + " club's " + hex(club_word),
                     [=](Cpu& c) {
                         build(c, 99, 99, 99);
                         world::make_record(c, node(0), word);
                         c.put32(node(0) + 0x90, own);
                         c.put32(world::club(5) + 0x90, club_word);
                         c.put32(world::club(6) + 0x90, club_word);
                         c.set(3, node(0));
                     }});
}

// 0x100d12d0: the record's word against the limits, the club, the owner's
// type and its link.
void club_less_candidate() {
    constexpr std::uint32_t link_record = data + 0x1c00;
    const std::array<std::int32_t, 6> type_slots{-0x7504, -0x7a14, -0x7b68, -0x7508, -0x7970, -0x7974};
    add({0x100d12d0, "null record", [](Cpu& c) { c.set(3, 0); }});
    for (const std::uint32_t word : {0u, 5u, 6u, 13u, 14u, 15u, 16u, 17u, 18u, 0x77u, 0x80000000u, 0xffffffffu})
        for (const std::uint32_t owner_pointer : {0u, world::owner})
            for (const std::uint32_t type : {7u, 8u, 9u, 21u, 22u, 23u, 24u, 12u})
                for (const std::uint32_t link : {0u, 25u, 26u, 99u}) {
                    add({0x100d12d0,
                         "word " + hex(word) + " owner " + hex(owner_pointer) + " type " + std::to_string(type) + " link " +
                             std::to_string(link),
                         [=](Cpu& c) {
                             world::place(c);
                             world::make_record(c, world::record, word);
                             c.put32(world::record + 0x2c, owner_pointer);
                             c.set(3, world::record);
                             for (unsigned k = 0; k < type_slots.size(); ++k) {
                                 c.put32(c.r(2) + std::uint32_t(type_slots[k]), data + 0x1700 + 4 * k);
                                 c.put32(data + 0x1700 + 4 * k, 21 + k);
                             }
                             c.put32(world::owner + 0x68, type);
                             c.put32(world::owner + 0x74, link ? link_record : 0);
                             c.put32(link_record + 8, link);
                         },
                         (word + type + link) % 5 == 0});
                }
}

// 0x100d3c40: the record against the range limits, its link and the table, the owner's flags and rating.
void eligible_record() {
    constexpr std::uint32_t link_record = data + 0x1c00, listing = data + 0x1d00, table_cell = data + 0x1d80;
    add({0x100d3c40, "null record", [](Cpu& c) { c.set(3, 0); }});
    for (const std::uint32_t word : {0u, 5u, 6u, 13u, 14u, 15u, 16u, 17u, 18u, 0x77u, 0x80000000u})
        for (const std::uint32_t owner_pointer : {0u, world::owner})
            for (const std::uint32_t link : {0u, link_record})
                for (const std::uint32_t listed : {0u, 1u, 2u})  // no table, entry empty, entry listed
                    for (const std::uint32_t bits : {0u, 1u, 2u, 3u})
                        for (const std::uint32_t rating : {0x95u, 0x96u, 0x97u, 0x80000000u})
                            add({0x100d3c40,
                                 "word " + hex(word) + " owner " + hex(owner_pointer) + " link " + hex(link) + " listed " +
                                     std::to_string(listed) + " bits " + std::to_string(bits) + " rating " + hex(rating),
                                 [=](Cpu& c) {
                                     world::place(c);
                                     world::make_record(c, world::record, word);
                                     c.put32(world::record + 0x2c, owner_pointer);
                                     c.put32(world::record + 0x30, link);
                                     c.put32(link_record, 3);  // the entry's index
                                     c.put32(c.r(2) - 0x7b5c, table_cell);
                                     c.put32(table_cell, listed ? listing : 0);
                                     c.put32(listing + 12, listed == 2 ? 0x1234 : 0);
                                     c.put8(world::owner + 0x120, bits);
                                     c.put32(world::owner + 0x6c, rating);
                                     c.set(3, world::record);
                                 },
                                 (word + bits + rating) % 7 == 0});
}

// 0x100c9070: the members of a record and its club kept or not, by the limit, the
// flag's contract lookups (CONTRACT_MANAGER's real leaf routines read a table of
// contract indexes per person: the person's first word times 100) and the kind.
void count_members_kept() {
    constexpr std::uint32_t people = data + 0x2000, contract_table = data + 0x9000, contracts = data + 0x9800,
                            contracts_cell = data + 0xb00, table_cell = data + 0xa00;
    const std::array<std::uint32_t, 4> firsts{0, 5, 9, 0xffffffff};
    add({0x100c9070, "null record", [](Cpu& c) { c.set({{3, 0}, {4, 1}, {5, 1}}); }});
    const auto record = world::club(10);
    for (const std::uint32_t word : {5u, 6u, 0x77u, 16u})
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x102u, 0x80u})
            for (const std::uint32_t flag : {0u, 1u, 0x100u})
                for (const std::uint32_t secondary : {0xffffffffu, 0u, 1u})
                    for (const std::uint32_t primary : {0xffffffffu, 0u, 1u, 2u})
                        for (unsigned seed = 1; seed <= 2; ++seed) {
                            add({0x100c9070,
                                 "word " + hex(word) + " kind " + hex(kind) + " flag " + hex(flag) + " secondary " +
                                     hex(secondary) + " primary " + hex(primary) + " seed " + std::to_string(seed),
                                 [=](Cpu& c) {
                                     world::place(c);
                                     world::make_record(c, record, word);
                                     c.set({{3, record}, {4, kind}, {5, flag}});
                                     auto state = seed * 49979687u + word + kind;
                                     // The contract tables: indexes by the person's first word (0, 5, 9).
                                     c.put32(c.r(2) - 0x7b9c, table_cell);
                                     c.put32(table_cell, contract_table);
                                     c.put32(contracts_cell, contracts);
                                     for (const unsigned v : {0u, 5u, 9u}) {
                                         c.put32(contract_table + v * 100, v == 5 ? primary : seed == 1 ? 1u : primary);
                                         c.put32(contract_table + v * 100 + 4, v == 9 ? secondary : seed == 1 ? 0u : secondary);
                                     }
                                     for (unsigned j = 0; j < 4; ++j) c.put32(contracts + j * 100 + 4, j == 1 ? 10 : 11);
                                     for (unsigned i = 0; i < 16; ++i) {
                                         const auto person = people + i * 0x80;
                                         c.put32(person, firsts[next_random(state) % firsts.size()]);
                                         const auto pick = next_random(state) % 3;
                                         c.put32(person + 0x24, pick == 0 ? record : pick == 1 ? world::club(11) : 0);
                                         c.put8(person + 0x6a, next_random(state));
                                     }
                                     for (const auto base : {record, world::club(5), world::club(6)})
                                         for (unsigned i = 0; i < 50; ++i) {
                                             const auto pick = next_random(state);
                                             c.put32(base + 0x98 + 4 * i, pick % 3 == 0 ? 0 : people + (pick >> 4) % 16 * 0x80);
                                         }
                                 },
                                 seed == 2 && flag == 1});
                        }
    // A record without a club that club_less_candidate accepts (word 3 or 14 or 16 with an owner type
    // outside the five words), so the pass with the kind filter runs, for every kind.
    const std::array<std::int32_t, 6> type_slots{-0x7504, -0x7a14, -0x7b68, -0x7508, -0x7970, -0x7974};
    for (const std::uint32_t word : {3u, 14u, 16u})
        for (const std::uint32_t type : {7u, 12u})
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x102u, 0x80u})
                for (const std::uint32_t flag : {0u, 1u})
                    for (const std::uint32_t secondary : {0xffffffffu, 1u})
                        for (const std::uint32_t primary : {0u, 1u})
                            for (unsigned seed = 1; seed <= 2; ++seed)
                                add({0x100c9070,
                                     "club-less word " + hex(word) + " type " + std::to_string(type) + " kind " + hex(kind) +
                                         " flag " + hex(flag) + " secondary " + hex(secondary) + " primary " + std::to_string(primary) +
                                         " seed " + std::to_string(seed),
                                     [=](Cpu& c) {
                                         world::place(c);
                                         world::make_record(c, record, word);
                                         c.put32(world::owner + 0x68, type);
                                         c.put32(world::owner + 0x74, 0);
                                         for (unsigned k = 0; k < type_slots.size(); ++k) {
                                             c.put32(c.r(2) + std::uint32_t(type_slots[k]), data + 0x1700 + 4 * k);
                                             c.put32(data + 0x1700 + 4 * k, 21 + k);
                                         }
                                         c.set({{3, record}, {4, kind}, {5, flag}});
                                         auto state = seed * 67867967u + word + kind;
                                         c.put32(c.r(2) - 0x7b9c, table_cell);
                                         c.put32(table_cell, contract_table);
                                         c.put32(contracts_cell, contracts);
                                         for (const unsigned v : {0u, 5u, 9u}) {
                                             c.put32(contract_table + v * 100, v == 5 ? primary : 1u);
                                             c.put32(contract_table + v * 100 + 4, v == 9 ? secondary : 0u);
                                         }
                                         for (unsigned j = 0; j < 4; ++j) c.put32(contracts + j * 100 + 4, j == 1 ? 10 : 11);
                                         for (unsigned i = 0; i < 16; ++i) {
                                             const auto person = people + i * 0x80;
                                             c.put32(person, firsts[next_random(state) % 3]);
                                             const auto pick = next_random(state) % 3;
                                             c.put32(person + 0x24, pick == 0 ? record : pick == 1 ? world::club(11) : 0);
                                             c.put8(person + 0x6a, next_random(state));
                                         }
                                         for (unsigned i = 0; i < 50; ++i) {
                                             const auto pick = next_random(state);
                                             c.put32(record + 0x98 + 4 * i, pick % 3 == 0 ? 0 : people + (pick >> 4) % 16 * 0x80);
                                         }
                                     },
                                     seed == 2 && flag == 1});
}

// 0x100d0540: every club of the table (three here) with and without a club, owner and
// second-stage word, members to be trimmed.
void trim_club_members() {
    constexpr std::uint32_t people = data + 0x2000;
    const std::array<std::uint32_t, 5> firsts{0, 5, 9, 10, 0xffffffff};
    for (const std::uint32_t words : {0u, 1u, 2u, 3u, 4u, 5u, 6u})
        for (unsigned seed = 1; seed <= 8; ++seed)
            add({0x100d0540, "words " + std::to_string(words) + " seed " + std::to_string(seed),
                 [=](Cpu& c) {
                     world::place(c);
                     c.put32(high_word, 3);  // the club count
                     c.put32(c.r(2) - 0x796c, data + 0x1b00);
                     c.put32(c.r(2) - 0x7be8, table_word);
                     auto state = seed * 86028121u + words;
                     // Clubs 0-2: code words by `words` (5: club 6, 6: club 5 and the second stage, other: none).
                     const std::array<std::uint32_t, 7> patterns{0x555, 0x556, 0x565, 0x655, 0x666, 0x575, 0x777};
                     for (unsigned k = 0; k < 3; ++k) {
                         const auto pick = patterns[words] >> (4 * (2 - k)) & 0xf;
                         world::make_record(c, world::club(k), pick == 5 ? 5u : pick == 6 ? 6u : 0x77u);
                         c.put8(world::club(k) + 0x244, 0x55);
                     }
                     world::make_record(c, world::club(5), 0x77);
                     world::make_record(c, world::club(6), 0x77);
                     // People: first words against the limit; each has a rating record at +0x14 and no club.
                     for (unsigned i = 0; i < 40; ++i) {
                         const auto person = people + i * 0x80;
                         c.put32(person, firsts[next_random(state) % firsts.size()]);
                         c.put32(person + 0x14, next_random(state) % 4 == 0 ? 0 : people + 0x4000 + (i % 8) * 0x20);
                         c.put32(person + 0x24, 0);
                     }
                     for (unsigned i = 0; i < 8; ++i) c.put8(people + 0x4000 + i * 0x20 + 0xf, next_random(state) % 3 == 0 ? 18 : 10);
                     for (const auto base : {world::club(0), world::club(1), world::club(2), world::club(5), world::club(6)})
                         for (unsigned i = 0; i < 50; ++i) {
                             const auto pick = next_random(state);
                             c.put32(base + 0x98 + 4 * i, pick % 6 == 0 ? 0 : people + (pick >> 4) % 40 * 0x80);
                         }
                 },
                 seed % 3 == 0});
}

// 0x100d0540's trimming with exact member counts: each member has its own club
// object holding the person at +0x80 (kind 1), so remove_from_record visibly
// clears the removed ones. The members are club 0's own list (n1) and its club's
// (the rest); the last `rated` members are rated 15 or more.
void trim_exact() {
    constexpr std::uint32_t persons = data + 0xc000, clubs_of = data + 0x11000, ratings = data + 0x15000;
    const auto emit = [](unsigned n1, unsigned total, unsigned rated) {
        add({0x100d0540, "members " + std::to_string(total) + " (" + std::to_string(n1) + " own), rated " + std::to_string(rated),
             [=](Cpu& c) {
                 world::place(c);
                 c.put32(high_word, 3);  // three clubs; the other two have no club
                 c.put32(c.r(2) - 0x796c, data + 0x1b00);
                 c.put32(c.r(2) - 0x7be8, table_word);
                 world::make_record(c, world::club(0), 5);
                 world::make_record(c, world::club(1), 0x77);
                 world::make_record(c, world::club(2), 0x77);
                 world::make_record(c, world::club(6), 0x77);
                 for (unsigned i = 0; i < 50; ++i) {
                     c.put32(world::club(0) + 0x98 + 4 * i, 0);
                     c.put32(world::club(6) + 0x98 + 4 * i, 0);
                 }
                 for (unsigned j = 0; j < total; ++j) {
                     const auto person = persons + j * 0x80, owner_club = clubs_of + j * 0x90, rating = ratings + j * 0x10;
                     c.put32(person, 5);
                     c.put32(person + 0x14, rating);
                     c.put32(person + 0x24, owner_club);
                     c.put8(person + 0x60, 1);
                     c.put8(person + 0x62, 1);
                     c.put8(rating + 0xf, j >= total - rated ? 18 : 10);
                     c.put32(owner_club, 5);
                     c.put32(owner_club + 0x80, person);
                     const auto holder = j < n1 ? world::club(0) : world::club(6);
                     c.put32(holder + 0x98 + 4 * (j < n1 ? j : j - n1), person);
                 }
             },
             total % 2 == 0});
    };
    for (const unsigned total : {47u, 48u, 49u, 50u}) emit(30, total, 0);
    for (const unsigned rated : {0u, 1u, 2u, 3u, 4u, 5u, 6u}) {
        emit(30, 53, rated);
        emit(30, 49, rated);
        emit(50, 100, rated);
    }
    emit(50, 100, 0);
    emit(10, 99, 3);
}
}

std::vector<Fixture> record_fixtures() {
    fixtures.clear();
    date_difference();
    club_at();
    range_position();
    record_pair();
    record_kind();
    contract_rating();
    compare_rating();
    record_matches();
    format_date_gap();
    classify_by_slots();
    attribute_by_number();
    attribute_by_number_31();
    record_accepts_id();
    add_to_record();
    build_slot_summaries();
    club_of_code();
    count_valid();
    id_word_94();
    person_attributes();
    in_second_set();
    classes_compatible();
    copy_ids();
    copy_member_ids();
    collect_ids();
    id_word_90_chain();
    club_less_candidate();
    eligible_record();
    count_members_kept();
    trim_club_members();
    trim_exact();
    return std::move(fixtures);
}
}
