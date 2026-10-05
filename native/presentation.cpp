#include "presentation.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cm::aot {
std::string_view scale_name(InterfaceScale scale) {
    switch (scale) {
        case InterfaceScale::original: return "100";
        case InterfaceScale::percent125: return "125";
        case InterfaceScale::percent150: return "150";
        case InterfaceScale::percent200: return "200";
        case InterfaceScale::fit: return "fit";
    }
    throw std::invalid_argument("Invalid interface scale");
}
std::optional<InterfaceScale> parse_scale(std::string_view name) {
    for (auto choice : interface_scales) if (scale_name(choice) == name) return choice;
    return {};
}
namespace {
double factor(InterfaceScale choice) {
    switch (choice) {
        case InterfaceScale::original: return 1;
        case InterfaceScale::percent125: return 1.25;
        case InterfaceScale::percent150: return 1.5;
        case InterfaceScale::percent200: return 2;
        case InterfaceScale::fit: return 0;
    }
    throw std::invalid_argument("Invalid interface scale");
}
bool valid(PresentationRect r) {
    return std::isfinite(r.x) && std::isfinite(r.y) && std::isfinite(r.width) &&
        std::isfinite(r.height) && r.width > 0 && r.height > 0;
}
}
bool PresentationRect::contains(PresentationPoint p) const {
    return p.x >= x && p.y >= y && p.x < x + width && p.y < y + height;
}
PresentationPoint PresentationLayout::to_host(PresentationPoint p) const {
    return {host.x + (p.x - logical.x) * scale, host.y + (p.y - logical.y) * scale};
}
PresentationPoint PresentationLayout::to_logical(PresentationPoint p) const {
    return {logical.x + (p.x - host.x) / scale, logical.y + (p.y - host.y) / scale};
}
PresentationRect PresentationLayout::to_host(PresentationRect r) const {
    const auto p = to_host(PresentationPoint{r.x, r.y});
    return {p.x, p.y, r.width * scale, r.height * scale};
}
bool scale_fits(InterfaceScale choice, PresentationRect logical, PresentationRect available) {
    if (!valid(logical) || !valid(available)) return false;
    const auto scale = factor(choice);
    return choice == InterfaceScale::fit ||
        (logical.width * scale <= available.width && logical.height * scale <= available.height);
}
PresentationLayout presentation_layout(InterfaceScale choice, PresentationRect logical,
                                       PresentationRect available, double backing) {
    if (!valid(logical) || !valid(available) || !std::isfinite(backing) || backing <= 0)
        throw std::invalid_argument("Invalid presentation geometry");
    // Snap the usable area inward to backing pixels. The user scale remains in
    // AppKit points; Retina affects alignment, never the chosen enlargement.
    const auto right = std::floor((available.x + available.width) * backing) / backing;
    const auto bottom = std::floor((available.y + available.height) * backing) / backing;
    available.x = std::ceil(available.x * backing) / backing;
    available.y = std::ceil(available.y * backing) / backing;
    available.width = right - available.x; available.height = bottom - available.y;
    if (!valid(available)) throw std::invalid_argument("No usable presentation pixels");
    if (!scale_fits(choice, logical, available)) choice = InterfaceScale::fit;
    const auto scale = choice == InterfaceScale::fit
        ? std::min(available.width / logical.width, available.height / logical.height) : factor(choice);
    const auto width = logical.width * scale, height = logical.height * scale;
    auto x = logical.x + (logical.width - width) / 2;
    auto y = logical.y + (logical.height - height) / 2;
    if (choice == InterfaceScale::fit) {
        x = available.x + (available.width - width) / 2;
        y = available.y + (available.height - height) / 2;
    }
    x = std::clamp(std::round(x * backing) / backing, available.x, right - width);
    y = std::clamp(std::round(y * backing) / backing, available.y, bottom - height);
    return {logical, {x, y, width, height}, scale, choice};
}
}
