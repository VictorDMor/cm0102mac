#pragma once
#include <cstdint>
#include <optional>
#include <string_view>

namespace cm::aot {
struct State;
class NativeDataForks;
// Native storage remains owned by NativeDataForks. These functions marshal the
// PowerPC ABI; they never overlay pointer-bearing SDK structures on guest data.
std::int16_t read_guest_fork(State&, NativeDataForks&, std::int16_t ref,
    std::uint16_t mode, std::int64_t offset, std::uint32_t buffer,
    std::int64_t requested, std::uint32_t& actual);
std::int16_t write_guest_fork(State&, NativeDataForks&, std::int16_t ref,
    std::uint16_t mode, std::int64_t offset, std::uint32_t buffer,
    std::int64_t requested, std::uint32_t& actual);
struct ForkIoResult {
    std::int16_t error;
    bool asynchronous;
    std::uint32_t parameter_block, completion;
    enum class Return { result, queued, void_result } return_kind;
};
std::optional<ForkIoResult> dispatch_fork_io(State&, NativeDataForks&, std::string_view);
void complete_fork_io(State&, const ForkIoResult&);
}
