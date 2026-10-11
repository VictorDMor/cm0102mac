#include "unicode_bridge.hpp"
#include "aot_runtime.hpp"
#include <CoreServices/CoreServices.h>
#include <map>

namespace cm::aot {
namespace {
// Validate extents before allocating native buffers. A bad guest output must
// not turn into a giant host allocation or be truncated to a valid address.
void guest_extent(const State& s, std::uint32_t at, std::uint64_t size) {
    if (!size) return;
    const auto end = std::uint64_t(at) + size;
    if (end > (UINT64_C(1) << 32)) throw std::runtime_error("Unicode conversion exceeds word32 guest extent");
    auto position = std::uint64_t(at);
    for (const auto& [start, length] : s.memory.mapped_ranges()) {
        if (start > position) break;
        if (std::uint64_t(start) + length > position) position = std::uint64_t(start) + length;
        if (position >= end) return;
    }
    throw std::runtime_error("Unicode conversion exceeds mapped guest memory");
}
}
struct NativeUnicode::Impl {
    std::uint32_t next = 0x5b000000;
    std::map<std::uint32_t, TextToUnicodeInfo> contexts;
    ~Impl() { for (auto& [ticket, context] : contexts) { (void)ticket; ::DisposeTextToUnicodeInfo(&context); } }
};
NativeUnicode::NativeUnicode() : impl_(std::make_unique<Impl>()) {}
NativeUnicode::~NativeUnicode() = default;
std::optional<std::int32_t> NativeUnicode::dispatch(State& s, std::string_view name) {
    auto arg = [&](unsigned n) { return std::uint32_t(s.r<4>((n + 3) * 4)); };
    if (name == "CarbonLib::UpgradeScriptInfoToTextEncoding") {
        std::vector<std::uint8_t> font;
        if (arg(3)) font = s.memory.copy_out(arg(3), 1 + s.memory.read<1>(arg(3)));
        TextEncoding encoding = 0;
        const auto error = ::UpgradeScriptInfoToTextEncoding(std::int16_t(arg(0)), std::int16_t(arg(1)),
            std::int16_t(arg(2)), font.empty() ? nullptr : font.data(), &encoding);
        if (!error) s.memory.write<4>(arg(4), encoding);
        return error;
    }
    if (name == "CarbonLib::CreateTextToUnicodeInfoByEncoding") {
        TextToUnicodeInfo context = nullptr;
        const auto error = ::CreateTextToUnicodeInfoByEncoding(arg(0), &context);
        if (error) { if (context) ::DisposeTextToUnicodeInfo(&context); return error; }
        // Publishing the ticket is the only guest effect. If it faults, release
        // the just-created native object rather than retaining an unreachable one.
        const auto ticket = impl_->next++;
        try { s.memory.write<4>(arg(1), ticket); }
        catch (...) { ::DisposeTextToUnicodeInfo(&context); throw; }
        impl_->contexts.emplace(ticket, context);
        return error;
    }
    if (name == "CarbonLib::DisposeTextToUnicodeInfo") {
        const auto pointer = arg(0), ticket = std::uint32_t(s.memory.read<4>(pointer));
        const auto found = impl_->contexts.find(ticket);
        if (found == impl_->contexts.end()) return paramErr;
        const auto error = ::DisposeTextToUnicodeInfo(&found->second);
        if (!error) { impl_->contexts.erase(found); s.memory.write<4>(pointer, 0); }
        return error;
    }
    if (name != "CarbonLib::ConvertFromTextToUnicode") return std::nullopt;
    const auto found = impl_->contexts.find(arg(0));
    if (found == impl_->contexts.end()) return paramErr;
    // Arguments 9--12 are in the PPC stack parameter area (+56..+68).
    const auto stack = s.r<4>(4);
    const auto capacity = std::uint32_t(s.memory.read<4>(stack + 56));
    const auto source_read_pointer = std::uint32_t(s.memory.read<4>(stack + 60));
    const auto output_size_pointer = std::uint32_t(s.memory.read<4>(stack + 64));
    const auto output_pointer = std::uint32_t(s.memory.read<4>(stack + 68));
    guest_extent(s, arg(2), arg(1)); guest_extent(s, output_pointer, capacity);
    const auto source = s.memory.copy_out(arg(2), arg(1));
    const auto offset_count = arg(4);
    if (offset_count && !arg(5)) return paramErr;
    if (arg(5)) guest_extent(s, arg(5), std::uint64_t(offset_count) * 4);
    if (arg(7)) guest_extent(s, arg(7), std::uint64_t(offset_count) * 4);
    std::vector<ByteOffset> input_offsets(offset_count), output_offsets(offset_count);
    for (std::uint64_t n = 0; n < offset_count; ++n) input_offsets[n] = s.memory.read<4>(std::uint64_t(arg(5)) + n * 4);
    std::vector<UniChar> output((std::uint64_t(capacity) + 1) / 2);
    ByteCount source_read = 0, output_size = 0;
    ItemCount offsets_written = 0;
    std::uint8_t empty_source = 0;
    UniChar empty_output = 0;
    const auto error = ::ConvertFromTextToUnicode(found->second, source.size(),
        source.empty() ? &empty_source : source.data(), arg(3), offset_count,
        arg(5) ? input_offsets.data() : nullptr, arg(6) ? &offsets_written : nullptr,
        arg(7) ? output_offsets.data() : nullptr, capacity, &source_read, &output_size,
        output.empty() ? &empty_output : output.data());
    if (source_read > source.size() || output_size > capacity || output_size % 2 || offsets_written > offset_count)
        throw std::runtime_error("Native Unicode converter returned an invalid output extent");
    for (unsigned n = 0; n < output_size / 2; ++n) s.memory.write<2>(std::uint64_t(output_pointer) + n * 2, output[n]);
    if (source_read_pointer) s.memory.write<4>(source_read_pointer, source_read);
    if (output_size_pointer) s.memory.write<4>(output_size_pointer, output_size);
    if (arg(6)) s.memory.write<4>(arg(6), offsets_written);
    if (arg(7)) for (unsigned n = 0; n < offsets_written; ++n)
        s.memory.write<4>(std::uint64_t(arg(7)) + n * 4, output_offsets[n]);
    return error;
}
}
