#include "calendar_adapter.hpp"
#include "calendar.hpp"
#include "callbacks.hpp"
#include <iostream>
#include <set>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000;
struct Writes final : MemoryObserver {
    std::set<std::uint32_t> bytes;
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Calendar mapped memory"); }
    void written(std::uint32_t start, const std::uint8_t*, std::size_t size) override {
        for (std::size_t i = 0; i < size; ++i) bytes.insert(start + std::uint32_t(i));
    }
    void cleared(std::uint32_t start, std::uint64_t size) override { written(start, nullptr, size); }
};
void require(bool condition, const std::string& message) { if (!condition) throw std::runtime_error(message); }
void initialize(State& s, unsigned year, unsigned day, unsigned leap, std::uint32_t source,
                unsigned flags, bool shared_scratch, std::uint32_t raw_offset, bool subtract) {
    s.memory.observer = nullptr;
    const auto initial_stack = s.r<4>(4), initial_toc = s.r<4>(8);
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    s.wr<4>(4, initial_stack); s.wr<4>(8, initial_toc);
    s.memory.copy_in(fixture, std::vector<std::uint8_t>(256, 0xa5));
    const auto stack = std::uint32_t(s.r<4>(4));
    s.memory.copy_in(stack - 4096, std::vector<std::uint8_t>(4112, 0xa7));
    s.memory.write<4>(0x108b7ae4, fixture + 48);
    s.memory.write<4>(0x108b7adc, fixture + (shared_scratch ? 48 : 64));
    s.memory.write<1>(0x108b84d5, flags); s.memory.write<1>(0x108b84d3, flags);
    s.memory.write<2>(source, day); s.memory.write<2>(source + 2, year); s.memory.write<1>(source + 4, leap);
    for (unsigned r = 3; r <= 31; ++r) s.wr<4>(r * 4, 0x12340000 + r);
    s.wr<1>(2304, 0xf9); s.wr<1>(1024, 1); s.wr<1>(1027, 1);
    s.wr<4>(16, source); s.wr<4>(20, raw_offset);
    s.wr<4>(4128, return_sentinel); s.pc = subtract ? 0x10717b20 : 0x10717890;
    s.executed = 0; s.native_calls = 0; s.budget = 15000;
    s.calendar_calls = {}; s.calendar_exceptional_calls = 0;
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        auto original = std::make_unique<State>(), modern = std::make_unique<State>();
        load_program(*original, argv[1]); load_program(*modern, argv[1]);
        original->calendar_original = true;
        original->memory.map(fixture, 256); modern->memory.map(fixture, 256);
        unsigned cases = 0;
        for (unsigned year : {0u, 1u, 4u, 100u, 400u, 1900u, 1999u, 2000u, 2001u, 2004u, 2100u, 32767u, 65535u})
         for (unsigned day : {0u, 58u, 59u, 60u, 364u, 365u, 366u, 32767u, 32768u, 65535u})
          for (int offset : {-32767, -730, -366, -1, 0, 1, 366, 730, 32767})
           for (bool subtract : {false, true})
            for (unsigned alias = 0; alias < 7; ++alias) {
                const auto source = fixture + (alias == 5 ? 48 : alias == 6 ? 64 : 16);
                const auto destination = alias == 0 ? fixture + 32 : alias == 1 ? source :
                    alias == 2 ? source + 1 : alias == 3 ? fixture + 48 : alias == 4 ? fixture + 64 : fixture + 32;
                const auto raw = std::uint32_t(offset) ^ (cases % 2 ? 0x5a5a0000 : 0);
                const unsigned flags = cases % 3 == 0 ? 0 : cases % 3 == 1 ? 1 : 128;
                for (State* s : {original.get(), modern.get()}) {
                    initialize(*s, year, day, cases % 2 ? 255 : 0, source, flags, cases % 5 == 0, raw, subtract);
                    s->wr<4>(12, destination);
                }
                Writes a, b; original->memory.observer = &a; modern->memory.observer = &b;
                run(*original); run(*modern);
                original->memory.observer = nullptr; modern->memory.observer = nullptr;
                const std::string context = " case=" + std::to_string(cases) + " year=" + std::to_string(year) +
                    " day=" + std::to_string(day) + " offset=" + std::to_string(offset) + " alias=" + std::to_string(alias);
                if (original->registers != modern->registers) {
                    for (unsigned i = 0; i < original->registers.size(); ++i)
                        if (original->registers[i] != modern->registers[i])
                            std::cerr << "register byte " << i << " expected=" << unsigned(original->registers[i])
                                      << " actual=" << unsigned(modern->registers[i]) << '\n';
                    throw std::runtime_error("Direct ABI mismatch" + context);
                }
                require(original->unique == modern->unique && original->pc == modern->pc,
                        "Temporary or return PC mismatch" + context);
                require(a.bytes == b.bytes, "Calendar write footprint mismatch" + context);
                for (auto address : a.bytes) require(original->memory.read<1>(address) == modern->memory.read<1>(address),
                    "Calendar memory mismatch at " + hex_address(address) + context);
                require(modern->calendar_calls[subtract ? 1 : 0] == 1 && !modern->calendar_exceptional_calls &&
                        original->native_calls == 0 && modern->native_calls == 0, "Modern dispatch was not exercised" + context);
                ++cases;
            }
        // SHRT_MIN has no terminating date result: preserve bounded failure.
        for (bool subtract : {false, true}) {
            for (State* s : {original.get(), modern.get()}) {
                initialize(*s, 2000, 59, 1, fixture + 16, 0, false, 0xffff8000, subtract);
                s->wr<4>(12, fixture + 32); s->budget = 400;
                bool stopped = false;
                try { run(*s); } catch (const std::runtime_error&) { stopped = true; }
                require(stopped, "SHRT_MIN unexpectedly returned");
            }
            require(original->registers == modern->registers && original->executed == modern->executed &&
                    original->pc == modern->pc && modern->calendar_exceptional_calls,
                    "SHRT_MIN compatibility failure differs");
            require(original->memory.copy_out(0x70000000, 0x100000) == modern->memory.copy_out(0x70000000, 0x100000) &&
                    original->memory.copy_out(fixture, 256) == modern->memory.copy_out(fixture, 256) &&
                    original->memory.copy_out(0x108b84d3, 3) == modern->memory.copy_out(0x108b84d3, 3),
                    "SHRT_MIN stack/scratch/flag failure residue differs");
        }
        std::cout << cases << " original/modern direct calendar comparisons: full registers, temporaries, PC, write footprint and every written byte; extreme offsets, invalid days, century/year wrap, inconsistent leap/flags and aliases; SHRT_MIN bounded failures\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
