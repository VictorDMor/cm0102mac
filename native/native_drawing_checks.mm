#include "native_drawing.hpp"
#include "regions.hpp"
#include <iostream>
#include <algorithm>
using namespace cm::aot;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
constexpr std::uint32_t pixels = 0x33000000, fonts = 0x34000000, mode = 0x35000000, text = 0x35000100, glyph = 0x35000200;
void write_range_checks() {
    State state;
    constexpr auto base = 0x32000000u;
    state.memory.map(base, 64);
    NativeDrawingObserver drawing;
    drawing.register_surface(state, base + 16, 1, 1, 2, 16);
    state.memory.write<2>(base + 16, 0x7c00);
    require(drawing.snapshot(state, base + 16)->background[1] == 255, "Initial surface lost its scalar repaint");
    drawing.register_surface(state, base + 40, 1, 1, 2, 16);
    state.memory.write<2>(base + 40, 0x03e0);
    require(drawing.snapshot(state, base + 40)->background[2] == 255, "Higher surface did not extend write interest");
    drawing.register_surface(state, base + 4, 1, 1, 2, 16);
    state.memory.write<2>(base + 4, 0x001f);
    require(drawing.snapshot(state, base + 4)->background[3] == 255, "Lower surface did not extend write interest");
    state.memory.write<4>(base + 2, 0xaabb7fff);
    const auto scene = drawing.snapshot(state, base + 4);
    require(scene->background[1] == 255 && scene->background[2] == 255 && scene->background[3] == 255,
            "A partially overlapping scalar write lost the surface prefix");
    drawing.remove_surface(base + 40);
    state.memory.write<2>(base + 40, 0x7fff);
    require(!drawing.snapshot(state, base + 40), "Removed surface was retained");
    state.memory.observer = nullptr;
}
void setup(State& s, const std::filesystem::path& program) {
    load_program(s, program);
    s.memory.map(pixels, 4096); s.memory.map(fonts, 0x1404); s.memory.map(mode, 4096);
    const auto toc = s.r<4>(8);
    s.memory.write<4>(toc - 0x3564, pixels); s.memory.write<2>(toc - 0x3568, 32); s.memory.write<2>(toc - 0x3566, 64);
    s.memory.write<1>(toc - 0x3550, 0); s.memory.write<4>(toc - 0x740c, mode); s.memory.write<4>(mode, UINT32_MAX);
    s.memory.write<4>(toc - 0x3bdc, fonts); s.memory.write<4>(fonts, 8);
    s.memory.write<4>(fonts + 65 * 20 + 4, 8); s.memory.write<4>(fonts + 65 * 20 + 20, glyph);
    s.memory.write<4>(fonts + 32 * 20 + 4, 4);
    s.memory.copy_in(glyph, std::vector<std::uint8_t>(32, 255));
    s.memory.copy_in(text, {'A', 0});
    for (unsigned i = 0; i < 32 * 32; ++i) s.memory.write<2>(pixels + i * 2, 0x001f);
    s.wr<4>(12, 3); s.wr<4>(16, 4); s.wr<4>(20, 1); s.wr<4>(24, 0x7fff); s.wr<4>(28, text); s.wr<4>(32, UINT32_MAX);
    s.pc = 0x1076ca50; s.budget = 1000000;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-native-drawing-checks ORIGINAL_PROGRAM");
        write_range_checks();
        auto original = std::make_unique<State>(); auto observed = std::make_unique<State>();
        setup(*original, argv[1]); setup(*observed, argv[1]);
        NativeDrawingObserver observer;
        observer.register_surface(*observed, pixels, 32, 32, 64, 16);
        const auto registers = observed->registers;
        observer.boundary(*observed, 0x1076ca50);
        require(observed->registers == registers && observed->executed == 0, "Observer changed original execution state");
        run(*original); run(*observed);
        observer.boundary(*observed, 0x1076d194);
        require(original->memory.copy_out(pixels, 2048) == observed->memory.copy_out(pixels, 2048), "Native text changed original pixels");
        require(original->registers == observed->registers && original->executed == observed->executed, "Native text changed original ABI/instruction behavior");
        auto scene = observer.snapshot(*observed, pixels);
        require(scene && scene->shapes.size() == 1, "Text glyph not retained");
        require(scene->shapes[0].bounds.x == 3 && scene->shapes[0].bounds.y == 4, "Original glyph position changed");
        require(scene->background[(4 * 32 + 3) * 4 + 3] == 255 && scene->background[(4 * 32 + 3) * 4 + 1] == 0, "Original bitmap glyph leaked into clean background");
        // Same-value writes still occlude native ink; a frame-difference approach
        // would incorrectly retain the glyph and fail this control.
        for (int y = 4; y < 12; ++y) for (int x = 3; x < 11; ++x) observed->memory.write<2>(pixels + (y * 32 + x) * 2, 0x7fff);
        require(observer.snapshot(*observed, pixels)->shapes.empty(), "Same-value raster repaint left a stale vector glyph");
        setup(*observed, argv[1]);
        observer.register_surface(*observed, pixels, 32, 32, 64, 16);
        observer.boundary(*observed, 0x1076ca50); observer.boundary(*observed, 0x1076d194);
        constexpr std::uint32_t target = pixels + 2048;
        observer.register_surface(*observed, target, 16, 16, 64, 32);
        observer.copy_surface(*observed, pixels, target, {2, 3, 12, 12}, 0, 0);
        const auto copied = observer.snapshot(*observed, target);
        require(copied->shapes.size() == 1 && copied->shapes[0].bounds.x == 1 && copied->shapes[0].bounds.y == 1, "CopyBits lost or shifted vector glyph");
        // A port's mask uses original global bounds, while its backing is local.
        const std::array<LegacyRect, 1> mask_rect{{{21, 11, 25, 15}}};
        const auto port_mask = region_union(mask_rect);
        observer.register_surface(*observed, target, 16, 16, 64, 32);
        observer.copy_surface(*observed, pixels, target, {2, 3, 12, 12}, 0, 0, port_mask, 10, 20);
        const auto masked = observer.snapshot(*observed, target);
        require(masked->shapes.size() == 1 && masked->shapes[0].clips.front().x == 1 &&
                masked->shapes[0].clips.front().width == 4, "CopyBits mask lost nonzero port origin");
        // Saved-under dialogs and the original startup fade must retain curves.
        auto& state = *observed;
        state.wr<4>(12, 0); state.wr<4>(16, 0); state.wr<4>(20, 31); state.wr<4>(24, 31);
        observer.boundary(state, 0x1076b020);
        state.memory.map(0x36000000, 2048);
        state.memory.copy_in(0x36000000, state.memory.copy_out(pixels, 2048));
        state.memory.write<4>(0x35000900, 32); state.memory.write<4>(0x35000904, 32);
        state.memory.write<4>(0x3500090c, 0x36000000);
        state.wr<4>(12, 0x35000900); observer.boundary(state, 0x1076b138);
        state.wr<4>(28, 10); observer.boundary(state, 0x10769460);
        state.wr<4>(104, 0x35000900); state.wr<4>(112, 5); state.wr<4>(120, 10);
        state.memory.write<2>(state.r<4>(4) + 0x44, 0); state.memory.write<2>(state.r<4>(4) + 0x46, 0);
        // Simulate the actual fade loop's destructive original raster writes.
        for (unsigned i = 0; i < 1024; ++i) state.memory.write<2>(pixels + i * 2, 0x000f);
        observer.boundary(state, 0x107696cc);
        auto faded = observer.snapshot(state, pixels);
        require(faded->shapes.size() == 1 && faded->shapes[0].color == 0x3def, "Fade flattened or miscolored native text");
        state.wr<4>(112, 10); observer.boundary(state, 0x107696cc); observer.boundary(state, 0x107697b0);
        require(observer.snapshot(state, pixels)->shapes[0].color == 0x7fff, "Fade did not restore original text color");
        state.wr<4>(12, 0); state.wr<4>(16, 0); state.wr<4>(20, 31); state.wr<4>(24, 31);
        observer.boundary(state, 0x10769980);
        state.memory.write<2>(pixels + (4 * 32 + 3) * 2, 0x4a52);
        observer.boundary(state, 0x10769b48);
        require(observer.snapshot(state, pixels)->shapes[0].color == 0x4a52, "Modal dimming lost native text");
        state.wr<4>(12, 0); state.wr<4>(16, 0); state.wr<4>(20, 0x35000900);
        observer.boundary(state, 0x1076b140); observer.boundary(state, 0x1076b234);
        require(observer.snapshot(state, pixels)->shapes[0].color == 0x7fff, "Saved-under restoration retained modal dimming");
        // A reused original image must invalidate the saved native scene.
        state.memory.write<2>(0x36000000, 0x1234);
        observer.boundary(state, 0x1076b140);
        for (unsigned i = 0; i < 1024; ++i) state.memory.write<2>(pixels + i * 2, 0x1234);
        observer.boundary(state, 0x1076b234);
        require(observer.snapshot(state, pixels)->shapes.empty(), "Reused original image restored obsolete vector text");
        // Restore a fresh glyph for independent backing-resolution checks.
        state.wr<4>(12, 3); state.wr<4>(16, 4); state.wr<4>(20, 1); state.wr<4>(24, 0x7fff);
        state.wr<4>(28, text); observer.boundary(state, 0x1076ca50); observer.boundary(state, 0x1076d194);
        const auto original_bytes = observed->memory.copy_out(pixels, 2048);
        // The physical rasterizer must resolve new edge coverage at each scale.
        std::vector<unsigned> edge_counts;
        for (unsigned scale : {1u, 2u, 4u}) {
            const auto width = 32 * scale, height = 32 * scale;
            std::vector<std::uint8_t> pixels_out(width * height * 4, 0);
            auto color = CGColorSpaceCreateDeviceRGB();
            auto ctx = CGBitmapContextCreate(pixels_out.data(), width, height, 8, width * 4, color, kCGImageAlphaPremultipliedLast);
            require(ctx, "Cannot create verification backing");
            CGContextTranslateCTM(ctx, 0, height); CGContextScaleCTM(ctx, scale, -double(scale));
            draw_native_shapes(ctx, *observer.snapshot(*observed, pixels));
            unsigned edges = 0;
            for (std::size_t i = 3; i < pixels_out.size(); i += 4) if (pixels_out[i] && pixels_out[i] != 255) ++edges;
            edge_counts.push_back(edges);
            CGContextRelease(ctx); CGColorSpaceRelease(color);
        }
        require(edge_counts[0] > 0 && edge_counts[2] > edge_counts[1], "Native glyph rasterization did not add high-resolution edge detail");
        require(observed->memory.copy_out(pixels, 2048) == original_bytes, "Backing redraw mutated game memory");
        // Capture a synthetic original quadratic contour at the rasterizer
        // boundary, then reuse its glyph cache entry with the original metrics.
        constexpr auto record = mode + 0x300, starts = mode + 0x500, ends = mode + 0x502;
        constexpr auto xs = mode + 0x510, ys = mode + 0x530, flags = mode + 0x550, mask = mode + 0x600;
        observer.boundary(state, 0x10750580); observer.boundary(state, 0x10750740);
        state.memory.write<2>(starts, 0); state.memory.write<2>(ends, 2);
        const int vx[]{0, 256, 512}, vy[]{0, 512, 0};
        for (unsigned i = 0; i < 3; ++i) { state.memory.write<4>(xs + i * 4, vx[i]); state.memory.write<4>(ys + i * 4, vy[i]); state.memory.write<1>(flags + i, i != 1); }
        state.wr<4>(16, 1); state.wr<4>(20, starts); state.wr<4>(24, ends);
        state.wr<4>(28, xs); state.wr<4>(32, ys); state.wr<4>(36, flags);
        observer.boundary(state, 0x107620d0);
        state.wr<4>(12, record); state.memory.write<4>(record + 20, mask);
        observer.boundary(state, 0x107509f0);
        state.wr<4>(28, 0x200); state.wr<4>(32, 10); state.wr<4>(36, 0x7fff);
        observer.boundary(state, 0x1074f800);
        state.memory.write<4>(record, 0); state.memory.write<4>(record + 4, 512);
        state.memory.write<4>(record + 8, 8); state.memory.write<4>(record + 12, 8); state.memory.write<4>(record + 16, 8);
        state.memory.copy_in(mask, std::vector<std::uint8_t>(64, 255));
        state.memory.write<4>(state.r<4>(4) + 0xd4, text);
        state.wr<4>(0, 3); state.wr<4>(16, 4); state.wr<4>(32, 3); state.wr<4>(20, 4);
        state.wr<4>(28, 10); state.wr<4>(40, 11); state.wr<4>(120, record); state.wr<4>(124, 0);
        observer.boundary(state, 0x1074fb58); observer.boundary(state, 0x107500b4); observer.boundary(state, 0x107500e8);
        const auto curves = observer.snapshot(state, pixels);
        require(curves->shapes.back().outline && curves->shapes.back().outline->contours[0][1].x == 4 &&
                !curves->shapes.back().outline->contours[0][1].on_curve && curves->shapes.back().baseline == 12,
                "Original quadratic outline or glyph baseline was lost");
        observed->memory.observer = nullptr;
        std::cout << "Native drawing: original pixels/ABI, clean text background, same-value occlusion, CopyBits, saved dialogs, fades, original outlines, and 1x/2x/4x vector edges passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
