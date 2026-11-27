#include "croatia_country.hpp"
#include "croatia_second_rules.hpp"
#include "guest_abi.hpp"

namespace cm::aot::croatia_country {
namespace {
bool dispatch_awards(guest::Cpu& c, std::uint32_t address) {
    if (!croatia_second::uses_single_table(c.s, c.i16(c.toc(-0x7b80) + 2))) return false;

    // The 3.89 awards constructor creates six awards, using the regional A
    // award definitions with the national league as their source. It has no B awards.
    switch (address) {
    case 0x106ae950: c.set(30, c.toc(croatia_second::national_division_slot)); break;
    case 0x106ae96c: c.set(0, 6); break;
    case 0x106aebfc: c.s.pc = 0x106aecf8; return true;
    case 0x106aed20: c.set(31, c.toc(croatia_second::national_division_slot)); break;
    case 0x106aee6c: c.s.pc = 0x106aeedc; return true;
    default: return false;
    }
    c.s.pc = address + 4;
    return true;
}
}

bool dispatch(State& s, std::uint32_t address) {
    guest::Cpu c(s);
    if (address >= 0x106ae940 && address < 0x106aef00) return dispatch_awards(c, address);

    // r26 is the year supplied to every constructor in the country's list.
    if (!croatia_second::uses_single_table(s, std::int16_t(c.r(26)))) return false;
    switch (address) {
    case 0x10350c88:
        // The currently supported competitions are A1, A2, and the domestic cup.
        c.put32(c.r(25) + 0x0c, 3);
        break;
    case 0x10350d5c:
        c.set(5, c.toc(croatia_second::national_division_slot));
        break;
    case 0x10350d84:
        // A2 has already been published. Proceed to the cup allocation.
        c.set(3, 0xb4);
        s.pc = 0x10350db8;
        return true;
    case 0x10350df0:
        c.put32(c.r(6) + 8, c.r(28));
        break;
    case 0x10350df4:
        // The 3.89 country's June reference date is in the leap year 2000.
        c.set(6, 2000);
        break;
    default:
        return false;
    }
    s.pc = address + 4;
    return true;
}
}
