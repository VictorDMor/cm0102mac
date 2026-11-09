// MANAGER_MANAGER.CPP manager texts block (manager_manager_texts.hpp): 0x101b76b0, 0x101b7960 and 0x101b5370.
//
// All three keep the original's stack image: the prologue stores, the text buffers the formatter and the
// callees fill, the formatter's extra argument words from 0x38 and the registers the original holds at each
// call (a callee's prologue saves r13-r31) and leaves at its end (r3-r10, cr0). The formatter is varargs: it
// spills r3-r10 into the frame, so each of r3-r10 the original sets for a call is set the same way here, and
// the ones it leaves alone stay as the callee before left them. r0, r11 and r12 at a call are not part of
// that: the original's values are set only where a routine already had them at hand.
#include "manager_manager_texts.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Frame;
using guest::compare;
using guest::compare_logical;
namespace {
namespace callee {
constexpr std::uint32_t field = 0x102cb570,         // a message's field by index
    field_text = 0x102cb7f0,                        // a message's text field (index, buffer, size)
    formatter = 0x10727890, sink_text = 0x1074b830, sink_post = 0x1074bab0, own_text = 0x1019d320,
    club_flag = 0x100c7d00, person_name = 0x100caeb0, name_in_text = 0x1077dda0, post = 0x1077b720,
    people_ids = 0x1019c590, people_text = 0x101980a0, person_text = 0x100cb750, three_words = 0x100cb590,
    person_flag = 0x100cbde0, kind_name = 0x100c6bf0;
}
namespace toc {
constexpr std::int32_t clubs = -0x7be8, people = -0x7be0, people_total = -0x7be4,
                       club_total = -0x7b50, reserve = -0x7b04, news_line = -0x7bb8, id_list = -0x5404;
}
constexpr std::uint32_t club_size = 0x248, person_size = 0x70;
namespace club {
constexpr std::uint32_t name = 0x1ee, name_class = 0x242;
}

std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
// A string of the unit's pool as the original forms it: `addis r4,pool,pages; subi r4,r4,below`.
std::uint32_t text_at(std::uint32_t pool, std::uint32_t pages, std::uint32_t below) {
    return pool + (pages << 16) - below;
}
}  // namespace

// 0x101b76b0. Frame 0x13e0 (r25-r31 saved with stmw, LR at +8).
void post_text(Cpu& c, std::uint32_t object, std::uint32_t message, std::uint32_t flag, std::uint32_t sink) {
    c.set(0, c.lr());
    Frame frame(c, 0x101b76b0, 0x13e0, {.first_gpr = 25});
    const auto clubs = c.toc(toc::clubs), pool = c.toc(slots::pool);
    guest::keep(c, {{25, object}, {26, message}, {27, flag}, {28, pool}, {29, sink}, {30, clubs}});
    const auto text = frame.at(0x38);

    c.set({{3, message}, {4, 0}});
    c.call(callee::field, 0x101b76e4);
    const auto club = c.r(3);
    c.set(31, club);
    c.set({{3, message}, {4, 1}});
    c.call(callee::field, 0x101b76f4);
    compare(c, sbyte(flag));  // extsb. r0,r27
    const auto flags = word(std::int16_t(c.r(3)));
    c.set(5, flags);
    if (sbyte(flag) == 0) {
        // The club's place against the clubs left after the reserve decides the first text; then the flags.
        const auto total = c.u32(c.toc(toc::club_total));
        const auto reserve = c.u32(c.toc(toc::reserve));
        const auto limit = std::int32_t(total - (reserve << 1));
        std::uint32_t below, back;
        compare(c, std::int32_t(club), limit);
        if (std::int32_t(club) > limit) { below = 0x6f6c; back = 0x101b7730; }
        else if (compare(c, std::int32_t(flags & 0x400)) != 0) { below = 0x6f51; back = 0x101b774c; }
        else if (compare(c, std::int32_t(flags & 0x2)) != 0) { below = 0x6f3e; back = 0x101b7768; }
        else if (compare(c, std::int32_t(flags & 0x85)) != 0) { below = 0x6f29; back = 0x101b7784; }
        else if (compare(c, std::int32_t(flags & 0x30)) != 0) { below = 0x6f08; back = 0x101b77a0; }
        else if (compare(c, std::int32_t(flags & 0x308)) != 0) { below = 0x6ee7; back = 0x101b77bc; }
        else { below = 0x6ec6; back = 0x101b77d0; }
        c.set({{3, text}, {4, text_at(pool, 2, below)}});
        c.call(callee::formatter, back);
    } else {
        c.set({{3, object}, {4, c.u32(clubs) + club * club_size}, {6, text}, {7, 0x1388}});
        c.call(callee::own_text, 0x101b77f0);
    }
    c.set({{3, sink}, {4, text}});
    c.call(callee::sink_text, 0x101b77fc);
    const auto record = c.u32(clubs) + club * club_size;
    c.set({{3, sink}, {5, 0x7d5}, {6, record}, {4, record + club::name}});
    c.call(callee::sink_post, 0x101b7818);
    lmw_leaves(c, frame.caller());
}

// 0x101b7960. Frame 0x2860 (r18-r31 saved with stmw, LR at +8).
void post_people_text(Cpu& c, std::uint32_t object, std::uint32_t message, std::uint32_t flag, std::uint32_t sink) {
    c.set(0, c.lr());
    Frame frame(c, 0x101b7960, 0x2860, {.first_gpr = 18});
    constexpr std::uint32_t ids = 0x38, club_text = 0x1494, own = 0x10c, people_buffer = 0xeb8, name = 0x74;
    const auto list = c.toc(toc::id_list), clubs_cell = c.toc(toc::clubs);
    // The 0x3c bytes of ids at the table are copied to the frame (words, then two halfwords); the registers
    // the original loads them through are the callee's arguments at the first call.
    std::uint32_t words[14];
    for (unsigned n = 0; n < 14; ++n) words[n] = c.u32(list + n * 4);
    const auto half_a = c.u16(list + 0x38), half_b = c.u16(list + 0x3a);
    const auto people = c.toc(toc::people), pool = c.toc(slots::pool);
    guest::keep(c, {{18, list}, {19, words[2]}, {20, words[3]}, {21, words[4]}, {22, words[5]}, {23, object},
                    {24, message}, {25, flag}, {26, sink}, {27, 0}, {28, clubs_cell}, {29, people},
                    {30, pool}, {31, words[6]}});
    c.set({{3, message}, {4, 0}, {5, half_a}, {6, words[13]}, {7, words[12]}, {8, words[11]}, {9, words[10]},
           {10, words[9]}, {11, words[8]}, {12, words[7]}});
    for (unsigned n = 0; n < 14; ++n) c.put32(frame.at(ids + n * 4), words[n]);
    c.put16(frame.at(ids + 0x38), half_a);
    c.put16(frame.at(ids + 0x3a), half_b);
    c.call(callee::field, 0x101b7a20);
    const auto club = c.r(3);
    c.set(31, club);
    c.set({{3, message}, {4, 1}});
    c.call(callee::field, 0x101b7a30);
    compare(c, sbyte(flag));  // extsb. r0,r25
    c.set(5, c.r(3));

    std::int32_t count = 0;
    if (sbyte(flag) == 0) {
        // No people: the club's text, by whether the club flag singles it out.
        c.set(3, c.u32(clubs_cell) + club * club_size);
        c.call(callee::club_flag, 0x101b7a4c);
        const bool flagged = compare(c, sbyte(c.r(3))) != 0;
        c.set({{3, frame.at(club_text)}, {4, text_at(pool, 2, flagged ? 0x6eaa : 0x6e83)}});
        c.call(callee::formatter, flagged ? 0x101b7a64 : 0x101b7a78);
    } else {
        const auto record = c.u32(clubs_cell) + club * club_size;
        c.set(18, record);
        c.set({{3, object}, {4, record}, {6, frame.at(own)}, {7, 0x1388}});
        c.call(callee::people_text, 0x101b7a9c);
        c.set({{3, object}, {5, record}, {4, frame.at(people_buffer)}, {6, frame.at(ids)}});
        c.call(callee::people_ids, 0x101b7ab0);
        count = sbyte(c.r(3));
        c.set(27, word(count));
        compare(c, count);  // extsb. r27,r3
        if (count != 0) {
            c.set({{3, frame.at(club_text)}, {4, text_at(pool, 1, 0x47)}, {5, frame.at(own)}, {6, frame.at(people_buffer)}});
            c.call(callee::post, 0x101b7ad0);
        } else {
            c.set({{3, frame.at(club_text)}, {4, text_at(pool, 1, 0x40)}, {5, frame.at(own)}});
            c.call(callee::post, 0x101b7ae8);
        }
    }
    c.set({{3, sink}, {4, frame.at(club_text)}});
    c.call(callee::sink_text, 0x101b7af4);
    compare(c, count);  // cmpwi r27,0
    if (count != 0) {
        // The names of the ids the callee found, each appended to the text and posted when it fits.
        c.set({{23, frame.at(ids)}, {18, 0}});
        for (std::int32_t index = 0;; ++index) {
            compare(c, index, count);
            if (index >= count) break;
            const auto id = c.u32(c.r(23));
            if (compare(c, std::int32_t(id), -1) != 0) {
                c.set({{3, c.u32(people) + id * person_size}, {4, frame.at(name)}, {5, 0x96}, {6, 1}});
                c.call(callee::person_name, 0x101b7b30);
                c.set({{3, frame.at(club_text)}, {4, frame.at(name)}});
                c.call(callee::name_in_text, 0x101b7b3c);
                if (compare_logical(c, c.r(3)) != 0) {
                    const auto again = c.u32(c.r(23));
                    c.set({{3, sink}, {6, c.u32(people) + again * person_size}, {4, frame.at(name)}, {5, 0x7db}});
                    c.call(callee::sink_post, 0x101b7b64);
                }
            }
            c.set(23, c.r(23) + 4);
            c.set(18, std::uint32_t(index + 1));
        }
    }
    const auto offset = club * club_size;
    c.set(18, offset);
    c.set({{3, frame.at(club_text)}, {4, c.u32(clubs_cell) + offset + club::name}});
    c.call(callee::name_in_text, 0x101b7b8c);
    if (compare_logical(c, c.r(3)) != 0) {
        const auto record = c.u32(clubs_cell) + offset;
        c.set({{3, sink}, {5, 0x7d5}, {6, record}, {4, record + club::name}});
        c.call(callee::sink_post, 0x101b7bac);
    }
    lmw_leaves(c, frame.caller());
}

namespace {
// 0x101b5370's work: the message's fields into registers, a text by the kind, and the posts.
class Appointment {
public:
    Appointment(Cpu& c, Frame& frame, std::uint32_t message, std::uint32_t sink)
        : c(c), frame(frame), message(message), sink(sink), pool(c.toc(slots::pool)),
          news_line(c.toc(toc::news_line)), clubs_cell(c.toc(toc::clubs)), people_cell(c.toc(toc::people)),
          text(frame.at(0x134)) {
        guest::keep(c, {{19, c.toc(toc::people_total)}, {20, message}, {21, pool}, {22, people_cell},
                        {27, news_line}, {28, clubs_cell}, {29, sink}});
    }

    // Flag clear: person (r24), three words (r25, r26, r30), the club (r23) and the kind (r18).
    void single() {
        const auto person = field(0, 0x101b53b0);
        c.set(24, person);
        const auto a = field(1, 0x101b53c4);
        c.set(25, a);
        const auto b = field(2, 0x101b53d8);
        c.set(26, b);
        const auto d = field(3, 0x101b53ec);
        c.set(30, d);
        const auto club = field(5, 0x101b5400);
        c.set(23, club);
        const auto kind = word(sbyte(field(6, 0x101b5410)));
        c.set(18, kind);
        const auto total = c.u32(c.r(19));
        compare(c, std::int32_t(person), std::int32_t(total - 0x10));
        if (std::int32_t(person) >= std::int32_t(total - 0x10)) {
            person_by_text(0x100, 0xcc, {0x101b5438, 0x101b5444, 0x101b5458, 0x101b5478});
        } else {
            c.set({{3, a}, {4, b}, {5, d}, {9, person}, {6, text}, {7, 0x190}, {8, 5}});
            c.call(callee::three_words, 0x101b549c);
        }
        // Both arms of the kind test (3 or not) format the same sentence.
        compare(c, std::int32_t(kind), 3);
        line(club, 0x23b, 0x221, 0x7c45, kind == 3 ? 0x101b54dc : 0x101b5514);
        c.set({{3, sink}, {4, news_line}});
        c.call(callee::sink_text, 0x101b5520);
        const auto offset = person * person_size;
        c.set(18, offset);
        const auto people = c.u32(people_cell);
        c.set({{4, a}, {5, b}, {6, d}, {7, 0}, {3, people + offset}});
        c.call(callee::person_flag, 0x101b5540);
        if (compare(c, sbyte(c.r(3))) != 0) post(0x7da, c.u32(people_cell) + offset, 0x101b5560);
        post_club(club, 0x101b557c);
    }

    // Flag set: person (r26), three words (r23, r24, r25), the first club (r30), the second (r31), the
    // kind (r18) and a level (r19).
    void pair() {
        const auto person = field(0, 0x101b558c);
        c.set(26, person);
        const auto a = field(1, 0x101b55a0);
        c.set(23, a);
        const auto b = field(2, 0x101b55b4);
        c.set(24, b);
        const auto d = field(3, 0x101b55c8);
        c.set(25, d);
        const auto first = field(4, 0x101b55dc);
        c.set(30, first);
        const auto second = field(5, 0x101b55ec);
        c.set(31, second);
        const auto kind = word(sbyte(field(6, 0x101b55fc)));
        c.set(18, kind);
        const auto level_field = field(7, 0x101b560c);
        const auto total = c.u32(c.r(19));  // lwz r4,0(r19), before r19 takes the level
        const auto level = word(sbyte(level_field));
        c.set(19, level);
        compare(c, std::int32_t(person), std::int32_t(total - 0x10));
        if (std::int32_t(person) >= std::int32_t(total - 0x10)) {
            person_by_text(0x98, 0x64, {0x101b5634, 0x101b5640, 0x101b5654, 0x101b5674});
        } else {
            c.set({{3, a}, {4, b}, {5, d}, {9, person}, {6, text}, {7, 0x190}, {8, 1}});
            c.call(callee::three_words, 0x101b5698);
        }
        sentence(kind, first, second, level, person);
        c.set({{3, sink}, {4, news_line}});
        c.call(callee::sink_text, 0x101b5a38);
        const auto offset = person * person_size;
        c.set(19, offset);
        c.set({{4, a}, {5, b}, {6, d}, {7, 0}, {3, c.u32(people_cell) + offset}});
        c.call(callee::person_flag, 0x101b5a58);
        if (compare(c, sbyte(c.r(3))) != 0) {
            // A person the flag singles out is posted as one kind when the second club exists and has the
            // club flag, as another otherwise.
            bool club_with_flag = false;
            if (compare(c, std::int32_t(second), -1) > 0) {
                const auto count = c.u32(c.toc(toc::club_total));
                if (compare(c, std::int32_t(second), std::int32_t(count)) < 0) {
                    c.set(3, c.u32(clubs_cell) + second * club_size);
                    c.call(callee::club_flag, 0x101b5a88);
                    club_with_flag = compare(c, sbyte(c.r(3))) != 0;
                }
            }
            post(club_with_flag ? 0x7dc : 0x7d7, c.u32(people_cell) + offset, club_with_flag ? 0x101b5aa8 : 0x101b5ac4);
        }
        post_club(second, 0x101b5ae0);
        if (compare(c, std::int32_t(first), -1) != 0) post_club(first, 0x101b5b04);
    }

private:
    std::uint32_t club_at(std::uint32_t index) { return c.u32(clubs_cell) + index * club_size; }
    std::uint32_t field(std::uint32_t index, std::uint32_t back) {
        c.set({{3, message}, {4, index}});
        c.call(callee::field, back);
        return c.r(3);
    }
    std::uint32_t class_of(std::uint32_t club_record, std::uint32_t offset) { return word(sbyte(c.u8(club_record + offset))); }
    void format(std::uint32_t below, std::uint32_t back) {
        c.set({{3, news_line}, {4, text_at(pool, 2, below)}});
        c.call(callee::formatter, back);
    }
    // The club's class and name (or the fields at the given offsets) and the news text on the stack.
    void line(std::uint32_t index, std::uint32_t class_offset, std::uint32_t name_offset, std::uint32_t below,
              std::uint32_t back) {
        const auto record = club_at(index);
        const auto cls = class_of(record, class_offset);
        c.set({{5, cls}, {6, cls}, {7, record + name_offset}, {8, text}});
        format(below, back);
    }
    void post(std::uint32_t kind, std::uint32_t record, std::uint32_t back) {
        c.set({{3, sink}, {4, text}, {5, kind}, {6, record}});
        c.call(callee::sink_post, back);
    }
    void post_club(std::uint32_t index, std::uint32_t back) {
        const auto record = club_at(index);
        c.set({{3, sink}, {5, 0x7d5}, {6, record}, {4, record + club::name}});
        c.call(callee::sink_post, back);
    }
    // The person is past the human managers: the text of two message texts (fields 8 and 7) goes through the
    // person text routine. `back` holds the return addresses of the four calls; the buffers are at the offsets.
    void person_by_text(std::uint32_t first_buffer, std::uint32_t second_buffer, const std::uint32_t (&back)[4]) {
        c.set({{3, message}, {4, 8}, {5, frame.at(first_buffer)}, {6, 0x33}});
        c.call(callee::field_text, back[0]);
        const auto kind = field(7, back[1]);
        c.set({{3, message}, {4, word(sbyte(kind))}, {5, frame.at(second_buffer)}, {6, 0x33}});
        c.call(callee::field_text, back[2]);
        c.set({{3, frame.at(first_buffer)}, {4, frame.at(second_buffer)}, {5, 0}, {6, text}, {7, 0x190}, {8, 1},
               {9, 0xffffffffu}});
        c.call(callee::person_text, back[3]);
    }

    // 0x101b5698..0x101b5a2c: the sentence of the kind (1, 2 or 3; none for the others) about the two clubs.
    void sentence(std::uint32_t kind, std::uint32_t first, std::uint32_t second, std::uint32_t level,
                  std::uint32_t person) {
        compare(c, std::int32_t(first), -1);
        if (first == 0xffffffffu) return line(second, club::name_class, club::name, 0x76d0, 0x101b5a2c);
        const auto k = std::int32_t(kind);
        compare(c, k, 2);
        if (k == 2) return kind_two(first, second, level, person);
        if (k > 2) {
            compare(c, k, 4);
            if (k >= 4) return;
            return two_clubs(first, second, 0x777f, 0x101b59f4);  // kind 3
        }
        compare(c, k, 1);
        if (k < 1) return;
        compare(c, std::int32_t(second), std::int32_t(first));  // cmpw r31,r30
        if (second == first) return line(second, club::name_class, club::name, 0x7bef, 0x101b5700);
        two_clubs_first(first, second);
    }

    // Kind 1, two clubs: both classes and names, the text on the stack.
    void two_clubs_first(std::uint32_t first, std::uint32_t second) {
        const auto club_first = club_at(first), club_second = club_at(second);
        const auto class_first = class_of(club_first, club::name_class), class_second = class_of(club_second, club::name_class);
        c.put32(frame.at(0x38), text);
        c.set({{5, class_second}, {6, class_second}, {7, class_first}, {8, class_first},
               {9, club_second + club::name}, {10, club_first + club::name}});
        format(0x7b78, 0x101b5754);
    }
    // Kind 3: the first club's name on the stack, the second's name and the text in registers.
    void two_clubs(std::uint32_t first, std::uint32_t second, std::uint32_t below, std::uint32_t back) {
        const auto club_first = club_at(first), club_second = club_at(second);
        const auto class_first = class_of(club_first, club::name_class), class_second = class_of(club_second, club::name_class);
        c.put32(frame.at(0x38), club_first + club::name);
        c.set({{5, class_second}, {6, class_second}, {7, class_first}, {8, class_first},
               {9, club_second + club::name}, {10, text}});
        format(below, back);
    }
    // Kind 2: by the person's flag byte (+0x62 of the person), the level (r19) and the two clubs.
    void kind_two(std::uint32_t first, std::uint32_t second, std::uint32_t level, std::uint32_t person) {
        const auto record = c.u32(people_cell) + person * person_size;
        if (compare(c, sbyte(c.u8(record + 0x62))) == 0) return line(second, club::name_class, club::name, 0x77d6, 0x101b59a0);
        compare(c, std::int32_t(second), std::int32_t(first));
        if (second == first) {
            const bool has_level = compare(c, sbyte(level)) != 0 && compare(c, std::int32_t(level), 0xb) != 0;
            if (has_level) {
                c.set({{3, level}, {4, 0}, {5, 0}, {6, frame.at(0x61)}});
                c.call(callee::kind_name, 0x101b579c);
                const auto result = c.r(3);
                const auto record_second = club_at(second);
                const auto cls = class_of(record_second, club::name_class);
                const auto byte = word(sbyte(c.u8(frame.at(0x61))));
                c.put32(frame.at(0x38), text);
                c.set({{5, cls}, {6, cls}, {7, byte}, {8, byte}, {9, record_second + club::name}, {10, result}});
                return format(0x7adf, 0x101b57e4);
            }
            const bool eleven = compare(c, std::int32_t(level), 0xb) == 0;
            return line(second, club::name_class, club::name, eleven ? 0x7a5e : 0x79e1, eleven ? 0x101b5824 : 0x101b585c);
        }
        c.set({{3, level}, {4, 0}, {5, 0}, {6, frame.at(0x60)}});
        c.call(callee::kind_name, 0x101b5874);
        const auto result = c.r(3);
        const bool eleven = compare(c, std::int32_t(level), 0xb) == 0;
        const auto club_first = club_at(first), club_second = club_at(second);
        const auto class_first = class_of(club_first, club::name_class), class_second = class_of(club_second, club::name_class);
        const auto byte = word(sbyte(c.u8(frame.at(0x60))));
        if (eleven) {
            c.put32(frame.at(0x38), club_second + club::name);
            c.put32(frame.at(0x3c), club_first + club::name);
            c.put32(frame.at(0x40), result);
            c.put32(frame.at(0x44), text);
            c.set({{5, class_second}, {6, class_second}, {7, class_first}, {8, class_first}, {9, byte}, {10, byte}});
            return format(0x7974, 0x101b58e4);
        }
        const std::uint32_t slots[] = {class_first, class_first, byte, byte, club_second + club::name,
                                       club_first + club::name, result, text, club_first + club::name, result};
        for (unsigned n = 0; n < 10; ++n) c.put32(frame.at(0x38 + n * 4), slots[n]);
        c.set({{5, class_second}, {6, class_second}, {7, class_first}, {8, class_first}, {9, byte}, {10, byte}});
        format(0x78d6, 0x101b5968);
    }

    Cpu& c;
    Frame& frame;
    std::uint32_t message, sink, pool, news_line, clubs_cell, people_cell, text;
};
}  // namespace

// 0x101b5370. Frame 0x300 (r18-r31 saved with stmw, LR at +8).
void appointment_text(Cpu& c, std::uint32_t, std::uint32_t message, std::uint32_t flag, std::uint32_t sink) {
    c.set(0, c.lr());
    Frame frame(c, 0x101b5370, 0x300, {.first_gpr = 18});
    Appointment notice(c, frame, message, sink);
    compare(c, sbyte(flag));  // extsb. r0,r5
    if (sbyte(flag) == 0) notice.single();
    else notice.pair();
    lmw_leaves(c, frame.caller());
}
}  // namespace cm::aot::manager_manager
