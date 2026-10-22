#include "aot_runtime.hpp"
#include "platform.hpp"
#include "decimal_bridge.hpp"
#include "decimal_input_bridge.hpp"
#include "application_exit.hpp"
#include "import_bridge.hpp"
#include <algorithm>

namespace cm::aot {
std::uint32_t allocate(State& s, std::uint32_t size, bool zero) {
    constexpr std::uint32_t maximum = 0x10000000;
    if (size > maximum) { s.memory_error = -108; return 0; }
    const auto reserved = (std::max(size, 1u) + 15u) & ~15u;
    std::uint32_t address = 0;
    for (auto it = s.available.begin(); it != s.available.end(); ++it) {
        if (it->second < reserved) continue;
        address = it->first;
        const auto remainder = it->second - reserved;
        s.available.erase(it);
        if (remainder) s.available.emplace(address + reserved, remainder);
        break;
    }
    if (!address) {
        if (s.heap_end > 0x30000000 - reserved) { s.memory_error = -108; return 0; }
        address = s.heap_end;
        s.memory.map(address, reserved);
        s.heap_end += reserved;
    }
    s.allocated.emplace(address, size);
    if (zero) s.memory.clear(address, reserved);
    s.memory_error = 0;
    return address;
}
void deallocate(State& s, std::uint32_t address) {
    if (!address) { s.memory_error = 0; return; }
    auto allocated = s.allocated.find(address);
    if (allocated == s.allocated.end()) throw std::runtime_error("Invalid or double DisposePtr");
    auto size = (std::max(allocated->second, 1u) + 15u) & ~15u;
    s.allocated.erase(allocated);
    auto [it, inserted] = s.available.emplace(address, size);
    if (!inserted) throw std::runtime_error("Duplicate free interval");
    if (it != s.available.begin()) {
        auto previous = std::prev(it);
        if (previous->first + previous->second == it->first) {
            previous->second += it->second;
            s.available.erase(it);
            it = previous;
        }
    }
    auto next = std::next(it);
    if (next != s.available.end() && it->first + it->second == next->first) {
        it->second += next->second;
        s.available.erase(next);
    }
    s.memory_error = 0;
}
bool bridge_import(State& s, const std::string& name) { return bridge_import_name(s, name); }
bool bridge_import_name(State& s, std::string_view name) {
    auto arg = [&](unsigned index) { return std::uint32_t(s.r<4>((index + 3) * 4)); };
    auto result = [&](std::uint32_t value) { s.wr<4>(12, value); };
    if (name == "CarbonLib::ExitToShell") {
        throw ApplicationExit{};
    } else if (name == "CarbonLib::Gestalt") {
        if (arg(0) == 0x7174696d) {
            // This native port does not provide the legacy QuickTime component
            // API. Let the original availability check take its absent path.
            result(std::uint32_t(-5551)); // gestaltUndefSelectorErr
            s.pc = std::uint32_t(s.r<4>(4128));
            return true;
        }
        if (arg(0) != 0x70706366 && arg(0) != 0x74747363)
            return s.platform && s.platform->dispatch(s, name); // Platform-specific selectors.
        // No optional PowerPC/AltiVec intrinsics are provided by the compiler.
        // This selects the original scalar path; it is not a host CPU claim.
        // The bridge also does not provide the optional Speech Manager ('ttsc').
        s.memory.write<4>(arg(1), 0);
        result(0);
    } else if (name == "CarbonLib::NewPtr" || name == "CarbonLib::NewPtrClear") {
        result(allocate(s, arg(0), name == "CarbonLib::NewPtrClear"));
    } else if (name == "CarbonLib::DisposePtr") {
        deallocate(s, arg(0));
    } else if (name == "CarbonLib::BlockMoveData") {
        if (arg(2)) s.memory.copy_in(arg(1), s.memory.copy_out(arg(0), arg(2)));
    } else if (name == "CarbonLib::MemError") {
        result(std::uint32_t(std::int32_t(s.memory_error)));
    } else if (name == "CarbonLib::num2dec") {
        original_num2dec(s);
    } else if (name == "CarbonLib::dec2num") {
        original_dec2num(s);
    } else if (name == "CarbonLib::nan") {
        original_nan(s);
    } else if (name == "CarbonLib::sin" || name == "CarbonLib::cos" || name == "CarbonLib::atan" ||
               name == "CarbonLib::atan2" || name == "CarbonLib::sqrt" || name == "CarbonLib::pow") {
        // The six imported libm functions use f1/f2 double arguments and f1
        // results. Do not take their values from the integer argument registers.
        const auto a = float_value<8>(s.r<8>(16400)), b = float_value<8>(s.r<8>(16416));
        const auto value = name == "CarbonLib::sin" ? std::sin(a) : name == "CarbonLib::cos" ? std::cos(a) :
            name == "CarbonLib::atan" ? std::atan(a) : name == "CarbonLib::atan2" ? std::atan2(a, b) :
            name == "CarbonLib::sqrt" ? std::sqrt(a) : std::pow(a, b);
        s.wr<8>(16400, float_bits<8>(value));
    } else {
        return s.platform && s.platform->dispatch(s, name);
    }
    s.pc = std::uint32_t(s.r<4>(4128));
    return true;
}
}
