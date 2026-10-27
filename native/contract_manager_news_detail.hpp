#pragma once
#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager::news {
using namespace detail;
template<unsigned Register>
void person_link(Cpu& c, Program& p, const Frame& frame, Kept<Register>& offset,
                 std::uint32_t staff, std::uint32_t output, std::uint32_t first,
                 std::uint32_t second, std::uint32_t third, unsigned name_offset,
                 std::uint32_t check_back, std::uint32_t link_back,std::uint32_t link_type=0x7d7) {
    offset = std::uint32_t(offset) * Staff::size;
    const auto records = c.u32(staff); c.set(0, records);
    const auto available = std::int8_t(p.call(0x100cbde0, check_back,
        {records + std::uint32_t(offset), first, second, third, 0}));
    c.set(0, std::uint32_t(std::int32_t(available)));
    if (nonzero(c, available)) {
        const auto current = c.u32(staff); c.set(0, current);
        p.call(0x1074bab0, link_back, {output, frame.at(name_offset), link_type, current + std::uint32_t(offset)});
    }
}
}
