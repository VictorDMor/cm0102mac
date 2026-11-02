#include "news_screens_detail.hpp"

namespace cm::aot::news_screens {
using namespace detail;

// The news screen's commands. Each test recomputes the control's action, as the original does.
// `table` stays in r3 through the tests that address through r4; the last test of each chain
// addresses through r3.
std::uint32_t news_command(Cpu& c, std::uint32_t control) {
    Frame f(c, 0x102cfcd0, 0x60, {.first_gpr = 23});
    Kept<27> screens(c, c.toc(slot::screens));
    Kept<29> id(c, control);
    Kept<26> news(c, c.toc(slot::news));
    Kept<25> current(c, screens + 0x2c58);
    Kept<31> manager(c, screen_manager_person(c, screens));
    const History history{c, screens};
    Kept<24> unread_seen(c, std::uint32_t(std::int32_t(std::int8_t(history.value(0, 0x102cfd0c)))));
    Kept<23> tab(c, std::uint32_t(std::int32_t(std::int8_t(history.value(1, 0x102cfd1c)))));
    Kept<28> selected(c, history.value(5, 0x102cfd2c));
    const auto unread_before = history.value(6, 0x102cfd3c);
    c.set(4, tab * 3);
    const bool check_flag = byte_flag(c, unread_seen);
    unread_seen = unread_before;
    Kept<30> tab_values(c, tab * 3 + 9);
    if (check_flag && !byte_flag(c, bl(c, screen_flag, 0x102cfd5c, {screens, manager})))
        return result(c, std::uint32_t(leave_screen));

    const auto control_id = std::int16_t(control);
    if (compare(c, control_id, -1) != 0) {
        const auto chosen = bl(c, history_value, 0x102cfd84, {screens, tab_values + 2});
        const auto row = bl(c, row_for_item, 0x102cfd90, {screens, extended(std::int16_t(chosen))});
        bl(c, set_history_value, 0x102cfda4, {screens, tab_values + 1, row, 0});
    }
    const auto count = bl(c, unread_count, 0x102cfdb4, {c.u32(news), manager, 0});
    if (compare(c, std::int32_t(std::uint32_t(unread_seen)), std::int32_t(count)) != 0) {
        history.set(4, 1, 0x102cfdd0);
        return result(c, std::uint32_t(redraw));
    }

    keep(c, {{24, screens + 0x24}});
    auto table = screen_table(c, screens);
    c.set(3, table);
    const auto action = [&](unsigned via) { return control_action(c, screens, table, control_id, via); };
    const auto value_through_r3 = [&]() { return control_word(c, table, control_id, control_value_word, 3); };
    if (compare(c, action(4), 1) == 0) {
        const auto chosen = std::int8_t(control_word(c, table, control_id, control_value_word, 4));
        if (compare(c, std::int32_t(std::uint32_t(tab)), chosen) != 0) {
            const auto value = value_through_r3();
            history.set(1, value, 0x102cfe90);
            history.set(4, 1, 0x102cfea4);
            return result(c, std::uint32_t(redraw));
        }
    }
    if (compare(c, action(4), 2) == 0) {
        const auto value = value_through_r3();
        history.set(tab_values, value, 0x102cff24);
        return result(c, std::uint32_t(redraw));
    }
    if (compare(c, action(4), 9) == 0) {
        history.set(tab_values, 0, 0x102cff84);
        return result(c, std::uint32_t(redraw));
    }
    bool next_unread = compare(c, action(4), 5) == 0 || compare(c, action(4), 0x3e9) == 0;
    if (!next_unread && compare(c, action(3), 0x3e8) == 0)
        next_unread = byte_flag(c, bl(c, requires_attention, 0x102d0068, {c.u32(news), manager, 0}));
    if (next_unread) {
        table = screen_table(c, screens);
        c.set(3, table);
        if (compare(c, action(3), 5) == 0 && nonzero(c, std::int32_t(bl(c, unread_count, 0x102d00d0, {c.u32(news), manager, tab})))) {
            history.set(tab_values, 0, 0x102d00ec);
            history.set(4, 1, 0x102d0100);
            return result(c, std::uint32_t(redraw));
        }
        history.set(1, 0, 0x102d011c);
        history.set(9, 0, 0x102d0130);
        history.set(7, 1, 0x102d0144);
        history.set(4, 1, 0x102d0158);
        return result(c, std::uint32_t(redraw));
    }

    table = screen_table(c, screens);
    c.set(4, table);
    if (compare(c, action(3), 3) == 0) {
        c.set(12, control_word(c, table, control_id, control_value_word, 3));
        c.set({{4, NewsItem{c, selected}.id()}, {3, manager}});
        c.call(pointer_glue, 0x102d01e0);
        c.set(2, c.u32(f.at(0x14)));
        return result(c, std::uint32_t(popup));
    }
    if (nonzero(c, std::int32_t(bl(c, default_action, 0x102d01f4, {id})))) return c.r(3);
    bl(c, common_action, 0x102d0208, {id});
    return common_action_result(c);
}
}
