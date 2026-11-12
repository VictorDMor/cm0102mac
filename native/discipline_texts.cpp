// DISCIPLINE.CPP texts block (discipline_texts.hpp), native-first.
#include "discipline_texts.hpp"
#include "discipline_detail.hpp"
#include <initializer_list>

namespace cm::aot::discipline {
namespace {
namespace callee {
// Outside the module: the localized formatter (varargs: buffer, format, values), the name copy, the date text and
// the glue that calls a virtual method (the caller loads the method's descriptor into r12).
constexpr std::uint32_t format = 0x10727890, copy_name = 0x10715e00, date_text = 0x1030e880, virtual_call = 0x10781320;
}
namespace toc {
constexpr std::int32_t pool = -0x5608, today = -0x7b80;
}
namespace format_at {  // offsets in the module's pool
// 0x100e7490
constexpr std::uint32_t refuse_four = 0xa465, four_bodies = 0xa46c, three_bodies = 0xa477, two_bodies = 0xa483;
// 0x100e7210: by days still ahead, then with the name; each for a length of 0 or less (a date), one, several
constexpr std::uint32_t ahead_date = 0xa34d, ahead_one = 0xa292, ahead_many = 0xa2e6;
constexpr std::uint32_t past_date = 0xa422, past_one = 0xa3b7, past_many = 0xa3e2;
// 0x100e7780: by the status kind (5, 6, 0, 1, 4, 2, 3, other); kinds 1 and 2 have a text for a count of one
constexpr std::uint32_t status_5 = 0xa48c, status_6 = 0xa4a8, status_0 = 0xa4b9, status_1_one = 0xa4d6,
                        status_1_many = 0xa4e2, status_4 = 0xa504, status_2_one = 0xa531, status_2_many = 0xa53a,
                        status_3 = 0xa559, status_other = 0xa564;
}

// `bl 0x10727890`: the buffer, the format at `format_offset` of the pool and the values the format reads.
void print(guest::Cpu& c, std::uint32_t buffer, std::uint32_t format_offset, std::initializer_list<std::uint32_t> values,
           std::uint32_t return_address) {
    c.set({{3, buffer}, {4, c.toc(toc::pool) + format_offset}});
    unsigned reg = 5;
    for (const auto value : values) c.set(reg++, value);
    c.call(callee::format, return_address);
}

void failed_check(guest::Cpu& c, const guest::Frame& frame, std::uint32_t at, std::uint32_t line, std::int32_t name,
                  std::int32_t extension) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {at, pool + 0x50, pool + 0x5f, pool + 0x6c, frame.at(name), frame.at(extension), line});
}

// The ruling body object at index `index` (a signed byte) of the manager's array: -1 reads the word before it.
std::uint32_t body_at(const guest::Cpu& c, std::uint32_t owner, std::int8_t index) {
    return c.u32(c.u32(owner) + std::uint32_t(std::int32_t(index)) * 4);
}

// What a ruling body's kind says about the ban's first body (the result the callers' texts read).
std::uint32_t body_class(const guest::Cpu& c, std::uint32_t body) {
    const auto kind = c.i8(body + 4);
    if (kind == 1) return 9;
    if (kind == 0) return c.i8(body + 0x21) == -1 ? 0 : 8;
    if (kind == 3) return 1;
    switch (c.i8(body + 5)) {
        case 1: return 2;
        case 2: return 6;
        case 0x23: return 7;
        case 0x29: return 3;
        default: return 0xffffffffu;
    }
}
}

std::uint32_t describe_ban_bodies(guest::Cpu& c, std::uint32_t owner, std::uint32_t bodies, std::uint32_t buffer,
                                  std::int32_t size, std::int8_t refuse_four, std::uint32_t argument) {
    guest::Frame frame(c, 0x100e7490, 0x70, {.first_gpr = 28});
    if (size < 0x46) {
        failed_check(c, frame, 0x100e74e4, 0x1474, 0x40, 0x38);
        return 0xffffffffu;
    }
    // The original keeps three flags (all four bodies named, exactly the first three, exactly the first two) and
    // the first body named; a fourth body ends it early when the caller refuses it.
    const bool present0 = c.i8(bodies) != -1, present1 = c.i8(bodies + 1) != -1, present2 = c.i8(bodies + 2) != -1,
               present3 = c.i8(bodies + 3) != -1;
    if (present3 && refuse_four != 0) {
        print(c, buffer, format_at::refuse_four, {}, 0x100e75e8);
        return 0xffffffffu;
    }
    std::int8_t first = -1;
    for (const auto at : {0u, 1u, 2u, 3u})
        if (first == -1) first = c.i8(bodies + at);
    const bool four = present0 && present1 && present2 && present3;
    const bool three = present0 && present1 && present2 && !present3;
    const bool two = present0 && present1 && !present2 && !present3;
    if (four) print(c, buffer, format_at::four_bodies, {}, 0x100e7670);
    else if (three) print(c, buffer, format_at::three_bodies, {}, 0x100e768c);
    else if (two) print(c, buffer, format_at::two_bodies, {}, 0x100e76a0);
    else {
        // The first body's virtual method at +0x18 describes itself: `lwz r12,0(r3); lwz r12,0x18(r12)`.
        const auto body = body_at(c, owner, first);
        c.set({{3, body}, {4, buffer}, {5, std::uint32_t(size)}, {6, argument}, {12, c.u32(c.u32(body) + 0x18)}});
        c.call(callee::virtual_call, 0x100e7650);
        c.set(2, c.u32(c.sp() + 0x14));
    }
    return body_class(c, body_at(c, owner, first));
}

namespace {
// Whether the ban starts after today: today's date is a word of two halfwords (the later one compared first, then
// the earlier, signed, by the compiler's branch-free test of `ban < today`).
bool ban_is_ahead(const guest::Cpu& c, std::uint32_t ban) {
    const auto today = c.toc(toc::today);
    const auto today_late = c.u16(today + 2), ban_late = c.u16(ban + 6);
    if (today_late < ban_late) return true;
    if (today_late > ban_late) return false;
    const auto today_early = std::uint32_t(std::int32_t(c.i16(today))), ban_early = std::uint32_t(std::int32_t(c.i16(ban + 4)));
    const auto differ = ban_early ^ today_early;
    const auto half = std::uint32_t(std::int32_t(differ) >> 1);  // srawi
    return ((half - (differ & ban_early)) >> 31) != 0;
}
}

void ban_line(guest::Cpu& c, std::uint32_t owner, std::uint32_t ban, std::uint32_t buffer, std::int32_t size) {
    guest::Frame frame(c, 0x100e7210, 0x150, {.first_gpr = 28});
    if (size < 0x32) {
        failed_check(c, frame, 0x100e7258, 0x1402, 0x88, 0x40);
        return;
    }
    const auto bodies_text = frame.at(0xdc), name = frame.at(0xa8);
    const auto refuse_four = std::int8_t((c.u8(ban + 0x14) >> 6) & 1);
    const auto bodies = std::uint32_t(std::int32_t(std::int8_t(describe_ban_bodies(c, owner, ban + 0xc, bodies_text, 100, refuse_four, 4))));
    const auto length = std::uint32_t(std::int32_t(c.i8(ban)));
    const auto date_buffer = [&](std::int32_t at, std::uint32_t return_address) {
        c.set({{3, std::uint32_t(std::int32_t(c.i16(ban + 2)))}, {4, frame.at(at)}, {5, 0x1e}, {6, 0}});
        c.call(callee::date_text, return_address);
        return frame.at(at);
    };
    if (ban_is_ahead(c, ban)) {
        c.set({{3, ban + 4}, {4, name}, {5, 0x32}, {6, 0xff}});
        c.call(callee::copy_name, 0x100e7334);
        if (c.i8(ban) <= 0) {
            const auto date = date_buffer(0x68, 0x100e73b0);
            print(c, buffer, format_at::ahead_date, {100, bodies, bodies, date, bodies_text, name}, 0x100e73d8);
        } else if (c.i8(ban) == 1) {
            print(c, buffer, format_at::ahead_one, {bodies, bodies, 100, bodies_text, name}, 0x100e7370);
        } else {
            print(c, buffer, format_at::ahead_many, {bodies, bodies, 100, length, bodies_text, name}, 0x100e7398);
        }
    } else if (c.i8(ban) <= 0) {
        const auto date = date_buffer(0x48, 0x100e744c);
        print(c, buffer, format_at::past_date, {100, bodies, bodies, date, bodies_text}, 0x100e7470);
    } else if (c.i8(ban) == 1) {
        print(c, buffer, format_at::past_one, {bodies, bodies, bodies_text}, 0x100e7410);
    } else {
        print(c, buffer, format_at::past_many, {length, bodies, bodies, length, bodies_text}, 0x100e7434);
    }
}

void status_text(guest::Cpu& c, std::uint32_t, std::uint32_t status, std::uint32_t buffer, std::int32_t size) {
    guest::Frame frame(c, 0x100e7780, 0x70, {.first_gpr = 30});
    if (size < 0xf) {
        failed_check(c, frame, 0x100e77c0, 0x150d, 0x40, 0x38);
        return;
    }
    const auto count = std::uint32_t(std::int32_t(c.i16(status + 0x12)));
    switch (c.i8(status + 0x10)) {
        case 5: print(c, buffer, format_at::status_5, {}, 0x100e7820); break;
        case 6: print(c, buffer, format_at::status_6, {}, 0x100e7838); break;
        case 0: print(c, buffer, format_at::status_0, {count}, 0x100e7854); break;
        case 1: print(c, buffer, count == 1 ? format_at::status_1_one : format_at::status_1_many, {count}, count == 1 ? 0x100e7878 : 0x100e7888); break;
        case 4: print(c, buffer, format_at::status_4, {count}, 0x100e78a4); break;
        case 2: print(c, buffer, count == 1 ? format_at::status_2_one : format_at::status_2_many, {count}, count == 1 ? 0x100e78c8 : 0x100e78d8); break;
        case 3: print(c, buffer, format_at::status_3, {}, 0x100e78f0); break;
        default: print(c, buffer, format_at::status_other, {}, 0x100e7900); break;
    }
}
}
