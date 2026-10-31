#include "rng_core_adapter.hpp"
#include "rng_core.hpp"
#include "utils.hpp"
#include "utils_adapter.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <tuple>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
void draw(State& s) {
    // Load after the original r3 assignment; leave r4 as the incoming state.
    set(s, 3, 0x41c60000);
    set(s, 4, s.memory.read<4>(reg(s, 2) - 0x34c4));
    set(s, 0, cm::rng::multiplier);
    set(s, 3, reg(s, 4) * cm::rng::multiplier);
    set(s, 0, reg(s, 3) + cm::rng::increment);
    s.memory.write<4>(reg(s, 2) - 0x34c4, reg(s, 0));
    set(s, 3, cm::rng::sample(reg(s, 0)));
    s.pc = std::uint32_t(s.r<4>(4128));
}
void seed(State& s) {
    s.memory.write<4>(reg(s, 2) - 0x34c4, reg(s, 3));
    s.pc = std::uint32_t(s.r<4>(4128));
}
void digest_boundary(State& s, std::uint32_t entry, unsigned event) {
    // Compact ordered evidence only; full state/effects are compared by the
    // direct harness and the existing strict season transcript independently.
    for (auto word : {event, entry, s.pc, reg(s, 0), reg(s, 2), reg(s, 3),
                      reg(s, 4), reg(s, 5), reg(s, 6), reg(s, 7), reg(s, 8),
                      std::uint32_t(s.r<4>(4128))})
        for (unsigned shift = 0; shift < 32; shift += 8) {
            s.rng_ordered_digest ^= (word >> shift) & 255;
            s.rng_ordered_digest *= UINT64_C(1099511628211);
        }
}
class StorageObserver final : public MemoryObserver {
    State& state;
    MemoryObserver* previous;
    std::array<std::uint32_t, 4> addresses;
    struct Visits { std::uint64_t count = 0, first = 0, last = 0; };
    std::map<std::tuple<unsigned, bool, std::uint32_t>, Visits> visits;
    std::uint64_t sequence = 0;
    void access(std::uint32_t address, std::uint64_t size, bool write) {
        for (unsigned i = 0; i < addresses.size(); ++i) {
            if (std::uint64_t(address) >= std::uint64_t(addresses[i]) + 4 ||
                std::uint64_t(address) + size <= addresses[i]) continue;
            auto& record = visits[{i, write, state.pc}];
            ++sequence; if (!record.count++) record.first = sequence;
            record.last = sequence;
        }
    }
public:
    explicit StorageObserver(State& s) : state(s), previous(s.memory.date_observer) {
        const auto toc = reg(s, 2);
        addresses[0] = toc - 0x34c4;
        for (unsigned i = 1; i < 4; ++i) {
            const auto slot = toc - std::array<std::uint32_t,3>{0x7b90,0x7b88,0x7b84}[i-1];
            const auto bytes = s.memory.copy_out(slot, 4);
            addresses[i] = std::uint32_t(read_be<4>(bytes.data()));
        }
        if (previous) throw std::runtime_error("RNG storage inventory requires an unused validation observer");
        state.memory.date_observer = this;
    }
    ~StorageObserver() override {
        if (state.memory.date_observer == this) state.memory.date_observer = previous;
        for (const auto& [key, value] : visits) {
            const auto [field, write, pc] = key;
            std::fprintf(stderr, "rng-storage field=%s access=%s pc=%08x count=%llu first=%llu last=%llu\n",
                std::array<const char*,4>{"crt","salt","end","cursor"}[field], write ? "write" : "read", pc,
                static_cast<unsigned long long>(value.count), static_cast<unsigned long long>(value.first),
                static_cast<unsigned long long>(value.last));
        }
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t address, std::uint64_t size) override { access(address, size, true); }
    void written(std::uint32_t address, const std::uint8_t*, std::size_t size) override { access(address, size, true); }
    void read(std::uint32_t address, const std::uint8_t*, unsigned size) override { access(address, size, false); }
};
}
std::unique_ptr<MemoryObserver> observe_rng_storage(State& s) {
    return std::make_unique<StorageObserver>(s);
}
bool try_modern_rng_core(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.rng_original_resume == entry) { s.rng_original_resume = 0; return false; }
    const auto found = std::find(rng_core_entries.begin(), rng_core_entries.end(), entry);
    if (found == rng_core_entries.end()) throw std::runtime_error("Unknown RNG core entry");
    const auto index = std::size_t(found - rng_core_entries.begin());
    const bool original = s.rng_original || s.rng_entry_original[index] || (index == 2 && utils_declining());
    auto& count = original ? s.rng_original_calls[index] : s.rng_calls[index];
    ++count;
    if (original && !s.rng_trace && !s.rng_observer) return false;
    const auto caller = std::uint32_t(s.r<4>(4128)), input = reg(s, 3);
    if (s.rng_trace) digest_boundary(s, entry, 1);
    if (s.rng_observer) s.rng_observer->enter(s, entry, original);
    if (original) {
        s.rng_original_resume = entry;
        try { run_until(s, caller); }
        catch (...) { s.rng_original_resume = 0; throw; }
    } else {
        s.step(entry);
        if (index == 0) draw(s);
        else if (index == 1) seed(s);
        else {
            guest::Cpu c(s);
            utils::initialize_random(c, c.r(3));
            c.finish();
        }
    }
    if (s.rng_observer) s.rng_observer->leave(s, entry, original);
    if (s.rng_trace) {
        digest_boundary(s, entry, 2);
        if (count <= 8 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER"))
            std::fprintf(stderr, "rng-core.%s entry=%08x sample=%llu caller=%08x input=%08x r0=%08x r3=%08x r4=%08x r8=%08x\n",
                original ? "original" : "modern", entry, static_cast<unsigned long long>(count),
                caller, input, reg(s, 0), reg(s, 3), reg(s, 4), reg(s, 8));
    }
    return true;
}
void reset_rng_diagnostics(State& s) {
    s.rng_original = s.rng_trace = false;
    s.rng_entry_original.fill(false);
    s.rng_calls.fill(0); s.rng_original_calls.fill(0);
    s.rng_original_resume = 0;
    s.rng_ordered_digest = UINT64_C(14695981039346656037);
    s.rng_observer = nullptr;
}
std::string rng_core_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < rng_core_entries.size(); ++i)
        out << "rng.entry=" << hex_address(rng_core_entries[i])
            << " mode=" << (s.rng_original || s.rng_entry_original[i] || (i == 2 && utils_declining()) ? "original" : "modern")
            << " modern=" << s.rng_calls[i] << " original=" << s.rng_original_calls[i] << '\n';
    if (s.rng_trace) out << "rng.ordered-boundaries=" << std::hex << s.rng_ordered_digest << '\n';
    return out.str();
}
}
