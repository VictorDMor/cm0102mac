#include "cup_stage.hpp"
#include "guest_values.hpp"

namespace cm::aot::cup_stage {
using namespace guest;

std::uint32_t initialize_entrants(Cpu& c, std::uint32_t stage, std::int32_t count,
                                 std::uint32_t clubs, std::int8_t numbered,
                                 std::uint32_t seeds) {
    c.set(8, seeds);
    if (compare(c, count) > 0) {
        for (std::uint32_t index = 0; index < std::uint32_t(count); ++index) {
            const auto club = c.u32(clubs);
            compare_logical(c, seeds);
            const auto offset = index * 8;
            auto entrants = c.u32(stage + 0xa4);
            c.put32(entrants + offset, club);
            entrants = c.u32(stage + 0xa4);
            c.put8(entrants + offset + 5, 0);
            std::uint32_t seed = 0;
            if (seeds) seed = c.u8(seeds + index);
            else if (compare(c, numbered) != 0) seed = index;
            entrants = c.u32(stage + 0xa4);
            c.put8(entrants + offset + 4, seed);
            clubs += 4;
            leave(c, {{4, entrants}, {5, clubs}, {8, seeds + index + 1},
                      {9, seeds ? seed : 0}, {10, club}});
        }
    }
    return result(c, 1);
}

std::uint32_t destroy(Cpu& c, std::uint32_t stage, std::int16_t deleting) {
    Frame frame(c, 0x104bc9b0, 0x50, {.first_gpr = 28});
    keep(c, {{28, stage}, {29, c.r(4)}});
    if (compare(c, stage) != 0) {
        c.put32(stage, c.toc(-0x613c));
        for (auto [offset, back] : {std::pair{0xa4u, 0x104bc9f0u},
                                   std::pair{0xa0u, 0x104bca00u}}) {
            const auto allocation = c.u32(stage + offset);
            c.set(3, allocation);
            if (compare_logical(c, allocation) != 0) c.call(0x10775170, back);
        }
        if (compare_logical(c, c.u32(stage + 0xac)) != 0) {
            std::uint32_t round = 0;
            keep(c, {{30, 0}, {31, 0}});
            while (compare(c, round, c.i16(stage + 0x3c)) < 0) {
                const auto allocation = c.u32(c.u32(stage + 0xac) + round * 4);
                c.set(3, allocation);
                if (compare_logical(c, allocation) != 0) c.call(0x10775170, 0x104bca2c);
                ++round;
                keep(c, {{30, round}, {31, round * 4}});
            }
            c.set(3, c.u32(stage + 0xac));
            c.call(0x10775170, 0x104bca48);
        }
        leave(c, {{3, stage}, {4, 0}});
        c.call(0x10478f70, 0x104bca54);
        if (compare(c, deleting) > 0) {
            c.set(3, stage);
            c.call(0x107735a0, 0x104bca64);
        }
    }
    return result(c, stage);
}
}
