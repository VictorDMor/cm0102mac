#include "transfer_manager_future.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t future_array = 0xa50, future_capacity = 0xa54, future_count = 0xa58,
    future_size = 12, future_flag = 0x10, status_table = 0x9a0, person_size = 0x70,
    shortlist = 0x988, forget_staff = 0x10363b80, next_member = 0x10738e00;

struct Offer {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t list() const { return c.u32(address + 4); }
    std::uint32_t primary() const { return c.u32(address + 0xc); }
    std::uint32_t reference() const { return c.u32(address + 0x28); }
};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

void copy_date(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    c.put16(destination + 2, c.u16(source + 2));
    c.put16(destination, c.u16(source));
    c.put8(destination + 4, c.u8(source + 4));
}

std::uint32_t person_address(Cpu& c, std::uint32_t people, std::uint32_t id) {
    const auto offset = id * person_size;
    c.set(0, offset);
    return c.u32(people) + offset;
}

bool start_members(Cpu& c, const Offer& offer) {
    const auto list = offer.list();
    c.set(3, list);
    if (!nonzero_logical(c, list)) return false;
    const auto head = c.u32(list + 4);
    c.set(0, head);
    c.put32(list + 0xc, head);
    return true;
}

void update_future_flag(Cpu& c, std::uint32_t manager, std::uint32_t id, bool marked) {
    const auto offset = id * 10, table = c.u32(manager + status_table);
    const auto flags = c.u8(table + offset);
    const auto updated = marked ? flags | future_flag : flags & ~future_flag;
    c.set({{0, updated}, {3, offset}, {4, table}});
    c.put8(table + offset, updated);
}
}

void enqueue_future_transfer(Cpu& c, std::uint32_t manager, std::uint32_t offer_address, std::uint32_t date) {
    Frame frame(c, 0x103f2ec0, 0x80, {.first_gpr = 24});
    const Kept<24> body(c, c.toc(-0x79e0));
    const Kept<25> title(c, c.toc(-0x79dc));
    const Kept<26> message(c, c.toc(-0x7ba8));
    const Kept<27> pool(c, c.toc(-0x4b9c));
    const Kept<28> transfers(c, manager);
    const Kept<29> offer_register(c, offer_address);
    Kept<30> source(c, date);
    const Kept<31> people(c, c.toc(-0x7be0));
    const Offer offer{c, offer_register};
    const auto count = c.u32(manager + future_count), capacity = c.u32(manager + future_capacity);
    c.set({{0, count}, {3, capacity}});
    if (compare(c, count, capacity) == 0) {
        c.put32(transfers + future_capacity, capacity + 10);
        const auto previous = c.u32(transfers + future_array);
        c.set(3, previous);
        const bool resize = nonzero_logical(c, previous);
        const auto grown = c.u32(transfers + future_capacity);
        c.set(0, grown);
        const auto length = grown * future_size;
        const auto storage = resize ? call(c, 0x107751b0, 0x103f2f20, {previous, length})
                                    : call(c, 0x10775130, 0x103f2f34, {length});
        c.put32(transfers + future_array, storage);
        c.set(0, c.u32(transfers + future_array));
        if (!nonzero_logical(c, c.r(0))) {
            const auto name = frame.at(0x40), extension = frame.at(0x38);
            call(c, 0x10739890, 0x103f2f5c, {pool + 0x37, 0, 0, name, extension});
            const auto version = call(c, 0x10768810, 0x103f2f60);
            call(c, 0x1077b720, 0x103f2f80, {message, pool + 0x4c, version, name, extension, 0x27c2, pool + 0x1b5a});
            call(c, 0x10727890, 0x103f2f8c, {title, pool + 0xa6});
            call(c, 0x10727890, 0x103f2f9c, {body, pool + 0xbe, message});
            call(c, 0x10769b60, 0x103f2fac, {title, body, 0});
            return;
        }
    }
    const auto comparator = c.toc(-0x4bbc);
    copy_date(c, c.u32(transfers + future_array) + c.u32(transfers + future_count) * future_size, source);
    const auto length = c.u32(transfers + future_count);
    const auto reference = offer.reference(), offset = length * future_size;
    const auto array = c.u32(transfers + future_array);
    c.set({{0, offset}, {7, reference}});
    c.put32(transfers + future_count, length + 1);
    c.put32(array + offset + 8, reference);
    call(c, 0x10768300, 0x103f300c,
         {c.u32(transfers + future_array), c.u32(transfers + future_count), future_size, comparator});
    c.put8(offer.address + 0x35, 1);
    c.put8(offer.address + 0x34, 0x13);
    call(c, forget_staff, 0x103f3038, {c.u32(transfers + shortlist), person_address(c, people, offer.primary()), 0});

    const auto status_offset = offer.primary() * 10, status = c.u32(transfers + status_table);
    const auto ledger = c.toc(-0x7b7c);
    c.set(5, status);
    c.put8(status + status_offset, c.u8(status + status_offset) | future_flag);
    const auto contract = call(c, 0x10082f20, 0x103f306c, {c.u32(ledger), person_address(c, people, offer.primary())});
    if (nonzero_logical(c, contract)) {
        const auto eligible = c.i8(contract + 0x54);
        c.set(0, std::uint32_t(eligible));
        if (nonzero(c, eligible)) {
            const auto key = offer.reference();
            c.set(0, key);
            c.put32(contract + 0x58, key);
        }
    }
    if (!start_members(c, offer)) return;
    for (;;) {
        source = call(c, next_member, 0x103f30dc, {offer.list()});
        if (!nonzero(c, source)) return;
        call(c, forget_staff, 0x103f30bc, {c.u32(transfers + shortlist), person_address(c, people, c.u32(source)), 0});
        update_future_flag(c, transfers, c.u32(source), true);
    }
}

void remove_future_transfer(Cpu& c, std::uint32_t manager, std::uint32_t offer_address) {
    Frame frame(c, 0x103f3540, 0x40, {.first_gpr = 30});
    const Kept<30> transfers(c, manager);
    const Kept<31> offer_register(c, offer_address);
    const Offer offer{c, offer_register};
    update_future_flag(c, transfers, offer.primary(), false);
    if (start_members(c, offer)) {
        for (;;) {
            const auto row = call(c, next_member, 0x103f35b0, {offer.list()});
            if (!nonzero_logical(c, row)) break;
            update_future_flag(c, transfers, c.u32(row), false);
        }
    }
    std::uint32_t offset = 0;
    for (std::int32_t index = 0; ; ++index, offset += future_size) {
        const auto count = c.u32(transfers + future_count);
        c.set({{0, count}, {5, offset}, {6, std::uint32_t(index)}});
        if (compare(c, index, count) >= 0) return;
        const auto array = c.u32(transfers + future_array), reference = offer.reference();
        const auto key = c.u32(array + offset + 8);
        c.set({{0, key}, {3, array}, {4, reference}});
        if (compare(c, key, reference) != 0) continue;
        for (;; ++index, offset += future_size) {
            const auto remaining = c.u32(transfers + future_count) - 1;
            c.set({{0, remaining}, {3, remaining + 1}, {5, offset}, {6, std::uint32_t(index)}});
            if (compare(c, index, remaining) >= 0) {
                c.put32(transfers + future_count, remaining);
                return;
            }
            const auto destination = c.u32(transfers + future_array) + offset, following = destination + future_size;
            c.set(4, following);
            copy_date(c, destination, following);
            c.put32(destination + 8, c.u32(following + 8));
        }
    }
}
}
