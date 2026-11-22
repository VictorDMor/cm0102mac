// The font engine, block `tsimem` (TSIMEM.C, 0x10767460..0x10767ce0): cases for the allocator over the game's malloc. The
// objects and blocks are the snapshot's own: the font records' tsiMemObjects (found by their stamps) with the blocks their tables
// hold, so a case frees and allocates real heap blocks on both sides. Fields of an object or a block are then changed by the
// case's memory writes (undone after it) to take every branch: the scratch slots' flags, sizes and pointers, the table's count,
// capacity, cursor and slots, the stamps and trailer bytes of a block.
//
// The error sites. No state the game produces reaches an error site (the engine never runs out of memory, never corrupts a
// block), so each is provoked: out of memory by answering the game's malloc/realloc with null, a bad header or trailer, a null
// object, a full or overfull table, a slot that does not point back, by writing the field. The longjmp (0x10773f30) is
// answered on both sides (Case::answers), so what each side does is compared up to and after the call: the stamp written
// before it, the arguments it receives (call trace: the jump buffer and the code), the order of the calls and the state the code
// that follows the site leaves (in the game the call never returns; the answered run compares the C++ at each site as written).
// The real transfer, through the generated API entries' setjmp, is the end-to-end run's error sweep (font_engine_checks_e2e.cpp).
// Cases whose original runs on through a null pointer fault alike on both sides unless the checker runs with --null-page.
// Labels start with "tsimem:" (CM_FONT_ENGINE_BLOCK=tsimem).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t new_memory = 0x10767460, scratch = 0x107675c0, scratch_release = 0x10767690, delete_memory = 0x10767720,
                        free_all = 0x10767820, allocate = 0x107678a0, reallocate = 0x10767a00, release = 0x10767bb0;
constexpr std::uint32_t reporter = 0x107755c0, game_malloc = 0x10775130, game_free = 0x10775170, game_realloc = 0x107751b0, longjmp_entry = 0x10773f30;
constexpr std::uint32_t cell = scratch_base + 0x50000, fake_block = scratch_base + 0x50100;
// tsiMemObject and block fields (font_engine_detail.hpp has the named offsets; the checks keep their own numbers).
constexpr std::uint32_t o_count = 4, o_capacity = 8, o_table = 0xc, o_pointer = 0x128, o_size = 0x140, o_free = 0x158, o_cursor = 0x170, o_end_stamp = 0x178;
constexpr std::uint32_t block_stamp = 0xaa53c5aa;

struct Block { std::uint32_t user, size, slot; };
struct Object { std::uint32_t at, table, capacity, count; std::vector<Block> blocks; };

// The snapshot's tsiMemObjects: stamp 0xab1500ff first and 0xa5a55a5a at +0x178, a table of the blocks it handed out (each block's
// stamp, size, and the slot index it records, checked).
std::vector<Object> memory_objects(const Scenario& scenario) {
    auto& s = scenario.s;
    std::vector<Object> found;
    for (const auto& [low, size] : s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x17c <= std::uint64_t(low) + size; at += 4) {
            if (scenario.r32(std::uint32_t(at)) != 0xab1500ff || scenario.r32(std::uint32_t(at) + o_end_stamp) != 0xa5a55a5a) continue;
            Object object{std::uint32_t(at), scenario.r32(std::uint32_t(at) + o_table), scenario.r32(std::uint32_t(at) + o_capacity),
                          scenario.r32(std::uint32_t(at) + o_count), {}};
            if (object.capacity != 0x1f5 || object.table < 0x20000000) continue;
            for (std::uint32_t n = 0; n < object.capacity; ++n) {
                const auto header = scenario.r32(object.table + 4 * n);
                if (header && scenario.r32(header) == block_stamp && scenario.r32(header + 8) == n)
                    object.blocks.push_back({header + 0x10, scenario.r32(header + 4), n});
            }
            found.push_back(std::move(object));
        }
    }
    return found;
}
std::vector<Write> concat(std::vector<Write> a, const std::vector<Write>& b) { append(a, b); return a; }
// The scratch slot k of an object: pointer, size, free flag.
std::vector<Write> slot_of(const Object& m, std::uint32_t k, std::uint32_t pointer, std::uint32_t size, std::uint32_t flag) {
    const std::uint32_t at = m.at + 4 * k;  // k may be any slot number: the original's offset wraps the same way
    return {w32(at + o_pointer, pointer), w32(at + o_size, size), w32(at + o_free, flag)};
}
}

FONT_ENGINE_CASES("tsimem") {
    const auto objects = memory_objects(scenario);
    if (objects.empty()) return;
    // The two objects with the most blocks (the font records' own).
    std::vector<Object> some = objects;
    std::ranges::sort(some, [](const Object& a, const Object& b) { return a.blocks.size() > b.blocks.size(); });
    if (some.size() > 2) some.resize(2);
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> no_longjmp, answered = {{longjmp_entry, 0}, {reporter, 0}};
    const auto answer = [&](Case& item) { item.answers = answered; };
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, const std::string& label, bool answer_calls = false) -> Case& {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = "tsimem: " + label});
        // The game's malloc, free and realloc dispatch on both sides, so each call of the unit's is paired and its arguments compared.
        cases.back().original_owners = {game_malloc, game_free, game_realloc};
        if (answer_calls) answer(cases.back());
        return cases.back();
    };
    // The game's malloc and realloc answered null: the switch makes the owners dispatch, so the answers reach both sides.
    const auto fail_allocator = [&](Case& item, std::uint32_t entry) {
        item.answers.push_back({entry, 0});
    };
    std::mt19937 random(0x7517);

    // ---- tsi_NewMemhandler 0x10767460 ----
    add(new_memory, {{3, cell}}, {w32(cell, 0xdeadbeef)}, "new: success, the error cell cleared");
    add(new_memory, {{3, cell}}, {w32(cell, 0)}, "new: success, cell already zero");
    // The assertion for a null error cell, answered and real; then the original stores through the null pointer.
    add(new_memory, {{3, 0}}, {}, "new: null error cell, real reporter");
    add(new_memory, {{3, 0}}, {}, "new: null error cell, answered", true);
    {   // The object's malloc fails: the code is stored, the scratch fields are stored through null.
        auto& item = add(new_memory, {{3, cell}}, {w32(cell, 7)}, "new: first malloc fails");
        fail_allocator(item, game_malloc);
    }
    {   // The table's malloc fails: the object is freed, the code stored.
        auto& item = add(new_memory, {{3, cell}}, {w32(cell, 7)}, "new: table malloc fails");
        item.nth_answers.push_back({game_malloc, 2, 0});
    }

    // ---- tsi_AllocMem 0x107678a0 ----
    for (const auto& m : some) {
        for (const std::uint32_t size : {0u, 1u, 2u, 7u, 0x10u, 0x11u, 0x100u, 0x1234u})
            add(allocate, {{3, m.at}, {4, size}}, {}, "alloc: size " + hex(size) + " at " + hex(m.at));
        // The cursor at every kind of position: before a taken slot, on a free one, at the capacity, past it (the wrap).
        for (const std::uint32_t cursor : {0u, 1u, 5u, 0x100u, 0x1f4u, 0x1f5u, 0x1f6u, 0x400u, 0xffffffffu}) {
            add(allocate, {{3, m.at}, {4, 0x20}}, {w32(m.at + o_cursor, cursor)}, "alloc: cursor " + hex(cursor), cursor >= 0x1f5);
        }
        // The first blocks' slots taken: the cursor scans past them and counts them.
        for (const auto& block : m.blocks) {
            if (block.slot > 3) break;
            add(allocate, {{3, m.at}, {4, 0x30}}, {w32(m.at + o_cursor, block.slot)}, "alloc: cursor on taken slot " + hex(block.slot));
        }
        // Every slot taken (the count stays below the capacity): no free slot (0x2721); the counts that fail first (0x271c).
        std::vector<Write> full;
        for (std::uint32_t n = 0; n < m.capacity; ++n) full.push_back(w32(m.table + 4 * n, 0x20000000 + 4 * n));
        add(allocate, {{3, m.at}, {4, 0x20}}, full, "alloc: no free slot", true);
        add(allocate, {{3, m.at}, {4, 0x20}}, concat(full, {w32(m.at + o_cursor, 0x100)}), "alloc: no free slot from the middle", true);
        for (const std::uint32_t count : {0x1f4u, 0x1f5u, 0x1f6u, 0x7fffffffu, 0x80000000u, 0xffffffffu})
            add(allocate, {{3, m.at}, {4, 0x20}}, {w32(m.at + o_count, count)}, "alloc: count " + hex(count), true);
        // Capacity at its edges: zero, negative, one (the loop's only slot), small.
        for (const std::uint32_t capacity : {0u, 1u, 2u, 3u, 0x80000000u, 0xffffffffu})
            add(allocate, {{3, m.at}, {4, 0x20}}, {w32(m.at + o_capacity, capacity), w32(m.at + o_count, 0)}, "alloc: capacity " + hex(capacity), true);
        {   auto& item = add(allocate, {{3, m.at}, {4, 0x20}}, {}, "alloc: malloc fails", true);
            fail_allocator(item, game_malloc); }
    }
    add(allocate, {{3, 0}, {4, 0x20}}, {}, "alloc: null object", true);
    add(allocate, {{3, some[0].at}, {4, 0xffffffffu}}, {}, "alloc: size wraps (size + 0x12)", true);
    add(allocate, {{3, some[0].at}, {4, 0xffffffeeu}}, {}, "alloc: size + 0x12 is zero", true);

    // ---- tsi_DeAllocMem 0x10767bb0 ----
    for (const auto& m : some) {
        for (std::size_t n = 0; n < m.blocks.size() && n < 12; ++n) {
            const auto& block = m.blocks[n * 5 % m.blocks.size()];
            add(release, {{3, m.at}, {4, block.user}}, {}, "release: block " + hex(block.user) + " size " + hex(block.size));
        }
        const auto& b = m.blocks.front();
        const auto header = b.user - 0x10;
        add(release, {{3, m.at}, {4, 0}}, {}, "release: null pointer");
        add(release, {{3, 0}, {4, b.user}}, {}, "release: null object", true);
        add(release, {{3, 0}, {4, 0}}, {}, "release: null object and pointer", true);
        // The header stamp: every byte lane wrong.
        for (unsigned lane = 0; lane < 4; ++lane)
            add(release, {{3, m.at}, {4, b.user}}, {w8(header + lane, scenario.u8(header + lane) ^ 0x01)}, "release: header stamp lane " + std::to_string(lane), true);
        // The trailer's two bytes, and a size that points elsewhere.
        add(release, {{3, m.at}, {4, b.user}}, {w8(b.user + b.size, 0x5b)}, "release: first trailer byte", true);
        add(release, {{3, m.at}, {4, b.user}}, {w8(b.user + b.size + 1, 0xf1)}, "release: second trailer byte", true);
        add(release, {{3, m.at}, {4, b.user}}, {w8(b.user + b.size, 0), w8(b.user + b.size + 1, 0)}, "release: both trailer bytes", true);
        // The table slot: another pointer, null, a slot index past the table, the count at zero (it goes to -1).
        add(release, {{3, m.at}, {4, b.user}}, {w32(m.table + 4 * b.slot, header + 4)}, "release: slot points elsewhere", true);
        add(release, {{3, m.at}, {4, b.user}}, {w32(m.table + 4 * b.slot, 0)}, "release: slot empty", true);
        add(release, {{3, m.at}, {4, b.user}}, {w32(header + 8, 0x1f5)}, "release: index at the capacity", true);
        add(release, {{3, m.at}, {4, b.user}}, {w32(header + 8, b.slot + 1)}, "release: index of the neighbour", true);
        add(release, {{3, m.at}, {4, b.user}}, {w32(m.at + o_count, 0)}, "release: count zero");
        add(release, {{3, m.at}, {4, b.user}}, {w32(m.at + o_count, 0x80000000u)}, "release: count minimum");
        // A user pointer that is no block: the stamp read before it is wrong (the answered error then runs on through the original's reads).
        add(release, {{3, m.at}, {4, m.at + 0x20}}, {}, "release: pointer inside the object", true);
    }

    // ---- tsi_ReAllocMem 0x10767a00 (not run by the game: cases only) ----
    for (const auto& m : some) {
        const auto& b = m.blocks.front();
        const auto header = b.user - 0x10;
        for (const std::uint32_t size : {0u, 1u, b.size, b.size + 1, b.size / 2, 0x200u, 0x2000u})
            add(reallocate, {{3, m.at}, {4, b.user}, {5, size}}, {}, "realloc: block " + hex(b.user) + " size " + hex(b.size) + " to " + hex(size));
        add(reallocate, {{3, m.at}, {4, 0}, {5, 0x20}}, {}, "realloc: null pointer");
        add(reallocate, {{3, 0}, {4, 0}, {5, 0x20}}, {}, "realloc: null pointer and object");
        add(reallocate, {{3, 0}, {4, b.user}, {5, 0x20}}, {}, "realloc: null object", true);
        for (unsigned lane = 0; lane < 4; ++lane)
            add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w8(header + lane, scenario.u8(header + lane) ^ 0x10)}, "realloc: header stamp lane " + std::to_string(lane), true);
        add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w8(b.user + b.size, 0x5b)}, "realloc: first trailer byte", true);
        add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w8(b.user + b.size + 1, 0xf1)}, "realloc: second trailer byte", true);
        // The count at 0, negative, the capacity, one more.
        for (const std::uint32_t count : {0u, 0x80000000u, 0xffffffffu, 0x1f5u, 0x1f6u, 1u})
            add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w32(m.at + o_count, count)}, "realloc: count " + hex(count), count == 0 || count > 0x1f5);
        // The table's slot does not point back: the assertion (real and answered), and the realloc goes on.
        add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w32(m.table + 4 * b.slot, header + 4)}, "realloc: slot points elsewhere, real reporter");
        add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w32(m.table + 4 * b.slot, header + 4)}, "realloc: slot points elsewhere", true);
        {   auto& item = add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {}, "realloc: the game's realloc returns null", true);
            fail_allocator(item, game_realloc); }
        // The new block with a bad stamp (the realloc answered with a block of the scratch area), and with a good one.
        {   auto& item = add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w32(fake_block, 0x12345678)}, "realloc: new block's stamp is wrong", true);
            item.answers.push_back({game_realloc, fake_block}); }
        {   auto& item = add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w32(fake_block, block_stamp)}, "realloc: new block answered with a good stamp", true);
            item.answers.push_back({game_realloc, fake_block}); }
        // The old slot's index at or past the capacity: the final check (0x271f). The word after the table must hold the header to pass the slot check.
        add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w32(header + 8, 0x1f5), w32(m.table + 4 * 0x1f5, header)}, "realloc: index at the capacity", true);
        add(reallocate, {{3, m.at}, {4, b.user}, {5, 0x40}}, {w32(header + 8, 0x1f5)}, "realloc: index at the capacity, slot mismatch", true);
    }

    // ---- tsi_GetScratch 0x107675c0 ----
    for (const auto& m : some) {
        const auto& live = m.blocks[1];
        for (const unsigned slot : {0u, 2u, 5u}) {
            const auto label = "scratch: slot " + std::to_string(slot) + " ";
            // A fresh slot (free, no pointer): size 0 is served by the empty buffer, anything larger by a new block.
            for (const std::uint32_t size : {0u, 1u, 0x40u, 0x1000u})
                add(scratch, {{3, m.at}, {4, size}, {5, slot}}, slot_of(m, slot, 0, 0, 1), label + "fresh, size " + hex(size));
            // A slot holding a live block: the request fits (equal, smaller) or outgrows it (the old block released, a new one allocated).
            for (const std::uint32_t size : {0u, live.size - 1, live.size, live.size + 1, live.size + 0x200})
                add(scratch, {{3, m.at}, {4, size}, {5, slot}}, slot_of(m, slot, live.user, live.size, 1), label + "holds " + hex(live.size) + ", size " + hex(size));
            // A slot already taken: a fresh block each time (the pointer and size recorded are left alone).
            for (const std::uint32_t size : {0u, 0x40u})
                add(scratch, {{3, m.at}, {4, size}, {5, slot}}, slot_of(m, slot, live.user, live.size, 0), label + "taken, size " + hex(size));
            // Any word but 1 is "free" (the test is not zero): 2 and the sign bit.
            add(scratch, {{3, m.at}, {4, live.size + 1}, {5, slot}}, slot_of(m, slot, live.user, live.size, 0x80000000u), label + "flag 0x80000000");
            add(scratch, {{3, m.at}, {4, 0x40}, {5, slot}}, slot_of(m, slot, live.user, live.size, 2), label + "flag 2");
            // The released block is bad (the release's error is answered), or the allocation then fails.
            add(scratch, {{3, m.at}, {4, live.size + 1}, {5, slot}}, concat(slot_of(m, slot, live.user, live.size, 1), {w8(live.user + live.size, 0)}), label + "release raises", true);
            add(scratch, {{3, m.at}, {4, live.size + 1}, {5, slot}}, concat(slot_of(m, slot, live.user, live.size, 1), {w32(m.at + o_count, 0x1f5)}), label + "allocation raises", true);
            {   auto& item = add(scratch, {{3, m.at}, {4, 0x40}, {5, slot}}, slot_of(m, slot, 0, 0, 1), label + "malloc fails", true);
                fail_allocator(item, game_malloc); }
        }
        // Slots outside 0..5: the assertion (real, then answered) and the fields read at the offsets it computes.
        for (const std::uint32_t slot : {6u, 7u, 8u, 0xffffffffu, 0x80000000u, 0x40000000u, 0x7fffffffu}) {
            add(scratch, {{3, m.at}, {4, 0x20}, {5, slot}}, slot_of(m, slot, live.user, live.size, 1), "scratch: slot " + hex(slot) + ", slot fields set", true);
            add(scratch, {{3, m.at}, {4, live.size + 4}, {5, slot}}, slot_of(m, slot, live.user, live.size, 1), "scratch: slot " + hex(slot) + ", slot fields set, grows", true);
        }
        add(scratch, {{3, m.at}, {4, 0x20}, {5, 6}}, {}, "scratch: slot 6, real reporter");
    }

    // ---- scratch release 0x10767690 and tsi_DeleteMemhandler 0x10767720 ----
    for (const auto& m : some) {
        // Six slots holding live blocks, all marked taken (flag 0), as a font's use leaves them.
        std::vector<Write> taken, mixed, empty;
        for (unsigned k = 0; k < 6; ++k) {
            const auto& block = m.blocks[(k * 3 + 1) % m.blocks.size()];
            append(taken, slot_of(m, k, block.user, block.size, 0));
            append(mixed, slot_of(m, k, k % 2 ? block.user : 0, k % 2 ? block.size : 0, k % 3 == 0 ? 1 : 0));
            append(empty, slot_of(m, k, 0, 0, 0));
        }
        add(scratch_release, {{3, m.at}}, taken, "scratch release: six held blocks");
        add(scratch_release, {{3, m.at}}, empty, "scratch release: six empty taken slots");
        add(scratch_release, {{3, m.at}}, mixed, "scratch release: mixed, some free (assertion)");
        add(scratch_release, {{3, m.at}}, mixed, "scratch release: mixed, answered", true);
        add(scratch_release, {{3, m.at}}, {}, "scratch release: the object as it is");
        for (unsigned k = 0; k < 6; ++k)
            add(scratch_release, {{3, m.at}}, concat(taken, {w32(m.at + o_free + 4 * k, 1)}), "scratch release: slot " + std::to_string(k) + " free, real reporter");
        add(scratch_release, {{3, m.at}}, concat(taken, {w8(m.blocks[1].user + m.blocks[1].size, 0)}), "scratch release: a block raises", true);

        // Delete: stamps, the scratch slots, the count (a table with blocks left asserts), the frees.
        std::vector<Write> drained = {w32(m.at + o_count, 0)};
        add(delete_memory, {{3, m.at}}, concat(empty, drained), "delete: drained object, empty slots");
        add(delete_memory, {{3, m.at}}, concat(taken, drained), "delete: drained object, six held blocks");
        add(delete_memory, {{3, m.at}}, empty, "delete: blocks left (assertion, real)");
        add(delete_memory, {{3, m.at}}, concat(empty, {w32(m.at + o_count, 0x80000000u)}), "delete: negative count", true);
        add(delete_memory, {{3, m.at}}, concat(empty, concat(drained, {w32(m.at, 0)})), "delete: first stamp wrong, real reporter");
        add(delete_memory, {{3, m.at}}, concat(empty, concat(drained, {w32(m.at, 0)})), "delete: first stamp wrong", true);
        add(delete_memory, {{3, m.at}}, concat(empty, concat(drained, {w32(m.at + o_end_stamp, 0)})), "delete: second stamp wrong", true);
        add(delete_memory, {{3, m.at}}, concat(empty, concat(drained, {w32(m.at + o_end_stamp, 0xa5a55a5b)})), "delete: second stamp off by one", true);
        add(delete_memory, {{3, m.at}}, concat(mixed, drained), "delete: free slots (assertion)", true);
    }
    add(delete_memory, {{3, 0}}, {}, "delete: null object");

    // ---- 0x10767820: the unwinding's frees ----
    for (const auto& m : some) {
        add(free_all, {{3, m.at}}, {}, "free blocks: the object as it is");
        std::vector<Write> sparse;
        for (std::uint32_t n = 0; n < m.capacity; n += 2) sparse.push_back(w32(m.table + 4 * n, 0));
        add(free_all, {{3, m.at}}, sparse, "free blocks: every other slot empty");
        std::vector<Write> none;
        for (std::uint32_t n = 0; n < m.capacity; ++n) none.push_back(w32(m.table + 4 * n, 0));
        add(free_all, {{3, m.at}}, none, "free blocks: empty table");
        for (const std::uint32_t capacity : {0u, 1u, 5u, 0x80000000u, 0xffffffffu})
            add(free_all, {{3, m.at}}, concat(none, {w32(m.at + o_capacity, capacity)}), "free blocks: capacity " + hex(capacity));
    }
    add(free_all, {{3, 0}}, {}, "free blocks: null object");
    (void)no_longjmp;
    (void)random;
}
}
