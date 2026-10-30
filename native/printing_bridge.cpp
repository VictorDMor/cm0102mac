#include "printing_bridge.hpp"
#include "print_panels.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <CoreText/CoreText.h>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace cm::aot {
struct NativePrinting::Impl {
    struct Object { void* value; Kind kind; };
    std::map<std::uint32_t, Object> objects;
    std::uint32_t next_token = 0x50000000;
    std::vector<std::string> families;
    std::string font_name = "Courier";
    std::int16_t font_size = 0, pen_x = 0, pen_y = 0;
    CGContextRef page = nullptr;
    int page_index = -1, previous_pages_height = 0;
    ~Impl() { for (const auto& [token, object] : objects) { (void)token; ::PMRelease(object.value); } }
    // Takes over a reference the caller retained.
    void replace(std::uint32_t token, void* value) {
        auto& object = objects.at(token);
        ::PMRelease(object.value);
        object.value = value;
    }
    // TextSize(0) is the default size; CoreText reads a zero size the same way.
    CTFontRef font() const {
        const auto name = ::CFStringCreateWithCString(nullptr, font_name.c_str(), kCFStringEncodingMacRoman);
        const auto font = ::CTFontCreateWithName(name, font_size, nullptr);
        ::CFRelease(name);
        return font;
    }
    // The game's text is Latin-1, from its Windows-era data and strings.
    CTLineRef line(std::span<const std::uint8_t> latin1) const {
        auto text = ::CFStringCreateWithBytes(nullptr, latin1.data(), CFIndex(latin1.size()),
                                              kCFStringEncodingWindowsLatin1, false);
        // Windows Latin-1 leaves five bytes undefined; ISO Latin-1 takes every byte.
        if (!text) text = ::CFStringCreateWithBytes(nullptr, latin1.data(), CFIndex(latin1.size()),
                                                    kCFStringEncodingISOLatin1, false);
        const auto native_font = font();
        const void* keys[] = {kCTFontAttributeName, kCTForegroundColorFromContextAttributeName};
        const void* values[] = {native_font, kCFBooleanTrue};
        const auto attributes = ::CFDictionaryCreate(nullptr, keys, values, 2, &kCFTypeDictionaryKeyCallBacks,
                                                     &kCFTypeDictionaryValueCallBacks);
        const auto styled = ::CFAttributedStringCreate(nullptr, text, attributes);
        const auto result = ::CTLineCreateWithAttributedString(styled);
        ::CFRelease(styled); ::CFRelease(attributes); ::CFRelease(native_font); ::CFRelease(text);
        return result;
    }
    void* get(std::uint32_t token, Kind kind) const {
        const auto found = objects.find(token);
        return found != objects.end() && found->second.kind == kind ? found->second.value : nullptr;
    }
    std::uint32_t insert(void* object, Kind kind) {
        const auto token = next_token++;
        objects.emplace(token, Object{object, kind});
        return token;
    }
};
NativePrinting::NativePrinting() : impl_(std::make_unique<Impl>()) {}
NativePrinting::~NativePrinting() = default;
std::int32_t NativePrinting::create(Kind kind, std::uint32_t& token) {
    token = 0;
    void* value = nullptr;
    OSStatus error;
    if (kind == Kind::session) {
        PMPrintSession session = nullptr;
        error = ::PMCreateSession(&session); value = session;
    } else if (kind == Kind::page_format) {
        PMPageFormat format = nullptr;
        error = ::PMCreatePageFormat(&format); value = format;
    } else {
        PMPrintSettings settings = nullptr;
        error = ::PMCreatePrintSettings(&settings); value = settings;
    }
    if (!error) token = impl_->insert(value, kind);
    return error;
}
std::int32_t NativePrinting::release(std::uint32_t token) {
    const auto found = impl_->objects.find(token);
    if (found == impl_->objects.end()) return kPMInvalidObject;
    const auto error = ::PMRelease(found->second.value);
    if (!error) impl_->objects.erase(found);
    return error;
}
std::int32_t NativePrinting::default_page_format(std::uint32_t session, std::uint32_t format) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_session || !native_format) return kPMInvalidObject;
    return ::PMSessionDefaultPageFormat(native_session, native_format);
}
std::int32_t NativePrinting::validate_page_format(std::uint32_t session, std::uint32_t format, bool& changed) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_session || !native_format) return kPMInvalidObject;
    Boolean native_changed = false;
    const auto error = ::PMSessionValidatePageFormat(native_session, native_format, &native_changed);
    changed = native_changed;
    return error;
}
std::int32_t NativePrinting::flatten(std::uint32_t format, std::vector<std::uint8_t>& bytes) {
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_format) return kPMInvalidObject;
    CFDataRef data = nullptr;
    const auto error = ::PMPageFormatCreateDataRepresentation(native_format, &data, kPMDataFormatXMLDefault);
    if (!error && data) {
        // The original program treats the flattened format as opaque bytes.
        // Identify this port's native representation for subsequent restoration.
        bytes = {'C', 'M', 'P', 'F', 0, 0, 0, 1};
        bytes.insert(bytes.end(), ::CFDataGetBytePtr(data), ::CFDataGetBytePtr(data) + ::CFDataGetLength(data));
    }
    if (data) ::CFRelease(data);
    return error;
}
std::int32_t NativePrinting::unflatten(std::span<const std::uint8_t> bytes, std::uint32_t& token) {
    token = 0;
    constexpr std::array<std::uint8_t, 8> prefix{'C', 'M', 'P', 'F', 0, 0, 0, 1};
    if (bytes.size() < prefix.size() || !std::equal(prefix.begin(), prefix.end(), bytes.begin()))
        return kPMInvalidParameter; // Old flattened formats require a separate conversion.
    CFDataRef data = ::CFDataCreate(nullptr, bytes.data() + prefix.size(), bytes.size() - prefix.size());
    if (!data) return memFullErr;
    PMPageFormat format = nullptr;
    const auto error = ::PMPageFormatCreateWithDataRepresentation(data, &format);
    ::CFRelease(data);
    if (!error) token = impl_->insert(format, Kind::page_format);
    return error;
}
std::int32_t NativePrinting::page_rect(std::uint32_t format, std::array<double, 4>& rect) {
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_format) return kPMInvalidObject;
    PMRect bounds{};
    const auto error = ::PMGetAdjustedPageRect(native_format, &bounds);
    if (!error) rect = {bounds.top, bounds.left, bounds.bottom, bounds.right};
    return error;
}
std::int32_t NativePrinting::default_print_settings(std::uint32_t session, std::uint32_t settings) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_settings = static_cast<PMPrintSettings>(impl_->get(settings, Kind::settings));
    if (!native_session || !native_settings) return kPMInvalidObject;
    return ::PMSessionDefaultPrintSettings(native_session, native_settings);
}
std::int32_t NativePrinting::print_dialog(std::uint32_t session, std::uint32_t settings, std::uint32_t format, bool& accepted) {
    accepted = false;
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_settings = static_cast<PMPrintSettings>(impl_->get(settings, Kind::settings));
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_session || !native_settings || !native_format) return kPMInvalidObject;
    if (const char* pdf = std::getenv("CM_AOT_PRINT_PDF"); pdf && *pdf) {
        const auto url = ::CFURLCreateFromFileSystemRepresentation(
            nullptr, reinterpret_cast<const UInt8*>(pdf), CFIndex(std::char_traits<char>::length(pdf)), false);
        const auto error = ::PMSessionSetDestination(native_session, native_settings, kPMDestinationFile,
                                                     kPMDocumentFormatPDF, url);
        ::CFRelease(url);
        accepted = !error;
        return error;
    }
    const auto chosen = run_print_panel(native_settings, native_format);
    if (!chosen) return noErr;
    impl_->replace(session, chosen->session);
    impl_->replace(settings, chosen->settings);
    impl_->replace(format, chosen->format);
    accepted = true;
    return noErr;
}
std::int32_t NativePrinting::page_setup_dialog(std::uint32_t session, std::uint32_t format, bool& accepted) {
    accepted = false;
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!impl_->get(session, Kind::session) || !native_format) return kPMInvalidObject;
    const auto chosen = run_page_layout(native_format);
    if (!chosen) return noErr;
    ::PMRelease(chosen->settings);
    impl_->replace(session, chosen->session);
    impl_->replace(format, chosen->format);
    accepted = true;
    return noErr;
}
std::int32_t NativePrinting::begin_document(std::uint32_t session, std::uint32_t settings, std::uint32_t format) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_settings = static_cast<PMPrintSettings>(impl_->get(settings, Kind::settings));
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_session || !native_settings || !native_format) return kPMInvalidObject;
    impl_->font_name = "Courier";
    impl_->font_size = impl_->pen_x = impl_->pen_y = 0;
    impl_->page_index = -1;
    return ::PMSessionBeginCGDocumentNoDialog(native_session, native_settings, native_format);
}
std::int32_t NativePrinting::end_document(std::uint32_t session) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    if (!native_session) return kPMInvalidObject;
    return ::PMSessionEndDocumentNoDialog(native_session);
}
std::int32_t NativePrinting::begin_page(std::uint32_t session, std::uint32_t format) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_session || !native_format) return kPMInvalidObject;
    PMRect paper{}, printable{};
    if (const auto error = ::PMGetAdjustedPaperRect(native_format, &paper)) return error;
    if (const auto error = ::PMGetAdjustedPageRect(native_format, &printable)) return error;
    if (const auto error = ::PMSessionBeginPageNoDialog(native_session, native_format, nullptr)) return error;
    CGContextRef context = nullptr;
    if (const auto error = ::PMSessionGetCGGraphicsContext(native_session, &context)) {
        ::PMSessionEndPageNoDialog(native_session);
        return error;
    }
    // The port's coordinates: y down, origin at the printable area's top left.
    // The context starts at the paper's bottom left.
    ::CGContextSaveGState(context);
    ::CGContextTranslateCTM(context, -paper.left, paper.bottom);
    ::CGContextScaleCTM(context, 1, -1);
    ::CGContextSetTextMatrix(context, ::CGAffineTransformMakeScale(1, -1));
    impl_->page = context;
    ++impl_->page_index;
    // SIPRINT positions every line by its index in the whole document, so past
    // the first page its lines would fall off the sheet. Paper is not something
    // the game reads: draw_text brings them back with SIPRINT's own page length.
    const auto metrics = font_info();
    const int spacing = metrics.ascent + metrics.descent + metrics.leading;
    impl_->previous_pages_height = spacing > 0
        ? impl_->page_index * (int(printable.bottom - printable.top) / spacing) * spacing : 0;
    return noErr;
}
std::int32_t NativePrinting::end_page(std::uint32_t session) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    if (!native_session) return kPMInvalidObject;
    if (impl_->page) ::CGContextRestoreGState(impl_->page);
    impl_->page = nullptr;
    return ::PMSessionEndPageNoDialog(native_session);
}
std::int16_t NativePrinting::font_family(std::span<const std::uint8_t> name) {
    const std::string family(name.begin(), name.end());
    const auto found = std::find(impl_->families.begin(), impl_->families.end(), family);
    if (found != impl_->families.end()) return std::int16_t(found - impl_->families.begin() + 1);
    impl_->families.push_back(family);
    return std::int16_t(impl_->families.size());
}
void NativePrinting::set_font(std::int16_t family) {
    if (family >= 1 && std::size_t(family) <= impl_->families.size())
        impl_->font_name = impl_->families[std::size_t(family) - 1];
}
void NativePrinting::set_font_size(std::int16_t size) { impl_->font_size = size; }
void NativePrinting::move_to(std::int16_t x, std::int16_t y) { impl_->pen_x = x; impl_->pen_y = y; }
std::int16_t NativePrinting::text_width(std::span<const std::uint8_t> latin1) const {
    const auto line = impl_->line(latin1);
    const auto width = ::CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
    ::CFRelease(line);
    return std::int16_t(std::lround(width));
}
NativePrinting::FontInfo NativePrinting::font_info() const {
    const auto font = impl_->font();
    const FontInfo info{std::int16_t(std::ceil(::CTFontGetAscent(font))), std::int16_t(std::ceil(::CTFontGetDescent(font))),
                        std::int16_t(std::ceil(::CTFontGetBoundingBox(font).size.width)),
                        std::int16_t(std::lround(::CTFontGetLeading(font)))};
    ::CFRelease(font);
    return info;
}
void NativePrinting::draw_text(std::span<const std::uint8_t> latin1) {
    const auto line = impl_->line(latin1);
    if (impl_->page) {
        // The pen is QuickDraw's 16 bits; wrap the difference the same way.
        ::CGContextSetTextPosition(impl_->page, impl_->pen_x,
                                   std::int16_t(impl_->pen_y - impl_->previous_pages_height));
        ::CTLineDraw(line, impl_->page);
    }
    impl_->pen_x = std::int16_t(impl_->pen_x + std::lround(::CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr)));
    ::CFRelease(line);
}
}
