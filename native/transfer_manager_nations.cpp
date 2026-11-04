#include "transfer_manager_nations.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

struct NationRecord {
    std::uint32_t id, constructor;
    std::int32_t nation_slot;
};
constexpr std::array nations{
    NationRecord{0, 0x105bf320, -0x7a70}, NationRecord{1, 0x105bd570, -0x7508},
    NationRecord{2, 0x106bcb30, -0x7650}, NationRecord{3, 0x105bdc00, -0x7510},
    NationRecord{4, 0x105be6d0, -0x7b68}, NationRecord{5, 0x106bce80, -0x7590},
    NationRecord{6, 0x105bea70, -0x750c}, NationRecord{7, 0x105beb80, -0x7a10},
    NationRecord{8, 0x106bd110, -0x7594}, NationRecord{9, 0x105bf550, -0x7a00},
    NationRecord{10, 0x105c0300, -0x7af4}, NationRecord{11, 0x106bd5c0, -0x7598},
    NationRecord{12, 0x105c0d70, -0x7a28}, NationRecord{13, 0x106bd8f0, -0x7574},
    NationRecord{14, 0x105c10a0, -0x7a14}, NationRecord{15, 0x105c1d00, -0x7a24},
    NationRecord{17, 0x105c2340, -0x7504}, NationRecord{16, 0x106bdfb0, -0x7578},
    NationRecord{18, 0x106be3a0, -0x75a8}, NationRecord{19, 0x105c2a40, -0x79fc},
    NationRecord{20, 0x106be640, -0x758c}, NationRecord{21, 0x105c3260, -0x7a1c},
    NationRecord{22, 0x105c37d0, -0x7a18}, NationRecord{23, 0x105c4080, -0x7514},
    NationRecord{24, 0x106be9d0, -0x75b0}, NationRecord{25, 0x105e0240, -0x7a20},
    NationRecord{26, 0x106bedf0, -0x757c},
};
constexpr std::uint32_t nation_records = 0x9e0, date_index = 0x9d8, date_count = 0x9dc;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

void count_date_slots(Cpu& c, std::uint32_t manager) {
    c.put32(manager + date_count, 0);
    for (std::uint32_t index = 0, offset = 0; ; ++index, offset += 4) {
        c.set({{0, index}, {4, offset}, {5, index}});
        if (compare(c, index, nations.size()) >= 0) return;
        const auto record = c.u32(manager + nation_records + offset);
        c.set(3, record);
        if (nonzero_logical(c, record)) {
            const auto count = c.u32(manager + date_count);
            c.set(3, count);
            c.put32(manager + date_count, count + std::uint32_t(c.i8(record + 8)) * 2);
        }
    }
}

void index_dates(Cpu& c, std::uint32_t manager, Kept<30>& out) {
    for (std::uint32_t index = 0, offset = 0; ; ++index, offset += 4) {
        c.set({{0, index}, {6, offset}, {8, index}});
        if (compare(c, index, nations.size()) >= 0) return;
        const auto slot = nation_records + offset;
        const auto entry = c.u32(manager + slot);
        c.set({{0, entry}, {7, slot}});
        if (!nonzero_logical(c, entry)) continue;
        for (std::uint32_t group = 0, row_offset = 0; ; ++group, row_offset += 12) {
            const auto row = c.u32(manager + slot);
            const auto count = c.i8(row + 8);
            c.set({{0, std::uint32_t(count)}, {3, group}, {4, row}, {5, row_offset}, {9, group}});
            if (compare(c, group, count) >= 0) break;
            const auto target = c.u32(manager + date_index);
            c.put32(target + out, c.u32(row + 4) + row_offset);
            out = out + 4;
            const auto current = c.u32(manager + slot);
            c.put32(c.u32(manager + date_index) + out, c.u32(current + 4) + row_offset + 6);
            out = out + 4;
        }
    }
}
}

void initialize_nation_transfer_records(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x103fd420, 0x80, {.first_gpr = 25});
    c.set(0, c.lr());
    const Kept<31> transfers(c, manager);
    const Kept<28> body(c, c.toc(-0x79e0));
    const Kept<27> title(c, c.toc(-0x79dc));
    const Kept<26> message(c, c.toc(-0x7ba8));
    const Kept<29> pool(c, c.toc(-0x4b9c));
    Kept<30> date_cursor(c, 0);
    Kept<25> object(c, c.r(25));
    for (std::uint32_t index = 0; index < nations.size(); ++index) {
        const auto& spec = nations[index];
        const auto return_offset = index * 0x30;
        object = call(c, 0x10773520, 0x103fd450 + return_offset, {0x18});
        if (nonzero(c, object)) {
            const auto nation = c.u32(c.toc(spec.nation_slot));
            const auto back = 0x103fd474 + return_offset;
            if (spec.id == 0) call(c, spec.constructor, back, {object, 0, 0xffffffff, nation, 2, 0});
            else call(c, spec.constructor, back, {object, spec.id, nation, 0xffffffff, 1, 0});
        }
        c.put32(transfers + nation_records + spec.id * 4, object);
    }
    count_date_slots(c, transfers);
    const auto length = c.u32(transfers + date_count);
    c.set(0, length);
    const auto storage = call(c, 0x10775130, 0x103fd9b0, {length * 4});
    c.put32(transfers + date_index, storage);
    c.set(0, c.u32(transfers + date_index));
    if (!nonzero_logical(c, c.r(0))) {
        const auto name = frame.at(0x40), extension = frame.at(0x38);
        call(c, 0x10739890, 0x103fd9d8, {pool + 0x37, 0, 0, name, extension});
        const auto version = call(c, 0x10768810, 0x103fd9dc);
        call(c, 0x1077b720, 0x103fd9fc,
             {message, pool + 0x4c, version, name, extension, 0x3bdd, pool + 0x23f2});
        call(c, 0x10727890, 0x103fda08, {title, pool + 0xa6});
        call(c, 0x10727890, 0x103fda18, {body, pool + 0xbe, message});
        call(c, 0x10769b60, 0x103fda28, {title, body, 0});
        return;
    }
    index_dates(c, transfers, date_cursor);
    call(c, 0x10768300, 0x103fdacc,
         {c.u32(transfers + date_index), c.u32(transfers + date_count), 4, c.toc(-0x4bcc)});
}
}
