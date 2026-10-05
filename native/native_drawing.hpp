#pragma once
#include "aot_runtime.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <memory>

namespace cm::aot {
// Presentation-only display list. The original renderer continues to write its
// own buffers; none of these objects is visible to the game or serialized.
struct DrawingRect {
    int x = 0, y = 0, width = 0, height = 0;
    bool empty() const { return width <= 0 || height <= 0; }
};
struct OutlinePoint { double x, y; bool on_curve; };
struct NativeOutline { std::vector<std::vector<OutlinePoint>> contours; };
struct NativeShape {
    enum class Kind { glyph, line, fill, ellipse } kind = Kind::glyph;
    DrawingRect bounds;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::uint16_t color = 0;
    unsigned font = 1, character = 0, style = 0;
    std::shared_ptr<const NativeOutline> outline;
    double origin_x = 0, baseline = 0;
    std::vector<DrawingRect> clips;
};
struct NativeDrawing {
    int width = 0, height = 0;
    std::vector<std::uint8_t> background;
    std::vector<NativeShape> shapes;
};
// Draw directly into the supplied backing context, in logical top-left units.
void draw_native_shapes(CGContextRef context, const NativeDrawing& drawing);

class NativeDrawingObserver final : public MemoryObserver {
public:
    NativeDrawingObserver();
    ~NativeDrawingObserver();
    void register_surface(State&, std::uint32_t address, int width, int height, unsigned row_bytes, unsigned depth);
    void remove_surface(std::uint32_t address);
    void boundary(State&, std::uint32_t pc);
    void copy_surface(State&, std::uint32_t source, std::uint32_t destination,
                      DrawingRect from, int x, int y, const std::vector<std::uint8_t>& mask = {}, int mask_x = 0, int mask_y = 0);
    std::shared_ptr<const NativeDrawing> snapshot(State&, std::uint32_t address);
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override;
    void cleared(std::uint32_t, std::uint64_t) override;
    // CopyBits itself still runs. Its pixel writes must not bake the original
    // glyphs back into the clean presentation background.
    bool copying = false;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
void native_drawing_boundary(State&, std::uint32_t pc);
}
