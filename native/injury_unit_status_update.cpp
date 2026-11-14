// INJURY.CPP: the pending daily update of a person's status row (0x10187a40, injury_unit_status.hpp).
//
// The routine is the hottest of the unit and the match engine reaches it, so it keeps the whole frame image of
// the original (the dates and the int-to-double words it stores) and does nothing more than the original on the
// common path (a row already dated today).
#include "injury_unit_status.hpp"
#include "injury_unit_status_detail.hpp"
#include <stdexcept>

namespace cm::aot::injury_unit {
using guest::Cpu;
using guest::Frame;
using namespace status_detail;

namespace {
// Frame offsets of the original's locals.
namespace slot {
constexpr std::uint32_t record_pointer = 0x88;           // the person's record (+0x14)
constexpr std::uint32_t row_date_a = 0x38, row_date_b = 0x3c, row_date_c = 0x50, row_date_d = 0x54;
constexpr std::uint32_t cached_date = 0x40;               // (+0x44) the cached club-check date
constexpr std::uint32_t season_start = 0x48;              // (+0x4c) the date 0x102af650 answers
constexpr std::uint32_t injury_days = 0x58, row_days = 0x60;  // 0x10717dd0's out-parameters
}
// The unit's float pool (TOC -0x543c).
namespace pool {
constexpr std::uint32_t ease_cap = 0x28, ease_high = 0x30, ease_middle = 0x38, ease_low_a = 0x40, ease_low_b = 0x48,
                        condition_base = 0x50, condition_offset = 0x58, condition_slope = 0x60, condition_cap = 0x68;
}

// The non-volatile registers the original holds at its calls (r13-r31). The callees save them into their
// frames, so each call sets them as they are by then; the common path never calls and never sets them.
class Held {
public:
    Held(Cpu& cpu, std::uint32_t manager_object, std::uint32_t person) : c(cpu), manager(manager_object), person(person) {}
    // The registers are filled in at the first use, so the common path reads none of the table-of-contents words.
    std::uint32_t& operator[](unsigned n) {
        if (!ready) prepare();
        return r[n];
    }
    template <class... Args>
    std::uint32_t call(std::uint32_t entry, std::uint32_t back, Args... arguments) {
        sync();
        return invoke(c, entry, back, {std::uint32_t(arguments)...});
    }
    // Every caller has used `operator[]` first (an argument, an assignment), which prepares the registers, so
    // there is nothing to prepare here.
    void sync() {
        for (unsigned n = 13; n < 32; ++n) c.set(n, r[n]);
    }

private:
    void prepare() {
        ready = true;
        // r15, r17 and r20 keep the caller's values until the original sets them (nothing has set them yet).
        r[15] = c.r(15);
        r[17] = c.r(17);
        r[20] = c.r(20);
        r[13] = 0;
        r[14] = c.toc(cell::checked_date);
        r[16] = 0;
        r[18] = manager;
        r[19] = 0;
        r[21] = c.toc(cell::window_a);
        r[22] = c.toc(cell::person_stats);
        r[23] = c.toc(toc::floats);
        r[24] = c.toc(sample_table::base);
        r[25] = c.toc(sample_table::first);
        r[26] = c.toc(sample_table::last);
        r[27] = c.toc(sample_table::cursor);
        r[28] = c.toc(toc::today);
        r[29] = 0;
        r[30] = c.toc(cell::window_b);
        r[31] = person;
    }
    Cpu& c;
    std::uint32_t manager, person;
    std::uint32_t r[32] = {};
    bool ready = false;
};

// The days since the row was last updated, shared out by the original's rules; `rest` and `half` and `full` and
// `drain` are the four counts the final formula takes.
struct Days {
    std::int32_t rest = 0, half = 0, full = 0, drain = 0;
};

// The final formula: fitness, sharpness from the days, with the original's int-to-double through the float unit.
void apply_days(Cpu& c, Frame& frame, std::uint32_t row, std::uint32_t profile, std::uint32_t today, const Days& days) {
    guest::Fpu fpu(c);
    const auto floats = c.toc(toc::floats);
    // A scaled integer as the original computes it: the biased integer in a stack word pair, the magic subtracted
    // in single precision, the constant multiplied in, converted by fctiwz into the stack word the original reads.
    const auto scaled_single = [&](std::uint32_t in_slot, std::int32_t value, std::uint32_t constant, std::uint32_t out_slot) {
        c.put32(frame.at(std::int32_t(in_slot)), 0x43300000);
        c.put32(frame.at(std::int32_t(in_slot + 4)), std::uint32_t(value) ^ 0x80000000u);
        const double shifted = fpu.fsubs(magic(value), c.f64(floats + 8));
        const auto bits = fpu.fctiwz(0, fpu.fmul(c.f64(floats + constant), shifted));
        c.put64(frame.at(std::int32_t(out_slot)), bits);
        return std::int32_t(std::uint32_t(bits));
    };
    const std::int32_t fitness = c.i16(row + row::fitness);
    const std::int32_t trait = c.i8(profile + 0x34);
    // How far the day count of the row's byte +0x22 stands over the trait, 0..100.
    const std::int32_t gap = std::int8_t(c.u8(row + status::flag) - std::uint32_t(trait));
    const std::int32_t level = gap < 0 ? 0 : gap > 100 ? 100 : gap;
    const std::int16_t full_rate = std::int16_t(level * -12 + 1000);
    const std::int16_t half_rate = std::int16_t(full_rate / 2);
    const std::int16_t floor_value = std::int16_t((trait * 25 / 20 + 15) * 100);
    std::int32_t value = std::int32_t(std::uint32_t(fitness) - std::uint32_t(days.rest) * 0x42u);
    if (value < floor_value) value = 4000;
    const std::int32_t ceiling_base = std::int32_t(std::uint32_t(trait) << 4) / 20 + 0x54;
    std::int16_t ceiling = std::int16_t(ceiling_base * 100);
    if (ceiling < value) ceiling = std::int16_t(value);
    value = std::int32_t(std::uint32_t(value) + std::uint32_t(days.half) * std::uint32_t(std::int32_t(half_rate)));
    if (value > std::int32_t(ceiling)) value = ceiling;
    value = std::int32_t(std::uint32_t(value) + std::uint32_t(days.full) * std::uint32_t(std::int32_t(full_rate)));
    if (fitness > 0x267a) {
        value -= fitness;
        value = fitness + std::int16_t(scaled_single(0x70, value, pool::ease_high, 0x78));
    } else if (fitness > 0x2328) {
        value -= fitness;
        value = fitness + std::int16_t(scaled_single(0x70, value, pool::ease_middle, 0x78));
        if (value > 0x267a)
            value = 0x267a + std::int16_t(scaled_single(0x70, value - 0x267a, pool::ease_cap, 0x78));
    } else if (value > 0x1b58) {
        value = 0x1b58 + std::int16_t(scaled_single(0x70, value - 0x1b58, pool::ease_low_a, 0x78));
        if (value > 0x2328) value = 0x2328 + std::int16_t(scaled_single(0x80, value - 0x2328, pool::ease_low_b, 0x68));
        if (value > 0x267a) value = 0x267a + std::int16_t(scaled_single(0x68, value - 0x267a, pool::ease_cap, 0x70));
    }
    if (value > 10000) value = 10000;
    value = std::int32_t(std::uint32_t(value) - std::uint32_t(days.drain) * 0x37u);
    if (value < floor_value) value = 5000;
    std::int16_t cap = std::int16_t(ceiling_base);
    if (cap < value) cap = std::int16_t(value);
    if (value > std::int32_t(cap)) value = cap;
    // Sharpness: the condition tilts the rate, and a spell of light or full days fills it.
    const std::int32_t sharpness = c.i16(row + row::sharpness);
    const std::int32_t condition = c.i16(row + row::condition);
    double rate = c.f64(floats + pool::condition_base);
    // (f3 starts as the constant; above 1000 it is the slope less the converted condition.)
    if (condition > 1000) {
        c.put32(frame.at(0x68), 0x43300000);
        c.put32(frame.at(0x6c), std::uint32_t(condition) ^ 0x80000000u);
        const double converted = fpu.fsub(magic(condition), c.f64(floats + 8));
        rate = fpu.multiply_add(0x101883cc, c.f64(floats + pool::condition_slope), converted, c.f64(floats + pool::condition_offset));
    }
    const double limit = c.f64(floats + pool::condition_cap);
    fpu.fcmp(0, rate, limit);
    if ((c.cr(0) & 4) && days.half > 0) rate = limit;
    const std::int32_t half_scaled = std::int32_t(std::uint32_t(days.half) * 0xfau);
    const std::int32_t full_scaled = std::int32_t(std::uint32_t(days.full) * 0x1f4u);
    c.put32(frame.at(0x68), 0x43300000);
    c.put32(frame.at(0x78), 0x43300000);
    c.put32(frame.at(0x6c), std::uint32_t(half_scaled) ^ 0x80000000u);
    c.put32(frame.at(0x7c), std::uint32_t(full_scaled) ^ 0x80000000u);
    const double first = fpu.fsub(magic(half_scaled), c.f64(floats + 8));
    const double second = fpu.fsub(magic(full_scaled), c.f64(floats + 8));
    const auto first_product = fpu.fmul(rate, first), second_product = fpu.fmul(rate, second);
    const auto first_bits = fpu.fctiwz(1, first_product);
    const auto second_bits = fpu.fctiwz(0, second_product);
    c.put64(frame.at(0x70), first_bits);
    c.put64(frame.at(0x80), second_bits);
    std::int32_t new_sharpness = std::int32_t(std::uint32_t(sharpness) + std::uint32_t(first_bits) + std::uint32_t(second_bits));
    if (new_sharpness > 10000) new_sharpness = 10000;
    c.put16(row + row::fitness, std::uint32_t(value));
    c.put16(row + row::sharpness, std::uint32_t(new_sharpness));
    c.put16(row + row::date + 2, c.u16(today + date::year_part));
    c.put16(row + row::date, std::uint32_t(c.i16(today + date::day_part)));
    c.put8(row + row::date + 4, c.u8(today + date::byte_part));
}
}

void update_status(Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    Frame frame(c, 0x10187a40, 0xe0, {.first_gpr = 13});
    Held held(c, manager_object, person);
    const auto r2 = c.r(2);
    const auto at = [&](std::int32_t offset) { return r2 + std::uint32_t(offset); };
    // The first-use guards of the function's statics (CodeWarrior): the dates are constructed once, then the flags.
    if (c.i8(at(statics::init_a)) == 0) {
        c.put32(at(statics::cached_club), 0);
        c.put8(at(statics::init_a), 1);
    }
    if (c.i8(at(statics::init_b)) == 0) {
        held.call(0x10715880, 0x10187abc, held[14]);
        c.put8(at(statics::init_b), 1);
    }
    if (c.i8(at(statics::init_c)) == 0) {
        held.call(0x10715880, 0x10187ad8, held[30]);
        c.put8(at(statics::init_c), 1);
    }
    if (c.i8(at(statics::init_d)) == 0) {
        held.call(0x10715880, 0x10187af4, held[21]);
        c.put8(at(statics::init_d), 1);
    }
    if (c.i8(at(statics::init_cache)) == 0) {
        c.put8(at(statics::flag_cached), 0);
        c.put8(at(statics::init_cache), 1);
    }
    if (person == 0) return;
    if (c.i32(person + person::index) >= c.i32(c.toc(toc::people_count))) return;
    const auto profile = c.u32(person + person::profile);
    c.put32(frame.at(slot::record_pointer), profile);
    if (profile == 0) return;

    const auto today = c.toc(toc::today);
    const auto row = status_row(c, manager_object, person);
    // The row's date, copied to the frame; the row is up to date when it is today.
    const auto row_word_a = c.u32(row), row_word_b = c.u32(row + 4);
    c.put32(frame.at(slot::row_date_c), row_word_a);
    c.put32(frame.at(slot::row_date_a), row_word_a);
    c.put32(frame.at(slot::row_date_b), row_word_b);
    c.put32(frame.at(slot::row_date_d), row_word_b);
    if (std::uint16_t(row_word_a) == c.u16(today + date::year_part) &&
        std::int16_t(row_word_a >> 16) == c.i16(today + date::day_part))
        return;

    held[15] = row;
    held[20] = today + 2;
    held.call(0x10717dd0, 0x10187bb8, frame.at(slot::row_days), today, row_word_a, row_word_b);
    const std::int32_t elapsed = days_of(c, frame.at(slot::row_days));
    Days days;
    held[17] = std::uint32_t(elapsed);
    held[13] = 0;

    // An injury index past the records, or below -1, is dropped.
    {
        const auto index = c.i32(row + status::injury);
        if (index > c.i32(manager_object + manager::injury_count) || index < -1) c.put32(row + status::injury, no_injury);
    }
    const auto index = c.u32(row + status::injury);
    if (index != no_injury) {
        const auto record = c.u32(manager_object + manager::injuries) + index * injury::size;
        held[14] = record;
        held.call(0x10717dd0, 0x10187c14, frame.at(slot::injury_days), today, c.u32(record + injury::started),
                  c.u32(record + injury::started + 4));
        const std::int32_t since = days_of(c, frame.at(slot::injury_days));
        const std::int32_t unfit = c.i16(record + injury::days_unfit), out = c.i16(record + injury::days_out);
        if (since < unfit) {
            days.rest = elapsed;
            held[13] = std::uint32_t(days.rest);
        } else if (since < out) {
            if (elapsed < since + unfit) {
                days.half = since - unfit;
                days.rest = elapsed - days.half;
                held[19] = std::uint32_t(days.half);
                held[13] = std::uint32_t(days.rest);
            } else {
                days.half = elapsed;
                held[19] = std::uint32_t(days.half);
            }
        } else {
            const std::int32_t healed = since - unfit;
            if (elapsed > healed) {
                days.full = elapsed - out;
                days.half = healed - days.full;
                days.rest = elapsed - (days.full + days.half);
                held[16] = std::uint32_t(days.full);
                held[19] = std::uint32_t(days.half);
                held[13] = std::uint32_t(days.rest);
            } else {
                // 0x10187c84 tests `since < out` again, false here (0x10187c3c took the branch for since >= out),
                // so the arm at 0x10187c8c..0x10187c94 never runs and the original takes 0x10187c98.
                days.full = elapsed;
                held[16] = std::uint32_t(days.full);
            }
            held.sync();
            c.set({{3, manager_object}, {4, person}, {5, 1}});
            c.link(0x10187cac);
            close_injury(c, manager_object, person, 1);
        }
    } else {
        days.full = elapsed;
        held[16] = std::uint32_t(days.full);
        bool not_yet = false;
        const auto club_entry = c.u32(person + person::club_entry);
        if (club_entry != 0) {
            // The date the person's club entry puts the start at, against today.
            const auto hooks = c.u32(c.toc(cell::club_hooks));
            const auto key = held.call(0x102ae9c0, 0x10187cdc, hooks, club_entry, c.u8(person + person::flags) & 8);
            const auto answer = held.call(0x102af650, 0x10187cf4, c.u32(c.toc(cell::club_hooks)),
                                          c.u32(c.u32(person + person::club_entry) + 0x2c), key);
            const auto answer_a = c.u32(answer), answer_b = c.u32(answer + 4);
            c.put32(frame.at(slot::season_start), answer_a);
            c.put32(frame.at(slot::season_start + 4), answer_b);
            const std::uint32_t day = std::uint16_t(answer_a), today_day = c.u16(today + date::year_part);
            if (day > today_day) not_yet = true;
            else if (day < today_day) not_yet = false;
            else not_yet = halfword_greater(std::int16_t(answer_a >> 16), c.i16(today + date::day_part));
        }
        const auto club = c.u32(person + person::club);
        if (club != 0 && !not_yet) {
            // The club check is cached for the day: a cached club on today's date keeps its answer.
            const auto cache = held[14];
            bool fresh = false;
            if (club == c.u32(at(statics::cached_club))) {
                const auto cached_a = c.u32(cache), cached_b = c.u32(cache + 4);
                c.put32(frame.at(slot::cached_date), cached_a);
                c.put32(frame.at(slot::cached_date + 4), cached_b);
                fresh = std::uint16_t(cached_a) == c.u16(today + date::year_part) &&
                        std::int16_t(cached_a >> 16) == c.i16(today + date::day_part);
            }
            if (!fresh) {
                const auto answer = held.call(0x101929b0, 0x10187db8, club, held[30], held[21]);
                const auto day_part = c.u16(today + date::year_part);
                const auto byte_part = c.u8(today + date::byte_part);
                const auto year_part = c.i16(today + date::day_part);
                c.put8(at(statics::flag_cached), std::int8_t(answer) == 0 ? 1 : 0);
                const auto club_again = c.u32(person + person::club);
                c.put16(cache + 2, day_part);
                c.put16(cache, std::uint32_t(year_part));
                c.put32(at(statics::cached_club), club_again);
                c.put8(cache + 4, byte_part);
            }
            if (c.i8(at(statics::flag_cached)) != 0) {
                days.full = 0;
                held[16] = 0;
                // The days from `window_a` to today and from today to `window_b`.
                const auto window_a = held[21];
                const auto before = held.call(0x10718130, 0x10187e08, today, c.u32(window_a), c.u32(window_a + 4)) & 0xffff;
                held[18] = before;
                const auto window_b = held[30];
                const auto after = held.call(0x10718130, 0x10187e1c, window_b, c.u32(today), c.u32(today + 4)) & 0xffff;
                held[14] = after;
                if (before < after) {
                    // A person still inside the window: the days up to the window's edge feed the drain, a
                    // draw at each of them may wear the person's counter by one.
                    days.drain = elapsed < std::int32_t(before) ? elapsed : std::int32_t(before);
                    days.half = elapsed - days.drain;
                    held[29] = std::uint32_t(days.drain);
                    held[19] = std::uint32_t(days.half);
                    held[30] = 0x55555556;
                    std::int32_t step = 0;
                    held[17] = 0;
                    const auto stats_row = [&] {
                        return c.u32(c.toc(cell::person_stats)) + c.u32(person + person::index) * stats::size + stats::counter;
                    };
                    while (step < days.drain && c.u8(stats_row()) < 0x32) {
                        const std::int32_t range = std::int32_t(c.i8(person + person::temper) / 3 + 2);
                        held[21] = std::uint32_t(range);
                        std::int32_t roll = 0;
                        if (range != 0) {
                            const auto sample = draw_sample(c, 0x10187e90, [&] { held.sync(); });
                            // 0x10187eb8..0x10187f88 draw over a range past 0xffff: the range is a byte's third
                            // plus two, in -40..44.
                            if (!fits_one_draw(range)) throw std::logic_error("injury status: range past 0xffff");
                            roll = remainder_in(sample, range);
                        }
                        held[18] = std::uint32_t(roll);
                        if (roll == 0) {
                            const auto counter = stats_row();
                            c.put8(counter, c.u8(counter) + 1);
                        }
                        ++step;
                        held[17] = std::uint32_t(step);
                    }
                } else {
                    // Past the window's middle the whole stretch is light work, as far as the window reaches.
                    std::int32_t part = std::int32_t(before) - std::int32_t(before + after) / 2;
                    // Here before >= after (the unsigned compare above failed), so (before + after) / 2 <= before
                    // and the original's clamp at 0x10188020 (`li r0,0`) never runs.
                    if (part < 0) throw std::logic_error("injury status: part of the window below 0");
                    days.half = elapsed < part ? elapsed : part;
                    days.rest = elapsed - days.half;
                    held[19] = std::uint32_t(days.half);
                    held[13] = std::uint32_t(days.rest);
                }
                if (after < 0xf) {
                    days.full = 0xf - std::int32_t(after);
                    held[16] = std::uint32_t(days.full);
                    days.half -= days.full;
                    if (days.half < 0) days.half = 0;
                    held[19] = std::uint32_t(days.half);
                } else {
                    const std::int32_t product = std::int32_t(std::uint32_t(c.i16(row + row::fitness)) *
                                                              std::uint32_t(std::int32_t(c.i16(row + row::sharpness))));
                    if (product / 10000 < 8000) {
                        const auto sample = draw_sample(c, 0x101880a0, [&] { held.sync(); });
                        const std::int32_t sign = std::int32_t(sample >> 31);
                        const std::int32_t trim = ((std::int32_t(sample) & 1) ^ sign) - sign;
                        const std::int32_t sum = days.half + trim;
                        days.full = sum / 2;
                        held[16] = std::uint32_t(days.full);
                        days.half -= days.full;
                        held[19] = std::uint32_t(days.half);
                    }
                }
            }
        }
    }
    apply_days(c, frame, row, profile, today, days);
}
}
