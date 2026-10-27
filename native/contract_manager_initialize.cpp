#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

// This module owns sixteen persistent date objects, used by its constructor
// to schedule the corresponding contract processing dates.
void initialize_dates(Cpu& c) {
    Frame frame(c, 0x100939b0, 0x40, {});
    Program program(c);
    for (unsigned index = 0; index < 16; ++index) {
        const auto date = c.toc(-0x799c - std::int32_t(index) * 4);
        program.call(0x10715880, 0x100939c4 + index * 8, {date});
    }
}
}
