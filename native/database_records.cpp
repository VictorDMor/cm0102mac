// DATABASE.CPP's records block (database_records.hpp): record accessors that
// report an out-of-range argument through the game's assert macro.
#include "database.hpp"
#include "database_records.hpp"
#include "database_records_detail.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Frame;
using guest::Saves;
using namespace records;

// The compiler's signed three-way test `x > y`-style idiom shared by 0x100c7d00
// and 0x100ce250: t = high - twice_or_once_low; the answer is the sign of x
// (0 or -1), plus t's sign bit, plus the carry of x - t (`subfc`), as a signed
// byte. XER[CA] is left as `adde` leaves it.
std::int32_t position_in_range(Cpu& c, std::uint32_t x, std::uint32_t t) {
    const std::uint32_t sign = std::uint32_t(std::int32_t(x) >> 31), top = t >> 31;
    const std::uint32_t carry = x >= t;
    const std::uint64_t sum = std::uint64_t(sign) + top + carry;
    c.s.wr<1>(guest::reg::xer_ca, std::uint8_t(sum >> 32));
    c.set(4, sign);
    return std::int8_t(std::uint32_t(sum));
}
}

// 0x100c6a10: two dates' signed difference as a byte (inferred: the 16-bit
// fields at +0x34 and +0x36 of the first record, +0 and +2 of the second).
std::int32_t date_difference(Cpu& c, std::uint32_t a, std::uint32_t b) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c6a10, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(slot::text_pool), message = c.toc(slot::message);
    c.set({{31, pool}, {30, message}});
    if (compare_logical(c, a) == 0) {
        fail_assertion(c, frame, 0x100c6a48, 0x30d4, 0x40, 0x38, 0xffffffff, true);
        return -1;
    }
    const auto first = guest::extended(c.i16(a + 0x34));
    const auto second = guest::extended(c.i16(b));
    c.set({{6, first}, {5, second}});
    const auto gap = std::int32_t(first - second);
    std::uint32_t answer;
    if (compare(c, gap) > 0) {
        const auto day_a = c.u16(a + 0x36);
        const auto day_b = c.u16(b + 2);
        c.set({{3, day_a}, {4, day_b}});
        answer = std::uint32_t(std::int8_t(day_b - (day_a + 1)));
    } else {
        const auto day_a = c.u16(a + 0x36);  // read before the second record's, as the original
        const auto day_b = c.u16(b + 2);
        c.set(3, day_b);
        answer = std::uint32_t(std::int8_t(day_b - day_a));
    }
    c.set(3, answer);
    return std::int32_t(answer);
}

namespace {
// 0x100c76c0 and 0x100c7860: the club of a record's slot, `bias` being the
// range words' difference (doubled for the first). Three asserts, in order:
// no record, no club table, club table not ready.
std::uint32_t club_at_biased_index(Cpu& c, std::uint32_t entry, std::uint32_t record, bool doubled) {
    const std::uint32_t sites[3] = {entry + 0x44, entry + 0xa8, entry + 0x110};
    const std::uint32_t lines[3] = {doubled ? 0x32ecu : 0x330eu, doubled ? 0x32f3u : 0x3315u,
                                    doubled ? 0x32fau : 0x331cu};
    c.set(0, c.lr());
    Frame frame(c, entry, 0xc0, {.first_gpr = 29});
    const auto pool = c.toc(slot::text_pool), message = c.toc(slot::message), success = c.toc(slot::success);
    const auto table_slot = c.toc(slot::club_table);
    c.set({{31, pool}, {30, message}, {29, success}, {6, table_slot}});
    if (compare_logical(c, record) == 0) {
        fail_assertion(c, frame, sites[0], lines[0], 0x90, 0x48, 0, false);
        return 0;
    }
    const auto table = c.u32(table_slot);
    c.set(0, table);
    if (compare_logical(c, table) == 0) {
        fail_assertion(c, frame, sites[1], lines[1], 0x70, 0x40, 0, false);
        return 0;
    }
    const auto ready_word = c.toc(slot::club_table_ready);
    c.set(4, ready_word);
    const auto ready = c.u32(ready_word);
    c.set(0, ready);
    if (compare_logical(c, ready) == 0) {
        fail_assertion(c, frame, sites[2], lines[2], 0x50, 0x38, 0, false);
        return 0;
    }
    const auto low_slot = c.toc(slot::range_low), high_slot = c.toc(slot::range_high);
    const auto low = c.u32(low_slot);
    const auto high = c.u32(high_slot);
    const auto bias = high - (doubled ? low << 1 : low);
    const auto index = c.u32(record + 0x68) + bias;
    const auto club = c.u32(table_slot) + index * club_stride;  // 32-bit wrap
    c.set({{5, low}, {4, c.u32(table_slot)}});
    c.set(3, club);
    return club;
}
}

std::uint32_t club_at_doubled_bias(Cpu& c, std::uint32_t record) { return club_at_biased_index(c, 0x100c76c0, record, true); }
std::uint32_t club_at_bias(Cpu& c, std::uint32_t record) { return club_at_biased_index(c, 0x100c7860, record, false); }

// 0x100c7d00: the idiom above on the record's first word against the range
// words (inferred: where the record lies in the range).
std::int32_t range_position_doubled(Cpu& c, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c7d00, 0x70, {.first_gpr = 30});
    c.set({{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}});
    if (compare_logical(c, record) == 0) {
        fail_assertion(c, frame, 0x100c7d38, 0x33ef, 0x40, 0x38, 0, true);
        return 0;
    }
    const auto high_slot = c.toc(slot::range_high), low_slot = c.toc(slot::range_low);
    const auto low = c.u32(low_slot);
    const auto word = c.u32(record);
    const auto high = c.u32(high_slot);
    const auto t = high - (low << 1);
    c.set({{5, low}, {6, word}});
    c.set(3, std::uint32_t(position_in_range(c, word, t)));
    return std::int32_t(c.r(3));
}

// 0x100ce250: the same with the range words' plain difference.
std::int32_t range_position(Cpu& c, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x100ce250, 0x70, {.first_gpr = 30});
    c.set({{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}});
    if (compare_logical(c, record) == 0) {
        fail_assertion(c, frame, 0x100ce288, 0x42e9, 0x40, 0x38, 0, true);
        return 0;
    }
    const auto high_slot = c.toc(slot::range_high), low_slot = c.toc(slot::range_low);
    c.set({{4, high_slot}, {5, low_slot}});
    const auto word = c.u32(record);
    const auto high = c.u32(high_slot);  // high, then low, as the original
    const auto low = c.u32(low_slot);
    const auto t = high - low;
    c.set({{6, word}});
    c.set(3, std::uint32_t(position_in_range(c, word, t)));
    return std::int32_t(c.r(3));
}

namespace {
// The three-byte values 0x100c7a50 and 0x100c7bc0 hand to 0x1076bb20.
std::uint16_t packed(Cpu& c, std::uint32_t record, std::uint32_t back) {
    c.set({{3, c.u8(record + 0x37)}, {6, 0}, {4, c.u8(record + 0x38)}, {5, c.u8(record + 0x39)}});
    c.call(pack_colour, back);
    return std::uint16_t(c.r(3));
}
}

// 0x100c7bc0: two packed 16-bit values of a record's two linked records, into
// the two outputs (inferred: a pair of colours); the outputs first get the
// module's default shorts. The first link comes from +8's record when it has
// one (its +0x80/+0x84), else from +0xc/+0x10.
bool record_pair_values(Cpu& c, std::uint32_t record, std::uint32_t first_out, std::uint32_t second_out) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c7bc0, 0x70, {.first_gpr = 28});
    const auto pool = c.toc(slot::text_pool), message = c.toc(slot::message);
    const auto default_a = c.toc(slot::short_a), default_b = c.toc(slot::short_b);
    c.set({{6, default_a}, {4, default_b}});
    guest::keep(c, {{31, second_out}, {30, first_out}, {29, pool}, {28, message}});
    c.set(0, c.u16(default_a));
    c.put16(first_out, c.r(0));
    c.set(0, c.u16(default_b));
    c.put16(second_out, c.r(0));
    if (compare_logical(c, record) == 0) {
        fail_assertion(c, frame, 0x100c7c20, 0x33ba, 0x40, 0x38, 0, true);
        return false;
    }
    std::uint32_t first, second;
    const auto owner = c.u32(record + 8);
    c.set(0, owner);
    if (compare_logical(c, owner) != 0) {
        first = c.u32(owner + 0x80);
        second = c.u32(owner + 0x84);
        c.set({{4, owner}, {3, owner}});
    } else {
        first = c.u32(record + 0xc);
        second = c.u32(record + 0x10);
    }
    c.set(5, first);
    guest::keep(c, {{28, second}});
    if (compare_logical(c, first) != 0) {
        c.put16(first_out, packed(c, first, 0x100c7cac));
    }
    if (compare_logical(c, second) != 0) {
        c.put16(second_out, packed(c, second, 0x100c7ccc));
    }
    c.set(3, 1);
    return true;
}

// 0x100c7a50: the same two values from the record's link pair chosen by
// `kind` (1: +0x44/+0x48; 2 and 3: +0x4c/+0x50 or +0x54/+0x58 when both are
// set, else +0x48/+0x44; any other kind: none).
bool record_kind_values(Cpu& c, std::uint32_t record, std::uint32_t kind, std::uint32_t first_out,
                        std::uint32_t second_out) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c7a50, 0x80, {.first_gpr = 27});
    const auto pool = c.toc(slot::text_pool), message = c.toc(slot::message);
    const auto default_a = c.toc(slot::short_c), default_b = c.toc(slot::short_a);
    guest::keep(c, {{29, first_out}, {27, message}, {30, second_out}, {28, pool}, {31, 0}});
    c.set({{7, default_a}, {5, default_b}});
    compare_logical(c, record);  // cmplwi r3,0 (before the stores; the answer is tested after)
    c.set(0, c.u16(default_a));
    c.set(7, 0);
    c.put16(first_out, c.r(0));
    c.set(0, c.u16(default_b));
    c.put16(second_out, c.r(0));
    if (record == 0) {
        fail_assertion(c, frame, 0x100c7aac, 0x3363, 0x40, 0x38, 0, true);
        return false;
    }
    const auto selected = kind & 0xff;
    std::uint32_t first = 0, second = 0;
    const auto link_pair = [&](std::uint32_t at_first, std::uint32_t at_second) {
        first = c.u32(record + at_first);
        second = c.u32(record + at_second);
        c.set(7, first);
        guest::keep(c, {{31, second}});
    };
    const auto fallback = [&] { link_pair(0x48, 0x44); };
    c.set(0, selected);
    if (compare_logical(c, selected, 1) == 0) {
        link_pair(0x44, 0x48);
    } else if (compare_logical(c, selected, 2) == 0) {
        link_pair(0x4c, 0x50);
        if (compare_logical(c, first) == 0 || compare_logical(c, second) == 0) fallback();
    } else if (compare_logical(c, selected, 3) == 0) {
        link_pair(0x54, 0x58);
        if (compare_logical(c, first) == 0 || compare_logical(c, second) == 0) fallback();
    }
    if (compare_logical(c, first) != 0) c.put16(first_out, packed(c, first, 0x100c7b84));
    if (compare_logical(c, second) != 0) c.put16(second_out, packed(c, second, 0x100c7ba4));
    c.set(3, 1);
    return true;
}

// 0x100ca010: whether a club contract exists for the person and club: the
// club's index must be below the range words' difference (the second
// parameter is a pointer to it), the person's below the club count, and
// CONTRACT_MANAGER must find the contract. The answer is the flags' high
// nibble less 10, as a byte; 0 when anything fails. The original also tests
// the club pointer for null after its null exit, a check that cannot fail
// (so no assert is reachable).
std::int32_t contract_rating(Cpu& c, std::uint32_t person, std::uint32_t club_index) {
    c.set(0, c.lr());
    Frame frame(c, 0x100ca010, 0x80, {.first_gpr = 27});
    const auto low_slot = c.toc(slot::range_low), success = c.toc(slot::success), message = c.toc(slot::message),
               pool = c.toc(slot::text_pool), high_slot = c.toc(slot::range_high);
    guest::keep(c, {{31, club_index}, {27, success}, {30, person}, {28, message}, {29, pool}});
    c.set({{5, low_slot}, {4, high_slot}});
    if (compare(c, std::int32_t(club_index)) == 0) {  // or. r31,r4,r4
        c.set(3, 0);
        return 0;
    }
    compare_logical(c, club_index);  // bne: always taken (the assert is dead code)
    const bool below = below_range_limit(c, club_index);  // low, high, then the index
    if (!below) {
        c.set(3, 0);
        return 0;
    }
    const auto count_slot = c.toc(slot::club_count);
    c.set(3, count_slot);
    const auto person_index = c.u32(person);
    const auto count = c.u32(count_slot);
    c.set({{4, person_index}, {0, count}});
    compare(c, std::int32_t(person_index), std::int32_t(count));
    if (!(std::int32_t(person_index) < std::int32_t(count))) {
        c.set(3, 0);
        return 0;
    }
    const auto manager_slot = c.toc(slot::contract_manager);
    c.set({{3, manager_slot}, {4, person}, {5, club_index}});
    c.set(3, c.u32(manager_slot));
    c.call_in_place(club_contract_lookup, 0x100ca108);
    const auto contract = c.r(3);
    if (compare_logical(c, contract) == 0) {
        c.set(3, 0);
        return 0;
    }
    const auto nibble = (std::uint32_t(c.u8(contract + 0x60)) >> 4) & 0xf;
    c.set(0, nibble);
    const auto answer = std::uint32_t(std::int8_t(nibble - 10));
    c.set(3, answer);
    return std::int32_t(answer);
}

namespace {
// 0x100cd060 and 0x100cd180: compare the doubles at `field` of the records
// each argument's word points through (inferred: sort comparators on a
// rating pair). A missing record asserts; a missing value counts as the
// pool's default.
std::int32_t compare_through_pointers(Cpu& c, std::uint32_t entry, std::uint32_t a, std::uint32_t b,
                                      std::uint32_t field, std::uint32_t line) {
    c.set(0, c.lr());
    Frame frame(c, entry, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(slot::number_pool);
    guest::Fpu fpu(c);
    c.set({{5, pool}, {30, c.toc(slot::message)}, {31, c.toc(slot::text_pool)}});
    c.set_f(0, c.f64(pool + 0x40));
    c.set_f(1, c.f64(pool + 0x40));
    const auto first = c.u32(a), second = c.u32(b);
    c.set({{3, first}, {4, second}});
    if (compare_logical(c, first) == 0 || compare_logical(c, second) == 0) {
        fail_assertion(c, frame, entry + 0x54, line, 0x40, 0x38, 0, true);
        return 0;
    }
    const auto linked = [&](std::uint32_t record, unsigned reg) {
        const auto link = c.u32(record + 0x3c);
        c.set(0, link);
        std::uint32_t value = 0;
        if (compare_logical(c, link) != 0) {
            c.set(3, link);
            value = c.u32(link + 0x10);
        }
        c.set(reg, value);
        return value;
    };
    const auto left = linked(first, 5);
    const auto right = linked(second, 3);
    double x = c.f(0), y = c.f(1);
    if (compare_logical(c, left) != 0) c.set_f(0, x = c.f64(left + field));
    if (compare_logical(c, right) != 0) c.set_f(1, y = c.f64(right + field));
    fpu.fcmp(0, x, y);
    if (x < y) {
        c.set(3, 0xffffffff);
        return -1;
    }
    fpu.fcmp(0, x, y);
    const std::uint32_t greater = c.cr(0) >> 2 & 1;  // mfcr; rlwinm r3,r0,2,31,31
    c.set(3, greater);
    return std::int32_t(greater);
}
}

std::int32_t compare_rating_first(Cpu& c, std::uint32_t a, std::uint32_t b) {
    return compare_through_pointers(c, 0x100cd060, a, b, 0x0, 0x3f28);
}
std::int32_t compare_rating_second(Cpu& c, std::uint32_t a, std::uint32_t b) {
    return compare_through_pointers(c, 0x100cd180, a, b, 0x8, 0x3f4f);
}

// 0x100cbde0: whether a record's three linked records carry the three ids
// (their word +0x20, compared signed; an absent link matches anything) and,
// when `date` is given, its 16-bit pair is the record's (+0x34, +0x38) pair:
// the low half of +0x34 against the date's +2, then the high half against
// its +0. A record with neither +0x14 nor +0x1c never matches. The two words
// are copied below the frame's stack pointer first, as the original does.
bool record_matches(Cpu& c, std::uint32_t record, std::uint32_t first_id, std::uint32_t second_id,
                    std::uint32_t third_id, std::uint32_t date) {
    c.set(0, c.lr());
    Frame frame(c, 0x100cbde0, 0x70, {.first_gpr = 30});
    c.set({{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}});
    if (compare_logical(c, record) == 0) {
        fail_assertion(c, frame, 0x100cbe18, 0x3cd1, 0x48, 0x40, 0, true);
        return false;
    }
    const auto no = [&] {
        c.set(3, 0);
        return false;
    };
    const auto first_guard = c.u32(record + 0x14);
    c.set(0, first_guard);
    if (compare_logical(c, first_guard) == 0) {
        const auto second_guard = c.u32(record + 0x1c);
        c.set(0, second_guard);
        if (compare_logical(c, second_guard) == 0) return no();
    }
    // Each link in turn: r8 (first only) or r4 holds the link, r0 its id.
    const auto link_matches = [&](std::uint32_t at, unsigned link_register, std::uint32_t id) {
        const auto link = c.u32(record + at);
        c.set(0, link);
        if (compare_logical(c, link) == 0) return true;
        c.set(link_register, link);
        const auto own = c.u32(link + 0x20);
        c.set(0, own);
        return compare(c, std::int32_t(own), std::int32_t(id)) == 0;
    };
    if (!link_matches(0x28, 8, first_id) || !link_matches(0x2c, 4, second_id) || !link_matches(0x30, 4, third_id))
        return no();
    if (compare_logical(c, date) == 0) {
        c.set(3, 1);
        return true;
    }
    const auto word = c.u32(record + 0x34), next = c.u32(record + 0x38);
    c.set(0, word);
    c.set(4, 0);
    c.set(5, next);
    c.put32(frame.at(0x38), word);
    const auto low = std::uint32_t(word & 0xffff);
    const auto date_day = c.u16(date + 2);
    c.set(0, date_day);
    c.set(3, low);
    c.put32(frame.at(0x3c), next);
    bool same = false;
    if (compare_logical(c, low, date_day) == 0) {
        const auto high = guest::extended(std::int16_t(word >> 16));
        c.set(3, high);
        c.set(0, guest::extended(c.i16(date)));
        if (compare(c, std::int32_t(high), std::int32_t(c.r(0))) == 0) {
            same = true;
            c.set(4, 1);
        }
    }
    compare(c, same);
    c.set(3, same);
    return same;
}

// 0x100cd7c0: formats the gap between two dates, in days (inferred: the
// first record's +2 less the second's, less one when the second's year is
// later), as text into `buffer` of `capacity` >= 5 bytes: a zero byte first,
// then "%d" of the 16-bit value (the pool's format at +0x1b1d).
bool format_date_gap(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t buffer, std::int32_t capacity) {
    c.set(0, c.lr());
    Frame frame(c, 0x100cd7c0, 0xc0, {.first_gpr = 29});
    const auto pool = c.toc(slot::text_pool);
    guest::keep(c, {{31, pool}, {30, c.toc(slot::message)}, {29, c.toc(slot::success)}});
    compare_logical(c, buffer);
    if (buffer == 0 || compare(c, capacity, 5) < 0) {
        fail_assertion(c, frame, 0x100cd808, 0x417f, 0x90, 0x48, 0, false);
        return false;
    }
    c.set(0, 0);
    compare_logical(c, a);
    c.put8(buffer, 0);
    if (a == 0) {
        fail_assertion(c, frame, 0x100cd870, 0x418a, 0x70, 0x40, 0, false);
        return false;
    }
    if (compare_logical(c, b) == 0) {
        fail_assertion(c, frame, 0x100cd8d0, 0x4192, 0x50, 0x38, 0, false);
        return false;
    }
    const auto year_b = guest::extended(c.i16(b)), year_a = guest::extended(c.i16(a));
    const auto day_b = c.u16(b + 2), day_a = c.u16(a + 2);
    c.set({{7, year_b}, {6, day_b}, {4, year_b}, {3, day_a}});
    compare(c, std::int32_t(year_b), std::int32_t(year_a));
    auto gap = std::uint32_t(day_a - day_b) & 0xffff;
    if (std::int32_t(year_b) > std::int32_t(year_a)) gap = (gap - 1) & 0xffff;
    c.set({{0, gap}, {3, buffer}, {4, pool + 0x1b1d}, {5, gap}});
    c.call(0x1077b720, 0x100cd954);
    c.set(3, 1);
    return true;
}

// 0x100cdae0: which of ten classes (0-9... as numbered by the original) the
// value at `entry` belongs to, by comparing it, signed, with the words the
// TOC slots point at, in order; a value that matches none asserts and
// answers 2. The classes and their slots are the table below (inferred:
// kinds of a numbered list).
std::int32_t classify_by_slots(Cpu& c, std::uint32_t entry) {
    struct Class {
        std::int32_t slot;
        std::uint32_t answer;
    };
    static constexpr Class classes[] = {
        {-0x7668, 10}, {-0x766c, 10}, {-0x7670, 1}, {-0x7674, 1}, {-0x7678, 2}, {-0x767c, 3}, {-0x7680, 3},
        {-0x7684, 3},  {-0x7688, 8},  {-0x768c, 8}, {-0x7690, 8}, {-0x7694, 8}, {-0x7698, 9}, {-0x769c, 9},
        {-0x76a0, 4},  {-0x76a4, 4},  {-0x76a8, 4}, {-0x76ac, 4}, {-0x76b0, 5}, {-0x76b4, 5}, {-0x76b8, 5},
        {-0x76bc, 5},  {-0x76c0, 5},  {-0x76c4, 6}, {-0x76c8, 6}, {-0x76cc, 6}, {-0x76d0, 6}, {-0x76d4, 6},
        {-0x76d8, 6},  {-0x76dc, 7},  {-0x76e0, 7}, {-0x7664, 7}, {-0x76e4, 7}, {-0x76e8, 7}};
    c.set(0, c.lr());
    Frame frame(c, 0x100cdae0, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(slot::message)}, {31, c.toc(slot::text_pool)}});
    std::uint32_t value = 0;
    for (const auto& [at, answer] : classes) {
        const auto word = c.toc(at);
        c.set(4, word);
        const bool last = at == -0x76e8;
        value = c.u32(entry);
        c.set(last ? 3 : 5, value);
        const auto expected = c.u32(word);
        c.set(0, expected);
        if (compare(c, std::int32_t(value), std::int32_t(expected)) == 0) {
            c.set(3, answer);
            return std::int32_t(answer);
        }
    }
    return fail_assertion(c, frame, 0x100cde08, 0x4206, 0x40, 0x38, 2, true);
}

// 0x100d2b40: attribute number 1-12 of a person: bytes of the first record
// (1: +0x61, 4: +0x64) or of the second, either raw (7: +0x34, 12: +0x42) or
// through attribute_rating (2, 3, 5, 6, 8-11; offsets in the table). Answers
// -1 after an assert for a null record, an index outside 1-12 or a null second
// record. The jump table's case 0 and its range check cannot be reached.
std::int32_t attribute_by_number(Cpu& c, std::uint32_t first, std::uint32_t second, std::int8_t number) {
    struct Attribute {
        std::uint32_t handler, offset;
        bool from_first, rated;
    };
    static constexpr Attribute attributes[13] = {
        {0, 0, false, false},          {0x100d2c60, 0x61, true, false},  {0x100d2c68, 0x31, false, true},
        {0x100d2c78, 0x30, false, true}, {0x100d2c88, 0x64, true, false}, {0x100d2c90, 0x37, false, true},
        {0x100d2ca0, 0x38, false, true}, {0x100d2cb0, 0x34, false, false}, {0x100d2cb8, 0x39, false, true},
        {0x100d2cc8, 0x3b, false, true}, {0x100d2cd8, 0x3e, false, true}, {0x100d2ce8, 0x41, false, true},
        {0x100d2cf8, 0x42, false, false}};
    c.set(0, c.lr());
    Frame frame(c, 0x100d2b40, 0xc0, {.first_gpr = 29});
    guest::keep(c, {{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}, {29, c.toc(slot::success)}});
    c.set(6, second);
    const auto fail = [&](std::uint32_t site, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
        fail_assertion(c, frame, site, line, name, extension, 0xffffffff, false);
        return -1;
    };
    if (compare_logical(c, first) == 0) return fail(0x100d2b9c, 0x49a7, 0x90, 0x48);
    const auto index = guest::extended(number);  // the parameter (the hook passes r5's low byte): range check and table index
    c.set(0, index);
    if (compare(c, std::int32_t(index), 1) < 0) return fail(0x100d2b9c, 0x49a7, 0x90, 0x48);
    if (compare(c, std::int32_t(index), 0xc) > 0) return fail(0x100d2b9c, 0x49a7, 0x90, 0x48);
    if (compare_logical(c, second) == 0) return fail(0x100d2bfc, 0x49ae, 0x70, 0x40);
    compare_logical(c, index, 0xc);  // bgt: never taken after the checks above
    const auto& attribute = attributes[number];
    c.set(0, index << 2);
    c.set(4, attribute.handler);
    c.s.wr<4>(guest::reg::ctr, attribute.handler);
    const auto record = attribute.from_first ? first : second;
    const auto byte = std::uint32_t(c.u8(record + attribute.offset));
    if (!attribute.rated) {
        c.set(3, byte);
        return std::int32_t(byte);
    }
    c.set(4, byte);
    guest::enter(c, attribute.handler + 0xc, {second, byte});
    return database::attribute_rating(c, second, std::int8_t(byte));
}

// 0x100d2330: attribute number 1-31 of a person's second record. Most are a
// byte (of the first record for 10, +0x64): raw, or sign-extended (25). The
// rest go through attribute_rating_04 (0x100d2e60), or attribute_rating_tenth
// (0x100d2d70) when the record's position byte (+0xf) is 20 exactly (cases
// 7, 8, 11, 12, 19, 20), or, for 14 and 24, below 15 as a signed byte. The
// jump table's case 0 (an assert) cannot be reached after the range checks.
std::int32_t attribute_by_number_31(Cpu& c, std::uint32_t first, std::uint32_t second, std::int8_t number) {
    enum class How { raw, raw_signed, raw_first, rating, rating_if_20, rating_if_15 };
    struct Attribute {
        std::uint32_t handler;
        std::uint32_t offset;
        How how;
    };
    constexpr auto base = 0x100d0000u;
    static constexpr Attribute attributes[32] = {
        {0, 0, How::raw},
        {base + 0x2450, 0x1b, How::raw},          {base + 0x2468, 0x1c, How::raw},
        {base + 0x2470, 0x1d, How::raw},          {base + 0x2478, 0x1e, How::rating},
        {base + 0x2488, 0x1f, How::raw},          {base + 0x2490, 0x20, How::raw},
        {base + 0x2498, 0x43, How::rating_if_20}, {base + 0x24c4, 0x23, How::rating_if_20},
        {base + 0x2458, 0x24, How::rating},       {base + 0x24f0, 0x64, How::raw_first},
        {base + 0x24f8, 0x26, How::rating_if_20}, {base + 0x2638, 0x27, How::rating_if_20},
        {base + 0x2524, 0x28, How::raw},          {base + 0x252c, 0x2a, How::rating_if_15},
        {base + 0x255c, 0x2b, How::rating},       {base + 0x256c, 0x2f, How::raw},
        {base + 0x2574, 0x2e, How::raw},          {base + 0x2694, 0x31, How::rating},
        {base + 0x257c, 0x32, How::rating_if_20}, {base + 0x25a8, 0x33, How::rating_if_20},
        {base + 0x25d4, 0x36, How::raw},          {base + 0x25dc, 0x37, How::rating},
        {base + 0x25ec, 0x39, How::rating},       {base + 0x25fc, 0x3a, How::rating_if_15},
        {base + 0x262c, 0x29, How::raw_signed},   {base + 0x2664, 0x3c, How::raw},
        {base + 0x266c, 0x3d, How::raw},          {base + 0x2674, 0x3e, How::rating},
        {base + 0x2684, 0x3f, How::raw},          {base + 0x268c, 0x40, How::raw},
        {base + 0x26a4, 0x44, How::raw}};
    c.set(0, c.lr());
    Frame frame(c, 0x100d2330, 0xc0, {.first_gpr = 29});
    guest::keep(c, {{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}, {29, c.toc(slot::success)}});
    c.set(6, second);
    const auto fail = [&](std::uint32_t site, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
        fail_assertion(c, frame, site, line, name, extension, 0xffffffff, false);
        return -1;
    };
    if (compare_logical(c, first) == 0) return fail(0x100d238c, 0x483d, 0x90, 0x48);
    const auto index = guest::extended(number);  // the parameter (the hook passes r5's low byte): range check and table index
    c.set(0, index);
    if (compare(c, std::int32_t(index), 1) < 0) return fail(0x100d238c, 0x483d, 0x90, 0x48);
    if (compare(c, std::int32_t(index), 0x1f) > 0) return fail(0x100d238c, 0x483d, 0x90, 0x48);
    if (compare_logical(c, second) == 0) return fail(0x100d23ec, 0x4844, 0x70, 0x40);
    compare_logical(c, index, 0x1f);  // bgt: never taken after the checks above
    const auto& attribute = attributes[number];
    c.set(0, index << 2);
    c.set(4, attribute.handler);
    c.s.wr<4>(guest::reg::ctr, attribute.handler);
    const auto record = attribute.how == How::raw_first ? first : second;
    const auto byte = std::uint32_t(c.u8(record + attribute.offset));
    if (attribute.how == How::raw || attribute.how == How::raw_first) {
        c.set(3, byte);
        return std::int32_t(byte);
    }
    if (attribute.how == How::raw_signed) {
        c.set(0, byte);
        const auto extended = guest::extended(std::int8_t(byte));
        c.set(3, extended);
        return std::int32_t(extended);
    }
    // The position byte first, then the rating's own byte.
    bool tenth = false;
    std::uint32_t back = attribute.handler + 0xc;
    if (attribute.how != How::rating) {
        const auto position = std::uint32_t(c.u8(second + 0xf));
        c.set(0, position);
        if (attribute.how == How::rating_if_20) {
            tenth = compare(c, std::int32_t(position), 0x14) == 0;
            back = attribute.handler + (tenth ? 0x18 : 0x28);
        } else {
            const auto signed_position = guest::extended(std::int8_t(position));
            c.set(0, signed_position);
            tenth = compare(c, std::int32_t(signed_position), 0xf) < 0;
            back = attribute.handler + (tenth ? 0x1c : 0x2c);
        }
    }
    c.set(4, byte);
    guest::enter(c, back, {second, byte});
    return tenth ? database::attribute_rating_tenth(c, second, std::int8_t(byte))
                 : database::attribute_rating_04(c, second, std::int8_t(byte));
}

namespace {
// A list of `count` ids at record+offset, as the original's loops test it:
// an empty (zero) entry or a match ends the scan, and the scan has found
// the id when it ended before `count`. Leaves what the loop leaves: r4 the
// index, r6 the last entry's address and r0 its word (when one was read),
// and cr0 from the last compare.
bool list_accepts(Cpu& c, std::uint32_t record, std::uint32_t offset, std::int32_t count, std::uint32_t id) {
    std::int32_t index = 0;
    c.set(4, 0);
    for (; compare(c, index, count) < 0; c.set(4, ++index)) {
        const auto entry = record + (std::uint32_t(index) << 2);
        c.set(6, entry);
        const auto word = c.u32(entry + offset);
        c.set(0, word);
        if (compare_logical(c, word) == 0) break;
        if (compare_logical(c, word, id) == 0) break;
    }
    return compare(c, index, count) < 0;
}
// A single id word: empty or equal.
bool word_accepts(Cpu& c, std::uint32_t record, std::uint32_t offset, std::uint32_t id) {
    c.set(0, c.u32(record + offset));
    if (compare_logical(c, c.u32(record + offset)) == 0) return true;
    c.set(0, c.u32(record + offset));
    return compare_logical(c, c.u32(record + offset), id) == 0;
}
}

// 0x100c7dd0: whether a record (kind 1-13 or 15, a signed byte) accepts an id:
// every id word or list the kind consults is empty (an empty entry ends a
// list scan as a match) or holds it. The word at +0x80 (1), the list at +0x84
// (2-4, three entries), words +0x90 (5, and 12 with the 50-entry list at +0x98)
// and +0x94 (6, 7 and 13 with the same list), lists +0x160 (8, five; 15 with
// the +0x98 list), +0x174 (9, seven) and +0x190 (10, three), +0x98 (11).
// Kinds 0 and 14 and anything past 15 assert, as do a null record or id.
bool record_accepts_id(Cpu& c, std::uint32_t record, std::int8_t kind, std::uint32_t id) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c7dd0, 0xc0, {.first_gpr = 29});
    guest::keep(c, {{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}, {29, c.toc(slot::success)}});
    const auto fail = [&](std::uint32_t site, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
        fail_assertion(c, frame, site, line, name, extension, 0, false);
        return false;
    };
    const auto answer = [&](bool value) {
        c.set(3, value);
        return value;
    };
    if (compare_logical(c, record) == 0) return fail(0x100c7e10, 0x340e, 0x90, 0x48);
    if (compare_logical(c, id) == 0) return fail(0x100c7e70, 0x3415, 0x70, 0x40);
    const auto index = guest::extended(kind);
    c.set(0, index);
    if (compare_logical(c, index, 0xf) > 0) return fail(0x100c8234, 0x34d4, 0x50, 0x38);
    constexpr std::uint32_t handlers[16] = {0x100c8220, 0x100c7ed4, 0x100c7ef4, 0x100c7ef4, 0x100c7ef4, 0x100c7f40,
                                            0x100c7f60, 0x100c7f60, 0x100c7f80, 0x100c7fcc, 0x100c8018, 0x100c8064,
                                            0x100c80b0, 0x100c811c, 0x100c8220, 0x100c8188};
    c.set(0, index << 2);
    c.set(4, handlers[index]);
    c.s.wr<4>(guest::reg::ctr, handlers[index]);
    switch (index) {
    case 1: return answer(word_accepts(c, record, id_slot::word_80, id));
    case 2: case 3: case 4: return answer(list_accepts(c, record, id_slot::list_84, id_slot::list_84_count, id));
    case 5: return answer(word_accepts(c, record, id_slot::word_90, id));
    case 6: case 7: return answer(word_accepts(c, record, id_slot::word_94, id));
    case 8: return answer(list_accepts(c, record, id_slot::list_160, id_slot::list_160_count, id));
    case 9: return answer(list_accepts(c, record, id_slot::list_174, id_slot::list_174_count, id));
    case 10: return answer(list_accepts(c, record, id_slot::list_190, id_slot::list_190_count, id));
    case 11: return answer(list_accepts(c, record, id_slot::list_98, id_slot::list_98_count, id));
    case 12: return answer(word_accepts(c, record, id_slot::word_90, id) && list_accepts(c, record, id_slot::list_98, id_slot::list_98_count, id));
    case 13: return answer(word_accepts(c, record, id_slot::word_94, id) && list_accepts(c, record, id_slot::list_98, id_slot::list_98_count, id));
    case 15: return answer(list_accepts(c, record, id_slot::list_160, id_slot::list_160_count, id) && list_accepts(c, record, id_slot::list_98, id_slot::list_98_count, id));
    default: return fail(0x100c8234, 0x34d4, 0x50, 0x38);  // kinds 0 and 14
    }
}

namespace {
// The first empty entry of a list of `count` ids at record+offset, as the
// original's loop finds it (r3 the index, r4 the last entry's address
// computed, cr0 from the compares); `count` when the list is full.
std::int32_t first_empty(Cpu& c, std::uint32_t record, std::uint32_t offset, std::int32_t count) {
    std::int32_t index = 0;
    c.set(3, 0);
    for (; compare(c, index, count) < 0; c.set(3, std::uint32_t(++index))) {
        c.set(0, std::uint32_t(index) << 2);
        const auto entry = record + (std::uint32_t(index) << 2);
        c.set(4, entry);
        const auto word = c.u32(entry + offset);
        c.set(0, word);
        if (compare_logical(c, word) == 0) break;
    }
    compare(c, index, count);
    return index;
}
}

// 0x100c82a0: records `id_record` in the slot `kind` names of `record` (the
// words and lists of record_accepts_id, whose first empty entry takes it), and
// the date and flags: with a date, three fields into id_record at +0x3c or
// +0x4c (whether the record's first word is at or past the range limit); with
// the second flag, kinds 11-13 and 15 become 11 and the rest 0; an
// id_record with a link at +0x14 and no answer from 0x1014c2e0 whose first
// field exceeds 0x55 turns kind 6 into 13 and 8 into 15; with the first flag,
// the record and kind byte are stored in id_record (+0x20/+0x60 or +0x24/+0x62).
// Answers false, with no change beyond what it did first, when the slot is
// taken: kinds 12, 13 and 15 write their single word before they fill their
// list (13 and 15 turn the kind into 6 or 8 instead of failing when the list is
// full). Out of range: a null record or id_record asserts; an id_record
// index past the count less 16 needs kind 5 (inferred); kind 14 and past 15
// answer false. Asserts for a null record on the date and flag paths cannot
// be reached.
bool add_to_record(Cpu& c, std::uint32_t record, std::uint32_t id_record, std::uint32_t kind, std::uint32_t date,
                   std::uint32_t flag_a, std::uint32_t flag_b) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c82a0, 0x110, {.first_gpr = 21});
    const auto low_slot = c.toc(slot::range_low), success = c.toc(slot::success), message = c.toc(slot::message),
               pool = c.toc(slot::text_pool), high_slot = c.toc(slot::range_high);
    guest::keep(c, {{21, record}, {27, low_slot}, {28, success}, {22, id_record}, {29, message}, {23, kind},
                    {30, pool}, {24, date}, {31, high_slot}, {25, flag_a}, {26, flag_b}});
    const auto set_kind = [&](std::uint32_t value) {
        kind = value;
        guest::keep(c, {{23, value}});
    };
    const auto fail = [&](std::uint32_t site, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
        fail_assertion(c, frame, site, line, name, extension, 0, false);
        return false;
    };
    const auto no = [&] {
        c.set(3, 0);
        return false;
    };
    if (compare(c, std::int32_t(record)) == 0) return fail(0x100c82f4, 0x34f0, 0xb8, 0x50);
    if (compare_logical(c, id_record) == 0) return fail(0x100c8354, 0x34f7, 0x98, 0x48);
    {
        const auto count_pointer = c.toc(slot::club_count);
        c.set(3, count_pointer);
        const auto index = c.u32(id_record);
        c.set(4, index);
        const auto count = c.u32(count_pointer);
        c.set({{3, count}, {0, count - 16}});
        if (!(compare(c, std::int32_t(index), std::int32_t(count - 16)) < 0)) {
            const auto number = guest::extended(std::int8_t(kind));
            c.set(0, number);
            if (compare(c, std::int32_t(number), 5) != 0) return no();
        }
    }
    if (compare_logical(c, date) != 0) {
        compare_logical(c, record);  // never null here: the assert is dead code
        const bool below = below_range_limit(c, record);
        const auto fields = below ? 0x4cu : 0x3cu;  // not below (extsb. r3 = 1): +0x3c
        c.set(0, c.u16(date + 2));
        c.put16(id_record + fields + 2, c.r(0));
        c.set(0, guest::extended(c.i16(date)));
        c.put16(id_record + fields, c.r(0));
        c.set(0, c.u8(date + 4));
        c.put8(id_record + fields + 4, c.r(0));
    }
    if (compare(c, guest::extended(std::int8_t(flag_b))) != 0) {
        const auto reduced = (kind - 0xb) & 0xff;
        c.set(0, reduced);
        bool wanted;
        if (compare_logical(c, reduced, 2) <= 0) {
            wanted = true;
        } else {
            const auto number = guest::extended(std::int8_t(kind));
            c.set(0, number);
            wanted = compare(c, std::int32_t(number), 0xf) == 0;
        }
        c.set(0, wanted);
        compare(c, wanted);
        set_kind(wanted ? 0xb : 0);
    }
    const auto link = c.u32(id_record + 0x14);
    c.set(0, link);
    if (compare_logical(c, link) != 0) {
        c.set({{3, record}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(0x1014c2e0, 0x100c84f8);
        const auto answer = guest::extended(std::int8_t(c.r(3)));
        c.set(0, answer);
        if (compare(c, std::int32_t(answer)) == 0) {
            const auto owner = c.u32(id_record + 0x14);
            c.set(3, owner);
            const auto first = guest::extended(c.i16(owner + 4));
            c.set(0, first);
            if (compare(c, std::int32_t(first), 0x55) > 0) {
                auto number = guest::extended(std::int8_t(kind));
                c.set(0, number);
                if (compare(c, std::int32_t(number), 6) == 0) {
                    set_kind(0xd);
                } else {
                    number = guest::extended(std::int8_t(kind));
                    c.set(0, number);
                    if (compare(c, std::int32_t(number), 8) == 0) set_kind(0xf);
                }
            }
        }
    }
    const auto number = guest::extended(std::int8_t(kind));
    c.set(0, number);
    if (compare_logical(c, number, 0xf) > 0) return no();
    constexpr std::uint32_t handlers[16] = {0x100c8864, 0x100c8554, 0x100c8570, 0x100c8570, 0x100c8570, 0x100c85b8,
                                            0x100c85d4, 0x100c85d4, 0x100c85f0, 0x100c8638, 0x100c8680, 0x100c86c8,
                                            0x100c8710, 0x100c8770, 0x100c885c, 0x100c87d0};
    c.set(0, number << 2);
    c.set(3, handlers[number]);
    c.s.wr<4>(guest::reg::ctr, handlers[number]);
    // A single word that must be empty, then set.
    const auto put_word = [&](std::uint32_t offset) {
        const auto word = c.u32(record + offset);
        c.set(0, word);
        if (compare_logical(c, word) != 0) return false;
        c.put32(record + offset, id_record);
        return true;
    };
    // Into the first empty entry of a list; false when it is full.
    const auto put_in_list = [&](std::uint32_t offset, std::int32_t count) {
        const auto index = first_empty(c, record, offset, count);
        if (index >= count) return false;
        const auto at = record + (std::uint32_t(index) << 2);
        c.set(0, std::uint32_t(index) << 2);
        c.set(3, at);
        c.put32(at + offset, id_record);
        return true;
    };
    switch (number) {
    case 0: break;
    case 1: if (!put_word(id_slot::word_80)) return no(); break;
    case 2: case 3: case 4: if (!put_in_list(id_slot::list_84, id_slot::list_84_count)) return no(); break;
    case 5: if (!put_word(id_slot::word_90)) return no(); break;
    case 6: case 7: if (!put_word(id_slot::word_94)) return no(); break;
    case 8: if (!put_in_list(id_slot::list_160, id_slot::list_160_count)) return no(); break;
    case 9: if (!put_in_list(id_slot::list_174, id_slot::list_174_count)) return no(); break;
    case 10: if (!put_in_list(id_slot::list_190, id_slot::list_190_count)) return no(); break;
    case 11: if (!put_in_list(id_slot::list_98, id_slot::list_98_count)) return no(); break;
    case 12:
        if (!put_word(id_slot::word_90)) return no();
        if (!put_in_list(id_slot::list_98, id_slot::list_98_count)) return no();
        break;
    case 13:
        if (!put_word(id_slot::word_94)) return no();
        if (!put_in_list(id_slot::list_98, id_slot::list_98_count)) set_kind(6);
        break;
    case 15: {
        const auto index = first_empty(c, record, id_slot::list_160, id_slot::list_160_count);
        if (index >= 5) return no();
        const auto at = record + (std::uint32_t(index) << 2);
        c.set(0, std::uint32_t(index) << 2);
        c.set(3, 0);
        c.set(4, at);
        c.put32(at + id_slot::list_160, id_record);
        if (!put_in_list(id_slot::list_98, id_slot::list_98_count)) set_kind(8);
        break;
    }
    default: return no();  // kind 14
    }
    if (compare(c, guest::extended(std::int8_t(flag_a))) != 0) {
        compare_logical(c, record);  // never null here
        const bool below = below_range_limit(c, record);
        if (!below) {
            c.put32(id_record + 0x20, record);
            c.put8(id_record + 0x60, kind);
        } else {
            c.put32(id_record + 0x24, record);
            c.put8(id_record + 0x62, kind);
        }
    }
    c.set(3, 1);
    return true;
}

namespace {
// Where a pass stores a group's totals in a 0x24-byte slot record.
struct SlotOffsets {
    std::uint32_t sum, index, count;
};
constexpr std::uint32_t entry_stride = 0x2c, entry_club = 0x24, entry_weight = 0x28, club_slot = 0x68;

// One of the three passes of 0x100c3cb0 over a table of 0x2c-byte entries
// (`table_cell` holds the table's address, `entry_count_word` its entry count; `slot_count_word` is the slot count). An
// entry names a club (+0x24: an index below the slot count, else skipped);
// consecutive entries of one club are grouped: their weight bytes (+0x28,
// signed) are summed and counted, and when the club changes the previous
// group's sum, the index of its first entry and its count go to the slot of
// that club (the club's +0x68). The last group is never stored (quirk of
// the original). The registers are the original's: r3 the group's club,
// r0 the sum, r4 the first entry's index, r5 the count, r6 the entry.
void totals_pass(Cpu& c, std::uint32_t summaries_cell, std::uint32_t slot_count_word, std::uint32_t clubs_cell,
                 std::uint32_t table_cell, std::uint32_t entry_count_word, SlotOffsets at) {
    c.set({{3, 0}, {0, 0}, {4, 0xffffffff}, {5, 0}, {6, 0}});
    for (std::int32_t i = 0;; c.set(6, std::uint32_t(++i))) {
        const auto count = c.u32(entry_count_word);
        c.set(7, count);
        if (!(compare(c, i, std::int32_t(count)) < 0)) break;
        const auto entry_offset = std::uint32_t(i) * entry_stride;
        auto base = c.u32(table_cell);
        const auto club_index = c.u32(base + entry_offset + entry_club);
        c.set({{7, club_index}, {8, base}});
        if (compare(c, std::int32_t(club_index), -1) <= 0) continue;
        base = c.u32(table_cell);
        const auto slots = c.u32(slot_count_word);
        const auto index_again = c.u32(base + entry_offset + entry_club);
        c.set({{9, base}, {7, slots}, {8, index_again}});
        if (compare(c, std::int32_t(index_again), std::int32_t(slots)) >= 0) continue;
        base = c.u32(table_cell);
        const auto clubs = c.u32(clubs_cell);
        const auto club = clubs + c.u32(base + entry_offset + entry_club) * 0x128;
        c.set({{8, base}, {9, clubs}, {7, club}});
        const auto previous = c.r(3);
        if (compare_logical(c, previous, club) != 0) {
            if (compare_logical(c, previous) != 0) {
                const auto slot_number = c.u32(previous + club_slot);
                const auto slot_offset = slot_number * 0x24;
                c.set(8, slot_number);
                auto summaries = c.u32(summaries_cell);
                c.set({{7, summaries}, {3, slot_offset}});
                c.set(3, summaries + slot_offset);
                c.put32(summaries + slot_offset + at.sum, c.r(0));
                summaries = c.u32(summaries_cell);
                c.set({{7, summaries}, {0, slot_offset}});
                c.set(3, summaries + slot_offset);
                c.put32(summaries + slot_offset + at.index, c.r(4));
                summaries = c.u32(summaries_cell);
                c.set(3, summaries + slot_offset);
                c.put32(summaries + slot_offset + at.count, c.r(5));
            }
            // A new group starts with this entry.
            c.set(0, entry_offset);
            base = c.u32(table_cell);
            c.set({{7, base}, {3, base}, {4, std::uint32_t(i)}});
            const auto clubs_again = c.u32(clubs_cell);
            c.set({{8, clubs_again}, {5, 1}});
            const auto index = c.u32(base + entry_offset + entry_club);
            c.set({{7, base + entry_offset}, {0, entry_offset}});
            c.set(7, index);
            c.set(7, index * 0x128);
            c.set(3, base + entry_offset);
            c.set(0, c.u8(base + entry_offset + entry_weight));
            c.set(3, clubs_again + index * 0x128);
            c.set(0, guest::extended(std::int8_t(c.r(0))));
        } else {
            base = c.u32(table_cell);
            c.set(5, c.r(5) + 1);
            c.set({{8, base}, {7, base + entry_offset}});
            c.set(7, c.u8(base + entry_offset + entry_weight));
            c.set(7, guest::extended(std::int8_t(c.r(7))));
            c.set(0, c.r(0) + c.r(7));
        }
    }
}
}

// 0x100c3cb0: allocates one 0x24-byte summary per slot (the range word's
// count; a failed allocation asserts and calls 0x1076f7d0, which ends the game
// and never returns, so the code after it is the original's fall-through only),
// sets each to -1 at +0, +4 and +8 and zero elsewhere,
// then fills the sums, first-entry indexes and counts from the three tables
// of 0x2c-byte entries (inferred: the numbers of members per club).
void build_slot_summaries(Cpu& c) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c3cb0, 0x90, {.first_gpr = 21});
    const auto count_pointer = c.toc(slot::range_low), summaries_cell = c.toc(-0x7580);
    const auto count_a = c.toc(-0x7554), count_b = c.toc(-0x7550), count_c = c.toc(-0x754c);
    const auto cell_a = c.toc(-0x7534), cell_b = c.toc(-0x7530), cell_c = c.toc(-0x752c);
    const auto clubs_pointer = c.toc(slot::club_table_ready), message = c.toc(slot::message),
               pool = c.toc(slot::text_pool);
    guest::keep(c, {{30, count_pointer}, {23, summaries_cell}, {24, count_a}, {25, count_b}, {26, count_c},
                    {27, cell_a}, {28, cell_b}, {29, cell_c}, {31, clubs_pointer}, {21, message}, {22, pool}});
    const auto slots = c.u32(count_pointer);
    c.set(0, slots);
    c.set(3, slots * 0x24);
    c.call(0x10775130, 0x100c3cf8);
    c.put32(summaries_cell, c.r(3));
    const auto block = c.u32(summaries_cell);
    c.set(0, block);
    if (compare_logical(c, block) == 0) {
        assertion(c, frame, 0x100c3d1c, 0x2bf9, 0x40, 0x38);
        c.set({{4, c.toc(slot::success)}, {0, 0}, {3, 0xffffffff}});
        c.call(0x1076f7d0, 0x100c3d64);
    }
    c.set(6, 0);
    std::int32_t i = 0;
    // The summaries cell is read again before every store, in the original's
    // order, so a block that overlaps the cell is seen as the original sees it.
    std::uint32_t read_for_r3 = 0, read_for_r5 = 0, read_for_r8 = 0, read_for_r10 = 0;
    for (;; ++i) {
        const auto limit = c.u32(count_pointer);
        c.set(0, limit);
        if (!(compare(c, i, std::int32_t(limit)) < 0)) break;
        const auto offset = std::uint32_t(i) * 0x24;
        const auto cell = [&] { return c.u32(summaries_cell) + offset; };
        c.put32(cell() + 0xc, 0);
        c.put32(cell() + 0x10, 0);
        c.put32(cell() + 0x14, 0);
        c.put32(cell(), 0xffffffff);
        c.put32(cell() + 0x4, 0xffffffff);
        read_for_r10 = c.u32(summaries_cell);
        c.put32(read_for_r10 + offset + 0x8, 0xffffffff);
        read_for_r8 = c.u32(summaries_cell);
        c.put32(read_for_r8 + offset + 0x18, 0);
        read_for_r5 = c.u32(summaries_cell);
        c.put32(read_for_r5 + offset + 0x1c, 0);
        read_for_r3 = c.u32(summaries_cell);
        c.put32(read_for_r3 + offset + 0x20, 0);
        c.set(6, std::uint32_t(i + 1));
    }
    if (i > 0) {
        // What the last iteration leaves in the scratch registers.
        c.set({{3, read_for_r3 + std::uint32_t(i - 1) * 0x24}, {4, 0}, {5, read_for_r5}, {7, 0}, {8, read_for_r8},
               {9, 0}, {10, read_for_r10}});
        c.set(11, 0xffffffff);
        c.set(12, 0xffffffff);
    }
    totals_pass(c, summaries_cell, count_pointer, clubs_pointer, cell_b, count_b, {0x10, 0x4, 0x1c});
    totals_pass(c, summaries_cell, count_pointer, clubs_pointer, cell_c, count_c, {0xc, 0x0, 0x18});
    totals_pass(c, summaries_cell, count_pointer, clubs_pointer, cell_a, count_a, {0x14, 0x8, 0x20});
}
}
