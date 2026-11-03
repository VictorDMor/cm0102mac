#include "contract_screens.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;

constexpr std::uint32_t history = 0x1073ef70, localize = 0x10727890, format = 0x1077b720,
                        copy_text = 0x1077dbf0, add_button = 0x107444e0, add_label = 0x107445a0,
                        has_role = 0x1007e4f0, player_role = 0x100c7a20;

struct ChoiceScreen {
    Cpu& c;
    const Frame& frame;
    Program program;
    std::uint32_t screens, text, strings, selected_ink, disabled_ink, normal_ink, recruitment;
    std::uint32_t person = 0, offer = 0;
    int selected_category = 0, selected_role = 0, proposal_category = 0, offer_rating = 0;
    unsigned row = 0;
    std::uint32_t options = 0;
    std::int16_t notes = 0;

    void words(std::initializer_list<std::uint32_t> values) const {
        auto at = frame.at(0x38);
        for (const auto value : values) {
            c.put32(at, value);
            at += 4;
        }
    }

    void advance(unsigned lines = 1) {
        row += lines;
        c.set(20, row);
    }

    std::uint32_t profile() const {
        const auto address = c.u32(person + 0x14);
        c.set(0, address);
        return address;
    }

    std::uint32_t secondary_record() const {
        const auto address = c.u32(person + 0x1c);
        c.set(0, address);
        return address;
    }

    bool selected_player_role(std::uint32_t back) const {
        const auto answer = std::int8_t(program.call(player_role, back, {std::uint32_t(selected_role)}));
        c.set(0, std::uint32_t(answer));
        return compare(c, answer) != 0;
    }

    std::uint32_t layout_control() const {
        const auto offset = std::uint32_t(c.i16(c.u32(frame.at(0x340)))) * 0x2c0;
        c.set(0, offset);
        return c.u32(c.u32(frame.at(0x33c)) + offset);
    }

    void caption(unsigned offset, std::uint32_t back) const {
        program.call(localize, back, {text, strings + offset});
    }

    void label(std::uint32_t back, std::uint32_t grid, std::uint32_t ink, unsigned at) const {
        const auto color = c.u16(ink);
        c.set(0, color);
        words({color, text});
        program.call(add_label, back, {screens, grid, 1, at, 1, 0, 1, 3});
    }

    void option(std::uint32_t back, std::uint32_t grid, std::uint32_t ink,
                unsigned command, unsigned value) const {
        words({c.u16(ink), text, command, value});
        program.call(add_button, back, {screens, grid, 1, row, 1, 0, 1, 3});
    }

    bool exceeds_offer_limit(int limit) const {
        return compare_logical(c, offer) != 0 && compare(c, offer_rating, limit) > 0 &&
               compare(c, limit, -1) != 0;
    }

    void category_row(unsigned category);
    void role_rows(int roles);
    void footnotes(std::uint32_t club);
    void buttons();
};

void ChoiceScreen::category_row(unsigned category) {
    const auto name = frame.at(0xe0);
    program.call(0x1036f7e0, 0x10095634, {c.u32(c.u32(recruitment) + 0x988), category, name, 2000});
    c.set(0, category);
    if (compare(c, category, proposal_category) == 0) {
        program.call(format, 0x10095650, {text, strings + 0xcd, name});
        c.set(0, 1);
        c.put8(frame.at(0x338), 1);
    } else {
        program.call(copy_text, 0x10095668, {text, name});
    }
    const auto limit = std::int32_t(program.call(0x1036f670, 0x10095680,
        {c.u32(c.u32(recruitment) + 0x988), c.r(14), person, category}));
    c.set(31, std::uint32_t(limit));
    if (!selected_player_role(0x1009568c)) {
        label(0x100956c4, options, disabled_ink, row);
        return;
    }

    c.set(0, std::uint32_t(proposal_category));
    const bool has_proposal = compare(c, proposal_category) != 0;
    if (has_proposal) {
        const auto record = c.u32(frame.at(0x348));
        c.set(0, record);
        if (compare_logical(c, record) != 0) {
            c.set(0, category);
            const bool selected = compare(c, category, selected_category) == 0;
            label(selected ? 0x10095718 : 0x1009574c, options, selected ? selected_ink : disabled_ink, row);
            return;
        }
    }

    const auto allowed = std::int8_t(program.call(0x1036f920, 0x10095764,
        {c.u32(c.u32(recruitment) + 0x988), person, category}));
    c.set(0, std::uint32_t(allowed));
    if (compare(c, allowed) == 0 || exceeds_offer_limit(limit) || compare_logical(c, profile()) == 0) {
        program.call(format, 0x100957a0, {text, strings + 0xd2, name});
        c.set(0, 1);
        // The original writes this flag after the offer-pointer comparison.
        const bool has_offer = compare_logical(c, offer) != 0;
        c.put8(frame.at(0x33a), 1);
        if (has_offer && compare(c, offer_rating, limit) > 0 && compare(c, limit, -1) != 0 &&
            compare_logical(c, profile()) != 0) {
            c.set(0, 1);
            c.put8(frame.at(0x339), 1);
        }
        label(0x10095804, options, disabled_ink, row);
        return;
    }

    c.set(0, std::uint32_t(proposal_category));
    const bool with_proposal = compare(c, proposal_category) != 0;
    c.set(0, category);
    const bool selected = compare(c, category, selected_category) == 0;
    const auto back = with_proposal ? (selected ? 0x10095858 : 0x10095898)
                                    : (selected ? 0x100958e4 : 0x10095924);
    c.set(11, 3);
    option(back, options, selected ? selected_ink : normal_ink, 3, category);
}

void ChoiceScreen::role_rows(int roles) {
    c.set(0, std::uint32_t(roles));
    advance();
    if (compare(c, roles, 1) <= 0) return;
    caption(0xd7, 0x10095960);
    c.set(0, 0);
    option(0x1009599c, notes, normal_ink, 0, 0);
    advance(2);
    for (unsigned role = 0;; ++role) {
        c.set({{13, role}, {0, role}});
        if (compare(c, role, 15) > 0) break;
        const auto enabled = std::int8_t(program.call(has_role, 0x100959c0,
            {c.u32(c.toc(-0x7b7c)), role, person}));
        c.set(0, std::uint32_t(enabled));
        if (compare(c, enabled) == 0) continue;
        c.set(0, role);
        if (compare(c, role, 10) == 0 && compare_logical(c, secondary_record()) != 0 &&
            compare_logical(c, profile()) != 0) continue;
        const auto name = program.call(0x100c6bf0, 0x10095a00, {role, 0, 1, 0});
        program.call(copy_text, 0x10095a0c, {text, name});
        c.set(11, role);
        const bool selected = compare(c, selected_role, role) == 0;
        c.set(0, 0x2f);
        option(selected ? 0x10095a54 : 0x10095a94, options, selected ? selected_ink : normal_ink, 0x2f, role);
        advance();
    }
}

void ChoiceScreen::footnotes(std::uint32_t club) {
    if (compare_logical(c, secondary_record()) != 0 && compare_logical(c, profile()) != 0) return;
    c.set(0, std::uint32_t(proposal_category));
    const bool has_proposal = compare(c, proposal_category) != 0;
    if (has_proposal) c.set(0, c.u8(frame.at(0x338)));
    if (has_proposal && compare(c, std::int8_t(c.r(0))) != 0) {
        const auto record = c.u32(frame.at(0x348));
        c.set(0, record);
        advance();
        if (compare_logical(c, record) != 0) {
            const auto date = frame.at(0x7c);
            program.call(0x10717270, 0x10095afc, {record + 0xc, date, 100, 255});
            program.call(localize, 0x10095b10, {text, strings + 0xe7, 100, date});
        } else {
            caption(0x129, 0x10095b20);
        }
        label(0x10095b50, notes, normal_ink, row);
    } else {
        c.set(0, c.u8(frame.at(0x33a)));
        if (compare(c, std::int8_t(c.r(0))) != 0) {
            const auto rating_warning = c.u8(frame.at(0x339));
            c.set(0, rating_warning);
            advance();
            bool details = false;
            if (compare(c, std::int8_t(rating_warning)) != 0) {
                if (compare_logical(c, club) == 0) {
                    details = true;
                } else {
                    const auto current_club = c.u32(person + 0x24);
                    c.set(0, current_club);
                    details = compare_logical(c, club, current_club) != 0;
                }
            }
            caption(details ? 0x174 : 0x142, details ? 0x10095ba0 : 0x10095b90);
            label(0x10095bd0, notes, normal_ink, row);
        }
    }
    if (compare_logical(c, profile()) == 0 || compare_logical(c, secondary_record()) != 0) return;
    const auto age = std::int8_t(c.i16(person + 0x5c));
    c.set(0, std::uint32_t(age));
    if (compare(c, age, 30) > 0) {
        caption(0x1ad, 0x10095c04);
        label(0x10095c34, notes, normal_ink, row + 1);
    }
}

void ChoiceScreen::buttons() {
    program.call(0x102a1070, 0x10095c40, {4, 0});
    words({0, 1, c.u16(frame.at(0x70)), 0xffffffff});
    const auto grid = program.call(0x107198c0, 0x10095c94,
        {layout_control(), 100, 555, 790, 590, 2, frame.at(0x6c), 1});
    c.set(13, grid);
    caption(0x7b, 0x10095ca4);
    c.set(0, 0);
    words({c.u16(normal_ink), text, 1, 0});
    const auto color = c.u16(disabled_ink);
    program.call(add_button, 0x10095ce4, {screens, grid, 0, 0, 0x30, color, 12, 3});
    program.call(localize, 0x10095cf4, {text, strings + 0x1ef, strings + 0x214});
    bool available = false;
    if (compare(c, selected_category, 7) < 0) {
        c.set(0, std::uint32_t(selected_category));
        available = compare(c, selected_category) > 0;
    }
    if (!available && compare_logical(c, profile()) != 0 && selected_player_role(0x10095d18)) {
        const auto ink = c.u16(normal_ink);
        c.set(0, ink);
        words({ink, text});
        program.call(add_label, 0x10095d94, {screens, grid, 1, 0, 0x30, c.u16(disabled_ink), 0x2c, 3});
    } else {
        c.set(0, 0);
        words({c.u16(normal_ink), text, 2, 0});
        program.call(add_button, 0x10095d60, {screens, grid, 1, 0, 0x30, c.u16(disabled_ink), 12, 3});
    }
}
}

void draw_contract_choice(Cpu& c) {
    Frame frame(c, 0x100952b0, 0x3a0, {.first_gpr = 13});
    const auto screens = c.toc(-0x7bb0);
    ChoiceScreen ui{c, frame, Program{c}, screens, c.toc(-0x7bb8), c.toc(-0x56cc),
                    c.toc(-0x7bd0), c.toc(-0x7bc4), c.toc(-0x7bc8), c.toc(-0x7b40)};
    keep(c, {{13, ui.recruitment}, {16, 0}, {20, 0}, {24, ui.selected_ink}, {25, ui.disabled_ink},
             {26, ui.normal_ink}, {27, ui.strings}, {28, ui.text}, {29, screens}});
    c.put8(frame.at(0x338), 0);
    c.put8(frame.at(0x339), 0);
    c.put8(frame.at(0x33a), 0);
    const auto style = c.toc(-0x56dc);
    const auto word = c.u32(style), tail = std::uint32_t(c.u16(style + 4)), flag = std::uint32_t(c.u8(style + 6));
    const auto button_style = c.u16(c.r(2) - 0x3a08), role_style = c.u16(c.r(2) - 0x3a04);
    c.put32(frame.at(0x74), word);
    c.put16(frame.at(0x78), tail);
    c.put8(frame.at(0x7a), flag);
    c.put16(frame.at(0x6c), button_style);
    c.put16(frame.at(0x68), role_style);
    c.set({{0, role_style}, {5, button_style}, {6, flag}, {7, tail}});

    ui.person = ui.program.call(history, 0x10095334, {screens, 0});
    c.set({{0, ui.person}, {21, ui.person}});
    ui.selected_category = std::int8_t(ui.program.call(history, 0x10095348, {screens, 1}));
    c.set(19, std::uint32_t(ui.selected_category));
    ui.offer = ui.program.call(history, 0x10095358, {screens, 2});
    c.set(30, ui.offer);
    ui.selected_role = std::int8_t(ui.program.call(history, 0x10095368, {screens, 4}));
    c.set(15, std::uint32_t(ui.selected_role));
    const auto pending = ui.program.call(history, 0x10095378, {screens, 5});
    c.put32(frame.at(0x348), pending);
    ui.proposal_category = std::int8_t(ui.program.call(history, 0x10095388, {screens, 6}));
    c.set(18, std::uint32_t(ui.proposal_category));
    if (compare_logical(c, ui.offer) != 0) {
        ui.offer_rating = std::int32_t(ui.program.call(0x10416710, 0x100953ac, {c.u32(ui.offer), 0, 0, 1, 1}));
        c.set(17, std::uint32_t(ui.offer_rating));
    }

    int roles = 0;
    for (unsigned role = 0;; ++role) {
        c.set({{14, role}, {0, role}});
        if (compare(c, role, 15) > 0) break;
        const auto enabled = std::int8_t(ui.program.call(has_role, 0x100953d0,
            {c.u32(c.toc(-0x7b7c)), role, ui.person}));
        c.set(0, std::uint32_t(enabled));
        if (compare(c, enabled) != 0) {
            c.set(0, ++roles);
            c.set(16, roles);
        }
    }

    const Manager manager{c, screens};
    c.put32(frame.at(0x340), manager.current().address());
    const auto index = manager.current().get();
    const auto group = manager.group(index);
    const auto instance = c.u32(group + 4), club = c.u32(instance + 0x24);
    c.set({{0, std::uint32_t(index) * 0x2c0}, {6, instance}, {14, club}});
    ui.program.call(0x107136d0, 0x10095420, {ui.person, 0, 0});
    ui.program.call(0x100c7a50, 0x10095434, {club, 1, frame.at(0x72), frame.at(0x70)});
    const auto name = frame.at(0x144);
    ui.program.call(0x100caeb0, 0x10095448, {ui.person, name, 500, 1});
    ui.program.call(0x10744250, 0x1009545c, {screens, c.u16(frame.at(0x70)), c.u16(frame.at(0x72)), name});
    ui.caption(0xaf, 0x10095468);
    ui.program.call(0x107442f0, 0x10095474, {screens, ui.text});

    c.set({{11, 0xffffffff}, {12, 0x7d7}, {22, 7}});
    c.put32(frame.at(0x33c), screens + 0x24);
    ui.words({1, 0, 0, 12, 7, 0, 0, 0, 0x7d7, ui.person, 0xffffffff});
    ui.program.call(0x10719740, 0x100954f4, {ui.layout_control(), 6, 125, 10, 790, 70, 0, 0});
    const auto height = ui.program.call(0x1076d470, 0x100954fc, {3});
    const auto lines = std::uint8_t(divide_signed<4>(405, height));
    c.set(22, lines);
    ui.words({0, 2, 0, 0xffffffff});
    ui.notes = std::int16_t(ui.program.call(0x107198c0, 0x10095558,
        {ui.layout_control(), 110, 140, 780, 545, 2, frame.at(0x74), lines}));
    c.put16(frame.at(0x344), std::uint16_t(ui.notes));
    if (compare_logical(c, ui.profile()) != 0) {
        ui.caption(0xc0, 0x10095574);
        c.set(0, 0);
        ui.option(0x100955b0, ui.notes, ui.normal_ink, 0, 0);
        ui.advance(2);
    }
    ui.words({0, 1, 0, 0xffffffff});
    ui.options = ui.program.call(0x107198c0, 0x10095604,
        {ui.layout_control(), 110, 140, 780, 545, 2, frame.at(0x68), lines});
    c.set(23, ui.options);
    for (unsigned category = 1;; ++category) {
        c.set({{22, category}, {0, category}});
        if (compare(c, category, 8) >= 0 || compare_logical(c, ui.profile()) == 0) break;
        c.set(0, category);
        if (compare(c, category, 7) == 0) continue;
        ui.category_row(category);
        ui.advance();
    }
    ui.role_rows(roles);
    ui.footnotes(club);
    ui.buttons();
}
}
