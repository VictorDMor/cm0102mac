#include "aot_runtime.hpp"
#include "direct_commands.hpp"
#include "callbacks.hpp"
#include "match_debugging.hpp"
#include "ui_bridge.hpp"
#include <iostream>
using namespace cm::aot;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        auto state = std::make_unique<State>(); auto& s = *state;
        load_program(s, argv[1]);
        const auto root = allocate(s, 0x260000, true), screen = allocate(s, 0x130000, true);
        const auto descriptor = allocate(s, 0x200, true), vector = allocate(s, 8, true);
        s.memory.write<4>(0x108b3ee0, root);
        s.memory.write<2>(root + 0x2c38, 1);
        s.memory.write<4>(root + 0x24, screen);
        s.memory.write<4>(root + 0x10, descriptor);
        s.memory.write<4>(descriptor + 8, vector);
        s.memory.write<4>(vector, 0x10334940);
        s.wr<4>(4128, 0x1076ff7c);
        const auto pressed = std::uint32_t(s.memory.read<4>(0x108b4644));
        s.memory.write<1>(pressed, 0);
        s.memory.write<2>(screen + 0x12a904, 0xffff);
        auto controls = [&](std::initializer_list<std::int16_t> actions) {
            s.memory.write<2>(screen + 0x129da6, actions.size());
            unsigned i = 0;
            for (auto action : actions) {
                s.memory.write<4>(screen + 0xba864 + i * 0x17c + 8, std::uint32_t(action));
                s.memory.write<4>(screen + 0xba864 + i * 0x17c + 12, 2); ++i;
            }
        };
        controls({3,6,1000});
        auto c = original_command_context(s, 0xffff);
        require(c.ready && c.actions[2] == 3 && c.actions[3] == 6 && c.actions[0] == 0, "Startup command whitelist");
        require(!original_command_context(s, 0x400).ready, "Loading must reject commands");
        s.wr<4>(4128, 0x1077018c);
        require(!original_command_context(s, 0xffff).ready, "Only the original interactive event boundary is supported");
        s.wr<4>(4128, 0x1076ff7c);
        const auto before = s.memory.copy_out(root, 0x260000);
        const auto character = std::uint32_t(s.memory.read<4>(0x108b45e0));
        const auto key = std::uint32_t(s.memory.read<4>(0x108b5e74));
        s.memory.write<1>(character, 13); s.memory.write<4>(key, 36);
        require(deliver_original_command(s, c, DirectCommand::load_game), "Named Load should select its original control");
        require(s.memory.read<1>(character) == 0 && s.memory.read<4>(key) == 0,
            "Previous Return must not close the new filename editor");
        require(!deliver_original_command(s, c, DirectCommand::load_game), "Duplicate delivery while input occupied");
        require(s.memory.read<2>(screen + 0x12a904) == 0 && s.memory.read<1>(pressed) == 1, "Load original control identity");
        require(before == s.memory.copy_out(root, 0x260000), "Named control changed root, career or global menu action data");
        const auto picker = allocate(s, 8, true), stack = allocate(s, 0x10000, true);
        s.memory.write<4>(picker, 0x10719350); s.memory.write<4>(picker + 4, 0x108bba90);
        s.wr<4>(4, stack + 0xff00);
        s.memory.write<1>(s.memory.read<4>(0x108b5f7c), 0);
        const std::array<std::uint32_t,1> arguments{screen};
        require(call_original(s, picker, arguments) == 0, "Original picker did not deliver selected control index");
        require(s.memory.read<2>(screen + 0x12a904) == 0xffff && s.memory.read<1>(pressed) == 0,
            "Original picker failed to consume command exactly once");
        s.memory.write<4>(vector, 0x1033f840); controls({40,41});
        s.memory.write<1>(character, 'x'); s.memory.write<4>(key, 7);
        require(!deliver_original_command(s, c, DirectCommand::load_game), "Stale screen command accepted");
        require(s.memory.read<1>(character) == 'x' && s.memory.read<4>(key) == 7,
            "Rejected commands must preserve the current keyboard input");
        c = original_command_context(s, 0xffff);
        require(c.actions[4] == 40 && c.actions[0] == 0 && c.actions[2] == 0, "Options modal must expose only Cancel");
        require(deliver_original_command(s, c, DirectCommand::back_or_cancel), "Cancel original selected control");
        require(call_original(s, picker, arguments) == 0, "Original picker Cancel control index");
        s.memory.write<4>(vector, 0x100ee9d0); controls({3,4,1000});
        c = original_command_context(s, 0xffff);
        require(c.actions[4] == 3 && c.actions[0] == 0, "File modal whitelist and stale Continue rejection");
        s.memory.write<4>(vector, 0x102c0000); controls({1000,1022,1059,1071,-2});
        c = original_command_context(s, 0xffff);
        require(c.actions == std::array<std::int16_t,direct_command_count>{1000,1022,0,1059,-2,0}, "Career action identities and startup-only Load");
        s.memory.write<2>(root + 0x2c3a, 1);
        require(!original_command_context(s, 0xffff).ready, "Nested active frames must reject commands");
        s.memory.write<2>(root + 0x2c3a, 0);
        s.memory.write<4>(screen + 0xba864 + 12, 1);
        require(!deliver_original_command(s, c, DirectCommand::continue_game), "Disabled action must invalidate old context");
        controls({40});
        require(original_command_context(s, 0xffff).actions[0] == 0, "Must ignore records beyond live control count");
        s.memory.write<2>(screen + 0x129da6, 1201);
        require(!original_command_context(s, 0xffff).ready, "Invalid control capacity");

        controls({1022,1000,42});
        s.memory.write<2>(screen + 0x12a936, 0xffff);
        UIEvent z{UIEvent::Kind::key_down}; z.message = (6u << 8) | 'z';
        const auto refused_key = [&](const UIEvent& event, const CommandContext& context) {
            const auto old_root = s.memory.copy_out(root, 0x260000);
            const auto old_screen = s.memory.copy_out(screen, 0x130000);
            s.memory.write<1>(character, 'x'); s.memory.write<4>(key, 7);
            require(!deliver_match_shortcut(s, context, event), "Unavailable shortcut consumed keyboard input");
            require(old_root == s.memory.copy_out(root, 0x260000) && old_screen == s.memory.copy_out(screen, 0x130000) &&
                    s.memory.read<1>(character) == 'x' && s.memory.read<4>(key) == 7 && s.memory.read<1>(pressed) == 0,
                "Refused shortcut changed guest state");
        };
        for (const auto handler : {0x102c0000u, 0x10036f10u, 0x1028bc90u, 0x1028dc80u}) {
            s.memory.write<4>(vector, handler); // Ordinary, squad, daily fixtures and live match screens.
            if (handler == 0x1028dc80u) controls({1000,2,42}); // Match play/pause takes precedence over Continue.
            for (const bool uppercase : {false, true}) {
                z.message = (6u << 8) | (uppercase ? 'Z' : 'z'); z.modifiers = uppercase ? 0x200 : 0;
                c = original_command_context(s, 0xffff);
                const auto career = s.memory.copy_out(root, 0x260000);
                require(deliver_match_shortcut(s, c, z), "Z did not activate Continue or match play/pause");
                require(!deliver_match_shortcut(s, c, z), "Z duplicated an occupied input");
                require(call_original(s, picker, arguments) == 1, "Z did not use the original Continue control picker");
                require(career == s.memory.copy_out(root, 0x260000), "Z bypassed Continue and changed career state");
                s.wr<4>(4128, 0x1076ff7c);
                z.kind = UIEvent::Kind::auto_key;
                require(deliver_match_shortcut(s, original_command_context(s, 0xffff), z) &&
                        s.memory.read<1>(pressed) == 0 && s.memory.read<2>(screen + 0x12a904) == 0xffff,
                    "Held Z repeated Continue");
                z.kind = UIEvent::Kind::key_down;
            }
        }
        s.memory.write<4>(vector, 0x1028bc90); controls({1022,1000,42});
        z.message = (6u << 8) | 'z'; z.modifiers = 0;
        c = original_command_context(s, 0xffff);
        for (const auto modifier : {0x100, 0x800, 0x1000}) {
            z.modifiers = modifier; refused_key(z, c);
        }
        z.modifiers = 0; z.kind = UIEvent::Kind::key_up; refused_key(z, c);
        z.kind = UIEvent::Kind::key_down; z.message = 'a'; refused_key(z, c);
        z.message = '1'; refused_key(z, c); // Debug number keys remain match-only.
        z.message = (6u << 8) | 'z';
        const auto field = screen + 0xba864 + 2 * 0x17c;
        s.memory.write<4>(field + 12, 8); s.memory.write<4>(field + 0x44, 0);
        refused_key(z, original_command_context(s, 0xffff));
        z.kind = UIEvent::Kind::auto_key; refused_key(z, original_command_context(s, 0xffff));
        z.kind = UIEvent::Kind::key_down;
        s.memory.write<4>(field + 0x44, 0xffffffffu); s.memory.write<2>(screen + 0x12a936, 2);
        refused_key(z, original_command_context(s, 0xffff));
        s.memory.write<2>(screen + 0x12a936, 0xffff);
        require(deliver_match_shortcut(s, original_command_context(s, 0xffff), z), "Unfocused text field blocked Continue");
        require(call_original(s, picker, arguments) == 1, "Unfocused text field changed Continue target");
        s.wr<4>(4128, 0x1076ff7c);
        c = original_command_context(s, 0xffff);
        s.memory.write<4>(screen + 0xba864 + 0x17c + 12, 0);
        refused_key(z, c); refused_key(z, original_command_context(s, 0xffff));
        controls({1022,1000,42}); c = original_command_context(s, 0xffff);
        s.memory.write<2>(root + 0x2c3a, 1); refused_key(z, c);
        s.memory.write<2>(root + 0x2c3a, 0);
        s.memory.write<1>(root + 0x19, 1); refused_key(z, c);
        s.memory.write<1>(root + 0x19, 0);
        refused_key(z, original_command_context(s, 0x400));
        s.wr<4>(4128, 0x1077018c); refused_key(z, c);
        s.wr<4>(4128, 0x1076ff7c);
        for (const auto handler : {0x10334940u, 0x100ee9d0u, 0x1033f840u}) {
            s.memory.write<4>(vector, handler);
            refused_key(z, c); refused_key(z, original_command_context(s, 0xffff));
        }
        s.memory.write<4>(vector, 0x102c0000); controls({42});
        refused_key(z, original_command_context(s, 0xffff));

        const auto best = std::size_t(DirectCommand::best_xi);
        s.memory.write<4>(vector, 0x10036f10);
        s.memory.write<4>(descriptor + 0x14, 123); // Club.
        s.memory.write<4>(descriptor + 0x14 + 4 * 8, 1); // Squad section.
        controls({1000,30004,30023,30024});
        const auto best_control = screen + 0xba864 + 0x17c;
        s.memory.write<4>(best_control + 0x38, 0x10); // Closed Lineup popup item.
        const std::string title = "     Melhor 11";
        s.memory.copy_in(best_control + 0x78, std::vector<std::uint8_t>(title.begin(), title.end()));
        c = original_command_context(s, 0xffff);
        require(c.actions[best] == 30004 && c.controls[best] == 1 && c.best_xi_title == "Melhor 11",
            "Best XI must reuse the enabled localized Lineup item");
        const auto squad_root = s.memory.copy_out(root, 0x260000);
        require(deliver_original_command(s, c, DirectCommand::best_xi), "Best XI shortcut rejected the closed popup item");
        require(!deliver_original_command(s, c, DirectCommand::best_xi), "Best XI was delivered twice");
        require(call_original(s, picker, arguments) == 1 && s.memory.read<1>(pressed) == 0 &&
                s.memory.read<2>(screen + 0x12a904) == 0xffff,
            "Best XI did not pass through the original picker exactly once");
        require(squad_root == s.memory.copy_out(root, 0x260000), "Best XI shortcut bypassed the handler and changed game state");
        s.wr<4>(4128, 0x1076ff7c);
        for (unsigned index : {0u,1u,3u,4u,7u}) {
            const auto address = descriptor + 0x14 + index * 8;
            const auto value = s.memory.read<4>(address);
            s.memory.write<4>(address, value + 1);
            require(!deliver_original_command(s, c, DirectCommand::best_xi), "Best XI accepted a stale squad/fixture context");
            s.memory.write<4>(address, value);
        }
        s.memory.write<4>(descriptor + 0x14 + 7 * 8, 0x800);
        require(!original_command_context(s, 0xffff).actions[best], "Best XI accepted a locked squad list");
        s.memory.write<4>(descriptor + 0x14 + 7 * 8, 0);
        s.memory.write<4>(best_control + 12, 0);
        require(!original_command_context(s, 0xffff).actions[best] &&
            !deliver_original_command(s, c, DirectCommand::best_xi), "Best XI accepted a disabled menu item");
        controls({1000,30023,30024});
        require(!original_command_context(s, 0xffff).actions[best], "Best XI appeared without its squad item");
        controls({30004});
        s.memory.write<4>(vector, 0x102c0000);
        require(!original_command_context(s, 0xffff).actions[best], "Best XI appeared on another screen");
        s.memory.write<4>(vector, 0x10036f10);
        s.memory.write<2>(root + 0x2c3a, 1);
        require(!deliver_original_command(s, c, DirectCommand::best_xi), "Best XI entered a nested dialog");
        std::cout << "Original commands, global Z, text editing, held/modifier/modal/busy guards and picker consumption checks passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
