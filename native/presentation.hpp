#pragma once
#include <optional>
#include <string_view>

namespace cm::aot {
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
