// DATABASE.CPP's loading block (database_loading.hpp): the allocator wrappers for the tables and the
// language object (its release, its file load and its seven accessors), the language-number lookups.
// Every routine keeps the original's frame, the registers it holds across each call (a callee's prologue
// saves them) and what it leaves in r3-r10 and cr0.
#include "database_loading.hpp"
#include "database_loading_detail.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::extended;
namespace callee = loading::callee;
namespace toc = loading::toc;

std::uint32_t sign_extended_byte(std::uint32_t value) { return extended(std::int16_t(std::int8_t(value))); }

// The byte at TOC -0x39d0: the original frees through one call site or another by it.
std::uint32_t release_flag(Cpu& c) { return sign_extended_byte(c.u8(c.r(2) + std::uint32_t(toc::release_flag))); }

// 0x100a4c00 and 0x100a4c70: one body. A frame of 0x40 saving r31 and LR; r31 holds the base word's address.
void release_table(Cpu& c, std::uint32_t entry, std::int32_t word_slot, std::int32_t count_slot) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 31});
    const auto word = c.toc(word_slot);
    c.set(31, word);
    const auto block = c.u32(word);
    c.set(0, block);
    if (compare_logical(c, block) == 0) return;
    const auto flag = release_flag(c);
    c.set(0, flag);
    const bool flagged = compare(c, std::int32_t(flag)) != 0;
    c.set(3, c.u32(word));
    c.call(callee::release, entry + (flagged ? 0x34 : 0x40));  // the same call at two sites
    c.set({{3, c.toc(count_slot)}, {4, 0}, {0, 0}});
    c.put32(word, 0);
    c.put32(c.r(3), 0);
}
}

void release_table_74c4(Cpu& c) { release_table(c, 0x100a4c00, -0x74c4, -0x74c8); }
void release_table_74cc(Cpu& c) { release_table(c, 0x100a4c70, -0x74cc, -0x74d0); }

// 0x100a4ce0: frame of 0x50 saving r29-r31; r29 the count to add, r30 the count word's address, r31 the
// base word's address.
bool grow_table_7a34(Cpu& c, std::int32_t count) {
    constexpr std::uint32_t entry = 0x100a4ce0, record_size = 0x1c;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 29});
    const auto base_word = c.toc(-0x7a34), count_word = c.toc(-0x7a38);
    c.set({{31, base_word}, {30, count_word}, {29, std::uint32_t(count)}});
    const auto old_count = c.u32(count_word);
    c.set({{0, old_count + std::uint32_t(count)}, {3, c.u32(base_word)}, {4, (old_count + std::uint32_t(count)) * record_size}});  // r0 as the add leaves it
    c.call(callee::resize, entry + 0x38);
    const auto block = c.r(3);
    if (compare_logical(c, block) == 0) {
        c.set(3, 0);
        return false;
    }
    // The table now holds the grown count; its first index and the address of the record with that index are
    // published in the two words TOC -0x751c and -0x7518 point at.
    const auto new_count = c.u32(count_word) + std::uint32_t(count);
    const auto first_index_word = c.toc(-0x751c), published = c.toc(-0x7518);
    const auto first_index = c.u32(first_index_word);
    c.put32(count_word, new_count);
    c.put32(base_word, block);
    c.put32(published, block + (new_count - first_index) * record_size);
    // The new records, the last one first: the index in the first word, zeros elsewhere.
    std::uint32_t step = 1;
    for (; std::int32_t(step) <= count; ++step) {
        const auto record = block + (new_count - step) * record_size;
        c.put32(record, new_count - step);
        c.put32(record + 0x4, 0);
        c.put16(record + 0x18, 0);
        c.put32(record + 0x8, 0);
        c.put32(record + 0xc, 0);
        c.put32(record + 0x10, 0);
        c.put32(record + 0x14, 0);
    }
    // What the callers can see: the loop counter in r4 and zeros in r5-r10 (or, with no record, the words the
    // setup left there), and the last compare.
    c.compare(0, std::int32_t(step), count);
    c.set(4, step);
    if (step > 1) c.set({{5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}});
    else c.set({{5, published}, {6, first_index}});
    c.set(3, 1);
    return true;
}

// 0x100d4880: frame of 0x40 saving r30 (the object) and r31 (the delete flag).
std::uint32_t release_language_object(Cpu& c, std::uint32_t object, std::int32_t delete_object) {
    constexpr std::uint32_t entry = 0x100d4880;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 30});
    c.set({{31, std::uint32_t(delete_object)}, {30, object}});
    if (compare(c, std::int32_t(object)) != 0) {
        const loading::LanguageObject view(c, object);
        for (std::uint32_t n = 0; n < loading::object::by_address.size(); ++n) {
            const auto block = view.array(loading::object::by_address[n]);
            c.set(0, block);
            if (compare_logical(c, block) == 0) continue;
            c.set({{0, block}, {3, block}});  // r0 holds the word the test loaded
            c.call(callee::release, entry + 0x34 + 0x14 * n);
        }
        const auto flag = extended(std::int16_t(delete_object));
        c.set(0, flag);
        if (compare(c, std::int32_t(flag)) > 0) {
            c.set(3, object);
            c.call(callee::operator_delete, entry + 0xd0);
        }
    }
    c.set(3, object);
    return object;
}

// 0x100b8b80: a frame of 0x40 saving only LR. The language file's name (an offset in the string pool) per language number.
std::uint32_t language_file_exists(Cpu& c, std::int32_t language) {
    constexpr std::uint32_t entry = 0x100b8b80;
    constexpr std::array<std::uint32_t, 11> names{0x846, 0x846, 0x836, 0x83e, 0x84e, 0x866, 0x876, 0x82e, 0x85e, 0x856, 0x86e};
    c.set(0, c.lr());
    const auto pool = c.toc(toc::string_pool);
    c.set(4, pool);
    guest::Frame frame(c, entry, 0x40, {});
    const auto number = sign_extended_byte(std::uint32_t(language));
    if (std::int32_t(number) < 0 || number > 10) {
        // The original compares the number with 0, 1 ... 10 in turn; the last compare is the one left in cr0.
        c.compare(0, std::int32_t(number), 10);
        c.set(3, 0);
        return 0;
    }
    // The compare chain ends equal at the language's own case, with r0 holding the number.
    c.set(0, number);
    c.compare(0, std::int32_t(number), std::int32_t(number));
    // Each case has its own pair of call sites, 0x20 apart (0 and 1 share the first).
    const auto site = 0x20 * (number == 0 ? 0 : number - 1);
    c.set({{3, pool + loading::pool::folder}, {4, pool + names[number]}});
    c.call(callee::data_folder_path, entry + 0x30 + site);
    c.call(callee::file_exists, entry + 0x34 + site);
    return c.r(3);
}
}
