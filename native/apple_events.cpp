#include "apple_events.hpp"
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <map>

namespace cm::aot {
struct NativeAppleEvents::Impl {
    std::map<std::uint32_t, AEDesc> descriptors;
    std::uint32_t next_token = 0x51000000;
    ~Impl() { for (auto& [token, descriptor] : descriptors) { (void)token; ::AEDisposeDesc(&descriptor); } }
};
NativeAppleEvents::NativeAppleEvents() : impl_(std::make_unique<Impl>()) {}
NativeAppleEvents::~NativeAppleEvents() = default;
std::int32_t NativeAppleEvents::application_event(std::uint32_t id, NativeAEDescriptor& descriptor) {
    descriptor = {};
    AEAddressDesc target{typeNull, nullptr};
    const ProcessSerialNumber current{0, kCurrentProcess};
    auto error = ::AECreateDesc(typeProcessSerialNumber, &current, sizeof(current), &target);
    if (error) return error;
    AppleEvent event{typeNull, nullptr};
    error = ::AECreateAppleEvent(kCoreEventClass, id, &target, kAutoGenerateReturnID, kAnyTransactionID, &event);
    ::AEDisposeDesc(&target);
    if (!error) {
        descriptor = {event.descriptorType, impl_->next_token++};
        impl_->descriptors.emplace(descriptor.token, event);
    }
    return error;
}
std::int32_t NativeAppleEvents::dispose(NativeAEDescriptor descriptor) {
    if (!descriptor.token && descriptor.type == typeNull) return 0;
    auto found = impl_->descriptors.find(descriptor.token);
    if (found == impl_->descriptors.end() || found->second.descriptorType != descriptor.type) return errAENotAEDesc;
    const auto error = ::AEDisposeDesc(&found->second);
    if (!error) impl_->descriptors.erase(found);
    return error;
}
}
