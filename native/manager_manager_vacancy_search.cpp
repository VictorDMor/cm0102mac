// MANAGER_MANAGER.CPP vacancy block (manager_manager_vacancy.hpp), native-first: the candidates a club takes from its
// staff for a move (0x101bf5c0, with the roster test 0x101c00d0) and the news message 0x138c that reports them
// (0x101b9130). Values live in native locals; the guest ABI appears only at the calls to code outside the block.
#include "manager_manager_assess_detail.hpp"
#include "manager_manager_vacancy.hpp"
#include "manager_manager_vacancy_detail.hpp"
#include <array>

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace vacancy;
namespace {
namespace cell {
constexpr std::int32_t array_ctor = -0x7acc, roster_cell = -0x7b4c, people = -0x7be0, clubs = -0x7be8, link_table = -0x7b5c,
                       found_cell = -0x7b78, finder = -0x7b7c;
}
namespace call {
constexpr std::uint32_t array_new = 0x10773320, roster_get = 0x1029ec40, injured = 0x100c7d00, link_record = 0x10459b50,
                        staff_a = 0x100db060, staff_b = 0x100dafa0, find = 0x10082f20,
                        message_a = 0x1030d4a0, message_b = 0x1030dcd0;
}
constexpr std::uint32_t row_stride = 0x4c, club_stride = 0x248;
}

std::int8_t roster_standing(Cpu& c, std::uint32_t candidate, std::uint32_t key) {
    guest::Frame frame(c, 0x101c00d0, 0xe50, {.first_gpr = 29});
    c.set({{3, frame.at(0x158)}, {4, c.toc(cell::array_ctor)}, {5, 0}, {6, 0x664}, {7, 2}});
    c.call(call::array_new, 0x101c010c);
    if (!candidate) return 0;
    c.set({{3, c.u32(c.toc(cell::roster_cell))}, {4, key}, {5, frame.at(0x38)}});
    c.call(call::roster_get, 0x101c0128);
    if (std::int8_t(c.r(3)) == 0) return 0;
    // (the original also loads the byte at +0x84 into a register the two arms below do not tell apart)
    const auto people = c.u32(c.toc(cell::people));
    for (std::uint32_t squad = 0; squad < 2; ++squad)
        for (std::uint32_t slot = 0; slot < 20; ++slot) {
            const auto at = frame.at(0x38 + 0x194 + squad * 0x664 + slot * 0x4c);
            const auto index = c.u32(at);
            if (index == 0xffffffff || people + index * 0x70 != candidate) continue;
            if (slot >= 11) continue;
            const auto rating = c.u8(at + 0x35);
            return rating < 7 ? -1 : rating > 7 ? 1 : 0;
        }
    return 0;
}

Candidates read_candidates(const Cpu& c) {
    const auto sp = c.sp();
    return {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10), c.u32(sp + 0x38), c.u32(sp + 0x3c), c.u32(sp + 0x40)};
}

std::int8_t gather_candidates(Cpu& c, const Candidates& a) {
    guest::Frame frame(c, 0x101bf5c0, 0x190, {.first_gpr = 13});
    assess::Draws draws(c);
    constexpr std::uint32_t count_at = 0x38, list_at = 0x54;
    std::array<std::uint32_t, 128> stronger{}, weaker{};
    std::uint32_t stronger_count = 0, weaker_count = 0;
    if (a.ids) {
        c.put32(a.ids, 0xffffffff);
        c.put32(a.ids + 4, 0xffffffff);
        c.put32(a.ids + 8, 0xffffffff);
    }
    const auto club = a.club;
    if (!club || !c.u32(club + 0x80) || !c.u32(c.u32(club + 0x80) + 0x1c)) return 0;
    c.set(3, club);
    c.call(call::injured, 0x101bf654);
    if (std::int8_t(c.r(3)) != 0) return 0;
    // The club's willingness: the byte at +0x36 of the record's record is a signed byte, so the original's
    // floating-point branch for a magnitude above 0xffff (0x101bf6d0..0x101bf794, with its call of 0x10195e10) is
    // dead.
    const std::int32_t bound = std::int8_t(c.u8(c.u32(c.u32(club + 0x80) + 0x1c) + 0x36));
    std::int32_t willing = 0;
    if (bound != 0) willing = draws.below(std::int8_t(bound), 0x101bf6a8);
    if (willing < draws.modulo(10, 0x101bf7fc)) return 0;
    c.put8(a.out_e, 0);
    c.put8(a.out_a, 0);
    c.put8(a.out_b, 0);
    c.put8(a.out_c, 0);
    c.put8(a.out_d, 0);
    const auto link = c.u32(a.move + 0x14);
    if (link && c.u32(club + 0x30) == link) {
        c.put8(a.out_e, 1);
        // The club's link is non-null here (it equals the move's non-null link), so the original's null test of it at
        // 0x101bf884 never fails.
        {
            const auto entry = c.u32(c.u32(c.toc(cell::link_table)) + (c.u32(link) << 2));
            if (entry && c.u32(entry + 8)) {
                c.set({{3, c.u32(entry + 8)}, {4, club}});
                c.call(call::link_record, 0x101bf8b8);
                if (const auto record = c.r(3); record && c.u8(record + 4) > 8) {
                    c.put8(a.out_d, c.u8(record + 0x77));
                    c.put8(a.out_c, c.u8(record + 0x76));
                    c.put8(a.out_b, c.u32(c.u32(a.table) + c.u32(club) * row_stride + 0x2c));
                    if (c.u32(record + 0x1c) == 2) c.put8(a.out_a, 2);
                    else if (c.u32(record + 0x30) == 2) c.put8(a.out_a, 0xfe);
                }
            }
        }
    }
    const bool weak_mode = c.i8(a.mode) == 0;
    c.set({{3, c.u32(club + 0x80)}, {4, frame.at(list_at)}, {5, frame.at(count_at)}});
    c.call(weak_mode ? call::staff_a : call::staff_b, weak_mode ? 0x101bf940 : 0x101bfa24);
    for (std::int32_t i = 0; i < std::int32_t(c.i8(frame.at(count_at))); ++i) {
        const auto candidate = c.u32(frame.at(list_at + 4 * std::uint32_t(i)));
        if (!candidate || !c.u32(candidate + 0x14) || c.u8(candidate + 0x62) != 0xb) continue;
        c.set({{3, c.u32(c.toc(cell::finder))}, {4, candidate}});
        c.call(call::find, weak_mode ? 0x101bf978 : 0x101bfa5c);
        const auto found = c.r(3);
        c.put32(c.toc(cell::found_cell), found);
        const auto owner = found ? c.u32(c.toc(cell::clubs)) + c.u32(found + 4) * club_stride : 0;
        if (!owner || owner != club) continue;
        const std::int32_t rating = c.i16(c.u32(candidate + 0x14) + 0xa), reputation = c.i16(club + 0x1ec);
        if (weak_mode) {
            if (rating >= reputation) continue;
        } else {
            // The halfword at +0x5c, as a signed byte: its low byte.
            if (c.i8(candidate + 0x5d) >= 0x24 || rating <= reputation - 0x2ee) continue;
        }
        if (std::int8_t(roster_standing(c, candidate, a.key)) != -1) continue;
        if (weak_mode) weaker[weaker_count++] = candidate;
        else stronger[stronger_count++] = candidate;
    }
    if (a.ids) {
        std::uint32_t out = a.ids;
        for (std::uint32_t k = 0; k < stronger_count; ++k, out += 4) c.put32(out, c.u32(stronger[k]));
        for (std::uint32_t k = 0; k < weaker_count; ++k, out += 4) c.put32(out, c.u32(weaker[k]));
    }
    const auto row = c.u32(a.table) + c.u32(club) * row_stride;
    const auto grade = std::uint32_t(std::int32_t(std::int8_t(c.u8(c.u32(club + 0x80) + 0x63))) * 10);
    if (stronger_count) c.put16(row + 6, std::uint16_t(c.u16(row + 6) + grade));
    else if (weaker_count) c.put16(row + 6, std::uint16_t(c.u16(row + 6) - grade));
    return std::int8_t(stronger_count + weaker_count);
}

void file_vacancy_event(Cpu& c, std::uint32_t table, std::uint32_t person, std::uint32_t second, std::uint32_t move,
                        std::int16_t half, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t extra) {
    guest::Frame frame(c, 0x101b9130, 0x1a0, {.first_gpr = 21});
    constexpr std::uint32_t counter = 0x48, buffer = 0x80, object = 0x5c, ids = 0x50, flag = 0x4e, bytes = 0x49;
    for (std::uint32_t k = 0; k < 6; ++k) c.put8(frame.at(bytes + k), 0);
    if (!person || !second || !move || !c.u32(move + 0x14)) return;
    start_message(c, frame.at(buffer), 0x138c, 0x101b91bc);
    c.put8(frame.at(counter), 0);
    c.set(3, frame.at(object));
    c.call(0x10715880, 0x101b91cc);
    for (std::uint32_t k = 0; k < 3; ++k) c.put32(frame.at(ids + 4 * k), 0xffffffff);
    // Each record word is read just before its field is set, as the original does.
    set_field(c, frame.at(buffer), 0, std::uint32_t(std::int32_t(half)), 0x101b91f4);
    set_field(c, frame.at(buffer), 1, c.u32(person), 0x101b920c);
    set_field(c, frame.at(buffer), 2, c.u32(second), 0x101b9224);
    set_field(c, frame.at(buffer), 3, a, 0x101b923c);
    set_field(c, frame.at(buffer), 4, b, 0x101b9254);
    set_field(c, frame.at(buffer), 5, d, 0x101b926c);
    set_field(c, frame.at(buffer), 6, extra, 0x101b9284);
    set_field(c, frame.at(buffer), 7, c.u32(move + 8), 0x101b929c);
    const bool stronger = std::int32_t(a) > std::int32_t(b);
    c.put8(frame.at(flag), stronger ? 1 : 0);
    gather_candidates(c, {table, move, person, frame.at(ids), c.u32(move + 8), frame.at(flag), frame.at(flag - 1),
                          frame.at(flag - 2), frame.at(flag - 3), frame.at(flag - 4), frame.at(flag - 5)});
    set_field(c, frame.at(buffer), 8, c.u32(frame.at(ids)), 0x101b9348);
    set_field(c, frame.at(buffer), 9, c.u32(frame.at(ids + 4)), 0x101b9360);
    set_field(c, frame.at(buffer), 10, c.u32(frame.at(ids + 8)), 0x101b9378);
    assess::Draws draws(c);
    set_field(c, frame.at(buffer), 11, std::uint32_t(draws.modulo(4, 0x101b93a0)), 0x101b93e0);
    const std::uint32_t backs_bytes[6] = {0x101b93fc, 0x101b9418, 0x101b9434, 0x101b9450, 0x101b946c, 0x101b9488};
    for (std::uint32_t k = 0; k < 6; ++k)
        set_field(c, frame.at(buffer), 12 + k, sext8(c.i8(frame.at(flag - k))), backs_bytes[k]);
    c.put8(frame.at(counter), 18);
    c.set({{3, move}, {4, person}, {5, frame.at(object)}});
    c.call(call::message_a, 0x101b9498);
    c.set({{3, frame.at(object)}, {4, frame.at(buffer)}, {5, frame.at(counter)}});
    c.call(call::message_b, 0x101b94a8);
    c.set({{3, c.u32(c.toc(toc_cell::news_desk))}, {4, frame.at(buffer)}, {5, person}});
    c.call(callee::file_message, 0x101b94bc);
}
}
