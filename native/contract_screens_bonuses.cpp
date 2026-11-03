#include "contract_screens.hpp"
#include "contract_screens_controls.hpp"
#include "contract_screens_money.hpp"
#include "screen_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;
constexpr auto history = 0x1073ef70u, write_history = 0x1073f1e0u, add_grid = 0x107198c0u,
               button = 0x107444e0u, label = 0x107445a0u;
constexpr auto redraw = std::uint32_t(-11);

using detail::MoneySites;
using detail::money_text;

struct Division {
    std::int32_t value, shifted;
    unsigned sign;
};

Division divide_money(Cpu& c, std::int32_t value, unsigned shift) {
    const auto high = std::int32_t((std::int64_t(value) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)) != 0);
    const auto shifted = high >> shift;
    const auto sign = std::uint32_t(shifted) >> 31;
    return {std::int32_t(std::uint32_t(shifted) + sign), shifted, sign};
}

std::uint32_t club_budget(Cpu& c, const Program& program, std::uint32_t proposal, std::uint32_t clubs,
                         std::uint32_t back) {
    const auto offset = c.u32(proposal + 4) * 0x248;
    c.set(0, offset);
    return program.call(0x10103510, back, {c.u32(c.toc(-0x7ab8)), c.u32(clubs) + offset});
}

struct ControlSites { std::uint32_t type, style, action, value; };

struct BonusCommand {
    Cpu& c;
    const Frame& frame;
    Program program;
    const detail::Controls& controls;
    std::uint32_t screens, proposal, draft, text, clubs, people;

    void configure(std::int32_t widget, bool enabled, unsigned command, int bonus, ControlSites sites,
                   bool keep_bonus = false) const {
        program.call(0x10740620, sites.type, {screens, std::uint32_t(widget), enabled ? 2u : 6u});
        program.call(0x10741450, sites.style, {screens, std::uint32_t(widget), enabled ? 12u : 44u});
        program.call(0x10742100, sites.action, {screens, std::uint32_t(widget), enabled ? command : 0u});
        if (keep_bonus) c.set(24, std::uint32_t(bonus));
        program.call(0x10742420, sites.value, {screens, std::uint32_t(widget), enabled ? std::uint32_t(bonus) : 0u});
    }

    int index() const {
        const auto raw = controls.value();
        c.set(0, raw);
        return std::int8_t(raw);
    }

    Division limit(std::uint32_t club_back, std::uint32_t salary_back) const {
        const auto bank = club_budget(c, program, proposal, clubs, club_back);
        const auto offset = c.u32(proposal) * 0x70;
        c.set(0, offset);
        return divide_money(c, std::int32_t(program.call(0x100f2680, salary_back,
            {bank, c.u32(people) + offset, 0xffffffff})), 2);
    }

    void adjust(bool increase) const {
        const auto bonus = index();
        const auto offset = std::uint32_t(bonus) * 4;
        const auto local = frame.at(increase ? 0x38 : 0x3c), field = draft + offset + 0x10;
        c.set({{22, draft + offset}, {23, std::uint32_t(bonus)}});
        const auto old = c.u32(field);
        c.set(0, old);
        c.put32(local, old);
        program.call(increase ? 0x1000a060 : 0x1000a170, increase ? 0x1009c458 : 0x1009c268, {local, 8});
        const auto amount = c.u32(local);
        c.set(0, amount);
        c.put32(field, amount);
        const auto widget = std::int16_t(program.call(history, increase ? 0x1009c46c : 0x1009c27c,
            {screens, std::uint32_t(bonus + 0x12)}));
        c.set(increase ? 26 : 22, std::uint32_t(widget));
        if (increase) {
            configure(widget, true, 0x10, bonus, {0x1009c480, 0x1009c490, 0x1009c4a0, 0x1009c4b4}, true);
            money_text(c, program, text, local, {0x1009c4c0, 0x1009c4d8, 0x1009c4e0, 0x1009c4f4, 0x1009c50c});
            program.call(0x10741da0, 0x1009c51c, {screens, std::uint32_t(widget + 1), text});
            const auto cap = limit(0x1009c534, 0x1009c54c);
            const auto new_amount = c.u32(local);
            c.set({{0, new_amount}, {3, std::uint32_t(cap.value)}, {4, cap.sign}});
            const bool can_increase = compare(c, new_amount, cap.value) < 0;
            configure(widget + 2, can_increase, 0xf, bonus,
                      can_increase ? ControlSites{0x1009c5c4, 0x1009c5d4, 0x1009c5e4, 0x1009c5f4}
                                   : ControlSites{0x1009c580, 0x1009c590, 0x1009c5a0, 0x1009c5b0});
            program.call(0x10742ec0, 0x1009c604, {screens, 0, 0});
        } else {
            const auto new_amount = c.u32(local);
            c.set(0, new_amount);
            const bool can_decrease = compare(c, new_amount, 5) > 0;
            configure(widget, can_decrease, 0x10, bonus,
                      can_decrease ? ControlSites{0x1009c2e0, 0x1009c2f0, 0x1009c300, 0x1009c310}
                                   : ControlSites{0x1009c29c, 0x1009c2ac, 0x1009c2bc, 0x1009c2cc});
            money_text(c, program, text, local, {0x1009c31c, 0x1009c334, 0x1009c33c, 0x1009c350, 0x1009c368});
            program.call(0x10741da0, 0x1009c378, {screens, std::uint32_t(widget + 1), text});
            configure(widget + 2, true, 0xf, bonus, {0x1009c388, 0x1009c398, 0x1009c3a8, 0x1009c3b8});
            program.call(0x10742ec0, 0x1009c3c8, {screens, 0, 0});
        }
    }

    void suggest() const {
        const auto offset = std::uint32_t(index()) * 4;
        c.set(24, offset);
        const auto current = c.u32(proposal + offset + 0x10);
        c.set({{0, current}, {3, proposal + offset}});
        const auto field = draft + offset + 0x10;
        if (compare(c, current, -1) > 0) {
            c.set(3, draft + offset);
            c.put32(field, current);
        } else {
            const auto default_value = divide_money(c, c.i32(proposal + 0xc), 2);
            c.set({{0, std::uint32_t(default_value.value)}, {3, draft + offset}, {4, default_value.sign}});
            c.put32(field, std::uint32_t(default_value.value));
        }
        const auto cap = limit(0x1009c6d4, 0x1009c6ec);
        const auto proposed = c.u32(field);
        c.set({{0, proposed}, {3, std::uint32_t(cap.value)}, {4, cap.sign}, {5, draft + offset}});
        if (compare(c, proposed, cap.value) > 0) c.put32(field, std::uint32_t(cap.value));
    }

    void toggle(bool add) const {
        const auto bonus = index();
        if (!add) {
            program.call(0x10374b80, 0x1009c84c, {draft, std::uint32_t(bonus)});
            return;
        }
        const auto offset = std::uint32_t(bonus) * 4, proposed = c.u32(proposal + offset + 0x10);
        c.set(0, offset);
        if (compare(c, proposed, -1) > 0) {
            program.call(0x10374b40, 0x1009c7a8, {draft, std::uint32_t(bonus), proposed});
        } else {
            const auto fallback = divide_money(c, c.i32(draft + 0xc), 2);
            c.set(0, std::uint32_t(fallback.shifted));
            program.call(0x10374b40, 0x1009c7d0, {draft, std::uint32_t(bonus), std::uint32_t(fallback.value)});
        }
    }
};
}

std::uint32_t negotiation_bonuses_command(Cpu& c, std::uint32_t pressed) {
    Frame frame(c, 0x1009c020, 0x90, {.first_gpr = 22});
    const auto screens = c.toc(-0x7bb0), text = c.toc(-0x7bb8), strings = c.toc(-0x56cc),
               clubs = c.toc(-0x7be8), people = c.toc(-0x7be0);
    keep(c, {{22, c.toc(-0x7ba8)}, {23, strings}, {24, pressed}, {25, people}, {27, c.toc(-0x7ab8)},
             {28, clubs}, {30, text}, {31, screens}});
    c.set(0, c.lr());
    Program program{c};
    const auto proposal = program.call(history, 0x1009c05c, {screens, 2});
    const auto drafts = c.toc(-0x7a54);
    c.set({{5, drafts}, {29, proposal}});
    const auto draft = program.call(0x101487d0, 0x1009c070, {c.u32(drafts), 0});
    c.set(26, draft);
    if (compare(c, draft) == 0) {
        report_assertion(c, {0x1009c08c, strings + 0x87, strings + 0x9c, strings + 0xa9,
                             frame.at(0x48), frame.at(0x40), 0xec2});
        c.set({{0, 0}, {4, c.toc(-0x7bac)}});
        program.call(0x1076f7d0, 0x1009c0d4, {0xffffffff});
    }
    const auto layout = c.u32(Manager{c, screens}.group() + 0x24);
    c.set(3, layout);
    const detail::Controls controls{c, screens, layout, std::int16_t(pressed)};
    const BonusCommand ui{c, frame, program, controls, screens, proposal, draft, text, clubs, people};
    if (compare(c, controls.action(4), 0x2a) >= 0 && compare(c, controls.action(4), 0x2d) <= 0) {
        const auto amount = controls.value(4);
        c.set(5, amount);
        const auto offset = std::uint32_t(controls.action(3)) * 4;
        c.set({{0, offset}, {4, draft + offset}});
        c.put32(draft + offset - 0x98, amount);
        return redraw;
    }
    if (compare(c, controls.action(4), 0x10) == 0) {
        ui.adjust(false);
        return 0;
    }
    if (compare(c, controls.action(4), 0xf) == 0) {
        ui.adjust(true);
        return 0;
    }
    if (compare(c, controls.action(4), 0x11) == 0) {
        ui.suggest();
        return redraw;
    }
    if (compare(c, controls.action(4), 0xd) == 0) {
        ui.toggle(true);
        return redraw;
    }
    if (compare(c, controls.action(4), 0xe) == 0) {
        ui.toggle(false);
        return redraw;
    }
    return 0;
}

namespace {
struct BonusScreen {
    Cpu& c;
    const Frame& frame;
    Program program;
    std::uint32_t screens, text, strings, ink, muted, enabled, selected, secondary, accent,
                  draft, proposal, person, grid = 0;
    std::int32_t cap = 0;

    void words(std::initializer_list<std::uint32_t> words) const {
        auto at = frame.at(0x38);
        for (auto value : words) {
            c.put32(at, value);
            at += 4;
        }
    }

    std::uint32_t layout(bool indexed = false) const {
        const auto offset = std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + (indexed ? 0x24 : 0);
        c.set(0, offset);
        return c.u32(screens + offset + (indexed ? 0 : 0x24));
    }

    std::uint32_t cell(std::uint32_t back, unsigned column, std::uint32_t color, std::uint32_t caption,
                       unsigned type, std::uint32_t decoration, unsigned style, std::uint32_t action,
                       unsigned index, bool interactive = true) const {
        if (interactive) {
            c.set(0, action);
            words({color, caption, action, index});
        } else {
            c.set(0, caption);
            words({color, caption});
        }
        return program.call(interactive ? button : label, back, {screens, grid, column, c.u32(frame.at(0x78)),
                            type, decoration, style, 3});
    }

    void text_row(std::uint32_t caption_back, unsigned caption, std::uint32_t text_back,
                  std::uint32_t render_back, std::uint32_t color) const {
        const auto box = frame.at(0x84);
        program.call(0x10727890, caption_back, {text, strings + caption});
        program.call(0x1074b830, text_back, {box, text});
        c.set(0, 0);
        words({0});
        program.call(0x1074bcd0, render_back, {box, grid, 120, 770, frame.at(0x78), 3, color, 0});
    }

    void draw_active(unsigned index, std::int32_t amount) const;
    void draw_inactive(unsigned index) const;
    void draw_row(unsigned index) const;
    void draw_choices() const;
};

void BonusScreen::draw_active(unsigned index, std::int32_t amount) const {
    const auto amount_at = frame.at(0x6c), note = frame.at(0xac);
    c.put32(amount_at, std::uint32_t(amount));
    program.call(0x10380270, 0x1009b8f0, {index, text});
    c.set(19, index);
    cell(0x1009b930, 2, c.u16(ink), text, 1, 0, 1, 0xe, index);

    c.set(0, std::uint32_t(amount));
    const bool reduce = compare(c, amount, 5) > 0;
    const auto lower = cell(reduce ? 0x1009b9b8 : 0x1009b970, 3, c.u16(ink), strings + 0xe1d,
                            0x30, c.u16(enabled), reduce ? 12 : 44, 0x10, index, reduce);
    c.set(0, lower);
    program.call(write_history, 0x1009b9d4, {screens, index + 0x12, std::uint32_t(std::int16_t(lower)), 0});
    money_text(c, program, text, amount_at, {0x1009b9e0, 0x1009b9f8, 0x1009ba00, 0x1009ba14, 0x1009ba2c});
    const auto shown_amount = cell(0x1009ba68, 4, c.u16(ink), text, 1, 0, 12, 0, 0);
    words({0, 0x130, c.u16(secondary), shown_amount});
    const auto choices = program.call(add_grid, 0x1009bab4, {layout(true), 0, 0, 0, 0, 0, 0, 0});
    c.put16(frame.at(0x7c) + index * 2, choices);

    const auto value = c.u32(amount_at);
    c.set({{0, value}, {4, frame.at(0x7c)}});
    const bool raise = compare(c, value, cap) < 0;
    cell(raise ? 0x1009bb40 : 0x1009bafc, 5, c.u16(ink), strings + 0xe1f,
         0x30, c.u16(enabled), raise ? 12 : 44, 0xf, index, raise);
    cell(0x1009bb80, 6, c.u16(ink), strings + 0xe21, 0x30, c.u16(enabled), 12, 0x11, index);
    const auto show_note = c.i8(note);
    c.set(0, std::uint32_t(show_note));
    if (compare(c, show_note) != 0)
        cell(0x1009bbcc, 8, c.u16(accent), note, 1, 0, 1, 0x11, index);
}

void BonusScreen::draw_inactive(unsigned index) const {
    program.call(0x10380270, 0x1009bbdc, {index, text});
    c.set(19, index);
    cell(0x1009bc20, 2, c.u16(muted), text, 1, 0, 1, 0xd, index);
    cell(0x1009bc60, 3, c.u16(ink), strings + 0xe1d, 0x30, c.u16(enabled), 44, 0xd, index);
    const auto note = frame.at(0xac);
    const auto show_note = c.i8(note);
    c.set(0, std::uint32_t(show_note));
    if (compare(c, show_note) != 0) {
        program.call(0x1077dbf0, 0x1009bc78, {text, note});
    } else {
        const auto estimate = divide_money(c, c.i32(draft + 0xc), 2);
        c.set({{0, std::uint32_t(estimate.value)}, {5, estimate.sign}});
        c.put32(frame.at(0x68), std::uint32_t(estimate.value));
        money_text(c, program, text, frame.at(0x68), {0x1009bca8, 0x1009bcc0, 0x1009bcc8, 0x1009bcdc, 0x1009bcf4});
    }
    cell(0x1009bd30, 4, c.u16(ink), text, 1, 0, 44, 0xd, index);
    cell(0x1009bd70, 5, c.u16(ink), strings + 0xe1f, 0x30, c.u16(enabled), 44, 0xd, index);
    cell(0x1009bdb0, 6, c.u16(ink), strings + 0xe21, 0x30, c.u16(enabled), 44, 0xd, index);
    const auto hint = c.i8(note);
    c.set(0, std::uint32_t(hint));
    if (compare(c, hint) != 0)
        cell(0x1009bdfc, 8, c.u16(accent), note, 1, 0, 33, 0xd, index);
}

void BonusScreen::draw_row(unsigned index) const {
    const auto supported = std::int8_t(program.call(0x103803b0, 0x1009b7e4, {person, index}));
    c.set(0, std::uint32_t(supported));
    if (compare(c, supported) == 0) return;
    const auto offset = index * 4 + 0x10;
    c.set(20, offset);
    const auto requested = c.u32(draft + offset);
    c.set(0, requested);
    if (compare(c, requested, cap) > 0) c.put32(draft + offset, std::uint32_t(cap));

    c.set(19, offset);
    const auto proposed = c.u32(proposal + offset);
    c.set(3, proposed);
    bool show_offer = false;
    if (compare(c, proposed, -1) > 0) {
        const auto valid = c.i8(frame.at(0xd0) + 3);
        c.set(0, std::uint32_t(valid));
        show_offer = compare(c, valid) != 0 && compare(c, proposed, cap) <= 0;
    }
    const auto note = frame.at(0xac);
    if (show_offer) {
        cell(0x1009b858, 1, c.u16(accent), strings + 0x120b, 1, 0, 12, 0, 0, false);
        const auto offered = c.u32(proposal + offset);
        c.set(0, offered);
        c.put32(frame.at(0x70), offered);
        money_text(c, program, text, frame.at(0x70), {0x1009b86c, 0x1009b884, 0x1009b88c, 0x1009b8a0, 0x1009b8b8});
        program.call(0x1077b720, 0x1009b8c8, {note, strings + 0x120d, text});
    } else {
        c.set(0, 0);
        c.put8(note, 0);
    }

    const auto amount = c.i32(draft + offset);
    c.set(0, std::uint32_t(amount));
    if (compare(c, amount, -1) > 0) draw_active(index, amount);
    else draw_inactive(index);
    const auto row = c.u32(frame.at(0x78));
    c.set({{0, row + 1}, {3, row}});
    c.put32(frame.at(0x78), row + 1);
}

void BonusScreen::draw_choices() const {
    const auto choices = frame.at(0x7c), amounts = frame.at(0x98);
    std::array<unsigned, 5> signs{};
    std::uint32_t last = 0;
    c.set({{8, 0x66666667}, {15, choices}, {13, 0}});
    for (unsigned i = 0; i < 5; ++i) {
        const auto portion = divide_money(c, std::int32_t(std::uint32_t(cap) * (i + 1)), 1);
        last = std::uint32_t(portion.value);
        c.put32(amounts + i * 4, last);
        signs[i] = portion.sign;
    }
    c.set({{0, last}, {3, signs[4]}, {4, signs[3]}, {5, signs[2]}, {6, signs[1]}, {7, signs[0]}});
    for (unsigned row = 0;; ++row) {
        c.set({{0, row}, {13, row}, {15, choices + row * 2}});
        if (compare(c, row, 3) >= 0) break;
        const auto menu = c.i16(choices + row * 2);
        c.set(0, std::uint32_t(menu));
        if (compare(c, menu, -1) == 0) continue;
        const auto command = row + 0x2a;
        c.set({{3, row}, {14, 0}, {16, command}});
        for (unsigned item = 0;; ++item) {
            c.set({{0, item}, {14, item}});
            if (compare(c, item, 5) >= 0) break;
            const auto value = frame.at(0x74);
            program.call(0x1000a330, 0x1009bf34, {value, item, amounts, 5});
            money_text(c, program, text, value, {0x1009bf40, 0x1009bf58});
            words({16, c.u16(secondary), c.u16(selected), 1, 1, c.u16(c.r(25)),
                   text, 0, command, c.u32(value), std::uint32_t(c.i16(c.r(15)))});
            c.set(11, c.u16(selected));
            program.call(0x10719740, 0x1009bfd4, {layout(true), 2, 0, 0, 0, 0, 0, 0});
        }
    }
}
}

void draw_negotiation_bonuses(Cpu& c, std::uint32_t draft, std::uint32_t proposal, std::uint32_t person,
                             std::uint32_t top, std::uint32_t bottom) {
    Frame frame(c, 0x1009b620, 0x130, {.first_gpr = 13});
    BonusScreen ui{c, frame, Program{c}, c.toc(-0x7bb0), c.toc(-0x7bb8), c.toc(-0x56cc), c.toc(-0x7bc8),
                   c.toc(-0x7bc4), c.toc(-0x7bdc), c.toc(-0x7bd0), c.toc(-0x7bcc), c.toc(-0x7bf4),
                   draft, proposal, person};
    keep(c, {{13, ui.accent}, {14, proposal}, {15, draft}, {16, bottom}, {17, top}, {24, ui.enabled},
             {25, c.toc(-0x7bd4)}, {26, ui.secondary}, {27, ui.selected}, {28, ui.ink},
             {29, ui.strings}, {30, ui.text}, {31, ui.screens}});
    const auto template_address = c.toc(-0x56fc);
    const auto style = c.u32(template_address), border = c.u32(template_address + 4);
    const auto tail = c.u8(template_address + 8);
    c.put32(frame.at(0x150), person);
    c.put32(frame.at(0x78), 0);
    c.put32(frame.at(0x8c), style);
    c.put32(frame.at(0x90), border);
    c.put8(frame.at(0x94), tail);
    c.set({{0, tail}, {5, border}, {8, style}, {9, template_address}, {10, 0}});
    ui.program.call(0x1074b6e0, 0x1009b698, {frame.at(0x84), 0});
    c.set(0, 0);
    c.put32(frame.at(0x74), 0);
    ui.program.call(0x10778300, 0x1009b6b0, {frame.at(0x7c), 0xffffffff, 6});
    const auto eligible = ui.program.call(0x10089ea0, 0x1009b6c0, {c.u32(c.toc(-0x7b7c)), proposal});
    c.put32(frame.at(0xd0), eligible);
    const auto line_height = ui.program.call(0x1076d470, 0x1009b6cc, {3});
    const auto rows = std::uint8_t(divide_signed<4>(bottom - top, line_height));
    c.put8(frame.at(0xcc), rows);
    c.set(18, ui.screens + 0x2c58);
    ui.words({0, 2, c.u16(ui.ink), 0xffffffff});
    ui.grid = ui.program.call(add_grid, 0x1009b730, {ui.layout(), 110, top, 780, bottom, 9, frame.at(0x8c), rows});
    c.set(23, ui.grid);
    ui.text_row(0x1009b740, 0x11e6, 0x1009b74c, 0x1009b778, c.u16(ui.ink));
    c.put32(frame.at(0x78), c.u32(frame.at(0x78)) + 2);
    c.set(5, c.toc(-0x7ab8));
    const auto bank = club_budget(c, ui.program, proposal, c.toc(-0x7be8), 0x1009b7a4);
    const auto salary = ui.program.call(0x100f2680, 0x1009b7b0, {bank, person, 0xffffffff});
    const auto cap = divide_money(c, std::int32_t(salary), 2);
    ui.cap = cap.value;
    c.set({{0, std::uint32_t(cap.shifted)}, {3, cap.sign}, {4, 0x66660000}, {17, std::uint32_t(cap.value)}});
    for (unsigned index = 0;; ++index) {
        c.set({{0, index}, {16, index}, {21, index * 2}, {22, index * 4}});
        if (compare(c, index, 3) >= 0) break;
        ui.draw_row(index);
    }
    const auto show_legend = c.i8(frame.at(0xd0) + 3);
    c.set(0, std::uint32_t(show_legend));
    if (compare(c, show_legend) != 0) {
        const auto last_row = std::uint32_t(rows) - 1;
        c.set(0, last_row);
        c.put32(frame.at(0x78), last_row);
        ui.text_row(0x1009be44, 0x1212, 0x1009be50, 0x1009be7c, c.u16(ui.accent));
    }
    ui.draw_choices();
    ui.program.call(0x1074b7c0, 0x1009c004, {frame.at(0x84), 0xffffffff});
}
}
