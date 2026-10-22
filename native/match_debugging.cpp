#include "match_debugging.hpp"
#include "aot_runtime.hpp"
#include "match_debugger_goal.hpp"
#include <optional>
#include <string>

namespace cm::aot {
namespace {
constexpr unsigned option_action = 30001, goal_action = 30002, repeat_action = 30003;
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
        run_until(s, return_sentinel);
        return std::uint32_t(s.r<4>(12));
    }
    void args(std::initializer_list<std::uint32_t> values) {
        unsigned offset = 0x38;
        for (auto value : values) { s.memory.write<4>(stack + offset, value); offset += 4; }
    }
    std::uint32_t text(const std::string& value) {
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
bool recent_valid(const State& s, std::uint32_t match) {
    return recent && recent->state == &s && recent->match == match &&
        eligible_debug_goal(s, match, recent->player) &&
        s.memory.read<4>(actor_address(match, recent->player) + 0x6c) == recent->staff;
}
}

bool eligible_debug_goal(const State& s, std::uint32_t match, unsigned player) {
    if (!match || player >= 40) return false;
    try {
        if (s.memory.read<1>(match) != 0 || signed_value<2>(s.memory.read<2>(match + 0x7948)) <= 0 ||
            s.memory.read<1>(match + 0x7929) != 0)
            return false;
        const auto actor = actor_address(match, player);
        const auto position = signed_value<1>(s.memory.read<1>(actor + 0x19));
        const auto score = signed_value<1>(s.memory.read<1>(match + (player < 20 ? 0xe03d : 0xe073)));
        return s.memory.read<4>(actor + 0x6c) && position >= 0 && position < 11 &&
            s.memory.read<1>(actor + 0x27) == player / 20 && s.memory.read<4>(actor + 0x1a4) == match &&
            score >= 0 && score < 127 && s.memory.read<1>(actor + 0xc) < 255;
    } catch (const std::runtime_error&) { return false; }
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
    recent = RecentScorer{&s, match, std::uint32_t(s.memory.read<4>(actor + 0x6c)), player};
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
        call.args({std::uint32_t(active ? selected : normal), call.text(value ? "Ligado" : "Desligado"), option_action, value});
        call.call(active ? 0x107445a0 : 0x107444e0, {ui, grid, value + 1, row, active ? 0x221u : 1u, 0, 12, 2});
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
    call.args({0x1021, 0, 0, 12, 1, color, call.text("Adicionar Gol"), 0, 0, 0, sidebar});
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
    const auto label = recent && recent->state == &s && recent->match == match ?
        "Gol " + player_name(s, recent->staff, true) : "Gol";
    call.args({0x1021, 0, 0, 12, 1, color, call.text(label), 0, repeat_action, 0, sidebar});
    call.call(0x10719740, {page, repeat ? 2u : 0u, 0, 0, 0, 0, 0, 7});
}

bool dispatch_match_debugging_control(State& s) {
    if (!native_debugging_enabled()) return false;
    const auto control = selected_control(s);
    if (!control) return false;
    const auto action = s.memory.read<4>(control + 8);
    if (action != goal_action && action != repeat_action) return false;
    unsigned player = s.memory.read<4>(control + 0x48);
    bool accepted = false;
    {
        GuestCall call(s);
        const auto fixture = call.call(0x1073ef70, {root(s), 0});
        const auto match = std::uint32_t(s.memory.read<4>(fixture + 0x58));
        const auto status = std::uint32_t(s.memory.read<4>(fixture + 0x5c));
        if (action == repeat_action && recent_valid(s, match)) player = recent->player;
        else if (action == repeat_action) player = 40;
        if ((s.memory.read<4>(control + 12) & 2) && status && !s.memory.read<1>(status + 0x56) &&
            eligible_debug_goal(s, match, player)) {
            award_debug_goal(s, match, player);
            // Publish the original match report before rebuilding the screen,
            // including its scorer/minute rows, without advancing the clock.
            const auto report = std::uint32_t(s.memory.read<4>(fixture + 0x60));
            const auto reports = s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7b4c));
            call.call(0x1029f920, {std::uint32_t(reports), match, report, 1});
            accepted = true;
        }
    }
    // The original handler's -11 result rebuilds the screen, as speed changes do.
    s.wr<4>(12, std::uint32_t(accepted ? -11 : 0)); s.pc = s.r<4>(4128);
    return true;
}
}
