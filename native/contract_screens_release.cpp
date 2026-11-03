#include "contract_screens.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;

constexpr int screen_manager_slot = -0x7bb0, strings_slot = -0x56cc, text_slot = -0x7bb8,
              foreground_slot = -0x7bc8, disabled_slot = -0x7bc4, title_style_slot = -0x7b14;
constexpr std::uint32_t history_value = 0x1073ef70, localize = 0x10727890, add_button = 0x107444e0,
                        human_club = 0x1014c2e0;

struct Staff {
    Cpu& cpu;
    std::uint32_t address;
    std::uint32_t club() const { return cpu.u32(address + 0x24); }
    std::int8_t role() const { return cpu.i8(address + 0x62); }
};

std::int16_t action(Cpu& c, std::uint32_t screens, std::uint32_t layout, std::int16_t control) {
    std::int16_t selected = 0;
    if (compare(c, control, -1) != 0) {
        c.set(3, layout + 0xc0000 + std::uint32_t(control) * 0x17c);
        selected = std::int16_t(c.u32(c.r(3) - 0x5794));
    }
    if (compare(c, selected) == 0) {
        c.set(3, screens + 0x260000);
        selected = c.i16(c.r(3) - 0x7e62);
    }
    c.set(0, std::uint32_t(selected));
    return selected;
}
}

void draw_staff_release(Cpu& c) {
    Frame frame(c, 0x1009e8f0, 0x1a0, {.first_gpr = 24});
    Kept<31> screens(c, c.toc(screen_manager_slot));
    Kept<30> text(c, c.toc(text_slot));
    Kept<29> strings(c, c.toc(strings_slot));
    Kept<28> foreground(c, c.toc(foreground_slot));
    Kept<27> disabled(c, c.toc(disabled_slot));
    Kept<26> panel(c, screens + 0x2c58);
    const Manager screen{c, screens};
    const auto group_offset = std::uint32_t(screen.current().get()) * 0x2c0;
    const auto instance = c.u32(screens + group_offset + 4);
    const auto title_style = c.u16(c.toc(title_style_slot)), ink = c.u16(foreground),
               border = c.u16(c.r(2) - 0x39f0);
    c.put16(frame.at(0x4e), title_style);
    c.put16(frame.at(0x4c), ink);
    c.put16(frame.at(0x48), border);
    Kept<25> layout(c, c.u32(instance + 0x24));
    c.set({{0, group_offset}, {5, instance}, {6, border}, {7, ink}});
    Program program{c};
    const auto staff = program.call(history_value, 0x1009e954, {screens, 0});
    c.set(24, staff);
    program.call(0x107136d0, 0x1009e964, {staff, 0, 0});
    program.call(0x100c7a50, 0x1009e978, {layout, 1, frame.at(0x4e), frame.at(0x4c)});
    const auto name = frame.at(0x50);
    program.call(0x100caeb0, 0x1009e98c, {staff, name, 0x12c, 1});
    program.call(0x10744250, 0x1009e9a0, {screens, c.u16(frame.at(0x4c)), c.u16(frame.at(0x4e)), name});
    program.call(localize, 0x1009e9ac, {text, strings + 0x144e});
    program.call(0x107442f0, 0x1009e9b8, {screens, text});

    const auto line_height = program.call(0x1076d470, 0x1009e9c0, {3});
    const auto lines = divide_signed<4>(405, line_height);
    Kept<24> row_count(c, std::uint32_t(std::int8_t(lines)));
    c.set(0, lines);
    program.call(localize, 0x1009e9e0, {text, strings + 0x1461, 100, name});
    const auto stack_words = [&](std::initializer_list<std::uint32_t> words) {
        auto address = frame.at(0x38);
        for (const auto word : words) { c.put32(address, word); address += 4; }
    };
    stack_words({c.u16(foreground), text, 0});
    c.set(0, 0);
    program.call(0x1074a350, 0x1009ea18, {110, 140, 780, 545, 2, 0, row_count, 3});
    program.call(0x102a1070, 0x1009ea24, {4, 0});

    stack_words({0, 1, c.u16(frame.at(0x4c)), 0xffffffff});
    c.set(0, std::uint32_t(c.i16(panel)) * 0x2c0);
    panel = program.call(0x107198c0, 0x1009ea74, {c.u32(screens + c.r(0) + 0x24),
        100, 555, 790, 590, 2, frame.at(0x48), 1});
    program.call(localize, 0x1009ea84, {text, strings + 0x80});
    stack_words({c.u16(foreground), text, 2, 0});
    c.set(0, 0);
    program.call(add_button, 0x1009eac4, {screens, panel, 0, 0, 0x30, c.u16(disabled), 12, 3});
    program.call(localize, 0x1009ead0, {text, strings + 0x83});
    stack_words({c.u16(foreground), text, 1, 0});
    c.set(0, 0);
    program.call(add_button, 0x1009eb10, {screens, panel, 1, 0, 0x30, c.u16(disabled), 12, 3});
}

std::uint32_t staff_release_command(Cpu& c, std::uint32_t control) {
    Frame frame(c, 0x1009eb30, 0x50, {.first_gpr = 29});
    Kept<31> pressed(c, control);
    const auto screens = c.toc(screen_manager_slot);
    c.set(4, screens);
    const Manager screen{c, screens};
    const auto group = screen.group();
    const auto layout = c.u32(group + 0x24);
    c.set({{3, group}, {5, layout}});
    const auto command = action(c, screens, layout, std::int16_t(pressed));
    Program program{c};
    if (compare(c, command, 1) == 0) {
        pressed = program.call(history_value, 0x1009ebb0, {screens, 0});
        const Staff staff{c, pressed};
        const auto contracts = c.toc(-0x7b7c);
        c.set(5, contracts);
        Kept<30> role(c, std::uint8_t(staff.role()));
        Kept<29> club(c, staff.club());
        program.call(0x10084a80, 0x1009ebcc, {c.u32(contracts), staff.address});
        program.call(0x100c8930, 0x1009ebe4, {staff.club(), staff.address, 0, 1, 0});
        program.call(0x103e68c0, 0x1009ec04, {c.u32(c.toc(-0x7b40)), staff.address, club, 0, 0, 1});
        const auto old_role = std::int8_t(role);
        c.set(0, std::uint32_t(old_role));
        const auto human = [&](std::uint32_t back) {
            const auto answer = std::int8_t(program.call(human_club, back, {club, 1, 1, 0, 0}));
            c.set(0, std::uint32_t(answer));
            return compare(c, answer) != 0;
        };
        if (compare(c, old_role, 9) == 0) {
            compare(c, old_role);  // The original tests the same role byte against zero.
            if (human(0x1009ec30))
                program.call(0x1030f950, 0x1009ec48, {c.u32(c.toc(-0x7b1c)), c.u32(club + 0x90)});
        }
        if (compare_logical(c, club) != 0 && human(0x1009ec68))
            program.call(0x103d2b30, 0x1009ec84, {c.u32(c.toc(-0x7a7c)), club, 0});
        return std::uint32_t(-10);
    }

    // No service runs between the original's two reads of the action.
    if (compare(c, command, 2) == 0) return std::uint32_t(-9);
    const auto handled = program.call(0x102a54b0, 0x1009ece0, {pressed});
    if (compare(c, handled) != 0) return handled;
    const auto common = program.call(0x102a83a0, 0x1009ecf4, {pressed});
    // The original masks this with the arithmetic sign of value | -value.
    c.s.wr<1>(reg::xer_ca, common != 0 && common != 0x80000000);
    return common;
}
}
