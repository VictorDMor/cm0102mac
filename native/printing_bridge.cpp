#include "printing_bridge.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <map>
#include <algorithm>

namespace cm::aot {
struct NativePrinting::Impl {
    struct Object { void* value; Kind kind; };
    std::map<std::uint32_t, Object> objects;
    std::uint32_t next_token = 0x50000000;
    ~Impl() { for (const auto& [token, object] : objects) { (void)token; ::PMRelease(object.value); } }
    void* get(std::uint32_t token, Kind kind) const {
        const auto found = objects.find(token);
        return found != objects.end() && found->second.kind == kind ? found->second.value : nullptr;
    }
    std::uint32_t insert(void* object, Kind kind) {
        const auto token = next_token++;
        objects.emplace(token, Object{object, kind});
        return token;
    }
};
NativePrinting::NativePrinting() : impl_(std::make_unique<Impl>()) {}
NativePrinting::~NativePrinting() = default;
std::int32_t NativePrinting::create(Kind kind, std::uint32_t& token) {
    token = 0;
    void* value = nullptr;
    OSStatus error;
    if (kind == Kind::session) {
        PMPrintSession session = nullptr;
        error = ::PMCreateSession(&session); value = session;
    } else if (kind == Kind::page_format) {
        PMPageFormat format = nullptr;
        error = ::PMCreatePageFormat(&format); value = format;
    } else {
        PMPrintSettings settings = nullptr;
        error = ::PMCreatePrintSettings(&settings); value = settings;
    }
    if (!error) token = impl_->insert(value, kind);
    return error;
}
std::int32_t NativePrinting::release(std::uint32_t token) {
    const auto found = impl_->objects.find(token);
    if (found == impl_->objects.end()) return kPMInvalidObject;
    const auto error = ::PMRelease(found->second.value);
    if (!error) impl_->objects.erase(found);
    return error;
}
std::int32_t NativePrinting::default_page_format(std::uint32_t session, std::uint32_t format) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_session || !native_format) return kPMInvalidObject;
    return ::PMSessionDefaultPageFormat(native_session, native_format);
}
std::int32_t NativePrinting::validate_page_format(std::uint32_t session, std::uint32_t format, bool& changed) {
    auto native_session = static_cast<PMPrintSession>(impl_->get(session, Kind::session));
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_session || !native_format) return kPMInvalidObject;
    Boolean native_changed = false;
    const auto error = ::PMSessionValidatePageFormat(native_session, native_format, &native_changed);
    changed = native_changed;
    return error;
}
std::int32_t NativePrinting::flatten(std::uint32_t format, std::vector<std::uint8_t>& bytes) {
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_format) return kPMInvalidObject;
    CFDataRef data = nullptr;
    const auto error = ::PMPageFormatCreateDataRepresentation(native_format, &data, kPMDataFormatXMLDefault);
    if (!error && data) {
        // The original program treats the flattened format as opaque bytes.
        // Identify this port's native representation for subsequent restoration.
        bytes = {'C', 'M', 'P', 'F', 0, 0, 0, 1};
        bytes.insert(bytes.end(), ::CFDataGetBytePtr(data), ::CFDataGetBytePtr(data) + ::CFDataGetLength(data));
    }
    if (data) ::CFRelease(data);
    return error;
}
std::int32_t NativePrinting::unflatten(std::span<const std::uint8_t> bytes, std::uint32_t& token) {
    token = 0;
    constexpr std::array<std::uint8_t, 8> prefix{'C', 'M', 'P', 'F', 0, 0, 0, 1};
    if (bytes.size() < prefix.size() || !std::equal(prefix.begin(), prefix.end(), bytes.begin()))
        return kPMInvalidParameter; // Old flattened formats require a separate conversion.
    CFDataRef data = ::CFDataCreate(nullptr, bytes.data() + prefix.size(), bytes.size() - prefix.size());
    if (!data) return memFullErr;
    PMPageFormat format = nullptr;
    const auto error = ::PMPageFormatCreateWithDataRepresentation(data, &format);
    ::CFRelease(data);
    if (!error) token = impl_->insert(format, Kind::page_format);
    return error;
}
std::int32_t NativePrinting::page_rect(std::uint32_t format, std::array<double, 4>& rect) {
    auto native_format = static_cast<PMPageFormat>(impl_->get(format, Kind::page_format));
    if (!native_format) return kPMInvalidObject;
    PMRect bounds{};
    const auto error = ::PMGetAdjustedPageRect(native_format, &bounds);
    if (!error) rect = {bounds.top, bounds.left, bounds.bottom, bounds.right};
    return error;
}
}
