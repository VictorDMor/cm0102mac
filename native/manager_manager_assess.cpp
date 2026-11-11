// MANAGER_MANAGER.CPP assessment block (manager_manager_assess.hpp), native-first: values live in native
// locals; the guest ABI appears only at the calls to code outside the block.
#include "manager_manager_assess.hpp"
#include "manager_manager_assess_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace assess;
namespace {
namespace callee {
constexpr std::uint32_t clear_message = 0x102bdf90, set_field = 0x102cb4b0, file_message = 0x102cc140;
}
constexpr std::uint32_t frame_size = 0x180, buffer_offset = 0x60, high_offset = 0x40, low_offset = 0x38;
constexpr std::uint32_t message_kind = 0x178f, assert_line = 0x5cc1;
namespace person_at { constexpr std::uint32_t id = 0, club_a = 0x28, club_b = 0x2c, club_c = 0x30; }
namespace club_at {
constexpr std::uint32_t id = 0, tier = 0x2c, board = 0x3c, reputation = 0x1ec, kind = 0x23d, standing = 0x240;
}

void set_field(Cpu& c, std::uint32_t buffer, std::uint32_t index, std::uint32_t value, std::uint32_t back) {
    c.set({{3, buffer}, {4, index}, {5, value}});
    c.call(callee::set_field, back);
}
std::uint32_t byte_value(std::int8_t value) { return std::uint32_t(std::int32_t(value)); }  // extsb
}

namespace assess {
void report_assertion(Cpu& c, const guest::Frame& frame, std::uint32_t high, std::uint32_t low, std::uint32_t line,
                      const std::uint32_t (&back)[4]) {
    // The original keeps the pool and the message cell in registers across the calls.
    const auto message = c.toc(toc_slot::message), pool = c.toc(toc_slot::pool);
    c.set({{3, pool + 0x37}, {4, 0}, {5, 0}, {6, frame.at(high)}, {7, frame.at(low)}});
    c.call(0x10739890, back[0]);
    c.call(0x10768810, back[1]);
    c.set({{3, message}, {4, pool + 0x4b}, {5, c.r(3)}, {6, frame.at(high)}, {7, frame.at(low)}, {8, line}});
    c.call(0x1077b720, back[2]);
    c.set({{3, pool + 0x58}, {4, message}, {5, 0}});
    c.call(0x10769b60, back[3]);
    c.put8(c.toc(toc_slot::success), 0);
}
}

void post_assessment_message(Cpu& c, std::uint32_t person, std::uint32_t club, std::int8_t first,
                             std::int8_t second, std::int8_t third, std::int8_t fourth) {
    guest::Frame frame(c, 0x101c3860, frame_size, {.first_gpr = 24});
    const auto buffer = frame.at(buffer_offset);
    if (!person || !club) {
        report_assertion(c, frame, high_offset, low_offset, assert_line, {0x101c38b4, 0x101c38b8, 0x101c38d4, 0x101c38e4});
        return;
    }
    c.set({{3, buffer}, {4, message_kind}, {5, 0}});
    c.call(callee::clear_message, 0x101c3904);
    set_field(c, buffer, 0, c.u32(person + person_at::id), 0x101c3914);
    set_field(c, buffer, 1, c.u32(c.u32(person + person_at::club_a) + 0x20), 0x101c3928);
    set_field(c, buffer, 2, c.u32(c.u32(person + person_at::club_b) + 0x20), 0x101c393c);
    set_field(c, buffer, 3, c.u32(c.u32(person + person_at::club_c) + 0x20), 0x101c3950);
    set_field(c, buffer, 4, c.u32(club + club_at::id), 0x101c3960);
    set_field(c, buffer, 5, byte_value(first), 0x101c3970);
    set_field(c, buffer, 6, byte_value(second), 0x101c3980);
    set_field(c, buffer, 7, byte_value(third), 0x101c3990);
    set_field(c, buffer, 8, byte_value(fourth), 0x101c39a0);

    // The limit for the budget: by the club's tier (a signed byte) two to three times its reputation (a
    // halfword; each step wraps to 16 bits) plus a base; below the reputation 0x109a it is held to 5000.
    const std::int32_t reputation = c.i16(club + club_at::reputation);
    const std::int8_t tier = c.i8(c.u32(club + club_at::tier) + 0x11f);
    std::int32_t limit;
    if (tier > 14) limit = half(half(std::uint32_t(reputation) * 3) + 0x1388u);
    else if (tier > 8) limit = half(half(std::uint32_t(reputation) * 2) + 0x9c4u);
    else limit = half(half(std::uint32_t(reputation) * 2) + 0x4e2u);
    if (reputation < 0x109a && half(std::uint32_t(limit)) > 0x1388) limit = 0x1388;
    if (c.u8(club + club_at::kind) == 3) {
        // The scale of the pool (the double at +0x20) over the limit, truncated; its low word is the new limit.
        guest::Fpu fpu(c);
        const auto pool = c.toc(toc_slot::constants);
        const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(std::int32_t(half(std::uint32_t(limit)))) ^ 0x80000000u);
        auto scaled = fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + pool_magic));
        scaled = fpu.fmul(c.f64(pool + pool_scale_c), scaled);
        limit = std::int32_t(fpu.fctiwz(0, scaled));
    }
    const std::int32_t budget = c.i32(c.u32(club + club_at::board) + 4);
    set_field(c, buffer, 9, budget < std::int32_t(half(std::uint32_t(limit))) ? 1 : 0, 0x101c3a8c);

    const std::int8_t standing = c.i8(club + club_at::standing);
    const bool upper = (reputation > 0x109a && standing < 10) || (reputation > 0x1676 && standing < 14);
    set_field(c, buffer, 10, upper ? 1 : 0, 0x101c3ae0);
    c.set({{3, c.u32(c.toc(toc_slot::news_desk))}, {4, buffer}, {5, club}});
    c.call(callee::file_message, 0x101c3af4);
}

namespace {
namespace callee3220 {
constexpr std::uint32_t init_local = 0x10715880, date_of = 0x10717890, date_score = 0x10718130,
                        append_record = 0x10738d10, owner_decision = 0x100d9b80;
}
namespace toc_cell { constexpr std::int32_t date = -0x7b80, tiers = -0x7984; }
constexpr std::uint32_t row_size = 0x4c;
namespace row_at { constexpr std::uint32_t flags = 2, rating = 6, size_class = 8; }  // flags: byte; the others int16
namespace person_at2 { constexpr std::uint32_t profile = 0x1c, date_a = 0x4c, date_b = 0x50; }  // profile: +0x2a int16
namespace club2 { constexpr std::uint32_t league = 0x30, board = 0x3c, owner = 0x80, reputation = 0x1ec; }
namespace owner_at { constexpr std::uint32_t link_a = 0x18, profile = 0x1c; }  // profile: +0x3d int8
constexpr std::uint32_t flag_flagged = 0x40;

// The club's row in the list object's table.
std::uint32_t row_of(Cpu& c, std::uint32_t list, std::uint32_t club) {
    return c.u32(list) + c.u32(club + club_at::id) * row_size;
}
// The row's rating: set to 1 below 1 and to 0x2710 above it (the original reads it back from the row).
void clamp_rating(Cpu& c, std::uint32_t list, std::uint32_t club) {
    const auto row = row_of(c, list, club);
    const std::int32_t rating = c.i16(row + row_at::rating);
    c.put16(row + row_at::rating, std::uint32_t(rating < 1 ? 1 : rating > 0x2710 ? 0x2710 : rating));
}
void lower_rating(Cpu& c, std::uint32_t list, std::uint32_t club, std::uint32_t by) {
    const auto row = row_of(c, list, club);
    c.put16(row + row_at::rating, std::uint32_t(c.i16(row + row_at::rating)) - by);
}
}

void assess_manager(Cpu& c, std::uint32_t list, std::uint32_t person, std::uint32_t club, std::uint32_t kind_word) {
    namespace f = callee3220;
    const std::int8_t kind = std::int8_t(kind_word);
    guest::Frame frame(c, 0x101c3220, 0xb0, {.first_gpr = 21});
    constexpr std::uint32_t scratch_local = 0x54, date_local = 0x38, post_local = 0x40, record_local = 0x48;
    const auto date = c.toc(toc_cell::date);
    Draws draws(c);
    c.set(3, frame.at(scratch_local));
    c.call(f::init_local, 0x101c3260);
    if (!person || !c.u32(person + person_at2::profile) || !club || !c.u32(club + club2::league) || !c.u32(club + club2::board))
        return;
    // The record handed to the verdict: +8 the kind, +9 its answer (the person's and club's ids it holds are not read).
    c.put32(frame.at(record_local), c.u32(person));
    c.put32(frame.at(record_local + 4), c.u32(club));
    c.set({{3, frame.at(date_local)}, {4, date}, {5, 1}});
    c.call(f::date_of, 0x101c32bc);  // the date it returns, which the original copies to a local nobody reads
    c.put8(frame.at(record_local + 8), std::uint32_t(kind));
    auto score = [&](std::uint32_t back) {
        c.set({{3, date}, {4, c.u32(person + person_at2::date_a)}, {5, c.u32(person + person_at2::date_b)}});
        c.call(f::date_score, back);
        return std::int32_t(c.r(3));
    };
    auto reputation_of_person = [&] { return std::int32_t(c.i16(c.u32(person + person_at2::profile) + 0x2a)); };

    // Whether the club takes an interest: always above 0x1a5e, otherwise by a draw against the value.
    bool interested = false;
    if (reputation_of_person() > 0x1a5e) {
        interested = true;
    } else {
        const auto drawn = draws.modulo(0x1a5e, 0x101c3318);
        if (reputation_of_person() > std::int32_t(half(std::uint32_t(drawn) + 0x7d0u))) interested = true;
    }
    if (interested) {
        bool urgent = false;
        const auto tier = c.i8(c.u32(c.toc(toc_cell::tiers)) + c.u32(club + club_at::id) * 8 + 4);
        if (tier > 0) {
            urgent = true;
        } else {
            if (c.i16(row_of(c, list, club) + row_at::size_class) > 0x1482 && score(0x101c33cc) > 0x3e8) {
                urgent = true;
            } else if (c.i16(row_of(c, list, club) + row_at::size_class) > 0x186a) {
                urgent = true;
            } else if (score(0x101c3410) < 0xb4 && reputation_of_person() > 0x1964) {
                urgent = true;
            }
        }
        post_assessment_message(c, person, club, kind, 1, urgent ? 2 : 1, 0);
    }
    // The original clears the verdict byte (0x101c3480) before the call; every path of the verdict writes it, so the
    // store is left out.
    const auto verdict = assess_verdict(c, list, frame.at(record_local), club, person);
    post_assessment_message(c, person, club, std::int8_t(c.u8(frame.at(record_local + 8))), 3, verdict, interested ? 1 : 0);
    if (verdict == 2) {
        std::uint8_t code;
        // The person's profile is read as non-null here (the entry checked it and nothing in between writes it), but the
        // original tests it again.
        if (const auto profile = c.u32(person + person_at2::profile);
            profile && c.i16(profile + 0x2a) > 0x1482 &&
            (c.i16(profile + 0x2a) > 0x1c52 || c.i16(profile + 0x2a) > std::int32_t(c.i16(club + club2::reputation)) - 0x2ee)) {
            code = 0xc;
        } else if (score(0x101c3518) < 0xf0) {
            code = 0xd;
        } else {
            code = interested ? 0xe : 0xf;
        }
        c.put8(frame.at(post_local), 1);
        c.put8(frame.at(post_local + 1), code);
        c.put32(frame.at(post_local + 4), c.u32(club + club_at::id));
        c.set({{3, c.u32(list + 4)}, {4, frame.at(post_local)}, {5, 8}});
        c.call(f::append_record, 0x101c3564);
        const auto row = row_of(c, list, club);
        c.put8(row + row_at::flags, c.u8(row + row_at::flags) | flag_flagged);
        const auto owner = c.u32(club + club2::owner);
        if (!owner || !c.u32(owner + owner_at::link_a) || !c.u32(owner + owner_at::profile)) return;
        const std::int8_t bound = c.i8(c.u32(owner + owner_at::profile) + 0x3d);
        std::int32_t chance = 0;
        if (bound != 0) chance = draws.below(bound, 0x101c35dc);
        if (chance < draws.modulo(0x15, 0x101c3728)) {
            c.set({{3, c.u32(club + club2::owner)}, {4, person}, {5, 1}});
            c.call(f::owner_decision, 0x101c3778);
        }
    } else if (verdict == 3) {
        // The club is non-null here (0x101c3784 tests it again; the entry already did).
        lower_rating(c, list, club, 0x4e2);
        clamp_rating(c, list, club);
    } else if (verdict == 4) {
        lower_rating(c, list, club, 500);
        clamp_rating(c, list, club);
    }
}
}
