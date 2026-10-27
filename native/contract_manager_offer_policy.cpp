#include "contract_manager_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

std::uint32_t contract_offer_allowed(Cpu& c, std::uint32_t manager_address,
                                      std::uint32_t proposal) {
    Frame frame(c, 0x10089ea0, 0x60, {.first_gpr = 27});
    const auto people_holder = c.toc(-0x7be0); c.set(5, people_holder);
    Kept<31> index_holder(c, c.toc(slot::contract_index));
    const auto proposal_person = c.u32(proposal), people = c.u32(people_holder);
    c.set(8, people); c.set(0, proposal_person * Staff::size);
    const auto selected_person = people + proposal_person * Staff::size; c.set(7, selected_person);
    Kept<30> contract(c, c.r(30));
    if (!nonzero(c, std::int32_t(selected_person))) contract = 0;
    else {
        const bool person_present = nonzero_logical(c, selected_person);
        bool outside = false;
        if (person_present) {
            const auto count_holder = c.toc(slot::person_count); c.set(5, count_holder);
            const auto id = c.i32(selected_person), count = c.i32(count_holder);
            c.set(6, std::uint32_t(id)); c.set(0, std::uint32_t(count));
            outside = compare(c, id, count) >= 0;
        }
        if (outside) contract = 0;
        else {
            const auto id = c.u32(selected_person), indices = c.u32(index_holder);
            c.set(5, indices); c.set(0, id * Contract::size);
            const auto index = c.i32(indices + id * Contract::size); c.set(0, std::uint32_t(index));
            if (compare(c, index) < 0) contract = 0;
            else {
                const auto offset = std::uint32_t(index) * Contract::size, records = c.u32(manager_address);
                c.set(0, offset); c.set(3, records); contract = records + offset;
            }
        }
    }
    if (!nonzero_logical(c, contract)) return result(c, 1);
    const auto unavailable = c.u32(contract + 0x50) & 2u; c.set(0, unavailable);
    if (nonzero(c, std::int32_t(unavailable))) return result(c, 1);
    const auto target_id = c.i32(proposal + 4), current_id = c.i32(contract + 4);
    c.set(0, std::uint32_t(target_id)); c.set(3, std::uint32_t(current_id));
    if (compare(c, target_id, current_id) != 0) return result(c, 1);
    const auto person_id = c.u32(contract), club_offset = std::uint32_t(current_id) * 0x248;
    const auto clubs_holder = c.toc(-0x7be8); c.set(5, clubs_holder); c.set(3, club_offset);
    const auto clubs = c.u32(clubs_holder); c.set(5, clubs); c.set(0, person_id * Staff::size);
    Kept<28> club(c, clubs + club_offset); Kept<27> person(c, people + person_id * Staff::size);
    const auto player = c.u32(person + 0x14); c.set(0, player);
    if (!nonzero_logical(c, player)) {
        const auto nonplayer = c.u32(person + 0x1c); c.set(0, nonplayer);
        if (nonzero_logical(c, nonplayer)) return result(c, 1);
    }
    const auto flags = c.u8(proposal + 0x38);
    const auto role = std::int32_t(std::int8_t(flags)) >> 2;
    c.set(0, std::uint32_t(role)); c.s.wr<1>(guest::reg::xer_ca, role < 0 && (flags & 2));
    if (!nonzero(c, role)) return result(c, 0);
    const auto locked = c.i8(contract + 0x54); c.set(0, std::uint32_t(std::int32_t(locked)));
    if (nonzero(c, locked)) return result(c, 0);
    Program program(c);
    const auto protected_person = std::int8_t(program.call(0x100d3150, 0x10089fd8, {person, 0}));
    c.set(0, std::uint32_t(std::int32_t(protected_person)));
    if (nonzero(c, protected_person)) return result(c, 0);
    const auto squad_role = c.u8(contract + 0x60) & 15u; c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 7) == 0) return result(c, 0);
    Kept<29> age(c, std::uint32_t(std::int32_t(std::int8_t(c.i16(person + 0x5c)))));
    if (compare(c, std::int32_t(std::uint32_t(age)), 25) < 0) {
        const auto id = c.u32(person), indices = c.u32(index_holder); c.set(4, indices); c.set(3, id * 100);
        const auto status = c.u8(indices + id * 100 + 12) >> 4; c.set(0, status);
        bool ordinary = !nonzero(c, std::int32_t(status));
        if (ordinary) {
            const auto terms = c.u32(contract + 0x50); c.set(3, terms);
            // These tests are ordered because the final condition code and
            // scratch value are part of the original call boundary.
            for (auto mask : {16u, 1u, 8u, 32u, 64u, 128u, 256u, 4u}) {
                const auto selected = terms & mask; c.set(0, selected);
                if (nonzero(c, std::int32_t(selected))) { ordinary = false; break; }
            }
        }
        if (ordinary) return result(c, 1);
    }
    const auto special = std::int8_t(program.call(0x1037f110, 0x1008a080, {contract}));
    c.set(0, std::uint32_t(std::int32_t(special)));
    bool excluded = nonzero(c, special);
    if (!excluded) {
        const auto active = c.u8(contract + 0x5c) & 8u; c.set(0, active);
        excluded = nonzero(c, std::int32_t(active));
    }
    if (excluded) return result(c, 0);
    if (compare(c, std::int32_t(std::uint32_t(age)), 23) > 0) {
        const auto role_state = c.i8(contract + 0x46); c.set(0, std::uint32_t(std::int32_t(role_state)));
        if (compare(c, role_state, 3) < 0) {
            const auto discontent = c.i8(contract + 0x44); c.set(0, std::uint32_t(std::int32_t(discontent)));
            if (compare(c, discontent, -50) < 0) {
                const auto owner = c.u32(club + 0x90); c.set(0, owner);
                if (nonzero_logical(c, owner)) {
                    program.call(0x10717b20, 0x1008a0e0, {frame.at(0x40), c.toc(-0x7b80), 150});
                    const auto first = c.u32(frame.at(0x40)); c.set(0, first);
                    const auto current_owner = c.u32(club + 0x90); c.set(3, current_owner); c.put32(frame.at(0x38), first);
                    const auto second = c.u32(frame.at(0x44)); c.set(0, second);
                    const auto year = c.u16(frame.at(0x3a)), owner_year = c.u16(current_owner + 0x4e);
                    c.set(5, year); c.set(4, owner_year); c.put32(frame.at(0x3c), second);
                    const auto order = compare_logical(c, year, owner_year);
                    bool newer = order > 0;
                    if (order == 0) {
                        const auto day = c.i16(frame.at(0x38)), owner_day = c.i16(current_owner + 0x4c);
                        const auto difference = std::uint32_t(std::int32_t(day) ^ std::int32_t(owner_day));
                        const auto half = std::uint32_t(std::int32_t(difference) >> 1);
                        c.set(4, extended(day)); c.set(3, half);
                        c.s.wr<1>(guest::reg::xer_ca, std::int32_t(difference) < 0 && (difference & 1));
                        newer = day > owner_day;
                    }
                    c.set(0, newer);
                    if (nonzero(c, newer)) return result(c, 0);
                }
            }
        }
    }
    return result(c, 1);
}

void rebalance_pending_offer(Cpu& c, std::uint32_t manager_address, std::uint32_t proposal_address) {
    Frame frame(c, 0x1008a160, 0x80, {.first_gpr = 24, .gpr_offset = -0x30, .first_fpr = 30});
    Kept<24> object(c, manager_address); Kept<30> singles(c, c.toc(-0x5734));
    Kept<28> proposal(c, proposal_address); const auto people_holder = c.toc(-0x7be0); c.set(3, people_holder);
    Kept<31> constants(c, c.toc(-0x571c)); c.set_f(30, c.f32(singles + 0x2c));
    Kept<25> person_id(c, c.u32(proposal)); const auto list = c.u32(object + 0x34); c.set(4, list);
    c.set(0, std::uint32_t(person_id) * Staff::size); const auto people = c.u32(people_holder); c.set(3, people);
    const bool list_present = nonzero_logical(c, list); Kept<26> club_id(c, c.u32(proposal + 4));
    Kept<29> person(c, people + std::uint32_t(person_id) * Staff::size);
    Kept<27> pending(c, c.r(27)); Program program(c); guest::Fpu fp(c);
    if (list_present) {
        const auto head = c.u32(list + 4); c.set(0, head); c.put32(list + 12, head);
        while (true) {
            pending = program.call(0x10738e00, 0x1008a1e8, {c.u32(object + 0x34)});
            if (!nonzero(c, std::int32_t(std::uint32_t(pending)))) break;
            const auto id = c.i32(pending); c.set(0, std::uint32_t(id));
            if (compare(c, id, std::int32_t(std::uint32_t(person_id))) != 0) continue;
            const auto target = c.i32(pending + 4); c.set(0, std::uint32_t(target));
            if (compare(c, target, std::int32_t(std::uint32_t(club_id))) == 0) break;
        }
    } else pending = 0;
    if (!nonzero_logical(c, pending)) return;
    const auto kind = c.u8(pending + 0x64); c.set(0, kind);
    if (compare(c, std::int32_t(kind), 3) != 0) return;
    const auto baseline = program.call(0x1037e660, 0x1008a21c, {proposal, person, 0});
    c.set(3, baseline ^ 0x80000000u); c.set(0, 0x43300000);
    c.put32(frame.at(0x4c), baseline ^ 0x80000000u); c.set(3, pending + 0x10);
    const auto bias = c.f64(constants + 8); c.set_f(1, bias); c.set(4, proposal + 0x10);
    c.put32(frame.at(0x48), 0x43300000); c.set(5, 12); c.set_f(0, c.f64(frame.at(0x48)));
    c.set_f(31, fp.fsubs(c.f(0), bias));
    program.call(0x1077fc28, 0x1008a248); c.set(2, c.u32(frame.at(0x14)));
    program.call(0x1077fc28, 0x1008a25c, {pending + 0x1c, proposal + 0x1c, 20}); c.set(2, c.u32(frame.at(0x14)));
    const auto year = c.u16(pending + 0x32); c.set(0, year); c.set(3, proposal); c.set(4, person); c.set(5, 0); c.put16(proposal + 0x32, year);
    const auto day = c.i16(pending + 0x30); c.set(0, extended(day)); c.put16(proposal + 0x30, extended(day));
    const auto leap = c.u8(pending + 0x34); c.set(0, leap); c.put8(proposal + 0x34, leap);
    const auto assessed = program.call(0x1037e660, 0x1008a288);
    c.set(0, assessed ^ 0x80000000u); pending = 0x43300000;
    c.put32(frame.at(0x44), assessed ^ 0x80000000u); c.set_f(1, c.f64(constants + 8));
    c.put32(frame.at(0x40), pending); c.set_f(0, c.f64(frame.at(0x40)));
    auto current_value = fp.fsubs(c.f(0), c.f(1)); c.set_f(2, current_value);
    while (true) {
        const auto ratio = fp.fdivs(current_value, c.f(31)); c.set_f(1, ratio);
        const auto lower = c.f64(constants + 0x400); c.set_f(0, lower); fp.fcmp(0, ratio, lower);
        if (!(ratio < lower)) {
            const auto upper = c.f64(constants + 0x408); c.set_f(0, upper); fp.fcmp(0, ratio, upper);
            // The original ble branches when GT is clear, including NaN.
            if (!(ratio > upper)) break;
        }
        const auto minimum = c.f32(singles + 0x30); c.set_f(0, minimum); fp.fcmp(0, c.f(30), minimum);
        if (!(c.f(30) > minimum)) break;
        const auto wage = c.u32(proposal + 12); c.set(0, wage);
        const auto difference = fp.fsubs(c.f(31), current_value); c.set_f(2, difference);
        const auto step = c.f32(singles + 12); c.set_f(0, step); c.set(3, proposal);
        c.set(0, wage ^ 0x80000000u); c.put32(frame.at(0x40), pending); c.set_f(1, c.f64(constants + 8));
        c.put32(frame.at(0x44), wage ^ 0x80000000u); c.set_f(30, fp.fmuls(difference, step));
        c.set(4, person); c.set(5, 0); c.set_f(0, c.f64(frame.at(0x40)));
        c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, fp.fadds(c.f(0), c.f(30)));
        c.put64(frame.at(0x48), fp.fctiwz(0, c.f(0))); const auto revised = c.u32(frame.at(0x4c)); c.set(0, revised); c.put32(proposal + 12, revised);
        const auto revalued = program.call(0x1037e660, 0x1008a2f4);
        c.set(0, revalued ^ 0x80000000u); c.put32(frame.at(0x38), pending); c.set_f(1, c.f64(constants + 8));
        c.put32(frame.at(0x3c), revalued ^ 0x80000000u); c.set_f(0, c.f64(frame.at(0x38)));
        current_value = fp.fsubs(c.f(0), c.f(1)); c.set_f(2, current_value);
    }
}
}
