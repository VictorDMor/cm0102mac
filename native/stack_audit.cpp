#include "stack_audit.hpp"
#include "aot_runtime.hpp"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <ostream>
#include <vector>

namespace cm::aot {
namespace {
constexpr std::uint32_t stack_low = 0x70000000, stack_high = 0x70100000;
}
struct StackAudit::Impl final : MemoryObserver {
    State& s;
    // 1 where the byte has been below the stack pointer since its last write.
    std::vector<std::uint8_t> stale = std::vector<std::uint8_t>(stack_high - stack_low, 0);
    // The PC that last wrote each byte (0: before the run).
    std::vector<std::uint32_t> writer = std::vector<std::uint32_t>(stack_high - stack_low, 0);
    std::uint32_t last_sp = 0;
    struct Site { std::uint64_t reads = 0; std::map<std::int32_t, std::uint64_t> offsets; std::map<std::uint32_t, std::uint64_t> writers; };
    std::map<std::uint32_t, Site> sites;
    std::uint64_t scramble = 0;   // xorshift state; 0: off
    bool scrambling = false;
    std::uint64_t scrambled = 0;
    // Stack below the starting SP holds whatever earlier calls left there.
    explicit Impl(State& state) : s(state), last_sp(std::uint32_t(state.r<4>(4))) {
        if (last_sp > stack_low) std::fill(stale.begin(), stale.begin() + (std::min(last_sp, stack_high) - stack_low), 1);
    }
    // A rising stack pointer frees the frames it leaves.
    void track() {
        const auto sp = std::uint32_t(s.r<4>(4));
        if (sp > last_sp) {
            const auto low = std::max(last_sp, stack_low), high = std::min(sp, stack_high);
            if (low < high) std::fill(stale.begin() + (low - stack_low), stale.begin() + (high - stack_low), 1);
        }
        last_sp = sp;
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t start, std::uint64_t size) override { touch(start, size); }
    void written(std::uint32_t start, const std::uint8_t*, std::size_t size) override { touch(start, size); }
    void touch(std::uint32_t start, std::uint64_t size) {
        if (scrambling) return;
        track();
        const auto low = std::max<std::uint64_t>(start, stack_low), high = std::min<std::uint64_t>(std::uint64_t(start) + size, stack_high);
        if (low < high) {
            std::fill(stale.begin() + (low - stack_low), stale.begin() + (high - stack_low), 0);
            std::fill(writer.begin() + (low - stack_low), writer.begin() + (high - stack_low), s.pc);
        }
    }
    void read(std::uint32_t start, const std::uint8_t*, unsigned size) override {
        track();
        if (start < stack_low || start >= stack_high || start < last_sp) return;
        const auto high = std::min<std::uint64_t>(std::uint64_t(start) + size, stack_high);
        // CM_STACK_SCRAMBLE_PCS=lo-hi limits scrambling to reads whose PC is in [lo, hi).
        static const std::pair<std::uint32_t, std::uint32_t> pcs = [] {
            const char* v = std::getenv("CM_STACK_SCRAMBLE_PCS");
            if (!v) return std::pair<std::uint32_t, std::uint32_t>{0, 0xffffffffu};
            char* end = nullptr; const auto lo = std::strtoul(v, &end, 16);
            return std::pair<std::uint32_t, std::uint32_t>{std::uint32_t(lo), std::uint32_t(std::strtoul(end + 1, nullptr, 16))};
        }();
        if (scramble && (s.pc < pcs.first || s.pc >= pcs.second)) {
            for (auto address = std::uint64_t(start); address < high; ++address) stale[address - stack_low] = 0;
            return;
        }
        if (scramble) {
            // Different leftovers: each stale byte the game reads becomes noise
            // (before the read takes it), and stays that noise until written.
            for (auto address = std::uint64_t(start); address < high; ++address)
                if (stale[address - stack_low]) {
                    scramble ^= scramble << 13; scramble ^= scramble >> 7; scramble ^= scramble << 17;
                    scrambling = true;
                    s.memory.copy_in(std::uint32_t(address), {std::uint8_t(scramble)});
                    scrambling = false;
                    stale[address - stack_low] = 0;
                    ++scrambled;
                }
            return;
        }
        for (auto address = std::uint64_t(start); address < high; ++address)
            if (stale[address - stack_low]) {
                auto& site = sites[s.pc];
                ++site.reads;
                ++site.offsets[std::int32_t(start - last_sp)];
                for (auto byte = address; byte < high; ++byte)
                    if (stale[byte - stack_low]) ++site.writers[writer[byte - stack_low]];
                return;
            }
    }
};
namespace {
MemoryObserver* bulk_reader = nullptr;
const State* bulk_state = nullptr;
}
StackAudit::StackAudit(State& s, std::uint64_t scramble_seed) : StackAudit(s) {
    impl->scramble = scramble_seed | 1;
}
StackAudit::StackAudit(State& s) : impl(std::make_unique<Impl>(s)) {
    bulk_reader = impl.get(); bulk_state = &s;
    block_move_reads = [](State& state, std::uint32_t address, std::uint32_t length) {
        if (bulk_reader && bulk_state == &state) bulk_reader->read(address, nullptr, length);
    };
}
StackAudit::~StackAudit() { block_move_reads = nullptr; bulk_reader = nullptr; bulk_state = nullptr; }
MemoryObserver* StackAudit::observer() { return impl.get(); }
void StackAudit::report(std::ostream& out, unsigned limit) const {
    std::vector<std::pair<std::uint32_t, const Impl::Site*>> ranked;
    for (const auto& [pc, site] : impl->sites) ranked.emplace_back(pc, &site);
    std::sort(ranked.begin(), ranked.end(), [](auto a, auto b) { return a.second->reads > b.second->reads; });
    if (impl->scramble) out << "scrambled " << impl->scrambled << " stale stack bytes as they were read\n";
    out << "stale stack reads: " << ranked.size() << " sites\n";
    for (unsigned i = 0; i < ranked.size() && i < limit; ++i) {
        out << "  " << hex_address(ranked[i].first) << " reads " << ranked[i].second->reads << " at sp+";
        unsigned shown = 0;
        for (const auto& [offset, count] : ranked[i].second->offsets) {
            if (shown++ == 6) { out << " ..."; break; }
            out << ' ' << offset << "(x" << count << ')';
        }
        out << " left by";
        shown = 0;
        for (const auto& [pc, count] : ranked[i].second->writers) {
            if (shown++ == 6) { out << " ..."; break; }
            out << ' ' << hex_address(pc) << "(x" << count << ')';
        }
        out << '\n';
    }
}
}
