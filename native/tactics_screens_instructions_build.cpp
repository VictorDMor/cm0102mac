// The team-instructions screen's construction (0x103becb0): the dialog box,
// its title and buttons, the pitch, the grid of team options and the lists
// of set-piece takers and playmakers.
#include "tactics_screens_instructions.hpp"
#include "tactics_leaves.hpp"
#include <array>
#include <initializer_list>

namespace cm::aot::tactics_screens::instructions {
using guest::Cpu;
using guest::Frame;
namespace {
// The UI framework (the game's own drawing and control code).
constexpr std::uint32_t make_control = 0x10323c00, add_item = 0x10323a40, add_box = 0x10744390,
                        add_buttons = 0x10744430, add_button = 0x107444e0, add_label = 0x107445a0,
                        format_text = 0x10727890, print = 0x1077b720, staff_name = 0x100caeb0,
                        make_colour = 0x1076bb20;
constexpr std::int32_t pool_slot = -0x4c90;  // TACTICS_SCREENS.CPP's string pool
std::uint32_t index(std::uint32_t value, std::uint32_t size) { return std::uint32_t(std::int32_t(std::int8_t(value))) * size; }

// One cell of the option grid: its text, the team option it sets (0 for a
// row's title), its column, and the return addresses of its calls: the
// text, then the selected label (the title's label), the read-only test,
// the button, the read-only label.
struct Cell {
    std::uint16_t label;
    bool comment;  // the text takes the empty comment string as its argument
    std::uint32_t text_back, option, column;
    std::uint32_t selected_back, leaf_back, button_back, read_only_back;
};
// Rows: mentality, passing, tackling, pressing, offside trap, counter
// attack, men behind the ball.
constexpr std::array<std::array<Cell, 5>, 7> grid{{
    {{{0xcd6, false, 0x103bf180, 0, 0, 0x103bf1b4, 0, 0, 0},
      {0xce1, true, 0x103bf1c4, 0x20000, 1, 0x103bf200, 0x103bf20c, 0x103bf260, 0x103bf298},
      {0xd06, true, 0x103bf2a8, 0x8000, 2, 0x103bf2e4, 0x103bf2f0, 0x103bf344, 0x103bf37c},
      {0xd2e, true, 0x103bf38c, 0x10000, 3, 0x103bf3c8, 0x103bf3d4, 0x103bf428, 0x103bf460}}},
    {{{0x15e, false, 0x103bf470, 0, 0, 0x103bf4a4, 0, 0, 0},
      {0x2f0, true, 0x103bf4b4, 0x1, 1, 0x103bf4f0, 0x103bf4fc, 0x103bf54c, 0x103bf584},
      {0x31a, true, 0x103bf594, 0x2, 2, 0x103bf5d0, 0x103bf5dc, 0x103bf62c, 0x103bf664},
      {0x344, true, 0x103bf674, 0x4, 3, 0x103bf6b0, 0x103bf6bc, 0x103bf70c, 0x103bf744},
      {0x36f, true, 0x103bf758, 0x8, 4, 0x103bf798, 0x103bf7a4, 0x103bf7f4, 0x103bf830}}},
    {{{0x167, false, 0x103bf844, 0, 0, 0x103bf878, 0, 0, 0},
      {0x3b6, true, 0x103bf88c, 0x10, 1, 0x103bf8cc, 0x103bf8dc, 0x103bf930, 0x103bf968},
      {0x3d6, true, 0x103bf978, 0x20, 2, 0x103bf9b8, 0x103bf9c4, 0x103bfa18, 0x103bfa50},
      {0x3f4, true, 0x103bfa64, 0x40, 3, 0x103bfaa4, 0x103bfab4, 0x103bfb08, 0x103bfb40}}},
    {{{0x171, false, 0x103bfb50, 0, 0, 0x103bfb84, 0, 0, 0},
      {0x64d, false, 0x103bfb90, 0x80, 1, 0x103bfbcc, 0x103bfbd8, 0x103bfc28, 0x103bfc60},
      {0x650, false, 0x103bfc6c, 0x100, 2, 0x103bfca8, 0x103bfcb4, 0x103bfd04, 0x103bfd3c}}},
    {{{0xd56, false, 0x103bfd4c, 0, 0, 0x103bfd80, 0, 0, 0},
      {0x64d, false, 0x103bfd8c, 0x200, 1, 0x103bfdc8, 0x103bfdd4, 0x103bfe28, 0x103bfe60},
      {0x650, false, 0x103bfe6c, 0x400, 2, 0x103bfeac, 0x103bfeb8, 0x103bff0c, 0x103bff44}}},
    {{{0xd64, false, 0x103bff54, 0, 0, 0x103bff88, 0, 0, 0},
      {0x64d, false, 0x103bff94, 0x800, 1, 0x103bffd0, 0x103bffdc, 0x103c002c, 0x103c0064},
      {0x650, false, 0x103c0070, 0x1000, 2, 0x103c00ac, 0x103c00b8, 0x103c0108, 0x103c0140}}},
    {{{0xd74, false, 0x103c0150, 0, 0, 0x103c0184, 0, 0, 0},
      {0x64d, false, 0x103c0190, 0x2000, 1, 0x103c01cc, 0x103c01d8, 0x103c0228, 0x103c0260},
      {0x650, false, 0x103c026c, 0x4000, 2, 0x103c02a8, 0x103c02b4, 0x103c0304, 0x103c033c}}},
}};

// The routine's state: its non-volatile registers as the original names them.
class Builder {
public:
    Builder(Cpu& cpu, Frame& f) : c(cpu), frame(f) {}
    Cpu& c;
    Frame& frame;
    std::uint32_t local(std::int32_t offset) const { return frame.at(offset); }
    std::uint32_t word(std::int32_t offset) const { return c.u32(local(offset)); }
    // Stack arguments 9 and up (sp+0x38, 0x3c, ...).
    void stack(std::initializer_list<std::uint32_t> words) {
        auto at = local(0x38);
        for (const auto value : words) { c.put32(at, value); at += 4; }
    }
    void registers(std::initializer_list<std::uint32_t> values) {
        unsigned n = 3;
        for (const auto value : values) c.set(n++, value);
    }
    std::uint32_t game() const { return c.r(31); }
    std::uint32_t text() const { return c.r(30); }  // the shared text buffer
    std::uint32_t pool() const { return c.r(28); }
    std::uint32_t screen() const { return c.r(21); }
    std::uint32_t row() const { return c.r(24); }
    std::uint32_t control() const { return c.r(23); }
    std::uint32_t colour(unsigned reg) const { return c.u16(c.r(reg)); }
    std::uint32_t colour_at(std::int32_t slot) const { return c.u16(c.toc(slot)); }
    std::uint32_t grid_colour() const { return c.u16(c.r(2) - 0x3828); }
    // The text buffer, formatted from a pool string (0x10727890; the
    // arguments the original passes, the other registers left as they are).
    void say(std::uint32_t label, std::uint32_t back) {
        c.set({{3, text()}, {4, pool() + label}});
        c.call(format_text, back);
    }
    void say(std::uint32_t label, std::uint32_t argument, std::uint32_t back) {
        c.set({{3, text()}, {4, pool() + label}, {5, argument}});
        c.call(format_text, back);
    }
    bool read_only(std::uint32_t back) {
        c.set(3, screen());
        c.link(back);
        tactics_leaf_103b3c40(c.s);
        return std::int8_t(c.r(3)) != 0;
    }
    // A row's title in column 0.
    void title(std::uint32_t back) {
        stack({colour_at(-0x7b10), text()});
        registers({game(), control(), 0, row(), 1, 0, 1, 3});
        c.call(add_label, back);
    }
    void cell(const Cell& cell, std::uint32_t options);
    void set_piece_lists();
    void playmaker_lists();
    // The list of other players, made on its first entry: a button opening it
    // in the column after the current choice's, and the list control.
    void make_list(std::uint32_t button_back, std::uint32_t control_back) {
        stack({colour(27), 0, 0, 0});
        registers({game(), control(), word(0x80) + 1, row(), 0x30, colour(26), 0xc, 1});
        c.call(add_button, button_back);
        stack({0, 0x500030, colour(25), c.r(3)});
        registers({game(), 0, 0, 0, 0, 0, 0, 0});
        c.call(make_control, control_back);
        c.put16(local(0x56), c.r(3));
    }
    // A list entry: "<slot label> <name>" sending `event` with the player.
    void add_entry(std::uint32_t staff, std::uint32_t event, std::uint32_t name_back, std::uint32_t print_back,
                   std::uint32_t add_back) {
        name(staff, name_back);
        c.set({{3, text()}, {4, pool() + 0xdca}, {5, local(0x172)}, {6, local(0xaa)}});  // "%s %s"
        c.call(print, print_back);
        c.set({{3, game()}, {4, list()}, {5, text()}, {6, event}, {7, staff}});
        c.call(add_item, add_back);
    }
    // The list's end marker, when it was made.
    void close_list(std::uint32_t event, std::uint32_t back) {
        if (c.i16(local(0x56)) == -1) return;
        c.set({{3, game()}, {4, list()}, {5, 0}, {6, event}, {7, 0}});
        c.call(add_item, back);
    }
    std::uint32_t list() const { return std::uint32_t(std::int32_t(c.i16(local(0x56)))); }
    void name(std::uint32_t staff, std::uint32_t back) {
        c.set({{3, staff}, {4, local(0xaa)}, {5, 0xc8}, {6, 2}});
        c.call(staff_name, back);
    }
    // The current choice: "  %s" (first) or "  or %s" (second) as a button
    // in the current column, highlighted (1, else 0x21) when one of the
    // listed players holds it; its event depends on 0x103baca0. `found` and
    // `style` are the section's locals.
    struct CurrentCalls { std::uint32_t name_first, name_second, say_second, print_first, query, button; };
    void current(std::uint32_t staff, bool second, std::int32_t found, std::int32_t style, const CurrentCalls& at) {
        name(staff, second ? at.name_second : at.name_first);
        if (second) {
            say(0xdd0, local(0xaa), at.say_second);
        } else {
            c.set({{3, text()}, {4, pool() + 0xde8}, {5, local(0xaa)}});
            c.call(print, at.print_first);
        }
        c.set(3, screen());
        c.link(at.query);
        const bool query = std::int8_t(club_query(c, screen())) != 0;
        const auto highlight = local(query ? style : style - 4);
        c.put32(highlight, c.i8(local(found)) ? 1 : 0x21);
        stack({colour(22), text(), query ? 0x7dcu : 0x7d7u, staff});
        registers({game(), control(), word(0x80), row(), 1, 0, c.u32(highlight), 2});
        c.call(add_button, query ? at.button : at.button + 0x64);
    }
};

// An option cell: its text, then a label when the option is chosen (or the
// screen is read-only), else a button sending the option.
void Builder::cell(const Cell& cell, std::uint32_t options) {
    if (cell.comment) say(cell.label, pool() + 0x22b, cell.text_back);
    else say(cell.label, cell.text_back);
    if (!cell.option) {
        title(cell.selected_back);
        return;
    }
    if (options & cell.option) {
        stack({colour(22), text()});
        registers({game(), control(), cell.column, row(), 0x221, colour(22), 0xc, 2});
        c.call(add_label, cell.selected_back);
    } else if (!read_only(cell.leaf_back)) {
        stack({grid_colour(), text(), 3, cell.option});
        registers({game(), control(), cell.column, row(), 1, 0, 0xc, 2});
        c.call(add_button, cell.button_back);
    } else {
        stack({grid_colour(), text()});
        registers({game(), control(), cell.column, row(), 1, 0, 0xc, 2});
        c.call(add_label, cell.read_only_back);
    }
}

// Each set piece's first and second taker (the table at r29: kind, order,
// event), two per row: a list of the other players who may take it, then
// the current taker.
void Builder::set_piece_lists() {
    c.set(20, 0);
    for (;; c.set(20, c.r(20) + 1)) {
        const auto entry = c.r(29) + index(c.r(20), 8);
        if (c.i8(entry) == -1) break;
        const auto kind = std::uint32_t(std::int32_t(c.i8(entry)));
        const auto order = std::uint32_t(std::int32_t(c.i8(entry + 1)));
        const auto event = c.u32(entry + 4);
        c.put8(local(0x51), 0);       // a listed player is the current taker
        c.put16(local(0x56), 0xffff);  // the list control, none yet
        bool listed = true;
        if (std::int8_t(order)) {
            // No list for the second taker while there is no first.
            c.set({{3, screen()}, {4, kind}, {5, 0}});
            c.link(0x103c0550);
            c.set(3, taker(c, screen(), kind, 0));
            listed = c.r(3) != 0;
        }
        if (listed) {
            for (c.put8(local(0x52), 0); c.i8(local(0x52)) < 20; c.put8(local(0x52), c.u8(local(0x52)) + 1)) {
                const auto slot = c.u8(local(0x52));
                c.set({{3, screen()}, {4, slot}, {5, 0}, {6, 0}});
                c.link(0x103c0578);
                c.set(3, slot_available(c, screen(), slot, 0, 0));
                if (!std::int8_t(c.r(3))) continue;
                c.set({{3, screen()}, {4, slot}, {5, 0}});
                c.link(0x103c0594);
                const auto staff = slot_player(c, screen(), slot, 0);
                c.set({{3, staff}, {19, staff}});
                if (!staff) continue;
                c.set({{3, screen()}, {4, slot}, {5, local(0x172)}, {6, 0xc8}, {7, 0}, {8, 0}});
                c.link(0x103c05bc);
                c.set(3, slot_number(c, screen(), slot, local(0x172), 0xc8, 0, 0));
                c.set({{3, screen()}, {4, kind}, {5, order}});
                c.link(0x103c05e8);
                c.set(3, taker(c, screen(), kind, order));
                if (c.r(3) == staff) {
                    c.set(3, 1);
                    c.put8(local(0x51), 1);
                }
                const auto other = std::uint32_t(std::int32_t(std::int8_t(1 - c.u8(entry + 1))));
                c.set({{3, screen()}, {4, kind}, {5, other}});
                c.link(0x103c0628);
                c.set(3, taker(c, screen(), kind, other));
                if (c.r(3) == staff) continue;
                if (c.i16(local(0x56)) == -1) make_list(0x103c0684, 0x103c06c8);
                add_entry(staff, event, 0x103c06e0, 0x103c06f4, 0x103c0718);
            }
        }
        close_list(event, 0x103c0764);
        c.set({{3, screen()}, {4, kind}, {5, order}});
        c.link(0x103c0790);
        const auto staff = taker(c, screen(), kind, order);
        c.set({{3, staff}, {19, staff}});
        if (staff)
            current(staff, std::int8_t(order) > 0, 0x51, 0x6c,
                    {0x103c07f4, 0x103c07cc, 0x103c07dc, 0x103c0804, 0x103c0810, 0x103c087c});
        c.put32(local(0x80), word(0x80) + 2);
        if (std::int32_t(word(0x80)) > 3) {
            c.put32(local(0x80), 1);
            c.set(24, row() + 1);
        }
    }
}

// The first and alternative playmaker (events 14 and 15), after the takers
// in the same row.
void Builder::playmaker_lists() {
    for (c.set(20, 0); std::int8_t(c.r(20)) < 2; c.set(20, c.r(20) + 1)) {
        const auto which = c.r(20);
        const auto event = which ? 0xfu : 0xeu;
        c.put8(local(0x50), 0);
        c.put16(local(0x56), 0xffff);
        bool listed = true;
        if (std::int8_t(which)) {
            c.set({{3, screen()}, {4, 0}});
            c.link(0x103c0950);
            c.set(3, playmaker(c, screen(), 0));
            listed = c.r(3) != 0;
        }
        if (listed) {
            c.set(8, 0);
            for (c.put8(local(0x52), 0); c.i8(local(0x52)) < 20; c.put8(local(0x52), c.u8(local(0x52)) + 1)) {
                const auto slot = c.u8(local(0x52));
                c.set({{3, screen()}, {4, slot}, {5, 0}, {6, 0}});
                c.link(0x103c0978);
                c.set(3, slot_available(c, screen(), slot, 0, 0));
                if (!std::int8_t(c.r(3))) continue;
                c.set({{3, screen()}, {4, slot}, {5, 0}});
                c.link(0x103c0994);
                const auto staff = slot_player(c, screen(), slot, 0);
                c.set({{3, staff}, {19, staff}});
                if (!staff) continue;
                c.set({{3, screen()}, {4, slot}, {5, local(0x172)}, {6, 0xc8}, {7, 0}, {8, 0}});
                c.link(0x103c09bc);
                c.set(3, slot_number(c, screen(), slot, local(0x172), 0xc8, 0, 0));
                c.set({{3, screen()}, {4, which}});
                c.link(0x103c09c8);
                c.set(3, playmaker(c, screen(), which));
                if (c.r(3) == staff) {
                    c.set(8, 1);
                    c.put8(local(0x50), 1);
                }
                const auto other = std::uint32_t(std::int8_t(which) == 0);
                c.set({{3, screen()}, {4, other}});
                c.link(0x103c09f0);
                c.set(3, playmaker(c, screen(), other));
                if (c.r(3) == staff) continue;
                if (c.i16(local(0x56)) == -1) {
                    stack({colour(27), 0, 0, 0});
                    registers({game(), control(), word(0x80) + 1, row(), 0x30, colour(26), 0xc, 1});
                    c.call(add_button, 0x103c0a4c);
                    c.set(11, colour(25));
                    stack({0, 0x500030, colour(25), c.r(3)});
                    registers({game(), 0, 0, 0, 0, 0, 0, 0});
                    c.call(make_control, 0x103c0a94);
                    c.put16(local(0x56), c.r(3));
                }
                name(staff, 0x103c0aac);
                c.set({{3, text()}, {4, pool() + 0xdca}, {5, local(0x172)}, {6, local(0xaa)}});
                c.call(print, 0x103c0ac0);
                c.put32(local(0x64), event);
                c.set({{3, game()}, {4, list()}, {5, text()}, {6, c.u32(local(0x64))}, {7, staff}});
                c.call(add_item, 0x103c0af8);
            }
        }
        if (c.i16(local(0x56)) != -1) {
            c.put32(local(0x60), event);
            c.set({{3, game()}, {4, list()}, {5, 0}, {6, c.u32(local(0x60))}, {7, 0}});
            c.call(add_item, 0x103c0b58);
        }
        c.set({{3, screen()}, {4, which}});
        c.link(0x103c0b64);
        const auto staff = playmaker(c, screen(), which);
        c.set({{3, staff}, {19, staff}});
        if (staff)
            current(staff, std::int8_t(which) != 0, 0x50, 0x5c,
                    {0x103c0b84, 0x103c0b84, 0x103c0ba0, 0x103c0bb4, 0x103c0bbc, 0x103c0c28});
        c.put32(local(0x80), word(0x80) + 2);
    }
}
}

void build(Cpu& c) {
    Frame frame(c, 0x103becb0, 0x280, {.first_gpr = 16});
    Builder b(c, frame);
    c.set({{29, c.toc(-0x4cd0)}, {25, c.toc(-0x7bcc)}, {26, c.toc(-0x7bd4)}, {27, c.toc(-0x7bc8)},
           {30, c.toc(-0x7bb8)}, {22, c.toc(-0x7bd0)}, {31, c.toc(-0x7bb0)}, {28, c.toc(pool_slot)}});
    // The grid's colour, made once.
    const auto made = c.r(2) - 0x382a;
    if (!c.i8(made)) {
        b.registers({0, 0xc0, 0, 0});
        c.call(make_colour, 0x103bed04);
        c.put16(c.r(2) - 0x3828, c.r(3));
        c.set(6, 1);
        c.put8(made, 1);
    }
    // Locals: the box's height, a 2-byte and two 5-byte style records with
    // pointers to them.
    c.put32(b.local(0x9c), 0x1ea);
    c.put16(b.local(0x53), c.u16(c.r(2) - 0x383a));
    c.put32(b.local(0x78), b.local(0x53));
    const auto copy5 = [&](std::int32_t to, std::int32_t slot) {
        const auto from = c.toc(slot);
        c.put32(b.local(to), c.u32(from));
        c.put8(b.local(to + 4), c.u8(from + 4));
    };
    copy5(0xa5, -0x4cc8);
    c.put32(b.local(0x74), b.local(0xa5));
    copy5(0xa0, -0x4ccc);
    c.put32(b.local(0x70), b.local(0xa0));
    c.set(6, c.toc(-0x4ccc));
    b.registers({b.game(), 0});
    c.call(0x1073ef70, 0x103bed7c);  // this screen's state object
    c.set(21, c.r(3));
    c.set(3, 0);
    c.call(0x10712e60, 0x103bed8c);
    b.stack({0, 2, 0, 0xffffffffu});
    b.registers({b.game(), 0, 0, 0x31f, 0x257, 1, 0, 1});
    c.call(make_control, 0x103bedd0);
    // The box: 0x1ea high, 0xa5 less for a read-only screen, centred.
    if (b.read_only(0x103bedd8)) c.put32(b.local(0x9c), b.word(0x9c) - 0xa5);
    c.put32(b.local(0x98), 0xaa);
    const auto spare = std::int32_t(0x258 - b.word(0x9c));
    c.set(16, std::uint32_t(spare / 2));
    c.put32(b.local(0x94), b.word(0x98) + 0x225);
    c.set(17, b.word(0x9c) - 1 + c.r(16));
    b.registers({0, 0x38, 0, 0});
    c.call(make_colour, 0x103bee38);
    b.stack({0, 0x30, c.r(3), 0xffffffffu});
    b.registers({b.game(), b.word(0x98), c.r(16), b.word(0x94), c.r(17), 1, 0, 1});
    c.call(make_control, 0x103bee78);
    const auto inset = [&](std::uint32_t by) {
        c.put32(b.local(0x98), b.word(0x98) + by);
        c.set(16, c.r(16) + by);
        c.put32(b.local(0x94), b.word(0x94) - by);
        c.set(17, c.r(17) - by);
    };
    inset(0xa);
    // The title.
    b.say(0x2f, 0x103beea4);
    b.stack({4, b.colour(27), b.text()});
    b.registers({b.game(), b.word(0x98), c.r(16), b.word(0x94), c.r(16) + 0x2d, 0x30, b.colour(26), 0xc});
    c.call(add_box, 0x103beedc);
    c.set(16, c.r(16) + 0x37);
    // The buttons: Cancel and OK, or only OK when read-only.
    b.stack({0, 1, 0, 0xffffffffu});
    b.registers({b.game(), b.word(0x98), c.r(17) - 0x1e, b.word(0x94), c.r(17), 2, b.local(0x53), 1});
    c.call(make_control, 0x103bef24);
    c.set(23, c.r(3));
    if (!b.read_only(0x103bef30)) {
        for (const auto [label, event, text_back, back] : {std::array<std::uint32_t, 4>{0x6fb, 1, 0x103bef48, 0x103bef8c},
                                                           std::array<std::uint32_t, 4>{0x702, 2, 0x103bef98, 0x103befdc}}) {
            b.say(label, text_back);
            b.stack({b.colour(27), b.text(), event, 0});
            b.registers({b.game(), b.control(), event - 1, 0, 0x30, b.colour_at(-0x7bc4), 0xc, 3});
            c.call(add_button, back);
        }
    } else {
        b.say(0x702, 0x103beff0);
        b.stack({3, b.colour(27), b.text(), 2, 0});
        b.registers({b.game(), b.word(0x98), c.r(17) - 0x1e, b.word(0x94), c.r(17), 0x30, b.colour_at(-0x7bc4), 0xc});
        c.call(add_buttons, 0x103bf040);
    }
    c.set(17, c.r(17) - 0x28);
    // The pitch, drawn across (stack argument 1), inside a margin; its inner
    // corners go to sp+0x90..0x84.
    inset(0xa);
    b.stack({1, b.control()});
    b.registers({b.word(0x98), c.r(16), b.word(0x94), c.r(17), b.local(0x90), b.local(0x8c), b.local(0x88), b.local(0x84)});
    c.set(11, c.toc(-0x4cac));
    c.set_f(1, double(c.f32(c.r(11) + 4)));  // the scale (lfs)
    c.link(0x103bf09c);
    draw_pitch(c, c.f(1), b.word(0x98), c.r(16), b.word(0x94), c.r(17), b.local(0x90), b.local(0x8c), b.local(0x88),
               b.local(0x84));
    inset(5);
    // The option grid: one row per option group.
    const bool locked = b.read_only(0x103bf0c4);
    b.stack({0, 1, 0, 0xffffffffu});
    if (!locked) {
        b.registers({b.game(), b.word(0x98), c.r(16), b.word(0x94), c.r(17), 5, b.local(0xa5), 0xc});
        c.call(make_control, 0x103bf114);
    } else {
        b.registers({b.game(), b.word(0x98), c.r(16), b.word(0x94), c.r(17), 5, b.local(0xa5), 7});
        c.call(make_control, 0x103bf160);
    }
    c.set(23, c.r(3));
    c.set(24, 0);
    c.set(3, b.screen());
    c.link(0x103bf170);
    const auto options = team_options(c, b.screen());
    c.set(18, options);
    for (const auto& row : grid) {
        for (const auto& cell : row) {
            if (!cell.text_back) break;
            b.cell(cell, options);
        }
        c.set(24, b.row() + 1);
    }
    if (b.read_only(0x103c0348)) {
        compare(c, std::int8_t(c.r(3)), 0);
        return;
    }
    // The lists: set-piece takers (each kind's first and second taker side
    // by side) and the playmakers, under five row titles.
    c.put32(b.local(0x7c), b.row());
    b.stack({0, 1, 0, 0xffffffffu});
    b.registers({b.game(), b.word(0x98), c.r(16), b.word(0x94), c.r(17), 5, b.local(0xa0), 0xc});
    c.call(make_control, 0x103c039c);
    c.set(23, c.r(3));
    for (const auto [label, text_back, back] : {std::array<std::uint32_t, 3>{0xd85, 0x103c03ac, 0x103c03e4},
                                                std::array<std::uint32_t, 3>{0xd95, 0x103c03f0, 0x103c0428},
                                                std::array<std::uint32_t, 3>{0xda5, 0x103c0434, 0x103c046c},
                                                std::array<std::uint32_t, 3>{0xdb2, 0x103c0478, 0x103c04b0},
                                                std::array<std::uint32_t, 3>{0xdbf, 0x103c04bc, 0x103c04f4}}) {
        b.say(label, text_back);
        b.stack({b.colour_at(-0x7b10), b.text()});
        b.registers({b.game(), b.control(), 0, b.row(), 1, 0, 1, 3});
        c.set(24, b.row() + 1);
        c.call(add_label, back);
    }
    c.put32(b.local(0x80), 1);  // the column: 1 first, 3 second
    c.set(24, b.word(0x7c));
    b.set_piece_lists();
    b.playmaker_lists();
    c.set(3, 2);  // the playmaker loop's last test leaves its counter
    compare(c, 2, 2);
}
}
