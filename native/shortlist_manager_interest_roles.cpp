#include "shortlist_manager_interest_detail.hpp"

// evaluate_club_interest (0x10369080), the roles: what his role at his club
// and the role the club has in mind make of a move (0x10369f24-0x1036a37c),
// or of staying at his own club (0x1036b710-0x1036b92c); where the move
// takes him (0x1036ad00-0x1036b3e0); and his career stage (0x1036b3e0-
// 0x1036b710). The roles run 0-7 (inferred: 1 the most important, 7 not
// wanted); the original switches on them through three jump tables.
namespace cm::aot::shortlist_manager::interest {
namespace {
// A role as the jump tables take it: `cmplwi r0,7; bgt` on the signed byte.
bool in_table(Cpu& c, std::int32_t role) { return compare_logical(c, std::uint32_t(role), 7) <= 0; }
}

void Evaluation::move_roles() {
    const Person p = person();
    if (nonzero(c, std::int8_t(program.call(routine::human_controlled, 0x10369f3c, {club_at, 0, 1, 0, 0})))) {
        // A human manager: 0.5 + his pressure byte / 40.
        const Person manager{c, club().manager()};
        const bool w = wanted();
        const double factor = fpu.fadd(0.5, fpu.fdiv(real(0x10c, manager.loyalty()), 40.0));
        scale(factor);
        if (w && order(factor, 0.675).lt) reason(0x20000000);
    }
    const auto young = [&] { return compare(c, age(), 0x20) < 0; };
    const auto in_prime = [&] { return compare(c, age(), 0x17) > 0 && compare(c, age(), 0x20) < 0; };
    const auto youth = [&] { return nonzero_logical(c, p.byte_5e()); };
    // His current role (cases_current_role).
    const auto unflagged = [&](double factor) {
        // Quirk: the flags are read again from the contract.
        const bool w = wanted();
        scale(factor);
        if (w && compare(c, c.i32(contract_at + 0x50)) == 0) reason(0x1000);
    };
    const std::int32_t was = current_role();
    if (in_table(c, was)) {
        switch (was) {
        case 1: unflagged(0.65); break;
        case 2: unflagged(0.75); break;
        case 3: scale(0.9); break;
        case 4:
            scale(1.8, 0x2000);
            if (in_prime()) scale(2.0);
            if (young() && youth()) scale(2.0);
            break;
        case 5:
            if (!loan()) scale(0.75);
            if (young() && youth()) scale(2.0);
            break;
        case 6: {
            const bool under_32 = young();
            scale(1.1);
            if (under_32 && youth()) scale(4.0);
            break;
        }
        case 7: scale(7.0, 0x2000); break;
        default: break;
        }
    }
    // The role the club has in mind (cases_club_role); r15 holds it from here.
    r15 = std::uint32_t(std::int32_t(club_role()));
    const std::int32_t wanted_role = club_role();
    const auto single = [&](std::uint32_t offset) { return double(c.f32(c.toc(slot::singles) + offset)); };
    const auto keen = [&] { return has_contract() && nonzero(c, std::int32_t(contract().flags() & flag::bit_10)); };
    const auto goalkeeper = [&] { return compare(c, p.player_record().goalkeeping(), 0x14) == 0; };
    if (!in_table(c, wanted_role)) return;
    switch (wanted_role) {
    case 1:
        if (keen()) {
            const bool w = wanted();
            divide(0.5);
            if (w) reason(0x4000);
        } else {
            divide(0.65);
        }
        weight_league = fpu.fmuls(weight_league, single(0x14));
        break;
    case 2:
        if (keen()) {
            const bool w = wanted();
            divide(0.6);
            if (w) reason(0x4000);
        } else {
            divide(0.75);
        }
        weight_league = fpu.fmuls(weight_league, single(0x18));
        break;
    case 3:
        if (!keen()) divide(0.9);
        if (goalkeeper()) scale(0.8);
        break;
    case 4:
        if (flag(flag::bit_10)) {
            scale(0.175);
            if (nonzero_logical(c, p.non_player())) divide(0.375);
            else if (wanted()) reason(0x8000);
        } else {
            scale(goalkeeper() ? 0.15 : 0.375);
        }
        if (in_prime()) scale(0.5);
        if (young() && youth()) scale(0.5);
        weight_league = fpu.fmuls(weight_league, single(0x1c));
        break;
    case 5:
        if (flag(flag::bit_10)) {
            const bool w = wanted();
            divide(30.0);
            if (w) reason(0x8000);
        } else {
            scale(goalkeeper() ? 0.2 : 0.4);
        }
        if (young() && youth()) scale(0.5);
        weight_league = fpu.fmuls(weight_league, single(0x20));
        break;
    case 6:
        if (flag(flag::bit_10)) scale(0.02, 0x8000);
        else scale(goalkeeper() ? 0.1 : 0.15);
        if (young() && youth()) scale(0.25);
        weight_league = fpu.fmuls(weight_league, single(0x4));
        break;
    case 7: scale(0.002); break;
    default: break;
    }
}

void Evaluation::staying_role() {
    const Person p = person();
    if (nonzero(c, std::int8_t(program.call(routine::human_controlled, 0x1036b728, {club_at, 0, 1, 0, 0}))) &&
        manager_date_near(club().manager(), 0x58, 0x40, 0x1036b740)) {
        // A human manager: his happiness against his pressure sets the role
        // (inferred: an unhappy player is less important).
        const std::int32_t happiness = std::int8_t(std::uint32_t(r25));
        const std::int32_t threshold = std::int8_t(std::uint32_t(-50) - c.u8(person_at + 0x65));
        const auto at_least = [&](std::int32_t floor) {
            const std::int32_t was = current_role();
            role = std::uint32_t(std::int32_t(std::int8_t(compare(c, was, floor) < 0 ? floor : was)));
        };
        // Below the threshold he is not wanted, unless his role is 5 or 6.
        const std::int32_t was = current_role();
        if (compare(c, happiness, threshold) < 0 && compare(c, was, 5) != 0 && compare(c, was, 6) != 0) role = 7;
        else if (compare(c, happiness) < 0) at_least(4);
        else if (compare(c, happiness, 0x32) < 0) at_least(3);
        else if (compare(c, happiness, 0x4b) < 0) at_least(2);
        else at_least(1);
    }
    const auto young = [&] { return compare(c, age(), 0x20) < 0; };
    const auto youth = [&] { return nonzero_logical(c, p.byte_5e()); };
    const std::int32_t now = current_role();
    if (!in_table(c, now)) return;
    switch (now) {
    case 1: scale(1.2); break;
    case 3: scale(0.9); break;
    case 4:
        scale(0.85);
        if (compare(c, age(), 0x17) > 0 && compare(c, age(), 0x20) < 0) scale(0.5);
        if (young() && youth()) scale(0.5);
        break;
    case 5: {
        const bool under_32 = young();
        scale(0.75);
        if (under_32 && youth()) scale(0.5);
        break;
    }
    case 6: {
        const bool under_32 = young();
        scale(0.7);
        if (under_32 && youth()) scale(0.25);
        break;
    }
    case 7: scale(0.01); break;
    default: break;
    }
}
}
