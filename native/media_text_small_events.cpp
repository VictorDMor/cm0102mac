// The media response dialog's event handler (0x1067ca30): which control was used decides between
// an answer button (the answer is remembered and the three answer labels re-mark), OK (the
// press story is filed against the club and its people) and Cancel or anything else.
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"
#include "media.hpp"
#include "media_eval_d.hpp"
#include "media_pick_c.hpp"
#include "media_store.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

namespace {
namespace routines {
constexpr std::uint32_t club_reaction = 0x1069ed10, signing = 0x10672800, confirm = 0x102cd0c0, set_label = 0x10741a80,
    refresh = 0x10742ec0, key_press = 0x102a54b0, default_key = 0x102a83a0;
}
constexpr std::uint32_t answer_command = 0x1485, ok_command = 0x109b, cancel_command = 0x109c;
constexpr std::uint32_t control_size = 0x17c;
}

void media_response_event(Cpu& c, std::uint32_t control) {
    Frame frame(c, 0x1067ca30, 0x160, {.first_gpr = 17});
    keep(c, {{31, c.toc(-0x7bb0)}, {28, control}, {17, c.toc(-0x7bd0)}, {21, c.toc(-0x7bc4)}, {20, c.toc(-0x7b94)},
             {18, c.toc(-0x7bf0)}, {23, c.toc(slot::clubs)}, {19, c.toc(-0x7be0)}});
    const auto world = c.r(31);
    Kept<25> v0{c, world_value(c, world, 0, 0x1067ca6c)};
    Kept<27> club_index{c, world_value(c, world, 1, 0x1067ca80)};
    Kept<24> kind{c, world_value(c, world, 2, 0x1067ca94)};
    Kept<30> answer{c, world_value(c, world, 3, 0x1067caa8)};
    Kept<29> staff{c, world_value(c, world, 4, 0x1067cabc)};
    const auto last = world_value(c, world, 9, 0x1067cad0);
    c.set(4, window_index(c, world));
    Kept<22> other_club{c, last};
    const auto window = window_record(c, world);
    c.set(3, window);
    const auto flag = std::int8_t(c.u8(world + 0x2c5f));
    Kept<26> window_owner{c, c.u32(window + 4)};
    if (compare(c, flag) != 0) {
        c.put32(world + 0x260000 - 0x7e5c, 0);
        c.set(3, 0xfffffff7u);  // -9
        return;
    }
    // The control used: its command from the pane's control table, or the default when it has none.
    const auto pane = c.u32(window + 0x24);
    const std::int32_t id = std::int16_t(control);
    const auto record = pane + 0xc0000 + std::uint32_t(id) * control_size;
    const bool none = compare(c, id, -1) == 0;
    c.set(3, none ? pane : record);
    auto raw = none ? 0 : c.u32(record - 0x5794);
    if (compare(c, std::int16_t(raw)) == 0) {
        c.set(3, world + 0x260000);
        raw = c.u16(world + 0x260000 - 0x7e62);
    }
    const std::int32_t command = std::int16_t(raw);
    const auto persons = [&](std::uint32_t index) { return c.u32(c.r(19)) + index * row_size; };
    const auto clubs_base = [&] { return c.u32(c.r(23)); };
    const auto clubs = [&](std::uint32_t index) { return clubs_base() + index * club_size; };
    if (compare(c, command, answer_command) == 0) {
        // An answer button: the answer is the control's tone, and the three labels are re-marked.
        Kept<18> tone{c, none ? 0 : c.u32(record - 0x5754)};
        Kept<19> label{c, sign16(world_value(c, world, 8, 0x1067cba4))};
        const auto set_label = [&](std::uint32_t half_word, std::uint32_t back) {
            c.set({{3, world}, {4, label}, {5, half_word}});
            c.call(routines::set_label, back);
        };
        const auto marked = c.u16(c.r(17)), plain = c.u16(c.r(21));
        // The marked label (the answer's own) sits at the position the tone names; the others are plain.
        std::uint32_t order[3], backs[3];
        if (compare(c, tone, 2) == 0) { order[0] = plain; order[1] = marked; order[2] = plain; backs[0] = 0x1067cbd0; backs[1] = 0x1067cbe4; backs[2] = 0x1067cbf8; }
        else if (compare(c, tone, 2) > 0 || compare(c, tone, 1) < 0) { order[0] = plain; order[1] = plain; order[2] = marked; backs[0] = 0x1067cc48; backs[1] = 0x1067cc5c; backs[2] = 0x1067cc70; }
        else { order[0] = marked; order[1] = plain; order[2] = plain; backs[0] = 0x1067cc0c; backs[1] = 0x1067cc20; backs[2] = 0x1067cc34; }
        for (unsigned n = 0; n < 3; ++n) {
            set_label(order[n], backs[n]);
            if (n < 2) label = label + 1;
        }
        c.set({{3, world}, {4, 3}, {5, tone}, {6, 0}});
        c.call(library::set_field, 0x1067cc84);
        c.set({{3, world}, {4, 0}, {5, 0}});
        c.call(routines::refresh, 0x1067cc94);
        c.set(3, 0);
        return;
    }
    if (compare(c, command, ok_command) == 0) {
        // OK: the story is filed unless nothing was chosen.
        if (compare(c, answer) != 0) {
            const auto item = frame.at(0x38);
            c.set({{3, item}, {4, 0}, {5, 0}});
            c.call(library::begin_item, 0x1067cd0c);
            c.set({{3, c.u32(c.r(18))}, {4, window_owner}, {5, v0}, {6, item}});
            c.call(library::fetch_item, 0x1067cd20);
            if (compare(c, staff, -1) > 0) {
                // (The original tests the person against -1 a second time, which can only pass.)
                compare(c, staff, -1);
                const auto press = c.u32(c.r(20));
                enter(c, 0x1067cd64, {press, persons(staff), clubs(club_index), c.u32(item), sign8(kind), sign8(answer)});
                mark_history_row(c, press, persons(staff), clubs(club_index), c.u32(item), sign8(kind), sign8(answer));
            }
            if (compare(c, kind, 0x1f) == 0 && compare(c, other_club, -1) != 0 && compare(c, staff, -1) != 0) {
                c.set({{3, c.u32(c.r(20))}, {4, persons(staff)}, {5, clubs(other_club)}, {6, clubs(club_index)}, {7, sign8(answer)}, {8, clubs_base()}});
                media::call(c, routines::club_reaction, 0x1067cdac);  // (pick_b's routine: its typed function may still change)
            }
            const auto item_kind = c.i32(item);
            if (item_kind == 0xbdf) {
                if (compare(c, staff, -1) > 0) {
                    c.set({{3, c.u32(c.r(20))}, {4, clubs(club_index)}, {5, sign8(kind)}, {6, sign8(answer)}, {7, persons(staff)}, {8, clubs_base()}});
                    media::call(c, routines::signing, 0x1067cde8);  // (eval_c's routine: not finished)
                } else {
                    c.set({{3, c.u32(c.r(20))}, {4, clubs(club_index)}, {5, sign8(kind)}, {6, sign8(answer)}, {7, 0}});
                    media::call(c, routines::signing, 0x1067ce0c);
                }
            } else if (item_kind == 0xbe6) {
                const auto press = c.u32(c.r(20));
                enter(c, 0x1067ce4c, {press, persons(staff), clubs(club_index), clubs(other_club), sign8(answer), clubs_base()});
                settle_event(c, press, persons(staff), clubs(club_index), clubs(other_club), sign8(answer));
            } else if (item_kind == 0xbe8) {
                const auto press = c.u32(c.r(20));
                enter(c, 0x1067ce80, {press, persons(staff), clubs(club_index), sign8(answer), clubs_base()});
                react_to_event(c, press, persons(staff), clubs(club_index), sign8(answer));
            } else {
                const auto press = c.u32(c.r(20));
                enter(c, 0x1067ceac, {press, persons(staff), clubs(club_index), sign8(kind), sign8(answer), c.u32(c.r(19))});
                evaluate_story(c, press, persons(staff), clubs(club_index), sign8(kind), sign8(answer));
            }
            // The compiler's decision tree over the item kind, compare for compare.
            std::uint32_t reaction = 0xa, back = 0x1067cf84;
            if (compare(c, item_kind, 0xbe7) != 0) {
                if (compare(c, item_kind, 0xbe7) > 0) {
                    if (compare(c, item_kind, 0xfbc) == 0) { reaction = 7; back = 0x1067cf44; }
                    else if (compare(c, item_kind, 0xfbc) < 0 && compare(c, item_kind, 0xbe9) < 0) { reaction = 0x12; back = 0x1067cf04; }
                } else if (compare(c, item_kind, 0xbdd) == 0) { reaction = 0xd; back = 0x1067cf64; }
                else if (compare(c, item_kind, 0xbdd) > 0 && compare(c, item_kind, 0xbe6) >= 0) { reaction = 0x12; back = 0x1067cf24; }
            }
            c.set({{3, c.u32(c.r(18))}, {4, window_owner}, {5, v0}, {6, reaction}, {7, answer}, {8, 0}});
            c.call(routines::confirm, back);
        }
        c.set(3, 0xfffffff7u);  // -9
        return;
    }
    if (compare(c, command, cancel_command) == 0) {
        c.set(3, 0xfffffff7u);  // -9
        return;
    }
    c.set(3, control);
    c.call(routines::key_press, 0x1067cff4);
    if (compare(c, std::int32_t(c.r(3))) != 0) return;
    c.set(3, control);
    c.call(routines::default_key, 0x1067d008);
}
}
