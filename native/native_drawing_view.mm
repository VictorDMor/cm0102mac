#include "native_drawing.hpp"
#import <CoreText/CoreText.h>
#include <map>

namespace cm::aot {
namespace {
struct FontPaths {
    std::map<std::pair<unsigned, unsigned>, CGPathRef> paths;
    ~FontPaths() { for (const auto& [key, path] : paths) { (void)key; if (path) CGPathRelease(path); } }
    CGPathRef path(unsigned font, unsigned character) {
        const auto key = std::make_pair(font, character);
        if (auto it = paths.find(key); it != paths.end()) return it->second;
        // Font substitution is presentation-only. Every glyph retains the
        // original renderer's position and ink box, including narrow headings.
        const auto name = font <= 2 ? CFSTR("ArialNarrow") : font >= 6 ? CFSTR("ArialNarrow-Bold") : CFSTR("ArialMT");
        const auto face = CTFontCreateWithName(name, 100, nullptr);
        const std::uint8_t byte = character;
        const auto text = CFStringCreateWithBytes(nullptr, &byte, 1, kCFStringEncodingWindowsLatin1, false);
        CGGlyph glyph = 0;
        if (text && CFStringGetLength(text) == 1) {
            const UniChar c = CFStringGetCharacterAtIndex(text, 0);
            CTFontGetGlyphsForCharacters(face, &c, &glyph, 1);
        }
        const auto outline = glyph ? CTFontCreatePathForGlyph(face, glyph, nullptr) : nullptr;
        if (text) CFRelease(text);
        CFRelease(face);
        paths.emplace(key, outline);
        return outline;
    }
};
CGRect rect(DrawingRect r) { return CGRectMake(r.x, r.y, r.width, r.height); }
void add_outline(CGContextRef context, const NativeOutline& outline, double x, double baseline) {
    auto point = [=](OutlinePoint p) { return CGPointMake(x + p.x, baseline - p.y); };
    for (const auto& contour : outline.contours) {
        if (contour.empty()) continue;
        auto start = contour.front();
        std::size_t begin = 1, count = contour.size() - 1;
        if (!start.on_curve) {
            if (contour.back().on_curve) { start = contour.back(); begin = 0; }
            else { start.x = (start.x + contour.back().x) / 2; start.y = (start.y + contour.back().y) / 2; begin = 0; count = contour.size(); }
        }
        auto p = point(start); CGContextMoveToPoint(context, p.x, p.y);
        for (std::size_t i = 0; i < count; ++i) {
            const auto current = contour[(begin + i) % contour.size()];
            const auto q = point(current);
            if (current.on_curve) CGContextAddLineToPoint(context, q.x, q.y);
            else {
                const auto next = i + 1 < count ? contour[(begin + i + 1) % contour.size()] : start;
                auto end = next;
                if (!next.on_curve) { end.x = (current.x + next.x) / 2; end.y = (current.y + next.y) / 2; }
                else if (i + 1 < count) ++i;
                const auto r = point(end); CGContextAddQuadCurveToPoint(context, q.x, q.y, r.x, r.y);
            }
        }
        CGContextClosePath(context);
    }
}
}
void draw_native_shapes(CGContextRef context, const NativeDrawing& drawing) {
    static FontPaths fonts;
    CGContextSetBlendMode(context, kCGBlendModeNormal);
    CGContextSetShouldAntialias(context, true);
    CGContextSetAllowsAntialiasing(context, true);
    for (const auto& shape : drawing.shapes) {
        if (shape.clips.empty()) continue;
        CGContextSaveGState(context);
        // All fragments form one clipping path, so adjacent scanlines do not
        // composite an antialiased glyph several times along shared boundaries.
        CGContextBeginPath(context);
        for (auto r : shape.clips) CGContextAddRect(context, rect(r));
        CGContextClip(context);
        const CGFloat red = ((shape.color >> 10) & 31) / 31.0;
        const CGFloat green = ((shape.color >> 5) & 31) / 31.0;
        const CGFloat blue = (shape.color & 31) / 31.0;
        CGContextSetRGBFillColor(context, red, green, blue, 1);
        CGContextSetRGBStrokeColor(context, red, green, blue, 1);
        if (shape.kind == NativeShape::Kind::glyph) {
            if (shape.outline) {
                CGContextBeginPath(context);
                add_outline(context, *shape.outline, shape.origin_x, shape.baseline);
                CGContextFillPath(context);
            } else if (const auto path = fonts.path(shape.font, shape.character)) {
                const auto ink = CGPathGetPathBoundingBox(path);
                const auto box = rect(shape.bounds);
                if (ink.size.width > 0 && ink.size.height > 0) {
                    const double sx = box.size.width / ink.size.width, sy = box.size.height / ink.size.height;
                    // Core Text outlines are y-up; the game and view are y-down.
                    const CGAffineTransform transform{sx, 0, 0, -sy,
                        box.origin.x - ink.origin.x * sx, box.origin.y + box.size.height + ink.origin.y * sy};
                    const auto positioned = CGPathCreateCopyByTransformingPath(path, &transform);
                    CGContextAddPath(context, positioned); CGContextFillPath(context);
                    CGPathRelease(positioned);
                }
            }
        } else if (shape.kind == NativeShape::Kind::fill) {
            CGContextFillRect(context, rect(shape.bounds));
        } else if (shape.kind == NativeShape::Kind::line) {
            CGContextSetLineWidth(context, 1);
            CGContextSetLineCap(context, kCGLineCapSquare);
            if (!(shape.style & 2)) {
                const CGFloat dash[]{4, 2}; CGContextSetLineDash(context, 0, dash, 2);
            }
            if (shape.y1 == shape.y2) {
                CGContextSetLineCap(context, kCGLineCapButt);
                CGContextMoveToPoint(context, shape.bounds.x, shape.bounds.y + 0.5);
                CGContextAddLineToPoint(context, shape.bounds.x + shape.bounds.width, shape.bounds.y + 0.5);
            } else if (shape.x1 == shape.x2) {
                CGContextSetLineCap(context, kCGLineCapButt);
                CGContextMoveToPoint(context, shape.bounds.x + 0.5, shape.bounds.y);
                CGContextAddLineToPoint(context, shape.bounds.x + 0.5, shape.bounds.y + shape.bounds.height);
            } else {
                CGContextMoveToPoint(context, shape.x1 + 0.5, shape.y1 + 0.5);
                CGContextAddLineToPoint(context, shape.x2 + 0.5, shape.y2 + 0.5);
            }
            CGContextStrokePath(context);
        } else {
            if (shape.style & 4) CGContextFillEllipseInRect(context, rect(shape.bounds));
            else {
                CGContextSetLineWidth(context, 1);
                if (!(shape.style & 2)) { const CGFloat dash[]{4, 2}; CGContextSetLineDash(context, 0, dash, 2); }
                CGContextStrokeEllipseInRect(context, CGRectInset(rect(shape.bounds), 0.5, 0.5));
            }
        }
        CGContextRestoreGState(context);
    }
}
}
