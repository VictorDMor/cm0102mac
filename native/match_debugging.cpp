#include "match_debugging.hpp"
#include "aot_runtime.hpp"
#include "match_debugger_goal.hpp"
#include "match_debugger_cards.hpp"
#include "direct_commands.hpp"
#include "ui_bridge.hpp"
#include "debug_localization.hpp"
#include "debug_contract_date.hpp"
#include <array>
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>

namespace cm::aot {
namespace {
constexpr unsigned option_action = 30001, goal_action = 30002, repeat_action = 30003;
constexpr unsigned shortcut_first = 30010, shortcut_last = shortcut_first + 5;
constexpr unsigned best_xi_action = 30004, recover_squad_action = 30005;
constexpr unsigned extend_contract_action = 30006;
constexpr std::uint32_t control_table = 0xba864, control_stride = 0x17c;
std::optional<bool> pending_option;
struct RecentScorer { const State* state; std::uint32_t match, staff; unsigned player; };
std::optional<RecentScorer> recent;

// Calls made between original UI operations own a separate argument frame.
// Preserve the interrupted caller, including its LR, FP registers and scratch.
class GuestCall {
    State& s;
    decltype(State::registers) registers;
    decltype(State::unique) unique;
    std::uint32_t pc;
public:
    std::uint32_t stack;
    explicit GuestCall(State& state) : s(state), registers(s.registers), unique(s.unique), pc(s.pc),
        stack(std::uint32_t(s.r<4>(4)) - 0x400) {
        if (s.r<4>(4) < 0x800) throw std::runtime_error("Invalid debugging caller stack");
        (void)s.memory.copy_out(stack, 0x400);
        s.wr<4>(4, stack);
    }
    ~GuestCall() { s.registers = registers; s.unique = unique; s.pc = pc; }
    std::uint32_t call(std::uint32_t entry, std::initializer_list<std::uint32_t> values) {
        unsigned reg = 3;
        for (auto value : values) s.wr<4>(reg++ * 4, value);
        s.wr<4>(4128, return_sentinel); s.pc = entry;
        try { run_until(s, return_sentinel); }
        catch (const std::exception& error) {
            throw std::runtime_error("Debugging call " + hex_address(entry) + " at " + hex_address(s.pc) + ": " + error.what());
        }
        return std::uint32_t(s.r<4>(12));
    }
    void args(std::initializer_list<std::uint32_t> values) {
        unsigned offset = 0x38;
        for (auto value : values) { s.memory.write<4>(stack + offset, value); offset += 4; }
    }
    std::uint32_t text(std::string_view value) {
        if (value.size() > 200) throw std::runtime_error("Debug control label is too long");
        std::vector<std::uint8_t> bytes(value.begin(), value.end()); bytes.push_back(0);
        s.memory.copy_in(stack + 0x100, bytes);
        return stack + 0x100;
    }
};
std::uint32_t root(const State& s) { return s.memory.read<4>(0x108b3ee0); }
std::uint32_t screen(const State& s, std::uint32_t ui) {
    const auto manager = signed_value<2>(s.memory.read<2>(ui + 0x2c58));
    return s.memory.read<4>(ui + 0x24 + manager * 0x2c0);
}
std::uint32_t selected_control(const State& s) {
    const auto index = signed_value<2>(s.r<4>(12));
    const auto page = screen(s, root(s));
    const auto count = signed_value<2>(s.memory.read<2>(page + 0x129da6));
    if (index < 0 || index >= count || count > 1200) return 0;
    return page + control_table + unsigned(index) * control_stride;
}
std::string original_text(const State& s, std::uint32_t address) {
    std::string result;
    if (address) for (unsigned i = 0; i < 104; ++i) {
        const auto c = char(s.memory.read<1>(address + i));
        if (!c) return result;
        result += c;
    }
    return result;
}
std::string player_name(const State& s, std::uint32_t staff, bool short_name) {
    auto common = original_text(s, s.memory.read<4>(staff + 0x30));
    if (!common.empty()) return common;
    const auto first = original_text(s, s.memory.read<4>(staff + 0x28));
    const auto last = original_text(s, s.memory.read<4>(staff + 0x2c));
    if (short_name) return last.empty() ? first : last;
    return first.empty() ? last : last.empty() ? first : first + " " + last;
}
std::uint32_t actor_address(std::uint32_t match, unsigned player) { return match + 0x311c + player * 0x1c4; }
bool on_pitch(const State& s, std::uint32_t match, unsigned player) {
    if (!match || player >= 40) return false;
    const auto actor = actor_address(match, player);
    const auto position = signed_value<1>(s.memory.read<1>(actor + 0x19));
    return s.memory.read<4>(actor + 0x6c) && position >= 0 && position < 11 &&
        s.memory.read<1>(actor + 0x27) == player / 20 && s.memory.read<4>(actor + 0x1a4) == match &&
        !s.memory.read<1>(actor + 0x23);
}
bool active_player(const State& s, std::uint32_t match, unsigned player) {
    return on_pitch(s, match, player) && s.memory.read<1>(match) == 0 &&
        signed_value<2>(s.memory.read<2>(match + 0x7948)) > 0 && !s.memory.read<1>(match + 0x7929);
}
std::uint32_t opposing_player(const State& s, std::uint32_t match, unsigned player) {
    const unsigned begin = player < 20 ? 20 : 0;
    for (unsigned other = begin; other < begin + 20; ++other)
        if (on_pitch(s, match, other)) return actor_address(match, other);
    return 0;
}
bool recent_valid(const State& s, std::uint32_t match) {
    return recent && recent->state == &s && recent->match == match &&
        eligible_debug_goal(s, match, recent->player) &&
        s.memory.read<4>(actor_address(match, recent->player) + 0x6c) == recent->staff;
}
}

bool eligible_debug_goal(const State& s, std::uint32_t match, unsigned player) {
    try {
        if (!active_player(s, match, player)) return false;
        const auto actor = actor_address(match, player);
        const auto score = signed_value<1>(s.memory.read<1>(match + (player < 20 ? 0xe03d : 0xe073)));
        return score >= 0 && score < 127 && s.memory.read<1>(actor + 0xc) < 255;
    } catch (const std::runtime_error&) { return false; }
}

unsigned debug_player_weight(const State& s, std::uint32_t match, unsigned player, DebugMatchEvent event) {
    try {
        if (event == DebugMatchEvent::red_card)
            return active_player(s, match, player) && opposing_player(s, match, player) ? 1 : 0;
        if (event == DebugMatchEvent::injury)
            return active_player(s, match, player) ? 1 : 0;
        if (!eligible_debug_goal(s, match, player)) return 0;
        const auto position = s.memory.read<1>(actor_address(match, player) + 0x19);
        const auto roles = s.memory.read<4>(match + 0x7934 + (player / 20) * 4);
        const auto role = s.memory.read<2>(roles + position * 2);
        if (position == 0 || (role & 1)) return 0;
        // Original formation bits: forward line 0x40, central lanes
        // 0x100/0x200/0x400 (including the two strikers in a 4-4-2).
        return (role & 0x40) && (role & 0x700) ? 13 : 10;
    } catch (const std::runtime_error&) { return 0; }
}

std::optional<unsigned> random_debug_player(const State& s, std::uint32_t match, unsigned side, DebugMatchEvent event) {
    if (side > 1) return {};
    std::array<unsigned, 20> weights{};
    unsigned total = 0;
    for (unsigned i = 0; i < weights.size(); ++i) {
        weights[i] = debug_player_weight(s, match, side * 20 + i, event);
        total += weights[i];
    }
    if (!total) return {};
    // Host randomness leaves the original simulation RNG untouched by selection.
    auto ticket = arc4random_uniform(total);
    for (unsigned i = 0; i < weights.size(); ++i) {
        if (ticket < weights[i]) return side * 20 + i;
        ticket -= weights[i];
    }
    return {};
}

void award_debug_red_card(State& s, std::uint32_t match, unsigned player) {
    if (!native_debugging_enabled() || !debug_player_weight(s, match, player, DebugMatchEvent::red_card))
        throw std::invalid_argument("Debug red card requires an enabled, active match and an on-pitch player");
    const auto actor = actor_address(match, player), opponent = opposing_player(s, match, player);
    const auto home = s.memory.read<1>(match + 0xe03d), away = s.memory.read<1>(match + 0xe073);
    const auto clock = s.memory.read<2>(match + 0x7948);
    {
        GuestCall caller(s);
        match_debugger::apply_original_card(s, match, actor, opponent, false);
    }
    if (s.memory.read<1>(actor + 0x23) != 1 || signed_value<1>(s.memory.read<1>(actor + 0x19)) != -3 ||
        s.memory.read<1>(match + 0xe03d) != home || s.memory.read<1>(match + 0xe073) != away ||
        s.memory.read<2>(match + 0x7948) != clock)
        throw std::runtime_error("Debug red card failed original dismissal, score or clock bookkeeping");
}

unsigned random_debug_injury() {
    // Original injury types 0..76; 77..81 are recovery/fitness statuses.
    return arc4random_uniform(77);
}

void award_debug_injury(State& s, std::uint32_t match, unsigned player, unsigned injury) {
    if (!native_debugging_enabled() || injury >= 77 ||
        !debug_player_weight(s, match, player, DebugMatchEvent::injury))
        throw std::invalid_argument("Debug injury requires an enabled, active match, an on-pitch player and an injury type");
    const auto actor = actor_address(match, player);
    const auto staff = std::uint32_t(s.memory.read<4>(actor + 0x6c));
    const auto controller = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7ba0)));
    const auto home = s.memory.read<1>(match + 0xe03d), away = s.memory.read<1>(match + 0xe073);
    const auto clock = s.memory.read<2>(match + 0x7948), red = s.memory.read<1>(actor + 0x23);
    {
        GuestCall call(s);
        // The original medical record treats an unfinished fixture's -1 score
        // as a training injury. Supply a private fixture view with the current
        // score, without marking the live fixture complete or changing it.
        const auto context = call.stack + 0x200;
        s.memory.copy_in(context, s.memory.copy_out(s.memory.read<4>(match + 0x3118), 0x58));
        for (const auto offset : {0x43u, 0x47u}) {
            s.memory.write<1>(context + offset, home);
            s.memory.write<1>(context + offset + 1, away);
        }
        // Use the same retirement/registration sequence as original post-match
        // injury handling. The original owns duration, medical traits and notices.
        call.call(0x1018b9f0, {controller, staff, 0});
        call.args({0});
        const auto record = call.call(0x10188cc0, {controller, injury, 0, staff, context, 1, 0xffffffff, 0});
        if (record == 0xffffffff || s.memory.read<1>(s.memory.read<4>(controller + 4) + record * 0x38) != injury)
            throw std::runtime_error("Original injury registration did not assign the selected injury");
        match_debugger::apply_original_injury(s, match, actor);
    }
    if (!s.memory.read<1>(actor + 0x1c) || s.memory.read<1>(actor + 0x1d) ||
        signed_value<1>(s.memory.read<1>(actor + 0x19)) >= 0 || s.memory.read<1>(actor + 0x23) != red ||
        s.memory.read<1>(match + 0xe03d) != home || s.memory.read<1>(match + 0xe073) != away ||
        s.memory.read<2>(match + 0x7948) != clock)
        throw std::runtime_error("Debug injury failed original withdrawal, discipline, score or clock bookkeeping");
}

void award_debug_goal(State& s, std::uint32_t match, unsigned player) {
    if (!native_debugging_enabled() || !eligible_debug_goal(s, match, player))
        throw std::invalid_argument("Debug goal requires an enabled, active match and an on-pitch player");
    const auto actor = actor_address(match, player);
    const auto home = s.memory.read<1>(match + 0xe03d), away = s.memory.read<1>(match + 0xe073);
    const auto scorer = s.memory.read<1>(actor + 0xc), clock = s.memory.read<2>(match + 0x7948);
    {
        GuestCall caller(s);
        match_debugger::apply_original_awarded_goal(s, match, actor, player / 20);
    }
    if (s.memory.read<1>(match + 0xe03d) != home + (player < 20) ||
        s.memory.read<1>(match + 0xe073) != away + (player >= 20) ||
        s.memory.read<1>(actor + 0xc) != scorer + 1 || s.memory.read<2>(match + 0x7948) != clock)
        throw std::runtime_error("Debug goal failed original score, scorer or clock bookkeeping");
}

bool deliver_match_shortcut(State& s, const CommandContext& expected, const UIEvent& event) {
    if (event.kind != UIEvent::Kind::key_down && event.kind != UIEvent::Kind::auto_key) return false;
    if (event.modifiers & 0x1900) return false; // Command, Option, Control.
    const auto character = event.message & 0xff;
    const bool toggle = character == 'z' || character == 'Z';
    if (!toggle && (character < '1' || character > '6' || !native_debugging_enabled())) return false;
    const auto current = original_command_context(s, 0xffff);
    if (!current.ready || current.handler != 0x1028dc80 || current != expected) return false;
    // Both number rows arrive with the same character and different hardware
    // codes. A held key must never inject multiple events or toggle repeatedly.
    if (event.kind == UIEvent::Kind::auto_key) return true;
    if (toggle) {
        for (const auto& control : original_controls(s, current))
            if (control.action == 2 && (control.flags & 2))
                return deliver_original_control(s, current, control.index);
        return false;
    }
    clear_previous_command_key(s);
    s.memory.write<2>(current.root + 0x25819e, shortcut_first + character - '1');
    return true;
}

void reset_match_debugging() { recent.reset(); }
void begin_debugging_options() { pending_option = native_debugging_enabled(); }

void append_debugging_option(State& s) {
    if (!native_debugging_available()) return;
    if (!pending_option) pending_option = native_debugging_enabled();
    // At the end of the original options builder, these are its live grid,
    // first-row index, UI root and theme colors. The grid has one extra row.
    const auto grid = std::uint32_t(s.r<4>(13 * 4)), row = std::uint32_t(s.r<4>(17 * 4)) + 14;
    const auto ui = std::uint32_t(s.r<4>(29 * 4));
    const auto normal = s.memory.read<2>(s.r<4>(2 * 4) - 0x3890);
    const auto selected = s.memory.read<2>(s.r<4>(27 * 4));
    const auto title = s.memory.read<2>(s.memory.read<4>(s.r<4>(2 * 4) - 0x7b10));
    GuestCall call(s);
    call.args({std::uint32_t(title), call.text(" Debugging")});
    call.call(0x107445a0, {ui, grid, 0, row, 1, 0, 1, 3});
    for (unsigned value = 0; value != 2; ++value) {
        const bool active = *pending_option == bool(value);
        call.args({std::uint32_t(active ? selected : normal), call.text(debug_text(s, value ? DebugText::on : DebugText::off)), option_action, value});
        call.call(active ? 0x107445a0 : 0x107444e0,
            {ui, grid, value + 1, row, active ? 0x221u : 1u, active ? std::uint32_t(selected) : 0u, 12, 2});
    }
}

bool dispatch_debugging_option(State& s) {
    if (!native_debugging_available()) return false;
    const auto control = selected_control(s);
    auto action = control ? s.memory.read<4>(control + 8) : 0;
    if (!action) action = s.memory.read<2>(root(s) + 0x25819e);
    if (action == option_action && control && (s.memory.read<4>(control + 12) & 2)) {
        pending_option = s.memory.read<4>(control + 0x48) != 0;
        s.wr<4>(12, std::uint32_t(-11)); s.pc = s.r<4>(4128);
        return true;
    }
    if (action == 41) { select_native_debugging(pending_option.value_or(native_debugging_enabled())); pending_option.reset(); }
    if (action == 40) pending_option.reset();
    if (action == 39) pending_option = false;
    return false;
}

void append_match_debugging_controls(State& s) {
    if (!native_debugging_enabled()) return;
    // 1028ce80: the original sidebar grid is still r22, before the speed menu.
    const auto fixture = std::uint32_t(s.r<4>(31 * 4));
    const auto match = std::uint32_t(s.memory.read<4>(fixture + 0x58));
    const auto ui = std::uint32_t(s.r<4>(29 * 4)), page = screen(s, ui);
    const auto sidebar = std::uint32_t(s.r<4>(22 * 4));
    const auto color = std::uint32_t(s.memory.read<2>(s.r<4>(25 * 4)));
    const auto background = std::uint32_t(s.memory.read<2>(s.r<4>(27 * 4)));
    bool active = false;
    for (unsigned player = 0; player < 40; ++player) active |= eligible_debug_goal(s, match, player);
    GuestCall call(s);
    call.args({0x1021, 0, 0, 12, 1, color, call.text(debug_text(s, DebugText::add_goal)), 0, 0, 0, sidebar});
    const auto menu = call.call(0x10719740, {page, active ? 0x82u : 0x80u, 0, 0, 0, 0, 0, 6});
    call.args({0, 0x30, background, menu});
    const auto group = call.call(0x107198c0, {page, 0, 0, 0, 0, 0, 0, 0});
    for (unsigned player = 0; player < 40; ++player) {
        if (!eligible_debug_goal(s, match, player)) continue;
        const auto staff = std::uint32_t(s.memory.read<4>(actor_address(match, player) + 0x6c));
        const auto club = std::uint32_t(s.memory.read<4>(fixture + (player < 20 ? 0x20 : 0x24)));
        const auto label = original_text(s, club + 0x221) + " - " + player_name(s, staff, false);
        call.args({0x10, background, 0, 1, 1, color, call.text(label), 0, goal_action, player, group});
        call.call(0x10719740, {page, 2, 0, 0, 0, 0, 0, 0});
    }
    const bool repeat = recent_valid(s, match);
    const auto goal = std::string(debug_text(s, DebugText::goal));
    const auto label = recent && recent->state == &s && recent->match == match ?
        goal + " " + player_name(s, recent->staff, true) : goal;
    call.args({0x1021, 0, 0, 12, 1, color, call.text(label), 0, repeat_action, 0, sidebar});
    call.call(0x10719740, {page, repeat ? 2u : 0u, 0, 0, 0, 0, 0, 7});
}

bool dispatch_match_debugging_control(State& s) {
    const auto control = selected_control(s);
    const auto mailbox = s.memory.read<2>(root(s) + 0x25819e);
    const bool shortcut = mailbox >= shortcut_first && mailbox <= shortcut_last;
    const auto action = shortcut ? mailbox : control ? s.memory.read<4>(control + 8) : 0;
    if (!shortcut && action != goal_action && action != repeat_action) return false;
    if (shortcut) s.memory.write<2>(root(s) + 0x25819e, 0);
    if (!native_debugging_enabled()) {
        s.wr<4>(12, 0); s.pc = s.r<4>(4128);
        return true;
    }
    const unsigned number = shortcut ? unsigned(action - shortcut_first) : 0;
    const auto event = number < 2 ? DebugMatchEvent::goal : number < 4 ? DebugMatchEvent::red_card : DebugMatchEvent::injury;
    // Odd keys favor home: 1 scores for home, 3/5 penalize away.
    unsigned player = shortcut ? (number < 2 ? number : 1 - number % 2) : s.memory.read<4>(control + 0x48);
    bool accepted = false;
    {
        GuestCall call(s);
        const auto fixture = call.call(0x1073ef70, {root(s), 0});
        const auto match = std::uint32_t(s.memory.read<4>(fixture + 0x58));
        const auto status = std::uint32_t(s.memory.read<4>(fixture + 0x5c));
        if (action == repeat_action && recent_valid(s, match)) player = recent->player;
        else if (action == repeat_action) player = 40;
        if ((shortcut || (s.memory.read<4>(control + 12) & 2)) && status && !s.memory.read<1>(status + 0x56)) {
            if (shortcut)
                player = random_debug_player(s, match, player, event).value_or(40);
            const bool eligible = shortcut ? debug_player_weight(s, match, player, event) != 0 :
                eligible_debug_goal(s, match, player);
            if (eligible) {
                if (event == DebugMatchEvent::injury) award_debug_injury(s, match, player, random_debug_injury());
                else if (event == DebugMatchEvent::red_card) award_debug_red_card(s, match, player);
                else {
                    award_debug_goal(s, match, player);
                    if (!shortcut) recent = RecentScorer{&s, match,
                        std::uint32_t(s.memory.read<4>(actor_address(match, player) + 0x6c)), player};
                }
                // Publish original event/minute rows before the screen rebuild.
                const auto report = std::uint32_t(s.memory.read<4>(fixture + 0x60));
                const auto reports = s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7b4c));
                call.call(0x1029f920, {std::uint32_t(reports), match, report, 1});
                accepted = true;
            }
        }
    }
    // The original handler's -11 result rebuilds the screen, as speed changes do.
    s.wr<4>(12, std::uint32_t(accepted ? -11 : 0)); s.pc = s.r<4>(4128);
    return true;
}

bool select_debug_best_xi(State& s, std::uint32_t club, unsigned managed,
                         std::uint32_t formation, std::uint32_t fixture) {
    if (!native_debugging_enabled() || !club || !formation || managed > 1) return false;
    GuestCall call(s);
    const auto candidates = call.stack + 0x100, roles = call.stack + 0x200, output = call.stack + 0x240;
    const auto bench = fixture ? std::min<unsigned>(s.memory.read<1>(fixture + 0x4e) >> 4, 9) : 9;
    std::vector<std::uint32_t> available;
    for (unsigned i = 0; i < 50; ++i) {
        const auto staff = std::uint32_t(s.memory.read<4>(club + 0x98 + i * 4));
        if (!staff || !s.memory.read<4>(staff + 0x14) ||
            std::find(available.begin(), available.end(), staff) != available.end()) continue;
        // The same eligibility/status operation used by the squad screen:
        // ineligible, unregistered, international duty, suspended, injured,
        // recovering and absent players cannot be picked.
        std::uint32_t status;
        try { status = call.call(0x10060c00, {staff, club, fixture}); }
        catch (const std::exception& error) {
            throw std::runtime_error("Best XI availability at " + hex_address(s.pc) + ": " + error.what());
        }
        if ((status >= 1 && status <= 6) || status == 8 || status == 21) continue;
        available.push_back(staff);
    }
    for (unsigned i = 0; i < available.size(); ++i) s.memory.write<4>(candidates + i * 4, available[i]);
    for (unsigned i = 0; i < 11; ++i)
        s.memory.write<2>(roles + i * 2, s.memory.read<2>(formation + 0x12e + i * 2));
    // The original selector chooses unused reserves with positional coverage.
    constexpr std::array<unsigned, 9> reserve_roles{1, 4, 16, 64, 16, 4, 64, 16, 4};
    for (unsigned i = 0; i < bench; ++i) s.memory.write<2>(roles + (11 + i) * 2, reserve_roles[i]);
    // All optional inputs are explicitly absent; never inherit caller stack data.
    call.args({0, 0, 0, 0, 0, 0, 0});
    call.call(0x10210c50, {candidates, unsigned(available.size()), roles, 11, bench, output, 0x2014, club});
    std::array<std::uint32_t, 20> selected{};
    for (unsigned i = 0; i < 11 + bench; ++i) {
        const auto staff = std::uint32_t(s.memory.read<4>(output + i * 4));
        if (!staff) continue;
        if (std::find(available.begin(), available.end(), staff) == available.end() ||
            std::find(selected.begin(), selected.begin() + i, staff) != selected.begin() + i)
            throw std::runtime_error("Best XI returned an unavailable or duplicate player");
        selected[i] = staff;
    }
    const auto tactics = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7a6c)));
    for (unsigned i = 0; i < selected.size(); ++i) {
        if (selected[i]) call.call(0x103af9d0, {tactics, club, i, selected[i], managed});
        else call.call(0x103afe90, {tactics, club, i, managed});
    }
    for (unsigned i = 0; i < 11; ++i)
        call.call(0x103b17b0, {tactics, club, i, selected[i] ? i : 0xffffffffu, managed});
    return true;
}

bool recover_debug_squad(State& s, std::uint32_t club) {
    if (!native_debugging_enabled() || !club) return false;
    const auto medical = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7ba0)));
    const auto discipline = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7aa4)));
    if (!medical || !discipline) return false;
    const auto staff_count = signed_value<4>(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7be4)));
    std::vector<std::uint32_t> players;
    for (unsigned i = 0; i < 50; ++i) {
        const auto staff = std::uint32_t(s.memory.read<4>(club + 0x98 + i * 4));
        if (!staff || !s.memory.read<4>(staff + 0x14)) continue;
        const auto id = signed_value<4>(s.memory.read<4>(staff));
        if (id >= 0 && id < staff_count && std::find(players.begin(), players.end(), staff) == players.end())
            players.push_back(staff);
    }
    GuestCall call(s);
    for (const auto staff : players) {
        const auto id = std::uint32_t(s.memory.read<4>(staff));
        // Retire through the original medical operation so the live slot and
        // completed history stay consistent. A full record reset erases history.
        call.call(0x1018b9f0, {medical, staff, 0});
        const auto queue = std::uint32_t(s.memory.read<4>(medical + 0x28));
        if (queue)
            while (const auto update = call.call(0x10186e10, {medical, staff}))
                call.call(0x10738e60, {queue, update});
        const auto condition = std::uint32_t(s.memory.read<4>(medical)) + id * 0x24;
        s.memory.copy_in(condition, s.memory.copy_out(s.memory.read<4>(s.r<4>(8) - 0x7b80), 5));
        s.memory.write<2>(condition + 8, 10000);
        s.memory.write<2>(condition + 10, 10000);
        s.memory.write<2>(condition + 12, 0);
        s.memory.write<1>(condition + 0x14, 0xff); // No deferred medical risk.
        s.memory.write<1>(condition + 0x15, 0);
        s.memory.write<1>(condition + 0x20, 0);
        s.memory.write<1>(condition + 0x21, 0xff);
        // Each player has four competition categories. Removing an active ban
        // through the original operation also detaches every linked category.
        for (unsigned category = 0; category < 4; ++category) {
            const auto index = std::uint32_t(s.memory.read<4>(discipline + 0x14)) + id * 0x2c + category * 8;
            const auto group = signed_value<1>(s.memory.read<1>(index));
            if (group < 0) continue;
            const auto rules = std::uint32_t(s.memory.read<4>(s.memory.read<4>(discipline) + unsigned(group) * 4));
            const auto record = call.call(0x105bc2b0, {rules, id, 0});
            if (record) {
                const auto ban = signed_value<2>(s.memory.read<2>(record + 6));
                if (ban >= 0) call.call(0x100e0090, {discipline, id, unsigned(ban)});
            }
        }
        // Cancel deferred bans too, retaining disciplinary history and cards.
        call.call(0x100df950, {discipline, staff});
    }
    return true;
}

void append_best_xi_control(State& s) {
    if (!native_debugging_enabled()) return;
    // Squad builder 1001f6a0, after its column heading advanced r14 by 25.
    // Six equal 110-pixel controls with two-pixel gaps on the original row.
    const auto club = std::uint32_t(s.r<4>(25 * 4));
    const auto ui = std::uint32_t(s.r<4>(31 * 4));
    const auto top = std::uint32_t(s.r<4>(14 * 4)) - 25;
    GuestCall call(s);
    if (!call.call(0x1014c490, {club, 0, 1}) || (call.call(0x1073ef70, {ui, 7}) & 0x800)) return;
    const auto page = screen(s, ui);
    const auto count = s.memory.read<2>(page + 0x129da4);
    // Original two-cell View/Sort and Clear Team/Filter grids. The ordinary
    // layout publishes their drawing and hit rectangles from these bounds.
    for (unsigned i = 0; i < count && i < 250; ++i) {
        const auto grid = page + 4 + i * 0xbf0;
        if (s.memory.read<4>(grid + 4) != top || s.memory.read<4>(grid + 12) != top + 20 ||
            s.memory.read<1>(grid + 0xbb0) != 2 || s.memory.read<1>(grid + 0xbb1) != 1) continue;
        const auto left = s.memory.read<4>(grid), right = s.memory.read<4>(grid + 8);
        if (left == 110 && right == 360) s.memory.write<4>(grid + 8, 332);
        else if (left == 530 && right == 780) s.memory.write<4>(grid, 558);
    }
    const auto ink = std::uint32_t(s.memory.read<2>(s.memory.read<4>(s.r<4>(8) - 0x7bc4)));
    const auto paper = std::uint32_t(s.memory.read<2>(s.memory.read<4>(s.r<4>(8) - 0x7bc8)));
    call.args({0, 1, 0, 0xffffffffu});
    const auto grid = call.call(0x107198c0, {page, 334, top, 444, top + 20, 1, 0, 1});
    const auto best_label = debug_text(s, DebugText::best_xi);
    call.args({paper, call.text(best_label), best_xi_action, 0});
    call.call(0x107444e0, {ui, grid, 0, 0, 0x30, ink, 12, 1});
    call.args({0, 1, 0, 0xffffffffu});
    const auto recovery_grid = call.call(0x107198c0, {page, 446, top, 556, top + 20, 1, 0, 1});
    const auto recovery_label = debug_text(s, DebugText::recover_squad);
    call.args({paper, call.text(recovery_label), recover_squad_action, 0});
    call.call(0x107444e0, {ui, recovery_grid, 0, 0, 0x30, ink, 12, 1});
}

bool dispatch_best_xi_control(State& s) {
    if (!native_debugging_enabled()) return false;
    const auto control = selected_control(s);
    if (!control) return false;
    const auto action = s.memory.read<4>(control + 8);
    if (action != best_xi_action && action != recover_squad_action) return false;
    bool accepted = false;
    if (native_debugging_enabled() && (s.memory.read<4>(control + 12) & 2)) {
        GuestCall call(s);
        const auto ui = root(s), club = call.call(0x1073ef70, {ui, 0});
        const auto managed = call.call(0x1073ef70, {ui, 1});
        const auto view = call.call(0x1073ef70, {ui, 4});
        const auto columns = call.call(0x1073ef70, {ui, 7});
        if (view == 1 && !(columns & 0x800) && call.call(0x1014c490, {club, 0, 1})) {
            if (action == recover_squad_action) accepted = recover_debug_squad(s, club);
            else {
                const auto humans = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7a54)));
                const auto formation = call.call(0x101466c0, {humans, 0});
                const auto fixture = call.call(0x1073ef70, {ui, 3});
                accepted = select_debug_best_xi(s, club, managed, formation, fixture);
            }
        }
    }
    s.wr<4>(12, std::uint32_t(accepted ? -11 : 0)); s.pc = s.r<4>(4128);
    return true;
}

namespace {
struct ContractProposal {
    std::uint32_t origin, revision, club, offer;
};

// The original collection has an intrusive cursor. Leave it as it was when
// inspecting the list; unlinking is done with the original collection service.
std::vector<std::uint32_t> contract_rows(State& s, GuestCall& call, std::uint32_t list) {
    std::vector<std::uint32_t> rows;
    if (!list) return rows;
    const auto count = signed_value<2>(s.memory.read<2>(list));
    if (count < 0) throw std::runtime_error("Invalid contract proposal list");
    const auto previous = std::uint32_t(s.memory.read<4>(list + 12));
    struct Restore {
        State& state; std::uint32_t list, cursor;
        ~Restore() { state.memory.write<4>(list + 12, cursor); }
    } restore{s, list, previous};
    s.memory.write<4>(list + 12, s.memory.read<4>(list + 4));
    for (std::int64_t i = 0; i < count; ++i) {
        const auto row = call.call(0x10738e00, {list});
        if (!row) throw std::runtime_error("Truncated contract proposal list");
        rows.push_back(row);
    }
    if (call.call(0x10738e00, {list})) throw std::runtime_error("Invalid contract proposal count");
    return rows;
}

std::uint32_t debug_primary_contract(State& s, GuestCall& call, std::uint32_t staff) {
    const auto toc = std::uint32_t(s.r<4>(8));
    if (!staff || !s.memory.read<4>(staff + 0x14)) return 0;
    const auto id = std::uint32_t(s.memory.read<4>(staff));
    const auto count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be4)));
    if (id >= count) return 0;
    const auto manager = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b7c)));
    return manager ? call.call(0x10082f20, {manager, staff}) : 0;
}
}

bool extend_debug_contract(State& s, std::uint32_t staff) {
    if (!native_debugging_enabled()) return false;
    GuestCall call(s);
    const auto contract = debug_primary_contract(s, call, staff);
    if (!contract) return false;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto manager = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b7c)));
    const auto transfers = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b40)));
    if (!transfers) return false;
    const auto id = std::uint32_t(s.memory.read<4>(staff));
    const auto employer = std::uint32_t(s.memory.read<4>(contract + 4));
    const auto club_count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b50)));
    if (employer >= club_count) return false;
    const cm::calendar::Date expiry{std::int16_t(s.memory.read<2>(contract + 0x30)),
                                    std::uint16_t(s.memory.read<2>(contract + 0x32))};
    const auto next = extend_contract_date(expiry);
    if (!next || s.memory.copy_out(contract + 0x30, 5) != s.memory.copy_out(staff + 0x54, 5) ||
        s.memory.read<1>(contract + 0x34) != cm::calendar::is_leap(expiry.year)) return false;

    const auto pending = std::uint32_t(s.memory.read<4>(manager + 0x34));
    const auto deferred = std::uint32_t(s.memory.read<4>(manager + 0x38));
    const auto pending_rows = contract_rows(s, call, pending);
    const auto deferred_rows = contract_rows(s, call, deferred);
    const auto slots = std::uint32_t(s.memory.read<4>(transfers));
    const auto capacity = std::uint32_t(s.memory.read<4>(transfers + 4));
    std::vector<ContractProposal> selected;
    const auto consider = [&](std::uint32_t row) {
        if (s.memory.read<4>(row) != id || s.memory.read<1>(row + 0x64) != 0) return;
        const auto club = std::uint32_t(s.memory.read<4>(row + 4));
        const auto index = std::uint32_t(s.memory.read<4>(row + 0x70));
        const auto revision = std::uint32_t(s.memory.read<4>(row + 0x74));
        if (club == employer || club >= club_count || index >= capacity) return;
        const auto offer = std::uint32_t(s.memory.read<4>(slots + index * 4));
        if (!offer || s.memory.read<4>(offer + 0x28) != index ||
            s.memory.read<4>(offer + 0x2c) != revision || s.memory.read<4>(offer + 12) != id ||
            s.memory.read<4>(offer + 8) != club || s.memory.read<1>(offer + 0x24) != 0 ||
            !s.memory.read<1>(offer + 0x37) || s.memory.read<1>(offer + 0x10) == 2) return;
        const auto timing = s.memory.read<1>(offer + 0x38) >> 4;
        const auto status = s.memory.read<1>(offer + 0x34);
        if ((timing != 3 && timing != 4) || (status >= 11 && status <= 14) || status >= 19) return;
        if (std::none_of(selected.begin(), selected.end(), [index](const auto& item) { return item.origin == index; }))
            selected.push_back({index, revision, club, offer});
    };
    for (auto row : pending_rows) consider(row);
    for (auto row : deferred_rows) consider(row);

    // The original deletion looks up a pending row by person and club. Reject
    // an ambiguous pair before any writes, and do not remove exchange players.
    for (const auto& item : selected) {
        unsigned matching_pending = 0;
        for (auto row : pending_rows)
            if (s.memory.read<4>(row) == id && s.memory.read<4>(row + 4) == item.club) {
                if (s.memory.read<4>(row + 0x70) != item.origin || s.memory.read<4>(row + 0x74) != item.revision ||
                    s.memory.read<1>(row + 0x64) != 0 || ++matching_pending > 1) return false;
            }
        const auto exchange = std::uint32_t(s.memory.read<4>(item.offer + 4));
        if (exchange && s.memory.read<2>(exchange)) return false;
        const auto secondary = std::uint32_t(s.memory.read<4>(item.offer + 0x30));
        if (secondary != 0xffffffffu && (secondary >= s.memory.read<4>(transfers + 0x10) ||
            !s.memory.read<4>(s.memory.read<4>(transfers + 0xc) + secondary * 4))) return false;
    }
    for (const auto& item : selected) {
        for (;;) {
            const auto rows = contract_rows(s, call, deferred);
            const auto found = std::find_if(rows.begin(), rows.end(), [&](auto row) {
                return s.memory.read<4>(row) == id && s.memory.read<4>(row + 4) == item.club &&
                    s.memory.read<4>(row + 0x70) == item.origin && s.memory.read<4>(row + 0x74) == item.revision;
            });
            if (found == rows.end()) break;
            call.call(0x10738e60, {deferred, *found});
        }
        call.call(0x103fb630, {transfers, item.offer});
    }
    for (const auto target : {contract + 0x30, staff + 0x54}) {
        s.memory.write<2>(target, std::uint16_t(next->day));
        s.memory.write<2>(target + 2, next->year);
        s.memory.write<1>(target + 4, cm::calendar::is_leap(next->year));
    }
    return true;
}

void append_debug_contract_action(State& s) {
    if (!native_debugging_enabled()) return;
    const auto staff = std::uint32_t(s.r<4>(23 * 4));
    const auto original_stack = std::uint32_t(s.r<4>(4));
    GuestCall call(s);
    if (!debug_primary_contract(s, call, staff)) return;
    const auto label = debug_text(s, DebugText::extend_contract);
    call.call(0x103abf20, {original_stack + 0x6c,
        std::uint32_t(s.memory.read<2>(original_stack + 0x70)),
        std::uint32_t(s.memory.read<2>(original_stack + 0x6e)),
        call.text(label), extend_contract_action, 0});
}

bool dispatch_debug_contract_action(State& s) {
    if (!native_debugging_enabled()) return false;
    const auto control = selected_control(s);
    if (!control || s.memory.read<4>(control + 8) != extend_contract_action) return false;
    bool accepted = false;
    if (s.memory.read<4>(control + 12) & 2) {
        GuestCall call(s);
        const auto staff = call.call(0x1073ef70, {root(s), 0});
        accepted = extend_debug_contract(s, staff);
    }
    s.wr<4>(12, std::uint32_t(accepted ? -11 : 0));
    s.pc = s.r<4>(4128);
    return true;
}
}
