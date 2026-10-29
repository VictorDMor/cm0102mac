// Deterministic external service boundaries; both sides execute complete owned bodies.
#include "transfer_screens_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <vector>

#if defined(CM_TRANSFER_SCREENS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
namespace cm::aot { bool transfer_screens_reference(State&); }
using namespace cm::aot;
using guest::Cpu;
namespace {
constexpr std::uint32_t data = 0x60000000, screens = data + 0x10000,
    player = data + 0x1000, link = data + 0x2000, offer = data + 0x3000,
    contract = data + 0x4000, clubs = data + 0x5000, people = data + 0x8000,
    history = data + 0x20000, trace_at = data + 0x30000,
    stack = 0x71010000, sentinel = 0xfffffff0;
constexpr std::array entries{0x10417590u, 0x10417880u, 0x1041e760u, 0x1041ea60u, 0x10422570u,
    0x10421a20u, 0x10421af0u, 0x10421be0u, 0x10421ce0u, 0x10421dd0u,
    0x1041f830u, 0x10420aa0u, 0x10422ac0u, 0x10423160u, 0x104248a0u,
    0x10424930u, 0x10426550u, 0x1042a780u, 0x1042ae20u, 0x1042b500u,
    0x1042ed80u, 0x1042ee90u, 0x10428650u, 0x10420e40u, 0x10421960u, 0x10422a00u,
    0x10426830u, 0x1042b3a0u, 0x10426970u, 0x10423040u};
struct Case {
    std::uint32_t entry;
    int create = 1, create_result = 1, contract_present = 1, contract_type = 0,
        link_present = 1, offer_present = 1, status = 5, response = 0,
        eligible = 1, controlled = 1, group = 0, news_revision = 17,
        action = 0, default_action = 0, control = 0, command = 0, fallback = 1,
        candidates = 1, actual_query = 0, clear_link = 0, mixed_terms = 0;
    std::uint32_t mode = 1;
};
Case active{entries[0]};
bool original_mode = false, production_mode = false;
unsigned call_count = 0;
const std::set<std::uint32_t> dependencies{
    0x1073e4b0, 0x1073f1e0, 0x10082f20, 0x103f8820, 0x101929b0,
    0x102bdf90, 0x102ccdd0, 0x102cb570, 0x103eb440, 0x10094ae0,
    0x103ea350, 0x1073ef70, 0x10775170, 0x102a54b0, 0x102a83a0,
    0x103e44b0, 0x103e6290, 0x10718360};

void trace(Cpu& c, std::uint32_t entry) {
    if (++call_count > 1000) throw std::runtime_error("fixture call limit");
    auto n = c.u32(trace_at);
    const auto put = [&](std::uint32_t value) { c.put32(trace_at + 4 + 4 * n++, value); };
    put(entry); put(c.lr());
    for (unsigned r = 3; r <= 10; ++r) put(c.r(r));
    for (unsigned r = 13; r <= 31; ++r) put(c.r(r));
    for (unsigned f = 0; f < 8; ++f) put(c.cr(f));
    c.put32(trace_at, n);
}
bool probe(State& s) {
    Cpu c(s); const auto target = s.pc;
    if (target >= 0x10417590 && target < 0x1042fe10) {
        if (original_mode) return transfer_screens_reference(s);
        if (production_mode) return false;
        if (transfer_screens_lists(target)) { transfer_screens_run_rewrite(s, target); return true; }
        throw std::runtime_error("Unimplemented owned dependency in fixture " + hex_address(target));
    }
    if (!dependencies.contains(target)) return false;
    trace(c, target);
    if (target == 0x103eb440 && active.actual_query) return false;
    switch (target) {
    case 0x1073e4b0: c.set(3, active.create_result); break;
    case 0x1073f1e0:
        if (c.r(4) >= 32) throw std::runtime_error("Invalid fixture history index");
        c.put32(history + c.r(4) * 4, c.r(5)); break;
    case 0x10082f20: c.set(3, active.contract_present ? contract : 0); break;
    case 0x103f8820:
        // Dependency-boundary mutation checks the caller's live reread. This
        // characterizes the contract; it is not a claim that country rules do it.
        if (active.clear_link) c.put32(link, 0);
        c.set(3, active.eligible); break;
    case 0x101929b0: c.set(3, active.controlled); break;
    case 0x102bdf90:
        for (unsigned i = 0; i < 0xe4; ++i) c.put8(c.r(3) + i, 0);
        break;
    case 0x102ccdd0:
        for (unsigned i = 0; i < 13; ++i) c.put32(c.r(6) + 8 + i * 4, i == 0 ? 3 : 17);
        c.put32(c.r(6) + 8 + (active.entry == 0x10421be0 ? 8 : active.entry == 0x10421dd0 ? 12 : 7) * 4,
                active.news_revision);
        c.set(3, 1); break;
    case 0x102cb570: c.set(3, c.u32(c.r(3) + 8 + c.r(4) * 4)); break;
    case 0x103eb440: c.set(3, link); break;
    case 0x10094ae0: c.set(3, 1); break;
    case 0x103ea350: c.set(3, active.mode); break;
    case 0x1073ef70: c.set(3, c.u32(history + c.r(4) * 4)); break;
    case 0x10775170: c.put32(data + 0x120, c.r(3)); break;
    case 0x102a54b0: c.set(3, active.command); break;
    case 0x102a83a0: c.set(3, active.fallback); break;
    case 0x10718360: c.set(3, active.mode); break;
    case 0x103e44b0: case 0x103e6290: c.set(3, 1); break;
    }
    s.pc = c.lr(); return true;
}
void initialize(Cpu& c, const Case& test) {
    c.s.memory.map(data, 0x400000); c.s.memory.map(stack - 0x10000, 0x20000);
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, f + 1);
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), data + offset); c.put32(data + offset, value);
    };
    c.put32(c.r(2) - 0x7bb0, screens);
    variable(-0x7b78, 0x100, 0); variable(-0x7b7c, 0x104, data + 0x9000);
    variable(-0x7be8, 0x108, clubs); variable(-0x7be0, 0x10c, people);
    variable(-0x7b40, 0x110, data + 0xa000);
    variable(-0x7bf0, 0x114, data + 0xd000);
    c.put32(c.r(2) - 0x7b80, data + 0x130); c.put16(data + 0x132, 2002);
    c.put32(player, 3); c.put32(link, test.offer_present ? offer : 0);
    c.put32(data + 0xa000, data + 0x2200); c.put32(data + 0xa004, 18);
    for (unsigned i = 0; i < 18; ++i) c.put32(data + 0x2200 + i * 4, offer);
    c.put32(people + 3 * 0x70, 3);
    c.put32(offer + 8, 2); c.put32(offer + 12, 3); c.put32(offer + 0x2c, 17);
    c.put8(offer + 0x34, test.status); c.put8(offer + 0x35, test.response);
    c.put32(contract + 4, 1); c.put8(contract + 0x38, test.contract_type);
    c.put16(screens + 0x2c58, test.group);
    c.put32(screens + std::uint32_t(test.group) * 0x2c0 + 4, data + 0xb000);
    c.put32(screens + std::uint32_t(test.group) * 0x2c0 + 0x24, data + 0x100000);
    c.put32(screens + std::uint32_t(test.group) * 0x2c0 + 0x10, data + 0xe000);
    c.put32(data + 0xb024, data + 0xc000);
    c.put16(screens + 0x25819e, test.default_action);
    if (std::int16_t(test.control) >= 0)
        c.put32(data + 0x100000 + 0xba86c + std::uint32_t(std::int16_t(test.control)) * 0x17c, test.action);
    c.put32(history + 16, test.candidates ? data + 0x40000 : 0);
    c.put32(history + 20, 23);
    c.put32(history, offer); c.put32(history + 4, 2);
    c.set({{3, player}, {4, test.link_present ? link : 0}, {5, 0x12345678}});
    if (test.entry == 0x10417880) c.set({{3, link}, {4, std::uint32_t(test.create)}});
    if (test.entry == 0x10422570) c.set({{3, offer}, {4, 17}});
    if (test.entry == 0x10423160) c.set(3, link);
    if (test.entry == 0x104248a0 || test.entry == 0x1042a780 || test.entry == 0x1042ed80) c.set(4, test.mode);
    if (test.entry == 0x10424930 || test.entry == 0x10420aa0) c.set(5, test.mode);
    if (test.entry == 0x1042ee90) for (unsigned r = 4; r <= 9; ++r) c.set(r, test.mode);
    if (test.mixed_terms) c.set({{4, 2}, {5, 2002}, {6, 50}, {7, 1}, {8, 0}, {9, offer}});
    if (test.entry == 0x10420e40 || test.entry == 0x10421960 || test.entry == 0x10422a00 ||
        test.entry == 0x10426830 || test.entry == 0x1042b3a0)
        c.set(3, test.control);
    c.s.budget = 100000;
}
void returned(State& s) {
    Cpu c(s);
    if (c.sp() != stack || c.lr() != sentinel || s.pc != sentinel)
        throw std::runtime_error("fixture did not return");
}
void original(State& s, std::uint32_t entry) {
    original_mode = true; s.pc = entry; run_until(s, sentinel); returned(s);
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    if (production_mode) { s.pc = entry; run_until(s, sentinel); }
    else transfer_screens_run_rewrite(s, entry);
    returned(s);
#if defined(CM_TRANSFER_SCREENS_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void corrupted(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.put32(history, c.u32(history) ^ 1);
}
void corrupted_result(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.set(3, c.r(3) ^ 1);
}
struct ObservedWrites final : MemoryObserver {
    unsigned count = 0;
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override { ++count; }
    void cleared(std::uint32_t, std::uint64_t) override { ++count; }
};
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-transfer-screens-checks <original-segments>\n"; return 2; }
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        std::vector<Case> cases;
        for (auto entry : entries) cases.push_back({entry});
        for (auto entry : {entries[0], entries[1]}) {
            for (int created : {0, 1, -1, 128, 256, 0x12340001}) for (int eligible : {0, 1, -1, 128, 256})
                for (int status : {0, 4, 5, 16, 17, 127, 128, 255}) {
                    Case t{entry}; t.create_result = created; t.eligible = eligible; t.status = status; cases.push_back(t);
                }
            for (int present : {0, 1}) for (int type : {0, 16, 20, 21, 23, 24, 127, 128, 255}) {
                Case t{entry}; t.contract_present = present; t.contract_type = type; cases.push_back(t);
            }
            for (int group : {0, 1, 3}) { Case t{entry}; t.group = group; cases.push_back(t); }
        }
        for (int present : {0, 1}) for (int offer_present : {0, 1}) {
            Case t{entries[0]}; t.link_present = present; t.offer_present = offer_present; cases.push_back(t);
        }
        for (int create : {0, 1, -1, 128, 256}) for (int response : {0, 1, 2, 128, 255})
            for (int controlled : {0, 1, -1, 127, 128, 255, 256}) {
                Case t{entries[1]}; t.create = create; t.response = response; t.controlled = controlled; cases.push_back(t);
            }
        for (auto entry : {entries[3], entries[4]}) for (int created : {0, 1, -1, 256}) {
            Case t{entry}; t.create_result = created; cases.push_back(t);
        }
        for (auto entry : {0x10421a20u, 0x10421af0u, 0x10421be0u, 0x10421ce0u, 0x10421dd0u, 0x10423040u})
            for (int revision : {16, 17, 18}) for (int status : {0, 2, 3, 5, 16, 255})
                for (int created : {0, 1}) {
                    Case t{entry}; t.news_revision = revision; t.status = status;
                    t.create_result = created; cases.push_back(t);
                }
        for (unsigned index = 10; index < 22; ++index) for (int created : {0, 1, 128, 256})
            for (auto mode : {0u, 1u, 2u, 127u, 128u, 255u, 256u, 65535u, 0xdeadbeefu, 0x80000000u}) {
                Case t{entries[index]}; t.create_result = created; t.mode = mode; cases.push_back(t);
            }
        for (int present : {0, 1}) { Case t{0x10428650}; t.candidates = present; cases.push_back(t); }
        for (auto entry : {0x10420e40u, 0x10421960u, 0x10422a00u, 0x10426830u, 0x1042b3a0u})
            for (int control : {-1, 0, 1, 5, 65535}) for (int action : {0, 1, 2, 3, 255, 65537})
                for (int fallback_action : {0, 1, 3}) for (int command : {0, 1, -1}) {
                    Case t{entry}; t.control = control; t.action = action; t.default_action = fallback_action;
                    t.command = command; t.fallback = command ? 7 : -9; cases.push_back(t);
                }
        for (int created : {0, 1, 128, 256}) for (auto month : {0u, 1u, 12u, 128u, 255u, 65535u}) {
            Case t{0x10426970}; t.create_result = created; t.mode = month; cases.push_back(t);
        }
        { Case t{0x10417880}; t.clear_link = 1; cases.push_back(t); }
        { Case t{0x1042ee90}; t.mixed_terms = 1; cases.push_back(t); }
        for (auto entry : {0x10420e40u, 0x10421960u, 0x10422a00u, 0x10426830u, 0x1042b3a0u})
            for (int fallback : {0, 1, -1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
                Case t{entry}; t.fallback = fallback; cases.push_back(t);
            }
        for (auto entry : {0x10421a20u, 0x10421af0u, 0x10421be0u, 0x10421ce0u, 0x10421dd0u, 0x10423040u})
            for (int revision : {16, 17, 18}) for (int status : {2, 3, 5}) {
                Case t{entry}; t.actual_query = 1; t.news_revision = revision; t.status = status; cases.push_back(t);
            }
        unsigned completed = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (bool production : {false, true}) {
          production_mode = production;
          for (const auto& test : cases) {
            auto s = cm::semantic_io_checks::clone(*base); Cpu c(*s); active = test; call_count = 0;
            initialize(c, test); s->pc = test.entry;
            ObservedWrites observer;
            if (production) s->memory.observer = &observer;
            const auto differences = compare_routine_call(*s, test.entry, original, rewrite,
                RoutineChecker::result_r3 | RoutineChecker::result_f1);
            if (!differences.empty()) {
                std::cerr << "Case " << completed << " entry " << hex_address(test.entry) << '\n';
                for (const auto& d : differences) std::cerr << d << '\n';
                return 1;
            }
            ++completed;
            ++counts[test.entry][production];
          }
        }
        production_mode = false;
        auto s = cm::semantic_io_checks::clone(*base); Cpu c(*s); active = Case{entries[0]}; call_count = 0;
        initialize(c, active); s->pc = active.entry;
        if (compare_routine_call(*s, active.entry, original, corrupted, RoutineChecker::result_r3).empty())
            throw std::runtime_error("Corrupted history was not detected");
        active = Case{0x10420e40}; call_count = 0;
        auto result_case = cm::semantic_io_checks::clone(*base); Cpu result_cpu(*result_case);
        initialize(result_cpu, active); result_case->pc = active.entry;
        const auto result_differences = compare_routine_call(*result_case, active.entry, original,
            corrupted_result, RoutineChecker::result_r3);
        if (result_differences.size() != 1 || !result_differences.front().starts_with("r3 "))
            throw std::runtime_error("Corrupted command result was not detected independently");
        for (auto entry : entries) {
            active = Case{entry};
            const auto prepared = [&] {
                auto state = cm::semantic_io_checks::clone(*base); Cpu cpu(*state);
                call_count = 0; initialize(cpu, active); return state;
            };
            auto expected = prepared(); original(*expected, entry);
            const auto limit = expected->executed - 1;
            auto normal = prepared(); normal->budget = limit; ObservedWrites observer;
            normal->memory.observer = &observer; production_mode = true; rewrite(*normal, entry);
            auto rejected = prepared(); rejected->budget = limit; bool exhausted = false;
            try { original(*rejected, entry); } catch (const std::exception& error) {
                exhausted = std::string_view(error.what()).find("budget") != std::string_view::npos;
            }
            if (!exhausted || normal->executed >= expected->executed)
                throw std::runtime_error("Production ownership budget control failed " + hex_address(entry));
        }
        for (const auto& [entry, n] : counts)
            std::cout << "entry " << hex_address(entry) << ": " << n[0] << " controlled, " << n[1] << " production\n";
        std::cout << "TRANSFER_SCREENS: " << completed << " returned comparisons; corruption and production controls passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
