#include "contract_screens.hpp"
#include "contract_screens_calls.hpp"
#include "contract_screens_controls.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Program;
constexpr std::uint32_t history_value = 0x1073ef70, set_history_value = 0x1073f1e0;
constexpr int screens_slot = -0x7bb0, contracts_slot = -0x7b7c;

using detail::Controls;
using detail::needs_refresh;

void proceed_to_negotiation(Cpu& c, bool existing, std::uint32_t link, std::uint32_t person,
                            std::uint32_t selected_terms, std::uint32_t kind) {
    if (existing)
        detail::call_owned(c, 0x100965c0, 0x1009606c, open_existing_negotiation, link, person, selected_terms, kind, 1u);
    else
        detail::call_owned(c, 0x100960d0, 0x10096084, open_new_negotiation, person, selected_terms, kind, 1u);
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

namespace {
constexpr std::uint32_t redraw = std::uint32_t(-11), navigate = std::uint32_t(-4);
constexpr std::uint32_t active_draft = 0x101487d0, refresh_offer = 0x10422570;

std::uint32_t confirm_negotiation(Cpu& c, Program& program, const Frame& frame,
                                  std::uint32_t screens, std::uint32_t drafts) {
    const auto draft = program.call(active_draft, 0x10097ef8, {c.u32(drafts), 0});
    c.set(26, draft);
    const auto link = program.call(history_value, 0x10097f08, {screens, 3});
    c.set({{27, 0}, {28, link}});
    const auto offer = compare(c, link) == 0 ? 0 : c.u32(link);
    c.set(27, offer);
    const auto history_byte = [&](std::uint32_t back, unsigned slot, unsigned kept) {
        const auto value = std::uint32_t(std::int8_t(program.call(history_value, back, {screens, slot})));
        c.set(kept, value);
        return value;
    };
    const auto kind = link != 0 ? c.u8(offer + 0x35) : history_byte(0x10097f2c, 7, 25);
    c.set(25, kind);
    const auto category = history_byte(0x10097f3c, 8, 24);
    const auto mode = history_byte(0x10097f4c, 0x1c, 30);
    const auto option = history_byte(0x10097f5c, 0x1d, 29);
    const auto year = history_byte(0x10097f6c, 0xb, 23);
    const auto day = std::uint16_t(program.call(history_value, 0x10097f7c, {screens, 0xc}));
    c.set(22, day);

    const auto interval = std::int16_t(program.call(0x107182f0, 0x10097f88, {draft + 0x28}));
    c.set(0, std::uint32_t(interval));
    const auto duration = compare(c, interval, 28) > 0 ? 28 :
        std::int16_t(program.call(0x107182f0, 0x10097fa4, {draft + 0x28}));
    c.set(0, std::uint32_t(duration));
    const auto date = frame.at(0x38);
    program.call(0x107156f0, 0x10097fc0, {date, std::uint32_t(duration), year, day, 0xffffffff});
    c.put16(draft + 0x32, c.u16(date + 2));
    c.put16(draft + 0x30, c.u16(date));
    const auto flags = c.u8(date + 4);
    c.set(0, flags);
    c.put8(draft + 0x34, flags);
    const auto revision = program.call(history_value, 0x10097fe4, {screens, 5});
    c.set(4, revision);
    if (needs_refresh(c, offer, revision)) {
        program.call(refresh_offer, 0x1009801c, {offer});
        return redraw;
    }
    detail::call_owned(c, 0x1009dab0, 0x10098040, open_offer_details, link, kind, category, 0u, option, mode);
    return navigate;
}

std::uint32_t inspect_negotiation(Cpu& c, Program& program, std::uint32_t screens, std::uint32_t drafts) {
    const auto link = program.call(history_value, 0x10097d58, {screens, 3});
    c.set({{26, link}, {27, 0}});
    const auto offer = compare(c, link) == 0 ? 0 : c.u32(link);
    c.set(27, offer);
    const auto allow_review = std::int8_t(program.call(history_value, 0x10097d74, {screens, 6}));
    c.set(0, std::uint32_t(allow_review));
    if (compare(c, allow_review) != 0) {
        const auto revision = program.call(history_value, 0x10097d88, {screens, 5});
        c.set(28, revision);
        if (needs_refresh(c, offer, revision)) {
            program.call(0x10411f10, 0x10097dc4, {c.u32(link), 1});
            c.set(0, 0);
            c.put32(link, 0);
            program.call(refresh_offer, 0x10097dd8, {offer, revision});
            return redraw;
        }
        const auto draft = program.call(active_draft, 0x10097dec, {c.u32(drafts), 0});
        program.call(0x10422ac0, 0x10097dfc, {link, c.u32(draft)});
    } else if (compare_logical(c, link) != 0) {
        program.call(0x1041ea60, 0x10097e1c, {link, 2, link});
    } else {
        detail::call_owned(c, 0x1009dab0, 0x10097e40, open_offer_details, 0u, 0u, 0xbu, 1u, 0u, 3u);
    }
    return navigate;
}
}

std::uint32_t negotiation_command(Cpu& c, std::uint32_t pressed) {
    Frame frame(c, 0x10097b70, 0x70, {.first_gpr = 22});
    const auto screens = c.toc(screens_slot), drafts = c.toc(-0x7a54);
    keep(c, {{27, pressed}, {30, drafts}, {31, screens}});
    const auto group = screen_manager::detail::Manager{c, screens}.group();
    const Controls controls{c, screens, c.u32(group + 0x24), std::int16_t(pressed)};
    c.set(3, controls.layout);
    Program program{c};
    if (compare(c, controls.action(4), 0xc) == 0) {
        program.call(set_history_value, 0x10097c14, {screens, 1, controls.value(), 0});
        return redraw;
    }
    if (compare(c, controls.action(4), 3) == 0) {
        program.call(set_history_value, 0x10097c94, {screens, 0, controls.value(), 0});
        program.call(set_history_value, 0x10097ca8, {screens, 2, 0, 0});
        return redraw;
    }
    if (compare(c, controls.action(4), 0x31) == 0) {
        program.call(0x106ff550, 0x10097d00, {0, 0});
        return navigate;
    }
    if (compare(c, controls.action(4), 2) == 0) return inspect_negotiation(c, program, screens, drafts);
    if (compare(c, controls.action(4), 0x1b) == 0) {
        const auto draft = program.call(active_draft, 0x10097e98, {c.u32(drafts), 0});
        c.set(0, 0xffffffff);
        c.put32(draft, 0xffffffff);
        return std::uint32_t(-10);
    }
    if (compare(c, controls.action(3), 1) == 0) return confirm_negotiation(c, program, frame, screens, drafts);

    // These subcommands are separate CONTRACT_SCREENS entries. The normal
    // dispatcher selects their bodies while their reconstruction is pending.
    using Handler = std::uint32_t (*)(Cpu&, std::uint32_t);
    struct Command { std::uint32_t entry, back; Handler handler; };
    constexpr Command commands[]{{0x1009a440, 0x10098050, nullptr},
                                 {0x1009c020, 0x10098064, negotiation_bonuses_command},
                                 {0x1009d410, 0x10098078, nullptr},
                                 {0x102a54b0, 0x1009808c, nullptr}};
    for (const auto& command : commands) {
        const auto handled = command.handler
            ? detail::call_owned(c, command.entry, command.back, command.handler, pressed)
            : program.call(command.entry, command.back, {pressed});
        if (compare(c, handled) != 0) return handled;
    }
    const auto common = program.call(0x102a83a0, 0x100980a0, {pressed});
    c.s.wr<1>(reg::xer_ca, common != 0 && common != 0x80000000);
    return common;
}
}
