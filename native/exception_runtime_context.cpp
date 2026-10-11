#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"
#include "vector_context.hpp"

namespace cm::aot::exception_runtime {
namespace {
struct ContextAbi : MacAbi {
    explicit ContextAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes) {}
    void register_suffix(unsigned first, std::uint32_t base, std::uint32_t pc, bool restoring) {
        s.wr<4>(2432, base);
        for (unsigned n = first; n < 32; ++n) {
            if (restoring) load(n, std::uint32_t(s.r<4>(2432)), pc);
            else store(std::uint32_t(s.r<4>(2432)), r(n), pc);
            s.wr<4>(2432, std::uint32_t(s.r<4>(2432)) + 4);
        }
    }
    unsigned vector_suffix(unsigned scratch, std::uint32_t link, std::uint32_t bias) {
        put(5, r(5) & 0xfff);
        const auto first = unsigned(std::countl_zero(r(5)));
        put(5, first * 8);
        put(scratch, link - bias + r(5));
        s.wr<4>(4128, r(scratch));
        return first;
    }
};
}
void restore_context(State& state) {
    ContextAbi a(state);
    a.put(8, a.r(5)); a.put(2, a.r(4)); a.load(0, a.r(3) + 0x398, 0x10772658);
    for (unsigned field = 0; field < 8; ++field) state.wr<1>(2304 + field, (a.r(0) >> (28 - field * 4)) & 15);
    a.record(a.r(6));
    if (a.r(6)) {
        a.load(5, a.r(3) + 0x210, 0x10772668);
        // The linked branch to its next instruction captures PC, rather than
        // making an ordinary call; it does not write Ghidra's TOC-save slot.
        state.wr<4>(4128, 0x10772670);
        a.put(5, a.r(5) | 0x80000000); state.wr<4>(5120, a.r(5));
        const auto first = a.vector_suffix(7, 0x10772670, 0x78);
        for (unsigned n = first; n < 32; ++n) {
            a.put(31, a.r(3) + n * 16);
            state.pc = 0x1077269c + (n - 20) * 8;
            state.write_vector_register(0x4200 + n * 16, state.memory.read_vector(a.r(31) & ~std::uint32_t(15)));
        }
    }
    a.register_suffix(13, a.r(3) + 0x34c, 0x107726f8, true);
    for (unsigned n = 14; n < 32; ++n) {
        state.pc = 0x107726fc + (n - 14) * 4;
        state.wr<8>(0x4000 + n * 16, state.memory.read<8>(a.r(3) + 0x218 + n * 8));
    }
    state.wr<4>(4128, a.r(8)); a.record(a.r(6));
    if (a.r(6)) {
        a.put(5, a.r(3) + 0x200); state.pc = 0x10772754;
        state.write_vector_register(0x4200, load_vector_word(state, 0, a.r(5)));
        move_to_vector_status(state, state.read_vector_register(0x4200));
        a.load(5, a.r(3) + 0x210, 0x1077275c); state.wr<4>(5120, a.r(5));
    }
    a.load(1, a.r(3) + 0x3a4, 0x10772764); a.load(3, a.r(3) + 0x39c, 0x10772768);
    record_context_frame(state, a.r(3));
    a.load(3, a.r(3), 0x1077276c); a.store(a.r(1), a.r(3), 0x10772770); a.finish();
    complete_context_transfer(state);
}
void capture_vector_throw(State& state) {
    ContextAbi a(state);
    a.capture_link(); a.put(11, a.r(30)); a.save(31, 0x10772b18); a.put(12, a.r(31));
    a.save(30, 0x10772b20); a.save_link(0x10772b24);
    a.store(a.r(1) + 0x18, a.r(3), 0x10772b28); a.store(a.r(1) + 0x1c, a.r(4), 0x10772b2c);
    a.put(30, a.r(1)); a.store(a.r(1) + 0x20, a.r(5), 0x10772b34);
    a.put(1, a.r(1) & ~std::uint32_t(15)); a.store(a.r(1) - 0x400, a.r(30), 0x10772b3c);
    a.put(1, a.r(1) - 0x400); a.put(31, a.r(1) + 0x40);
    a.register_suffix(13, a.r(31) + 0x34c, 0x10772b44, false);
    a.store(a.r(31) + 0x390, a.r(11), 0x10772b48); a.store(a.r(31) + 0x394, a.r(12), 0x10772b4c);
    a.put(5, std::uint32_t(state.r<4>(5120))); a.store(a.r(31) + 0x210, a.r(5), 0x10772b54);
    state.wr<4>(4128, 0x10772b5c);
    a.store(a.r(31) + 0x210, a.r(5), 0x10772b5c);
    const auto first = a.vector_suffix(4, 0x10772b5c, 0x7c);
    for (unsigned n = first; n < 32; ++n) {
        a.put(3, n * 16); state.pc = 0x10772b84 + (n - 20) * 8;
        state.memory.write_vector((a.r(3) + a.r(31)) & ~std::uint32_t(15), state.read_vector_register(0x4200 + n * 16));
    }
    a.put(6, std::uint32_t(state.r<4>(5120)) | 0x80000000); state.wr<4>(5120, a.r(6));
    state.write_vector_register(0x4200, vector_status_register(state));
    a.put(5, a.r(31) + 0x200);
    a.store(a.r(5) & ~std::uint32_t(3), vector_word_for_address(state.read_vector_register(0x4200), a.r(5)), 0x10772bf4);
    a.put(5, 0); state.wr<4>(5120, 0);
    for (unsigned n = 14; n < 32; ++n) {
        state.pc = 0x10772c00 + (n - 14) * 4;
        state.memory.write<8>(a.r(31) + 0x218 + n * 8, state.r<8>(0x4000 + n * 16));
    }
    std::uint32_t condition = 0;
    for (unsigned field = 0; field < 8; ++field) condition |= std::uint32_t(state.r<1>(2304 + field) & 15) << (28 - field * 4);
    a.put(3, condition); a.store(a.r(31) + 0x398, a.r(3), 0x10772c4c);
    a.load(3, a.r(1), 0x10772c50); a.load(4, a.r(3) + 8, 0x10772c54);
    a.store(a.r(31) + 0x39c, a.r(3), 0x10772c58); a.store(a.r(31) + 0x3a4, a.r(3), 0x10772c5c);
    a.store(a.r(31) + 0x3a8, a.r(4), 0x10772c60);
    for (unsigned n = 0; n < 3; ++n) {
        a.load(3, a.r(30) + 0x18 + n * 4, 0x10772c64 + n * 8);
        a.store(a.r(31) + 0x3ac + n * 4, a.r(3), 0x10772c68 + n * 8);
    }
    a.put(3, a.r(31)); state.wr<4>(2440, a.r(2)); state.wr<4>(4128, 0x10772c84); state.pc = 0x10772810;
}
}
