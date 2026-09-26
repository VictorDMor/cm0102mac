#include "process_bridge.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#include <cmath>
#include <cstring>
#include <stdexcept>

// Marshal the native Process Manager's scalars. Never truncate host pointers or
// pass an original big-endian ProcessInfoRec directly to a modern macOS API.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
namespace cm::aot {
double native_extended80(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != 10) throw std::runtime_error("Extended80 requires exactly ten bytes");
    extended80 value{};
    // x80tod accepts the stored 68K big-endian representation even on ARM64.
    // Assigning host-endian exp/man struct fields would corrupt AIFF rates.
    std::memcpy(&value, bytes.data(), 10);
    return ::x80tod(&value);
}
std::int32_t native_fixed(double value) { return ::X2Fix(value); }
std::uint32_t native_local_date_time() {
    // Classic GetDateTime counts local wall-clock seconds since 1904. Its
    // replacement APIs are documented in the SDK's UTCUtils.h. Convert through
    // an integer before narrowing so the original unsigned 32-bit wrap is defined.
    const auto now = CFAbsoluteTimeGetCurrent();
    const auto zone = CFTimeZoneCopySystem();
    if (!zone) throw std::runtime_error("Cannot obtain the native system time zone");
    const auto offset = CFTimeZoneGetSecondsFromGMT(zone, now);
    CFRelease(zone);
    return std::uint32_t(std::int64_t(std::floor(now + kCFAbsoluteTimeIntervalSince1904 + offset)));
}
std::uint32_t native_tick_count() { return ::TickCount(); }
std::uint64_t native_microseconds() {
    UnsignedWide value{};
    ::Microseconds(&value);
    return (std::uint64_t(value.hi) << 32) | value.lo;
}
NativeMachineLocation native_machine_location() {
    MachineLocation location{};
    ::ReadLocation(&location);
    // The low 24 bits are a signed GMT offset; the high byte is the DST flag.
    // Marshal each scalar, since the native union contains a 64-bit long.
    return {std::uint32_t(location.latitude), std::uint32_t(location.longitude),
            std::uint32_t(location.u.gmtDelta)};
}
NativeHardwareValue native_hardware_value(std::uint32_t selector) {
    // Ask the native OS for the legacy API's hardware scalar, including its
    // real failure/overflow policy. Do not invent a processor or RAM size.
    if (selector != gestaltProcClkSpeed && selector != gestaltPhysicalRAMSize) return {paramErr, 0};
    SInt32 value = 0;
    const auto error = ::Gestalt(selector, &value);
    return {error, std::uint32_t(value)};
}
NativeProcess current_native_process() {
    ProcessSerialNumber psn{};
    NativeProcess result;
    result.error = ::GetCurrentProcess(&psn);
    result.high = psn.highLongOfPSN;
    result.low = psn.lowLongOfPSN;
    return result;
}
NativeProcess native_process_information(std::uint32_t high, std::uint32_t low) {
    ProcessSerialNumber psn{high, low};
    ProcessInfoRec info{};
    info.processInfoLength = sizeof(info);
    NativeProcess result;
    result.error = ::GetProcessInformation(&psn, &info);
    result.high = info.processNumber.highLongOfPSN;
    result.low = info.processNumber.lowLongOfPSN;
    result.type = info.processType;
    result.signature = info.processSignature;
    result.mode = info.processMode;
    result.launcher_high = info.processLauncher.highLongOfPSN;
    result.launcher_low = info.processLauncher.lowLongOfPSN;
    result.launch_date = info.processLaunchDate;
    result.active_time = info.processActiveTime;
    return result;
}
}
#pragma clang diagnostic pop
