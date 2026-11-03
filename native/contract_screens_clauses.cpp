#include "contract_screens.hpp"
#include "contract_screens_controls.hpp"
#include "contract_screens_money.hpp"
#include <array>

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;
using detail::MoneySites;
using detail::money_text;

constexpr auto history = 0x1073ef70u, grid_control = 0x107198c0u, localized = 0x10727890u,
               button = 0x107444e0u, label = 0x107445a0u;
constexpr auto redraw = std::uint32_t(-11);
constexpr std::int32_t maximum_clause = 50'000'000;

struct ClauseCommand {
    Cpu& c;
    const Frame& frame;
    Program program;
    const detail::Controls& controls;
    std::uint32_t screens, draft, proposal, text;

    std::uint32_t read(std::uint32_t back, unsigned slot) const {
        return program.call(history, back, {screens, slot});
    }

    std::uint32_t value() const {
        const auto value = controls.value();
        c.set(0, value);
        return value;
    }

    bool has_minimum() const {
        const auto flag = c.u8(draft + 0x1d);
        c.set(0, flag);
        return compare(c, flag, 1) == 0;
    }

    void enable() const {
        const auto clause = std::int8_t(value());
        c.set(25, std::uint32_t(clause));
        if (compare(c, clause, 1) == 0) {
            const auto min = read(0x1009d5bc, 0x17), amount = c.u32(draft + 0x24);
            c.set(0, amount);
            if (compare(c, amount, min) < 0) c.put32(draft + 0x24, min);
        }
        program.call(0x10374af0, 0x1009d5d8, {draft, std::uint32_t(clause)});
    }

    void reset_amount() const {
        const auto amount = c.u32(proposal + 0x24);
        c.put32(draft + 0x24, amount);
        if (!has_minimum()) return;
        const auto min = read(0x1009da68, 0x17), current = c.u32(draft + 0x24);
        c.set(0, current);
        if (compare(c, current, min) < 0) c.put32(draft + 0x24, read(0x1009da80, 0x17));
    }

    void configure(std::uint32_t widget, bool enabled, unsigned action, std::uint32_t back) const {
        program.call(0x10740620, back, {screens, widget, enabled ? 2u : 6u});
        program.call(0x10741450, back + 0x10, {screens, widget, enabled ? 12u : 44u});
        program.call(0x10742100, back + 0x20, {screens, widget, enabled ? action : 0u});
        program.call(0x10742420, back + 0x30, {screens, widget, 0});
    }

    void adjust(bool increase) const {
        const auto local = frame.at(increase ? 0x38 : 0x3c), before = c.u32(draft + 0x24);
        c.set(0, before);
        c.put32(local, before);
        program.call(increase ? 0x1000a060 : 0x1000a170, increase ? 0x1009d894 : 0x1009d6b4, {local, 4});
        const auto amount = c.u32(local);
        c.set(0, amount);
        c.put32(draft + 0x24, amount);
        const auto widget = std::int16_t(read(increase ? 0x1009d8a8 : 0x1009d6c8, 0xe));
        c.set(25, std::uint32_t(widget));

        if (increase) {
            configure(widget, true, 0x28, 0x1009d8bc);
            money_text(c, program, text, local, {0x1009d8f8, 0x1009d910, 0x1009d918, 0x1009d92c, 0x1009d944}, 6);
            program.call(0x10741da0, 0x1009d954, {screens, std::uint32_t(widget + 1), text});
            const auto next = c.u32(local);
            c.set({{0, maximum_clause}, {3, 0x02fb0000}, {4, next}});
            const bool enabled = compare(c, next, maximum_clause) < 0;
            configure(widget + 2, enabled, 0x27, enabled ? 0x1009d9bc : 0x1009d978);
            program.call(0x10742ec0, 0x1009d9fc, {screens, 0, 0});
        } else {
            const auto min = has_minimum() ? read(0x1009d6e4, 0x17) : 5u;
            const auto next = c.u32(local);
            c.set({{0, next}, {3, min}});
            const bool enabled = compare(c, next, min) > 0;
            configure(widget, enabled, 0x28, enabled ? 0x1009d74c : 0x1009d708);
            money_text(c, program, text, local, {0x1009d788, 0x1009d7a0, 0x1009d7a8, 0x1009d7bc, 0x1009d7d4}, 6);
            program.call(0x10741da0, 0x1009d7e4, {screens, std::uint32_t(widget + 1), text});
            configure(widget + 2, true, 0x27, 0x1009d7f4);
            program.call(0x10742ec0, 0x1009d834, {screens, 0, 0});
        }
    }
};

struct ClauseScreen {
    Cpu& c;
    const Frame& frame;
    Program program;
    std::uint32_t screens, text, strings, ink, muted, accent, secondary, draft, proposal, person;
    std::uint32_t flags_grid = 0, amount_grid = 0, presets_grid = 0xffffffff;
    std::int32_t minimum = 0;

    std::uint32_t row() const { return c.u32(frame.at(0x78)); }

    void advance(unsigned count) const {
        const auto previous = row();
        c.set({{0, previous + count}, {3, previous}});
        c.put32(frame.at(0x78), previous + count);
    }

    void words(std::initializer_list<std::uint32_t> values) const {
        auto at = frame.at(0x38);
        for (auto value : values) {
            c.put32(at, value);
            at += 4;
        }
    }

    std::uint32_t layout(bool indexed = false) const {
        const auto offset = std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
        c.set(0, offset + (indexed ? 0x24 : 0));
        return c.u32(screens + offset + 0x24);
    }

    void caption(std::uint32_t back, unsigned offset) const {
        program.call(localized, back, {text, strings + offset});
    }

    void heading(unsigned name, unsigned color, std::uint32_t title_back,
                 std::uint32_t assign_back, std::uint32_t draw_back) const {
        const auto box = frame.at(0x7c);
        caption(title_back, name);
        program.call(0x1074b830, assign_back, {box, text});
        c.set(0, 0);
        words({0});
        program.call(0x1074bcd0, draw_back, {box, flags_grid, 120, 770, frame.at(0x78), 3, color, 0});
    }

    std::uint32_t cell(std::uint32_t back, std::uint32_t grid, unsigned column, unsigned type,
                       std::uint32_t decoration, unsigned style, unsigned color, std::uint32_t text,
                       bool clickable = false, unsigned action = 0, unsigned value = 0) const {
        if (clickable) words({color, text, action, value});
        else words({color, text});
        return program.call(clickable ? button : label, back, {screens, grid, column, row(), type, decoration, style, 3});
    }

    bool draw_flags(bool optional_group) const;
    void draw_amount(bool editable);
    void draw_presets() const;
};

bool ClauseScreen::draw_flags(bool optional_group) const {
    bool active = false;
    for (unsigned index = 0;; ++index) {
        c.set({{0, index}, {19, index}});
        if (compare(c, index, 5) >= 0) break;
        if ((compare(c, index, 4) == 0) != optional_group) continue;
        const auto club = c.u32(draft + 4);
        c.set(0, club);
        if (compare(c, club, -1) == 0) continue;
        const auto offset = club * 0x248;
        c.set(0, offset);
        const auto supported = std::int8_t(program.call(0x10380400, optional_group ? 0x1009d0ec : 0x1009cac4,
            {person, c.u32(c.toc(-0x7be8)) + offset, index}));
        c.set(0, std::uint32_t(supported));
        if (compare(c, supported) == 0) continue;
        c.set(optional_group ? 13 : 21, index);
        const auto proposed = c.i8(proposal + 0x1c + index);
        c.set(0, std::uint32_t(proposed));
        if (compare(c, proposed) != 0) {
            const auto eligible = std::int8_t(c.r(31));
            c.set(0, std::uint32_t(eligible));
            if (compare(c, eligible) != 0) {
                c.set(0, strings + 0x120b);
                cell(optional_group ? 0x1009d144 : 0x1009cb1c, flags_grid, 1, 1, 0, 12,
                     c.u16(accent), strings + 0x120b);
            }
        }
        const auto enabled = c.i8(draft + 0x1c + index);
        c.set(0, std::uint32_t(enabled));
        const bool selected = compare(c, enabled, 1) == 0;
        program.call(0x10380300,
            optional_group ? (selected ? 0x1009d164 : 0x1009d1b4) : (selected ? 0x1009cb3c : 0x1009cb90),
            {index, text});
        c.set(0, index);
        cell(optional_group ? (selected ? 0x1009d1a4 : 0x1009d1f4) : (selected ? 0x1009cb7c : 0x1009cbd0),
             flags_grid, 2, 1, 0, 1, c.u16(selected ? ink : muted), text, true, selected ? 0x13 : 0x12, index);
        if (!optional_group && selected) {
            c.set(13, 1);
            active = true;
        }
        advance(1);
    }
    return active;
}

void ClauseScreen::draw_amount(bool editable) {
    const auto stored = frame.at(editable ? 0x6c : 0x68);
    if (editable) {
        const auto amount = c.u32(draft + 0x24);
        c.set(0, amount);
        c.put32(stored, amount);
    }
    caption(editable ? 0x1009cc14 : 0x1009ceec, 0x1251);
    const auto name_ink = c.u16(editable ? ink : muted);
    c.set(0, name_ink);
    cell(editable ? 0x1009cc44 : 0x1009cf1c, flags_grid, 2, 1, 0, 1, name_ink, text);
    const auto action_ink = c.u16(c.toc(-0x7bdc));

    if (editable) {
        const auto clause = c.u8(draft + 0x1d);
        c.set(0, clause);
        minimum = compare(c, clause, 1) == 0 ? std::int32_t(program.call(history, 0x1009cc5c, {screens, 0x17})) : 5;
        const auto amount = c.u32(stored);
        c.set({{0, amount}, {3, std::uint32_t(minimum)}, {18, std::uint32_t(minimum)}});
        const bool can_decrease = compare(c, amount, minimum) > 0;
        c.set(0, can_decrease ? 0u : strings + 0xe1d);
        const auto lower = cell(can_decrease ? 0x1009ccfc : 0x1009ccac, amount_grid, 3, 0x30, action_ink,
                                can_decrease ? 12 : 44, c.u16(ink), strings + 0xe1d, can_decrease, 0x28);
        c.set(0, lower);
        program.call(0x1073f1e0, 0x1009cd14, {screens, 0xe, std::uint32_t(std::int16_t(lower)), 0});
        money_text(c, program, text, stored, {0x1009cd20, 0x1009cd38, 0x1009cd40, 0x1009cd54, 0x1009cd6c}, 6);
        c.set(0, 0);
        const auto text_widget = cell(0x1009cda8, amount_grid, 4, 1, 0, 12, c.u16(ink), text, true);
        words({0, 0x130, c.u16(secondary), std::uint32_t(std::int16_t(text_widget))});
        presets_grid = program.call(grid_control, 0x1009cdf8, {layout(), 0, 0, 0, 0, 0, 0, 0});
        const auto next = c.u32(stored);
        c.set({{0, maximum_clause}, {4, 0x02fb0000}, {5, next}, {20, presets_grid}});
        const bool can_increase = compare(c, next, maximum_clause) < 0;
        c.set(0, can_increase ? 0u : strings + 0xe1f);
        cell(can_increase ? 0x1009ce94 : 0x1009ce48, amount_grid, 5, 0x30, action_ink,
             can_increase ? 12 : 44, c.u16(ink), strings + 0xe1f, can_increase, 0x27);
        c.set(0, 0);
        cell(0x1009cedc, amount_grid, 6, 0x30, action_ink, 12, c.u16(ink), strings + 0xe21, true, 0x29);
    } else {
        c.set(0, strings + 0xe1d);
        cell(0x1009cf54, amount_grid, 3, 0x30, action_ink, 44, c.u16(ink), strings + 0xe1d);
        const auto amount = c.u32(draft + 0x24);
        c.set(0, amount);
        c.put32(stored, amount);
        money_text(c, program, text, stored, {0x1009cf68, 0x1009cf80, 0x1009cf88, 0x1009cf9c, 0x1009cfb4}, 6);
        const auto amount_ink = c.u16(ink);
        c.set(0, amount_ink);
        cell(0x1009cfe4, amount_grid, 4, 1, 0, 44, amount_ink, text);
        c.set(0, strings + 0xe1f);
        cell(0x1009d01c, amount_grid, 5, 0x30, action_ink, 44, c.u16(ink), strings + 0xe1f);
        c.set(0, strings + 0xe21);
        cell(0x1009d054, amount_grid, 6, 0x30, action_ink, 44, c.u16(ink), strings + 0xe21);
    }
}

void ClauseScreen::draw_presets() const {
    const auto grid = std::int16_t(presets_grid);
    c.set(0, std::uint32_t(grid));
    if (compare(c, grid, -1) == 0) return;
    const auto amount = frame.at(0x70);
    for (unsigned index = 0;; ++index) {
        c.set({{0, index}, {13, index}});
        if (compare(c, index, 9) >= 0) break;
        program.call(0x1000a330, 0x1009d2a0, {amount, index, frame.at(0x90), 9});
        const auto number = program.call(0x10009f20, 0x1009d2ac, {amount, 5});
        program.call(0x1000b410, 0x1009d2c4, {text, 2000, number, c.r(4), 5});
        const auto candidate = c.u32(amount);
        c.set(14, candidate);
        const bool available = compare(c, candidate, minimum) >= 0;
        c.set({{11, available ? 0u : 1u}, {12, available ? 1u : 33u}});
        words({16, c.u16(secondary), c.u16(c.toc(-0x7bd0)), available ? 1u : 33u, 1,
               c.u16(c.toc(-0x7bd4)), text, 0, available ? 0x26u : 0u,
               available ? candidate : 0u, presets_grid});
        program.call(0x10719740, available ? 0x1009d3d4 : 0x1009d350, {layout(true), 2, 0, 0, 0, 0, 0, 0});
    }
}
}

std::uint32_t negotiation_clauses_command(Cpu& c, std::uint32_t pressed) {
    Frame frame(c, 0x1009d410, 0x90, {.first_gpr = 25});
    const auto screens = c.toc(-0x7bb0), strings = c.toc(-0x56cc), text = c.toc(-0x7bb8);
    keep(c, {{25, c.toc(-0x7ba8)}, {26, strings}, {27, pressed}, {30, text}, {31, screens}});
    c.set(0, c.lr());
    Program program{c};
    const auto proposal = program.call(history, 0x1009d440, {screens, 2});
    const auto drafts = c.toc(-0x7a54);
    c.set({{5, drafts}, {29, proposal}});
    const auto draft = program.call(0x101487d0, 0x1009d454, {c.u32(drafts), 0});
    c.set(28, draft);
    if (compare(c, draft) == 0) {
        report_assertion(c, {0x1009d470, strings + 0x87, strings + 0x9c, strings + 0xa9,
                             frame.at(0x48), frame.at(0x40), 0x1098});
        c.set({{0, 0}, {4, c.toc(-0x7bac)}});
        program.call(0x1076f7d0, 0x1009d4b8, {0xffffffff});
    }
    const auto layout = c.u32(Manager{c, screens}.group() + 0x24);
    c.set(3, layout);
    const detail::Controls controls{c, screens, layout, std::int16_t(pressed)};
    const ClauseCommand ui{c, frame, program, controls, screens, draft, proposal, text};
    if (compare(c, controls.action(4), 0x26) == 0) {
        c.put32(draft + 0x24, ui.value());
        return redraw;
    }
    if (compare(c, controls.action(4), 0x12) == 0) {
        ui.enable();
        return redraw;
    }
    if (compare(c, controls.action(4), 0x13) == 0) {
        const auto clause = std::int8_t(ui.value());
        program.call(0x10374b60, 0x1009d654, {draft, std::uint32_t(clause)});
        return redraw;
    }
    if (compare(c, controls.action(4), 0x28) == 0) {
        ui.adjust(false);
        return 0;
    }
    if (compare(c, controls.action(4), 0x27) == 0) {
        ui.adjust(true);
        return 0;
    }
    if (compare(c, controls.action(3), 0x29) == 0) {
        ui.reset_amount();
        return redraw;
    }
    return 0;
}

void draw_negotiation_clauses(Cpu& c, std::uint32_t draft, std::uint32_t proposal, std::uint32_t person,
                             std::uint32_t top, std::uint32_t bottom) {
    Frame frame(c, 0x1009c870, 0x110, {.first_gpr = 13});
    ClauseScreen ui{c, frame, Program{c}, c.toc(-0x7bb0), c.toc(-0x7bb8), c.toc(-0x56cc), c.toc(-0x7bc8),
                    c.toc(-0x7bc4), c.toc(-0x7bf4), c.toc(-0x7bcc), draft, proposal, person};
    keep(c, {{14, ui.muted}, {15, draft}, {16, proposal}, {17, person}, {19, top}, {20, 0xffffffff},
             {21, bottom}, {25, ui.accent}, {26, ui.secondary}, {27, ui.ink}, {28, ui.strings},
             {29, ui.text}, {30, ui.screens}});
    const auto layout_template = c.toc(-0x5700);
    const auto code = c.u16(c.r(2) - 0x39f8);
    const auto style = c.u8(c.r(2) - 0x39f6);
    const auto head = c.u32(layout_template), tail = c.u32(layout_template + 4);
    const auto end = c.u8(layout_template + 8);
    c.put32(frame.at(0x78), 0);
    c.put16(frame.at(0x74), code);
    c.put8(frame.at(0x76), style);
    c.put32(frame.at(0x84), head);
    c.put32(frame.at(0x88), tail);
    c.put8(frame.at(0x8c), end);
    c.set({{0, end}, {5, tail}, {8, head}, {9, layout_template}, {10, style}, {11, code}, {12, 0}});
    ui.program.call(0x1074b6e0, 0x1009c8f4, {frame.at(0x7c), 0});
    const auto presets = c.toc(-0x5704), contracts = c.toc(-0x7b7c);
    keep(c, {{13, 0}, {22, c.u32(presets)}, {23, 0}, {24, contracts}});
    for (unsigned i = 0; i < 8; ++i) c.put32(frame.at(0x90 + i * 4), c.u32(presets + i * 4));
    c.put16(frame.at(0xb0), c.u16(presets + 0x20));
    c.put16(frame.at(0xb2), c.u16(presets + 0x22));
    c.put32(frame.at(0x70), 0);
    c.set({{0, c.u16(presets + 0x22)}, {5, c.u16(presets + 0x20)}, {6, c.u32(presets + 0x1c)},
           {7, c.u32(presets + 0x18)}, {8, c.u32(presets + 0x14)}, {9, c.u32(presets + 0x10)},
           {10, c.u32(presets + 0xc)}, {11, c.u32(presets + 8)}, {12, c.u32(presets + 4)}});
    const auto eligible = ui.program.call(0x10089ea0, 0x1009c964, {c.u32(contracts), proposal});
    c.set(31, eligible);
    const auto height = ui.program.call(0x1076d470, 0x1009c970, {3});
    const auto rows = std::uint8_t(divide_signed<4>(bottom - top, height));
    c.put8(frame.at(0xb4), rows);
    c.put32(frame.at(0xb8), ui.screens + 0x24);
    c.set({{22, ui.screens + 0x2c58}, {23, rows}});
    ui.words({0, 2, c.u16(ui.ink), 0xffffffff});
    ui.flags_grid = ui.program.call(grid_control, 0x1009c9e0, {ui.layout(), 110, top, 780, bottom, 3, frame.at(0x74), rows});
    c.set(24, ui.flags_grid);
    ui.words({0, 1, c.u16(ui.ink), 0xffffffff});
    ui.amount_grid = ui.program.call(grid_control, 0x1009ca34, {ui.layout(), 110, top, 780, bottom, 9, frame.at(0x84), rows});
    c.set(23, ui.amount_grid);

    ui.heading(0x122c, c.u16(ui.ink), 0x1009ca44, 0x1009ca50, 0x1009ca7c);
    ui.advance(2);
    const bool active = ui.draw_flags(false);
    const auto row = ui.row();
    c.set(3, row);
    compare(c, std::int8_t(c.r(13)));
    c.set(0, row + 1);
    c.put32(frame.at(0x78), row + 1);
    ui.draw_amount(active);

    const auto next = ui.row();
    c.set({{0, next + 2}, {5, next}});
    c.put32(frame.at(0x78), next + 2);
    ui.heading(0x1264, c.u16(ui.ink), 0x1009d06c, 0x1009d078, 0x1009d0a4);
    ui.advance(2);
    ui.draw_flags(true);
    const auto last = ui.row();
    c.set(3, last);
    const bool legend = compare(c, std::int8_t(c.r(31))) != 0;
    c.set(0, last + 1);
    c.put32(frame.at(0x78), last + 1);
    if (legend) {
        c.set(0, std::uint32_t(rows) - 1);
        c.put32(frame.at(0x78), std::uint32_t(rows) - 1);
        ui.heading(0x1212, c.u16(ui.accent), 0x1009d23c, 0x1009d248, 0x1009d274);
    }
    ui.draw_presets();
    ui.program.call(0x1074b7c0, 0x1009d3f0, {frame.at(0x7c), 0xffffffff});
}
}
