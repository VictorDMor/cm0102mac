"""Compile and execute actual Mac instructions through both scratch lowerings.

Usage: python3 tools/test_generated_temporaries.py <instructions.sqlite>
      [--build-dir .local/current/build] [--sanitize]

Select one original record per normalized instruction pattern, preserving its
unaltered operations and addresses. Normalize only storage names and constant
categories, retaining widths, overlap relationships, and operation order. The
original export and emitted C++ stay outside Git in a temporary directory.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import shutil
import sqlite3
import subprocess
import tempfile

import compile_recomp
from local_temporaries import LocalTemporaries


def pattern(operations):
    groups = {}
    for space in ("register", "unique"):
        spans = sorted({(int(node[1]), int(node[1]) + node[2])
                        for op in operations for node in op[1:]
                        if node is not None and node[0] == space})
        merged = []
        for begin, end in spans:
            if merged and begin < merged[-1][1]:
                merged[-1][1] = max(merged[-1][1], end)
            else:
                merged.append([begin, end])
        groups[space] = merged
    names = {}
    def node_key(node):
        if node is None:
            return None
        space, offset, size = node
        offset = int(offset)
        if space in groups:
            group = next(i for i, (begin, end) in enumerate(groups[space])
                         if begin <= offset and offset + size <= end)
            identity = (space, group)
            if identity not in names:
                names[identity] = len(names)
            return space, names[identity], offset - groups[space][group][0], size
        if space == "const":
            # Preserve important shift/sign/mask boundaries; arbitrary immediate
            # and absolute branch values need not produce thousands of clones.
            value = offset & ((1 << (size * 8)) - 1)
            if value < 65 or value in (127, 128, 255, 256, 417, 32767, 32768, 65535,
                                     0x7fffffff, 0x80000000, 0xffffffff):
                category = value
            elif value and value & (value - 1) == 0:
                category = ("power", value.bit_length())
            elif value and (value + 1) & value == 0:
                category = ("mask", value.bit_length())
            else:
                category = "other"
            return space, category, size
        return space, size
    return tuple((op[0], *(node_key(node) for node in op[1:])) for op in operations)


def select_records(database):
    chosen, operations, rejected = {}, Counter(), Counter()
    count = 0
    with sqlite3.connect(database.resolve().as_uri() + "?mode=ro", uri=True) as connection:
        for (body,) in connection.execute("SELECT body FROM instructions ORDER BY address"):
            record = json.loads(body)
            count += 1
            ops = record["ops"]
            operations.update(op[0] for op in ops)
            try:
                LocalTemporaries.require_instruction_local_unique(ops)
            except ValueError as error:
                rejected[str(error)] += 1
                continue
            key = pattern(ops)
            if key not in chosen:
                # Unsupported original operations must not silently disappear
                # from differential coverage.
                for index, op in enumerate(ops):
                    compile_recomp.operation(op, record["address"], index, -1, set(), len(ops))
                chosen[key] = record
    if rejected:
        raise RuntimeError(f"Export includes unsafe local lifetimes: {dict(rejected)}")
    print(f"Selected {len(chosen)} normalized actual instruction patterns from {count} records; "
          f"{len(operations)} P-code operation kinds", flush=True)
    print("Operation coverage: " + ", ".join(sorted(operations)), flush=True)
    return list(chosen.values())


def instruction(record, index, local):
    address, ops = record["address"], record["ops"]
    lowering = LocalTemporaries.for_instruction(ops, compile_recomp.read, compile_recomp.write)
    if local and lowering is None:
        raise RuntimeError("Selected instruction lost its locality proof")
    callbacks = dict(read_node=lowering.read, write_node=lowering.write) if local else {}
    lines = [f"bool {'local' if local else 'original'}_{index}(State& s) {{",
             f"s.step(0x{address:x});"]
    for op_index, op in enumerate(ops):
        lines.append(compile_recomp.operation(op, address, op_index, -1, set(), len(ops), **callbacks))
    lines += [f"s.pc = 0x{record.get('fallthrough', address + record['size']):x};", "return false;", "}"]
    return "\n".join(lines)


HARNESS = r'''
#include "aot_runtime.hpp"
#include <algorithm>
#include <iostream>
#include <span>
#include <string_view>
using namespace cm::aot;
struct Register { unsigned offset, size; };
struct Case { bool (*original)(State&); bool (*local)(State&); std::uint32_t pc;
              std::span<const Register> registers; };
'''

DRIVER = r'''
constexpr std::uint64_t values[] = {
    0, 1, 2, 31, 32, 63, 64, 127, 128, 255, 256, 32767, 32768, 65535,
    0x7fffffff, 0x80000000, 0xffffffff, UINT64_C(0x7fffffffffffffff),
    UINT64_C(0x8000000000000000), UINT64_MAX, 0x30008000,
    UINT64_C(0x3ff0000000000000), UINT64_C(0xbff0000000000000),
    UINT64_C(0x7ff0000000000000), UINT64_C(0x7ff8000000000042),
    0x3f800000, 0xbf800000, 0x7f800000, 0x7fc00042
};
void reset(State& s, const Case& c, unsigned seed, unsigned mode, bool local) {
    for (std::size_t i = 0; i < s.registers.size(); ++i)
        s.registers[i] = std::uint8_t(i * 73u + seed * 19u);
    // Unequal poison proves no candidate read depends on discarded scratch.
    s.unique.fill(local ? 0x5a : 0xa5);
    unsigned operand = 0;
    for (const auto& r : c.registers) {
        const auto value = values[(seed + operand++) % std::size(values)];
        switch (r.size) {
        case 1: s.wr<1>(r.offset, value); break;
        case 2: s.wr<2>(r.offset, value); break;
        case 4: s.wr<4>(r.offset, value); break;
        case 8: s.wr<8>(r.offset, value); break;
        default:
            // Some deliberately unsupported intrinsics carry wide register
            // operands. Their original CALLOTHER path must still be tested for
            // the same exception; initialize these bytes without scalar reads.
            if (r.offset > s.registers.size() || r.size > s.registers.size() - r.offset)
                throw std::runtime_error("Test register lies outside architectural state");
            for (unsigned i = 0; i < r.size; ++i)
                s.registers[r.offset + i] = std::uint8_t(value >> (8 * (i % 8)));
            break;
        }
    }
    s.pc = c.pc;
    s.executed = mode == 2 ? UINT64_MAX : 17;
    s.budget = mode == 2 ? UINT64_MAX : mode == 1 ? 17 : 18;
    for (const auto& [address, size] : s.memory.mapped_ranges()) {
        std::vector<std::uint8_t> bytes(size);
        for (std::size_t i = 0; i < bytes.size(); ++i)
            bytes[i] = std::uint8_t(address + i * 13u + seed * 23u);
        s.memory.copy_in(address, bytes);
    }
}
std::string execute(State& s, bool (*fn)(State&)) {
    try { return fn(s) ? "transfer" : "fallthrough"; }
    catch (const std::exception& error) { return error.what(); }
}
void compare(const State& original, const State& local, const std::string& first,
             const std::string& second) {
    if (first != second || original.pc != local.pc || original.executed != local.executed ||
        original.budget != local.budget || original.registers != local.registers)
        throw std::runtime_error("Architectural register/PC/budget/exception mismatch: " + first + " / " + second);
    if (original.memory.mapped_ranges() != local.memory.mapped_ranges())
        throw std::runtime_error("Memory mapping mismatch");
    for (const auto& [address, size] : original.memory.mapped_ranges())
        if (original.memory.copy_out(address, size) != local.memory.copy_out(address, size))
            throw std::runtime_error("Mapped memory payload mismatch");
}
int main() {
    std::size_t completed = 0, faults = 0, successful = 0, transfers = 0;
    std::size_t current = 0;
    unsigned seed = 0, mode = 0, attempt = 0;
    try {
        for (; current < std::size(cases); ++current) {
            const auto& c = cases[current];
            auto original = std::make_unique<State>();
            auto local = std::make_unique<State>();
            for (seed = 0; seed < std::size(values); ++seed) {
                for (mode = 0; mode < 3; ++mode) {
                    // First run observes memory failures. Retry only after
                    // mapping the reported address identically in both isolated
                    // states, to exercise successful LOAD/STORE paths as well.
                    for (attempt = 0; attempt < 9; ++attempt) {
                        reset(*original, c, seed, mode, false);
                        reset(*local, c, seed, mode, true);
                        const auto first = execute(*original, c.original);
                        const auto second = execute(*local, c.local);
                        compare(*original, *local, first, second);
                        ++completed;
                        if (first == "transfer") { ++transfers; break; }
                        if (first == "fallthrough") { ++successful; break; }
                        constexpr std::string_view prefix = "Unmapped data access at ";
                        if (!first.starts_with(prefix)) break;
                        ++faults;
                        const auto address = std::stoull(first.substr(prefix.size()), nullptr, 16);
                        const auto begin = std::uint32_t(address & ~UINT64_C(15));
                        // A wrapping access remains an intentional failure.
                        if (attempt == 8 || (begin == 0xfffffff0u && address > 0xfffffff8u)) break;
                        original->memory.map(begin, 16);
                        local->memory.map(begin, 16);
                    }
                }
            }
        }
        std::cout << completed << " differential executions across " << std::size(cases)
                  << " actual instruction patterns; " << successful << " normal completions, "
                  << transfers << " transfers, " << faults << " matched memory failures\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Pattern " << current << ", original PC 0x" << std::hex << cases[current].pc
                  << std::dec << ", seed " << seed << ", mode " << mode << ", attempt " << attempt
                  << ": " << error.what() << '\n';
        return 1;
    }
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path)
    parser.add_argument("--build-dir", type=Path, default=Path(".local/current/build"))
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    records = select_records(args.database)
    compiler = shutil.which("clang++") or shutil.which("c++")
    if compiler is None:
        raise RuntimeError("A C++ compiler is required")
    root = Path(__file__).resolve().parent.parent
    build = args.build_dir.resolve()
    with tempfile.TemporaryDirectory(prefix="cm-actual-temporaries-") as temporary:
        directory = Path(temporary)
        emitted = [HARNESS]
        for index, record in enumerate(records):
            emitted += [instruction(record, index, False), instruction(record, index, True)]
        register_tables = []
        for index, record in enumerate(records):
            registers = dict.fromkeys((int(node[1]), node[2]) for op in record["ops"]
                                      for node in op[1:] if node is not None and node[0] == "register")
            nodes = ",".join(f"{{{offset},{size}}}" for offset, size in registers)
            if registers:
                emitted.append(f"constexpr Register registers_{index}[] = {{{nodes}}};")
                register_tables.append(f"registers_{index}")
            else:
                register_tables.append("{}")
        emitted.append("const Case cases[] = {")
        for index, record in enumerate(records):
            emitted.append(f"{{original_{index}, local_{index}, 0x{record['address']:x}, {register_tables[index]}}},")
        emitted += ["};", DRIVER]
        source = directory / "actual.cpp"
        source.write_text("\n".join(emitted))
        binary = directory / "actual"
        command = [compiler, "-std=c++23", "-O2", "-ffp-contract=off", "-Wall", "-Wextra", "-Werror",
                   "-I", str(root / "native"), str(source), "-o", str(binary)]
        if args.sanitize:
            command += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
        command += [str(build / name) for name in ("libcm_aot.a", "libcm_ui.a", "libcm_audio.a")]
        command += ["-lz"]
        for framework in ("AppKit", "Carbon", "AVFAudio", "Foundation", "CoreFoundation",
                          "CoreServices", "ApplicationServices"):
            command += ["-framework", framework]
        print(f"Compiling {len(records) * 2} original/local functions", flush=True)
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
