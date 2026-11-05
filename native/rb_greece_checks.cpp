#include "rb_greece_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <random>

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto entry = 0x106ab890u, done = fixture + 0x80;
constexpr auto tally = fixture + 0x100, first = tally + 0x40, second = tally + 0x41;

void check_threshold() {
    std::mt19937 random(0x47524545);
    constexpr std::array outputs{
        std::pair{0u, 0u}, std::pair{first, 0u}, std::pair{0u, second},
        std::pair{first, second}, std::pair{first, first},
        std::pair{tally + 4, second}, std::pair{first, tally + 4}, std::pair{tally + 4, tally + 4},
    };
    for (unsigned count = 0; count < 256; ++count) {
        for (const auto [ban, dismissal] : outputs) {
            auto seed = std::make_unique<State>();
            seed->memory.map(fixture, 0x1000);
            Cpu c(*seed);
            for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
            for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
            seed->wr<1>(guest::reg::xer_so, random() & 1);
            c.set({{4, tally}, {5, ban}, {6, dismissal}});
            c.set_lr(done);
            c.put8(tally + 4, count);
            c.put8(first, 0x55);
            c.put8(second, 0xaa);
            seed->pc = entry;
            seed->budget = 32;
            auto old = clone(*seed), modern = clone(*seed);
            Effects old_effects, new_effects;
            old_effects.state = old.get();
            new_effects.state = modern.get();
            old->memory.date_observer = &old_effects;
            modern->memory.date_observer = &new_effects;
            rb_greece_run_original(*old, entry);
            run_until(*modern, done);
            const auto label = "Greek rulebook threshold " + std::to_string(count) + " " +
                hex_address(ban) + " " + hex_address(dismissal);
            require(old->pc == done && modern->pc == done, label + " did not return");
            require(old_effects.bytes == new_effects.bytes, label + " ordered effects mismatch");
            old->memory.date_observer = nullptr;
            modern->memory.date_observer = nullptr;
            const Image expected(*old);
            expected.compare(*modern, label);
            if (count == 2 && ban == first && dismissal == second) {
                Cpu candidate(*modern);
                candidate.set(3, 0);
                bool rejected = false;
                try {
                    expected.compare(*modern, "wrong threshold result");
                } catch (const std::runtime_error& error) {
                    rejected = std::string_view(error.what()).starts_with("wrong threshold result register");
                }
                require(rejected, "threshold corruption control was not rejected");
            }
        }
    }
}

constexpr auto construct = 0x106ab8f0u, format_text = 0x106ab7d0u;
constexpr auto object = fixture + 0x200, source = fixture + 0x300, message = fixture + 0x400;
constexpr auto success = fixture + 0x500, nation_slot = fixture + 0x510;
constexpr auto nation_array_slot = fixture + 0x514, league_slot = fixture + 0x518, group_slot = fixture + 0x51c;
constexpr auto nation_array = fixture + 0x600, vtable = fixture + 0x900;
using Call = std::array<std::uint32_t, 34>;
std::vector<Call>* calls = nullptr;

bool dependency(State& s) {
    Cpu c(s);
    Call call{s.pc, c.lr()};
    for (unsigned reg = 0; reg < 32; ++reg) call[reg + 2] = c.r(reg);
    switch (s.pc) {
    case 0x105bb3e0:
        require(c.r(3) == object && c.r(31) == object && c.sp() == stack - 0x40, "base rulebook ABI");
        c.put32(object + 8, fixture + 0xa00);
        c.set(3, object);
        break;
    case 0x10168a00:
        require(c.r(3) == nation_array + 0x128 && c.r(4) == destination && c.r(6) == 13,
                "nation formatting ABI");
        c.put32(c.r(4), 0x47726565);
        c.set(3, c.r(4));
        break;
    case 0x10739890:
        require(c.r(3) == source && !c.r(4) && !c.r(5) && c.r(6) == c.sp() + 0x40 &&
                    c.r(7) == c.sp() + 0x38, "diagnostic filename ABI");
        c.put8(c.r(6), 'G');
        c.put8(c.r(7), 'C');
        break;
    case 0x1077b720:
        require(c.r(3) == message && c.r(4) == source + 0xe && c.r(5) == source + 0x30 &&
                    c.u8(c.r(6)) == 'G' && c.u8(c.r(7)) == 'C' && c.r(8) == 0xbc,
                "diagnostic format ABI: " + hex_address(c.r(3)) + " " + hex_address(c.r(4)) + " " +
                    hex_address(c.r(5)) + " " + hex_address(c.r(6)) + " " + hex_address(c.r(7)) + " " +
                    hex_address(c.r(8)));
        c.put32(message, c.r(8));
        c.set(3, 1);
        break;
    case 0x10769b60:
        require(c.r(3) == source + 0x1b && c.r(4) == message && !c.r(5), "diagnostic display ABI");
        c.set(3, 0);
        break;
    default: return false;
    }
    calls->push_back(call);
    c.finish();
    return true;
}

struct Model {
    decltype(dispatch_probe) previous = dispatch_probe;
    Model() { dispatch_probe = dependency; }
    ~Model() { dispatch_probe = previous; calls = nullptr; }
};

void check_lifetime_and_text() {
    std::mt19937 random(0x52424752);
    Model model;
    bool corrupt_trace_rejected = false;
    const std::array widths{INT32_MIN, -1, 0, 19, 20, 21, 255, 32768, INT32_MAX};
    for (unsigned test = 0; test < 64; ++test) {
        auto seed = std::make_unique<State>();
        seed->memory.map(fixture, 0x1000);
        seed->memory.map(stack - 0x1000, 0x2000);
        seed->memory.map(toc - 0x8000, 0x8000);
        Cpu c(*seed);
        for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
        for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
        seed->wr<1>(guest::reg::xer_so, random() & 1);
        for (unsigned offset = 0; offset < 0x100; offset += 4) c.put32(stack - offset, random());
        for (unsigned offset = 0; offset < 0x24; offset += 4) c.put32(object + offset, random());
        c.set({{1, stack}, {2, toc}, {3, object}, {4, destination}, {6, 13}});
        c.put32(toc - 0x5ddc, vtable);
        c.put32(toc - 0x7574, league_slot);
        c.put32(toc - 0x7a70, group_slot);
        c.put32(toc - 0x41d4, source);
        // The original inlines the version getter. Both routes read its
        // real TOC slot instead of trying to intercept it.
        c.put32(toc - 0x5b20, source + 0x30);
        c.put32(toc - 0x7ba8, message);
        c.put32(toc - 0x7bac, success);
        c.put32(toc - 0x7598, nation_slot);
        c.put32(toc - 0x7b98, nation_array_slot);
        c.put32(league_slot, random());
        c.put32(group_slot, random());
        c.put32(nation_slot, 1);
        c.put32(nation_array_slot, nation_array);
        c.put8(success, 1);
        const auto target = test < 32 ? construct : format_text;
        c.set(5, widths[test % widths.size()]);
        c.set_lr(done);
        seed->pc = target;
        seed->budget = 100;
        auto old = clone(*seed), modern = clone(*seed);
        std::vector<Call> old_calls, new_calls;
        Effects old_effects, new_effects;
        old_effects.state = old.get();
        new_effects.state = modern.get();
        old->memory.date_observer = &old_effects;
        modern->memory.date_observer = &new_effects;
        calls = &old_calls;
        try {
            rb_greece_run_original(*old, target);
        } catch (const std::exception& error) {
            throw std::runtime_error("original: " + std::string(error.what()));
        }
        calls = &new_calls;
        run_until(*modern, done);
        const auto label = "Greek rulebook " + hex_address(target) + " case " + std::to_string(test);
        require(old->pc == done && modern->pc == done, label + " did not return");
        require(!old_calls.empty() && old_calls == new_calls, label + " ordered call arguments differ");
        require(old_effects.bytes == new_effects.bytes, label + " ordered memory effects differ");
        old->memory.date_observer = nullptr;
        modern->memory.date_observer = nullptr;
        Image(*old).compare(*modern, label);
        if (test == 0) {
            ++new_calls.front()[5];
            corrupt_trace_rejected = old_calls != new_calls;
        }
    }
    require(corrupt_trace_rejected, "wrong dependency argument was not detected");
}

}

int main() try {
    check_threshold();
    check_lifetime_and_text();
    std::cout << "Greek rulebook threshold: 2112 completed original/production comparisons and two corruption controls\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
