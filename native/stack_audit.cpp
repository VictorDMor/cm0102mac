#include "stack_audit.hpp"
#include "aot_runtime.hpp"
#include <algorithm>
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
    std::uint32_t last_sp = 0;
    struct Site { std::uint64_t reads = 0; std::map<std::int32_t, std::uint64_t> offsets; };
    std::map<std::uint32_t, Site> sites;
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
        track();
        const auto low = std::max<std::uint64_t>(start, stack_low), high = std::min<std::uint64_t>(std::uint64_t(start) + size, stack_high);
        if (low < high) std::fill(stale.begin() + (low - stack_low), stale.begin() + (high - stack_low), 0);
    }
    void read(std::uint32_t start, const std::uint8_t*, unsigned size) override {
        track();
        if (start < stack_low || start >= stack_high || start < last_sp) return;
        const auto high = std::min<std::uint64_t>(std::uint64_t(start) + size, stack_high);
        for (auto address = std::uint64_t(start); address < high; ++address)
            if (stale[address - stack_low]) {
                auto& site = sites[s.pc];
                ++site.reads;
                ++site.offsets[std::int32_t(start - last_sp)];
                return;
            }
    }
};
namespace {
MemoryObserver* bulk_reader = nullptr;
const State* bulk_state = nullptr;
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
    out << "stale stack reads: " << ranked.size() << " sites\n";
    for (unsigned i = 0; i < ranked.size() && i < limit; ++i) {
        out << "  " << hex_address(ranked[i].first) << " reads " << ranked[i].second->reads << " at sp+";
        unsigned shown = 0;
        for (const auto& [offset, count] : ranked[i].second->offsets) {
            if (shown++ == 6) { out << " ..."; break; }
            out << ' ' << offset << "(x" << count << ')';
        }
        out << '\n';
    }
}
}
