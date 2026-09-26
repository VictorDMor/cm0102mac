#pragma once
#include <cstdint>
#include <memory>

namespace cm::aot {
struct NativeAEDescriptor {
    std::uint32_t type = 0x6e756c6c, token = 0;
};
class NativeAppleEvents {
public:
    NativeAppleEvents();
    ~NativeAppleEvents();
    std::int32_t application_event(std::uint32_t id, NativeAEDescriptor& descriptor);
    std::int32_t dispose(NativeAEDescriptor descriptor);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
