#include "file_controller_internal.hpp"

namespace cm::aot::file_controller {
namespace {
void unlink_record(Abi& a, unsigned record, std::uint32_t first) {
    a.load(0, a.r(2) - 0x3508, first); a.compare(a.r(0), a.r(record));
    if (a.r(0) == a.r(record)) {
        a.load(3, a.r(record) + 152, first + 12); a.compare(a.r(3)); a.store(a.r(2) - 0x3508, a.r(3), first + 20);
        if (a.r(3)) { a.put(0, 0); a.store(a.r(3) + 156, a.r(0), first + 32); }
    } else {
        a.load(3, a.r(record) + 152, first + 40); a.compare(a.r(3));
        if (a.r(3)) { a.load(0, a.r(record) + 156, first + 52); a.store(a.r(3) + 156, a.r(0), first + 56); }
        a.load(3, a.r(record) + 156, first + 60); a.compare(a.r(3));
        if (a.r(3)) { a.load(0, a.r(record) + 152, first + 72); a.store(a.r(3) + 152, a.r(0), first + 76); }
    }
    a.put(3, a.r(record)); a.call(0x10775170, first + 88);
}
struct ForkRequest {
    unsigned block, result;
    std::uint32_t async, async_link, yield_link, poll, sync, sync_link;
};
void request(Abi& a, const ForkRequest& layout) {
    if (a.r(30)) {
        a.put(3, a.r(1) + layout.block); a.native_call(layout.async, layout.async_link);
        for (;;) {
            a.load<2>(0, a.r(1) + layout.result, layout.poll); a.signed_word(0, a.r(0)); a.compare(a.signed_r(0));
            if (a.signed_r(0) <= 0) break;
            a.put(12, a.r(30)); a.callback(layout.yield_link); a.s.step(layout.poll);
        }
    } else { a.put(3, a.r(1) + layout.block); a.native_call(layout.sync, layout.sync_link); }
}
constexpr ForkRequest modern_size{108,124,0x1077fc88,0x10777484,0x10777498,0x1077749c,0x1077fca0,0x107774b4};
constexpr ForkRequest modern_grow{108,124,0x1077fcb8,0x10777584,0x10777598,0x1077759c,0x1077fcd0,0x107775b4};
constexpr ForkRequest modern_position{108,124,0x1077fce8,0x107775ec,0x10777600,0x10777604,0x1077fd00,0x1077761c};
constexpr ForkRequest classic_size{56,72,0x1077fd18,0x10777654,0x10777668,0x1077766c,0x1077fd30,0x10777684};
constexpr ForkRequest classic_grow{56,72,0x1077fd48,0x10777708,0x10777718,0x1077771c,0x1077fd60,0x10777734};
constexpr ForkRequest classic_position{56,72,0x1077fd78,0x10777768,0x10777778,0x1077777c,0x1077fd90,0x10777794};
void record_request_result(Abi& a, unsigned offset, std::uint32_t pc) {
    a.load<2>(0, a.r(1) + offset, pc); a.signed_word(0, a.r(0)); a.compare(a.signed_r(0));
}
std::int64_t signed_pair(std::uint32_t high, std::uint32_t low) {
    return std::bit_cast<std::int64_t>((std::uint64_t(high) << 32) | low);
}
}
void close_file(Abi& a) {
    a.save(31, 0x107777d0); a.capture_link(); a.save(30, 0x107777d8); a.put(30, a.r(3)); a.save_link(0x107777e0); a.frame(192, 0x107777e4);
    a.call(0x10776bc0, 0x107777ec); a.signed_byte_record(0, a.r(3));
    if (a.r(0)) {
        a.signed_word(3, a.r(30)); a.call(0x10776cd0, 0x107777fc); a.put(31, a.r(3)); a.signed_word(3, a.r(30));
        a.native_call(0x1077fe68, 0x10777808); a.put(30, a.r(3)); a.signed_word_record(0, a.r(30));
        bool remove = !a.r(0);
        if (remove) { a.compare(a.r(31)); remove = a.r(31) != 0; }
        if (remove) { a.put(3, a.r(31) + 72); a.native_call(0x1077fe80, 0x10777828); a.put(30, a.r(3)); }
        // The bookkeeping record is unlinked even when close/delete reports an
        // error. Its lifetime is not tied to the success-only deletion above.
        a.compare(a.r(31)); if (a.r(31)) unlink_record(a, 31, 0x10777838);
        a.signed_word(3, a.r(30)); a.boolean_error();
    } else {
        a.signed_word(3, a.r(30)); a.call(0x10776cd0, 0x107778ac); a.store<2>(a.r(1) + 80, a.r(30), 0x107778ac);
        a.put(30, a.r(3)); a.put(3, a.r(1) + 56); a.native_call(0x1077fe98, 0x107778bc); a.put(31, a.r(3)); a.signed_word_record(0, a.r(31));
        bool remove = !a.r(0);
        if (remove) { a.compare(a.r(30)); remove = a.r(30) != 0; }
        if (remove) {
            a.put(0, a.r(30) + 8); a.store(a.r(1) + 74, a.r(0), 0x107778d8); a.put(3, a.r(1) + 56);
            a.load<2>(0, a.r(30) + 2, 0x107778e0); a.signed_word(0, a.r(0)); a.store<2>(a.r(1) + 78, a.r(0), 0x107778e4);
            a.load(0, a.r(30) + 4, 0x107778e8); a.store(a.r(1) + 104, a.r(0), 0x107778ec); a.native_call(0x1077feb0, 0x107778f4); a.put(31, a.r(3));
        }
        a.compare(a.r(30)); if (a.r(30)) unlink_record(a, 30, 0x10777904);
        a.signed_word(3, a.r(31)); a.boolean_error();
    }
    a.pop(192, 0x1077796c); a.restore(31, 0x10777974); a.restore_link(); a.restore(30, 0x1077797c); a.finish();
}

void seek_file(Abi& a) {
    a.save(31, 0x10777430); a.capture_link(); a.put(31, a.r(3)); a.save(30, 0x1077743c); a.put(30, a.r(6));
    a.save(29, 0x10777444); a.put(29, a.r(4)); a.save(28, 0x1077744c); a.put(28, a.r(5)); a.save_link(0x10777454); a.frame(224, 0x10777458);
    a.call(0x10776bc0, 0x10777460); a.signed_byte_record(0, a.r(3));
    if (a.r(0)) {
        a.put(0, 0); a.store<2>(a.r(1) + 132, a.r(31), 0x1077746c); a.compare(a.r(30)); a.store(a.r(1) + 120, a.r(0), 0x10777474);
        request(a, modern_size); record_request_result(a, 124, 0x107774b8);
        if (a.r(0)) { a.put(3, 1); goto done; }
        a.compare(a.signed_r(28), 1); a.load(5, a.r(1) + 154, 0x107774d0); a.load(6, a.r(1) + 158, 0x107774d4);
        bool absolute = false, relative_end = false;
        if (a.r(28) != 1) {
            if (a.signed_r(28) < 1) { a.compare(a.signed_r(28)); absolute = a.signed_r(28) >= 0; }
            else { a.compare(a.signed_r(28), 3); relative_end = a.signed_r(28) < 3; }
        }
        if (relative_end) {
            a.load(3, a.r(29), 0x107774f4); a.put(0, a.signed_r(3) < 0 ? UINT32_MAX : 0);
            const auto low = std::uint64_t(a.r(6)) + a.r(3);
            const auto high = std::uint64_t(a.r(5)) + a.r(0) + (low >> 32);
            a.put(3, std::uint32_t(low)); a.put(0, std::uint32_t(high)); a.s.wr<1>(1027, high >> 32 != 0);
            a.put(31, a.r(3)); a.put(28, a.r(0));
        } else if (absolute) {
            a.load(31, a.r(29), 0x10777510); a.s.wr<1>(1027, (a.r(31) & 0x80000000u) && (a.r(31) & 0x7fffffffu));
            a.put(28, a.signed_r(31) < 0 ? UINT32_MAX : 0);
        } else { a.put(3, 1); goto done; }
        // Named signed-64-bit comparisons retain the Mac scratch/carry results
        // while avoiding a chain of host opcode interpretations.
        a.put(0, a.r(31)); a.put(4, a.r(28) ^ 0x80000000u);
        const auto desired = signed_pair(a.r(28), a.r(31));
        a.s.wr<1>(1027, desired >= 0); a.put(3, desired < 0); a.record(a.r(3));
        if (desired < 0) { a.put(3, 1); goto done; }
        a.put(3, a.r(5) ^ 0x80000000u);
        const bool grow = desired > signed_pair(a.r(5), a.r(6));
        a.s.wr<1>(1027, !grow); a.put(0, grow); a.record(a.r(0));
        if (grow) {
            a.put(0, 1); a.store(a.r(1) + 158, a.r(31), 0x10777568); a.compare(a.r(30));
            a.store<2>(a.r(1) + 152, a.r(0), 0x10777570); a.store(a.r(1) + 154, a.r(28), 0x10777574); request(a, modern_grow);
        }
        record_request_result(a, 124, 0x107775b8); if (a.r(0)) { a.put(3, 1); goto done; }
        a.put(0, 1); a.store(a.r(1) + 158, a.r(31), 0x107775d0); a.compare(a.r(30));
        a.store<2>(a.r(1) + 152, a.r(0), 0x107775d8); a.store(a.r(1) + 154, a.r(28), 0x107775dc); request(a, modern_position);
        a.store(a.r(29), a.r(31), 0x10777620); a.load<2>(3, a.r(1) + 124, 0x10777624); a.signed_word(3, a.r(3)); a.boolean_error();
    } else {
        a.put(0, 0); a.store<2>(a.r(1) + 80, a.r(31), 0x1077763c); a.compare(a.r(30)); a.store(a.r(1) + 68, a.r(0), 0x10777644);
        request(a, classic_size); record_request_result(a, 72, 0x10777688); if (a.r(0)) { a.put(3, 1); goto done; }
        a.compare(a.signed_r(28), 1); a.load(3, a.r(1) + 84, 0x107776a0);
        bool absolute = false, relative_end = false;
        if (a.r(28) != 1) {
            if (a.signed_r(28) < 1) { a.compare(a.signed_r(28)); absolute = a.signed_r(28) >= 0; }
            else { a.compare(a.signed_r(28), 3); relative_end = a.signed_r(28) < 3; }
        }
        if (relative_end) { a.load(0, a.r(29), 0x107776c0); a.put(28, a.r(3) + a.r(0)); }
        else if (absolute) a.load(28, a.r(29), 0x107776cc);
        else { a.put(3, 1); goto done; }
        a.compare(a.signed_r(28)); if (a.signed_r(28) < 0) { a.put(3, 1); goto done; }
        a.compare(a.signed_r(28), a.signed_r(3));
        if (a.signed_r(28) > a.signed_r(3)) { a.compare(a.r(30)); a.store(a.r(1) + 84, a.r(28), 0x107776f8); request(a, classic_grow); }
        record_request_result(a, 72, 0x10777738); if (a.r(0)) { a.put(3, 1); goto done; }
        a.put(0, 1); a.store(a.r(1) + 102, a.r(28), 0x10777750); a.compare(a.r(30)); a.store<2>(a.r(1) + 100, a.r(0), 0x10777758);
        request(a, classic_position); a.store(a.r(29), a.r(28), 0x10777798); a.load<2>(3, a.r(1) + 72, 0x1077779c); a.signed_word(3, a.r(3)); a.boolean_error();
    }
done:
    a.pop(224, 0x107777ac); a.restore(31, 0x107777b4); a.restore(30, 0x107777b8); a.restore_link();
    a.restore(29, 0x107777c0); a.restore(28, 0x107777c4); a.finish();
}
}
