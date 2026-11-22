// The font engine, block `tsimem` (TSIMEM.C, 0x10767460..0x10767ce0): the allocator the engine keeps over the game's malloc. A
// tsiMemObject (font_engine_detail.hpp, namespace tsimem) holds a table of the blocks it handed out, six reusable scratch
// buffers, a jump buffer the API entries fill with setjmp, and stamps. Every block is malloc(size + 0x12): a header (stamp, size,
// table index), the user bytes at +0x10 and the two trailer bytes 0x5a, 0xf0 after them.
//
// Error sites. The original stores the error stamp 0x5500aaff at mem + 0x174 and calls longjmp(mem + 0x10, code) (0x10773f30)
// from the API entry's setjmp: `raise` does the same call at the same stack pointer, so the generated longjmp owner finds the
// transfer's destination in the dispatch scope of the setjmp that filled the buffer, and the exception it throws unwinds this
// C++ without running any epilogue. In the game the call never returns. Every site is written as the original's: the code
// that follows it is the original's, which runs only where a checker answers the longjmp (Case::answers) so that the stamp, the
// arguments and what follows can be compared; production never reaches it.
//
// Registers. Callers read r3 and pass r9/r10 through; the checker compares r4-r10 at return, so each routine writes the
// volatile registers where the original's instructions do (r4-r7 below) and leaves the rest to its callees. The non-volatile
// registers the original holds at a call are set before it (a callee's prologue saves them into the stack image, and the stack
// contract of 0x10767bb0 is made of exactly those saves), and Frame restores them.
#include "font_engine_detail.hpp"

namespace cm::aot::font_engine {
namespace {
using namespace tsimem;

// The pool's address (the assertion texts and the file name live in it).
std::uint32_t pool_of(Cpu& c) { return c.toc(toc::tsimem_pool); }

// `bl 0x107755c0` with the text at pool + `text`, the file name and the line (the reporter returns: the engine goes on).
void assertion(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back) {
    c.set({{3, pool + text}, {4, pool + name_offset}, {5, line}});
    c.call(assertion_report, back);
}

// An error site (see the head of the file).
void raise(Cpu& c, std::uint32_t mem, std::uint32_t code, std::uint32_t back) {
    c.set({{3, mem + jump_buffer}, {4, code}});
    c.put32(mem + error_stamp, error_raised);
    c.call(longjmp_entry, back);
}

// The two trailer bytes after a block's user bytes.
void put_trailer(Cpu& c, std::uint32_t block, std::uint32_t size) {
    c.put8(block + size + header_size, trailer_first);
    c.put8(block + size + header_size + 1, trailer_second);
}

// Direct calls between the unit's routines (`bl` to a C++ routine): the registers the callee reads, the link register.
void call_release_block(Cpu& c, std::uint32_t mem, std::uint32_t pointer, std::uint32_t back) {
    c.set({{3, mem}, {4, pointer}});
    c.link(back);
    release_block(c, mem, pointer);
}
std::uint32_t call_allocate_block(Cpu& c, std::uint32_t mem, std::uint32_t size, std::uint32_t back) {
    c.set({{3, mem}, {4, size}});
    c.link(back);
    return allocate_block(c, mem, size);
}
}

// 0x10767460: tsi_NewMemhandler(error). The object, then its table zero-filled. A failed malloc stores the code in *error and
// returns null, but the original then stores its scratch fields through the null pointer (as it runs on, so does this).
std::uint32_t new_memory_handler(Cpu& c, std::uint32_t error_out) {
    guest::Frame frame(c, 0x10767460, 0x40, {.first_gpr = 30});
    const auto pool = pool_of(c);
    c.set(30, error_out);
    if (error_out == 0) {
        c.set({{3, pool}, {4, pool + name_offset}, {5, 0x2c}});
        c.call(assertion_report, 0x1076748c);
    }
    c.put32(error_out, 0);
    c.set(3, object_size);
    c.call(game_malloc, 0x1076749c);
    auto mem = c.r(3);
    c.set(31, mem);
    if (mem == 0) {
        c.put32(error_out, error_out_of_memory);
    } else {
        c.put32(mem + stamp, object_stamp);
        c.put32(mem + error_stamp, error_stamp_initial);
        c.put32(mem + end_stamp, end_stamp_value);
        c.put32(mem + count, 0);
        c.put32(mem + capacity, 0x111);
        c.put32(mem + capacity, initial_capacity);
        c.put32(mem + cursor, 0);
        c.set({{4, 0}, {5, end_stamp_value}});
        c.set(3, rlwinm(c.u32(mem + capacity), 2, 0, 29));
        c.call(game_malloc, 0x107674f0);
        c.put32(mem + table, c.r(3));
        if (c.u32(mem + table) == 0) {
            c.set(3, mem);
            c.call(game_free, 0x1076753c);
            c.put32(error_out, error_out_of_memory);
            mem = 0;
            c.set(31, 0);
        } else {
            // The table's words are cleared one at a time: r5 is the byte offset, r6 the index the loop leaves.
            std::uint32_t offset = 0, index = 0;
            c.set({{5, 0}, {6, 0}, {4, 0}});
            while (std::int32_t(index) < c.i32(mem + capacity)) {
                c.put32(c.u32(mem + table) + offset, 0);
                ++index;
                offset += 4;
            }
            c.set({{5, offset}, {6, index}});
        }
    }
    // The scratch buffers: no pointer, no size, free.
    c.set(4, 0);
    for (std::uint32_t slot = 0; slot < scratch_slots; ++slot) {
        c.put32(mem + scratch_pointer + 4 * slot, 0);
        c.put32(mem + scratch_size + 4 * slot, 0);
        c.put32(mem + scratch_free + 4 * slot, 1);
    }
    c.set(3, mem);
    return mem;
}

// 0x107675c0: the scratch buffer of a slot (0..5), grown if the request is larger than the one it holds. A slot marked free
// (word 1) is taken (cleared) and kept; a slot already taken gets a fresh block (the buffer is then the caller's to release).
std::uint32_t scratch_allocate(Cpu& c, std::uint32_t mem, std::uint32_t size, std::int32_t slot) {
    guest::Frame frame(c, 0x107675c0, 0x50, {.first_gpr = 28});
    const auto pool = pool_of(c);
    c.set(5, pool);
    const auto offset = rlwinm(std::uint32_t(slot), 2, 0, 29);
    const auto entry = mem + offset;
    const auto hold = [&] { c.set({{28, mem}, {29, size}, {30, offset}, {31, entry}}); };
    if (slot >= 6) {
        // r30 still holds the slot itself and r31 the caller's value.
        c.set({{28, mem}, {29, size}, {30, std::uint32_t(slot)}});
        assertion(c, pool, 0x19, 0x55, 0x10767604);
    }
    std::uint32_t result;
    if (c.u32(entry + scratch_free) != 0) {
        c.put32(entry + scratch_free, 0);
        if (size > c.u32(entry + scratch_size)) {
            const auto old = c.u32(entry + scratch_pointer);
            hold();
            call_release_block(c, mem, old, 0x10767638);
            hold();
            const auto fresh = call_allocate_block(c, mem, size, 0x10767644);
            c.put32(entry + scratch_pointer, fresh);
            c.put32(entry + scratch_size, size);
        }
        result = c.u32(entry + scratch_pointer);
        c.set(3, result);
    } else {
        hold();
        result = call_allocate_block(c, mem, size, 0x10767664);
    }
    return result;
}

// 0x10767690: the six scratch buffers given back (each slot must be marked taken: the assertion says so otherwise), and the slots
// reset to no pointer, no size, free.
void scratch_release_all(Cpu& c, std::uint32_t mem) {
    guest::Frame frame(c, 0x10767690, 0x60, {.first_gpr = 24});
    const auto pool = pool_of(c);
    c.set({{27, 0}, {28, pool}, {24, mem}, {25, 0}, {29, 0}, {30, 0}, {31, 1}});
    std::uint32_t offset = 0;
    for (std::uint32_t slot = 0; slot < scratch_slots; ++slot, offset += 4) {
        const auto entry = mem + offset;
        c.set({{26, entry}, {25, slot}, {27, offset}});
        if (c.u32(entry + scratch_free) == 0) assertion(c, pool, 0x31, 0x68, 0x107676e0);
        const auto pointer = c.u32(entry + scratch_pointer);
        c.set({{26, entry}, {25, slot}, {27, offset}});
        call_release_block(c, mem, pointer, 0x107676ec);
        c.put32(entry + scratch_pointer, 0);
        c.put32(entry + scratch_size, 0);
        c.put32(entry + scratch_free, 1);
    }
}

// 0x10767720: tsi_DeleteMemhandler(mem): both stamps checked (assertion), the scratch buffers released, the table's blocks must
// all have been released (assertion if the count is not zero), the table and the object freed.
void delete_memory_handler(Cpu& c, std::uint32_t mem) {
    guest::Frame frame(c, 0x10767720, 0x60, {.first_gpr = 24});
    const auto pool = pool_of(c);
    c.set({{24, mem}, {28, pool}, {4, 0}});
    bool valid = false;
    if (c.u32(mem + stamp) == object_stamp && c.u32(mem + end_stamp) == end_stamp_value) valid = true;
    if (valid) c.set(4, 1);
    if (!valid) assertion(c, pool, 0x41, 0x74, 0x10767778);
    c.set({{27, 0}, {31, 1}, {26, 0}, {29, 0}, {30, 0}});
    std::uint32_t offset = 0;
    for (std::uint32_t slot = 0; slot < scratch_slots; ++slot, offset += 4) {
        const auto entry = mem + offset;
        c.set({{25, entry}, {26, offset}, {27, slot}});
        if (c.u32(entry + scratch_free) == 0) assertion(c, pool, 0x31, 0x68, 0x107677ac);
        const auto pointer = c.u32(entry + scratch_pointer);
        c.set({{25, entry}, {26, offset}, {27, slot}});
        call_release_block(c, mem, pointer, 0x107677b8);
        c.put32(entry + scratch_pointer, 0);
        c.put32(entry + scratch_size, 0);
        c.put32(entry + scratch_free, 1);
    }
    // r25 holds the last slot's address, r26 the byte offset past it, r27 the count.
    const std::initializer_list<std::pair<unsigned, std::uint32_t>> held{{25, mem + offset - 4}, {26, offset}, {27, scratch_slots}, {28, pool}, {29, 0}, {30, 0}, {31, 1}};
    if (c.u32(mem + count) != 0) {
        c.set(held);
        assertion(c, pool, 0x6c, 0x76, 0x107677f0);
    }
    c.set(held);
    c.set(3, c.u32(mem + table));
    c.call(game_free, 0x107677f8);
    c.set(held);
    c.set(3, mem);
    c.call(game_free, 0x10767800);
}

// 0x10767820: every block of the table, the table and the object returned to the game's allocator (the unwinding of an error
// frees what the table still holds). A null object is ignored.
void free_blocks(Cpu& c, std::uint32_t mem) {
    guest::Frame frame(c, 0x10767820, 0x50, {.first_gpr = 27});
    c.set(27, mem);
    if (mem == 0) return;
    const auto blocks = c.u32(mem + table), capacity_now = c.u32(mem + capacity);
    c.set({{28, blocks}, {30, 0}, {29, capacity_now}, {31, blocks}});
    std::uint32_t index = 0, at = blocks;
    for (; std::int32_t(index) < std::int32_t(capacity_now); ++index, at += 4) {
        const auto block = c.u32(at);
        c.set(3, block);
        if (block != 0) {
            c.set({{30, index}, {31, at}});
            c.call(game_free, 0x10767860);
        }
    }
    c.set({{30, index}, {31, at}});
    c.set(3, blocks);
    c.call(game_free, 0x10767878);
    c.set(3, mem);
    c.call(game_free, 0x10767880);
}

// 0x107678a0: tsi_AllocMem(mem, size): a block of size + 0x12 bytes with header and trailer, recorded in the first free slot of
// the table from the cursor on (the cursor stays at the slot taken), and the user pointer (block + 0x10) returned.
std::uint32_t allocate_block(Cpu& c, std::uint32_t mem, std::uint32_t size) {
    guest::Frame frame(c, 0x107678a0, 0x50, {.first_gpr = 29});
    c.set({{30, mem}, {29, size}});
    if (mem == 0) raise(c, mem, error_null_object, 0x107678dc);
    c.set(3, size + header_size + 2);
    c.call(game_malloc, 0x107678e4);
    const auto block = c.r(3);
    c.set(31, block);
    if (block == 0) raise(c, mem, error_out_of_memory, 0x10767904);
    c.set({{4, trailer_first}});
    c.put32(block + block_stamp_at, block_stamp);
    c.put32(block + block_size_at, size);
    put_trailer(c, block, size);
    c.set(3, block + size);
    const auto used = c.u32(mem + count), room = c.u32(mem + capacity);
    c.set(3, used);
    if (!(std::int32_t(used) < std::int32_t(room))) raise(c, mem, error_too_many_pointers, 0x10767950);
    const auto slots = c.u32(mem + capacity), blocks = c.u32(mem + table);
    auto next = c.u32(mem + cursor);
    std::uint32_t skipped = 0;
    c.set({{5, slots}, {4, 0}, {6, blocks}, {7, next}});
    if (std::int32_t(slots) > 0) {
        for (std::uint32_t remaining = slots; remaining != 0; --remaining) {
            const auto at = rlwinm(next, 2, 0, 29);
            if (c.u32(blocks + at) == 0) {
                c.put32(blocks + at, block);
                c.put32(block + block_index_at, next);
                c.put32(mem + count, c.u32(mem + count) + 1);
                break;
            }
            ++skipped;
            next = (next + 1) % slots;
        }
    }
    c.set({{4, skipped}, {7, next}});
    c.put32(mem + cursor, next);
    if (!(std::int32_t(skipped) < std::int32_t(slots))) raise(c, mem, error_no_free_slot, 0x107679d4);
    c.set(3, block + header_size);
    return block + header_size;
}

// 0x10767a00: tsi_ReAllocMem(mem, pointer, size): the block checked as in release, resized by the game's realloc and its slot
// updated. A null pointer returns null. Not run by the game (dormant): its callers are the glyph, scan converter and cmap code.
std::uint32_t reallocate_block(Cpu& c, std::uint32_t mem, std::uint32_t pointer, std::uint32_t size) {
    guest::Frame frame(c, 0x10767a00, 0x60, {.first_gpr = 25});
    const auto pool = pool_of(c);
    c.set({{29, pointer}, {31, pool}, {28, mem}, {30, size}});
    if (pointer == 0) {
        c.set(3, 0);
        return 0;
    }
    if (c.u32(pointer - header_size + block_stamp_at) != block_stamp) raise(c, mem, error_bad_block, 0x10767a4c);
    c.set(25, c.u32(pointer - 0xc));
    const auto old_size = c.r(25);
    if (c.u8(pointer + old_size) != trailer_first) raise(c, mem, error_bad_block, 0x10767a74);
    if (c.u8(pointer + old_size + 1) != trailer_second) raise(c, mem, error_bad_block, 0x10767a9c);
    const auto used = c.u32(mem + count);
    const auto blocks = c.u32(mem + table);
    c.set(25, blocks);
    const auto room = c.u32(mem + capacity);
    if (std::int32_t(used) <= 0 || !(std::int32_t(used) <= std::int32_t(room))) raise(c, mem, error_bad_count, 0x10767ad0);
    const auto index = c.u32(pointer - 8);
    const auto offset = rlwinm(index, 2, 0, 29);
    c.set({{26, index}, {27, offset}});
    if (c.u32(blocks + offset) != pointer - header_size) {
        c.set(5, 0x13c);
        c.set({{3, pool + 0x80}, {4, pool + name_offset}});
        c.call(assertion_report, 0x10767af8);
    }
    c.set({{3, pointer - header_size}, {4, size + header_size + 2}});
    c.call(game_realloc, 0x10767b04);
    c.put32(blocks + offset, c.r(3));
    const auto block = c.u32(blocks + offset);
    c.set(27, block);
    if (block == 0) raise(c, mem, error_realloc_failed, 0x10767b2c);
    if (c.u32(block + block_stamp_at) != block_stamp) raise(c, mem, error_bad_block, 0x10767b54);
    c.put32(block + block_size_at, size);
    c.set(3, block + size);
    c.set(4, trailer_first);
    put_trailer(c, block, size);
    if (!(std::int32_t(index) < c.i32(mem + capacity))) raise(c, mem, error_slot_mismatch, 0x10767b90);
    c.set(3, block + header_size);
    return block + header_size;
}

// 0x10767bb0: tsi_DeAllocMem(mem, pointer): header stamp, trailer bytes and the table slot verified (error 0x2719 / 0x271f
// otherwise), the slot cleared, the count decremented and the block freed. A null pointer is ignored.
void release_block(Cpu& c, std::uint32_t mem, std::uint32_t pointer) {
    guest::Frame frame(c, 0x10767bb0, 0x50, {.first_gpr = 28});
    c.set({{31, pointer}, {30, mem}});
    if (mem == 0) raise(c, mem, error_null_object, 0x10767bf0);
    if (pointer == 0) return;
    if (c.u32(pointer - header_size + block_stamp_at) != block_stamp) raise(c, mem, error_bad_block, 0x10767c20);
    const auto size = c.u32(pointer - 0xc);
    c.set(28, size);
    if (c.u8(pointer + size) != trailer_first) raise(c, mem, error_bad_block, 0x10767c48);
    if (c.u8(pointer + size + 1) != trailer_second) raise(c, mem, error_bad_block, 0x10767c70);
    const auto index = c.u32(pointer - 8);
    const auto blocks = c.u32(mem + table);
    const auto offset = rlwinm(index, 2, 0, 29);
    c.set({{28, blocks}, {29, offset}});
    if (c.u32(blocks + offset) != pointer - header_size) raise(c, mem, error_slot_mismatch, 0x10767ca4);
    c.put32(blocks + offset, 0);
    const auto used = c.u32(mem + count);
    c.set(4, used);
    c.put32(mem + count, used - 1);
    c.set(3, pointer - header_size);
    c.call(game_free, 0x10767cc0);
}
}
