#include "shortlist_manager_interest_detail.hpp"

// evaluate_club_interest (0x10369080), a move elsewhere: where it takes him
// (his nation, the club manager's, the distance between the grounds;
// 0x1036ad00-0x1036b3e0) and what his career stage makes of it (age,
// happiness, the time since +0x4c; 0x1036b3e0-0x1036b710).
namespace cm::aot::shortlist_manager::interest {
namespace {
// The role the club has in mind as the original's compare chains take it
// (r15: cmpwi 4, then 6 or 2): 2-6, or 0 for the others.
std::int32_t chained_role(Cpu& c, std::int32_t role) {
    const auto against_4 = compare(c, role, 4);
    if (against_4 == 0) return 4;
    if (against_4 > 0) {
        const auto against_6 = compare(c, role, 6);
        return against_6 == 0 ? 6 : against_6 > 0 ? 0 : 5;
    }
    const auto against_2 = compare(c, role, 2);
    return against_2 == 0 ? 2 : against_2 > 0 ? 3 : 0;
}
}

// The distance between the club's ground and his club's (their places' two
// coordinates, sqrt through the Toolbox glue) x 55: near grounds (under 40)
// x2.9, under 80 x(4.5 - distance / 25), farther x1.3.
void Evaluation::distance_factor(std::uint32_t club_place, std::uint32_t current_place, std::uint32_t site,
                                 std::uint32_t back) {
    const double dx = fpu.fsub(c.f64(club_place), c.f64(current_place));
    const double dy = fpu.fsub(c.f64(club_place + 8), c.f64(current_place + 8));
    c.set_f(1, multiply_add(site, dy, dy, fpu.fmul(dx, dx)));
    program.toolbox(routine::sqrt_glue, back, {current_place, club_place});
    const double distance = fpu.fmul(55.0, c.f(1));
    if (order(distance, 40.0).lt) {
        scale(2.9, 0x20000);
    } else if (order(distance, 80.0).lt) {
        const bool w = wanted();
        const double factor = fpu.fsub(4.5, fpu.fdiv(distance, 25.0));
        scale(factor);
        if (w && order(factor, 1.6).gt) reason(0x20000);
    } else {
        scale(1.3);
    }
}

// base + determination / first + adaptability / second, multiplied in.
double Evaluation::personality_factor(double base, double first, double second) {
    const Person p = person();
    const double determination = fpu.fsub(raw_signed(0xe4, p.ambition()), k::signed_bias);
    const double adaptability_raw = raw_signed(0xec, p.adaptability());
    double factor = fpu.fdiv(determination, first);
    const double adaptability = fpu.fsub(adaptability_raw, k::signed_bias);
    factor = fpu.fadd(base, factor);
    return fpu.fadd(factor, fpu.fdiv(adaptability, second));
}

void Evaluation::location() {
    const Person p = person();
    const auto his_club = p.club().get();
    if (nonzero_logical(c, his_club) && compare_logical(c, Club{c, his_club}.nation(), club().nation()) == 0) {
        // His club is in the club's nation.
        if (!loan()) return;
        if (test(routine::staff_test_3230, 0x1036ad30)) {
            const auto player = p.player().get();
            if (compare(c, p.age(), 0x18) < 0 && nonzero_logical(c, player) &&
                compare(c, Player{c, player}.reputation(), 0x109a) < 0 && compare(c, p.sportsmanship(), 0xf) > 0 &&
                compare(c, p.professionalism(), 0xa) > 0 && has_contract()) {
                const std::int32_t now = current_role();
                if (compare(c, now, 4) != 0 && compare(c, now, 1) != 0 && compare(c, now, 2) != 0 &&
                    compare(c, now, 3) != 0)
                    scale(2.5);
            }
        }
        if (has_club()) {
            const auto nation = club().nation().get();
            if (nonzero_logical(c, nation) && compare(c, Nation{c, nation}.id(), c.i32(c.toc(toc::region_c))) == 0 &&
                compare(c, p.age(), 0x18) < 0) {
                const auto division = current().division().get();
                if (nonzero_logical(c, division) && compare(c, c.i32(division), c.i32(c.toc(toc::division_a))) == 0)
                    scale(3.0);
            }
        }
        if (nonzero_logical(c, club_at)) {
            const auto ground = club().ground().get();
            if (nonzero_logical(c, ground)) {
                const auto club_place = c.u32(ground + 0x10);
                leave(c, {{4, club_place}});
                if (nonzero_logical(c, club_place) && has_club()) {
                    const auto other = current().ground().get();
                    if (nonzero_logical(c, other)) {
                        const auto current_place = c.u32(other + 0x10);
                        if (nonzero_logical(c, current_place)) {
                            distance_factor(club_place, current_place, 0x1036ae70, 0x1036ae78);
                            return;
                        }
                    }
                }
            }
        }
        scale(1.3);
        return;
    }
    const auto club_nation = club().nation().get();
    leave(c, {{4, club_nation}});
    if (nonzero_logical(c, club_nation) &&
        !nonzero(c, std::int8_t(program.call(routine::nation_tie, 0x1036af24, {person_at, club_nation, 1})))) {
        // Not tied to the club's nation: the distance, on loan.
        if (!loan() || !has_club()) return;
        const auto ground = club().ground().get();
        if (nonzero_logical(c, ground)) {
            const auto club_place = c.u32(ground + 0x10);
            leave(c, {{4, club_place}});
            if (nonzero_logical(c, club_place)) {
                const auto other = current().ground().get();
                if (nonzero_logical(c, other)) {
                    const auto current_place = c.u32(other + 0x10);
                    if (nonzero_logical(c, current_place)) {
                        distance_factor(club_place, current_place, 0x1036af88, 0x1036af90);
                        return;
                    }
                }
            }
        }
        scale(1.3);
        return;
    }
    // Tied to the club's nation (or it has none): the club manager's nation.
    const auto manager = club().manager().get();
    bool foreign_manager = false;
    if (nonzero_logical(c, manager)) {
        const auto manager_nation = c.u32(manager + 0xc);
        leave(c, {{4, manager_nation}});
        foreign_manager = nonzero_logical(c, manager_nation) &&
                          !nonzero(c, std::int8_t(program.call(routine::nation_tie, 0x1036b048,
                                                               {person_at, manager_nation, 1})));
    }
    const auto region_d = [&] {
        const auto nation = club().nation().get();
        if (nonzero_logical(c, nation) && compare(c, Nation{c, nation}.id(), c.i32(c.toc(toc::region_d))) == 0)
            scale(0.75);
    };
    region_d();
    const std::int32_t wanted_role = std::int32_t(std::uint32_t(r15));
    if (foreign_manager) {
        // He does not share the manager's nation.
        if (loan()) {
            scale(0.45, 0x40000);
            return;
        }
        switch (chained_role(c, wanted_role)) {
        case 2: scale(personality_factor(0.8, 50.0, 200.0)); break;
        case 3: scale(personality_factor(0.7, 50.0, 200.0)); break;
        case 4: scale(0.5, 0x80000); break;
        case 5: scale(0.4, 0x80000); break;
        case 6: scale(0.2, 0x80000); break;
        default: break;
        }
        return;
    }
    if (loan()) {
        scale(0.08, 0x40000);
        return;
    }
    switch (chained_role(c, wanted_role)) {
    case 2: scale(personality_factor(0.7, 100.0, 100.0)); break;
    case 3: scale(personality_factor(0.6, 100.0, 100.0)); break;
    case 4: scale(0.25, 0x80000); break;
    case 5: {
        const bool w = wanted();
        scale(personality_factor(0.25, 250.0, 250.0));
        if (w) reason(0x80000);
        break;
    }
    case 6: scale(0.025, 0x80000); break;
    default: break;
    }
}

std::optional<double> Evaluation::later_career() {
    const Person p = person();
    if (!loan()) {
        // Not a loan: listed (or 0x100d3040 says so) x2.5; under 2 and wanted
        // by a club of reputation over 6750 or bigger than his: 1 + score, at most 2.
        if ((has_contract() && nonzero(c, contract().listed())) || test(routine::staff_test_3040, 0x1036b408)) {
            score = fpu.fmul(score, 2.5);
            if (order(score, 2.0).lt) {
                // Quirk: his club's reputation is read even without a club.
                const std::int32_t reputation = club().reputation();
                if (compare(c, reputation, 0x1a5e) > 0 || compare(c, reputation, current().reputation()) > 0) {
                    const double raised = fpu.fadd(1.0, score);
                    score = order(2.0, raised).lt ? 2.0 : raised;
                }
            }
        }
    } else if (loan() && test(routine::staff_test_3230, 0x1036b474)) {
        // On loan: 1.5 - happiness / 100.
        const double happiness = real(0xe4, std::int8_t(std::uint32_t(r25)));
        scale(fpu.fsub(1.5, fpu.fdiv(happiness, 100.0)));
    }
    if (loan()) {
        // 1 - 3 happiness / 200, at least 0.1; then 0.25 + |years left| / 12
        // (goalkeepers to 33, the others to 28, half again before).
        const double happiness = real(0xe4, std::int8_t(std::uint32_t(r25)));
        double factor = fpu.fsub(1.0, fpu.fdiv(fpu.fmul(3.0, happiness), 200.0));
        if (order(factor, 0.1).lt) factor = 0.1;
        scale(factor);
        if (wanted() && order(factor, 0.6).lt) reason(0x40000);
        double years;
        if (compare(c, p.player_record().goalkeeping(), 0x14) == 0) {
            years = real(0xe4, std::int32_t(0x21 - std::uint32_t(age())));
        } else {
            years = real(0xe4, std::int32_t(0x1c - std::uint32_t(age())));
            if (order(years, 0.0).lt) years = fpu.fdiv(years, 2.5);
        }
        const bool w = wanted();
        const double stage = fpu.fadd(0.25, fpu.fdiv(std::abs(years), 12.0));
        scale(stage);
        if (w && order(stage, 0.6).lt) reason(0x40000);
    }
    if (!has_club()) {
        scale(1.75);
        return std::nullopt;
    }
    if (test(routine::staff_test_3150, 0x1036b5e4) || test(routine::staff_test_3230, 0x1036b5f8)) return std::nullopt;
    if (has_contract() && nonzero(c, contract().listed())) return std::nullopt;
    // The career factor (f27): the days since +0x4c, x(20 - happiness / 5)
    // on loan, else x20 / (pressure + 10) (none when his contract has no
    // flags and the days are under 15 pressure + 120), clamped to 0..400, / 400.
    const Date now{c, today()};
    const Date since = p.joined();
    const bool on_loan = loan();
    leave(c, {{5, std::uint32_t(std::uint16_t(since.year()))}, {4, std::uint32_t(std::int32_t(since.day()))}});
    std::int32_t days = days_from(since, now);
    if (on_loan) {
        const std::int32_t happiness = std::int8_t(std::uint32_t(r25));
        leave(c, {{4, happiness < 0 ? 1u : 0u}});
        days = std::int32_t(std::uint32_t(days) * std::uint32_t(20 - happiness / 5));
    } else {
        // Quirk: the flags are read from the contract even without one.
        if (compare(c, c.i32(contract_at + 0x50)) == 0 &&
            compare(c, days, std::int32_t(std::uint32_t(p.loyalty()) * 15 + 0x78)) < 0)
            return 0.0;
        const std::int32_t divisor = compare(c, p.loyalty() + 10, 1) < 0 ? 1 : p.loyalty() + 10;
        leave(c, {{4, std::uint32_t(divisor)}});
        days = detail::divide(0x1036b6b4, std::int32_t(std::uint32_t(days) * 20), divisor);
    }
    std::int32_t clamped;
    if (compare(c, days) < 0) clamped = 0;
    else clamped = compare(c, days, 0x190) > 0 ? 0x190 : days;
    if (!nonzero(c, clamped)) return 0.0;
    fit = fpu.fdiv(real(0xe4, clamped), 400.0);
    return std::nullopt;
}
}
