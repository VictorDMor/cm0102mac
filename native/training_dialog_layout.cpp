#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
namespace {
// Both training dialogs use the same shell. Grids 0..2 are the backdrop,
// panel and footer. Each field grid covers the same pitch, while any popup
// grids after them are positioned by the original picker when they open.
void layout_training_dialog(State& s, const GuestScreen& screen, const FormLayout& layout, GuiCanvas canvas,
                            ScreenRect original_title, ScreenRect pitch, unsigned field_grids) {
    auto& c = screen.c;
    const auto footer = screen.grid(2);
    const auto position = [](int value, int origin, int length, int start, int extent) {
        return start + ((value - origin) * extent + length / 2) / length;
    };
    const auto x = [&](int v) { return position(v, pitch.left, pitch.width(), layout.body.left, layout.body.width()); };
    const auto y = [&](int v) { return position(v, pitch.top, pitch.height(), layout.body.top, layout.body.height()); };

    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    const auto controls = std::min<unsigned>(1200, c.u16(screen.display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        const auto old = screen.rect(control + 0x10);
        ScreenRect box;
        if (c.i16(control + 0x72) == -1 && old == original_title) {
            box = layout.title;
        } else if (c.i16(control + 0x72) == 2 && !c.u32(control + 8) &&
                   old.left >= pitch.left && old.right <= pitch.right && old.top >= pitch.top && old.bottom <= pitch.bottom) {
            // The original pitch markings belong to the footer grid, but each
            // has an absolute rectangle that overrides its grid cell.
            box = {x(old.left), y(old.top), x(old.right), y(old.bottom)};
        } else continue;
        screen.write(control + 0x10, box);
        c.put32(control + 0x28, box.left); c.put32(control + 0x2c, box.right);
        c.put32(control + 0x30, box.top); c.put32(control + 0x34, box.bottom);
    }

    c.put8(footer + 0xbb2, 1);
    c.put8(footer + 0xbb3, 1);
    const auto apply = [&](unsigned index, ScreenRect box) {
        const auto area = screen.grid(index);
        screen.write(area, box);
        screen.measure(area);
        screen.publish(area);
    };
    apply(0, {0, 0, canvas.width - 1, canvas.height - 1});
    apply(1, layout.panel);
    apply(2, layout.footer);
    for (unsigned i = 0; i < field_grids; ++i) apply(3 + i, layout.fields);
}
}

void layout_training_coaches(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const auto matrix = screen.grid(3);
    if (c.u16(display + 0x129da4) != 4 || screen.columns(screen.grid(2)) != 2 ||
        screen.columns(matrix) != 6 || screen.rows(matrix) != 6) return;
    const auto layout = form_layout({24, 20, canvas.width - 24, canvas.height - 20},
        {.maximum_width = 1120, .maximum_height = 400, .toolbar_height = 0, .footer_width = 340});
    c.put8(matrix + 0xbb2, 14);
    layout_training_dialog(s, screen, layout, canvas, {130, 150, 759, 195}, {140, 215, 749, 399}, 1);
}

void layout_training_schedule(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    if (c.u16(display + 0x129da4) < 6 || screen.columns(screen.grid(2)) != 2 ||
        screen.columns(screen.grid(3)) != 4 || screen.columns(screen.grid(4)) != 5 ||
        screen.columns(screen.grid(5)) != 5) return;
    const auto layout = form_layout({24, 20, canvas.width - 24, canvas.height - 20},
        {.maximum_width = 1000, .maximum_height = 480, .toolbar_height = 0, .footer_width = 340});
    // Align the editable name with the first intensity and give it the
    // width of three intensity choices. All three grids total 34 units.
    const auto name_weights = screen.grid(3) + 0xbb2;
    constexpr std::array<unsigned char, 4> weights{9, 1, 18, 6};
    for (unsigned i = 0; i < weights.size(); ++i) c.put8(name_weights + i, weights[i]);
    layout_training_dialog(s, screen, layout, canvas, {170, 135, 719, 180}, {180, 200, 709, 414}, 3);
}
}
