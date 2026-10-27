#include "contract_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_manager {
using namespace detail;
std::uint32_t describe_status(Cpu& c, std::uint32_t person, std::uint32_t buffer) {
    Frame frame(c, 0x1007f030, 0x40, {});
    const auto pool = c.toc(slot::pool); c.set(6, pool);
    const auto attributes = c.u32(person + 0x14); c.set(3, attributes);
    const auto status = c.i8(attributes + 0x45); c.set(0, std::uint32_t(std::int32_t(status)));
    constexpr std::array<int, 5> limits{4, 8, 12, 15, 18};
    constexpr std::array<unsigned, 6> texts{0x297, 0x2be, 0x2df, 0x2ff, 0x321, 0x348};
    constexpr std::array<std::uint32_t, 6> links{0x1007f064, 0x1007f080, 0x1007f09c, 0x1007f0b8, 0x1007f0d4, 0x1007f0e8};
    unsigned category = 0;
    while (category < limits.size() && compare(c, status, limits[category]) >= 0) ++category;
    return Program(c).call(0x10727890, links[category], {buffer, pool + texts[category], pool + 0x2bd});
}

std::uint32_t describe_status_difference(Cpu& c, std::uint32_t first, std::uint32_t second, std::uint32_t buffer) {
    Frame frame(c, 0x1007f100, 0x40, {.first_gpr = 31});
    const auto left = c.u32(first + 0x14), right = c.u32(second + 0x14);
    c.set(3, left); c.set(5, right);
    const auto first_status = c.u8(left + 0x45), second_status = c.u8(right + 0x45);
    c.set(0, first_status); c.set(4, second_status);
    const auto pool = c.toc(slot::pool); c.set(5, pool);
    const auto raw = std::uint32_t(first_status) - second_status; c.set(0, raw);
    Kept<31> difference(c, std::uint32_t(std::int32_t(std::int8_t(raw))));
    constexpr std::array<int, 6> limits{-13, -7, -3, 3, 7, 13};
    constexpr std::array<unsigned, 7> texts{0x36c, 0x392, 0x3b6, 0x3dc, 0x3b6, 0x392, 0x36c};
    constexpr std::array<std::uint32_t, 7> links{0x1007f144, 0x1007f160, 0x1007f17c, 0x1007f198,
                                               0x1007f1b4, 0x1007f1d0, 0x1007f1e4};
    unsigned category = 0;
    while (category < limits.size() && compare(c, std::int32_t(std::uint32_t(difference)), limits[category]) >= 0)
        ++category;
    Program(c).call(0x10727890, links[category], {buffer, pool + texts[category], pool + 0x2bd});
    return result(c, difference);
}
}
