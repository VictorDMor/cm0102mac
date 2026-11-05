#include "african_nations_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <array>
#include <random>

#ifdef CM_AFRICAN_NATIONS_PROFILE_DIR
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto placements = 0x104f7000u, lookup = 0x10459b50u;
constexpr auto notify = 0x104f5da0u, interpolate = 0x10476780u;
constexpr auto destroy = 0x104f4ee0u, update = 0x104f5d60u;
constexpr auto object = fixture + 0x100, key = fixture + 0x300;
constexpr auto stage = fixture + 0x1000, rows = fixture + 0x2000;
constexpr auto manager_slot = fixture + 0x400, manager = fixture + 0x500, alternate_manager = fixture + 0x540;
constexpr auto definition = fixture + 0x600, alternate_definition = fixture + 0x640;
constexpr auto event = fixture + 0x700, extra = fixture + 0x780;
constexpr auto child_array = fixture + 0x800, child = fixture + 0x900, standings = fixture + 0x2800;
constexpr auto service_log = fixture + 0x3c00, mutate = fixture + 0x44;
constexpr auto comparator = 0x108b99f4u;
constexpr auto child_vtable = fixture + 0x3a00, child_descriptor = fixture + 0x3b00;
constexpr auto lifecycle_mutation = fixture + 0x48;
struct Call {
    std::uint32_t entry, continuation;
    decltype(State::registers) machine;
    bool operator==(const Call&) const = default;
};
std::vector<Call>* active_trace = nullptr;
unsigned expected_scale = 0;
unsigned external_calls = 0;
bool message_service(std::uint32_t entry) {
    return entry == 0x10384500 || entry == 0x10384820 || entry == 0x103846e0
        || entry == 0x10383410 || entry == 0x10383b70 || entry == 0x10384020;
}
bool lifecycle_service(std::uint32_t entry) {
    return entry == 0x104a6660 || entry == 0x10775170 || entry == 0x104536c0
        || entry == 0x1049e600 || entry == 0x107735a0;
}
bool trace_lookup(State& s) {
    Cpu c(s);
    const auto entry = s.pc;
    if (entry == lookup || entry == interpolate || entry == 0x10781320 || entry == 0x10715880
            || message_service(entry) || lifecycle_service(entry)) {
        if (entry == lookup) require(c.r(3) == stage && c.r(4) == key && c.lr() == 0x104f7034,
                "African Nations real record lookup arguments");
        if (entry == interpolate) {
            require(c.r(3) == 40 && c.r(4) == 100 && c.r(6) == 3 && std::int32_t(c.r(5)) >= 0
                        && c.r(5) <= 3, "African Nations real interpolation arguments");
            expected_scale = c.r(5) * 30;
        }
        if (message_service(entry)) {
            require(c.r(3) == c.u32(manager_slot) && c.r(4) == key, "African Nations notification manager/team");
            if (entry == 0x10384500 || entry == 0x10384820 || entry == 0x103846e0)
                require(c.r(5) == c.u32(object + 4), "African Nations live competition definition");
            if (entry == 0x10384820) require(c.r(6) == 0x41c && c.r(7) == 25, "African Nations elimination notification");
            if (entry == 0x10384500 && c.lr() == 0x104f5dfc)
                require(c.r(6) == 0 && c.r(7) == 120 && c.r(8) == 25, "African Nations group notification");
            if (entry == 0x10384500 && c.lr() == 0x104f5e74)
                require(c.r(6) == std::uint32_t(c.i16(event + 0x32)) && c.r(8) == expected_scale
                            && c.r(7) == std::uint32_t(c.i16(standings + c.i16(event + 0x34) * 0x78 + 0x80)),
                        "African Nations live table-based award inputs");
            if (entry == 0x103846e0)
                require(c.r(6) == std::uint32_t(c.i16(event + 0x32)) && c.r(7) == std::uint32_t(c.i16(event + 0x30))
                            && c.r(8) == expected_scale, "African Nations live event-based award inputs");
            if (entry == 0x10383410 || entry == 0x10383b70 || entry == 0x10384020)
                require(c.r(5) == event && c.r(6) == extra, "African Nations forwarded event notification inputs");
        }
        if (active_trace) active_trace->push_back({entry, c.lr(), s.registers});
        if (lifecycle_service(entry)) {
            const auto pointer = c.r(3);
            if (entry == 0x104a6660 || entry == 0x1049e600)
                require(pointer == object && c.r(4) == 0, "African competition base cleanup arguments");
            if (entry == 0x10775170)
                require(pointer == extra || pointer == child_array, "African owned allocation release argument");
            if (entry == 0x104536c0) {
                require(c.r(4) == 1 && (pointer == stage || pointer == child || pointer == child + 0x100
                            || pointer == child + 0x200), "African child/table destructor arguments");
                // The parent must reread its live count after a child call.
                if (pointer == child && c.u8(lifecycle_mutation)) c.put32(object + 0x30, 0);
            }
            if (entry == 0x107735a0) require(pointer == object, "African final object delete argument");
            for (unsigned i = 0; i < 4; ++i) c.put32(service_log + external_calls * 16 + i * 4, c.r(3 + i));
            ++external_calls;
            // These neighboring destructors/allocators are boundary models.
            // Record a consumed-pointer effect without substituting an owned
            // lifecycle routine or fabricating its branches and returned data.
            c.put32(pointer, 0xd0000000u | (entry & 0xffff));
            c.set({{3, 0x90909090}, {4, 0xbad0bad0}, {9, 0xfeedefee}});
            c.finish();
            return true;
        }
        if (message_service(entry)) {
            // Consume the validated manager/key and record the arguments in
            // game memory. These external news services are modeled; neither
            // the owned callback nor the real interpolation is substituted.
            c.put32(c.r(3), c.u32(c.r(4)));
            for (unsigned i = 0; i < 8; ++i) c.put32(service_log + i * 4, c.r(3 + i));
            c.set({{3, 0x90909090}, {4, 0xbad0bad0}, {9, 0xfeedefee}});
            c.finish();
            return true;
        }
    }
    return false; // Execute the real lookup/interpolation and their children.
}
struct Services {
    decltype(dispatch_probe) saved_probe = dispatch_probe;
    std::vector<Call>* saved_trace = active_trace;
    explicit Services(std::vector<Call>* trace) {
        active_trace = trace;
        expected_scale = 0;
        external_calls = 0;
        dispatch_probe = trace ? trace_lookup : nullptr;
    }
    ~Services() { dispatch_probe = saved_probe; active_trace = saved_trace; }
};

// mode0: cached record; mode1: sorted search; mode2: real sort then search;
// mode3: missing key; mode4: missing table; mode5: cached stack alias.
std::unique_ptr<State> placement_fixture(std::uint32_t phase = 0, std::uint32_t first = 1,
                                        std::uint32_t second = 3, std::uint32_t third = 4,
                                        unsigned mode = 0, unsigned id = 42) {
    auto s = std::make_unique<State>();
    s->registers.fill(0xa5);
    s->unique.fill(0x37);
    s->memory.map(fixture, 0x4000);
    s->memory.map(stack - 0x2000, 0x2100);
    s->memory.map(toc - 0x8000, 0x8000);
    Cpu c(*s);
    c.set({{1, stack}, {2, toc}, {3, object}, {4, key}, {5, phase}, {6, first}, {7, second}, {8, third}});
    c.set_lr(return_sentinel);
    c.put32(object + 8, stage);
    c.put32(key, mode == 3 ? 1234 : id);
    c.put32(stage + 0xc, mode == 4 ? 0 : rows);
    c.put32(stage + 0x14, 3);
    c.put8(stage + 0x18, mode == 2 ? 0 : 0xff);
    c.put32(stage + 0x5cc, mode == 0 ? rows + 0x7c : mode == 5 ? stack - 0x80 : 0);
    c.put32(toc - 0x6194, comparator);
    c.put32(comparator, 0x10460e70);
    c.put32(comparator + 4, toc);
    c.put32(toc - 0x7ba8, fixture + 0xe00);
    c.put32(toc - 0x4984, fixture + 0xf00);
    const std::array ordered{17u, 42u, 99u}, unordered{99u, 17u, 42u};
    for (unsigned i = 0; i < 3; ++i) {
        const auto row = rows + i * 0x7c;
        c.put32(row, (mode == 2 ? unordered : ordered)[i]);
        for (unsigned offset = 4; offset < 0x7c; ++offset) c.put8(row + offset, 0xcb);
    }
    s->pc = placements;
    s->budget = 1000000;
    return s;
}

std::unique_ptr<State> notification_fixture(std::uint32_t kind, std::uint32_t phase,
                                          unsigned rank = 2, bool mutation = false) {
    auto s = placement_fixture();
    Cpu c(*s);
    c.set({{5, kind}, {6, phase}, {7, 0xa1b2c3d4}, {8, event}, {9, extra}});
    c.put32(toc - 0x7b48, manager_slot);
    c.put32(manager_slot, manager);
    c.put32(object + 4, definition);
    c.put32(object + 0xc, child_array);
    c.put32(child_array + 0xc, child);
    c.put32(child + 0xa0, standings);
    c.put16(event + 0x30, 13);
    c.put16(event + 0x32, std::uint16_t(-7));
    c.put16(event + 0x34, rank);
    for (unsigned i = 0; i < 4; ++i) c.put16(standings + i * 0x78 + 0x80, 500 + i);
    c.put32(toc - 0x47e4, fixture + 0xf00);
    c.put8(mutate, mutation);
    s->pc = notify;
    return s;
}

std::unique_ptr<State> destruction_fixture(std::uint32_t mode, int last_child = 2,
                                         unsigned present = 5, bool schedule = true,
                                         bool table = true, bool array = true,
                                         bool null_object = false, bool mutation = false) {
    auto s = placement_fixture();
    Cpu c(*s);
    c.set(3, null_object ? 0 : object);
    c.set(4, mode);
    c.put32(toc - 0x6078, child_vtable);
    c.put32(object + 0xbc, schedule ? extra : 0);
    c.put32(object + 8, table ? stage : 0);
    c.put32(object + 0xc, array ? child_array : 0);
    c.put32(object + 0x30, std::uint32_t(last_child));
    c.put32(child_vtable + 8, child_descriptor);
    c.put32(child_descriptor, 0x104536c0);
    c.put32(child_descriptor + 4, toc);
    for (unsigned i = 0; i < 3; ++i) {
        c.put32(child_array + i * 4, present & (1u << i) ? child + i * 0x100 : 0);
        c.put32(child + i * 0x100, child_vtable);
    }
    c.put8(lifecycle_mutation, mutation);
    // Generated callers invoke the allocator owner in place, before the
    // dispatcher probe. Decline that neighboring owner on both sides so its
    // modeled boundary is reached through the same dispatcher path.
    s->allocator_original = true;
    s->pc = destroy;
    return s;
}

std::unique_ptr<State> child_fixture(unsigned mode = 0) {
    auto s = placement_fixture();
    Cpu c(*s);
    c.put32(object + 0xc, child_array);
    c.put32(child_array + 0xc, mode == 1 ? 0 : child);
    c.put32(child, child_vtable);
    c.put32(child_vtable + 0x38, mode == 2 ? 0 : child_descriptor);
    c.put32(child_descriptor, 0x10715880);
    // Use a different mapped TOC to prove the real CFM adapter switches it,
    // saves the caller's value and lets this owner restore it afterwards.
    c.put32(child_descriptor + 4, toc - 0x100);
    s->pc = update;
    return s;
}

std::unique_ptr<State> entry_fixture(std::uint32_t entry) {
    if (entry == notify) return notification_fixture(0, 3);
    if (entry == destroy) return destruction_fixture(1);
    if (entry == update) return child_fixture();
    return placement_fixture();
}

struct Outcome { std::string stop; unsigned instructions; std::array<unsigned, 3> fields, ids; unsigned record_id, notifications; };
unsigned rejected_results = 0, rejected_arguments = 0;
Outcome compare_routes(const State& seed, bool observed = true, bool negative = false) {
    auto original = clone(seed), modern = clone(seed);
    original->allocator_original = modern->allocator_original = seed.allocator_original;
    Effects expected, actual;
    expected.state = original.get(); actual.state = modern.get();
    if (seed.pc == notify && Cpu(*original).u8(mutate)) {
        expected.on_read = actual.on_read = [](State& s, Effects& effects, std::uint32_t address, unsigned) {
            if (effects.mutated || address != stack - 0x48 || effects.log.back().value != 0x104f5e40) return;
            effects.mutated = true;
            Cpu c(s);
            c.put16(event + 0x34, 3);
            c.put16(event + 0x32, std::uint16_t(-6));
            c.put32(manager_slot, alternate_manager);
            c.put32(object + 4, alternate_definition);
        };
    }
    if (observed) {
        original->memory.date_observer = &expected;
        modern->memory.date_observer = &actual;
    }
    std::vector<Call> old_calls, new_calls;
    const auto run = [observed](State& s, bool reference, std::vector<Call>& calls) {
        Services services(observed ? &calls : nullptr);
        try {
            if (reference) african_nations_run_original(s, s.pc);
            else run_until(s, return_sentinel);
            return std::string{};
        } catch (const std::runtime_error& error) { return std::string(error.what()); }
    };
    const auto old_stop = run(*original, true, old_calls), new_stop = run(*modern, false, new_calls);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = "African " + hex_address(seed.pc) + " budget=" + std::to_string(seed.budget);
    if (old_stop != new_stop) {
        std::cerr << label << " originalPC=" << hex_address(original->pc) << " C++PC=" << hex_address(modern->pc) << '\n';
        for (const auto& call : old_calls) std::cerr << " original call=" << hex_address(call.entry) << " LR=" << hex_address(call.continuation) << '\n';
        for (const auto& call : new_calls) std::cerr << " C++ call=" << hex_address(call.entry) << " LR=" << hex_address(call.continuation) << '\n';
    }
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    require(old_stop.empty() || old_stop == "Native instruction budget exhausted"
                || old_stop.starts_with("Unmapped data access at "), label + " unexpected stop=" + old_stop);
    if (expected.bytes != actual.bytes) {
        for (unsigned i = 0; i < std::min(expected.log.size(), actual.log.size()); ++i) {
            const auto& a = expected.log[i]; const auto& b = actual.log[i];
            if (a.kind != b.kind || a.address != b.address || a.value != b.value || a.size != b.size)
                throw std::runtime_error(label + " effect=" + std::to_string(i) + " address="
                    + hex_address(a.address) + "/" + hex_address(b.address));
        }
        throw std::runtime_error(label + " ordered effect counts=" + std::to_string(expected.log.size())
            + "/" + std::to_string(actual.log.size()) + " PCs=" + hex_address(original->pc) + "/" + hex_address(modern->pc));
    }
    if (old_calls != new_calls) {
        std::cerr << label << " call counts=" << old_calls.size() << '/' << new_calls.size() << '\n';
        for (unsigned i = 0; i < std::min(old_calls.size(), new_calls.size()); ++i) {
            const auto& a = old_calls[i]; const auto& b = new_calls[i];
            if (a == b) continue;
            std::cerr << " first different call=" << i << " entries=" << hex_address(a.entry) << '/' << hex_address(b.entry)
                      << " LR=" << hex_address(a.continuation) << '/' << hex_address(b.continuation) << '\n';
            if (a.machine != b.machine) {
                const auto p = std::mismatch(a.machine.begin(), a.machine.end(), b.machine.begin());
                std::cerr << " register byte=" << p.first - a.machine.begin() << " values=" << unsigned(*p.first) << '/' << unsigned(*p.second) << '\n';
            }
            break;
        }
    }
    require(old_calls == new_calls, label + " dependency call-state mismatch");
    require(original->executed == modern->executed, label + " instruction accounting="
            + std::to_string(original->executed) + "/" + std::to_string(modern->executed));
    const Image image(*original);
    image.compare(*modern, label);
    Cpu c(*modern);
    if (seed.pc == notify && c.u8(mutate)) require(expected.mutated && actual.mutated, "African post-interpolation observer was not reached");
    const auto record = c.r(3);
    if (negative) {
        const auto corrupt_at = seed.pc == notify || seed.pc == destroy ? service_log
            : seed.pc == update ? child : record + 0x76;
        const auto before = c.u8(corrupt_at);
        c.put8(corrupt_at, before ^ 1);
        bool detected = false;
        try { image.compare(*modern, label); }
        catch (const std::runtime_error& error) { detected = std::string_view(error.what()).find("memory mismatch") != std::string_view::npos; }
        require(detected, "African placement corruption was missed");
        c.put8(corrupt_at, before);
        ++rejected_results;
        const auto target = std::ranges::find_if(new_calls, [](const Call& call) {
            return call.entry == lookup || message_service(call.entry) || lifecycle_service(call.entry) || call.entry == 0x10715880;
        });
        require(target != new_calls.end(), "African argument control did not reach its call");
        target->machine[guest::reg::gpr(seed.pc == update ? 3 : 4)] ^= 4;
        require(old_calls != new_calls, "African consumed lookup argument corruption was missed");
        ++rejected_arguments;
    }
    Outcome out{new_stop, unsigned(modern->executed), {}, {}, 0, 0};
    out.notifications = unsigned(std::ranges::count_if(new_calls, [](const Call& call) { return message_service(call.entry); }));
    if (new_stop.empty() && seed.pc == placements) {
        for (unsigned i = 0; i < 3; ++i) { out.fields[i] = c.u8(record + 0x76 + i); out.ids[i] = c.u32(rows + i * 0x7c); }
        out.record_id = c.u32(record);
    }
    return out;
}

void compare_block_sequence() {
    auto seed = destruction_fixture(1, 2, 0);
    Cpu c(*seed);
    c.put32(child_array + 0xc, child);
    c.put32(child_vtable + 0x38, child_descriptor + 8);
    c.put32(child_descriptor + 8, 0x10715880);
    c.put32(child_descriptor + 12, toc - 0x100);
    c.put32(toc - 0x7b48, manager_slot);
    c.put32(manager_slot, manager);
    c.put32(object + 4, definition);
    c.put32(child + 0xa0, standings);
    c.put16(event + 0x30, 13);
    c.put16(event + 0x32, std::uint16_t(-7));
    c.put16(event + 0x34, 2);
    c.put16(standings + 2 * 0x78 + 0x80, 502);
    c.put32(toc - 0x47e4, fixture + 0xf00);
    auto original = clone(*seed), modern = clone(*seed);
    original->allocator_original = modern->allocator_original = true;
    Effects old_effects, new_effects;
    old_effects.state = original.get(); new_effects.state = modern.get();
    original->memory.date_observer = &old_effects;
    modern->memory.date_observer = &new_effects;
    std::vector<Call> old_calls, new_calls;
    const auto run = [](State& state, bool reference, std::vector<Call>& calls) {
        Services services(&calls);
        Cpu cpu(state);
        for (const auto entry : {update, notify, placements, destroy}) {
            cpu.set({{3, object}, {4, key}, {5, 0}, {6, 3}, {7, 4}, {8, event}, {9, extra}});
            if (entry == placements) cpu.set({{5, 0}, {6, 1}, {7, 3}, {8, 4}});
            if (entry == destroy) cpu.set(4, 1);
            cpu.set_lr(return_sentinel);
            state.pc = entry;
            if (reference) african_nations_run_original(state, entry);
            else run_until(state, return_sentinel);
            require(state.pc == return_sentinel && cpu.r(2) == toc, "African block sequence return/TOC");
        }
    };
    run(*original, true, old_calls);
    run(*modern, false, new_calls);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(old_effects.bytes == new_effects.bytes && old_calls == new_calls,
            "African four-entry sequence ordered memory/dependency state");
    require(original->executed == modern->executed, "African four-entry sequence instruction accounting");
    Image(*original).compare(*modern, "African four-entry sequence final state");
    Cpu result(*modern);
    require(result.u16(child) == 40 && result.u16(child + 2) == 1972 && result.u8(child + 4) == 1,
            "African real virtual date constructor persisted through sequence");
    require(result.u32(manager) == result.u32(key) && result.u8(rows + 0x7c + 0x76) == 5
                && !result.u32(object + 0xc) && !result.u32(object + 0xbc),
            "African sequence consumes notifications, placements and cleanup");
    std::cout << "AFRICAN_NATIONS controlled update/notify/placements/destroy sequence: complete final state matches\n";
}

void verify() {
    unsigned returned = 0, budgets = 0, faults = 0;
    for (const auto phase : {0u, 2u, 3u, 0xffu, 0x100u, 0x123480u})
        for (const auto placing : {0u, 1u, 2u, 3u, 4u, 5u, 0xffu, 0x100u, 0x101u, 0x1234feu}) {
            require(compare_routes(*placement_fixture(phase, placing, placing, placing)).stop.empty(), "African placing width partitions");
            ++returned;
        }
    for (const auto mode : {0u, 1u, 2u}) for (const auto id : {17u, 42u, 99u}) {
        const auto out = compare_routes(*placement_fixture(0, 1, 3, 4, mode, id), true, mode == 1 && id == 42);
        require(out.stop.empty() && out.fields == std::array<unsigned, 3>{5, 9, 13}, "African real cached/search/sort result fields");
        require(out.record_id == (mode == 0 ? 42u : id) && out.ids == std::array<unsigned, 3>{17, 42, 99},
                "African cache preference and real sorted lookup identity");
        ++returned;
    }
    for (const auto mode : {3u, 4u}) {
        require(compare_routes(*placement_fixture(0, 1, 3, 4, mode)).stop.starts_with("Unmapped data access at "),
                "African absent lookup retains partial field fault");
        ++faults;
    }
    require(compare_routes(*placement_fixture(0, 1, 3, 4, 5)).stop.empty(), "African output aliases retained stack words");
    ++returned;
    std::mt19937 random(0x414652);
    for (unsigned i = 0; i < 12; ++i) {
        const auto phase = random(), first = random(), second = random(), third = random();
        require(compare_routes(*placement_fixture(phase, first, second, third, i % 3)).stop.empty(), "African seeded field mutation");
        ++returned;
    }
    for (const auto mode : {0u, 1u}) {
        require(compare_routes(*placement_fixture(0, 1, 3, 4, mode), false).stop.empty(), "African normal route without diagnostics");
        ++returned;
    }
    for (const auto entry : {placements, notify, destroy, update}) {
        auto seed = entry_fixture(entry);
        const auto complete = compare_routes(*seed);
        ++returned;
        for (unsigned limit = 0; limit < complete.instructions; limit += 7) {
            seed->budget = limit;
            if (compare_routes(*seed).stop == "Native instruction budget exhausted") ++budgets;
        }
    }
    for (const auto phase : {0u, 2u, 3u, 4u, 0xffffff80u, 0x103u})
        for (const auto kind : {0u, 1u, 2u, 3u, 4u, 6u, 0xfffffffeu, 0xffffffffu, 0x106u}) {
            const auto out = compare_routes(*notification_fixture(kind, phase));
            const auto signed_phase = std::int8_t(phase), signed_kind = std::int8_t(kind);
            const auto expected_call = signed_phase < 3 ? signed_kind == 6 || signed_kind == -2
                : signed_phase == 3 && (signed_kind == 0 || signed_kind == -1 || (signed_kind >= 1 && signed_kind <= 3));
            require(out.stop.empty() && out.notifications == unsigned(expected_call), "African notification stage/kind partitions");
            ++returned;
        }
    for (const auto rank : {0u, 1u, 2u, 3u}) {
        require(compare_routes(*notification_fixture(0, 3, rank), true, rank == 2).stop.empty(), "African real award interpolation");
        ++returned;
    }
    require(compare_routes(*notification_fixture(0, 3, 2, true)).stop.empty(), "African live fields after interpolation");
    ++returned;
    for (const auto mode : {0u, 1u, 0xffffu, 0x10000u, 0x8000u, 0x7fffu}) {
        for (const auto last : {-1, 0, 1, 2}) for (const auto present : {0u, 1u, 5u, 7u}) {
            require(compare_routes(*destruction_fixture(mode, last, present), true, mode == 1 && last == 2 && present == 5).stop.empty(),
                    "African destructor signed mode/live child partitions");
            ++returned;
        }
        require(compare_routes(*destruction_fixture(mode, 2, 7, true, true, true, true)).stop.empty(), "African null destructor receiver");
        ++returned;
    }
    for (const auto schedule : {false, true}) for (const auto table : {false, true}) for (const auto array : {false, true}) {
        require(compare_routes(*destruction_fixture(1, -1, 0, schedule, table, array)).stop.empty(), "African absent destructor allocations");
        ++returned;
    }
    require(compare_routes(*destruction_fixture(1, 2, 7, true, true, true, false, true)).stop.empty(),
            "African child destructor changes live parent count");
    ++returned;
    require(compare_routes(*child_fixture(), true, true).stop.empty(), "African real child virtual constructor");
    ++returned;
    for (const auto mode : {1u, 2u}) {
        require(compare_routes(*child_fixture(mode)).stop.starts_with("Unmapped data access at "), "African invalid child callback partial fault");
        ++faults;
    }
    for (const auto entry : {notify, destroy, update}) {
        auto seed = entry == notify ? notification_fixture(0, 4)
            : entry == destroy ? destruction_fixture(1, -1, 0, false, false, false, true) : child_fixture();
        require(compare_routes(*seed, false).stop.empty(), "African default route without probes/observers");
        ++returned;
    }
    for (unsigned i = 0; i < 16; ++i) {
        const auto mode = random();
        const auto last = int(random() % 4) - 1;
        const auto present = random() & 7;
        const auto schedule = bool(random() & 1), table = bool(random() & 1);
        require(compare_routes(*destruction_fixture(mode, last, present, schedule, table)).stop.empty(), "African seeded lifecycle mutation");
        ++returned;
    }
    for (const auto entry : {placements, notify, destroy, update}) {
        RoutineChecker checker([entry](std::uint32_t actual) { return actual == entry; }, 1, 1);
        checker.strict_volatile = true;
        routine_checker = &checker;
        std::vector<Call> calls;
        Services services(entry == placements ? nullptr : &calls);
        auto seed = entry_fixture(entry);
        run_until(*seed, return_sentinel);
        routine_checker = nullptr;
        require(checker.totals().at(entry).checked == 1 && !checker.differences(), "African checked default hook");
    }
    {
        RoutineChecker excluded([](std::uint32_t) { return false; }, 0, 0);
        routine_checker = &excluded;
        auto seed = placement_fixture();
        require(!try_modern_african_nations(*seed, placements), "African excluded hook");
        routine_checker = nullptr;
    }
    for (const auto entry : {0u, placements + 4, UINT32_MAX}) {
        auto seed = placement_fixture();
        bool rejected = false;
        try { african_nations_run_rewrite(*seed, entry); }
        catch (const std::logic_error& error) { rejected = std::string_view(error.what()).starts_with("Unknown AFRICAN_NATIONS.CPP entry "); }
        require(rejected, "African unknown owner entry accepted");
    }
    compare_block_sequence();
    std::cout << "AFRICAN_NATIONS " << returned << " returned comparisons (28 scalar mutations; seed0x414652), "
              << budgets << " budget stops, " << faults << " partial faults; " << rejected_results << " wrong result and "
              << rejected_arguments << " wrong consumed argument detected; four checked hooks\n";
}
}

int main() try {
#ifdef CM_AFRICAN_NATIONS_PROFILE_DIR
    routine_child_exit = [] {
        const auto name = std::string(CM_AFRICAN_NATIONS_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
        __llvm_profile_set_filename(name.c_str());
        if (__llvm_profile_write_file()) throw std::runtime_error("Cannot flush African Nations coverage");
    };
#endif
    verify();
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
