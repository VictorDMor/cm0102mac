#include "printing_bridge.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

std::span<const std::uint8_t> bytes_of(std::string_view text) {
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}
struct Ink { int left, top, right, bottom, top_row_right, bottom_row_right; };
// Rasterizes one PDF page at 72 dpi, top-left origin: where its marks are.
Ink page_ink(CGPDFPageRef page) {
    const auto media = ::CGPDFPageGetBoxRect(page, kCGPDFMediaBox);
    const int width = int(media.size.width), height = int(media.size.height);
    std::vector<std::uint8_t> pixels(std::size_t(width * height), 255);
    const auto space = ::CGColorSpaceCreateDeviceGray();
    const auto bitmap = ::CGBitmapContextCreate(pixels.data(), std::size_t(width), std::size_t(height), 8,
                                                std::size_t(width), space, kCGImageAlphaNone);
    ::CGContextDrawPDFPage(bitmap, page);
    ::CGContextRelease(bitmap); ::CGColorSpaceRelease(space);
    Ink ink{width, height, -1, -1, -1, -1};
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        if (pixels[std::size_t(y * width + x)] > 128) continue;
        ink.left = std::min(ink.left, x); ink.right = std::max(ink.right, x);
        ink.top = std::min(ink.top, y); ink.bottom = std::max(ink.bottom, y);
    }
    for (int x = 0; x < width; ++x) {
        if (ink.right < 0) break;  // A blank page.
        if (pixels[std::size_t(ink.top * width + x)] <= 128) ink.top_row_right = x;
        if (pixels[std::size_t(ink.bottom * width + x)] <= 128) ink.bottom_row_right = x;
    }
    return ink;
}
// Prints a long document to PDF the way SIPRINT does: page 1 has an "L" at
// (100, 200) of the printable area, the next ones are blank and the last has a
// Latin-1 "ã" and an undefined Windows Latin-1 byte where SIPRINT puts them, by
// document-wide line index, which is past the 16-bit pen's range by then.
void check_document(cm::aot::NativePrinting& printing, const std::function<void(PMPageFormat)>& change_format) {
    PMPrintSession native_session = nullptr; PMPageFormat native_format = nullptr;
    require(::PMCreateSession(&native_session) == 0 && ::PMCreatePageFormat(&native_format) == 0 &&
            ::PMSessionDefaultPageFormat(native_session, native_format) == 0, "Native format setup");
    change_format(native_format);
    require(::PMSessionValidatePageFormat(native_session, native_format, nullptr) == 0, "Native format validation");
    PMRect paper{}; require(::PMGetAdjustedPaperRect(native_format, &paper) == 0, "Native paper rectangle");
    CFDataRef data = nullptr;
    require(::PMPageFormatCreateDataRepresentation(native_format, &data, kPMDataFormatXMLDefault) == 0, "Native format data");
    std::vector<std::uint8_t> flattened{'C', 'M', 'P', 'F', 0, 0, 0, 1};
    flattened.insert(flattened.end(), ::CFDataGetBytePtr(data), ::CFDataGetBytePtr(data) + ::CFDataGetLength(data));
    ::CFRelease(data); ::PMRelease(native_format); ::PMRelease(native_session);

    const auto path = std::filesystem::temp_directory_path() / ("cm-printing-" + std::to_string(::getpid()) + ".pdf");
    ::setenv("CM_AOT_PRINT_PDF", path.c_str(), 1);
    struct Output {
        const std::filesystem::path& path;
        ~Output() { std::error_code ignored; std::filesystem::remove(path, ignored); ::unsetenv("CM_AOT_PRINT_PDF"); }
    } output{path};
    std::uint32_t session = 0, settings = 0, format = 0;
    require(printing.create(cm::aot::NativePrinting::Kind::session, session) == 0, "Print session");
    require(printing.create(cm::aot::NativePrinting::Kind::settings, settings) == 0, "Print settings");
    require(printing.unflatten(flattened, format) == 0, "Print format");
    require(printing.default_print_settings(session, settings) == 0, "Default print settings");
    require(printing.default_print_settings(settings, session) != 0, "Incorrect print settings kinds accepted");
    bool accepted = false;
    require(printing.print_dialog(session, settings, format, accepted) == 0 && accepted, "Print destination");
    require(printing.begin_document(session, settings, format) == 0, "Begin document");

    const auto family = printing.font_family(bytes_of("Courier"));
    require(family == printing.font_family(bytes_of("Courier")), "Font family is not stable");
    printing.set_font(family);
    printing.set_font_size(50);
    require(printing.text_width(bytes_of("00000")) == 150, "Courier is 0.6 em wide per character");
    const auto metrics = printing.font_info();
    require(metrics.ascent > 0 && metrics.descent > 0 && metrics.width_max >= 30 && metrics.leading >= 0, "Font metrics");

    require(printing.begin_page(session, format) == 0, "Begin page");
    printing.move_to(100, 200);
    printing.draw_text(bytes_of("L"));
    require(printing.end_page(session) == 0, "End page");
    std::array<double, 4> printable{};
    require(printing.page_rect(format, printable) == 0, "Printable area");
    const int spacing = metrics.ascent + metrics.descent + metrics.leading;
    const int lines_per_page = int(printable[2] - printable[0]) / spacing;
    const int last_page = 32768 / (lines_per_page * spacing) + 1;
    for (int page = 1; page < last_page; ++page) {
        require(printing.begin_page(session, format) == 0, "Begin blank page");
        require(printing.end_page(session) == 0, "End blank page");
    }
    require(printing.begin_page(session, format) == 0, "Begin last page");
    printing.move_to(100, std::int16_t(printable[0] + (last_page * lines_per_page + 3) * spacing));
    constexpr std::array<std::uint8_t, 2> latin1_text{0xe3, 0x81};
    printing.draw_text(latin1_text);
    require(printing.end_page(session) == 0, "End last page");
    require(printing.end_document(session) == 0, "End document");

    const auto url = ::CFURLCreateFromFileSystemRepresentation(
        nullptr, reinterpret_cast<const UInt8*>(path.c_str()), CFIndex(path.native().size()), false);
    const auto document = ::CGPDFDocumentCreateWithURL(url);
    ::CFRelease(url);
    require(document && ::CGPDFDocumentGetNumberOfPages(document) == std::size_t(last_page + 1), "Printed page count");
    const auto ink = page_ink(::CGPDFDocumentGetPage(document, 1));
    // A scaled format widens the adjusted paper; the pen lands scaled on the sheet.
    const auto media = ::CGPDFPageGetBoxRect(::CGPDFDocumentGetPage(document, 1), kCGPDFMediaBox);
    const auto scale = media.size.width / (paper.right - paper.left);
    const auto expected_x = (100 - paper.left) * scale, baseline = (200 - paper.top) * scale;
    require(std::abs(ink.bottom - baseline) <= 3, "Text baseline is not at the pen");
    require(ink.left >= expected_x - 1 && ink.left <= expected_x + 12 * scale, "Text is not at the pen");
    require(ink.bottom_row_right > ink.top_row_right + 2, "Printed text is mirrored");
    require(page_ink(::CGPDFDocumentGetPage(document, 2)).right < 0, "Second page has marks");
    const auto last = page_ink(::CGPDFDocumentGetPage(document, std::size_t(last_page + 1)));
    const auto line_baseline = (printable[0] + 3 * spacing - paper.top) * scale;
    require(std::abs(last.bottom - line_baseline) <= 3, "A later page's line is not on its sheet");
    // MacRoman would read this byte as a low quotation mark, far shorter than an a with tilde.
    require(last.bottom - last.top > 20 * scale, "Printed text is not decoded as Latin-1");
    ::CGPDFDocumentRelease(document);
    require(printing.release(format) == 0 && printing.release(settings) == 0 && printing.release(session) == 0, "Print cleanup");
}
}
int main() {
    try {
        cm::aot::NativePrinting printing;
        std::uint32_t session = 0, format = 0;
        require(printing.create(cm::aot::NativePrinting::Kind::session, session) == 0 && session,
                "Native print session creation");
        require(printing.create(cm::aot::NativePrinting::Kind::page_format, format) == 0 && format,
                "Native page format creation");
        require(printing.default_page_format(session, format) == 0, "Native default page format");
        std::array<double, 4> original{}, restored{};
        require(printing.page_rect(format, original) == 0 && original[2] > original[0] && original[3] > original[1],
                "Native printable page bounds");
        std::vector<std::uint8_t> bytes;
        require(printing.flatten(format, bytes) == 0 && bytes.size() > 8, "Native page format persistence");
        require(printing.release(format) == 0, "Native page format release");
        require(printing.release(format) != 0, "Released printing token accepted twice");
        std::uint32_t reloaded = 0;
        require(printing.unflatten(bytes, reloaded) == 0 && reloaded && reloaded != format,
                "Page format restoration after original object release");
        require(printing.page_rect(reloaded, restored) == 0 && original == restored,
                "Page geometry changed across native serialization");
        require(printing.default_page_format(reloaded, session) != 0, "Incorrect native object kinds accepted");
        std::uint32_t invalid = 1;
        require(printing.unflatten({bytes.data(), 7}, invalid) != 0 && invalid == 0,
                "Truncated page format accepted");
        require(printing.release(reloaded) == 0 && printing.release(session) == 0, "Native print object cleanup");
        check_document(printing, [](PMPageFormat) {});
        check_document(printing, [](PMPageFormat f) { require(::PMSetOrientation(f, kPMLandscape, false) == 0, "Landscape"); });
        check_document(printing, [](PMPageFormat f) { require(::PMSetScale(f, 50) == 0, "Scale"); });
        std::cout << "Native printing object lifecycle and page-format roundtrip passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
