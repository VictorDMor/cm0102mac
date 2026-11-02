#include "news_screens_detail.hpp"

namespace cm::aot::news_screens {
using namespace detail;

// Registers the screen's values with the manager: the flag, seven words, the filter text (the
// one it owns), and for each of the eight tabs the selected item, the scroll and the table.
void open_news(Cpu& c, std::uint32_t person, std::uint32_t check_attention) {
    Frame f(c, 0x102ce710, 0x80, {.first_gpr = 26});
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<26> id(c, person);
    Kept<27> flag(c, check_attention);
    Kept<29> assert_buffer(c, c.toc(slot::assert_buffer));
    Kept<30> pool(c, c.toc(slot::pool));
    if (!byte_flag(c, bl(c, broadcast_history, 0x102ce754, {screens, id, c.toc(slot::draw_vector), c.toc(slot::command_vector), 0, c.toc(slot::close_vector), 0}))) return;
    Kept<28> filter(c, bl(c, allocate, 0x102ce764, {0x1e}));
    if (compare(c, std::int32_t(std::uint32_t(filter))) == 0) {
        report_assertion(c, Assertion{.at = 0x102ce780, .file = pool, .format = pool + phrase::assert_format, .title = pool + phrase::assert_title,
                                      .name = f.at(0x40), .extension = f.at(0x38), .line = 0x66});
        c.set(3, c.toc(-0x7bac));
        return;
    }
    // The compiler unrolled this loop by ten.
    for (std::uint32_t at = 0; at < 0x1e; ++at) c.put8(filter + at, at == 0x1d ? 0 : 0x20);
    compare(c, 0x1d, 0x1d);
    c.set({{5, 0}, {6, 0x1e}});
    bl(c, format_text, 0x102ce8dc, {filter, pool + phrase::filter_placeholder});
    const auto add = [&](std::uint32_t index, std::uint32_t value, std::uint32_t owned, std::uint32_t back) {
        bl(c, broadcast_history_value, back, {screens, id, index, value, owned});
    };
    add(0, std::uint32_t(std::int32_t(std::int8_t(flag))), 0, 0x102ce8f4);
    for (std::uint32_t index = 1; index < 8; ++index) add(index, 0, 0, 0x102ce90c + (index - 1) * 0x18);
    add(8, filter, 1, 0x102ce9b4);
    keep(c, {{30, 0}, {29, 0}});
    for (std::int32_t tab = 0; tab <= 7; ++tab) {
        c.set(28, std::uint32_t(tab) * 3 + 9);
        add(c.r(28), 0, 0, 0x102ce9d8);
        add(c.r(28) + 1, 0, 0, 0x102ce9f0);
        add(c.r(28) + 2, 0, 0, 0x102cea08);
        keep(c, {{30, std::uint32_t(tab) + 1}, {29, (std::uint32_t(tab) + 1) * 3}});
        compare(c, tab + 1, 7);
    }
}

void close_news(Cpu& c) {
    Frame f(c, 0x102d0230, 0x40, {.first_gpr = 31});
    Kept<31> screens(c, c.toc(slot::screens));
    const History history{c, screens};
    if (compare_logical(c, history.value(2, 0x102d0250)) == 0) return;
    bl(c, release_list, 0x102d0268, {history.value(2, 0x102d0264)});
    history.set(2, 0, 0x102d027c);
    history.set(3, 0, 0x102d0290);
}

namespace {
// What both ways into the dialog do: the recipient, a new message buffer and the item it answers.
// The original keeps zero in r31 for the last call, whose result is a byte it clears.
void fill_message(Cpu& c, std::uint32_t screens, std::uint32_t person, std::uint32_t answered, std::uint32_t back) {
    const History history{c, screens};
    history.set(0, person, back);
    const auto buffer = bl(c, allocate, back + 8, {0x65});
    history.set(1, buffer, back + 0x1c);
    history.set(2, answered, back + 0x30);
    c.set(3, screens);
    keep(c, {{31, 0}});
    c.set(4, 1);
    c.call(history_value, back + 0x40);
    c.put8(c.r(3), c.r(31));
}
}

void open_message(Cpu& c, std::uint32_t recipient, std::uint32_t reply_to) {
    Frame f(c, 0x102d02b0, 0x50, {.first_gpr = 29});
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<30> answered(c, reply_to);
    Kept<29> person(c, recipient);
    if (byte_flag(c, bl(c, push_history, 0x102d02f0, {screens, c.toc(slot::message_draw_vector), c.toc(slot::message_command_vector), 1, 0, 0})))
        fill_message(c, screens, person, answered, 0x102d030c);
}

void open_reply(Cpu& c, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x102d0370, 0x140, {.first_gpr = 27});
    Kept<27> news_id(c, item);
    Kept<28> command_vector(c, c.toc(slot::message_command_vector));
    Kept<29> draw_vector(c, c.toc(slot::message_draw_vector));
    Kept<31> owner(c, manager);
    Kept<30> screens(c, c.toc(slot::screens));
    const auto found = f.at(0x38);
    bl(c, construct_item, 0x102d03a4, {found, 0, 0});
    bl(c, find_item, 0x102d03bc, {c.u32(c.toc(slot::news)), owner, news_id, found});
    owner = bl(c, item_word, 0x102d03c8, {found, 0});
    const auto from_person = bl(c, item_word, 0x102d03d8, {found, 1});
    if (byte_flag(c, from_person)) {
        const std::uint32_t person = c.u32(c.toc(slot::people)) + owner * 0x70;
        const std::uint32_t command = command_vector;
        command_vector = person;
        if (byte_flag(c, bl(c, push_history, 0x102d040c, {screens, draw_vector, command, 1, 0, 0})))
            fill_message(c, screens, person, news_id, 0x102d0428);
    } else if (byte_flag(c, bl(c, push_history, 0x102d048c, {screens, draw_vector, command_vector, 1, 0, 0}))) {
        fill_message(c, screens, 0, news_id, 0x102d04a8);
    }
}
}
