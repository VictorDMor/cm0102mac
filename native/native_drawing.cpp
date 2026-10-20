#include "native_drawing.hpp"
#include "platform.hpp"
#include "regions.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

namespace cm::aot {
namespace {
DrawingRect intersection(DrawingRect a, DrawingRect b) {
    const int x = std::max(a.x, b.x), y = std::max(a.y, b.y);
    return {x, y, std::min(a.x + a.width, b.x + b.width) - x,
                  std::min(a.y + a.height, b.y + b.height) - y};
}
void subtract(std::vector<DrawingRect>& clips, DrawingRect cut) {
    std::vector<DrawingRect> out;
    for (auto r : clips) {
        const auto i = intersection(r, cut);
        if (i.empty()) { out.push_back(r); continue; }
        if (r.y < i.y) out.push_back({r.x, r.y, r.width, i.y - r.y});
        if (i.y + i.height < r.y + r.height) out.push_back({r.x, i.y + i.height, r.width, r.y + r.height - i.y - i.height});
        if (r.x < i.x) out.push_back({r.x, i.y, i.x - r.x, i.height});
        if (i.x + i.width < r.x + r.width) out.push_back({i.x + i.width, i.y, r.x + r.width - i.x - i.width, i.height});
    }
    clips = std::move(out);
}
void erase_region(NativeDrawing& scene, DrawingRect rect) {
    for (auto& shape : scene.shapes) subtract(shape.clips, rect);
    std::erase_if(scene.shapes, [](const auto& shape) { return shape.clips.empty(); });
}
std::uint16_t shade(std::uint16_t pixel, unsigned percent, bool gray) {
    unsigned channels[3]{unsigned((pixel >> 10) & 31), unsigned((pixel >> 5) & 31), unsigned(pixel & 31)};
    if (gray) channels[0] = channels[1] = channels[2] = (channels[0] + channels[1] + channels[2]) / 3;
    for (auto& c : channels) c = std::min(31u, c * percent / 100);
    return (channels[0] << 10) | (channels[1] << 5) | channels[2];
}
void shade_scene(NativeDrawing& scene, unsigned percent, bool gray = false) {
    for (auto& shape : scene.shapes) shape.color = shade(shape.color, percent, gray);
    for (std::size_t i = 0; i < scene.background.size(); i += 4) {
        auto* p = scene.background.data() + i;
        const auto color = shade(((p[1] >> 3) << 10) | ((p[2] >> 3) << 5) | (p[3] >> 3), percent, gray);
        for (unsigned c = 0; c < 3; ++c) { const auto v = (color >> (10 - c * 5)) & 31; p[c + 1] = (v << 3) | (v >> 2); }
    }
}
void translate(NativeShape& shape, int x, int y) {
    shape.bounds.x += x; shape.bounds.y += y;
    shape.x1 += x; shape.y1 += y;
    shape.origin_x += x; shape.baseline += y;
    shape.x2 += x; shape.y2 += y;
    for (auto& r : shape.clips) { r.x += x; r.y += y; }
}
void blit(NativeDrawing& destination, const NativeDrawing& source, DrawingRect from,
          int x, int y, const std::vector<std::uint8_t>& mask = {}, int mask_x = 0, int mask_y = 0) {
    // Make a source snapshot before changing a possibly overlapping destination.
    const NativeDrawing input = source;
    const int ox = x - from.x, oy = y - from.y;
    auto clip = intersection(from, {0, 0, input.width, input.height});
    clip = intersection(clip, {-ox, -oy, destination.width, destination.height});
    if (clip.empty()) return;
    std::vector<DrawingRect> regions;
    for (int sy = clip.y; sy < clip.y + clip.height; ++sy) {
        int begin = -1;
        for (int sx = clip.x; sx <= clip.x + clip.width; ++sx) {
            const bool covered = sx < clip.x + clip.width && (mask.empty() || region_contains(mask, sx + ox + mask_x, sy + oy + mask_y));
            if (covered) {
                if (begin < 0) begin = sx;
                const auto a = (std::size_t(sy) * input.width + sx) * 4;
                const auto b = (std::size_t(sy + oy) * destination.width + sx + ox) * 4;
                std::copy_n(input.background.data() + a, 4, destination.background.data() + b);
            } else if (begin >= 0) {
                regions.push_back({begin + ox, sy + oy, sx - begin, 1}); begin = -1;
            }
        }
    }
    // Rectangular copies dominate real presentation and saved dialog surfaces.
    if (mask.empty()) regions = {{clip.x + ox, clip.y + oy, clip.width, clip.height}};
    for (auto r : regions) erase_region(destination, r);
    for (auto shape : input.shapes) {
        std::vector<DrawingRect> clips;
        for (auto r : shape.clips) {
            r = intersection(r, clip);
            if (!r.empty()) clips.push_back(r);
        }
        shape.clips = std::move(clips);
        if (shape.clips.empty()) continue;
        translate(shape, ox, oy);
        if (!mask.empty()) {
            clips.clear();
            for (auto r : shape.clips) for (auto region : regions) {
                auto i = intersection(r, region); if (!i.empty()) clips.push_back(i);
            }
            shape.clips = std::move(clips);
        }
        if (!shape.clips.empty()) destination.shapes.push_back(std::move(shape));
    }
}
}
struct NativeDrawingObserver::Impl {
    struct Surface {
        std::uint32_t address;
        unsigned row_bytes, depth;
        NativeDrawing scene;
        std::vector<std::uint8_t> raw, dirty;
        bool changed = false;
        void flush() {
            if (!changed) return;
            std::vector<DrawingRect> rows;
            for (int y = 0; y < scene.height; ++y) {
                int start = -1;
                for (int x = 0; x <= scene.width; ++x) {
                    const auto at = std::size_t(y) * scene.width + x;
                    if (x < scene.width && dirty[at]) {
                        if (start < 0) start = x;
                        dirty[at] = 0;
                        const auto* p = raw.data() + std::size_t(y) * row_bytes + x * (depth / 8);
                        auto* out = scene.background.data() + at * 4;
                        if (depth == 32) std::copy_n(p, 4, out);
                        else {
                            const auto pixel = read_be<2>(p);
                            for (unsigned c = 0; c < 3; ++c) {
                                const auto v = (pixel >> (10 - c * 5)) & 31;
                                out[c + 1] = (v << 3) | (v >> 2);
                            }
                        }
                    } else if (start >= 0) {
                        const DrawingRect row{start, y, x - start, 1};
                        if (!rows.empty() && rows.back().x == row.x && rows.back().width == row.width &&
                            rows.back().y + rows.back().height == y) ++rows.back().height;
                        else rows.push_back(row);
                        start = -1;
                    }
                }
            }
            for (auto r : rows) erase_region(scene, r);
            changed = false;
        }
        void append(NativeShape shape) {
            if (shape.clips.empty()) {
                auto clip = intersection(shape.bounds, {0, 0, scene.width, scene.height});
                if (clip.empty()) return;
                shape.clips = {clip};
            }
            if (shape.kind == NativeShape::Kind::fill) erase_region(scene, shape.bounds);
            scene.shapes.push_back(std::move(shape));
        }
    };
    std::map<std::uint32_t, Surface> surfaces;
    struct SavedDrawing {
        NativeDrawing scene;
        std::uint32_t pixels;
        std::vector<std::uint8_t> original;
    };
    std::map<std::uint32_t, SavedDrawing> saved;
    const NativeDrawing* saved_scene(State& s, std::uint32_t token) {
        const auto it = saved.find(token);
        if (it == saved.end()) return nullptr;
        const auto& image = it->second;
        // Original image objects can be reused or repainted without being freed.
        // Never restore an obsolete display list over their new raster content.
        if (s.memory.read<4>(token) != unsigned(image.scene.width) ||
            s.memory.read<4>(token + 4) != unsigned(image.scene.height) ||
            s.memory.read<4>(token + 12) != image.pixels ||
            s.memory.copy_out(image.pixels, image.original.size()) != image.original) {
            saved.erase(it);
            return nullptr;
        }
        return &image.scene;
    }
    std::map<std::tuple<std::uint32_t, unsigned, unsigned, unsigned, bool>, DrawingRect> ink_bounds;
    struct TextContext { unsigned font; std::uint16_t color; unsigned flags; };
    std::vector<TextContext> outline_contexts;
    std::vector<unsigned> text_fonts;
    struct PendingOutline { bool created = false; std::shared_ptr<NativeOutline> path; };
    std::vector<PendingOutline> pending_outlines;
    std::map<std::uint32_t, std::shared_ptr<const NativeOutline>> glyph_outlines;
    std::vector<bool> fade_in;
    std::uint32_t low = UINT32_MAX;
    std::uint64_t high = 0;
    struct Capture { std::uint32_t exit, surface; };
    std::vector<Capture> captures;
    struct SavedCapture { DrawingRect rect; std::uint32_t surface; };
    std::vector<SavedCapture> saving;
    Surface* active(State& s) {
        const auto toc = s.r<4>(8);
        if (toc != 0x108bba90 || s.memory.read<1>(toc - 0x3550)) return nullptr;
        const auto it = surfaces.find(s.memory.read<4>(toc - 0x3564));
        return it == surfaces.end() ? nullptr : &it->second;
    }
    DrawingRect glyph_bounds(State& s, std::uint32_t pixels, unsigned width, unsigned height, unsigned pitch = 0, bool packed = true) {
        const auto row_bytes = pitch ? pitch : (width + 1) / 2;
        const auto key = std::make_tuple(pixels, width, height, row_bytes, packed);
        if (auto it = ink_bounds.find(key); it != ink_bounds.end()) return it->second;
        if (!pixels || !width || width > 128 || height > 128) return {};
        const auto bytes = s.memory.copy_out(pixels, row_bytes * height);
        int left = int(width), top = int(height), right = -1, bottom = -1;
        for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
            const auto v = bytes[y * row_bytes + (packed ? x / 2 : x)];
            const auto coverage = packed ? ((x & 1) ? (v & 15) : (v >> 4)) : v;
            if (!coverage) continue;
            left = std::min(left, int(x)); right = std::max(right, int(x));
            top = std::min(top, int(y)); bottom = std::max(bottom, int(y));
        }
        const DrawingRect result{left, top, right - left + 1, bottom - top + 1};
        ink_bounds.emplace(key, result);
        return result;
    }
};
NativeDrawingObserver::NativeDrawingObserver() : impl_(std::make_unique<Impl>()) {}
NativeDrawingObserver::~NativeDrawingObserver() = default;
void NativeDrawingObserver::register_surface(State& s, std::uint32_t address, int width, int height, unsigned row_bytes, unsigned depth) {
    if (!address || width <= 0 || height <= 0 || (depth != 16 && depth != 32) ||
        row_bytes < std::uint64_t(width) * (depth / 8)) return;
    Impl::Surface surface{address, row_bytes, depth, {width, height, std::vector<std::uint8_t>(std::size_t(width) * height * 4), {}},
        s.memory.copy_out(address, std::size_t(row_bytes) * height), std::vector<std::uint8_t>(std::size_t(width) * height, 1), true};
    surface.flush();
    impl_->surfaces.insert_or_assign(address, std::move(surface));
    impl_->low = std::min(impl_->low, address);
    impl_->high = std::max(impl_->high, std::uint64_t(address) + std::uint64_t(row_bytes) * height);
    restrict_write_notifications(impl_->low, impl_->high);
    // Fixture/transcript observers keep exclusive ownership of their stream.
    if (!s.memory.observer) s.memory.observer = this;
}
void NativeDrawingObserver::remove_surface(std::uint32_t address) { impl_->surfaces.erase(address); }
void NativeDrawingObserver::written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) {
    if (copying || std::uint64_t(address) + size <= impl_->low || address >= impl_->high) return;
    for (auto& [base, surface] : impl_->surfaces) {
        const auto first = std::max(std::uint64_t(address), std::uint64_t(base));
        const auto last = std::min(std::uint64_t(address) + size, std::uint64_t(base) + surface.raw.size());
        if (first >= last) continue;
        if (std::any_of(impl_->captures.begin(), impl_->captures.end(), [base](const auto& c) { return c.surface == base; })) continue;
        const auto offset = first - base, length = last - first;
        if (bytes) std::memcpy(surface.raw.data() + offset, bytes + first - address, length);
        else std::memset(surface.raw.data() + offset, 0, length);
        const auto bpp = surface.depth / 8;
        for (auto row = offset / surface.row_bytes; row <= (offset + length - 1) / surface.row_bytes; ++row) {
            const auto start = std::max(offset, row * surface.row_bytes) - row * surface.row_bytes;
            const auto end = std::min(offset + length, (row + 1) * surface.row_bytes) - row * surface.row_bytes;
            const auto x1 = std::min(start / bpp, std::uint64_t(surface.scene.width));
            const auto x2 = std::min((end + bpp - 1) / bpp, std::uint64_t(surface.scene.width));
            std::fill(surface.dirty.begin() + row * surface.scene.width + x1,
                      surface.dirty.begin() + row * surface.scene.width + x2, 1);
        }
        surface.changed = true;
    }
}
void NativeDrawingObserver::cleared(std::uint32_t address, std::uint64_t size) { written(address, nullptr, size); }
std::shared_ptr<const NativeDrawing> NativeDrawingObserver::snapshot(State& s, std::uint32_t address) {
    if (s.memory.observer != this) return {};
    auto it = impl_->surfaces.find(address);
    if (it == impl_->surfaces.end()) return {};
    it->second.flush();
    return std::make_shared<const NativeDrawing>(it->second.scene);
}
void NativeDrawingObserver::copy_surface(State& s, std::uint32_t source, std::uint32_t destination,
                                        DrawingRect from, int x, int y, const std::vector<std::uint8_t>& mask, int mask_x, int mask_y) {
    if (s.memory.observer != this) return;
    auto a = impl_->surfaces.find(source), b = impl_->surfaces.find(destination);
    if (a == impl_->surfaces.end() || b == impl_->surfaces.end()) return;
    a->second.flush(); b->second.flush();
    blit(b->second.scene, a->second.scene, from, x, y, mask, mask_x, mask_y);
}
void NativeDrawingObserver::boundary(State& s, std::uint32_t pc) {
    if (s.memory.observer != this) return;
    if (pc == 0x10750580) { impl_->pending_outlines.push_back({}); return; }
    if (pc == 0x10750740 && !impl_->pending_outlines.empty()) { impl_->pending_outlines.back().created = true; return; }
    if (pc == 0x107509f0 && !impl_->pending_outlines.empty()) {
        if (impl_->pending_outlines.back().created) {
            impl_->glyph_outlines.erase(s.r<4>(12));
            if (impl_->pending_outlines.back().path) impl_->glyph_outlines.emplace(s.r<4>(12), impl_->pending_outlines.back().path);
            const auto pixels = s.memory.read<4>(s.r<4>(12) + 20);
            std::erase_if(impl_->ink_bounds, [pixels](const auto& entry) { return std::get<0>(entry.first) == pixels; });
        }
        impl_->pending_outlines.pop_back(); return;
    }
    if (pc == 0x107620d0 && !impl_->pending_outlines.empty()) {
        const auto contours = std::int16_t(s.r<4>(16));
        if (contours <= 0 || contours > 256) return;
        auto outline = std::make_shared<NativeOutline>();
        for (int c = 0; c < contours; ++c) {
            const auto first = s.memory.read<2>(s.r<4>(20) + c * 2), last = s.memory.read<2>(s.r<4>(24) + c * 2);
            if (first > last || last >= 8192) return;
            std::vector<OutlinePoint> points;
            for (auto i = first; i <= last; ++i)
                points.push_back({std::int32_t(s.memory.read<4>(s.r<4>(28) + i * 4)) / 64.0,
                                  std::int32_t(s.memory.read<4>(s.r<4>(32) + i * 4)) / 64.0,
                                  s.memory.read<1>(s.r<4>(36) + i) != 0});
            outline->contours.push_back(std::move(points));
        }
        impl_->pending_outlines.back().path = std::move(outline); return;
    }
    if (pc == 0x1076ca50) impl_->text_fonts.push_back(std::uint16_t(s.r<4>(20)));
    if (pc == 0x1076d194 && !impl_->text_fonts.empty()) impl_->text_fonts.pop_back();
    if (pc == 0x1074f800) {
        const unsigned font = impl_->text_fonts.empty() ? (s.r<4>(32) <= 10 ? 1 : 3) : impl_->text_fonts.back();
        impl_->outline_contexts.push_back({font, std::uint16_t(s.r<4>(36)), unsigned(s.r<4>(28))}); return;
    }
    if (pc == 0x107500e8 && !impl_->outline_contexts.empty()) { impl_->outline_contexts.pop_back(); return; }
    if (pc == 0x10769460) { impl_->fade_in.push_back(std::int32_t(s.r<4>(28)) > 0); return; }
    if (pc == 0x107697b0 && !impl_->fade_in.empty()) { impl_->fade_in.pop_back(); return; }
    if (!impl_->captures.empty() && impl_->captures.back().exit == pc) {
        impl_->captures.pop_back(); return;
    }
    if (pc == 0x1076b138) {
        if (impl_->saving.empty()) return;
        const auto saved = impl_->saving.back(); impl_->saving.pop_back();
        const auto token = std::uint32_t(s.r<4>(12));
        if (!token || !impl_->surfaces.contains(saved.surface)) return;
        auto& surface = impl_->surfaces.at(saved.surface); surface.flush();
        NativeDrawing image{saved.rect.width, saved.rect.height, std::vector<std::uint8_t>(std::size_t(saved.rect.width) * saved.rect.height * 4), {}};
        blit(image, surface.scene, saved.rect, 0, 0);
        const auto pixels = s.memory.read<4>(token + 12);
        if (pixels && s.memory.read<4>(token) == unsigned(image.width) && s.memory.read<4>(token + 4) == unsigned(image.height))
            impl_->saved.insert_or_assign(token, Impl::SavedDrawing{std::move(image), std::uint32_t(pixels),
                s.memory.copy_out(pixels, std::size_t(saved.rect.width) * saved.rect.height * 2)});
        return;
    }
    if (pc == 0x1076b7e0) { impl_->saved.erase(s.r<4>(12)); return; }
    if (pc == 0x1076c0b0 || pc == 0x1076c980) { impl_->ink_bounds.clear(); return; }
    auto* surface = impl_->active(s);
    if (!surface) return;
    const auto reg = [&](unsigned n) { return std::int32_t(s.r<4>(n * 4)); };
    if (pc == 0x107696cc && !impl_->fade_in.empty()) {
        const auto saved = impl_->saved_scene(s, reg(26));
        if (!saved || reg(30) <= 0) return;
        surface->flush();
        auto frame = *saved;
        const auto step = impl_->fade_in.back() ? reg(28) : reg(30) - reg(28);
        shade_scene(frame, std::clamp(step * 100 / reg(30), 0, 100));
        const auto sp = s.r<4>(4);
        blit(surface->scene, frame, {0, 0, frame.width, frame.height},
             std::int16_t(s.memory.read<2>(sp + 0x46)), std::int16_t(s.memory.read<2>(sp + 0x44)));
        return;
    }
    if (pc == 0x10769980 || pc == 0x107697c0) {
        const int left = std::int16_t(std::min(reg(3), reg(5))), top = std::int16_t(std::min(reg(4), reg(6)));
        const int right = std::int16_t(std::max(reg(3), reg(5))), bottom = std::int16_t(std::max(reg(4), reg(6)));
        const auto region = intersection({left, top, right - left + 1, bottom - top + 1}, {0, 0, surface->scene.width, surface->scene.height});
        if (region.empty()) return;
        surface->flush();
        NativeDrawing shaded{region.width, region.height, std::vector<std::uint8_t>(std::size_t(region.width) * region.height * 4), {}};
        blit(shaded, surface->scene, region, 0, 0);
        shade_scene(shaded, pc == 0x10769980 ? 60 : 100, pc == 0x107697c0);
        blit(surface->scene, shaded, {0, 0, region.width, region.height}, region.x, region.y);
        impl_->captures.push_back({pc == 0x10769980 ? 0x10769b48u : 0x107698c4u, surface->address}); return;
    }
    if (pc == 0x1074fb58 && !impl_->outline_contexts.empty()) {
        // The original optional soft shadow is a deliberately blurred bitmap.
        // Retain that artwork; only the foreground glyph uses the outline.
        if (impl_->outline_contexts.back().flags & 0x10) return;
        const auto record = std::uint32_t(reg(30));
        const auto width = s.memory.read<4>(record + 8), height = s.memory.read<4>(record + 16);
        const auto pitch = s.memory.read<4>(record + 12), pixels = s.memory.read<4>(record + 20);
        if (!pixels || pitch < width || pitch > 512) return;
        auto bounds = impl_->glyph_bounds(s, pixels, width, height, pitch, false);
        if (bounds.empty()) return;
        const auto sp = s.r<4>(4), text = s.memory.read<4>(sp + 0xd4);
        const auto character = s.memory.read<1>(text + std::int16_t(reg(31)));
        if (character <= 32 || character == '|') return;
        surface->flush();
        bounds.x += reg(0); bounds.y += reg(4);
        NativeShape shape; shape.bounds = bounds;
        shape.font = impl_->outline_contexts.back().font; shape.color = impl_->outline_contexts.back().color;
        shape.character = character;
        if (auto outline = impl_->glyph_outlines.find(record); outline != impl_->glyph_outlines.end()) {
            shape.outline = outline->second;
            shape.origin_x = reg(0) - std::int32_t(s.memory.read<4>(record)) / 64.0;
            shape.baseline = reg(4) + std::int32_t(s.memory.read<4>(record + 4)) / 64.0;
        }
        const auto clip = intersection(bounds, {reg(8), reg(5), reg(7) - reg(8) + 1, reg(10) - reg(5) + 1});
        if (!clip.empty()) { shape.clips = {clip}; surface->append(std::move(shape)); }
        impl_->captures.push_back({0x107500b4, surface->address}); return;
    }
    if (pc == 0x1076b020) {
        const int left = std::int16_t(std::min(reg(3), reg(5))), top = std::int16_t(std::min(reg(4), reg(6)));
        const int right = std::int16_t(std::max(reg(3), reg(5))), bottom = std::int16_t(std::max(reg(4), reg(6)));
        impl_->saving.push_back({{left, top, right - left + 1, bottom - top + 1}, surface->address}); return;
    }
    if (pc == 0x1076b140) {
        const auto saved = impl_->saved_scene(s, reg(5));
        if (!saved) return;
        surface->flush();
        blit(surface->scene, *saved, {0, 0, saved->width, saved->height}, std::int16_t(reg(3)), std::int16_t(reg(4)));
        impl_->captures.push_back({0x1076b234, surface->address}); return;
    }
    if (pc == 0x1076ca50) {
        const auto toc = s.r<4>(8);
        const auto font = std::int16_t(reg(5));
        if (font < 1 || font > 7 || !reg(7)) return;
        // The bitmap-font branch has an explicitly characterized glyph/kerning
        // table. The alternate outline engine is observed at its glyph boundary.
        if (std::int32_t(s.memory.read<4>(s.memory.read<4>(toc - 0x740c))) != -1) return;
        const auto table = s.memory.read<4>(toc - 0x3bdc) + (font - 1) * 0x1404;
        const auto height = s.memory.read<4>(table);
        if (!height || height > 128) return;
        surface->flush();
        int x = reg(3);
        for (unsigned i = 0; i < 4096; ++i) {
            auto c = unsigned(s.memory.read<1>(std::uint32_t(reg(7)) + i));
            if (!c) break;
            if (c == '|') c = ' ';
            if (c < 32) continue;
            const auto record = table + c * 20;
            const auto width = s.memory.read<4>(record + 4), pixels = s.memory.read<4>(record + 20);
            if (width > 128) return;
            auto bounds = impl_->glyph_bounds(s, pixels, width, height);
            if (c != ' ' && !bounds.empty()) {
                bounds.x += x; bounds.y += reg(4);
                NativeShape shape; shape.bounds = bounds; shape.color = reg(6); shape.font = font; shape.character = c;
                surface->append(std::move(shape));
            }
            auto next = unsigned(s.memory.read<1>(std::uint32_t(reg(7)) + i + 1));
            if (next == '|') next = ' ';
            int spacing = 0;
            if (next) spacing = std::max(0, int(s.memory.read<4>(table + 32 * 20 + 4) * 3 / 4) -
                int(s.memory.read<4>(record + 16)) - int(s.memory.read<4>(table + next * 20 + 12)));
            x += int(width) + spacing;
        }
        impl_->captures.push_back({0x1076d194, surface->address}); return;
    }
    if (pc == 0x1076a140 || pc == 0x1076a580 || pc == 0x1076ad50) {
        if (std::any_of(impl_->captures.begin(), impl_->captures.end(), [](const auto& c) { return c.exit == 0x1076b014; })) return;
        if (pc == 0x1076a580 && (reg(7) & 3)) return; // Its four line calls retain the original edge/dash order.
        surface->flush();
        NativeShape shape;
        shape.kind = pc == 0x1076a140 ? NativeShape::Kind::line : pc == 0x1076ad50 ? NativeShape::Kind::ellipse : NativeShape::Kind::fill;
        shape.bounds = {std::min(reg(3), reg(5)), std::min(reg(4), reg(6)), std::abs(reg(5) - reg(3)), std::abs(reg(6) - reg(4))};
        shape.x2 = reg(5); shape.y2 = reg(6); shape.color = reg(8); shape.style = reg(7);
        if (shape.kind == NativeShape::Kind::line) {
            ++shape.bounds.width; ++shape.bounds.height;
            // The stroke keeps its direction, separately from its clipping box.
            shape.x1 = reg(3); shape.y1 = reg(4);
            if ((shape.x1 == shape.x2 || shape.y1 == shape.y2) && (shape.style & 2)) shape.kind = NativeShape::Kind::fill;
        } else if (shape.kind == NativeShape::Kind::ellipse) {
            const int radius = (reg(6) - reg(4) - 1) / 2;
            if (radius < 1) return;
            const int cx = reg(3) + 1 + (reg(5) - reg(3) - 1) / 2;
            const int cy = reg(4) + 1 + (reg(6) - reg(4) - 1) / 2;
            shape.bounds = {cx - radius, cy - radius, radius * 2 + 1, radius * 2 + 1};
            const auto clip = intersection(shape.bounds, {std::min(reg(3), reg(5)), std::min(reg(4), reg(6)), std::abs(reg(5) - reg(3)) + 1, std::abs(reg(6) - reg(4)) + 1});
            if (clip.empty()) return;
            shape.clips = {clip};
        }
        surface->append(std::move(shape));
        impl_->captures.push_back({pc == 0x1076a140 ? 0x1076a570u : pc == 0x1076ad50 ? 0x1076b014u : 0x1076a784u, surface->address});
    }
}
void native_drawing_boundary(State& s, std::uint32_t pc) { if (s.platform) s.platform->observe_drawing(s, pc); }
}
