#include "printing_bridge.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
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
        std::cout << "Native printing object lifecycle and page-format roundtrip passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
