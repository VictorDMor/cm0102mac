#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"

namespace cm::aot::exception_runtime {
namespace {
struct CaptureAbi : MacAbi {
    explicit CaptureAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes) {}
    void save_context_registers(std::uint32_t base) {
        s.wr<4>(2432, base);
        for (unsigned n = 13; n < 32; ++n) {
            store(std::uint32_t(s.r<4>(2432)), r(n), 0x10772cd4);
            s.wr<4>(2432, std::uint32_t(s.r<4>(2432)) + 4);
        }
    }
    void finish_capture(std::uint32_t pc) {
        load(0, r(30) + 8, pc); put(1, r(30)); restore_link();
        restore(31, pc + 12); restore(30, pc + 16); finish();
    }
    void child(std::uint32_t entry, std::uint32_t link) {
        s.wr<4>(2440, r(2)); s.wr<4>(4128, link); s.pc = entry; invoke_exception_runtime(s, entry);
    }
};
}
void capture_scalar_throw(State& state) {
    CaptureAbi a(state);
    a.capture_link(); a.put(11, a.r(30)); a.save(31, 0x10772ca8); a.put(12, a.r(31));
    a.save(30, 0x10772cb0); a.save_link(0x10772cb4);
    a.store(a.r(1) + 0x18, a.r(3), 0x10772cb8); a.store(a.r(1) + 0x1c, a.r(4), 0x10772cbc);
    a.put(30, a.r(1)); a.store(a.r(1) + 0x20, a.r(5), 0x10772cc4);
    a.put(1, a.r(1) & ~std::uint32_t(15)); a.store(a.r(1) - 0x400, a.r(30), 0x10772ccc);
    a.put(1, a.r(1) - 0x400); a.guest_scope.frame(a.r(1)); a.put(31, a.r(1) + 0x40);
    a.save_context_registers(a.r(31) + 0x34c);
    a.store(a.r(31) + 0x390, a.r(11), 0x10772cd8); a.store(a.r(31) + 0x394, a.r(12), 0x10772cdc);
    for (unsigned n = 14; n < 32; ++n) {
        state.pc = 0x10772ce0 + (n - 14) * 4;
        state.memory.write<8>(a.r(31) + 0x218 + n * 8, state.r<8>(0x4000 + n * 16));
    }
    std::uint32_t condition = 0;
    for (unsigned field = 0; field < 8; ++field) condition |= std::uint32_t(state.r<1>(2304 + field) & 15) << (28 - field * 4);
    a.put(3, condition); a.store(a.r(31) + 0x398, a.r(3), 0x10772d2c);
    a.load(3, a.r(1), 0x10772d30); a.load(4, a.r(3) + 8, 0x10772d34);
    a.store(a.r(31) + 0x39c, a.r(3), 0x10772d38); a.store(a.r(31) + 0x3a4, a.r(3), 0x10772d3c);
    a.store(a.r(31) + 0x3a8, a.r(4), 0x10772d40);
    for (unsigned n = 0; n < 3; ++n) {
        a.load(3, a.r(30) + 0x18 + n * 4, 0x10772d44 + n * 8);
        a.store(a.r(31) + 0x3ac + n * 4, a.r(3), 0x10772d48 + n * 8);
    }
    a.put(3, a.r(31)); state.wr<4>(2440, a.r(2)); state.wr<4>(4128, 0x10772d64); state.pc = 0x10772810;
}
void throw_vector(State& state) {
    CaptureAbi a(state);
    capture_vector_throw(state); a.guest_scope.frame(a.r(1)); invoke_exception_runtime(state, 0x10772810);
    if (state.pc == 0x10772c84) a.finish_capture(0x10772c84);
}
void throw_scalar(State& state) {
    CaptureAbi a(state);
    capture_scalar_throw(state); a.guest_scope.frame(a.r(1)); invoke_exception_runtime(state, 0x10772810);
    if (state.pc == 0x10772d64) a.finish_capture(0x10772d64);
}
void throw_dispatch(State& state) {
    CaptureAbi a(state);
    a.capture_link(); a.save_link(0x10772ad4); a.frame(64, 0x10772ad8);
    a.load<1>(0, a.r(2) - 0x352a, 0x10772adc); a.compare(a.r(0));
    const auto link = a.r(0) ? 0x10772aecu : 0x10772af4u;
    if (a.r(0)) a.child(0x10772b10, link);
    else a.child(0x10772ca0, link);
    if (state.pc != link) return;
    a.pop(64, 0x10772af4); a.restore_link(); a.finish();
}
}
