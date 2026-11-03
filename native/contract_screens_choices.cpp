#include "contract_screens.hpp"
#include "contract_screens_adapter.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Program;
constexpr std::uint32_t history_value = 0x1073ef70, set_history_value = 0x1073f1e0;
constexpr int screens_slot = -0x7bb0, contracts_slot = -0x7b7c;

struct Controls {
    Cpu& cpu;
    std::uint32_t screens, layout;
    std::int16_t control;

    std::uint32_t address() const { return layout + 0xc0000 + std::uint32_t(control) * 0x17c; }

    std::int16_t action(unsigned scratch) const {
        std::int16_t selected = 0;
        if (compare(cpu, control, -1) != 0) {
            cpu.set(scratch, address());
            selected = std::int16_t(cpu.u32(address() - 0x5794));
        }
        if (compare(cpu, selected) == 0) {
            cpu.set(scratch, screens + 0x260000);
            selected = cpu.i16(screens + 0x260000 - 0x7e62);
        }
        cpu.set(0, std::uint32_t(selected));
        return selected;
    }

    std::uint32_t value() const {
        cpu.set(0, std::uint32_t(control));
        if (compare(cpu, control, -1) == 0) return 0;
        cpu.set(0, std::uint32_t(control) * 0x17c);
        cpu.set(3, address());
        return cpu.u32(address() - 0x5754);
    }
};

bool needs_refresh(Cpu& c, std::uint32_t offer, std::uint32_t revision) {
    if (compare_logical(c, offer) == 0) return false;
    const auto saved_revision = c.u32(offer + 0x2c);
    c.set(0, saved_revision);
    if (compare(c, saved_revision, revision) != 0) return true;
    const auto status = c.i8(offer + 0x34);
    c.set(0, std::uint32_t(status));
    return compare(c, status, 3) == 0 || compare(c, status, 2) == 0;
}

void proceed_to_negotiation(Cpu& c, bool existing, std::uint32_t link, std::uint32_t person,
                            std::uint32_t selected_terms, std::uint32_t kind) {
    if (existing) {
        constexpr auto entry = 0x100965c0u, back = 0x1009606cu;
        leave(c, {{3, link}, {4, person}, {5, selected_terms}, {6, kind}, {7, 1}});
        if (contract_screens_can_call_directly(entry)) {
            c.link(back);
            open_existing_negotiation(c, link, person, selected_terms, kind, 1);
            c.s.pc = back;
        } else {
            c.call(entry, back);
        }
    } else {
        constexpr auto entry = 0x100960d0u, back = 0x10096084u;
        leave(c, {{3, person}, {4, selected_terms}, {5, kind}, {6, 1}});
        if (contract_screens_can_call_directly(entry)) {
            c.link(back);
            open_new_negotiation(c, person, selected_terms, kind, 1);
            c.s.pc = back;
        } else {
            c.call(entry, back);
        }
    }
}
}

std::uint32_t negotiation_choice_command(Cpu& c, std::uint32_t pressed) {
    Frame frame(c, 0x10095db0, 0x60, {.first_gpr = 25});
    const auto screens = c.toc(screens_slot);
    keep(c, {{29, pressed}, {30, screens + 0x2c58}, {31, screens}});
    const auto group = screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
    const Controls controls{c, screens, c.u32(group + 0x24), std::int16_t(pressed)};
    c.set(3, controls.layout);
    Program program{c};
    if (compare(c, controls.action(4), 3) == 0) {
        program.call(set_history_value, 0x10095e54, {screens, 1, controls.value(), 0});
        return std::uint32_t(-11);
    }
    if (compare(c, controls.action(4), 0x2f) == 0) {
        program.call(set_history_value, 0x10095ed4, {screens, 4, controls.value(), 0});
        return std::uint32_t(-11);
    }
    if (compare(c, controls.action(4), 1) == 0) return std::uint32_t(-9);
    if (compare(c, controls.action(3), 2) != 0) {
        const auto handled = program.call(0x102a54b0, 0x10096094, {pressed});
        if (compare(c, handled) != 0) return handled;
        const auto common = program.call(0x102a83a0, 0x100960a8, {pressed});
        c.s.wr<1>(reg::xer_ca, common != 0 && common != 0x80000000);
        return common;
    }

    const auto person = program.call(history_value, 0x10095f78, {screens, 0});
    keep(c, {{28, person}});
    c.set(0, person);
    const auto selected = program.call(history_value, 0x10095f8c, {screens, 1});
    const std::int32_t term = std::int8_t(selected);
    const auto selected_terms = std::uint32_t(std::int8_t(term < 0 ? -term : term));
    // The original's leaf abs call leaves the sign mask and xor residue.
    const auto sign = term < 0 ? 0xffffffffu : 0u;
    c.set({{0, sign ^ std::uint32_t(term)}, {4, sign}});
    c.s.wr<1>(reg::xer_ca, term < 0);
    keep(c, {{27, selected_terms}});
    const auto link = program.call(history_value, 0x10095fa4, {screens, 2});
    keep(c, {{26, link}});
    const auto offer = compare(c, link) != 0 ? c.u32(link) : 0;
    keep(c, {{29, offer}});
    const auto existing = std::uint32_t(std::int8_t(program.call(history_value, 0x10095fc4, {screens, 3})));
    keep(c, {{25, existing}});
    const auto kind = std::uint32_t(std::int8_t(program.call(history_value, 0x10095fd4, {screens, 4})));
    const auto offset = std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
    const auto instance = c.u32(screens + offset + 4);
    c.set(0, offset);
    keep(c, {{30, kind}});
    program.call(0x1007bb70, 0x10096000, {c.u32(c.toc(contracts_slot)), person, instance, c.u32(instance + 0x24), selected_terms});
    const auto revision = program.call(history_value, 0x1009600c, {screens, 7});
    c.set(4, revision);
    if (needs_refresh(c, offer, revision)) {
        program.call(0x10422570, 0x10096044, {offer});
        return std::uint32_t(-11);
    }

    c.set(0, existing);
    proceed_to_negotiation(c, compare(c, existing) != 0, link, person, selected_terms, kind);
    return std::uint32_t(-4);
}
}
