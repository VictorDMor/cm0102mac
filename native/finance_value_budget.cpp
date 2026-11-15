// FINANCE.CPP, block value: a club's budget (0x100f2680).
//
// The budget is worked out in two steps. An "ask" starts from the account's balance and then from
// the club's contracts (the club asking for itself, `outside` null) or from the price of the deal
// being weighed (`outside` set), and is scaled by the club's wealth band and league. A "price"
// then follows from the account's figures and the people on both sides of the deal, is scaled by
// the spending kind, and the budget is chosen between the ask and the price within a base figure.
//
// The original keeps these values in registers that its callees save in their frames; `hold` puts
// those registers to what the original holds before each call (r18 the capped rating and later the
// price, r22 the balance figure and later the ask, r23 the rating, later a table bucket, later the
// floor, f30 and f31 floating figures).
#include "finance_value.hpp"

namespace cm::aot::finance {
using guest::Frame;
using namespace value_detail;

namespace {
namespace callee_of_budget {
constexpr std::uint32_t balance_as_double = 0x10773c80, contract_of = 0x10082f90, deal_price = 0x10084b60,
                        human_managed = 0x1014c2e0, owner_trusts = 0x100ca800, days_between = 0x10718130,
                        deal_for_owner = 0x100d9520, deal_for_club = 0x100d9480;
}
namespace deal {  // the deal being weighed (`outside`)
constexpr std::uint32_t other_club = 0x14, buyer = 0x1c, seller = 0x24, level = 0x5c, flag = 0x5e, kind_of_deal = 0x62;
}
constexpr std::int32_t table_a = -0x55a8, table_b = -0x55ac, table_c = -0x55b0, table_d = -0x55b4,
                       table_e = -0x55b8, table_f = -0x55bc;

class Budget {
public:
    Budget(Cpu& cpu, Frame& f, std::uint32_t account_, std::uint32_t outside_, std::uint32_t kind_)
        : c(cpu), frame(f), n(cpu), account(account_), outside(outside_), kind(kind_) {}
    std::uint32_t run();

private:
    Cpu& c;
    Frame& frame;
    Numbers n;
    const std::uint32_t account, outside;
    std::uint32_t kind;  // the spending kind (a word: 0xff takes the negotiator's)
    std::uint32_t club = 0, record = 0, table = 0, band = 0, game_cell = 0, table_d_address = 0;
    std::int32_t base = 100, rating = 0, capped = 0, stipend = 0, ask = 0, price = 0, floor = 0;
    double ratio = 0;

    std::int8_t kind8() const { return std::int8_t(kind); }
    std::int8_t band8() const { return std::int8_t(band); }
    std::int32_t reputation() const { return c.i16(club + club::reputation); }
    std::uint32_t league_id(std::uint32_t league) const { return c.u32(league + 0x68); }
    bool any(std::uint32_t id, std::initializer_list<std::int32_t> slots) { return is_any_competition(c, id, slots); }
    bool is(std::uint32_t id, std::int32_t slot) { return is_competition(c, id, slot); }
    std::uint32_t at_frame(std::uint32_t offset) const { return frame.at(offset); }

    void hold(std::int32_t r18, std::int32_t r22, std::int32_t r23) {
        c.set({{17, kind}, {18, std::uint32_t(r18)}, {19, record}, {20, std::uint32_t(base)}, {22, std::uint32_t(r22)},
               {23, std::uint32_t(r23)}, {24, table}, {31, band}});
        c.set_f(31, ratio);
    }
    // Whether a human manages the club (0x1014c2e0, native/club_control.hpp): the flag is its second argument.
    bool rule(std::uint32_t back, std::uint32_t flag, std::int32_t r18, std::int32_t r22, std::int32_t r23) {
        hold(r18, r22, r23);
        return std::int8_t(invoke(c, callee_of_budget::human_managed, back, {club, flag, 1, 0, 0})) != 0;
    }
    std::int32_t band_of(std::uint32_t back, std::int32_t r18, std::int32_t r22, std::int32_t r23) {
        hold(r18, r22, r23);
        return std::int8_t(invoke_unit(c, 0x100f55b0, back, {account}));
    }
    std::int32_t price_of_deal_for(std::uint32_t back, std::int32_t r18, std::int32_t r22, std::int32_t r23) {
        hold(r18, r22, r23);
        return std::int32_t(invoke(c, callee_of_budget::deal_price, back, {c.u32(game_cell), outside}));
    }
    // A conversion the original makes through the stack and the single-precision subtraction.
    std::int32_t scale_price(std::uint32_t factor) { return price = n.scale_single(price, factor); }

    void choose_table();
    void adjust_rating();
    double league_factor(std::uint32_t league, std::int32_t strength);
    void ask_of_deal();
    void ask_of_club();
    void ask_from_contracts();
    void scale_ask_by_band(std::int32_t bucket);
    void floor_of_club();
    void price_of_club();
    void price_of_deal();
    std::int32_t rival_ceiling(std::uint32_t other);
    void price_by_people();
    void price_by_owner(std::uint32_t other);
    void price_by_kind();
    void final_price();
};

// Which of the unit's rank tables the club's league uses.
void Budget::choose_table() {
    const auto league = c.u32(club + 0x2c);
    if (league == 0) { table = c.toc(table_a); return; }
    const auto id = league_id(league);
    if (any(id, {-0x757c, -0x7574, -0x7578, -0x7650})) {
        table = c.toc(table_a);
    } else if (any(id, {-0x7a24, -0x7514, -0x7590, -0x75a8, -0x7504, -0x7a20, -0x79fc, -0x750c, -0x7594, -0x7a1c, cell_d})) {
        table = c.toc(table_b);
    } else if (is(id, cell_e)) {
        table = c.toc(table_c);
    } else if (c.i8(club + 0x241) != 0) {
        table = (band8() == 2 || band8() == 1) ? c.toc(table_d) : c.toc(table_c);
    } else {
        table = c.toc(table_d);
    }
}

// The rating (reputation / 50) moves up with the wealth band.
void Budget::adjust_rating() {
    const auto rep = reputation();
    const bool uses_table_d = table == table_d_address;
    switch (band8()) {
        case 1:
            if (rep <= 0x1482) rating += 5;
            else if (uses_table_d) rating = std::int16_t((rep - 0x1482) / 25 + 0x69);
            else rating += 10;
            break;
        case 2:
            if (rep <= 0x128e) rating += 10;
            else if (uses_table_d) rating = std::int16_t((rep - 0x128e) / 25 + 0x5f);
            else rating += 15;
            break;
        case 0:
            if (rep > 0x1676) {
                if (uses_table_d) rating = std::int16_t((rep - 0x186a) / 25 + 0x73);
                else rating += 5;
            }
            break;
        default: break;
    }
}

// The factor from the league and the club's strength (0x100f2bec..0x100f2ecc): each family of leagues starts
// from its own pool figure and, where the rating is above ten times the league's strength, ramps it with
// the ratio of the two; the result never exceeds 1.
double Budget::league_factor(std::uint32_t league, std::int32_t strength) {
    const auto id = league_id(league);
    const std::int32_t step = std::int16_t(rating);
    const double one = n.constant(pool_one);
    bool ramped = false;
    const auto ramp = [&](std::uint32_t start) {
        auto factor = n.constant(start);
        if (step > strength * 10) {
            const auto numerator = n.widen(step);
            const auto denominator = n.widen(strength < 1 ? 1 : strength);
            const auto quotient = n.fpu.fdiv(numerator, denominator);
            factor = n.fpu.fmul(factor, n.fpu.fdiv(quotient, n.constant(0x80)));
            ramped = true;
        }
        return factor;
    };
    double factor;
    if (any(id, {cell_c, cell_b, cell_a, -0x7a24})) {
        factor = one;
    } else if (is(id, -0x7a20)) {
        factor = n.constant(0x70);
    } else if (any(id, {cell_d, cell_e, -0x7a1c, -0x7a28})) {
        factor = ramp(0x78);
    } else if (any(id, {-0x7b68, -0x7508, -0x7514, -0x7510, -0x79fc})) {
        factor = ramp(0x88);
    } else if (any(id, {-0x750c, -0x7504, -0x7594})) {
        factor = ramp(0x90);
        if (ramped) {
            n.fpu.fcmp(0, factor, n.constant(0x70));
            if (factor > n.constant(0x70)) factor = n.constant(0x70);
        }
    } else {
        factor = ramp(0x98);
    }
    n.fpu.fcmp(0, factor, one);
    if (factor > one) factor = one;
    return factor;
}

// The club asking for itself: the ratio of its league and the base figure from its two best-rated staff
// (0x100f2b6c..0x100f2fc4), then the ask from the contracts.
void Budget::ask_of_club() {
    if (const auto league = c.u32(club + 0x2c); league != 0) {
        const auto level = level_of(c, league);
        const std::int32_t strength = c.i8(league + 0x11f);
        const auto numerator = n.widen(strength);
        const auto denominator = n.widen(level * 20);
        ratio = n.fpu.fdiv(numerator, denominator);
        c.set_f(31, ratio);
        if (level == 1) {
            ratio = n.fpu.fmul(ratio, league_factor(league, strength));
            c.set_f(31, ratio);
        }
        if (const auto first = c.u32(club + 0x30); first != 0) {
            const auto second = c.u32(club + 0x34);
            const std::int32_t low = c.i16(first + 0x18);
            const auto cube = [](std::int32_t x) { return std::int32_t(std::uint32_t(x) * std::uint32_t(x) * std::uint32_t(x)); };
            if (second != 0 && std::int32_t(c.i16(second + 0x18)) > low) {
                const std::int32_t high = c.i16(second + 0x18);
                const auto factor = n.constant(0xa0);
                auto lower = n.fpu.fsub(magic_word(cube(low)), n.constant(pool_magic));
                const auto upper_raw = magic_word(cube(high));
                lower = n.fpu.fmul(factor, lower);
                auto upper = n.fpu.fsub(upper_raw, n.constant(pool_magic));
                lower = n.fpu.fmul(ratio, lower);
                upper = n.fpu.fmul(factor, upper);
                const auto first_bits = n.fpu.fctiwz(0, lower);
                upper = n.fpu.fmul(ratio, upper);
                const auto second_bits = n.fpu.fctiwz(0, upper);
                base = std::int32_t(std::uint32_t(first_bits) + std::uint32_t(second_bits));
            } else {
                base = n.truncate(n.fpu.fmul(ratio, n.fpu.fmul(n.constant(0xa8), n.widen(cube(low)))));
            }
            if (base < 0xc8) base = 0xc8;
        }
    }
    ask_from_contracts();
}

// 0x100f2fc8..0x100f3180: the ask from the balance figure, the rating, the ratio and the rank tables; the run
// at 0x100f3000 and the words it leaves in the frame included.
void Budget::ask_from_contracts() {
    const std::int32_t step = std::int16_t(rating);
    const auto quotient = divide(0xc8 - step, 10);
    const std::int32_t divisor = quotient >= 1 ? quotient : 1;
    const auto flipped = std::uint32_t(step) ^ 0x80000000u;
    for (const auto offset : {0x64u, 0x5cu, 0x54u}) c.put32(at_frame(offset), flipped);
    for (const auto offset : {0x60u, 0x58u, 0x50u, 0x68u}) c.put32(at_frame(offset), 0x43300000);
    c.put32(at_frame(0x6c), std::uint32_t(divisor) ^ 0x80000000u);
    const auto weighted = n.fpu.fmul(ratio, n.widen(step));
    auto product = n.fpu.fmul(n.widen(step), weighted);
    product = n.fpu.fmul(n.widen(step), product);
    const auto bits = n.fpu.fctiwz(0, n.fpu.fdiv(product, n.widen(divisor)));
    c.put64(at_frame(0x48), bits);
    ask = std::int32_t(std::uint32_t(stipend) + std::uint32_t(bits));
    ask = divide(std::int32_t(0u - std::uint32_t(ask)), 52);
    ask = std::int32_t(std::uint32_t(ask) + c.u32(account + 0x0c));

    // The rank tables hold a 64-bit figure per ten points of rating; the ask is set against the table's.
    const std::int32_t bucket = divide(step, 10) * 8;
    const bool first_table = table == table_d_address;
    const auto lead = c.toc(first_table ? table_e : table_f);
    const std::uint32_t back_first = first_table ? 0x100f30c8 : 0x100f313c, back_second = first_table ? 0x100f30e8 : 0x100f315c;
    hold(capped, ask, bucket);
    invoke(c, callee_of_budget::balance_as_double, back_first, {c.u32(lead + bucket), c.u32(lead + bucket + 4)});
    const auto reached = n.fpu.fmul(ratio, c.f(1));
    c.set_f(30, reached);
    const auto row = table + std::uint32_t(bucket);
    const auto low = c.u32(row + 4), high = c.u32(row);
    hold(capped, ask, bucket);
    invoke(c, callee_of_budget::balance_as_double, back_second,
           {high - std::uint32_t(low < std::uint32_t(ask)), low - std::uint32_t(ask)});
    auto needed = n.fpu.fmul(ratio, c.f(1));
    n.fpu.fcmp(0, needed, reached);
    if (!(needed > reached)) needed = reached;
    const auto bits_after = n.fpu.fctiwz(0, needed);
    c.put64(at_frame(0x48), bits_after);
    ask = std::int32_t(std::uint32_t(bits_after));

    hold(capped, ask, bucket);
    if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f3184, {club})) == 0) scale_ask_by_band(bucket);
}

// A club that is not inactive: the wealth band and the league scale the ask (0x100f318c..0x100f332c).
void Budget::scale_ask_by_band(std::int32_t bucket) {
    const auto wealth = band_of(0x100f3194, capped, ask, bucket);
    if (wealth == 2) ask = n.scale_single(ask, 0xb0);
    else if (wealth == 1) ask = n.scale_single(ask, 0xb8);
    const auto league = c.u32(club + 0x2c);
    if (league == 0) return;
    const auto id = league_id(league);
    if (any(id, {cell_b, cell_a})) {
        if (reputation() > 0x1e46) ask = n.scale_single(ask, 0xb8);
    } else if (is(id, cell_c)) {
        if (reputation() > 0x1e46) ask = n.scale_single(ask, 0xc0);
    }
    if (c.i8(club + 0x241) == 0 && reputation() > 0x1e46 && any(league_id(league), {cell_b, cell_a}))
        ask = n.scale_single(ask, 0xb8);
}

// The deal's ask: what the deal costs the club (when the club sells), kept between the account's budget and
// twice that, and held down by the kind of deal (0x100f2a6c..0x100f2b68).
void Budget::ask_of_deal() {
    std::int32_t value = 0;
    if (c.u32(outside + deal::seller) == club) {
        value = price_of_deal_for(0x100f2a88, capped, stipend, rating);
        const auto doubled = std::int32_t(c.u32(account + 0x14) << 1);
        if (value > doubled) value = doubled;
    }
    if (c.i32(account + 0x14) > value) value = c.i32(account + 0x14);
    ask = value;
    if (c.u32(outside + deal::buyer) != 0 && c.u32(outside + deal::other_club) == 0) {
        const auto ceiling = kind_ceiling(c.i8(outside + deal::kind_of_deal));
        if (value > ceiling) ask = ceiling;
    }
}

// The floor of the price (the original's r23 from 0x100f3334): only for the top leagues' clubs of high
// reputation and the better wealth bands.
void Budget::floor_of_club() {
    floor = 0;
    const auto league = c.u32(club + 0x2c);
    if (league == 0 || !any(league_id(league), {cell_c, cell_b, cell_a}) || reputation() <= 0x128e) return;
    if (!(std::uint8_t(std::uint32_t(band) - 1) <= 1 || band8() == 0)) return;
    const std::int32_t offset = c.i8(club + 0x241) != 0 ? 0x4e2 : 0x2ee;
    const std::int32_t from = std::int16_t(capped);
    std::int32_t factor = 6;
    if (from >= 0x1676) factor = band8() == 0 ? 6 : band8() == 1 ? 8 : 10;
    floor = (from - offset) * factor;
}

// The club asking for itself: the price from the account's figures (0x100f38f0..0x100f3b5c).
void Budget::price_of_club() {
    const std::int32_t from = std::int16_t(capped);
    const auto largest = c.i32(account + 0x10);
    // The run at 0x100f38f0 and the words it leaves in the frame.
    const auto flipped = std::uint32_t(from) ^ 0x80000000u;
    c.put32(at_frame(0x54), flipped);
    c.put32(at_frame(0x4c), flipped);
    c.put32(at_frame(0x44), std::uint32_t(largest) ^ 0x80000000u);
    for (const auto offset : {0x50u, 0x48u, 0x40u}) c.put32(at_frame(offset), 0x43300000);
    {
        auto weighted = n.fpu.fmul(ratio, n.widen(from));
        weighted = n.fpu.fmul(n.widen(from), weighted);
        const auto share = n.fpu.fdiv(weighted, n.constant(0xc8));
        const auto bits = n.fpu.fctiwz(0, n.fpu.fadd(n.widen(largest), share));
        c.put64(at_frame(0x58), bits);
        price = std::int32_t(std::uint32_t(bits));
    }
    if (price < floor) {
        c.put32(at_frame(0x6c), std::uint32_t(floor) ^ 0x80000000u);
        c.put32(at_frame(0x68), 0x43300000);
        c.put32(at_frame(0x64), std::uint32_t(price) ^ 0x80000000u);
        c.put32(at_frame(0x60), 0x43300000);
        const auto toward = n.fpu.fmul(n.constant(0x60), n.widen(floor));
        const auto above = n.widen(price);
        const auto bits = n.fpu.fctiwz(0, n.fpu.multiply_add(0x100f3994, n.constant(0x70), above, toward));
        c.put64(at_frame(0x38), bits);
        price = std::int32_t(std::uint32_t(bits));
    }
    hold(price, ask, floor);
    if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f39ac, {club})) != 0) return;
    const auto league = c.u32(club + 0x2c);
    if (league == 0) return;
    const auto id = league_id(league);
    if (is(id, cell_d) || is(id, cell_e)) {
        const auto wealth = band_of(0x100f39e4, price, ask, floor);
        if (wealth == 2) scale_price(0xb0);
        else if (wealth == 1) scale_price(0xb8);
    } else if (any(id, {cell_b, cell_c, cell_a})) {
        const auto wealth = band_of(0x100f3a94, price, ask, floor);
        if (wealth == 1) scale_price(0xb8);
        else if (wealth == 0) { if (reputation() > 0x109a) scale_price(0xc0); }
        else if (wealth == 2) scale_price(0xb0);
    }
}

// The ceiling the other club's standing puts on the ask (0x100f36d8..0x100f38ac).
std::int32_t Budget::rival_ceiling(std::uint32_t other) {
    const std::int32_t a = c.i16(other + 0xa), b = c.i16(other + 4), d = c.i16(other + 0xc);
    const std::int32_t deal_level = std::int8_t(c.i16(outside + deal::level));
    const bool flagged = c.u8(outside + deal::flag) != 0;
    const auto k = kind8();
    // `ordinary` (0 < kind < 4) already excludes 7: the listing's test of 7 at 0x100f37bc (and 0x100f3828) is
    // reached only with kind < 4, so its equal branch is dead and has no counterpart here.
    const bool ordinary = k < 4 && k != 0;
    if (a < 0xea6) {
        if (b < 0x3c) return deal_level > 0x17 ? 0x1d4c : 0x1388;
        if (b < 0x64) return (ordinary || deal_level > 0x17) ? 15000 : 10000;
        return 25000;
    }
    if (a < 0x1482) return b < 0x64 ? 25000 : 30000;
    if (a < 0x1c52) {
        if (deal_level > 0x22 && b < 0x78) return ordinary ? 0x927c : 0x7ef4;
        if (b < 0x8c && d < 0xea6) return flagged ? 45000 : 40000;
        if (!ordinary) return 45000;
        if (flagged) return 0xe09c;
        return b < 0x8c ? 65000 : 80000;
    }
    if (d < 0x1a5e) return flagged ? 0x1e848 : 100000;
    return 0x2ab98;
}

// The deal's price: from the account's figures and the deal's cost (0x100f3434..0x100f3b5c).
void Budget::price_of_deal() {
    const auto first = price_of_deal_for(0x100f3448, capped, ask, floor);
    const std::int32_t rep = reputation();
    const auto largest = c.i32(account + 0x10);
    // The run at 0x100f3448 and the words it leaves in the frame.
    const auto rep_flipped = std::uint32_t(rep) ^ 0x80000000u;
    c.put32(at_frame(0x64), rep_flipped);
    c.put32(at_frame(0x5c), rep_flipped);
    c.put32(at_frame(0x54), std::uint32_t(largest) ^ 0x80000000u);
    c.put32(at_frame(0x4c), std::uint32_t(first) ^ 0x80000000u);
    for (const auto offset : {0x60u, 0x58u, 0x50u, 0x48u}) c.put32(at_frame(offset), 0x43300000);
    auto weighted = n.fpu.fmul(ratio, n.widen(rep));
    weighted = n.fpu.fmul(n.widen(rep), weighted);
    const auto largest_f = n.widen(largest);
    const auto share = n.fpu.fdiv(weighted, n.constant(0xc8));
    const auto first_f = n.widen(first);
    auto figure = n.fpu.fadd(largest_f, share);
    n.fpu.fcmp(0, figure, first_f);
    if (!(figure > first_f)) {
        const auto second = price_of_deal_for(0x100f34d0, capped, ask, floor);
        c.put32(at_frame(0x6c), std::uint32_t(second) ^ 0x80000000u);
        c.put32(at_frame(0x68), 0x43300000);
        figure = n.widen(second);
    }
    {
        const auto bits = n.fpu.fctiwz(0, figure);
        c.put64(at_frame(0x40), bits);
        price = std::int32_t(std::uint32_t(bits));
    }

    // The ask is raised to the floor, or dragged to the other club's own limit, by the kind of deal.
    if (const auto other = c.u32(outside + deal::other_club);
        other != 0 && c.i16(other + 0xa) > 0x1482 &&
        (std::uint8_t(kind - 2) <= 1 || kind8() == 1) && std::int8_t(c.i16(outside + deal::level)) < 0x20) {
        const auto k = kind8();
        bool settled = false;
        if (k == 3) {
            const auto limit = n.fpu.fmul(n.constant(0xd0), n.widen(floor));
            c.set_f(30, limit);
            const auto asked = n.widen(ask);
            n.fpu.fcmp(0, asked, limit);
            if (asked < limit) {
                if (!rule(0x100f35a0, 0, price, ask, floor) && c.u32(outside + deal::other_club) != 0 &&
                    c.i16(c.u32(outside + deal::other_club) + 0xa) > 0x1c52)
                    ask = n.truncate(n.fpu.fmul(n.constant(0xd8), n.widen(floor)));
                else
                    ask = n.truncate(limit);
                settled = true;
            }
        }
        if (!settled && k == 2) {
            const auto limit = n.fpu.fmul(n.constant(0xd8), n.widen(floor));
            c.set_f(30, limit);
            const auto asked = n.widen(ask);
            n.fpu.fcmp(0, asked, limit);
            if (asked < limit) {
                if (!rule(0x100f3668, 0, price, ask, floor) && c.u32(outside + deal::other_club) != 0 &&
                    c.i16(c.u32(outside + deal::other_club) + 0xa) > 0x1e46)
                    ask = floor;
                else
                    ask = n.truncate(limit);
                settled = true;
            }
        }
        if (!settled && k == 1 && ask < floor) ask = floor;
    }

    if (rule(0x100f36d0, 0, price, ask, floor)) {
        if (const auto other = c.u32(outside + deal::other_club); other != 0) {
            const auto ceiling = rival_ceiling(other);
            if (ceiling < ask) ask = ceiling + n.scale_single(ask - ceiling, 0xe0);
        }
    }
}

// The price from the people on both sides, for a deal with another club (0x100f3b70..0x100f4254).
void Budget::price_by_owner(std::uint32_t other) {
    const auto owner = c.u32(club + 0x80);
    if (owner == 0 || c.u32(owner + 0x1c) == 0 || !(reputation() < std::int32_t(c.i16(other + 0xa)) + 0x4e2)) return;
    {
        const auto strength = n.fpu.fdiv(n.widen(c.i8(owner + 0x63)), n.constant(0xf8));
        const auto widened = n.widen(price);
        price = n.truncate(n.fpu.fmul(widened, n.fpu.fadd(n.constant(0xd8), strength)));
    }
    hold(price, ask, floor);
    bool pleased = std::int8_t(invoke(c, callee_of_budget::deal_for_owner, 0x100f400c, {outside, owner})) != 0;
    if (!pleased) {
        hold(price, ask, floor);
        pleased = std::int8_t(invoke(c, callee_of_budget::deal_for_club, 0x100f4020, {outside, club})) != 0;
    }
    if (pleased) {
        const auto strength = n.fpu.fdiv(n.widen(c.i8(c.u32(club + 0x80) + 0x63)), n.constant(0xf8));
        const auto widened = n.widen(price);
        price = n.truncate(n.fpu.fmul(widened, n.fpu.fadd(n.constant(pool_one), strength)));
    }
    const auto owner_now = c.u32(club + 0x80);
    const auto profile = c.u32(owner_now + 0x1c);
    const std::int32_t profile_level = c.i8(profile + 0x2f);
    if (profile_level < 6) {
        const auto manager = c.u32(club + 0x90);
        if (manager != 0 && c.i8(manager + 0x63) > 0xf && c.u32(manager + 0x1c) != 0 &&
            c.i16(c.u32(manager + 0x1c) + 0x2a) > 0x1c52)
            scale_price(0xb0);
        else
            scale_price(0xb8);
    } else if (profile_level > 0xf) {
        scale_price(0x100);
    }
    if (c.i8(club + 0x241) == 0 && c.i8(profile + 0x40) > 0xf && c.i8(owner_now + 0x63) > 0xf) {
        const auto rep = reputation();
        if (rep < 0xabe || c.i16(c.u32(outside + deal::other_club) + 0xa) < 0x1c52) scale_price(0xb8);
        else if (rep < 0x1676) scale_price(0xb0);
        else scale_price(0x108);
    }
}

void Budget::price_by_people() {
    const auto other = c.u32(outside + deal::other_club);
    if (const auto league = c.u32(club + 0x2c);
        league != 0 && any(league_id(league), {cell_b, cell_c, cell_d, cell_e, cell_a})) {
        hold(price, ask, floor);
        if (std::int8_t(invoke(c, callee_of_budget::owner_trusts, 0x100f3bc4, {club})) != 0) scale_price(0xb8);
        if (const auto director = c.u32(c.toc(-0x7b40)); director != 0) {
            const auto rep = reputation();
            const std::int32_t rated = rep < 1000 ? 1000 : rep;
            const auto row = c.u32(c.u32(director + 0x988) + 0xdc) + c.u32(club) * 12;
            const std::int32_t score = c.i16(row + 6);
            const auto quotient = n.fpu.fdiv(n.widen(score), n.widen(rated));
            auto factor = n.constant(pool_one);
            n.fpu.fcmp(0, factor, quotient);
            if (!(factor > quotient)) {
                factor = n.constant(0xe8);
                n.fpu.fcmp(0, factor, quotient);
                if (!(factor < quotient)) factor = quotient;
            }
            price = n.truncate(n.fpu.fmul(n.widen(price), factor));
        }
        const auto close = [&](std::uint32_t back) {
            if (record == 0 || c.u32(outside + deal::seller) != club) return false;
            hold(price, ask, floor);
            const auto today = c.toc(toc::today);
            return std::int32_t(invoke(c, callee_of_budget::days_between, back, {record + 0x30, c.u32(today), c.u32(today + 4)})) < 200;
        };
        const auto rep = reputation();
        if (rep > 0x186a) {
            if (close(0x100f3cec)) {
                if (c.i16(other + 0xc) > 0x1482) {
                    scale_price(0xe8);
                } else if (const std::int32_t a = c.i16(other + 0xa); a > 0x1c52) {
                    scale_price(0xb0);
                } else {
                    const auto v = c.u8(record + 0x60) & 0xf;
                    if ((v == 3 && a > 0x1482) || v == 1 || v == 2) scale_price(0xb8);
                }
            } else if (c.i16(other + 0xc) > 0x1482) {
                scale_price(0xb0);
            } else if (c.i16(other + 0xa) > 0x1a5e) {
                scale_price(0xb8);
            }
        } else if (rep > 0x109a) {
            if (close(0x100f3e90)) {
                const std::int32_t a = c.i16(other + 0xa);
                if (a > 0x1a5e) {
                    scale_price(0xb8);
                } else {
                    const auto v = c.u8(record + 0x60) & 0xf;
                    if ((v == 3 && a > 0x1482) || v == 1 || v == 2) scale_price(0xf0);
                }
            } else if (c.i16(other + 0xa) > 0x1e46) {
                scale_price(0xf0);
            }
        }
    }
    price_by_owner(other);
    price_by_kind();
}

// The kind's own shaping of the price (0x100f4258..0x100f463c): a ramp above a knee and a ceiling.
void Budget::price_by_kind() {
    const auto k = kind8();
    if (k <= -1) return;
    const auto other_rating = [&] { return std::int32_t(c.i16(c.u32(outside + deal::other_club) + 0xa)); };
    switch (k) {
        case 1:
            price = n.scale_single(price, 0xa0);
            break;
        case 2:
            if (!rule(0x100f42e4, 1, price, ask, floor) && other_rating() > 0x1a5e) {
                price = n.scale_single(price, 0xa0);
                break;
            }
            price = n.truncate(n.widen_single(price));
            if (!rule(0x100f4370, 1, price, ask, floor) && (c.u32(outside + deal::other_club) == 0 ||
                                                             c.i16(c.u32(outside + deal::other_club) + 0xa) >= 0x1e46)) {
                break;
            }
            if (price > 85000)
                price = n.truncate(n.fpu.multiply_add(0x100f43cc, n.constant(0x118), n.widen_single(price - 85000), n.constant(0x110)));
            break;
        case 3:
            if (!rule(0x100f43f8, 1, price, ask, floor) && other_rating() > 0x1a5e) {
                price = n.scale_single(price, 0xa0);
                break;
            }
            price = n.scale_single(price, 0x100);
            if (price > 55000) price = 55000;
            break;
        case 4:
            if (!rule(0x100f44a4, 1, price, ask, floor) && other_rating() > 0x1a5e) price = n.scale_single(price, 0x100);
            else price = n.scale_single(price, 0x70);
            if (price > 0x927c) price = 0x927c;
            break;
        case 5:
            if (price > 0xc80)
                price = n.truncate(n.fpu.multiply_add(0x100f4568, n.constant(0x98), n.widen_single(price - 0xc80), n.constant(0x120)));
            else
                price = n.scale_single(price, 0x128);
            if (price > 0x30d4) price = 0x30d4;
            break;
        case 6:
            if (price > 0xc80)
                price = n.truncate(n.fpu.multiply_add(0x100f45ec, n.constant(0x138), n.widen_single(price - 0xc80), n.constant(0x130)));
            else
                price = n.scale_single(price, 0x38);
            if (price > 0x203a) price = 0x203a;
            break;
        default: break;
    }
}

// The club's type adjusts the price, the base figure bounds it, and the budget is picked (0x100f463c..).
void Budget::final_price() {
    const auto type = c.i8(club + 0x23d);
    const auto rep = reputation();
    const auto floats = c.toc(toc::floats);
    if (type == 1) {
        if (price < 0xfa) price = 0xfa;
        if (rep < 0x8ca && price > 0x834) {
            const auto widened = n.widen_single(price);
            const auto knee = n.fpu.fsubs(widened, double(c.f32(floats + 4)));
            price = n.truncate(n.fpu.multiply_add(0x100f469c, n.constant(0x118), knee, n.constant(0x140)));
        }
    } else if (type == 2) {
        price = n.scale(price, 0x70);
        if (rep < 0xabe && price > 0x4b0) {
            const auto widened = n.widen_single(price);
            const auto knee = n.fpu.fsubs(widened, double(c.f32(floats + 8)));
            price = n.truncate(n.fpu.multiply_add(0x100f4724, n.constant(0x118), knee, n.constant(0x148)));
        }
        if (price < 0x96) price = 0x96;
    }
    // The floor is base + 100, wrapping in a word as the original's add does (base can be a saturated conversion).
    const auto floor_price = std::int32_t(std::uint32_t(base) + 100u);
    if (price < floor_price) price = floor_price;
}

std::uint32_t Budget::run() {
    game_cell = c.toc(-0x7b7c);
    table_d_address = c.toc(table_d);
    c.set({{13, c.toc(cell_d)}, {14, c.toc(cell_e)}, {15, account}, {16, outside}, {17, kind}, {18, c.toc(table_c)},
           {25, game_cell}, {26, table_d_address}, {27, c.toc(cell_a)}, {28, c.toc(cell_b)}, {29, c.toc(cell_c)},
           {30, n.pool}});
    invoke(c, callee_of_budget::balance_as_double, 0x100f26d4, {c.u32(account), c.u32(account + 4)});
    const auto scaled_balance = n.fpu.fmul(n.constant(0x58), c.f(1));
    ratio = n.constant(0x60);
    club = club_record(c, c.u32(account + account::club));
    stipend = n.truncate(scaled_balance);
    if (c.u8(club + 0x23d) == 3) return 0;
    rating = std::int16_t(c.i16(club + club::reputation) / 50);

    c.set({{19, 0}, {20, 100}, {21, club}, {22, std::uint32_t(stipend)}, {23, std::uint32_t(rating)}});
    c.set_f(31, ratio);
    if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f2744, {club})) != 0) {
        band = 0;
    } else {
        band = invoke_unit(c, 0x100f55b0, 0x100f275c, {account});
    }
    choose_table();
    adjust_rating();

    // The run at 0x100f29c0: the rating capped for the price, and its words in the frame.
    {
        const std::int32_t step = std::int16_t(rating);
        put_magic_pair(c, at_frame(0x68), step);
        const auto bits = n.fpu.fctiwz(0, n.fpu.fmul(n.constant(0x68), n.widen(step)));
        c.put64(at_frame(0x60), bits);
        capped = std::int32_t(std::uint32_t(bits));
        if (std::int16_t(capped) > 10000) capped = 10000;
        if (std::int16_t(rating) > 0xd2) rating = 0xd2;
    }
    if (rule(0x100f2a2c, 0, capped, stipend, rating) && kind8() == -1) {
        hold(capped, stipend, rating);
        record = invoke(c, callee_of_budget::contract_of, 0x100f2a50, {c.u32(game_cell), outside, club});
        if (record != 0) kind = c.u8(record + 0x60) & 0xf;
    }

    if (outside == 0) ask_of_club();
    else ask_of_deal();
    floor_of_club();
    if (outside == 0) price_of_club();
    else price_of_deal();
    if (outside != 0 && c.u32(outside + deal::other_club) != 0) price_by_people();
    final_price();

    if (base > ask) return std::uint32_t(base);
    if (price < ask) ask = price;
    return std::uint32_t(ask);
}
}

std::uint32_t club_budget(Cpu& c, std::uint32_t account, std::uint32_t outside, std::uint32_t kind) {
    Frame frame(c, 0x100f2680, 0xd0, {.first_gpr = 13, .first_fpr = 30});
    return Budget(c, frame, account, outside, kind).run();
}
}
