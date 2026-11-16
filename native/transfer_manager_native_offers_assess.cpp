#include "transfer_manager_native_offers.hpp"
#include "transfer_manager_native_offers_common.hpp"

namespace cm::aot::transfer_manager_native {
using namespace offers;

namespace {
// Doubles of the module's constant block (cell::floats) this routine reads.
namespace pool {
inline constexpr std::uint32_t scale_a = 0x00, scale_b = 0x18, floor_scale = 0x50, cap_scale = 0x70,
                               fit_a = 0x20, fit_b = 0x60, power_low = 0x30, power_high = 0x58, value_power = 0x48,
                               value_limit = 0x68, value_ceiling = 0x78;
}
constexpr std::uint32_t glue_pow = 0x10780fc0;
}

void assess_player_after_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer_record) {
    // 0x10773804 (saves f27-f31 below the GPR block) is part of the frame.
    Frame frame(c, 0x103e5b70, 0xf0, {.first_gpr = 17, .gpr_offset = -0x6c, .first_fpr = 27});
    guest::Fpu fpu(c);
    const Samples samples(c);
    const auto people_cell = c.toc(toc::people), contracts_cell = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract),
               clubs_cell = c.toc(toc::clubs), awol_cell = c.toc(cell::awol), constants = c.toc(cell::floats);
    // What the non-volatile registers hold at the calls (the callees save them): constants from the prologue, and
    // the values the original assigns as it goes. They start as the caller's.
    std::uint32_t cap_b_or_found = found_cell, score = c.r(18), player_contract = c.r(19), contract_club = c.r(20),
                  awol_or_kind = awol_cell, first_roll_or_last = samples.last_cell, cap_a_or_contracts = contracts_cell,
                  club = people_cell, person = clubs_cell, verdict_or_offer = offer_record;
    double value = c.f(31), value_floor = c.f(27), trait_a = c.f(28), trait_b = c.f(29), trait_c = c.f(30);
    const auto hold = [&] {
        keep(c, 17, cap_b_or_found);
        keep(c, 18, score);
        keep(c, 19, player_contract);
        keep(c, 20, contract_club);
        keep(c, 21, awol_or_kind);
        keep(c, 22, samples.base_cell);
        keep(c, 23, samples.first);
        keep(c, 24, first_roll_or_last);
        keep(c, 25, samples.cursor_cell);
        keep(c, 26, constants);
        keep(c, 27, club);
        keep(c, 28, cap_a_or_contracts);
        keep(c, 29, person);
        keep(c, 30, manager);
        keep(c, 31, verdict_or_offer);
        c.set_f(27, value_floor);
        c.set_f(28, trait_a);
        c.set_f(29, trait_b);
        c.set_f(30, trait_c);
        c.set_f(31, value);
    };
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        hold();
        return invoke(c, entry, back, arguments);
    };
    // A math import through its glue (it stores r2 at sp+0x14, and the original reloads it from there).
    const auto power = [&](double base, double exponent, std::uint32_t back) {
        hold();
        c.set_f(1, base);
        c.set_f(2, exponent);
        c.call(glue_pow, back);
        c.set(2, c.u32(frame.at(0x14)));
        return c.f(1);
    };
    const auto roll = [&](std::uint32_t refill_back) { return std::int32_t(samples.next(refill_back, hold)); };
    const auto low_word = [](std::uint64_t bits) { return std::uint32_t(bits); };

    const auto contract = call(0x10082f20, 0x103e5bd0, {c.u32(contracts_cell), person_of(c, offer_record)});
    c.put32(found_cell, contract);
    contract_club = contract ? c.u32(clubs_cell) + c.u32(contract + 4) * club_size : 0;
    club = c.u32(clubs_cell) + c.u32(offer_record + offer::club) * club_size;
    person = c.u32(people_cell) + c.u32(offer_record + offer::person) * person_size;
    // A draw that may flag the offer: the club's rating (halfword at +0x1ec) against a remainder of 5750.
    if (c.i16(club + 0x1ec) > roll(0x103e5c34) % 0x1676 + 0x109a) c.put8(offer_record + offer::flagged, 1);
    if (c.u8(offer_record + offer::flagged) != 1 || !contract_club || !club || !c.u32(person + 0x14)) return;

    player_contract = call(0x10082f20, 0x103e5cbc, {c.u32(contracts_cell), person});
    const auto interest = call(0x1036d510, 0x103e5cd8, {c.u32(manager + shortlists), person, club, 0});
    call(0x10369080, 0x103e5cfc, {c.u32(manager + shortlists), person, club, 1, 0, interest, 0});
    value = c.f(1);
    // The value floor: the smaller of value^a and b*value, with the power taken again when it is the smaller.
    const double powered = power(value, c.f64(constants + pool::value_power), 0x103e5d08);
    value_floor = fpu.fmul(c.f64(constants + pool::floor_scale), value);
    fpu.fcmp(0, powered, value_floor);
    if (powered < value_floor) value_floor = power(value, c.f64(constants + pool::value_power), 0x103e5d28);
    // Four traits of the person (signed bytes +0x69, +0x65, +0x67, +0x63) raised to powers.
    trait_a = power(to_double(fpu, c, constants, c.i8(person + 0x69)), c.f64(constants + pool::power_low), 0x103e5d5c);
    trait_b = power(to_double(fpu, c, constants, c.i8(person + 0x65)), c.f64(constants + pool::power_high), 0x103e5d90);
    trait_c = power(to_double(fpu, c, constants, c.i8(person + 0x67)), c.f64(constants + pool::power_high), 0x103e5dc4);
    const double trait_d = power(to_double(fpu, c, constants, c.i8(person + 0x63)), c.f64(constants + pool::power_low), 0x103e5df8);
    // The person's mood: a sum of truncated terms, kept as a halfword.
    const auto term_c = low_word(fpu.fctiwz(4, trait_c));
    const auto term_b = low_word(fpu.fctiwz(3, trait_b));
    const double fit = fpu.multiply_add(0x103e5e14, c.f64(constants + pool::fit_a), trait_a, c.f64(constants + pool::fit_b));
    const auto term_floor = low_word(fpu.fctiwz(2, value_floor));
    const auto term_fit = low_word(fpu.fctiwz(0, fit));
    const auto term_d = low_word(fpu.fctiwz(1, trait_d));
    const std::int32_t rating_share = std::int16_t(c.i16(c.u32(person + 0x14) + 0xa) / 0x28);
    score = std::uint32_t(std::int16_t(term_fit + term_c + term_b - term_floor - std::uint32_t(rating_share) - term_d));

    if (player_contract && (c.u8(player_contract + 0x5c) & 8)) {
        const auto unrest = std::int16_t(roll(0x103e5eb4) % 6000 + 0x7d0);
        score = score - std::uint32_t(std::int32_t(unrest));
    }
    // Never taken once the test at 0x103e5c9c passed: contract_club came from the same lookup of the same person.
    if (!player_contract) return;
    fpu.fcmp(0, value, c.f64(constants + pool::value_limit));
    if (!(value > c.f64(constants + pool::value_limit) || value == c.f64(constants + pool::value_limit))) return;
    const auto shortfall = roll(0x103e5f30) % 700;
    const auto mood = std::int16_t(score);
    if (mood >= -shortfall) return;
    if (!(mood < -0xfa0) && c.i8(person + 0x65) + c.i8(person + 0x67) >= 0xf) return;
    if (c.i16(c.u32(person + 0x14) + 0xa) <= 0xdac) return;

    const auto contracts = c.u32(contracts_cell);
    const auto cap_a_raw = call(0x10084be0, 0x103e5fc8, {contracts, person, contract_club, 0, 0xffffffff, 0});
    const auto cap_a_bits = fpu.fctiwz(0, fpu.fmul(c.f64(constants + pool::scale_a), to_double(fpu, c, constants, std::int32_t(cap_a_raw))));
    cap_a_or_contracts = low_word(cap_a_bits);
    const auto cap_b_raw = call(0x10085860, 0x103e6018, {c.u32(contracts_cell), contract_club, club, person, 0, 1, 1});
    cap_b_or_found = low_word(fpu.fctiwz(0, fpu.fmul(c.f64(constants + pool::cap_scale), to_double(fpu, c, constants, std::int32_t(cap_b_raw)))));
    verdict_or_offer = call(0x10416840, 0x103e6064, {offer_record, 0, 0, 0, 1, 1});
    const auto smaller_cap = [&] {
        return std::int32_t(cap_a_or_contracts) < std::int32_t(cap_b_or_found) ? std::int32_t(cap_a_or_contracts) : std::int32_t(cap_b_or_found);
    };
    if (std::int32_t(verdict_or_offer) < smaller_cap()) return;
    call(0x1037fa80, 0x103e6090, {player_contract, 0x80, 0});

    const auto status_byte = c.i8(person + 0x5d);
    if (c.i16(c.u32(person + 0x14) + 0xa) > 0x1482 && status_byte > 0x12 && status_byte < 0x20 && std::int16_t(score) < -8000 &&
        std::int8_t(call(0x1037f110, 0x103e60cc, {player_contract})) != 0 &&
        c.u8(c.u32(manager + status_rows) + c.u32(person) * status_row_size + 1) == 0 &&
        std::int8_t(call(0x10003dc0, 0x103e6100, {c.u32(awol_cell), person, 0})) == 0) {
        call(0x10006900, 0x103e611c, {c.u32(awol_cell), person, club, 1});
        awol_or_kind = c.u8(player_contract + 0x4c);
        const auto first = roll(0x103e6140) % 400;
        first_roll_or_last = std::uint32_t(first);
        const auto second = roll(0x103e6194) % 200;
        if (std::int32_t(std::int8_t(awol_or_kind)) + first - second < -0x96) call(0x100da320, 0x103e61f0, {person, contract_club, 1});
    }
    bool release = false;
    fpu.fcmp(0, value, c.f64(constants + pool::value_ceiling));
    if (value > c.f64(constants + pool::value_ceiling) || std::int16_t(score) < -0x1388) {
        release = true;
    } else {
        const double scaled = fpu.fmul(c.f64(constants + pool::scale_b), to_double(fpu, c, constants, std::int32_t(verdict_or_offer)));
        const double smaller = to_double(fpu, c, constants, smaller_cap());
        fpu.fcmp(0, scaled, smaller);
        release = !(scaled <= smaller);
    }
    if (release) call(0x100d9b80, 0x103e6270, {person, c.u32(contract_club + 0x90), 1});
}
}
