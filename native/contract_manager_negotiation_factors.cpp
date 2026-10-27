#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
bool byte_answer(Cpu& c, std::uint32_t value) {
    const auto answer = std::int8_t(value);
    c.set(0, std::uint32_t(std::int32_t(answer)));
    return nonzero(c, answer);
}

void tenure_weight(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    const auto enabled = std::int8_t(std::uint32_t(n.tenure_or_shortlist));
    c.set(0, std::uint32_t(std::int32_t(enabled)));
    if (!nonzero(c, enabled)) return;
    if (byte_answer(c, n.call(0x1014c2e0, 0x10085e78, {n.employer, 0, 1, 0, 0}))) return;
    if (byte_answer(c, n.call(0x100d3150, 0x10085e8c, {n.person, 0}))) return;
    const auto squad_role = c.u8(n.contract + 0x60) & 15u;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 7) == 0) return;
    c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 8) == 0) return;
    unsigned coefficient = 0x1c;
    const auto middle = compare(c, std::int32_t(squad_role), 3);
    if (middle == 0) coefficient = 0x28;
    else if (middle < 0) {
        if (compare(c, std::int32_t(squad_role), 1) > 0) coefficient = 0x20;
    } else {
        const auto upper = compare(c, std::int32_t(squad_role), 5);
        if (upper <= 0) coefficient = 0x24;
    }
    const auto initial = double(c.f32(n.singles_or_finance + coefficient));
    c.set_f(3, initial);
    const auto joined_year = c.u16(n.person + 0x4e);
    c.set(4, joined_year);
    const auto current_year = c.u16(n.today + 2);
    c.set(0, current_year);
    const auto joined_day = c.i16(n.person + 0x4c);
    c.set(3, extended(joined_day));
    const auto year_days = (std::uint32_t(current_year) - joined_year) * 365u;
    c.set(4, std::uint32_t(current_year) - joined_year);
    const auto current_day = c.i16(n.today);
    c.set(0, extended(current_day));
    c.set(4, year_days);
    const auto day_delta = extended(current_day) - extended(joined_day);
    c.set(0, day_delta);
    const auto elapsed = std::int32_t(year_days + day_delta);
    c.set(4, std::uint32_t(elapsed));
    std::int32_t clamped;
    if (compare(c, elapsed) < 0) clamped = 0;
    else {
        const auto beyond_year = compare(c, elapsed, 365) > 0;
        clamped = beyond_year ? 365 : elapsed;
    }
    c.set(3, std::uint32_t(clamped));
    const auto over_floor = compare(c, clamped, 121) > 0;
    const auto tenure_days = over_floor ? clamped : 121;
    c.set(0, std::uint32_t(tenure_days));
    const auto one = c.f64(n.constants + 0x18);
    c.set_f(0, one);
    c.set(3, std::uint32_t(tenure_days) ^ 0x80000000u);
    c.set(0, 0x43300000);
    const auto year = c.f64(n.constants + 0xf8);
    c.set_f(1, year);
    const auto premium = fp.fsub(initial, one);
    c.set_f(0, premium);
    c.put32(n.frame.at(0xa0), c.r(3));
    const auto bias = c.f64(n.constants + 8);
    c.set_f(2, bias);
    const auto report = nonzero_logical(c, n.diagnostics);
    c.put32(n.frame.at(0x9c), 0x43300000);
    const auto days_per_premium = fp.fdiv(year, premium);
    c.set_f(0, days_per_premium);
    const auto encoded = c.f64(n.frame.at(0x9c));
    c.set_f(1, encoded);
    const auto days = fp.fsub(encoded, bias);
    c.set_f(1, days);
    const auto reduction = fp.fdiv(days, days_per_premium);
    c.set_f(0, reduction);
    n.tenure_factor = fp.fsub(initial, reduction);
    c.set_f(30, n.tenure_factor);
    if (!report) return;
    const auto threshold = c.f64(n.constants + 0x98);
    c.set_f(0, threshold);
    fp.fcmp(0, n.tenure_factor, threshold);
    if (n.tenure_factor > threshold) {
        const auto flags = extended(c.i16(n.diagnostics)) | 0x100u;
        c.set(0, flags);
        c.put16(n.diagnostics, flags);
    }
}

void mood_weight(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    const auto role_score = std::int16_t(std::uint32_t(n.national_context_or_role_score));
    const auto mood = std::int16_t(std::uint32_t(n.mood));
    c.set(4, extended(role_score));
    c.set(5, extended(mood));
    const auto adjusted = extended(role_score) + (extended(role_score) >> 31);
    c.set(3, adjusted);
    c.set(0, 0x43300000);
    c.put32(n.frame.at(0x9c), 0x43300000);
    const auto half_role_score = std::int32_t(adjusted) >> 1;
    c.set(0, std::uint32_t(half_role_score));
    c.s.wr<1>(guest::reg::xer_ca, std::int32_t(adjusted) < 0 && (adjusted & 1));
    const auto bias = c.f64(n.constants + 8);
    c.set_f(2, bias);
    n.national_context_or_role_score = extended(mood) + std::uint32_t(half_role_score);
    const auto slope = c.f64(n.constants + 0x2a0);
    c.set_f(3, slope);
    c.set(0, std::uint32_t(n.national_context_or_role_score) ^ 0x80000000u);
    const auto intercept = c.f64(n.constants + 0x298);
    c.set_f(0, intercept);
    c.put32(n.frame.at(0xa0), c.r(0));
    const auto high_mood = compare(c, mood, 50) >= 0;
    const auto encoded = c.f64(n.frame.at(0x9c));
    c.set_f(1, encoded);
    const auto score = fp.fsub(encoded, bias);
    c.set_f(1, score);
    n.quote_factor = fp.multiply_add(0x10085fe8, slope, score, intercept);
    c.set_f(27, n.quote_factor);
    if (high_mood && !byte_answer(c, n.call(0x1014c2e0, 0x10086008, {n.employer, 0, 1, 0, 0}))) {
        const auto square = std::uint32_t(n.national_context_or_role_score) * std::uint32_t(n.national_context_or_role_score);
        c.set(4, square);
        c.set(3, 0x51ec0000);
        c.set(0, 0x43300000);
        const auto curve_bias = c.f64(n.constants + 8);
        c.set_f(2, curve_bias);
        c.set(3, 0x51eb851f);
        c.put32(n.frame.at(0x9c), 0x43300000);
        const auto high = multiply_high(0x51eb851f, std::int32_t(square));
        c.set(0, std::uint32_t(high));
        const auto coefficient = c.f64(n.constants + 0x2a8);
        c.set_f(3, coefficient);
        const auto base = c.f64(n.constants + 0x178);
        c.set_f(0, base);
        const auto floor = high >> 4;
        c.set(0, std::uint32_t(floor));
        c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 15));
        c.set(3, std::uint32_t(floor) >> 31);
        const auto rounded = std::uint32_t(floor) + (std::uint32_t(floor) >> 31);
        c.set(0, rounded ^ 0x80000000u);
        c.put32(n.frame.at(0xa0), c.r(0));
        const auto curve_encoded = c.f64(n.frame.at(0x9c));
        c.set_f(1, curve_encoded);
        const auto squared_score = fp.fsub(curve_encoded, curve_bias);
        c.set_f(1, squared_score);
        const auto curve = fp.multiply_add(0x10086050, coefficient, squared_score, base);
        c.set_f(0, curve);
        n.quote_factor = fp.fmul(n.quote_factor, curve);
        c.set_f(27, n.quote_factor);
    }
    if (!nonzero_logical(c, n.diagnostics)) return;
    c.set(0, extended(mood));
    unsigned flag = 0;
    if (compare(c, mood, 60) > 0) flag = 0x200;
    else if (compare(c, mood, 20) > 0) flag = 0x400;
    if (flag) {
        const auto flags = extended(c.i16(n.diagnostics)) | flag;
        c.set(0, flags);
        c.put16(n.diagnostics, flags);
    }
}

void finance_weight(Context& n) {
    auto& c = n.c;
    auto& fp = n.fp;
    const auto finances = c.toc(-0x7ab8);
    c.set(3, finances);
    c.set(4, n.employer);
    n.singles_or_finance = n.call(0x10103510, 0x100860a0, {c.u32(finances), n.employer});
    if (nonzero(c, std::int32_t(std::uint32_t(n.singles_or_finance)))) {
        const auto high = c.u32(n.singles_or_finance);
        c.set(3, high);
        const auto low = c.u32(n.singles_or_finance + 4);
        c.set(4, low);
        n.call(0x10773c80, 0x100860b4);
        n.finance_balance = c.f(1); // ABI result of the signed 64-bit conversion.
    } else n.finance_balance = c.f64(n.constants + 0x60);
    c.set_f(26, n.finance_balance);
    c.set(3, 0x10620000);
    const auto reputation = c.i16(n.employer + 0x1ec);
    c.set(0, extended(reputation));
    c.set(3, 0x10624dd3);
    const auto table = c.toc(-0x5740);
    c.set(4, table);
    const auto high = multiply_high(0x10624dd3, reputation);
    const auto floor = high >> 5;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 31));
    c.set(3, std::uint32_t(floor) >> 31);
    const auto bracket = std::uint32_t(floor) + (std::uint32_t(floor) >> 31);
    c.set(0, bracket * 8);
    const auto threshold = c.f64(table + bracket * 8);
    c.set_f(1, threshold);
    fp.fcmp(0, n.finance_balance, threshold);
    double pressure;
    if (n.finance_balance < threshold) {
        const auto scale = c.f64(n.constants + 0x2b0);
        c.set_f(0, scale);
        const auto deficit = fp.fsub(n.finance_balance, threshold);
        c.set_f(3, deficit);
        auto balance_weight = c.f64(n.constants + 0x60);
        c.set_f(2, balance_weight);
        const auto balance_scale = fp.fdiv(n.finance_balance, scale);
        c.set_f(0, balance_scale);
        const auto relative_deficit = fp.fdiv(deficit, threshold);
        c.set_f(3, relative_deficit);
        fp.fcmp(0, balance_weight, balance_scale);
        if (!(balance_weight > balance_scale)) {
            balance_weight = c.f64(n.constants + 0xc8);
            c.set_f(2, balance_weight);
            fp.fcmp(0, balance_weight, balance_scale);
            if (!(balance_weight < balance_scale)) { balance_weight = balance_scale; c.set_f(2, balance_weight); }
        }
        const auto one = c.f64(n.constants + 0x18);
        c.set_f(0, one);
        const auto numerator = fp.fadd(balance_weight, relative_deficit);
        c.set_f(1, numerator);
        const auto denominator = fp.fadd(one, balance_weight);
        c.set_f(0, denominator);
        pressure = fp.fdiv(numerator, denominator);
    } else pressure = fp.fdiv(n.finance_balance, threshold);
    c.set_f(5, pressure);
    const auto mood = std::int16_t(std::uint32_t(n.mood));
    c.set(0, extended(mood));
    c.set(3, 0x43300000);
    c.set(0, extended(mood) ^ 0x80000000u);
    const auto id = c.u32(n.person);
    c.set(4, id);
    c.put32(n.frame.at(0xa8), c.r(0));
    c.set(4, id * 100);
    const auto indices = c.toc(-0x7b9c);
    c.set(5, indices);
    c.put32(n.frame.at(0xa4), 0x43300000);
    const auto bias = c.f64(n.constants + 8);
    c.set_f(3, bias);
    const auto mood_encoded = c.f64(n.frame.at(0xa4));
    c.set_f(1, mood_encoded);
    c.set(0, id * 100 + 12);
    const auto divisor = c.f64(n.constants + 0x2b8);
    c.set_f(0, divisor);
    const auto mood_value = fp.fsub(mood_encoded, bias);
    c.set_f(1, mood_value);
    const auto index = c.u32(indices);
    c.set(4, index);
    c.put32(n.frame.at(0x9c), 0x43300000);
    const auto status_byte = c.u8(index + id * 100 + 12);
    c.set(0, status_byte);
    const auto mood_share = fp.fdiv(mood_value, divisor);
    c.set_f(1, mood_share);
    const auto status_scale = c.f64(n.constants + 0x68);
    c.set_f(4, status_scale);
    const auto status = status_byte >> 4;
    c.set(0, status);
    const auto one = c.f64(n.constants + 0x18);
    c.set_f(0, one);
    c.set(0, std::uint32_t(status) * status);
    c.set(0, c.r(0) ^ 0x80000000u);
    c.put32(n.frame.at(0xa0), c.r(0));
    const auto status_encoded = c.f64(n.frame.at(0x9c));
    c.set_f(2, status_encoded);
    const auto status_square = fp.fsub(status_encoded, bias);
    c.set_f(2, status_square);
    const auto status_pressure = fp.fmul(status_scale, status_square);
    c.set_f(2, status_pressure);
    pressure = fp.fadd(pressure, status_pressure);
    c.set_f(5, pressure);
    pressure = fp.fadd(pressure, mood_share);
    c.set_f(5, pressure);
    fp.fcmp(0, pressure, one);
    double candidate;
    if (pressure > one) {
        const auto slope = c.f64(n.constants + 0x248);
        c.set_f(1, slope);
        const auto intercept = c.f64(n.constants + 0x2c0);
        c.set_f(0, intercept);
        candidate = fp.multiply_add(0x100861bc, pressure, slope, intercept);
    } else {
        const auto slope = c.f64(n.constants + 0x2d0);
        c.set_f(1, slope);
        const auto intercept = c.f64(n.constants + 0x2c8);
        c.set_f(0, intercept);
        candidate = fp.multiply_add(0x100861cc, slope, pressure, intercept);
    }
    c.set_f(4, candidate);
    c.set(3, extended(mood));
    c.set(0, 0x43300000);
    c.set(3, extended(mood) ^ 0x80000000u);
    c.put32(n.frame.at(0x94), 0x43300000);
    const auto lower_bias = c.f64(n.constants + 8);
    c.set_f(2, lower_bias);
    c.put32(n.frame.at(0x98), c.r(3));
    const auto lower_divisor = c.f64(n.constants + 0x2d8);
    c.set_f(0, lower_divisor);
    const auto lower_encoded = c.f64(n.frame.at(0x94));
    c.set_f(1, lower_encoded);
    const auto half = c.f64(n.constants + 0xb8);
    c.set_f(3, half);
    const auto lower_mood = fp.fsub(lower_encoded, lower_bias);
    c.set_f(1, lower_mood);
    const auto contribution = fp.fdiv(lower_mood, lower_divisor);
    c.set_f(0, contribution);
    n.finance_factor = fp.fadd(half, contribution);
    c.set_f(23, n.finance_factor);
    fp.fcmp(0, n.finance_factor, candidate);
    if (!(n.finance_factor > candidate)) {
        n.finance_factor = c.f64(n.constants + 0xf0);
        c.set_f(23, n.finance_factor);
        fp.fcmp(0, n.finance_factor, candidate);
        if (!(n.finance_factor < candidate)) { n.finance_factor = candidate; c.set_f(23, candidate); }
    }
}
}

void terms_and_finance(Context& context) {
    tenure_weight(context);
    mood_weight(context);
    finance_weight(context);
}
}
