#include "contract_screens.hpp"
#include "contract_screens_calls.hpp"
#include "guest_fpu.hpp"
#include "screen_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;
constexpr std::uint32_t history_read = 0x1073ef70, history_write = 0x1073f1e0, read_year = 0x10718360,
                        locate_pending = 0x1007f720, locate_primary = 0x10082f20,
                        text_init = 0x1074b6e0, text_dispose = 0x1074b7c0, text_set = 0x1074b830,
                        localized_text = 0x10727890, grid_control = 0x107198c0, button = 0x107444e0;

struct NegotiationScreen {
    Cpu& c;
    const Frame& frame;
    Program program;
    std::uint32_t screens, text, strings, ink, secondary_ink, highlight, contract_cache;
    std::uint32_t person = 0, club = 0, link = 0, proposal = 0, draft = 0;
    int tab = 0;

    std::uint32_t read(std::uint32_t back, unsigned slot) const {
        return program.call(history_read, back, {screens, slot});
    }
    void write(std::uint32_t back, unsigned slot, std::uint32_t value) const {
        program.call(history_write, back, {screens, slot, value, 0});
    }
    void words(std::initializer_list<std::uint32_t> values) const {
        auto at = frame.at(0x38);
        for (const auto value : values) {
            c.put32(at, value);
            at += 4;
        }
    }
    void caption(std::uint32_t back, unsigned offset) const {
        program.call(localized_text, back, {text, strings + offset});
    }
    std::uint32_t layout_offset() const {
        return std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
    }
    std::uint32_t layout() const {
        const auto offset = layout_offset();
        c.set(0, offset);
        return c.u32(screens + 0x24 + offset);
    }
    void dispose(std::uint32_t back) const {
        program.call(text_dispose, back, {frame.at(0xa8), 0xffffffff});
    }
    std::uint32_t pending(std::uint32_t back) const {
        return program.call(locate_pending, back, {c.u32(c.toc(-0x7b7c)), c.u32(person), c.u32(club)});
    }
    std::uint32_t primary(std::uint32_t back) const {
        return program.call(locate_primary, back, {c.u32(c.toc(-0x7b7c)), person});
    }
    bool player(std::uint32_t back, std::uint32_t role) const {
        const auto result = std::int8_t(program.call(0x100c7a20, back, {role}));
        c.set(0, std::uint32_t(result));
        return compare(c, result) != 0;
    }
    int contract_kind(std::uint32_t record) const {
        const auto kind = c.u8(record + 0x38);
        c.s.wr<1>(reg::xer_ca, (kind & 0x82) == 0x82);
        const auto decoded = std::int8_t(kind) >> 2;
        c.set(0, std::uint32_t(decoded));
        return decoded;
    }
    void alert(std::uint32_t at, unsigned name, unsigned extension, unsigned line) const {
        report_assertion(c, {at, strings + 0x87, strings + 0x9c, strings + 0xa9,
                             frame.at(name), frame.at(extension), line});
        c.set(0, 0);
    }

    void copy_terms(std::uint32_t source, std::uint32_t back) const;
    void initialize_offer(std::int32_t offer_id, std::uint32_t date);
    void draw_comparisons();
    void draw_heading();
    void draw_tabs();
    void draw_buttons();
};

void NegotiationScreen::copy_terms(std::uint32_t source, std::uint32_t back) const {
    for (const auto at : {0, 4, 8, 0xc, 0x10, 0x14}) c.put32(draft + at, c.u32(source + at));
    const auto trailing = c.u16(source + 0x1a);
    c.put16(draft + 0x18, c.u16(source + 0x18));
    c.put16(draft + 0x1a, trailing);
    c.set({{0, trailing}, {6, c.u32(source + 0x10)}});
    program.call(0x10771110, back, {draft + 0x1c, source + 0x1c, 5});
    // The date fields are signed years, unsigned days and one flags byte.
    for (const auto at : {0x24, 0x3c, 0x48, 0x50, 0x58, 0x60}) c.put32(draft + at, c.u32(source + at));
    for (const auto at : {0x2a, 0x28, 0x32, 0x30, 0x42}) c.put16(draft + at, c.u16(source + at));
    for (const auto at : {0x2c, 0x34, 0x38, 0x40, 0x44, 0x45, 0x46, 0x4c, 0x54, 0x5c})
        c.put8(draft + at, c.u8(source + at));
    c.set(0, c.u32(source + 0x60));
}

void NegotiationScreen::initialize_offer(std::int32_t offer_id, std::uint32_t date) {
    const auto role = std::int8_t(read(0x10096dec, 0xd));
    c.set(23, std::uint32_t(role));
    proposal = program.call(0x10773520, 0x10096df8, {0x64});
    c.set(18, proposal);
    if (compare(c, proposal) != 0) program.call(0x103721f0, 0x10096e04, {proposal});
    program.call(0x10374f50, 0x10096e28, {proposal, person, club, std::uint32_t(offer_id), std::uint32_t(role),
                 c.u8(frame.at(0xb44)), frame.at(0x78), frame.at(0x74)});
    write(0x10096e3c, 0x19, c.u32(frame.at(0x78)));

    Fpu fp(c);
    const auto multiplier = c.f64(c.toc(-0x56d0) + 0x30);
    c.set_f(1, multiplier);
    const auto converted = fp.fctiwz(0, fp.fmul(multiplier, c.f32(frame.at(0x74))));
    c.put64(frame.at(0xb3c), converted);
    write(0x10096e68, 0x1a, std::uint32_t(converted));

    const auto previous = pending(0x10096e7c);
    c.set(16, previous);
    bool reuse = false;
    if (compare(c, previous) != 0) {
        const auto kind = contract_kind(previous);
        reuse = compare(c, kind, 5) != 0 && compare(c, kind, 6) != 0;
    }
    if (reuse) {
        copy_terms(previous, 0x10096ef4);
        const auto limit = read(0x10096fa8, 0x15);
        const auto wage = c.u32(draft + 8);
        c.set(0, wage);
        if (compare(c, wage, limit) > 0) c.put32(draft + 8, limit);
        write(0x10096fd0, 8, std::uint32_t(c.i8(previous + 0x78)));
        const auto category = c.u8(previous + 0x78);
        c.set(0, category);
        c.put8(frame.at(0xb44), category);
    } else {
        const auto new_person = c.u32(proposal), old_person = c.u32(draft);
        c.set({{0, old_person}, {4, new_person}});
        bool changed = compare(c, old_person, new_person) != 0;
        if (!changed) {
            const auto old_club = c.u32(draft + 4), new_club = c.u32(proposal + 4);
            c.set({{3, old_club}, {0, new_club}});
            changed = compare(c, old_club, new_club) != 0;
        }
        if (changed) {
            copy_terms(proposal, 0x10097048);
            program.call(0x1008a160, 0x10097100, {c.u32(c.toc(-0x7b7c)), proposal});
            const auto limit = read(0x1009710c, 0x15);
            const auto wage = c.u32(draft + 8);
            c.set(0, wage);
            if (compare(c, wage, limit) > 0) c.put32(draft + 8, limit);
        }
    }

    const auto current = primary(0x1009712c);
    c.set(16, current);
    bool same_club = false;
    if (compare(c, current) != 0) {
        const auto draft_club = c.u32(draft + 4);
        c.set(0, draft_club);
        same_club = compare(c, c.u32(current + 4), draft_club) == 0;
    }
    if (same_club) {
        const auto year = std::int8_t(program.call(read_year, 0x1009714c, {current + 0x30}));
        write(0x10097160, 9, std::uint32_t(year));
        write(0x10097174, 0xa, c.u16(current + 0x32));
    } else {
        program.call(0x10717b20, 0x10097188, {frame.at(0x90), date, 32});
        const auto year = std::int8_t(program.call(read_year, 0x10097190, {frame.at(0x90)}));
        write(0x100971a4, 9, std::uint32_t(year));
        program.call(0x10717b20, 0x100971b4, {frame.at(0x88), date, 32});
        write(0x100971c8, 0xa, c.u16(frame.at(0x8a)));
    }
    write(0x100971dc, 2, proposal);
    const auto year = std::int8_t(program.call(read_year, 0x100971e4, {proposal + 0x30}));
    write(0x100971f8, 0xb, std::uint32_t(year));
    write(0x1009720c, 0xc, c.u16(proposal + 0x32));
}

void NegotiationScreen::draw_comparisons() {
    program.call(0x107136d0, 0x1009722c, {person, 0, 0});
    c.set(15, 0);
    const auto existing = primary(0x10097240);
    const bool has_contract = compare_logical(c, existing) != 0;
    c.put32(contract_cache, existing);
    const bool employed = has_contract && compare(c, contract_kind(existing), 5) != 0;
    c.set(15, employed);
    compare(c, employed);
    std::uint32_t employer = 0;
    if (employed) {
        const auto offset = c.u32(c.u32(contract_cache) + 4) * 0x248;
        const auto table = c.toc(-0x7be8);
        c.set({{0, offset}, {3, c.u32(table)}, {4, table}});
        employer = c.r(3) + offset;
    }
    c.set(24, employer);
    if (compare_logical(c, employer) == 0 || compare_logical(c, employer, club) != 0 ||
        !player(0x100972a8, c.u8(person + 0x62))) {
        const auto ink_value = c.u16(ink);
        const auto accent = c.u16(c.toc(-0x7b14));
        c.set({{0, ink_value}, {3, accent}});
        c.put16(frame.at(0x6c), ink_value);
        c.put16(frame.at(0x6e), accent);
        return;
    }

    program.call(0x100c7a50, 0x100972c4, {employer, 1, frame.at(0x6e), frame.at(0x6c)});
    c.set(0, 0);
    words({1, c.u16(frame.at(0x6e)), 0, 0, 0});
    const auto list = program.call(0x10744430, 0x10097308,
        {screens, 105, 15, 120, 35, 0x30, c.u16(frame.at(0x6c)), 12});
    words({0, 0x30, c.u16(secondary_ink), list});
    const auto grid = program.call(grid_control, 0x10097354, {layout(), 0, 0, 0, 0, 0, 0, 0});
    c.set(31, grid);
    const auto players = employer + 0x98;
    program.call(0x10768300, 0x1009736c, {players, 50, 4, c.toc(-0x7988)});
    program.call(0x10768300, 0x10097380, {players, 20, 4, c.toc(-0x798c)});
    for (unsigned index = 0;; ++index) {
        c.set({{19, index}, {23, index * 4}});
        if (index == 20) {
            compare(c, index, 20);
            break;
        }
        if (index) compare(c, index, 20);
        const auto candidate = c.u32(players + index * 4);
        c.set({{0, 0x98 + index * 4}, {15, candidate}});
        if (compare_logical(c, candidate) == 0 || compare_logical(c, candidate, person) == 0) continue;
        program.call(0x100caeb0, 0x100973b4, {candidate, text, 2000, 4});
        words({16, c.u16(secondary_ink), c.u16(highlight), 1, 1, c.u16(c.r(25)),
               text, 0, 3, c.u32(candidate), grid});
        c.set({{0, layout_offset() + 0x24}, {11, 0}, {12, 3}});
        program.call(0x10719740, 0x10097430, {c.u32(screens + c.r(0)), 2, 0, 0, 0, 0, 0, 0});
    }
}

void NegotiationScreen::draw_heading() {
    if (compare_logical(c, club) != 0)
        program.call(0x100c7a50, 0x10097474, {club, 1, frame.at(0x6e), frame.at(0x6c)});
    const auto name = frame.at(0x36c), box = frame.at(0xa8);
    program.call(0x100caeb0, 0x10097488, {person, name, 2000, 1});
    const auto employer = c.u32(person + 0x24);
    c.set(6, employer);
    if (compare_logical(c, employer) != 0) {
        program.call(0x1077b720, 0x100974a8, {text, strings + 0x215, name, employer + 0x221});
        program.call(text_set, 0x100974b4, {box, text});
        const auto live_employer = c.u32(person + 0x24);
        c.set(6, live_employer);
        program.call(0x1074bab0, 0x100974c8, {box, live_employer + 0x221, 0x7d5});
    } else {
        program.call(0x1077dbf0, 0x100974d8, {text, name});
        program.call(text_set, 0x100974e4, {box, text});
    }
    c.set(25, screens + 0x24);
    words({0, 0x30, c.u16(frame.at(0x6c)), 0xffffffff});
    const auto grid = program.call(grid_control, 0x10097534, {layout(), 100, 10, 790, 70, 1, 0, 1});
    c.set(0, 0);
    c.put32(frame.at(0x70), 0);
    words({0});
    program.call(0x1074bcd0, 0x10097564, {box, grid, 100, 790, frame.at(0x70), 7, c.u16(frame.at(0x6e)), 1});
    caption(0x10097570, 0x21d);
    program.call(0x107442f0, 0x1009757c, {screens, text});
    const auto option_slot = c.toc(-0x7a68);
    const auto active = c.u8(option_slot);
    c.set({{0, active}, {3, option_slot}});
    if (compare(c, active, 1) == 0 && compare_logical(c, read(0x10097598, 0x1d)) != 0) {
        program.call(localized_text, 0x100975b0, {text, strings + 0x22c, strings + 0x214});
        c.set({{0, 0}, {11, 0x31}});
        words({1, c.u16(ink), text, 0x31, 0});
        program.call(0x10744430, 0x100975fc, {screens, 730, 85, 785, 105, 0x30, c.u16(c.toc(-0x7bdc)), 12});
    }
}

void NegotiationScreen::draw_tabs() {
    c.put32(frame.at(0x80), 140);
    c.put32(frame.at(0x7c), 545);
    c.set(0, 545);
    if (player(0x10097614, c.u8(frame.at(0xb44)))) {
        c.set(14, 0);
        program.call(localized_text, 0x1009762c, {frame.at(0xf4), strings + 0x267});
        const auto command = frame.at(0x1bc), value = frame.at(0x1c0);
        c.set({{0, frame.at(0xf0)}, {5, 0}, {15, 1}, {19, 0xd4}, {23, value}, {26, command}});
        c.put32(command, 12);
        c.put32(value, 1);
        program.call(localized_text, 0x10097660, {frame.at(0x1c8), strings + 0x26d});
        c.set({{0, frame.at(0xf0)}, {5, 0xd4}, {14, 2}, {24, 0x1a8}});
        c.put32(command + 0xd4, 12);
        c.put32(value + 0xd4, 2);
        program.call(localized_text, 0x1009768c, {frame.at(0x29c), strings + 0x275});
        c.set(0, 0x1a8);
        c.put32(command + 0x1a8, 12);
        c.put32(value + 0x1a8, 3);
        program.call(0x1074aa90, 0x100976bc, {frame.at(0xf0), 3, std::uint32_t(tab),
                     frame.at(0x80), frame.at(0x7c), 1});
    }
    std::uint32_t target = 0, back = 0;
    if (compare(c, tab, 2) == 0) {
        detail::call_owned(c, 0x1009b620, 0x10097730, draw_negotiation_bonuses, draft, proposal,
                           person, c.u32(frame.at(0x80)), c.u32(frame.at(0x7c)));
        return;
    } else if (tab < 2 && compare(c, tab, 1) >= 0) {
        target = 0x10098130;
        back = 0x100976f8;
    } else if (tab >= 2 && compare(c, tab, 4) < 0) {
        target = 0x1009c870;
        back = 0x10097714;
    }
    if (target)
        program.call(target, back, {draft, proposal, person, c.u32(frame.at(0x80)), c.u32(frame.at(0x7c))});
    else {
        alert(0x10097748, 0xb0, 0x98, 0x659);
        c.set(3, c.toc(-0x7bac));
    }
}

void NegotiationScreen::draw_buttons() {
    program.call(0x102a1070, 0x10097794, {4, 0});
    const bool linked = compare_logical(c, link) != 0;
    bool can_review = linked;
    if (!linked) {
        const auto previous = pending(0x100978f4);
        if (compare_logical(c, previous) != 0) {
            const auto status = c.i8(previous + 0x64);
            c.set(0, std::uint32_t(status));
            can_review = compare(c, status, 4) != 0 && compare(c, status, 1) != 0;
        }
    }
    words({0, 1, c.u16(frame.at(0x6c)), 0xffffffff});
    const auto grid_back = linked ? 0x100977e8 : can_review ? 0x10097960 : 0x10097aa4;
    const auto grid = program.call(grid_control, grid_back, {layout(), 100, 555, 790, 590,
        can_review ? 3u : 2u, frame.at(can_review ? 0x84 : 0x68), 1});
    c.set(13, grid);

    struct Control { unsigned label, command; std::uint32_t localize, render; };
    constexpr std::array existing{Control{0x7b, 0x1b, 0x100977f8, 0x1009783c},
                                  Control{0x27d, 2, 0x10097848, 0x1009788c},
                                  Control{0x284, 1, 0x10097898, 0x100978dc}};
    constexpr std::array review{Control{0x7b, 0x1b, 0x10097970, 0x100979b4},
                                Control{0x27d, 2, 0x100979c0, 0x10097a04},
                                Control{0x284, 1, 0x10097a10, 0x10097a54}};
    constexpr std::array fresh{Control{0x27d, 0x1b, 0x10097ab4, 0x10097af8},
                               Control{0x284, 1, 0x10097b04, 0x10097b48}};
    const auto render = [&](auto controls) {
        unsigned column = 0;
        for (const auto& control : controls) {
            caption(control.localize, control.label);
            c.set(0, 0);
            words({c.u16(ink), text, control.command, 0});
            program.call(button, control.render, {screens, grid, column++, 0, 0x30, c.u16(c.toc(-0x7bc4)), 12, 3});
        }
    };
    if (linked) render(existing);
    else if (can_review) render(review);
    else render(fresh);
}
}

void draw_negotiation(Cpu& c) {
    Frame frame(c, 0x10096c60, 0xba0, {.first_gpr = 13});
    const auto screens = c.toc(-0x7bb0), date = c.toc(-0x7b80);
    NegotiationScreen ui{c, frame, Program{c}, screens, c.toc(-0x7bb8), c.toc(-0x56cc), c.toc(-0x7bc8),
                         c.toc(-0x7bcc), c.toc(-0x7bd0), c.toc(-0x7b78)};
    keep(c, {{14, ui.highlight}, {15, date}, {19, ui.contract_cache}, {25, c.toc(-0x7bd4)},
             {26, ui.secondary_ink}, {27, ui.ink}, {28, ui.strings}, {29, ui.text}, {30, screens}});
    c.put16(frame.at(0x84), c.u16(c.r(2) - 0x3a00));
    const auto style = c.u8(c.r(2) - 0x39fe);
    const auto standard = c.u16(c.r(2) - 0x39fc);
    c.put8(frame.at(0x86), style);
    c.put16(frame.at(0x68), standard);
    for (const auto at : {0xf0, 0x1c4, 0x298}) {
        c.put16(frame.at(at), 0xffff);
        c.put16(frame.at(at + 2), 0);
    }
    c.set({{0, 0}, {5, 0xffffffff}, {6, standard}, {7, style}});
    ui.program.call(text_init, 0x10096cd8, {frame.at(0xa8), 0});
    c.set(16, 0xffffffff);
    const auto person_offset = ui.read(0x10096ce8, 0) * 0x70;
    const auto people = c.u32(c.toc(-0x7be0));
    ui.person = people + person_offset;
    c.set({{0, people}, {5, person_offset}, {21, ui.person}});
    ui.link = ui.read(0x10096d04, 3);
    c.set(13, ui.link);
    std::int32_t offer_id = -1;
    if (compare(c, ui.link) != 0) {
        const auto offer = c.u32(ui.link);
        c.set(3, offer);
        if (compare_logical(c, offer) != 0) {
            offer_id = std::int32_t(c.u32(offer + 0x28));
            c.set(16, std::uint32_t(offer_id));
        }
    }
    const auto role = std::int8_t(ui.read(0x10096d28, 8));
    c.set(0, std::uint32_t(role));
    const bool has_person = compare_logical(c, ui.person) != 0;
    c.put8(frame.at(0xb44), std::uint8_t(role));
    if (!has_person) {
        ui.alert(0x10096d4c, 0xd0, 0xa0, 0x567);
        ui.dispose(0x10096d98);
        return;
    }
    ui.proposal = ui.read(0x10096da8, 2);
    const auto offset = ui.layout_offset();
    const auto instance = c.u32(screens + offset + 4);
    ui.club = c.u32(instance + 0x24);
    c.set({{0, offset}, {5, instance}, {18, ui.proposal}, {20, ui.club}, {22, screens + 0x2c58}});
    ui.draft = ui.program.call(0x101487d0, 0x10096dd4, {c.u32(c.toc(-0x7a54)), 0});
    c.set(17, ui.draft);
    if (compare_logical(c, ui.proposal) == 0) ui.initialize_offer(offer_id, date);
    ui.tab = std::int8_t(ui.read(0x10097218, 1));
    c.set(16, std::uint32_t(ui.tab));
    ui.draw_comparisons();
    ui.draw_heading();
    ui.draw_tabs();
    ui.draw_buttons();
    ui.dispose(0x10097b54);
}
}
