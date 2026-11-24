#include "guest_scratch.hpp"
#include "aot_runtime.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <stdexcept>

namespace cm::aot {
namespace {
// Slots are written on the game thread while no worker runs and read by all; `active` keeps the allocator's
// ordinary path to one load when there is no scratch heap at all.
std::array<std::atomic<State*>, GuestScratch::limit> owners{};
std::atomic<unsigned> active{0};
std::array<std::atomic<PlayContext*>, GuestScratch::limit> plays{};
// The pool context is the first thing in the range (52 bytes, zero: the allocator's own initial state).
constexpr std::uint32_t context_size = 64;
static_assert(context_size <= GuestScratch::toc_copy && GuestScratch::toc_copy + GuestScratch::toc_span <= GuestScratch::statics &&
              GuestScratch::statics < GuestScratch::heap && GuestScratch::heap < GuestScratch::size);
int slot_of(std::uint32_t address) {
    if (address < GuestScratch::start(0) || address >= GuestScratch::start(GuestScratch::limit)) return -1;
    return int((address - GuestScratch::start(0)) / GuestScratch::size);
}
int slot_of(const State& s) {
    for (unsigned n = 0; n < GuestScratch::limit; ++n) if (owners[n].load(std::memory_order_relaxed) == &s) return int(n);
    return -1;
}
}
void GuestScratch::enter(State& main, State& worker, unsigned index) {
    if (index >= limit) throw std::out_of_range("Guest scratch index");
    if (owners[index].load() || slot_of(worker) >= 0) throw std::logic_error("Guest scratch range or worker already in use");
    const auto begin = start(index);
    bool mapped = false;
    for (const auto& [address, bytes] : main.memory.mapped_ranges())
        mapped = mapped || (address <= begin && begin + std::uint64_t{size} <= address + bytes);
    if (!mapped) main.memory.map(begin, size);
    main.memory.clear(begin, size);   // also what an earlier worker left: a match must not read it
    worker.allocated.clear();
    worker.available.clear();
    worker.available.emplace(begin + heap, size - heap);
    worker.heap_end = 0x30000000;   // the growing heap's limit: these records never extend
    owners[index].store(&worker);
    ++active;
}
void GuestScratch::leave(State& worker) {
    const auto slot = slot_of(worker);
    if (slot < 0) return;
    plays[slot].store(nullptr);
    owners[slot].store(nullptr);
    --active;
}
std::uint32_t GuestScratch::in_use(const State& worker) {
    std::uint32_t bytes = 0;
    for (const auto& [address, size] : worker.allocated) bytes += (std::max(size, 1u) + 15u) & ~15u;
    return bytes;
}
std::uint32_t scratch_pool(const State& s, std::uint32_t block) {
    if (!active.load(std::memory_order_relaxed)) [[likely]] return 0;
    const auto mine = slot_of(s);
    if (!block) return mine < 0 ? 0 : GuestScratch::start(unsigned(mine));
    const auto slot = slot_of(block);
    if (mine >= 0 && slot != mine) throw std::logic_error("A guest worker released or resized a block that is not its own: " + hex_address(block));
    if (slot < 0) return 0;
    if (!owners[slot].load(std::memory_order_relaxed)) throw std::logic_error("A block of a scratch heap outlived it: " + hex_address(block));
    return GuestScratch::start(unsigned(slot));
}
State& heap_records(State& s, std::uint32_t address) {
    if (!active.load(std::memory_order_relaxed)) [[likely]] return s;
    const auto slot = slot_of(address);
    if (slot < 0) return s;
    auto* owner = owners[slot].load(std::memory_order_relaxed);
    if (!owner) throw std::logic_error("A block of a scratch heap outlived it: " + hex_address(address));
    return *owner;
}
std::uint32_t GuestScratch::range_of(const State& worker) {
    const auto slot = slot_of(worker);
    return slot < 0 ? 0 : start(unsigned(slot));
}
void GuestScratch::set_play(State& worker, PlayContext* play) {
    const auto slot = slot_of(worker);
    if (slot < 0) throw std::logic_error("A play phase needs a worker with a scratch range");
    plays[slot].store(play, std::memory_order_release);
}
std::uint32_t private_toc(const State& s, std::uint32_t loaded) {
    if (!active.load(std::memory_order_relaxed) || loaded != GuestScratch::game_toc) [[likely]] return loaded;
    const auto slot = slot_of(s);
    if (slot < 0 || !plays[slot].load(std::memory_order_relaxed)) return loaded;
    return GuestScratch::start(unsigned(slot)) + GuestScratch::toc_copy + 0x8000;
}
PlayContext* play_context(const State& s) {
    if (!active.load(std::memory_order_relaxed)) [[likely]] return nullptr;
    const auto slot = slot_of(s);
    return slot < 0 ? nullptr : plays[slot].load(std::memory_order_relaxed);
}
bool is_scratch(const State& s) { return active.load(std::memory_order_relaxed) && slot_of(s) >= 0; }
}
