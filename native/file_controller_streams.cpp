#include "file_controller_internal.hpp"

namespace cm::aot::file_controller {
void filename_facade(Abi& a, bool rename) {
    const auto entry = rename ? 0x107766c0u : 0x10776690u;
    a.capture_link(); a.save_link(entry + 4); a.frame(64, entry + 8);
    a.call(rename ? 0x10777a60 : 0x10777990, entry + 16);
    a.put(0, 0 - a.r(3)); a.put(0, a.r(0) | a.r(3));
    a.s.wr<1>(1027, (a.r(0) & 0x80000000u) && (a.r(0) & 0x7fffffffu));
    a.put(3, a.r(0) >> 31 ? UINT32_MAX : 0);
    a.pop(64, entry + 28); a.restore_link(); a.finish();
}
void open_stream(Abi& a) {
    a.save(31, 0x107768f0); a.capture_link(); a.put(31, a.r(4)); a.save(30, 0x107768fc);
    a.put(30, a.r(3)); a.save_link(0x10776904); a.frame(64, 0x10776908);
    a.call(0x10775240, 0x10776910); a.put(5, a.r(3)); a.put(3, a.r(30)); a.put(4, a.r(31));
    a.call(0x10776940, 0x10776920); a.pop(64, 0x10776920); a.restore(31, 0x10776928);
    a.restore_link(); a.restore(30, 0x10776930); a.finish();
}
void reopen_stream(Abi& a) {
    a.save(31, 0x10776940); a.capture_link(); a.put(31, a.r(5)); a.save(30, 0x1077694c);
    a.put(30, a.r(4)); a.save(29, 0x10776954); a.put(29, a.r(3)); a.save_link(0x1077695c); a.frame(80, 0x10776960);
    a.call(0x107784b0, 0x10776968); a.compare(a.r(31));
    if (!a.r(31)) { a.put(3, 0); goto done; }
    a.put(3, a.r(31)); a.call(0x107766f0, 0x10776980); a.put(3, a.r(31)); a.call(0x107784a0, 0x10776988);
    a.put(3, a.r(30)); a.put(4, a.r(1) + 56); a.call(0x10776a40, 0x10776994); a.compare(a.signed_r(3));
    if (!a.r(3)) { a.put(3, 0); goto done; }
    a.load(4, a.r(1) + 56, 0x107769a4); a.put(3, a.r(31)); a.put(5, 0); a.put(6, 4096);
    a.call(0x107752d0, 0x107769b8); a.load(4, a.r(1) + 56, 0x107769b8);
    a.put(3, a.r(29)); a.put(5, a.r(31)); a.call(0x10776da0, 0x107769c8); a.compare(a.signed_r(3));
    if (a.r(3)) {
        a.load<2>(0, a.r(31) + 4, 0x107769d0); a.put(3, 0); a.put(0, a.r(0) & ~0x1c0u);
        a.store<2>(a.r(31) + 4, a.r(0), 0x107769dc);
        a.load<1>(0, a.r(31) + 8, 0x107769e0); a.put(0, (a.r(0) >> 4) & 1); a.record(a.r(0));
        if (a.r(0)) { a.load(3, a.r(31) + 32, 0x107769ec); a.call(0x10775170, 0x107769f4); }
        a.put(3, 0);
    } else {
        a.load<1>(0, a.r(1) + 56, 0x107769fc); a.put(0, (a.r(0) >> 3) & 4); a.record(a.r(0));
        // Append positions at EOF after opening. The historical controller
        // returns the descriptor even if this positioning callback rejects.
        if (a.r(0)) { a.put(3, a.r(31)); a.put(4, 0); a.put(5, 2); a.call(0x10777f10, 0x10776a18); }
        a.put(3, a.r(31));
    }
done:
    a.pop(80, 0x10776a1c); a.restore(31, 0x10776a24); a.restore(30, 0x10776a28);
    a.restore_link(); a.restore(29, 0x10776a30); a.finish();
}
void set_classic_finder(Abi& a) {
    a.save(31, 0x10776c30); a.capture_link(); a.put(31, a.r(4)); a.save(30, 0x10776c3c);
    a.put(30, a.r(3)); a.save_link(0x10776c44); a.put(0, a.r(30) + 6); a.frame(176, 0x10776c4c);
    a.store(a.r(1) + 74, a.r(0), 0x10776c50); a.put(0, 0); a.load<2>(4, a.r(3), 0x10776c58); a.signed_word(4, a.r(4));
    a.put(3, a.r(1) + 56); a.store<2>(a.r(1) + 78, a.r(4), 0x10776c60); a.store<2>(a.r(1) + 84, a.r(0), 0x10776c64);
    a.load(0, a.r(30) + 2, 0x10776c68); a.store(a.r(1) + 104, a.r(0), 0x10776c6c);
    a.native_call(0x1077fc10, 0x10776c74); a.signed_word_record(0, a.r(3));
    if (!a.r(0)) {
        a.put(3, a.r(31)); a.call(0x10776d60, 0x10776c88); a.store(a.r(1) + 88, a.r(3), 0x10776c88);
        a.put(3, a.r(31)); a.call(0x10776d20, 0x10776c94); a.store(a.r(1) + 92, a.r(3), 0x10776c94);
        a.put(3, a.r(1) + 56); a.load(0, a.r(30) + 2, 0x10776c9c); a.store(a.r(1) + 104, a.r(0), 0x10776ca0);
        a.native_call(0x10780030, 0x10776ca8);
    }
    a.pop(176, 0x10776cac); a.restore(31, 0x10776cb4); a.restore_link(); a.restore(30, 0x10776cbc); a.finish();
}

namespace {
struct TransferLayout {
    unsigned block, result, ref, completion, buffer, request, actual, mode, high, low;
    std::uint32_t initialize, async, async_link, yield, yield_link, poll, sync, sync_link, publish, final_result, error_boolean;
};
constexpr TransferLayout modern_read{108,124,132,120,140,144,148,152,154,158,
    0x10777178,0x1077fda8,0x107771ac,0x107771b8,0x107771c0,0x107771c4,0x1077fdc0,0x107771dc,0x107771e0,0x107771e8,0x10777210};
constexpr TransferLayout classic_read{56,72,80,68,88,92,96,100,0,0,
    0x10777224,0x1077fdd8,0x10777250,0x10777258,0x10777260,0x10777264,0x1077fdf0,0x1077727c,0x10777280,0x10777288,0x107772b0};
constexpr TransferLayout modern_write{108,124,132,120,140,144,148,152,154,158,
    0x10777318,0x1077fe08,0x1077734c,0x10777358,0x10777360,0x10777364,0x1077fe20,0x1077737c,0x10777380,0x10777388,0x10777388};
constexpr TransferLayout classic_write{56,72,80,68,88,92,96,100,0,0,
    0x1077739c,0x1077fe38,0x107773c8,0x107773d0,0x107773d8,0x107773dc,0x1077fe50,0x107773f4,0x107773f8,0x10777400,0x10777400};
void wait_for_transfer(Abi& a, const TransferLayout& layout) {
    if (a.r(31)) {
        a.put(3, a.r(1) + layout.block); a.native_call(layout.async, layout.async_link);
        for (;;) {
            a.load<2>(0, a.r(1) + layout.result, layout.poll); a.signed_word(0, a.r(0)); a.compare(a.signed_r(0));
            if (a.signed_r(0) <= 0) break;
            a.put(12, a.r(31)); a.callback(layout.yield_link); a.s.step(layout.poll);
        }
    } else { a.put(3, a.r(1) + layout.block); a.native_call(layout.sync, layout.sync_link); }
}
}
void transfer_file(Abi& a, bool writing) {
    const auto entry = writing ? 0x107772e0u : 0x10777140u;
    a.save(31, entry); a.capture_link(); a.put(31, a.r(6)); a.save(30, entry + 12); a.put(30, a.r(5));
    a.save(29, entry + 20); a.put(29, a.r(4)); a.save(28, entry + 28); a.put(28, a.r(3));
    a.save_link(entry + 36); a.frame(224, entry + 40); a.call(0x10776bc0, entry + 48); a.signed_byte_record(0, a.r(3));
    const auto& layout = writing ? (a.r(0) ? modern_write : classic_write) : (a.r(0) ? modern_read : classic_read);
    const auto p = layout.initialize;
    a.put(3, 0); a.store<2>(a.r(1) + layout.ref, a.r(28), p + 4); a.compare(a.r(31));
    a.store(a.r(1) + layout.completion, a.r(3), p + 12); a.store(a.r(1) + layout.buffer, a.r(29), p + 16);
    a.load(0, a.r(30), p + 20); a.store(a.r(1) + layout.request, a.r(0), p + 24); a.store<2>(a.r(1) + layout.mode, a.r(3), p + 28);
    if (layout.high) {
        a.store(a.r(1) + layout.low, a.r(3), p + 32); a.store(a.r(1) + layout.high, a.r(3), p + 36);
    }
    wait_for_transfer(a, layout);
    a.load(0, a.r(1) + layout.actual, layout.publish); a.store(a.r(30), a.r(0), layout.publish + 4);
    if (!writing) {
        a.load<2>(0, a.r(1) + layout.result, layout.final_result); a.signed_word(0, a.r(0)); a.compare(a.signed_r(0), -39);
        if (a.signed_r(0) == -39) {
            a.load(4, a.r(30), layout.final_result + 12); a.put(0, 2); a.put(3, 0 - a.r(4)); a.put(3, a.r(3) | a.r(4));
            a.s.wr<1>(1027, (a.r(3) & 0x80000000u) && (a.r(3) & 0x7fffffffu));
            a.put(3, a.r(3) >> 31 ? UINT32_MAX : 0); a.put(3, a.r(0) & ~a.r(3));
        } else { a.load<2>(3, a.r(1) + layout.result, layout.error_boolean); a.signed_word(3, a.r(3)); a.boolean_error(); }
    } else { a.load<2>(3, a.r(1) + layout.result, layout.error_boolean); a.signed_word(3, a.r(3)); a.boolean_error(); }
    const auto tail = writing ? 0x10777410u : 0x107772c0u;
    a.pop(224, tail); a.restore(31, tail + 8); a.restore(30, tail + 12); a.restore_link();
    a.restore(29, tail + 20); a.restore(28, tail + 24); a.finish();
}
}
