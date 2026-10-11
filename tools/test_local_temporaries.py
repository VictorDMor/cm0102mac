"""Validate local scratch lowering, optionally auditing a complete Mac export.

Usage: python3 tools/test_local_temporaries.py [instructions.sqlite]
The differential C++ fixture uses the existing operation emitter and compares
registers, memory, and exception state against its State.unique-based lowering.
"""
from copy import deepcopy
import json
from pathlib import Path
import random
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import unittest

import compile_recomp
from local_temporaries import LocalTemporaries


def unique(offset, size):
    return ["unique", str(offset), size]


def register(offset, size):
    return ["register", str(offset), size]


def constant(value, size=8):
    return ["const", str(value), size]


class LocalTemporariesTests(unittest.TestCase):
    database = None

    def local(self, operations):
        return LocalTemporaries.for_instruction(operations, compile_recomp.read,
                                                compile_recomp.write)

    def test_scalar_narrowing_and_unsigned_expression_types(self):
        operations = [["COPY", unique(0, 1), constant(0x1234)],
                      ["COPY", register(0, 1), unique(0, 1)]]
        local = self.local(operations)
        self.assertIsNotNone(local)
        self.assertIn("const std::uint8_t", local.write(unique(0, 1), "UINT64_C(4660)"))
        self.assertEqual(local.read(unique(0, 1)), "std::uint64_t(local_unique_0)")
        self.assertEqual(local.read(register(4, 4)), "s.r<4>(4)")
        self.assertEqual(local.write(register(4, 4), "value"), "s.wr<4>(4, value);")

    def test_big_endian_alias_ownership(self):
        ops = [["COPY", unique(0, 8), constant(0)],
               ["COPY", unique(2, 2), constant(0)],
               ["COPY", register(0, 8), unique(0, 8)]]
        local = self.local(ops)
        local.write(unique(0, 8), "UINT64_C(123)")
        local.write(unique(2, 2), "UINT64_C(456)")
        self.assertIn("local_unique_0", local.read(unique(0, 8)))
        self.assertIn("local_unique_1", local.read(unique(0, 8)))
        self.assertIn(">> 56", local.read(unique(0, 1)))
        self.assertEqual(local.read(unique(2, 2)), "std::uint64_t(local_unique_1)")

    def test_mutations_reject_missing_partial_and_invalid_definitions(self):
        valid = [["COPY", unique(20, 8), constant(0)],
                 ["COPY", register(0, 8), unique(20, 8)]]
        self.assertIsNotNone(self.local(valid))
        candidates = [valid[1:], valid[::-1]]
        for bad in (unique(19, 8), unique(21, 8), unique(-1, 1),
                    unique(LocalTemporaries.unique_size, 1), unique(20, 3)):
            for op_index in (0, 1):
                mutation = deepcopy(valid)
                mutation[op_index][1 if op_index == 0 else 2] = bad
                candidates.append(mutation)
        candidates += [valid + [[branch, None, constant(1), constant(1, 1)]]
                       for branch in ("BRANCH", "CBRANCH", "CALL")]
        for operations in candidates:
            with self.subTest(operations=operations):
                self.assertIsNone(self.local(operations))
                with self.assertRaises(ValueError):
                    LocalTemporaries.require_instruction_local_unique(operations)

    def test_recovered_vector_context_address_scratch_is_in_bounds(self):
        ops = [["COPY", unique(0x44b00, 4), constant(0)],
               ["COPY", register(12, 4), unique(0x44b00, 4)]]
        self.assertIsNotNone(self.local(ops))
        LocalTemporaries.require_instruction_local_unique(ops)

    def test_cpp_differential_aliases_evaluation_and_exceptions(self):
        compiler = shutil.which("clang++") or shutil.which("c++")
        self.assertIsNotNone(compiler, "A C++ compiler is required for semantic validation")
        fixtures = []
        # Captured register and memory values must not be lazily reread after
        # intervening writes. Narrowing and overlapping writes must be exact.
        fixtures.append([
            ["COPY", unique(0, 4), register(0, 4)],
            ["COPY", register(0, 4), constant(999)],
            ["COPY", register(4, 4), unique(0, 4)],
            ["LOAD", unique(8, 8), constant(417), constant(16)],
            ["STORE", None, constant(417), constant(16), constant(999)],
            ["COPY", register(8, 8), unique(8, 8)],
            ["COPY", unique(10, 2), constant(0x12345678)],
            ["COPY", register(16, 8), unique(8, 8)],
            ["LOAD", unique(0, 8), constant(417), constant(4095)],
        ])
        rng = random.Random(0x10773870)
        for _ in range(64):
            operations = [["COPY", unique(offset, 8), constant(rng.getrandbits(64))]
                          for offset in range(0, 64, 8)]
            for _ in range(64):
                size = rng.choice((1, 2, 4, 8))
                def operand():
                    return rng.choice((unique(rng.randrange(65 - size), size),
                                       register(rng.randrange(65 - size), size),
                                       constant(rng.getrandbits(size * 8), size)))
                output = rng.choice((unique(rng.randrange(65 - size), size),
                                     register(rng.randrange(65 - size), size)))
                name = rng.choice(("COPY", "INT_ADD", "INT_SUB", "INT_MULT", "INT_XOR",
                                   "INT_OR", "INT_AND", "INT_LEFT", "INT_RIGHT", "INT_CARRY"))
                inputs = [operand()] if name == "COPY" else [operand(), operand()]
                operations.append([name, output, *inputs])
            operations += [["COPY", register(offset, 8), unique(offset, 8)]
                           for offset in range(0, 64, 8)]
            fixtures.append(operations)

        emitted = []
        for index, operations in enumerate(fixtures):
            for optimized in (False, True):
                local = self.local(operations) if optimized else None
                statements = []
                for pos, op in enumerate(operations):
                    callbacks = {"read_node": local.read, "write_node": local.write} if local else {}
                    statements.append(compile_recomp.operation(
                        op, 0x10000000, pos, 0x10000, set(), len(operations), **callbacks))
                if optimized:
                    self.assertIsNotNone(local)
                    self.assertIn("local_unique_", "\n".join(statements))
                    self.assertNotIn("s.u<", "\n".join(statements))
                    self.assertNotIn("s.wu<", "\n".join(statements))
                emitted.append(f"void fixture_{index}_{int(optimized)}(Fixture& s) {{\n" +
                               "\n".join(statements) + "\n}")
        root = Path(__file__).resolve().parents[1]
        source = r'''
#include "aot_runtime.hpp"
#include <iostream>
using namespace cm::aot;
struct FixtureMemory {
    std::array<std::uint8_t, 4096> bytes{};
    template<unsigned N> auto read(std::uint64_t p) const {
        if (p > bytes.size() - N) throw std::runtime_error("memory bounds");
        return read_be<N>(bytes.data() + p);
    }
    template<unsigned N> void write(std::uint64_t p, std::uint64_t value) {
        if (p > bytes.size() - N) throw std::runtime_error("memory bounds");
        write_be<N>(bytes.data() + p, value);
    }
};
struct Fixture {
    std::array<std::uint8_t, 128> registers{};
    std::array<std::uint8_t, 0x45000> unique{};
    FixtureMemory memory;
    template<unsigned N> auto r(std::size_t p) const { return read_be<N>(registers.data() + p); }
    template<unsigned N> auto u(std::size_t p) const { return read_be<N>(unique.data() + p); }
    template<unsigned N> void wr(std::size_t p, std::uint64_t v) { write_be<N>(registers.data() + p, v); }
    template<unsigned N> void wu(std::size_t p, std::uint64_t v) { write_be<N>(unique.data() + p, v); }
};
void compare(void (*original)(Fixture&), void (*local)(Fixture&), unsigned seed) {
    Fixture a;
    for (unsigned i = 0; i < a.registers.size(); ++i) a.registers[i] = (i * 37) ^ seed;
    for (unsigned i = 0; i < a.memory.bytes.size(); ++i) a.memory.bytes[i] = (i * 13) ^ seed;
    a.unique.fill(0xa5);
    Fixture b = a;
    std::string error_a, error_b;
    try { original(a); } catch (const std::exception& e) { error_a = e.what(); }
    try { local(b); } catch (const std::exception& e) { error_b = e.what(); }
    if (a.registers != b.registers || a.memory.bytes != b.memory.bytes || error_a != error_b)
        throw std::runtime_error("Local temporary architectural state or exception mismatch");
}
'''
        source += "\n".join(emitted)
        source += "\nint main() {\n"
        for index in range(len(fixtures)):
            source += (f"for (unsigned seed = 0; seed < 32; ++seed) "
                       f"compare(fixture_{index}_0, fixture_{index}_1, seed);\n")
        source += f'std::cout << "{len(fixtures) * 32} local temporary differential fixtures passed\\n";\n}}\n'
        with tempfile.TemporaryDirectory(prefix="cm-local-temporaries-") as directory:
            path = Path(directory)
            (path / "checks.cpp").write_text(source)
            build = subprocess.run([compiler, "-std=c++23", "-O2", "-Wall", "-Wextra", "-Werror",
                                    "-Wpedantic", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                                    "-I", str(root / "native"), str(path / "checks.cpp"),
                                    "-o", str(path / "checks")], capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            result = subprocess.run([str(path / "checks")], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            print(result.stdout.strip())

    def test_original_export_coverage(self):
        if self.database is None:
            self.skipTest("Pass the local instructions.sqlite to audit all original instructions")
        counts = [0, 0]
        with sqlite3.connect(self.database.as_uri() + "?mode=ro", uri=True) as connection:
            for body, in connection.execute("SELECT body FROM instructions"):
                record = json.loads(body)
                try:
                    LocalTemporaries.require_instruction_local_unique(record["ops"])
                except ValueError as error:
                    self.fail(f"Original instruction {record['address']:x}: {error}")
                counts[0] += 1
                counts[1] += len(record["ops"])
        print(f"{counts[0]} original instructions / {counts[1]} P-code operations proved local")


if __name__ == "__main__":
    if len(sys.argv) > 1 and not sys.argv[1].startswith("-"):
        LocalTemporariesTests.database = Path(sys.argv.pop(1)).resolve()
    unittest.main()
