// The font engine, block `stream` (0x10762f40..0x107630e0): cases for the three readers of the input stream. A stream record
// ({+0 base, +4 position, +8 length}) and its data are built in the scratch area, with every position at the edges of the check
// (position + size equal to the length, one over, positions past the length, the wrap of the sum), bytes of every value in
// every lane (so a swapped lane shows), the unreported assertion (the reporter 0x107755c0 answered, so the original goes on) and
// the real reporter (whose dialog the checker answers). Labels start with "stream:" (CM_FONT_ENGINE_BLOCK=stream).
#include "font_engine_checks.hpp"
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t read_32 = 0x10762f40, read_16 = 0x10762fd0, read_bytes = 0x10763050, reporter = 0x107755c0;
constexpr std::uint32_t stream_at = scratch_base + 0x30000, data_at = scratch_base + 0x30100, target_at = scratch_base + 0x31000;
std::vector<Write> stream_of(std::uint32_t base, std::uint32_t position, std::uint32_t length, const std::vector<std::uint8_t>& bytes) {
    std::vector<Write> memory{w32(stream_at, base), w32(stream_at + 4, position), w32(stream_at + 8, length)};
    for (std::size_t n = 0; n < bytes.size(); ++n) memory.push_back(w8(data_at + std::uint32_t(n), bytes[n]));
    return memory;
}
}

FONT_ENGINE_CASES("stream") {
    std::mt19937 random(0x57ea);
    std::vector<std::uint8_t> bytes(64);
    for (auto& byte : bytes) byte = std::uint8_t(random());
    // The assertion first (the cases before the 32nd of an entry are traced): the real reporter, then answered.
    for (const auto entry : {read_32, read_16, read_bytes}) {
        const std::uint32_t size = entry == read_32 ? 4 : entry == read_16 ? 2 : 5;
        for (const bool answered : {false, true}) {
            Case item{.entry = entry, .registers = {{3, stream_at}, {4, target_at}, {5, size}}, .memory = stream_of(data_at, 8, 8 + size - 1, bytes),
                      .label = std::string("stream: overrun by one at ") + hex(entry) + (answered ? " answered" : " real reporter")};
            if (answered) item.answers = {{reporter, 0}};
            cases.push_back(item);
        }
    }
    for (const auto entry : {read_32, read_16}) {
        const std::uint32_t size = entry == read_32 ? 4 : 2;
        for (const std::uint32_t position : {0u, 1u, 2u, 3u, 4u, 5u, 31u, 56u, 59u, 60u, 61u, 62u, 63u, 0xfffffffcu, 0xfffffffeu, 0x7fffffffu, 0x80000000u})
            for (const std::uint32_t length : {0u, 1u, 3u, 4u, 5u, 63u, 64u, 65u, 0xffffffffu}) {
                Case item{.entry = entry, .registers = {{3, stream_at}}, .memory = stream_of(position >= 0xfffffff0u ? data_at + 8 : data_at, position, length, bytes),
                          .label = std::string("stream: ") + hex(entry) + " position " + hex(position) + " length " + hex(length)};
                item.answers = {{reporter, 0}};
                cases.push_back(item);
            }
        // Bytes of every value in every lane.
        for (unsigned lane = 0; lane < size; ++lane)
            for (const unsigned value : {0u, 1u, 0x7fu, 0x80u, 0xffu}) {
                auto lanes = bytes;
                lanes[16 + lane] = std::uint8_t(value);
                cases.push_back({.entry = entry, .registers = {{3, stream_at}}, .memory = stream_of(data_at, 16, 64, lanes),
                                 .label = "stream: " + hex(entry) + " lane " + std::to_string(lane) + " value " + hex(value)});
            }
        for (unsigned n = 0; n < 100; ++n) {
            auto random_bytes = bytes;
            for (auto& byte : random_bytes) byte = std::uint8_t(random());
            cases.push_back({.entry = entry, .registers = {{3, stream_at}}, .memory = stream_of(data_at, random() % 60, 64, random_bytes),
                             .label = "stream: " + hex(entry) + " random " + std::to_string(n)});
        }
    }
    // read_bytes: counts around zero, the length and the destination; the move itself is the game's.
    for (const std::int32_t count : {-1, 0, 1, 2, 7, 16, 33, 64}) {
        for (const std::uint32_t position : {0u, 1u, 30u, 63u, 64u})
            for (const std::uint32_t length : {0u, 31u, 64u, 100u}) {
                if (position + std::uint32_t(count > 0 ? count : 0) > 70) continue;
                Case item{.entry = read_bytes, .registers = {{3, stream_at}, {4, target_at}, {5, std::uint32_t(count)}},
                          .memory = stream_of(data_at, position, length, bytes), .label = "stream: copy " + std::to_string(count) + " at " + hex(position) + " of " + hex(length)};
                item.answers = {{reporter, 0}};
                cases.push_back(item);
            }
    }
}
}
