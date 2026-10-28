// Full returned comparisons for the implemented MATCH_PL block. Dependencies
// not yet reconstructed are scripted identically at their original boundaries.
#include "match_pl_adapter.hpp"
#include "match_pl.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <cstdlib>
#include <iostream>
#include <map>

#if defined(CM_MATCH_PL_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
namespace cm::aot { bool match_pl_reference(State&); }
using namespace cm::aot;
using guest::Cpu;
namespace {
constexpr std::uint32_t data = 0x60000000, player = data + 0x1000,
    match = data + 0x10000, other_match = data + 0x20000,
    trace_at = data + 0x38000, stack = 0x71010000, sentinel = 0xfffffff0;
struct Case {
    std::uint32_t entry;
    int side = 0, column = 4, row = 5;
    std::uint32_t argument = 0, first_role = 1, second_role = 4;
    bool null_player = false, null_match = false, mutate = false;
};
Case active;
bool reference_mode = false, generated_mode = false;
constexpr std::uint32_t production_entries[]{
#define MATCH_PL_ROUTINE(entry, ...) entry,
#define MATCH_PL_DRAFT(...)
#include "match_pl_routines.inc"
#undef MATCH_PL_DRAFT
#undef MATCH_PL_ROUTINE
};

bool scripted(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    const std::array services{0x107735a0u, 0x10244c60u, 0x102374f0u,
                             0x1022e7e0u, 0x10283690u, 0x101e3d50u, 0x101e3de0u};
    if (std::ranges::find(services, target) == services.end()) return false;
    auto used = c.u32(trace_at);
    const auto append = [&](std::uint32_t word) { c.put32(trace_at + 4 + used, word); used += 4; };
    append(target); append(c.lr());
    for (unsigned reg = 3; reg <= 10; ++reg) append(c.r(reg));
    for (unsigned reg = 13; reg < 32; ++reg) append(c.r(reg));
    for (unsigned field = 0; field < 8; ++field) append(c.cr(field));
    for (unsigned reg = 1; reg <= 2; ++reg) {
        const auto bits = std::bit_cast<std::uint64_t>(c.f(reg));
        append(bits >> 32); append(bits);
    }
    c.put32(trace_at, used);
    if (target == 0x10283690) c.set(3, c.u8(c.r(3) + 0x27));
    else if (target == 0x101e3d50) {
        if (active.mutate) c.put32(player + 0x1a4, other_match);
        c.set(3, active.first_role);
    } else if (target == 0x101e3de0) c.set(3, active.second_role);
    else if (target == 0x107735a0) c.put32(data + 0x300, c.r(3));
    else {
        c.put32(data + 0x304, target);
        c.put32(data + 0x308, c.r(4)); c.put32(data + 0x30c, c.r(5));
        c.set(3, active.first_role);
    }
    // These services leave arguments intact, making caller preparation visible.
    c.finish();
    return true;
}
bool probe(State& s) {
    // Cpu::call runs lowered leaves before the probe. Keep the reference's
    // dispatch path symmetric, particularly the two-instruction side getter.
    if (run_leaf(s)) return true;
    if (scripted(s)) return true;
    if (reference_mode && match_pl_lists(s.pc)) return match_pl_reference(s);
    if (reference_mode && s.pc >= 0x102238a0 && s.pc < 0x1028ad60)
        return match_pl_reference(s);
    return false;
}

void initialize_case(Cpu& c) {
    c.s.memory.map(data, 0x40000); c.s.memory.map(stack - 0x10000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x40000, 0xa5));
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0x5a));
    for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, 0x51000000 + reg * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    c.set_f(1, 1.25); c.set_f(2, -2.5);
    c.put32(trace_at, 0);
    c.put32(player + 0x1a4, active.null_match ? 0 : match);
    c.put8(player + 0x27, active.side);
    c.put8(player + 0x105, active.column); c.put8(player + 0x106, active.row);
    c.put8(match + 0xe03d, active.row); c.put8(match + 0xe073, active.column);
    c.put8(match + 0x791e, active.row);
    c.put8(match + 0x7925, active.row); c.put8(match + 0x7926, active.row);
    if (active.entry == 0x1022e3a0) {
        const auto flags = active.argument;
        const auto previous = data + 0x3000, receiver = data + 0x5000;
        c.put32(previous + 0x1a4, match);
        c.put32(match + 0xe000, flags & 1 ? previous : 0);
        c.put32(match + 0xdff4, flags & 2 ? receiver : 0);
        c.put8(receiver + 0x27, flags & 4 ? 1 - active.side : active.side);
        c.put8(match + 0x1be + std::uint32_t(std::int32_t(std::int8_t(active.side))), flags & 8 ? 1 : 0);
        c.put8(match + 0x77bc, flags & 16 ? 40 : 0);
        c.put8(match + 0x77bd, flags & 16 ? 40 : 0);
        c.put8(match + 0x7921, flags & 32 ? active.side : 1 - active.side);
        if (flags & 64) c.put32(match + 0xe000, player);
        c.put8(receiver + 2, flags & 128 ? 255 : 1);
        c.put16(receiver + 0x36, flags & 128 ? 32760 : 6000);
        if (flags & 256) c.put8(match + 0x77bc + std::uint32_t(active.side == 0), active.second_role);
        c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
    }
    if (active.entry == 0x102238a0 && active.argument == 1) {
        c.put32(c.r(2) - 0x51e0, player + 0x70); c.put32(player + 0x70, 0x3f800000);
    }
    c.set(3, active.null_player ? 0 : active.entry == 0x102238a0 && active.argument == 2 ? stack - 0x180 : player);
    c.set(4, active.argument);
    c.set(5, std::uint32_t(active.column)); c.set(6, std::uint32_t(active.row));
    if (active.entry == 0x10239dc0) c.set(4, std::uint32_t(active.side));
}

void require_return(const State& s) {
    cm::semantic_io_checks::require(s.pc == sentinel && s.r<4>(4) == stack,
                                   "MATCH_PL comparison did not return normally");
}
void original(State& s, std::uint32_t entry) {
    reference_mode = true;
    s.pc = entry; run_until(s, sentinel); require_return(s);
}
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false;
    if (generated_mode) { s.pc = entry; run_until(s, sentinel); }
    else match_pl_run_rewrite(s, entry);
    require_return(s);
#if defined(CM_MATCH_PL_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void corrupt(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.set(3, c.r(3) ^ 1);
}
void corrupt_memory(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.put8(player + 0x19, c.u8(player + 0x19) ^ 1);
}

void require_production_routes(const State& base) {
    reference_mode = false;
    for (const auto entry : production_entries) {
        active = {entry};
        auto state = cm::semantic_io_checks::clone(base); Cpu c(*state); initialize_case(c);
        // Hooks precede original instruction accounting. An original fallback
        // cannot return with this budget; role_distance permits only its real
        // four-instruction external abs dependency. This is routing evidence,
        // not a requirement to preserve the original's diagnostic counts.
        state->budget = entry == 0x1028ac50 ? 4 : 0;
        state->pc = entry;
        cm::semantic_io_checks::Effects observer;
        observer.state = state.get(); state->memory.date_observer = &observer;
        run_until(*state, sentinel);
        state->memory.date_observer = nullptr;
        require_return(*state);
        std::cout << "ok production C++ route " << hex_address(entry) << '\n';
    }
}

std::vector<Case> cases() {
    std::vector<Case> result;
    const std::array queries{0x10237b10u, 0x10239d10u, 0x10239dc0u, 0x10251240u,
        0x10272050u, 0x102720a0u, 0x102720d0u, 0x10272100u, 0x10272150u,
        0x102721a0u, 0x102721f0u, 0x10283620u};
    // Every signed-byte outcome is exercised; unrelated coordinates stay valid.
    for (auto entry : queries) for (int side : {0, 1, 2, 255}) {
        for (int value : {-128, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 127}) {
            Case test{entry, side, 4, value, std::uint32_t(side)};
            if (entry == 0x102720a0 || entry == 0x102720d0) {
                test.column = value; test.argument = std::uint32_t(value);
            } else if (entry == 0x102721f0) test.argument = std::uint32_t(value);
            result.push_back(test);
        }
    }
    for (auto entry : {0x10239d10u, 0x10239dc0u})
        for (int side : {0, 1}) for (int column : {-128, -1, 0, 1, 2, 3, 6, 7, 127})
            for (int row : {-1, 0, 1, 2, 9, 10, 11, 12})
                result.push_back({entry, side, column, row, std::uint32_t(side)});
    for (auto entry : {0x102238a0u, 0x1022e710u}) result.push_back({entry});
    result.push_back({0x102238a0, 0, 4, 5, 1});
    result.push_back({0x102238a0, 0, 4, 5, 2});
    for (int release : {-32768, -1, 0, 1, 32767, 65536}) for (bool null : {false, true}) {
        Case test{0x102270f0}; test.argument = std::uint32_t(release); test.null_player = null;
        result.push_back(test);
    }
    for (int side : {0, 1, 2, 255}) for (int column : {2, 3, 5, 6}) for (int row : {0, 1, 11, 255})
        result.push_back({0x10237420, side, column, row});
    for (int side : {0, 1, 2, 255}) for (int row : {-128, -1, 0, 1, 10, 11, 12, 127})
        for (auto immediate : {0u, 1u, 255u, 256u}) result.push_back({0x10239e50, side, 4, row, immediate});
    for (std::uint32_t flags = 0; flags < 128; ++flags)
        result.push_back({0x1028acf0, 0, 4, 5, flags});
    for (auto entry : {0x1028abc0u, 0x1028ac50u})
        for (auto first : {0u, 1u, 4u, 8u, 16u, 32u, 64u, 0x80u, 0x800u, 0x880u, UINT32_MAX})
            for (auto second : {0u, 1u, 4u, 8u, 16u, 32u, 64u, 0x800u, 0x880u}) {
                Case test{entry}; test.first_role = first; test.second_role = second;
                result.push_back(test);
                test.mutate = true; result.push_back(test);
            }
    for (auto entry : {0x1028abc0u, 0x1028ac50u}) { Case test{entry}; test.null_match = true; result.push_back(test); }
    for (int side : {0, 1, 255}) for (int row : {0, 1, 5, 6, 10, 11})
        for (auto flags : {0u, 1u, 2u, 3u, 6u, 10u, 18u, 34u})
            for (auto role : {1u, 4u}) {
                Case test{0x1022e3a0, side, 4, row, flags}; test.first_role = role; result.push_back(test);
            }
    for (int side : {0, 1}) for (auto flags : {2u, 3u, 66u, 130u}) {
        Case test{0x1022e3a0, side, 4, 5, flags};
        test.mutate = true; result.push_back(test);
    }
    for (auto count : {0u, 1u, 9u, 10u, 127u, 128u, 255u}) {
        Case test{0x1022e3a0, 0, 4, 5, 256}; test.second_role = count; result.push_back(test);
    }
    return result;
}
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) return 2;
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        generated_mode = argc == 3 && std::string_view(argv[2]) == "--production";
        auto tests = cases();
        if (generated_mode) std::erase_if(tests, [](const Case& test) {
            return std::ranges::find(production_entries, test.entry) == std::end(production_entries);
        });
        unsigned failures = 0, completed = 0;
        std::map<std::uint32_t, unsigned> compared;
        for (const auto& test : tests) {
            active = test;
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
            const auto found = compare_routine_call(*state, active.entry, original, rewrite,
                RoutineChecker::result_r3 | RoutineChecker::result_f1);
            if (found.empty()) { ++completed; ++compared[active.entry]; }
            else {
                std::cout << "DIFFERS " << hex_address(active.entry) << " side=" << active.side
                          << " column=" << active.column << " row=" << active.row << " argument=" << active.argument << '\n';
                for (const auto& line : found) std::cout << "  " << line << '\n';
                if (++failures == 5) break;
            }
        }
        if (!failures) for (bool memory : {false, true}) {
            active = {memory ? 0x102238a0u : 0x1028acf0u};
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
            const auto found = compare_routine_call(*state, active.entry, original,
                memory ? corrupt_memory : corrupt, RoutineChecker::result_r3 | RoutineChecker::result_f1);
            const auto prefix = memory ? "byte " : "r3 ";
            const bool detected = std::ranges::any_of(found, [&](const auto& line) { return line.starts_with(prefix); });
            if (!detected) { ++failures; std::cout << "FAIL negative control\n"; }
            else std::cout << "ok " << (memory ? "memory" : "result") << " corruption detected\n";
        }
        if (!failures && generated_mode) require_production_routes(*base);
        for (const auto& [entry, count] : compared)
            std::cout << hex_address(entry) << " returned " << count << '\n';
        std::cout << completed << '/' << tests.size() << " returned comparisons, " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
