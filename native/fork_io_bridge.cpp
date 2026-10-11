#include "fork_io_bridge.hpp"
#include "aot_runtime.hpp"
#include "callbacks.hpp"
#include "data_forks.hpp"
#include <algorithm>
#include <array>
#include <bit>

namespace cm::aot {
namespace {
constexpr std::int16_t parameter_error = -50;
constexpr std::uint32_t chunk_size = 1024 * 1024;
void check_extent(std::uint32_t buffer, std::uint32_t count, const char* action) {
    if (std::uint64_t(buffer) + count > (UINT64_C(1) << 32))
        throw std::runtime_error(std::string("Original file ") + action +
                                 " crosses the 32-bit data address boundary");
}
enum class Operation { read, write, size, resize, position };
struct ParameterCall { std::string_view name; Operation operation; bool fork, asynchronous; };
constexpr std::array calls{
    ParameterCall{"CarbonLib::PBReadForkSync", Operation::read, true, false},
    ParameterCall{"CarbonLib::PBReadForkAsync", Operation::read, true, true},
    ParameterCall{"CarbonLib::PBWriteForkSync", Operation::write, true, false},
    ParameterCall{"CarbonLib::PBWriteForkAsync", Operation::write, true, true},
    ParameterCall{"CarbonLib::PBGetForkSizeSync", Operation::size, true, false},
    ParameterCall{"CarbonLib::PBGetForkSizeAsync", Operation::size, true, true},
    ParameterCall{"CarbonLib::PBSetForkSizeSync", Operation::resize, true, false},
    ParameterCall{"CarbonLib::PBSetForkSizeAsync", Operation::resize, true, true},
    ParameterCall{"CarbonLib::PBSetForkPositionSync", Operation::position, true, false},
    ParameterCall{"CarbonLib::PBSetForkPositionAsync", Operation::position, true, true},
    ParameterCall{"CarbonLib::PBReadAsync", Operation::read, false, true},
    ParameterCall{"CarbonLib::PBWriteAsync", Operation::write, false, true},
    ParameterCall{"CarbonLib::PBGetEOFAsync", Operation::size, false, true},
    ParameterCall{"CarbonLib::PBSetEOFAsync", Operation::resize, false, true},
    ParameterCall{"CarbonLib::PBSetFPosAsync", Operation::position, false, true},
};
std::uint32_t arg(const State& s, unsigned n) { return std::uint32_t(s.r<4>((n + 3) * 4)); }
std::int64_t word_pair(const State& s, unsigned n) {
    return std::bit_cast<std::int64_t>((std::uint64_t(arg(s, n)) << 32) | arg(s, n + 1));
}
}
std::int16_t read_guest_fork(State& s, NativeDataForks& forks, std::int16_t ref,
    std::uint16_t mode, std::int64_t offset, std::uint32_t buffer,
    std::int64_t requested, std::uint32_t& actual) {
    actual = 0;
    if (requested < 0 || requested > UINT32_MAX) return parameter_error;
    check_extent(buffer, std::uint32_t(requested), "read");
    do {
        std::vector<std::uint8_t> bytes(std::min<std::uint32_t>(requested - actual, chunk_size));
        std::uint32_t count = 0;
        const auto error = forks.read(ref, mode, offset, bytes, count);
        bytes.resize(count);
        if (count) s.memory.copy_in(buffer + actual, bytes);
        actual += count;
        if (error || actual == std::uint32_t(requested)) return error;
        if (!count) throw std::runtime_error("Native file read made no progress without reporting EOF");
        mode = 0; offset = 0;
    } while (true);
}
std::int16_t write_guest_fork(State& s, NativeDataForks& forks, std::int16_t ref,
    std::uint16_t mode, std::int64_t offset, std::uint32_t buffer,
    std::int64_t requested, std::uint32_t& actual) {
    actual = 0;
    if (requested < 0 || requested > UINT32_MAX) return parameter_error;
    check_extent(buffer, std::uint32_t(requested), "write");
    do {
        const auto count = std::min<std::uint32_t>(requested - actual, chunk_size);
        const auto bytes = count ? s.memory.copy_out(buffer + actual, count) : std::vector<std::uint8_t>{};
        std::uint32_t written = 0;
        const auto error = forks.write(ref, mode, offset, bytes, written);
        actual += written;
        if (error || actual == std::uint32_t(requested)) return error;
        if (!written) throw std::runtime_error("Native file write made no progress without reporting an error");
        mode = 0; offset = 0;
    } while (true);
}
std::optional<ForkIoResult> dispatch_fork_io(State& s, NativeDataForks& forks, std::string_view name) {
    const auto found = std::find_if(calls.begin(), calls.end(), [=](const auto& call) { return call.name == name; });
    if (found == calls.end()) {
        if (name == "CarbonLib::FSCloseFork") return ForkIoResult{forks.close(std::int16_t(arg(s, 0))), false, 0, 0, ForkIoResult::Return::result};
        if (name == "CarbonLib::FSSetForkSize")
            return ForkIoResult{forks.resize(std::int16_t(arg(s, 0)), std::uint16_t(arg(s, 1)), word_pair(s, 2)), false, 0, 0, ForkIoResult::Return::result};
        return std::nullopt;
    }
    const auto pb = arg(s, 0);
    const auto completion = found->asynchronous ? std::uint32_t(s.memory.read<4>(pb + 12)) : 0;
    if (found->asynchronous) s.memory.write<2>(pb + 16, 1);
    // File Manager synchronous parameter calls clear ioCompletion. Never invoke
    // it from a synchronous service, even if a previous async call left it set.
    else s.memory.write<4>(pb + 12, 0);
    const auto ref = std::int16_t(s.memory.read<2>(pb + 24));
    std::int16_t error = 0;
    switch (found->operation) {
    case Operation::read:
    case Operation::write: {
        const auto mode = std::uint16_t(s.memory.read<2>(pb + 44));
        const auto offset = found->fork ? std::bit_cast<std::int64_t>(s.memory.read<8>(pb + 46)) :
                                        std::int64_t(std::bit_cast<std::int32_t>(std::uint32_t(s.memory.read<4>(pb + 46))));
        const auto count = std::uint32_t(s.memory.read<4>(pb + 36));
        const auto requested = found->fork ? std::int64_t(count) : std::int64_t(std::bit_cast<std::int32_t>(count));
        const auto buffer = std::uint32_t(s.memory.read<4>(pb + 32));
        std::uint32_t actual = 0;
        error = found->operation == Operation::read ?
            read_guest_fork(s, forks, ref, mode, offset, buffer, requested, actual) :
            write_guest_fork(s, forks, ref, mode, offset, buffer, requested, actual);
        s.memory.write<4>(pb + 40, actual);
        if (!found->fork) {
            std::int64_t position = 0;
            if (!forks.position(ref, position)) s.memory.write<4>(pb + 46, std::uint32_t(position));
        }
        break;
    }
    case Operation::size: {
        std::int64_t size = 0;
        error = forks.size(ref, size);
        if (!error) {
            if (found->fork) s.memory.write<8>(pb + 46, std::uint64_t(size));
            else {
                if (size > INT32_MAX) throw std::runtime_error("Large file requires the 64-bit fork API bridge");
                s.memory.write<4>(pb + 28, std::uint32_t(size));
            }
        }
        break;
    }
    case Operation::resize:
        error = found->fork ? forks.resize(ref, std::uint16_t(s.memory.read<2>(pb + 44)),
                    std::bit_cast<std::int64_t>(s.memory.read<8>(pb + 46))) :
                             forks.resize(ref, std::bit_cast<std::int32_t>(std::uint32_t(s.memory.read<4>(pb + 28))));
        break;
    case Operation::position: {
        const auto mode = std::uint16_t(s.memory.read<2>(pb + 44));
        const auto offset = found->fork ? std::bit_cast<std::int64_t>(s.memory.read<8>(pb + 46)) :
                                        std::int64_t(std::bit_cast<std::int32_t>(std::uint32_t(s.memory.read<4>(pb + 46))));
        error = forks.seek(ref, mode, offset);
        if (!found->fork) {
            std::int64_t position = 0;
            const auto position_error = forks.position(ref, position);
            if (!position_error) s.memory.write<4>(pb + 46, std::uint32_t(position));
            else if (!error) error = position_error;
        }
        break;
    }
    }
    const auto return_kind = !found->asynchronous ? ForkIoResult::Return::result :
        found->fork ? ForkIoResult::Return::void_result : ForkIoResult::Return::queued;
    return ForkIoResult{error, found->asynchronous, pb, completion, return_kind};
}
void complete_fork_io(State& s, const ForkIoResult& result) {
    if (result.parameter_block) s.memory.write<2>(result.parameter_block + 16, std::uint16_t(result.error));
    // Local native forks complete immediately. Publish ioResult and actualCount
    // before invoking the original CFM completion routine exactly once. No host
    // worker touches guest memory or re-enters a dispatcher after State dies.
    if (result.asynchronous && result.completion) {
        const std::array arguments{result.parameter_block};
        call_original(s, result.completion, arguments);
    }
}
}
