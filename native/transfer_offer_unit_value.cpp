// TRANSFER_OFFER.CPP: the draw below a limit (0x10415e50) and the value of an offer (0x10416840).
#include "transfer_offer_unit_value.hpp"
#include "transfer_offer_unit_detail.hpp"
#include <stdexcept>

namespace cm::aot::transfer_offer_unit {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using namespace value_detail;

namespace {
// ---- 0x10415e50 ----
// The original unrolls the recursion inline: the draw at 0x10415e84 (the limit itself) and, for a limit past
// 0xffff, the inner draw below a tenth of it (0x10415f74) with its own big-limit branch (0x10415fb8..0x10416320:
// three more levels and a call to itself at 0x1041622c). That branch is dead: the inner limit is
// `int32(limit * 10) / 0xffff`, and an int32 over 0xffff is at most 0x80000000 / 0xffff = 32768 in magnitude,
// so the inner level always takes its small-limit branch (0x10416324).
constexpr std::uint32_t pool_ratio = 0xa0;  // the divisor of the scale (a double)
constexpr std::int32_t period = 0xffff;

// A limit within +-0xffff: the sample's remainder (a divw, mullw, subf), with the limit's sign.
std::int32_t small_draw(std::int32_t sample, std::int32_t limit) {
    const auto rest = std::int32_t(std::uint32_t(sample) - std::uint32_t(divide(sample, limit)) * std::uint32_t(limit));
    return limit > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
bool is_small(std::int32_t limit) { return limit >= -0xffff && limit <= 0xffff; }
}

std::int32_t draw_below(Cpu& c, std::int32_t limit) {
    Frame frame(c, 0x10415e50, 0x80, {.first_gpr = 20});
    const auto pool = c.toc(toc::doubles);
    // What the original keeps in its non-volatile registers (callees save them in their frames).
    c.set({{25, pool},
           {26, c.toc(sample_table::base)}, {27, c.toc(sample_table::first)},
           {28, c.toc(sample_table::last)}, {29, c.toc(sample_table::cursor)},
           {30, std::uint32_t(limit)}});
    if (limit == 0) return 0;
    const auto sample = std::int32_t(draw(c, 0x10415ea4));
    if (is_small(limit)) return small_draw(sample, limit);
    // A large limit: the sample's place in the period, scaled by the limit over the ratio, plus a tenth of the
    // draw below a tenth of the limit (the sum clamped below the limit).
    Fpu fpu(c);
    const auto scale = fpu.fdiv(to_double(c, fpu, pool, limit), c.f64(pool + pool_ratio));
    const auto place = std::int32_t(std::uint32_t(sample) - std::uint32_t(divide(sample, period)) * std::uint32_t(period));
    auto result = truncate(fpu, fpu.fmul(scale, to_double(c, fpu, pool, place)));
    const auto tenth = divide(std::int32_t(std::uint32_t(limit) * 10u), period);
    std::int32_t inner = 0;
    if (tenth != 0) {
        c.set({{22, std::uint32_t(tenth)}, {31, std::uint32_t(result)}});  // the inner level's limit and this level's share
        const auto inner_sample = std::int32_t(draw(c, 0x10415f90));
        if (!is_small(tenth)) throw std::logic_error("0x10415e50: the inner limit is past 0xffff (dead branch at 0x10415fb8)");
        inner = small_draw(inner_sample, tenth);
    }
    result = std::int32_t(std::uint32_t(result) + std::uint32_t(divide(std::int32_t(std::uint32_t(inner) + 5u), 10)));
    const auto top = std::int32_t(std::uint32_t(limit) - 1u);
    return result > top ? top : result;
}

// ---- 0x10416840 ----
namespace {
namespace toc_cell {
inline constexpr std::int32_t finder = -0x7b7c,     // the cell holding the object 0x10082f20 and 0x10084be0 answer from
                              found = -0x7b78,      // the cell the found record's address is kept in
                              employers = -0x7ab8;  // the cell holding the employers' table
}
namespace person_field {
inline constexpr std::uint32_t value = 0x08, contract = 0x0c, profile = 0x14, owner = 0x24, age = 0x5d, settled = 0x5e;
}
namespace found_field { inline constexpr std::uint32_t club = 0x04, kind = 0x38; }  // the record 0x10082f20 finds
namespace d {  // the unit's double constants
inline constexpr std::uint32_t base_weight = 0x00, decay_cap = 0x18, sell_on_ratio = 0x30, sell_on_divisor = 0x98,
                               month_rate = 0xd0, extra_base = 0xd8, extra_divisor = 0xe0, age_divisor = 0xe8,
                               share_divisor = 0xf0, leaving_weight = 0xc8;
}
namespace s {  // the unit's float constants
inline constexpr std::uint32_t base = 0x10, loan_low = 0x14, loan_high = 0x18, short_stay = 0x1c, mid_stay = 0x20,
                               long_stay = 0x24;
}
constexpr std::uint32_t assess = 0x10219e40, find_player = 0x10082f20, value_player = 0x10084be0, find_employer = 0x10103510,
                        employer_relation = 0x100f55b0;

std::uint32_t uadd(std::uint32_t a, std::int32_t b) { return a + std::uint32_t(b); }
}

std::int32_t offer_value(Cpu& c, std::uint32_t offer, std::int8_t fee_only, std::int8_t without_exchange,
                         std::int8_t exchange_valued, std::int8_t by_club_standing, std::int8_t with_sell_on) {
    Frame frame(c, 0x10416840, 0xb0, {.first_gpr = 16, .first_fpr = 30});
    Fpu fpu(c);
    const auto doubles = c.toc(toc::doubles), floats = c.toc(toc::floats);
    const auto people_cell = c.toc(toc::people), finder_cell = c.toc(toc_cell::finder), found_cell = c.toc(toc_cell::found);
    double weight = c.f64(doubles + d::base_weight);
    double multiplier = c.f32(floats + s::base);
    const auto person_at = [&](std::uint32_t index) { return c.u32(people_cell) + index * person_size; };
    const auto person = person_at(c.u32(offer + field::player));
    // What the original keeps in its non-volatile registers (callees save them in their frames): the pools, the
    // cells, the offer, its five flag arguments as passed (whole words), and the values it later changes.
    c.set({{16, 0}, {17, 0}, {18, floats}, {19, doubles}, {20, people_cell}, {21, finder_cell}, {22, offer},
           {23, c.r(4)}, {24, c.r(5)}, {25, c.r(6)}, {26, c.r(7)}, {27, c.r(8)}, {28, person}, {29, found_cell}, {30, 0}});
    c.set_f(30, multiplier);
    c.set_f(31, weight);
    const auto found = invoke(c, find_player, 0x104168b4, {c.u32(finder_cell), person});
    c.put32(found_cell, found);
    // The player's club: none when he has no club record or is of class 5 (a retired or free player).
    std::uint32_t club = 0;
    if (found != 0 && (std::int32_t(c.i8(found + found_field::kind)) >> 2) != 5)
        club = c.u32(c.toc(toc::clubs)) + c.u32(c.u32(found_cell) + found_field::club) * club_size;
    c.set(17, club);
    std::uint32_t fee = c.u32(offer + field::fee);
    // lha of the halfword at +0x5c, then extsb: the byte at +0x5d.
    const std::int32_t age = std::int8_t(c.i16(person + person_field::age - 1));
    c.set({{31, fee}, {29, std::uint32_t(age)}});
    std::int16_t first_assessment = 0, second_assessment = 0;
    if (c.u8(offer + field::kind) == 0xb && c.u32(person + person_field::owner) != 0) {
        auto owner = c.u32(person + person_field::owner);
        first_assessment = std::int16_t(invoke(c, assess, 0x10416940,
                                               {c.u32(owner + 0x90), person, owner, 2,
                                                std::uint32_t(std::int32_t(c.i16(c.u32(person + person_field::profile) + 0xa)))}));
        c.set(30, std::uint32_t(std::int32_t(first_assessment)));
        owner = c.u32(person + person_field::owner);  // read again after the call
        second_assessment = std::int16_t(invoke(c, assess, 0x10416960,
                                                {c.u32(owner + 0x90), person, owner, 1,
                                                 std::uint32_t(std::int32_t(std::int16_t(first_assessment * 0x32)))}));
        c.set(16, std::uint32_t(std::int32_t(second_assessment)));
    }
    // The club's standing with its employer decides how long a stay counts (the months are read again at each test).
    if (club != 0 && by_club_standing != 0 && c.i8(offer + field::months) > 0) {
        const auto employer = invoke(c, find_employer, 0x10416990, {c.u32(c.toc(toc_cell::employers)), club});
        const auto relation = std::int8_t(invoke(c, employer_relation, 0x10416994, {employer}));
        if (relation == 0) {
            const auto months = c.i8(offer + field::months);
            multiplier = c.f32(floats + (months < 7 ? s::short_stay : months < 13 ? s::mid_stay : s::long_stay));
            c.set_f(30, multiplier);
        } else if (relation < 0) {
            if (relation == -2) {
                multiplier = c.f32(floats + s::loan_low);
                weight = c.f64(doubles + d::leaving_weight);
                c.set_f(30, multiplier);
                c.set_f(31, weight);
            } else if (relation >= -2) {  // -1
                multiplier = c.f32(floats + s::loan_high);
                weight = c.f64(doubles + d::decay_cap);
                c.set_f(30, multiplier);
                c.set_f(31, weight);
            }
        } else if (relation < 2 && c.i8(offer + field::months) > 12) {  // 1
            multiplier = c.f32(floats + s::short_stay);
            c.set_f(30, multiplier);
        }
    }
    if (fee_only == 0 && c.u32(person + person_field::profile) != 0) {
        // The instalments: weighted by how many months they run.
        if (const auto instalments = c.i32(offer + field::instalments); instalments > 0) {
            const auto months = to_double(c, fpu, doubles, c.i8(offer + field::instalment_months));
            const auto discount = fpu.multiply_add(0x10416a74, c.f64(doubles + d::month_rate), months, c.f64(doubles + d::base_weight));
            const auto paid = fpu.fmul(to_double(c, fpu, doubles, instalments), discount);
            fee = uadd(fee, truncate(fpu, fpu.fmul(weight, paid)));
        }
        // The appearance extra: for a young player, not yet settled, whose clubs' assessment of him is above the
        // other club's rating of itself.
        if (c.i32(offer + field::extra) > 0 && c.u8(person + person_field::settled) == 0 && age < 0x1c) {
            const std::int32_t wanted = std::int32_t(second_assessment) * 0x41;
            const std::int32_t rating = c.i16(c.u32(person + person_field::contract) + 0xac);
            if (wanted > rating) {
                const auto gap = to_double(c, fpu, doubles, std::int32_t(std::uint32_t(wanted) - std::uint32_t(rating)));
                const auto spread = fpu.fdiv(gap, c.f64(doubles + d::extra_divisor));
                const auto from_prime = to_double(c, fpu, doubles, std::int32_t(std::uint32_t(age) - 0x18u));
                auto chance = fpu.fadd(c.f64(doubles + d::extra_base), spread);
                const auto appearances = to_double(c, fpu, doubles, c.i8(offer + field::extra_count));
                const auto count_factor = fpu.multiply_add(0x10416b44, c.f64(doubles + d::month_rate), appearances,
                                                           c.f64(doubles + d::base_weight));
                const auto age_taper = fpu.fdiv(std::fabs(from_prime), c.f64(doubles + d::age_divisor));
                chance = fpu.fmul(chance, count_factor);
                chance = fpu.fmul(chance, fpu.fsub(c.f64(doubles + d::base_weight), age_taper));
                const auto cap = c.f64(doubles + d::decay_cap);
                fpu.fcmp(0, chance, cap);
                if (chance > cap) chance = cap;
                const auto extra = to_double(c, fpu, doubles, c.i32(offer + field::extra));
                fee = uadd(fee, truncate(fpu, fpu.fmul(weight, fpu.fmul(extra, chance))));
            }
        }
        // The sell-on share: only when the second assessment is enough above the first.
        const auto percent = c.i8(offer + field::sell_on_percent);
        if (percent > 0 && with_sell_on != 0) {
            const auto second = to_double(c, fpu, doubles, second_assessment);
            const auto scaled = fpu.fmul(c.f64(doubles + d::sell_on_ratio), second);
            const auto first = to_double(c, fpu, doubles, first_assessment);
            fpu.fcmp(0, scaled, first);
            if (scaled > first) {
                const auto gap = to_double(c, fpu, doubles, std::int32_t(std::uint32_t(std::int32_t(second_assessment)) -
                                                                         std::uint32_t(std::int32_t(first_assessment))));
                const auto share = fpu.fdiv(gap, c.f64(doubles + d::share_divisor));
                const auto worth = to_double(c, fpu, doubles, std::int32_t(c.u32(person + person_field::value) * std::uint32_t(std::int32_t(percent))));
                const auto due = fpu.fmul(weight, fpu.fmul(share, worth));
                fee = uadd(fee, truncate(fpu, fpu.fdiv(due, c.f64(doubles + d::sell_on_divisor))));
            }
        }
    }
    // The players in exchange (the list's cursor walked from its head).
    if (const auto list = c.u32(offer + field::exchange); list != 0 && without_exchange == 0) {
        c.set(31, fee);
        rewind(c, list);
        for (;;) {
            const auto row = invoke(c, callee::list_next, 0x10416cd4, {c.u32(offer + field::exchange)});
            if (row == 0) break;
            if (exchange_valued == 0) {
                fee += c.u32(c.u32(people_cell) + c.u32(row) * person_size + person_field::value);
                c.set(31, fee);
            } else {
                fee += invoke(c, value_player, 0x10416cac,
                              {c.u32(finder_cell), c.u32(people_cell) + c.u32(row) * person_size, 0, 0, 0xffffffffu});
                c.set(31, fee);
            }
        }
    }
    // The result: the fee converted and scaled in single precision.
    const auto fee_bits = (std::uint64_t(0x43300000) << 32) | (fee ^ 0x80000000u);
    const auto total = fpu.fsubs(std::bit_cast<double>(fee_bits), c.f64(doubles + magic));
    return truncate(fpu, fpu.fmuls(total, multiplier));
}
}
