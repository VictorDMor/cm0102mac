#include "aot_runtime.hpp"
#include "direct_commands.hpp"
#include "callbacks.hpp"
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
        const auto descriptor = allocate(s, 24, true), vector = descriptor + 16;
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
        require(c.actions == std::array<std::int16_t,5>{1000,1022,0,1059,-2}, "Career action identities and startup-only Load");
        s.memory.write<2>(root + 0x2c3a, 1);
        require(!original_command_context(s, 0xffff).ready, "Nested active frames must reject commands");
        s.memory.write<2>(root + 0x2c3a, 0);
        s.memory.write<4>(screen + 0xba864 + 12, 1);
        require(!deliver_original_command(s, c, DirectCommand::continue_game), "Disabled action must invalidate old context");
        controls({40});
        require(original_command_context(s, 0xffff).actions[0] == 0, "Must ignore records beyond live control count");
        s.memory.write<2>(screen + 0x129da6, 1201);
        require(!original_command_context(s, 0xffff).ready, "Invalid control capacity");
        std::cout << "Original command context, stale/duplicate rejection, modal and control consumption checks passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
