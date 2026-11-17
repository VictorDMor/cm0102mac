#include "presentation_frame.hpp"
#include "native_drawing.hpp"
#include "main_thread.hpp"
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string_view>

using namespace cm::aot;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
// The previous present_window image construction, retained as the independent
// pixel/metadata oracle and benchmark. Neither path opens a window or a career.
QuartzImage copied_image(unsigned width, unsigned height, unsigned stride, const std::vector<std::uint8_t>& bytes) {
    const auto data = CFDataCreate(nullptr, bytes.data(), bytes.size());
    const auto provider = CGDataProviderCreateWithCFData(data);
    const auto colors = CGColorSpaceCreateDeviceRGB();
    QuartzImage image(CGImageCreate(width, height, 8, 32, stride, colors,
        CGBitmapInfo(kCGImageAlphaNoneSkipFirst) | CGBitmapInfo(kCGBitmapByteOrder32Big),
        provider, nullptr, false, kCGRenderingIntentDefault));
    CGColorSpaceRelease(colors); CGDataProviderRelease(provider); CFRelease(data);
    require(bool(image), "Baseline image allocation failed");
    return image;
}
PresentationFrame baseline(unsigned width, unsigned height, unsigned stride,
                           const std::vector<std::uint8_t>& pixels, std::shared_ptr<const NativeDrawing> drawing = {}) {
    PresentationFrame frame;
    frame.image = copied_image(width, height, stride, pixels);
    if (drawing) frame.background = copied_image(width, height, width * 4, drawing->background);
    frame.drawing = std::move(drawing);
    return frame;
}
std::vector<std::uint8_t> render(CGImageRef image, unsigned scale = 1) {
    const auto width = CGImageGetWidth(image) * scale, height = CGImageGetHeight(image) * scale;
    std::vector<std::uint8_t> out(width * height * 4);
    const auto colors = CGColorSpaceCreateDeviceRGB();
    const auto context = CGBitmapContextCreate(out.data(), width, height, 8, width * 4, colors,
        CGBitmapInfo(kCGImageAlphaPremultipliedLast) | CGBitmapInfo(kCGBitmapByteOrder32Big));
    CGColorSpaceRelease(colors);
    require(context != nullptr, "Cannot create headless presentation context");
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextSetBlendMode(context, kCGBlendModeCopy);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
    CGContextRelease(context);
    return out;
}
std::vector<std::uint8_t> pattern(unsigned width, unsigned height, unsigned stride, unsigned seed) {
    std::vector<std::uint8_t> bytes(stride * height);
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        const auto at = y * stride + 4 * x;
        bytes[at] = (seed + x) % 256; // Unused alpha must still match the oracle.
        bytes[at + 1] = (x * 7 + seed) % 256;
        bytes[at + 2] = (y * 11 + seed) % 256;
        bytes[at + 3] = (x + y + seed * 3) % 256;
    }
    return bytes;
}
void equivalent(CGImageRef expected, CGImageRef actual) {
    require(CGImageGetWidth(expected) == CGImageGetWidth(actual) &&
            CGImageGetHeight(expected) == CGImageGetHeight(actual) &&
            CGImageGetBytesPerRow(expected) == CGImageGetBytesPerRow(actual) &&
            CGImageGetBitmapInfo(expected) == CGImageGetBitmapInfo(actual) &&
            CGImageGetBitsPerComponent(expected) == CGImageGetBitsPerComponent(actual) &&
            CGImageGetBitsPerPixel(expected) == CGImageGetBitsPerPixel(actual) &&
            CGImageGetShouldInterpolate(expected) == CGImageGetShouldInterpolate(actual) &&
            CGImageGetRenderingIntent(expected) == CGImageGetRenderingIntent(actual), "Prepared image metadata changed");
    auto a = CGDataProviderCopyData(CGImageGetDataProvider(expected));
    auto b = CGDataProviderCopyData(CGImageGetDataProvider(actual));
    const bool equal = a && b && CFEqual(a, b);
    if (a) CFRelease(a); if (b) CFRelease(b);
    require(equal, "Prepared image changed stored bytes or row padding");
    for (auto scale : {1u, 2u, 4u}) require(render(expected, scale) == render(actual, scale), "Rendered pixels changed at backing scale");
}
void checks() {
    const unsigned width = 17, height = 13, stride = width * 4 + 12;
    auto pixels = pattern(width, height, stride, 3);
    auto drawing = std::make_shared<NativeDrawing>();
    drawing->width = width; drawing->height = height;
    drawing->background = pattern(width, height, width * 4, 9);
    auto original = baseline(width, height, stride, pixels, drawing);
    auto frame = prepare_presentation_frame(width, height, stride, pixels, drawing);
    equivalent(original.image.get(), frame.image.get());
    equivalent(original.background.get(), frame.background.get());
    require(frame.drawing == drawing, "Frame lost the immutable native display list");
    pixels[1] ^= 0xff;
    require(render(copied_image(width, height, stride, pixels).get()) != render(frame.image.get()), "Pixel comparator missed a deliberate change");
    // An lvalue caller still owns a separate source; changing it after the
    // call cannot mutate a Quartz image. Production transfers an rvalue.
    equivalent(original.image.get(), frame.image.get());
    bool caught = false;
    auto invalid = [&](auto operation) {
        caught = false;
        try { operation(); } catch (const std::runtime_error&) { caught = true; }
        require(caught, "Invalid presentation buffer was accepted");
    };
    invalid([&] { prepare_presentation_frame(0, 1, 4, {}); });
    invalid([&] { prepare_presentation_frame(UINT32_MAX, 1, 4, {0,0,0,0}); });
    invalid([&] { prepare_presentation_frame(width, height, width * 4 - 1, pixels); });
    invalid([&] { prepare_presentation_frame(width, height, stride, {}); });
    auto bad = std::make_shared<NativeDrawing>(*drawing); bad->width++;
    invalid([&] { prepare_presentation_frame(width, height, stride, pixels, bad); });
    bad->width--; bad->background.pop_back();
    invalid([&] { prepare_presentation_frame(width, height, stride, pixels, bad); });

    std::vector<PresentationFrame> held;
    std::vector<std::vector<std::uint8_t>> expected;
    std::weak_ptr<const NativeDrawing> retained_background;
    require(run_game_thread([&] {
        require(!on_main_thread(), "Preparation fixture did not use the game worker");
        for (unsigned i = 0; i < 24; ++i) {
            auto source = pattern(width, height, stride, i);
            auto prepared = prepare_presentation_frame(width, height, stride, std::move(source));
            auto expected_pixels = render(copied_image(width, height, stride, pattern(width, height, stride, i)).get());
            on_main([&] { held.push_back(std::move(prepared)); expected.push_back(std::move(expected_pixels)); });
        }
        auto snapshot = std::make_shared<NativeDrawing>();
        snapshot->width = width; snapshot->height = height;
        snapshot->background = pattern(width, height, width * 4, 42);
        retained_background = snapshot;
        auto prepared = prepare_presentation_frame(width, height, stride, pattern(width, height, stride, 42), snapshot);
        prepared.drawing.reset(); snapshot.reset();
        require(!retained_background.expired(), "Background provider failed to retain its snapshot");
        on_main([&] { held.push_back(std::move(prepared)); });
        return 0;
    }, service_main_queue) == 0, "Presentation worker failed");
    for (unsigned i = 0; i < expected.size(); ++i)
        require(render(held[i].image.get()) == expected[i], "A later frame or worker teardown changed earlier pixels");
    equivalent(original.image.get(), frame.image.get());
    require(render(held.back().background.get()) == render(copied_image(width, height, width * 4,
        pattern(width, height, width * 4, 42)).get()), "Released source corrupted its retained background");
    held.clear();
    require(retained_background.expired(), "Quartz release callback retained a dead frame");
    std::cout << "Presentation pixel/stride/scale equivalence, immutable lifetime, FIFO handoff and errors passed\n";
}
struct CapturedFrame { unsigned width, height, stride; std::vector<std::uint8_t> pixels; };
CapturedFrame read_capture(const char* path) {
    const auto url = CFURLCreateFromFileSystemRepresentation(nullptr, reinterpret_cast<const UInt8*>(path), std::strlen(path), false);
    const auto source = CGImageSourceCreateWithURL(url, nullptr); CFRelease(url);
    require(source, "Cannot read existing screen capture");
    QuartzImage image(CGImageSourceCreateImageAtIndex(source, 0, nullptr)); CFRelease(source);
    require(bool(image), "Cannot decode existing screen capture");
    const auto width = unsigned(CGImageGetWidth(image.get())), height = unsigned(CGImageGetHeight(image.get()));
    auto rgba = render(image.get());
    for (std::size_t i = 0; i < rgba.size(); i += 4) {
        const auto r = rgba[i], g = rgba[i + 1], b = rgba[i + 2];
        rgba[i] = 0; rgba[i + 1] = r; rgba[i + 2] = g; rgba[i + 3] = b;
    }
    return {width, height, width * 4, std::move(rgba)};
}
void benchmark(const char* path) {
    const auto capture = read_capture(path);
    auto scene = std::make_shared<NativeDrawing>();
    scene->width = capture.width; scene->height = capture.height; scene->background = capture.pixels;
    // Both sides include the guest raster copy and independent drawing snapshot
    // that platform.cpp makes for each frame. Rendering is measured separately.
    auto make = [&](bool candidate) {
        auto pixels = capture.pixels;
        auto snapshot = std::make_shared<const NativeDrawing>(*scene);
        return candidate ? prepare_presentation_frame(capture.width, capture.height, capture.stride, std::move(pixels), std::move(snapshot))
                         : baseline(capture.width, capture.height, capture.stride, pixels, std::move(snapshot));
    };
    auto a = make(false), b = make(true);
    equivalent(a.image.get(), b.image.get()); equivalent(a.background.get(), b.background.get());
    const unsigned count = 160;
    std::uint64_t checksum = 0;
    auto timed = [&](bool candidate, bool draw) {
        const auto start = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < count; ++i) {
            auto frame = make(candidate);
            if (draw) {
                const auto out = render(frame.background.get());
                checksum += out[(i * 31) % out.size()];
            } else checksum += CGImageGetBytesPerRow(frame.image.get());
        }
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / count;
    };
    for (bool draw : {false, true}) {
        (void)timed(false, draw); (void)timed(true, draw);
        for (unsigned pair = 0; pair < 5; ++pair) {
            double old_ms, new_ms;
            if (pair % 2) { new_ms = timed(true, draw); old_ms = timed(false, draw); }
            else { old_ms = timed(false, draw); new_ms = timed(true, draw); }
            std::cout << "{\"input\":\"" << path << "\",\"width\":" << capture.width << ",\"height\":" << capture.height
                      << ",\"draw\":" << draw << ",\"pair\":" << pair << ",\"baseline_ms\":" << old_ms
                      << ",\"prepared_ms\":" << new_ms << "}\n";
        }
    }
    require(checksum != 0, "Benchmark did not consume prepared images");
}
}
int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            if (argc == 1) checks();
            else if (argc == 3 && std::string_view(argv[1]) == "--benchmark") benchmark(argv[2]);
            else throw std::runtime_error("Usage: cm-presentation-frame-checks [--benchmark <existing-screen.png>]");
            return 0;
        } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    }
}
