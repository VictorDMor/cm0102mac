#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace cm::aot {
struct NativeAEDescriptor {
    std::uint32_t type = 0x6e756c6c, token = 0;
};
class NativeAppleEvents {
public:
    NativeAppleEvents();
    ~NativeAppleEvents();
    std::int32_t application_event(std::uint32_t id, NativeAEDescriptor& descriptor);
    // An open- or print-documents event ('odoc', 'pdoc') whose direct object
    // lists the documents, each an FSSpec ('fss ', 70 bytes) as the original
    // reads it.
    std::int32_t documents_event(std::uint32_t id, const std::vector<std::vector<std::uint8_t>>& specs,
                                 NativeAEDescriptor& descriptor);
    std::int32_t dispose(NativeAEDescriptor descriptor);
    // An event's id ('odoc', 'pdoc', ...), 0 when the token names no event.
    std::uint32_t event_id(NativeAEDescriptor event) const;
    // The Apple Event Manager calls the original makes on them.
    std::int32_t parameter(NativeAEDescriptor event, std::uint32_t key, std::uint32_t type, NativeAEDescriptor& result);
    std::int32_t count(NativeAEDescriptor list, std::int32_t& items);
    std::int32_t nth(NativeAEDescriptor list, std::int32_t index, std::uint32_t type, std::uint32_t& keyword,
                     NativeAEDescriptor& result);
    std::int32_t coerce(NativeAEDescriptor descriptor, std::uint32_t type, NativeAEDescriptor& result);
    std::int32_t data(NativeAEDescriptor descriptor, std::uint32_t size, std::vector<std::uint8_t>& bytes);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
