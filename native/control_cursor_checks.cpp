#include "aot_runtime.hpp"
#include "control_cursor.hpp"
#include "match_turbo.hpp"
#include <iostream>
#include <string_view>

using namespace cm::aot;
namespace {
constexpr std::uint32_t start = 0x1071a828, continuation = 0x1071a864;
constexpr std::string_view exhausted = "Native instruction budget exhausted";
enum class Slot { none, index, consumed, available, entry };
enum class Strategy { original, helper, generated };

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Fixture {
    std::int16_t index = 0, consumed = 0, available = 1, entry = 42;
    std::uint32_t base = 0x30020000, index_offset = 0x20000;
    std::uint8_t summary_overflow = 0;
    Slot fault = Slot::none;
    bool partial = false, split = false;

    std::uint32_t record_address() const {
        return base + std::uint32_t(std::int32_t(index)) * 3056u + 4u;
    }
    std::uint32_t consumed_address() const { return record_address() + 2936u; }
    std::uint32_t entry_address() const {
        return record_address() + 530u + std::uint32_t(std::int32_t(consumed)) * 2u;
    }
    unsigned instruction_count() const { return consumed < available ? 14 : 9; }

    std::unique_ptr<State> state(std::uint64_t executed, std::uint64_t budget) const {
        auto result = std::make_unique<State>();
        auto& s = *result;
        // Nonzero unrelated state detects an accidental wide register write.
        for (std::size_t i = 0; i < s.registers.size(); ++i)
            s.registers[i] = std::uint8_t(i * 37u + 19u);
        s.unique.fill(0xa5);
        s.wr<4>(112, base);
        s.wr<4>(76, index_offset);
        s.wr<1>(1024, summary_overflow);
        s.pc = start;
        s.executed = executed;
        s.budget = budget;
        s.native_calls = 17;
        s.memory_error = -42;
        const auto write_slot = [&](Slot slot, std::uint32_t address, std::int16_t value) {
            if (slot == fault) {
                if (partial) {
                    s.memory.map(address, 1);
                    s.memory.write<1>(address, std::uint16_t(value) >> 8);
                }
                return;
            }
            if (split) {
                s.memory.map(address, 1);
                s.memory.map(address + 1, 1);
            } else {
                s.memory.map(address, 2);
            }
            s.memory.write<2>(address, std::uint16_t(value));
        };
        write_slot(Slot::index, base + index_offset, index);
        write_slot(Slot::consumed, consumed_address(), consumed);
        write_slot(Slot::available, record_address() + 2932u, available);
        write_slot(Slot::entry, entry_address(), entry);
        return result;
    }
};

template <bool Finite> void helper(State& s) {
    MatchPageCounter<true, Finite> counter(s);
    next_control_cursor_entry(s, counter);
    // The generated page goes to this instruction immediately after the
    // helper. All nonfaulting cases stop before its architectural effects.
    counter.step(continuation);
}

std::string execute(State& s, Strategy strategy) {
    try {
        if (strategy == Strategy::helper) {
            if (s.budget == UINT64_MAX) helper<false>(s);
            else helper<true>(s);
        } else {
            set_match_turbo_enabled(strategy == Strategy::generated);
            require(dispatch_page(s), "Original control-cursor page was not compiled");
        }
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    throw std::runtime_error("Control-cursor check failed to stop at its budget or memory fault");
}

void same_architectural_state(const State& expected, const State& actual) {
    require(expected.pc == actual.pc, "Control-cursor failure PC changed");
    require(expected.executed == actual.executed, "Control-cursor instruction count changed");
    require(expected.registers == actual.registers, "Control-cursor architectural register state changed");
    require(expected.budget == actual.budget && expected.native_calls == actual.native_calls &&
            expected.memory_error == actual.memory_error && expected.allocated == actual.allocated &&
            expected.available == actual.available && expected.heap_end == actual.heap_end,
            "Control-cursor changed unrelated runtime state");
    const auto ranges = expected.memory.mapped_ranges();
    require(ranges == actual.memory.mapped_ranges(), "Control-cursor memory mappings changed");
    for (const auto& [address, size] : ranges)
        require(expected.memory.copy_out(address, size) == actual.memory.copy_out(address, size),
                "Control-cursor mapped bytes changed");
    // P-code 'unique' values are instruction-local scratch, not PowerPC state.
    // They are deliberately excluded; registers and every mapped byte are not.
}

unsigned cases = 0;
bool check_generated = true;
void compare(const Fixture& fixture, unsigned allowed, bool overflow = false) {
    const auto initial = overflow ? UINT64_MAX - allowed : UINT64_C(731);
    const auto budget = overflow ? UINT64_MAX : initial + allowed;
    const auto expected = fixture.state(initial, budget);
    const auto reason = execute(*expected, Strategy::original);
    require(reason == exhausted || reason.starts_with("Unmapped data access at "),
            "Original control cursor stopped for an unexpected reason");
    for (const auto strategy : {Strategy::helper, Strategy::generated}) {
        if (strategy == Strategy::generated && !check_generated) continue;
        const auto actual = fixture.state(initial, budget);
        require(execute(*actual, strategy) == reason, "Control-cursor exception diagnostic changed");
        same_architectural_state(*expected, *actual);
    }
    if (fixture.fault == Slot::none && allowed == fixture.instruction_count()) {
        require(expected->pc == continuation && reason == exhausted,
                "Original cursor did not reach its audited continuation");
        const bool selected = fixture.consumed < fixture.available;
        const auto selected_entry = fixture.entry_address() == fixture.consumed_address()
            ? std::int16_t(fixture.consumed + 1) : fixture.entry;
        const auto entry = selected ? std::uint32_t(std::int32_t(selected_entry)) : UINT32_MAX;
        require(expected->r<4>(92) == entry, "Signed cursor result did not match the original contract");
        require(expected->memory.read<2>(fixture.consumed_address()) ==
                    std::uint16_t(fixture.consumed + (selected ? 1 : 0)),
                "Original cursor consumed count did not match the selected branch");
    }
    ++cases;
}

void every_interruption(const Fixture& fixture) {
    for (unsigned allowed = 0; allowed <= fixture.instruction_count(); ++allowed) {
        compare(fixture, allowed);
        compare(fixture, allowed, true);
    }
}
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--helper-only") check_generated = false;
    else if (argc != 1) {
        std::cerr << "Usage: cm-control-cursor-checks [--helper-only]\n";
        return 2;
    }
    try {
        // Signed comparisons, sign extension and 32-bit multiplication all
        // differ from an unsigned/index-bounds interpretation of these fields.
        constexpr std::int16_t counts[] = {-32768, -32767, -2, -1, 0, 1, 2, 32766, 32767};
        constexpr std::int16_t entries[] = {-32768, -1, 0, 1, 32767};
        for (const auto index : {-32768, -1, 0, 1, 32767}) {
            for (const auto consumed : counts) {
                for (const auto available : counts) {
                    Fixture fixture;
                    fixture.index = std::int16_t(index);
                    fixture.consumed = consumed;
                    fixture.available = available;
                    fixture.entry = entries[cases % 5];
                    fixture.summary_overflow = cases % 2 ? 0xff : 0xfe;
                    compare(fixture, fixture.instruction_count());
                }
            }
        }
        for (const auto& [consumed, available] :
             {std::pair{-32768, -32767}, std::pair{-1, 0}, std::pair{0, 1},
              std::pair{32766, 32767}, std::pair{0, 0}, std::pair{32767, -32768}}) {
            Fixture fixture;
            fixture.consumed = std::int16_t(consumed);
            fixture.available = std::int16_t(available);
            fixture.entry = -32768;
            fixture.summary_overflow = 0xff;
            every_interruption(fixture);
        }
        // The original layout permits the selected entry to alias one of
        // these fields. In particular, the entry read must follow the store.
        for (const auto consumed : {1201, 1203}) {
            Fixture fixture;
            fixture.consumed = std::int16_t(consumed);
            fixture.available = std::int16_t(consumed + 1);
            fixture.entry = consumed == 1201 ? fixture.available : fixture.consumed;
            every_interruption(fixture);
        }
        // Exact two-byte maps, merged adjacent one-byte maps, unaligned
        // addresses, the top of memory, and address arithmetic wrapping to zero.
        for (const auto base : {0u, 1u, 0xfffffff0u, 0xfffff488u, 0xfffffdebu, 0x3000fffbu}) {
            Fixture fixture;
            fixture.base = base;
            fixture.split = true;
            every_interruption(fixture);
        }
        // Each read can fail before or after architectural changes. In
        // particular, entry-load failure occurs after consumed was written.
        for (const auto fault : {Slot::index, Slot::consumed, Slot::available, Slot::entry}) {
            for (bool partial : {false, true}) {
                Fixture fixture;
                fixture.fault = fault;
                fixture.partial = partial;
                every_interruption(fixture);
            }
        }
        Fixture crosses_address_limit;
        crosses_address_limit.base = UINT32_MAX - crosses_address_limit.index_offset;
        crosses_address_limit.fault = Slot::index;
        crosses_address_limit.partial = true;
        every_interruption(crosses_address_limit);
        set_match_turbo_enabled(false);
        std::cout << cases << " control-cursor differential cases passed against original Mac instructions"
                  << (check_generated ? " (helper and generated Turbo)\n" : " (helper only)\n");
        return 0;
    } catch (const std::exception& error) {
        set_match_turbo_enabled(false);
        std::cerr << "Control-cursor case " << cases << ": " << error.what() << '\n';
        return 1;
    }
}
