#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
bool has_nonplayer(Cpu& c, std::uint32_t person) {
    if (!nonzero_logical(c, person)) return false;
    const auto record = c.u32(person + 0x1c); c.set(0, record);
    return nonzero_logical(c, record);
}
bool attached_role(Cpu& c, std::uint32_t person) {
    if (!has_nonplayer(c, person)) return false;
    const auto role = c.i8(person + 0x62); c.set(0, std::uint32_t(std::int32_t(role)));
    return compare(c, role, 8) == 0 || compare(c, role, 6) == 0 || compare(c, role, 5) == 0;
}
bool attached_kind(Cpu& c, std::uint32_t person, unsigned wanted) {
    if (!has_nonplayer(c, person)) return false;
    const auto role = c.u8(person + 0x62); c.set(0, role);
    return compare(c, role, int(wanted)) == 0;
}
std::int8_t age(Cpu& c, std::uint32_t person, bool result_register) {
    const auto stored = c.i16(person + 0x5c); c.set(0, extended(stored));
    const auto value = std::int8_t(stored);
    c.set(result_register ? 3 : 0, std::uint32_t(std::int32_t(value)));
    return value;
}
std::int8_t adaptability(Cpu& c, std::uint32_t person) {
    const auto value = c.i8(person + 0x63); c.set(0, std::uint32_t(std::int32_t(value))); return value;
}
std::int8_t qualification(Cpu& c, std::uint32_t attributes) {
    const auto value = c.i8(attributes + 0x2f); c.set(0, std::uint32_t(std::int32_t(value))); return value;
}
}

std::uint32_t role_available(Cpu& c, std::uint32_t requested, std::uint32_t person) {
    const auto role = std::uint32_t(std::int32_t(std::int8_t(requested)));
    c.set(0, role);
    if (compare_logical(c, role, 16) > 0) return result(c, 0);
    const auto table = c.toc(-0x5738); c.set(3, table); c.set(0, role * 4);
    const auto selected = c.u32(table + role * 4); c.set(3, selected); c.s.wr<4>(guest::reg::ctr, selected);
    bool available = false;
    switch (selected) {
    case 0x1007e510: {
        if (nonzero_logical(c, person)) {
            const auto years = age(c, person, true);
            if (compare(c, years, 30) >= 0) {
                const auto nonplayer = c.u32(person + 0x1c); c.set(0, nonplayer);
                if (nonzero_logical(c, nonplayer) || compare(c, years, 36) > 0) available = true;
                else if (compare(c, adaptability(c, person), 10) < 0) {
                    const auto attributes = c.u32(person + 0x14); c.set(3, attributes);
                    available = compare(c, qualification(c, attributes), 12) >= 0;
                }
            }
        }
        if (!available) available = attached_role(c, person);
        break;
    }
    case 0x1007e5ac: {
        bool mature = false;
        if (nonzero_logical(c, person)) {
            const auto years = age(c, person, true);
            if (compare(c, years, 30) >= 0) {
                const auto nonplayer = c.u32(person + 0x1c); c.set(0, nonplayer);
                mature = nonzero_logical(c, nonplayer) || compare(c, years, 36) > 0;
                if (!mature) {
                    const auto attributes = c.u32(person + 0x14); c.set(3, attributes);
                    mature = compare(c, qualification(c, attributes), 12) >= 0 &&
                             compare(c, adaptability(c, person), 10) < 0;
                }
            }
        }
        if (mature) {
            const auto current = c.i8(person + 0x62); c.set(0, std::uint32_t(std::int32_t(current)));
            available = compare(c, current, 5) != 0 && compare(c, current, 10) != 0;
        }
        if (!available) available = attached_role(c, person);
        break;
    }
    case 0x1007e660:
        if (has_nonplayer(c, person) && compare(c, age(c, person, false), 28) >= 0) {
            const auto current = c.u8(person + 0x62); c.set(0, current);
            available = compare(c, current, 9) == 0;
        }
        if (!available) available = attached_kind(c, person, 9);
        break;
    case 0x1007e6c8:
        if (nonzero_logical(c, person)) {
            const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
            available = nonzero_logical(c, attributes);
        }
        break;
    case 0x1007e6ec:
        if (nonzero_logical(c, person)) {
            const auto attributes = c.u32(person + 0x14); c.set(3, attributes);
            if (nonzero_logical(c, attributes)) {
                const auto nonplayer = c.u32(person + 0x1c); c.set(0, nonplayer);
                if (nonzero_logical(c, nonplayer)) available = true;
                else {
                    const auto years = age(c, person, false);
                    if (compare(c, years, 34) > 0) available = true;
                    else if (compare(c, years, 30) >= 0) available = compare(c, qualification(c, attributes), 12) >= 0;
                }
            }
        }
        break;
    case 0x1007e744:
        if (has_nonplayer(c, person)) {
            const auto primary = c.u8(person + 0x62); c.set(0, primary);
            available = compare(c, primary, 10) == 0;
            if (!available) { const auto secondary = c.u8(person + 0x60); c.set(0, secondary); available = compare(c, secondary, 10) == 0; }
        }
        if (!available) available = attached_kind(c, person, 10);
        break;
    case 0x1007e7a8: break;
    default: throw std::runtime_error("Unrecognized contract role rule at " + hex_address(selected));
    }
    return result(c, available);
}
}
