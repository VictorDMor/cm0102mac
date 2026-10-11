#pragma once
#include "exception_runtime_context.hpp"
#include "semantic_io_checks.hpp"

namespace cm::exception_checks {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t context = fixture + 0x2000, descriptors = fixture + 0x4000,
    descriptor = descriptors + 0x200, action = descriptor + 0x80, metadata = fixture + 0x5000,
    actual = fixture + 0x8000, expected = fixture + 0x8100, object = fixture + 0x8200,
    callback = fixture + 0x8400, frame = fixture + 0xa000, indirect = fixture + 0xb000,
    restored_stack = fixture + 0xc000, node = fixture + 0x1000, ranges = fixture + 0x1800,
    base = 0x10000000, landing = base + 0x300;
using Operation = void (*)(State&);
using Expect = std::function<void(const State&, const Effects&)>;
using Mutation = decltype(Effects::on_read);
struct Fixture {
    std::unique_ptr<State> state;
    Operation operation = exception_runtime::search_throw;
    std::uint32_t stop = landing, block = frame + 0x40, selected = action,
        published = object, selected_frame = frame, restored_backchain = 0x12345678,
        catch_toc = toc;
    unsigned callback_count = 0;
    bool specification = false;
};
inline void text(State& s, std::uint32_t address, const std::string& value) {
    std::vector<std::uint8_t> bytes(value.begin(), value.end()); bytes.push_back(0); s.memory.copy_in(address, bytes);
}
inline void handler(State& s, std::uint32_t record, std::uint32_t type = expected, unsigned offset = 0x200, int block_offset = 0x40) {
    s.memory.write<1>(record, 12); s.memory.write<4>(record + 2, type);
    s.memory.write<2>(record + 6, offset); s.memory.write<2>(record + 8, std::uint32_t(block_offset));
}
inline Fixture setup(const char* program, bool selected_indirect = false, bool vectors = false,
        unsigned seed = 0, std::uint32_t live_toc = toc) {
    Fixture f; f.state = std::make_unique<State>(); auto& s = *f.state; load_program(s, program);
    s.memory.map(fixture, 0x10000); s.memory.clear(fixture, 0x10000);
    s.memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s.wr<4>(n * 4, 0xcafe0000 + n);
    s.wr<4>(4, stack); s.wr<4>(8, live_toc); s.wr<4>(12, context); s.wr<4>(4128, return_sentinel);
    s.wr<4>(4132, 0x31415926); s.wr<1>(2304, seed % 2 ? 0xf9 : 0xa6);
    s.wr<1>(1024, seed % 4); s.wr<1>(1027, (seed / 2) % 2); s.budget = 50000; s.pc = 0x10772810;
    if (live_toc != toc) for (auto offset : {0x3b9c, 0x3b8c, 0x3b88})
        s.memory.write<4>(live_toc - offset, s.memory.read<4>(toc - offset));
    s.memory.write<1>(live_toc - 0x3529, 1); s.memory.write<1>(live_toc - 0x352a, vectors);
    s.memory.write<4>(live_toc - 0x3524, node); s.memory.write<4>(live_toc - 0x3528, 0);
    s.memory.write<4>(live_toc - 0x3520, callback);
    for (auto [offset, value] : {std::pair{0u, ranges}, {4u, ranges + 12}, {8u, base},
            {12u, base + 0x1000}, {16u, descriptors}, {24u, live_toc}}) s.memory.write<4>(node + offset, value);
    s.memory.write<4>(ranges, 0x100); s.memory.write<4>(ranges + 4, 0x40); s.memory.write<4>(ranges + 8, 0x200);
    s.memory.write<1>(descriptor + 1, selected_indirect ? 16 : 0);
    s.memory.write<2>(descriptor + 2, 0x10); s.memory.write<2>(descriptor + 4, 0x30); s.memory.write<2>(descriptor + 6, 0x80);
    handler(s, action); text(s, actual, "i"); text(s, expected, "i"); s.memory.write<4>(object, 0xa0b0c0d0);
    s.memory.write<4>(callback, 0x10773700); s.memory.write<4>(callback + 4, fixture + 0xf100);
    for (unsigned n = 13; n < 32; ++n) s.memory.write<4>(context + 0x318 + n * 4, 0xabc00000 + n);
    for (unsigned n = 14; n < 32; ++n) s.memory.write<8>(context + 0x218 + n * 8, 0x1122334400000000ULL + n);
    s.memory.write<4>(context + 0x210, 3); s.memory.write<4>(context + 0x200, 0x10001);
    for (unsigned n = 20; n < 32; ++n) {
        Vector128 bytes{}; for (unsigned i = 0; i < bytes.size(); ++i) bytes[i] = std::uint8_t(n * 11 + i);
        s.memory.write_vector(context + n * 16, bytes);
    }
    s.memory.write<4>(context + 0x398, 0x13579bdf); s.memory.write<4>(context + 0x39c, frame);
    s.memory.write<4>(context + 0x394, indirect); s.memory.write<4>(context + 0x3a4, restored_stack);
    s.memory.write<4>(context + 0x3a8, base + 0x120); s.memory.write<4>(context + 0x3ac, actual);
    s.memory.write<4>(context + 0x3b0, object); s.memory.write<4>(context + 0x3b4, callback);
    s.memory.write<4>(context + 0x3b8, 0xdeadbeef); s.memory.write<4>(frame, f.restored_backchain);
    f.selected_frame = selected_indirect ? indirect : frame; f.block = f.selected_frame + 0x40; f.catch_toc = live_toc; return f;
}
inline Fixture specification_fixture(const char* program, unsigned count = 1, bool accepted = false, bool direct = false) {
    auto f = setup(program); auto& s = *f.state;
    s.memory.write<1>(action, 15); s.memory.write<2>(action + 2, count);
    s.memory.write<4>(action + 4, 0x200); s.memory.write<4>(action + 8, 0x40);
    text(s, expected, accepted ? "i" : "l");
    for (unsigned n = 0; n < count; ++n) s.memory.write<4>(action + 12 + n * 4, expected);
    f.specification = !accepted || direct;
    if (direct) {
        s.memory.write<4>(metadata, descriptor); s.memory.write<4>(metadata + 4, base + 0x100);
        s.memory.write<4>(metadata + 8, action); s.memory.write<4>(metadata + 0x14, toc);
        s.memory.write<4>(context + 0x3a0, frame); s.wr<4>(16, metadata); s.wr<4>(20, action); s.pc = 0x10772780;
        f.operation = exception_runtime::install_catch;
    } else if (accepted) {
        f.selected = action + 12 + count * 4; text(s, expected + 0x40, "i"); handler(s, f.selected, expected + 0x40);
    }
    return f;
}
inline std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->budget = initial.budget; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    s->allocated = initial.allocated; s->available = initial.available;
    const auto copy = [&](std::uint32_t address, std::uint64_t size) {
        if (!size) return; s->memory.map(address, size); s->memory.copy_in(address, initial.memory.copy_out(address, size));
    };
    for (auto [address, size] : initial.memory.mapped_ranges()) {
        if (omitted >= address && std::uint64_t(omitted) < std::uint64_t(address) + size) {
            copy(address, omitted - address); const auto remaining = std::uint64_t(address) + size - omitted - 1;
            if (remaining) copy(omitted + 1, remaining);
        } else copy(address, size);
    }
    return s;
}
}
