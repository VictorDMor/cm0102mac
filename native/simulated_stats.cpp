#include "simulated_stats_adapter.hpp"
#include "guest_values.hpp"
#include "guest_range_sampler.hpp"
#include "routine_check.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

constexpr std::uint32_t destroy_entry = 0x10370db0, count_entry = 0x10371d90;
constexpr unsigned stats_rows = 4, staff_profile = 0x14, squad_entries = 0x98, squad_capacity = 50;
constexpr unsigned club_stride = 0x248;
constexpr std::uint32_t free_stats_rows = 0x10775170, delete_stats_object = 0x107735a0;
constexpr std::uint32_t squad_role = 0x10187680, squad_exclusion = 0x102b7c70;

void block(Cpu& c, std::uint32_t pc, unsigned instructions) {
    c.s.step(pc);
    c.s.charge(instructions - 1);
}

int record_value(Cpu& c, unsigned destination, std::int32_t value) {
    const auto upper = c.cr(0) & 0xf0;
    c.set(destination, std::uint32_t(value));
    const auto sign = compare(c, value);
    c.set_cr(0, std::uint8_t(upper | c.cr(0)));
    return sign;
}

void destroy_stats(Cpu& c) {
    GuestNativeScope scope(c.s, destroy_entry);
    block(c, destroy_entry, 8);
    const auto caller_stack = c.sp();
    c.set(0, c.lr());
    c.put32(caller_stack - 4, c.r(31));
    const guest::Kept<31> deleting(c, c.r(4));
    c.put32(caller_stack - 8, c.r(30));
    const guest::Kept<30> object(c, c.r(3));
    const auto present = record_value(c, 30, std::int32_t(object)) != 0;
    c.put32(caller_stack + 8, c.r(0));
    const auto frame = caller_stack - 0x40;
    c.put32(frame, caller_stack);
    c.set(1, frame);
    scope.frame(frame);

    if (present) {
        block(c, 0x10370dd0, 3);
        const auto rows = c.u32(object + stats_rows);
        c.set(3, rows);
        if (compare_logical(c, rows) != 0) {
            block(c, 0x10370ddc, 1);
            c.call(free_stats_rows, 0x10370de0);
        }
        block(c, 0x10370de0, 2);
        if (record_value(c, 0, std::int16_t(deleting)) > 0) {
            block(c, 0x10370de8, 2);
            c.set(3, object);
            c.call(delete_stats_object, 0x10370df0);
        }
    }

    block(c, 0x10370df0, 7);
    c.set(3, object);
    c.set(0, c.u32(frame + 0x48));
    c.set(1, caller_stack);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller_stack - 4));
    c.set(30, c.u32(caller_stack - 8));
    c.finish();
}

bool include_squad_candidate(Cpu& c, std::uint32_t member, std::uint32_t club,
                             std::uint32_t role_service, std::uint32_t date, std::uint32_t filter_service) {
    block(c, 0x10371de0, 3);
    const auto profile = c.u32(member + staff_profile);
    c.set(0, profile);
    if (compare_logical(c, profile) == 0) return false;

    block(c, 0x10371dec, 3);
    c.set({{3, c.u32(role_service)}, {4, member}});
    c.call(squad_role, 0x10371df8);
    block(c, 0x10371df8, 3);
    const auto role = std::int8_t(c.r(3));
    c.set(0, guest::extended(role));
    if (compare(c, role, 2) == 0) return false;

    block(c, 0x10371e04, 7);
    c.set({{3, c.u32(filter_service)}, {4, member}, {5, date}, {6, club}, {7, 0}, {8, 0}});
    c.call(squad_exclusion, 0x10371e20);
    block(c, 0x10371e20, 2);
    return compare_logical(c, c.r(3)) == 0;
}

void count_squad_candidates(Cpu& c) {
    GuestNativeScope scope(c.s, count_entry);
    block(c, count_entry, 15);
    const auto caller_stack = c.sp(), frame = caller_stack - 0x60;
    const auto club_index = c.r(3);
    c.set(0, c.lr());
    guest::transfer_saved_registers<24>(c, caller_stack);
    const auto clubs = c.toc(-0x7be8);
    c.set({{4, clubs}, {26, 0}});
    const guest::Kept<29> role_service(c, c.toc(-0x7ba0));
    c.set(27, 0);
    const guest::Kept<30> date(c, c.toc(-0x7b80));
    c.set(28, 0);
    const guest::Kept<31> filter_service(c, c.toc(-0x7a64));
    c.put32(caller_stack + 8, c.r(0));
    c.set(0, club_index * club_stride);
    c.put32(frame, caller_stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, c.u32(clubs));
    const guest::Kept<24> club(c, c.r(3) + club_index * club_stride);

    unsigned count = 0;
    for (unsigned slot = 0; ; ++slot) {
        block(c, 0x10371e34, 3);
        c.set(0, slot);
        if (compare(c, slot, squad_capacity) >= 0) break;
        block(c, 0x10371dd0, 4);
        const auto offset = squad_entries + slot * 4;
        c.set(0, offset);
        const guest::Kept<25> member(c, c.u32(club + offset));
        if (compare_logical(c, member) != 0 && include_squad_candidate(c, member, club, role_service, date, filter_service)) {
            block(c, 0x10371e28, 1);
            c.set(26, ++count);
        }
        block(c, 0x10371e2c, 2);
        c.set({{28, (slot + 1) * 4}, {27, slot + 1}});
    }

    block(c, 0x10371e40, 6);
    c.set(3, count);
    c.set(0, c.u32(frame + 0x68));
    c.set(1, caller_stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<24, true>(c, caller_stack);
    c.finish();
}

constexpr std::uint32_t load_entry = 0x10371e60, save_entry = 0x10372040;
constexpr std::uint32_t open_file = 0x107475b0, read_file = 0x10747a80, write_file = 0x10747cc0;
constexpr std::uint32_t close_file = 0x10747900, allocate_rows = 0x10775130;
constexpr unsigned row_count = 0, row_size = 0x44, stream_slot = 0x38, file_frame_size = 0xd0;

struct StatsFile {
    Cpu& c;
    std::uint32_t frame;
    std::uint32_t object() const { return c.r(28); }
    std::uint32_t strings() const { return c.r(31); }
};

StatsFile open_stats_frame(Cpu& c, GuestNativeScope& scope, bool writing) {
    const auto caller_stack = c.sp(), frame = caller_stack - file_frame_size;
    c.set(0, c.lr());
    c.put32(caller_stack - 4, c.r(31));
    if (!writing) compare_logical(c, c.r(4));
    c.set(31, c.toc(-0x4dcc));
    if (writing) c.set(5, 1);
    c.put32(caller_stack - 8, c.r(30));
    if (writing) c.set(6, 0x16);
    c.set(30, c.toc(-0x7ba8));
    if (writing) c.set(7, 4);
    c.put32(caller_stack - 0xc, c.r(29));
    c.set(29, c.toc(-0x7bac));
    c.put32(caller_stack - 0x10, c.r(28));
    c.set(28, c.r(3));
    if (writing) c.set({{3, c.r(4)}, {4, c.r(31) + 0x5e}});
    c.put32(caller_stack + 8, c.r(0));
    c.put32(frame, caller_stack);
    c.set(1, frame);
    scope.frame(frame);
    return {c, frame};
}

void finish_stats_file(StatsFile& file, std::uint32_t pc) {
    auto& c = file.c;
    block(c, pc, 8);
    c.set(0, c.u32(file.frame + file_frame_size + 8));
    const auto caller_stack = c.sp() + file_frame_size;
    c.set(1, caller_stack);
    c.set_lr(c.r(0));
    for (unsigned n = 31; n >= 28; --n) c.set(n, c.u32(caller_stack - (32 - n) * 4));
    c.finish();
}

bool keep_stats_stream(StatsFile& file, std::uint32_t pc) {
    auto& c = file.c;
    block(c, pc, 3);
    const bool opened = record_value(c, 6, std::int32_t(c.r(3))) != 0;
    c.put32(file.frame + stream_slot, c.r(3));
    return opened;
}

void report_stats_file_failure(StatsFile& file, std::uint32_t pc, unsigned phase, unsigned line, bool loading) {
    auto& c = file.c;
    // Each failed operation has its own original stack scratch locations.
    const auto message = file.frame + 0x94 - phase * 0x20;
    const auto detail = file.frame + 0x4c - phase * 8;
    block(c, pc, 6);
    c.set({{3, file.strings() + 0x37}, {6, message}, {7, detail}, {4, 0}, {5, 0}});
    c.call(0x10739890, pc + 24);

    block(c, pc + 24, 1);
    // The original generator inlines the version getter.
    c.link(pc + 28);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);

    block(c, pc + 28, 7);
    c.set({{5, c.r(3)}, {3, c.r(30)}, {4, file.strings() + 0x4b},
           {6, message}, {7, detail}, {8, line}});
    c.call(0x1077b720, pc + 56);
    block(c, pc + 56, 4);
    c.set({{4, c.r(30)}, {3, file.strings() + 0x58}, {5, 0}});
    c.call(0x10769b60, pc + 72);

    block(c, pc + 72, loading ? 4 : 3);
    c.set(0, 0);
    if (loading) c.set(3, 0);
    c.put8(c.r(29), 0);
}

bool transfer_stats_header(StatsFile& file, std::uint32_t pc, std::uint32_t io) {
    auto& c = file.c;
    block(c, pc, 4);
    c.set({{3, file.object() + row_count}, {4, 4}, {5, 1}});
    c.call(io, pc + 16);
    block(c, pc + 16, 2);
    return compare(c, std::int32_t(c.r(3)), 1) == 0;
}

bool transfer_stats_complete(StatsFile& file, std::uint32_t pc) {
    auto& c = file.c;
    block(c, pc, 3);
    const auto count = c.i32(file.object() + row_count);
    c.set(0, std::uint32_t(count));
    return compare(c, count, std::int32_t(c.r(3))) == 0;
}

void close_stats_stream(StatsFile& file, std::uint32_t pc) {
    block(file.c, pc, 2);
    file.c.set(3, file.frame + stream_slot);
    file.c.call(close_file, pc + 8);
}

void load_stats(Cpu& c) {
    GuestNativeScope scope(c.s, load_entry);
    block(c, load_entry, 13);
    const auto source = c.r(4);
    auto file = open_stats_frame(c, scope, false);
    if (!source) {
        block(c, 0x10371e94, 2);
        c.set(3, 0);
    } else {
        block(c, 0x10371e9c, 6);
        c.set({{3, source}, {4, file.strings() + 0x5e}, {5, 0}, {6, 0x16}, {7, 4}});
        c.call(open_file, 0x10371eb4);
        if (!keep_stats_stream(file, 0x10371eb4)) {
            report_stats_file_failure(file, 0x10371ec0, 0, 0x1dd, true);
        } else if (!transfer_stats_header(file, 0x10371f18, read_file)) {
            report_stats_file_failure(file, 0x10371f30, 1, 0x1e4, true);
        } else {
            block(c, 0x10371f88, 3);
            c.set(0, c.u32(file.object() + row_count));
            c.set(3, c.r(0) * row_size);
            c.call(allocate_rows, 0x10371f94);

            block(c, 0x10371f94, 6);
            c.put32(file.object() + stats_rows, c.r(3));
            c.set(4, row_size);
            c.set(3, c.u32(file.object() + stats_rows));
            c.set(5, c.u32(file.object() + row_count));
            c.set(6, c.u32(file.frame + stream_slot));
            c.call(read_file, 0x10371fac);
            if (!transfer_stats_complete(file, 0x10371fac)) {
                report_stats_file_failure(file, 0x10371fb8, 2, 0x1ef, true);
            } else {
                close_stats_stream(file, 0x10372010);
                block(c, 0x10372018, 1);
                c.set(3, 1);
            }
        }
    }
    finish_stats_file(file, 0x1037201c);
}

void save_stats(Cpu& c) {
    GuestNativeScope scope(c.s, save_entry);
    block(c, save_entry, 17);
    auto file = open_stats_frame(c, scope, true);
    c.call(open_file, 0x10372084);
    if (!keep_stats_stream(file, 0x10372084)) {
        report_stats_file_failure(file, 0x10372090, 0, 0x209, false);
    } else if (!transfer_stats_header(file, 0x103720e4, write_file)) {
        report_stats_file_failure(file, 0x103720fc, 1, 0x210, false);
    } else {
        block(c, 0x10372150, 5);
        c.set(3, c.u32(file.object() + stats_rows));
        c.set(4, row_size);
        c.set(5, c.u32(file.object() + row_count));
        c.set(6, c.u32(file.frame + stream_slot));
        c.call(write_file, 0x10372164);
        if (!transfer_stats_complete(file, 0x10372164)) {
            report_stats_file_failure(file, 0x10372170, 2, 0x218, false);
        } else {
            close_stats_stream(file, 0x103721c4);
        }
    }
    finish_stats_file(file, 0x103721cc);
}

void sample_stats_range(Cpu& c) {
    guest::RandomRangeSampler<0x10370830, -0x4dd0, 8>::sample(c, std::int32_t(c.r(3)));
    c.finish();
}

struct Routine {
    std::uint32_t entry;
    void (*run)(Cpu&);
};
constexpr auto routines = std::to_array<Routine>({
#define SIMULATED_STATS_ROUTINE(entry, function) Routine{entry, function},
#include "simulated_stats_routines.inc"
#undef SIMULATED_STATS_ROUTINE
});

bool declining = std::getenv("CM_SIMULATED_STATS_ORIGINAL") != nullptr;

const Routine& find_routine(std::uint32_t entry) {
    const auto found = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (found == routines.end() || found->entry != entry)
        throw std::logic_error("Unknown SIMULATED_STATS.CPP entry " + hex_address(entry));
    return *found;
}
}

void simulated_stats_run_original(State& s, std::uint32_t entry) {
    run_original_body(s, entry, declining);
}

void simulated_stats_run_rewrite(State& s, std::uint32_t entry) {
    Cpu cpu(s);
    find_routine(entry).run(cpu);
}

bool try_modern_simulated_stats(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(s, entry, simulated_stats_run_original, simulated_stats_run_rewrite,
                               RoutineChecker::result_r3);
    simulated_stats_run_rewrite(s, entry);
    return true;
}
}
