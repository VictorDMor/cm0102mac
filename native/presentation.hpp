#pragma once
#include <optional>
#include <string_view>

namespace cm::aot {
struct GuiCanvas {
    int width = 800, height = 600;
    bool operator==(const GuiCanvas&) const = default;
    bool original() const { return width == 800 && height == 600; }
};
// Bounds also keep both the 16-bit game stride and the 32-bit window PixMap
// within QuickDraw's 14-bit row-byte field.
inline constexpr GuiCanvas bounded_gui_canvas(int width, int height) {
    return {width < 800 ? 800 : width > 3200 ? 3200 : width,
            height < 600 ? 600 : height > 1800 ? 1800 : height};
}
enum class InterfaceScale { original, percent125, percent150, percent200, fit };
inline constexpr InterfaceScale interface_scales[]{InterfaceScale::original, InterfaceScale::percent125,
    InterfaceScale::percent150, InterfaceScale::percent200, InterfaceScale::fit};
std::string_view scale_name(InterfaceScale scale);
std::optional<InterfaceScale> parse_scale(std::string_view name);
struct PresentationPoint { double x = 0, y = 0; };
// All rectangles use a downward Y axis and half-open right/bottom edges.
struct PresentationRect {
    double x = 0, y = 0, width = 0, height = 0;
    bool contains(PresentationPoint point) const;
};
struct PresentationLayout {
    PresentationRect logical, host;
    double scale = 1;
    InterfaceScale effective = InterfaceScale::original;
    PresentationPoint to_host(PresentationPoint point) const;
    PresentationPoint to_logical(PresentationPoint point) const;
    PresentationRect to_host(PresentationRect rectangle) const;
};
bool scale_fits(InterfaceScale choice, PresentationRect logical, PresentationRect available);
PresentationLayout presentation_layout(InterfaceScale choice, PresentationRect logical,
                                       PresentationRect available, double backing);
}
