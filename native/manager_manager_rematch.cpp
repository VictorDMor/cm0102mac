// MANAGER_MANAGER.CPP rematch block (manager_manager_rematch.hpp): the smaller routines. The two
// large ones (0x101aaa90, 0x101ac280) are in manager_manager_rematch_search.cpp.
#include "manager_manager_rematch.hpp"
#include "manager_manager_assessment.hpp"
#include "manager_manager_daily.hpp"
#include "manager_manager_lists.hpp"
#include "manager_manager_rematch_detail.hpp"
#include "manager_manager_review.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace rematch;
// The registers the original holds across its calls. A callee saves the ones it uses in its frame, and
// those words stay on the stack under the frames the next calls build; 0x101aaa90 builds its 0x28-byte
// offer record in such a frame and 0x10738d10 copies it whole into the offer list, so the record's padding
// bytes are the save areas of earlier callees (checker case "rematch settle 14" shows it). Each routine
// therefore sets the registers of its prologue (the arguments and the table cells it loads) where the
// original has them, as the brief's rule for 0x1074bab0 does. The ones the original changes while it loops
// are set at the loop top where a callee can save them.
namespace {
constexpr std::int8_t i8(std::uint32_t v) { return std::int8_t(v); }
// The shared list container: cursor to the head (`lwz r0,4(r3); stw r0,0xc(r3)`).
void rewind(Cpu& c, std::uint32_t list) { c.put32(list + list::cursor, c.u32(list + list::head)); }
}

namespace rematch {
std::int32_t Draw::split(std::int32_t n, std::int32_t drawn, std::uint32_t back_draw) {
    guest::Fpu fpu(c);
    const auto pool_at = c.toc(cell::pool);
    auto f1 = to_double(fpu, c, pool_at, n);
    f1 = fpu.fdiv(f1, c.f64(pool_at + pool::divisor));
    const auto quotient = divide(drawn, 0xffff);
    const auto rest = std::int32_t(std::uint32_t(drawn) - std::uint32_t(quotient) * 0xffffu);
    auto f0 = to_double(fpu, c, pool_at, rest);
    f0 = fpu.fmul(f1, f0);
    const auto index = std::int32_t(fpu.fctiwz(0, f0));
    const auto bound = divide(std::int32_t(std::uint32_t(n) * 10u), 0xffff);
    guest::enter(c, back_draw, {std::uint32_t(bound)});
    const auto d = manager_manager::draw(c, bound);
    const auto tenth = divide(std::int32_t(std::uint32_t(d) + 5u), 10);
    auto result = std::int32_t(std::uint32_t(index) + std::uint32_t(tenth));
    if (result > std::int32_t(std::uint32_t(n) - 1u)) result = std::int32_t(std::uint32_t(n) - 1u);
    return result;
}
}

void forward_to_c8a80(Cpu& c, std::uint32_t second) {
    guest::Frame frame(c, 0x101aa650, 0x40, {});
    c.set(3, second);  // 0x101c8a80 also reads r9 and r10 (masks.txt): the caller's, which nothing here touches
    c.call(0x101c8a80, 0x101aa664);
}

// ---- 0x101b2030 ----
void post_approach_news(Cpu& c, std::uint32_t person, std::uint32_t club) {
    guest::Frame frame(c, 0x101b2030, 0x130, {.first_gpr = 30});
    guest::keep(c, {{30, person}, {31, club}});
    if (!person || !c.u32(person + person::job_club) || !club) return;
    if (event_after_today(c)) return;
    const auto buffer = frame.at(0x40);
    start_message(c, buffer, 0x1775, 0x101b20d8);
    set_field(c, buffer, 0, c.u32(person + person::id), 0x101b20e8);
    set_field(c, buffer, 1, c.u32(c.u32(person + person::name_a) + 0x20), 0x101b20fc);
    set_field(c, buffer, 2, c.u32(c.u32(person + person::name_b) + 0x20), 0x101b2110);
    set_field(c, buffer, 3, c.u32(c.u32(person + person::name_c) + 0x20), 0x101b2124);
    set_field(c, buffer, 4, c.u32(c.u32(person + person::job_club)), 0x101b2138);
    set_field(c, buffer, 5, c.u32(club), 0x101b2148);
    set_field(c, buffer, 6, sext8(c.u8(person + person::status)), 0x101b215c);
    out(c, 0x102cc140, 0x101b2170, {c.u32(c.toc(cell::news_desk)), buffer, c.u32(person + person::job_club)});
}

// ---- 0x101abda0 ----
std::int32_t best_candidate_score(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t exclude) {
    guest::Frame frame(c, 0x101abda0, 0x70, {.first_gpr = 21});
    const auto pool_at = c.toc(cell::pool);
    std::int32_t best = -10000;
    std::uint32_t person = c.u32(c.toc(cell::people));
    std::uint32_t row_offset = 0;
    guest::keep(c, {{21, object}, {22, club}, {23, exclude}, {28, c.toc(cell::rows)}, {29, c.toc(cell::object)},
                    {30, c.toc(cell::pool)}, {31, c.toc(cell::people_count)}});
    for (std::int32_t index = 0;; ++index, row_offset += 100, person += person::size) {
        guest::keep(c, {{24, std::uint32_t(index)}, {25, std::uint32_t(best)}, {26, person}, {27, row_offset}});
        const auto count = people_count(c);
        if (index >= count) return best;
        if (c.u32(person + person::staff) == 0) continue;
        if (!(index < count - 0x10 || c.i8(person + person::flag) != 0)) continue;
        if (exclude == person) continue;
        if (((c.u8(c.u32(c.toc(cell::rows)) + row_offset + 0xc) >> 2) & 3) != 0) continue;
        if (out(c, 0x10083110, 0x101abe34, {c.u32(c.toc(cell::object)), person}) != 0) continue;
        c.link(0x101abe48);
        if (i8(list_contains(c, object, person)) != 0) continue;
        const auto region = c.u32(club + club::region), home = c.u32(person + person::home);
        if (region != home && c.i16(c.u32(person + person::staff) + staff::b) < 0xea6) continue;
        if (region == 0 || home == 0) continue;
        if (c.i8(region + 0x117) != c.i8(home + 0x117) &&
            (c.i16(c.u32(person + person::staff) + staff::c) < 0x109a || c.i16(club + club::reputation) < 0x128e))
            continue;
        if (c.u32(person + person::post_a) != 0 && c.u32(person + person::job_club) != 0) {
            const auto status = c.u8(person + person::status);
            if (std::int8_t(status) == 0xb || std::uint8_t(status - 0xc) <= 1 || std::int8_t(status) == 0xf) {
                const auto deal = out(c, 0x10082f90, 0x101abf04, {c.u32(c.toc(cell::object)), person, club});
                if (deal != 0 && c.i8(deal + 0x54) != 0) continue;
            }
        }
        c.link(0x101abf2c);
        if (!candidate_willing(c, c.f64(pool_at + pool::bar), person, club)) continue;
        guest::enter(c, 0x101abf4c, {object, person, club, 0, 5});
        const auto score = std::int32_t(assess(c, person, club, 0, 5));
        if (score > best) best = score;
    }
}

// ---- 0x101acf00 ----
void plan_all_vacancies(Cpu& c, std::uint32_t object) {
    guest::Frame frame(c, 0x101acf00, 0x90, {.first_gpr = 23});
    const auto pool_str = c.toc(slots::pool);
    const auto answer = frame.at(0x38);
    c.put8(answer, 0);
    std::uint32_t club = c.u32(c.toc(cell::clubs));
    guest::keep(c, {{25, c.toc(cell::report)}, {26, c.toc(-0x7ba8)}, {27, c.toc(cell::club_count)}, {28, pool_str}, {29, object}});
    for (std::int32_t index = 0; index < club_count(c); ++index, club += club::size) {
        guest::keep(c, {{30, std::uint32_t(index)}, {31, club}, {24, 0}});
        if (out(c, 0x100c96e0, 0x101acf44, {club}) != 0) continue;
        if (i8(out(c, 0x100d3c40, 0x101acf54, {club})) == 0) continue;
        if (out(c, 0x100ce320, 0x101acf70, {club, answer, 1}) != 0 && i8(c.u8(answer)) == 0) continue;
        bool found = false;
        rewind(c, c.u32(object + 8));
        for (std::int32_t n = 0; n < c.i16(c.u32(object + 8) + list::count); ++n) {
            guest::keep(c, {{23, std::uint32_t(n)}});
            const auto node = out(c, 0x10738e00, 0x101acfa0, {c.u32(object + 8)});
            if (!node) {
                // Line 0x310d: the offer list is shorter than its count. The routine ends.
                guest::report_assertion(c, {.at = 0x101acfbc, .file = pool_str + 0x37, .format = pool_str + 0x4b,
                                            .title = pool_str + 0x58, .name = frame.at(0x44), .extension = frame.at(0x3c),
                                            .line = 0x310d});
                return;
            }
            if (c.u32(node + 0x10) == c.u32(club)) {
                found = true;
                guest::keep(c, {{24, 1}});
                break;
            }
        }
        if (found) continue;
        out(c, 0x1077b720, 0x101ad048, {c.toc(cell::report), pool_str + 0x138cb, club + club::name});
        c.link(0x101ad058);
        plan_vacancy(c, object, club, 0);
    }
}

// ---- 0x101aa510 ----
void settle_queue(Cpu& c, std::uint32_t object) {
    guest::Frame frame(c, 0x101aa510, 0x90, {.first_gpr = 23});
    const auto pool_str = c.toc(slots::pool);
    guest::keep(c, {{26, c.toc(cell::clubs)}, {27, c.toc(-0x7bac)}, {28, c.toc(-0x7ba8)}, {29, pool_str}, {30, object}});
    if (c.u32(object + 4) == 0) return;
    rewind(c, c.u32(object + 4));
    for (std::int32_t n = 0; n < c.i16(c.u32(object + 4)); ++n) {
        guest::keep(c, {{31, std::uint32_t(n)}});
        const auto node = out(c, 0x10738e00, 0x101aa554, {c.u32(object + 4)});
        if (!node) {
            // Line 0x2b70: the queue is shorter than its count. The loop goes on.
            guest::report_assertion(c, {.at = 0x101aa5e4, .file = pool_str + 0x37, .format = pool_str + 0x4b,
                                        .title = pool_str + 0x58, .name = frame.at(0x40), .extension = frame.at(0x38),
                                        .line = 0x2b70});
            continue;
        }
        const auto club = c.u32(c.toc(cell::clubs)) + c.u32(node + 4) * club::size;
        const auto manager = c.u32(club + club::manager);
        guest::keep(c, {{25, node}, {24, club}, {23, manager}});
        if (manager == 0) {
            out(c, 0x10738e60, 0x101aa584, {c.u32(object + 4), node});
            continue;
        }
        c.link(0x101aa5a0);
        end_tenure(c, object, club, c.u8(node), c.u8(node + 1), 0);
        if (i8(out(c, 0x100d3c40, 0x101aa5a8, {club})) != 0) {
            c.link(0x101aa5c0);
            plan_vacancy(c, object, club, manager);
        }
        out(c, 0x10738e60, 0x101aa5cc, {c.u32(object + 4), node});
    }
}

// ---- 0x101aa680 ----
void end_tenure(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t kind, std::uint32_t extra,
                std::uint32_t keep_rating) {
    guest::Frame frame(c, 0x101aa680, 0x70, {.first_gpr = 21});
    const auto manager = c.u32(club + club::manager);
    if (manager == 0) return;
    std::uint32_t status;
    if (c.u32(manager + person::job_club) == club) status = c.u8(manager + person::status);
    else if (c.u32(manager + person::side_club) == club) status = c.u8(manager + person::side_status);
    else return;
    guest::keep(c, {{21, c.toc(cell::links)}, {22, object}, {23, club}, {24, kind}, {25, extra}, {26, keep_rating},
                    {27, status}, {28, manager}, {29, c.toc(cell::deals)}, {30, c.toc(cell::object)}, {31, c.toc(cell::today)}});
    if (i8(out(c, 0x1014c2e0, 0x101aa704, {club, 1, 1, 0, 0})) != 0) {
        out(c, 0x102cda60, 0x101aa720, {c.u32(c.toc(cell::news_desk)), manager, club});
        out(c, 0x103afff0, 0x101aa738, {c.u32(c.toc(cell::links)), club, 1, 0, 0});
        out(c, 0x103afff0, 0x101aa750, {c.u32(c.toc(cell::links)), club, 0, 0, 0});
    }
    // Today's date goes into the manager's record, in the slot of the side that is his.
    const auto today = c.toc(cell::today);
    if (i8(out(c, 0x100c7d00, 0x101aa758, {club})) != 0) {
        c.put16(manager + 0x3e, c.u16(today + 2));
        c.put16(manager + 0x3c, c.u16(today));
        c.put8(manager + 0x40, c.u8(today + 4));
    } else {
        c.put16(manager + 0x4e, c.u16(today + 2));
        c.put16(manager + 0x4c, c.u16(today));
        c.put8(manager + 0x50, c.u8(today + 4));
    }
    // The status picks one of two sequences (0xc, 0xd and 0xf end a post first); both release the
    // manager's hold, tell the deals and file the move.
    const auto status_byte = std::int32_t(std::int8_t(status));
    const bool ends_post = status_byte != 0xe && status_byte >= 0xc && status_byte < 0x10;
    if (ends_post) {
        guest::enter(c, 0x101aa7c8, {object, manager, club});
        end_post(c, object, manager, club);
    }
    guest::enter(c, ends_post ? 0x101aa7d8 : 0x101aa850, {object, manager, club});
    clear_post(c, object, manager, club);
    if (i8(out(c, 0x100c7d00, ends_post ? 0x101aa7e0 : 0x101aa858, {club})) == 0)
        out(c, 0x10084a80, ends_post ? 0x101aa7f4 : 0x101aa86c, {c.u32(c.toc(cell::object)), manager});
    if (std::int8_t(kind) != 3) {
        c.link(ends_post ? 0x101aa81c : 0x101aa894);
        post_move_news(c, manager, kind, extra, club, ends_post ? 2 : 3);
    }
    if (i8(keep_rating) == 0) c.put16(c.u32(object) + c.u32(club) * row::size + 6, 0xdac);
    if (std::int8_t(kind) == 2) out(c, 0x10382890, 0x101aa8d4, {c.u32(c.toc(cell::deals)), manager, club, 0});
    else out(c, 0x10382890, 0x101aa8ec, {c.u32(c.toc(cell::deals)), manager, club, 2});
    if (const auto watched = c.u32(c.toc(cell::watch)); watched != 0)
        out(c, 0x1010a060, 0x101aa904, {watched, club});
    if (i8(out(c, 0x100c7d00, 0x101aa90c, {club})) != 0) return;
    for (std::uint32_t slot = 0; slot < 0x32; ++slot) {
        guest::keep(c, {{28, slot}, {29, slot * 4}});
        const auto holder = c.u32(club + club::slots + slot * 4);
        if (holder == 0 || c.u32(holder + 0x14) == 0) continue;
        const auto deal = out(c, 0x10082f20, 0x101aa940, {c.u32(c.toc(cell::object)), holder});
        if (deal == 0) continue;
        guest::keep(c, {{27, deal}});
        // The deal's flags are cleared one by one: the call that reads them (0x1037fa70) and the one that
        // clears (0x1037fa50, bits of its own word) or the other (0x1037f7d0, bits of the word at +0x50).
        const auto clear_a = [&](std::uint32_t bit, std::uint32_t back) { out(c, 0x1037f7d0, back, {deal, bit}); };
        const auto clear_b = [&](std::uint32_t bit, std::uint32_t back) { out(c, 0x1037fa50, back, {deal, bit}); };
        if (out(c, 0x1037fa70, 0x101aa94c, {deal}) & 0x10000000u) clear_a(0x10000000, 0x101aa964);
        if (out(c, 0x1037fa70, 0x101aa96c, {deal}) & 0x80u) clear_b(0x80, 0x101aa980);
        if (out(c, 0x1037fa70, 0x101aa988, {deal}) & 0x100u) clear_b(0x100, 0x101aa99c);
        constexpr struct { std::uint32_t bit, back; } bits[] = {{0x8000000, 0x101aa9b8}, {0x8, 0x101aa9d0},
                                                                {0x200, 0x101aa9e8}, {0x400, 0x101aaa00},
                                                                {0x2000, 0x101aaa18}, {0x10000, 0x101aaa34},
                                                                {0x80000, 0x101aaa50}, {0x800000, 0x101aaa6c}};
        for (const auto& b : bits)
            if (c.u32(deal + 0x50) & b.bit) clear_a(b.bit, b.back);
    }
}

// ---- 0x101b39f0 ----
void post_move_news(Cpu& c, std::uint32_t person, std::uint32_t kind, std::uint32_t extra, std::uint32_t club,
                    std::uint32_t mode) {
    guest::Frame frame(c, 0x101b39f0, 0x140, {.first_gpr = 26});
    guest::keep(c, {{26, person}, {27, kind}, {28, extra}, {29, club}, {30, mode}, {31, c.toc(cell::news_desk)}});
    if (!person || !club) return;
    if (event_after_today(c)) return;
    if (std::int8_t(kind) == 5) return;
    const auto buffer = frame.at(0x44);
    start_message(c, buffer, 0x1770, 0x101b3aa4);
    set_field(c, buffer, 0, c.u32(person + person::id), 0x101b3ab4);
    set_field(c, buffer, 1, c.u32(c.u32(person + person::name_a) + 0x20), 0x101b3ac8);
    set_field(c, buffer, 2, c.u32(c.u32(person + person::name_b) + 0x20), 0x101b3adc);
    set_field(c, buffer, 3, c.u32(c.u32(person + person::name_c) + 0x20), 0x101b3af0);
    set_field(c, buffer, 4, sext8(kind), 0x101b3b00);
    set_field(c, buffer, 5, c.u32(club), 0x101b3b10);
    set_field(c, buffer, 7, sext8(mode), 0x101b3b20);
    if (const auto staff = c.u32(club + 0x80); staff != 0) {
        set_field(c, buffer, 8, c.u32(c.u32(staff + 0x28) + 0x20), 0x101b3b40);
        set_field(c, buffer, 9, c.u32(c.u32(c.u32(club + 0x80) + 0x2c) + 0x20), 0x101b3b58);
        set_field(c, buffer, 10, c.u32(c.u32(c.u32(club + 0x80) + 0x30) + 0x20), 0x101b3b70);
    } else {
        set_field(c, buffer, 8, 0xffffffff, 0x101b3b84);
        set_field(c, buffer, 9, 0xffffffff, 0x101b3b94);
        set_field(c, buffer, 10, 0xffffffff, 0x101b3ba4);
    }
    set_field(c, buffer, 11, sext8(extra), 0x101b3bb4);
    const auto news_desk = c.u32(c.toc(cell::news_desk));
    if (people_count(c) - 0x10 <= c.i32(person)) {
        // A person of the last 16 (a club's staff): the pair of names goes through 0x102cb630 first.
        const auto pick = frame.at(0x38);
        c.put8(pick, 0xd);
        out(c, 0x102cb630, 0x101b3be8, {buffer, 0xd, c.u32(person + person::name_a), pick});
        set_field(c, buffer, 0xc, sext8(c.u8(pick)), 0x101b3bfc);
        out(c, 0x102cb630, 0x101b3c10, {buffer, c.u8(pick), c.u32(person + person::name_b), pick});
        out(c, 0x102cc8e0, 0x101b3c1c, {news_desk, buffer});
        return;
    }
    if (i8(out(c, 0x100d3c40, 0x101b3c28, {club})) == 0) return;
    if (i8(out(c, 0x100c7d00, 0x101b3c38, {club})) != 0 || c.i16(club + club::reputation) > 0x1c52) {
        out(c, 0x102cc8e0, 0x101b3c58, {news_desk, buffer});
    } else if (const auto region = c.u32(club + club::region); region != 0) {
        out(c, 0x102cc400, 0x101b3c74, {news_desk, buffer, region});
    }
}

// ---- 0x101b0f90 ----
bool candidate_willing(Cpu& c, double bar, std::uint32_t person, std::uint32_t club) {
    guest::Frame frame(c, 0x101b0f90, 0x90, {.first_gpr = 27, .gpr_offset = -0x24, .first_fpr = 31});
    guest::Fpu fpu(c);
    const auto pool_at = c.toc(cell::pool);
    const auto today = c.toc(cell::today);
    guest::keep(c, {{28, person}, {29, club}, {30, pool_at}, {31, today}});
    c.set_f(31, bar);  // fmr f31,f1: callees save it
    const auto staff_at = c.u32(person + person::staff);
    if (staff_at == 0 || club == 0) return false;
    const auto region_of = [&](std::uint32_t who) { return c.u32(who + club::region); };
    if (c.i16(staff_at + staff::c) > 0x109a && c.u32(person + person::home) != 0 && region_of(club) != 0 &&
        c.u32(person + person::home) != region_of(club) && c.i8(region_of(club) + 0x11f) < 10)
        return false;
    if (c.i16(staff_at + staff::a) > 0x186a && c.u32(person + person::home) != region_of(club) &&
        c.u8(person + person::status) == 5 && c.i16(club + club::reputation) < 0x1676)
        return false;
    if (i8(out(c, 0x100c7d00, 0x101b105c, {club})) != 0) {
        if (const auto post = c.u32(person + person::post_b);
            post != 0 && (c.u32(post + 0x10) == club || c.u32(post + 0x14) == club || c.u32(post + 0x18) == club))
            return false;
    }
    if (c.u8(person + person::status) == 0xb) {
        const auto post = c.u32(person + person::post_a);
        if (post != 0 && c.i16(post + 0xa) > 0x128e && c.i16(club + club::reputation) < 0x1a5e) {
            const auto deal = out(c, 0x10082f20, 0x101b10dc, {c.u32(c.toc(cell::object)), person});
            guest::keep(c, {{27, deal}});
            if (deal != 0) {
                const auto kind = c.u8(deal + 0x60) & 0xf;
                if (kind < 4 && kind >= 1) return false;
                const auto days = std::int32_t(out(c, 0x10718130, 0x101b1118, {today, c.u32(deal + 0x28), c.u32(deal + 0x2c)}));
                if (days < 0xb4) return false;
                if (c.i8(deal + 0x46) > 1) return false;
                if (c.i8(deal + 0x45) == 0) return false;
                if (std::int8_t(c.u16(person + person::outlook)) < 0x23 && c.i8(deal + 0x44) > 0) return false;
                if (c.u32(person + person::home) != region_of(club) && c.i16(club + club::reputation) < 0x1c52 &&
                    c.u32(person + person::job_club) != club)
                    return false;
            }
        }
    }
    if (c.u8(person + person::status) == 5) {
        const auto rival = c.u32(club + 0x30), owner = c.u32(person + person::job_club);
        if (rival != 0 && owner != 0) {
            const auto owner_rival = c.u32(owner + 0x30);
            if (owner_rival != 0) {
                if (c.i16(rival + 0x18) < c.i16(owner_rival + 0x18)) return false;
                if (rival == owner_rival &&
                    std::int32_t(std::uint32_t(std::int32_t(c.i16(club + club::reputation))) - 0x4e2u) < c.i16(owner + club::reputation))
                    return false;
                if (c.i16(owner + club::reputation) > c.i16(club + club::reputation)) return false;
                const auto days = std::int32_t(out(c, 0x10718130, 0x101b123c, {today, c.u32(person + person::since_a), c.u32(person + person::since_b)}));
                if (days < c.i8(person + person::level) * 10 + 0xd2 &&
                    c.i16(c.u32(person + person::job_club) + club::reputation) > 0x1482)
                    return false;
            }
        }
    }
    if (c.u32(person + person::side_club) != 0 && c.u8(person + person::side_status) == 5) return false;
    {
        const auto staff_now = c.u32(person + person::staff);
        if ((staff_now == 0 || c.i16(staff_now + staff::b) > 0x128e) && c.i16(club + club::reputation) < 0x8ca) return false;
    }
    if (c.u8(person + person::tenure) > 0x4b && c.i16(club + club::reputation) < 0xcb2) return false;
    if (c.u32(person + person::post_a) == 0 &&
        i8(out(c, 0x102eb610, 0x101b12f4, {c.u32(c.toc(cell::clubs_extra)), person})) != 0)
        return false;
    if (c.i16(club + club::reputation) < 0x9c4 && c.u32(person + person::home) == region_of(club) &&
        c.i16(c.u32(person + person::staff) + staff::a) < 0x5dc && c.u32(person + person::job_club) == 0 &&
        c.u32(person + person::side_club) == 0)
        return true;
    if (i8(out(c, 0x100c7d00, 0x101b1358, {club})) != 0 && c.u32(person + person::home) == region_of(club)) {
        if (c.i16(c.u32(person + person::staff) + staff::b) < 0x1a5e) return true;
        if (c.u32(person + person::job_club) == 0) return true;
        const auto days = std::int32_t(out(c, 0x10718130, 0x101b13b0, {today, c.u32(person + person::since_a), c.u32(person + person::since_b)}));
        if (days > c.i8(person + person::level) * 10 + 0xd2) {
            const auto rating = std::int32_t(c.i16(c.u32(person + person::job_club) + club::reputation));
            if (rating < 0x1e46 && rating + 0x2ee < c.i16(club + club::reputation)) return true;
        }
    }
    const std::int32_t rating = c.i16(club + club::reputation);
    if (rating > 0x1e46) {
        // The reputation, scaled by its region's factor (a byte) and folded past 0x203a, for the club and
        // for the person's own club.
        const auto folded = [&](std::uint32_t at) {
            const auto region = c.u32(at + club::region);
            const std::int32_t own = c.i16(at + club::reputation);
            std::int16_t v = region != 0
                                 ? std::int16_t(scaled(fpu, c, pool_at, pool::scale_b, std::int32_t(std::uint32_t(own) * std::uint32_t(c.i8(region + 0x11f)))))
                                 : std::int16_t(scaled(fpu, c, pool_at, pool::scale_c, own));
            if (v > 0x203a)
                v = std::int16_t(std::uint32_t(scaled(fpu, c, pool_at, pool::scale_g, std::int32_t(v) - 0x203a)) + 0x203au);
            return v;
        };
        const auto mine = folded(club);
        if (const auto owner = c.u32(person + person::job_club); owner != 0) {
            const auto theirs = folded(owner);
            const std::int32_t bonus = std::int16_t(c.i8(person + person::level) * 0x19);
            if (std::int32_t(mine) > std::int32_t(theirs) + bonus + 0xfa) return true;
        }
    }
    const auto tactics = c.u32(c.toc(cell::tactics));
    out(c, 0x10366470, 0x101b15ec, {c.u32(tactics + 0x988), person, club, 0, 5, 0});
    fpu.fcmp(0, c.f(1), bar);
    return (c.cr(0) & 8) == 0;
}
}
