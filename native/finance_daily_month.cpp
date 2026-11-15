// FINANCE.CPP, block daily: the last day of a month (0x100fb024..0x100fcd1c of 0x100fa6e0). Every club
// that takes part pays and earns what the month brings, its ledger rolls over, and 0x10101400 is told.
#include "finance_daily.hpp"
#include "finance_detail.hpp"
#include <stdexcept>

namespace cm::aot::finance {

namespace {
constexpr std::uint32_t squad_places = 0x10688e80, find_contract = 0x10082f90, retired_or_loaned = 0x100d3150,
                        manager_dislikes = 0x100d9580, manager_likes = 0x100d9520, mark_for_sale = 0x1037fa80,
                        long_to_double = 0x10773c80, unscheduled_event = 0x10101400, wealth_of = 0x100f55b0;
constexpr std::uint32_t person_player = 0x14, person_age = 0x5d, person_skill = 0x63, person_trait_65 = 0x65,
                        person_trait_66 = 0x66, person_trait_67 = 0x67, contract_kind = 0x60;
constexpr std::uint32_t player_reputation = 0x0a;
constexpr std::uint32_t assistant_staff = 0x1c, staff_skill = 0x2f;
constexpr std::int32_t sold_places_cell = -0x7b94;
std::uint32_t extend16(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int16_t(value))); }
std::uint32_t extend8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
}

namespace daily {
void Pass::month_end() {
    Held<16> index(c, 0);
    Held<21> account(c, c.u32(manager + manager::accounts));
    Held<20> club_h(c, c.u32(c.toc(toc::clubs)));
    while (index.i32() < c.i32(c.r(22))) {
        const std::uint32_t club_at = club_h, account_at = account;
        if (inactive(club_at, 0x100fb040)) return;
        if (takes_part(club_at)) month_end_club(account_at, club_at);
        index = index + 1;
        account = account + account::size;
        club_h = club_h + club_size;
    }
}

void Pass::month_end_club(std::uint32_t account, std::uint32_t club_at) {
    // A club record that stands for another club is left to that club.
    const auto stands_for = invoke(c, callee::club_of_code, 0x100fb0a0, {club_at, frame.at(0x40), 1});
    if (stands_for != 0 && c.i8(frame.at(0x40)) == 0) return;
    if (std::int8_t(invoke_unit(c, wealth_of, 0x100fb0bc, {account})) == -2) sell_unwanted_players(club_at);
    ground_and_tier_costs(account, club_at);
    division_earnings(account, club_at);
    balance_interest(account, club_at);
    assistant_costs(account, club_at);
    gate_earnings(account, club_at);
    running_costs(account, club_at);
    invoke_unit(c, unscheduled_event, 0x100fcc10, {manager, club_at, 1});
    roll_ledger(account);
}

// A club in trouble (wealth -2) may put players up for sale: the chance grows with its reputation,
// and each squad player the manager can part with has a chance from a value that falls with his
// age, standing and the manager's feelings.
void Pass::sell_unwanted_players(std::uint32_t club_at) {
    Held<13> limit(c, std::uint32_t(reputation(club_at)));
    Held<15> chance(c);
    if (limit.i32() == 0) chance = 0;
    else draw_below<15>(limit.i32(), 0x100fb0fc);
    if (chance.i32() <= std::int32_t(draw_sample(0x100fb244)) % 20000) return;
    invoke(c, squad_places, 0x100fb294, {c.u32(c.toc(sold_places_cell)), club_at});
    Held<30> place(c, 0);
    Held<17> offset(c, 0);
    while (place.i32() < std::int32_t(club::squad_size)) {
        chance = c.u32(club_at + offset + club::squad);
        const std::uint32_t person = chance;
        if (person != 0 && c.u32(person + person_player) != 0) {
            Held<31> record(c, invoke(c, find_contract, 0x100fb2c8, {c.u32(c.r(23)), person, club_at}));
            if (std::uint32_t(record) != 0 && (c.u8(record + contract_kind) & 0xf) != 7 &&
                std::int8_t(invoke(c, retired_or_loaned, 0x100fb2ec, {person, club_at})) == 0) {
                consider_selling(person, record, club_at);
            }
        }
        place = place + 1;
        offset = offset + 4;
    }
}

// The value of selling a player (r19 of the original): 5 for a player the club should keep, 10-15 by
// age, less what the manager and the player's traits say; then the chance draw and, when it passes
// a draw of 20, the player is marked for sale (0x1037fa80).
void Pass::consider_selling(std::uint32_t person, std::uint32_t record, std::uint32_t club_at) {
    Held<19> value(c, 10);
    const auto age = std::int8_t(c.u8(person + person_age));
    if (age < 0x15) value = 0xf;
    else if (age < 0x18) value = 0xd;
    else if (age > 0x20) value = 0xb;
    else {
        const auto kind = c.u8(record + contract_kind) & 0xf;
        if (kind == 1 || kind == 2 || kind == 3) {
            const std::int32_t reputed = c.i16(c.u32(person + person_player) + player_reputation);
            if (reputed > 0xcb2 && (reputed < 0x128e || reputed < reputation(club_at))) value = 5;
        }
    }
    if (c.i8(person + person_skill) > 0xf) {
        value = extend16(value + (0x15u - extend8(c.u8(person + person_trait_65))));
        if (std::int8_t(invoke(c, manager_dislikes, 0x100fb3ac, {c.u32(club_at + club::manager), person})) != 0)
            value = extend16(value + (0x14u - extend8(c.u8(person + person_trait_67))));
    }
    if (std::int8_t(invoke(c, manager_likes, 0x100fb3d4, {c.u32(club_at + club::manager), person})) != 0) {
        const auto share = truncate(fpu.fmul(pool_f(0x38), exact(c.i8(person + person_trait_65))));
        value = extend16(value - std::uint32_t(share));
    }
    if (c.i16(c.u32(person + person_player) + player_reputation) > 0x1676 && c.i8(person + person_skill) < 0xa)
        value = value - 3;
    value = extend16(value - extend8(c.u8(person + person_trait_66)));
    Held<15> chance(c);
    if (value.i32() == 0) chance = 0;
    else draw_below<15>(value.i32(), 0x100fb47c);
    if (chance.i32() > std::int32_t(draw_sample(0x100fb5c4)) % 20)
        invoke(c, mark_for_sale, 0x100fb618, {c.r(31), 0x400000, 0});
}

// The ground's two flags cost 0x30d4 each (the second less for a small club), and the club's tier
// costs by steps; both are booked on the same line.
void Pass::ground_and_tier_costs(std::uint32_t account, std::uint32_t club_at) {
    if (const auto ground_at = c.u32(club_at + club::ground); ground_at != 0) {
        std::uint32_t cost = 0;
        if (c.i8(ground_at + 0x4c) != 0) cost = 0x30d4;
        if (c.i8(ground_at + 0x4d) != 0) cost += reputation(club_at) > 0x1194 ? 0x30d4 : 0x1d4c;
        if (cost != 0) book_expense(account, line::running_8c, cost);
    }
    const std::int32_t tier = c.i8(club_at + club::tier);
    std::int32_t cost;
    if (tier < 6) cost = tier * 0x190;
    else if (tier < 0x10) cost = (tier - 4) * 0x3e8 + 0x7d0;
    else if (tier >= 0x14 && reputation(club_at) >= 0x186a) cost = 40000;
    else cost = (tier - 0xf) * 0xfa0 + 0x2ee0;
    book_expense(account, line::running_8c, std::uint32_t(cost));
}

// A club whose division has no table entry earns from its attendance and reputation.
void Pass::division_earnings(std::uint32_t account, std::uint32_t club_at) {
    const auto table = c.u32(c.r(14));
    const auto number = c.u32(c.u32(club_at + club::division));
    if (c.u32(table + (number << 2)) != 0) return;
    const auto product = std::uint32_t(reputation(club_at)) * c.u32(club_at + club::attendance);
    const auto earned = truncate(fpu.frsp(fpu.fmul(pool_f(0x278), exact(std::int32_t(product)))));
    book_income(account, line::earnings_a0, earned);
}

// A positive balance earns interest, a negative one pays it (at a rate by how deep it is against
// the reputation): the balance goes through 0x10773c80, the 64-bit integer to double conversion.
void Pass::balance_interest(std::uint32_t account, std::uint32_t club_at) {
    const auto high = c.u32(account), low = c.u32(account + 4);
    const auto balance = std::int64_t(std::uint64_t(high) << 32 | low);
    const std::int64_t rep = reputation(club_at);
    if (0 < balance) {
        invoke(c, long_to_double, 0x100fb7f8, {high, low});
        book_income(account, line::interest, truncate(fpu.fmul(pool_f(0x280), c.f(1))));
        return;
    }
    std::uint32_t pool_at, back;
    if (-rep * 1000 < balance) { pool_at = 0x288; back = 0x100fb874; }
    else if (-rep * 2500 < balance) { pool_at = 0x290; back = 0x100fb8f0; }
    else { pool_at = 0x298; back = 0x100fb944; }
    invoke(c, long_to_double, back, {high, low});
    const auto owed = truncate(-fpu.fmul(pool_f(pool_at), c.f(1)));
    book_expense(account, line::overdraft, std::uint32_t(owed));
}

// The assistant's cost (a draw scaled by his skill, in f31) booked on expense_80. Without an assistant
// record two plain draws stand in for his skill.
void Pass::assistant_costs(std::uint32_t account, std::uint32_t club_at) {
    const auto assistant_at = c.u32(club_at + club::assistant);
    const auto staff_at = assistant_at != 0 ? c.u32(assistant_at + assistant_staff) : 0;
    Held<13> limit(c);
    Held<15> chance(c);
    double factor;
    if (assistant_at != 0 && staff_at != 0) {
        limit = (0x1fu - extend8(c.u8(staff_at + staff_skill))) * 10;
        if (limit.i32() == 0) chance = 0;
        else draw_below<15>(limit.i32(), 0x100fb9e8);
        const auto steps = rounded(chance.i32() + 1);
        const auto share = fpu.frsp(fpu.fdiv(steps, pool_f(0xf8)));
        factor = fpu.frsp(fpu.fadd(pool_f(0), share));
        const auto again = c.u32(c.u32(club_at + club::assistant) + assistant_staff);
        limit = extend8(c.u8(again + staff_skill)) + 1;
        c.set_f(31, factor);
        if (limit.i32() == 0) chance = 0;
        else draw_below<15>(limit.i32(), 0x100fbb88);
        if (chance.i32() == 0) factor = fpu.frsp(fpu.fmul(factor, pool_f(0x2a0)));
    } else {
        const auto first = std::int32_t(draw_sample(0x100fbce8));
        const bool wrapped = step_sample();  // the second draw's step; its wrap comes after the arithmetic
        const auto steps = rounded(first % 150 + 1);
        const auto share = fpu.frsp(fpu.fdiv(steps, pool_f(0xf8)));
        factor = fpu.frsp(fpu.fadd(pool_f(0), share));
        c.set_f(31, factor);
        if (wrapped) rewind_sample(0x100fbd74);
        if (std::int32_t(current_sample()) % 11 == 0) factor = fpu.frsp(fpu.fmul(factor, pool_f(0x2a0)));
    }
    c.set_f(31, factor);
    std::int32_t amount;
    if (c.u8(club_at + club::status) == 1) {
        amount = truncate(fpu.fmuls(rounded(reputation(club_at) + 0x7d), factor));
    } else {
        const auto scaled = fpu.fmuls(rounded(reputation(club_at) + 0x32), factor);
        amount = truncate(fpu.fmul(pool_f(0x60), scaled));
    }
    chance = 0;
    book_expense(account, ledger::expense_80, std::uint32_t(amount));
}

// What a draw scaled by the reputation earns on two lines: a4 always, b4 when the club's flag at
// +0x241 is set. The scale (f5) comes from the assistant's skill, or from a plain draw.
void Pass::gate_earnings(std::uint32_t account, std::uint32_t club_at) {
    const auto polynomial = [&](std::uint32_t site, std::int32_t rep, double scale) {
        const auto first = exact(rep);
        const auto second = exact(rep);
        const auto square = fpu.fmul(first, second);
        return truncate(fpu.fmul(scale, fpu.multiply_add(site, pool_f(0x1a0), square, pool_f(0x1f8))));
    };
    const auto assistant_at = c.u32(club_at + club::assistant);
    const auto staff_at = assistant_at != 0 ? c.u32(assistant_at + assistant_staff) : 0;
    Held<13> limit(c);
    double scale;
    if (assistant_at != 0 && staff_at != 0) {
        limit = extend8(c.u8(staff_at + staff_skill)) * 10;
        if (limit.i32() != 0) draw_below<15>(limit.i32(), 0x100fbec0);
        scale = fpu.frsp(fpu.fadd(pool_f(0), fpu.frsp(fpu.fmul(pool_f(0x170), rounded(std::int32_t(c.r(15)) + 1)))));
    } else {
        const auto sample = std::int32_t(draw_sample(0x100fc044));
        scale = fpu.frsp(fpu.fadd(pool_f(0), fpu.frsp(fpu.fmul(pool_f(0x170), rounded(sample % 100 + 1)))));
    }
    std::int32_t amount;
    if (c.u8(club_at + club::status) == 1) {
        if (reputation(club_at) >= 0x1388) amount = truncate(fpu.fmuls(rounded(reputation(club_at) + 0x32), scale));
        else amount = polynomial(0x100fc104, reputation(club_at), scale);
    } else {
        amount = polynomial(0x100fc18c, reputation(club_at), scale);
    }
    book_income(account, line::earnings_a4, amount);
    if (c.i8(club_at + club::flag_241) != 0) {
        const auto sample = std::int32_t(draw_sample(0x100fc200));
        scale = fpu.frsp(fpu.fadd(pool_f(0x70), fpu.frsp(fpu.fmul(pool_f(0x170), rounded(sample % 100 + 1)))));
        if (reputation(club_at) > 0x1388) amount = truncate(fpu.fmuls(rounded(reputation(club_at)), scale));
        else amount = polynomial(0x100fc2d4, reputation(club_at), scale);
        book_income(account, line::earnings_b4, amount);
    }
}

// The month's running cost: a base from the club's ground row (scaled by the account's class and
// the money in the row for a club of the first status), then a random share of it, booked on 0x8c.
void Pass::running_costs(std::uint32_t account, std::uint32_t club_at) {
    struct Step { std::int32_t above; std::uint32_t scale; };  // scale 0: none
    struct Ladder { Step steps[5]; std::uint32_t lowest; };
    static constexpr Ladder class_0{{{70000, 0x2a8}, {50000, 0x2b0}, {27500, 0x2b8}, {21000, 0x108}, {10000, 0}}, 0x228},
                            class_1{{{70000, 0x2b8}, {50000, 0x2c0}, {27500, 0x2d0}, {21000, 0x2d8}, {10000, 0x208}}, 0x2e0},
                            other{{{70000, 0x2b0}, {50000, 0x2b8}, {27500, 0x2c0}, {21000, 0xc0}, {10000, 0xd8}}, 0x2c8};
    const bool first_status = c.u8(club_at + club::status) == 1;
    const auto ground_at = c.u32(club_at + club::ground);
    const bool settled_row = c.i8(club_at + club::on_row) == 0 && ground_at != 0;
    Held<13> base(c);
    if (first_status) {
        if (settled_row) {
            base = std::uint32_t(truncate(fpu.fmul(pool_f(0), exact(c.i32(ground_at + 4)))));
        } else if (ground_at == 0) {
            base = 0x9c4;
        } else {
            const auto money = c.i32(ground_at + 4);
            const auto class_byte = c.i8(account + account_class);
            const Ladder& ladder = class_byte == 0 ? class_0 : class_byte == 1 ? class_1 : other;
            std::uint32_t scale = ladder.lowest;
            for (const auto& step : ladder.steps)
                if (money > step.above) { scale = step.scale; break; }
            base = std::uint32_t(truncate(scale == 0 ? exact(money) : fpu.fmul(pool_f(scale), exact(money))));
            // The nation of the club's own game country pays 0xd0 of it; then the reputation scales it.
            if (const auto nation_at = c.u32(club_at + club::nation);
                nation_at != 0 && c.u32(nation_at + 0x68) == c.u32(c.toc(-0x7af4)))
                base = std::uint32_t(truncate(fpu.fmul(pool_f(0xd0), exact(base.i32()))));
            const auto rep = reputation(club_at);
            // The original's last test also asks for status 1 again, which the branch has settled.
            if (rep > 0x1c52) base = std::uint32_t(truncate(fpu.fmul(pool_f(0x2e8), exact(base.i32()))));
            else if (rep > 0x1676) base = std::uint32_t(truncate(fpu.fmul(pool_f(0xa0), exact(base.i32()))));
            else if (rep > 0x109a) base = std::uint32_t(truncate(fpu.fmul(pool_f(0x1b0), exact(base.i32()))));
            else if (rep > 0x4e2) base = std::uint32_t(truncate(fpu.fmul(pool_f(0xb8), exact(base.i32()))));
        }
    } else {
        if (settled_row) {
            base = c.u32(ground_at + 4);
        } else if (ground_at == 0) {
            base = 0x3e8;
        } else {
            const auto scale = c.u8(account + account_class) == 1 ? 0x2f0u : 0x38u;
            base = std::uint32_t(truncate(fpu.fmul(pool_f(scale), exact(c.i32(ground_at + 4)))));
        }
        // The original's test of status 1 here is settled the other way by the branch.
        if (base.i32() > 0x2ee)
            base = 0x2eeu + std::uint32_t(truncate(fpu.fmul(pool_f(0xe0), exact(base.i32() - 0x2ee))));
    }
    const auto sample = std::int32_t(draw_sample(first_status ? 0x100fc954 : 0x100fcb38));
    const auto whole = rounded(base.i32());
    const auto step = rounded(sample % 25 + 1);
    const auto share = fpu.frsp(fpu.fmul(pool_f(0x170), step));
    const auto factor = fpu.frsp(fpu.fadd(pool_f(first_status ? 0x70 : 0x128), share));
    book_expense(account, line::running_8c, std::uint32_t(truncate(fpu.fmuls(whole, factor))));
}

// The month's lines (0x70..0xbc) move to the previous month's place (0x20..0x6c) and clear.
void Pass::roll_ledger(std::uint32_t account) {
    for (std::uint32_t at = 0x70; at <= 0xbc; at += 4) c.put32(account + at - 0x50, c.u32(account + at));
    for (std::uint32_t at = 0x70; at <= 0xbc; at += 4) c.put32(account + at, 0);
}
}
}
