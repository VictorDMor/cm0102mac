#include "aot_runtime.hpp"
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t pool = 0x31000000, arena = pool + 0x100;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
std::unique_ptr<State> setup(const char* program, std::uint32_t hint) {
    auto state = std::make_unique<State>();
    load_program(*state, program);
    state->allocator_original = true; // Keep this fixture on the guarded translated reference.
    auto& s = *state;
    s.memory.map(pool, 0x2000);
    s.memory.clear(pool, 0x2000);
    s.memory.write<4>(pool, arena);
    s.memory.write<4>(arena, arena);
    s.memory.write<4>(arena + 4, arena);
    s.memory.write<4>(arena + 16, hint);
    s.memory.write<4>(arena + 20, 0x1003);
    // A fully occupied arena with a null free-list head. The live failure had
    // this valid block layout with a stale, optimistic largest-block hint.
    s.memory.write<4>(arena + 24, 0xfe2);
    s.memory.write<4>(arena + 28, arena | 1);
    s.memory.write<4>(arena + 0xff8, 0x1007);
    return state;
}
std::uint32_t allocate_object(State& s, unsigned size) {
    s.wr<4>(12, pool);
    s.wr<4>(16, size);
    s.wr<4>(4128, return_sentinel);
    s.pc = 0x10774e80;
    s.budget = s.executed + 500000;
    run(s);
    return s.r<4>(12);
}
}
int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) return 2;
    const bool reproduce = argc == 3 && std::string(argv[2]) == "--expect-retry-loop";
    if (argc == 3 && !reproduce) return 2;
    try {
        if (reproduce) {
            auto s = setup(argv[1], 0x7b0);
            try { (void)allocate_object(*s, 44); }
            catch (const std::runtime_error& error) {
                require(std::string(error.what()) == "Native instruction budget exhausted",
                        "Reference failed outside the audited retry loop");
                require(s->pc >= 0x10774190 && s->pc < 0x10774d20, "Reference stalled outside allocator");
                std::cout << "Reproduced unmodified pool retry loop after 500000 original instructions\n";
                return 0;
            }
            throw std::runtime_error("Unmodified allocator unexpectedly made progress");
        }
        unsigned cases = 0;
        for (auto hint : {0u, 0x100u, 0x7b0u, 0x1010u}) {
            for (auto size : {1u, 12u, 44u, 76u}) {
                auto s = setup(argv[1], hint);
                const auto first = allocate_object(*s, size);
                require(first != 0, "Pool replenishment failed to return memory");
                const std::vector<std::uint8_t> payload(size, 0x5a);
                s->memory.copy_in(first, payload);
                const auto second = allocate_object(*s, size);
                require(second && (second + size <= first || first + size <= second), "Pool objects overlap");
                s->memory.clear(second, size);
                require(s->memory.copy_out(first, size) == payload, "Pool allocation damaged an existing object");
                require(s->memory.read<4>(arena + 24) == 0xfe2 && s->memory.read<4>(arena + 0xffc) == 0,
                        "Retry changed the occupied arena or fabricated a free block");
                ++cases;
            }
        }
        std::cout << cases << " original allocator replenishment cases passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
