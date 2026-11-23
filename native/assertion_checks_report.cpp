// The assertion module, block `report` (0x107755c0): cases for the C assertion reporter. It loads the error stream (the FILE array's third
// entry) and the format "Assertion (%s) failed in \"%s\" on line %d\n", calls fprintf (0x1077b610) and abort (0x10773fb0). fprintf runs for
// real on both sides (the original's page calls its C++ owner in place, so an answer would reach the rewrite only); abort is answered on both
// sides (the game ends there), so the run goes on to the reporter's return and its registers, its frame and what fprintf wrote are compared.
// r9 and r10 are distinct: fprintf stores them in its frame. Labels start with "report:".
#include "assertion_checks.hpp"
#include <string>

namespace cm::aot::assertion_checks {
namespace {
constexpr std::uint32_t report_entry = 0x107755c0, abort_entry = 0x10773fb0;
constexpr std::uint32_t condition_at = scratch_base + 0x20000, file_at = scratch_base + 0x21000;

std::vector<std::pair<unsigned, std::uint32_t>> registers(std::uint32_t condition, std::uint32_t file, std::uint32_t line) {
    std::vector<std::pair<unsigned, std::uint32_t>> all;
    for (unsigned n = 3; n <= 10; ++n) all.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    all[0].second = condition;
    all[1].second = file;
    all[2].second = line;
    return all;
}
std::vector<Write> text(std::uint32_t at, const std::string& value) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), std::uint8_t(value[n])));
    memory.push_back(w8(at + std::uint32_t(value.size()), 0));
    return memory;
}
}

ASSERTION_CASES("report") {
    const auto make = [&](const std::string& condition, const std::string& file, std::uint32_t line, std::string label, std::uint32_t abort_answer = 0x77) {
        Case item{.entry = report_entry, .registers = registers(condition_at, file_at, line), .label = "report: " + std::move(label)};
        item.answers = {{abort_entry, abort_answer}};
        append(item.memory, text(condition_at, condition));
        append(item.memory, text(file_at, file));
        return item;
    };
    // The game's own call shape first (QSORT4X, FONT.CPP and the font engine pass the pool's strings and a line number).
    cases.push_back(make("size > 0", "graph_carbon.cpp", 0x1a1, "typical"));
    cases.push_back(make("", "", 0, "empty strings, line 0"));
    for (const std::uint32_t line : {1u, 9u, 10u, 0x7fu, 0x1a1u, 99999u, 0x7fffffffu, 0x80000000u, 0xffffffffu})
        cases.push_back(make("n != 0", "DATE.CPP", line, "line " + hex(line)));
    cases.push_back(make("100%s%d%n", "50%s.CPP", 3, "percent signs in the strings (arguments, not formats)"));
    for (const std::size_t size : {0x40u, 0xffu, 0x100u, 0x101u, 0x200u, 0x3ffu, 0x400u, 0x800u})
        cases.push_back(make(std::string(size, 'c'), std::string(size, 'f'), 77, "strings of " + hex(std::uint32_t(size))));
    cases.push_back(make("x", std::string(0x300, 'f'), 5, "short condition, long file"));
    cases.push_back(make(std::string(0x300, 'c'), "x", 5, "long condition, short file"));
    for (const std::uint32_t answer : {0u, 1u, 0xffffffffu, 0x80000000u})
        cases.push_back(make("a", "b", 1, "abort answered " + hex(answer), answer));
    // A null condition or file (the formatter prints what it prints for a null %s: the same on both sides).
    for (const auto [condition_null, file_null] : {std::pair{true, false}, std::pair{false, true}, std::pair{true, true}}) {
        auto item = make("a", "b", 2, std::string("null ") + (condition_null ? "condition " : "") + (file_null ? "file" : ""));
        if (condition_null) item.registers[0].second = 0;
        if (file_null) item.registers[1].second = 0;
        cases.push_back(item);
    }
}
}
