#include "presentation_frame.hpp"
#include "native_drawing.hpp"
#include "profiling.hpp"
#include <stdexcept>

namespace cm::aot {
namespace {
using Pixels = std::shared_ptr<const std::vector<std::uint8_t>>;
QuartzImage image(std::uint32_t width, std::uint32_t height, std::uint32_t stride, Pixels bytes) {
    auto owner = std::make_unique<Pixels>(std::move(bytes));
    const auto provider = CGDataProviderCreateWithData(owner.get(), (*owner)->data(), (*owner)->size(),
        [](void* context, const void*, std::size_t) { delete static_cast<Pixels*>(context); });
    if (!provider) return {};
    owner.release(); // The provider now owns this reference, not the caller.
    const auto colors = CGColorSpaceCreateDeviceRGB();
    QuartzImage result;
    if (colors) {
        result.reset(CGImageCreate(width, height, 8, 32, stride, colors,
            CGBitmapInfo(kCGImageAlphaNoneSkipFirst) | CGBitmapInfo(kCGBitmapByteOrder32Big),
            provider, nullptr, false, kCGRenderingIntentDefault));
        CGColorSpaceRelease(colors);
    }
    CGDataProviderRelease(provider);
    return result;
}
}
PresentationFrame prepare_presentation_frame(std::uint32_t width, std::uint32_t height,
    std::uint32_t row_bytes, std::vector<std::uint8_t> pixels, std::shared_ptr<const NativeDrawing> drawing) {
    CM_PROFILE_ZONE("Frame preparation");
    if (!width || !height || row_bytes < std::uint64_t(width) * 4 || pixels.size() != std::uint64_t(row_bytes) * height)
        throw std::runtime_error("Invalid native window pixel buffer");
    if (drawing && (drawing->width != std::int64_t(width) || drawing->height != std::int64_t(height) ||
        drawing->background.size() != std::uint64_t(width) * height * 4))
        throw std::runtime_error("Invalid native drawing background");
    PresentationFrame frame;
    frame.image = image(width, height, row_bytes,
        std::make_shared<const std::vector<std::uint8_t>>(std::move(pixels)));
    if (!frame.image) throw std::runtime_error("Native Quartz rejected the original window pixel buffer");
    if (drawing) {
        frame.background = image(width, height, width * 4, Pixels(drawing, &drawing->background));
        if (frame.background) frame.drawing = std::move(drawing);
    }
    return frame;
}
}
