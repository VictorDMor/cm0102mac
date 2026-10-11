#include "stdlib_core_fixtures.hpp"
#include <numeric>
#include <random>

namespace {
using namespace cm::stdlib_core_checks;
void absolute_values() {
    std::mt19937 random(0x23ab501);
    for (unsigned index : {0u, 1u}) {
        auto s = fixture_state(index);
        const auto exercise = [&](std::uint32_t value, unsigned seed, bool negative = false) {
            s->wr<4>(12, value); s->wr<1>(1024, seed & 0xff); s->wr<1>(1027, (seed >> 8) & 0xff);
            s->wr<1>(2304, (seed >> 16) & 0xff);
            check(index, *s, hex_address(value), [value](const State& out, const Comparator& c) {
                const auto sign = value >> 31;
                require(out.r<4>(12) == (sign ? 0u - value : value), "Independent word absolute result including INT_MIN");
                require(out.r<1>(1027) == unsigned(sign && (value & 0x7fffffff)), "Independent arithmetic-shift carry");
                require(out.pc == caller && c.arguments.empty(), "Absolute leaf return");
            }, {}, {}, negative);
        };
        exercise(0xfffffff9, 0xa7b3c1, true);
        for (auto value : {0u, 1u, 2u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xfffffffeu, 0xffffffffu})
            for (unsigned seed : {0u, 0xffffffu, 0xabcdefu, 0x123456u}) exercise(value, seed);
        for (unsigned n = 0; n < 512; ++n) exercise(random(), random());
    }
}
void remainder_values() {
    auto s = fixture_state(2); std::mt19937 random(0x23f1002);
    check(2, *s, "independent negative division", [](const State& out, const Comparator&) {
        require(out.r<4>(12) == 2, "Floor remainder differs from truncating remainder for -7/3");
    }, {}, {}, true);
    for (std::int32_t a = -17; a <= 17; ++a) for (std::int32_t b = -9; b <= 9; ++b) {
        s->wr<4>(12, std::uint32_t(a)); s->wr<4>(16, std::uint32_t(b));
        s->wr<1>(1024, random()); s->wr<1>(2304, random());
        const auto expected = b ? a % b + ((a % b != 0 && ((a < 0) != (b < 0))) ? b : 0) : 0;
        check(2, *s, std::to_string(a) + "/" + std::to_string(b),
            [expected](const State& out, const Comparator&) { require(out.r<4>(12) == std::uint32_t(expected), "Independent floor-modulo result"); },
            {}, b ? "" : "Signed division by zero in translated code");
    }
    constexpr std::uint32_t edges[]{0u, 1u, 2u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xfffffffeu, 0xffffffffu};
    for (auto a : edges) for (auto b : edges) {
        s->wr<4>(12, a); s->wr<4>(16, b);
        check(2, *s, "extreme-" + hex_address(a) + "/" + hex_address(b), {}, {}, b ? "" : "Signed division by zero in translated code");
    }
    for (unsigned n = 0; n < 512; ++n) {
        s->wr<4>(12, random()); s->wr<4>(16, random()); s->wr<1>(1024, random());
        check(2, *s, "random-" + std::to_string(n));
    }
}
Expectation result(std::uint32_t address, unsigned callbacks = 0) {
    return [address, callbacks](const State& s, const Comparator& c) {
        require(s.r<4>(12) == address && s.pc == caller && s.r<4>(4) == stack, "Independent search result and ABI return");
        if (callbacks) require(c.arguments.size() == callbacks, "Independent callback count");
        for (const auto& args : c.arguments) require(args[0] == key && args[2] == comparator_toc && args[3] == stack - 96,
            "Independent comparator key, descriptor TOC and frame");
        if (!c.arguments.empty()) require(c.arguments.front()[1] == table && c.arguments.front()[4] == 0x1077565c,
            "Element zero is always the first comparator probe");
    };
}
void ordinary_search() {
    for (unsigned argument : {3u, 4u, 5u, 6u, 7u}) {
        auto s = fixture_state(3); s->wr<4>(argument * 4, 0);
        check(3, *s, "null-argument-" + std::to_string(argument), [](const State& out, const Comparator& c) {
            require(out.r<4>(12) == 0 && out.r<4>(4) == stack && c.arguments.empty(), "Null guard before comparator access");
        });
        fault_matrix(*s, "null-argument-" + std::to_string(argument));
    }
    auto s = fixture_state(3);
    check(3, *s, "root and own negative", result(table + 20), {}, {}, true);
    fault_matrix(*s, "found search");
    constexpr std::int32_t values[]{-17, -10, -3, 0, 1, 7, 11, 11, 28};
    for (unsigned width : {4u, 5u, 12u, 64u}) for (unsigned count : {1u, 2u, 3u, 8u, 9u})
        for (std::int32_t needle = -19; needle <= 30; ++needle) {
            s = fixture_state(3); s->wr<4>(20, count); s->wr<4>(24, width);
            for (unsigned n = 0; n < count; ++n) s->memory.write<4>(table + n * width, std::uint32_t(values[n]));
            s->memory.write<4>(key, std::uint32_t(needle));
            const bool present = std::find(values, values + count, needle) != values + count;
            check(3, *s, "width=" + std::to_string(width) + "/count=" + std::to_string(count) + "/key=" + std::to_string(needle),
                [present, width, count, needle](const State& out, const Comparator& c) {
                    const auto found = std::uint32_t(out.r<4>(12));
                    require(!present ? found == 0 : found >= table && found < table + count * width && (found - table) % width == 0 &&
                        out.memory.read<4>(found) == std::uint32_t(needle), "Independent membership and returned element value");
                    require(!c.arguments.empty() && c.arguments.front()[1] == table, "Independent initial probe");
                });
        }
    s = fixture_state(3); s->memory.write<4>(key, std::uint32_t(-17));
    check(3, *s, "first equality", result(table, 1));
    s->memory.write<4>(key, std::uint32_t(-18)); check(3, *s, "below first", result(0, 1));
    s = fixture_state(3); s->memory.write<4>(key, 11);
    check(3, *s, "duplicate midpoint", result(table + 24, 3));
}
void live_search() {
    auto s = fixture_state(3);
    check(3, *s, "live first arguments", [](const State& out, const Comparator& c) {
        require(out.r<4>(12) == table && c.arguments.size() == 1 && c.arguments[0][0] == key + 4 && c.arguments[0][1] == table + 4,
            "First callback uses current arguments without restoring saved key/base");
    }, [](Monitor& m, Comparator& c) {
        m.on_write = [](State& out, std::uint32_t address, unsigned) {
            if (address == stack - 96) { out.wr<4>(12, key + 4); out.wr<4>(16, table + 4); }
        };
        c.body = [](State& out, unsigned) { out.wr<4>(12, 0); };
    });
    s->wr<4>(12, 0);
    check(3, *s, "live condition after frame", {}, [](Monitor& m, Comparator& c) {
        m.on_write = [](State& out, std::uint32_t address, unsigned) { if (address == stack - 96) out.wr<1>(2304, 4); };
        c.body = [](State& out, unsigned) { out.wr<4>(12, 0); };
    });
    s = fixture_state(3);
    check(3, *s, "live save and restore address cursor", {}, [](Monitor& m, Comparator&) {
        m.on_write = [](State& out, std::uint32_t address, unsigned) {
            if (address >= stack - 32 && address < stack) out.wr<4>(2432, out.r<4>(2432) + 1);
        };
        m.effects.on_read = [](State& out, Effects&, std::uint32_t address, unsigned) {
            if (address >= stack - 32 && address < stack) out.wr<4>(2432, out.r<4>(2432) + 1);
        };
    });
    check(3, *s, "live next saved register", {}, [](Monitor& m, Comparator&) {
        m.on_write = [](State& out, std::uint32_t address, unsigned) { if (address == stack - 32) out.wr<4>(100, 0xabcdef01); };
    });
    check(3, *s, "mutating comparator descriptor", [](const State& out, const Comparator& c) {
        require(out.r<4>(12) == table + 20 && c.arguments.size() > 1, "Descriptor mutation still finds requested value");
        for (unsigned n = 0; n < c.arguments.size(); ++n) require(c.arguments[n][2] == comparator_toc + (n ? 256u : 0u),
            "Each comparator call loads the live descriptor TOC");
    }, [](Monitor&, Comparator& c) {
        c.body = [](State& out, unsigned ordinal) {
            const auto a = std::bit_cast<std::int32_t>(std::uint32_t(out.memory.read<4>(out.r<4>(12))));
            const auto b = std::bit_cast<std::int32_t>(std::uint32_t(out.memory.read<4>(out.r<4>(16))));
            out.wr<4>(12, a < b ? std::uint32_t(-99) : a > b ? 42 : 0);
            if (ordinal == 1) out.memory.write<4>(descriptor + 4, comparator_toc + 256);
            out.wr<4>(8, 0xdeadbeef); out.wr<1>(2304, 0xf7); out.wr<1>(1024, 0xfe);
        };
    });
    for (unsigned n = 24; n < 32; ++n) check(3, *s, "comparator changes saved register " + std::to_string(n), {},
        [n](Monitor&, Comparator& c) {
            c.body = [n](State& out, unsigned ordinal) {
                if (ordinal == 1) out.wr<4>(n * 4, n == 29 ? 0 : 0xfffffff0);
                out.wr<4>(12, ordinal == 1 ? 1 : 0);
            };
        }, n == 27 ? "Unmapped data access at " : "");
    check(3, *s, "midpoint sum wraps", [](const State& out, const Comparator& c) {
        require(out.r<4>(12) == table && c.arguments.size() == 2 && c.arguments[1][1] == table,
            "Word32 midpoint sum wraps to zero after a live count clobber");
    }, [](Monitor&, Comparator& c) {
        c.body = [](State& out, unsigned ordinal) {
            if (ordinal == 1) out.wr<4>(116, 0);
            out.wr<4>(12, ordinal == 1 ? 1 : 0);
        };
    });
    check(3, *s, "nonvolatile comparator clobber", {}, [](Monitor&, Comparator& c) {
        c.body = [](State& out, unsigned ordinal) {
            if (ordinal == 1) { out.wr<4>(112, table + 44); out.wr<4>(12, 0); }
        };
    });
    for (unsigned ordinal : {1u, 2u, 3u, 4u}) {
        auto absent = fixture_state(3); absent->memory.write<4>(key, 100);
        check(3, *absent, "callback fault " + std::to_string(ordinal), {}, [ordinal](Monitor&, Comparator& c) {
            c.body = [ordinal](State& out, unsigned call) {
                out.wr<4>(12, 1); if (call == ordinal) throw std::runtime_error("Fixture comparator fault");
            };
        }, "Fixture comparator fault");
    }
    s = fixture_state(3); s->memory.write<4>(descriptor, 0x1badc000);
    check(3, *s, "uncompiled comparator", {}, {}, "Uncompiled original address: ");
}
void minimal_and_wrapped_search() {
    auto s = fixture_state(3); const auto evidence = check(3, *s, "minimal footprint inventory");
    std::set<std::uint32_t> bytes;
    for (const auto& access : evidence.log) for (unsigned n = 0; n < access.size; ++n) bytes.insert(access.address + n);
    auto tight = std::make_unique<State>(); tight->registers = s->registers; tight->unique = s->unique;
    tight->pc = s->pc; tight->executed = s->executed; tight->budget = s->budget; tight->native_calls = s->native_calls;
    tight->allocated = s->allocated; tight->available = s->available; tight->heap_end = s->heap_end; tight->memory_error = s->memory_error;
    for (auto it = bytes.begin(); it != bytes.end();) {
        const auto start = *it; auto end = std::uint64_t(start);
        while (it != bytes.end() && *it == end) { ++end; ++it; }
        tight->memory.map(start, end - start); tight->memory.copy_in(start, s->memory.copy_out(start, end - start));
    }
    check(3, *tight, "only bytes actually accessed", result(table + 20));
    for (unsigned shift : {1u, 2u, 3u}) {
        s = fixture_state(3); s->wr<4>(4, stack + shift); s->wr<4>(28, descriptor + shift);
        s->memory.write<4>(descriptor + shift, comparator_import); s->memory.write<4>(descriptor + shift + 4, comparator_toc);
        check(3, *s, "unaligned frame and descriptor " + std::to_string(shift));
    }
    for (std::uint32_t width : {0x80000000u, 0xfffffffcu, 0xffffffffu}) {
        s = fixture_state(3); s->wr<4>(20, 5); s->wr<4>(24, width);
        check(3, *s, "word32 width multiplication " + hex_address(width), [width](const State& out, const Comparator& c) {
            require(c.arguments.size() == 2 && c.arguments[1][1] == table + width * 2 && out.r<4>(12) == table + width * 2,
                "Word32 product and guest address wrap");
        }, [](Monitor&, Comparator& c) { c.body = [](State& out, unsigned ordinal) { out.wr<4>(12, ordinal == 1 ? 1 : 0); }; });
    }
    s = fixture_state(3); s->wr<4>(28, stack - 32);
    check(3, *s, "descriptor aliases original register save", {}, {}, "Uncompiled original address: ");
}
void accounting() {
    for (std::int32_t value : {-18, -17, -3, 7, 11, 28, 29}) {
        auto s = fixture_state(3); s->memory.write<4>(key, std::uint32_t(value));
        const auto evidence = check(3, *s, "accounting inventory " + std::to_string(value), {}, {}, {}, false, true);
        for (std::uint64_t available = 0; available < evidence.original_steps; ++available) {
            s->budget = s->executed + available;
            check(3, *s, "budget=" + std::to_string(available) + "/key=" + std::to_string(value), {}, {},
                "Native instruction budget exhausted", false, true);
        }
        s->budget = s->executed + evidence.original_steps;
        check(3, *s, "exact complete budget " + std::to_string(value), {}, {}, {}, false, true);
    }
    auto malformed = fixture_state(3); malformed->budget = malformed->executed + 200;
    check(3, *malformed, "nonprogress after comparator count mutation", {}, [](Monitor&, Comparator& c) {
        c.body = [](State& out, unsigned ordinal) { if (ordinal == 1) out.wr<4>(116, 0); out.wr<4>(12, 1); };
    }, "Native instruction budget exhausted", false, true);
}
}
int main() {
    try {
        absolute_values(); remainder_values(); ordinary_search(); live_search(); minimal_and_wrapped_search(); accounting();
        for (unsigned n = 0; n < entries.size(); ++n) {
            require(comparisons[n] > 0 && negatives[n] == 1, "Missing individual stdlib-core evidence");
            std::cout << hex_address(entries[n]) << ": " << comparisons[n] << " comparisons, " << faults[n]
                << " fault/budget outcomes, " << negatives[n] << " rejected negative\n";
        }
        require(budget_stops > 0 && continuations > 0, "Missing bounded search continuation evidence");
        std::cout << std::accumulate(comparisons.begin(), comparisons.end(), 0u) << " component comparisons; "
            << accounting_cases << " exact-accounting cases; " << budget_stops << " exact budget stops; "
            << continuations << " diagnostic original continuations\n"
            << "Components only: production hooks and installed-runtime coverage are not established here.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
