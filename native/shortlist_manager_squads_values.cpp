#include "shortlist_manager_squads_detail.hpp"

// 0x1035a9f0: each open player place valued (budget, priority) and its
// choice kept or let go (0x1035d4c0-0x1035de78).
namespace cm::aot::shortlist_manager::squads {
namespace {
void add_happiness(Cpu& c, std::uint32_t contract, std::int32_t change) {
    const std::uint8_t byte = c.u8(contract + 0x44);
    const std::int32_t moved = std::int32_t(std::int8_t(byte)) + change;
    if (compare(c, moved, 100) > 0) c.put8(contract + 0x44, 100);
    else if (compare(c, moved, -100) < 0) c.put8(contract + 0x44, std::uint8_t(-100));
    else c.put8(contract + 0x44, std::uint32_t(byte) + std::uint32_t(change));
}
std::uint32_t role_of(Assessment& a, std::uint32_t offset) { return Row{a.c, a.slots() + offset}.category(); }

// The choice's rating for the place (r25), by the place's role: first team
// and rotation with the keep (0x1007edd0) and, in the game's first 200 days,
// scaled by 1.25 - 0.00125 * days; backups with the keep; prospects after a
// release (unless kept). Other roles leave r25 as it was (quirk: the
// previous place's rating).
void rate_choice(Assessment& a, std::uint32_t offset, std::uint32_t person, std::uint32_t contract) {
    Cpu& c = a.c;
    const auto club = a.club();
    const std::uint32_t role = role_of(a, offset);
    const std::uint32_t flags = std::uint32_t(a.local32(local::estimate_flags));
    auto position = [&] { return extended(c.i16(a.slots() + offset + 2)); };
    auto estimate = [&](std::uint32_t mode, std::uint32_t back) {
        keep(c, {{25, a.program.call(call::estimate, back, {person, position(), flags | mode, club, 0xffffffff, a.human_manager()})}});
    };
    if (role == 4) {
        a.program.call(call::keep_contract, 0x1035d70c, {a.contracts(), person, club});
        return estimate(0x1c0, 0x1035d734);
    }
    if (role < 4) {
        if (role < 2) return;
        if (nonzero_logical(c, contract) && nonzero(c, std::int8_t(c.r(21)))) {
            add_happiness(c, contract, 50);
            c.put8(contract + 0x46, 10);
            c.put8(contract + 0x45, 0);
        }
        a.program.call(call::keep_contract, 0x1035d624, {a.contracts(), person, club});
        if (a.human(1, 0x1035d63c)) estimate(0x8100c0, 0x1035d670);
        else estimate(0xc0, 0x1035d69c);
        const std::int32_t days = a.local32(local::days);
        if (compare(c, days, 200) >= 0) return;
        const double since = a.integer(0x9a4, days);
        c.put32(a.at(0x99c), 0x43300000);
        c.put32(a.at(0x9a0), signed_word(std::int32_t(c.r(25))));
        const double early = a.fpu.multiply_add(0x1035d6e0, a.constant(0xd0), since, a.constant(0xc8));
        const double rating = a.fpu.fsub(c.f64(a.at(0x99c)), a.constant(8));
        c.set_f(1, rating);  // the original computes it into f1
        keep(c, {{25, std::uint32_t(a.truncated(0x9ac, a.fpu.fmul(rating, early)))}});
        return;
    }
    if (role >= 6) return;
    if (compare_logical(c, person, a.club_manager()) != 0 &&
        (!nonzero(c, a.local8(local::staff_flag)) || compare(c, c.i16(c.u32(person + 0x14) + 0xc), 5000) < 0))
        a.program.call(call::release_contract, 0x1035d774, {a.contracts(), person, club});
    estimate(0x100, 0x1035d79c);
}

// The place's priority (+0xc) from its budget (r17), the choice's rating
// (r25) and the club's reputation, by role.
struct RoleScale {
    std::uint32_t constant;               // k
    std::uint32_t add_site, subtract_site;  // the multiply-adds
};
const RoleScale* scale_for(std::uint32_t role, bool with_choice) {
    static constexpr RoleScale chosen[] = {{0xe0, 0x1035dae0, 0x1035dae8}, {0xe8, 0x1035db4c, 0x1035db54}, {0xf0, 0x1035dbb8, 0x1035dbc0}};
    static constexpr RoleScale open[] = {{0xe0, 0x1035dd20, 0}, {0xe8, 0x1035dd70, 0}, {0xf0, 0x1035ddc0, 0}};
    if (role >= 1 && role <= 3) return with_choice ? &chosen[role - 1] : &open[role - 1];
    return nullptr;
}

// A place with a choice that stays a choice: its happiness moved by how far
// the rating falls short of the budget, then the priority.
void price_choice(Assessment& a, std::uint32_t offset, std::uint32_t contract) {
    Cpu& c = a.c;
    auto& fpu = a.fpu;
    const std::int32_t budget = std::int32_t(c.r(17)), rating = std::int32_t(c.r(25));
    const double rated = a.integer(0x99c, rating);
    const double shortfall = fpu.fdiv(rated, a.integer(0x9a4, budget));
    const std::int32_t change = a.truncated(0x9ac, fpu.fmul(a.constant(0xd8), fpu.fsub(a.constant(0x80), shortfall)));
    const Row slot{c, a.slots() + offset};
    compare_logical(c, contract);
    if (nonzero_logical(c, contract)) {
        if (nonzero(c, std::int8_t(c.r(21)))) add_happiness(c, contract, std::int8_t(change));
        const std::uint32_t role = slot.category();
        if (role == 2 || role == 3) {
            if (compare(c, c.i8(contract + 0x44), 25) < 0 && nonzero(c, std::int8_t(c.r(21)))) c.put8(contract + 0x44, 25);
            Row{c, a.slots() + offset}.target() = -1;
        } else if (compare(c, c.i8(contract + 0x44), 0) < 0 && nonzero(c, std::int8_t(c.r(21)))) {
            c.put8(contract + 0x44, 0);
        }
        if (!nonzero(c, a.local8(local::option)) && role_of(a, offset) == 5)
            c.put8(contract + 0x60, (c.u8(contract + 0x60) & 0xf0) | 6);
    } else {
        const std::uint32_t role = slot.category();
        if (role == 2 || role == 3) Row{c, a.slots() + offset}.target() = -1;
    }
    // The weight: 1.25 - rating / max(budget, 1000), between 0.5 and 1.
    const double rated_again = a.integer(0x99c, rating);
    Row{c, a.slots() + offset}.level() = budget;
    Row{c, a.slots() + offset}.budget() = 0;
    const std::int32_t floor = budget > 1000 ? budget : 1000;
    double weight = fpu.fsub(a.constant(0xc8), fpu.fdiv(rated_again, a.integer(0x9a4, floor)));
    fpu.fcmp(0, a.constant(0x80), weight);
    if (c.cr(0) & 8) {
        weight = a.constant(0x80);
    } else {
        const double rated_third = a.integer(0x9ac, rating);
        weight = fpu.fsub(a.constant(0xc8), fpu.fdiv(rated_third, a.integer(0x9b4, floor)));
    }
    fpu.fcmp(0, weight, a.constant(0x68));
    if (c.cr(0) & 8) weight = a.constant(0x68);
    const std::uint32_t role = role_of(a, offset);
    const auto at = a.slots() + offset;
    if (const auto* scale = scale_for(role, true)) {
        const double reputation = fpu.fmul(weight, a.integer(0x9ac, a.reputation()));
        const double k = a.constant(scale->constant);
        const double for_budget = a.integer(0x9a4, budget);
        c.put32(a.at(0x99c), 0x43300000);
        c.put32(a.at(0x9a0), signed_word(rating));
        const double sum = fpu.multiply_add(scale->add_site, k, for_budget, reputation);
        const double for_rating = fpu.fsub(c.f64(a.at(0x99c)), a.constant(8));
        c.set_f(1, for_rating);  // the original computes it into f1
        Row{c, at}.weight() = a.truncated(0x9b4, fpu.multiply_add(scale->subtract_site, k, for_rating, sum));
        return;
    }
    // Roles 4-5 (0.8) and the others (0.5): k * (budget - rating).
    const bool backup = role == 4 || role == 5;
    const double k = a.constant(backup ? 0xf8 : 0x68);
    const double for_rating = a.integer(0x9a4, rating);
    c.put32(a.at(0x99c), 0x43300000);
    c.put32(a.at(0x9a0), signed_word(budget));
    const double part = fpu.fmul(k, for_rating);
    const double for_budget = fpu.fsub(c.f64(a.at(0x99c)), a.constant(8));
    c.set_f(1, for_budget);  // the original computes it into f1
    Row{c, at}.weight() = a.truncated(0x9ac, fpu.multiply_add(backup ? 0x1035dc10 : 0x1035dc60, k, for_budget, part));
}

// A place without a choice: the whole budget is open.
void price_open(Assessment& a, std::uint32_t offset) {
    Cpu& c = a.c;
    auto& fpu = a.fpu;
    const std::int32_t budget = std::int32_t(c.r(17));
    {
        const Row slot{c, a.slots() + offset};
        slot.level() = budget;
    }
    Row{c, a.slots() + offset}.target() = -1;
    Row{c, a.slots() + offset}.budget() = 0;
    const std::uint32_t role = role_of(a, offset);
    const auto at = a.slots() + offset;
    if (const auto* scale = scale_for(role, false)) {
        const double for_budget = a.integer(0x99c, budget);
        c.set_f(1, for_budget);  // the original computes it into f1
        const double reputation = a.integer(0x9a4, a.reputation());
        Row{c, at}.weight() = a.truncated(0x9ac, fpu.multiply_add(scale->add_site, a.constant(scale->constant), for_budget, reputation));
    } else {
        const bool backup = role == 4 || role == 5;
        Row{c, at}.weight() = a.truncated(0x9a4, fpu.fmul(a.constant(backup ? 0xf8 : 0x68), a.integer(0x99c, budget)));
        c.set_f(1, a.constant(8));  // the original loads the bias into f1 (lfd f1)
    }
    const Row slot{c, a.slots() + offset};
    if (c.u16(slot.address + 2) & 1) slot.weight() = std::int32_t(std::uint32_t(slot.weight()) * 3);
}
}

void value_open_places(Assessment& a) {
    Cpu& c = a.c;
    Kept<14> index{c, 0};
    Kept<13> offset{c, 0};
    for (; compare(c, byte_of(index), slot_count) < 0; offset = offset + Row::size, index = index + 1) {
        const Row slot{c, a.slots() + offset};
        if (compare(c, slot.target(), -1) > 0) continue;
        if (c.u8(slot.address) != Row::player) continue;
        // The budget: the club's value up to 36000, three quarters of it
        // for an open key place.
        const std::int32_t value = a.local32(local::value);
        Kept<17> budget{c, std::uint32_t(compare(c, value, 36000) < 0 ? value : 36000)};
        if (compare(c, std::int16_t(slot.positions()), 1) == 0 && compare(c, slot.previous(), -1) == 0) {
            budget = std::uint32_t(a.truncated(0x9a4, a.fpu.fmul(a.constant(0xa0), a.integer(0x99c, std::int32_t(std::uint32_t(budget))))));
            c.set_f(1, a.constant(8));  // the original loads the bias into f1 (lfd f1)
        }
        const std::int32_t choice = slot.previous();
        if (compare(c, choice, -1) <= 0) {
            price_open(a, offset);
            continue;
        }
        const Kept<15> person{c, person_address(c, choice)};
        const Kept<16> contract{c, a.program.call(call::club_contract, 0x1035d580, {a.contracts(), person, a.club()})};
        rate_choice(a, offset, person, contract);
        std::int32_t rating = std::int32_t(c.r(25));
        if (compare(c, rating, -10000) < 0) rating = -10000;
        else if (compare(c, 60000, rating) < 0) rating = 60000;
        keep(c, {{25, std::uint32_t(rating)}});
        // A choice rated within the budget whose contract runs on (no flag
        // 0x100 or 1 at +0x50 and a year or more left) holds the place.
        bool priced = compare(c, rating, std::int32_t(std::uint32_t(budget))) < 0;
        if (!priced) {
            if (!nonzero_logical(c, contract) || compare(c, c.i32(contract + 0x50), 0) == 0) {
                priced = false;
            } else {
                const std::uint32_t terms = c.u32(contract + 0x50);
                if (nonzero(c, std::int32_t(terms & 0x100)) || nonzero(c, std::int32_t(terms & 1))) priced = true;
                else priced = compare(c, std::int16_t(a.program.call(call::end_date_delta, 0x1035d7fc, {contract})), 0x16d) < 0;
            }
        }
        if (!priced) {
            Row{c, a.slots() + offset}.target() = c.i32(person);
            continue;
        }
        price_choice(a, offset, contract);
    }
}
}
