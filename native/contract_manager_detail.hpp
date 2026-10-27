#pragma once
#include "contract_manager.hpp"
#include "guest_values.hpp"

namespace cm::aot::contract_manager::detail {
using namespace guest;
namespace slot {
constexpr int person_count = -0x7be4, contract_index = -0x7b9c, pool = -0x5718;
}

// The per-person contract index and the manager's contracts both use
// 100-byte rows. Negative indices denote an absent contract.
struct Contract {
    Cpu& c;
    std::uint32_t address;
    Field<std::int32_t> person_id() const { return {c, address}; }
    Field<std::int32_t> club_id() const { return {c, address + 4}; }
    static constexpr unsigned size = 100;
};
struct Manager {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> records() const { return {c, address}; }
    Field<std::int32_t> capacity() const { return {c, address + 4}; }
    Field<std::int32_t> count() const { return {c, address + 8}; }
};
struct Date {
    Cpu& c;
    std::uint32_t address;
    Field<std::int16_t> day() const { return {c, address}; }
    Field<std::uint16_t> year() const { return {c, address + 2}; }
    Field<std::uint8_t> leap() const { return {c, address + 4}; }
};
struct Staff {
    Cpu& c;
    std::uint32_t address;
    static constexpr unsigned size = 112;
    Field<std::uint32_t> club() const { return {c, address + 0x24}; }
    Field<std::uint32_t> secondary_club() const { return {c, address + 0x20}; }
    Field<std::uint32_t> attributes() const { return {c, address + 0x14}; }
    Field<std::uint32_t> value() const { return {c, address + 8}; }
    Date expiry() const { return {c, address + 0x54}; }
    Date secondary_expiry() const { return {c, address + 0x44}; }
};
struct KeyedRecords {
    Cpu& c;
    std::uint32_t manager;
    static constexpr unsigned size = 24;
    Field<std::uint32_t> data() const { return {c, manager + 0x28}; }
    Field<std::int32_t> count() const { return {c, manager + 0x2c}; }
    Field<std::int32_t> capacity() const { return {c, manager + 0x30}; }
    std::uint32_t append_address() const { return data().get() + std::uint32_t(count().get()) * size; }
};
struct PendingRecord {
    Cpu& c;
    std::uint32_t address;
    Field<std::int32_t> person_id() const { return {c, address}; }
    Field<std::int32_t> club_id() const { return {c, address + 4}; }
    Field<std::uint8_t> kind() const { return {c, address + 0x64}; }
    Field<std::uint32_t> origin() const { return {c, address + 0x70}; }
    Field<std::int32_t> news_id() const { return {c, address + 0x7c}; }
};
struct ScheduledRecords {
    Cpu& c;
    std::uint32_t manager;
    static constexpr unsigned size = 16;
    Field<std::uint32_t> data() const { return {c, manager + 12}; }
    Field<std::int32_t> capacity() const { return {c, manager + 16}; }
    Field<std::int32_t> count() const { return {c, manager + 20}; }
    Field<std::int32_t> cursor() const { return {c, manager + 24}; }
    std::uint32_t append_address() const { return data().get() + std::uint32_t(count().get()) * size; }
};

bool call_rewritten(Cpu&, std::uint32_t entry, std::uint32_t back);
class Program {
public:
    explicit Program(Cpu& cpu) : c(cpu) {}
    std::uint32_t call(std::uint32_t entry, std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments = {}) {
        unsigned n = 3;
        for (auto value : arguments) c.set(n++, value);
        if (!call_rewritten(c, entry, back)) c.call(entry, back);
        return c.r(3);
    }
private:
    Cpu& c;
};
}
