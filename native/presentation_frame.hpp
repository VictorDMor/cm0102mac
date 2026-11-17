#pragma once
#include <CoreGraphics/CoreGraphics.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace cm::aot {
struct NativeDrawing;
struct QuartzImageRelease {
    void operator()(CGImageRef image) const noexcept { CGImageRelease(image); }
};
using QuartzImage = std::unique_ptr<CGImage, QuartzImageRelease>;
struct PresentationFrame {
    QuartzImage image, background;
    std::shared_ptr<const NativeDrawing> drawing;
};
// Runs on the game owner before main-thread presentation. The pixel vector is
// transferred, and drawing must be an immutable snapshot. Providers own their
// bytes until Quartz releases them, including after this frame is destroyed.
PresentationFrame prepare_presentation_frame(std::uint32_t width, std::uint32_t height,
    std::uint32_t row_bytes, std::vector<std::uint8_t> pixels,
    std::shared_ptr<const NativeDrawing> drawing = {});
}
