#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace cm::aot {
struct State;
// Conversion contexts are native owned objects. Only process-local, word32
// tickets cross the PowerPC ABI; SDK pointers never enter guest memory.
class NativeUnicode {
public:
    NativeUnicode();
    ~NativeUnicode();
    std::optional<std::int32_t> dispatch(State&, std::string_view);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
