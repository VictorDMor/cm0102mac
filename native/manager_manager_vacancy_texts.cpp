// MANAGER_MANAGER.CPP vacancy block (manager_manager_vacancy.hpp), native-first: the texts of three news messages
// (0x29, 0x1789, 0x2b). Each reads the message record's fields through 0x102cb570, formats a text into the shared
// buffer and hands it to the output object (0x1074b830); the first two, with the flag set, also append the club's
// name (0x1074bab0). Text only: no draw, no record of the game written.
#include "manager_manager_vacancy.hpp"
#include "manager_manager_vacancy_detail.hpp"
#include <initializer_list>
#include <utility>

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace vacancy;
namespace {
namespace callee_text {
constexpr std::uint32_t formatter = 0x10727890, to_object = 0x1074b830, name_to_object = 0x1074bab0, field = 0x102cb570,
                        row_name = 0x10168a00, row_text = 0x100c76c0, assert_text = 0x1077b720;
}
constexpr std::uint32_t club_stride = 0x248, row_stride = 0x128, club_name = 0x1ee, club_standing = 0x242, name_tag = 0x7d5;

class Text {
public:
    Text(Cpu& cpu, std::uint32_t message, std::uint32_t out)
        : c(cpu), message(message), out(out), pool(cpu.toc(toc_cell::pool)), text(cpu.toc(toc_cell::text_buffer)) {}
    std::uint32_t field(std::uint32_t index, std::uint32_t back) {
        c.set({{3, message}, {4, index}});
        c.call(callee_text::field, back);
        return c.r(3);
    }
    // The formatter with the format at pool + `offset` (the original's addis/addi pair): `bl` is the call's address.
    void say(std::uint32_t bl, std::uint32_t offset, std::initializer_list<std::pair<unsigned, std::uint32_t>> args = {}) {
        c.set({{3, text}, {4, pool + offset}});
        c.set(args);
        c.call(callee_text::formatter, bl + 4);
    }
    void to_object(std::uint32_t bl) {
        c.set({{3, out}, {4, text}});
        c.call(callee_text::to_object, bl + 4);
    }
    // The club's name appended with the tag; r6 is the record the original keeps in it.
    void name_to_object(std::uint32_t name, std::uint32_t record, std::uint32_t bl) {
        c.set({{3, out}, {4, name}, {5, name_tag}, {6, record}});
        c.call(callee_text::name_to_object, bl + 4);
    }
    void row_name(std::uint32_t row, std::uint32_t buffer, std::uint32_t bl) {
        c.set({{3, row}, {4, buffer}, {5, 0x78}, {6, 9}});
        c.call(callee_text::row_name, bl + 4);
    }
    std::uint32_t clubs_cell() const { return c.toc(toc_cell::clubs); }
    std::uint32_t club_record(std::uint32_t index) const { return c.u32(clubs_cell()) + index * club_stride; }
    Cpu& c;
    std::uint32_t message, out, pool, text;
};
}

// ---- 0x101bede0: message 0x29, a manager's application between two clubs ----
void vacancy_news_text(Cpu& c, std::uint32_t message, std::uint32_t flag_word, std::uint32_t out) {
    guest::Frame frame(c, 0x101bede0, 0xf0, {.first_gpr = 23});
    constexpr std::uint32_t row_buffer = 0x48;
    Text n(c, message, out);
    if (std::int8_t(flag_word) == 0) {
        n.field(0, 0x101bee1c);
        n.field(1, 0x101bee28);
        n.field(2, 0x101bee34);
        n.say(0x101bee40, 0x1fa16);
        n.to_object(0x101bee4c);
        return;
    }
    const auto a = n.field(0, 0x101bee60), b = n.field(1, 0x101bee70);
    n.field(2, 0x101bee80);
    const auto row_index = sext8(std::int8_t(n.field(4, 0x101bee8c)));
    n.field(5, 0x101bee9c);
    const auto rows = c.u32(c.toc(toc_cell::rows));
    const auto row = rows + row_index * row_stride;
    // The registers the original holds from here on (0x1074bab0's frame copies the save area of 0x1074b830).
    c.set({{23, message}, {24, n.pool}, {25, a}, {26, out}, {27, row_index * row_stride}, {28, b}, {29, c.toc(toc_cell::rows)},
           {30, n.text}, {31, n.clubs_cell()}});
    n.row_name(row, frame.at(row_buffer), 0x101beeb4);
    const auto club_a = n.club_record(a), club_b = n.club_record(b);
    const auto standing_a = sext8(c.i8(club_a + club_standing)), standing_b = sext8(c.i8(club_b + club_standing));
    c.set({{24, a * club_stride}, {28, b * club_stride}});
    c.put32(frame.at(0x38), frame.at(row_buffer));
    c.put32(frame.at(0x3c), club_a + club_name);
    c.put32(frame.at(0x40), club_b + club_name);
    n.say(0x101bef14, 0x1fa35, {{5, 100}, {6, 100}, {7, standing_a}, {8, standing_a}, {9, standing_b}, {10, standing_b}});
    n.to_object(0x101bef20);
    n.name_to_object(club_a + club_name, club_a, 0x101bef38);
    c.set({{3, row}});
    c.call(callee_text::row_text, 0x101bef48);
    const auto row_text = c.r(3);
    c.set({{3, out}, {4, frame.at(row_buffer)}, {5, name_tag}, {6, row_text}});
    c.call(callee_text::name_to_object, 0x101bef5c);
    n.name_to_object(club_b + club_name, club_b, 0x101bef70);
}

// ---- 0x101bf070: message 0x1789, the answer a manager gets ----
void answer_news_text(Cpu& c, std::uint32_t message, std::uint32_t flag_word, std::uint32_t out) {
    guest::Frame frame(c, 0x101bf070, 0xe0, {.first_gpr = 23});
    constexpr std::uint32_t row_buffer = 0x38;
    Text n(c, message, out);
    const auto clubs = n.clubs_cell();
    if (std::int8_t(flag_word) == 0) {
        const auto person = n.field(0, 0x101bf318);
        const auto kind = std::int8_t(n.field(1, 0x101bf328));
        c.set({{23, message}, {26, n.text}, {27, clubs}, {28, n.pool}, {30, person}, {31, out}});
        n.say(kind == 2 ? 0x101bf348 : 0x101bf35c, kind == 2 ? 0x203f6 : 0x20424);
        n.to_object(0x101bf368);
        n.name_to_object(n.club_record(person) + club_name, n.club_record(person), 0x101bf384);
        return;
    }
    const auto person = n.field(0, 0x101bf0a8);
    const auto first = std::int8_t(n.field(1, 0x101bf0b8));
    const auto second = std::int8_t(n.field(2, 0x101bf0c8));
    const auto row_index = n.field(3, 0x101bf0d8);
    const auto number = std::int32_t(n.field(4, 0x101bf0e8));
    n.row_name(c.u32(c.toc(toc_cell::rows)) + row_index * row_stride, frame.at(row_buffer), 0x101bf108);
    // The registers the original holds from here on (see vacancy_news_text).
    c.set({{23, message}, {24, sext8(second)}, {25, sext8(first)}, {26, n.text}, {27, clubs}, {28, n.pool}, {29, person},
           {30, std::uint32_t(number)}, {31, out}});
    const auto buffer = frame.at(row_buffer);
    auto say = [&](std::uint32_t bl, std::uint32_t offset) { n.say(bl, offset, {{5, 100}, {6, 100}, {7, buffer}}); };
    if (second == 12) say(0x101bf12c, 0x1fb21);
    else if (second == 4) say(0x101bf154, 0x1fc37);
    else if (second == 10) say(0x101bf17c, 0x1fd0d);
    else if (first == 1) {
        if (second == 1 || second > 1 || second < 0) {
            if (number < 5) say(0x101bf1f8, 0x1fefe);
            else if (number < 6) say(0x101bf220, 0x1ffc9);
            else say(0x101bf240, 0x200d7);
        } else say(0x101bf1d0, 0x1fe28);
    } else if (first < 1 || first >= 3) {
        c.set({{3, n.text}, {4, n.pool + 0x203aa}, {5, buffer}});
        c.call(callee_text::assert_text, 0x101bf2e0);
    } else if (second == 1) say(0x101bf2c4, 0x2030a);
    else if (second < 1) say(second >= 0 ? 0x101bf2a4 : 0x101bf2c4, second >= 0 ? 0x20259 : 0x2030a);
    else if (second == 6) say(0x101bf284, 0x201ad);
    else say(0x101bf2c4, 0x2030a);
    n.to_object(0x101bf2e8);
    const auto club = n.club_record(person);
    n.name_to_object(club + club_name, club, 0x101bf304);
}

// ---- 0x101bf460: message 0x2b, a club's offer ----
void offer_news_text(Cpu& c, std::uint32_t message, std::uint32_t flag_word, std::uint32_t out) {
    guest::Frame frame(c, 0x101bf460, 0xd0, {.first_gpr = 24});
    constexpr std::uint32_t row_buffer = 0x38;
    Text n(c, message, out);
    if (std::int8_t(flag_word) == 0) {
        n.say(0x101bf594, 0x20705);
        n.to_object(0x101bf5a0);
        return;
    }
    const auto row_index = n.field(0, 0x101bf498), a = n.field(1, 0x101bf4a8), b = n.field(2, 0x101bf4b8);
    const auto person_index = n.field(3, 0x101bf4c8);
    const auto table = c.u32(c.toc(-0x7530));
    const auto entry_a = table + a * 0x2c, entry_b = table + b * 0x2c;
    const auto person = c.u32(c.toc(toc_cell::people)) + person_index * 0x70;
    n.row_name(c.u32(c.toc(toc_cell::rows)) + row_index * row_stride, frame.at(row_buffer), 0x101bf50c);
    const auto cell = c.toc(-0x7bb0);
    const auto current = cell + std::uint32_t(std::int32_t(c.i16(cell + 0x2c58))) * 0x2c0;
    const auto buffer = frame.at(row_buffer);
    if (person == c.u32(current + 4)) n.say(0x101bf548, 0x2044b, {{5, 100}, {6, 100}, {7, buffer}, {8, entry_a}, {9, entry_b}});
    else n.say(0x101bf574, 0x20597, {{5, 100}, {6, 100}, {7, 100}, {8, buffer}, {9, entry_a}, {10, entry_b}});
    n.to_object(0x101bf580);
}
}
