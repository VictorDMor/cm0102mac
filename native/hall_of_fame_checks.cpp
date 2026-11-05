#include "hall_of_fame_adapter.hpp"
#include "guest_abi.hpp"
#include "semantic_io_checks.hpp"

#include <random>

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto entry = 0x10142100u, initialize = 0x101407a0u, done = fixture + 0x80;
constexpr auto first = fixture + 0x100, second = fixture + 0x200;
constexpr auto absent = fixture + 0x4000;

enum class Stop { returned, unmapped, budget };

void compare_call(const State& initial, Stop expected, bool corrupt = false) {
    auto original = clone(initial), rewrite = clone(initial);
    Effects original_effects, rewrite_effects;
    const auto invoke = [&](State& s, Effects& effects, bool old) {
        effects.state = &s;
        s.memory.date_observer = &effects;
        try {
            if (old) hall_of_fame_run_original(s, initial.pc);
            else run_until(s, done);
        } catch (const std::runtime_error& error) {
            const std::string_view message = error.what();
            s.memory.date_observer = nullptr;
            if (message == "Native instruction budget exhausted") return std::pair{Stop::budget, std::string(message)};
            if (message.starts_with("Unmapped data access at "))
                return std::pair{Stop::unmapped, std::string(message)};
            throw;
        }
        s.memory.date_observer = nullptr;
        require(s.pc == done, "Hall of Fame routine returned to a different address");
        return std::pair{Stop::returned, std::string{}};
    };
    const auto a = invoke(*original, original_effects, true), b = invoke(*rewrite, rewrite_effects, false);
    require(a.first == expected && a == b, "Hall of Fame stop or fault changed");
    require(original_effects.bytes == rewrite_effects.bytes, "Hall of Fame ordered memory effects changed");
    if (corrupt) {
        Cpu c(*rewrite);
        c.set(3, c.r(3) ^ 1);
    }
    Image(*original).compare(*rewrite, corrupt ? "corrupted comparator" : "Hall of Fame " + hex_address(initial.pc), expected == Stop::unmapped);
}

std::unique_ptr<State> seed(std::mt19937& random) {
    auto state = std::make_unique<State>();
    state->memory.map(fixture, 0x1000);
    Cpu c(*state);
    for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
    state->wr<1>(guest::reg::xer_so, random() & 1);
    state->wr<1>(guest::reg::xer_ca, random() & 1);
    c.set({{3, first}, {4, second}});
    c.set_lr(done);
    state->pc = entry;
    state->budget = 100;
    return state;
}

void check_rank() {
    std::mt19937 random(0x484f464d);
    constexpr std::array values{0u, 1u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xfffffffeu, 0xffffffffu};
    unsigned completed = 0;
    for (auto a : values) {
        for (auto b : values) {
            for (const bool tie : {false, true}) {
                auto s = seed(random);
                Cpu c(*s);
                c.put32(first + 0x68, tie ? 10 : a);
                c.put32(second + 0x68, tie ? 10 : b);
                c.put32(first + 0x64, a);
                c.put32(second + 0x64, b);
                compare_call(*s, Stop::returned);
                ++completed;
            }
        }
    }
    for (auto left : {0u, first, absent}) {
        for (auto right : {0u, first, absent}) {
            auto s = seed(random);
            Cpu c(*s);
            c.set({{3, left}, {4, right}});
            const auto faults = left && right && (left == absent || right == absent);
            compare_call(*s, faults ? Stop::unmapped : Stop::returned);
            completed += !faults;
        }
    }
    auto s = seed(random);
    bool rejected = false;
    try { compare_call(*s, Stop::returned, true); }
    catch (const std::runtime_error& error) {
        rejected = std::string_view(error.what()).starts_with("corrupted comparator register byte=");
    }
    require(rejected, "wrong comparator result was not detected");
    s->budget = 1;
    compare_call(*s, Stop::budget);
    std::cout << completed << " Hall of Fame ranking comparisons, three mapped-range faults, "
                 "one budget stop and one corruption control\n";
}

void check_record_initialization() {
    std::mt19937 random(0x52454344);
    for (unsigned test = 0; test < 32; ++test) {
        auto s = seed(random);
        Cpu c(*s);
        s->memory.map(stack - 0x1000, 0x2000);
        for (unsigned offset = 0; offset < 0x100; offset += 4) c.put32(stack - offset, random());
        for (unsigned offset = 0; offset < 0x78; offset += 4) c.put32(first + offset, random());
        c.set({{1, stack}, {3, first}});
        s->pc = initialize;
        compare_call(*s, Stop::returned);
    }
    std::cout << "32 Hall of Fame record initialization comparisons with the shared date constructor\n";
}

}

int main() try {
    check_rank();
    check_record_initialization();
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
