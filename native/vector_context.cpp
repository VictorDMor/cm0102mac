#include "vector_context.hpp"
#include <algorithm>

namespace cm::aot {
Vector128 Memory::read_vector(std::uint64_t address) const {
    check(address, 16);
    if (read_trace) read_trace->emplace_back(std::uint32_t(address), 16);
    if (date_observer) date_observer->read(std::uint32_t(address), data_ + address, 16);
    Vector128 value;
    std::copy_n(data_ + address, value.size(), value.begin());
    return value;
}
void Memory::write_vector(std::uint64_t address, const Vector128& value) {
    // Check the entire access before changing any guest byte or observer.
    check(address, value.size());
    std::copy(value.begin(), value.end(), data_ + address);
    if (debug_attribute_lock_) apply_debug_attribute_lock(std::uint32_t(address), value.size());
    if (observer) observer->written(std::uint32_t(address), data_ + address, 16);
    if (date_observer) date_observer->written(std::uint32_t(address), data_ + address, 16);
}
namespace {
template<std::size_t N>
Vector128 read_storage(const std::array<std::uint8_t, N>& storage, std::size_t offset) {
    if (offset > N || 16 > N - offset) throw std::out_of_range("Vector storage offset");
    Vector128 result;
    std::copy_n(storage.data() + offset, 16, result.begin());
    return result;
}
template<std::size_t N>
void write_storage(std::array<std::uint8_t, N>& storage, std::size_t offset, const Vector128& value) {
    if (offset > N || 16 > N - offset) throw std::out_of_range("Vector storage offset");
    std::copy(value.begin(), value.end(), storage.data() + offset);
}
}
Vector128 State::read_vector_register(std::size_t offset) const { return read_storage(registers, offset); }
void State::write_vector_register(std::size_t offset, const Vector128& value) { write_storage(registers, offset, value); }
Vector128 State::read_vector_temporary(std::size_t offset) const { return read_storage(unique, offset); }
void State::write_vector_temporary(std::size_t offset, const Vector128& value) { write_storage(unique, offset, value); }
Vector128 load_vector_word(State& state, std::uint32_t base, std::uint32_t index) {
    const auto address = (base + index) & ~std::uint32_t(3);
    const auto word = state.memory.read<4>(address);
    // The ISA leaves other lanes boundedly undefined. Choose zero consistently;
    // retain this choice in both the generated reference and semantic contexts.
    Vector128 value{};
    write_be<4>(value.data() + (address & 12), word);
    return value;
}
std::uint32_t vector_word_for_address(const Vector128& value, std::uint32_t address) {
    return std::uint32_t(read_be<4>(value.data() + (address & 12)));
}
Vector128 vector_status_register(const State& state) {
    Vector128 value{};
    write_be<4>(value.data() + 12, state.r<4>(vector_status_offset) & 0x10001);
    return value;
}
void move_to_vector_status(State& state, const Vector128& value) {
    state.wr<4>(vector_status_offset, read_be<4>(value.data() + 12) & 0x10001);
}
}
