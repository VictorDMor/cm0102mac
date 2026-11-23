// The font engine, block `autogrid_a` (AUTOGRID.C's first half, 0x10751350..0x10753560): cases for the height and stem analysis of the autogridder.
// The game never hints (FONT.CPP passes hint = 0 and cmd = 2), so no capture reaches these routines; the cases build the autogridder's object from real
// data and run the routines on it:
//  * the object's font data (heights, stem widths, units per em, the size scaling) comes from the ORIGINAL engine: a scaler of the snapshot (the .t2k fonts,
//    which carry a T2KG table) is made to hint at a chosen size (0x1075eb10), and the object the hinter makes is copied out;
//  * the glyphs are real outlines of the shipped fonts (loaded by the original 0x107664a0 and copied out) plus synthetic ones (empty, single points, two and
//    three points, off-curve runs, collinear and equal-coordinate runs past the walkers' limits, exact thresholds);
//  * each (object, glyph) is laid out in the scratch area and the ORIGINAL runs the earlier steps of the pipeline on it (0x10751fd0 the directions, then
//    0x10752990 the classification) while a child process records the memory after each step: those three states are the cases' memory, so a case is a plain
//    list of writes (and the mutated runs change values the call reads).
// Labels start with "autogrid_a:" (CM_FONT_ENGINE_BLOCK=autogrid_a).
#include "font_engine_adapter.hpp"
#include "font_engine_checks.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <random>
#include <sys/wait.h>
#include <unistd.h>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t scale_heights = 0x10751350, band_entry = 0x10751b10, nearest = 0x10751bd0, normalize = 0x10751eb0, directions = 0x10751fd0, extrema_entry = 0x107522b0,
                        inflections = 0x107524b0, classify = 0x10752990, link_entry = 0x10753110;
constexpr std::uint32_t hint_prepare = 0x1075eb10, glyph_loader = 0x107664a0, character_glyph = 0x10766770;
constexpr std::uint32_t reporter = 0x107755c0, game_toc = 0x108bba90;
// The scratch area: the object, then the arrays, a region each.
constexpr std::uint32_t area = scratch_base + 0x20000, area_size = 0x20000, obj_at = area, region = 0x1800, first_region = area + 0x1000;
enum Region { starts_r, ends_r, on_r, xs_r, ys_r, next_r, prev_r, extrema_r, flags_r, curve_x_r, curve_y_r, before_r, after_r, in_x_r, in_y_r, out_x_r, out_y_r, regions };
constexpr std::uint32_t at_region(Region r) { return first_region + region * std::uint32_t(r); }
constexpr std::uint32_t outside = scratch_base + 0x58000;  // the child's own words (the loader's advance outputs)
constexpr std::uint32_t object_size = 0x45c;
// The object's fields.
[[maybe_unused]] constexpr std::uint32_t f_contours = 0x4, f_starts = 0x8, f_ends = 0xc, f_on = 0x10, f_xs = 0x14, f_ys = 0x18, f_mode = 0x1c, f_next = 0x20, f_prev = 0x24, f_extrema = 0x28,
                        f_flags = 0x2c, f_curve_x = 0x30, f_curve_y = 0x34, f_before = 0x38, f_after = 0x3c, f_in_x = 0x44, f_in_y = 0x48, f_out_x = 0x4c, f_out_y = 0x50,
                        f_scaled = 0x60, f_heights = 0x1e0, f_em = 0x2a0, f_size_x = 0x2a4, f_size_y = 0x2a8, f_flavor = 0x2ac, f_limits = 0x2e0, f_capacity = 0x350,
                        f_records = 0x354, f_stems_x = 0x390, f_stems_y = 0x3a8, f_keep_flat = 0x454;

using Bytes = std::vector<std::uint8_t>;
struct Glyph {
    std::string name;
    std::vector<int> starts, ends;  // per contour
    std::vector<int> xs, ys, on;    // per point, the four phantom points last
    int mode = 0;
    unsigned points() const { return unsigned(xs.size()); }
};
// One state of an (object, glyph): the memory of the scratch area after the layout (stage 0), the directions (1) and the classification (2).
struct Fixture {
    std::string name;
    std::uint32_t em = 0, size = 0;
    int contours = 0;
    unsigned points = 0;
    std::array<std::vector<Write>, 3> stage;
    std::array<bool, 3> ok{false, false, false};
};

// A halfword of a stage's memory (0 when it holds none).
std::uint32_t half_of(const Fixture& fixture, std::uint32_t address) {
    for (const auto& w : fixture.stage[2]) if (w.address == (address & ~3u)) return (address & 2) ? (w.value & 0xffff) : (w.value >> 16);
    return 0;
}
std::uint32_t be32(const Bytes& bytes, std::size_t at) { return std::uint32_t(bytes[at]) << 24 | std::uint32_t(bytes[at + 1]) << 16 | std::uint32_t(bytes[at + 2]) << 8 | bytes[at + 3]; }

// ---- The child's side ----------------------------------------------------------------------------------------------------------------------
// The object and the arrays of a glyph in the scratch area: the object as given with its pointers set, the contour tables, the points, and the next and
// previous tables the hinter's own first loop builds (0x10753560).
void lay(guest::Cpu& c, const Bytes& object, const Glyph& glyph) {
    for (std::uint32_t a = area; a < area + area_size; a += 4) c.put32(a, 0);
    for (std::uint32_t n = 0; n < object_size; n += 4) c.put32(obj_at + n, be32(object, n));
    const auto count = std::uint32_t(glyph.starts.size());
    c.put16(obj_at + f_contours, count);
    for (const auto& [field, r] : {std::pair{f_starts, starts_r}, {f_ends, ends_r}, {f_on, on_r}, {f_xs, xs_r}, {f_ys, ys_r}, {f_next, next_r}, {f_prev, prev_r},
                                   {f_extrema, extrema_r}, {f_flags, flags_r}, {f_curve_x, curve_x_r}, {f_curve_y, curve_y_r}, {f_before, before_r}, {f_after, after_r},
                                   {f_in_x, in_x_r}, {f_in_y, in_y_r}, {f_out_x, out_x_r}, {f_out_y, out_y_r}})
        c.put32(obj_at + field, at_region(r));
    c.put16(obj_at + f_mode, std::uint32_t(glyph.mode));
    for (std::uint32_t k = 0; k < count; ++k) {
        c.put16(at_region(starts_r) + 2 * k, std::uint32_t(glyph.starts[k]));
        c.put16(at_region(ends_r) + 2 * k, std::uint32_t(glyph.ends[k]));
    }
    for (std::uint32_t n = 0; n < glyph.points(); ++n) {
        c.put16(at_region(xs_r) + 2 * n, std::uint32_t(glyph.xs[n]));
        c.put16(at_region(ys_r) + 2 * n, std::uint32_t(glyph.ys[n]));
        c.put8(at_region(on_r) + n, std::uint32_t(glyph.on[n]));
    }
    // 0x10753560's first loop: previous[i] = i - 1 and next[i] = i + 1 round each contour (a one-point contour is its own neighbour).
    for (std::uint32_t k = 0; k < count; ++k) {
        const int first = glyph.starts[k], last = glyph.ends[k];
        const auto prev = [&](int i) { return at_region(prev_r) + 2 * std::uint32_t(i); };
        const auto next = [&](int i) { return at_region(next_r) + 2 * std::uint32_t(i); };
        if (last <= first) { c.put16(prev(first), std::uint32_t(first)); c.put16(next(first), std::uint32_t(first)); continue; }
        int before = last;
        for (int i = first; i <= last; ++i) {
            c.put16(prev(i), std::uint32_t(before));
            c.put16(next(i), std::uint32_t(i + 1));
            before = i;
        }
        c.put16(next(last), std::uint32_t(first));
    }
}
std::vector<Write> dump(guest::Cpu& c) {
    std::vector<Write> writes;
    for (std::uint32_t a = area; a < area + area_size; a += 4)
        if (const auto value = c.u32(a)) writes.push_back(w32(a, value));
    return writes;
}
// The three stages of one glyph in one object; the ORIGINAL runs the steps. A step that faults leaves its stage (and the later ones) unrecorded.
Fixture run_stages(State& s, const Bytes& object, const Glyph& glyph) {
    guest::Cpu c(s);
    Fixture fixture;
    fixture.name = glyph.name;
    fixture.contours = int(glyph.starts.size());
    fixture.points = glyph.points();
    fixture.em = be32(object, f_em) >> 16;
    fixture.size = be32(object, f_size_y);
    c.set(2, game_toc);
    lay(c, object, glyph);
    fixture.stage[0] = dump(c);
    fixture.ok[0] = true;
    try {
        for (unsigned step = 0; step < 2; ++step) {
            c.set({{3, obj_at}, {9, 0x91919191}, {10, 0xa0a0a0a0}});
            s.budget = s.executed + 400'000'000ULL;
            font_engine_run_original(s, step == 0 ? directions : classify);
            fixture.stage[step + 1] = dump(c);
            fixture.ok[step + 1] = true;
        }
    } catch (...) {}
    return fixture;
}
// ---- The wire between the child and the parent: a flat list of words ----
void send(int fd, const std::vector<std::uint32_t>& words) {
    std::size_t done = 0;
    while (done < words.size() * 4) {
        const auto n = ::write(fd, reinterpret_cast<const char*>(words.data()) + done, words.size() * 4 - done);
        if (n <= 0) return;
        done += std::size_t(n);
    }
}
void encode(std::vector<std::uint32_t>& out, const Fixture& f) {
    out.push_back(std::uint32_t(f.name.size()));
    for (const char ch : f.name) out.push_back(std::uint32_t(std::uint8_t(ch)));
    out.push_back(f.em); out.push_back(f.size); out.push_back(std::uint32_t(f.contours)); out.push_back(f.points);
    for (unsigned k = 0; k < 3; ++k) {
        out.push_back(f.ok[k] ? 1u : 0u);
        out.push_back(std::uint32_t(f.stage[k].size()));
        for (const auto& w : f.stage[k]) { out.push_back(w.address); out.push_back(w.value); }
    }
}
// Forks a child that runs `work` on the scenario's own state and sends its words; returns them (empty on failure).
template <class Work>
std::vector<std::uint32_t> in_child(const Scenario& scenario, Work work) {
    int fds[2];
    if (::pipe(fds) != 0) return {};
    const auto pid = ::fork();
    if (pid == 0) {
        ::close(fds[0]);
        std::vector<std::uint32_t> out;
        try { out = work(scenario.s); } catch (...) { _exit(2); }
        send(fds[1], out);
        ::close(fds[1]);
        _exit(0);
    }
    ::close(fds[1]);
    std::vector<std::uint32_t> words(1 << 16);
    std::size_t have = 0;
    for (;;) {
        if (have * 4 + 65536 > words.size() * 4) words.resize(words.size() * 2);
        const auto n = ::read(fds[0], reinterpret_cast<char*>(words.data()) + have * 4, 65536);
        if (n <= 0) break;
        have += std::size_t(n) / 4;
    }
    ::close(fds[0]);
    int status = 0;
    ::waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return {};
    words.resize(have);
    return words;
}
std::vector<Fixture> decode(const std::vector<std::uint32_t>& words) {
    std::vector<Fixture> fixtures;
    std::size_t at = 0;
    while (at + 6 < words.size()) {
        Fixture f;
        const auto length = words[at++];
        for (std::uint32_t n = 0; n < length; ++n) f.name += char(words[at++]);
        f.em = words[at++]; f.size = words[at++]; f.contours = int(words[at++]); f.points = words[at++];
        for (unsigned k = 0; k < 3; ++k) {
            f.ok[k] = words[at++] != 0;
            const auto count = words[at++];
            for (std::uint32_t n = 0; n < count; ++n) { f.stage[k].push_back(w32(words[at], words[at + 1])); at += 2; }
        }
        fixtures.push_back(std::move(f));
    }
    return fixtures;
}

// ---- Real objects and glyphs: a scaler of the snapshot made to hint at a size by the original ----
struct Scaler { std::uint32_t t2k, sfnt; };
std::vector<Scaler> t2kg_scalers(const Scenario& scenario) {
    std::vector<Scaler> found;
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4)
            if (scenario.r32(std::uint32_t(at)) == 0x5a1234a5 && scenario.r32(std::uint32_t(at) + 0x5f4) == 0xa5fedc5a) {
                const auto t = std::uint32_t(at), sfnt = scenario.r32(t + 0x144);
                const auto directory = scenario.r32(sfnt), count = scenario.u16(directory + 8), array = scenario.r32(directory + 0x10);
                bool t2kg = false;
                for (std::uint32_t n = 0; n < count; ++n) t2kg = t2kg || scenario.r32(scenario.r32(array + 4 * n) + 4) == 0x54324b47;
                if (t2kg && scenario.r32(t + 0x148) == 0 && scenario.r32(t + 0x5f4) == 0xa5fedc5a) found.push_back({t, sfnt});
            }
    }
    // One scaler per distinct font file (the loaded fonts repeat for each style and size; the two small fonts are the same file).
    std::vector<Scaler> distinct;
    std::vector<std::uint32_t> lengths;
    for (const auto& scaler : found) {
        const auto length = scenario.r32(scenario.r32(scaler.sfnt + 0x54) + 8);
        if (std::ranges::find(lengths, length) != lengths.end()) continue;
        lengths.push_back(length);
        distinct.push_back(scaler);
    }
    return distinct;
}

// The child that makes the real fixtures: for each font file, size and character the engine's own steps (0x1075eb10 the hinter and its data for the size,
// 0x10766770 the glyph of the character, 0x107664a0 the glyph) and the three stages of the glyph.
std::vector<std::uint32_t> real_fixtures(State& s, const std::vector<Scaler>& scalers, const std::vector<std::uint32_t>& sizes, const std::string& characters) {
    guest::Cpu c(s);
    c.set(2, game_toc);
    std::vector<std::uint32_t> out;
    unsigned font = 0;
    for (const auto& scaler : scalers) {
        ++font;
        for (const auto size : sizes) {
            const auto t = scaler.t2k;
            c.put32(t + 0xe8, size); c.put32(t + 0xec, size); c.put32(t + 0x148, 0); c.put32(t + 0x12c, 0); c.put32(t + 0x130, 0); c.put32(scaler.sfnt + 0x38, 0);
            c.set({{3, t}, {9, 0}, {10, 0}});
            s.budget = s.executed + 400'000'000ULL;
            font_engine_run_original(s, hint_prepare);
            const auto handle = c.u32(t + 0x148);
            if (handle == 0) continue;
            Bytes object(object_size);
            for (std::uint32_t n = 0; n < object_size; ++n) object[n] = c.u8(handle + n);
            for (const char ch : characters) {
                c.set({{3, scaler.sfnt}, {4, std::uint32_t(ch)}});
                font_engine_run_original(s, character_glyph);
                const auto index = c.r(3);
                if (index == 0) continue;
                c.set({{3, scaler.sfnt}, {4, index}, {5, 0}, {6, outside}, {7, outside + 4}});
                font_engine_run_original(s, glyph_loader);
                const auto record = c.r(3);
                if (record == 0 || c.i16(record + 0x36) <= 0) continue;
                Glyph glyph;
                glyph.name = "font " + std::to_string(font) + " size " + std::to_string(size) + " '" + std::string(1, ch) + "'";
                const int contours = c.i16(record + 0x36), points = c.i16(record + 0x38);
                if (points <= 0 || points > 300) continue;
                for (int k = 0; k < contours; ++k) {
                    glyph.starts.push_back(c.i16(c.u32(record + 0x3c) + 2 * std::uint32_t(k)));
                    glyph.ends.push_back(c.i16(c.u32(record + 0x40) + 2 * std::uint32_t(k)));
                }
                for (int n = 0; n < points + 4; ++n) {
                    glyph.xs.push_back(c.i16(c.u32(record + 0x44) + 2 * std::uint32_t(n)));
                    glyph.ys.push_back(c.i16(c.u32(record + 0x48) + 2 * std::uint32_t(n)));
                    glyph.on.push_back(c.u8(c.u32(record + 0x4c) + std::uint32_t(n)));
                }
                encode(out, run_stages(s, object, glyph));
                c.set(2, game_toc);
            }
        }
    }
    return out;
}
// The object bytes of one real fixture, for the synthetic glyphs (the first scaler at one size).
std::vector<std::uint32_t> real_object(State& s, const Scaler& scaler, std::uint32_t size) {
    guest::Cpu c(s);
    c.set(2, game_toc);
    const auto t = scaler.t2k;
    c.put32(t + 0xe8, size); c.put32(t + 0xec, size); c.put32(t + 0x148, 0); c.put32(t + 0x12c, 0); c.put32(t + 0x130, 0); c.put32(scaler.sfnt + 0x38, 0);
    c.set({{3, t}, {9, 0}, {10, 0}});
    s.budget = s.executed + 400'000'000ULL;
    font_engine_run_original(s, hint_prepare);
    const auto handle = c.u32(t + 0x148);
    std::vector<std::uint32_t> out;
    if (handle == 0) return out;
    for (std::uint32_t n = 0; n < object_size; n += 4) out.push_back(c.u32(handle + n));
    return out;
}
Bytes bytes_of(const std::vector<std::uint32_t>& words) {
    Bytes bytes;
    for (const auto word : words) for (int shift = 24; shift >= 0; shift -= 8) bytes.push_back(std::uint8_t(word >> shift));
    return bytes;
}

// ---- Synthetic glyphs ----
struct Pt { int x, y; bool on = true; };
using Shape = std::vector<std::vector<Pt>>;
Glyph make_glyph(const std::string& name, const Shape& shape, int advance = 1000) {
    Glyph glyph;
    glyph.name = name;
    int index = 0;
    for (const auto& contour : shape) {
        glyph.starts.push_back(index);
        for (const auto& p : contour) { glyph.xs.push_back(p.x); glyph.ys.push_back(p.y); glyph.on.push_back(p.on ? 1 : 0); ++index; }
        glyph.ends.push_back(index - 1);
    }
    // The phantom points: the origin, the advance, the top and the bottom.
    for (const auto& p : {Pt{0, 0}, Pt{advance, 0}, Pt{0, 1000}, Pt{0, -200}}) { glyph.xs.push_back(p.x); glyph.ys.push_back(p.y); glyph.on.push_back(1); }
    return glyph;
}
Shape random_shape(std::mt19937& random, int range, unsigned contours, unsigned max_points, int curvy) {
    Shape shape(contours);
    for (auto& contour : shape) {
        contour.resize(1 + random() % max_points);
        for (auto& p : contour) {
            p.x = int(random() % (2 * range + 1)) - range;
            p.y = int(random() % (2 * range + 1)) - range;
            p.on = curvy == 0 || random() % unsigned(curvy) != 0;
        }
    }
    return shape;
}
std::vector<Glyph> synthetic_glyphs(std::mt19937& random) {
    std::vector<Glyph> glyphs;
    const auto add = [&](const std::string& name, const Shape& shape) { glyphs.push_back(make_glyph("synthetic " + name, shape)); };
    add("empty", {});
    add("one point", {{{300, 400}}});
    add("two points", {{{0, 0}, {500, 700}}});
    add("three points", {{{0, 0}, {500, 0}, {250, 700}}});
    add("square", {{{100, 0}, {100, 700}, {600, 700}, {600, 0}}});
    add("square counter-clockwise", {{{100, 0}, {600, 0}, {600, 700}, {100, 700}}});
    add("square with a hole", {{{100, 0}, {100, 700}, {600, 700}, {600, 0}}, {{200, 100}, {500, 100}, {500, 600}, {200, 600}}});
    add("rectangle 65532 wide", {{{-32766, 0}, {-32766, 100}, {32766, 100}, {32766, 0}}});
    add("H", {{{100, 0}, {100, 700}, {200, 700}, {200, 400}, {500, 400}, {500, 700}, {600, 700}, {600, 0}, {500, 0}, {500, 300}, {200, 300}, {200, 0}}});
    add("L", {{{100, 0}, {100, 700}, {200, 700}, {200, 100}, {600, 100}, {600, 0}}});
    add("o as eight off-curve points", {{{0, 350, false}, {0, 700, false}, {350, 700, false}, {700, 700, false}, {700, 350, false}, {700, 0, false}, {350, 0, false}, {0, 0, false}}});
    add("o as quadratic arcs", {{{350, 0}, {700, 0, false}, {700, 350}, {700, 700, false}, {350, 700}, {0, 700, false}, {0, 350}, {0, 0, false}}});
    add("o with a counter", {{{350, 0}, {700, 0, false}, {700, 350}, {700, 700, false}, {350, 700}, {0, 700, false}, {0, 350}, {0, 0, false}},
                             {{350, 100}, {200, 100, false}, {200, 350}, {200, 600, false}, {350, 600}, {500, 600, false}, {500, 350}, {500, 100, false}}});
    add("S curve", {{{100, 100}, {300, 0, false}, {500, 200, false}, {400, 350}, {250, 450, false}, {350, 650, false}, {600, 700}, {600, 600}, {400, 550, false}, {450, 400}, {550, 250, false}, {300, 100}}});
    add("diamond", {{{350, 0}, {700, 350}, {350, 700}, {0, 350}}});
    add("collinear points", {{{0, 0}, {100, 0}, {200, 0}, {300, 0}, {400, 0}, {400, 100}, {400, 200}, {400, 300}, {300, 300}, {200, 300}, {100, 300}, {0, 300}, {0, 200}, {0, 100}}});
    add("all points equal", {{{50, 50}, {50, 50}, {50, 50}, {50, 50}}});
    add("duplicate points", {{{0, 0}, {0, 0}, {500, 0}, {500, 0}, {500, 500}, {0, 500}, {0, 500}}});
    add("tiny", {{{0, 0}, {1, 0}, {1, 1}, {0, 1}}});
    add("extremes", {{{-32768, -32768}, {32767, -32768}, {32767, 32767}, {-32768, 32767}}});
    add("extremes with curves", {{{-32768, 0}, {-32768, 32767, false}, {32767, 32767}, {32767, -32768, false}}});
    add("almost collinear", {{{0, 0}, {100, 1}, {200, 0}, {300, 1}, {400, 0}, {200, 600}}});
    add("nine contours", [] { Shape shape; for (int k = 0; k < 9; ++k) shape.push_back({{100 * k, 0}, {100 * k + 50, 300}, {100 * k + 80, 10}}); return shape; }());
    // A run of 140 points with one x (past the 0x81 steps the extremum walk goes back), and with one y.
    add("equal x run of 140", [] { std::vector<Pt> c; for (int k = 0; k < 140; ++k) c.push_back({100, 10 * k}); c.push_back({300, 700}); c.push_back({300, 0}); return Shape{c}; }());
    add("equal y run of 140", [] { std::vector<Pt> c; for (int k = 0; k < 140; ++k) c.push_back({10 * k, 100}); c.push_back({700, 300}); c.push_back({0, 300}); return Shape{c}; }());
    add("all x equal, 140 points", [] { std::vector<Pt> c; for (int k = 0; k < 140; ++k) c.push_back({100, 7 * k}); return Shape{c}; }());
    add("all y equal, 20 points", [] { std::vector<Pt> c; for (int k = 0; k < 20; ++k) c.push_back({7 * k, 100}); return Shape{c}; }());
    // A run of N equal coordinates just BEFORE the contour's first point (the walk back from it stops after 0x81 steps): N around that limit, in x and in y.
    for (const int n : {0x7e, 0x7f, 0x80, 0x81, 0x82}) {
        add("run of " + std::to_string(n) + " equal x before the start", [n] { std::vector<Pt> c{{100, 0}, {300, 0}, {300, 300}, {500, 300}}; for (int k = 0; k < n; ++k) c.push_back({100, 700 - 4 * k}); return Shape{c}; }());
        add("run of " + std::to_string(n) + " equal y before the start", [n] { std::vector<Pt> c{{0, 100}, {0, 300}, {300, 300}, {300, 500}}; for (int k = 0; k < n; ++k) c.push_back({700 - 4 * k, 100}); return Shape{c}; }());
    }
    // A long smooth contour: an off-curve circle of 40 points (inflection walks of more than eight steps) and a wavy one.
    add("circle of 40", [] { std::vector<Pt> c; for (int k = 0; k < 40; ++k) c.push_back({int(500 * std::cos(k * 0.15707963)), int(500 * std::sin(k * 0.15707963)), k % 2 == 0}); return Shape{c}; }());
    add("wave of 60", [] { std::vector<Pt> c; for (int k = 0; k < 60; ++k) c.push_back({10 * k, int(200 * std::sin(k * 0.3)), k % 3 != 1}); c.push_back({600, -300}); c.push_back({0, -300}); return Shape{c}; }());
    add("wave of 40 all off", [] { std::vector<Pt> c; for (int k = 0; k < 40; ++k) c.push_back({12 * k, int(150 * std::sin(k * 0.45)), false}); return Shape{c}; }());
    for (unsigned n = 0; n < 40; ++n) add("random " + std::to_string(n), random_shape(random, n % 4 == 0 ? 20 : 700, 1 + random() % 3, 2 + random() % 14, n % 3 == 0 ? 0 : 3));
    for (unsigned n = 0; n < 8; ++n) add("random big " + std::to_string(n), random_shape(random, 32767, 1 + random() % 2, 2 + random() % 10, 2));
    for (unsigned n = 0; n < 6; ++n) add("random long " + std::to_string(n), random_shape(random, 600, 1, 60 + random() % 80, n % 2 ? 0 : 2));
    return glyphs;
}
}

FONT_ENGINE_CASES("autogrid_a") {
    const bool exhaustive = std::getenv("CM_FONT_ENGINE_EXHAUSTIVE") != nullptr;
    // Every glyph for the walkers (the extrema, the inflections, the classification, the links), which the default list thins (see below): CM_FONT_ENGINE_AG_WALKERS=1.
    const bool all_walkers = exhaustive || std::getenv("CM_FONT_ENGINE_AG_WALKERS") != nullptr;
    std::mt19937 random(0xa6a1d);
    const auto scalers = t2kg_scalers(scenario);
    if (scalers.empty()) return;
    // The real fixtures: every font file at three sizes (the sizes of the game's small, medium and large text), a dozen characters; and the synthetic glyphs on
    // the object of the first font at its middle size.
    const std::string characters = exhaustive ? "AaBbEegGHhiLloOpQrSsTtWwXxyZ0123456789&%@.,:;-" : "aBegiMoS8&%.";
    std::vector<Fixture> fixtures = decode(in_child(scenario, [&](State& s) { return real_fixtures(s, scalers, {9, 17, 40}, characters); }));
    const auto object_words = in_child(scenario, [&](State& s) { return real_object(s, scalers.front(), 17); });
    if (fixtures.empty() || object_words.size() != object_size / 4) {
        std::fprintf(stderr, "autogrid_a: no real fixtures (%zu fixtures, %zu object words)\n", fixtures.size(), object_words.size());
        return;
    }
    const Bytes object = bytes_of(object_words);
    const auto synthetic = synthetic_glyphs(random);
    {
        auto made = decode(in_child(scenario, [&](State& s) {
            std::vector<std::uint32_t> out;
            for (const auto& glyph : synthetic) encode(out, run_stages(s, object, glyph));
            return out;
        }));
        for (auto& f : made) fixtures.push_back(std::move(f));
    }
    const auto first_case = cases.size();
    const auto toc = std::pair<unsigned, std::uint32_t>{2, game_toc};
    const std::pair<unsigned, std::uint32_t> r9{9, 0x91919191}, r10{10, 0xa0a0a0a0};
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, const std::string& label) -> Case& {
        registers.push_back(toc);
        registers.push_back(r9);
        registers.push_back(r10);
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = "autogrid_a: " + label, .answers = {{reporter, 0}}});
        return cases.back();
    };
    const auto join = [](std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
    const auto word_at = [](Region r, std::uint32_t index) { return at_region(r) + 4 * index; };
    const auto half_at = [](Region r, std::uint32_t index) { return at_region(r) + 2 * index; };
    const std::uint32_t gray_at = outside + 0x100, input_at = area + area_size - 0x400, output_at = outside + 0x400;
    // The shape of every stage-dependent case: the entry, the stage it needs and what each state is called.
    unsigned fixture_index = 0;
    for (const auto& f : fixtures) {
        // The walkers loop for ever on a mutated next table (each such mutated run costs the alarm, and the first 32 cases of an entry seed its mutated runs):
        // by default a handful of glyphs; CM_FONT_ENGINE_EXHAUSTIVE=1 runs them all.
        (void)fixture_index;
        const bool walks = all_walkers || f.name == "font 1 size 17 'B'" || f.name == "synthetic square with a hole" || f.name == "synthetic equal x run of 140";
        const auto name = f.name + " (" + std::to_string(f.points) + " points)";
        // 0x10751350: the size entry, with and without the scaled values.
        if (f.ok[0]) {
            for (const std::uint32_t scale : {1u, 0u}) add(scale_heights, {{3, obj_at}, {4, scale}, {5, gray_at}}, f.stage[0], name + " scale " + std::to_string(scale) + " " + std::to_string(f.size));
            // 0x10751b10: the band values of the points (the input record: +2 the count, +0x1c the output).
            auto memory = join(f.stage[0], {w16(input_at + 2, f.points - 4), w32(input_at + 0x1c, output_at)});
            add(band_entry, {{3, obj_at}, {4, input_at}}, memory, name + " bands");
            // 0x10751fd0: the directions.
            add(directions, {{3, obj_at}}, f.stage[0], name + " directions");
            // 0x107522b0: the extrema, from the clean flags and from a classified glyph's.
            if (walks) add(extrema_entry, {{3, obj_at}}, f.stage[0], name + " extrema");
        }
        if (f.ok[2] && walks) add(extrema_entry, {{3, obj_at}}, f.stage[2], name + " extrema over classified flags");
        // 0x107524b0: the inflections over the clean flags (no corners) and the classified ones.
        if (f.ok[1] && walks) add(inflections, {{3, obj_at}}, f.stage[1], name + " inflections, clean flags");
        if (f.ok[2] && walks) add(inflections, {{3, obj_at}}, f.stage[2], name + " inflections");
        // 0x10752990: the classification from the directions.
        if (f.ok[1] && walks) add(classify, {{3, obj_at}}, f.stage[1], name + " classify");
        // 0x10753110: the links of the classified points.
        if (f.ok[2]) add(link_entry, {{3, obj_at}}, f.stage[2], name + " link");
        // 0x10751bd0: the nearest height of points, in both modes.
        if (f.ok[2]) {
            std::vector<unsigned> chosen;
            const unsigned limit = f.points > 4 ? f.points - 4 : 0;
            for (unsigned p = 0; p < limit; ++p) if (exhaustive || limit <= 6 || random() % std::max(1u, limit / 5) == 0) chosen.push_back(p);
            for (const auto p : chosen)
                for (const std::uint32_t mode : {0u, 1u})
                    add(nearest, {{3, obj_at}, {4, p}}, join(f.stage[2], {w16(obj_at + f_mode, mode)}), name + " nearest height point " + std::to_string(p) + " mode " + std::to_string(mode));
        }
    }
    // The normalisation of vectors: pairs of values (zero, one axis, the 2.14 extremes, the wrap of the squares, signs).
    {
        const std::array<std::int32_t, 20> values = {0, 1, -1, 2, 3, -3, 100, -100, 0x4000, -0x4000, 0x3fff, 0x7fff, 0x10000, -0x10000, 0x20000, 0x7fffffff, -0x7fffffff, std::int32_t(0x80000000), 0x1000000, 0x12345};
        const std::uint32_t a_at = area + 0x100, b_at = area + 0x104;
        for (const auto a : values)
            for (const auto b : values) {
                auto& item = add(normalize, {{3, a_at}, {4, b_at}}, {w32(a_at, std::uint32_t(a)), w32(b_at, std::uint32_t(b))}, "normalize " + hex(std::uint32_t(a)) + " " + hex(std::uint32_t(b)));
                (void)item;
            }
        for (unsigned n = 0; n < 300; ++n) {
            const auto draw = [&] { return random() % 3 == 0 ? std::uint32_t(random()) : std::uint32_t(std::int32_t(random() % 0x8001) - 0x4000); };
            const auto a = draw(), b = draw();
            add(normalize, {{3, a_at}, {4, b_at}}, {w32(a_at, a), w32(b_at, b)}, "normalize random " + hex(a) + " " + hex(b));
        }
    }
    // ---- Poked cases: the real fixtures' states with fields changed to reach what the glyphs do not ----
    // The unit vectors the directions are poked with: along the axes, at 45 degrees, and values at the thresholds the routines compare (0x3fc2/0x3fc3 of an axis,
    // 0x2cb/0x2cc, the corner dot 0x3fc2, 0x3f07/0x3f08 and the wraps of the products).
    const std::vector<std::pair<std::int32_t, std::int32_t>> palette = {
        {0x4000, 0}, {-0x4000, 0}, {0, 0x4000}, {0, -0x4000}, {0x2d41, 0x2d41}, {-0x2d41, 0x2d41}, {0x2d41, -0x2d41}, {-0x2d41, -0x2d41}, {0x3fc2, 0x300}, {0x3fc3, 0x2f0},
        {-0x3fc2, 0x300}, {-0x3fc3, -0x2f0}, {0x300, 0x3fc2}, {0x2f0, 0x3fc3}, {0x2cb, 0x3fff}, {0x2cc, 0x3fff}, {-0x2cb, 0x3ff0}, {-0x2cc, -0x3ff0}, {0x3fff, 0x2cb},
        {0x3fff, 0x2cc}, {0x3fff, -0x2cb}, {-0x3fff, -0x2cc}, {0x3f07, 0x400}, {0x3f08, 0x400}, {-0x3f07, 0x400}, {-0x3f08, 0x400}, {0, 0}, {0x40000, 0x40000},
        {0x7fffffff, 0x7fffffff}, {std::int32_t(0x80000000), 0}, {0x3fc1, 0}, {0x3fc4, 0}, {0x1000, 0x3d00}, {-0x3e14, 0x700}, {-0x3e13, 0x700}, {0x3e14, -0x700}};
    const auto pick = [&] { return palette[random() % palette.size()]; };
    // The first fixtures of each kind (the real ones, then the synthetic ones with more than a few points).
    std::vector<const Fixture*> base;
    for (const auto& f : fixtures) if (f.ok[2] && f.name.starts_with("font") && f.points > 12) { base.push_back(&f); if (base.size() >= 6) break; }
    for (const auto& f : fixtures) if (f.ok[2] && f.name.starts_with("synthetic") && f.points > 8 && base.size() < 18) base.push_back(&f);
    const auto dirs_poke = [&](const Fixture& f, unsigned mix) {
        std::vector<Write> writes;
        const unsigned n = f.points > 4 ? f.points - 4 : 0;
        for (unsigned i = 0; i < n; ++i) {
            const auto forward = pick();
            auto backward = random() % 3 == 0 ? forward : random() % 3 == 0 ? std::pair{-forward.first, -forward.second} : pick();
            if (mix == 1 && random() % 2 == 0) backward = {forward.first - std::int32_t(random() % 5), forward.second};
            writes.push_back(w32(word_at(in_x_r, i), std::uint32_t(forward.first)));
            writes.push_back(w32(word_at(in_y_r, i), std::uint32_t(forward.second)));
            writes.push_back(w32(word_at(out_x_r, i), std::uint32_t(backward.first)));
            writes.push_back(w32(word_at(out_y_r, i), std::uint32_t(backward.second)));
        }
        return writes;
    };
    const std::array<std::uint16_t, 10> flag_bits = {0x1, 0x2, 0x4, 0x8, 0x80, 0x100, 0x800, 0x1000, 0x2000, 0x4000};
    const auto flags_poke = [&](const Fixture& f, unsigned density) {
        std::vector<Write> writes;
        const unsigned n = f.points > 4 ? f.points - 4 : 0;
        for (unsigned i = 0; i < n + 4 && i < f.points; ++i) {
            std::uint32_t flags = 0;
            for (const auto bit : flag_bits) if (random() % 10 < density) flags |= bit;
            writes.push_back(w16(half_at(flags_r, i), flags));
        }
        return writes;
    };
    const std::array<std::int32_t, 14> ems = {1, 2, 3, 7, 8, 100, 255, 256, 257, 1000, 2048, 0x7fff, -1, -300};
    const unsigned rounds = all_walkers ? 12 : 1;
    unsigned base_index = 0;
    for (const auto* f : base) {
        const auto name = f->name + " poked";
        const bool base_walks = all_walkers || base_index++ % 6 == 0;
        for (unsigned round = 0; round < (base_walks ? rounds : 0u); ++round) {
            const auto em = ems[random() % ems.size()];
            // The classification from poked directions (the thresholds of the axes and the corner dot) and a poked em (the tolerances).
            add(classify, {{3, obj_at}}, join(join(f->stage[1], dirs_poke(*f, round % 2)), {w16(obj_at + f_em, std::uint32_t(em))}), name + " classify em " + std::to_string(em) + " round " + std::to_string(round));
            // The inflections over poked flags and em.
            add(inflections, {{3, obj_at}}, join(join(f->stage[2], flags_poke(*f, 1 + round % 4)), {w16(obj_at + f_em, std::uint32_t(em))}), name + " inflections flags em " + std::to_string(em) + " round " + std::to_string(round));
            // The links over poked directions and flags.
            add(link_entry, {{3, obj_at}}, join(join(f->stage[2], dirs_poke(*f, round % 2)), flags_poke(*f, 3 + round % 5)), name + " link round " + std::to_string(round));
            add(extrema_entry, {{3, obj_at}}, join(f->stage[2], flags_poke(*f, 1 + round % 3)), name + " extrema flags round " + std::to_string(round));
        }
    }
    // The size scaling with poked font data: em, sizes, flavor, heights (a second height close to the first, values at the halfword extremes), stems.
    {
        const std::array<std::int32_t, 12> heights = {0, 1, -1, 2, 100, -100, 0x7fff, -0x8000, 500, 700, 1000, 1500};
        const std::array<std::uint32_t, 10> sizes = {0, 1, 9, 17, 40, 72, 255, 0x1000, 0x7fffff, 0xfffffffbu};
        for (const auto* f : base) {
            for (unsigned round = 0; round < (exhaustive ? 30u : 12u); ++round) {
                std::vector<Write> memory = f->stage[0];
                const auto em = ems[random() % ems.size()];
                if (round % 4 != 3) memory.push_back(w16(obj_at + f_em, std::uint32_t(em)));
                memory.push_back(w32(obj_at + f_size_x, sizes[random() % sizes.size()]));
                memory.push_back(w32(obj_at + f_size_y, round % 3 ? sizes[random() % sizes.size()] : 17));
                memory.push_back(w32(obj_at + f_flavor, round % 5 == 4 ? 2u : round % 2));
                memory.push_back(w32(obj_at + f_keep_flat, round % 2));
                for (unsigned i = 0; i < 10; ++i) {
                    memory.push_back(w16(obj_at + f_records + 6 * i, std::uint32_t(heights[random() % heights.size()])));
                    memory.push_back(w16(obj_at + f_records + 6 * i + 2, std::uint32_t(heights[random() % heights.size()])));
                    memory.push_back(w16(obj_at + f_records + 6 * i + 4, std::uint32_t(heights[random() % heights.size()])));
                }
                if (round % 3 == 0) memory.push_back(w16(obj_at + f_records + 12, std::uint32_t(heights[0] + int(random() % 3))));  // record 2 near record 1
                for (unsigned i = 0; i < 12; ++i) {
                    memory.push_back(w16(obj_at + f_stems_x + 2 * i, std::uint32_t(heights[random() % heights.size()] / (round % 2 ? 1 : 50))));
                    memory.push_back(w16(obj_at + f_stems_y + 2 * i, std::uint32_t(heights[random() % heights.size()] / (round % 2 ? 1 : 50))));
                }
                add(scale_heights, {{3, obj_at}, {4, round % 4 == 1 ? 0u : 1u}, {5, gray_at}}, memory, f->name + " poked scale round " + std::to_string(round));
            }
        }
    }
    // The nearest height at the on-curve neighbour's distance around a sixteenth of the em: the second value (the overshoot) is dropped when the neighbour is
    // farther than that along an axis-aligned direction, and the record is made so that the second value is the nearest.
    for (const auto* f : base) {
        const unsigned n = f->points - 4;
        if (n < 3) continue;
        for (const std::int32_t d : {1, 31, 60, 63, 64, 65, 100, 127, 128, 129, 130, 200, -129, -128, -100}) {
            const unsigned p = 1, next = half_of(*f, half_at(next_r, p)), previous = half_of(*f, half_at(prev_r, p));
            const auto x_p = std::int16_t(half_of(*f, half_at(xs_r, p)));
            for (const bool far_next : {true, false}) for (const std::int32_t axis : {0x4000, 0x3fc3, 0x3fc2, -0x3fc2, -0x3fc3, 0x3fc1}) {
                if (d != 200 && axis != 0x4000) continue;
                std::vector<Write> memory = f->stage[2];
                for (const auto q : {p, next, previous}) memory.push_back(w8(at_region(on_r) + q, 1));
                memory.push_back(w16(at_region(xs_r) + 2 * (far_next ? next : previous), std::uint32_t(x_p + d)));
                memory.push_back(w16(at_region(xs_r) + 2 * (far_next ? previous : next), std::uint32_t(x_p)));
                memory.push_back(w32(word_at(in_x_r, p), std::uint32_t(axis)));
                memory.push_back(w32(word_at(out_x_r, p), std::uint32_t(axis)));
                memory.push_back(w16(half_at(flags_r, p), 0));
                memory.push_back(w16(at_region(ys_r) + 2 * p, 700));
                memory.push_back(w16(obj_at + f_records + 6 * 4, 690));
                memory.push_back(w16(obj_at + f_records + 6 * 4 + 2, 700));
                for (unsigned i = 0; i < 10; ++i) if (i != 4) memory.push_back(w16(obj_at + f_records + 6 * i, 5000));
                memory.push_back(w16(obj_at + f_mode, 0));
                add(nearest, {{3, obj_at}, {4, p}}, memory, f->name + " poked nearest height sixteenth " + std::to_string(d) + (far_next ? " next" : " previous") + " axis " + hex(std::uint32_t(axis)));
            }
        }
    }
    // A negative em (the quotients truncate towards zero) over the real states: every routine that reads it.
    for (const auto* f : base) {
        for (const std::int32_t em : {-1, -7, -100, -2047, -2048}) {
            for (unsigned p = 0; p < std::min(f->points - 4, 4u); ++p)
                add(nearest, {{3, obj_at}, {4, p}}, join(f->stage[2], {w16(obj_at + f_em, std::uint32_t(em))}), f->name + " poked nearest height em " + std::to_string(em) + " point " + std::to_string(p));
            if (!all_walkers) continue;
            add(classify, {{3, obj_at}}, join(f->stage[1], {w16(obj_at + f_em, std::uint32_t(em))}), f->name + " poked classify em " + std::to_string(em));
            add(inflections, {{3, obj_at}}, join(f->stage[2], {w16(obj_at + f_em, std::uint32_t(em))}), f->name + " poked inflections em " + std::to_string(em));
        }
    }
    // The scaling at its boundaries: with the grid values left zero (scale 0) a band limit's difference is the unrounded height, so records of +-0x5f, 0x60 and
    // 0x61 put each of the four assertions at its threshold; and the second height joins the first (flavor 1) when their distance is under 0x21 scaled.
    if (!base.empty()) {
        const auto* f = base.front();
        const auto records = [&](std::array<std::int32_t, 10> heights) {
            std::vector<Write> memory = f->stage[0];
            memory.push_back(w16(obj_at + f_em, 64));
            memory.push_back(w32(obj_at + f_size_x, 1));
            memory.push_back(w32(obj_at + f_size_y, 1));
            for (unsigned i = 0; i < 10; ++i) memory.push_back(w16(obj_at + f_records + 6 * i, std::uint32_t(heights[i])));
            return memory;
        };
        for (const unsigned record : {1u, 3u, 5u, 7u})
            for (const std::int32_t h : {0x5f, 0x60, 0x61, -0x5f, -0x60, -0x61}) {
                std::array<std::int32_t, 10> heights = {900, 900, 800, 700, 600, 500, 400, 300, 200, 100};
                heights[record] = h;
                for (const auto flavor : {0u, 1u}) {
                    auto memory = records(heights);
                    memory.push_back(w32(obj_at + f_flavor, flavor));
                    add(scale_heights, {{3, obj_at}, {4, 0}, {5, gray_at}}, memory, f->name + " poked scale boundary record " + std::to_string(record) + " height " + hex(std::uint32_t(h)) + " flavor " + std::to_string(flavor));
                }
            }
        for (const std::int32_t d : {30, 31, 32, 33, 34, 35, -32, -33}) {
            std::vector<Write> memory = f->stage[0];
            memory.push_back(w16(obj_at + f_em, 1024));
            memory.push_back(w32(obj_at + f_size_x, 16));
            memory.push_back(w32(obj_at + f_size_y, 16));
            memory.push_back(w32(obj_at + f_flavor, 1));
            memory.push_back(w16(obj_at + f_records + 6, 1000));
            memory.push_back(w16(obj_at + f_records + 12, std::uint32_t(1000 + d)));
            for (const auto scale : {1u, 0u}) add(scale_heights, {{3, obj_at}, {4, scale}, {5, gray_at}}, memory, f->name + " poked scale second height " + std::to_string(d) + " scale " + std::to_string(scale));
        }
    }
    // The band values with poked limits and counts.
    for (const auto* f : base) {
        for (unsigned round = 0; round < 6; ++round) {
            std::vector<Write> memory = f->stage[0];
            for (unsigned k = 0; k < 4; ++k) memory.push_back(w16(obj_at + f_limits + 2 * k, std::uint32_t(std::int32_t(random() % 2001) - 900)));
            if (round == 5) { memory.push_back(w16(obj_at + f_limits, 100)); memory.push_back(w16(obj_at + f_limits + 2, 100)); memory.push_back(w16(obj_at + f_limits + 4, 100)); memory.push_back(w16(obj_at + f_limits + 6, 100)); }
            const std::uint32_t count = round == 0 ? 0u : round == 1 ? 0xffffu : f->points - 4;
            memory.push_back(w16(input_at + 2, count));
            memory.push_back(w32(input_at + 0x1c, output_at));
            add(band_entry, {{3, obj_at}, {4, input_at}}, memory, f->name + " poked bands round " + std::to_string(round));
        }
    }
    // The nearest height with poked directions, flags, neighbours' y, on-curve bytes and heights (a record at the point's own y: the early exit).
    for (const auto* f : base) {
        const unsigned n = f->points - 4;
        for (unsigned round = 0; round < (exhaustive ? 60u : 24u); ++round) {
            const unsigned p = random() % n;
            std::vector<Write> memory = f->stage[2];
            const auto edge = [&] { static const std::array<std::int32_t, 14> v = {0x3f06, 0x3f07, 0x3f08, -0x3f06, -0x3f07, -0x3f08, 0, 0x4000, -0x4000, 0x3fc2, 0x3fc3, -0x3fc2, -0x3fc3, 0x7fffffff}; return std::uint32_t(v[random() % v.size()]); };
            memory.push_back(w32(word_at(in_x_r, p), edge()));
            memory.push_back(w32(word_at(out_x_r, p), edge()));
            memory.push_back(w16(half_at(flags_r, p), random() % 2 ? 8u : 0u));
            const unsigned next = half_of(*f, half_at(next_r, p)), previous = half_of(*f, half_at(prev_r, p));
            memory.push_back(w8(at_region(on_r) + p, random() % 3 != 0));
            memory.push_back(w8(at_region(on_r) + next, random() % 3 != 0));
            memory.push_back(w8(at_region(on_r) + previous, random() % 3 != 0));
            const auto y_near = [&] { return std::uint32_t(std::int32_t(random() % 1500) - 200); };
            memory.push_back(w16(at_region(ys_r) + 2 * p, y_near()));
            memory.push_back(w16(at_region(ys_r) + 2 * next, y_near()));
            memory.push_back(w16(at_region(ys_r) + 2 * previous, y_near()));
            if (round % 3 == 0) memory.push_back(w16(at_region(xs_r) + 2 * next, std::uint32_t(random() % 400)));
            if (round % 3 == 1) { memory.push_back(w16(obj_at + f_records + 6 * (random() % 10), std::uint32_t(random() % 1500))); memory.push_back(w16(obj_at + f_records + 6 * (random() % 10) + 2, std::uint32_t(random() % 1500))); }
            memory.push_back(w16(obj_at + f_mode, random() % 2));
            if (round % 2 == 1) memory.push_back(w16(obj_at + f_em, std::uint32_t(ems[random() % ems.size()])));
            add(nearest, {{3, obj_at}, {4, p}}, memory, f->name + " poked nearest height point " + std::to_string(p) + " round " + std::to_string(round));
        }
    }
    std::printf("autogrid_a: %zu real and synthetic fixtures, %zu cases\n", fixtures.size(), cases.size() - first_case);
    // The cases that reach the rare sites go first (the call trace pairs the first 32 of an entry, and the first 32 seed its mutated runs): the poked cases
    // of the routines whose mutated runs cannot loop; the walkers (the extrema, the inflections, the classification, the links: a mutated next table keeps them
    // going until the alarm) start with the glyphs' own states.
    const auto priority = [](const Case& item) {
        const bool walker = item.entry == extrema_entry || item.entry == inflections || item.entry == classify || item.entry == link_entry;
        if (item.label.find("boundary") != std::string::npos || item.label.find("second height") != std::string::npos || item.label.find("sixteenth") != std::string::npos) return 0;
        return item.label.find("poked") != std::string::npos && !walker ? 1 : 2;
    };
    std::stable_sort(cases.begin() + std::ptrdiff_t(first_case), cases.end(), [&](const Case& a, const Case& b) { return priority(a) < priority(b); });
}
}
