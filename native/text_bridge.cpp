#include "text_bridge.hpp"
#include <CoreServices/CoreServices.h>
#include <cstring>
#include <stdexcept>
#include <array>

// These APIs remain implemented for native ARM64 in CarbonCore. Using the
// vendor's byte-oriented Munger preserves its search/replace edge semantics.
// Only native temporary handles reach the OS; original 32-bit handles do not.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
std::int32_t native_region_code() { return ::GetScriptManagerVariable(smRegionCode); }
void native_uppercase(std::vector<std::uint8_t>& bytes, std::int16_t script) {
    if (script == smSystemScript) script = std::int16_t(::GetScriptManagerVariable(smSysScript));
    if (script != smRoman) throw std::runtime_error("Original uppercase conversion requires an additional script mapping");
    // UppercaseText is unavailable on ARM64. Its in-place Roman conversion
    // cannot expand strings or change their byte count. Use the SDK-recommended
    // CFStringUppercase, retaining characters without a single-byte uppercase
    // counterpart (for example sharp-s and the fi/fl ligatures).
    static const auto uppercase = [] {
        std::array<std::uint8_t, 256> table{};
        for (unsigned i = 0; i < table.size(); ++i) {
            table[i] = i;
            const auto original = std::uint8_t(i);
            auto source = CFStringCreateWithBytes(nullptr, &original, 1, kCFStringEncodingMacRoman, false);
            auto transformed = source ? CFStringCreateMutableCopy(nullptr, 0, source) : nullptr;
            if (source) CFRelease(source);
            if (!transformed) throw std::runtime_error("Cannot create native Roman uppercase mapping");
            CFStringUppercase(transformed, nullptr);
            std::uint8_t encoded[8]{};
            CFIndex written = 0;
            const auto count = CFStringGetLength(transformed);
            const auto converted = CFStringGetBytes(transformed, CFRangeMake(0, count), kCFStringEncodingMacRoman,
                                                     0, false, encoded, sizeof(encoded), &written);
            if (converted == count && written == 1) table[i] = encoded[0];
            CFRelease(transformed);
        }
        return table;
    }();
    for (auto& byte : bytes) byte = uppercase[byte];
}
MungerResult native_munger(const std::vector<std::uint8_t>& bytes, std::int32_t offset,
    const std::optional<std::vector<std::uint8_t>>& needle, std::int32_t needle_length,
    const std::optional<std::vector<std::uint8_t>>& replacement, std::int32_t replacement_length) {
    struct NativeHandle {
        Handle value;
        ~NativeHandle() { if (value) ::DisposeHandle(value); }
    } handle{::NewHandle(bytes.size())};
    if (!handle.value) return {-1, ::MemError(), bytes};
    if (!bytes.empty()) std::memcpy(*handle.value, bytes.data(), bytes.size());
    char empty = 0;
    const void* a = needle ? needle->empty() ? &empty : static_cast<const void*>(needle->data()) : nullptr;
    const void* b = replacement ? replacement->empty() ? &empty : static_cast<const void*>(replacement->data()) : nullptr;
    const auto result = ::Munger(handle.value, offset, a, needle_length, b, replacement_length);
    const auto error = ::MemError();
    if (error) return {std::int32_t(result), error, bytes};
    const auto size = ::GetHandleSize(handle.value);
    if (size < 0) throw std::runtime_error("Native Munger returned an invalid handle size");
    std::vector<std::uint8_t> output(size);
    if (size) std::memcpy(output.data(), *handle.value, size);
    return {std::int32_t(result), error, std::move(output)};
}
}
#pragma clang diagnostic pop
