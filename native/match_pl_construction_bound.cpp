#include "match_pl_construction.hpp"

namespace cm::aot::match_pl {
using namespace guest;
std::uint32_t PlayerConstruction::personality_bound() {
    bool home = (c.cr(0) & 2) != 0; // Use the retained comparison before the home-only reread.
    if (home) {
        const auto side = std::int8_t(actor.side()); c.set(3, actor.match()); c.set(3, c.r(3) + std::uint32_t(std::int32_t(side)));
        home = compare(c, c.i8(c.r(3) + 0x1c6)) == 0;
    }
    c.set(5, actor.match()); c.set(4, actor.match()); c.set(3, c.u32(c.r(5) + 0x3118));
    const auto attendance = c.u32(c.r(4) + 0x7c5c);
    if (home) c.set(4, attendance);
    const auto capacity_record = c.u32(c.r(3) + 0x18);
    if (!home) c.set(3, capacity_record);
    bool usable = compare_logical(c, capacity_record) != 0;
    if (usable) {
        c.set(3, c.u32(c.r(5) + 0x3118)); c.set(3, c.u32(c.r(3) + 0x18));
        const auto capacity = c.i32(c.r(3) + 4); if (!home) c.set(3, capacity);
        usable = compare(c, capacity) > 0;
    }
    if (usable) { c.set(3, c.u32(c.r(5) + 0x3118)); c.set(3, c.u32(c.r(3) + 0x18)); c.set(8, c.u32(c.r(3) + 4)); }
    else c.set(8, 1500);
    if (home) {
        const auto attendance_product = attendance * 2000; c.set(3, attendance_product);
        c.set(5, c.u8(staff + 0x67)); c.set(4, 0x43300000); c.set(7, c.u8(staff + 0x67)); c.set(6, 0x43300000);
        c.set(3, attendance_product ^ 0x80000000); c.put32(at(0x5c), 0x43300000);
        const auto first_trait = std::uint32_t(std::int32_t(std::int8_t(c.r(5)))); c.set(7, std::int32_t(std::int8_t(c.r(7))));
        c.put32(at(0x60), c.r(3)); c.set(5, first_trait ^ 0x80000000); c.set_f(1, c.f64(doubles + 0x80));
        c.set(7, c.r(7) ^ 0x80000000); c.set_f(0, c.f64(at(0x5c))); c.set(3, c.r(8) ^ 0x80000000);
        c.put32(at(0x58), c.r(5)); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(2, c.f64(doubles + 0x80));
        c.put32(at(0x54), c.r(4)); c.set_f(4, c.f64(doubles + 0x80)); c.set_f(0, fp.fmul(c.f(31), c.f(0)));
        c.set_f(1, c.f64(at(0x54))); c.put32(at(0x50), c.r(7)); c.set_f(2, fp.fsub(c.f(1), c.f(2)));
        c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x4c), c.r(6)); c.set_f(0, fp.fmul(c.f(30), c.f(0)));
        c.set_f(3, c.f64(at(0x4c))); c.put32(at(0x48), c.r(3)); c.set_f(3, fp.fsub(c.f(3), c.f(4)));
        c.put32(at(0x44), 0x43300000); c.set_f(2, fp.fmul(c.f(2), c.f(0))); c.set_f(0, c.f64(at(0x44)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(1, fp.fmul(c.f(3), c.f(2))); c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
        c.put64(at(0x3c), fp.fctiwz(0, c.f(0))); return c.u32(at(0x40));
    }
    c.set(3, c.u32(actor.address + 0x70)); c.set(5, 0x43300000); c.set(4, c.u8(staff + 0x67)); c.set(7, 0x43300000);
    c.set(3, c.u8(c.r(3) + 0x2c)); c.set(9, 0x43300000); c.set(6, std::int32_t(std::int8_t(c.r(4))));
    c.set(10, c.u8(staff + 0x67)); c.set(3, std::int32_t(std::int8_t(c.r(3)))); c.set(4, c.r(8) ^ 0x80000000);
    const auto adjustment = c.r(3) - 10; c.set(3, adjustment); c.set(8, c.r(6) ^ 0x80000000); c.set(6, adjustment * 100);
    c.set(10, std::int32_t(std::int8_t(c.r(10)))); c.set(3, 0x43300000); c.put32(at(0x4c), c.r(5));
    c.set(10, c.r(10) ^ 0x80000000); c.set_f(2, c.f64(doubles + 0x80)); c.set(5, c.r(6) + 2000);
    c.put32(at(0x48), c.r(8)); const auto attendance_product = c.r(5) * attendance;
    c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x44), c.r(7)); c.set_f(5, c.f64(doubles + 0x80));
    c.set_f(0, c.f64(at(0x44))); c.put32(at(0x40), c.r(10)); c.put32(at(0x3c), c.r(9));
    c.set_f(3, fp.fsub(c.f(0), c.f(1))); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x50), attendance_product ^ 0x80000000);
    c.set_f(4, c.f64(at(0x3c))); c.set_f(0, c.f64(at(0x4c))); c.put32(at(0x58), c.r(4));
    c.set_f(4, fp.fsub(c.f(4), c.f(5))); c.set_f(0, fp.fsub(c.f(0), c.f(2))); c.put32(at(0x54), c.r(3));
    c.set_f(2, fp.fmul(c.f(31), c.f(0))); c.set_f(0, c.f64(at(0x54))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set_f(1, fp.fmul(c.f(30), c.f(2))); c.set_f(1, fp.fmul(c.f(3), c.f(1))); c.set_f(1, fp.fmul(c.f(4), c.f(1)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.put64(at(0x5c), fp.fctiwz(0, c.f(0))); return c.u32(at(0x60));
}
}
