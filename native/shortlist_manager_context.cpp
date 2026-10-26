// SHORTLIST_MANAGER.CPP 0x10366470: the candidate context evaluation, the
// prologue and its first stages (filters, weights, the reference club,
// contract, likes, rivals, staff views). The later stages are in
// shortlist_manager_context_{roles,valuation,verdict}.cpp.
#include "shortlist_manager_context_detail.hpp"

namespace cm::aot::shortlist_manager {
using namespace context;

double evaluate_candidate_context(Cpu& c, std::uint32_t object, std::uint32_t person, std::uint32_t club,
                                  std::uint32_t refine, std::uint32_t category, std::uint32_t reasons) {
    Evaluation evaluation(c, object, person, club, refine, category, reasons);
    return evaluation.evaluate();
}

namespace context {
namespace {
constexpr double rejected = 0.0;  // constants +0x158
}

Evaluation::Evaluation(Cpu& c, std::uint32_t object, std::uint32_t person_address, std::uint32_t club_address,
                       std::uint32_t refine, std::uint32_t category_register, std::uint32_t reasons_address)
    : c(c), frame(c, 0x10366470, 0x170, {.first_gpr = 13, .first_fpr = 24}), fpu(c), program(c),
      constants(c.toc(slot::constants)), singles(c.toc(slot::singles)), pool(c.toc(slot::pool)),
      person{c, person_address}, club{c, club_address} {
    constants_register = constants;
    reasons = reasons_address;
    const bool reasons_wanted = nonzero(c, std::int32_t(reasons_address));  // or. r18,r8,r8
    rival_nation_b = c.toc(slot::rival_nation_b);
    person_register = person_address;
    c.put32(frame.at(0x188), object);  // the caller's parameter area, read back for the per-club records
    club_register = club_address;
    rival_nation_a = c.toc(slot::rival_nation_a);
    category = category_register;
    c.put8(frame.at(0x197), refine);  // read back at the end
    nation_level = 0;
    start_date = c.toc(slot::start_date);
    stadium_record = 0;
    today = c.toc(slot::today);
    reference_stadium = 0;
    reference_job = pool;
    service = 1.0;
    if (reasons_wanted) c.put32(reasons, 0);
}

double Evaluation::evaluate() {
    if (const auto verdict = check_candidate()) return *verdict;
    weigh_factors();
    if (const auto verdict = find_reference_club()) return *verdict;
    if (const auto verdict = base_score()) return *verdict;
    relation_factors();
    neighbour_factors();
    staff_factors();
    if (const auto verdict = offer_check()) return *verdict;
    contract_factor();
    // A move (not his own club): his job against the category, the club's
    // value and how long he has served (0x103670f4).
    if (compare_logical(c, club.address, reference) != 0) {
        if (const auto verdict = role_factors()) return *verdict;
        results_factor();
        value_considered_club();
        foreign_factor();
        if (const auto verdict = service_factor()) return *verdict;
    }
    // 0x10367fd8: what he has now, then the comparison.
    if (!nonzero_logical(c, reference)) value_free_agent();
    else value_reference_club();
    if (compare_logical(c, club.address, reference) != 0) compare_clubs();
    return refine_by_quote();
}

std::optional<double> Evaluation::check_candidate() {
    // 0x103664d4: a staff record, a club nation and a nation are needed.
    if (!nonzero_logical(c, person.non_player()) || !nonzero_logical(c, club.nation()) ||
        !nonzero_logical(c, person.nation()))
        return rejected;
    // Three persons named in the pool are never taken from a club.
    const auto named = [&](std::uint32_t text, std::uint32_t back) {
        return nonzero(c, byte_of(program.call(call::name_is, back, {person.address, pool + text})));
    };
    if (named(text::dario_gradi, 0x1036650c) || named(text::pat_nevin, 0x10366520) ||
        named(text::kevin_keegan, 0x10366534)) {
        if (nonzero_logical(c, person.club())) return rejected;
    }
    // 0x10366550: job 0x11 at his club or his national team rules him out.
    const std::uint32_t job = std::uint8_t(person.job()), national_job = person.national_job();
    if (compare(c, std::int32_t(job), 0x11) == 0) return rejected;
    if (compare(c, std::int32_t(national_job), 0x11) == 0) return rejected;
    const std::int32_t his_job = byte_of(job), his_national_job = byte_of(national_job), wanted = category8();
    c.set(4, word(wanted));
    if (compare(c, wanted, role::manager) != 0) {
        // The last 16 persons only as managers (inferred: reserved ids).
        const std::uint32_t limit = c.u32(c.toc(slot::person_count)) - 0x10;
        const std::int32_t id = person.id();
        leave(c, {{4, limit}, {5, word(id)}});
        if (compare(c, id, std::int32_t(limit)) >= 0) return rejected;
    }
    const std::uint32_t national_team = person.national_team();
    c.set(4, national_team);
    if (nonzero_logical(c, national_team)) {
        c.set(5, word(his_national_job));
        if (compare(c, his_national_job, role::manager) == 0) return rejected;
    }
    // 0x103665bc: category 0 skips the job rules.
    c.set(5, word(wanted));
    if (nonzero(c, wanted)) {
        if (compare(c, wanted, role::manager) != 0) {
            // A manager wanted elsewhere: not over 4250 reputation or 100 ability.
            c.set(5, word(his_job));
            bool manager = either(his_job, role::manager, role::assistant);
            if (!manager) {
                c.set(5, word(his_national_job));
                manager = either(his_national_job, role::manager, role::assistant);
            }
            if (manager) {
                const StaffRecord staff{c, person.non_player()};
                const std::int32_t reputation = staff.reputation();
                leave(c, {{5, word(reputation)}, {6, staff.address}});
                if (compare(c, reputation, 0x109a) > 0) return rejected;
                const std::int32_t ability = staff.ability();
                c.set(5, word(ability));
                if (compare(c, ability, 0x64) > 0) return rejected;
            }
        }
        // 0x1036661c: category 10 only for job 10 or no job.
        c.set(5, word(wanted));
        if (compare(c, wanted, 10) == 0) {
            c.set(5, word(his_job));
            if (compare(c, his_job, 10) != 0 && nonzero(c, his_job)) return rejected;
        }
        c.set(5, word(his_national_job));
        if (compare(c, his_national_job, role::manager) == 0 && nonzero_logical(c, national_team)) return rejected;
        // 0x10366660: a player with job 11 only to his own club, but for
        // categories 15, 12 and 5.
        c.set(4, word(wanted));
        if (compare(c, wanted, 15) != 0 && compare(c, wanted, role::assistant) != 0 &&
            compare(c, wanted, role::manager) != 0) {
            const std::uint32_t player = person.player();
            c.set(4, player);
            if (nonzero_logical(c, player)) {
                c.set(4, word(his_job));
                if (compare(c, his_job, 11) == 0) {
                    const std::uint32_t own = person.club();
                    c.set(4, own);
                    if (nonzero_logical(c, own) && compare_logical(c, club.address, own) != 0) return rejected;
                }
            }
        }
        // Job 10 only for category 10.
        c.set(4, word(wanted));
        if (compare(c, wanted, 10) != 0) {
            c.set(4, word(his_job));
            if (compare(c, his_job, 10) == 0) return rejected;
        }
    }
    // 0x103666d0: a manager not for a category-5 job leaves only for a club
    // in a better division and not over 750 below his own.
    if (nonzero_logical(c, club.address)) {
        c.set(4, word(wanted));
        if (compare(c, wanted, role::manager) != 0 &&
            (either(his_job, role::manager, role::assistant) ||
             either(his_national_job, role::manager, role::assistant))) {
            const std::uint32_t own = person.club();
            c.set(4, own);
            if (nonzero_logical(c, own)) {
                const Club current{c, own};
                const std::uint32_t division = club.division();
                c.set(5, division);
                if (nonzero_logical(c, division)) {
                    const std::uint32_t current_division = current.division();
                    if (nonzero_logical(c, current_division) &&
                        compare(c, Division{c, current_division}.reputation(), Division{c, division}.reputation()) >= 0)
                        return rejected;
                }
                const std::int32_t ours = club.reputation(), theirs = current.reputation();
                c.set(4, word(theirs));
                if (compare(c, theirs, std::int32_t(word(ours) - 0x2ee)) > 0) return rejected;
            }
        }
    }
    // 0x10366764: a club under 2750 takes no foreigner (inferred) from
    // another nation.
    if (nonzero_logical(c, club.address) && compare(c, club.reputation(), 0xabe) < 0) {
        const std::uint32_t nation = club.nation();
        c.set(4, nation);
        if (nonzero_logical(c, nation)) {
            const std::uint32_t own = person.club();
            if (nonzero_logical(c, own) && compare_logical(c, Club{c, own}.nation(), nation) == 0) return rejected;
            if (nonzero(c, byte_of(program.call(call::is_foreigner_in, 0x103667a8, {person.address, nation, 1}))))
                return rejected;
        }
    }
    return std::nullopt;
}

void Evaluation::weigh_factors() {
    // 0x103667b8: the weights of the club value's components: the staff's
    // rating 1 (0 for the manager categories), prestige 1.4 - (ambition +
    // trait 0x67) / 40, division 0.8 + ambition / 50, capacity 0.5 +
    // ambition / 20 (singles), their sum + 1 (0.5 when not positive).
    staff_weight = manager_category() ? k::zero_s : k::one_s;
    const std::int32_t ambition = byte_of(person.ambition()), trait = byte_of(person.sportsmanship());
    leave(c, {{4, signed_word(ambition)}});
    // The original's order of operations (FPSCR).
    const double ambition_1 = integer(0xd4, ambition);
    const double trait_stacked = stacked(0xcc, trait);
    const double ambition_40 = fpu.fdiv(ambition_1, 40.0);
    const double ambition_stacked = stacked(0xc4, ambition);
    const double trait_value = fpu.fsub(trait_stacked, k::signed_bias);
    const double prestige_part = fpu.fsub(1.4, ambition_40);
    const double ambition_2 = fpu.fsub(ambition_stacked, k::signed_bias);
    const double trait_40 = fpu.fdiv(trait_value, 40.0);
    const double ambition_third = stacked(0xbc, ambition);
    const double ambition_50 = fpu.fdiv(ambition_2, 50.0);
    const double ambition_3 = fpu.fsub(ambition_third, k::signed_bias);
    const double prestige_value = fpu.fsub(prestige_part, trait_40);
    const double division_value = fpu.fadd(0.8, ambition_50);
    const double ambition_20 = fpu.fdiv(ambition_3, 20.0);
    const double capacity_value = fpu.fadd(0.5, ambition_20);
    prestige_weight = fpu.frsp(prestige_value);
    division_weight = fpu.frsp(division_value);
    capacity_weight = fpu.frsp(capacity_value);
    double sum = fpu.fadds(staff_weight, prestige_weight);
    sum = fpu.fadds(division_weight, sum);
    sum = fpu.fadds(capacity_weight, sum);
    sum = fpu.fadds(k::one_s, sum);
    const auto order = fcompare(sum, 0.0);
    merge_equal(true);
    weight_sum = order.less || order.equal ? k::half_s : sum;
}

std::optional<double> Evaluation::find_reference_club() {
    // 0x103668ac: his club, or for a national team's job his national
    // team (unless his job there is 11).
    reference = person.club();
    reference_job = std::uint8_t(person.job());
    const bool national = !nonzero_logical(c, reference) ||
                          nonzero(c, byte_of(program.call(call::is_national_team, 0x103668c4, {club.address})));
    if (national) {
        const std::uint32_t team = person.national_team();
        if (nonzero_logical(c, team)) {
            const std::uint32_t job = person.national_job();
            c.set(4, job);
            if (compare(c, std::int32_t(job), 11) != 0) {
                reference = team;
                reference_job = job;
            }
        }
    }
    // Quirk: a reference club without a nation scores 3.5 (constants +0x160).
    if (nonzero_logical(c, reference) && !nonzero_logical(c, Club{c, reference}.nation())) return 3.5;
    return std::nullopt;
}

std::optional<double> Evaluation::base_score() {
    // 0x10366908
    contract = program.call(call::contract_with, 0x1036691c,
                            {c.u32(c.toc(slot::contracts)), person.address, reference});
    if (compare_logical(c, club.address, reference) == 0) {
        score = 3.5;  // his own club
        return std::nullopt;
    }
    // A contract flag (0x20) keeps him from a club of his club's nation group.
    if (nonzero_logical(c, contract) && nonzero_logical(c, reference)) {
        const std::uint32_t theirs = Club{c, reference}.nation();
        c.set(4, theirs);
        if (nonzero_logical(c, theirs)) {
            const std::uint32_t ours = club.nation();
            if (nonzero_logical(c, ours) &&
                compare(c, byte_of(Nation{c, ours}.group()), byte_of(Nation{c, theirs}.group())) == 0 &&
                nonzero(c, std::int32_t(Contract{c, contract}.flags() & 0x20)))
                return rejected;
        }
    }
    score = manager_category() ? 2.25 : 1.5;
    if (nonzero_logical(c, reference)) return std::nullopt;
    // 0x103669a4: out of work since the later of the game's start and the
    // day he left (inferred: Person::joined then), unless the game starts
    // after today: + min(12, days^2 / (900 + current reputation)), the
    // days capped at 500.
    const std::uint32_t start = start_date, now = today;
    const std::uint32_t start_copy = frame.at(0x40), joined_copy = frame.at(0x38);
    c.put32(start_copy, c.u32(start));
    c.put32(start_copy + 4, c.u32(start + 4));
    const std::uint32_t this_year = c.u16(now + 2);
    c.set(4, this_year);
    std::int32_t later_start = 0;
    if (const auto by_year = compare_logical(c, c.u16(start_copy + 2), this_year); by_year > 0) {
        later_start = 1;
    } else if (by_year == 0) {
        const std::int32_t start_day = c.i16(start_copy);
        c.set(5, word(start_day));
        later_start = after_day(start_day, c.i16(now));
    }
    if (nonzero(c, later_start)) return std::nullopt;
    c.put32(joined_copy, c.u32(person.joined().address));
    c.put32(joined_copy + 4, c.u32(person.joined().address + 4));
    const std::uint32_t start_year = c.u16(start + 2);
    std::int32_t since_day = person.joined().day();
    std::uint32_t since_year = person.joined().year();
    std::int32_t before_start = 0;
    if (const auto by_year = compare_logical(c, c.u16(joined_copy + 2), start_year); by_year < 0) {
        before_start = 1;
    } else if (by_year == 0) {
        const std::int32_t first = c.i16(start);
        const std::int32_t mixed = c.i16(joined_copy) ^ first;
        leave(c, {{7, word(mixed >> 1)}, {8, word(first)}});
        before_start = after_day(first, c.i16(joined_copy));
    }
    c.set(6, word(before_start));
    if (nonzero(c, before_start)) {
        since_day = c.i16(start);
        since_year = start_year;
    }
    const std::uint32_t years = this_year - (since_year & 0xffff);
    std::int32_t days = std::int32_t(years * 365 + word(c.i16(now) - since_day));
    if (compare(c, days, 500) > 0) days = 500;
    const StaffRecord staff{c, person.non_player()};
    const std::uint32_t squared = word(days) * word(days);
    leave(c, {{4, 0x43300000}, {5, squared}});
    const double reputation = integer(0xc4, staff.reputation());
    const double square = integer(0xbc, std::int32_t(squared));
    const double denominator = fpu.fadd(900.0, reputation);
    const double share = fpu.fdiv(square, denominator);
    score = fpu.fadd(score, fcompare(12.0, share).less ? 12.0 : share);
    return std::nullopt;
}

void Evaluation::relation_factors() {
    // 0x10366afc: what he thinks of the club and of the club he would leave.
    const auto asks = [&](std::uint32_t routine, std::uint32_t back, std::uint32_t of) {
        return nonzero(c, byte_of(program.call(routine, back, {person.address, of})));
    };
    if (asks(call::dislikes_club, 0x10366b08, club.address)) {
        score = fpu.fmul(score, 0.33);
        note(reason::hostile);
    }
    if (asks(call::dislikes_club, 0x10366b38, reference)) {
        score = fpu.fmul(score, 2.0);
        note(reason::dislikes_reference);
    }
    if (asks(call::likes_club, 0x10366b68, reference)) {
        // Quirk: the reference club's status is read even when there is none.
        const bool to_professional = compare(c, Club{c, reference}.status(), 1) != 0 &&
                                     compare(c, club.status(), 1) == 0;
        score = fpu.fmul(score, to_professional ? 0.85 : 0.5);
        if (has_reasons() && nonzero_logical(c, contract) && !nonzero(c, byte_of(Contract{c, contract}.listed())))
            add_reason(reason::likes_reference);
    }
    if (asks(call::likes_club, 0x10366bd0, club.address)) {
        note(reason::likes_club);
        score = fpu.fmul(score, 3.0);
    }
}

void Evaluation::neighbour_factors() {
    // 0x10366bf4: a move between related clubs (affiliates or 0x10464160)
    // weighs his loyalty.
    const Club ref{c, reference};
    const auto affiliated = [&] {
        return compare_logical(c, club.address, ref.affiliate(0)) == 0 ||
               compare_logical(c, club.address, ref.affiliate(1)) == 0 ||
               compare_logical(c, club.address, ref.affiliate(2)) == 0;
    };
    bool related = false;
    if (nonzero_logical(c, reference) && compare_logical(c, reference, club.address) != 0)
        related = affiliated() ||
                  nonzero(c, byte_of(program.call(call::clubs_related, 0x10366c34, {club.address, reference})));
    if (related) {
        const std::int32_t loyalty = byte_of(person.loyalty());
        // base - loyalty / divisor (pool doubles), in the original's order.
        const auto lessened = [&](double base, double divisor) {
            const double value = integer(0xbc, loyalty);
            const double part = fpu.fdiv(value, divisor);
            return fpu.fsub(base, part);
        };
        const std::int32_t rival_a = c.i32(c.toc(slot::rival_club_a));
        const auto rival_b = [&] { return c.i32(c.toc(slot::rival_club_b)); };
        const std::int32_t ours = club.id();
        leave(c, {{4, word(ours)}, {5, word(rival_a)}});
        bool rivals = false;
        if (compare(c, ours, rival_a) == 0 || compare(c, ours, rival_b()) == 0) {
            const std::int32_t theirs = ref.id();
            c.set(4, word(theirs));
            rivals = compare(c, theirs, rival_a) == 0 || compare(c, theirs, rival_b()) == 0;
        }
        double factor;
        if (rivals) {
            factor = lessened(0.5, 40.0);  // between the two rival clubs
        } else {
            const std::int32_t nation = Nation{c, club.nation()}.id();
            c.set(4, word(nation));
            if (compare(c, nation, c.i32(c.toc(slot::neighbour_nation))) == 0) {
                factor = lessened(1.0, 60.0);
            } else if (affiliated() && compare_logical(c, club.division(), ref.division()) == 0) {
                const std::uint32_t gap = program.call(call::absolute, 0x10366d48,
                                                       {word(club.reputation()) - word(ref.reputation())});
                factor = compare(c, std::int32_t(gap), 0x4e2) < 0 ? lessened(0.75, 40.0) : lessened(0.8, 60.0);
            } else {
                factor = lessened(0.85, 40.0);
            }
        }
        score = fpu.fmul(score, factor);
        if (has_reasons() && fcompare(factor, 0.7).less) add_reason(reason::local_rival);
        return;
    }
    // 0x10366e1c: between the two rival nations (TOC ids): from a club of B
    // to one of A for a person of B x0.05, the other way x0.1.
    const auto nation_id = [&](std::uint32_t nation) { return std::int32_t(Nation{c, nation}.id()); };
    if (nonzero_logical(c, reference) &&
        compare(c, nation_id(club.nation()), c.i32(rival_nation_a)) == 0) {
        const std::int32_t b = c.i32(rival_nation_b);
        c.set(4, word(b));
        if (compare(c, nation_id(ref.nation()), b) == 0 && compare(c, nation_id(person.nation()), b) == 0) {
            score = fpu.fmul(score, 0.05);
            note(reason::hostile);
            return;
        }
    }
    if (nonzero_logical(c, reference) &&
        compare(c, nation_id(club.nation()), c.i32(rival_nation_b)) == 0) {
        const std::int32_t a = c.i32(rival_nation_a);
        c.set(4, word(a));
        if (compare(c, nation_id(ref.nation()), a) == 0 && compare(c, nation_id(person.nation()), a) == 0) {
            score = fpu.fmul(score, 0.1);
            note(reason::hostile);
        }
    }
}

void Evaluation::staff_factors() {
    // 0x10366ed8: the clubs' staff's views of him (not for the manager
    // categories at the club itself).
    const auto asks = [&](std::uint32_t routine, std::uint32_t back, std::uint32_t staff) {
        return nonzero(c, byte_of(program.call(routine, back, {staff, person.address})));
    };
    if (!manager_category() && asks(call::staff_favours, 0x10366ef8, club.manager())) {
        score = fpu.fmul(score, 2.0);
        note(reason::staff_favours);
    }
    if (!manager_category() && asks(call::staff_dislikes, 0x10366f3c, club.manager())) {
        score = fpu.fmul(score, 0.33);
        note(reason::staff_dislikes);
    }
    const Club ref{c, reference};
    if (nonzero_logical(c, reference) && asks(call::staff_dislikes, 0x10366f74, ref.manager())) {
        score = fpu.fmul(score, 2.0);
        note(reason::reference_staff_dislikes);
    }
    if (nonzero_logical(c, reference) && asks(call::staff_favours, 0x10366fac, ref.manager())) {
        score = fpu.fmul(score, 0.5);
        note(reason::reference_staff_favours);
    }
}

std::optional<double> Evaluation::offer_check() {
    // 0x10366fd0: a player under a special offer (0x103f43b0) is out.
    if (nonzero_logical(c, person.player()) &&
        nonzero(c, byte_of(program.call(call::special_offer, 0x10366ff0,
                                   {c.u32(c.toc(slot::offers)), person.address, 0})))) {
        note(reason::offer);
        return rejected;
    }
    return std::nullopt;
}

void Evaluation::contract_factor() {
    // 0x10367014: his contract's signed byte +0x4c: at his own club x(1 +
    // b/400), elsewhere x(1 - b/400). At his own club the jobs compared
    // later are the category's.
    if (!nonzero_logical(c, contract)) {
        if (compare_logical(c, club.address, reference) == 0) reference_job = category;
        return;
    }
    const Contract terms{c, contract};
    const std::int32_t mood = byte_of(terms.morale());
    if (compare_logical(c, club.address, reference) == 0) {
        reference_job = category;
        const double value = integer(0xbc, mood);
        const double part = fpu.fdiv(value, 400.0);
        score = fpu.fmul(score, fpu.fadd(1.0, part));
        return;
    }
    const bool reasons_wanted = has_reasons();
    const double value = integer(0xbc, mood);
    const double part = fpu.fdiv(value, 400.0);
    const double factor = fpu.fsub(1.0, part);
    score = fpu.fmul(score, factor);
    if (reasons_wanted && !nonzero(c, std::int32_t(terms.options() & 8))) {
        if (fcompare(factor, 1.5).greater) add_reason(reason::contract_low);
        else if (fcompare(factor, 0.5).less) add_reason(reason::contract_high);
    }
}
}
}
