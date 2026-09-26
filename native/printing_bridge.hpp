#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace cm::aot {
class NativePrinting {
public:
    NativePrinting();
    ~NativePrinting();
    enum class Kind { session, page_format, settings };
    std::int32_t create(Kind kind, std::uint32_t& token);
    std::int32_t release(std::uint32_t token);
    std::int32_t default_page_format(std::uint32_t session, std::uint32_t format);
    std::int32_t validate_page_format(std::uint32_t session, std::uint32_t format, bool& changed);
    std::int32_t flatten(std::uint32_t format, std::vector<std::uint8_t>& bytes);
    std::int32_t unflatten(std::span<const std::uint8_t> bytes, std::uint32_t& token);
    std::int32_t page_rect(std::uint32_t format, std::array<double, 4>& rect);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
