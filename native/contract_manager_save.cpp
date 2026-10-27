#include "contract_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
struct WriteProblem {
    std::uint32_t start;
    unsigned first_buffer, second_buffer, detail, line;
};
struct ContractSection {
    unsigned count, data, width;
    std::uint32_t header_back, data_back;
    WriteProblem header_problem, data_problem;
};
}

void save_contracts(Cpu& c, std::uint32_t manager_address, std::uint32_t file_address) {
    Frame frame(c, 0x10093030, 0x260, {.first_gpr = 26});
    c.set(5, 0); Kept<30> message(c, c.toc(-0x79e0)); Kept<29> title(c, c.toc(-0x79dc));
    Kept<27> object(c, manager_address); Kept<28> error_buffer(c, c.toc(-0x7ba8));
    Kept<26> file(c, file_address); Kept<31> pool(c, c.toc(slot::pool)); Program program(c);
    c.set(0, 1); c.put16(frame.at(0x3a), 0); c.put8(frame.at(0x38), 1);
    const auto pending_sort = c.i32(object + 0x1c); c.set(0, std::uint32_t(pending_sort));
    if (nonzero(c, pending_sort)) {
        const auto first = c.u32(object + 0x18), count = c.u32(object + 0x14), rows = c.u32(object + 0x0c);
        c.set(4, first); c.set(5, 16); c.set(0, count); c.set(7, rows); c.set(3, first * 16);
        program.call(0x10768300, 0x10093098, {rows + first * 16, count - first, 16, c.toc(-0x5724)});
        c.set(0, 0); c.put32(object + 0x1c, 0);
    }
    // Keep the on-disk representation exactly: guest bytes and original
    // widths, including record padding and signed 16-bit list counts.
    auto report = [&](const WriteProblem& problem) {
        program.call(0x10739890, problem.start + 0x18,
                     {pool + 0x37, 0, 0, frame.at(problem.first_buffer), frame.at(problem.second_buffer)});
        const auto version = program.call(0x10768810, problem.start + 0x1c);
        program.call(0x1077b720, problem.start + 0x3c,
                     {error_buffer, pool + 0x4c, version, frame.at(problem.first_buffer), frame.at(problem.second_buffer), problem.line, pool + problem.detail});
        program.call(0x10727890, problem.start + 0x48, {title, pool + 0x7700});
        program.call(0x10727890, problem.start + 0x58, {message, pool + 0x7710, error_buffer});
        program.call(0x10769b60, problem.start + 0x68, {title, message, 0});
    };
    const auto stream = program.call(0x107475b0, 0x100930b8, {file, pool + 0x71e7, 1, 0x16, 4});
    const bool opened = nonzero_logical(c, stream); c.put32(frame.at(0x3c), stream);
    if (!opened) { report({0x100930c4, 0x228, 0xa0, 0x76c4, 0x31cb}); return; }
    constexpr std::array sections{
        ContractSection{0x2c, 0x28, 24, 0x10093144, 0x100931d4,
            {0x1009314c, 0x208, 0x98, 0x77f4, 0x31d2}, {0x100931e0, 0x1e8, 0x90, 0x7845, 0x31dd}},
        ContractSection{8, 0, 100, 0x10093260, 0x100932f0,
            {0x10093268, 0x1c8, 0x88, 0x788c, 0x31e5}, {0x100932fc, 0x1a8, 0x80, 0x78d9, 0x31f0}},
        ContractSection{0x14, 0x0c, 16, 0x1009337c, 0x1009340c,
            {0x10093384, 0x188, 0x78, 0x791c, 0x31f8}, {0x10093418, 0x168, 0x70, 0x7965, 0x3203}}
    };
    for (const auto& section : sections) {
        const auto header = program.call(0x10747cc0, section.header_back,
                                        {object + section.count, 4, 1, c.u32(frame.at(0x3c))});
        if (compare(c, std::int32_t(header), 1) != 0) { report(section.header_problem); return; }
        const auto count = c.i32(object + section.count); c.set(5, std::uint32_t(count));
        if (nonzero(c, count)) {
            const auto written = program.call(0x10747cc0, section.data_back,
                                              {c.u32(object + section.data), section.width, std::uint32_t(count), c.u32(frame.at(0x3c))});
            const auto current_count = c.i32(object + section.count); c.set(0, std::uint32_t(current_count));
            if (compare(c, current_count, std::int32_t(written)) != 0) { report(section.data_problem); return; }
        }
    }
    const auto saved_cursor = program.call(0x10747cc0, 0x10093498, {object + 0x18, 4, 1, c.u32(frame.at(0x3c))});
    if (compare(c, std::int32_t(saved_cursor), 1) != 0) { report({0x100934a0, 0x148, 0x68, 0x79a4, 0x320b}); return; }
    struct ListSection {
        unsigned offset;
        std::uint32_t count_back, row_back, next_back;
        WriteProblem count_problem, row_problem;
    };
    constexpr std::array lists{
        ListSection{0x34, 0x1009353c, 0x100935d8, 0x10093654,
            {0x10093544, 0x128, 0x60, 0x79eb, 0x321a}, {0x100935e0, 0x108, 0x58, 0x7a28, 0x3228}},
        ListSection{0x38, 0x1009368c, 0x1009372c, 0x100937a8,
            {0x10093694, 0xe8, 0x50, 0x79eb, 0x3239}, {0x10093734, 0xc8, 0x48, 0x7a28, 0x3247}}
    };
    for (const auto& section : lists) {
        const auto list = c.u32(object + section.offset); c.set(3, list);
        std::int16_t count = 0;
        if (nonzero_logical(c, list)) count = c.i16(list);
        c.set(0, extended(count)); c.put16(frame.at(0x3a), extended(count));
        const auto saved_count = program.call(0x10747cc0, section.count_back,
                                             {frame.at(0x3a), 2, 1, c.u32(frame.at(0x3c))});
        if (compare(c, std::int32_t(saved_count), 1) != 0) { report(section.count_problem); return; }
        const auto current = c.u32(object + section.offset); c.set(3, current);
        if (nonzero_logical(c, current)) {
            const auto first = c.u32(current + 4); c.set(0, first); c.put32(current + 12, first);
            for (;;) {
                const auto record = program.call(0x10738e00, section.next_back, {c.u32(object + section.offset)});
                if (!nonzero_logical(c, record)) break;
                const auto saved = program.call(0x10747cc0, section.row_back, {record, 136, 1, c.u32(frame.at(0x3c))});
                if (compare(c, std::int32_t(saved), 1) != 0) { report(section.row_problem); return; }
            }
        }
    }
    const auto flag = program.call(0x10747cc0, 0x100937c4, {frame.at(0x38), 1, 1, c.u32(frame.at(0x3c))});
    if (compare(c, std::int32_t(flag), 1) != 0) { report({0x100937cc, 0xa8, 0x40, 0x7a5e, 0x3250}); return; }
    program.call(0x10747900, 0x10093840, {frame.at(0x3c)});
}
}
