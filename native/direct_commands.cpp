#include "direct_commands.hpp"
#include "aot_runtime.hpp"
#include <algorithm>

namespace cm::aot {
namespace {
constexpr std::uint32_t root_slot = 0x108b3ee0;
constexpr std::uint32_t action_mailbox = 0x25819e;
constexpr std::uint32_t startup_handler = 0x10334940;
constexpr std::uint32_t file_handler = 0x100ee9d0;
constexpr std::uint32_t options_handler = 0x1033f840;
}
std::optional<CommandScreen> original_command_screen(const State& s, std::uint16_t mask) {
    // Processing uses a different poll with mask 0400. Never retain commands
    // until it becomes interactive again.
    if (mask != 0xffff || s.r<4>(4128) != 0x1076ff7c) return std::nullopt;
    try {
        CommandScreen c;
        c.root = s.memory.read<4>(root_slot);
        if (!c.root || s.memory.read<2>(c.root + 0x2c38) == 0 ||
            s.memory.read<1>(c.root + 0x19) || s.memory.read<2>(c.root + action_mailbox)) return std::nullopt;
        for (unsigned frame = 1; frame < 16; ++frame)
            if (s.memory.read<2>(c.root + 0x2c38 + frame * 2)) return std::nullopt;
        c.screen = s.memory.read<4>(c.root + 0x24);
        const auto descriptor = std::uint32_t(s.memory.read<4>(c.root + 0x10));
        const auto vector = descriptor ? std::uint32_t(s.memory.read<4>(descriptor + 8)) : 0;
        c.handler = vector ? s.memory.read<4>(vector) : 0;
        if (!c.screen || !c.handler) return std::nullopt;
        // 107195fc / 107196bc: signed control count, capacity 1200.
        c.count = std::int16_t(s.memory.read<2>(c.screen + 0x129da6));
        if (c.count < 1 || c.count > 1200) return std::nullopt;
        return c;
    } catch (const std::runtime_error&) { return std::nullopt; }
}
CommandContext original_command_context(const State& s, std::uint16_t mask) {
    const auto screen = original_command_screen(s, mask);
    return screen ? original_command_context(s, *screen) : CommandContext{};
}
CommandContext original_command_context(const State& s, const CommandScreen& screen) {
    CommandContext c;
    c.root = screen.root; c.screen = screen.screen; c.handler = screen.handler;
    const auto count = screen.count;
    try {
        std::vector<std::pair<std::int16_t, std::int16_t>> enabled;
        c.identity = (std::uint64_t(c.handler) << 32) | c.screen;
        for (int i = 0; i < count; ++i) {
            const auto control = c.screen + 0xba864 + i * 0x17c;
            const auto action = std::int16_t(s.memory.read<4>(control + 8));
            const auto flags = std::uint32_t(s.memory.read<4>(control + 12));
            c.identity = (c.identity ^ std::uint16_t(action) ^ (std::uint64_t(flags) << 16)) * 1099511628211ULL;
            if (flags & 2) enabled.emplace_back(action, i);
        }
        auto allow = [&](DirectCommand command, std::int16_t action) {
            const auto found = std::find_if(enabled.begin(), enabled.end(), [action](const auto& item) { return item.first == action; });
            if (found != enabled.end()) {
                c.actions[std::size_t(command)] = action;
                c.controls[std::size_t(command)] = found->second;
            }
        };
        if (c.handler == startup_handler) {
            allow(DirectCommand::load_game, 3);
            allow(DirectCommand::open_game_options, 6);
        } else if (c.handler == file_handler) {
            allow(DirectCommand::back_or_cancel, 3);
        } else if (c.handler == 0x100f0ba0) {
            allow(DirectCommand::back_or_cancel, 1);
        } else if (c.handler == options_handler) {
            allow(DirectCommand::back_or_cancel, 40);
        } else {
            allow(DirectCommand::continue_game, 1000);
            allow(DirectCommand::save_game, 1022);
            allow(DirectCommand::open_game_options, 1059);
            // Load is available on the start screen only. The Mac original
            // Restart action ends at the empty 1076f880 routine; do not offer
            // a command which accepts confirmation but cannot restart.
            allow(DirectCommand::back_or_cancel, -2);
        }
        c.ready = true;
        return c;
    } catch (const std::runtime_error&) { return {}; }
}
void clear_previous_command_key(State& s) {
    // 107703ac..107704b8 stores character and hardware key independently.
    // The file handler consumes Return's character but can leave key code 36;
    // 1071b70c then closes the next filename editor before its first input.
    // Replace both old fields, without discarding subsequent queued events.
    const auto character = std::uint32_t(s.memory.read<4>(0x108b45e0));
    const auto key = std::uint32_t(s.memory.read<4>(0x108b5e74));
    s.memory.write<1>(character, 0);
    s.memory.write<4>(key, 0);
}
std::vector<OriginalControl> original_controls(const State& s, const CommandContext& c) {
    // Copies, not scalar loads: a host listing must not reach read observers.
    std::vector<OriginalControl> result;
    if (!c.ready) return result;
    const auto counted = s.memory.copy_out(c.screen + 0x129da6, 2);
    const auto count = std::int16_t(counted[0] << 8 | counted[1]);
    if (count < 1 || count > 1200) return result;
    const auto table = s.memory.copy_out(c.screen + 0xba864, std::size_t(count) * 0x17c);
    const auto word = [&](std::size_t at) {
        return std::uint32_t(table[at]) << 24 | std::uint32_t(table[at + 1]) << 16 | std::uint32_t(table[at + 2]) << 8 | table[at + 3];
    };
    for (std::int16_t i = 0; i < count; ++i)
        result.push_back({i, std::int16_t(word(i * 0x17c + 8)), word(i * 0x17c + 12)});
    return result;
}
std::optional<std::array<std::int32_t, 4>> original_control_rect(const State& s, const CommandContext& c, std::int16_t index) {
    if (!c.ready) return std::nullopt;
    const auto counted = s.memory.copy_out(c.screen + 0x129da6, 2);
    if (index < 0 || index >= std::int16_t(counted[0] << 8 | counted[1])) return std::nullopt;
    // +0x10: left, top, right, bottom as 32-bit words, in window coordinates.
    const auto record = s.memory.copy_out(c.screen + 0xba864 + std::uint32_t(index) * 0x17c + 0x10, 16);
    std::array<std::int32_t, 4> rect{};
    for (unsigned i = 0; i < 4; ++i)
        rect[i] = std::int32_t(std::uint32_t(record[i * 4]) << 24 | std::uint32_t(record[i * 4 + 1]) << 16 |
                               std::uint32_t(record[i * 4 + 2]) << 8 | record[i * 4 + 3]);
    return rect;
}
namespace {
bool activate(State& s, const CommandContext& current, std::int16_t index) {
    const auto selected = current.screen + 0x12a904;
    const auto pressed = std::uint32_t(s.memory.read<4>(0x108b4644));
    if (!pressed || s.memory.read<1>(pressed) || std::int16_t(s.memory.read<2>(selected)) != -1) return false;
    clear_previous_command_key(s);
    // 107232bc registers this index; 107705cc raises the matching input.
    // 107193cc consumes both exactly once and returns the index to the normal
    // screen handler. Unlike a global menu action, this cannot leak into a
    // nested dialog with the same action number.
    s.memory.write<2>(selected, std::uint16_t(index));
    s.memory.write<1>(pressed, 1);
    return true;
}
}
bool deliver_original_command(State& s, const CommandContext& expected, DirectCommand command) {
    if (std::size_t(command) >= direct_command_count) return false;
    const auto current = original_command_context(s, 0xffff);
    if (!current.ready || current != expected || !current.actions[std::size_t(command)]) return false;
    return activate(s, current, current.controls[std::size_t(command)]);
}
// An item of a closed pop-up menu: kind 0x10 (+0x38) and no rectangle (+0x10)
// until its pop-up opens. A user reaches it only through the pop-up, while
// holding the mouse on its button; activated directly it once left the game
// reading menu state that was never set up (a null read at 0x100d4ea0).
static bool closed_popup_item(const State& s, const CommandContext& c, std::int16_t index) {
    const auto record = s.memory.copy_out(c.screen + 0xba864 + std::uint32_t(index) * 0x17c, 0x3c);
    const bool menu_item = record[0x3b] & 0x10;
    const bool no_rectangle = std::all_of(record.begin() + 0x10, record.begin() + 0x20, [](auto byte) { return byte == 0; });
    return menu_item && no_rectangle;
}
bool deliver_original_control(State& s, const CommandContext& expected, std::int16_t index) {
    const auto current = original_command_context(s, 0xffff);
    if (!current.ready || current != expected) return false;
    const auto controls = original_controls(s, current);
    if (index < 0 || std::size_t(index) >= controls.size() || !(controls[std::size_t(index)].flags & 2)) return false;
    if (closed_popup_item(s, current, index)) return false;
    return activate(s, current, index);
}
}
