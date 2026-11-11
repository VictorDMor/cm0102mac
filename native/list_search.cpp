#include "list_search.hpp"
#include "list_search_index.hpp"
#include "aot_runtime.hpp"
#include "match_turbo.hpp"
#include "routine_check.hpp"
#include <cstdlib>
#include <cstdio>
#include <algorithm>
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
    bool operator==(const Outcome&) const = default;
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
std::atomic<std::uint64_t> completions{0}, index_answers{0};
// CM_LIST_SEARCH_VERIFY=1: every index answer is checked against the walk (trace), and a difference aborts.
const bool verify_index = [] { const char* v = std::getenv("CM_LIST_SEARCH_VERIFY"); return v && *v == '1'; }();
// CM_LIST_SEARCH_INDEX=0: the walk only (for paired timing).
const bool index_allowed = [] { const char* v = std::getenv("CM_LIST_SEARCH_INDEX"); return !(v && *v == '0'); }();
std::atomic<bool> index_enabled{false};

// Builds the index from the list as the walk reads it; `usable` stays false where the walk would step aside for
// a reason that does not depend on the call (a null node or element before the count, or the cursor field
// overlapping something the walk reads).
void build(const State& s, ListSearchIndex& index, std::uint32_t at, std::uint32_t list_at, std::uint32_t head,
           std::int16_t count) {
    const auto word = [&](std::uint32_t address) { return std::uint32_t(s.memory.read<4>(address)); };
    index.built = true;
    index.usable = false;
    index.built_at = index.mutations;
    index.container = at;
    index.list = list_at;
    index.head = head;
    index.count = count;
    index.first.clear();
    index.last_node = index.last_data = 0;
    const auto cursor_field = list_at + list::cursor;
    std::uint64_t low = std::min<std::uint64_t>(std::uint64_t(at) + container::list, list_at);
    std::uint64_t high = std::max<std::uint64_t>(std::uint64_t(at) + container::list + 4, std::uint64_t(list_at) + 0x10);
    const auto read = [&](std::uint32_t address, unsigned size) {
        low = std::min<std::uint64_t>(low, address);
        high = std::max<std::uint64_t>(high, std::uint64_t(address) + size);
        return !overlaps(address, size, cursor_field, 4);
    };
    if (overlaps(at + container::list, 4, cursor_field, 4)) return;
    auto node = head;
    for (std::int32_t position = 0; position < count; ++position) {
        if (!node || !read(node + node::data, 4) || !read(node + node::next, 4)) return;
        const auto data = word(node + node::data);
        const auto next = word(node + node::next);
        if (!data || !read(data + element::key, 4)) return;
        index.first.try_emplace(std::int32_t(word(data + element::key)),
                                ListSearchIndex::Hit{std::uint32_t(position + 1), node, data});
        index.last_node = node;
        index.last_data = data;
        node = next;
    }
    if (high > (UINT64_C(1) << 32)) return;
    index.low = low;
    index.high = high;
    index.usable = true;
}
// The walk's outcome from the index, or nothing where the call itself rules it out: the frame the original
// writes, or the cursor it moves, would alias something the walk reads.
std::optional<Outcome> lookup(State& s, std::uint32_t at, std::uint32_t key, std::uint32_t sp) {
    if (!index_enabled.load(std::memory_order_relaxed) || !index_allowed || routine_checker) return std::nullopt;
    const auto word = [&](std::uint32_t address) { return std::uint32_t(s.memory.read<4>(address)); };
    if (sp < 0x80 || std::uint64_t(sp) + 12 > (UINT64_C(1) << 32)) return std::nullopt;
    const std::uint64_t frame_low = sp - 0x80, frame_high = std::uint64_t(sp) + 12;
    if (overlaps(at + container::list, 4, std::uint32_t(frame_low), 0x8c)) return std::nullopt;
    const auto list_at = word(at + container::list);
    const auto count = std::int16_t(s.memory.read<2>(list_at + list::count));
    const auto head = word(list_at + list::head);
    auto& index = list_search_index(s);
    if (!index.built || index.built_at != index.mutations || index.container != at || index.list != list_at ||
        index.head != head || index.count != count)
        build(s, index, at, list_at, head, count);
    if (!index.usable) return std::nullopt;
    if (index.low < frame_high && frame_low < index.high) return std::nullopt;
    if (overlaps(key, 4, std::uint32_t(frame_low), 0x8c) || overlaps(key, 4, list_at + list::cursor, 4)) return std::nullopt;
    Outcome o;
    o.list = list_at;
    o.saved = word(list_at + list::cursor);
    o.count = count;
    if (count <= 0) return o;  // the walk reads neither the key nor a node
    o.wanted = std::int32_t(word(key));
    if (const auto hit = index.first.find(o.wanted); hit != index.first.end()) {
        o.found = true;
        o.iterations = hit->second.position;
        o.node = hit->second.node;
        o.data = hit->second.data;
        o.last_key = o.wanted;
    } else {
        o.iterations = std::uint32_t(count);
        o.node = index.last_node;
        o.data = index.last_data;
        o.last_key = std::int32_t(word(o.data + element::key));
    }
    return o;
}
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
        outcome = lookup(s, at, key, sp);
        if (outcome) {
            index_answers.fetch_add(1, std::memory_order_relaxed);
            if (verify_index) {
                const auto walked = trace(s, at, key, sp);
                if (!walked || *walked != *outcome) {
                    std::fprintf(stderr, "list search index differs from the walk: container %08x key %08x\n", at, key);
                    std::abort();
                }
            }
        } else {
            outcome = trace(s, at, key, sp);
        }
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
std::uint64_t native_list_search_index_answers() { return index_answers.load(std::memory_order_relaxed); }
void set_native_list_search_index(bool enabled) { index_enabled.store(enabled, std::memory_order_relaxed); }
}
