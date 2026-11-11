// MANAGER_MANAGER.CPP vacancy block (manager_manager_vacancy.hpp), native-first: 0x101bdf80, the board's answer to a
// manager's request. Values live in native locals; the guest ABI appears only at the calls to code outside the block
// (the club-news filers 0x101bef90, 0x101bec80 and 0x101bf3a0 are the block's own C++ functions).
#include "manager_manager_assess_detail.hpp"
#include "manager_manager_review.hpp"
#include "manager_manager_vacancy.hpp"
#include "manager_manager_vacancy_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace vacancy;
namespace {
namespace cell {
constexpr std::int32_t clubs_rows = -0x7ab4, searcher = -0x7a64, quarter = -0x7ba0, people_index = -0x7b7c, search = -0x7a60,
                       interest = -0x7428, events = -0x7b94, scout = -0x7ab8, board = -0x7984, standing = -0x79f0,
                       limits = -0x7b44, constants = -0x536c, array_ctor = -0x7acc;
}
namespace call {
constexpr std::uint32_t array_new = 0x10773320, list_init = 0x10738c00, date_init = 0x10715880, list_free = 0x10738c20,
                        injured = 0x100c7d00, event = 0x106a15f0, club_state = 0x1014c2e0, scout_note = 0x10103510,
                        scout_flag = 0x100f70a0, copy_a = 0x10717cb0, copy_b = 0x10717a10, search = 0x1048b180,
                        next_a = 0x10738e30, next_b = 0x10738e00, interest = 0x102d58c0, interest_byte = 0x102d5880,
                        club_field_a = 0x100c4400, club_field_b = 0x100c4cf0, interest_update = 0x102d51a0,
                        move_slot = 0x10191fd0, date_make = 0x107156b0, job_check = 0x102b7c70, person_find = 0x10082f20,
                        role_check = 0x10187680, copy_move = 0x10739ef0, move_remove = 0x1048a140, move_add = 0x10489900,
                        move_apply = 0x104888f0;
}
// The row of the news table (0x4c bytes per person): a counter byte, the reputation halfword at +6.
constexpr std::uint32_t row_stride = 0x4c, row_count = 0, row_reputation = 6;
// A club's interest record (0x2c bytes): the two words +4 and +8, its link at +0xc, the byte +0x25 and halfwords.
constexpr std::uint32_t interest_stride = 0x2c;
// A move node: the person's day halfword (+0x2a) and year (+0x28), the club link (+0x14), the byte +0x43.
constexpr std::uint32_t frame_list = 0x70, frame_date = 0x68, frame_copy = 0x80;

class Request {
public:
    Request(Cpu& cpu, guest::Frame& f, std::uint32_t table, std::uint32_t person, std::uint32_t kind_word)
        : c(cpu), frame(f), table(table), person(person), kind_word(kind_word), kind(std::int8_t(kind_word)),
          date(cpu.toc(toc_cell::date)), draws(cpu), fpu(cpu) {}

    void run() {
        c.set({{3, frame.at(0x1f0)}, {4, c.toc(cell::array_ctor)}, {5, 0}, {6, 0x664}, {7, 2}});
        c.call(call::array_new, 0x101bdfe0);
        c.set(3, frame.at(frame_list));
        c.call(call::list_init, 0x101bdff4);
        c.set(3, frame.at(frame_date));
        c.call(call::date_init, 0x101be000);
        if (!person || !c.u32(person + 0x30) || !c.u32(person + 0x90) || !c.u32(c.u32(person + 0x90) + 0x1c))
            return release(0x101be038);
        c.set(3, person);
        c.call(call::injured, 0x101be044);
        if (std::int8_t(c.r(3)) != 0) return release(0x101be058);
        const auto id = c.u32(person);
        const auto row = row_of(id);
        c.put8(row + row_count, c.u8(row + row_count) + 1);
        // The limit the manager's reputation gives. The original's other values (4 above 0x1482 and 7 above
        // 0x1e46, 0x101be088..0x101be0a0) test a reputation already known to be at most 0xcb2: never reachable.
        const std::int8_t limit = c.i16(person + 0x1ec) > 0xcb2 ? 3 : 2;
        const auto count = [&] { return std::int8_t(c.u8(row + row_count)); };
        if (kind == 1) {
            if (limit == count() && std::int8_t(c.u8(c.u32(c.toc(cell::board)) + id * 8 + 4)) < -1) {
                event(4, 0xffffffff, 0xffffffff, 0xffffffff, 0x101be114);
                return release(0x101be120);
            }
            if (limit < count()) {
                club_state(0x101be14c);
                if (std::int8_t(c.r(3)) != 0 || limit + 1 == std::int32_t(count())) return accepted(limit);
            }
        }
        if (limit == count() && kind != 5) {
            answer(3, 0);
            return release(0x101be32c);
        }
        if (limit < count() && kind != 5) {
            answer(5, 0);
            const auto again = row_of(c.u32(person));  // read again after the news (0x101be364)
            c.put16(again + row_reputation, std::uint16_t(c.u16(again + row_reputation) - 100));
            return release(0x101be38c);
        }
        club_state(0x101be3a8);
        if (std::int8_t(c.r(3)) == 0) return release(0x101be3bc);
        if (kind == 2) return weigh_move(id);
        if (kind == 1) return find_interest();
        release(0x101bec60);
    }

private:
    Cpu& c;
    guest::Frame& frame;
    std::uint32_t table, person, kind_word;
    std::int8_t kind;
    std::uint32_t date;
    assess::Draws draws;
    guest::Fpu fpu;
    std::uint32_t row_of(std::uint32_t id) const { return c.u32(table) + id * row_stride; }
    std::uint32_t standing_object() const { return c.u32(c.toc(cell::standing)); }

    void release(std::uint32_t back) {
        c.set({{3, frame.at(frame_list)}, {4, 0xffffffff}});
        c.call(call::list_free, back);
    }
    void answer(std::int8_t a, std::int8_t b) { file_answer_news(c, person, std::int8_t(kind_word), a, b); }
    void event(std::uint32_t code, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t back) {
        c.set({{3, c.u32(c.toc(cell::events))}, {4, person}, {5, c.u32(person + 0x90)}, {6, code}, {7, a}, {8, b}, {9, d}});
        c.call(call::event, back);
    }
    void club_state(std::uint32_t back) {
        c.set({{3, person}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(call::club_state, back);
    }
    std::uint32_t interest_record(std::int32_t slot) const {
        return c.u32(c.toc(cell::clubs_rows)) + std::uint32_t(slot) * interest_stride;
    }
    std::uint32_t next(std::uint32_t function, std::uint32_t back) {
        c.set(3, frame.at(frame_list));
        c.call(function, back);
        return c.r(3);
    }
    void copy_date(std::uint32_t function, std::uint32_t at, std::uint32_t back) {
        c.set({{3, frame.at(at)}, {4, date}, {5, 8}});
        c.call(function, back);
    }
    // The search of the manager's pending moves into the list object; the two stack words are its ninth and tenth
    // arguments.
    void search_moves(std::uint32_t p4, std::uint32_t p5, std::uint32_t p6, std::uint32_t p7, std::uint32_t back) {
        c.put32(frame.at(0x38), 0xfffffffe);
        c.put32(frame.at(0x3c), 1);
        c.set({{3, c.u32(c.toc(cell::search))}, {4, p4}, {5, p5}, {6, p6}, {7, p7}, {8, c.u32(person)},
               {9, frame.at(frame_list)}, {10, 0xffffffff}});
        c.call(call::search, back);
    }
    std::uint32_t search_object() const { return c.u32(c.toc(cell::search)); }

    // The club answers with an event (the 0x1014c2e0 test passed in the first block): a draw, the scout's note and
    // the row's reputation.
    void accepted(std::int8_t limit) {
        const auto d = draws.draw(0x101be198);
        event(5, 0xffffffff, 0xffffffff, sext8(std::int8_t(d % 3 + 1)), 0x101be1f4);
        c.set({{3, c.u32(c.toc(cell::scout))}, {4, person}});
        c.call(call::scout_note, 0x101be204);
        if (const auto note = c.r(3)) {
            c.set({{3, note}, {4, 0x61a8}});
            c.call(call::scout_flag, 0x101be214);
        }
        std::int16_t delta = 0xfa;
        const auto id = c.u32(person);  // read again after the calls (0x101be21c)
        if (std::int8_t(c.u8(c.u32(c.toc(cell::board)) + id * 8 + 4)) < 0)
            delta = std::int16_t(std::uint32_t(std::int32_t(limit)) * 0xfa - sext8(std::int8_t(c.u8(row_of(id)))));
        // The person record is known to be non-null (0x101be000), so the original's null test at 0x101be25c never
        // fails.
        auto entry = c.u32(standing_object()) + id * row_stride;
        c.put16(entry + row_reputation, std::uint16_t(c.u16(entry + row_reputation) + std::uint16_t(delta)));
        entry = c.u32(standing_object()) + c.u32(person) * row_stride;
        const auto value = c.i16(entry + row_reputation);
        c.put16(entry + row_reputation, std::uint16_t(value < 1 ? 1 : value > 0x2710 ? 0x2710 : value));
        review(c, standing_object(), person, 0);
        release(0x101be2d8);
    }

    // Kind 1: the first pending move of the manager that has a club's index (a byte >= 0 at +0x43) and the club's
    // interest in him.
    void find_interest() {
        const std::uint32_t year = c.u16(date + 2), day = c.u16(date);
        copy_date(call::copy_a, 0x58, 0x101be3f8);
        const std::uint32_t first_year = c.u16(frame.at(0x5a));
        copy_date(call::copy_a, 0x60, 0x101be40c);
        search_moves(c.u16(frame.at(0x60)), first_year, day, year, 0x101be448);
        bool found = false;
        std::int32_t slot = 0;
        if (c.i16(frame.at(frame_list)) != 0) {
            c.put32(frame.at(frame_list + 0xc), c.u32(frame.at(frame_list + 8)));  // (kind 2 starts from +4)
            while (const auto node = next(call::next_a, 0x101be48c)) {
                if (c.i8(node + 0x43) <= -1) continue;
                slot = c.i16(node + 0x2e);
                found = true;
                break;
            }
        }
        if (!found) return release(0x101bec60);
        // The original reads the table's base (cell::clubs_rows) again after every call: the record's address too.
        const auto rec = [&] { return interest_record(slot); };
        c.set({{3, c.u32(c.toc(cell::interest))}, {4, rec()}});
        c.call(call::interest, 0x101be4b4);
        fpu.fcmp(0, c.f(1), c.f64(c.toc(cell::constants) + 0x2d0));
        bool refuse = (c.cr(0) & 8) == 0;  // bge: taken unless less than (an unordered compare takes it)
        if (!refuse) {
            c.set({{3, c.u32(c.toc(cell::interest))}, {4, rec()}});
            c.call(call::interest_byte, 0x101be4d8);
            refuse = std::int8_t(c.r(3)) >= 7;
        }
        if (refuse) {
            c.set({{3, c.u32(c.toc(cell::interest))}, {4, rec()}});
            c.call(call::interest_byte, 0x101be844);
            answer(1, std::int8_t(c.r(3)));
            return release(0x101bec60);
        }
        if (remainder(draws.draw(0x101be504), 200) != 0) {
            c.set({{3, c.u32(c.toc(cell::interest))}, {4, rec()}});
            c.call(call::interest_byte, 0x101be554);
            answer(1, std::int8_t(c.r(3)));
            return release(0x101bec60);
        }
        const auto before = c.u32(c.u32(rec() + 8) + 0x20);
        c.set(3, c.u32(rec() + 0xc));
        c.call(call::club_field_a, 0x101be588);
        c.put32(rec() + 4, c.r(3));
        c.set(3, c.u32(rec() + 0xc));
        c.call(call::club_field_b, 0x101be5a4);
        c.put32(rec() + 8, c.r(3));
        // Each value is drawn before the record's address is read again (the draws are calls too).
        std::uint32_t spirit;
        if (remainder(draws.draw(0x101be5d0), 10) == 0) {
            const auto a = remainder(draws.draw(0x101be680), 11);
            const auto b = remainder(draws.draw(0x101be6d4), 11);
            spirit = std::uint32_t(a + b);
        } else {
            spirit = std::uint32_t(draws.draw(0x101be628) & 15) + 5;
        }
        c.put8(rec() + 0x25, spirit);
        const auto year_offset = std::uint32_t(remainder(draws.draw(0x101be73c), 5));
        c.put16(rec() + 0x1e, std::uint16_t(c.u16(date + 2) + year_offset - 0x1b));
        const auto stamp = std::uint16_t(remainder(draws.draw(0x101be7ac), 0x154));
        c.put16(rec() + 0x1c, stamp);
        c.set({{3, c.u32(c.toc(cell::interest))}, {4, rec()}});
        c.call(call::interest_update, 0x101be808);
        file_offer_news(c, c.u32(person + 0x90), c.u32(person + 0x2c), before, c.u32(c.u32(rec() + 8) + 0x20));
        release(0x101bec60);
    }

    // Kind 2: the club's pending move against the manager's squad and the date.
    void weigh_move(std::uint32_t id) {
        // (the original tests the person for null at 0x101be860: it is not, see run())
        c.set(3, person);
        c.call(call::move_slot, 0x101be870);
        const auto slot = std::int8_t(c.r(3));
        copy_date(call::copy_b, 0x48, 0x101be890);
        const std::uint32_t first_year = c.u16(frame.at(0x4a));
        copy_date(call::copy_b, 0x50, 0x101be8a4);
        const std::uint32_t year = c.u16(date + 2);
        search_moves(c.u16(date), year, c.u16(frame.at(0x50)), first_year, 0x101be8e8);
        std::uint32_t found = 0;
        if (c.i16(frame.at(frame_list)) != 0) {
            c.put32(frame.at(frame_list + 0xc), c.u32(frame.at(frame_list + 4)));
            while (const auto node = next(call::next_b, 0x101be944)) {
                // A move dated today is skipped, and so is one whose byte +0x43 is not -1.
                if (std::int32_t(c.u16(node + 0x2a)) == std::int32_t(c.i16(date)) && c.u16(node + 0x28) == year) continue;
                if (c.i8(node + 0x43) != -1) continue;
                found = node;
                break;
            }
        }
        if (!found) return release(0x101be960);
        c.set({{3, frame.at(0x40)}, {4, std::uint32_t(std::int32_t(c.i16(found + 0x2a)))}, {5, c.u16(found + 0x28)}});
        c.call(call::date_make, 0x101be978);
        c.put16(frame.at(0x6a), c.u16(frame.at(0x42)));
        c.put16(frame.at(0x68), c.u16(frame.at(0x40)));
        c.put8(frame.at(0x6c), c.u8(frame.at(0x44)));
        std::int8_t jobs = 0, veterans = 0;
        for (std::uint32_t k = 0; k < 0x32; ++k) {
            const auto held = c.u32(person + 0x98 + 4 * k);
            if (!held) continue;
            c.set({{3, c.u32(c.toc(cell::searcher))}, {4, held}, {5, frame.at(frame_date)}, {6, person},
                   {7, c.toc(toc_cell::text_buffer)}, {8, 0}});
            c.call(call::job_check, 0x101be9c0);
            if (c.r(3)) ++jobs;
        }
        for (std::uint32_t k = 0; k < 0x32; ++k) {
            const auto held = c.u32(person + 0x98 + 4 * k);
            if (!held) continue;
            c.set({{3, c.u32(c.toc(cell::people_index))}, {4, held}});
            c.call(call::person_find, 0x101bea00);
            const auto found_person = c.r(3);
            if (!found_person || c.i8(found_person + 0x44) <= 0x14) continue;
            c.set({{3, c.u32(c.toc(cell::quarter))}, {4, held}});
            c.call(call::role_check, 0x101bea24);
            if (std::int8_t(c.r(3)) == 2) ++veterans;
        }
        if (c.u32(found + 0x14) != c.u32(person + 0x30)) {
            answer(6, 0);
            return release(0x101bea78);
        }
        if (slot >= 0 && slot < 0x22) {
            const auto entry = c.toc(cell::limits) + std::uint32_t(slot) * row_stride;
            const std::int32_t h = c.i16(entry + 0x3c);
            if (h > 0x3c) {
                if (year == c.u16(entry + 0x3e) && h - 0x3c < c.i16(date)) {
                    answer(6, 0);
                    return release(0x101beaf4);
                }
            } else if (year == c.u16(entry + 0x3e) && c.i16(date) > 0x154) {
                answer(6, 0);
                return release(0x101beb38);
            }
        }
        bool accept = jobs > 2;
        if (!accept) {
            bool draw = jobs > 1 || veterans > 4;
            if (draw) {
                const auto value = std::int16_t(std::uint32_t(remainder(draws.draw(0x101beb7c), 0x4e2)) + 0x128e);
                accept = c.i16(person + 0x1ec) > value;
            }
        }
        if (!accept) {
            answer(1, 0);
            return release(0x101bec60);
        }
        c.set({{3, frame.at(frame_copy)}, {4, found}, {5, 0x50}});
        c.call(call::copy_move, 0x101bebd8);
        c.set({{3, search_object()}, {4, found}});
        c.call(call::move_remove, 0x101bebe8);
        answer(0, 0);
        c.set({{3, search_object()}, {4, frame.at(frame_copy)}});
        c.call(call::move_add, 0x101bec10);
        c.set({{3, search_object()}, {4, frame.at(frame_copy)}, {5, 1}});
        c.call(call::move_apply, 0x101bec24);
        file_vacancy_news(c, found, id, jobs);
        release(0x101bec60);
    }
};
}

void answer_request(Cpu& c, std::uint32_t table, std::uint32_t person, std::uint32_t kind) {
    guest::Frame frame(c, 0x101bdf80, 0xf20, {.first_gpr = 13});
    Request(c, frame, table, person, kind).run();
}
}
