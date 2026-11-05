#include "simulated_stats_adapter.hpp"
#include "guest_values.hpp"
#include "guest_range_sampler.hpp"
#include "simulated_stats_detail.hpp"
#include "routine_check.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;
using simulated_stats_detail::RandomPool;
using simulated_stats_detail::RangeDraw;

bool declining = std::getenv("CM_SIMULATED_STATS_ORIGINAL") != nullptr;

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

void call_stats(Cpu& c, std::uint32_t entry, std::uint32_t resume, void (*operation)(Cpu&)) {
    if (declining || routine_checker) {
        c.call(entry, resume);
        return;
    }
    c.link(resume);
    operation(c);
    if (c.s.pc != resume) run_until(c.s, resume);
}

void advance_stats_pool(Cpu& c, const RandomPool& pool, std::uint32_t pc) {
    block(c, pc, 6);
    const auto current = c.u32(pool.cursor);
    c.set(3, current);
    const auto last = c.u32(pool.end);
    const auto next = current + 2;
    c.set({{0, last}, {3, next}});
    const bool wrap = compare_logical(c, next, last) > 0;
    c.put32(pool.cursor, next);
    if (!wrap) return;
    block(c, pc + 24, 2);
    c.put32(pool.cursor, pool.begin);
    c.call(0x1077b770, pc + 32);
    block(c, pc + 32, 2);
    c.set(0, c.r(3) & 0xffff);
    c.put32(pool.salt, c.r(0));
}

constexpr auto initialize_entry = 0x1036ff60u;
constexpr unsigned initialize_frame_size = 0x3f0;

struct InitialStats {
    Cpu& c;
    std::uint32_t object() const { return c.r(31); }
    std::uint32_t list() const { return c.r(15); }
    std::uint32_t strings() const { return c.r(29); }
    std::uint32_t eligible() const { return c.r(16); }
    RandomPool pool() const { return {c.r(22), c.r(23), c.r(24), c.r(25), c.r(26)}; }
};

void check_stats_media(InitialStats& stats) {
    auto& c = stats.c;
    const auto frame = c.sp();
    do {
        block(c, 0x1036ffc4, 15);
        c.set(20, c.u32(c.r(14)));
        c.set(3, frame + 0xf5);
        c.set(12, c.u32(c.r(14) + 4));
        c.set(4, frame + 0x99);
        c.set(11, c.u32(c.r(14) + 8));
        c.set(5, frame + 0x49);
        c.set(10, c.u32(c.r(14) + 12));
        c.set(19, 0);
        c.set(9, c.u8(c.r(14) + 16));
        c.set(8, c.u32(c.r(15)));
        c.set(7, c.u32(c.r(15) + 4));
        c.set(6, c.u32(c.r(15) + 8));
        c.set(0, c.u32(c.r(15) + 12));
        c.put32(frame + 0x98, c.r(20));
        c.put32(frame + 0x9c, c.r(12));
        block(c, 0x10370000, 8);
        c.put32(frame + 0xa0, c.r(11));
        c.put32(frame + 0xa4, c.r(10));
        c.put8(frame + 0xa8, c.r(9));
        c.put32(frame + 0x48, c.r(8));
        c.put32(frame + 0x4c, c.r(7));
        c.put32(frame + 0x50, c.r(6));
        c.put32(frame + 0x54, c.r(0));
        for (;;) {
            block(c, 0x1037003c, 3);
            if (!record_value(c, 0, c.i8(c.r(4)))) break;
            block(c, 0x10370020, 7);
            c.set(6, c.u8(c.r(4)));
            c.set(4, c.r(4) + 1);
            c.set(0, c.u8(c.r(5)));
            c.set(5, c.r(5) + 1);
            c.put8(c.r(3), c.r(6));
            c.put8(c.r(3) + 1, c.r(0));
            c.set(3, c.r(3) + 2);
        }
        block(c, 0x10370048, 9);
        c.set(7, c.u8(frame + 0x98));
        c.set(5, frame + 0xf4);
        c.set(0, c.u8(frame + 0x48));
        c.set({{6, frame + 0xac}, {3, 0}, {4, 0}, {0, c.r(7) + c.r(0)}});
        c.put8(frame + 0xf4, c.r(0));
        c.call(0x1077fee0, 0x1037006c);
        block(c, 0x1037006c, 3);
        c.set(2, c.u32(frame + 0x14));
        if (!record_value(c, 0, std::int16_t(c.r(3)))) {
            block(c, 0x10370078, 1);
            c.set(19, 1);
        }
        block(c, 0x1037007c, 2);
        if (!compare(c, std::int32_t(c.r(19)))) {
            block(c, 0x10370084, 3);
            c.set({{4, stats.strings()}, {3, frame + 0x2bc}});
            c.call(0x10727890, 0x10370090);
            block(c, 0x10370090, 3);
            c.set({{3, frame + 0x1f4}, {4, stats.strings() + 10}});
            c.call(0x10727890, 0x1037009c);
            block(c, 0x1037009c, 4);
            c.set({{3, frame + 0x2bc}, {4, frame + 0x1f4}, {5, 1}});
            c.call(0x10769b60, 0x103700ac);
        }
        block(c, 0x103700ac, 2);
    } while (!compare(c, std::int32_t(c.r(19))));
}

void report_stats_initialization(InitialStats& stats, bool allocation) {
    auto& c = stats.c;
    const auto pc = allocation ? 0x103700e4u : 0x10370744u;
    const auto message = c.sp() + (allocation ? 0x78 : 0x58);
    const auto detail = c.sp() + (allocation ? 0x40 : 0x38);
    block(c, pc, 6);
    c.set({{3, stats.strings() + 0x37}, {6, message}, {7, detail}, {4, 0}, {5, 0}});
    c.call(0x10739890, pc + 24);
    block(c, pc + 24, 1);
    c.link(pc + 28);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, pc + 28, 7);
    c.set({{5, c.r(3)}, {3, c.r(28)}, {4, stats.strings() + 0x4b},
           {6, message}, {7, detail}, {8, allocation ? 0x49u : 0x95u}});
    c.call(0x1077b720, pc + 56);
    block(c, pc + 56, 4);
    c.set({{4, c.r(28)}, {3, stats.strings() + 0x58}, {5, 0}});
    c.call(0x10769b60, pc + 72);
    block(c, pc + 72, allocation ? 4 : 2);
    c.set(0, 0);
    if (allocation) c.set(3, UINT32_MAX);
    c.put8(c.r(27), 0);
    if (allocation) c.call(0x1076f7d0, 0x1037013c);
}

bool stats_competition_selected(Cpu& c, std::uint32_t club, std::uint32_t competitions_slot,
                                unsigned club_field, std::uint32_t pc) {
    block(c, pc, 3);
    const auto competition = c.u32(club + club_field);
    c.set(3, competition);
    if (!compare_logical(c, competition)) return false;
    block(c, pc + 12, 6);
    const auto id = c.u32(competition);
    c.set(0, id);
    const auto competitions = c.u32(competitions_slot);
    c.set({{3, competitions}, {0, id * 4}});
    const auto selected = c.u32(competitions + id * 4);
    c.set(0, selected);
    return compare_logical(c, selected) != 0;
}

void collect_stats_clubs(InitialStats& stats) {
    auto& c = stats.c;
    block(c, 0x1037013c, 4);
    const auto clubs_slot = c.r(17), count_slot = c.r(21), competitions_slot = c.r(18);
    // These locals cross guest callbacks, whose stack frames expose the saved
    // values. Kept retains that boundary while naming the collection algorithm.
    guest::Kept<20> club(c, c.u32(clubs_slot));
    guest::Kept<19> index(c, 0);
    guest::Kept<14> club_offset(c, 0);
    guest::Kept<13> list_offset(c, c.r(13));
    for (;;) {
        block(c, 0x103701d4, 3);
        c.set(0, c.u32(count_slot));
        if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) >= 0) break;
        if (!stats_competition_selected(c, club, competitions_slot, 0x30, 0x10370150)
                && !stats_competition_selected(c, club, competitions_slot, 0x38, 0x10370174)) {
            block(c, 0x10370198, 5);
            const auto service = c.toc(-0x7a64);
            c.set(3, service);
            c.set(0, c.u32(clubs_slot));
            c.set(3, c.u32(service));
            c.set(4, c.r(0) + club_offset);
            c.call(0x102b7c00, 0x103701ac);
            block(c, 0x103701ac, 2);
            if (!record_value(c, 0, std::int8_t(c.r(3)))) {
                block(c, 0x103701b4, 5);
                const auto clubs = c.u32(clubs_slot);
                c.set({{3, clubs}, {16, stats.eligible() + 1}});
                const auto id = c.u32(clubs + club_offset);
                c.set(0, id);
                c.put32(stats.list() + list_offset, id);
                list_offset = list_offset + 4;
            }
        }
        block(c, 0x103701c8, 3);
        club_offset = club_offset + club_stride;
        index = index + 1;
        club = club + club_stride;
    }
    block(c, 0x103701e0, 9);
    const auto count = std::int32_t(stats.eligible());
    const auto sign = stats.eligible() >> 31;
    const auto rotated = std::rotl((stats.eligible() << 28) - sign, 4);
    const auto quotient = count / 16;
    const auto shift = std::uint32_t(count >> 4);
    const bool carry = count < 0 && (stats.eligible() & 15);
    c.set({{0, rotated}, {3, sign}, {4, std::uint32_t(quotient)}});
    c.s.wr<1>(guest::reg::xer_ca, std::uint64_t(shift) + unsigned(carry) > UINT32_MAX);
    const auto remainder = rotated + sign;
    const bool partial = record_value(c, 14, std::int32_t(remainder)) != 0;
    c.put32(stats.object(), std::uint32_t(quotient));
    if (partial) {
        block(c, 0x10370204, 3);
        c.set(3, c.u32(stats.object()));
        c.set(0, c.r(3) + 1);
        c.put32(stats.object(), c.r(0));
    }
}

void sort_stats_clubs(InitialStats& stats) {
    auto& c = stats.c;
    const auto clubs_slot = c.r(17);
    const auto count = stats.eligible();
    block(c, 0x10370210, 4);
    std::uint32_t position = 0, current = stats.list();
    const auto last = count - 1;
    c.set({{5, current}, {7, last}, {3, position}});
    for (;;) {
        block(c, 0x10370288, 2);
        if (compare(c, std::int32_t(position), std::int32_t(last)) >= 0) break;
        block(c, 0x10370220, 7);
        auto candidate = position + 1;
        auto displacement = candidate * 4;
        auto next = stats.list() + displacement;
        auto remaining = count - candidate;
        c.set({{4, candidate}, {10, displacement}, {0, remaining}, {6, next}});
        c.s.wr<4>(guest::reg::ctr, remaining);
        if (compare(c, std::int32_t(candidate), std::int32_t(count)) < 0) {
            do {
                block(c, 0x1037023c, 11);
                const auto first = c.u32(current);
                c.set(12, first);
                const auto second = c.u32(next);
                c.set({{13, second}, {9, first * club_stride}});
                const auto clubs = c.u32(clubs_slot);
                c.set({{11, clubs}, {8, second * club_stride}, {0, first * club_stride + 0x1ec}});
                const auto first_rating = c.i16(clubs + first * club_stride + 0x1ec);
                c.set({{9, std::uint32_t(first_rating)}, {0, second * club_stride + 0x1ec}});
                const auto second_rating = c.i16(clubs + second * club_stride + 0x1ec);
                c.set(0, std::uint32_t(second_rating));
                if (compare(c, first_rating, second_rating) < 0) {
                    block(c, 0x10370268, 2);
                    c.put32(current, second);
                    c.put32(next, first);
                }
                block(c, 0x10370270, 4);
                displacement += 4;
                next += 4;
                ++candidate;
                c.set({{10, displacement}, {6, next}, {4, candidate}});
                c.s.wr<4>(guest::reg::ctr, --remaining);
            } while (remaining);
        }
        block(c, 0x10370280, 2);
        current += 4;
        ++position;
        c.set({{5, current}, {3, position}});
    }
}

void shuffle_stats_clubs(InitialStats& stats) {
    auto& c = stats.c;
    block(c, 0x10370290, 7);
    const auto count = stats.eligible();
    c.set({{3, 0x1b4e81b5}, {0, count >> 31}, {20, 0},
           {30, std::uint32_t((INT64_C(0x1b4e81b5) * std::int32_t(count)) >> 32)},
           {13, count + (count >> 31)}});
    guest::Kept<20> swaps(c, 0);
    const guest::Kept<19> anchor(c, c.r(19));
    const guest::Kept<18> forward(c, c.r(18));
    const guest::Kept<17> backward(c, c.r(17));
    const guest::Kept<21> distance(c, c.r(21));
    for (;;) {
        block(c, 0x103706f0, 3);
        const auto half_input = std::int32_t(c.r(13));
        c.s.wr<1>(guest::reg::xer_ca, half_input < 0 && (c.r(13) & 1));
        c.set(0, std::uint32_t(half_input >> 1));
        if (compare(c, std::int32_t(std::uint32_t(swaps)), std::int32_t(c.r(0))) >= 0) break;
        block(c, 0x103702ac, 2);
        if (!compare(c, std::int32_t(count))) {
            block(c, 0x103702b4, 2);
            c.set(19, 0);
        } else {
            advance_stats_pool(c, stats.pool(), 0x103702bc);
            simulated_stats_detail::draw_range(c, stats.pool(), {0x103702e4, 19, 0x394, 0x38c, 0x384}, count);
        }
        block(c, 0x10370404, 1);
        for (;;) {
            block(c, 0x10370408, 4);
            const auto scaled = std::int32_t(c.r(30));
            const auto range = std::uint32_t(scaled / 16);
            c.s.wr<1>(guest::reg::xer_ca, scaled < 0 && (c.r(30) & 15));
            c.set({{0, std::uint32_t(scaled >> 4)}, {3, unsigned(scaled < 0)}});
            if (!record_value(c, 21, std::int32_t(range))) {
                block(c, 0x10370418, 2);
                c.set(18, 0);
            } else {
                advance_stats_pool(c, stats.pool(), 0x10370420);
                simulated_stats_detail::draw_range(c, stats.pool(), {0x10370448, 18, 0x384, 0x38c, 0x394}, range);
            }
            block(c, 0x10370568, 2);
            if (!compare(c, std::int32_t(std::uint32_t(distance)))) {
                block(c, 0x10370570, 2);
                c.set(17, 0);
            } else {
                advance_stats_pool(c, stats.pool(), 0x10370578);
                simulated_stats_detail::draw_range(c, stats.pool(), {0x103705a0, 17, 0x384, 0x38c, 0x394}, distance);
            }
            block(c, 0x103706c0, 3);
            const auto destination = anchor + forward - backward;
            if (record_value(c, 0, std::int32_t(destination)) < 0) continue;
            block(c, 0x103706cc, 2);
            if (compare(c, std::int32_t(destination), std::int32_t(count)) < 0) break;
        }
        block(c, 0x103706d4, 7);
        const auto from = anchor * 4, to = c.r(0) * 4;
        c.set({{4, from}, {3, to}});
        const auto first = c.u32(stats.list() + from);
        c.set(5, first);
        swaps = swaps + 1;
        const auto second = c.u32(stats.list() + to);
        c.set(0, second);
        c.put32(stats.list() + from, second);
        c.put32(stats.list() + to, first);
    }
}

void fill_stats_groups(InitialStats& stats) {
    auto& c = stats.c;
    block(c, 0x103706fc, 3);
    c.set(0, c.u32(stats.object()));
    c.set(3, c.r(0) * row_size);
    c.call(allocate_rows, 0x10370708);
    block(c, 0x10370708, 6);
    guest::Kept<17> row_offset(c, 0);
    c.put32(stats.object() + stats_rows, c.r(3));
    guest::Kept<18> input_offset(c, 0);
    guest::Kept<19> consumed(c, 0);
    guest::Kept<20> group(c, 0);
    guest::Kept<21> member(c, c.r(21));
    guest::Kept<22> input(c, c.r(22));
    guest::Kept<13> scratch_offset(c, c.r(13));
    for (;;) {
        block(c, 0x10370800, 3);
        c.set(0, c.u32(stats.object()));
        if (compare(c, std::int32_t(std::uint32_t(group)), std::int32_t(c.r(0))) >= 0) break;
        block(c, 0x10370720, 7);
        c.set(3, c.u32(stats.object() + stats_rows));
        c.set(0, 0);
        scratch_offset = input_offset;
        input = stats.list() + input_offset;
        c.put32(c.r(3) + row_offset, 0);
        member = 0;
        for (;;) {
            block(c, 0x1037073c, 2);
            if (!compare(c, std::int32_t(std::uint32_t(consumed)), std::int32_t(stats.eligible())))
                report_stats_initialization(stats, false);
            block(c, 0x10370794, 17);
            const auto rows = c.u32(stats.object() + stats_rows);
            c.set(0, rows);
            const bool partial = compare(c, std::int32_t(c.r(14))) != 0;
            const auto club_id = c.u32(input);
            c.set(4, club_id);
            scratch_offset = scratch_offset + 4;
            const auto row = rows + row_offset;
            c.set(5, row);
            input = input + 4;
            const auto count = c.u32(row);
            c.set(0, count);
            input_offset = input_offset + 4;
            consumed = consumed + 1;
            c.set({{3, count * 4}, {0, count * 4 + 4}});
            c.put32(row + count * 4 + 4, club_id);
            // Retain the fresh object-pointer and count reads after the write.
            const auto live_rows = c.u32(stats.object() + stats_rows);
            c.set(4, live_rows);
            const auto live_count = c.u32(live_rows + row_offset);
            c.set({{3, live_count}, {0, live_count + 1}});
            c.put32(live_rows + row_offset, live_count + 1);
            if (partial) {
                block(c, 0x103707d8, 3);
                c.set(0, 16 - c.r(14));
                c.s.wr<1>(guest::reg::xer_ca, c.r(14) <= 16);
                if (compare(c, std::int32_t(std::uint32_t(group)), std::int32_t(c.r(0))) < 0) {
                    block(c, 0x103707e4, 2);
                    if (!compare(c, std::int32_t(std::uint32_t(member)), 14)) break;
                }
            }
            block(c, 0x103707ec, 3);
            member = member + 1;
            if (compare(c, std::int32_t(std::uint32_t(member)), 16) >= 0) break;
        }
        block(c, 0x103707f8, 2);
        row_offset = row_offset + row_size;
        group = group + 1;
    }
    block(c, 0x1037080c, 2);
    c.set(3, stats.list());
    c.call(free_stats_rows, 0x10370814);
    block(c, 0x10370814, 1);
    c.set(3, stats.object());
}

void initialize_stats(Cpu& c) {
    GuestNativeScope scope(c.s, initialize_entry);
    block(c, initialize_entry, 25);
    const auto stack = c.sp(), frame = stack - initialize_frame_size;
    c.set(0, c.lr());
    guest::transfer_saved_registers<13>(c, stack);
    c.set(5, c.toc(-0x745c));
    c.set(31, c.r(3));
    c.set(22, c.toc(-0x4dd0));
    c.set(30, c.r(4));
    c.set(23, c.toc(-0x7b90));
    c.set(16, 0);
    c.set(24, c.toc(-0x7b8c));
    c.set(13, 0);
    c.set(25, c.toc(-0x7b88));
    c.set(26, c.toc(-0x7b84));
    c.set(18, c.toc(-0x7b5c));
    c.set(17, c.toc(-0x7be8));
    c.set(27, c.toc(-0x7bac));
    c.set(28, c.toc(-0x7ba8));
    c.set(21, c.toc(-0x7b50));
    c.set(29, c.toc(-0x4dcc));
    c.set(15, c.toc(-0x4dc8));
    c.set(14, c.toc(-0x4dc4));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    InitialStats stats{c};
    if (!record_value(c, 0, c.i8(c.r(5)))) check_stats_media(stats);
    block(c, 0x103700b4, 2);
    if (compare_logical(c, c.r(30))) {
        block(c, 0x103700bc, 3);
        c.set({{3, stats.object()}, {4, c.r(30)}});
        call_stats(c, load_entry, 0x103700c8, load_stats);
        block(c, 0x103700c8, 2);
        c.set(3, stats.object());
    } else {
        block(c, 0x103700d0, 3);
        c.set(0, c.u32(c.r(21)));
        c.set(3, c.r(0) * 4);
        c.call(allocate_rows, 0x103700dc);
        block(c, 0x103700dc, 2);
        if (!record_value(c, 15, std::int32_t(c.r(3)))) report_stats_initialization(stats, true);
        collect_stats_clubs(stats);
        sort_stats_clubs(stats);
        shuffle_stats_clubs(stats);
        fill_stats_groups(stats);
    }
    block(c, 0x10370818, 5);
    c.set(0, c.u32(frame + initialize_frame_size + 8));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<13, true>(c, stack);
    c.finish();
}

constexpr auto update_entry = 0x10370e10u;
constexpr unsigned update_frame_size = 0x190;

struct StatsUpdate {
    Cpu& c;
    std::uint32_t object() const { return c.r(31); }
    std::uint32_t date() const { return c.r(21); }
    std::uint32_t text() const { return c.r(22); }
    std::uint32_t strings() const { return c.r(28); }
    std::uint32_t clubs_slot() const { return c.r(13); }
    std::uint32_t match_service_slot() const { return c.r(14); }
    std::uint32_t row_offset() const { return c.r(17); }
    std::uint32_t scores() const { return c.sp() + 0x58; }
    std::uint32_t club_ids() const { return c.sp() + 0x88; }
    std::uint32_t match() const { return c.sp() + 0xc8; }
    RandomPool pool() const { return {c.r(23), c.r(24), c.r(25), c.r(26), c.r(27)}; }
};

bool choose_update_distance(StatsUpdate& update, std::uint32_t pc) {
    auto& c = update.c;
    block(c, pc, 6);
    const auto count = c.i32(update.object());
    c.set(0, std::uint32_t(count));
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(15))) * count) >> 32);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 15));
    c.set({{0, std::uint32_t(high >> 4)}, {3, unsigned(high < 0)}});
    return record_value(c, 18, count / 150) != 0;
}

void draw_update_range(StatsUpdate& update, std::uint32_t site, unsigned result,
                       std::int32_t bound, bool first = false, bool correction_in_r3 = true) {
    advance_stats_pool(update.c, update.pool(), site - 40);
    simulated_stats_detail::draw_range(update.c, update.pool(),
        {site, result, first ? 0x128u : 0x118u, 0x120, first ? 0x118u : 0x128u, correction_in_r3}, bound);
}

void shuffle_stats_rows(StatsUpdate& update) {
    auto& c = update.c;
    block(c, 0x10370e78, 4);
    c.set({{3, 0x1b4f0000}, {15, 0x1b4e81b5}});
    guest::Kept<20> swaps(c, 0);
    const guest::Kept<17> anchor(c, c.r(17));
    const guest::Kept<16> destination(c, c.r(16));
    const guest::Kept<18> distance(c, c.r(18));
    const guest::Kept<19> backward(c, c.r(19));
    block(c, 0x103715c4, 3);
    for (;;) {
        const auto count = c.u32(update.object());
        c.set(16, count);
        if (compare(c, std::int32_t(std::uint32_t(swaps)), std::int32_t(count)) >= 0) break;
        block(c, 0x10370e88, 2);
        if (!compare(c, std::int32_t(count))) {
            block(c, 0x10370e90, 2);
            c.set(17, 0);
        } else {
            draw_update_range(update, 0x10370ec0, 17, count, true);
        }
        for (;;) {
            if (!choose_update_distance(update, 0x10370fe0)) {
                block(c, 0x10370ff8, 2);
                c.set(16, 0);
            } else {
                draw_update_range(update, 0x10371028, 16, distance);
            }
            if (!choose_update_distance(update, 0x10371148)) {
                block(c, 0x10371160, 2);
                c.set(19, 0);
            } else {
                draw_update_range(update, 0x10371190, 19, distance);
            }
            block(c, 0x103712b0, 3);
            c.set(0, anchor + destination);
            if (record_value(c, 16, std::int32_t(c.r(0) - backward)) < 0) continue;
            block(c, 0x103712bc, 3);
            c.set(0, c.u32(update.object()));
            if (compare(c, std::int32_t(std::uint32_t(destination)), std::int32_t(c.r(0))) < 0) break;
        }
        block(c, 0x103712c8, 5);
        c.set(29, anchor * row_size);
        c.set(3, c.u32(update.object() + stats_rows));
        c.set(17, c.u32(c.r(3) + c.r(29)));
        if (!compare(c, std::int32_t(std::uint32_t(anchor)))) {
            block(c, 0x103712dc, 2);
            c.set(18, 0);
        } else {
            draw_update_range(update, 0x1037130c, 18, anchor);
        }
        block(c, 0x1037142c, 5);
        const auto destination_row = destination * row_size;
        c.set(17, destination_row);
        c.set(3, c.u32(update.object() + stats_rows));
        c.set(16, c.u32(c.r(3) + c.r(17)));
        if (!compare(c, std::int32_t(std::uint32_t(destination)))) {
            block(c, 0x10371440, 2);
            c.set(19, 0);
        } else {
            draw_update_range(update, 0x10371470, 19, destination);
        }
        block(c, 0x10371590, 16);
        c.set({{0, distance * 4}, {3, backward * 4}});
        const auto first = c.r(29) + c.r(0) + 4;
        c.set(4, c.r(29) + c.r(0));
        const auto rows = c.u32(update.object() + stats_rows);
        c.set({{5, rows}, {6, first}, {3, destination_row + c.r(3)}});
        const auto second = c.r(3) + 4;
        c.set(4, second);
        const auto first_club = c.u32(rows + first);
        c.set(7, first_club);
        const auto second_club = c.u32(rows + second);
        c.set(0, second_club);
        swaps = swaps + 1;
        c.put32(rows + first, second_club);
        const auto live_rows = c.u32(update.object() + stats_rows);
        c.set(3, live_rows);
        c.put32(live_rows + second, first_club);
    }
}

bool stats_group_due(StatsUpdate& update) {
    auto& c = update.c;
    block(c, 0x103715ec, 22);
    const auto reciprocal = c.u32(c.sp() + 0x134);
    c.set(3, reciprocal);
    const auto index = c.u32(c.sp() + 0x130);
    c.set(0, index);
    const auto day = c.i16(update.date());
    c.set(4, std::uint32_t(day));
    const auto high_index = std::int32_t((std::int64_t(std::int32_t(reciprocal)) * std::int32_t(index)) >> 32);
    const auto high_day = std::int32_t((std::int64_t(std::int32_t(reciprocal)) * day) >> 32);
    c.set({{5, std::uint32_t(high_index)}, {0, reciprocal}, {3, std::uint32_t(high_day)}});
    const auto live_index = c.u32(c.sp() + 0x130);
    const auto index_sum = std::int32_t(std::uint32_t(high_index) + live_index);
    const auto day_sum = std::int32_t(std::uint32_t(high_day) + std::uint32_t(day));
    const auto index_quotient = std::int32_t(live_index) / 7;
    const auto day_quotient = day / 7;
    c.s.wr<1>(guest::reg::xer_ca, day_sum < 0 && (std::uint32_t(day_sum) & 3));
    c.set({{5, std::uint32_t(index_quotient)}, {0, std::uint32_t(day_quotient)},
           {6, unsigned(index_sum < 0)}, {3, std::uint32_t(day_quotient * 7)}});
    const auto final_index = c.u32(c.sp() + 0x130);
    c.set({{5, final_index - std::uint32_t(index_quotient * 7)},
           {0, std::uint32_t(day - day_quotient * 7)}});
    if (compare(c, std::int32_t(c.r(5)), std::int32_t(c.r(0))) != 0) return false;
    block(c, 0x10371644, 2);
    c.set(3, update.date());
    c.call(0x10718360, 0x1037164c);
    block(c, 0x1037164c, 3);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), 4) <= 0) return true;
    block(c, 0x10371658, 2);
    c.set(3, update.date());
    c.call(0x10718360, 0x10371660);
    block(c, 0x10371660, 3);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0)), 7) >= 0;
}

void assess_stats_clubs(StatsUpdate& update) {
    auto& c = update.c;
    block(c, 0x1037166c, 6);
    guest::Kept<16> member(c, 0);
    guest::Kept<29> score(c, update.scores());
    guest::Kept<18> byte_offset(c, 0);
    guest::Kept<30> id(c, update.club_ids());
    guest::Kept<15> eligible(c, 0);
    for (;;) {
        block(c, 0x10371758, 5);
        c.set(0, c.u32(update.object() + stats_rows));
        const auto row = update.row_offset() + c.r(0);
        c.set(3, row);
        c.set(0, c.u32(row));
        if (compare(c, std::int32_t(std::uint32_t(member)), std::int32_t(c.r(0))) >= 0) break;
        block(c, 0x10371684, 7);
        c.set({{0, byte_offset + 4}, {4, update.date()}});
        c.set(0, c.u32(row + c.r(0)));
        c.set({{3, c.sp() + 0x48}, {5, 2}});
        c.put32(id, c.r(0));
        c.call(0x10717b20, 0x103716a0);
        block(c, 0x103716a0, 6);
        c.set(20, std::uint32_t(c.i16(c.sp() + 0x48)));
        c.set(4, update.date());
        c.set(19, c.u16(c.sp() + 0x4a));
        c.set({{3, c.sp() + 0x40}, {5, 3}});
        c.call(0x10717890, 0x103716b8);
        block(c, 0x103716b8, 15);
        c.set({{3, std::uint32_t(-2)}, {0, UINT32_MAX}});
        c.put32(c.sp() + 0x38, c.r(3));
        c.set(5, c.r(19));
        c.set(3, c.toc(-0x7a60));
        c.set(4, c.r(20) & 0xffff);
        c.put32(c.sp() + 0x3c, c.r(0));
        c.set(9, 0);
        c.set(8, c.u32(id));
        c.set(10, UINT32_MAX);
        c.set(0, std::uint32_t(c.i16(c.sp() + 0x40)));
        c.set(3, c.u32(c.r(3)));
        c.set(7, c.u16(c.sp() + 0x42));
        c.set(6, c.r(0) & 0xffff);
        c.call(0x1048b180, 0x103716f4);
        block(c, 0x103716f4, 2);
        bool available = record_value(c, 0, std::int32_t(c.r(3) & 0xffff)) == 0;
        if (available) {
            block(c, 0x103716fc, 7);
            const auto club_id = c.u32(id);
            c.set(3, club_id);
            const auto clubs = c.u32(update.clubs_slot());
            c.set({{5, clubs}, {4, club_id * club_stride}, {0, club_id * club_stride + 0x2c}});
            c.set(0, c.u32(clubs + c.r(0)));
            available = compare_logical(c, c.r(0)) != 0;
        }
        if (available) {
            block(c, 0x10371728, 1);
            call_stats(c, count_entry, 0x1037172c, count_squad_candidates);
            block(c, 0x1037172c, 2);
            c.set(4, update.scores());
            c.put8(update.scores() + member, c.r(3));
        } else {
            block(c, 0x10371718, 4);
            c.set({{3, update.scores()}, {0, UINT32_MAX}});
            c.put8(update.scores() + member, c.r(0));
        }
        block(c, 0x10371734, 3);
        if (record_value(c, 0, c.i8(score)) > 0) {
            block(c, 0x10371740, 2);
            c.set(0, eligible + 1);
            eligible = std::uint32_t(std::int8_t(c.r(0)));
        }
        block(c, 0x10371748, 4);
        byte_offset = byte_offset + 4;
        id = id + 4;
        member = member + 1;
        score = score + 1;
    }
}

void sort_stats_candidates(StatsUpdate& update) {
    auto& c = update.c;
    block(c, 0x1037176c, 7);
    const auto scores = update.scores(), ids = update.club_ids();
    std::uint32_t position = 0, displacement = 0, current_score = scores, current_id = ids;
    c.set({{0, scores}, {5, ids}, {9, scores}, {8, ids}, {6, 0}, {4, 0}});
    for (;;) {
        block(c, 0x103717fc, 5);
        c.set(3, c.u32(update.object() + stats_rows));
        c.set(3, c.u32(c.r(3) + update.row_offset()));
        c.set(3, c.r(3) - 1);
        if (compare(c, std::int32_t(position), std::int32_t(c.r(3))) >= 0) break;
        block(c, 0x10371788, 5);
        auto candidate = position + 1;
        auto next_offset = candidate * 4;
        auto next_score = scores + candidate;
        auto next_id = ids + next_offset;
        c.set({{7, candidate}, {3, next_offset}, {11, next_score}, {10, next_id}});
        for (;;) {
            block(c, 0x103717dc, 4);
            c.set(12, c.u32(update.object() + stats_rows));
            c.set(12, c.u32(c.r(12) + update.row_offset()));
            if (compare(c, std::int32_t(candidate), std::int32_t(c.r(12))) >= 0) break;
            block(c, 0x1037179c, 6);
            const auto first = c.u8(current_score);
            c.set(12, first);
            const auto second = c.u8(next_score);
            c.set({{18, second}, {16, std::uint32_t(std::int8_t(first))}, {12, std::uint32_t(std::int8_t(second))}});
            if (compare(c, std::int8_t(first), std::int8_t(second)) < 0) {
                block(c, 0x103717b4, 6);
                const auto first_id = c.u32(current_id);
                c.set(19, first_id);
                const auto second_id = c.u32(next_id);
                c.set(12, second_id);
                c.put8(current_score, second);
                c.put32(current_id, second_id);
                c.put8(next_score, std::uint32_t(std::int8_t(first)));
                c.put32(next_id, first_id);
            }
            block(c, 0x103717cc, 4);
            next_offset += 4;
            next_id += 4;
            ++candidate;
            ++next_score;
            c.set({{3, next_offset}, {10, next_id}, {7, candidate}, {11, next_score}});
        }
        block(c, 0x103717ec, 4);
        displacement += 4;
        current_id += 4;
        ++position;
        ++current_score;
        c.set({{4, displacement}, {8, current_id}, {6, position}, {9, current_score}});
    }
}

void shuffle_stats_candidates(StatsUpdate& update) {
    auto& c = update.c;
    block(c, 0x10371810, 6);
    c.set(18, std::uint32_t(std::int8_t(c.r(15))));
    c.set(19, update.scores());
    c.set(29, c.r(18) * c.r(18));
    c.set(20, update.club_ids());
    guest::Kept<30> swaps(c, 0);
    const guest::Kept<18> count(c, c.r(18));
    const guest::Kept<16> first(c, c.r(16));
    const guest::Kept<15> second(c, c.r(15));
    for (;;) {
        block(c, 0x10371b0c, 2);
        if (compare(c, std::int32_t(std::uint32_t(swaps)), std::int32_t(c.r(29))) >= 0) break;
        block(c, 0x10371b14, 2);
        if (compare(c, std::int32_t(std::uint32_t(count)), 2) <= 0) break;
        block(c, 0x1037182c, 2);
        if (!compare(c, std::int32_t(std::uint32_t(count)))) {
            block(c, 0x10371834, 2);
            c.set(16, 0);
        } else {
            draw_update_range(update, 0x10371864, 16, count, false, false);
        }
        block(c, 0x10371984, 2);
        if (!compare(c, std::int32_t(std::uint32_t(count)))) {
            block(c, 0x1037198c, 2);
            c.set(15, 0);
        } else {
            draw_update_range(update, 0x103719bc, 15, count, false, false);
        }
        block(c, 0x10371adc, 12);
        c.set({{4, first * 4}, {3, second * 4}});
        c.set(7, c.u8(update.scores() + first));
        swaps = swaps + 1;
        c.set(5, c.u8(update.scores() + second));
        c.set(6, c.u32(update.club_ids() + first * 4));
        c.set(7, std::uint32_t(std::int8_t(c.r(7))));
        c.set(0, c.u32(update.club_ids() + second * 4));
        c.put8(update.scores() + first, c.r(5));
        c.put32(update.club_ids() + first * 4, c.r(0));
        c.put8(update.scores() + second, c.r(7));
        c.put32(update.club_ids() + second * 4, c.r(6));
    }
}

void prepare_stats_match(StatsUpdate& update, std::uint32_t byte_offset) {
    auto& c = update.c;
    const auto frame = c.sp();
    block(c, 0x10371b2c, 47);
    c.set({{3, update.club_ids() + byte_offset}, {7, UINT32_MAX}, {11, 0}});
    c.set(12, c.u32(c.r(3)));
    c.set(9, 1);
    c.set(19, c.u32(c.r(3) + 4));
    c.set(5, std::uint32_t(-2));
    c.set(0, c.u8(frame + 0x116));
    c.set({{0, c.r(0) & 15}, {8, c.r(12) * club_stride}});
    const auto clubs = c.u32(update.clubs_slot());
    c.set(6, clubs);
    c.put8(frame + 0x116, c.r(0));
    c.set(0, 2);
    c.set(3, c.u32(update.match_service_slot()));
    c.set({{4, update.match()}, {20, c.r(19) * club_stride}});
    c.set(10, c.u8(frame + 0x116));
    c.set(10, c.r(10) & 0xf0);
    c.put32(frame + 0xc8, c.r(5));
    c.set({{8, clubs + c.r(8)}, {20, clubs + c.r(20)}});
    c.put32(frame + 0xdc, 0);
    c.set({{5, std::uint32_t(-2)}, {6, 0}});
    c.put8(frame + 0x116, c.r(10));
    c.put8(frame + 0x108, 1);
    c.put8(frame + 0x109, 1);
    c.put32(frame + 0xd4, c.r(12));
    c.put32(frame + 0xd8, c.r(19));
    c.put32(frame + 0xe4, c.r(8));
    c.put32(frame + 0xe8, c.r(20));
    c.put32(frame + 0xe0, 0);
    c.put32(frame + 0xcc, UINT32_MAX);
    c.put8(frame + 0x104, 0);
    c.put8(frame + 0x102, 2);
    c.put8(frame + 0x10b, 0);
    c.put8(frame + 0x10c, 0);
    for (unsigned field = 0x10d; field <= 0x114; ++field) c.put8(frame + field, 0xff);
    c.call(0x10430090, 0x10371be8);
}

void report_stats_match_allocation(StatsUpdate& update) {
    auto& c = update.c;
    const auto frame = c.sp();
    block(c, 0x10371c04, 6);
    c.set({{3, update.strings() + 0x37}, {6, frame + 0x68}, {7, frame + 0x50}, {4, 0}, {5, 0}});
    c.call(0x10739890, 0x10371c1c);
    block(c, 0x10371c1c, 1);
    c.link(0x10371c20);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, 0x10371c20, 7);
    c.set(5, c.r(3));
    c.set(3, c.toc(-0x7ba8));
    c.set({{4, update.strings() + 0x4b}, {6, frame + 0x68}, {7, frame + 0x50}, {8, 0x15e}});
    c.call(0x1077b720, 0x10371c3c);
    block(c, 0x10371c3c, 4);
    c.set(4, c.toc(-0x7ba8));
    c.set({{3, update.strings() + 0x58}, {5, 0}});
    c.call(0x10769b60, 0x10371c4c);
    block(c, 0x10371c4c, 4);
    c.set(3, c.toc(-0x7bac));
    c.set(0, 0);
    c.put8(c.r(3), 0);
}

void run_stats_match(StatsUpdate& update, std::uint32_t byte_offset) {
    auto& c = update.c;
    prepare_stats_match(update, byte_offset);
    block(c, 0x10371be8, 2);
    c.set(3, c.r(18) - 0x1f44);
    c.call(0x10773520, 0x10371bf0);
    block(c, 0x10371bf0, 2);
    if (record_value(c, 19, std::int32_t(c.r(3)))) {
        block(c, 0x10371bf8, 1);
        c.call(0x101d16a0, 0x10371bfc);
    }
    block(c, 0x10371bfc, 2);
    if (!compare_logical(c, c.r(19))) {
        report_stats_match_allocation(update);
        return;
    }
    block(c, 0x10371c5c, 6);
    c.set({{3, c.r(19)}, {4, update.match()}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x101d1760, 0x10371c74);
    block(c, 0x10371c74, 2);
    if (!record_value(c, 0, std::int8_t(c.r(3)))) return;
    block(c, 0x10371c7c, 3);
    c.set({{3, c.r(19)}, {4, UINT32_MAX}});
    c.call(0x101d3f60, 0x10371c88);
    block(c, 0x10371c88, 6);
    c.set(3, c.toc(-0x7b4c));
    c.set({{4, c.r(19)}, {5, update.text()}, {6, 0}});
    c.set(3, c.u32(c.r(3)));
    c.call(0x1029f920, 0x10371ca0);
    constexpr std::array<std::array<std::uint32_t, 3>, 5> listeners{{
        {0x10371ca0, std::uint32_t(-0x7b3c), 0x103024a0},
        {0x10371cb4, std::uint32_t(-0x7b7c), 0x1007c4e0},
        {0x10371cc8, std::uint32_t(-0x7ba0), 0x10186fc0},
        {0x10371cdc, std::uint32_t(-0x7bb4), 0x104363e0},
        {0x10371cf0, std::uint32_t(-0x7b40), 0x103e8d30}
    }};
    for (const auto& [pc, slot, target] : listeners) {
        block(c, pc, 5);
        c.set(3, c.toc(std::int32_t(slot)));
        c.set({{5, update.text()}, {4, update.match()}});
        c.set(3, c.u32(c.r(3)));
        c.call(target, pc + 20);
    }
    block(c, 0x10371d04, 3);
    c.set({{3, c.r(19)}, {4, 1}});
    c.call(0x101d3e50, 0x10371d10);
    block(c, 0x10371d10, 3);
    c.set(3, c.u32(update.match_service_slot()));
    c.set(4, update.match());
    c.call(0x10430550, 0x10371d1c);
    block(c, 0x10371d1c, 2);
    c.set({{15, c.r(15) + 8}, {16, c.r(16) + 2}});
}

void process_stats_pairs(StatsUpdate& update) {
    auto& c = update.c;
    block(c, 0x10371b1c, 4);
    const guest::Kept<16> pair(c, 0);
    const guest::Kept<15> byte_offset(c, 0);
    c.set(18, 0x10000);
    for (;;) {
        block(c, 0x10371d24, 5);
        c.set(0, c.u32(update.object() + stats_rows));
        c.set(3, c.u32(c.r(0) + update.row_offset()));
        c.set(0, c.r(3) - 1);
        if (compare(c, std::int32_t(std::uint32_t(pair)), std::int32_t(c.r(0))) >= 0) return;
        block(c, 0x10371d38, 5);
        c.set(3, update.scores() + pair);
        if (record_value(c, 0, c.i8(c.r(3))) <= 0) return;
        block(c, 0x10371d4c, 4);
        c.set(0, std::uint32_t(c.i8(c.r(3) + 1)));
        if (compare(c, std::int32_t(c.r(0)), -1) <= 0) return;
        run_stats_match(update, byte_offset);
    }
}

void update_stats(Cpu& c) {
    GuestNativeScope scope(c.s, update_entry);
    block(c, update_entry, 18);
    const auto stack = c.sp(), frame = stack - update_frame_size;
    c.set(0, c.lr());
    guest::transfer_saved_registers<13>(c, stack);
    c.set(14, c.toc(-0x7b20));
    c.set(31, c.r(3));
    c.set(21, c.toc(-0x7b80));
    c.set(22, c.toc(-0x4dd4));
    c.set(23, c.toc(-0x4dd0));
    c.set(24, c.toc(-0x7b90));
    c.set(25, c.toc(-0x7b8c));
    c.set(26, c.toc(-0x7b88));
    c.set(27, c.toc(-0x7b84));
    c.set(13, c.toc(-0x7be8));
    c.set(28, c.toc(-0x4dcc));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    StatsUpdate update{c};
    if (!record_value(c, 0, std::int8_t(c.u8(c.r(2) - 0x388e)))) {
        block(c, 0x10370e58, 6);
        c.set(4, c.toc(-0x7acc));
        c.set({{3, update.text() + 0x120}, {5, 0}, {6, 0x664}, {7, 2}});
        c.call(0x10773320, 0x10370e70);
        block(c, 0x10370e70, 2);
        c.set(0, 1);
        c.put8(c.r(2) - 0x388e, 1);
    }
    shuffle_stats_rows(update);
    block(c, 0x103715d0, 7);
    c.set({{0, 0}, {3, 0x92490000}});
    c.put32(frame + 0x130, 0);
    c.set({{0, 0x92492493}, {17, 0}});
    c.put32(frame + 0x134, c.r(0));
    for (;;) {
        block(c, 0x10371d6c, 4);
        c.set(3, c.u32(update.object()));
        c.set(0, c.u32(frame + 0x130));
        if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(3))) >= 0) break;
        if (stats_group_due(update)) {
            assess_stats_clubs(update);
            sort_stats_candidates(update);
            shuffle_stats_candidates(update);
            process_stats_pairs(update);
        }
        block(c, 0x10371d5c, 4);
        c.set(3, c.u32(frame + 0x130));
        c.set({{17, update.row_offset() + row_size}, {3, c.r(3) + 1}});
        c.put32(frame + 0x130, c.r(3));
    }
    block(c, 0x10371d7c, 5);
    c.set(0, c.u32(frame + update_frame_size + 8));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<13, true>(c, stack);
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

const Routine& find_routine(std::uint32_t entry) {
    const auto found = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (found == routines.end() || found->entry != entry)
        throw std::logic_error("Unknown SIMULATED_STATS.CPP entry " + hex_address(entry));
    return *found;
}
}

void simulated_stats_detail::draw_range(Cpu& c, const RandomPool& pool, RangeDraw draw, std::int32_t bound) {
    block(c, draw.site, 4);
    c.set({{3, 0x10000}, {0, 65535}});
    const bool above = compare(c, bound, 65535) > 0;
    bool wide = above;
    if (!above) {
        block(c, draw.site + 16, 4);
        c.set({{3, 0xffff0000}, {0, std::uint32_t(-65535)}});
        wide = compare(c, bound, -65535) < 0;
    }
    if (!wide) {
        block(c, draw.site + 0xf4, 10);
        const auto position = c.u32(pool.cursor);
        c.set(3, position);
        const auto sign = compare(c, bound);
        const auto salt = c.u32(pool.salt);
        c.set(0, salt);
        const auto biased = c.u16(position) + salt;
        c.set(3, biased);
        const auto divisor = std::uint32_t(bound);
        const auto quotient = std::uint32_t(divide_signed<4>(biased, divisor));
        const auto remainder = biased - quotient * divisor;
        c.set({{0, remainder}, {draw.result, 0u - remainder}});
        if (sign > 0) {
            block(c, draw.site + 0x11c, 1);
            c.set(draw.result, remainder);
        }
        return;
    }

    block(c, draw.site + 0x20, 40);
    guest::Fpu fp(c);
    const auto frame = c.sp();
    c.set({{0, 0x43300000}, {3, std::uint32_t(bound) ^ 0x80000000}});
    c.put32(frame + draw.bound_scratch + 4, c.r(3));
    c.set(3, 0x80010000);
    const auto position = c.u32(pool.cursor);
    c.set({{7, position}, {4, 0x10000}});
    c.put32(frame + draw.bound_scratch, 0x43300000);
    c.set(6, 0x80008001);
    const auto bias = c.f64(pool.constants + 8);
    c.set_f(2, bias);
    const auto expanded = std::uint32_t(bound) * 10;
    c.set(3, expanded);
    c.set_f(1, c.f64(frame + draw.bound_scratch));
    c.set(5, 65535);
    const auto scale = c.f64(pool.constants);
    c.set_f(0, scale);
    c.set_f(1, fp.fsub(c.f(1), bias));
    const auto product = [](std::uint32_t bits) {
        return std::uint32_t((std::int64_t(std::int32_t(0x80008001u)) * std::int32_t(bits)) >> 32);
    };
    c.set(4, product(expanded));
    const auto word = c.u16(position);
    c.set(8, word);
    const auto salt = c.u32(pool.salt);
    c.set(7, salt);
    c.set_f(1, fp.fdiv(c.f(1), scale));
    c.put32(frame + draw.remainder_scratch, 0x43300000);
    const auto biased = word + salt;
    const auto word_sum = std::int32_t(product(biased) + biased);
    const auto bound_sum = std::int32_t(product(expanded) + expanded);
    const auto word_sign = unsigned(word_sum < 0);
    const auto bound_sign = unsigned(bound_sum < 0);
    const auto quotient = std::int32_t(expanded) / 65535;
    const auto multiple = std::uint32_t(std::int32_t(biased) / 65535) * 65535;
    const auto remainder = (biased - multiple) ^ 0x80000000;
    c.set({{3, std::uint32_t(quotient)}, {4, bound_sign}, {5, multiple}, {6, word_sign}, {0, remainder}});
    c.s.wr<1>(guest::reg::xer_ca, bound_sum < 0 && (std::uint32_t(bound_sum) & 0x7fff));
    c.put32(frame + draw.remainder_scratch + 4, remainder);
    c.set_f(0, c.f64(frame + draw.remainder_scratch));
    c.set_f(0, fp.fsub(c.f(0), bias));
    c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    c.put64(frame + draw.result_scratch, fp.fctiwz(0, c.f(0)));
    c.set(draw.result, c.u32(frame + draw.result_scratch + 4));
    call_stats(c, 0x10370830, draw.site + 0xc0, sample_stats_range);

    block(c, draw.site + 0xc0, 11);
    const auto correction = std::int32_t(c.r(3) + 5);
    const auto high = (INT64_C(0x66666667) * correction) >> 32;
    const auto limit = std::uint32_t(bound) - 1;
    const auto sample = c.r(draw.result) + std::uint32_t(correction / 10);
    if (draw.correction_in_r3)
        c.set({{0, limit}, {3, std::uint32_t(correction / 10)}, {4, unsigned(high < 0)}, {draw.result, sample}});
    else
        c.set({{0, std::uint32_t(correction / 10)}, {3, unsigned(high < 0)}, {4, limit}, {draw.result, sample}});
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 3));
    if (compare(c, std::int32_t(sample), std::int32_t(limit)) > 0) {
        block(c, draw.site + 0xec, 2);
        c.set(draw.result, limit);
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
