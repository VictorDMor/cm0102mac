// DATABASE.CPP's save block: 0x100abfb0 (save_database), see database_save.hpp and database_save_detail.hpp.
// The original is one pass over 22 table files plus the index file. A section looks its directory entry up by id,
// copies it into the frame, opens a stream for it, hands the table to its own writer/reader and closes the stream;
// every failure closes what is open, resolves the indexes again (when the loose flag byte is set) and returns 0.
// The sections differ in data (database_save_detail.hpp), so one function runs them all.
// Registers: the non-volatile ones are set where the original sets them, since every callee saves them into the
// stack image (r19 counts progress calls, r20 remembers the people entry, r21 counts entries, r22 holds the open
// stream, r23 the directory scan index). Argument registers are set as the callees read them; the scratch registers
// the original leaves between calls reach nothing.
#include "database_people_names.hpp"
#include "database_resolve.hpp"
#include "database_save.hpp"
#include "database_save_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using namespace save;
constexpr std::uint32_t entry = 0x100abfb0, frame_size = 0xe00;
// Frame layout (offsets from the new stack pointer).
constexpr std::uint32_t progress_buffer = 0x4cc, entries_at = 0x7cc, index_name_buffer = 0x500, index_block = 0x600;
// A directory entry: 0x44 bytes, 22 of them (the original's table at TOC -0x756c, copied into the frame).
constexpr std::uint32_t entry_size = 0x44, entry_count = 22;
namespace directory_entry {
constexpr std::uint32_t id = 0x34, count = 0x38, stream_size = 0x3c, type = 0x40;
}
// TOC words and loose words.
constexpr std::int32_t pool_slot = -0x5618, directory_slot = -0x756c, progress_slot = -0x7bb0, buffer_slot = -0x7bb8;
constexpr std::int32_t flag_slot = -0x39d0, split_slot = -0x39d4, version_slot = -0x39d8;  // from r2, not through a pointer
// Offsets in the string pool.
constexpr std::uint32_t index_name = 0xe2, zip_prefix_a = 0xc7, zip_prefix_b = 0xd0, zip_name = 0xc1;
// Callees outside the block.
constexpr std::uint32_t init_progress = 0x10727890, report_progress = 0x10743120, copy_entry = 0x1077fc28;
constexpr std::uint32_t compare_names = 0x1077dc90, copy_name = 0x1077dbf0, formatted_output = 0x1077b720;
constexpr std::uint32_t close_write = 0x10747900, close_read = 0x107766f0, stream_size_write = 0x10748150,
                        stream_size_read = 0x10777cc0, put_bytes = 0x10747cc0, get_bytes = 0x10776360;
constexpr std::uint32_t zip_directory_name = 0x10746280, zip_finish = 0x107485f0;
// The second part of the section split at the loose word: the table at TOC -0x7bbc (records of 0x6c bytes, count at
// -0x7b64) from record count - split on.
constexpr std::int32_t second_table_slot = -0x7bbc, second_count_slot = -0x7b64;
constexpr std::uint32_t second_stride = 0x6c;
// The index file's tail: the optional array (TOC -0x74f8) of one word per club, the club count and a loose byte.
constexpr std::int32_t array_slot = -0x74f8, club_count_slot = -0x7b50, trailer_slot = -0x7ab0;
// Return addresses of the index file's calls (the address of each `bl`).
namespace tail {
constexpr std::uint32_t open_write = 0x100af53c, open_read = 0x100af54c, open_fail_resolve = 0x100af590;
constexpr std::uint32_t split_put = 0x100af5b4, split_get = 0x100af5cc, version_put = 0x100af5e8, version_get = 0x100af600;
constexpr std::uint32_t entries_put = 0x100af61c, entries_get = 0x100af634;
constexpr std::uint32_t first_close_write = 0x100af650, first_close_read = 0x100af65c;
constexpr std::uint32_t second_close_write = 0x100af678, second_close_read = 0x100af684, fail_resolve = 0x100af698;
constexpr std::uint32_t marker_put = 0x100af6c4, marker_get = 0x100af6dc, block_put = 0x100af6f8, block_get = 0x100af710;
constexpr std::uint32_t none_put = 0x100af744, none_get = 0x100af75c, count_put = 0x100af77c, count_get = 0x100af794;
constexpr std::uint32_t array_put = 0x100af7bc, array_get = 0x100af7e0, trailer_put = 0x100af7fc, trailer_get = 0x100af814;
constexpr std::uint32_t ok_close_write = 0x100af828, ok_close_read = 0x100af834, ok_resolve = 0x100af848;
constexpr std::uint32_t zip_name = 0x100af85c, zip_copy = 0x100af868, zip_finish = 0x100af880;
constexpr std::uint32_t slot_first = 0x48, slot_second = 0x44, slot_ok = 0x3c;  // frame slots holding the handle
constexpr std::uint32_t marker_local = 0x38, none_local = 0x40;
}
constexpr std::uint32_t prologue_progress = 0x100ac028, prologue_replace = 0x100ac038, prologue_progress_second = 0x100ac058;

class Saver {
public:
    Saver(Cpu& cpu, guest::Frame& frame) : c(cpu), frame_(frame) {}

    void start() {
        c.set({{3, frame_.at(progress_buffer)}, {4, c.r(31) + 0x1e6}});  // r4 was set earlier in the prologue
        c.call(init_progress, 0x100ac00c);
        progress(prologue_progress, true);
        if (flag_set()) {
            c.link(prologue_replace + 4);
            replace_pointers_with_ids(c);
        }
        progress(prologue_progress_second, true);
    }
    // One section; false when it failed (everything is closed and r3 is 0).
    bool section(const Section& s) {
        c.set(23, 0);
        scan(s);
        const bool present = c.u32(c.toc(s.table)) != 0;  // the table exists
        const bool found = std::int32_t(c.r(23)) < std::int32_t(entry_count);
        if (present && !found) report_missing(s);
        if (present && found && !transfer(s)) return false;
        if (found && !s.last) c.set(21, c.r(21) + 1);
        progress(s.sites.progress, !s.last);
        return true;
    }
    // The index file; returns the routine's result.
    std::uint32_t finish() {
        const auto name = c.r(31) + index_name;
        if (writing()) {
            c.set({{3, c.r(15)}, {4, name}, {5, 1}, {6, c.r(17)}, {7, c.r(18)}});
            c.call(0x107475b0, tail::open_write + 4);
        } else {
            c.set({{3, c.r(15)}, {4, name}});
            c.call(0x105d7200, tail::open_read + 4);
        }
        c.set(22, c.r(3));
        if (c.r(22) == 0) return fail(tail::open_fail_resolve);
        transfer_words(tail::split_put, tail::split_get, c.r(2) + std::uint32_t(split_slot), 4, 1);
        transfer_words(tail::version_put, tail::version_get, c.r(2) + std::uint32_t(version_slot), 4, 1);
        // Stale stack: frame+0x600..0x7cc (the 0x1cc block written below) and the directory slots from r21 on are never
        // written by this routine; the file receives whatever the stack held (per-reader stack contract, CLAUDE.md), so
        // nothing here may fill or reuse those bytes.
        const auto written = transfer_words(tail::entries_put, tail::entries_get, frame_.at(entries_at), entry_size, entry_count);
        if (written != entry_count) {
            // Both closes run: the original tests the handle again and it is still set.
            close(tail::first_close_write, tail::first_close_read, tail::slot_first);
            close(tail::second_close_write, tail::second_close_read, tail::slot_second);
            return fail(tail::fail_resolve);
        }
        c.set(3, 1);
        c.put8(frame_.at(tail::marker_local), 1);
        transfer_words(tail::marker_put, tail::marker_get, frame_.at(tail::marker_local), 1, 1);
        transfer_words(tail::block_put, tail::block_get, frame_.at(index_block), 0x1cc, 1);
        const auto array = c.u32(c.toc(array_slot));
        if (array == 0) {
            c.put32(frame_.at(tail::none_local), 0xffffffff);  // stored before the direction is tested
            transfer_words(tail::none_put, tail::none_get, frame_.at(tail::none_local), 4, 1);
        } else {
            transfer_words(tail::count_put, tail::count_get, c.toc(club_count_slot), 4, 1);
            const auto clubs = c.u32(c.toc(club_count_slot));
            transfer_words(tail::array_put, tail::array_get, array, clubs << 2, 1);
        }
        transfer_words(tail::trailer_put, tail::trailer_get, c.toc(trailer_slot), 1, 1);
        close(tail::ok_close_write, tail::ok_close_read, tail::slot_ok);
        if (flag_set()) resolve(tail::ok_resolve, 1);
        guest::compare(c, std::int8_t(c.r(16)));  // extsb. r0,r16: cr0 stays like this when nothing is written
        if (writing()) {
            c.set({{3, c.r(31) + zip_prefix_a}, {4, c.r(31) + zip_prefix_b}});
            c.call(zip_directory_name, tail::zip_name + 4);
            c.set({{4, c.r(3)}, {3, frame_.at(index_name_buffer)}});
            c.call(copy_name, tail::zip_copy + 4);
            c.set({{3, c.r(15)}, {4, frame_.at(index_name_buffer)}, {5, c.r(31) + zip_name}, {6, c.r(17)}, {7, c.r(18)}});
            c.call(zip_finish, tail::zip_finish + 4);
        }
        c.set(3, 1);
        return 1;
    }

private:
    bool writing() const { return std::int8_t(c.r(16)) != 0; }  // extsb.: the direction is the low byte
    // lbz; extsb.: the flag byte, compared with 0 into cr0.
    bool flag_set() { return guest::compare(c, std::int8_t(c.u8(c.r(2) - std::uint32_t(-flag_slot)))) != 0; }
    std::uint32_t entry_at(std::uint32_t index) const { return frame_.at(entries_at) + index * entry_size; }
    std::uint32_t loose(std::int32_t slot) const { return c.u32(c.r(2) - std::uint32_t(-slot)); }

    void progress(std::uint32_t at, bool advance) {
        const auto count = c.r(19);
        if (advance) c.set(19, count + 1);
        c.set({{3, c.r(25)}, {4, frame_.at(progress_buffer)}, {5, guest::extended(std::int16_t(count))}, {6, 0x18},
               {7, 0xffffffff}});
        c.call(report_progress, at + 4);
    }
    // The indexes are resolved again after a failure, only when the loose flag is set.
    void resolve(std::uint32_t at, std::uint32_t argument) {
        guest::enter(c, at + 4, {argument});
        resolve_database_indexes(c, argument);
    }
    std::uint32_t fail(std::uint32_t resolve_at) {
        if (flag_set()) resolve(resolve_at, 0);
        c.set(3, 0);
        return 0;
    }
    // Closes the open stream: a write stream through its handle slot in the frame, a read stream directly.
    void close(std::uint32_t write_at, std::uint32_t read_at, std::uint32_t slot) {
        if (writing()) {
            c.put32(frame_.at(slot), c.r(22));
            c.set(3, frame_.at(slot));
            c.call(close_write, write_at + 4);
        } else {
            c.set(3, c.r(22));
            c.call(close_read, read_at + 4);
        }
    }
    // Writes or reads `count` items of `size` bytes at `address` through the open stream; returns the callee's result.
    std::uint32_t transfer_words(std::uint32_t put_at, std::uint32_t get_at, std::uint32_t address, std::uint32_t size,
                                 std::uint32_t count) {
        c.set({{3, address}, {4, size}, {5, count}, {6, c.r(22)}});
        if (writing()) c.call(put_bytes, put_at + 4);
        else c.call(get_bytes, get_at + 4);
        return c.r(3);
    }

    // The directory entry with the section's id (or its second id) is copied to the frame's next slot.
    void scan(const Section& s) {
        for (; std::int32_t(c.r(23)) < std::int32_t(entry_count); c.set(23, c.r(23) + 1)) {
            const auto from = c.r(24) + c.r(23) * entry_size;
            const auto id = c.i32(from + directory_entry::id);
            if (id != std::int32_t(s.id) && !(s.also_id && id == std::int32_t(s.also_id))) continue;
            c.set({{3, from}, {4, entry_at(c.r(21))}, {5, entry_size}});
            c.call(copy_entry, s.sites.memcpy + 4);
            c.set(2, c.u32(c.sp() + 0x14));  // the glue saved the TOC pointer there
            if (s.marks_people) c.set(20, c.r(21));
            return;
        }
    }
    void report_missing(const Section& s) {
        c.set({{3, c.r(28)}, {4, c.r(31) + s.format}, {5, c.r(31) + index_name}});
        c.call(formatted_output, s.sites.say + 4);
        guest::report_assertion(c, {.at = s.sites.assertion, .file = c.r(31), .format = c.r(31) + 0xd,
                                    .title = c.r(31) + 0x1a, .name = frame_.at(s.name), .extension = frame_.at(s.extension),
                                    .line = s.line});
    }
    // The count the directory records for the section.
    std::uint32_t recorded_count(const Section& s) const {
        auto count = c.u32(c.toc(s.count));
        if (s.minus) count -= c.u32(c.toc(s.minus));
        if (s.minus_global) count -= loose(split_slot);
        return count;
    }
    // Opens the section's stream, runs its writer/reader and closes it; false on a failure.
    bool transfer(const Section& s) {
        const auto slot = entry_at(c.r(21));
        if (s.rename_format) {  // a file name equal to the people section's is replaced
            c.set({{3, slot}, {4, entry_at(c.r(20))}});
            c.call(compare_names, s.sites.rename_compare + 4);
            if (std::int32_t(c.r(3)) == 0) {
                c.set({{3, slot}, {4, c.r(31) + s.rename_format}});
                c.call(copy_name, s.sites.rename_copy + 4);
            }
        }
        if (writing()) {
            c.set({{3, c.r(15)}, {4, slot}, {5, 1}, {6, c.r(17)}, {7, c.r(18)}});
            c.call(0x107475b0, s.sites.open_write + 4);
        } else {
            c.set({{3, c.r(15)}, {4, slot}});
            c.call(0x105d7200, s.sites.open_read + 4);
        }
        c.set(22, c.r(3));
        if (c.r(22) == 0) {
            fail(s.sites.open_fail_resolve);
            return false;
        }
        std::uint32_t stream_size = 0;
        if (s.writes_id) c.put32(slot + directory_entry::id, 0x16);
        if (s.queries_size) {
            c.set(3, c.r(22));
            c.call(writing() ? stream_size_write : stream_size_read,
                   (writing() ? s.sites.size_write : s.sites.size_read) + 4);
            stream_size = c.r(3);
        }
        const auto count = recorded_count(s);
        c.put32(slot + directory_entry::stream_size, stream_size);
        c.put32(slot + directory_entry::type, s.type);
        c.put32(slot + directory_entry::count, count);
        if (!run_callee(s.callee, s.sites.callee, c.u32(c.toc(s.table)), c.u32(slot + directory_entry::count),
                        s.fail_slot, s.sites.fail_close_write, s.sites.fail_close_read, s.sites.fail_resolve))
            return false;
        if (s.second_pass) {
            // A positive loose count writes the tail of the second table as a second part.
            const auto split = std::int32_t(loose(split_slot));
            if (split > 0) {
                const auto first = c.u32(c.toc(second_table_slot));
                const auto from = first + (c.u32(c.toc(second_count_slot)) - std::uint32_t(split)) * second_stride;
                if (!run_callee(s.callee, s.sites.callee2, from, std::uint32_t(split), s.fail2_slot,
                                s.sites.fail2_close_write, s.sites.fail2_close_read, s.sites.fail2_resolve))
                    return false;
            }
        }
        close(s.sites.ok_close_write, s.sites.ok_close_read, s.ok_slot);
        return true;
    }
    bool run_callee(std::uint32_t callee, std::uint32_t at, std::uint32_t first, std::uint32_t count,
                    std::uint32_t slot, std::uint32_t close_write_at, std::uint32_t close_read_at,
                    std::uint32_t resolve_at) {
        c.set({{3, c.r(22)}, {4, c.r(16)}, {5, first}, {6, count}});
        c.call(callee, at + 4);
        if (std::int8_t(c.r(3)) != 0) return true;  // extsb.: the result is a byte
        close(close_write_at, close_read_at, slot);
        fail(resolve_at);
        return false;
    }

    Cpu& c;
    guest::Frame& frame_;
};
}

std::uint32_t save_database(Cpu& c, std::uint32_t archive, std::uint32_t writing, std::uint32_t extra_a,
                            std::uint32_t extra_b) {
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, frame_size, {.first_gpr = 13});
    c.set({{31, c.toc(pool_slot)}, {16, writing}, {24, c.toc(directory_slot)}, {15, archive}, {25, c.toc(progress_slot)},
           {17, extra_a}, {26, c.toc(-0x7530)}, {18, extra_b}, {27, c.toc(-0x752c)}, {14, c.toc(-0x7ab4)}, {21, 0},
           {13, c.toc(-0x7520)}, {19, 0}, {28, c.toc(buffer_slot)}, {29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}});
    Saver saver(c, frame);
    saver.start();
    for (const auto& section : sections)
        if (!saver.section(section)) return 0;
    return saver.finish();
}
}
