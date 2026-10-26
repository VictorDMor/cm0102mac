#include "shortlist_manager_interest_detail.hpp"

// evaluate_club_interest (0x10369080): how interested a club is in a person
// (a player or a member of staff), as a score in f1; 0 when the club would
// not approach him. The reasons it finds go into an optional mask. This
// file: the entry, the opening checks and the base score (0x10369080-
// 0x103695f8), and the loan reputation factors (0x103695f8-0x10369760).
// The other stages are in shortlist_manager_interest_*.cpp.
namespace cm::aot::shortlist_manager {
namespace interest {
std::optional<double> Evaluation::opening() {
    const Person p = person();
    program.call(routine::empty_date, 0x103690dc, {frame.at(0x78)});
    program.call(routine::empty_date, 0x103690e4, {frame.at(0x70)});
    fit = 1.0;
    if (wanted()) c.put32(reasons_at, 0);
    if (!nonzero_logical(c, p.player()) || !nonzero_logical(c, club().nation()) || !nonzero_logical(c, p.nation()))
        return 0.0;
    const auto offer = program.call(routine::pending_offer, 0x10369138, {c.u32(c.toc(toc::transfers)), person_at, 0});
    if (nonzero(c, std::int8_t(offer))) return 0.0;
    // The person's row in the flags table: bits 2-3 of +0xc rule him out (inferred).
    const auto flag_rows = c.u32(r13);
    leave(c, {{4, flag_rows}});
    if (nonzero(c, std::int32_t((c.u8(flag_rows + std::uint32_t(p.id()) * 100 + 0xc) >> 2) & 3))) return 0.0;

    // The weights of the club values (single precision): 1.4 - (determination
    // + sportsmanship) / 40, 0.8 + determination / 50, 0.5 + determination / 20.
    const std::int32_t determination = p.ambition(), sportsmanship = p.sportsmanship();
    leave(c, {{4, person_at}, {5, signed_word(determination)}, {6, extended(p.age_halfword())}});
    age_reg = std::uint32_t(p.age());
    const double d = raw_signed(0x124, determination);
    raw_signed(0x114, determination);
    raw_signed(0x10c, determination);
    const double s = raw_signed(0x11c, sportsmanship);
    double squad = fpu.fdiv(fpu.fsub(d, k::signed_bias), 40.0);
    const double d2 = fpu.fsub(d, k::signed_bias);
    squad = fpu.fsub(1.4, squad);
    double temper = fpu.fsub(s, k::signed_bias);
    const double wage = fpu.fdiv(d2, 50.0);
    const double d3 = fpu.fsub(d, k::signed_bias);
    temper = fpu.fdiv(temper, 40.0);
    const double league = fpu.fdiv(d3, 20.0);
    weight_squad = fpu.fsub(squad, temper);
    weight_wage = fpu.fadd(0.8, wage);
    weight_league = fpu.fadd(0.5, league);
    weight_squad = fpu.frsp(weight_squad);
    weight_wage = fpu.frsp(weight_wage);
    weight_league = fpu.frsp(weight_league);

    // His club: the one of his primary contract (published in the scratch
    // slot), unless that contract is of kind 5 (inferred: a loan).
    const auto primary = program.call(routine::primary_contract, 0x10369228, {contracts(), person_at});
    const bool found = nonzero_logical(c, primary);
    c.put32(r23, primary);
    const std::uint32_t club_of =
        found ? c.u32(c.toc(slot::clubs)) + c.u32(primary + 4) * Club::size : 0;
    current_at = club_of;
    if (nonzero_logical(c, club_of) && !nonzero_logical(c, current().nation())) return 3.5;
    if (nonzero_logical(c, club_of)) {
        const auto again = program.call(routine::primary_contract, 0x10369288, {contracts(), person_at});
        const std::int32_t kind = std::int8_t(c.u8(again + 0x38)) >> 2;  // rlwinm/srawi: bits 7-2, signed
        if (compare(c, kind, 5) == 0) current_at = 0;
    }
    contract_at = program.call(routine::club_contract, 0x103692b8, {contracts(), person_at, current_at});
    r23 = nonzero(c, std::int32_t(std::uint32_t(contract_at))) ? contract().flags().get() : 0u;

    // His own club, when he would stay for less than an allowance: 3.5.
    if (nonzero_logical(c, club_at) && own_club() && has_contract()) {
        const auto now = today();
        const std::int32_t left = std::int32_t(
            program.call(routine::days_between, 0x103692fc, {contract_at + 0x30, c.u32(now), c.u32(now + 4)}));
        const Player player = p.player_record();
        const bool keen = flag(flag::bit_10);
        const std::int32_t player_reputation = player.reputation();
        const std::int32_t gap = std::int32_t(std::uint32_t(std::int32_t(club().reputation())) - std::uint32_t(player_reputation));
        std::int32_t allowance = truncated(0x114, fpu.fmul(0.5, fpu.fsubs(raw_signed(0x10c, gap), k::signed_bias)));
        if (keen) {
            const double half = fpu.fsub(raw_signed(0x11c, std::int16_t(allowance)), k::signed_bias);
            allowance = truncated(0x124, fpu.fmul(0.5, half));
        }
        leave(c, {{4, std::uint32_t(player_reputation)}, {5, std::uint32_t(allowance)}});
        if (compare(c, std::int16_t(allowance)) < 0) {
            allowance = 0;
        } else {
            const std::int32_t cap = p.loyalty() * 10 + 0x465;
            leave(c, {{7, std::uint32_t(std::int32_t(std::int16_t(allowance)))}});
            if (compare(c, std::int16_t(allowance), cap) > 0) allowance = std::int16_t(cap);
        }
        leave(c, {{5, std::uint32_t(allowance)}, {6, 0x10000}});
        if (!nonzero(c, std::int32_t(flags() & flag::blocking))) {
            const std::int32_t id = p.id();
            const std::int32_t threshold = id % 35 + 20;
            leave(c, {{6, std::uint32_t(threshold)}, {7, id < 0 ? 1u : 0u}});
            if (compare(c, left, threshold) < 0) {
                const auto rows = c.u32(r13);
                leave(c, {{6, rows}});
                if (!nonzero(c, std::int32_t((c.u8(rows + std::uint32_t(id) * 100 + 0xc) >> 4) & 0xf))) {
                    const std::int32_t limit = p.loyalty() * 25 + std::int16_t(allowance) + 0x109a;
                    const std::int32_t standing = player_reputation + p.ambition() * 20;
                    leave(c, {{4, std::uint32_t(standing)}, {5, std::uint32_t(p.ambition() * 20)}});
                    if (compare(c, standing, limit) < 0) return 3.5;
                }
            }
        }
    }

    if (own_club()) {
        // His own club: only without a contract or with flags 0 or 2.
        if (has_contract() && compare(c, std::int32_t(flags())) != 0 && compare(c, std::int32_t(flags()), 2) != 0)
            return 0.0;
        score = 3.5;
        return std::nullopt;
    }
    if (has_contract() && has_club()) {
        const auto current_nation = current().nation().get();
        leave(c, {{4, current_nation}});
        if (nonzero_logical(c, current_nation)) {
            const auto club_nation = club().nation().get();
            // Same zone and contract flag 0x20 (inferred: no move within the zone).
            if (nonzero_logical(c, club_nation) &&
                compare(c, std::int8_t(c.u8(club_nation + 0x119)), std::int8_t(c.u8(current_nation + 0x119))) == 0 &&
                flag(flag::bit_20))
                return 0.0;
        }
    }
    if (compare(c, age(), 16) < 0 &&
        nonzero(c, std::int8_t(program.call(routine::nation_tie, 0x103694a0, {person_at, club().nation(), 1}))))
        return 0.0;
    const bool contracted = has_contract();
    score = 2.75;
    if (contracted && nonzero(c, contract().listed())) {
        // Listed: 1.1 - 0.002 * the days his contract has left.
        const Date now{c, today()};
        const Date ends = contract().ends();
        const bool w = wanted();
        const auto left = days_from(now, ends);
        const double days = fpu.fsub(raw_signed(0x10c, left), k::signed_bias);
        const double factor = multiply_add(0x10369514, 0.002, days, 1.1);
        if (w) {
            const double professionalism = fpu.fsub(raw_signed(0x10c, p.professionalism()), k::signed_bias);
            if (order(factor, multiply_add(0x10369540, 0.02, professionalism, 0.6)).lt) reason(1);
        }
        scale(factor);
    } else if (!nonzero_logical(c, current_at)) {
        // Without a club: the days since +0x4c, squared / 900, at most 12, added.
        const Date now{c, today()};
        const Date since = p.joined();
        const auto days = std::uint32_t(days_from(since, now));
        const double bonus = fpu.fdiv(fpu.fsub(raw_signed(0x10c, std::int32_t(days * days)), k::signed_bias), 900.0);
        score = fpu.fadd(score, order(12.0, bonus).lt ? 12.0 : bonus);
    }
    return std::nullopt;
}

void Evaluation::loan_reputation() {
    if (!loan()) return;
    const auto player_at = person().player().get();
    if (!nonzero_logical(c, player_at)) return;
    const Player player{c, player_at};
    if (compare(c, player.world_reputation(), 0x128e) >= 0 || !nonzero_logical(c, club_at) || !has_club() ||
        own_club() || !flag(flag::bit_10))
        return;
    // A loan to a club bigger than his: more when he is no star (reputation
    // at most 7250 and 0x100d3230 says so).
    const auto modest = [&](std::int32_t reputation, std::uint32_t back) {
        return compare(c, reputation, 0x1c52) <= 0 && test(routine::staff_test_3230, back);
    };
    const std::int32_t club_reputation = club().reputation(), current_reputation = current().reputation();
    const auto same_nation_or_over = [&](std::int32_t level) {
        return compare_logical(c, club().nation(), current().nation()) == 0 || compare(c, club_reputation, level) > 0;
    };
    if (compare(c, club_reputation, current_reputation) > 0 && same_nation_or_over(0x1676)) {
        scale(modest(player.reputation(), 0x10369678) ? 1.5 : 1.25);
        return;
    }
    if (compare(c, club_reputation, 0xabe) > 0 && same_nation_or_over(0x1676)) {
        const std::int32_t reputation = player.reputation();
        if (compare(c, club_reputation, reputation) > 0) {
            scale(modest(reputation, 0x103696d8) ? 1.35 : 1.15);
            return;
        }
    }
    if (compare(c, club_reputation, 0x109a) <= 0 || compare_logical(c, club().nation(), current().nation()) != 0) return;
    const std::int32_t reputation = player.reputation();
    if (compare(c, reputation, 0x186a) > 0 && compare(c, club_reputation, 0x1482) < 0) {
        scale(1.025);
        return;
    }
    scale(modest(reputation, 0x10369744) ? 1.25 : 1.1);
}
}

double evaluate_club_interest(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club,
                              std::uint8_t mode, std::uint32_t loan, std::uint32_t role, std::uint32_t reasons) {
    using namespace interest;
    Frame frame(c, 0x10369080, 0x1d0, {.first_gpr = 13, .gpr_offset = -0x8c, .first_fpr = 25});
    c.put32(frame.at(0x1e8), manager);
    c.put8(frame.at(0x1f7), mode);
    Evaluation e(c, frame);
    e.person_at = person;
    e.r25 = c.toc(toc::region_b);
    e.club_at = club;
    e.r15 = c.toc(toc::rival_club_b);
    e.loan_word = loan;
    e.role_word = role;
    e.r13 = c.toc(toc::person_flags);
    e.reasons_at = reasons;
    e.r23 = c.toc(toc::contract_scratch);
    e.constants = c.toc(slot::constants);
    e.candidate_tier = 0;
    e.candidate_league = 0;
    e.current_league = 0;
    if (const auto early = e.opening()) return *early;
    e.loan_reputation();
    e.relations();
    e.managers();
    e.current_status();
    if (e.own_club()) {
        e.staying_role();
    } else {
        e.move_roles();
        if (const auto early = e.candidate_value()) return *early;
        e.location();
        if (const auto early = e.later_career()) return *early;
    }
    e.current_value();
    if (compare_logical(c, e.club_at, e.current_at) != 0) e.value_comparison();
    if (const auto early = e.recent_contract()) return *early;
    return e.finances();
}
}
