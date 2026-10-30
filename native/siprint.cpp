#include "siprint.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"

namespace cm::aot::siprint {
namespace {
using namespace guest;
constexpr std::uint32_t format_type = 0x50466f72, format_id = 1000;
struct Printer {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t resource_file() const { return extended(c.i16(address + 2)); }
    std::uint32_t port() const { return c.u32(address + 4); }
    std::uint32_t session() const { return c.u32(address + 8); }
    std::uint32_t format() const { return c.u32(address + 12); }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments = {}) const {
        unsigned reg = 3;
        for (auto value : arguments) c.set(reg++, value);
        c.call(target, back);
        // The CFM glue saves the caller's TOC at sp+20.
        if (target >= 0x1077fad8 && target < 0x10781320)
            c.set(2, c.u32(c.sp() + 20));
        return c.r(3);
    }
};
}

std::uint32_t destroy(Cpu& c, std::uint32_t printer, std::int16_t deleting) {
    Frame frame(c, 0x1074c960, 0x40, {.first_gpr = 31});
    keep(c, {{31, printer}});
    if (compare(c, printer) != 0 && compare(c, deleting) > 0)
        Program{c}.call(0x107735a0, 0x1074c984);
    return result(c, printer);
}

void idle(Cpu&) {}

void initialize(Cpu& c, std::uint32_t printer) {
    Frame frame(c, 0x1074c9a0, 0x40, {.first_gpr = 31});
    Printer object{c, printer}; Program p{c};
    keep(c, {{31, printer}});
    p.call(0x107803f0, 0x1074c9bc, {printer + 4});
    p.call(0x10780378, 0x1074c9c4);
    c.put16(printer + 2, c.r(3));
    if (compare(c, p.call(0x10780408, 0x1074c9d4, {printer + 8})) != 0) return;
    enter(c, 0x1074c9e8, {printer}); load_format(c, printer);
    if (compare_logical(c, object.format()) != 0) {
        enter(c, 0x1074c9fc, {printer}); save_format(c, printer);
        c.put8(printer, 1);
    }
    if (compare_logical(c, object.session()) == 0) return;
    p.call(0x107804e0, 0x1074ca18, {printer + 8});
    c.put32(printer + 8, 0);
    p.call(0x107803c0, 0x1074ca2c, {object.resource_file()});
    p.call(0x107804f8, 0x1074ca38, {object.port()});
}

void load_format(Cpu& c, std::uint32_t printer) {
    Frame frame(c, 0x1074ca60, 0x50, {.first_gpr = 28});
    Printer object{c, printer}; Program p{c};
    keep(c, {{28, printer}});
    const auto previous = p.call(0x10780378, 0x1074ca84);
    keep(c, {{29, previous}});
    p.call(0x107803c0, 0x1074ca98, {object.resource_file()});
    const auto resource = p.call(0x10780150, 0x1074caac, {format_type, format_id});
    keep(c, {{31, resource}});
    bool valid = false;
    if (compare(c, resource) != 0) {
        const auto status = p.call(0x10780420, 0x1074cac0, {resource, printer + 12});
        keep(c, {{30, status}});
        if (compare(c, status) == 0) {
            p.call(0x10780438, 0x1074cadc, {object.session(), object.format(), frame.at(0x38)});
            keep(c, {{30, c.r(3)}});
        }
    }
    if (compare_logical(c, resource) != 0) valid = compare(c, c.r(30)) == 0;
    if (!valid) {
        if (compare(c, p.call(0x10780450, 0x1074cafc, {printer + 12})) == 0)
            p.call(0x10780468, 0x1074cb14, {object.session(), object.format()});
    }
    p.call(0x107803c0, 0x1074cb20, {previous});
}

void save_format(Cpu& c, std::uint32_t printer) {
    Frame frame(c, 0x1074cb50, 0x50, {.first_gpr = 30});
    Printer object{c, printer}; Program p{c};
    keep(c, {{30, printer}});
    const auto previous = p.call(0x10780378, 0x1074cb6c);
    keep(c, {{31, previous}});
    p.call(0x107803c0, 0x1074cb80, {object.resource_file()});
    auto resource = p.call(0x10780150, 0x1074cb94, {format_type, format_id});
    c.put32(frame.at(0x38), resource);
    if (compare_logical(c, resource) != 0) {
        p.call(0x10780480, 0x1074cba8); c.put32(frame.at(0x38), 0);
    }
    if (compare(c, p.call(0x10780498, 0x1074cbc0, {object.format(), frame.at(0x38)})) == 0) {
        p.call(0x107804b0, 0x1074cbe4, {c.u32(frame.at(0x38)), format_type, format_id, c.toc(-0x3e14)});
        p.call(0x107804c8, 0x1074cbf0, {c.u32(frame.at(0x38))});
    }
    p.call(0x107803c0, 0x1074cbfc, {previous});
}

std::int32_t available(Cpu& c, std::uint32_t printer) { return c.i8(printer); }

void page_setup(Cpu& c, std::uint32_t printer) {
    Frame frame(c, 0x1074cc30, 0x50, {.first_gpr = 30});
    Printer object{c, printer}; Program p{c};
    keep(c, {{30, printer}});
    if (compare_logical(c, c.u8(printer)) == 0) return;
    p.call(0x107803f0, 0x1074cc5c, {printer + 4});
    p.call(0x10780378, 0x1074cc64); c.put16(printer + 2, c.r(3));
    if (compare(c, p.call(0x10780408, 0x1074cc74, {printer + 8})) != 0) return;
    if (compare(c, p.call(0x107806f0, 0x1074cc90, {object.session(), object.format(), frame.at(0x38)})) == 0 &&
        compare_logical(c, c.u8(frame.at(0x38))) != 0) {
        const auto previous = p.call(0x10780378, 0x1074ccac);
        keep(c, {{31, previous}});
        p.call(0x107803c0, 0x1074ccbc, {object.resource_file()});
        const auto resource = p.call(0x10780150, 0x1074ccd0, {format_type, format_id});
        c.put32(frame.at(0x3c), resource);
        if (compare_logical(c, resource) != 0) {
            p.call(0x10780480, 0x1074cce4); c.put32(frame.at(0x3c), 0);
        }
        if (compare(c, p.call(0x10780498, 0x1074ccfc, {object.format(), frame.at(0x3c)})) == 0) {
            p.call(0x107804b0, 0x1074cd20, {c.u32(frame.at(0x3c)), format_type, format_id, c.toc(-0x3e14)});
            p.call(0x107804c8, 0x1074cd2c, {c.u32(frame.at(0x3c))});
        }
        p.call(0x107803c0, 0x1074cd38, {previous});
    }
    if (compare_logical(c, object.session()) == 0) return;
    p.call(0x107804e0, 0x1074cd50, {printer + 8}); c.put32(printer + 8, 0);
    p.call(0x107803c0, 0x1074cd64, {object.resource_file()});
    p.call(0x107804f8, 0x1074cd70, {object.port()});
}

void initialize_global(Cpu& c) {
    Frame frame(c, 0x1074d340, 0x40, {});
    const auto printer = c.toc(-0x7408);
    leave(c, {{3, printer}, {4, c.toc(-0x5b94)}, {5, c.toc(-0x3e1c)}});
    c.put32(printer + 12, 0); c.put8(printer, 0); c.put32(printer + 8, 0);
    Program{c}.call(0x10772df0, 0x1074d36c);
}
}
