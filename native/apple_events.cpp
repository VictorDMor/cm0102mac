#include "apple_events.hpp"
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <algorithm>
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
namespace {
// Keeps a native descriptor and gives the guest its type and token.
NativeAEDescriptor keep(std::map<std::uint32_t, AEDesc>& descriptors, std::uint32_t& next, const AEDesc& native) {
    const NativeAEDescriptor descriptor{native.descriptorType, next++};
    descriptors.emplace(descriptor.token, native);
    return descriptor;
}
}
std::int32_t NativeAppleEvents::open_documents_event(const std::vector<std::vector<std::uint8_t>>& specs,
                                                     NativeAEDescriptor& descriptor) {
    descriptor = {};
    AEDescList list{typeNull, nullptr};
    auto error = ::AECreateList(nullptr, 0, false, &list);
    for (const auto& spec : specs)
        if (!error) error = ::AEPutPtr(&list, 0, 'fss ', spec.data(), Size(spec.size()));
    AEAddressDesc target{typeNull, nullptr};
    const ProcessSerialNumber current{0, kCurrentProcess};
    if (!error) error = ::AECreateDesc(typeProcessSerialNumber, &current, sizeof(current), &target);
    AppleEvent event{typeNull, nullptr};
    if (!error) error = ::AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments, &target, kAutoGenerateReturnID,
                                             kAnyTransactionID, &event);
    if (!error) error = ::AEPutParamDesc(&event, keyDirectObject, &list);
    ::AEDisposeDesc(&target);
    ::AEDisposeDesc(&list);
    if (error) { ::AEDisposeDesc(&event); return error; }
    descriptor = keep(impl_->descriptors, impl_->next_token, event);
    return 0;
}
std::int32_t NativeAppleEvents::parameter(NativeAEDescriptor event, std::uint32_t key, std::uint32_t type,
                                          NativeAEDescriptor& result) {
    result = {};
    const auto found = impl_->descriptors.find(event.token);
    if (found == impl_->descriptors.end()) return errAENotAEDesc;
    AEDesc native{typeNull, nullptr};
    const auto error = ::AEGetParamDesc(&found->second, key, type, &native);
    if (!error) result = keep(impl_->descriptors, impl_->next_token, native);
    return error;
}
std::int32_t NativeAppleEvents::count(NativeAEDescriptor list, std::int32_t& items) {
    const auto found = impl_->descriptors.find(list.token);
    if (found == impl_->descriptors.end()) return errAENotAEDesc;
    long count = 0;
    const auto error = ::AECountItems(&found->second, &count);
    items = std::int32_t(count);
    return error;
}
std::int32_t NativeAppleEvents::nth(NativeAEDescriptor list, std::int32_t index, std::uint32_t type,
                                    std::uint32_t& keyword, NativeAEDescriptor& result) {
    result = {};
    const auto found = impl_->descriptors.find(list.token);
    if (found == impl_->descriptors.end()) return errAENotAEDesc;
    AEKeyword key = 0;
    AEDesc native{typeNull, nullptr};
    const auto error = ::AEGetNthDesc(&found->second, index, type, &key, &native);
    keyword = key;
    if (!error) result = keep(impl_->descriptors, impl_->next_token, native);
    return error;
}
std::int32_t NativeAppleEvents::coerce(NativeAEDescriptor descriptor, std::uint32_t type, NativeAEDescriptor& result) {
    result = {};
    const auto found = impl_->descriptors.find(descriptor.token);
    if (found == impl_->descriptors.end()) return errAENotAEDesc;
    AEDesc native{typeNull, nullptr};
    const auto error = ::AECoerceDesc(&found->second, type, &native);
    if (!error) result = keep(impl_->descriptors, impl_->next_token, native);
    return error;
}
std::int32_t NativeAppleEvents::data(NativeAEDescriptor descriptor, std::uint32_t size, std::vector<std::uint8_t>& bytes) {
    const auto found = impl_->descriptors.find(descriptor.token);
    if (found == impl_->descriptors.end()) return errAENotAEDesc;
    bytes.assign(std::min<std::size_t>(size, std::size_t(::AEGetDescDataSize(&found->second))), 0);
    return ::AEGetDescData(&found->second, bytes.data(), Size(bytes.size()));
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
