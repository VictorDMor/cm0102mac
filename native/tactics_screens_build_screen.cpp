// 0x103b3c50: builds the tactics screen: the Tactics, View and Edit menus,
// the overview / with-ball / without-ball tabs, the squad list with the
// column chosen in the View menu, the pitch and the buttons.
//
// Registers: the original keeps the game state, the text buffer, the string
// pool and the colours in r16-r31 for the whole routine and uses r13-r15 as
// scratch; callees save those into the stack image, so each non-volatile
// register holds the original's value at every call. The localisation
// routine (0x10727890) and sprintf (0x1077b720) spill r4-r10 into this
// frame, and the menu routines r4-r7, so the volatile registers the
// original sets between calls (copies of results, compared values) are set
// here too, with `keep` for a copy of the last result.
#include "tactics_screens_build.hpp"
#include "tactics_leaves.hpp"
#include <bit>

namespace cm::aot::tactics_screens::build {
using guest::Cpu;
using guest::Frame;
namespace {
// TOC slots of the globals the builder keeps in registers.
constexpr std::int32_t panel_colour_slot = -0x7bd4;    // r16 -> uint16 colour
constexpr std::int32_t ratings_slot = -0x7b3c;         // r18 -> the ratings object
constexpr std::int32_t paper_colour_slot = -0x7bc8;    // r21 -> uint16 colour
constexpr std::int32_t tactics_slot = -0x7a6c;         // r22 -> CTACTICS pointer
constexpr std::int32_t ink_colour_slot = -0x7bc4;      // r27 -> uint16 colour
constexpr std::int32_t mark_colour_slot = -0x7bd0;     // r28 -> uint16 colour
constexpr std::int32_t text_slot = -0x7bb8;            // r31 -> the shared text buffer
constexpr std::int32_t game_slot = -0x7bb0;            // r30 -> the game state
constexpr std::int32_t humans_slot = -0x7a54;          // -> human manager table
constexpr std::int32_t presets_flag_slot = -0x6330;    // -> int8: presets can be loaded
constexpr std::int32_t heading_colour_slot = -0x6224;  // -> uint16 colour of the tab headings
constexpr std::uint32_t screen_size = 0xc6b4;

std::uint32_t sext8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }

// The menus' command numbers (the screen handler's switch).
namespace command {
constexpr std::uint32_t set_to_default = 0, team_instructions = 1, formation = 2, load = 3, load_preset = 4, save = 5,
                        save_as = 6, save_for = 8, erase = 9, unlock = 10, lock = 11, preset = 12, view = 13,
                        undo = 15, copy = 16, copy_all = 17, paste = 18, reflect = 19, runs = 20,
                        penalty_takers = 0x22;
}

class Builder {
public:
    Builder(Cpu& cpu, Frame& f)
        : c(cpu), frame(f), game(c.toc(game_slot)), text(c.toc(text_slot)), pool(c.toc(detail::pool_slot)),
          tactics_global(c.toc(tactics_slot)) {
        c.set({{18, c.toc(ratings_slot)}, {16, c.toc(panel_colour_slot)}, {22, tactics_global},
               {21, c.toc(paper_colour_slot)}, {31, text}, {27, c.toc(ink_colour_slot)},
               {28, c.toc(mark_colour_slot)}, {30, game}, {29, pool}});
    }
    void build();

private:
    Cpu& c;
    Frame& frame;
    const std::uint32_t game, text, pool, tactics_global;
    std::uint32_t screen = 0;   // r26: the screen's state object
    std::uint32_t view = 0;     // r20: the View menu's column, sign-extended

    std::uint32_t at(std::int32_t offset) const { return frame.at(offset); }
    std::uint32_t tactics() const { return c.u32(tactics_global); }
    std::uint16_t colour(std::int32_t slot) const { return c.u16(c.toc(slot)); }
    std::uint16_t paper() const { return colour(paper_colour_slot); }
    std::uint16_t ink() const { return colour(ink_colour_slot); }
    std::uint16_t mark() const { return colour(mark_colour_slot); }
    // A halfword the original keeps in a stack slot and reloads with lha.
    std::uint32_t slot16(std::int32_t offset) const { return std::uint32_t(std::int32_t(c.i16(at(offset)))); }

    // `bl target`: this group's routines run directly as C++ (with the
    // arguments in the registers, as their hooks take them), the others
    // through the dispatcher.
    std::uint32_t call(std::uint32_t target, std::uint32_t back) {
        const auto r = [&](unsigned n) { return c.r(n); };
        const auto direct = [&](std::uint32_t result) {
            c.set(3, result);
            return result;
        };
        switch (target) {
        case 0x103c4b10:
            c.link(back);
            return direct(construct(c, r(3), r(4), r(5), r(6), r(7), r(8), r(9)));
        case 0x103c78a0: c.link(back); return direct(list_spot(c, r(3), r(4), r(5)));
        case 0x103c79d0: c.link(back); return direct(shown_spot(c, r(3), r(4), r(5)));
        case 0x103c8b60: c.link(back); return direct(custom_spot(c, r(3), r(4)));
        case 0x103c9970: c.link(back); return direct(entry_unavailable(c, r(3), r(4), r(5)));
        case 0x103c9aa0: c.link(back); return direct(entry_byte_24(c, r(3), r(4), r(5)));
        case 0x103c9bd0: c.link(back); return direct(entry_byte_23(c, r(3), r(4), r(5)));
        case 0x103c9d00: c.link(back); return direct(entry_label(c, r(3), r(4), r(5), r(6), r(7), r(8)));
        case 0x103c9f40: c.link(back); return direct(entry_rating(c, r(3), r(4), r(5)));
        case 0x103ca120: c.link(back); return direct(entry_byte_1b(c, r(3), r(4), r(5)));
        case 0x103ca260: c.link(back); return direct(entry_byte_0c(c, r(3), r(4), r(5)));
        case 0x103ca390: c.link(back); return direct(entry_plays_for_match_club(c, r(3), r(4), r(5)));
        case 0x103ca730: c.link(back); return direct(entry_with_status(c, r(3), r(4), r(5), r(6)));
        case 0x103cad40: c.link(back); return direct(entry_staff(c, r(3), r(4), r(5)));
        case 0x103caf20: c.link(back); return direct(paired_entry(c, r(3), r(4), r(5)));
        case 0x103ce330:
            c.link(back);
            return direct(position_point(c, r(3), r(4), r(5), r(6), r(7), r(8), r(9), r(10)));
        case 0x103ce790: c.link(back); return direct(arrow_head(c, r(3), r(4), r(5), r(6), c.f(1), r(9), r(10)));
        default: c.call(target, back); return c.r(3);
        }
    }
    // The original copies the last result into a volatile register that a
    // later spilling call may store into the stack image.
    void keep(unsigned n) {
        if (n) c.set(n, c.r(3));
    }
    // The last result as a char: the original copies it to `copy` and
    // sign-extends it into `extended` to test it.
    std::int8_t flag(unsigned copy, unsigned extended) {
        const auto value = c.r(3);
        if (copy) c.set(copy, value);
        if (extended) c.set(extended, sext8(value));
        return std::int8_t(value);
    }
    // The view compared, sign-extended into `n` first as the original does.
    bool view_is(std::int32_t value, unsigned n) {
        if (n) c.set(n, view);
        return std::int8_t(view) == value;
    }
    // Outgoing arguments past the eighth, from sp+0x38.
    void stack_args(std::initializer_list<std::uint32_t> words) {
        std::int32_t offset = 0x38;
        for (const auto word : words) {
            c.put32(at(offset), word);
            offset += 4;
        }
    }
    // 0x10727890: a pool string, localised into the text buffer.
    std::uint32_t localise(std::uint32_t offset, std::uint32_t back) {
        c.set({{3, text}, {4, pool + offset}});
        return call(0x10727890, back);
    }
    // 0x10323a40: a menu item (menu, text, command, value).
    std::uint32_t item(std::uint32_t menu, std::uint32_t command, std::uint32_t value, std::uint32_t back) {
        c.set({{3, game}, {4, menu}, {5, text}, {6, command}, {7, value}});
        return call(0x10323a40, back);
    }
    // 0x10323780: a disabled menu item.
    std::uint32_t disabled_item(std::uint32_t menu, std::uint32_t back) {
        c.set({{3, game}, {4, menu}, {5, text}});
        return call(0x10323780, back);
    }
    // 0x10323620: the ticked item of a menu.
    std::uint32_t ticked_item(std::uint32_t menu, std::uint32_t back) {
        c.set({{3, game}, {4, menu}, {5, text}, {6, 0}, {7, 0}});
        return call(0x10323620, back);
    }
    // 0x103238f0: a separator line.
    std::uint32_t separator(std::uint32_t menu, std::uint32_t back) {
        c.set({{3, game}, {4, menu}});
        return call(0x103238f0, back);
    }
    // 0x103239c0 / 0x10323b80: the submenu of an item, the menu of a heading.
    std::uint32_t submenu(std::uint32_t item, std::uint32_t back) {
        c.set({{3, game}, {4, item}});
        return call(0x103239c0, back);
    }
    std::uint32_t heading_menu(std::uint32_t heading, std::uint32_t back) {
        c.set({{3, game}, {4, heading}});
        return call(0x10323b80, back);
    }
    // 0x10323c00: a box of controls (left, top, right, bottom, kind, data,
    // extra; four arguments on the stack).
    std::uint32_t box(std::uint32_t left, std::uint32_t top, std::uint32_t right, std::uint32_t bottom,
                      std::uint32_t kind, std::uint32_t data, std::uint32_t extra,
                      std::initializer_list<std::uint32_t> more, std::uint32_t back) {
        stack_args(more);
        c.set({{3, game}, {4, left}, {5, top}, {6, right}, {7, bottom}, {8, kind}, {9, data}, {10, extra}});
        return call(0x10323c00, back);
    }
    // 0x107444e0 / 0x107445a0: a cell of a box (box, row, column, flags,
    // colour, font, style; four arguments on the stack).
    std::uint32_t cell(std::uint32_t target, std::uint32_t box, std::uint32_t row, std::uint32_t column,
                       std::uint32_t flags, std::uint32_t colour, std::uint32_t font, std::uint32_t style,
                       std::initializer_list<std::uint32_t> more, std::uint32_t back) {
        stack_args(more);
        c.set({{3, game}, {4, box}, {5, row}, {6, column}, {7, flags}, {8, colour}, {9, font}, {10, style}});
        return call(target, back);
    }
    // 0x1076bb20: a colour from its components.
    std::uint32_t make_colour(std::uint32_t red, std::uint32_t green, std::uint32_t blue, std::uint32_t back) {
        c.set({{3, red}, {4, green}, {5, blue}, {6, 0}});
        return call(0x1076bb20, back);
    }
    // 0x1077dbf0: strcpy.
    std::uint32_t copy_text(std::uint32_t to, std::uint32_t from, std::uint32_t back) {
        c.set({{3, to}, {4, from}});
        return call(0x1077dbf0, back);
    }
    // 0x103b3b20 (screens_misc): whether the screen belongs to a live match.
    std::int8_t in_match(std::uint32_t back) {
        c.set(3, screen);
        return std::int8_t(call(0x103b3b20, back));
    }
    // 0x103b3c40: the screen is read-only (no menus, no editing).
    std::int8_t read_only() {
        c.set(3, screen);
        tactics_leaf_103b3c40(c.s);
        return std::int8_t(c.r(3));
    }

    void statics();
    void open_state();
    void caption();
    void tactics_menu();
    void formations();
    void save_for(std::uint32_t menu);
    void view_menu();
    void edit_menu();
    void headings();
    void squad();
    void squad_entry();
    void entry_details(std::uint32_t player);
    void entry_role(std::uint32_t player);
    void view_column(std::uint32_t player);
    void instructions_column();
    // An instruction setting the column shows (views 7-19); see option().
    struct Option {
        std::uint32_t high, low;            // the instruction bit tested
        std::uint32_t name;                 // its localised name in the pool
        bool comment;                       // the name's format takes an empty string
        std::uint32_t next_high, next_low;  // the setting a click chooses
        bool first;                         // the team setting, in the ink colour
        std::uint32_t text_back, bit_back, cell_back;
    };
    // Where the original sets r13-r15, r19 or a stack temporary inside an option.
    enum class Stage { test, text, bit, cell };
    template <typename Effects> bool option(const Option& option, Effects&& effects);
    void marking_column();
    void marker_column();
    void role_mark(std::uint32_t player);
    void pitch();
    void position_arrows();
    void position_markers();
    void cells();
    void marker_colours(std::int32_t first, std::int32_t second, bool label);
    void runs();
    void cell_frames();
    void spots();
    void buttons();
    // The pitch rectangle 0x103cbb30 fills in: left +0x130, top +0x12c, right +0x128, bottom +0x124.
    std::int32_t local(std::int32_t offset) const { return c.i32(at(offset)); }
    std::int32_t pitch_left() const { return local(0x130); }
    std::int32_t pitch_top() const { return local(0x12c); }
    std::int32_t pitch_right() const { return local(0x128); }
    std::int32_t pitch_bottom() const { return local(0x124); }
    // 0x10323440: abs.
    std::int32_t absolute(std::int32_t value, std::uint32_t back) {
        c.set(3, std::uint32_t(value));
        return std::int32_t(call(0x10323440, back));
    }
    // A shape on the pitch (0x10323470 with the rectangle in r5-r8).
    std::uint32_t shape(std::uint32_t kind, std::int32_t left, std::int32_t top, std::int32_t right,
                        std::int32_t bottom, std::initializer_list<std::uint32_t> more, std::uint32_t back) {
        return control(kind, std::uint32_t(left), std::uint32_t(top), std::uint32_t(right), std::uint32_t(bottom), 0, 0,
                       more, back);
    }
    std::uint32_t slot_ = 0, bit_ = 0;   // the option being shown: sign-extended slot and bit index

    // 0x10323470: a control (kind and six values in registers, eleven on the stack).
    std::uint32_t control(std::uint32_t kind, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e,
                          std::uint32_t f, std::uint32_t g, std::initializer_list<std::uint32_t> more,
                          std::uint32_t back) {
        stack_args(more);
        c.set({{3, game}, {4, kind}, {5, a}, {6, b}, {7, d}, {8, e}, {9, f}, {10, g}});
        return call(0x10323470, back);
    }
    // The screens' routines that take (screen, entry, 0): 0x103ca4e0 and the list callbacks.
    std::uint32_t entry_query(std::uint32_t target, std::uint32_t back) {
        c.set({{3, screen}, {4, c.r(25)}, {5, 0}});
        return call(target, back);
    }
    // 0x103c9810 (screens_instructions): whether the entry's player can play (`out` receives his slot).
    std::uint32_t entry_playing(std::uint32_t out, std::uint32_t back) {
        c.set({{3, screen}, {4, c.r(25)}, {5, out}, {6, 0}});
        return call(0x103c9810, back);
    }
    // 0x103baca0 (screens_instructions): whether the screen shows the opponent.
    std::uint32_t opponent_view(std::uint32_t back) {
        c.set(3, screen);
        return call(0x103baca0, back);
    }
    std::uint32_t row() const { return c.r(24); }
    std::uint32_t entry() const { return sext8(c.r(25)); }
    std::uint32_t list() const { return c.r(23); }
};

// The routine's static colours, each made on the first call: a flag byte at
// TOC+flag and the colour halfword after it.
void Builder::statics() {
    const auto made = [&](std::int32_t flag, unsigned n) {
        const auto value = c.u8(c.r(2) + std::uint32_t(flag));
        c.set(n, value);
        return std::int8_t(value) != 0;
    };
    const auto store = [&](std::int32_t flag, std::uint32_t value, unsigned n) {
        c.put16(c.r(2) + std::uint32_t(flag + 2), value);
        c.set(n, 1);
        c.put8(c.r(2) + std::uint32_t(flag), 1);
    };
    if (!made(-0x3802, 4)) store(-0x3802, make_colour(0, 0, 0x40, 0x103b3ca8), 7);
    if (!made(-0x3806, 4)) store(-0x3806, make_colour(0, 0x90, 0, 0x103b3cd8), 5);
    if (!made(-0x380a, 3)) {
        c.set({{4, c.toc(-0x7b10)}, {3, c.u16(c.toc(-0x7b10))}});
        store(-0x380a, c.r(3), 4);
    }
    if (!made(-0x380e, 6)) {
        c.set(3, c.toc(-0x7bf4));
        c.set({{7, c.u16(c.r(3))}, {5, c.u16(c.r(3))}});
        store(-0x380e, c.r(5), 4);
    }
    if (!made(-0x3812, 0)) {
        c.set(13, make_colour(0, 0x60, 0, 0x103b3d58));
        c.put16(c.r(2) - 0x3810, c.r(13));
        c.put8(c.r(2) - 0x3812, 1);
    }
    c.set(5, sext8(c.u8(c.r(2) - 0x3816)));
    if (!made(-0x3816, 3)) {
        c.set({{6, mark()}, {4, mark()}});
        store(-0x3816, mark(), 3);
    }
    c.set(6, sext8(c.u8(c.r(2) - 0x381a)));
    if (!made(-0x381a, 0)) {
        c.set(5, ink());
        store(-0x381a, ink(), 3);
    }
    if (!made(-0x381e, 3)) {
        c.set(10, make_colour(0, 0x80, 0, 0x103b3dd4));
        store(-0x381e, c.r(10), 4);
    }
    if (!made(-0x3822, 7)) store(-0x3822, make_colour(0, 0x40, 0, 0x103b3e08), 0);

    // Locals: a copy of a 9-byte table (the list's column widths) and two
    // halfwords, with pointers to them.
    const auto table = c.toc(-0x4ca8);
    c.put32(at(0x144), c.u32(table));
    c.put32(at(0x148), c.u32(table + 4));
    c.put8(at(0x14c), c.u8(table + 8));
    c.put32(at(0xb8), at(0x144));
    c.put16(at(0x92), c.u16(c.r(2) - 0x383e));
    c.put32(at(0xb4), at(0x92));
    c.put16(at(0x90), c.u16(c.r(2) - 0x383c));
    c.put32(at(0xb0), at(0x90));
    c.put16(at(0x82), 0xffff);
    c.set({{5, at(0x92)}, {6, c.u16(c.r(2) - 0x383e)}});
}

// The screen's state object, kept by the game (0x1073ef70 setting 0): made
// and registered on the first visit, from the game's other settings.
void Builder::open_state() {
    const auto setting = [&](std::uint32_t which, std::uint32_t back) {
        c.set({{3, game}, {4, which}});
        return call(0x1073ef70, back);
    };
    screen = setting(0, 0x103b3e78);
    c.set({{5, screen}, {26, screen}});
    view = sext8(setting(10, 0x103b3e8c));
    c.set(20, view);
    if (screen) return;
    c.set(3, screen_size);
    const auto object = call(0x10773520, 0x103b3ea4);
    c.set(4, object);
    c.put32(at(0xbc), object);
    if (object) {
        c.set(15, sext8(setting(8, 0x103b3ec0)));
        c.set(6, c.r(3));
        c.put32(at(0x3e0), setting(7, 0x103b3ed4));
        c.set(5, setting(4, 0x103b3ee4));
        c.put32(at(0x3e4), sext8(c.r(5)));
        c.set(4, setting(3, 0x103b3efc));
        c.set(14, sext8(c.r(4)));
        c.set(13, setting(2, 0x103b3f10));
        const auto match = setting(1, 0x103b3f20);
        c.set({{3, object}, {4, match}, {5, c.r(13)}, {6, c.r(14)}, {7, c.u32(at(0x3e4))}, {8, c.u32(at(0x3e0))},
               {9, c.r(15)}});
        call(0x103c4b10, 0x103b3f40);
    }
    screen = c.u32(at(0xbc));
    c.set({{3, game}, {4, 0}, {26, screen}, {5, screen}, {6, 0}});
    c.call(0x1073f1e0, 0x103b3f58);
}

// The window's caption: the club's name, sized to the title area.
void Builder::caption() {
    c.set(3, screen);
    c.link(0x103b3f60);
    c.set(3, select_club(c, screen));
    c.link(0x103b3f7c);
    c.set(3, title(c, screen, text, 0x7d0, at(0xae), at(0xac)));
    c.set({{3, game}, {4, c.u16(at(0xac))}, {5, c.u16(at(0xae))}, {6, text}});
    const auto window = call(0x10744250, 0x103b3f94);
    c.set({{5, window}, {3, 4}, {4, 0}});
    c.call(0x102a1070, 0x103b3fa4);
    c.set(6, 0);
    c.put8(at(0x315), 0);  // the ticked column's name, set by the View menu
}

// The Tactics menu: team instructions, penalty takers, the formations,
// load, save, presets and saving for the other human managers.
void Builder::tactics_menu() {
    c.put16(at(0x9e), box(0x6e, 0x50, 0x168, 100, 2, 0, 1, {0, 1, 0, 0xffffffffu}, 0x103b4008));
    localise(0x27, 0x103b4018);  // "Tactics"
    cell(0x107444e0, slot16(0x9e), 0, 0, 0x30, ink(), 0xc, 1, {paper(), text, 0, 0}, 0x103b405c);
    c.put16(at(0x9c), heading_menu(c.r(3), 0x103b4068));
    const auto menu = [&] { return slot16(0x9c); };
    localise(0x2f, 0x103b4078);  // "Team Instructions"
    item(menu(), command::team_instructions, 0, 0x103b4094);
    keep(7);
    localise(0x41, 0x103b40a4);  // "Penalty Takers"
    item(menu(), command::penalty_takers, 0, 0x103b40c0);
    keep(8);
    localise(0x50, 0x103b40d0);  // "Set To Default"
    keep(9);
    item(menu(), command::set_to_default, 0, 0x103b40ec);
    keep(8);
    c.put16(at(0x9a), submenu(c.r(3), 0x103b40fc));
    formations();

    separator(menu(), 0x103b4368);
    keep(8);
    localise(0xd5, 0x103b4378);  // "Load"
    keep(9);
    item(menu(), command::load, 0, 0x103b4394);
    c.set(7, c.toc(presets_flag_slot));
    if (c.i8(c.r(7)) != 0) {
        localise(0xda, 0x103b43b8);  // "Load Preset"
        keep(13);
        item(menu(), command::load_preset, 0, 0x103b43d4);
    }
    localise(0xe6, 0x103b43e4);  // "Save"
    keep(9);
    c.set(3, screen);
    c.link(0x103b43f0);
    c.set(3, formation_saved(c, screen));
    bool can_save = flag(8, 0) == 0;
    if (can_save) {
        c.set(3, screen);
        c.link(0x103b4408);
        c.set(3, formation_flag(c, screen));
        can_save = flag(10, 9) != 0;
    }
    if (can_save) item(menu(), command::save, 0, 0x103b4430);
    else disabled_item(menu(), 0x103b4448);
    localise(0xeb, 0x103b4458);  // "Save As"
    keep(9);
    item(menu(), command::save_as, 0, 0x103b4474);
    keep(7);
    c.set(8, c.toc(presets_flag_slot));
    c.set(10, c.u8(c.r(8)));
    if (c.i8(c.r(8)) != 0) {
        localise(0xda, 0x103b4498);  // "Load Preset"
        item(menu(), command::load_preset, 0, 0x103b44b4);
        keep(9);
    }
    c.set(3, 0);
    call(0x103556e0, 0x103b44c0);
    keep(8);
    if (std::int32_t(c.r(3)) > 1) save_for(menu());

    localise(0xfc, 0x103b45e8);  // "Delete"
    keep(8);
    item(menu(), command::erase, 0, 0x103b4604);
    c.set(3, screen);
    c.link(0x103b4610);
    c.set(3, formation_locked(c, screen));
    if (flag(7, 15) != 0) {
        localise(0x103, 0x103b462c);  // "Unlock"
        item(menu(), command::unlock, 0, 0x103b4648);
        keep(9);
    } else {
        c.set(10, c.toc(humans_slot));
        c.set({{3, c.u32(c.r(10))}, {4, 0}});
        const auto club = call(0x10148eb0, 0x103b4660);
        keep(8);
        c.set({{11, c.u8(club)}, {9, sext8(c.u8(club))}});
        if (c.i8(club) != 0) {
            localise(0x10a, 0x103b4680);  // "Lock"
            item(menu(), command::lock, 0, 0x103b469c);
            keep(5);
        }
    }
    // The presets that exist, each "%d. %s", after a separator.
    c.put8(at(0x6a), 0);
    for (c.set(25, 0); std::int8_t(c.r(25)) < 4; c.set(25, c.r(25) + 1)) {
        c.set(10, c.toc(humans_slot));
        c.set({{3, c.u32(c.r(10))}, {4, c.r(25)}, {5, at(0x215)}, {6, 0x100}, {7, 1}, {8, 0}, {9, 1}});
        c.call(0x10147310, 0x103b46d4);
        c.set({{11, c.u8(at(0x215))}, {8, sext8(c.u8(at(0x215)))}});
        if (c.i8(at(0x215)) == 0) continue;
        c.set({{6, c.u8(at(0x6a))}, {7, sext8(c.u8(at(0x6a)))}});
        if (c.i8(at(0x6a)) == 0) {
            separator(menu(), 0x103b4704);
            keep(6);
            c.put8(at(0x6a), 1);
        }
        const auto number = sext8(c.r(25));
        c.set({{3, text}, {4, pool + 0x10f}, {9, number}, {5, number + 1}, {6, at(0x215)}});
        c.call(0x1077b720, 0x103b4728);
        keep(8);
        item(menu(), command::preset, number, 0x103b4744);
        keep(14);
    }
}

// The formations submenu: every formation, the standard ones by their
// localised names, the others by the name in their record.
void Builder::formations() {
    struct Name {
        std::uint32_t name, find_back, compare_back, localise_back;
        unsigned keep_find, keep_compare, keep_localise;
    };
    constexpr Name names[] = {
        {0x5f, 0x103b411c, 0x103b4124, 0x103b413c, 0, 4, 5},  {0x6f, 0x103b4154, 0x103b4164, 0x103b417c, 6, 4, 0},
        {0x7f, 0x103b4190, 0x103b41a0, 0x103b41b8, 4, 0, 0},  {0x8d, 0x103b41cc, 0x103b41dc, 0x103b41f4, 0, 0, 5},
        {0x9d, 0x103b420c, 0x103b421c, 0x103b4234, 6, 7, 8},  {0xad, 0x103b424c, 0x103b425c, 0x103b4274, 9, 5, 6},
        {0xbd, 0x103b428c, 0x103b429c, 0x103b42b4, 0, 7, 0},  {0xcd, 0x103b42cc, 0x103b42dc, 0x103b42f4, 5, 9, 10}};
    for (c.set(25, 0);; c.set(25, c.r(25) + 1)) {
        c.set({{3, tactics()}, {4, 1}});
        c.link(0x103b434c);
        const auto total = formation_total(c, tactics(), 1);
        c.set({{3, total}, {6, total}, {10, sext8(c.r(25))}});
        if (std::int32_t(sext8(c.r(25))) >= std::int32_t(total)) break;
        // 0x103af480 (TACTICS.CPP): a formation record by index.
        const auto formation = [&](std::uint32_t back) {
            c.set({{3, tactics()}, {4, sext8(c.r(25))}, {5, 1}});
            return call(0x103af480, back);
        };
        bool named = false;
        for (const auto& name : names) {
            c.set(3, formation(name.find_back));
            keep(name.keep_find);
            c.set(4, pool + name.name);
            call(0x107393f0, name.compare_back);
            keep(name.keep_compare);
            if (c.r(3) == 0) {
                localise(name.name, name.localise_back);
                keep(name.keep_localise);
                named = true;
                break;
            }
        }
        if (!named) {
            const auto record = formation(0x103b430c);
            copy_text(text, record, 0x103b431c);
            keep(7);
        }
        item(slot16(0x9a), command::formation, sext8(c.r(25)), 0x103b4338);
        keep(8);
    }
}

// "Save For": the tactics menu's submenu of the other human managers, each
// enabled when he manages a club.
void Builder::save_for(std::uint32_t menu) {
    localise(0xf3, 0x103b44d8);
    item(menu, 0, 0, 0x103b44f4);
    keep(8);
    c.put16(at(0x98), submenu(c.r(3), 0x103b4504));
    keep(8);
    for (c.set(25, 0); std::int8_t(c.r(25)) < 16; c.set(25, c.r(25) + 1)) {
        // The human managers hold the last staff ids.
        const auto staff = c.u32(c.toc(-0x7be0));
        const auto count = c.u32(c.toc(-0x7be4));
        const auto id = count - sext8(c.r(25)) - 1;
        const auto manager = staff + id * 0x70;
        c.set({{9, c.toc(-0x7be0)}, {5, count}, {7, count - sext8(c.r(25))}, {8, id}, {9, id * 0x70}, {17, manager},
               {10, c.u8(manager + 0x6b)}, {6, sext8(c.u8(manager + 0x6b))}});
        if (c.i8(manager + 0x6b) != 1) continue;
        c.set(3, game);
        if (call(0x10324090, 0x103b4550) == manager) continue;
        c.set({{3, manager}, {4, text}, {5, 0x7d0}, {6, 1}});
        c.call(0x100caeb0, 0x103b4570);
        keep(9);
        c.set(7, c.toc(humans_slot));
        c.set({{3, c.u32(c.r(7))}, {4, manager}});
        const auto club = call(0x10148eb0, 0x103b4584);
        c.set({{8, club}, {6, c.u8(club)}, {9, sext8(c.u8(club))}});
        if (c.i8(club) != 0) {
            item(slot16(0x98), command::save_for, manager, 0x103b45b0);
        } else {
            disabled_item(slot16(0x98), 0x103b45c8);
            keep(7);
        }
    }
}

// The View menu: which column the squad list shows (r20). The current one
// is ticked and, from Passing on, its name copied for the list's heading.
void Builder::view_menu() {
    localise(0x116, 0x103b4764);  // "View"
    keep(7);
    cell(0x107444e0, slot16(0x9e), 1, 0, 0x30, ink(), 0xc, 1, {paper(), text, 0, 0}, 0x103b47a8);
    c.set(19, heading_menu(c.r(3), 0x103b47b8));
    const auto menu = c.r(19);
    struct Column {
        std::uint32_t name, value;
        unsigned compare;  // the register the original compares the view in
        std::uint32_t text_back, tick_back, copy_back, item_back;
        unsigned keep_text, keep_tick, keep_copy, keep_item;
    };
    const auto column = [&](const Column& k) {
        localise(k.name, k.text_back);
        keep(k.keep_text);
        if (view_is(std::int32_t(k.value), k.compare)) {
            ticked_item(menu, k.tick_back);
            keep(k.keep_tick);
            if (k.copy_back) {
                copy_text(at(0x315), text, k.copy_back);
                keep(k.keep_copy);
            }
        } else {
            item(menu, command::view, k.value, k.item_back);
            keep(k.keep_item);
        }
    };
    column({0x11b, 1, 12, 0x103b47c8, 0x103b47f0, 0, 0x103b4810, 7, 15, 0, 8});      // " Position(s)"
    column({0x128, 2, 9, 0x103b4820, 0x103b4848, 0, 0x103b4868, 14, 10, 0, 8});     // " Condition"
    in_match(0x103b4874);
    if (flag(0, 11) == 0) {
        column({0x133, 3, 14, 0x103b4890, 0x103b48b8, 0, 0x103b48d8, 9, 0, 0, 10});  // " Goals"
        column({0x13a, 4, 9, 0x103b48e8, 0x103b4910, 0, 0x103b4930, 8, 0, 0, 8});    // " Form"
        column({0x140, 5, 10, 0x103b4940, 0x103b4968, 0, 0x103b4984, 9, 0, 0, 5});   // " Average Rating"
    } else {
        column({0x150, 6, 6, 0x103b4998, 0x103b49c0, 0, 0x103b49dc, 4, 0, 0, 0});    // " Match Rating"
    }
    separator(menu, 0x103b49ec);
    keep(7);
    column({0x15e, 7, 4, 0x103b49fc, 0x103b4a24, 0x103b4a34, 0x103b4a50, 0, 0, 0, 7});    // " Passing"
    column({0x167, 8, 0, 0x103b4a60, 0x103b4a88, 0x103b4a94, 0x103b4ab4, 4, 0, 5, 0});    // " Tackling"
    column({0x171, 9, 4, 0x103b4ac0, 0x103b4ae8, 0x103b4af4, 0x103b4b10, 0, 0, 0, 4});    // " Pressing"
    column({0x17b, 10, 0, 0x103b4b20, 0x103b4b48, 0x103b4b54, 0x103b4b74, 6, 0, 7, 5});   // " Pass To"
    column({0x184, 11, 0, 0x103b4b84, 0x103b4bac, 0x103b4bb8, 0x103b4bd8, 4, 0, 13, 0});  // " Set Pieces (A)"
    column({0x194, 12, 5, 0x103b4be8, 0x103b4c0c, 0x103b4c1c, 0x103b4c3c, 0, 6, 4, 0});   // " Set Pieces (D)"
    column({0x1a4, 13, 6, 0x103b4c48, 0x103b4c70, 0x103b4c80, 0x103b4ca0, 0, 0, 5, 0});   // " Free Role"
    column({0x1af, 14, 0, 0x103b4cac, 0x103b4cd0, 0x103b4ce0, 0x103b4d00, 0, 10, 4, 7});  // " Forward Runs"
    column({0x1bd, 15, 3, 0x103b4d10, 0x103b4d38, 0x103b4d48, 0x103b4d68, 6, 0, 4, 5});   // " Run With Ball"
    column({0x1cc, 16, 5, 0x103b4d78, 0x103b4d9c, 0x103b4da8, 0x103b4dc8, 0, 0, 4, 6});   // " Hold Up Ball"
    column({0x1da, 17, 0, 0x103b4dd8, 0x103b4e00, 0x103b4e10, 0x103b4e2c, 5, 4, 0, 4});   // " Long Shots"
    column({0x1e6, 18, 5, 0x103b4e3c, 0x103b4e64, 0x103b4e70, 0x103b4e90, 0, 0, 4, 6});   // " Try Through Balls"
    // " Cross Ball": the original compares the view through sp+0x3e0 and
    // keeps the copy's result at sp+0x3e4.
    localise(0x1f9, 0x103b4ea0);
    keep(15);
    c.put32(at(0x3e0), view);
    if (std::int8_t(view) == 0x13) {
        ticked_item(menu, 0x103b4ed0);
        keep(5);
        c.put32(at(0x3e4), copy_text(at(0x315), text, 0x103b4ee0));
    } else {
        item(menu, command::view, 0x13, 0x103b4f00);
        keep(4);
    }
    // " Marking", whose format takes an empty string.
    c.set({{3, text}, {4, pool + 0x205}, {5, pool + 0x22b}});
    call(0x10727890, 0x103b4f14);
    keep(14);
    if (view_is(0x14, 13)) {
        ticked_item(menu, 0x103b4f3c);
        keep(4);
        copy_text(at(0x315), text, 0x103b4f4c);
    } else {
        item(menu, command::view, 0x14, 0x103b4f68);
    }
    in_match(0x103b4f74);
    if (flag(4, 5) != 0) {
        separator(menu, 0x103b4f90);
        keep(6);
        column({0x22c, 0x15, 8, 0x103b4fa0, 0x103b4fc8, 0x103b4fd4, 0x103b4ff4, 7, 0, 0, 0});  // " Marker"
    }
}

// The Edit menu (Last Match heading; Undo, Copy, Copy All, Paste, Reflect,
// Show or Hide Runs), each enabled by the screen's state.
void Builder::edit_menu() {
    c.put16(at(0x9e), box(0x212, 0x50, 0x30c, 100, 2, 0, 1, {0, 1, 0, 0xffffffffu}, 0x103b5038));
    keep(5);
    localise(0x234, 0x103b504c);  // "Last Match"
    keep(6);
    cell(0x107444e0, slot16(0x9e), 0, 0, 0x30, ink(), 0xc, 1, {paper(), text, 0xe, 0}, 0x103b5090);
    keep(7);
    localise(0x23f, 0x103b50a0);  // "Edit"
    keep(6);
    cell(0x107444e0, slot16(0x9e), 1, 0, 0x30, ink(), 0xc, 1, {paper(), text, 0, 0}, 0x103b50e4);
    c.put16(at(0x96), heading_menu(c.r(3), 0x103b50f4));
    const auto menu = [&] { return slot16(0x96); };
    // An item enabled when `enabled` (the original's copy and test registers
    // given), else disabled.
    const auto entry = [&](std::uint32_t command, bool enabled, std::uint32_t item_back, unsigned keep_item,
                           std::uint32_t disabled_back, unsigned keep_disabled) {
        if (enabled) {
            item(menu(), command, 0, item_back);
            keep(keep_item);
        } else {
            disabled_item(menu(), disabled_back);
            keep(keep_disabled);
        }
    };
    localise(0x244, 0x103b5104);  // "Undo"
    keep(4);
    c.set(3, screen);
    tactics_leaf_103badd0(c.s);
    entry(command::undo, flag(5, 6) != 0, 0x103b5138, 4, 0x103b5150, 0);
    localise(0x249, 0x103b515c);  // "Copy"
    keep(4);
    c.set(3, screen);
    c.link(0x103b5168);
    c.set(3, not_positions_view(c, screen));
    entry(command::copy, flag(0, 3) != 0, 0x103b5190, 0, 0x103b51a8, 0);
    localise(0x24e, 0x103b51b8);  // "Copy All"
    keep(5);
    c.set(3, screen);
    c.link(0x103b51c4);
    c.set(3, not_positions_view(c, screen));
    entry(command::copy_all, flag(6, 7) != 0, 0x103b51ec, 8, 0x103b5204, 9);
    localise(0x257, 0x103b5214);  // "Paste"
    keep(5);
    c.set(3, screen);
    c.link(0x103b5220);
    c.set(3, free_kick_view(c, screen));
    entry(command::paste, flag(6, 0) != 0, 0x103b5248, 7, 0x103b5260, 0);
    localise(0x25d, 0x103b5270);  // "Reflect"
    keep(5);
    c.set(3, screen);
    c.link(0x103b527c);
    c.set(3, not_positions_view(c, screen));
    entry(command::reflect, flag(9, 10) != 0, 0x103b52a4, 0, 0x103b52bc, 7);
    c.set(3, screen);
    tactics_leaf_103bad00(c.s);
    if (flag(8, 6) == 0) {
        localise(0x265, 0x103b52e4);  // "Show Runs"
        keep(10);
    } else {
        localise(0x26f, 0x103b52f8);  // "Hide Runs"
        keep(8);
    }
    c.set(3, screen);
    tactics_leaf_103bacf0(c.s);
    entry(command::runs, flag(9, 0) != 1, 0x103b532c, 7, 0x103b5344, 0);
}

// The panels between the menus and the squad list: the formation's name
// (with "*" while unsaved, and LOCKED), the Overview / With Ball / Without
// Ball tabs, the list's column widths for the chosen view and the list boxes.
void Builder::headings() {
    c.set(3, screen);
    tactics_leaf_103bace0(c.s);
    keep(10);
    copy_text(text, c.r(10), 0x103b53c0);
    keep(9);
    c.set(3, screen);
    c.link(0x103b53cc);
    c.set(3, formation_saved(c, screen));
    if (flag(12, 0) == 0) {
        c.set({{3, text}, {4, pool + 0x279}});  // "*"
        call(0x1077dc60, 0x103b53e8);
        keep(9);
    }
    const auto panel = [&](std::uint32_t left, std::uint32_t right, std::uint32_t bottom, std::uint32_t kind,
                           std::uint32_t style, std::uint32_t first, std::uint32_t back) {
        stack_args({first, mark(), text});
        c.set({{3, game}, {4, left}, {5, 0x69}, {6, right}, {7, bottom}, {8, kind}, {9, 0}, {10, style}});
        return call(0x10744390, back);
    };
    panel(0x6e, 0x1b8, 0x8c, 2, 0xc, 3, 0x103b5424);
    keep(10);
    c.set(3, screen);
    c.link(0x103b5430);
    c.set(3, formation_locked(c, screen));
    if (flag(0, 11) != 0) {
        localise(0x27b, 0x103b544c);  // "LOCKED  "
        keep(9);
        panel(0x154, 0x1b8, 0x7d, 1, 0x4c, 1, 0x103b5488);
        keep(8);
    }

    // The tabs; the one for the pitch view (screen +0x27a4) highlighted, the
    // others shaded when the screen is read-only.
    c.put16(at(0x9e), box(0x1c2, 0x69, 0x30c, 0x8c, 3, 0, 1, {0, 1, 0, 0xffffffffu}, 0x103b54d0));
    const auto heading = [&] { return c.u16(c.toc(heading_colour_slot)); };
    struct Tab {
        std::uint32_t name, text_back;
        unsigned keep_text, view_copy, view_test;
        std::uint32_t current_back, tab_back, shaded_back;
        unsigned keep_current, keep_tab, keep_shaded, read_only_test;
    };
    const auto tab = [&](std::uint32_t column, const Tab& t) {
        localise(t.name, t.text_back);
        keep(t.keep_text);
        c.set(3, screen);
        tactics_leaf_103bacf0(c.s);
        if (flag(t.view_copy, t.view_test) == std::int8_t(column + 1)) {
            cell(0x107445a0, slot16(0x9e), column, 0, 0x830, heading(), 0xc, 1, {paper(), text}, t.current_back);
            keep(t.keep_current);
            return;
        }
        if (column == 0 || (read_only(), flag(7, t.read_only_test) == 0)) {
            cell(0x107444e0, slot16(0x9e), column, 0, 0x30, heading(), 0xc, 1, {paper(), text, 0x15, column + 1},
                 t.tab_back);
            keep(t.keep_tab);
        } else {
            cell(0x107445a0, slot16(0x9e), column, 0, 0x30, heading(), 0x2c, 1, {paper(), text}, t.shaded_back);
            keep(t.keep_shaded);
        }
    };
    keep(6);
    tab(0, {0x284, 0x103b54e4, 5, 7, 8, 0x103b5534, 0x103b5580, 0, 6, 6, 0, 0});        // "Overview"
    tab(1, {0x28d, 0x103b5590, 9, 0, 7, 0x103b55e0, 0x103b5644, 0x103b5680, 0, 11, 5, 15});  // "With Ball"
    // "Without Ball": the original loads the colour's address into r14 for
    // the plain tab.
    localise(0x297, 0x103b5690);
    c.set(3, screen);
    tactics_leaf_103bacf0(c.s);
    if (flag(10, 0) == 3) {
        cell(0x107445a0, slot16(0x9e), 2, 0, 0x830, heading(), 0xc, 1, {paper(), text}, 0x103b56e0);
        keep(6);
    } else if (read_only(), flag(7, 6) == 0) {
        c.set(14, c.toc(heading_colour_slot));
        cell(0x107444e0, slot16(0x9e), 2, 0, 0x30, heading(), 0xc, 1, {paper(), text, 0x15, 3}, 0x103b5744);
    } else {
        cell(0x107445a0, slot16(0x9e), 2, 0, 0x30, heading(), 0x2c, 1, {paper(), text}, 0x103b5780);
        keep(8);
    }

    // The list's column widths (the 9-byte table at sp+0x144): the views
    // without some columns give their width to a neighbour.
    const auto width = [&](std::int32_t offset) { return c.u8(at(offset)); };
    const auto merge = [&](std::int32_t into, std::int32_t from) {
        c.put8(at(into), width(into) + width(from));
        c.put8(at(from), 0);
    };
    const bool short_columns = view_is(1, 9) || view_is(3, 0) || view_is(4, 19) || view_is(5, 7);
    if (short_columns) {
        c.set(15, width(0x14b));
        merge(0x14a, 0x14b);
        c.set(14, 0);
        merge(0x14a, 0x14c);
    } else if (view_is(2, 11)) {
        c.set(14, width(0x14c));
        merge(0x14b, 0x14c);
    } else if (!view_is(6, 8)) {
        merge(0x149, 0x14a);
        merge(0x149, 0x14b);
        merge(0x149, 0x14c);
    }

    // The list: with a heading row naming the ticked column, or without.
    box(0x6e, 0x91, 0x1b8, 0x221, 1, 0, 1, {0, 2, 0, 0xffffffffu}, 0x103b58a8);
    if (c.i8(at(0x315)) != 0) {
        c.set(23, box(0x6e, 0x91, 0x1b8, 0x221, 9, at(0x144), 0x15, {0, 1, 0, 0xffffffffu}, 0x103b5900));
        c.put16(at(0x94), box(0x6e, 0x91, 0x1b8, 0x221, 2, at(0x92), 0x15, {0, 1, 0, 0xffffffffu}, 0x103b5948));
        cell(0x107445a0, list(), 5, 0, 0x10, colour(panel_colour_slot), 0xc, 1, {paper(), at(0x315)}, 0x103b5980);
        keep(6);
        c.set(24, 1);
    } else {
        c.set(23, box(0x6e, 0x99, 0x1b8, 0x219, 9, at(0x144), 0x14, {0, 1, 0, 0xffffffffu}, 0x103b59d0));
        c.set({{4, list()}, {13, 0}});
        c.put16(at(0x94), box(0x6e, 0x99, 0x1b8, 0x219, 2, at(0x92), 0x14, {0, 1, 0, 0xffffffffu}, 0x103b5a1c));
        c.set(24, 0);
    }
}

// The squad list: one row per squad entry (r25), from row r24.
void Builder::squad() {
    c.set(25, 0);
    for (;;) {
        c.set(9, entry());
        if (std::int8_t(c.r(25)) >= 20) break;
        squad_entry();
        c.set({{24, c.r(24) + 1}, {25, c.r(25) + 1}});
    }
}

// One row of the squad list: the drag handle, the penalty or captain mark
// (views 2 and 6), the player's name, his availability marks, the chosen
// view's column and his role.
void Builder::squad_entry() {
    const auto player = entry_query(0x103ca4e0, 0x103b5a3c);
    c.set({{5, player}, {17, player}});
    if (read_only() == 0) {
        flag(6, 4);
        control(0x102, 0, 0, 0, 0, 1, row(), {1, 0, 0, 0xc, 1, 0, 0, 0, 0x16, entry(), slot16(0x94)}, 0x103b5ad8);
        keep(6);
    } else {
        flag(6, 4);
    }
    if (view_is(2, 3) || view_is(6, 0)) {
        c.set(3, screen);
        tactics_leaf_103bacd0(c.s);
        if (flag(4, 5) != 0) {
            // A national team: the entry's penalty order.
            c.put8(at(0x71), 0);
            while (true) {
                c.set({{0, c.u8(at(0x71))}, {4, sext8(c.u8(at(0x71)))}});
                if (c.i8(at(0x71)) >= 11) break;
                c.set({{3, screen}, {4, c.u8(at(0x71))}});
                c.link(0x103b5b24);
                c.set(3, lineup_byte(c, screen, c.u8(at(0x71))));
                c.set({{5, c.r(3)}, {4, entry()}});
                c.set(3, sext8(c.r(5)));
                if (std::int8_t(c.r(25)) == std::int8_t(c.r(5))) break;
                c.set({{6, c.u8(at(0x71))}, {5, c.u8(at(0x71)) + 1}});
                c.put8(at(0x71), c.r(5));
            }
            c.set({{3, c.u8(at(0x71))}, {4, sext8(c.u8(at(0x71)))}});
            if (c.i8(at(0x71)) < 11) {
                const auto order = std::uint32_t(std::int32_t(c.i8(at(0x71))));
                c.set({{3, text}, {4, pool + 0x2a4}, {5, order + 1}});  // "pen <%d - number>"
                call(0x10727890, 0x103b5b7c);
                const auto first_five = [&](std::int32_t offset) {
                    return std::int32_t(c.u32(at(offset))) < 5;
                };
                if (read_only() == 0) {
                    flag(4, 6);
                    c.set(15, c.u8(at(0x71)));
                    c.put32(at(0x3e0), sext8(c.r(15)));
                    if (first_five(0x3e0)) {
                        c.put16(at(0x7e), mark());
                    } else {
                        c.put32(at(0x3e4), ink());
                        c.put16(at(0x7e), c.u32(at(0x3e4)));
                    }
                    c.set({{14, 0}, {13, 0}});
                    control(0x42, 0, 0, 0, 0, 5, row(),
                            {1, 0, 0, 0xc, 1, c.u16(at(0x7e)), text, 0, 0x18, sext8(c.u8(at(0x71))), list()},
                            0x103b5c40);
                } else {
                    flag(4, 6);
                    c.put16(at(0x7c), c.i8(at(0x71)) < 5 ? mark() : ink());
                    control(1, 0, 0, 0, 0, 5, row(),
                            {1, 0, 0, 0xc, 1, c.u16(at(0x7c)), text, 0, 0x18, sext8(c.u8(at(0x71))), list()},
                            0x103b5ce4);
                }
                keep(8);
            }
        } else {
            // A club: "capt" on the captain's row (on the first row when there is none).
            c.set(3, screen);
            c.link(0x103b5cf4);
            c.set(3, selected_entry(c, screen));
            c.set({{9, c.r(3)}, {8, sext8(c.r(3))}, {0, entry()}});
            bool captain = std::int8_t(c.r(25)) == std::int8_t(c.r(9));
            if (!captain) {
                c.set(3, screen);
                c.link(0x103b5d10);
                c.set(3, selected_entry(c, screen));
                c.set(4, sext8(c.r(3)));
                captain = std::int8_t(c.r(3)) == -1 && (c.set(5, entry()), std::int8_t(c.r(25)) == 0);
            }
            if (captain) {
                localise(0x2b6, 0x103b5d34);  // "capt"
                keep(6);
                if (read_only() == 0) {
                    flag(4, 3);
                    c.set(3, screen);
                    c.link(0x103b5d58);
                    c.set(3, selected_entry(c, screen));
                    if (flag(4, 0) == -1) {
                        c.set(7, c.toc(-0x7b10));
                        control(0x42, 0, 0, 0, 0, 5, row(),
                                {1, 0, 0, 0xc, 1, c.u16(c.r(7)), text, 0, 0x17, 0, list()}, 0x103b5de0);
                    } else {
                        control(0x42, 0, 0, 0, 0, 5, row(), {1, 0, 0, 0xc, 1, mark(), text, 0, 0x17, 0, list()},
                                0x103b5e5c);
                        keep(10);
                    }
                } else {
                    flag(4, 3);
                    c.set(13, 0);
                    control(1, 0, 0, 0, 0, 5, row(), {1, 0, 0, 0xc, 1, mark(), text, 0, 0x17, 0, list()},
                            0x103b5ed8);
                    keep(8);
                }
            }
        }
    }
    entry_details(player);
}

// The rest of a squad row: the name (coloured by availability), the
// warning marks, the chosen view's column and the role mark.
void Builder::entry_details(std::uint32_t player) {
    // The name's colour: the first eleven in one colour, the substitutes in another.
    if (std::int8_t(c.r(25)) < 11) {
        c.set(10, c.toc(-0x7bdc));
        c.set(9, c.u16(c.r(10)));
        c.put16(at(0x7a), c.r(9));
    } else {
        c.set(12, c.u16(c.r(2) - 0x3800));
        c.put16(at(0x7a), c.r(12));
    }
    c.put16(at(0xaa), c.u16(at(0x7a)));
    // 0x103c8e70 (screens_instructions): the entry's player name into the text buffer.
    c.set({{3, screen}, {4, c.r(25)}, {5, text}, {6, 0x7d0}, {7, 0}, {8, 0}});
    call(0x103c8e70, 0x103b5f24);
    keep(9);
    // 0x103c9720 (screens_instructions): whether the entry holds a player.
    c.set({{3, screen}, {4, c.r(25)}});
    call(0x103c9720, 0x103b5f34);
    if (flag(7, 8) == 0) {
        cell(0x107445a0, list(), 0, row(), 0x30, c.u16(at(0xaa)), 0x2c, 1, {paper(), text}, 0x103b5f74);
    } else {
        entry_playing(0, 0x103b5f90);
        bool marked = flag(11, 9) != 0;
        if (!marked) {
            entry_query(0x103c9970, 0x103b5fb0);
            if (flag(8, 0) != 0) {
                c.set(3, screen);
                c.link(0x103b5fc8);
                c.set(3, substitutes_left(c, screen));
                c.set(8, c.r(3));
                c.set(8, sext8(c.r(8)));
                marked = std::int8_t(c.r(8)) > 0;
            }
        }
        if (marked) {
            const auto name = c.u16(at(0xaa));
            if (read_only() == 0) {
                flag(9, 10);
                control(0x42, 0, 0, 0, 0, 0, row(), {0x30, name, name, 0xc, 1, paper(), text, 0, 0x19, entry(), list()},
                        0x103b6064);
                keep(9);
            } else {
                flag(9, 10);
                control(1, 0, 0, 0, 0, 0, row(), {0x30, name, name, 0xc, 1, paper(), text, 0, 0x19, entry(), list()},
                        0x103b60e0);
                keep(7);
            }
        } else {
            c.set(15, paper());
            cell(0x107445a0, list(), 0, row(), 0x30, c.u16(at(0xaa)), 0xc, 1, {paper(), text}, 0x103b6118);
        }
    }
    // The drop target of an editable list.
    if (read_only() == 0) {
        flag(9, 10);
        control(0x302, 0, 0, 0, 0, 0, row(), {1, 0, 0, 0xc, 1, 0, 0, 0, 0x16, entry(), list()}, 0x103b61ac);
        keep(6);
    } else {
        flag(9, 10);
    }
    // Warning marks (0x103c9aa0, 0x103c9bd0) in the third column.
    const auto warning = [&](std::uint32_t flags, std::uint32_t ink_colour, std::uint32_t back) {
        cell(0x107445a0, list(), 2, row(), flags, ink_colour, 0xc, 1, {colour(panel_colour_slot), 0}, back);
    };
    entry_query(0x103c9aa0, 0x103b61c0);
    if (flag(7, 6) != 0) {
        entry_query(0x103c9bd0, 0x103b61e0);
        if (flag(0, 9) != 0) {
            c.set(14, 0);
            warning(0x20030, mark(), 0x103b6228);
            warning(0x40030, c.u16(c.toc(-0x7a74)), 0x103b6268);
            keep(9);
        } else {
            c.set(19, 0);
            warning(0x20030, mark(), 0x103b62a8);
            keep(7);
        }
    } else {
        entry_query(0x103c9bd0, 0x103b62c0);
        if (flag(12, 15) != 0) {
            c.set({{14, 0}, {9, c.toc(-0x7a74)}});
            warning(0x20030, c.u16(c.toc(-0x7a74)), 0x103b630c);
            keep(10);
        }
    }
    entry_role(player);
}

// After the warning marks: the chosen view's column, then the role mark.
void Builder::entry_role(std::uint32_t player) {
    view_column(player);
    role_mark(player);
}

// The column of the chosen view for a squad row.
void Builder::view_column(std::uint32_t player) {
    // The player record of the entry, for views 1, 3, 4 and 5 (none when
    // the entry holds no player); the original tests it in `test`.
    const auto record_of = [&](unsigned test) -> std::uint32_t {
        if (!player) return 0;
        c.set(test, c.u32(player + 0x14));
        return c.u32(player + 0x14);
    };
    // The detail cell (column 6), in the mark colour when the entry's player is playing.
    const auto detail_cell = [&](bool playing, std::uint32_t font, std::uint32_t back) {
        cell(0x107445a0, list(), 6, row(), 1, 0, font, 1, {playing ? mark() : ink(), text}, back);
    };
    // 0x10303ff0: one of the player's ratings (`which`) as text, as seen by
    // the opponent (5) or the club (6).
    const auto rating = [&](std::uint32_t record, std::uint32_t which, bool opponent, std::uint32_t back) {
        stack_args({1});
        c.set({{3, c.u32(c.r(18))}, {4, record}, {5, opponent ? 5u : 6u}, {6, which}, {7, text}, {8, 0x7d0},
               {9, 1}, {10, 0}});
        call(0x10303ff0, back);
    };
    if (view_is(1, 8)) {
        // Position(s).
        const auto record = record_of(0);
        if (!record) return;
        c.set({{3, record}, {4, text}, {5, 0x7d0}, {6, 1}});
        call(0x100cc4a0, 0x103b6344);
        keep(11);
        entry_playing(0, 0x103b635c);
        if (flag(9, 14) != 0) {
            detail_cell(true, 1, 0x103b639c);
            keep(10);
        } else {
            detail_cell(false, 1, 0x103b63d4);
            keep(9);
        }
    } else if (view_is(3, 0)) {
        // Goals.
        const auto record = record_of(8);
        if (!record) return;
        opponent_view(0x103b6404);
        if (flag(9, 10) != 0) {
            rating(record, 2, true, 0x103b6440);
            keep(5);
        } else {
            rating(record, 2, false, 0x103b6474);
            keep(6);
        }
        entry_playing(0, 0x103b648c);
        const bool playing = flag(0, 0) != 0;
        detail_cell(playing, 0xc, playing ? 0x103b64c8 : 0x103b6500);
    } else if (view_is(4, 3)) {
        // Form.
        const auto record = record_of(7);
        if (!record) return;
        c.set({{3, c.u32(c.r(18))}, {4, record}, {5, text}, {6, 0x7d0}});
        call(0x10302e70, 0x103b653c);
        entry_playing(0, 0x103b6550);
        if (flag(4, 0) != 0) {
            detail_cell(true, 0xc, 0x103b6590);
            keep(5);
        } else {
            detail_cell(false, 0xc, 0x103b65c8);
        }
    } else if (view_is(5, 4)) {
        // Average rating.
        const auto record = record_of(3);
        if (!record) return;
        opponent_view(0x103b65f8);
        if (flag(0, 4) != 0) {
            rating(record, 0x11, true, 0x103b6630);
        } else {
            rating(record, 0x11, false, 0x103b6664);
            keep(7);
        }
        entry_playing(0, 0x103b667c);
        if (flag(5, 4) != 0) {
            detail_cell(true, 0xc, 0x103b66bc);
        } else {
            c.set(13, ink());
            detail_cell(false, 0xc, 0x103b66f0);
        }
    } else if (view_is(2, 3) || view_is(6, 5)) {
        // Condition / match rating: the entry's condition text (0x103c9d00)
        // and, for condition, the fitness or "Inj"; for match rating the
        // rating and the goals.
        c.set({{3, screen}, {4, c.r(25)}, {5, text}, {6, 0x7d0}, {7, at(0xaa)}, {8, 0}});
        call(0x103c9d00, 0x103b672c);
        keep(6);
        c.set({{4, c.u8(text)}, {3, sext8(c.u8(text))}});
        if (c.i8(text) != 0) {
            cell(0x107445a0, list(), 6, row(), 1, 0, 1, 1, {c.u16(at(0xaa)), text}, 0x103b6770);
            keep(6);
        }
        if (view_is(2, 0)) {
            c.put8(at(0x70), entry_query(0x103c9f40, 0x103b6790));
            if (flag(5, 3) == 0) return;
            c.set({{3, c.u8(at(0x70))}, {0, sext8(c.u8(at(0x70)))}});
            if (c.i8(at(0x70)) > 0) {
                c.set({{3, text}, {4, pool + 0x2bb}, {5, sext8(c.u8(at(0x70)))}});  // "%d%%"
                call(0x1077b720, 0x103b67c8);
                keep(10);
            } else {
                localise(0x2c0, 0x103b67dc);  // "Inj"
                keep(4);
            }
            entry_playing(0, 0x103b67f4);
            const bool playing = flag(7, 6) != 0;
            cell(0x107445a0, list(), 7, row(), 1, 0, 0xc, 1, {playing ? mark() : ink(), text},
                 playing ? 0x103b6834 : 0x103b686c);
            if (!playing) keep(5);
        } else {
            c.put8(at(0x6f), entry_query(0x103ca120, 0x103b6884));
            c.set(5, sext8(c.r(3)));
            if (std::int8_t(c.r(3)) <= 0) return;
            c.set({{3, text}, {4, pool + 0x2c4}, {5, sext8(c.u8(at(0x6f)))}});  // "%d"
            call(0x1077b720, 0x103b68a8);
            c.set(4, c.toc(-0x7bd8));
            cell(0x107445a0, list(), 7, row(), 1, 0, 0xc, 2, {c.u16(c.r(4)), text}, 0x103b68dc);
            keep(5);
            c.put8(at(0x6e), entry_query(0x103ca260, 0x103b68f0));
            c.set(4, sext8(c.r(3)));
            if (std::int8_t(c.r(3)) <= 0) return;
            c.set({{3, text}, {4, pool + 0x2c4}, {5, sext8(c.u8(at(0x6e)))}});
            call(0x1077b720, 0x103b6918);
            cell(0x107445a0, list(), 8, row(), 1, 0, 0xc, 1, {mark(), text}, 0x103b6948);
        }
    } else {
        instructions_column();
    }
}

// Views 7-21: the player's instruction setting, as a cell a click cycles.
void Builder::instructions_column() {
    entry_playing(at(0x6d), 0x103b6964);
    if (flag(5, 3) == 0) return;
    // 0x103bac60 (screens_misc): the slot's instruction fields (r3 high, r4 low).
    c.set({{3, screen}, {4, c.u8(at(0x6d))}});
    call(0x103bac60, 0x103b6980);
    c.set(6, c.r(3));
    c.put32(at(0x140), c.r(4));
    c.put32(at(0x13c), c.r(6));
    const auto none = [](Stage) {};
    const auto high = [&] { return c.u32(at(0x13c)); };
    const auto low = [&] { return c.u32(at(0x140)); };
    const auto slot = [&] { return sext8(c.u8(at(0x6d))); };
    const auto with_comment = true, plain = false;
    using O = Option;
    if (view_is(7, 15)) {  // Passing
        option(O{0, 1, 0x2c7, with_comment, 0, 2, true, 0x103b69ec, 0x103b69fc, 0x103b6a58}, [&](Stage stage) {
            if (stage != Stage::test) return;
            c.put32(at(0x3e0), low());
            c.put32(at(0x3e4), 1);
            c.set({{14, low() & 1}, {13, 0}});
        });
        option(O{0, 2, 0x2f0, with_comment, 0, 4, false, 0x103b6aa0, 0x103b6aac, 0x103b6b08}, none);
        option(O{0, 4, 0x31a, with_comment, 0, 8, false, 0x103b6b50, 0x103b6b60, 0x103b6bbc}, none);
        option(O{0, 8, 0x344, with_comment, 0, 0x10, false, 0x103b6c04, 0x103b6c14, 0x103b6c70}, [&](Stage stage) {
            if (stage == Stage::test) c.set(13, high());
        });
        option(O{0, 0x10, 0x36f, with_comment, 0, 1, false, 0x103b6cb8, 0x103b6cc8, 0x103b6d24}, [&](Stage stage) {
            if (stage == Stage::cell) keep(15);
        });
    } else if (view_is(8, 0)) {  // Tackling
        option(O{0, 0x200, 0x398, with_comment, 0, 0x400, true, 0x103b6d7c, 0x103b6d8c, 0x103b6de8},
               [&](Stage stage) {
                   if (stage == Stage::bit) c.set(14, (slot() & 0x3f) << 6 | (bit_ & 0x3f));
               });
        option(O{0, 0x400, 0x3b6, with_comment, 0, 0x800, false, 0x103b6e30, 0x103b6e40, 0x103b6e9c},
               [&](Stage stage) {
                   if (stage == Stage::test) c.set({{19, 0}, {15, 0}, {14, low() & 0x400}});
                   if (stage == Stage::bit) c.set(14, slot());
               });
        option(O{0, 0x800, 0x3d6, with_comment, 0, 0x1000, false, 0x103b6ee4, 0x103b6ef4, 0x103b6f50}, none);
        option(O{0, 0x1000, 0x3f4, with_comment, 0, 0x200, false, 0x103b6f94, 0x103b6fa4, 0x103b7000},
               [&](Stage stage) {
                   if (stage == Stage::text) keep(13);
               });
    } else if (view_is(9, 3)) {  // Pressing
        option(O{0, 0x2000, 0x412, with_comment, 0, 0x4000, true, 0x103b7054, 0x103b7064, 0x103b70bc}, none);
        option(O{0, 0x4000, 0x430, with_comment, 0, 0x8000, false, 0x103b7110, 0x103b7120, 0x103b717c},
               [&](Stage stage) {
                   if (stage != Stage::test) return;
                   c.set(15, 0x4000);
                   c.put32(at(0x3e0), 0);
                   c.put32(at(0x3e4), 0);
                   c.set({{14, 0}, {13, low() & 0x4000}});
               });
        option(O{0, 0x8000, 0x44c, with_comment, 0, 0x2000, false, 0x103b71c4, 0x103b71d4, 0x103b7230}, none);
    } else if (view_is(10, 0)) {  // Pass To
        option(O{0, 0x20, 0x469, with_comment, 0, 0x40, true, 0x103b7284, 0x103b7294, 0x103b72f0}, none);
        option(O{0, 0x40, 0x487, with_comment, 0, 0x80, false, 0x103b7338, 0x103b7348, 0x103b73a4},
               [&](Stage stage) {
                   if (stage == Stage::test) c.set(13, 0);
               });
        option(O{0, 0x80, 0x4a4, with_comment, 0, 0x100, false, 0x103b73ec, 0x103b73fc, 0x103b7458}, none);
        option(O{0, 0x100, 0x4c3, with_comment, 0, 0x20, false, 0x103b74a0, 0x103b74b0, 0x103b750c},
               [&](Stage stage) {
                   if (stage == Stage::test) c.set(15, high());
                   if (stage == Stage::cell) keep(14);
               });
    } else if (view_is(11, 0)) {  // Set Pieces (A)
        option(O{0, 0x10000, 0x4e1, with_comment, 0, 0x40000000, true, 0x103b7568, 0x103b757c, 0x103b75d8},
               [&](Stage stage) {
                   if (stage == Stage::test) c.set({{19, 0}, {15, low() & 0x10000}});
                   if (stage == Stage::text) keep(14);
                   if (stage == Stage::bit) c.set(14, (slot() & 0x3f) << 6);
               });
        option(O{0, 0x40000000, 0x521, with_comment, 0, 0x20000, false, 0x103b7624, 0x103b7638, 0x103b7694}, none);
        option(O{0, 0x20000, 0x559, with_comment, 0, 0x10000, false, 0x103b76e0, 0x103b76f4, 0x103b774c}, none);
    } else if (view_is(12, 13)) {  // Set Pieces (D)
        option(O{0, 0x40000, 0x597, with_comment, 0, 0x80000, true, 0x103b77a4, 0x103b77b4, 0x103b780c}, none);
        option(O{0, 0x80000, 0x5d7, with_comment, 0, 0x80000000u, false, 0x103b7854, 0x103b7868, 0x103b78d0},
               [&](Stage stage) {
                   if (stage != Stage::bit) return;
                   c.set(15, c.u8(at(0x6d)));
                   c.put32(at(0x3e0), sext8(c.r(15)));
                   c.put32(at(0x3e4), (c.u32(at(0x3e0)) & 0x3f) << 6);
                   c.set({{14, mark()}, {13, 0x1a}});
               });
        option(O{0, 0x80000000u, 0x60f, with_comment, 0, 0x40000, false, 0x103b791c, 0x103b7930, 0x103b7988}, none);
    } else if (view_is(13, 9)) {  // Free Role
        option(O{0, 0x100000, 0x64d, plain, 0, 0x200000, true, 0x103b79e0, 0x103b79f0, 0x103b7a4c}, none);
        option(O{0, 0x200000, 0x650, plain, 0, 0x100000, false, 0x103b7a94, 0x103b7aa8, 0x103b7b04},
               [&](Stage stage) {
                   if (stage == Stage::bit) c.set(13, slot());
               });
    } else if (view_is(14, 12)) {  // Forward Runs
        option(O{0, 0x400000, 0x64d, plain, 0, 0x800000, true, 0x103b7b5c, 0x103b7b70, 0x103b7bcc}, none);
        option(O{0, 0x800000, 0x650, plain, 0, 0x400000, false, 0x103b7c14, 0x103b7c28, 0x103b7c84},
               [&](Stage stage) {
                   if (stage == Stage::bit) c.set(15, bit_);
               });
    } else if (view_is(15, 10)) {  // Run With Ball
        option(O{0, 0x1000000, 0x64d, plain, 0, 0x2000000, true, 0x103b7cdc, 0x103b7cf0, 0x103b7d4c},
               [&](Stage stage) {
                   if (stage == Stage::test) c.set(14, low() & 0x1000000);
                   if (stage == Stage::bit) c.set({{19, slot() & 0x3f}, {15, ink()}});
                   if (stage == Stage::cell) keep(14);
               });
        option(O{0, 0x2000000, 0x650, plain, 0, 0x1000000, false, 0x103b7d94, 0x103b7da8, 0x103b7e04},
               [&](Stage stage) {
                   if (stage == Stage::test) c.set(14, 0);
               });
    } else if (view_is(16, 0)) {  // Hold Up Ball
        option(O{0, 0x4000000, 0x64d, plain, 0, 0x8000000, true, 0x103b7e5c, 0x103b7e70, 0x103b7ec8}, none);
        option(O{0, 0x8000000, 0x650, plain, 0, 0x4000000, false, 0x103b7f0c, 0x103b7f20, 0x103b7f78},
               [&](Stage stage) {
                   if (stage == Stage::test) c.set(13, low());
               });
    } else if (view_is(17, 3)) {  // Long Shots
        option(O{0, 0x10000000, 0x64d, plain, 0, 0x20000000, true, 0x103b7fd0, 0x103b7fe4, 0x103b804c},
               [&](Stage stage) {
                   if (stage != Stage::bit) return;
                   c.set(15, c.u8(at(0x6d)));
                   c.put32(at(0x3e0), sext8(c.r(15)));
                   c.put32(at(0x3e4), (c.u32(at(0x3e0)) & 0x3f) << 6);
                   c.set({{14, ink()}, {13, 0x1a}});
               });
        option(O{0, 0x20000000, 0x650, plain, 0, 0x10000000, false, 0x103b8094, 0x103b80a8, 0x103b8100}, none);
    } else if (view_is(18, 9)) {  // Try Through Balls
        option(O{2, 0, 0x64d, plain, 1, 0, true, 0x103b8154, 0x103b8160, 0x103b81bc}, none);
        option(O{1, 0, 0x650, plain, 2, 0, false, 0x103b8200, 0x103b8210, 0x103b826c}, [&](Stage stage) {
            if (stage == Stage::bit) c.set(13, slot());
        });
    } else if (view_is(19, 12)) {  // Cross Ball
        option(O{8, 0, 0x64d, plain, 4, 0, true, 0x103b82c0, 0x103b82d0, 0x103b832c}, none);
        option(O{4, 0, 0x650, plain, 8, 0, false, 0x103b8370, 0x103b8380, 0x103b83dc}, [&](Stage stage) {
            if (stage == Stage::bit) c.set(15, bit_);
        });
    } else if (view_is(20, 10)) {
        marking_column();
    } else if (view_is(21, 4)) {
        marker_column();
    }
}

// One instruction setting: when the player's instruction fields hold its
// bit, its name and a cell whose click value is the slot and the bit of the
// next setting ((slot & 0x3f) << 6 | bit & 0x3f). Returns whether shown.
template <typename Effects> bool Builder::option(const Option& o, Effects&& effects) {
    effects(Stage::test);
    if (!((c.u32(at(0x13c)) & o.high) | (c.u32(at(0x140)) & o.low))) return false;
    c.set({{3, text}, {4, pool + o.name}});
    if (o.comment) c.set(5, pool + 0x22b);
    call(0x10727890, o.text_back);
    effects(Stage::text);
    c.set({{3, o.next_high}, {4, o.next_low}});
    c.link(o.bit_back);
    c.set(3, bit_index(c, o.next_high, o.next_low));
    bit_ = sext8(c.r(3));
    slot_ = sext8(c.u8(at(0x6d)));
    effects(Stage::bit);
    const auto value = (slot_ & 0x3f) << 6 | (bit_ & 0x3f);
    cell(0x107444e0, list(), 5, row(), 1, 0, 0xc, 1, {o.first ? ink() : mark(), text, 0x1a, value}, o.cell_back);
    effects(Stage::cell);
    return true;
}

// View 20, Marking: none, zonal, or man marking with the marked player's initials.
void Builder::marking_column() {
    // 0x103bac20 (screens_misc): the slot's role bits.
    c.set({{3, screen}, {4, c.u8(at(0x6d))}});
    c.put8(at(0x6c), call(0x103bac20, 0x103b83fc));
    const auto marking_cell = [&](std::uint32_t next, bool first, std::uint32_t bit_back, std::uint32_t cell_back) {
        c.set({{3, 0}, {4, next}});
        c.link(bit_back);
        c.set(3, bit_index(c, 0, next));
        bit_ = sext8(c.r(3));
        slot_ = sext8(c.u8(at(0x6d)));
        return std::pair{first, cell_back};
    };
    const auto show = [&](std::pair<bool, std::uint32_t> where, std::uint32_t colour_value) {
        cell(0x107444e0, list(), 5, row(), 1, 0, 0xc, 1, {colour_value, text, 0x1b, (slot_ & 0x3f) << 6 | (bit_ & 0x3f)},
             where.second);
    };
    if (c.u8(at(0x6c)) & 1) {
        c.set({{3, text}, {4, pool + 0x654}, {5, pool + 0x22b}});  // "None"
        call(0x10727890, 0x103b8428);
        const auto where = marking_cell(2, true, 0x103b8438, 0x103b8494);
        c.set(14, slot_);
        show(where, ink());
    }
    c.set(19, c.u8(at(0x6c)));
    if (c.u8(at(0x6c)) & 2) {
        c.set({{3, text}, {4, pool + 0x674}, {5, pool + 0x22b}});  // "Zonal"
        call(0x10727890, 0x103b84bc);
        keep(15);
        const auto where = marking_cell(4, false, 0x103b84cc, 0x103b8528);
        c.set(14, bit_);
        c.set(14, mark());
        show(where, mark());
    }
    if (c.u8(at(0x6c)) & 4) {
        // 0x103cad40: the marked player's staff record, or 0.
        c.put32(at(0x138), entry_query(0x103cad40, 0x103b8550));
        if (const auto marked = c.u32(at(0x138))) {
            c.set({{3, marked}, {4, at(0x14d)}, {5, 0xc8}, {6, 6}});
            call(0x100caeb0, 0x103b8578);
            keep(10);
            c.set({{3, text}, {4, pool + 0x698}, {5, at(0x14d)}, {6, pool + 0x22b}});  // "Man (initials)"
            call(0x10727890, 0x103b8590);
        } else {
            c.set({{3, text}, {4, pool + 0x6d1}, {5, pool + 0x22b}});  // "Man"
            call(0x10727890, 0x103b85a4);
            keep(5);
        }
        show(marking_cell(1, false, 0x103b85b4, 0x103b8610), mark());
    }
}

// View 21, Marker: the player who marks this one, or "--".
void Builder::marker_column() {
    // 0x103caf20: the marking player's staff record, or 0.
    c.put32(at(0x134), entry_query(0x103caf20, 0x103b8634));
    if (const auto marker = c.u32(at(0x134))) {
        c.set({{3, marker}, {4, text}, {5, 0x7d0}, {6, 6}});
        call(0x100caeb0, 0x103b865c);
        keep(7);
    } else {
        copy_text(text, pool + 0x6f8, 0x103b8670);
        keep(4);
    }
    if (!c.u32(at(0x134))) {
        cell(0x107445a0, list(), 5, row(), 1, 0, 0xc, 1, {mark(), text}, 0x103b86b0);
        keep(5);
        return;
    }
    opponent_view(0x103b86c0);
    if (flag(0, 0) != 0) {
        cell(0x107444e0, list(), 5, row(), 1, 0, 0xc, 1, {mark(), text, 0x7dc, c.u32(at(0x134))}, 0x103b870c);
        keep(4);
    } else {
        cell(0x107444e0, list(), 5, row(), 1, 0, 0xc, 1, {mark(), text, 0x7d7, c.u32(at(0x134))}, 0x103b8754);
        keep(7);
    }
}

// The role mark at the end of a row (0x103c9260 fills the text): a control
// for the captain or set-piece role, draggable unless the screen shows the
// opponent.
void Builder::role_mark(std::uint32_t player) {
    c.set({{3, screen}, {4, c.r(25)}, {5, text}, {6, 0x7d0}, {7, 1}, {8, 1}, {9, 0}});
    call(0x103c9260, 0x103b8778);
    keep(5);
    c.set({{4, c.u8(text)}, {0, sext8(c.u8(text))}});
    if (c.i8(text) == 0) return;
    const auto colour_of = [&](std::int32_t slot) { return c.u16(c.toc(slot)); };
    const auto role = [&](std::uint32_t kind, std::uint32_t colour_value, std::uint32_t command, std::uint32_t staff,
                          std::uint32_t back) {
        control(kind, 0, 0, 0, 0, 4, row(), {1, 0, 0, 1, 2, colour_value, text, 0, command, staff, list()}, back);
    };
    if (player) {
        // 0x103ca390: whether the player holds a set-piece role.
        entry_query(0x103ca390, 0x103b87a4);
        c.set(13, sext8(c.r(3)));
        if (std::int8_t(c.r(3)) != 0) {
            opponent_view(0x103b87b8);
            if (flag(0, 3) != 0) {
                c.set(6, c.toc(-0x7bd8));
                role(0x202, colour_of(-0x7bd8), 0x7dc, player, 0x103b883c);
            } else {
                c.set(3, c.toc(-0x7bd8));
                role(0x202, colour_of(-0x7bd8), 0x7d7, player, 0x103b88b4);
            }
            return;
        }
        entry_playing(0, 0x103b88cc);
        if (flag(5, 3) != 0) {
            opponent_view(0x103b88e4);
            if (flag(4, 6) != 0) {
                role(0x202, paper(), 0x7dc, player, 0x103b8964);
            } else {
                c.set(15, 0);
                c.put32(at(0x3e0), 0);
                c.put32(at(0x3e4), 2);
                c.set({{14, 0}, {13, 0x7d7}});
                role(0x202, paper(), 0x7d7, player, 0x103b89ec);
            }
            keep(4);
        } else {
            opponent_view(0x103b89fc);
            if (flag(0, 0) != 0) {
                role(0x202, ink(), 0x7dc, player, 0x103b8a78);
                keep(4);
            } else {
                role(0x202, ink(), 0x7d7, player, 0x103b8af0);
                keep(7);
            }
        }
        return;
    }
    entry_query(0x103ca390, 0x103b8b08);
    if (flag(6, 8) != 0) {
        c.set(5, c.toc(-0x7bd8));
        role(1, colour_of(-0x7bd8), 0, 0, 0x103b8b90);
        return;
    }
    entry_playing(0, 0x103b8bac);
    if (flag(0, 0) != 0) {
        role(1, paper(), 0, 0, 0x103b8c2c);
        keep(7);
    } else {
        role(1, ink(), 0, 0, 0x103b8ca8);
        keep(8);
    }
}

namespace {
// The original's signed divisions by constants: multiply-high by a magic
// number, shift, and add the sign bit (division truncating toward zero).
std::int32_t multiply_high(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); }
std::int32_t sign_bit(std::int32_t value) { return std::int32_t(std::uint32_t(value) >> 31); }
std::int32_t third(std::int32_t value) {
    const auto high = multiply_high(0x55555556, value);
    return high + sign_bit(high);
}
[[maybe_unused]] std::int32_t ninth_third(std::int32_t value) {  // / 27
    const auto shifted = multiply_high(0x4bda12f7, value) >> 3;
    return shifted + sign_bit(shifted);
}
[[maybe_unused]] std::int32_t ninth_quarter(std::int32_t value) {  // / 36
    const auto shifted = multiply_high(0x38e38e39, value) >> 3;
    return shifted + sign_bit(shifted);
}
// srawi 2 / addze: a quarter, truncated toward zero.
std::int32_t quarter(std::int32_t value) { return value / 4; }
// rlwinm 1,31,31 / add / srawi 1: a half, truncated toward zero.
[[maybe_unused]] std::int32_t half(std::int32_t value) { return (value + sign_bit(value)) >> 1; }
}

// The pitch: its frame (0x103cbb30), then either the position markers and
// arrows (positions view) or the 3 by 4 cells with the players' spots and
// runs; colours depend on the competition (0x103c6bd0).
void Builder::pitch() {
    stack_args({0, 0xffffffffu});
    c.set({{3, 0x1c2}, {4, 0x91}, {5, 0x30c}, {6, 0x221}, {7, at(0x130)}, {8, at(0x12c)}, {9, at(0x128)},
           {10, at(0x124)}, {12, c.toc(-0x4cac)}});
    c.set_f(1, double(c.f32(c.r(12))));
    call(0x103cbb30, 0x103b8cfc);
    c.set(3, screen);
    c.link(0x103b8d08);
    c.set(3, kind_is_five(c, screen));
    if (flag(13, 11) != 0) {
        c.put16(at(0xa8), colour(panel_colour_slot));
        c.put16(at(0xa6), mark());
    } else {
        c.put16(at(0xa8), colour(panel_colour_slot));
        c.set(10, c.u16(c.r(2) - 0x3804));
        c.set(9, c.r(10));
        c.put16(at(0xa6), c.r(9));
    }
    c.set(3, screen);
    tactics_leaf_103bacf0(c.s);
    if (flag(12, 0) == 1) {
        // The pitch as one control, then each player's position marker and,
        // where his position with the ball differs, an arrow to it.
        const auto ro = read_only();
        flag(9, 7);
        c.put16(at(0x82), shape(ro ? 1 : 6, pitch_left(), pitch_top(), pitch_right(), pitch_bottom(),
                                {1, 0, 0, 0, 0, 0, 0, 0, 0x1c, 0, 0xffffffffu}, ro ? 0x103b8e74 : 0x103b8dec));
        position_arrows();
        position_markers();
    } else {
        cells();
        runs();
        cell_frames();
        spots();
    }
}

// The positions view's arrows: from each player's position without the ball
// to his position with it.
void Builder::position_arrows() {
    for (c.set(25, 0);; c.set(25, c.r(25) + 1)) {
        c.set(0, entry());
        if (std::int8_t(c.r(25)) >= 11) break;
        // 0x103ca730: the formation slot shown in this place (-1 none; its
        // side at sp+0x69).
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x69)}, {6, 0}});
        c.put8(at(0x6b), call(0x103ca730, 0x103b8e98));
        c.set({{9, c.r(3)}, {7, sext8(c.u8(at(0x6b)))}});
        if (c.i8(at(0x6b)) <= -1) continue;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x68)}});
        c.link(0x103b8ec0);
        c.set(3, displaced(c, screen, c.r(25), at(0x68)));
        if (flag(0, 8) == 0) continue;
        const auto position_of = [&](std::uint32_t with_ball, std::uint32_t back) {
            c.set({{3, screen}, {4, c.r(25)}, {5, with_ball}});
            c.link(back);
            return position(c, screen, c.r(25), with_ball);
        };
        c.put16(at(0x8e), position_of(0, 0x103b8ee0));
        c.put16(at(0x8c), position_of(1, 0x103b8ef8));
        c.set(15, slot16(0x8e));
        if (c.i16(at(0x8e)) == c.i16(at(0x8c))) continue;
        // The position's point on the pitch (0x103ce330), central players
        // (0x103bab40) placed differently.
        const auto central = [&](std::int32_t slot, std::uint32_t back) {
            c.set({{3, screen}, {4, slot16(slot)}});
            c.link(back);
            return central_in_line(c, screen, slot16(slot));
        };
        const auto point = [&](std::int32_t slot, std::int32_t x, std::int32_t y, std::uint32_t central_flag,
                               std::uint32_t back) {
            c.set({{3, slot16(slot)}, {4, c.u32(at(0x130))}, {5, c.u32(at(0x12c))},
                   {6, c.u32(at(0x128))}, {7, c.u32(at(0x124))}, {8, at(x)}, {9, at(y)}, {10, central_flag}});
            return call(0x103ce330, back);
        };
        c.set(10, central(0x8e, 0x103b8f20));
        point(0x8e, 0x120, 0x11c, c.r(10), 0x103b8f44);
        const auto split = [&](std::int32_t slot, std::int32_t line, std::int32_t side, std::uint32_t back) {
            // 0x10116780 (FORMATION.CPP): a position's line and side bits.
            c.set({{3, slot16(slot)}, {4, at(line)}, {5, at(side)}});
            return call(0x10116780, back);
        };
        c.set(3, split(0x8e, 0x8a, 0x88, 0x103b8f58));
        c.set(3, split(0x8c, 0x86, 0x84, 0x103b8f6c));
        c.set({{12, slot16(0x88)}, {5, slot16(0x88)}, {0, slot16(0x84)}});
        if (c.i16(at(0x88)) != c.i16(at(0x84))) {
            c.set(10, central(0x8c, 0x103b8f90));
            point(0x8c, 0x118, 0x114, c.r(10), 0x103b8fb4);
        } else {
            c.set(11, central(0x8e, 0x103b8fc8));
            point(0x8c, 0x118, 0x114, c.r(11), 0x103b8ff0);
        }
        // When another player stands at the target, the arrow ends short of it.
        c.set({{3, screen}, {4, slot16(0x8c)}});
        c.link(0x103b9000);
        c.set(3, player_at(c, screen, slot16(0x8c)));
        if (flag(6, 7) > -1) {
            const auto x0 = [&] { return local(0x120); };
            const auto y0 = [&] { return local(0x11c); };
            const auto x1 = [&] { return local(0x118); };
            const auto y1 = [&] { return local(0x114); };
            bool done = false;
            if (y1() < y0()) {
                c.set(14, std::uint32_t(absolute(x1() - x0(), 0x103b9030)));
                const auto dy = absolute(y1() - y0(), 0x103b9044);
                if (dy > std::int32_t(c.r(14))) {
                    c.put32(at(0x114), std::uint32_t(y1() + 0x10));
                    done = true;
                }
            }
            if (!done) {
                c.set(19, std::uint32_t(y0()));
                if (y1() > y0()) {
                    c.set(15, std::uint32_t(absolute(x1() - x0(), 0x103b9080)));
                    c.set(14, std::uint32_t(y1()));
                    const auto dy = absolute(y1() - y0(), 0x103b9094);
                    if (dy > std::int32_t(c.r(15))) {
                        c.put32(at(0x114), std::uint32_t(y1() - 0x12));
                        done = true;
                    }
                }
            }
            if (!done) {
                if (x1() < x0()) {
                    c.set(14, std::uint32_t(x1() + 0xd));
                    c.put32(at(0x118), c.r(14));
                } else if (x1() > x0()) {
                    c.put32(at(0x118), std::uint32_t(x1() - 0xd));
                }
            }
        }
        // The arrow's colour by the side shown (sp+0x69), its line and head.
        c.put16(at(0xaa), c.i8(at(0x69)) == 0 ? c.u16(c.r(2) - 0x3808) : c.u16(c.r(2) - 0x381c));
        shape(1, local(0x120), local(0x11c), local(0x118), local(0x114),
              {0x2400, c.u16(at(0xaa)), 0, 0, 0, 0, 0, 0, 0, 0, 0xffffffffu}, 0x103b9194);
        keep(5);
        // 0x103ce790: the arrow head's two points (sp+0x110/0x10c and +0x108/0x104).
        stack_args({at(0x10c), at(0x108), at(0x104)});
        c.set({{3, c.u32(at(0x120))}, {4, c.u32(at(0x11c))}, {5, c.u32(at(0x118))}, {6, c.u32(at(0x114))},
               {7, c.toc(-0x4cb0)}, {9, 8}, {10, at(0x110)}});
        c.set_f(1, c.f64(c.r(7)));
        call(0x103ce790, 0x103b91d4);
        shape(1, local(0x110), local(0x10c), local(0x118), local(0x114),
              {0x400, c.u16(at(0xaa)), 0, 0, 0, 0, 0, 0, 0, 0, 0xffffffffu}, 0x103b9254);
        c.set(13, 0);
        shape(1, local(0x108), local(0x104), local(0x118), local(0x114),
              {0x400, c.u16(at(0xaa)), 0, 0, 0, 0, 0, 0, 0, 0, 0xffffffffu}, 0x103b92d0);
    }
}

// The positions view's markers: each player's position without the ball,
// his name, a drag handle, his run mark and his role.
void Builder::position_markers() {
    for (c.set(25, 0);; c.set(25, c.r(25) + 1)) {
        c.set(0, entry());
        if (std::int8_t(c.r(25)) >= 11) break;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x69)}, {6, 0}});
        c.put8(at(0x6b), call(0x103ca730, 0x103b92fc));
        c.set(6, c.r(3));
        c.set(5, sext8(c.u8(at(0x6b))));
        if (c.i8(at(0x6b)) <= -1) continue;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x68)}});
        c.link(0x103b9324);
        c.set(3, displaced(c, screen, c.r(25), at(0x68)));
        if (flag(0, 3) == 0) continue;
        c.set({{3, screen}, {4, c.r(25)}, {5, 0}});
        c.link(0x103b9340);
        c.put16(at(0x8e), position(c, screen, c.r(25), 0));
        c.set({{3, screen}, {4, slot16(0x8e)}});
        c.link(0x103b9354);
        c.set(10, central_in_line(c, screen, slot16(0x8e)));
        c.set({{3, slot16(0x8e)}, {4, c.u32(at(0x130))}, {5, c.u32(at(0x12c))}, {6, c.u32(at(0x128))},
               {7, c.u32(at(0x124))}, {8, at(0x120)}, {9, at(0x11c)}});
        call(0x103ce330, 0x103b9378);
        keep(4);
        marker_colours(0x78, 0x76, true);
        c.set({{3, screen}, {4, c.u8(at(0x6b))}, {5, text}, {6, 0x7d0}, {7, 0}, {8, 0}});
        call(0x103c8e70, 0x103b942c);
        const auto x = local(0x120), y = local(0x11c);
        c.set({{4, c.u8(at(0x68))}, {6, sext8(c.u8(at(0x68)))}});
        if (c.i8(at(0x68)) != 0) {
            // A label for a player sharing his spot.
            c.set(15, 0x10030);
            c.put32(at(0x3e0), c.u16(at(0xa2)));
            c.put32(at(0x3e4), 0);
            c.set({{14, c.u16(at(0xa4))}, {13, pool + 0x22b}});
            shape(1, x - 0x11, y - 0x16, x + 9, y + 4,
                  {0x10030, c.u32(at(0x3e0)), c.u16(at(0xa2)), 0, 1, c.u16(at(0xa4)), pool + 0x22b, 0, 0, 0,
                   0xffffffffu},
                  0x103b94dc);
        }
        const auto colour_a2 = c.u16(at(0xa2)), colour_a4 = c.u16(at(0xa4));
        if (read_only() == 0) {
            flag(0, 3);
            shape(0x246, x - 0xd, y - 0x12, x + 0xd, y + 8,
                  {0x10030, colour_a2, colour_a2, 0, 1, colour_a4, text, 0, 0x1d, entry(), 0xffffffffu}, 0x103b9580);
            shape(0x346, x - 0xd, y - 0x12, x + 0xd, y + 8,
                  {0x10020, colour_a2, colour_a2, 0, 1, colour_a4, 0, 0, 0x1e, entry(), 0xffffffffu}, 0x103b9614);
        } else {
            flag(0, 3);
            shape(1, x - 0xd, y - 0x12, x + 0xd, y + 8,
                  {0x10030, colour_a2, colour_a2, 0, 1, colour_a4, text, 0, 0x1d, entry(), 0xffffffffu}, 0x103b96a8);
        }
        keep(9);
        // 0x103c8b60: whether the player's spots are set by hand: a ring.
        c.set({{3, screen}, {4, c.r(25)}});
        call(0x103c8b60, 0x103b96b8);
        if (flag(0, 7) != 0) {
            c.set(13, 0);
            shape(1, x - 0xe, y - 0x13, x + 0xe, y + 9, {0x10020, 0, 0, 0, 1, c.u16(at(0xa4)), 0, 0, 0, 0, 0xffffffffu},
                  0x103b9758);
        }
        // 0x103c9260 (screens_instructions): the role text under the marker.
        c.set({{3, screen}, {4, c.u8(at(0x6b))}, {5, text}, {6, 0x7d0}, {7, 1}, {8, 5}, {9, 0}});
        call(0x103c9260, 0x103b977c);
        keep(11);
        shape(1, x - 0x1e, y + 6, x + 0x1e, y + 0x10, {1, 0, 0, 0xc, 1, c.u16(at(0xa0)), text, 0, 0, 0, 0xffffffffu},
              0x103b9808);
        keep(6);
    }
}

// A marker's colours (sp+0xa4 fill, sp+0xa2 outline, sp+0xa0 label): the
// competition's for a goalkeeper (position bit 1), the club's kit
// (sp+0xae / sp+0xac, from the title) for the others, or the opponent's
// colours (statics) when the side shown is the opponent. The original
// passes them through the temporaries at sp+first and sp+second.
void Builder::marker_colours(std::int32_t first, std::int32_t second, bool label) {
    c.set({{7, c.u8(at(0x69))}, {6, sext8(c.u8(at(0x69)))}});
    if (c.i8(at(0x69)) == 0) {
        const bool keeper = c.i16(at(0x8e)) & 1;
        c.put16(at(first), c.u16(at(keeper ? 0xa8 : 0xae)));
        c.put16(at(0xa4), c.u16(at(first)));
        c.put16(at(second), c.u16(at(keeper ? 0xa6 : 0xac)));
        c.put16(at(0xa2), c.u16(at(second)));
        if (label) c.put16(at(0xa0), mark());
    } else {
        c.put16(at(0xa4), c.u16(c.r(2) - 0x381c));
        c.put16(at(0xa2), c.u16(c.r(2) - 0x3820));
        if (label) c.put16(at(0xa0), c.u16(at(0xa4)));
    }
}

// The 3 by 4 cells of the pitch, each a control whose value is its column
// plus 100 times its row; the original keeps the row and column at sp+0x100
// and sp+0xfc.
void Builder::cells() {
    for (c.put32(at(0x100), 0); local(0x100) < 3; c.put32(at(0x100), std::uint32_t(local(0x100) + 1))) {
        for (c.put32(at(0xfc), 0); local(0xfc) < 4; c.put32(at(0xfc), std::uint32_t(local(0xfc) + 1))) {
            c.put16(at(0x80), 0xffff);
            const auto row = local(0x100), column = local(0xfc);
            const auto width = pitch_right() - pitch_left(), height = pitch_bottom() - pitch_top();
            const auto colour_value = c.u16(c.r(2) - 0x3810);
            const auto value = std::uint32_t(column + row * 0x64);
            const auto left = pitch_left() + third(row * width);
            const auto top = pitch_bottom() - quarter((column + 1) * height);
            const auto right = pitch_left() + third((row + 1) * width);
            const auto bottom = pitch_bottom() - quarter(column * height);
            if (read_only() == 0) {
                flag(6, 9);
                // The original leaves its arithmetic in r14, r15 and r19.
                const auto product = (row + 1) * width;
                const auto high = multiply_high(0x55555556, product);
                c.set({{19, std::uint32_t(quarter((column + 1) * height))}, {15, std::uint32_t(high)},
                       {14, std::uint32_t(sign_bit(high))}});
                c.put16(at(0x80), shape(0x306, left, top, right, bottom,
                                        {0x2220, colour_value, colour_value, 0, 0, 0, 0, 0, 0x1f, value, 0xffffffffu},
                                        0x103b9988));
            } else {
                flag(6, 9);
                c.put16(at(0x80), shape(1, left, top, right, bottom,
                                        {0x2220, colour_value, colour_value, 0, 0, 0, 0, 0, 0x1f, value, 0xffffffffu},
                                        0x103b9ac0));
            }
        }
    }
}

// With "Show Runs" (screen +0x27ae): an arrow per player from his spot in
// the ball's cell (0x103c78a0) to his spot in the shown cell (0x103c79d0).
// Spots are 27 by 36 steps of the pitch rectangle.
void Builder::runs() {
    c.set({{3, screen}, {4, at(0xc4)}, {5, 0}});
    c.link(0x103b9b04);
    c.set(3, copy_cell(c, screen, at(0xc4), 0));
    c.set(3, screen);
    tactics_leaf_103bad00(c.s);
    if (flag(0, 5) == 0) return;
    for (c.set(25, 0);; c.set(25, c.r(25) + 1)) {
        c.set(0, entry());
        if (std::int8_t(c.r(25)) >= 11) break;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x69)}, {6, 0}});
        c.put8(at(0x6b), call(0x103ca730, 0x103b9b34));
        c.set(4, sext8(c.u8(at(0x6b))));
        if (c.i8(at(0x6b)) <= -1) continue;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x68)}});
        c.link(0x103b9b58);
        c.set(3, shared_spot(c, screen, c.r(25), at(0x68)));
        if (flag(0, 3) == 0) continue;
        // A spot's point: x from the left, y up from the bottom; the original
        // keeps the ends at sp+0xf8/0xec..0xf0/0xf4 and sp+0xe8/0xdc..0xe0/0xe4.
        const auto spot_point = [&](std::int32_t x_slot, std::int32_t y_slot, std::int32_t x2_slot,
                                    std::int32_t y2_slot) {
            const auto width = pitch_right() - pitch_left(), height = pitch_bottom() - pitch_top();
            c.put32(at(x_slot), std::uint32_t(pitch_left() + ninth_third(std::int32_t(c.u16(at(0xc8))) * width)));
            c.put32(at(y_slot),
                    std::uint32_t(pitch_bottom() - ninth_quarter(std::int32_t(c.u16(at(0xca))) * height)));
            c.put32(at(x2_slot), std::uint32_t(local(x_slot) + ninth_third(width)));
            c.put32(at(y2_slot), std::uint32_t(local(y_slot) - ninth_third(width)));
        };
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0xc8)}});
        call(0x103c78a0, 0x103b9b74);
        keep(4);
        spot_point(0xf8, 0xec, 0xf0, 0xf4);
        c.set(13, std::uint32_t(pitch_right() - pitch_left()));
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0xc8)}});
        call(0x103c79d0, 0x103b9c58);
        keep(6);
        {
            // The original computes the second point's x through sp+0x3e0/0x3e4.
            const auto width = pitch_right() - pitch_left();
            const auto high = multiply_high(0x4bda12f7, std::int32_t(c.u16(at(0xc8))) * width);
            c.put32(at(0x3e0), std::uint32_t(high));
            c.put32(at(0x3e4), std::uint32_t(sign_bit(high >> 3)));
            c.set({{15, std::uint32_t(pitch_left())}, {14, c.u16(at(0xc8))}, {13, std::uint32_t(pitch_left())}});
        }
        spot_point(0xe8, 0xdc, 0xe0, 0xe4);
        c.set({{5, c.u8(at(0x69))}, {6, sext8(c.u8(at(0x69)))}});
        c.put16(at(0xaa), c.i8(at(0x69)) == 0 ? c.u16(c.r(2) - 0x380c) : c.u16(c.r(2) - 0x381c));
        const auto centre = [&](std::int32_t from, std::int32_t to) { return local(from) + half(local(to) - local(from)); };
        c.set(13, c.u32(at(0xf4)));
        shape(1, centre(0xf8, 0xf0), centre(0xf4, 0xec), centre(0xe8, 0xe0), centre(0xe4, 0xdc),
              {0x2400, c.u16(at(0xaa)), 0, 0, 0, 0, 0, 0, 0, 0, 0xffffffffu}, 0x103b9e54);
        keep(10);
    }
    // The ball's cell framed when it is not the shown one.
    c.set({{3, screen}, {4, at(0xc0)}, {5, 1}});
    c.link(0x103b9e78);
    c.set(3, copy_cell(c, screen, at(0xc0), 1));
    if (c.u16(at(0xc0)) == c.u16(at(0xc4)) && c.u16(at(0xc2)) == c.u16(at(0xc6))) return;
    const auto colour_value = c.u16(c.r(2) - 0x3818);
    const auto width = pitch_right() - pitch_left(), height = pitch_bottom() - pitch_top();
    const std::int32_t row = c.u16(at(0xc0)), column = c.u16(at(0xc2));
    const auto bottom_edge = pitch_bottom() - quarter(column * height);
    c.set({{14, std::uint32_t(bottom_edge)}, {15, std::uint32_t((row + 1) * width)}, {19, std::uint32_t(pitch_left())}});
    shape(1, pitch_left() + third(row * width) - 1, pitch_bottom() - quarter((column + 1) * height) - 1,
          pitch_left() + third((row + 1) * width) + 1, bottom_edge + 1,
          {0x2220, colour_value, colour_value, 0, 0, 0, 0, 0, 0, 0, 0xffffffffu}, 0x103b9fd4);
}

// The shown cell framed, then the pitch as one control (sp+0x82).
void Builder::cell_frames() {
    const auto colour_value = c.u16(c.r(2) - 0x3814);
    const auto width = pitch_right() - pitch_left(), height = pitch_bottom() - pitch_top();
    const std::int32_t row = c.u16(at(0xc4)), column = c.u16(at(0xc6));
    shape(1, pitch_left() + third(row * width) - 1, pitch_bottom() - quarter((column + 1) * height) - 1,
          pitch_left() + third((row + 1) * width) + 1, pitch_bottom() - quarter(column * height) + 1,
          {0x2220, colour_value, colour_value, 0, 0, 0, 0, 0, 0, 0, 0xffffffffu}, 0x103ba10c);
    if (read_only() == 0) {
        flag(4, 0);
        c.put16(at(0x82), shape(6, pitch_left(), pitch_top(), pitch_right(), pitch_bottom(),
                                {1, 0, 0, 0, 0, 0, 0, 0, 0x1c, 0, 0xffffffffu}, 0x103ba1a4));
        keep(4);
    } else {
        flag(4, 0);
        c.set(13, 0x1c);
        c.put16(at(0x82), shape(1, pitch_left(), pitch_top(), pitch_right(), pitch_bottom(),
                                {1, 0, 0, 0, 0, 0, 0, 0, 0x1c, 0, 0xffffffffu}, 0x103ba22c));
        keep(5);
    }
}

// Each player's marker at his spot in the shown cell: label, drag handle
// and the ring for spots set by hand.
void Builder::spots() {
    for (c.set(25, 0);; c.set(25, c.r(25) + 1)) {
        c.set(4, entry());
        if (std::int8_t(c.r(25)) >= 11) break;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x69)}, {6, 0}});
        c.put8(at(0x6b), call(0x103ca730, 0x103ba250));
        c.set({{6, c.r(3)}, {4, c.u8(at(0x6b))}, {3, sext8(c.u8(at(0x6b)))}});
        if (c.i8(at(0x6b)) <= -1) continue;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0x68)}});
        c.link(0x103ba278);
        c.set(3, shared_spot(c, screen, c.r(25), at(0x68)));
        if (flag(0, 6) == 0) continue;
        c.set({{3, screen}, {4, c.r(25)}, {5, at(0xc8)}});
        call(0x103c79d0, 0x103ba298);
        const auto width = pitch_right() - pitch_left(), height = pitch_bottom() - pitch_top();
        c.put32(at(0xd8), std::uint32_t(pitch_left() + ninth_third(std::int32_t(c.u16(at(0xc8))) * width)));
        c.put32(at(0xcc), std::uint32_t(pitch_bottom() - ninth_quarter(std::int32_t(c.u16(at(0xca))) * height)));
        {
            // The original computes the right end through sp+0x3e4/0x3e0 and r13-r15.
            c.put32(at(0x3e4), std::uint32_t(pitch_right()));
            const auto shifted = multiply_high(0x4bda12f7, width) >> 3;
            c.set({{14, std::uint32_t(shifted)}, {15, std::uint32_t(sign_bit(shifted))}});
            c.put32(at(0x3e0), std::uint32_t(shifted + sign_bit(shifted)));
            c.set(13, std::uint32_t(local(0xd8) + local(0x3e0)));
            c.put32(at(0xd0), c.r(13));
        }
        c.put32(at(0xd4), std::uint32_t(local(0xcc) - ninth_third(width)));
        c.set({{3, screen}, {4, c.r(25)}, {5, 0}});
        c.link(0x103ba38c);
        c.put16(at(0x8e), position(c, screen, c.r(25), 0));
        marker_colours(0x74, 0x72, false);
        c.set({{3, screen}, {4, c.u8(at(0x6b))}, {5, text}, {6, 0x7d0}, {7, 0}, {8, 0}});
        call(0x103c8e70, 0x103ba430);
        keep(4);
        const auto x = local(0xd8) + half(local(0xd0) - local(0xd8));
        const auto y = local(0xd4) + half(local(0xcc) - local(0xd4));
        c.set({{5, c.u8(at(0x68))}, {6, sext8(c.u8(at(0x68)))}});
        if (c.i8(at(0x68)) != 0) {
            c.set(13, std::uint32_t(half(local(0xd0) - local(0xd8))));
            shape(1, x - 0x11, y - 0x11, x + 9, y + 9,
                  {0x10030, c.u16(at(0xa2)), c.u16(at(0xa2)), 0, 1, c.u16(at(0xa4)), pool + 0x22b, 0, 0, 0,
                   0xffffffffu},
                  0x103ba544);
        }
        const auto colour_a2 = c.u16(at(0xa2)), colour_a4 = c.u16(at(0xa4));
        if (read_only() == 0) {
            flag(9, 7);
            c.set(15, c.u32(at(0xd4)));
            shape(0x246, x - 0xd, y - 0xd, x + 0xd, y + 0xd,
                  {0x10030, colour_a2, colour_a2, 0, 1, colour_a4, text, 0, 0x1d, entry(), 0xffffffffu}, 0x103ba65c);
        } else {
            flag(9, 7);
            const auto rise = local(0xcc) - local(0xd4);
            c.set({{19, c.u32(at(0xcc))}, {15, std::uint32_t(rise + sign_bit(rise))}, {14, c.u32(at(0xd4))}});
            shape(1, x - 0xd, y - 0xd, x + 0xd, y + 0xd,
                  {0x10030, colour_a2, colour_a2, 0, 1, colour_a4, text, 0, 0x1d, entry(), 0xffffffffu}, 0x103ba760);
            keep(9);
        }
        c.set({{3, screen}, {4, c.r(25)}});
        call(0x103c8b60, 0x103ba770);
        if (flag(10, 3) != 0) {
            shape(1, x - 0xe, y - 0xe, x + 0xe, y + 0xe, {0x10020, 0, 0, 0, 1, c.u16(at(0xa4)), 0, 0, 0, 0, 0xffffffffu},
                  0x103ba880);
            keep(8);
        }
    }
}

// Cancel and Ok for an editable screen; a read-only one asks the game
// (0x1074b0a0) for its own buttons.
void Builder::buttons() {
    if (read_only() == 0) {
        flag(0, 3);
        c.put16(at(0x9e), box(0x64, 0x22b, 0x316, 0x24e, 2, at(0x90), 1, {0, 1, 0, 0xffffffffu}, 0x103ba8f0));
        localise(0x6fb, 0x103ba904);  // "Cancel"
        cell(0x107444e0, slot16(0x9e), 0, 0, 0x30, ink(), 0xc, 3, {paper(), text, 0x20, 0}, 0x103ba944);
        localise(0x702, 0x103ba950);  // "Ok"
        keep(4);
        cell(0x107444e0, slot16(0x9e), 1, 0, 0x30, ink(), 0xc, 3, {paper(), text, 0x21, 0}, 0x103ba994);
        keep(7);
        return;
    }
    flag(0, 3);
    const auto negate = [](std::uint32_t value) { return std::uint32_t(std::int8_t(value) == 0); };
    c.set(3, game);
    call(0x1073ea20, 0x103ba9a4);
    c.set({{5, c.r(3)}, {4, sext8(c.r(3))}});
    c.set(13, negate(c.r(5)));
    c.set({{3, game}, {4, 0}});
    call(0x1073eb00, 0x103ba9c4);
    const auto edited = negate(c.r(3));
    c.set({{6, std::uint32_t(std::countl_zero(sext8(c.r(3))))}, {5, edited}, {3, edited}, {4, c.r(13)}});
    call(0x1074b0a0, 0x103ba9e4);
}

void Builder::build() {
    statics();
    open_state();
    caption();
    read_only();
    if (flag(7, 8) == 0) {
        tactics_menu();
        view_menu();
        edit_menu();
    } else {
        // Read-only: no menus, only the "Team Instructions" heading.
        localise(0x2f, 0x103b5358);
        c.set(13, 1);
        stack_args({1, paper(), text, 1, 0});
        c.set({{3, game}, {4, 0x6e}, {5, 0x50}, {6, 0xeb}, {7, 0x64}, {8, 0x30}, {9, ink()}, {10, 0xc}});
        c.call(0x10744430, 0x103b53a4);
    }
    headings();
    squad();
    pitch();
    buttons();
}
}

void build_screen(Cpu& c) {
    Frame frame(c, 0x103b3c50, 0x440, {.first_gpr = 13});
    Builder(c, frame).build();
}
}
