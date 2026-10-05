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
CommandContext original_command_context(const State& s, std::uint16_t mask) {
    CommandContext c;
    // Processing uses a different poll with mask 0400. Never retain commands
    // until it becomes interactive again.
    if (mask != 0xffff || s.r<4>(4128) != 0x1076ff7c) return c;
    try {
        c.root = s.memory.read<4>(root_slot);
        if (!c.root || s.memory.read<2>(c.root + 0x2c38) == 0 ||
            s.memory.read<1>(c.root + 0x19) || s.memory.read<2>(c.root + action_mailbox)) return {};
        for (unsigned frame = 1; frame < 16; ++frame)
            if (s.memory.read<2>(c.root + 0x2c38 + frame * 2)) return {};
        c.screen = s.memory.read<4>(c.root + 0x24);
        const auto descriptor = std::uint32_t(s.memory.read<4>(c.root + 0x10));
        const auto vector = descriptor ? std::uint32_t(s.memory.read<4>(descriptor + 8)) : 0;
        c.handler = vector ? s.memory.read<4>(vector) : 0;
        if (!c.screen || !c.handler) return {};
        // 107195fc / 107196bc: signed control count, capacity 1200.
        const auto count = std::int16_t(s.memory.read<2>(c.screen + 0x129da6));
        if (count < 1 || count > 1200) return {};
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
bool deliver_original_command(State& s, const CommandContext& expected, DirectCommand command) {
    if (std::size_t(command) >= direct_command_count) return false;
    const auto current = original_command_context(s, 0xffff);
    if (!current.ready || current != expected) return false;
    const auto action = current.actions[std::size_t(command)];
    if (!action) return false;
    const auto selected = current.screen + 0x12a904;
    const auto pressed = std::uint32_t(s.memory.read<4>(0x108b4644));
    if (!pressed || s.memory.read<1>(pressed) || std::int16_t(s.memory.read<2>(selected)) != -1) return false;
    clear_previous_command_key(s);
    // 107232bc registers this index; 107705cc raises the matching input.
    // 107193cc consumes both exactly once and returns the index to the normal
    // screen handler. Unlike a global menu action, this cannot leak into a
    // nested dialog with the same action number.
    s.memory.write<2>(selected, std::uint16_t(current.controls[std::size_t(command)]));
    s.memory.write<1>(pressed, 1);
    return true;
}
}
