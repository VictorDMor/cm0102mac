#include "shortlist_manager_interest_detail.hpp"

// evaluate_club_interest (0x10369080), its ties: the person's relations with
// the club and his own club (0x10369760-0x10369b80), with their managers
// (0x10369b80-0x10369c5c), and his standing at his club: morale, his role
// there, his happiness (0x10369c5c-0x10369f1c).
namespace cm::aot::shortlist_manager::interest {
namespace {
// The original's inline compare of a date copy with another date (years as
// unsigned halfwords, then the days), leaving 0 or 1 tested by extsb.
bool date_after(Cpu& c, const Date& copy, const Date& other) {
    const auto year = std::uint16_t(copy.year()), other_year = std::uint16_t(other.year());
    const auto order = compare_logical(c, year, other_year);
    const bool after = order > 0 || (order == 0 && day_after(copy.day(), other.day()));
    return nonzero(c, after ? 1 : 0);
}
}

bool Evaluation::manager_date_near(std::uint32_t manager, std::int32_t out, std::int32_t copy, std::uint32_t back) {
    program.call(routine::date_plus, back, {frame.at(out), today(), 150});
    c.put32(frame.at(copy), c.u32(frame.at(out)));
    c.put32(frame.at(copy + 4), c.u32(frame.at(out + 4)));
    return date_after(c, Date{c, frame.at(copy)}, Person{c, manager}.joined());
}

void Evaluation::relations() {
    const Person p = person();
    if (test(routine::club_tie_a050, 0x1036976c, club_at)) scale(0.33, 2);
    if (test(routine::club_tie_a050, 0x1036979c, current_at)) {
        // Byte sum of three personality bytes, as a signed byte.
        const std::int32_t sum = std::int8_t(c.u8(person_at + 0x65) + c.u8(person_at + 0x67) + c.u8(person_at + 0x66));
        if (compare(c, sum, 0x2d) > 0) scale(1.25);
        else if (compare(c, sum, 0x19) > 0) scale(1.5);
        else scale(2.0);
        if (wanted()) reason(4);
    }
    // Quirk: the contract's +0x54 is read even without a contract.
    if (test(routine::club_tie_a0b0, 0x1036980c, current_at) &&
        !test(routine::staff_test_3150, 0x10369820, current_at) && !nonzero(c, contract().listed()))
        scale(0.5, 8);
    if (test(routine::club_tie_a0b0, 0x1036985c, club_at)) {
        if (wanted()) reason(0x10);
        scale(3.0);
    }
    if (compare(c, age(), 0x22) > 0 && compare_logical(c, current_at, club_at) != 0 &&
        compare_logical(c, club().nation(), p.nation()) != 0)
        scale(0.75);
    if (!has_club() || compare_logical(c, current_at, club_at) == 0) return;

    // Both clubs among the two rivals (TOC ids): 0.025 / pressure.
    const std::int32_t rival_a = c.i32(c.toc(toc::rival_club_a));
    const auto rival = [&](std::int32_t id) {
        return compare(c, id, rival_a) == 0 || compare(c, id, c.i32(r15)) == 0;
    };
    const auto at_least_one = [&](std::int32_t value) { return compare(c, value, 1) < 0 ? 1 : value; };
    if (rival(club().id()) && rival(current().id())) {
        const double factor = fpu.fdiv(0.025, real(0x10c, at_least_one(p.loyalty())));
        scale(factor, 0x20);
        return;
    }
    // Regions: the club in region a and the others in region b, or the reverse.
    const std::int32_t region = club().nation_record().id();
    const std::int32_t region_a = c.i32(c.toc(toc::region_a));
    const auto region_of = [&](std::uint32_t nation) { return Nation{c, nation}.id().get(); };
    if (compare(c, region, region_a) == 0) {
        const std::int32_t region_b = c.i32(r25);
        if (compare(c, region_of(current().nation()), region_b) == 0 && compare(c, region_of(p.nation()), region_b) == 0) {
            scale(0.05, 2);
            return;
        }
    }
    if (compare(c, region, c.i32(r25)) == 0 && compare(c, region_of(current().nation()), region_a) == 0 &&
        compare(c, region_of(p.nation()), region_a) == 0) {
        scale(0.1, 2);
        return;
    }
    // The club is one of his club's affiliates (inferred), then rivals.
    const auto region_c = [&] { return c.i32(c.toc(toc::region_c)); };
    const auto pressure_factor = [&](double over_region_c, double otherwise) {
        if (compare(c, club().nation_record().id(), region_c()) == 0) {
            const double pressure = real(0x10c, p.loyalty());
            return fpu.fsub(1.0, fpu.fdiv(pressure, over_region_c));
        }
        return fpu.fdiv(otherwise, real(0x10c, at_least_one(p.loyalty())));
    };
    const auto apply = [&](double factor) {
        scale(factor);
        if (wanted() && order(factor, 0.7).lt) reason(0x20);
    };
    if (compare_logical(c, club_at, current().affiliate(0)) == 0 || compare_logical(c, club_at, current().affiliate(1)) == 0 ||
        compare_logical(c, club_at, current().affiliate(2)) == 0) {
        // The first compare is the club's region already loaded (r0).
        const double factor = compare(c, region, region_c()) == 0
                                  ? fpu.fsub(1.0, fpu.fdiv(real(0x10c, p.loyalty()), 50.0))
                                  : fpu.fdiv(0.2, real(0x10c, at_least_one(p.loyalty())));
        apply(factor);
    }
    if (nonzero(c, std::int8_t(program.call(routine::club_rivalry, 0x10369ac8, {club_at, current_at}))))
        apply(pressure_factor(50.0, 0.75));
}

void Evaluation::managers() {
    const auto tie = [&](std::uint32_t entry, std::uint32_t back, std::uint32_t manager) {
        return nonzero(c, std::int8_t(program.call(entry, back, {manager, person_at})));
    };
    if (tie(routine::person_tie_9520, 0x10369b8c, club().manager())) scale(2.0, 0x40);
    if (tie(routine::person_tie_9580, 0x10369bbc, club().manager())) scale(0.33, 0x80);
    if (has_club() && tie(routine::person_tie_9580, 0x10369bf4, current().manager())) scale(2.0, 0x100);
    if (has_club() && tie(routine::person_tie_9520, 0x10369c2c, current().manager())) {
        const bool w = wanted();
        scale(0.5);
        // Quirk: the contract's options are read even without a contract.
        if (w && !nonzero(c, std::int32_t(contract().options() & 8))) reason(0x200);
    }
}

void Evaluation::current_status() {
    if (!has_contract()) {
        if (own_club()) {
            role = role_word;
            r25 = 0;
        } else {
            role = 7;
            r25 = std::uint32_t(-100);
        }
        return;
    }
    const Contract held = contract();
    if (own_club()) {
        // His morale: 1 + morale / 400; his role as the contract has it when
        // the manager's date falls within 150 days, else the club's.
        const auto manager = club().manager().get();
        const double morale = real(0x10c, held.morale());
        const bool has_manager = nonzero_logical(c, manager);
        scale(fpu.fadd(1.0, fpu.fdiv(morale, 400.0)));
        if (has_manager && manager_date_near(club().manager(), 0x68, 0x50, 0x10369ce0))
            role = program.call(routine::contract_role, 0x10369d54, {contracts(), person_at, contract_at});
        else
            role = role_word;
    } else {
        // Leaving: 1 - morale / 400, twice when over 1.01.
        const double morale = real(0x10c, held.morale());
        const double factor = fpu.fsub(1.0, fpu.fdiv(morale, 400.0));
        scale(factor);
        if (order(factor, 1.01).gt) scale(factor);
        if (flag(flag::bit_10)) weight_wage = fpu.frsp(fpu.fmul(weight_wage, 1.5));
        if (flag(flag::bit_100)) weight_wage = fpu.frsp(fpu.fmul(weight_wage, 1.5));
        if (flag(flag::bit_1)) weight_league = fpu.frsp(fpu.fmul(weight_league, 1.5));
        if (wanted()) {
            if (order(factor, 1.5).gt && !nonzero(c, std::int32_t(held.options() & 8))) reason(0x400);
            else if (order(factor, 0.5).lt) reason(0x800);
        }
        if (has_club() && nonzero_logical(c, current().manager()) &&
            manager_date_near(current().manager(), 0x60, 0x48, 0x10369e54))
            role = program.call(routine::contract_role, 0x10369ec8, {contracts(), person_at, contract_at});
        else
            role = held.squad_status() & 0xf;
        if (nonzero(c, std::int32_t(held.options() & 8))) {
            // Options bit 8: role 1 becomes 3, 2-3 become 4, the others 7.
            const std::int32_t was = current_role();
            const auto against_1 = compare(c, was, 1);
            if (against_1 == 0) role = 3;
            else if (against_1 < 0 || compare(c, was, 4) >= 0) role = 7;
            else role = 4;
        }
    }
    r25 = held.happiness().get() & 0xff;  // lbz: unsigned
}
}
