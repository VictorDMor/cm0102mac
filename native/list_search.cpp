#include "list_search.hpp"
#include "aot_runtime.hpp"
#include "match_turbo.hpp"
#include <atomic>
#include <initializer_list>

namespace cm::aot {
namespace {
// Record layouts the search reads (offsets in bytes).
namespace container { constexpr std::uint32_t list = 0x8; }
namespace list { constexpr std::uint32_t count = 0x0, head = 0x4, cursor = 0xc; }  // count: int16
namespace node { constexpr std::uint32_t data = 0x0, next = 0x8; }
namespace element { constexpr std::uint32_t key = 0x8; }

struct Outcome {
    bool found = false;
    std::uint32_t iterations = 0;  // list_next calls made
    std::uint32_t list = 0, saved = 0, data = 0, node = 0;
    std::int16_t count = 0;
    std::int32_t last_key = 0, wanted = 0;
};
// Whether [address, address + size) overlaps [start, start + length).
bool overlaps(std::uint32_t address, unsigned size, std::uint32_t start, unsigned length) {
    return std::uint64_t(address) < std::uint64_t(start) + length && std::uint64_t(start) < std::uint64_t(address) + size;
}
// Every read the original makes, in order, without writing. The original
// first writes its frame (stmw r26-r31 at sp-0x18, LR at sp+8, back chain
// at sp-0x80), then moves the list cursor (+0xc) on every list_next. Those
// writes only matter if they overlap a later read (or the cursor overlaps
// the frame lmw restores from); such cases step aside.
std::optional<Outcome> trace(const State& s, std::uint32_t at, std::uint32_t key, std::uint32_t sp) {
    const auto written = [&](std::uint32_t address, unsigned size, std::uint32_t cursor) {
        return overlaps(address, size, sp - 0x80, 4) || overlaps(address, size, sp - 0x18, 0x18) ||
               overlaps(address, size, sp + 8, 4) || (cursor && overlaps(address, size, cursor, 4));
    };
    const auto word = [&](std::uint32_t address) { return std::uint32_t(s.memory.read<4>(address)); };
    Outcome o;
    if (written(at + container::list, 4, 0)) return std::nullopt;
    o.list = word(at + container::list);
    const auto cursor_field = o.list + list::cursor;
    if (written(cursor_field, 4, 0) || written(o.list + list::head, 4, 0) || written(o.list + list::count, 2, cursor_field) ||
        written(at + container::list, 4, cursor_field) || written(key, 4, cursor_field)) return std::nullopt;
    o.saved = word(cursor_field);
    auto cursor = word(o.list + list::head);
    o.count = std::int16_t(s.memory.read<2>(o.list + list::count));
    // Loop head: re-reads container->list and the count, unchanged by the loop.
    while (std::int32_t(o.iterations) < o.count) {
        // list_next: node = cursor; a null node takes the error report.
        if (!cursor) return std::nullopt;
        if (written(cursor + node::data, 4, cursor_field) || written(cursor + node::next, 4, cursor_field)) return std::nullopt;
        o.node = cursor;
        o.data = word(cursor + node::data);
        cursor = word(cursor + node::next);
        ++o.iterations;
        if (!o.data) return std::nullopt;  // a null element takes the error report
        if (written(o.data + element::key, 4, cursor_field)) return std::nullopt;
        o.last_key = std::int32_t(word(o.data + element::key));
        o.wanted = std::int32_t(word(key));
        if (o.last_key == o.wanted) { o.found = true; return o; }
    }
    return o;
}
std::atomic<std::uint64_t> completions{0};
void condition(State& s, std::int64_t a, std::int64_t b) {
    s.wr<1>(2304, (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
}
bool try_native_list_search(State& s) {
    // Read observers would miss the reads; write observers see the frame and
    // the final cursor write, so only one watching the cursor (whose
    // intermediate writes are skipped) needs the original.
    if (s.memory.date_observer || s.memory.read_trace || match_pc_profile_enabled()) return false;
    const auto r = [&](unsigned n) { return std::uint32_t(s.r<4>(n * 4)); };
    const auto set = [&](unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); };
    const auto at = r(3), key = r(4), sp = r(1), link = std::uint32_t(s.r<4>(4128));
    std::optional<Outcome> outcome;
    try {
        // The frame slots must be writable, as for the original's first stores.
        for (std::uint32_t slot : {sp - 0x18, sp - 0x14, sp - 0x10, sp - 0x0c, sp - 0x08, sp - 0x04, sp + 8, sp - 0x80})
            s.memory.read<4>(slot);
        // The two TOC loads of the prologue (used only by the error report).
        s.memory.read<4>(r(2) - 0x7ba8); s.memory.read<4>(r(2) - 0x5368);
        outcome = trace(s, at, key, sp);
    } catch (const std::runtime_error&) { return false; }
    if (!outcome) return false;
    const auto& o = *outcome;
    if (s.memory.observer && s.memory.observer->observes_write(o.list + list::cursor, 4)) return false;
    // 14 prologue, 4 per loop head, 20 per element passed over; found: 15 for
    // the last element, 4 to restore, 5 epilogue; not found: 2 + 5.
    const std::uint64_t charge = o.found ? 42 + 20 * std::uint64_t(o.iterations - 1)
                                         : 25 + 20 * std::uint64_t(o.iterations);
    if (s.executed > s.budget || s.budget - s.executed < charge) return false;

    for (unsigned n = 26; n <= 31; ++n) s.memory.write<4>(sp - (32 - n) * 4, r(n));
    s.memory.write<4>(sp + 8, link);
    s.memory.write<4>(sp - 0x80, sp);
    // Every list_next moved the cursor; both exits put the saved one back.
    s.memory.write<4>(o.list + list::cursor, o.saved);
    if (o.iterations) {
        set(4, o.data); set(5, o.node);
        s.wr<4>(2440, r(2));                   // each bl to list_next records r2
    }
    if (o.found) {
        condition(s, o.last_key, o.wanted);   // cmpw r3,r0 (equal)
        set(4, o.list);                        // lwz r4,8(r26)
        set(3, 1);
    } else {
        condition(s, std::int32_t(o.iterations), o.count);  // cmpw r28,r0 (not less)
        set(3, 0);
    }
    set(0, link);                              // lwz r0,0x88(r1); mtlr r0
    s.wr<4>(2432, sp);                         // lmw's P-code address register ends past r31
    s.executed += charge;
    s.pc = link;                               // blr
    completions.fetch_add(1, std::memory_order_relaxed);
    return true;
}
std::uint64_t native_list_search_completions() { return completions.load(std::memory_order_relaxed); }
}
