// DISPLAY.CPP checker cases for the interpreter's dispatcher (display_interpreter.cpp): the command read
// and the commands outside the table. Each command's cases live with its file's checks.
#include "display_checks.hpp"
#include <string>

namespace cm::aot::display_checks {
namespace {
constexpr std::uint32_t interpret = 0x1071de80;
}

DISPLAY_CASES("interpreter") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const auto stream = scratch_base + 0x100, bytes = scratch_base + 0x200;
    for (const auto table : {game + 0x2c68, game + 0x12d6b0})
        for (const std::uint32_t length : {0u, 1u}) {
            for (const std::uint32_t command : {27u, 0x7fu, 0x80u, 0xffu})
                cases.push_back({.entry = interpret, .registers = {{3, table}, {4, 0x1234}, {5, 0xabcd}, {6, stream}, {21, 0x42}},
                                 .memory = {{4, stream, bytes}, {4, stream + 4, 7}, {4, stream + 8, length}, {1, bytes, command}},
                                 .label = "dispatch, length " + std::to_string(length) + ", command " + std::to_string(command)});
            // An empty stream: the read reports and the command is the caller's r21.
            for (const std::uint32_t r21 : {0x1bu, 0x80u, 0x1ffu})
                if (length == 0)
                    cases.push_back({.entry = interpret, .registers = {{3, table}, {4, 0}, {5, 0}, {6, stream}, {21, r21}},
                                     .memory = {{4, stream, bytes}, {4, stream + 8, 0}},
                                     .label = "dispatch, empty stream, r21 " + std::to_string(r21)});
        }
}
}
