#include "contract_screens.hpp"
#include "contract_screens_calls.hpp"
#include "guest_fpu.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;
using detail::call_owned;

namespace {
using screen_manager::detail::Program;
constexpr std::uint32_t category_fits = 0x1036f920, evaluate_candidate = 0x10369080,
                        set_history = 0x1073f1e0, contract_applies = 0x1037e3e0;
constexpr int screen_slot = -0x7bb0, contract_slot = -0x7b7c, primary_slot = -0x7b78,
              offer_manager_slot = -0x7b40, strings_slot = -0x56cc, constants_slot = -0x56d0;

struct Person {
    Cpu& cpu;
    std::uint32_t address;

    std::uint32_t profile() const { return cpu.u32(address + 0x14); }
    std::int32_t age() const { return std::int8_t(cpu.i16(address + 0x5c)); }
    std::int32_t reputation_factor() const { return cpu.i8(address + 0x63); }
    std::int32_t interest_factor() const { return cpu.i8(address + 0x65); }
    std::int32_t ambition_factor() const { return cpu.i8(address + 0x67); }
};

struct ExistingTerms {
    Cpu& cpu;
    std::uint32_t address;

    unsigned category() const { return cpu.u8(address + 0x60) & 0xf; }
    std::int32_t kind() const {
        const auto encoded = cpu.u8(address + 0x38);
        cpu.s.wr<1>(reg::xer_ca, (encoded & 0x82) == 0x82);
        return std::int8_t(encoded) >> 2;
    }
};

void compute_interest_limit(Cpu& c, const Frame& frame, const Person& person, std::uint32_t constants) {
    c.set_f(2, c.f64(constants + 8));
    c.set_f(0, c.f64(constants + 0x18));
    c.set_f(3, c.f64(constants + 0x10));
    const auto reputation = person.reputation_factor(), interest = person.interest_factor(), ambition = person.ambition_factor();
    c.set({{4, std::uint32_t(reputation)}, {7, std::uint32_t(ambition)},
           {8, std::uint8_t(ambition)}});
    const auto total = reputation + interest + ambition;
    const auto encoded = std::uint32_t(total + 42) ^ 0x80000000u;
    c.put32(frame.at(0x94), 0x43300000);
    c.put32(frame.at(0x98), encoded);
    Fpu fpu(c);
    c.set_f(1, fpu.fsub(c.f64(frame.at(0x94)), c.f(2)));
    c.set_f(0, fpu.fdiv(c.f(1), c.f(0)));
    c.set_f(30, fpu.fadd(c.f(3), c.f(0)));
}

bool pending_date_matches(Cpu& c, const Frame& frame, std::uint32_t pending) {
    const auto today = c.toc(-0x7b80);
    const auto day = c.u16(pending + 0xe);
    c.put32(frame.at(0x3c), c.u32(today));
    const auto current_day = c.u16(frame.at(0x3e));
    const auto tail = c.u32(today + 4);
    c.put32(frame.at(0x40), tail);
    c.set(5, tail);
    c.set(0, day);
    bool same = compare_logical(c, current_day, day) == 0;
    if (same) {
        const auto year = c.i16(pending + 0xc);
        c.set(0, std::uint32_t(year));
        same = compare(c, c.i16(frame.at(0x3c)), year) == 0;
    }
    c.set(4, same);
    return compare(c, same) != 0;
}

void open_negotiation(Cpu& c, bool with_offer, std::uint32_t person, std::uint32_t link,
                      std::uint32_t category, std::uint32_t kind, std::uint32_t existing_back,
                      std::uint32_t new_back) {
    if (with_offer)
        call_owned(c, 0x100965c0, existing_back, open_existing_negotiation, link, person, category, kind, 1u);
    else
        call_owned(c, 0x100960d0, new_back, open_new_negotiation, person, category, kind, 1u);
}

bool can_reuse_category(Cpu& c, Program& program, const Person& person, const ExistingTerms& terms,
                        std::uint32_t contracts, std::uint32_t primary_cache,
                        std::uint32_t restrictions, std::uint32_t role_count) {
    const auto restricted = std::int8_t(restrictions);
    c.set(0, std::uint32_t(restricted));
    if (compare(c, restricted) != 0) return false;
    c.set(0, role_count);
    if (compare(c, role_count, 2) >= 0 || compare_logical(c, terms.address) == 0) return false;

    const auto category = terms.category();
    c.set(0, category);
    if (compare(c, category, 7) >= 0 || compare(c, category) <= 0) return false;
    const auto primary = program.call(0x10082f20, 0x10094e10, {c.u32(contracts), person.address});
    const bool exists = compare_logical(c, primary) != 0;
    c.put32(primary_cache, primary);
    bool locked = false;
    if (exists) {
        const auto primary_kind = ExistingTerms{c, primary}.kind();
        c.s.wr<1>(reg::xer_ca, std::uint32_t(primary_kind) <= 5);
        locked = primary_kind == 5;
    }
    c.set(0, locked);
    if (compare(c, locked) != 0) return false;
    const auto current_kind = terms.kind();
    c.set(0, std::uint32_t(current_kind));
    return compare(c, current_kind, 6) != 0;
}

std::uint32_t recruitment_manager(Cpu& c, std::uint32_t offers) { return c.u32(c.u32(offers) + 0x988); }

bool meets_reputation(Cpu& c, const Person& person, std::uint32_t club, int category) {
    // Age under 24 has a separate floor before the all-ages check for category one.
    for (const int young_category : {1, 2, 3}) {
        c.set(0, category);
        if (compare(c, category, young_category) != 0) continue;
        const auto age = person.age();
        c.set(0, std::uint32_t(age));
        if (compare(c, age, 24) >= 0) continue;
        const auto profile = person.profile();
        c.set(young_category == 1 ? 3 : 4, profile);
        if (compare_logical(c, profile) == 0) continue;
        const auto reputation = c.i16(profile + 0xa);
        c.set(5, std::uint32_t(reputation));
        if (young_category == 1 && compare(c, reputation, 3250) < 0) return false;
        constexpr int allowances[] = {0, 0, 750, 1250};
        const auto needed = c.i16(club + 0x1ec);
        c.set(0, std::uint32_t(needed));
        const auto rating = reputation + person.reputation_factor() * 10 + person.ambition_factor() * 20
                            + allowances[young_category];
        c.set(3, std::uint32_t(rating));
        const auto scratch = young_category == 1 ? person.reputation_factor() * 10
                                                 : reputation + person.reputation_factor() * 10;
        c.set(4, std::uint32_t(scratch));
        if (compare(c, rating, needed) < 0) return false;
    }

    c.set(0, category);
    if (compare(c, category, 1) != 0) return true;
    const auto profile = person.profile();
    c.set(4, profile);
    // Category one has no intervening call after the loop's profile check.
    compare_logical(c, profile);
    const auto reputation = c.i16(profile + 0xa);
    const auto scratch = reputation + person.reputation_factor() * 10;
    const auto rating = scratch + person.ambition_factor() * 25 + 1750;
    const auto needed = c.i16(club + 0x1ec);
    c.set({{0, std::uint32_t(needed)}, {3, std::uint32_t(rating)},
           {4, std::uint32_t(scratch)}, {5, std::uint32_t(reputation)}});
    return compare(c, rating, needed) >= 0;
}

void choose_category(Cpu& c, Program& program, const Frame& frame, const Person& person, const ExistingTerms& terms,
                     std::uint32_t club, std::uint32_t offers, std::uint32_t constants) {
    const bool has_terms = compare_logical(c, terms.address) != 0;
    c.set(17, 0);
    if (has_terms) {
        const auto applies = std::int8_t(program.call(contract_applies, 0x10094f0c, {terms.address, person.address}));
        c.set(0, std::uint32_t(applies));
        if (compare(c, applies, 1) != 0) {
            const auto category = terms.category();
            const auto range = std::uint8_t(category - 7);
            c.set({{0, range}, {5, category}, {17, category}});
            if (compare_logical(c, range, 1) <= 0) {
                c.set(17, 0);
                return;
            }
            const auto profile = person.profile();
            c.set(0, profile);
            if (compare_logical(c, profile) != 0) {
                const auto allowed = std::int8_t(program.call(category_fits, 0x10094f50,
                                                    {recruitment_manager(c, offers), person.address, category}));
                c.set(0, std::uint32_t(allowed));
                if (compare(c, allowed) != 0) return;
            }
            c.set(17, 0);
            return;
        }
    }

    c.set_f(31, c.f64(constants + 0x20));
    Fpu fpu(c);
    for (unsigned category = 1;; ++category) {
        c.set({{18, category}, {0, category}});
        if (compare(c, category, 8) >= 0) break;
        const auto profile = person.profile();
        c.set(0, profile);
        if (compare_logical(c, profile) == 0) break;
        c.set(0, category);
        if (compare(c, category, 7) == 0) continue;
        if (compare(c, category, 1) != 0 && compare_logical(c, terms.address) != 0) {
            const auto applies = std::int8_t(program.call(contract_applies, 0x10094f94, {terms.address, person.address}));
            c.set(0, std::uint32_t(applies));
            if (compare(c, applies, 1) == 0) {
                c.set({{0, category}, {3, terms.category()}});
                if (compare(c, terms.category(), category) <= 0) continue;
            }
        }
        if (!meets_reputation(c, person, club, category)) continue;
        const auto allowed = std::int8_t(program.call(category_fits, 0x1009512c,
                                                {recruitment_manager(c, offers), person.address, category}));
        c.set(0, std::uint32_t(allowed));
        if (compare(c, allowed) == 0) continue;
        program.call(evaluate_candidate, 0x10095158,
                     {recruitment_manager(c, offers), person.address, club, 1, 0, category, frame.at(0x38)});
        const auto score = c.f(1);
        fpu.fcmp(0, score, c.f(30));
        if (c.cr(0) & 4) continue;
        fpu.fcmp(0, score, c.f(31));
        if (!(c.cr(0) & 4)) continue;
        const auto excluded = c.u32(frame.at(0x38)) & 0x8000;
        c.set(0, excluded);
        if (compare(c, excluded) != 0) continue;
        c.set(17, category);
        c.set_f(31, score);
    }
}

void report_missing_context(Cpu& c, const Frame& frame, std::uint32_t strings, bool missing_instance) {
    const auto call = missing_instance ? 0x10094bb0u : 0x10094c10u;
    report_assertion(c, {call, strings + 0x87, strings + 0x9c, strings + 0xa9,
                        frame.at(missing_instance ? 0x74 : 0x54),
                        frame.at(missing_instance ? 0x4c : 0x44), missing_instance ? 0x193u : 0x19bu});
}
}

void open_contract_choice(Cpu& c, std::uint32_t person_address, std::uint32_t link,
                          std::uint32_t with_offer, std::uint32_t restrictions) {
    Frame frame(c, 0x10094ae0, 0xf0, {.first_gpr = 16, .gpr_offset = -0x50, .first_fpr = 30});
    const auto screens = c.toc(screen_slot), contracts = c.toc(contract_slot), offers = c.toc(offer_manager_slot),
               primary = c.toc(primary_slot), constants = c.toc(constants_slot), strings = c.toc(strings_slot);
    keep(c, {{18, primary}, {19, c.toc(-0x7bac)}, {20, offers}, {21, constants}, {22, contracts},
             {23, screens}, {24, person_address}, {25, link}, {26, with_offer}, {27, restrictions},
             {28, strings}, {29, 0xffffffff}, {30, c.toc(-0x7ba8)}});
    const bool has_link = compare(c, link) != 0;
    const Person person{c, person_address};
    compute_interest_limit(c, frame, person, constants);
    if (has_link) with_offer = 1;
    c.set(26, with_offer);

    const auto offset = std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
    c.set({{0, offset}, {3, screens + offset}});
    const auto instance = c.u32(screens + offset + 4);
    c.set(17, instance);
    if (compare_logical(c, instance) == 0) {
        report_missing_context(c, frame, strings, true);
        return;
    }
    const auto club = c.u32(instance + 0x24);
    c.set(31, club);
    if (compare_logical(c, club) == 0) {
        report_missing_context(c, frame, strings, false);
        return;
    }

    Program program{c};
    unsigned role_count = 0;
    c.set(16, role_count);
    for (unsigned role = 0;; ++role) {
        c.set({{0, role}, {19, role}});
        if (compare(c, role, 15) > 0) break;
        const auto available = std::int8_t(program.call(0x1007e4f0, 0x10094c70, {c.u32(contracts), role, person_address}));
        c.set(0, std::uint32_t(available));
        if (compare(c, available) != 0) c.set(16, ++role_count);
    }
    const auto contract = program.call(0x10082f90, 0x10094ca0, {c.u32(contracts), person_address, club});
    c.set({{0, contract}, {19, contract}});
    const auto kind = program.call(0x10089cc0, 0x10094cb4, {c.u32(contracts), person_address});
    c.set({{0, kind}, {30, kind}});
    const auto pending = program.call(0x1007bf20, 0x10094cd0, {c.u32(contracts), person_address, instance, club});
    c.set(28, pending);
    const auto pending_category = compare(c, pending) != 0 ? c.u8(pending + 0x14) : 0xffffffff;
    c.set(29, pending_category);
    const auto existing_category = std::int8_t(pending_category);
    c.set(0, std::uint32_t(existing_category));

    const auto open = [&](std::uint32_t existing_back, std::uint32_t new_back) {
        const auto existing = std::int8_t(with_offer);
        c.set(0, std::uint32_t(existing));
        open_negotiation(c, compare(c, existing) != 0, person_address, link, pending_category, kind, existing_back, new_back);
    };
    if (compare(c, existing_category, -1) > 0) {
        // A nonnegative pending category is only read from an existing record.
        compare_logical(c, pending);
        if (pending_date_matches(c, frame, pending)) {
            c.set(0, role_count);
            if (compare(c, role_count, 2) < 0) {
                open(0x10094d60, 0x10094d78);
                return;
            }
        }
    }

    const auto preferred = std::int8_t(kind);
    c.set(0, std::uint32_t(preferred));
    if (compare(c, preferred, 11) != 0) {
        c.set(0, role_count);
        if (compare(c, role_count, 1) == 0) {
            open(0x10094db4, 0x10094dcc);
            return;
        }
    }

    const ExistingTerms terms{c, contract};
    if (can_reuse_category(c, program, person, terms, contracts, primary, restrictions, role_count)) {
        if (compare_logical(c, link) != 0) {
            c.set(0, std::uint32_t(existing_category));
            const auto chosen = compare(c, existing_category, -1) == 0 ? terms.category() : pending_category;
            c.set(0, chosen);
            open_negotiation(c, true, person_address, link, std::uint32_t(std::int8_t(chosen)),
                             kind, 0x10094e9c, 0);
        } else {
            c.set(0, c.u8(contract + 0x60));
            open_negotiation(c, false, person_address, link, terms.category(), kind, 0, 0x10094eb8);
        }
        return;
    }

    const auto opened = std::int8_t(program.call(0x1073e4b0, 0x10094ed8,
                           {screens, c.toc(-0x56d4), c.toc(-0x56d8), 1, 0, 0}));
    c.set(0, std::uint32_t(opened));
    if (compare(c, opened) == 0) return;
    const auto history = [&](unsigned slot, std::uint32_t value, std::uint32_t back) {
        program.call(set_history, back, {screens, slot, value, 0});
    };
    history(0, person_address, 0x10094ef4);
    choose_category(c, program, frame, person, terms, club, offers, constants);

    c.set(16, std::uint32_t(existing_category));
    if (compare(c, existing_category, -1) > 0) {
        history(1, std::uint32_t(existing_category), 0x100951b8);
        history(6, std::uint32_t(existing_category), 0x100951cc);
    } else {
        history(1, 0, 0x100951e4);
        history(6, std::uint32_t(std::int8_t(c.r(17))), 0x100951f8);
    }
    history(2, link, 0x1009520c);
    history(3, std::uint32_t(std::int8_t(with_offer)), 0x10095220);
    history(4, std::uint32_t(preferred), 0x10095234);
    history(5, pending, 0x10095248);
    if (compare_logical(c, link) != 0) {
        const auto offer = c.u32(link);
        c.set(3, offer);
        if (compare_logical(c, offer) != 0) {
            history(7, c.u32(offer + 0x2c), 0x10095270);
            return;
        }
    }
    history(7, 0, 0x10095288);
}
}
