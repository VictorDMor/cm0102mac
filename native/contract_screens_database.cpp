#include "contract_screens.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

void load_database_with_language_setup(Cpu& c, std::uint32_t directory, std::uint32_t options) {
    Frame frame(c, 0x1009ef30, 0x40, {});
    c.set({{0, c.lr()}, {3, directory}, {4, options}, {5, 1}});
    c.call(0x100a55a0, 0x1009ef44);
}

void save_database(Cpu& c, std::uint32_t directory, std::uint32_t options, std::uint32_t version) {
    Frame frame(c, 0x1009ef60, 0x40, {});
    c.set({{0, options}, {3, directory}, {4, 1}, {5, options}, {6, version}});
    c.call(0x100abfb0, 0x1009ef80);
}

void load_database(Cpu& c, std::uint32_t directory, std::uint32_t options) {
    Frame frame(c, 0x1009ef90, 0x40, {});
    c.set({{0, c.lr()}, {3, directory}, {4, options}, {5, 0}});
    c.call(0x100a55a0, 0x1009efa4);
}
}
