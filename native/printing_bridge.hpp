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
    std::int32_t default_print_settings(std::uint32_t session, std::uint32_t settings);
    // Test channel: with CM_AOT_PRINT_PDF set, print_dialog shows no panel,
    // accepts and sends the document to that PDF file.
    std::int32_t print_dialog(std::uint32_t session, std::uint32_t settings, std::uint32_t format, bool& accepted);
    std::int32_t page_setup_dialog(std::uint32_t session, std::uint32_t format, bool& accepted);
    std::int32_t begin_document(std::uint32_t session, std::uint32_t settings, std::uint32_t format);
    std::int32_t end_document(std::uint32_t session);
    std::int32_t begin_page(std::uint32_t session, std::uint32_t format);
    std::int32_t end_page(std::uint32_t session);

    // QuickDraw's text calls. Only SIPRINT makes them, on the printing port
    // PMSessionBeginDocument made current, so their state lives here.
    struct FontInfo { std::int16_t ascent, descent, width_max, leading; };
    std::int16_t font_family(std::span<const std::uint8_t> name);
    void set_font(std::int16_t family);
    void set_font_size(std::int16_t size);
    void move_to(std::int16_t x, std::int16_t y);
    std::int16_t text_width(std::span<const std::uint8_t> latin1) const;
    FontInfo font_info() const;
    void draw_text(std::span<const std::uint8_t> latin1);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
