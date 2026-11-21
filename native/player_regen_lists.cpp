// PLAYER_REGEN.CPP, block `regen_lists`: the regen object's four lists and their appenders and removers (see player_regen.hpp
// for the layout). The appenders reallocate a full list's block (count = capacity - 1) 250 entries larger through the game's
// allocator and test the result as the original does: an absent block reports the game's assertion and calls 0x1076f7d0 (which
// ends the game). Each routine stores in the original's order and with the original's partial copies of an entry (a field the
// original does not copy keeps what the destination held). The routines whose callers read r4-r10 after the call leave
// them as the original does (the listing's last writes; a callee's leftovers where the original's last writer is a call).
#include "player_regen_detail.hpp"

namespace cm::aot::player_regen {
namespace {
std::uint32_t call_dependency(Cpu& c, std::uint32_t target, std::uint32_t back) { return invoke(c, target, back); }

std::uint32_t at(std::uint32_t base, std::int32_t index, std::uint32_t size) { return base + std::uint32_t(index) * size; }

// The assertion the original's growth reports when the allocator gave no block: its frame's buffers, the source line and the
// calls' addresses, then (`exits`) 0x1076f7d0 with -1.
struct Failure {
    std::uint32_t entry, frame_size;
    unsigned first_gpr;
    std::uint32_t assertion_at;  // the address of the original's `bl 0x10739890`
    std::int32_t name, extension;
    std::uint32_t line, exit_back;  // exit_back: the address after the `bl 0x1076f7d0` (0 where the original has none)
};
void report_failure(Cpu& c, const Failure& f) {
    // The original's frame holds the two buffers the report fills (saved registers and link as its prologue stores them).
    Frame frame(c, f.entry, f.frame_size, {.first_gpr = f.first_gpr});
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {f.assertion_at, pool + 0x37, pool + 0x48, pool + 0x55, frame.at(f.name), frame.at(f.extension), f.line});
    if (f.exit_back != 0) {
        c.set(3, 0xffffffffu);
        c.call(callee::quit, f.exit_back);
    }
}

// The reallocation of a list's block, `entry_size` bytes an entry, 250 entries larger; the block pointer takes the result (0
// when the allocator fails: the caller tests it) and the capacity grows by 250 either way.
void grow(Cpu& c, std::uint32_t regen, const ListSlots& list, std::uint32_t entry_size, std::uint32_t back) {
    c.set(3, c.u32(regen + list.entries));
    c.set(4, (c.u32(regen + list.capacity) + list_growth) * entry_size);
    const auto block = call_dependency(c, callee::reallocate, back);
    c.put32(regen + list.entries, block);
    c.put32(regen + list.capacity, c.u32(regen + list.capacity) + list_growth);
}

// Removes the first entry of a pool list (C or D) whose first word is `key`: later entries move down, the vacated last entry's
// first three words become -1.
void remove_pool_entry(Cpu& c, std::uint32_t regen, const ListSlots& list, std::int32_t key) {
    std::int32_t index = 0;
    for (;; ++index) {
        if (index >= c.i32(regen + list.count)) return;
        if (key == c.i32(c.u32(regen + list.entries) + std::uint32_t(index) * 0x10)) break;
    }
    for (; index < std::int32_t(c.u32(regen + list.count) - 1); ++index) {
        const auto to = at(c.u32(regen + list.entries), index, 0x10), from = at(c.u32(regen + list.entries), index + 1, 0x10);
        for (std::uint32_t word = 0; word < 0x10; word += 4) c.put32(to + word, c.u32(from + word));
    }
    for (const std::uint32_t offset : {0x10u, 0xcu, 0x8u})
        c.put32(c.u32(regen + list.entries) + c.u32(regen + list.count) * 0x10 - offset, 0xffffffffu);
    c.put32(regen + list.count, c.u32(regen + list.count) - 1);
}
}

void remove_staff_pool_entry(Cpu& c, std::uint32_t regen, std::int32_t key) { remove_pool_entry(c, regen, staff_pool_list, key); }
void remove_player_pool_entry(Cpu& c, std::uint32_t regen, std::int32_t key) { remove_pool_entry(c, regen, player_pool_list, key); }

void pop_staff_pool_entry(Cpu& c, std::uint32_t regen, std::uint32_t out_id, std::uint32_t out_nation, std::uint32_t out_third) {
    // The three words are read and stored one at a time, the list's block read afresh each time (an out pointer may lie in it);
    // the id is then the word the first out pointer holds.
    c.put32(out_id, c.u32(c.u32(regen + staff_pool_list.entries)));
    c.put32(out_nation, c.u32(c.u32(regen + staff_pool_list.entries) + 4));
    c.put32(out_third, c.u32(c.u32(regen + staff_pool_list.entries) + 8));
    remove_pool_entry(c, regen, staff_pool_list, c.i32(out_id));
}

void append_staff_pool(Cpu& c, std::uint32_t regen, std::uint32_t first, std::uint32_t second, std::uint32_t third) {
    save_link(c);
    // The quirk: the list tested for fullness is D (+0x40 against +0x44 - 1), the list grown is C. r4 is left holding
    // D's count (the constructor returns it to callers that pass their registers on).
    c.set(4, c.u32(regen + player_pool_list.count));
    if (c.i32(regen + player_pool_list.count) == std::int32_t(c.u32(regen + player_pool_list.capacity) - 1)) {
        grow(c, regen, staff_pool_list, 0x10, 0x102deaa0);
        if (c.u32(regen + staff_pool_list.entries) == 0)
            report_failure(c, {0x102dea50, 0x80, 26, 0x102dead0, 0x40, 0x38, 0x9c4, 0x102deb18});
    }
    const auto entry = at(c.u32(regen + staff_pool_list.entries), c.i32(regen + staff_pool_list.count), 0x10);
    c.put32(entry, first);
    c.put32(entry + 4, second);
    c.put32(entry + 8, third);
    c.put32(regen + staff_pool_list.count, c.u32(regen + staff_pool_list.count) + 1);
}

void append_player_pool(Cpu& c, std::uint32_t regen, std::uint32_t person, std::uint32_t second, std::uint32_t third) {
    save_link(c);
    const auto capacity = c.u32(regen + player_pool_list.capacity);
    c.set(7, capacity);
    if (c.i32(regen + player_pool_list.count) == std::int32_t(capacity - 1)) {
        grow(c, regen, player_pool_list, 0x10, 0x102dec9c);
        if (c.u32(regen + player_pool_list.entries) == 0)
            report_failure(c, {0x102dec50, 0x80, 26, 0x102deccc, 0x40, 0x38, 0xa0a, 0x102ded14});
    }
    // r5 the people table's base, r4 the word it copies: the first word of the person's player record.
    const auto base = c.u32(c.toc(toc::people));
    c.set(5, base);
    const auto word = c.u32(c.u32(base + person * person::size + person::player));
    c.set(4, word);
    const auto entry = at(c.u32(regen + player_pool_list.entries), c.i32(regen + player_pool_list.count), 0x10);
    c.put32(entry, word);
    c.put32(entry + 4, second);
    c.put32(entry + 8, third);
    c.put32(regen + player_pool_list.count, c.u32(regen + player_pool_list.count) + 1);
}

void append_retirement(Cpu& c, std::uint32_t regen, std::uint32_t source) {
    save_link(c);
    const auto capacity = c.u32(regen + retirement_list.capacity);
    c.set(5, capacity);
    if (c.i32(regen + retirement_list.count) == std::int32_t(capacity - 1)) {
        grow(c, regen, retirement_list, retirement_entry::size, 0x102deec0);
        if (c.u32(regen + retirement_list.entries) == 0)
            report_failure(c, {0x102dee70, 0x70, 28, 0x102deef0, 0x40, 0x38, 0xa53, 0x102def38});
    }
    const auto entry = at(c.u32(regen + retirement_list.entries), c.i32(regen + retirement_list.count), retirement_entry::size);
    c.put32(entry + retirement_entry::id, c.u32(source));
    c.put32(entry + 4, c.u32(source + 4));
    c.put8(entry + retirement_entry::kind, c.u8(source + retirement_entry::kind));
    c.put16(entry + retirement_entry::first_date, c.u16(source + retirement_entry::first_date));
    c.put16(entry + retirement_entry::first_date + 2, c.u16(source + retirement_entry::first_date + 2));
    c.put16(entry + retirement_entry::second_date, c.u16(source + retirement_entry::second_date));
    c.put16(entry + retirement_entry::second_date + 2, c.u16(source + retirement_entry::second_date + 2));
    c.put8(entry + retirement_entry::announced, c.u8(source + retirement_entry::announced));
    c.set(4, c.u8(source + retirement_entry::announced));
    c.put32(regen + retirement_list.count, c.u32(regen + retirement_list.count) + 1);
}

namespace {
void remove_retirement_entry(Cpu& c, std::uint32_t regen, std::uint32_t key_entry) {
    // bsearch(key, entries, count, 0x1c, comparator): its leftovers stay in r4..r10 where the original has no later write.
    c.set({{3, key_entry}, {4, c.u32(regen + retirement_list.entries)}, {5, c.u32(regen + retirement_list.count)}, {6, retirement_entry::size},
           {7, c.toc(toc::bsearch_comparator)}});
    const auto found = call_dependency(c, callee::bsearch, 0x102df04c);
    if (found == 0) return;
    // The found entry's index (the byte offset divided by 0x1c, as unsigned words).
    std::int32_t index = std::int32_t((found - c.u32(regen + retirement_list.entries)) / retirement_entry::size);
    for (; index < std::int32_t(c.u32(regen + retirement_list.count) - 1); ++index) {
        const auto to = at(c.u32(regen + retirement_list.entries), index, retirement_entry::size);
        const auto from = at(c.u32(regen + retirement_list.entries), index + 1, retirement_entry::size);
        c.put32(to, c.u32(from));
        c.put32(to + 4, c.u32(from + 4));
        c.put16(to + 0xa, c.u16(from + 0xa));
        c.put16(to + 8, c.u16(from + 8));
        c.put8(to + 0xc, c.u8(from + 0xc));
        c.put16(to + 0x12, c.u16(from + 0x12));
        c.put16(to + 0x10, c.u16(from + 0x10));
        c.put8(to + 0x14, c.u8(from + 0x14));
        c.put8(to + 0x18, c.u8(from + 0x18));
        c.put8(to + 0x19, c.u8(from + 0x19));
    }
    // The vacated last entry (the count is reloaded for each store, as in the original).
    const auto last = [&](std::uint32_t offset) { return c.u32(regen + retirement_list.entries) + c.u32(regen + retirement_list.count) * retirement_entry::size - offset; };
    c.put32(last(0x1c), 0xffffffffu);
    c.put8(last(4), 0);
    c.put16(last(0x14), 0);
    c.put16(last(0x12), 0);
    c.put16(last(0xc), 0);
    c.put16(last(0xa), 0);
    c.set({{4, 0}, {5, 0xffffffffu}, {6, std::uint32_t(index)}});
    c.put32(regen + retirement_list.count, c.u32(regen + retirement_list.count) - 1);
}
}

void remove_retirement(Cpu& c, std::uint32_t regen, std::uint32_t key_entry) {
    save_link(c);
    remove_retirement_entry(c, regen, key_entry);
}

bool remove_retirement_by_id(Cpu& c, std::uint32_t regen, std::uint32_t key) {
    save_link(c);
    if (key == 0) return false;
    const auto count = c.i32(regen + retirement_list.count);
    if (count <= 0) return false;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto entries = c.u32(regen + retirement_list.entries);
        if (c.u32(key) == c.u32(entries + std::uint32_t(index) * retirement_entry::size)) {
            remove_retirement_entry(c, regen, at(entries, index, retirement_entry::size));
            return true;
        }
    }
    return false;
}

void pop_queue_entry(Cpu& c, std::uint32_t regen) {
    std::int32_t index = 0;
    for (; index < std::int32_t(c.u32(regen + queue_list.count) - 1); ++index) {
        const auto to = at(c.u32(regen + queue_list.entries), index, 0xc), from = at(c.u32(regen + queue_list.entries), index + 1, 0xc);
        c.put32(to, c.u32(from));
        c.put16(to + 6, c.u16(from + 6));
        c.put16(to + 4, c.u16(from + 4));
        c.put8(to + 8, c.u8(from + 8));
    }
    const auto last = [&](std::uint32_t offset) { return c.u32(regen + queue_list.entries) + c.u32(regen + queue_list.count) * 0xc - offset; };
    c.set({{4, c.u32(regen + queue_list.count)}, {5, 0}, {6, 0xffffffffu}, {7, std::uint32_t(index)}});
    c.put32(last(0xc), 0xffffffffu);
    c.put16(last(8), 0);
    c.put16(last(6), 0);
    c.put32(regen + queue_list.count, c.u32(regen + queue_list.count) - 1);
}

void register_regen(Cpu& c, std::uint32_t regen, std::uint32_t record) {
    save_link(c);
    const auto id = c.u32(record);
    c.set(4, id);
    if (std::int32_t(id) < 0 || std::int32_t(id) > c.i32(c.toc(toc::person_count))) {
        // Not a person of the table: the report, then nothing (no exit).
        report_failure(c, {0x102df1a0, 0x110, 24, 0x102df1fc, 0xb8, 0x50, 0xa9d, 0});
        return;
    }
    const auto people = c.u32(c.toc(toc::people));
    const auto person = people + id * person::size;
    c.set({{4, id * person::size}, {5, people}, {6, c.u32(regen + queue_list.capacity)}});
    // List B: the record's id and two halfwords.
    if (c.i32(regen + queue_list.count) == std::int32_t(c.u32(regen + queue_list.capacity) - 1)) {
        grow(c, regen, queue_list, 0xc, 0x102df270);
        if (c.u32(regen + queue_list.entries) == 0)
            report_failure(c, {0x102df1a0, 0x110, 24, 0x102df2a0, 0x98, 0x48, 0xaac, 0x102df2e4});
    }
    auto entry = at(c.u32(regen + queue_list.entries), c.i32(regen + queue_list.count), 0xc);
    c.put32(entry, c.u32(record));
    entry = at(c.u32(regen + queue_list.entries), c.i32(regen + queue_list.count), 0xc);
    c.put16(entry + 4, c.u16(record + 4));
    entry = at(c.u32(regen + queue_list.entries), c.i32(regen + queue_list.count), 0xc);
    c.put16(entry + 6, c.u16(record + 6));
    c.set(4, c.u16(record + 6));
    c.put32(regen + queue_list.count, c.u32(regen + queue_list.count) + 1);

    // List C: a person with a staff record.
    if (c.u32(person + person::staff) != 0) {
        const auto capacity = c.u32(regen + staff_pool_list.capacity);
        c.set(4, capacity);
        if (c.i32(regen + staff_pool_list.count) == std::int32_t(capacity - 1)) {
            grow(c, regen, staff_pool_list, 0x10, 0x102df364);
            if (c.u32(regen + staff_pool_list.entries) == 0)
                report_failure(c, {0x102df1a0, 0x110, 24, 0x102df394, 0x78, 0x40, 0xac5, 0x102df3d8});
        }
        const auto staff_word = c.u32(c.u32(person + person::staff));
        c.set(4, staff_word);
        c.put32(at(c.u32(regen + staff_pool_list.entries), c.i32(regen + staff_pool_list.count), 0x10), staff_word);
        const auto nation = c.u32(person + person::nation);
        c.set(4, nation);
        std::uint32_t second;
        if (nation != 0) {
            second = c.u32(nation + person::nation_key);
        } else {
            // No nation: a draw below the nation count (inline copy of the pooled Random).
            const auto bound = c.i32(c.toc(toc::nation_count));
            second = 0;
            if (bound != 0) {
                const Pool pool(c);
                ScaledLeftovers left;
                second = std::uint32_t(pool.draw(bound, refill_site::nation_draw, refill_site::random_first, nullptr, &left));
                // The scaled form leaves r5 = 0xffff, r6 = 0x80008001, r7 = the sum and r8 = the sample (the call of the second
                // draw, a small one, leaves them alone).
                if (bound >= 0x10000 || bound <= -0x10000)
                    c.set({{5, 0xffff}, {6, 0x80008001u}, {7, left.sum}, {8, left.sample}});
            }
        }
        c.put32(at(c.u32(regen + staff_pool_list.entries), c.i32(regen + staff_pool_list.count), 0x10) + 4, second);
        c.set(4, c.u32(person));
        c.put32(at(c.u32(regen + staff_pool_list.entries), c.i32(regen + staff_pool_list.count), 0x10) + 0xc, c.u32(person));
        c.put32(regen + staff_pool_list.count, c.u32(regen + staff_pool_list.count) + 1);
    }

    // List D: a person with a player record.
    if (c.u32(person + person::player) != 0) {
        const auto capacity = c.u32(regen + player_pool_list.capacity);
        c.set(4, capacity);
        if (c.i32(regen + player_pool_list.count) == std::int32_t(capacity - 1)) {
            grow(c, regen, player_pool_list, 0x10, 0x102df5ec);
            // The original tests the list's COUNT here (not its block): the assertion needs an empty list.
            if (c.u32(regen + player_pool_list.count) == 0)
                report_failure(c, {0x102df1a0, 0x110, 24, 0x102df61c, 0x58, 0x38, 0xae9, 0x102df660});
        }
        const auto word = c.u32(c.u32(person + person::player));
        c.set(4, word);
        c.put32(at(c.u32(regen + player_pool_list.entries), c.i32(regen + player_pool_list.count), 0x10), word);
        c.put32(at(c.u32(regen + player_pool_list.entries), c.i32(regen + player_pool_list.count), 0x10) + 4,
                c.u32(c.u32(person + person::nation) + person::nation_key));
        c.set(4, c.u32(person));
        c.put32(at(c.u32(regen + player_pool_list.entries), c.i32(regen + player_pool_list.count), 0x10) + 0xc, c.u32(person));
        c.put32(regen + player_pool_list.count, c.u32(regen + player_pool_list.count) + 1);
    }
}
}
