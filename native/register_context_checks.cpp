#include "register_context_fixtures.hpp"
#include <numeric>
#include <random>

namespace {
using namespace cm::register_context_checks;
std::unique_ptr<State> minimal(const State& initial, unsigned index) {
    auto s = std::make_unique<State>();
    s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->executed = initial.executed; s->budget = initial.budget; s->native_calls = initial.native_calls;
    s->allocated = initial.allocated; s->available = initial.available;
    s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    const auto copy = [&](std::uint32_t address, unsigned bytes) {
        s->memory.map(address, bytes); s->memory.copy_in(address, initial.memory.copy_out(address, bytes));
    };
    if (index < 28) copy(std::uint32_t(s->r<4>(4)) - (32 - entries[index].first) * 8, (32 - entries[index].first) * 8);
    else { copy(buffer, 16); copy(buffer + 20, 228); }
    return s;
}
Expectation expected(const State& before, unsigned index) {
    const auto registers = before.registers;
    return [registers, index](const State& s) {
        const auto reg = [&](unsigned n) { return std::uint32_t(read_be<4>(registers.data() + n * 4)); };
        const auto fp = [&](unsigned n) { return read_be<8>(registers.data() + 0x4000 + n * 16); };
        if (index < 14) {
            for (unsigned n = entries[index].first; n < 32; ++n)
                require(s.memory.read<8>(reg(1) - (32 - n) * 8) == fp(n), "Independent floating save footprint");
        } else if (index < 28) {
            for (unsigned n = entries[index].first; n < 32; ++n)
                require(s.r<8>(0x4000 + n * 16) == UINT64_C(0x7ff0445566770000) + n, "Independent floating restore bits");
        } else if (index == 28) {
            require(s.r<4>(12) == 0 && s.r<4>(20) == incoming_link && s.r<4>(24) == 0x01234567,
                "Independent capture return and saved scratch");
            require(s.memory.read<4>(buffer) == incoming_link && s.memory.read<4>(buffer + 4) == 0x01234567 &&
                s.memory.read<4>(buffer + 8) == reg(1) && s.memory.read<4>(buffer + 12) == reg(2), "Independent capture header");
            require(s.memory.read<4>(buffer + 16) == 0xa7a7a7a7, "Capture keeps original context hole untouched");
            for (unsigned n = 13; n < 32; ++n) require(s.memory.read<4>(buffer + 20 + (n - 13) * 4) == reg(n), "Independent general register capture");
            for (unsigned n = 14; n < 32; ++n) require(s.memory.read<8>(buffer + 96 + (n - 14) * 8) == fp(n), "Independent floating register capture");
            require(s.r<8>(0x4000) == 0x55555555 && s.memory.read<8>(buffer + 240) == 0x55555555,
                "Capture publishes normalized low-bit status into f0 and context");
        } else {
            require(s.pc == restored_link && s.r<4>(4) == restored_stack && s.r<4>(8) == restored_toc &&
                s.r<4>(12) == (reg(4) ? reg(4) : 1) && s.r<4>(20) == restored_link && s.r<4>(24) == 0x12345678,
                "Independent context destination, stack, TOC and nonzero return");
            for (unsigned n = 13; n < 32; ++n) require(s.r<4>(n * 4) == 0xcafe0000 + n, "Independent general register restore");
            for (unsigned n = 14; n < 32; ++n) require(s.r<8>(0x4000 + n * 16) == UINT64_C(0xff80112233440000) + n, "Independent floating register restore");
            require(s.r<8>(0x4000) == UINT64_C(0xdeadbeefff8100f3), "Restore retains complete raw status scratch");
            for (unsigned n = 0; n < 32; ++n) require(s.r<1>(1280 + n) == ((0xff8100f3u >> (31 - n)) & 1), "Independent floating status restore");
            for (unsigned n = 1; n < 8; ++n) require(s.r<1>(2304 + n) == n + 1, "Independent saved condition fields");
        }
        require(s.r<4>(2432) == (index >= 28 ? buffer + 96 : read_be<4>(registers.data() + 2432)), "Original register-address residue");
    };
}
void roots_and_faults() {
    std::mt19937_64 random(0x23c0ffee);
    for (unsigned index = 0; index < entries.size(); ++index) {
        auto s = fixture_state(index); check(index, *s, "independent root and own negative", false, true, {}, expected(*s, index));
        fault_matrix(index, *s);
        auto tight = minimal(*s, index); check(index, *tight, "minimal mappings and unmapped context hole");
        if (index < 28) s->wr<4>(4, stack + 3); else s->wr<4>(12, buffer + 3);
        check(index, *s, "unaligned addresses");
        for (unsigned iteration = 0; iteration < 64; ++iteration) {
            s = fixture_state(index);
            for (unsigned n = 0; n < 32; ++n) {
                s->wr<8>(0x4000 + n * 16, random()); s->wr<1>(1280 + n, random() & 0xff);
            }
            for (unsigned n = 0; n < 8; ++n) s->wr<1>(2304 + n, random() & 0xff);
            s->wr<1>(1024, random() & 0xff); s->wr<1>(1027, random() & 0xff);
            for (unsigned n = 13; n < 32; ++n) {
                s->wr<4>(n * 4, random()); s->memory.write<4>(buffer + 20 + (n - 13) * 4, random());
            }
            for (unsigned n = 14; n < 32; ++n) {
                s->memory.write<8>(buffer + 96 + (n - 14) * 8, random());
                s->memory.write<8>(stack - (32 - n) * 8, random());
            }
            s->memory.write<4>(buffer + 4, random()); s->memory.write<8>(buffer + 240, random());
            s->wr<4>(16, iteration & 1 ? std::uint32_t(random()) : 0);
            check(index, *s, "random-" + std::to_string(iteration));
        }
    }
    for (auto value : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
        auto s = fixture_state(29); s->wr<4>(16, value);
        check(29, *s, "literal jump argument " + hex_address(value), false, false, {}, expected(*s, 29));
    }
}
void live_effects() {
    for (unsigned index = 0; index < 28; ++index) {
        auto s = fixture_state(index);
        check(index, *s, "live stack address", false, false, [index](Monitor& m) {
            if (index < 14) m.on_write = [](State& state, std::uint32_t, unsigned) { state.wr<4>(4, state.r<4>(4) + 1); };
            else m.effects.on_read = [](State& state, Effects&, std::uint32_t, unsigned) { state.wr<4>(4, state.r<4>(4) + 1); };
        });
        if (entries[index].first < 31 && index >= 14) check(index, *s, "live next floating slot", false, false, [index](Monitor& m) {
            m.effects.on_read = [index](State& state, Effects& effects, std::uint32_t, unsigned) {
                if (!effects.mutated) { effects.mutated = true;
                    state.memory.write<8>(stack - (31 - entries[index].first) * 8, 0x0123456789abcdef); }
            };
        });
    }
    auto capture = fixture_state(28);
    check(28, *capture, "live general register source", false, false, [](Monitor& m) {
        m.on_write = [](State& s, std::uint32_t address, unsigned) {
            if (address == buffer + 20) s.wr<4>(56, 0xabcdef01);
        };
    });
    check(28, *capture, "live context base", false, false, [](Monitor& m) {
        m.on_write = [](State& s, std::uint32_t address, unsigned) {
            if (address == buffer) s.wr<4>(12, buffer + 256);
        };
    });
    check(28, *capture, "live floating status capture", false, false, [](Monitor& m) {
        m.on_write = [](State& s, std::uint32_t address, unsigned) {
            if (address == buffer + 8) s.wr<1>(1280, 0xff);
        };
    });
    check(28, *capture, "live f0 publication", false, false, [](Monitor& m) {
        m.on_write = [](State& s, std::uint32_t address, unsigned) {
            if (address == buffer + 232) s.wr<8>(0x4000, UINT64_C(0x1122334455667788));
        };
    });
    auto jump = fixture_state(29);
    check(29, *jump, "live final floating slot after status load", false, false, [](Monitor& m) {
        m.effects.on_read = [](State& s, Effects&, std::uint32_t address, unsigned) {
            if (address == buffer + 240) s.memory.write<8>(buffer + 232, UINT64_C(0x0123456789abcdef));
        };
    });
    check(29, *jump, "live f0 before status publication", false, false, [](Monitor& m) {
        m.effects.on_read = [](State& s, Effects&, std::uint32_t address, unsigned) {
            if (address == buffer + 232) s.wr<8>(0x4000, UINT64_C(0x1122334455667788));
        };
    });
    check(29, *jump, "live jump argument", false, false, [](Monitor& m) {
        m.effects.on_read = [](State& s, Effects&, std::uint32_t address, unsigned) {
            if (address == buffer + 232) s.wr<4>(16, 0x80000000);
        };
    });
}
void wrapped_contexts() {
    for (unsigned index : {28u, 29u}) {
        auto s = fixture_state(index); constexpr auto wrapped = 0xffffff80u;
        const auto bytes = s->memory.copy_out(buffer, 248);
        s->memory.map(wrapped, 128); s->memory.map(0, 120);
        s->memory.copy_in(wrapped, {bytes.begin(), bytes.begin() + 128});
        s->memory.copy_in(0, {bytes.begin() + 128, bytes.end()}); s->wr<4>(12, wrapped);
        check(index, *s, "word32 context address wrap");
        fault_matrix(index, *s);
    }
}
void compositions() {
    for (unsigned first = 18; first < 32; ++first) {
        auto a = fixture_state(first - 18), b = clone(*a); const auto before = a->registers;
        require(run(*a, first - 18, true).empty() && run(*b, first - 18, false).empty(), "Save composition root");
        for (unsigned n = first; n < 32; ++n) { a->wr<8>(0x4000 + n * 16, 0); b->wr<8>(0x4000 + n * 16, 0); }
        a->pc = b->pc = entries[first - 4].address;
        require(run(*a, first - 4, true).empty() && run(*b, first - 4, false).empty(), "Restore composition root");
        Image(*a).compare(*b, "Composed floating suffix");
        for (unsigned n = first; n < 32; ++n) require(b->r<8>(0x4000 + n * 16) == read_be<8>(before.data() + 0x4000 + n * 16), "Independent suffix round trip");
        ++comparisons[first - 18]; ++comparisons[first - 4];
    }
    auto a = fixture_state(28), b = clone(*a); const auto before = a->registers;
    require(run(*a, 28, true).empty() && run(*b, 28, false).empty(), "Capture composition root");
    for (unsigned n = 13; n < 32; ++n) { a->wr<4>(n * 4, 0); b->wr<4>(n * 4, 0); }
    for (unsigned n = 14; n < 32; ++n) { a->wr<8>(0x4000 + n * 16, 0); b->wr<8>(0x4000 + n * 16, 0); }
    a->pc = b->pc = entries[29].address; a->wr<4>(12, buffer); b->wr<4>(12, buffer);
    a->wr<4>(16, 17); b->wr<4>(16, 17);
    require(run(*a, 29, true).empty() && run(*b, 29, false).empty(), "Jump composition root");
    Image(*a).compare(*b, "Composed nonlocal capture/restore");
    require(b->pc == incoming_link && b->r<4>(12) == 17, "Independent captured destination and resumed value");
    for (unsigned n = 13; n < 32; ++n) require(b->r<4>(n * 4) == read_be<4>(before.data() + n * 4), "Independent general-register round trip");
    for (unsigned n = 14; n < 32; ++n) require(b->r<8>(0x4000 + n * 16) == read_be<8>(before.data() + 0x4000 + n * 16), "Independent floating-register round trip");
    ++comparisons[28]; ++comparisons[29];
}
}
int main() {
    try {
        roots_and_faults(); live_effects(); wrapped_contexts(); compositions();
        for (unsigned index = 0; index < entries.size(); ++index) {
            require(comparisons[index] > 0 && faults[index] > 0 && negatives[index] == 1, "Missing individual register-context component evidence");
            std::cout << hex_address(entries[index].address) << ": " << comparisons[index] << " comparisons, " << faults[index] << " fault prefixes, " << negatives[index] << " rejected negative\n";
        }
        std::cout << std::accumulate(comparisons.begin(), comparisons.end(), 0u) << " component comparisons; "
            << std::accumulate(faults.begin(), faults.end(), 0u) << " mapping-fault prefixes; "
            << std::accumulate(negatives.begin(), negatives.end(), 0u) << " individual negatives\n"
            << "Raw architectural components; production dispatch and host nonlocal routing are not covered here.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
