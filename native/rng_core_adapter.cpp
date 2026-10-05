#include "rng_core_adapter.hpp"
#include "rng_core.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <tuple>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
std::int32_t signed_word(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
std::uint32_t signed_high_product(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t((std::int64_t(signed_word(a)) * signed_word(b)) >> 32);
}
std::uint32_t arithmetic_shift(State& s, std::uint32_t value, unsigned shift) {
    s.wr<1>(1027, signed_word(value) < 0 && (value & ((1u << shift) - 1)) != 0);
    return std::uint32_t(signed_word(value) >> shift);
}
void compare_unsigned(State& s, std::uint32_t a, std::uint32_t b) {
    s.wr<1>(2304, (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
void call(State& s, std::uint32_t entry, std::uint32_t link) {
    s.wr<4>(2440, reg(s, 2));
    s.wr<4>(4128, link);
    s.pc = entry;
    run_until(s, link);
}
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
void initialize_pool(State& s) {
    // Interleave saves and slot reads: TOC slots may alias the frame.
    set(s, 0, s.r<4>(4128));
    s.memory.write<4>(reg(s, 1) - 4, reg(s, 31));
    set(s, 31, s.memory.read<4>(reg(s, 2) - 0x7b84));
    s.memory.write<4>(reg(s, 1) - 8, reg(s, 30));
    set(s, 30, s.memory.read<4>(reg(s, 2) - 0x7b8c));
    s.memory.write<4>(reg(s, 1) - 12, reg(s, 29));
    set(s, 29, s.memory.read<4>(reg(s, 2) - 0x7b88));
    s.memory.write<4>(reg(s, 1) - 16, reg(s, 28));
    set(s, 28, reg(s, 3));
    const auto requested = signed_word(reg(s, 28));
    s.wr<1>(2304, (s.r<1>(2304) & 0xf0) |
        (requested < 0 ? 8 : requested > 0 ? 4 : 2) | (s.r<1>(1024) & 1));
    s.memory.write<4>(reg(s, 1) + 8, reg(s, 0));
    const auto old_stack = reg(s, 1);
    s.memory.write<4>(old_stack - 80, old_stack);
    set(s, 1, old_stack - 80);
    if (!reg(s, 28)) {
        set(s, 3, 0);
        call(s, 0x1077f900, 0x107474d8);
        set(s, 28, reg(s, 3));
    }
    set(s, 3, reg(s, 28));
    call(s, 0x1077b790, 0x107474e4);
    set(s, 4, s.memory.read<4>(reg(s, 2) - 0x3e34));
    set(s, 5, reg(s, 28));
    set(s, 3, s.memory.read<4>(reg(s, 2) - 0x7bb8));
    set(s, 4, reg(s, 4) + 0x25b);
    call(s, 0x1077b720, 0x107474f8);
    call(s, 0x1077b770, 0x107474fc);

    // Signed constant division retains the original quotient/carry residues.
    // In normal calls the draw is 15-bit, but slot/frame aliases remain live.
    set(s, 5, signed_high_product(0x291ee171, reg(s, 3)));
    set(s, 4, arithmetic_shift(s, reg(s, 5), 13));
    set(s, 5, reg(s, 4) >> 31);
    set(s, 4, reg(s, 4) + reg(s, 5));
    set(s, 0, reg(s, 30) + 2 * (reg(s, 3) - reg(s, 4) * cm::rng::pool_samples));
    s.memory.write<4>(reg(s, 31), reg(s, 0));
    call(s, 0x1077b770, 0x10747534);

    set(s, 8, s.memory.read<4>(reg(s, 31)));
    set(s, 7, signed_high_product(0x80008001, reg(s, 3)));
    set(s, 4, reg(s, 30) + 0x20000);
    set(s, 6, 65535);
    set(s, 5, s.memory.read<4>(reg(s, 2) - 0x7b90));
    set(s, 0, reg(s, 4) - 0x7192); // Inclusive last uint16 sample.
    compare_unsigned(s, reg(s, 8), reg(s, 30));
    set(s, 4, reg(s, 7) + reg(s, 3));
    s.memory.write<4>(reg(s, 29), reg(s, 0));
    set(s, 4, arithmetic_shift(s, reg(s, 4), 15));
    set(s, 7, reg(s, 4) >> 31);
    set(s, 4, (reg(s, 4) + reg(s, 7)) * reg(s, 6));
    set(s, 3, reg(s, 3) - reg(s, 4));
    s.memory.write<4>(reg(s, 5), reg(s, 3));
    bool reset_cursor = reg(s, 8) < reg(s, 30);
    if (!reset_cursor) {
        compare_unsigned(s, reg(s, 8), reg(s, 0));
        reset_cursor = reg(s, 8) > reg(s, 0);
    }
    if (reset_cursor) s.memory.write<4>(reg(s, 31), reg(s, 30));
    set(s, 0, s.memory.read<4>(reg(s, 1) + 88));
    set(s, 1, reg(s, 1) + 80);
    s.wr<4>(4128, reg(s, 0));
    for (unsigned n = 31; n >= 28; --n)
        set(s, n, s.memory.read<4>(reg(s, 1) - (32 - n) * 4));
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
    const bool original = s.rng_original || s.rng_entry_original[index];
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
        else initialize_pool(s);
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
            << " mode=" << (s.rng_original || s.rng_entry_original[i] ? "original" : "modern")
            << " modern=" << s.rng_calls[i] << " original=" << s.rng_original_calls[i] << '\n';
    if (s.rng_trace) out << "rng.ordered-boundaries=" << std::hex << s.rng_ordered_digest << '\n';
    return out.str();
}
}
