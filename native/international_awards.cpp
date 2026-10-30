#include "international_awards.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::international_awards {
namespace {
using namespace guest;
constexpr std::uint32_t missing = 0xffffffff;
struct Manager {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t awards() const { return c.u32(address + 4); }
    std::uint32_t award(unsigned index) const { return c.u32(awards() + index * 4); }
    std::uint32_t identity() const { return c.u32(address + 24); }
};
struct Date {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t year() const { return c.u16(address + 2); }
    std::int32_t day() const { return c.i16(address); }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments = {}) const {
        unsigned reg = 3;
        for (const auto argument : arguments) c.set(reg++, argument);
        c.call(target, back);
        return c.r(3);
    }
};
std::uint32_t indirect(Cpu& c, int slot) { return c.u32(c.toc(slot)); }
}

std::uint32_t construct(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x10440620, 0x40, {.first_gpr = 31});
    Program p{c};
    keep(c, {{31, file}});
    c.put32(frame.at(0x58), manager);
    p.call(0x10444390, 0x10440640, {manager});
    const auto object = c.u32(frame.at(0x58));
    leave(c, {{6, c.toc(-0x61e0)}, {5, 0}, {3, object}, {4, 1}});
    compare_logical(c, file);
    c.put32(object, c.r(6));
    c.put32(object + 16, 0);
    c.put32(object + 24, 1);
    c.put8(object + 20, 4);
    bool loaded = false;
    if (file != 0) {
        const auto status = p.call(0x10446b00, 0x10440684, {object, file});
        loaded = compare(c, std::int8_t(status)) != 0;
    }
    if (!loaded) {
        // The derived vtable's +8 method initializes the six awards. Calling
        // the owned method directly retains the original pointer-glue boundary.
        leave(c, {{3, c.u32(frame.at(0x58))}, {4, file}});
        c.set(12, c.u32(c.u32(c.r(3)) + 8));
        c.put32(c.sp() + 0x14, c.r(2));
        const auto descriptor = c.r(12);
        c.set(2, c.u32(descriptor + 4));
        c.link(0x104406a8);
        if (c.u32(descriptor) == 0x104406d0) initialize(c, c.r(3), file);
        else c.call(c.u32(descriptor), 0x104406a8);
        c.set(2, c.u32(c.sp() + 0x14));
    }
    return result(c, c.u32(frame.at(0x58)));
}

void initialize(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x104406d0, 0xa0, {.first_gpr = 26});
    Program p{c};
    Manager object{c, manager};
    const auto message = c.toc(-0x7ba8), source = c.toc(-0x4a44);
    const auto identity = object.identity();
    keep(c, {{26, message}, {27, source}, {28, manager}, {29, file}, {30, 0}, {31, identity}});
    c.put16(manager + 8, 6);
    p.call(0x10775130, 0x1044070c, {24});
    c.put32(manager + 4, c.r(3));
    if (compare_logical(c, object.awards()) == 0) {
        report_assertion(c, {0x10440730, source, source + 0x19, source + 0x26,
                            frame.at(0x50), frame.at(0x48), 0x63},
                         [&](std::uint32_t target, std::uint32_t back) { c.call(target, back); });
        c.set(3, c.toc(-0x7bac));
        return;
    }
    c.put8(frame.at(0x70), 5);
    c.put32(frame.at(0x74), missing);
    c.put32(frame.at(0x78), missing);
    c.put32(frame.at(0x7c), missing);
    c.put8(frame.at(0x80), 10);
    leave(c, {{4, missing}});
    struct Specification { int label; unsigned first, second; std::uint32_t allocated, constructed; };
    constexpr std::array specifications{
        Specification{-0x6ba0, 9000, 8000, 0x1044079c, 0x104407e8},
        Specification{-0x6ba4, 9000, 8000, 0x104407f8, 0x10440840},
        Specification{-0x6bb0, 7000, 5250, 0x10440850, 0x10440898},
        Specification{-0x6ba8, 9000, 8000, 0x104408a8, 0x104408f0},
        Specification{-0x6bac, 7000, 6000, 0x10440900, 0x10440948},
        Specification{-0x70f8, 5000, 3000, 0x10440958, 0x1044099c}};
    unsigned ordinal = 0;
    for (unsigned index = 0; index < specifications.size(); ++index) {
        const auto& spec = specifications[index];
        const auto award = p.call(0x10773520, spec.allocated, {48});
        keep(c, {{26, award}});
        if (compare(c, award) != 0) {
            c.put32(frame.at(0x38), index == 0 ? frame.at(0x70) : 0);
            c.put32(frame.at(0x3c), 0);
            c.put32(frame.at(0x40), file);
            c.put32(frame.at(0x44), 1);
            leave(c, {{4, identity}, {5, ordinal}, {6, indirect(c, spec.label)},
                      {7, spec.first}, {8, spec.second}, {9, missing}, {10, 1}});
            // Failed allocations do not consume an ordinal. The final award
            // leaves the prior ordinal in r30, as its constructor observes.
            if (index != specifications.size() - 1) keep(c, {{30, ++ordinal}});
            p.call(0x1044cad0, spec.constructed);
        }
        const auto awards = object.awards();
        if (index != specifications.size() - 1) leave(c, {{4, awards}, {3, 48}});
        else c.set(3, awards);
        c.put32(awards + index * 4, award);
    }
}

void publish_due(Cpu& c, std::uint32_t manager, std::uint32_t date) {
    Frame frame(c, 0x104409c0, 0x90, {.first_gpr = 28});
    Program p{c};
    Manager object{c, manager};
    Date today{c, date};
    const auto ratings_global = c.toc(-0x7bb4);
    keep(c, {{31, ratings_global}, {30, date}, {29, manager}});
    c.set(6, today.year());
    if (compare_logical(c, c.r(6), 2001) == 0) return;
    struct Deadline { unsigned day, month, buffer, copy; std::uint32_t back; };
    constexpr std::array deadlines{
        Deadline{6, 4, 0x70, 0x58, 0x10440a08},
        Deadline{6, 5, 0x68, 0x50, 0x10440aa0},
        Deadline{16, 5, 0x60, 0x48, 0x10440b50}};
    for (unsigned index = 0; index < deadlines.size(); ++index) {
        const auto& deadline = deadlines[index];
        const auto made = p.call(0x107156f0, deadline.back,
            {frame.at(deadline.buffer), deadline.day, deadline.month, today.year(), missing});
        c.set(5, c.u32(made + 4));
        c.put32(frame.at(deadline.copy), c.u32(made));
        c.put32(frame.at(deadline.copy + 4), c.r(5));
        c.set(4, 0);
        Date scheduled{c, frame.at(deadline.copy)};
        c.set(3, scheduled.year());
        bool due = compare_logical(c, c.r(3), today.year()) == 0;
        if (due) {
            c.set(3, std::uint32_t(scheduled.day()));
            due = compare(c, scheduled.day(), today.day()) == 0;
            if (due) c.set(4, 1);
        }
        if (compare(c, c.r(4)) == 0) continue;
        if (index == 0) {
            c.put32(frame.at(0x38), missing); c.put32(frame.at(0x3c), 2); c.put32(frame.at(0x40), missing);
            p.call(0x10444580, 0x10440a88,
                {manager, date, object.award(0), c.u32(c.u32(ratings_global) + 16), missing, missing, missing, missing});
        } else if (index == 1) {
            const auto ratings = c.u32(c.u32(ratings_global) + 16);
            keep(c, {{28, ratings}});
            // r6 still holds the pointer loaded from the global, not ratings.
            c.set(6, c.u32(ratings_global));
            p.call(0x10444e80, 0x10440afc, {manager, date, object.award(0)});
            c.put32(frame.at(0x38), missing); c.put32(frame.at(0x3c), 3); c.put32(frame.at(0x40), missing);
            p.call(0x10444580, 0x10440b38,
                {manager, date, object.award(1), ratings, missing, missing, missing, missing});
        } else {
            const auto ratings = c.u32(c.u32(ratings_global) + 16);
            keep(c, {{28, ratings}});
            c.set(7, c.u32(ratings_global));
            constexpr std::array slots{-0x7974, -0x7a70, -0x6b9c, -0x7970};
            constexpr std::array backs{0x10440bc4u, 0x10440bf0u, 0x10440c1cu, 0x10440c48u};
            for (unsigned kind = 0; kind < slots.size(); ++kind)
                p.call(0x104449c0, backs[kind], {manager, date, object.award(kind + 2), ratings, indirect(c, slots[kind]), 0, 1, 0});
        }
    }
}
}
