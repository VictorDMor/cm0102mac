#include "presentation.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace cm::aot;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
bool close(double a, double b) { return std::abs(a - b) < 1e-8; }
void point(PresentationPoint p, double x, double y) {
    require(close(p.x, x) && close(p.y, y), "Presentation point differs from expected geometry");
}
}
int main() {
    try {
        const PresentationRect panel{500, 284, 800, 600}, desktop{0, 39, 1800, 1063};
        for (double backing : {1., 2.}) {
            auto original = presentation_layout(InterfaceScale::original, panel, desktop, backing);
            require(original.scale == 1 && original.host.width == 800 && original.host.height == 600,
                    "Retina doubled the chosen scale or changed the source extent");
            point(original.to_host(PresentationPoint{500, 284}), 500, 284);
            auto enlarged = presentation_layout(InterfaceScale::percent125, panel, desktop, backing);
            require(enlarged.scale == 1.25 && enlarged.host.width == 1000 && enlarged.host.height == 750,
                    "Fractional scale changed aspect ratio");
            point(enlarged.to_host(PresentationPoint{500, 284}), 400, 209);
            point(enlarged.to_logical({900, 584}), 900, 584);
            point(enlarged.to_logical({1399, 958}), 1299.2, 883.2);
            require(enlarged.host.contains({400, 209}) && !enlarged.host.contains({1400, 959}),
                    "Half-open presentation edges changed");
            require(!panel.contains(enlarged.to_logical({399, 500})) &&
                    !panel.contains(enlarged.to_logical({900, 959})), "Margin input was clamped into the game");
            const auto overlay = enlarged.to_host(PresentationRect{600, 384, 120, 80});
            require(overlay.x == 525 && overlay.y == 334 && overlay.width == 150 && overlay.height == 100,
                    "Overlay lost its relative position or uniform scale");
            auto fit = presentation_layout(InterfaceScale::fit, panel, desktop, backing);
            require(close(fit.scale, 1063. / 600) && close(fit.host.height, 1063) && fit.host.y == 39,
                    "Fit used full-screen height instead of the usable desktop");
            require(!scale_fits(InterfaceScale::percent200, panel, desktop), "Oversized fixed scale remained available");
            require(presentation_layout(InterfaceScale::percent200, panel, desktop, backing).effective == InterfaceScale::fit,
                    "A stale fixed preference was displayed as its requested scale");
            const auto small = presentation_layout(InterfaceScale::original, panel, {-900, -700, 640, 480}, backing);
            require(small.effective == InterfaceScale::fit && small.scale == .8 && small.host.x == -900 && small.host.y == -700,
                    "Small negative-origin display left content unreachable");
            point(small.to_logical({-900, -700}), 500, 284);
            const auto large = presentation_layout(InterfaceScale::percent200, panel, {-2000, 50, 1920, 1400}, backing);
            require(large.scale == 2 && large.host.width == 1600 && large.host.height == 1200 &&
                    large.host.x >= -2000 && large.host.x + large.host.width <= -80,
                    "A relocated 200 percent panel exceeds its monitor");
            for (auto choice : interface_scales) {
                auto layout = presentation_layout(choice, panel, desktop, backing);
                for (PresentationPoint p : {PresentationPoint{500, 284}, {1299, 883}, {900.25, 401.75}, {-10, -20}}) {
                    const auto roundtrip = layout.to_logical(layout.to_host(p));
                    point(roundtrip, p.x, p.y);
                }
            }
        }
        for (auto scale : interface_scales) require(parse_scale(scale_name(scale)) == scale, "Preference roundtrip");
        for (auto invalid : {"", "1.25", "300", "125%", "NaN", "fit\n"})
            require(!parse_scale(invalid), "Malformed scale accepted");
        bool rejected = false;
        try { (void)presentation_layout(InterfaceScale::fit, panel, {0, 0, 0, 0}, 2); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Empty usable area accepted");
        std::cout << "Presentation scale, Retina independence, monitor recovery, overlays, margins and roundtrips passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
