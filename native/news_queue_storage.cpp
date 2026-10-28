#include "news_detail.hpp"

namespace cm::aot::news {
using namespace detail;
namespace {
struct Failure {
    std::uint32_t assertion_site;
    unsigned name, extension, line;
    std::uint32_t fatal_back = 0, close_back = 0;
};
std::uint32_t fail(Cpu& c, const Frame& frame, const Failure& error) {
    assertion(c, frame, error.assertion_site, error.name, error.extension, error.line);
    Program program{c};
    if (error.fatal_back) program.call(0x1076f7d0, error.fatal_back, {0xffffffff});
    if (error.close_back) program.call(0x10747900, error.close_back, {frame.at(0x38)});
    return 0;
}
struct QueueFile {
    Cpu& c;
    const Frame& frame;
    bool reading;
    std::uint32_t transfer(std::uint32_t back, std::uint32_t buffer,
                           std::uint32_t size, std::uint32_t count) const {
        return Program{c}.call(reading ? 0x10747a80 : 0x10747cc0, back,
                               {buffer, size, count, c.u32(frame.at(0x38))});
    }
    void close(std::uint32_t back) const { Program{c}.call(0x10747900, back, {frame.at(0x38)}); }
};
}

std::uint32_t load_queue(Cpu& c, std::uint32_t address, std::uint32_t context) {
    Frame frame(c, 0x102cde30, 0x1c0, {.first_gpr = 28});
    Kept<28> queue_address(c, address);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    Queue queue{c, queue_address}; Program program{c}; QueueFile file{c, frame, true};
    if (compare_logical(c, context) == 0)
        return fail(c, frame, {0x102cde78,0x184,0x7c,0x2500});
    c.put32(frame.at(0x38), program.call(0x107475b0, 0x102cded4,
                                      {context, c.r(31) + 0x3fe3, 0, 0x16, 4}));
    if (compare_logical(c, c.u32(frame.at(0x38))) == 0)
        return fail(c, frame, {0x102cdef8,0x164,0x74,0x2508});
    if (compare(c, file.transfer(0x102cdf50, address + 4, 4, 1), 1) != 0)
        return fail(c, frame, {0x102cdf6c,0x144,0x6c,0x250f,0x102cdfb0,0x102cdfb8});
    if (compare(c, queue.item_count()) != 0) {
        queue.items() = program.call(0x10775130, 0x102cdfd8, {std::uint32_t(queue.item_count()) * Item::size});
        if (compare_logical(c, queue.items()) == 0)
            return fail(c, frame, {0x102cdffc,0x124,0x64,0x251b,0x102ce040,0x102ce048});
        const auto transferred = file.transfer(0x102ce064, queue.items(), Item::size, std::uint32_t(queue.item_count()));
        if (compare(c, queue.item_count(), transferred) != 0)
            return fail(c, frame, {0x102ce084,0x104,0x5c,0x2523,0x102ce0c8,0x102ce0d0});
    } else queue.items() = 0;
    if (compare(c, file.transfer(0x102ce0f4, address + 12, 4, 1), 1) != 0)
        return fail(c, frame, {0x102ce110,0xe4,0x54,0x2531,0x102ce154,0x102ce15c});
    queue.managers() = program.call(0x10775130, 0x102ce170, {std::uint32_t(queue.manager_count()) * Inbox::size});
    if (compare_logical(c, queue.managers()) == 0)
        return fail(c, frame, {0x102ce194,0xc4,0x4c,0x253a,0x102ce1d8,0x102ce1e0});
    const auto managers = file.transfer(0x102ce1fc, queue.managers(), Inbox::size, std::uint32_t(queue.manager_count()));
    if (compare(c, queue.manager_count(), managers) != 0)
        return fail(c, frame, {0x102ce21c,0xa4,0x44,0x2542,0x102ce260,0x102ce268});
    if (compare(c, file.transfer(0x102ce284, address + 16, 4, 1), 1) != 0)
        return fail(c, frame, {0x102ce2a0,0x84,0x3c,0x254a,0x102ce2e4,0x102ce2ec});
    file.close(0x102ce2fc);
    return 1;
}

std::uint32_t save_queue(Cpu& c, std::uint32_t address, std::uint32_t context) {
    Frame frame(c, 0x102ce320, 0x170, {.first_gpr = 28});
    Kept<28> queue_address(c, address);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    Queue queue{c, queue_address}; Program program{c}; QueueFile file{c, frame, false};
    if (compare_logical(c, context) == 0)
        return fail(c, frame, {0x102ce368,0x134,0x6c,0x2565});
    c.put32(frame.at(0x38), program.call(0x107475b0, 0x102ce3c4,
                                      {context, c.r(31) + 0x3fe3, 1, 0x16, 4}));
    if (compare_logical(c, c.u32(frame.at(0x38))) == 0)
        return fail(c, frame, {0x102ce3e8,0x114,0x64,0x256d,0x102ce42c});
    if (compare(c, file.transfer(0x102ce448, address + 4, 4, 1), 1) != 0)
        return fail(c, frame, {0x102ce464,0xf4,0x5c,0x2574,0x102ce4a8,0x102ce4b0});
    if (compare(c, queue.item_count()) != 0) {
        const auto transferred = file.transfer(0x102ce4d8, queue.items(), Item::size, std::uint32_t(queue.item_count()));
        if (compare(c, queue.item_count(), transferred) != 0)
            return fail(c, frame, {0x102ce4f8,0xd4,0x54,0x257f,0x102ce53c,0x102ce544});
    }
    if (compare(c, file.transfer(0x102ce560, address + 12, 4, 1), 1) != 0)
        return fail(c, frame, {0x102ce57c,0xb4,0x4c,0x2588,0x102ce5c0,0x102ce5c8});
    const auto managers = file.transfer(0x102ce5e4, queue.managers(), Inbox::size, std::uint32_t(queue.manager_count()));
    if (compare(c, queue.manager_count(), managers) != 0)
        return fail(c, frame, {0x102ce604,0x94,0x44,0x2590,0x102ce648,0x102ce650});
    if (compare(c, file.transfer(0x102ce66c, address + 16, 4, 1), 1) != 0)
        return fail(c, frame, {0x102ce688,0x74,0x3c,0x2598,0x102ce6cc,0x102ce6d4});
    file.close(0x102ce6e4);
    return 1;
}
}
