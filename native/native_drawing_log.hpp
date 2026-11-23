#pragma once
// What the host's drawing view (native_drawing.cpp, NativeDrawingObserver::boundary) reads at each observed instruction, for the
// checkers' boundary logs (graph_carbon_checks.cpp, font_cpp_checks.cpp). The checker's probe (native_drawing_boundary_probe) appends
// these words to the log it compares between the original and the rewrite, so a C++ routine that passes an observation point with a
// register or a memory word different from the original's is a difference. Derived from the observer's source, one case per address:
// change one only together with the other. Reads go through an unobserved path (the read trace of the case runner stays as it was) and
// a stray address logs a marker instead of faulting. The observer's own bookkeeping (its stacks, the registered surfaces) is not guest
// state and is not logged: the pairing of entries and exits is the order of the events.
#include "aot_runtime.hpp"
#include "guest_abi.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace cm::aot::drawing_log {

class Reader {
public:
    explicit Reader(State& s) : s_(s), trace_(std::exchange(s.memory.read_trace, nullptr)) {}
    ~Reader() { s_.memory.read_trace = trace_; }
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
    std::uint32_t r(unsigned n) const { return std::uint32_t(s_.r<4>(guest::reg::gpr(n))); }
    // The observer's `reg(n)`: the register as a signed word.
    std::int32_t sr(unsigned n) const { return std::int32_t(r(n)); }
    template <unsigned N> std::uint32_t at(std::uint32_t address) const {
        try {
            return std::uint32_t(s_.memory.read<N>(address));
        } catch (...) {
            return 0xdead0000u | N;  // a stray address: the same marker on both sides
        }
    }
    // A hash of `size` bytes (bounded), as the observer's copy_out or loops read them.
    std::uint32_t bytes(std::uint32_t address, std::uint64_t size, std::uint64_t limit = 1u << 22) const {
        std::uint32_t hash = 2166136261u;
        for (std::uint64_t i = 0; i < size && i < limit; ++i) hash = (hash ^ at<1>(std::uint32_t(address + i))) * 16777619u;
        return hash ^ std::uint32_t(size);
    }
private:
    State& s_;
    std::vector<std::pair<std::uint32_t, unsigned>>* trace_;
};

// `active(s)`: the TOC, the flag byte at toc - 0x3550 and the surface address at toc - 0x3564.
inline void active(const Reader& g, std::vector<std::uint32_t>& w) {
    const auto toc = g.r(2);
    w.push_back(toc);
    if (toc != 0x108bba90) return;
    const auto flag = g.at<1>(toc - 0x3550);
    w.push_back(flag);
    if (!flag) w.push_back(g.at<4>(toc - 0x3564));
}

// The saved drawing the observer restores (`saved_scene(token)`): the record's width, height and pixel pointer and the pixels.
inline void saved_scene(const Reader& g, std::uint32_t token, std::vector<std::uint32_t>& w) {
    const auto width = g.at<4>(token), height = g.at<4>(token + 4), pixels = g.at<4>(token + 12);
    w.insert(w.end(), {token, width, height, pixels});
    if (width && height && width <= 4096 && height <= 4096) w.push_back(g.bytes(pixels, std::uint64_t(width) * height * 2));
}

// The words the observer reads at `pc` (the registers and memory it consults, in its order), after the pc itself.
inline std::vector<std::uint32_t> observed_words(State& s, std::uint32_t pc) {
    const Reader g(s);
    std::vector<std::uint32_t> w;
    switch (pc) {
    case 0x107509f0: {  // glyph cache exit: the cache record (r3) and its pixel pointer
        const auto record = g.r(3);
        w.insert(w.end(), {record, g.at<4>(record + 20)});
        break;
    }
    case 0x107620d0: {  // the outline: contour count (r4), arrays r5..r9 and what the observer walks of them
        const auto contours = std::int16_t(g.r(4));
        w.insert(w.end(), {g.r(4), g.r(5), g.r(6), g.r(7), g.r(8), g.r(9)});
        if (contours <= 0 || contours > 256) break;
        for (int c = 0; c < contours; ++c) {
            const auto first = g.at<2>(g.r(5) + c * 2), last = g.at<2>(g.r(6) + c * 2);
            w.insert(w.end(), {first, last});
            if (first > last || last >= 8192) break;
            for (auto i = first; i <= last; ++i)
                w.insert(w.end(), {g.at<4>(g.r(7) + i * 4), g.at<4>(g.r(8) + i * 4), g.at<1>(g.r(9) + i)});
        }
        break;
    }
    case 0x1074f800:  // the string drawer: r7 (flags), r8 (font size), r9 (colour)
        w.insert(w.end(), {g.r(7), g.r(8), g.r(9)});
        break;
    case 0x10769460:  // the fade: r7 (direction)
        w.push_back(g.r(7));
        break;
    case 0x107696cc: {  // a fade frame: the saved capture (r26), the step (r28 of r30), the origin at sp + 0x44
        active(g, w);
        const auto sp = g.r(1);
        w.insert(w.end(), {g.r(26), g.r(28), g.r(30), sp, g.at<2>(sp + 0x46), g.at<2>(sp + 0x44)});
        saved_scene(g, g.r(26), w);
        break;
    }
    case 0x10769980: case 0x107697c0: case 0x1076b020: case 0x1076a140: case 0x1076a580: case 0x1076ad50:
        // dim, grey, capture, line, fill, ellipse: the rectangle r3..r6 (and style r7, colour r8 of the shapes)
        active(g, w);
        w.insert(w.end(), {g.r(3), g.r(4), g.r(5), g.r(6), g.r(7), g.r(8)});
        break;
    case 0x1076b138: {  // the capture's exit: the saved bitmap token (r3) and its pixels
        const auto token = g.r(3);
        w.push_back(token);
        if (!token) break;
        const auto width = g.at<4>(token), height = g.at<4>(token + 4), pixels = g.at<4>(token + 12);
        w.insert(w.end(), {width, height, pixels});
        if (pixels && width && height && width <= 4096 && height <= 4096) w.push_back(g.bytes(pixels, std::uint64_t(width) * height * 2));
        break;
    }
    case 0x1076b7e0:  // freeing a bitmap: its token (r3)
        w.push_back(g.r(3));
        break;
    case 0x1076b140:  // the blit: origin r3/r4 and the saved bitmap r5
        active(g, w);
        w.insert(w.end(), {g.r(3), g.r(4)});
        saved_scene(g, g.r(5), w);
        break;
    case 0x1076ca50: {  // the bitmap-font text drawer: r5 first (the font stack), then the glyph walk
        w.push_back(g.r(5));
        active(g, w);
        const std::uint32_t toc = g.r(2);
        const int font = std::int16_t(g.r(5));
        w.insert(w.end(), {g.r(3), g.r(4), g.r(6), g.r(7)});
        if (font < 1 || font > 7 || !g.r(7)) break;
        const auto flag = g.at<4>(g.at<4>(toc - 0x740c));
        w.push_back(flag);
        if (std::int32_t(flag) != -1) break;
        const auto table = g.at<4>(toc - 0x3bdc) + (font - 1) * 0x1404;
        w.insert(w.end(), {table, g.at<4>(table), g.bytes(table, 0x1404)});
        const auto text = g.r(7);
        for (unsigned i = 0; i < 4096; ++i) {
            auto c = g.at<1>(text + i);
            w.push_back(c);
            if (!c) break;
            if (c == '|') c = ' ';
            if (c < 32) continue;
            const auto record = table + c * 20;
            const auto width = g.at<4>(record + 4), pixels = g.at<4>(record + 20), height = g.at<4>(table);
            if (width > 128) break;
            if (pixels && width && height && height <= 128) w.push_back(g.bytes(pixels, std::uint64_t((width + 1) / 2) * height));
        }
        break;
    }
    case 0x1074fb58: {  // a bitmap glyph of the outline drawer: the glyph record in r30, the string character in r31, the pen in r0/r4
        active(g, w);
        const auto record = g.r(30);
        const auto width = g.at<4>(record + 8), height = g.at<4>(record + 16), pitch = g.at<4>(record + 12), pixels = g.at<4>(record + 20);
        w.insert(w.end(), {record, width, height, pitch, pixels});
        if (!pixels || pitch < width || pitch > 512) break;
        if (width && width <= 128 && height <= 128) w.push_back(g.bytes(pixels, std::uint64_t(pitch) * height));
        const std::uint32_t sp = g.r(1), text = g.at<4>(sp + 0xd4), index = std::uint16_t(g.r(31));
        const auto character = g.at<1>(text + std::uint32_t(std::int16_t(g.r(31))));
        w.insert(w.end(), {sp, text, g.r(31), character});
        if (character <= 32 || character == '|') break;
        w.insert(w.end(), {g.r(0), g.r(4), g.r(5), g.r(7), g.r(8), g.r(10), g.at<4>(record), g.at<4>(record + 4)});
        // the euro sign test: the formatter's currency symbol and the text around the character
        const auto symbol = g.at<4>(g.r(2) - 0x59d0);
        w.insert(w.end(), {symbol, g.at<1>(symbol), g.at<1>(symbol + 1), g.bytes(text + index - 1, 12)});
        break;
    }
    default:
        break;  // exits and entries the observer only pairs: nothing guest-side
    }
    return w;
}

}  // namespace cm::aot::drawing_log
