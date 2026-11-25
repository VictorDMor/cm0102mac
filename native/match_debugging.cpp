#include "match_debugging.hpp"
#include "aot_runtime.hpp"
#include "match_debugger_goal.hpp"
#include "match_debugger_cards.hpp"
#include "direct_commands.hpp"
#include "ui_bridge.hpp"
#include "debug_localization.hpp"
#include "debug_contract_date.hpp"
#include "debug_underdogs.hpp"
#include "debug_match_results.hpp"
#include "debug_force_purchase.hpp"
#include "debug_force_staff_signing.hpp"
#include "best_xi_selection.hpp"
#include "national_callup.hpp"
#include "transfer_screens.hpp"
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
constexpr unsigned best_u23_action = 30023, b_team_action = 30024;
constexpr unsigned call_up_action = 30031;
constexpr unsigned force_purchase_action = 30025;
constexpr unsigned force_staff_signing_action = 30033;
constexpr unsigned maximize_attributes_action = 30009, recover_player_action = 30034;
constexpr unsigned extend_contract_action = 30006, cancel_sale_action = 30007, remove_sale_clause_action = 30008;
constexpr unsigned free_squad_action = 30017, extend_all_contracts_action = 30021;
constexpr unsigned underdogs_action = 30018, underdogs_max_action = 30019;
constexpr unsigned match_results_action = 30022, all_draws_action = 30030;
// Right-click overlays on a fixture row's home name, "v" and away name.
constexpr unsigned result_home_action = 30230, result_draw_action = 30231, result_away_action = 30232;
static_assert(free_squad_action > shortcut_last);
constexpr std::uint32_t control_table = 0xba864, control_stride = 0x17c;
bool action_enabled(unsigned action);
std::optional<bool> pending_option;
bool pending_option_changed = false;
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
        const Memory::ExactBounds exact_bounds(s.memory);
        if (!active_player(s, match, player)) return false;
        const auto actor = actor_address(match, player);
        const auto score = signed_value<1>(s.memory.read<1>(match + (player < 20 ? 0xe03d : 0xe073)));
        return score >= 0 && score < 127 && s.memory.read<1>(actor + 0xc) < 255;
    } catch (const std::runtime_error&) { return false; }
}

unsigned debug_player_weight(const State& s, std::uint32_t match, unsigned player, DebugMatchEvent event) {
    try {
        const Memory::ExactBounds exact_bounds(s.memory);
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
    if (!native_debugging_feature(DebugFeature::red_cards) || !debug_player_weight(s, match, player, DebugMatchEvent::red_card))
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
    if (!native_debugging_feature(DebugFeature::injuries) || injury >= 77 ||
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
    if ((!native_debugging_feature(DebugFeature::add_goal) && !native_debugging_feature(DebugFeature::repeat_goal)) || !eligible_debug_goal(s, match, player))
        throw std::invalid_argument("Debug goal requires an enabled, active match and an on-pitch player");
    const auto actor = actor_address(match, player);
    const auto home = s.memory.read<1>(match + 0xe03d), away = s.memory.read<1>(match + 0xe073);
    const auto scorer = s.memory.read<1>(actor + 0xc), clock = s.memory.read<2>(match + 0x7948);
    {
        GuestCall caller(s);
        match_debugger::apply_awarded_goal(s, match, actor, player / 20);
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
    if (!toggle && (character < '1' || character > '6' || !action_enabled(shortcut_first + character - '1'))) return false;
    const auto current = original_command_context(s, 0xffff);
    if (!current.ready || current != expected) return false;
    if (toggle && current.handler != 0x1028dc80) {
        if (!current.actions[std::size_t(DirectCommand::continue_game)]) return false;
        // DISPLAY's text_field_key (1071b520) owns typing while a field has a
        // caret (+0x44), or while Tab/Return is transferring focus (12a936).
        // Keep plain Z available there, even when the sidebar allows Continue.
        for (const auto& control : original_controls(s, current)) {
            if (!(control.flags & 8)) continue;
            const auto record = current.screen + 0xba864 + std::uint32_t(control.index) * 0x17c;
            if (std::int32_t(s.memory.read<4>(record + 0x44)) >= 0 ||
                std::int16_t(s.memory.read<2>(current.screen + 0x12a936)) != -1) return false;
        }
        return event.kind == UIEvent::Kind::auto_key ||
            deliver_original_command(s, current, DirectCommand::continue_game);
    }
    if (current.handler != 0x1028dc80) return false;
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

namespace {
constexpr unsigned debug_options_action = 30020, feature_action = 30100;
constexpr unsigned debug_options_mode = 4;
constexpr unsigned club_results_action = 30026, add_club_action = 30027, remove_club_action = 30028;
constexpr unsigned force_club_action = 30029, club_result_action = 30300;
constexpr unsigned club_yes_action = 30304, club_no_action = 30305, club_prev_action = 30306;
constexpr unsigned club_next_action = 30307, club_back_action = 30308, club_results_mode = 5;
constexpr unsigned clubs_per_page = 12;
std::optional<std::uint32_t> pending_features;
constexpr std::array feature_labels{
    DebugText::recover_players, DebugText::play_for_free, DebugText::extend_contract,
    DebugText::cancel_sale, DebugText::remove_sale_clause, DebugText::underdogs,
    DebugText::underdogs_max, DebugText::add_goal, DebugText::repeat_goal,
    DebugText::red_cards, DebugText::injuries, DebugText::extend_all_contracts, DebugText::match_results,
    DebugText::force_purchase, DebugText::all_draws, DebugText::maximize_attributes,
    DebugText::force_staff_signing, DebugText::recover_player};
static_assert(feature_labels.size() == debug_feature_count);
std::uint32_t debugging_modal_mode(State& s) {
    GuestCall call(s);
    return call.call(0x1073ef70, {root(s), 1});
}
bool action_enabled(unsigned action) {
    switch (action) {
        case recover_squad_action: return native_debugging_feature(DebugFeature::recover_players);
        case free_squad_action: return native_debugging_feature(DebugFeature::play_for_free);
        case extend_all_contracts_action: return native_debugging_feature(DebugFeature::extend_all_contracts);
        case extend_contract_action: return native_debugging_feature(DebugFeature::extend_contract);
        case cancel_sale_action: return native_debugging_feature(DebugFeature::cancel_sale);
        case remove_sale_clause_action: return native_debugging_feature(DebugFeature::remove_sale_clause);
        case force_purchase_action: return native_debugging_feature(DebugFeature::force_purchase);
        case maximize_attributes_action: return native_debugging_feature(DebugFeature::maximize_attributes);
        case recover_player_action: return native_debugging_feature(DebugFeature::recover_player);
        case underdogs_action: return native_debugging_feature(DebugFeature::underdogs);
        case underdogs_max_action: return native_debugging_feature(DebugFeature::underdogs_max);
        case match_results_action: return native_debugging_feature(DebugFeature::match_result);
        case all_draws_action: return native_debugging_feature(DebugFeature::all_draws);
        case repeat_action: return native_debugging_feature(DebugFeature::repeat_goal);
        case goal_action: return native_debugging_feature(DebugFeature::add_goal);
        default:
            if (action >= shortcut_first && action <= shortcut_last)
                return native_debugging_feature(action < shortcut_first + 2 ? DebugFeature::add_goal :
                    action < shortcut_first + 4 ? DebugFeature::red_cards : DebugFeature::injuries);
            return false;
    }
}

std::vector<std::uint32_t> displayed_fixtures(const State& s, std::uint32_t group) {
    // MATCH_SCREENS 1028ae80: parameter 1 selects an inclusive range of
    // 0x6c-byte queue entries. Each entry's first word is the core fixture.
    const auto queue = s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7420));
    if (!group || !queue) return {};
    const auto first = std::int32_t(s.memory.read<4>(group + 4));
    const auto last = std::int32_t(s.memory.read<4>(group + 8));
    if (first < 0 || last < first || last - first > 10000) return {};
    const auto entries = s.memory.read<4>(queue + 0x1c);
    if (!entries) return {};
    std::vector<std::uint32_t> fixtures;
    for (auto n = first; n <= last; ++n) {
        const auto fixture = std::uint32_t(s.memory.read<4>(entries + std::uint32_t(n) * 0x6c));
        if (fixture) fixtures.push_back(fixture);
    }
    return fixtures;
}

void draw_debugging_panel(GuestCall& call, std::uint32_t ui, std::uint32_t page,
                          std::uint32_t left, std::uint32_t right, std::uint32_t title_color,
                          std::uint32_t title_ink, std::string_view title) {
    call.call(0x10712e60, {0});
    call.args({0, 2, 0, 0xffffffffu});
    call.call(0x107198c0, {page, 0, 0, 799, 599, 1, 0, 1});
    const auto panel_color = call.call(0x1076bb20, {0, 0x38, 0, 0});
    call.args({0, 0x30, panel_color, 0xffffffffu});
    call.call(0x107198c0, {page, left, 50, right, 549, 1, 0, 1});
    call.args({4, title_ink, call.text(title)});
    call.call(0x10744390, {ui, left + 10, 60, right - 10, 105, 0x30, title_color, 12});
}

void close_debugging_modal(State& s) {
    GuestCall call(s);
    const auto preferences = call.call(0x1073ef70, {root(s), 0});
    call.call(0x1013f4d0, {preferences, 1});
}

void open_club_results(State& s) {
    GuestCall call(s);
    const auto ui = root(s), toc = std::uint32_t(s.r<4>(8));
    // Reuse the options callbacks, but use an ordinary navigable history page:
    // no preferences object, modal lock or original settings writes.
    if (call.call(0x1073e4b0, {ui, std::uint32_t(s.memory.read<4>(toc - 0x4eb4)),
            std::uint32_t(s.memory.read<4>(toc - 0x4eb8)), 0, 0, 0})) {
        call.call(0x1073f1e0, {ui, 0, 0, 0});
        call.call(0x1073f1e0, {ui, 1, club_results_mode, 0});
        call.call(0x1073ede0, {ui, call.text(debug_text(s, DebugText::forced_clubs))});
    }
}

std::string result_club_name(State& s, GuestCall& call, std::uint32_t club) {
    // Original club-name formatter handles database/language naming variants.
    call.call(0x100d0940, {club, 0, call.stack + 0x200, 200, 1, 0});
    return original_text(s, call.stack + 0x200);
}

void build_club_results(State& s) {
    auto& choices = match_result_debugging(s);
    choices.choosing_club = false;
    GuestCall call(s);
    const auto ui = root(s), page = screen(s, ui), toc = std::uint32_t(s.r<4>(8));
    const bool portuguese = s.memory.read<1>(toc - 0x35ac) == 9;
    const auto color = [&](unsigned offset) { return std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - offset))); };
    const auto text = [&](DebugText label) { return debug_text(s, label); };
    draw_debugging_panel(call, ui, page, 110, 789, color(0x7bd4), color(0x7bc8), text(DebugText::forced_clubs));
    call.call(0x1073ede0, {ui, call.text(text(DebugText::forced_clubs))});
    const auto label = [&](unsigned left, unsigned top, unsigned right, unsigned bottom,
                           std::string_view value, unsigned font = 2) {
        call.args({font, color(0x7bc8), call.text(value)});
        call.call(0x10744390, {ui, left, top, right, bottom, 1, 0, 12});
    };
    const auto button = [&](unsigned left, unsigned top, unsigned right, std::string_view value,
                            unsigned action) {
        call.args({1, color(0x7bc8), call.text(value), action, 0});
        call.call(0x10744430, {ui, left, top, right, top + 30, 0x30, color(0x7bc4), 12});
    };
    if (choices.confirm_removal) {
        const auto name = result_club_name(s, call, *choices.confirm_removal);
        label(140, 180, 759, 215, portuguese ? "Remover este time da lista?" : "Remove this team from the list?", 4);
        label(140, 225, 759, 270, name, 4);
        label(140, 285, 759, 325, portuguese ? "O resultado deste time deixar\341 de ser for\347ado por esta regra."
            : "This team's results will no longer be forced by this rule.");
        button(245, 350, 435, text(DebugText::remove), club_yes_action);
        // Use the original Cancel translation, including languages other than Portuguese.
        const auto pool = std::uint32_t(s.memory.read<4>(toc - 0x4e18));
        const auto buffer = std::uint32_t(s.memory.read<4>(toc - 0x7bb8));
        call.call(0x10727890, {buffer, pool + 0x544});
        button(465, 350, 655, original_text(s, buffer), club_no_action);
        return;
    }
    call.call(0x102a1070, {4, 0});
    button(125, 115, 315, portuguese ? "Adicionar time" : "Add team", add_club_action);
    const unsigned count = unsigned(choices.clubs.size());
    const unsigned pages = std::max(1u, (count + clubs_per_page - 1) / clubs_per_page);
    choices.club_page = std::min(choices.club_page, pages - 1);
    const unsigned begin = choices.club_page * clubs_per_page, end = std::min(count, begin + clubs_per_page);
    const auto range = pages > 1 ? std::to_string(begin + 1) +
        (begin + 1 == end ? "" : "-" + std::to_string(end)) + (portuguese ? " de " : " of ") : "";
    label(325, 115, 774, 145, range + std::to_string(count) +
        (portuguese ? (count == 1 ? " time" : " times") : (count == 1 ? " team" : " teams")));
    label(130, 147, 769, 170, portuguese ? "Escolha o resultado de cada time. As altera\347\365es valem na hora."
        : "Choose each team's result. Changes apply immediately.");
    // Stable footer positions, explicit page count and inert unavailable arrows.
    s.memory.copy_in(call.stack + 0xc0, std::vector<std::uint8_t>{22, 30, 24, 24});
    call.args({0, 1, 0, 0xffffffffu});
    const auto footer = call.call(0x107198c0, {page, 125, 509, 774, 539, 4, call.stack + 0xc0, 1});
    const auto muted = call.call(0x1076bb20, {0x80, 0x80, 0x80, 0});
    const auto footer_button = [&](unsigned column, DebugText value, unsigned action, bool enabled) {
        call.args({enabled ? color(0x7bc8) : muted, call.text(text(value)), enabled ? action : 0, 0});
        call.call(enabled ? 0x107444e0 : 0x107445a0,
            {ui, footer, column, 0, enabled ? 0x30u : 1u, enabled ? color(0x7bc4) : 0u, 12, 2});
    };
    footer_button(0, DebugText::back, club_back_action, true);
    const auto page_label = std::string(portuguese ? "P\341gina " : "Page ") +
        std::to_string(choices.club_page + 1) + " / " + std::to_string(pages);
    call.args({color(0x7bc8), call.text(page_label)});
    call.call(0x107445a0, {ui, footer, 1, 0, 1, 0, 12, 2});
    footer_button(2, DebugText::previous, club_prev_action, choices.club_page != 0);
    footer_button(3, DebugText::next, club_next_action, choices.club_page + 1 < pages);
    s.wr<8>(0x4010, float_bits<8>(float_value<4>(s.memory.read<4>(s.memory.read<4>(toc - 0x4ec4)))));
    call.args({1, footer});
    call.call(0x103cbb30, {125, 175, 774, 485, call.stack + 0xd0,
        call.stack + 0xd4, call.stack + 0xd8, call.stack + 0xdc});
    if (!count) {
        label(145, 245, 754, 280, portuguese ? "Nenhum time adicionado" : "No teams added", 4);
        label(145, 290, 754, 355, portuguese
            ? "Clique em Adicionar time, abra o clube desejado e escolha For\347ar Resultados."
            : "Click Add team, open a club and choose Force Results.");
        return;
    }
    // Heading and body share column boundaries. Names are labels, not inert links.
    s.memory.copy_in(call.stack + 0xc0, std::vector<std::uint8_t>{44, 42, 14});
    call.args({0, 1, 0, 0xffffffffu});
    const auto heading = call.call(0x107198c0, {page, 130, 177, 769, 199, 3, call.stack + 0xc0, 1});
    const std::array headings{portuguese ? "Time" : "Team", portuguese ? "Resultado for\347ado" : "Forced result",
        portuguese ? "A\347\343o" : "Action"};
    for (unsigned column = 0; column < headings.size(); ++column) {
        call.args({color(0x7bc8), call.text(headings[column])});
        call.call(0x107445a0, {ui, heading, column, 0, 1, 0, 12, 2});
    }
    s.memory.copy_in(call.stack + 0xc0, std::vector<std::uint8_t>{44, 14, 14, 14, 14});
    call.args({0, 1, 0, 0xffffffffu});
    const auto grid = call.call(0x107198c0, {page, 130, 201, 769, 483, 5, call.stack + 0xc0, clubs_per_page});
    const auto available_ink = call.call(0x1076bb20, {0, 0xc0, 0, 0});
    for (unsigned i = begin; i < end; ++i) {
        const auto& rule = choices.clubs[i];
        const auto row = i - begin;
        call.args({color(0x7b10), call.text(result_club_name(s, call, rule.club))});
        call.call(0x107445a0, {ui, grid, 0, row, 1, 0, 1, 3});
        constexpr std::array labels{DebugText::win, DebugText::draw, DebugText::loss};
        for (unsigned value = 0; value < labels.size(); ++value) {
            const bool active = unsigned(rule.result) == value;
            const auto ink = active ? color(0x7bd0) : available_ink;
            call.args({ink, call.text(text(labels[value])), club_result_action + value, rule.club});
            call.call(active ? 0x107445a0 : 0x107444e0,
                {ui, grid, value + 1, row, active ? 0x221u : 1u, active ? ink : 0u, 12, 2});
        }
        call.args({color(0x7bc8), call.text(text(DebugText::remove)), remove_club_action, rule.club});
        call.call(0x107444e0, {ui, grid, 4, row, 0x30, color(0x7bc4), 12, 2});
    }
    label(130, 486, 769, 506, text(DebugText::club_rule_pair_hint));
}

bool dispatch_club_results(State& s, std::uint32_t control, unsigned action) {
    auto& choices = match_result_debugging(s);
    int result = -11;
    if (!control || !(s.memory.read<4>(control + 12) & 2)) {
        // Escape cancels a confirmation, or goes back from the list.
        if (action == 40) {
            if (choices.confirm_removal) choices.confirm_removal.reset();
            else {
                choices.choosing_club = false;
                result = -2; // The original monitor moves to the previous history page.
            }
        } else result = 0;
    } else if (choices.confirm_removal) {
        if (action == club_yes_action) {
            remove_debug_result_club(s, *choices.confirm_removal);
            choices.confirm_removal.reset();
        } else if (action == club_no_action) choices.confirm_removal.reset();
    } else if (action == add_club_action) {
        choices.choosing_club = true;
        GuestCall call(s);
        // Same Nations browser as the career sidebar's Nations & Clubs menu.
        const auto continent = call.call(0x10192250, {});
        const auto nation = call.call(0x101922e0, {});
        call.call(0x10103600, {1, continent, nation, 0});
        result = -4;
    } else if (action == remove_club_action) {
        const auto club = std::uint32_t(s.memory.read<4>(control + 0x48));
        if (std::ranges::find(choices.clubs, club, &ClubResultRule::club) != choices.clubs.end())
            choices.confirm_removal = club;
    } else if (action >= club_result_action && action < club_result_action + 3) {
        set_debug_club_result(s, s.memory.read<4>(control + 0x48), ForcedClubResult(action - club_result_action));
    } else if (action == club_prev_action && choices.club_page) --choices.club_page;
    else if (action == club_next_action) {
        const auto pages = std::max(1u, (unsigned(choices.clubs.size()) + clubs_per_page - 1) / clubs_per_page);
        if (choices.club_page + 1 < pages) ++choices.club_page;
    }
    else if (action == club_back_action) {
        choices.choosing_club = false;
        result = -2;
    } else {
        GuestCall call(s);
        result = std::int32_t(call.call(0x102a54b0, {std::uint32_t(s.r<4>(12))}));
    }
    // This page has no original preferences object. Never fall through to
    // the original options handler for unrelated keys or sidebar controls.
    s.wr<4>(12, std::uint32_t(result)); s.pc = s.r<4>(4128);
    return true;
}

}

void append_debugging_sidebar(State& s) {
    if (!native_debugging_available()) return;
    // MAIN_MENU's Game Options popup is complete. Insert a normal sidebar
    // row after its parent, using the same row counter, group, colors and font.
    const auto ui = std::uint32_t(s.r<4>(29 * 4)), page = screen(s, ui);
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto row = std::uint32_t(s.memory.read<4>(stack + 0x70)) + 1;
    const auto color = std::uint32_t(s.memory.read<2>(s.memory.read<4>(s.r<4>(8) - 0x7bd8)));
    GuestCall call(s);
    call.args({0x1021, 0, 0, 12, 1, color, call.text(debug_text(s, DebugText::options_sidebar)),
        0, 0, 0, std::uint32_t(s.r<4>(22 * 4))});
    const auto parent = call.call(0x10719740, {page, 0x82, 0, 0, 0, 0, 0, row});
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto theme = [&](unsigned at) { return std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - at))); };
    call.args({0, 0x30, theme(0x7bcc), parent});
    const auto menu = call.call(0x107198c0, {page, 0, 0, 0, 0, 0, 0, 0});
    for (const auto& [label, action] : {std::pair{DebugText::toggle_options, debug_options_action},
                                       std::pair{DebugText::forced_clubs, club_results_action}}) {
        call.args({0x10, theme(0x7bcc), theme(0x7bd0), 1, 1, theme(0x7bd4),
            call.text(debug_text(s, label)), 0, action, 0, menu});
        call.call(0x10719740, {page, 2, 0, 0, 0, 0, 0, 0});
    }
    s.memory.write<4>(stack + 0x70, row);
}

bool dispatch_debugging_sidebar(State& s) {
    if (!native_debugging_available()) return false;
    const auto control = selected_control(s);
    if (!control) return false;
    const auto action = s.memory.read<4>(control + 8);
    if (action != debug_options_action && action != club_results_action) return false;
    if (s.memory.read<4>(control + 12) & 2) {
        if (action == club_results_action) {
            auto& choices = match_result_debugging(s);
            choices.choosing_club = false;
            choices.confirm_removal.reset();
            open_club_results(s);
        } else {
            GuestCall call(s);
            // Original options owns the modal's lifetime and temporary settings.
            // Mode 4 selects only our body; Cancel/OK still close it normally.
            call.call(0x1033ce70, {debug_options_mode});
            pending_features = native_debugging_features();
        }
    }
    s.wr<4>(12, std::uint32_t(-11)); s.pc = s.r<4>(4128);
    return true;
}

bool build_debugging_options_screen(State& s) {
    if (!native_debugging_available()) return false;
    const auto mode = debugging_modal_mode(s);
    if (mode == club_results_mode) {
        build_club_results(s);
        s.pc = s.r<4>(4128);
        return true;
    }
    if (mode != debug_options_mode) return false;
    if (!pending_features) pending_features = native_debugging_features();
    {
        GuestCall call(s);
        const auto ui = root(s), page = screen(s, ui), toc = std::uint32_t(s.r<4>(8));
        const auto color = [&](unsigned offset) { return std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - offset))); };
        draw_debugging_panel(call, ui, page, 145, 744, color(0x7bd4), color(0x7bc8),
            debug_text(s, DebugText::options));

        // Original 75/25 footer: wide Cancel, compact OK, same relief and type.
        s.memory.write<2>(call.stack + 0xc0, s.memory.read<2>(toc - 0x3894));
        call.args({0, 1, 0, 0xffffffffu});
        const auto footer = call.call(0x107198c0, {page, 155, 509, 734, 539, 2, call.stack + 0xc0, 1});
        const auto pool = std::uint32_t(s.memory.read<4>(toc - 0x4e18));
        const auto buffer = std::uint32_t(s.memory.read<4>(toc - 0x7bb8));
        for (unsigned col = 0; col < 2; ++col) {
            call.call(0x10727890, {buffer, pool + (col ? 0x785u : 0x544u)});
            call.args({color(0x7bc8), buffer, col ? 41u : 40u, 0});
            call.call(0x107444e0, {ui, footer, col, 0, 0x30, color(0x7bc4), 12, 3});
        }
        call.call(0x10727890, {buffer, pool + 0x8a8});
        call.args({1, color(0x7bc8), buffer, 39, 0});
        call.call(0x10744430, {ui, 609, 115, 734, 135, 0x30, color(0x7bc4), 12});

        // Draw the same striped pitch underneath the settings grid. The
        // original pitch routine writes its four inset bounds to scratch.
        s.wr<8>(0x4010, float_bits<8>(float_value<4>(s.memory.read<4>(s.memory.read<4>(toc - 0x4ec4)))));
        call.args({1, footer});
        call.call(0x103cbb30, {165, 145, 724, 489, call.stack + 0xd0,
            call.stack + 0xd4, call.stack + 0xd8, call.stack + 0xdc});
        s.memory.copy_in(call.stack + 0xc0, std::vector<std::uint8_t>{64, 18, 18});
        call.args({0, 1, 0, 0xffffffffu});
        const auto grid = call.call(0x107198c0, {page, 170, 150, 719, 484, 3,
            call.stack + 0xc0, debug_feature_count});
        const auto normal = call.call(0x1076bb20, {0, 0xc0, 0, 0});
        for (unsigned feature = 0; feature < debug_feature_count; ++feature) {
            call.args({color(0x7b10), call.text(debug_text(s, feature_labels[feature]))});
            call.call(0x107445a0, {ui, grid, 0, feature, 1, 0, 1, 3});
            for (unsigned value = 0; value < 2; ++value) {
                const bool active = bool(*pending_features & (1u << feature)) == bool(value);
                const auto ink = active ? color(0x7bd0) : normal;
                call.args({ink, call.text(debug_text(s, value ? DebugText::on : DebugText::off)),
                    feature_action + feature, value});
                // Selected cells use the original yellow frame/text; only the
                // other choice is a button, exactly as in Game Settings.
                call.call(active ? 0x107445a0 : 0x107444e0,
                    {ui, grid, value + 1, feature, active ? 0x221u : 1u, active ? ink : 0u, 12, 2});
            }
        }
    }
    s.pc = s.r<4>(4128);
    return true;
}

void reset_match_debugging() { recent.reset(); }
void begin_debugging_options() {
    pending_option = native_debugging_enabled();
    pending_option_changed = false;
    pending_features.reset();
}

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
    call.args({std::uint32_t(title), call.text(" " + std::string(debug_text(s, DebugText::debugging)))});
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
    const auto mode = debugging_modal_mode(s);
    if (mode == club_results_mode) return dispatch_club_results(s, control, action);
    if (mode == debug_options_mode) {
        if (!pending_features) pending_features = native_debugging_features();
        if (control && action >= feature_action && action < feature_action + debug_feature_count &&
            (s.memory.read<4>(control + 12) & 2)) {
            const auto bit = 1u << (action - feature_action);
            if (s.memory.read<4>(control + 0x48)) *pending_features |= bit;
            else *pending_features &= ~bit;
        } else if (action == 39) pending_features = 0;
        else if (action == 40 || action == 41) {
            if (action == 41) select_native_debugging_features(*pending_features);
            pending_features.reset();
            pending_option.reset();
            close_debugging_modal(s);
            // Match the original close result, without rewriting unrelated
            // game preferences: this modal saves only host debug settings.
            s.wr<4>(12, std::uint32_t(-9)); s.pc = s.r<4>(4128);
            return true;
        } else return false;
        s.wr<4>(12, std::uint32_t(-11)); s.pc = s.r<4>(4128);
        return true;
    }
    if (action == option_action && control && (s.memory.read<4>(control + 12) & 2)) {
        pending_option = s.memory.read<4>(control + 0x48) != 0;
        pending_option_changed = true;
        s.wr<4>(12, std::uint32_t(-11)); s.pc = s.r<4>(4128);
        return true;
    }
    if (action == 41) {
        if (pending_option_changed && pending_option) select_native_debugging(*pending_option);
        pending_option.reset();
    }
    if (action == 40) pending_option.reset();
    if (action == 39) { pending_option = false; pending_option_changed = true; }
    return false;
}

void append_fixture_debugging_controls(State& s) {
    if (!action_enabled(underdogs_action) && !action_enabled(underdogs_max_action) &&
        !action_enabled(all_draws_action)) return;
    const auto ui = root(s), page = screen(s, ui), toc = std::uint32_t(s.r<4>(8));
    const auto color = [&](unsigned offset) { return std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - offset))); };
    GuestCall call(s);
    const auto fixtures = displayed_fixtures(s, call.call(0x1073ef70, {ui, 1}));
    const bool enabled = std::ranges::any_of(fixtures, [&](auto fixture) {
        return std::int8_t(s.memory.read<1>(fixture + 0x43)) == -1;
    });
    // Competition occupies x=655..780, y=145..165. Use the same row,
    // immediately to its left, preserving the original Competition button.
    call.args({0, 1, 0, 0xffffffffu});
    const auto grid = call.call(0x107198c0, {page, 528, 145, 653, 165, 1, 0, 1});
    call.args({color(0x7bc8), call.text(debug_text(s, DebugText::debugging)), 0, 0});
    const auto cell = call.call(0x107444e0, {ui, grid, 0, 0, 0x30, color(0x7bc4), 12, 1});
    call.args({0, 0x130, color(0x7bcc), cell});
    const auto menu = call.call(0x107198c0, {page, 0, 0, 0, 0, 0, 0, 0});
    const auto item = [&](unsigned action, std::string_view label, bool active) {
        call.args({0x10, color(0x7bcc), color(0x7bd0), 1, 1, color(0x7bd4),
                   call.text(label), 0, action, 0, menu});
        call.call(0x10719740, {page, active ? 2u : 0u, 0, 0, 0, 0, 0, 0});
    };
    for (const auto action : {underdogs_action, underdogs_max_action}) {
        if (!action_enabled(action)) continue;
        const auto mode = action == underdogs_action ? UnderdogMode::normal : UnderdogMode::maximum;
        const auto& plans = underdog_debugging(static_cast<const State&>(s)).fixtures;
        const bool selected = std::ranges::any_of(plans, [&](const auto& p) {
            return p.mode == mode && std::ranges::find(fixtures, p.fixture) != fixtures.end();
        });
        const auto text = debug_text(s, action == underdogs_action ? DebugText::underdogs : DebugText::underdogs_max);
        const auto label = std::string(selected ? "* " : "") + std::string(text);
        item(action, label, enabled);
    }
    if (action_enabled(all_draws_action)) {
        const bool selected = !fixtures.empty() && debug_draw_day(s, fixtures.front());
        const bool eligible = std::ranges::any_of(fixtures, [&](auto fixture) { return debug_match_result_eligible(s, fixture); });
        item(all_draws_action, std::string(selected ? "* " : "") + std::string(debug_text(s, DebugText::all_draws)), eligible);
    }
}

namespace {
// Cells of one fixture row made by 10076450: column 2 is the home name and 6
// the away name (data: the club), column 4 the "v" of an unplayed fixture
// (action 0x7d2, data: the core fixture). Rows are unique within the grid.
struct FixtureRowCells { std::uint32_t fixture; std::array<std::uint32_t, 3> cells; };
constexpr std::array<unsigned, 3> result_columns{2, 4, 6};
constexpr std::array result_actions{result_home_action, result_draw_action, result_away_action};
constexpr std::array result_values{ForcedMatchResult::home_win, ForcedMatchResult::draw, ForcedMatchResult::away_win};
constexpr unsigned control_limit = 0x4af, control_reserve = 32;

std::vector<FixtureRowCells> fixture_result_rows(const State& s, std::uint32_t page, std::int16_t grid) {
    const auto count = unsigned(signed_value<2>(s.memory.read<2>(page + 0x129da6)));
    const auto item = [&](unsigned i) { return page + control_table + i * control_stride; };
    const auto in_grid = [&](std::uint32_t c) { return signed_value<2>(s.memory.read<2>(c + 0x72)) == grid; };
    std::vector<FixtureRowCells> rows;
    for (unsigned i = 0; i < count && i < control_limit; ++i) {
        const auto versus = item(i);
        if (!in_grid(versus) || s.memory.read<4>(versus + 8) != 0x7d2 ||
            s.memory.read<4>(versus + 0x20) != result_columns[1]) continue;
        const auto fixture = std::uint32_t(s.memory.read<4>(versus + 0x48));
        const auto row = s.memory.read<4>(versus + 0x24);
        FixtureRowCells found{fixture, {0, versus, 0}};
        for (unsigned j = 0; j < count && j < control_limit; ++j) {
            const auto cell = item(j);
            if (!in_grid(cell) || s.memory.read<4>(cell + 0x24) != row) continue;
            const auto column = s.memory.read<4>(cell + 0x20);
            const auto club = s.memory.read<4>(cell + 0x48);
            if (column == result_columns[0] && club == s.memory.read<4>(fixture + 0x1c)) found.cells[0] = cell;
            if (column == result_columns[2] && club == s.memory.read<4>(fixture + 0x20)) found.cells[2] = cell;
        }
        if (found.cells[0] && found.cells[2]) rows.push_back(found);
    }
    return rows;
}
}

void append_fixture_result_controls(State& s) {
    if (!action_enabled(match_results_action) && !action_enabled(all_draws_action)) return;
    // 1028ae80 after its row loop: r13 is the fixture grid, r30 the UI root.
    const auto ui = std::uint32_t(s.r<4>(30 * 4)), page = screen(s, ui);
    const auto grid = std::int16_t(s.r<4>(13 * 4));
    if (ui != root(s) || grid < 0) return;
    auto rows = fixture_result_rows(s, page, grid);
    std::erase_if(rows, [&](const auto& row) { return !debug_match_result_eligible(s, row.fixture); });
    if (rows.empty()) return;
    GuestCall call(s);
    const auto light_green = call.call(0x1076bb20, {0x90, 0xee, 0x90, 0});
    for (const auto& row : rows) {
        // Show the choice as the original row would draw it: the winner's
        // name in light green, or the draw word in place of the "v".
        auto chosen = action_enabled(match_results_action) ? debug_match_result(s, row.fixture) : ForcedMatchResult::normal;
        if (chosen == ForcedMatchResult::normal && action_enabled(all_draws_action) && debug_draw_day(s, row.fixture))
            chosen = ForcedMatchResult::draw;
        switch (chosen) {
            case ForcedMatchResult::home_win: s.memory.write<2>(row.cells[0] + 0x70, light_green); break;
            case ForcedMatchResult::away_win: s.memory.write<2>(row.cells[2] + 0x70, light_green); break;
            case ForcedMatchResult::draw: {
                const auto word = debug_text(s, DebugText::draw);
                std::vector<std::uint8_t> text(word.begin(), word.end()); text.push_back(0);
                s.memory.copy_in(row.cells[1] + 0x78, text);
                break;
            }
            case ForcedMatchResult::normal: break;
        }
        // Invisible right-click-only cells over the three, as the original
        // makes its own (103b5ab8): flags 0x102, plain style, no text.
        if (!action_enabled(match_results_action)) continue;
        if (signed_value<2>(s.memory.read<2>(page + 0x129da6)) + 3 > int(control_limit - control_reserve)) continue;
        const auto row_index = std::uint32_t(s.memory.read<4>(row.cells[1] + 0x24));
        for (unsigned k = 0; k < 3; ++k) {
            call.args({1, 0, 0, 12, 1, 0, 0, 0, result_actions[k], row.fixture, std::uint32_t(grid)});
            call.call(0x10719740, {page, 0x102, 0, 0, 0, 0, result_columns[k], row_index});
        }
    }
}

bool dispatch_fixture_debugging_control(State& s) {
    const auto control = selected_control(s);
    if (!control) return false;
    const auto action = s.memory.read<4>(control + 8);
    const auto result = std::ranges::find(result_actions, action);
    if (result != result_actions.end()) {
        // Consume the press, accepted or refused: the hit test selects a control on every
        // loop pass while its button byte is set, and only mouse-up clears the right byte,
        // so a held button would toggle the choice again after the rebuild. The original
        // dispatcher clears its left byte the same way (0x1028bdb8).
        const auto toc = std::uint32_t(s.r<4>(8));
        for (const std::uint32_t slot : {0x5c0cu, 0x7bacu})
            s.memory.write<1>(std::uint32_t(s.memory.read<4>(toc - slot)), 0);
        bool accepted = false;
        const auto fixture = std::uint32_t(s.memory.read<4>(control + 0x48));
        if (action_enabled(match_results_action) && (s.memory.read<4>(control + 12) & 2)) {
            GuestCall call(s);
            const auto ui = root(s);
            const auto fixtures = displayed_fixtures(s, call.call(0x1073ef70, {ui, 1}));
            if (std::ranges::find(fixtures, fixture) != fixtures.end() && debug_match_result_eligible(s, fixture)) {
                const auto chosen = result_values[std::size_t(result - result_actions.begin())];
                // The chosen cell again clears; another cell switches.
                const auto next = debug_match_result(s, fixture) == chosen ? ForcedMatchResult::normal : chosen;
                accepted = set_debug_match_result(s, fixture, next);
                if (accepted) {
                    // Keep the list's scroll for the rebuild, as 1028bc90
                    // itself does before handling any selected control.
                    const auto list = std::int16_t(call.call(0x1073ef70, {ui, 3}));
                    call.call(0x1073f1e0, {ui, 2, call.call(0x10743510, {ui, std::uint32_t(std::int32_t(list))}), 0});
                }
            }
        }
        s.wr<4>(12, std::uint32_t(accepted ? -11 : 0)); s.pc = s.r<4>(4128);
        return true;
    }
    if (action != underdogs_action && action != underdogs_max_action && action != all_draws_action) return false;
    bool accepted = false;
    if (action_enabled(action) && (s.memory.read<4>(control + 12) & 2)) {
        GuestCall call(s);
        const auto fixtures = displayed_fixtures(s, call.call(0x1073ef70, {root(s), 1}));
        if (action == all_draws_action) {
            const auto pending = std::ranges::find_if(fixtures, [&](auto fixture) { return debug_match_result_eligible(s, fixture); });
            if (pending != fixtures.end()) accepted = set_debug_draw_day(s, *pending, !debug_draw_day(s, *pending));
        } else accepted = arm_debug_underdogs(s, fixtures,
            action == underdogs_action ? UnderdogMode::normal : UnderdogMode::maximum);
    }
    s.wr<4>(12, std::uint32_t(accepted ? -11 : 0)); s.pc = s.r<4>(4128);
    return true;
}

void append_match_debugging_controls(State& s) {
    if (!action_enabled(goal_action) && !action_enabled(repeat_action)) return;
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
    if (action_enabled(goal_action)) {
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
    }
    if (!action_enabled(repeat_action)) return;
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
    if (!action_enabled(action)) {
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
                         std::uint32_t formation, std::uint32_t fixture, LineupMode mode) {
    if (!club || !formation || managed > 1) return false;
    GuestCall call(s);
    const auto bench = fixture ? std::min<unsigned>(s.memory.read<1>(fixture + 0x4e) >> 4, 9) : 9;
    std::vector<std::uint32_t> available;
    const bool national = is_national_squad(s, club);
    // The game's stand-ins for a short squad (the grey rows of the squad list) are the people
    // numbered past the staff table's count: the original status operation tells them apart the
    // same way (10060d28, where they skip the contract lookup). They are never selected.
    const auto staff_count = signed_value<4>(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7be4)));
    for (unsigned i = 0; i < 50; ++i) {
        const auto staff = std::uint32_t(s.memory.read<4>(club + 0x98 + i * 4));
        if (!staff || !s.memory.read<4>(staff + 0x14) ||
            std::find(available.begin(), available.end(), staff) != available.end()) continue;
        if (signed_value<4>(s.memory.read<4>(staff)) >= staff_count) continue;
        // Roster membership and the display status do not establish where a
        // player currently plays. Outgoing loans can remain listed here.
        // Use the current club, not the primary contract owner: incoming
        // loans remain eligible. A national squad spans many club employers.
        if (!national && s.memory.read<4>(staff + 0x24) != club) continue;
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
    // The original profile getter decodes signed technical bytes using current
    // ability, including its goalkeeper-specific cases. Never mix those raw
    // bytes with the physical attributes' 1..20 scale. IDs are the Mac profile
    // switch at 0x100d2330, not a Windows record layout or estimated ability.
    constexpr std::array<best_xi::Attribute, 31> display_attributes{
        best_xi::acceleration, best_xi::aggression, best_xi::agility, best_xi::anticipation,
        best_xi::balance, best_xi::bravery, best_xi::creativity, best_xi::crossing,
        best_xi::decisions, best_xi::attribute_count, best_xi::dribbling, best_xi::finishing,
        best_xi::flair, best_xi::handling, best_xi::heading, best_xi::leadership,
        best_xi::jumping, best_xi::long_shots, best_xi::marking, best_xi::off_the_ball,
        best_xi::pace, best_xi::passing, best_xi::positioning, best_xi::reflexes,
        best_xi::free_kicks, best_xi::stamina, best_xi::strength, best_xi::tackling,
        best_xi::teamwork, best_xi::technique, best_xi::work_rate};
    constexpr std::array<unsigned, 10> position_offsets{0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x19};
    const auto medical = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7ba0)));
    std::vector<best_xi::Player> players, older;
    for (const auto staff : available) {
        const auto record = std::uint32_t(s.memory.read<4>(staff + 0x14));
        best_xi::Player player;
        player.staff = staff; player.id = signed_value<4>(s.memory.read<4>(staff));
        for (unsigned i = 0; i < display_attributes.size(); ++i)
            if (display_attributes[i] != best_xi::attribute_count)
                player.attributes[display_attributes[i]] =
                    std::clamp(int(signed_value<4>(call.call(0x100d2330, {staff, record, i + 1}))), 0, 20) * 100;
        for (unsigned i = 0; i < position_offsets.size(); ++i)
            player.positions[i] = std::clamp(int(signed_value<1>(s.memory.read<1>(record + position_offsets[i]))), 0, 20);
        player.condition = unsigned(std::clamp(int(signed_value<2>(call.call(0x101878d0, {medical, staff, 0}))), 0, 10000));
        // Age in whole years on the game date (Best U23 means 23 or less): the original age operation takes
        // a result slot, today's date and the person's birth date (+34, +38).
        // It stores days at +0 and years at +2 of the slot.
        if (mode == LineupMode::under_23) {
            const auto slot = call.stack + 0x300;
            call.call(0x10717dd0, {slot, std::uint32_t(s.memory.read<4>(s.r<4>(8) - 0x7b80)),
                                   std::uint32_t(s.memory.read<4>(staff + 0x34)),
                                   std::uint32_t(s.memory.read<4>(staff + 0x38))});
            if (s.memory.read<2>(slot + 2) > 23) { older.push_back(player); continue; }
        }
        players.push_back(player);
    }
    std::array<best_xi::Role, 11> roles{};
    for (unsigned i = 0; i < roles.size(); ++i)
        roles[i] = {unsigned(s.memory.read<2>(formation + 0x12e + i * 2)),
                    unsigned(s.memory.read<2>(formation + 0x150 + i * 2)),
                    s.memory.read<8>(formation + 0x1888 + i * 8),
                    unsigned(s.memory.read<4>(formation + 0x588))};
    // Fill the empty places of `out` (indexes into `roles`) from `pool`, optionally
    // only with compatible pairs; the chosen players leave the pool.
    const auto fill = [](std::vector<best_xi::Player>& pool, std::span<const best_xi::Role> roles,
                         std::span<std::uint32_t> out, bool compatible_only) {
        std::vector<unsigned> open;
        std::vector<best_xi::Role> jobs;
        for (unsigned i = 0; i < out.size(); ++i) if (!out[i]) { open.push_back(i); jobs.push_back(roles[i]); }
        if (open.empty() || pool.empty()) return;
        const auto chosen = best_xi::select(pool, jobs, compatible_only ? best_xi::Allowed(best_xi::compatible) : best_xi::Allowed());
        for (unsigned k = 0; k < open.size(); ++k) {
            if (!chosen[k]) continue;
            out[open[k]] = chosen[k];
            std::erase_if(pool, [&](const auto& player) { return player.staff == chosen[k]; });
        }
    };
    std::array<std::uint32_t, 11> starters{};
    if (mode == LineupMode::under_23) {
        // Players of 23 or less first, if compatible; then compatible older players;
        // then whoever is left by plain best fit, young before old.
        fill(players, roles, starters, true);
        fill(older, roles, starters, true);
        fill(players, roles, starters, false);
        fill(older, roles, starters, false);
    } else {
        starters = best_xi::select(players, roles);
        if (mode == LineupMode::b_team) {
            // The second team: the same selection over everyone the Best XI left out.
            std::erase_if(players, [&](const auto& player) {
                return std::find(starters.begin(), starters.end(), player.staff) != starters.end();
            });
            starters = best_xi::select(players, roles);
        }
    }
    std::array<std::uint32_t, 20> selected{};
    std::copy(starters.begin(), starters.end(), selected.begin());
    for (auto* pool : {&players, &older})
        std::erase_if(*pool, [&](const auto& player) {
            return std::find(starters.begin(), starters.end(), player.staff) != starters.end();
        });
    // Fill the fixture's bench from the remaining pool, retaining goalkeeper,
    // defensive, midfield and attacking coverage without spending a starter.
    constexpr std::array<unsigned, 9> reserve_roles{1, 4, 16, 64, 16, 4, 64, 16, 4};
    std::array<best_xi::Role, 9> reserves{};
    for (unsigned i = 0; i < bench; ++i) reserves[i] = {reserve_roles[i], reserve_roles[i], 0, 0};
    // Best U23: players of 23 or less first, then older players for the free places.
    const auto places = std::span(selected).subspan(11, bench);
    fill(players, reserves, places, false);
    fill(older, reserves, places, false);
    for (unsigned i = 0; i < 11 + bench; ++i) {
        const auto staff = selected[i];
        if (!staff) continue;
        if (std::find(available.begin(), available.end(), staff) == available.end() ||
            std::find(selected.begin(), selected.begin() + i, staff) != selected.begin() + i)
            throw std::runtime_error("Best XI returned an unavailable or duplicate player");
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

namespace {
std::uint32_t debug_attribute_player(const State& s, std::uint32_t staff);

// A person's medical row (36 bytes per person id) names his live injury record at +0x10;
// 0xffffffff means none (a recovering or fitness status is a record too).
std::uint32_t medical_condition(State& s, std::uint32_t medical, std::uint32_t staff) {
    return std::uint32_t(s.memory.read<4>(medical)) + std::uint32_t(s.memory.read<4>(staff)) * 0x24;
}

bool debug_injured(State& s, std::uint32_t medical, std::uint32_t staff) {
    return s.memory.read<4>(medical_condition(s, medical, staff) + 0x10) != 0xffffffffu;
}

// The discipline store keeps four competition categories per person (8 bytes each, at +0x14
// of the sorted index, 0x2c bytes per person) and a count of deferred bans at +0x28.
bool debug_suspended(State& s, GuestCall& call, std::uint32_t discipline, std::uint32_t staff) {
    const auto id = std::uint32_t(s.memory.read<4>(staff));
    const auto row = std::uint32_t(s.memory.read<4>(discipline + 0x14)) + id * 0x2c;
    if (signed_value<1>(s.memory.read<1>(row + 0x28)) > 0) return true;
    for (unsigned category = 0; category < 4; ++category) {
        const auto group = signed_value<1>(s.memory.read<1>(row + category * 8));
        if (group < 0) continue;
        const auto rules = std::uint32_t(s.memory.read<4>(s.memory.read<4>(discipline) + unsigned(group) * 4));
        const auto record = call.call(0x105bc2b0, {rules, id, 0});
        if (record && signed_value<2>(s.memory.read<2>(record + 6)) >= 0) return true;
    }
    return false;
}

void recover_player_injury(State& s, GuestCall& call, std::uint32_t medical, std::uint32_t staff) {
    // Retire through the original medical operation so the live slot and
    // completed history stay consistent. A full record reset erases history.
    call.call(0x1018b9f0, {medical, staff, 0});
    const auto queue = std::uint32_t(s.memory.read<4>(medical + 0x28));
    if (queue)
        while (const auto update = call.call(0x10186e10, {medical, staff}))
            call.call(0x10738e60, {queue, update});
    const auto condition = medical_condition(s, medical, staff);
    s.memory.copy_in(condition, s.memory.copy_out(s.memory.read<4>(s.r<4>(8) - 0x7b80), 5));
    s.memory.write<2>(condition + 8, 10000);
    s.memory.write<2>(condition + 10, 10000);
    s.memory.write<2>(condition + 12, 0);
    s.memory.write<1>(condition + 0x14, 0xff); // No deferred medical risk.
    s.memory.write<1>(condition + 0x15, 0);
    s.memory.write<1>(condition + 0x20, 0);
    s.memory.write<1>(condition + 0x21, 0xff);
}

void recover_player_suspension(State& s, GuestCall& call, std::uint32_t discipline, std::uint32_t staff) {
    const auto id = std::uint32_t(s.memory.read<4>(staff));
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
}

bool recover_debug_squad(State& s, std::uint32_t club) {
    if (!native_debugging_feature(DebugFeature::recover_players) || !club) return false;
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
        recover_player_injury(s, call, medical, staff);
        recover_player_suspension(s, call, discipline, staff);
    }
    return true;
}

namespace {
bool debug_recoverable_player(State& s, GuestCall& call, std::uint32_t staff, std::uint32_t& medical,
                              std::uint32_t& discipline, bool& injured, bool& suspended) {
    if (!debug_attribute_player(s, staff)) return false;
    medical = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7ba0)));
    discipline = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7aa4)));
    if (!medical || !discipline) return false;
    injured = debug_injured(s, medical, staff);
    suspended = debug_suspended(s, call, discipline, staff);
    return injured || suspended;
}
}

bool recover_debug_player(State& s, std::uint32_t staff) {
    if (!native_debugging_feature(DebugFeature::recover_player)) return false;
    std::uint32_t medical, discipline;
    bool injured, suspended;
    GuestCall call(s);
    if (!debug_recoverable_player(s, call, staff, medical, discipline, injured, suspended)) return false;
    // The same operations as the squad recovery, run only for what this person has.
    if (injured) recover_player_injury(s, call, medical, staff);
    if (suspended) recover_player_suspension(s, call, discipline, staff);
    return true;
}

bool append_forced_club_control(State& s) {
    // Club screen, before the original Apply for Job / Print header controls.
    const auto club = std::uint32_t(s.r<4>(31 * 4));
    if (!native_debugging_available() || !match_result_debugging(static_cast<const State&>(s)).choosing_club ||
        !valid_debug_result_club(s, club)) return false;
    // The club builder has already resolved the same ink/paper pair passed
    // to Apply for Job. Read its frame before GuestCall reserves its own.
    const auto caller_stack = std::uint32_t(s.r<4>(4));
    const auto ink = std::uint32_t(s.memory.read<2>(caller_stack + 0x6a));
    const auto paper = std::uint32_t(s.memory.read<2>(caller_stack + 0x6c));
    GuestCall call(s);
    const auto ui = std::uint32_t(s.r<4>(26 * 4));
    call.args({1, ink, call.text(debug_text(s, DebugText::force_results)), force_club_action, club});
    call.call(0x10744430, {ui, 620, 4, 785, 24, 0x30, paper, 12});
    return true;
}

void append_best_xi_control(State& s) {
    // Squad builder 1001f6a0, after its column heading advanced r14 by 25.
    // Preserve the original toolbar spacing, sharing the center with Debugging
    // only when at least one squad debugging action is enabled.
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
    const bool debug_squad = action_enabled(recover_squad_action) || action_enabled(free_squad_action) ||
                             action_enabled(extend_all_contracts_action);
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto menu_paper = std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - 0x7bcc)));
    const auto menu_ink = std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - 0x7bd0)));
    const auto menu_selected = std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - 0x7bd4)));
    // Each cell owns the original drop-down popup instead of acting directly,
    // matching the View/Sort toolbar control.
    const auto dropdown = [&](std::uint32_t left, std::uint32_t right, DebugText label) {
        call.args({0, 1, 0, 0xffffffffu});
        const auto cell_grid = call.call(0x107198c0, {page, left, top, right, top + 20, 1, 0, 1});
        call.args({paper, call.text(debug_text(s, label)), 0, 0});
        const auto cell = call.call(0x107444e0, {ui, cell_grid, 0, 0, 0x30, ink, 12, 1});
        call.args({0, 0x130, menu_paper, cell});
        return call.call(0x107198c0, {page, 0, 0, 0, 0, 0, 0, 0});
    };
    const auto item = [&](std::uint32_t menu, DebugText text, unsigned action) {
        call.args({0x10, menu_paper, menu_ink, 1, 1, menu_selected,
                   call.text(debug_text(s, text)), 0, action, 0, menu});
        call.call(0x10719740, {page, 2, 0, 0, 0, 0, 0, 0});
    };
    const auto lineup = dropdown(334, debug_squad ? 444u : 556u, DebugText::lineup);
    item(lineup, DebugText::best_xi, best_xi_action);
    item(lineup, DebugText::best_u23, best_u23_action);
    item(lineup, DebugText::b_team, b_team_action);
    if (is_national_squad(s, club)) item(lineup, DebugText::call_up, call_up_action);
    if (!debug_squad) return;
    const auto menu = dropdown(446, 556, DebugText::debugging);
    if (action_enabled(recover_squad_action)) item(menu, DebugText::recover_players, recover_squad_action);
    if (action_enabled(free_squad_action)) item(menu, DebugText::play_for_free, free_squad_action);
    if (action_enabled(extend_all_contracts_action)) item(menu, DebugText::extend_all_contracts, extend_all_contracts_action);
}

bool dispatch_best_xi_control(State& s) {
    const auto control = selected_control(s);
    if (!control) return false;
    const auto action = s.memory.read<4>(control + 8);
    if (action == force_club_action) {
        bool accepted = false;
        auto& choices = match_result_debugging(s);
        if (native_debugging_available() && choices.choosing_club && (s.memory.read<4>(control + 12) & 2)) {
            GuestCall call(s);
            const auto club = call.call(0x1073ef70, {root(s), 0});
            if (s.memory.read<4>(control + 0x48) == club) accepted = add_debug_result_club(s, club);
            if (accepted) {
                choices.choosing_club = false;
                choices.club_page = unsigned(std::ranges::find(choices.clubs, club, &ClubResultRule::club) -
                    choices.clubs.begin()) / clubs_per_page;
                open_club_results(s);
            }
        }
        s.wr<4>(12, std::uint32_t(accepted ? -4 : 0)); s.pc = s.r<4>(4128);
        return true;
    }
    const bool lineup_action = action == best_xi_action || action == best_u23_action || action == b_team_action ||
                               action == call_up_action;
    if (!lineup_action && action != recover_squad_action && action != free_squad_action &&
        action != extend_all_contracts_action) return false;
    bool accepted = false;
    if ((lineup_action || action_enabled(action)) && (s.memory.read<4>(control + 12) & 2)) {
        GuestCall call(s);
        const auto ui = root(s), club = call.call(0x1073ef70, {ui, 0});
        const auto managed = call.call(0x1073ef70, {ui, 1});
        const auto view = call.call(0x1073ef70, {ui, 4});
        const auto columns = call.call(0x1073ef70, {ui, 7});
        if (view == 1 && !(columns & 0x800) && call.call(0x1014c490, {club, 0, 1})) {
            if (action == call_up_action) accepted = call_up_national_squad(s, club, managed);
            else if (action == recover_squad_action) accepted = recover_debug_squad(s, club);
            else if (action == free_squad_action) accepted = free_debug_squad(s, club);
            else if (action == extend_all_contracts_action) accepted = extend_debug_club_contracts(s, club);
            else {
                const auto humans = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7a54)));
                const auto formation = call.call(0x101466c0, {humans, 0});
                const auto fixture = call.call(0x1073ef70, {ui, 3});
                accepted = select_debug_best_xi(s, club, managed, formation, fixture,
                    action == best_u23_action ? LineupMode::under_23 :
                    action == b_team_action ? LineupMode::b_team : LineupMode::best_xi);
            }
        }
    }
    s.wr<4>(12, std::uint32_t(accepted ? -11 : 0)); s.pc = s.r<4>(4128);
    return true;
}

namespace {
struct ContractProposal {
    std::uint32_t origin, revision, club, offer, scheduled;
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

std::uint32_t debug_primary_contract(State& s, GuestCall& call, std::uint32_t staff, bool player_only = true) {
    const auto toc = std::uint32_t(s.r<4>(8));
    if (!staff || (player_only && !s.memory.read<4>(staff + 0x14))) return 0;
    const auto id = std::uint32_t(s.memory.read<4>(staff));
    const auto count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be4)));
    if (id >= count) return 0;
    const auto manager = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b7c)));
    return manager ? call.call(0x10082f20, {manager, staff}) : 0;
}
}

bool free_debug_squad(State& s, std::uint32_t club) {
    if (!native_debugging_feature(DebugFeature::play_for_free) || !club) return false;
    const auto staff_count = signed_value<4>(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7be4)));
    GuestCall call(s);
    std::vector<std::uint32_t> players;
    bool processed = false;
    for (unsigned i = 0; i < 50; ++i) {
        const auto staff = std::uint32_t(s.memory.read<4>(club + 0x98 + i * 4));
        if (!staff || !s.memory.read<4>(staff + 0x14)) continue;
        const auto id = signed_value<4>(s.memory.read<4>(staff));
        if (id < 0 || id >= staff_count || std::find(players.begin(), players.end(), staff) != players.end()) continue;
        players.push_back(staff);
        const auto contract = debug_primary_contract(s, call, staff);
        if (!contract) continue;
        // Zero is the requested wage. The read-back also covers worlds whose
        // contract store rejects a zero word; one euro remains the fallback.
        s.memory.write<4>(contract + 0x0c, 0);
        if (s.memory.read<4>(contract + 0x0c)) s.memory.write<4>(contract + 0x0c, 1);
        for (unsigned option = 0; option < 5; ++option)
            call.call(0x10374b60, {contract, option});
        for (unsigned bonus = 0; bonus < 3; ++bonus)
            call.call(0x10374b80, {contract, bonus});
        call.call(0x10380700, {contract, 0});
        processed = true;
    }
    return processed;
}

namespace {
// Only terminalize agreements that the original has placed in its future
// transfer queue. Keeping the offer until its normal expiry also keeps older
// news items from immediately referring to a freed offer slot.
bool scheduled_sale(State& s, std::uint32_t staff, bool apply) {
    GuestCall call(s);
    const auto contract = debug_primary_contract(s, call, staff);
    if (!contract) return false;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto manager = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b7c)));
    const auto transfers = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b40)));
    if (!manager || !transfers) return false;
    const auto id = std::uint32_t(s.memory.read<4>(staff));
    const auto employer = std::uint32_t(s.memory.read<4>(contract + 4));
    const auto clubs = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b50)));
    const auto staff_count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be4)));
    const auto staff_base = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be0)));
    if (employer >= clubs) return false;
    const auto slots = std::uint32_t(s.memory.read<4>(transfers));
    const auto capacity = std::uint32_t(s.memory.read<4>(transfers + 4));
    const auto queue = std::uint32_t(s.memory.read<4>(transfers + 0xa50));
    const auto count = std::uint32_t(s.memory.read<4>(transfers + 0xa58));
    if (count > s.memory.read<4>(transfers + 0xa54) || (count && (!queue || !slots))) return false;
    const auto pending = std::uint32_t(s.memory.read<4>(manager + 0x34));
    const auto deferred = std::uint32_t(s.memory.read<4>(manager + 0x38));
    const auto pending_rows = contract_rows(s, call, pending);
    std::vector<ContractProposal> selected;
    for (std::uint32_t n = 0; n < count; ++n) {
        const auto index = std::uint32_t(s.memory.read<4>(queue + n * 12 + 8));
        if (index >= capacity) return false;
        const auto offer = std::uint32_t(s.memory.read<4>(slots + index * 4));
        if (!offer || s.memory.read<4>(offer + 0x28) != index) return false;
        const auto buyer = std::uint32_t(s.memory.read<4>(offer + 8));
        if (s.memory.read<1>(offer + 0x34) != 19 || s.memory.read<1>(offer + 0x24) != 0 || buyer >= clubs) continue;
        const auto main = std::uint32_t(s.memory.read<4>(offer + 12));
        if (main >= staff_count || !staff_base) return false;
        bool outgoing = main == id && buyer != employer;
        if (buyer == employer) for (const auto row : contract_rows(s, call, std::uint32_t(s.memory.read<4>(offer + 4))))
            if (s.memory.read<4>(row) == id) outgoing = true;
        if (!outgoing) continue;
        auto found = std::find_if(selected.begin(), selected.end(), [index](const auto& p) { return p.origin == index; });
        if (found == selected.end()) {
            selected.push_back({index, std::uint32_t(s.memory.read<4>(offer + 0x2c)), buyer, offer, 1});
        } else ++found->scheduled;
    }
    if (selected.empty()) return false;
    // ec440 identifies the pending contract by player and buyer, so reject an
    // ambiguous row rather than removing a different negotiation.
    std::vector<std::uint32_t> participants;
    for (const auto& item : selected) {
        const auto main = std::uint32_t(s.memory.read<4>(item.offer + 12));
        if (std::find(participants.begin(), participants.end(), main) == participants.end()) participants.push_back(main);
        unsigned matches = 0;
        for (const auto row : pending_rows)
            if (s.memory.read<4>(row) == main && s.memory.read<4>(row + 4) == item.club &&
                (s.memory.read<4>(row + 0x70) != item.origin || s.memory.read<4>(row + 0x74) != item.revision || ++matches > 1)) return false;
        for (const auto row : contract_rows(s, call, std::uint32_t(s.memory.read<4>(item.offer + 4)))) {
            const auto person = std::uint32_t(s.memory.read<4>(row));
            if (person >= staff_count) return false;
            if (std::find(participants.begin(), participants.end(), person) == participants.end()) participants.push_back(person);
            for (const auto pending_row : pending_rows)
                if (s.memory.read<4>(pending_row) == person && s.memory.read<4>(pending_row + 0x70) == item.origin &&
                    s.memory.read<4>(pending_row + 0x74) != item.revision) return false;
        }
    }
    if (!apply) return true;
    std::vector<std::uint32_t> other_scheduled;
    for (std::uint32_t n = 0; n < count; ++n) {
        const auto index = std::uint32_t(s.memory.read<4>(queue + n * 12 + 8));
        if (std::any_of(selected.begin(), selected.end(), [index](const auto& p) { return p.origin == index; })) continue;
        const auto offer = std::uint32_t(s.memory.read<4>(slots + index * 4));
        const auto main = std::uint32_t(s.memory.read<4>(offer + 12));
        if (std::find(participants.begin(), participants.end(), main) != participants.end()) other_scheduled.push_back(main);
        for (const auto row : contract_rows(s, call, std::uint32_t(s.memory.read<4>(offer + 4))))
            if (std::find(participants.begin(), participants.end(), s.memory.read<4>(row)) != participants.end())
                other_scheduled.push_back(std::uint32_t(s.memory.read<4>(row)));
    }
    for (const auto& item : selected) {
        for (;;) {
            const auto rows = contract_rows(s, call, deferred);
            const auto found = std::find_if(rows.begin(), rows.end(), [&](auto row) {
                return std::find(participants.begin(), participants.end(), s.memory.read<4>(row)) != participants.end() &&
                    s.memory.read<4>(row + 0x70) == item.origin && s.memory.read<4>(row + 0x74) == item.revision;
            });
            if (found == rows.end()) break;
            call.call(0x10738e60, {deferred, *found});
        }
        for (std::uint32_t n = 0; n < item.scheduled; ++n) call.call(0x103f3540, {transfers, item.offer});
        s.memory.write<1>(item.offer + 0x34, 16);
        call.call(0x103ec4f0, {transfers, item.offer});
        call.call(0x103ec440, {transfers, item.offer});
        call.call(0x103eb4a0, {transfers, item.offer, 20, 0});
        call.call(0x103f6190, {transfers, item.offer});
    }
    for (const auto person : other_scheduled) {
        const auto flags = std::uint32_t(s.memory.read<4>(transfers + 0x9a0)) + person * 10;
        s.memory.write<1>(flags, s.memory.read<1>(flags) | 0x10);
    }
    for (const auto person : participants)
        for (const auto row : contract_rows(s, call, pending))
            if (s.memory.read<4>(row) == person && s.memory.read<1>(row + 0x64) == 2) {
                call.call(0x103ea250, {transfers, staff_base + person * 0x70});
                break;
            }
    return true;
}

bool current_managed_employer(State& s, GuestCall& call, std::uint32_t staff) {
    if (!staff) return false;
    const auto contract = debug_primary_contract(s, call, staff);
    if (!contract) return false;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto id = std::uint32_t(s.memory.read<4>(contract + 4));
    const auto count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b50)));
    const auto clubs = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be8)));
    return clubs && id < count && call.call(0x1014c490, {clubs + id * 0x248, 0, 1});
}
}

bool cancel_scheduled_sale(State& s, std::uint32_t staff) {
    return native_debugging_feature(DebugFeature::cancel_sale) && scheduled_sale(s, staff, true);
}

bool remove_debug_sale_clause(State& s, std::uint32_t staff) {
    if (!native_debugging_feature(DebugFeature::remove_sale_clause)) return false;
    GuestCall call(s);
    const auto contract = debug_primary_contract(s, call, staff);
    if (!contract || !s.memory.read<1>(contract + 0x1d)) return false;
    call.call(0x10374b60, {contract, 1});
    return true;
}

namespace {
bool extend_primary_debug_contract(State& s, std::uint32_t staff, std::uint32_t contract) {
    GuestCall call(s);
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto manager = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b7c)));
    const auto transfers = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b40)));
    if (!transfers) return false;
    const auto id = std::uint32_t(s.memory.read<4>(staff));
    const auto employer = std::uint32_t(s.memory.read<4>(contract + 4));
    const auto club_count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7b50)));
    if (employer >= club_count) return false;
    const auto read_expiry = [&](std::uint32_t address) {
        return cm::calendar::Date{std::int16_t(s.memory.read<2>(address)),
                                 std::uint16_t(s.memory.read<2>(address + 2))};
    };
    // The contract is authoritative when its date is valid. Legacy/generated
    // contracts can have an invalid date while the staff's displayed expiry
    // remains valid. Extend that date instead, then synchronize both copies.
    // Neither a zero wage nor a stale cache/leap byte invalidates an expiry.
    auto next = extend_contract_date(read_expiry(contract + 0x30));
    if (!next) next = extend_contract_date(read_expiry(staff + 0x54));
    if (!next) return false;

    const auto pending = std::uint32_t(s.memory.read<4>(manager + 0x34));
    const auto deferred = std::uint32_t(s.memory.read<4>(manager + 0x38));
    const auto pending_rows = contract_rows(s, call, pending);
    const auto deferred_rows = contract_rows(s, call, deferred);
    const auto slots = std::uint32_t(s.memory.read<4>(transfers));
    const auto capacity = std::uint32_t(s.memory.read<4>(transfers + 4));
    const auto schedule = std::uint32_t(s.memory.read<4>(transfers + 0xa50));
    const auto schedule_capacity = std::uint32_t(s.memory.read<4>(transfers + 0xa54));
    const auto schedule_count = std::uint32_t(s.memory.read<4>(transfers + 0xa58));
    if (schedule_count > schedule_capacity || (schedule_count && !schedule)) return false;
    std::vector<ContractProposal> selected;
    const auto consider = [&](std::uint32_t index, std::uint32_t revision, std::uint32_t club) {
        if (club == employer || club >= club_count || index >= capacity || !slots) return;
        const auto offer = std::uint32_t(s.memory.read<4>(slots + index * 4));
        if (!offer || s.memory.read<4>(offer + 0x28) != index ||
            s.memory.read<4>(offer + 0x2c) != revision || s.memory.read<4>(offer + 12) != id ||
            s.memory.read<4>(offer + 8) != club || s.memory.read<1>(offer + 0x24) != 0 ||
            !s.memory.read<1>(offer + 0x37)) return;
        const auto timing = s.memory.read<1>(offer + 0x38) >> 4;
        const auto status = s.memory.read<1>(offer + 0x34);
        if ((timing != 3 && timing != 4) || (status >= 11 && status <= 14) || status > 19) return;
        if (std::none_of(selected.begin(), selected.end(), [index](const auto& item) { return item.origin == index; }))
            selected.push_back({index, revision, club, offer, 0});
    };
    const auto consider_row = [&](std::uint32_t row) {
        const auto state = s.memory.read<1>(row + 0x64);
        if (s.memory.read<4>(row) == id && (state == 0 || state == 2))
            consider(std::uint32_t(s.memory.read<4>(row + 0x70)),
                     std::uint32_t(s.memory.read<4>(row + 0x74)),
                     std::uint32_t(s.memory.read<4>(row + 4)));
    };
    for (auto row : pending_rows) consider_row(row);
    for (auto row : deferred_rows) consider_row(row);
    // An accepted agreement may no longer have a negotiation row, including
    // the interval before the future-transfer queue is populated.
    for (std::uint32_t index = 0; slots && index < capacity; ++index) {
        const auto offer = std::uint32_t(s.memory.read<4>(slots + index * 4));
        if (offer && s.memory.read<4>(offer + 12) == id && s.memory.read<1>(offer + 0x10) == 2)
            consider(index, std::uint32_t(s.memory.read<4>(offer + 0x2c)),
                     std::uint32_t(s.memory.read<4>(offer + 8)));
    }
    // Scheduled agreements can also be found directly in the original queue.
    for (std::uint32_t n = 0; n < schedule_count; ++n) {
        const auto index = std::uint32_t(s.memory.read<4>(schedule + n * 12 + 8));
        if (index >= capacity || !slots) return false;
        const auto offer = std::uint32_t(s.memory.read<4>(slots + index * 4));
        if (!offer) return false;
        if (s.memory.read<4>(offer + 12) == id && s.memory.read<1>(offer + 0x34) == 19)
            consider(index, std::uint32_t(s.memory.read<4>(offer + 0x2c)),
                     std::uint32_t(s.memory.read<4>(offer + 8)));
        for (auto& item : selected) if (item.origin == index) ++item.scheduled;
    }
    bool another_scheduled = false;
    for (std::uint32_t n = 0; n < schedule_count; ++n) {
        const auto index = std::uint32_t(s.memory.read<4>(schedule + n * 12 + 8));
        if (std::any_of(selected.begin(), selected.end(), [index](const auto& item) { return item.origin == index; }))
            continue;
        const auto offer = std::uint32_t(s.memory.read<4>(slots + index * 4));
        if (s.memory.read<4>(offer + 12) == id) another_scheduled = true;
        for (const auto exchange : contract_rows(s, call, std::uint32_t(s.memory.read<4>(offer + 4))))
            if (s.memory.read<4>(exchange) == id) another_scheduled = true;
    }
    // The original deletion looks up a pending row by person and club. Reject
    // an ambiguous pair before any writes, and do not remove exchange players.
    for (const auto& item : selected) {
        unsigned matching_pending = 0;
        for (auto row : pending_rows)
            if (s.memory.read<4>(row) == id && s.memory.read<4>(row + 4) == item.club) {
                if (s.memory.read<4>(row + 0x70) != item.origin || s.memory.read<4>(row + 0x74) != item.revision ||
                    (s.memory.read<1>(row + 0x64) != 0 && s.memory.read<1>(row + 0x64) != 2) || ++matching_pending > 1) return false;
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
        for (std::uint32_t n = 0; n < item.scheduled; ++n)
            call.call(0x103f3540, {transfers, item.offer});
        call.call(0x103fb630, {transfers, item.offer});
    }
    // The original unlink operations clear person-wide flags. Another scheduled
    // purchase or selected contract can still belong to this same player.
    if (!selected.empty()) {
        if (another_scheduled) {
            const auto flags = std::uint32_t(s.memory.read<4>(transfers + 0x9a0)) + id * 10;
            s.memory.write<1>(flags, s.memory.read<1>(flags) | 0x10);
        }
        for (const auto row : contract_rows(s, call, pending))
            if (s.memory.read<4>(row) == id && s.memory.read<1>(row + 0x64) == 2) {
                call.call(0x103ea250, {transfers, staff});
                break;
            }
    }
    for (const auto target : {contract + 0x30, staff + 0x54}) {
        s.memory.write<2>(target, std::uint16_t(next->day));
        s.memory.write<2>(target + 2, next->year);
        s.memory.write<1>(target + 4, cm::calendar::is_leap(next->year));
    }
    return true;
}

} // namespace

bool extend_debug_contract(State& s, std::uint32_t staff) {
    if (!native_debugging_feature(DebugFeature::extend_contract)) return false;
    GuestCall call(s);
    const auto contract = debug_primary_contract(s, call, staff);
    return contract && extend_primary_debug_contract(s, staff, contract);
}

bool extend_debug_club_contracts(State& s, std::uint32_t club) {
    if (!native_debugging_feature(DebugFeature::extend_all_contracts) || !club) return false;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto count = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be4)));
    const auto people = std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x7be0)));
    const auto employer = s.memory.read<4>(club);
    if (!people || employer >= s.memory.read<4>(s.memory.read<4>(toc - 0x7b50))) return false;
    GuestCall call(s);
    bool processed = false;
    // The roster excludes non-playing staff and players loaned out. Primary
    // contract ownership includes both and leaves borrowed players' deals alone.
    // Visit each person once, including player/coaches with both record types.
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto person = people + i * 0x70;
        const auto contract = debug_primary_contract(s, call, person, false);
        if (contract && s.memory.read<4>(contract + 4) == employer)
            processed = extend_primary_debug_contract(s, person, contract) || processed;
    }
    return processed;
}

namespace {
std::uint32_t active_debug_employer(const State& s, const guest::Cpu& c) {
    const auto ui = root(s);
    const auto manager = c.u32(ui + std::uint32_t(c.i16(ui + 0x2c58)) * 0x2c0 + 4);
    return manager ? c.u32(manager + 0x24) : 0;
}

bool human_controls_club(GuestCall& call, std::uint32_t club, bool ignore_absence) {
    // Use the transfer scheduler's test for a human-controlled club.
    return call.call(0x1014c2e0, {club, ignore_absence, 1, 0, 0}) != 0;
}

std::uint32_t eligible_ai_employer(State& s, GuestCall& call, std::uint32_t contract) {
    if (!contract || (std::int8_t(s.memory.read<1>(contract + 0x38)) >> 2) == 5) return 0;
    const guest::Cpu c(s);
    const auto id = c.u32(contract + 4), clubs = c.u32(c.toc(-0x7be8));
    if (!clubs || id >= c.u32(c.toc(-0x7b50))) return 0;
    const auto buyer = active_debug_employer(s, c), seller = clubs + id * 0x248;
    return buyer && buyer != seller && !human_controls_club(call, seller, false) ? seller : 0;
}

std::uint32_t debug_offer_at(const guest::Cpu& c, std::uint32_t transfers, std::uint32_t index) {
    if (!transfers || index >= c.u32(transfers + 4)) return 0;
    const auto slots = c.u32(transfers);
    return slots ? c.u32(slots + index * 4) : 0;
}

const ForcedPurchase* find_forced_purchase(const guest::Cpu& c, std::uint32_t offer) {
    const auto& marked = forced_purchases(static_cast<const State&>(c.s)).offers;
    if (marked.empty() || !offer || c.u8(offer + 0x24) != 0) return nullptr;
    const auto found = marked.find(c.u32(offer + 0x28));
    if (found == marked.end()) return nullptr;
    const auto& purchase = found->second;
    if (purchase.revision != c.u32(offer + 0x2c) || purchase.player != c.u32(offer + 12) ||
        purchase.buyer != c.u32(offer + 8)) return nullptr;
    return &purchase;
}
constexpr auto staff_signing_option = DebugFeature::force_staff_signing;
std::uint32_t staff_signing_role(const guest::Cpu&, std::uint32_t person);
StaffSigningClubs eligible_staff_signing_clubs(guest::Cpu&, GuestCall&, std::uint32_t person);
bool dispatch_debug_staff_signing(State&, std::uint32_t control);
}

std::uint32_t forced_purchase_seller(State& s, std::uint32_t player) {
    if (!action_enabled(force_purchase_action)) return 0;
    GuestCall call(s);
    return eligible_ai_employer(s, call, debug_primary_contract(s, call, player));
}

void record_forced_purchase(State& s, std::uint32_t transfers, std::uint32_t draft,
                            std::uint32_t expected_revision, std::uint32_t seller) {
    const guest::Cpu c(s);
    const auto index = c.u32(draft + 0x28);
    const auto offer = debug_offer_at(c, transfers, index);
    if (!offer || c.u32(draft + 0x2c) != expected_revision || c.u32(offer + 0x2c) != expected_revision ||
        c.u32(offer + 0x28) != index || c.u8(offer + 0x24) != 0 ||
        c.u32(offer + 8) != c.u32(draft + 8) || c.u32(offer + 12) != c.u32(draft + 12)) return;
    forced_purchases(s).offers.insert_or_assign(index,
        ForcedPurchase{offer, expected_revision, c.u32(offer + 12), c.u32(offer + 8), seller});
}

bool forced_purchase_for_seller(State& s, std::uint32_t offer, std::uint32_t seller) {
    const guest::Cpu c(s);
    const auto purchase = find_forced_purchase(c, offer);
    if (!purchase || purchase->seller != seller) return false;
    if (purchase->offer == offer) return true;
    // The transfer-completion screen validates its own copy of the live offer.
    const auto live = debug_offer_at(c, c.u32(c.toc(-0x7b40)), c.u32(offer + 0x28));
    return live == purchase->offer && find_forced_purchase(c, live) == purchase;
}

bool forced_purchase_contract(State& s, std::uint32_t contract) {
    const auto& marked = forced_purchases(static_cast<const State&>(s)).offers;
    if (marked.empty()) return false;
    const guest::Cpu c(s);
    const auto index = c.u32(contract + 0x70);
    const auto found = marked.find(index);
    if (found == marked.end()) return false;
    const auto& purchase = found->second;
    if (purchase.revision != c.u32(contract + 0x74) || purchase.player != c.u32(contract) ||
        purchase.buyer != c.u32(contract + 4)) return false;
    const auto live = debug_offer_at(c, c.u32(c.toc(-0x7b40)), index);
    if (live != purchase.offer || find_forced_purchase(c, live) != &purchase) return false;
    const auto status = c.u8(live + 0x34);
    return status == 7 || status == 8;
}

namespace {
std::uint32_t debug_attribute_player(const State& s, std::uint32_t staff) {
    if (!staff) return 0;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto global = [&](unsigned offset) {
        const auto slot = std::uint32_t(s.memory.read<4>(toc - offset));
        return slot ? std::uint32_t(s.memory.read<4>(slot)) : 0;
    };
    const auto people = global(0x7be0), count = global(0x7be4);
    if (!people || staff < people || (staff - people) % 0x70 || (staff - people) / 0x70 >= count ||
        std::uint64_t(staff) + 0x70 > (UINT64_C(1) << 32) || s.memory.read<4>(staff) != (staff - people) / 0x70) return 0;
    const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
    const auto players = global(0x7b38), player_count = global(0x74d4);
    if (!players || player < players || (player - players) % 0x48 || (player - players) / 0x48 >= player_count ||
        std::uint64_t(player) + 0x48 > (UINT64_C(1) << 32)) return 0;
    return player;
}
}

bool maximize_debug_player_attributes(State& s, std::uint32_t staff) {
    if (!native_debugging_feature(DebugFeature::maximize_attributes)) return false;
    const auto player = debug_attribute_player(s, staff);
    if (!player) return false;
    // The original profile getter (100d2330) decodes signed technical bytes
    // through current ability. Raw 100 saturates at displayed 20, including
    // its goalkeeper exceptions; the remaining attributes use the 1..20 scale.
    constexpr std::array technical{0x1e, 0x23, 0x24, 0x26, 0x27, 0x2a, 0x2b, 0x31, 0x32,
        0x33, 0x35, 0x37, 0x38, 0x39, 0x3a, 0x3e, 0x41, 0x43};
    constexpr std::array positive{0x1b, 0x1c, 0x1d, 0x1f, 0x20, 0x21, 0x22, 0x28, 0x29, 0x2c,
        0x2e, 0x2f, 0x30, 0x34, 0x36, 0x3b, 0x3c, 0x3d, 0x3f, 0x40, 0x42, 0x44};
    constexpr std::array personality{0x61, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69};
    for (auto offset : technical) s.memory.write<1>(player + offset, 100);
    for (auto offset : positive) s.memory.write<1>(player + offset, 20);
    s.memory.write<1>(player + 0x25, 1); // Dirtiness: lower is better.
    s.memory.write<1>(player + 0x2d, 1); // Injury proneness: lower is better.
    for (auto offset : personality) s.memory.write<1>(staff + offset, 20);
    return true;
}

void append_debug_contract_action(State& s) {
    const auto staff_enabled = native_debugging_feature(staff_signing_option);
    if (!action_enabled(extend_contract_action) && !action_enabled(remove_sale_clause_action) &&
        !action_enabled(cancel_sale_action) && !action_enabled(force_purchase_action) &&
        !action_enabled(maximize_attributes_action) && !action_enabled(recover_player_action) && !staff_enabled) return;
    const auto staff = std::uint32_t(s.r<4>(23 * 4));
    const auto original_stack = std::uint32_t(s.r<4>(4));
    GuestCall call(s);
    // Attribute changes also apply to real unattached players. Contract-only
    // actions still require a primary contract; never show an empty submenu.
    const bool maximize = action_enabled(maximize_attributes_action) && debug_attribute_player(s, staff);
    // Recovery applies to any person who is injured or suspended, with or without a contract.
    std::uint32_t medical, discipline;
    bool injured, suspended;
    const bool recover = action_enabled(recover_player_action) &&
        debug_recoverable_player(s, call, staff, medical, discipline, injured, suspended);
    const auto contract = debug_primary_contract(s, call, staff);
    const bool extend = contract && action_enabled(extend_contract_action);
    const bool clause = contract && action_enabled(remove_sale_clause_action) && s.memory.read<1>(contract + 0x1d);
    const bool sale = contract && action_enabled(cancel_sale_action) && current_managed_employer(s, call, staff) &&
        scheduled_sale(s, staff, false);
    const bool purchase = contract && action_enabled(force_purchase_action) && eligible_ai_employer(s, call, contract);
    guest::Cpu c(s);
    const bool sign_staff = staff_enabled && staff_signing_role(c, staff) && eligible_staff_signing_clubs(c, call, staff);
    if (!maximize && !recover && !extend && !clause && !sale && !purchase && !sign_staff) return;
    // Build the same parent control and child menu used by the original
    // player comparison menu, then add the debugging actions to that child.
    const auto ui = root(s), page = screen(s, ui);
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto width = std::uint32_t(s.memory.read<2>(s.memory.read<4>(toc - 0x7bcc)));
    const auto heading = call.call(0x103abf20, {original_stack + 0x6c,
        std::uint32_t(s.memory.read<2>(original_stack + 0x70)),
        std::uint32_t(s.memory.read<2>(original_stack + 0x6e)), call.text(debug_text(s, DebugText::debugging)), 0, 0});
    call.args({0, 0x30, width, heading});
    const auto submenu = call.call(0x107198c0, {page, 0, 0, 0, 0, 0, 0, 0});
    s.memory.write<2>(call.stack + 0xd0, submenu);
    const auto append = [&](DebugText text, unsigned action) {
        call.call(0x103abf20, {call.stack + 0xd0,
            std::uint32_t(s.memory.read<2>(original_stack + 0x70)),
            std::uint32_t(s.memory.read<2>(original_stack + 0x6e)),
            call.text(debug_text(s, text)), action, 0});
    };
    if (recover) append(DebugText::recover_player, recover_player_action);
    if (maximize) append(DebugText::maximize_attributes, maximize_attributes_action);
    if (extend) append(DebugText::extend_contract, extend_contract_action);
    if (clause) append(DebugText::remove_sale_clause, remove_sale_clause_action);
    if (sale) append(DebugText::cancel_sale, cancel_sale_action);
    if (purchase) append(DebugText::force_purchase, force_purchase_action);
    if (sign_staff) append(DebugText::force_staff_signing, force_staff_signing_action);
}

bool dispatch_debug_contract_action(State& s) {
    if (!native_debugging_enabled()) return false;
    const auto control = selected_control(s);
    if (!control) return false;
    const auto action = s.memory.read<4>(control + 8);
    if (action == force_staff_signing_action) return dispatch_debug_staff_signing(s, control);
    if (action != extend_contract_action && action != cancel_sale_action &&
        action != remove_sale_clause_action && action != force_purchase_action && action != maximize_attributes_action &&
        action != recover_player_action) return false;
    bool accepted = false;
    std::int32_t result = 0;
    if (s.memory.read<4>(control + 12) & 2) {
        GuestCall call(s);
        const auto staff = call.call(0x1073ef70, {root(s), 0});
        if (action == force_purchase_action && action_enabled(action)) {
            const auto seller = eligible_ai_employer(s, call, debug_primary_contract(s, call, staff));
            if (seller) {
                guest::Cpu cpu(s);
                cpu.set_lr(return_sentinel);
                s.pc = 0x10417590;
                if (transfer_screens::open_player_offer(cpu, staff, 0, cpu.u32(seller) + 1))
                    result = -4;
            }
        } else if (action == maximize_attributes_action) accepted = maximize_debug_player_attributes(s, staff);
        else if (action == recover_player_action) accepted = recover_debug_player(s, staff);
        else if (action == extend_contract_action) accepted = extend_debug_contract(s, staff);
        else if (action == remove_sale_clause_action) accepted = remove_debug_sale_clause(s, staff);
        else if (action == cancel_sale_action && current_managed_employer(s, call, staff))
            accepted = cancel_scheduled_sale(s, staff);
        // Refresh the player page after changing attributes, a contract or a transfer.
        if (accepted) {
            call.call(0x103ab9e0, {});
            result = -11;
        }
    }
    s.wr<4>(12, std::uint32_t(result));
    s.pc = s.r<4>(4128);
    return true;
}

namespace {
// These slots are unused by the negotiation and offer confirmation screens.
constexpr std::uint32_t staff_employer_field = 0x14 + 58 * 8, staff_buyer_field = 0x14 + 59 * 8;

std::uint32_t staff_signing_role(const guest::Cpu& c, std::uint32_t person) {
    if (!person || c.u32(person + 0x14) || !c.u32(person + 0x1c)) return 0;
    switch (const auto role = c.u8(person + 0x62)) {
        case 5: return 6; // A manager joins as assistant manager.
        case 6: case 8: case 9: case 10: return role;
        default: return 0;
    }
}

StaffSigningClubs eligible_staff_signing_clubs(guest::Cpu& c, GuestCall& call, std::uint32_t person) {
    // Club managers are employed without a primary record in the contract ledger.
    const auto employer = c.u32(person + 0x24), clubs = c.u32(c.toc(-0x7be8));
    if (!clubs || employer < clubs || (employer - clubs) % 0x248 ||
        (employer - clubs) / 0x248 >= c.u32(c.toc(-0x7b50))) return {};
    const auto buyer = active_debug_employer(c.s, c);
    if (!buyer || employer == buyer || human_controls_club(call, employer, true)) return {};
    return {c.u32(employer) + 1, c.u32(buyer) + 1};
}

bool matches_staff_offer(const guest::Cpu& c, std::uint32_t offer, std::uint32_t index,
                         std::uint32_t revision, std::uint32_t person, std::uint32_t buyer, std::uint32_t role) {
    // A staff offer enters review directly at stage 7, using the engine's direct-offer flag.
    return offer && c.u32(offer + 0x28) == index && c.u32(offer + 0x2c) == revision &&
        c.u32(offer + 12) == person && c.u32(offer + 8) == buyer && c.u8(offer + 0x11) == role &&
        c.u8(offer + 0x37) == 1 && c.u8(offer + 0x34) == 7;
}
}

std::uint32_t staff_signing_history(const guest::Cpu& c, std::uint32_t screens) {
    const auto group = c.i16(screens + 0x2c58);
    return group >= 0 && group < 16 ? c.u32(screens + std::uint32_t(group) * 0x2c0 + 0x10) : 0;
}

StaffSigningClubs staff_signing_clubs(const guest::Cpu& c, std::uint32_t history) {
    if (!history || !native_debugging_feature(staff_signing_option)) return {};
    return {c.u32(history + staff_employer_field), c.u32(history + staff_buyer_field)};
}

void set_staff_signing_clubs(guest::Cpu& c, std::uint32_t history, StaffSigningClubs clubs) {
    if (!history) return;
    c.put32(history + staff_employer_field, clubs.employer);
    c.put32(history + staff_buyer_field, clubs.buyer);
}

void record_forced_staff_signing(State& s, StaffSigningClubs intended, std::uint32_t person, std::uint32_t buyer,
                                 std::uint32_t role, std::uint32_t index, std::uint32_t revision) {
    if (!intended || !native_debugging_feature(staff_signing_option) || !role) return;
    guest::Cpu c(s);
    if (role != staff_signing_role(c, person) || !buyer || c.u32(buyer) + 1 != intended.buyer) return;
    const auto offer = debug_offer_at(c, c.u32(c.toc(-0x7b40)), index);
    const auto id = c.u32(person);
    if (!matches_staff_offer(c, offer, index, revision, id, intended.buyer - 1, role)) return;
    GuestCall call(s);
    const auto actual = eligible_staff_signing_clubs(c, call, person);
    if (actual.employer != intended.employer || actual.buyer != intended.buyer) return;
    forced_staff_signings(s).insert_or_assign(index, ForcedStaffSigning{offer, revision, id, role, intended});
}

bool forced_staff_contract(State& s, std::uint32_t contract, std::uint32_t person) {
    const auto& marked = forced_staff_signings(static_cast<const State&>(s));
    if (marked.empty()) return false;
    const guest::Cpu c(s);
    const auto index = c.u32(contract + 0x70);
    const auto found = marked.find(index);
    if (found == marked.end()) return false;
    const auto& signing = found->second;
    const auto offer = debug_offer_at(c, c.u32(c.toc(-0x7b40)), index);
    if (offer != signing.offer || signing.revision != c.u32(contract + 0x74) || signing.person != c.u32(person) ||
        signing.person != c.u32(contract) || signing.clubs.buyer - 1 != c.u32(contract + 4) ||
        signing.role != c.u8(contract + 0x78) || signing.role != staff_signing_role(c, person) ||
        !matches_staff_offer(c, offer, index, signing.revision, signing.person, signing.clubs.buyer - 1, signing.role)) return false;
    const auto employer = c.u32(person + 0x24);
    if (!employer || c.u32(employer) + 1 != signing.clubs.employer) return false;
    GuestCall call(s);
    return !human_controls_club(call, employer, true);
}

namespace {
bool dispatch_debug_staff_signing(State& s, std::uint32_t control) {
    std::uint32_t result = 0;
    if (native_debugging_feature(staff_signing_option) && (s.memory.read<4>(control + 12) & 2)) {
        guest::Cpu c(s);
        GuestCall call(s);
        const auto ui = root(s), person = call.call(0x1073ef70, {ui, 0});
        if (const auto role = staff_signing_role(c, person)) {
            const auto clubs = eligible_staff_signing_clubs(c, call, person);
            if (clubs) {
                const auto previous = staff_signing_history(c, ui);
                call.call(0x100960d0, {person, 0, role, 1});
                const auto opened = staff_signing_history(c, ui);
                if (opened && opened != previous) {
                    set_staff_signing_clubs(c, opened, clubs);
                    result = std::uint32_t(-4);
                }
            }
        }
    }
    s.wr<4>(12, result);
    s.pc = s.r<4>(4128);
    return true;
}
}
}
