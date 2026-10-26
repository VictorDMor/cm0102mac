// SHORTLIST_MANAGER.CPP 0x10366470, the candidate context evaluation: the
// factors of his job at the reference club and of the requested category
// (the original's jump tables, slot::cases_10366470_a-d, as switches), the
// club's results, the foreigner factor and the time he has served.
#include "shortlist_manager_context_detail.hpp"

namespace cm::aot::shortlist_manager::context {
std::optional<double> Evaluation::role_factors() {
    // 0x103670fc: by his job at the reference club (table d, 17 cases;
    // others as 0 and 11).
    const std::uint32_t job = word(job8());
    switch (compare_logical(c, job, 0x10) > 0 ? 0 : job) {
    case 1:
        score = fpu.fmul(score, 0.01);
        break;
    case role::manager:
    case role::assistant:
        if (nonzero_logical(c, reference)) {
            score = fpu.fmul(score, 0.45);
            note(reason::national_job);
        } else {
            score = fpu.fmul(score, 0.65);
        }
        break;
    case 16: {
        // 10 / max(1, ambition).
        const std::int32_t ambition = byte_of(person.ambition());
        const std::int32_t at_least_one = compare(c, ambition, 1) > 0 ? ambition : 1;
        const double value = integer(0xbc, at_least_one);
        score = fpu.fmul(score, fpu.fdiv(10.0, value));
        break;
    }
    case 2:
    case 3:
    case 4: {
        const bool big_manager_job =
            compare(c, category8(), role::manager) == 0 && compare(c, club.reputation(), 0x1770) >= 0;
        score = fpu.fmul(score, big_manager_job ? 0.5 : 0.01);
        break;
    }
    case 6:
    case 13:
        score = fpu.fmul(score, 1.5);
        if (has_reasons() && manager_category()) add_reason(reason::job);
        break;
    case 8:
    case 15: {
        // 0.8 + ambition / 20.
        const bool reasons_wanted = has_reasons();
        const double value = integer(0xbc, byte_of(person.ambition()));
        const double part = fpu.fdiv(value, 20.0);
        score = fpu.fmul(score, fpu.fadd(0.8, part));
        if (reasons_wanted && manager_category()) add_reason(reason::job);
        break;
    }
    case 7:
    case 14:
        score = fpu.fmul(score, 2.2);
        if (has_reasons() && manager_category()) add_reason(reason::job);
        break;
    case 9:
    case 10:
        // Categories 12-15, 5-7 and 8, by the category's low byte.
        if (has_reasons() && (compare_logical(c, (category - 12) & 0xff, 3) <= 0 ||
                              compare_logical(c, (category - 5) & 0xff, 2) <= 0 || compare(c, category8(), 8) == 0))
            add_reason(reason::job);
        break;
    default:  // 0, 11 and above 16
        score = fpu.fmul(score, 10.0);
        note(reason::job);
        break;
    }
    // 0x103672fc: by the category (table c, 16 cases; others as 11).
    category_index = word(category8());
    switch (compare_logical(c, category_index, 0xf) > 0 ? 11 : std::uint32_t(category_index)) {
    case 0:
        return 0.0;
    case 1:
        score = fpu.fdiv(score, 0.1);
        break;
    case 2:
    case 3:
    case 4:
        score = fpu.fdiv(score, 0.85);
        break;
    case role::manager:
    case role::assistant: {
        const bool manager = compare(c, job8(), role::manager) == 0;
        score = fpu.fdiv(score, 0.45);
        if (manager || compare(c, job8(), role::assistant) == 0) score = fpu.fmul(score, 1.5);
        break;
    }
    case 6:
    case 13: {
        const bool manager = compare(c, job8(), role::manager) == 0;
        score = fpu.fmul(score, 0.5);
        if (manager || compare(c, job8(), role::assistant) == 0) score = fpu.fdiv(score, 2.5);
        break;
    }
    case 7:
    case 8:
    case 14:
    case 15: {
        const bool manager = compare(c, job8(), role::manager) == 0;
        score = fpu.fdiv(score, 2.5);
        if (manager || compare(c, job8(), role::assistant) == 0) score = fpu.fdiv(score, 2.5);
        break;
    }
    case 9:
    case 10:
        score = either(job8(), 9, 10) ? fpu.fdiv(score, 2.5) : fpu.fmul(score, 0.25);
        if (nonzero_logical(c, person.player())) score = fpu.fmul(score, 0.25);
        if (manager_job()) score = fpu.fdiv(score, 2.5);
        break;
    default: {  // 11 and above 15
        const bool manager = compare(c, job8(), role::manager) == 0;
        score = fpu.fdiv(score, 10.0);
        if (manager || compare(c, job8(), role::assistant) == 0) score = fpu.fdiv(score, 2.5);
        break;
    }
    }
    return std::nullopt;
}

void Evaluation::results_factor() {
    // 0x10367434: a manager job at a club whose stats records (+6, +8)
    // plus 500 x (trait 0x64 + loyalty) stay under 8000: x1.4.
    if (!nonzero_logical(c, reference) || !manager_job()) return;
    const std::uint32_t stats = c.u32(c.toc(slot::club_stats));
    c.set(7, stats);
    if (!nonzero_logical(c, stats)) return;
    const auto figure = [&](std::uint32_t offset) {
        if (!nonzero_logical(c, reference)) return 0;
        return std::int32_t(c.i16(c.u32(stats) + word(Club{c, reference}.id()) * 0x4c + offset));
    };
    const std::int32_t first = figure(6);
    const std::uint32_t trait = word(byte_of(person.trait_64())) * 500, loyalty = word(byte_of(person.loyalty())) * 500;
    const std::int32_t second = figure(8);
    leave(c, {{4, word(first)}, {5, loyalty}, {6, trait}});
    if (compare(c, std::int32_t(word(second) + loyalty + trait + word(first)), 0x1f40) < 0)
        score = fpu.fmul(score, 1.4);
}

void Evaluation::foreign_factor() {
    // 0x10367dc8: a foreigner where the club is (inferred), coming from
    // another nation, by category: table b when he is no foreigner in its
    // staff's nation, table a otherwise.
    if (nonzero_logical(c, reference)) {
        const std::uint32_t nation = club.nation();
        c.set(4, nation);
        if (nonzero_logical(c, nation) &&
            !nonzero(c, byte_of(program.call(call::is_foreigner_in, 0x10367de8, {person.address, nation, 1}))))
            return;
    }
    if (nonzero_logical(c, reference) && compare_logical(c, Club{c, reference}.nation(), club.nation()) == 0) return;
    bool at_home_with_staff = false;
    if (!manager_category()) {
        const std::uint32_t staff = club.manager();
        if (nonzero_logical(c, staff)) {
            const std::uint32_t staff_nation = c.u32(staff + 0xc);
            c.set(4, staff_nation);
            if (nonzero_logical(c, staff_nation))
                at_home_with_staff = !nonzero(
                    c, byte_of(program.call(call::is_foreigner_in, 0x10367e40, {person.address, staff_nation, 1})));
        }
    }
    const std::uint32_t index = compare_logical(c, category_index, 0xf) > 0 ? 0 : std::uint32_t(category_index);
    double factor;
    if (at_home_with_staff) {  // table b
        switch (index) {
        case 2: case 3: case 4: factor = 0.85; break;
        case 6: case 13: factor = 0.75; break;
        case 7: case 8: case 14: case 15: factor = 0.65; break;
        case 9: case 10: factor = 0.5; break;
        default: factor = 0.01; break;  // 0, 1, 5, 11, 12 and above 15
        }
    } else {  // table a
        switch (index) {
        case 2: case 3: case 4: factor = 0.25; break;
        case role::manager: case role::assistant: factor = 0.3; break;
        case 6: case 13: factor = 0.225; break;
        case 7: case 8: case 9: case 10: case 14: case 15: factor = 0.2; break;
        default: factor = 0.001; break;  // 0, 1, 11 and above 15
        }
    }
    score = fpu.fmul(score, factor);
}

std::optional<double> Evaluation::service_factor() {
    // 0x10367f00: out of work x2.5. Otherwise he must have served 120 +
    // 15 x loyalty days at his club; f29 = min(400, days^2 / 20 / max(1,
    // loyalty + 6)) / 400, and 0 days of that rule him out.
    if (!nonzero_logical(c, reference)) {
        score = fpu.fmul(score, 2.5);
        return std::nullopt;
    }
    const std::uint32_t now = today;
    const std::int32_t loyalty = byte_of(person.loyalty());
    const std::uint32_t years = word(c.u16(now + 2)) - word(person.joined().year());
    const std::uint32_t day_part = word(c.i16(now)) - word(person.joined().day());
    const std::int32_t days = std::int32_t(years * 365 + day_part);
    leave(c, {{4, day_part}, {5, years}, {6, word(loyalty)}, {7, word(days)}});
    if (compare(c, days, std::int32_t(word(loyalty) * 15 + 120)) < 0) return 0.0;
    const std::int32_t squared = std::int32_t(word(days) * word(days));
    const std::int32_t high = multiply_high(0x66666667, squared) >> 3;
    const std::int32_t twentieth = std::int32_t(word(high) + word(sign_bit(high)));
    const std::int32_t plus_six = std::int32_t(word(loyalty) + 6);
    const std::int32_t divisor = compare(c, plus_six, 1) < 0 ? 1 : plus_six;
    const std::int32_t quotient = divide(0x10367f84, twentieth, divisor);
    leave(c, {{4, word(sign_bit(high))}, {5, word(plus_six)}, {7, word(quotient)}});
    std::int32_t served = 0;
    if (compare(c, quotient) >= 0) served = compare(c, quotient, 400) > 0 ? 400 : quotient;  // divw. then cmpwi
    if (compare(c, served) == 0) return 0.0;
    const double value = integer(0xac, served);
    service = fpu.fdiv(value, 400.0);
    return std::nullopt;
}
}
