#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace cm::aot {
std::int32_t native_region_code();
void native_uppercase(std::vector<std::uint8_t>& bytes, std::int16_t script);
struct MungerResult {
    std::int32_t offset;
    std::int16_t error;
    std::vector<std::uint8_t> bytes;
};
MungerResult native_munger(const std::vector<std::uint8_t>& bytes, std::int32_t offset,
    const std::optional<std::vector<std::uint8_t>>& needle, std::int32_t needle_length,
    const std::optional<std::vector<std::uint8_t>>& replacement, std::int32_t replacement_length);
}
