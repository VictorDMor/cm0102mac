#include "dialog_resource.hpp"
#include <stdexcept>

namespace cm::aot {
namespace {
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t position = 0;
    std::uint8_t byte() {
        if (position >= bytes.size()) throw std::runtime_error("Truncated dialog resource");
        return bytes[position++];
    }
    std::uint16_t word() { const auto high = byte(); return std::uint16_t(high << 8 | byte()); }
    LegacyRect rect() {
        LegacyRect rect{};
        for (auto& side : rect) side = std::int16_t(word());
        return rect;
    }
    std::string text(std::size_t size) {
        std::string result;
        for (std::size_t i = 0; i < size; ++i) result += char(byte());
        return result;
    }
};
}
DialogDefinition parse_dialog(std::span<const std::uint8_t> bytes) {
    // rect, procID, visible, filler, goAwayFlag, filler, refCon, itemsID, title.
    Reader reader{bytes};
    DialogDefinition dialog;
    dialog.bounds = reader.rect();
    reader.word();
    reader.word();
    reader.word();
    reader.word();
    reader.word();
    dialog.items = std::int16_t(reader.word());
    dialog.title = reader.text(reader.byte());
    if (dialog.bounds[2] <= dialog.bounds[0] || dialog.bounds[3] <= dialog.bounds[1])
        throw std::runtime_error("Invalid dialog bounds");
    return dialog;
}
std::vector<DialogItem> parse_dialog_items(std::span<const std::uint8_t> bytes) {
    // count - 1, then per item: a placeholder long, rect, type, data length,
    // data padded to an even length.
    Reader reader{bytes};
    std::vector<DialogItem> items(std::size_t(reader.word()) + 1);
    for (auto& item : items) {
        reader.word();
        reader.word();
        item.bounds = reader.rect();
        item.type = reader.byte();
        const auto size = reader.byte();
        const auto data = reader.text(size);
        if ((size & 1) && reader.position < bytes.size()) reader.byte();  // the last pad may be missing
        if (item.kind() == DialogItem::button || item.kind() == DialogItem::check_box ||
            item.kind() == DialogItem::radio_button || item.kind() == DialogItem::static_text ||
            item.kind() == DialogItem::edit_text)
            item.text = data;
    }
    return items;
}
}
