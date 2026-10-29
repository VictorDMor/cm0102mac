#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
namespace {
std::uint32_t signed_byte(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }

// These confirmation screens share their frame and two history fields. The
// source entry and callback pair retain each original's call boundaries.
void open_two_fields(Cpu& c, std::uint32_t entry, int callbacks,
                     std::uint32_t primary, std::uint32_t secondary,
                     bool person_id, bool byte_mode) {
    Frame f(c, entry, 0x50, {.first_gpr = 29});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> mode(c, secondary);
    Kept<29> subject(c, primary); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, entry + 0x40,
        {screens, c.toc(callbacks), c.toc(callbacks - 4), 1, 0, 0}))) return;
    p.history(entry + 0x5c, screens, 0, person_id ? c.u32(subject) : std::uint32_t(subject));
    p.history(entry + 0x70, screens, 1, byte_mode ? signed_byte(mode) : std::uint32_t(mode));
}

void open_subject(Cpu& c, std::uint32_t entry, int callbacks, std::uint32_t subject) {
    Frame f(c, entry, 0x40, {.first_gpr = 30});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> selected(c, subject); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, entry + 0x38,
        {screens, c.toc(callbacks), c.toc(callbacks - 4), 1, 0, 0}))) return;
    p.history(entry + 0x54, screens, 0, selected);
}

std::uint32_t resolve_command(Cpu& c, std::uint32_t entry, std::uint32_t control,
                              int special_action, std::uint32_t special_result) {
    Frame f(c, entry, 0x40, {.first_gpr = 31});
    c.set(4, c.toc(slot::screens)); Kept<31> selected(c, control); Program p{c};
    const auto manager = c.r(4);
    const auto action = control_action(c, manager, control);
    if (compare(c, action, special_action) == 0) return special_result;
    const auto command = p.call(0x102a54b0, entry + 0x80, {selected});
    if (compare(c, command) != 0) return command;
    // The original's neg/or/sign-mask sequence preserves every 32-bit result.
    return p.call(0x102a83a0, entry + 0x94, {selected});
}
}

void open_bid_rejection(Cpu& c, std::uint32_t player, std::uint32_t offer) {
    Frame f(c, 0x1041f830, 0x50, {.first_gpr = 29});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> selected_offer(c, offer);
    Kept<29> person(c, player); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x1041f870,
        {screens, c.toc(-0x4ae0), c.toc(-0x4ae4), 1, 0, 0}))) return;
    p.history(0x1041f88c, screens, 0, person);
    p.history(0x1041f8a0, screens, 1, selected_offer);
    const auto count = p.call(0x103ea350, 0x1041f8b0, {indirect(c, slot::transfers), c.u32(person)});
    p.history(0x1041f8c8, screens, 2, extended(std::int16_t(count)));
}
void open_offer_restriction(Cpu& c, std::uint32_t first, std::uint32_t second, std::uint32_t reason) {
    Frame f(c, 0x10420aa0, 0x50, {.first_gpr = 28});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> restriction(c, reason);
    Kept<29> other(c, second); Kept<28> primary(c, first); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x10420ae8,
        {screens, c.toc(-0x4ae8), c.toc(-0x4aec), 1, 0, 0}))) return;
    p.history(0x10420b04, screens, 0, primary);
    p.history(0x10420b18, screens, 1, other);
    p.history(0x10420b2c, screens, 2, extended(std::int16_t(std::uint32_t(restriction))));
}
void open_remove_exchange_player(Cpu& c, std::uint32_t player, std::uint32_t offer) {
    open_two_fields(c, 0x10422ac0, -0x4b08, player, offer, false, false);
}
void open_transfer_completion_confirmation(Cpu& c, std::uint32_t link, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x10423160, 0x50, {.first_gpr = 28});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> news(c, item);
    Kept<29> recipient(c, manager); Kept<28> selected(c, link); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x104231a8,
        {screens, c.toc(-0x4b10), c.toc(-0x4b14), 1, 0, 0}))) return;
    p.history(0x104231c4, screens, 0, selected);
    p.history(0x104231d8, screens, 1, recipient);
    p.history(0x104231ec, screens, 2, news);
    p.history(0x10423200, screens, 3, 0);
    p.history(0x10423218, screens, 4, c.u32(c.u32(selected) + 0x2c));
    p.history(0x10423230, screens, 5, c.u32(c.u32(selected) + 12));
    p.history(0x10423244, screens, 6, 0);
}
void open_shortlist_confirmation(Cpu& c, std::uint32_t player, std::uint32_t mode) {
    open_two_fields(c, 0x104248a0, -0x4b18, player, mode, true, true);
}
void open_player_comparison_confirmation(Cpu& c, std::uint32_t player, std::uint32_t which) {
    Frame f(c, 0x10424930, 0x50, {.first_gpr = 29});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> second(c, which);
    Kept<29> person(c, player); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x10424970,
        {screens, c.toc(-0x4b20), c.toc(-0x4b24), 1, 0, 0}))) return;
    second = std::int8_t(std::uint32_t(second)) == 1;
    p.history(0x104249a4, screens, 0, c.u32(person));
    p.history(0x104249b8, screens, 1, signed_byte(second));
}
void open_withdraw_bid_confirmation(Cpu& c, std::uint32_t offer) {
    open_subject(c, 0x10426550, -0x4b28, offer);
}
void open_transfer_listing_confirmation(Cpu& c, std::uint32_t player, std::uint32_t mode) {
    open_two_fields(c, 0x1042a780, -0x4b50, player, mode, false, true);
}
void open_release_confirmation(Cpu& c, std::uint32_t player) {
    open_subject(c, 0x1042ae20, -0x4b58, player);
}
void open_trial_period_selection(Cpu& c, std::uint32_t player) {
    Frame f(c, 0x1042b500, 0x40, {.first_gpr = 30});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> person(c, player); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x1042b538,
        {screens, c.toc(-0x4b60), c.toc(-0x4b64), 1, 0, 0}))) return;
    p.history(0x1042b554, screens, 0, person);
    p.history(0x1042b568, screens, 1, 2);
}
void open_temporary_offer_confirmation(Cpu& c, std::uint32_t player, std::uint32_t mode) {
    Frame f(c, 0x1042ed80, 0x50, {.first_gpr = 29});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> selected_mode(c, mode);
    Kept<29> person(c, player); Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x1042edc0,
        {screens, c.toc(-0x4b78), c.toc(-0x4b7c), 1, 0, 0}))) return;
    p.history(0x1042eddc, screens, 0, person);
    p.history(0x1042edf0, screens, 1, signed_byte(selected_mode));
    for (unsigned i = 2; i <= 6; ++i) p.history(0x1042ee04 + (i - 2) * 20, screens, i, missing);
    p.history(0x1042ee68, screens, 7, 0);
}
void open_temporary_offer_terms_confirmation(Cpu& c, std::uint32_t player, std::uint32_t expiry_month,
    std::uint32_t expiry_year, std::uint32_t wage_contribution_percent, std::uint32_t cup_match_option, std::uint32_t parent_club_match_option, std::uint32_t offer_link) {
    Frame f(c, 0x1042ee90, 0x60, {.first_gpr = 24});
    Kept<25> month(c, expiry_month); Kept<26> end_year(c, expiry_year); Kept<27> contribution(c, wage_contribution_percent);
    Kept<28> cup_option(c, cup_match_option); Kept<29> parent_option(c, parent_club_match_option);
    Kept<31> screens(c, c.toc(slot::screens)); Kept<24> person(c, player); Kept<30> selected(c, offer_link);
    Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x1042eedc,
        {screens, c.toc(-0x4b78), c.toc(-0x4b7c), 1, 0, 0}))) return;
    p.history(0x1042eef8, screens, 0, person);
    p.history(0x1042ef0c, screens, 1, missing);
    p.history(0x1042ef20, screens, 2, signed_byte(month));
    p.history(0x1042ef34, screens, 3, std::uint16_t(std::uint32_t(end_year)));
    p.history(0x1042ef48, screens, 4, signed_byte(contribution));
    p.history(0x1042ef5c, screens, 5, signed_byte(cup_option));
    p.history(0x1042ef70, screens, 6, signed_byte(parent_option));
    p.history(0x1042ef84, screens, 7, selected);
}
void clear_exchange_candidates(Cpu& c) {
    Frame f(c, 0x10428650, 0x40, {.first_gpr = 31});
    Kept<31> screens(c, c.toc(slot::screens)); Program p{c};
    const auto candidates = p.call(0x1073ef70, 0x10428670, {screens, 4});
    if (compare_logical(c, candidates) == 0) return;
    p.call(0x10775170, 0x1042867c);
    p.history(0x10428690, screens, 4, 0); p.history(0x104286a4, screens, 5, 0);
}
std::uint32_t restriction_command(Cpu& c, std::uint32_t control) {
    return resolve_command(c, 0x10420e40, control, 1, std::uint32_t(-9));
}
std::uint32_t offer_command(Cpu& c, std::uint32_t control) {
    return resolve_command(c, 0x10421960, control, 1, std::uint32_t(-9));
}
std::uint32_t previous_offer_command(Cpu& c, std::uint32_t control) {
    return resolve_command(c, 0x10422a00, control, 3, std::uint32_t(-10));
}
std::uint32_t withdraw_bid_command(Cpu& c, std::uint32_t control) {
    Frame f(c, 0x10426830, 0x40, {.first_gpr = 31});
    c.set(4, c.toc(slot::screens)); Kept<31> selected(c, control); Program p{c};
    if (compare(c, control_action(c, c.r(4), selected), 1) == 0) return std::uint32_t(-10);
    if (compare(c, control_action(c, c.r(4), selected), 2) == 0) {
        const auto offer = p.call(0x1073ef70, 0x1042690c, {c.r(4), 0});
        p.call(0x103e44b0, 0x10426924, {indirect(c, slot::transfers), offer, 0, 0});
        return std::uint32_t(-10);
    }
    const auto command = p.call(0x102a54b0, 0x10426934, {selected});
    if (compare(c, command) != 0) return command;
    return p.call(0x102a83a0, 0x10426948, {selected});
}
std::uint32_t release_player_command(Cpu& c, std::uint32_t control) {
    Frame f(c, 0x1042b3a0, 0x40, {.first_gpr = 30});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> selected(c, control); Program p{c};
    if (compare(c, control_action(c, screens, selected), 1) == 0) return std::uint32_t(-10);
    if (compare(c, control_action(c, screens, selected), 2) == 0) {
        selected = p.call(0x1073ef70, 0x1042b480, {screens, 0});
        const auto reason = p.call(0x1073ef70, 0x1042b494, {screens, 1});
        p.call(0x103e6290, 0x1042b4ac, {indirect(c, slot::transfers), selected, 0, reason});
        return std::uint32_t(-10);
    }
    const auto command = p.call(0x102a54b0, 0x1042b4bc, {selected});
    if (compare(c, command) != 0) return command;
    return p.call(0x102a83a0, 0x1042b4d0, {selected});
}
void open_exchange_candidates(Cpu& c) {
    Frame f(c, 0x10426970, 0x40, {.first_gpr = 30});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<30> date(c, c.toc(slot::transfer_context));
    Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x104269a8,
        {screens, c.toc(-0x4b30), c.toc(-0x4b34), 0, c.toc(-0x4b38), 0}))) return;
    p.history(0x104269c4, screens, 0, missing);
    const auto month = p.call(0x10718360, 0x104269cc, {date});
    p.history(0x104269e4, screens, 1, signed_byte(month));
    p.history(0x104269f8, screens, 2, c.u16(std::uint32_t(date) + 2));
    p.history(0x10426a0c, screens, 3, 1);
    p.history(0x10426a20, screens, 4, 0);
    p.history(0x10426a34, screens, 5, 0);
    p.history(0x10426a48, screens, 6, 1);
    p.history(0x10426a5c, screens, 7, 1);
    p.history(0x10426a70, screens, 8, 0);
    p.history(0x10426a84, screens, 9, 0);
}
}
