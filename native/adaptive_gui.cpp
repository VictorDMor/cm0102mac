#include "adaptive_gui.hpp"
#include "guest_abi.hpp"
#include "platform.hpp"
#include "new_screens.hpp"
#include <algorithm>
#include <vector>

namespace cm::aot {
bool adaptive_screen_enabled(std::uint32_t builder) {
    // Register future adapted screens here, behind the same user preference.
    return new_screens_enabled() && (builder == 0x103b3c50 || builder == 0x103baec0);
}
GuiCanvas gui_canvas(const State& s) {
    return s.platform ? s.platform->gui_canvas_size() : GuiCanvas{};
}
bool adaptive_gui_instruction(State& s, std::uint32_t pc) {
    const auto canvas = gui_canvas(s);
    if (canvas.original()) return false;
    s.step(pc);
    guest::Cpu c(s);
    const auto right = std::uint32_t(canvas.width - 1), bottom = std::uint32_t(canvas.height - 1);
    const auto subtract = [&](unsigned reg, unsigned from, std::uint32_t limit) {
        const auto result = std::uint64_t(~c.r(from)) + limit + 1;
        c.set(reg, std::uint32_t(result));
        s.wr<1>(guest::reg::xer_ca, result >> 32);
    };
    switch (pc) {
    case 0x10718f8c: case 0x10718fa0: case 0x10719074:
    case 0x1071929c: case 0x107192c8: case 0x10719318: c.set(5, right); break;
    case 0x10718f90: case 0x10718fa4: case 0x10719078:
    case 0x107192a0: case 0x107192cc: case 0x1071931c: c.set(6, bottom); break;
    case 0x10713e1c: case 0x10714f88:
    case 0x10725b04: case 0x1071b074: c.compare(0, std::int32_t(c.r(0)), std::int32_t(right)); break;
    case 0x10713e28: case 0x10725b10: c.set(0, right); break;
    case 0x10713e34: case 0x10715054:
    case 0x10725b1c: case 0x1071b0a0: c.compare(0, std::int32_t(c.r(0)), std::int32_t(bottom)); break;
    case 0x10713e40: case 0x10725b28: c.set(0, bottom); break;
    case 0x10714d00: case 0x10714dbc: case 0x10714e34:
        c.compare(0, std::int32_t(c.r(0)), canvas.height); break;
    case 0x10714df4: c.compare(0, std::int32_t(c.r(0)), canvas.width / 2); break;
    case 0x10714e84: c.compare(0, std::int32_t(c.r(6)), canvas.width / 2); break;
    case 0x10714fe4: subtract(28, 0, right); break;
    case 0x107150b0: subtract(27, 0, bottom); break;
    case 0x1071b07c: subtract(27, 4, right); break;
    case 0x1071b0a8: subtract(30, 5, bottom); break;
    case 0x1076add4: c.set(3, canvas.width); break;
    case 0x1076ae34: c.set(4, canvas.width); break;
    case 0x10769c40: subtract(3, 4, canvas.width); break;
    case 0x1074b0c4: c.set(5, canvas.height - 45); break;
    case 0x1074b0cc: c.set(7, canvas.height - 10); break;
    case 0x1074b100: c.set(6, canvas.width - 10); break;
    default: return false;
    }
    return true;
}
void draw_adaptive_background(State& s) {
    const auto canvas = gui_canvas(s);
    if (canvas.original()) return;
    guest::Cpu c(s);
    const auto image = c.u32(c.r(24) + 0x12a928);
    if (!image) return;
    const auto width = c.i32(image), height = c.i32(image + 4);
    if (width <= 0 || height <= 0 || width > 3200 || height > 1800) return;
    // Full-canvas captures already carry the observer's original vector
    // glyphs. Repainting their bitmap would flatten those contours.
    if (width == canvas.width && height == canvas.height) return;
    const auto pixels = c.u32(image + 12), destination = c.u32(c.r(2) - 0x3564);
    const auto stride = c.u16(c.r(2) - 0x3566);
    if (!pixels || !destination || stride < canvas.width * 2) return;
    const auto source = s.memory.copy_out(pixels, std::uint64_t(width) * height * 2);
    std::vector<std::uint8_t> row(std::size_t(canvas.width) * 2);
    // Cover the extra canvas with the selected original image, keeping its
    // aspect ratio. Only the background is sampled; text/control geometry
    // and original font contours are rendered afterwards at their own size.
    const double scale = std::max(double(canvas.width) / width, double(canvas.height) / height);
    const double offset_x = (width * scale - canvas.width) / 2;
    const double offset_y = (height * scale - canvas.height) / 2;
    for (int y = 0; y < canvas.height; ++y) {
        const auto sy = std::clamp(int((y + offset_y) / scale), 0, height - 1);
        for (int x = 0; x < canvas.width; ++x) {
            const auto sx = std::clamp(int((x + offset_x) / scale), 0, width - 1);
            const auto from = (std::size_t(sy) * width + sx) * 2;
            row[x * 2] = source[from]; row[x * 2 + 1] = source[from + 1];
        }
        s.memory.copy_in(destination + y * stride, row);
    }
}
}
