#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_policy.hpp"

namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace policy;
namespace {
// TOC slots this group reads that the shared header does not name.
constexpr std::int32_t transfers = -0x7b40;     // variable holding the transfer manager object (inferred)
constexpr std::int32_t buyers = -0x7a54;        // variable holding an object classify_contract_offer queries
constexpr std::int32_t home_region = -0x7a30;   // address of an int32 compared with the club's word at +0x14

// The game's calls these routines make (each: argument registers, then r3).
// Meanings are inferred from use where given; otherwise unknown.
constexpr std::uint32_t club_predicate = 0x1014c2e0;  // (club, 0, 1, 0, 0) -> byte; also eligible_for_category's (meaning unknown)
constexpr std::uint32_t find_club_contract = 0x10082f90;  // (contracts, person, club) -> contract or 0

namespace ratio {
// Pool constants (slot::constants, +offset).
constexpr double quarter_over = 1.25, three_quarters = 0.75, eight_tenths = 0.8, seven_fifths = 1.4;
constexpr double half = 0.5, sixty_five = 0.65, nine_tenths = 0.9;
}

// The original's branch-free signed `a > b` of two 32-bit values.
std::uint32_t signed_greater(std::uint32_t a, std::uint32_t b) {
    const std::uint32_t difference = a ^ b;
    const std::uint32_t halved = std::uint32_t(std::int32_t(difference) >> 1);
    return (halved - (difference & a)) >> 31;
}
}

std::uint32_t check_candidate_country_policy(Cpu& c, std::uint32_t staff, std::uint32_t club) {
    Frame frame(c, 0x1036f590, 0x40, {.first_gpr = 30});
    const Person who{c, staff};
    const std::uint32_t nation = who.nation();
    c.set(3, nation);
    if (!nonzero_logical(c, nation)) return result(c, 0);
    const std::uint32_t club_nation = Club{c, club}.nation();
    c.set(4, club_nation);
    // Only clubs of a nation whose rule byte is 14 restrict candidates.
    if (compare(c, Nation{c, club_nation}.rule(), 0xe) != 0) return result(c, 1);
    // A candidate of the club's own nation (first or second) passes.
    if (compare_logical(c, nation, club_nation) == 0) return result(c, 1);
    if (compare_logical(c, who.second_nation(), club_nation) == 0) return result(c, 1);
    keep(c, {{31, club}, {30, staff}});
    const std::uint32_t exempt = Program{c}.call(0x100ca6a0, 0x1036f5fc, {staff});
    if (nonzero(c, std::int8_t(exempt))) return result(c, 1);
    // Candidates with either unknown mark set go through the transfer
    // manager's check; its answer is returned as it is (not normalised).
    if (!nonzero_logical(c, who.national_team()) && !nonzero_logical(c, who.byte_5e())) return result(c, 0);
    const std::uint32_t manager = c.u32(c.toc(transfers));
    return result(c, Program{c}.call(0x103ed520, 0x1036f644, {manager, staff, Club{c, club}.nation(), 0, 0, 0}));
}

std::uint32_t evaluate_row_contract_gate(Cpu&c, std::uint32_t, std::uint32_t amounts, std::uint32_t contract_address,
                                         std::uint32_t club, std::uint32_t price_out) {
    Frame frame(c, 0x1036fbf0, 0x70, {.first_gpr = 25});
    const Contract contract{c, contract_address};
    Field<std::int32_t> price{c, price_out};
    price = 0;
    // No contract, no amounts or a contract with its +0x54 flag set: nothing to gate.
    if (compare(c, std::int32_t(contract_address), 0) == 0 || !nonzero_logical(c, amounts) ||
        nonzero(c, std::int8_t(contract.listed())))
        return result(c, 1);
    const std::uint32_t persons = c.toc(slot::persons), transfer_variable = c.toc(transfers);
    keep(c, {{25, club}, {26, amounts}, {27, contract_address}, {28, price_out}, {29, transfer_variable},
             {30, c.toc(slot::constants)}, {31, persons}});
    const auto person_of_contract = [&] { return c.u32(persons) + c.u32(contract_address) * Person::size; };
    const std::uint32_t asked = Program{c}.call(0x10380f20, 0x1036fc58, {contract_address, club, 0, 0});
    if (!nonzero(c, std::int8_t(asked))) {
        // The price the transfer manager puts on the person (0x103fac70,
        // mode 0), compared with his value: 1 when his value is above it.
        price = std::int32_t(Program{c}.call(0x103fac70, 0x1036fc80,
                                             {c.u32(transfer_variable), person_of_contract(), club, 0}));
        const std::uint32_t value = Person{c, person_of_contract()}.value().get(), found = price.get();
        c.set(4, value);
        c.set(5, found);
        return result(c, std::uint32_t(std::int8_t(signed_greater(value, found))));
    }
    const std::uint32_t usable = Program{c}.call(0x103faff0, 0x1036fcd0, {c.u32(transfer_variable), person_of_contract()});
    if (!nonzero(c, std::int8_t(usable))) return result(c, 0);
    price = std::int32_t(Program{c}.call(0x103fac70, 0x1036fd00, {c.u32(transfer_variable), person_of_contract(), club, 1}));
    c.set(4, price.get());
    // A price above the person's value fails the gate.
    if (compare(c, price.get(), Person{c, person_of_contract()}.value()) > 0) return result(c, 0);
    // The contract's expiry band (0x10380710) picks the share of the
    // amounts' second value (+0xc) the first (+4) must stay under.
    const std::int32_t band = std::int8_t(Program{c}.call(0x10380710, 0x1036fd34, {contract_address}));
    const auto under = [&](double factor, bool first_at_38) {
        const std::int32_t first = c.i32(amounts + 4), second = c.i32(amounts + 0xc);
        c.set(4, signed_word(second));
        Fpu fpu{c};
        const double a = stacked_integer(c, frame.at(first_at_38 ? 0x38 : 0x40), signed_word(first));
        const double b = stacked_integer(c, frame.at(first_at_38 ? 0x40 : 0x38), signed_word(second));
        const double scaled = fpu.fmul(factor, fpu.fsub(a, k::signed_bias));
        const double other = fpu.fsub(b, k::signed_bias);
        c.set_f(1, other);  // the original computes it into f1 (fsub f1,f2,f3)
        fpu.fcmp(0, other, scaled);
        return result(c, other < scaled ? 1 : 0);
    };
    if (compare(c, band, 5) >= 0) {
        if (compare(c, band, 9) >= 0) {
            compare(c, band, 0xf);
            return result(c, band >= 0xf ? 0 : 1);
        }
        if (compare(c, band, 7) >= 0) return under(ratio::nine_tenths, false);
        return under(ratio::eight_tenths, false);
    }
    if (compare(c, band, 0) <= 0) return result(c, 0);
    if (compare(c, band, 3) >= 0) return under(ratio::sixty_five, false);
    return under(ratio::half, true);
}

std::uint32_t classify_contract_offer(Cpu& c, std::uint32_t club, std::uint32_t person_address, std::uint32_t offer_high,
                                      std::uint32_t offer_low, std::uint32_t considered_address,
                                      std::uint32_t reference_high, std::uint32_t reference_low) {
    Frame frame(c, 0x1036f170, 0x80, {.first_gpr = 20});
    // r28 holds the contracts variable's address, then the value the offer
    // is measured against; the other registers keep the arguments.
    Kept<28> contracts_or_value{c, c.toc(slot::contracts)};
    keep(c, {{30, club}, {31, person_address}, {29, c.toc(slot::constants)}, {21, offer_high}, {20, offer_low},
             {22, considered_address}, {24, reference_high}, {23, reference_low}});
    const std::uint32_t buyers_variable = c.toc(buyers);
    keep(c, {{27, buyers_variable}});
    // The caller's ninth argument: a byte on its stack, at (sp before the prologue)+0x38+3.
    Kept<25> strict{c, c.u8(frame.caller() + 0x3b)};
    const Person person{c, person_address};
    const Club buying{c, club}, considered{c, considered_address};
    const std::uint32_t contracts_object = c.u32(contracts_or_value);
    const std::uint32_t found = Program{c}.call(find_club_contract, 0x1036f1bc, {contracts_object, person_address, club});
    keep(c, {{26, found}});
    const Contract contract{c, found};
    Fpu fpu{c};
    if (!nonzero_logical(c, club)) return result(c, 0);
    if (nonzero_logical(c, found) && nonzero(c, std::int8_t(contract.listed()))) return result(c, 0);
    if (compare_logical(c, buying.manager(), person_address) == 0) return result(c, 2);
    if (!(nonzero_logical(c, found) && compare(c, contract.mode(), 2) == 0))
        return result(c, nonzero(c, std::int8_t(std::uint32_t(strict))) ? 2 : 0);
    const Player profile{c, person.player()};
    if (nonzero(c, std::int8_t(std::uint32_t(strict)))) {
        // A contract in negotiation, strict mode.
        const std::uint32_t blocked = Program{c}.call(club_predicate, 0x1036f250, {club, 0, 1, 0, 0});
        if (nonzero(c, std::int8_t(blocked))) return result(c, 0);
        const std::uint32_t exempt = Program{c}.call(0x100d3230, 0x1036f264, {person_address, 0});
        if (nonzero(c, std::int8_t(exempt))) {
            // The person's reputation must reach 1.25 times the considered club's.
            c.set(3, 0x43300000);
            c.set(4, extended(profile.reputation()));
            const double club_reputation = stacked_integer(c, frame.at(0x38), signed_word(considered.reputation()));
            const double person_reputation = stacked_integer(c, frame.at(0x40), signed_word(profile.reputation()));
            const double wanted = fpu.fmul(ratio::quarter_over, fpu.fsub(club_reputation, k::signed_bias));
            const double actual = fpu.fsub(person_reputation, k::signed_bias);
            c.set_f(1, actual);  // the original computes it into f1
            fpu.fcmp(0, actual, wanted);
            if (actual < wanted) return result(c, 0);
        }
        const std::uint32_t contract_nibble = contract.squad_status() & 0xf;
        compare(c, std::int32_t(contract_nibble), 6);
        bool wants_rating_check = contract_nibble == 6;
        if (!wants_rating_check) {
            compare(c, std::int32_t(contract_nibble), 7);
            wants_rating_check = contract_nibble == 7;
        }
        if (!wants_rating_check) {
            c.set(3, person.player());
            // A rating byte of exactly 20 also asks.
            if (compare(c, std::int32_t(c.u8(person.player() + 0xf)), 0x14) != 0) return result(c, 2);
        }
        c.set(4, signed_word(profile.reputation()));
        c.set(3, 0x43300000);
        const double person_reputation = stacked_integer(c, frame.at(0x38), signed_word(profile.reputation()));
        const double club_reputation = stacked_integer(c, frame.at(0x40), signed_word(considered.reputation()));
        const double wanted = fpu.fmul(ratio::three_quarters, fpu.fsub(person_reputation, k::signed_bias));
        c.set_f(1, wanted);  // the original computes it into f1
        const double actual = fpu.fsub(club_reputation, k::signed_bias);
        fpu.fcmp(0, wanted, actual);
        if (!(wanted < actual)) return result(c, 2);
        if (compare(c, contract.happiness(), -0x32) >= 0) return result(c, 2);
        const std::uint32_t club_word = c.u32(club + 0x14);
        c.set(4, club_word);
        if (compare(c, std::int32_t(club_word), c.i32(c.toc(home_region))) == 0) return result(c, 2);
        return result(c, 0);
    }
    // A contract in negotiation, ordinary mode: the value the person is worth.
    const std::uint32_t blocked = Program{c}.call(club_predicate, 0x1036f384, {club, 0, 1, 0, 0});
    if (nonzero(c, std::int8_t(blocked))) {
        const std::uint32_t estimate = Program{c}.call(0x10089a40, 0x1036f3a0,
                                                       {c.u32(contracts_or_value), club, considered_address, person_address});
        const double estimated = fpu.fsub(stacked_integer(c, frame.at(0x38), signed_word(std::int32_t(estimate))), k::signed_bias);
        c.set_f(1, estimated);  // the original computes it into f1
        const double scaled = fpu.fmul(ratio::eight_tenths, estimated);
        // fctiwz through the stack: the low word is the integer.
        c.put64(frame.at(0x40), fpu.fctiwz(0, scaled));
        contracts_or_value = c.u32(frame.at(0x44));
        const std::uint32_t released = Program{c}.call(0x10380520, 0x1036f3d8, {found});
        if (!nonzero(c, std::int8_t(released))) {
            const std::uint32_t club_key = Program{c}.call(0x100c96e0, 0x1036f3e8, {club});
            const std::uint32_t known = Program{c}.call(0x10147e70, 0x1036f3f4, {c.u32(buyers_variable), club_key});
            if (nonzero(c, std::int8_t(known))) {
                const std::uint32_t again = Program{c}.call(0x100c96e0, 0x1036f404, {club});
                const std::int32_t answer = std::int8_t(Program{c}.call(0x10149df0, 0x1036f414, {c.u32(buyers_variable), 0, again}));
                if (compare(c, answer, 2) == 0) contracts_or_value = std::uint32_t(person.value().get());
                else if (answer < 2 && compare(c, answer, 1) >= 0) return result(c, 2);
            }
        }
    } else {
        // An explicit reference (a non-negative high word) gives its low word.
        const std::uint32_t explicit_reference = std::int32_t(reference_high) >= 0 ? 1 : 0;
        c.set(4, 0x7fffffff);
        c.set(3, explicit_reference);
        nonzero(c, std::int32_t(explicit_reference));
        if (explicit_reference) {
            contracts_or_value = reference_low;
        } else if (!nonzero_logical(c, person.player())) {
            contracts_or_value = std::uint32_t(person.value().get());
        } else {
            contracts_or_value = Program{c}.call(0x10085860, 0x1036f49c,
                                                 {c.u32(contracts_or_value), club, considered_address, person_address, 0, 1, 1});
        }
    }
    // The offer (signed 64-bit) must be below the value.
    const std::uint32_t value = contracts_or_value;
    const std::int64_t offer = std::int64_t(std::uint64_t(offer_high) << 32 | offer_low);
    const std::int64_t value64 = std::int32_t(value);
    c.set(4, offer_high ^ 0x80000000);
    const bool below = offer < value64;
    nonzero(c, below ? 1 : 0);
    if (!below) return result(c, 0);
    const std::uint32_t asks = Program{c}.call(club_predicate, 0x1036f4e0, {club, 0, 1, 0, 0});
    if (nonzero(c, std::int8_t(asks))) {
        const std::uint32_t shifted = value - 0x170000u + 0x1ca0u;
        const std::uint32_t shifted_high = std::uint32_t(std::int32_t(shifted) >> 31);
        c.set(5, shifted);
        c.set(4, shifted_high ^ 0x80000000);
        const bool close = std::int64_t(std::int32_t(shifted)) < offer;
        nonzero(c, close ? 1 : 0);
        if (close) {
            const std::uint32_t flagged = Program{c}.call(0x100d3040, 0x1036f51c, {person_address, 0});
            if (nonzero(c, std::int8_t(flagged))) return result(c, 1);
        }
    }
    Program{c}.call(0x10773c80, 0x1036f538, {offer_high, offer_low});
    const double offer_as_double = c.f(1);
    c.set(3, value ^ 0x80000000);
    const double value_double = fpu.fsub(stacked_integer(c, frame.at(0x38), value ^ 0x80000000), k::signed_bias);
    const double limit = fpu.fdiv(value_double, ratio::seven_fifths);
    fpu.fcmp(0, offer_as_double, limit);
    // cror eq,gt,eq: equal also when greater.
    if (c.cr(0) & 4) c.set_cr(0, c.cr(0) | 2);
    return result(c, (c.cr(0) & 2) ? 1 : 2);
}
}
