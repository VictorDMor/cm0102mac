#include "contract_screens.hpp"
#include "screen_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_screens {
using namespace guest;

void draw_transfer_request(Cpu& c) {
    using screen_manager::detail::Manager;
    using screen_manager::detail::Program;
    Frame frame(c, 0x10093bd0, 0x17e0, {.first_gpr = 25});
    Kept<31> screens(c, c.toc(-0x7bb0));
    Kept<30> text(c, c.toc(-0x7bb8));
    Kept<29> strings(c, c.toc(-0x56cc));
    Kept<28> foreground(c, c.toc(-0x7bc8));
    Kept<27> disabled(c, c.toc(-0x7bc4));
    const auto button_style = frame.at(0x4c);
    const auto style_byte = c.u8(c.r(2) - 0x3a0a);
    c.put16(button_style, c.u16(c.r(2) - 0x3a0c));
    c.put8(button_style + 2, style_byte);
    c.set(0, style_byte);

    Program program{c};
    Kept<26> club_or_grid(c, program.call(0x1073ef70, 0x10093c10, {screens, 2}));
    const auto person_id = program.call(0x1073ef70, 0x10093c20, {screens, 3});
    const auto person_offset = person_id * 0x70, club_offset = club_or_grid * 0x248;
    const auto people = c.u32(c.toc(-0x7be0)), clubs = c.u32(c.toc(-0x7be8));
    Kept<25> staff(c, people + person_offset);
    club_or_grid = clubs + club_offset;
    c.set({{0, person_offset}, {6, club_offset}, {7, clubs}});
    program.call(0x107136d0, 0x10093c50, {staff, 0, 0});
    program.call(0x102a1070, 0x10093c5c, {4, 0});
    program.call(0x100c7a50, 0x10093c70, {club_or_grid, 1, frame.at(0x4a), frame.at(0x48)});
    program.call(0x100caeb0, 0x10093c84, {staff, text, 2000, 1});
    const auto subtitle = frame.at(0xff0), title = frame.at(0x820), question = frame.at(0x50);
    program.call(0x10727890, 0x10093c94, {subtitle, strings, text});
    program.call(0x10744250, 0x10093ca8, {screens, c.u16(frame.at(0x48)), c.u16(frame.at(0x4a)), subtitle});
    program.call(0x10727890, 0x10093cb4, {title, strings + 0x1e});
    program.call(0x107442f0, 0x10093cc0, {screens, title});
    program.call(0x10727890, 0x10093cd0, {question, strings + 0x2f, text});

    const auto stack_words = [&](std::initializer_list<std::uint32_t> words) {
        auto address = frame.at(0x38);
        for (const auto word : words) { c.put32(address, word); address += 4; }
    };
    stack_words({c.u16(foreground), question, 0});
    c.set(0, 0);
    program.call(0x1074a350, 0x10093d0c, {110, 145, 780, 535, 2, 0, 16, 4});

    const Manager screen{c, screens};
    stack_words({0, 1, 0, 0xffffffff});
    const auto group_offset = std::uint32_t(screen.current().get()) * 0x2c0;
    c.set(0, group_offset);
    club_or_grid = program.call(0x107198c0, 0x10093d58, {c.u32(screens + group_offset + 0x24),
        100, 555, 790, 590, 3, button_style, 1});

    struct Button { unsigned column, command, label; std::uint32_t localized, drawn; };
    constexpr std::array buttons{Button{0, 0x22, 0x7b, 0x10093d68, 0x10093da8},
        Button{1, 0x21, 0x80, 0x10093db4, 0x10093df4}, Button{2, 0x20, 0x83, 0x10093e00, 0x10093e40}};
    for (const auto& button : buttons) {
        program.call(0x10727890, button.localized, {text, strings + button.label});
        stack_words({c.u16(foreground), text, button.command, 0});
        c.set(0, 0);
        program.call(0x107444e0, button.drawn, {screens, club_or_grid, button.column, 0, 0x30, c.u16(disabled), 12, 3});
    }
}
}
