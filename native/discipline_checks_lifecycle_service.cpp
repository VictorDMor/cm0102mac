// The lifecycle block's scripted services (discipline_checks_lifecycle_service.hpp).
#include "discipline_checks_lifecycle_service.hpp"

#include "code_fragment_adapter.hpp"
#include <string_view>
namespace cm::aot::discipline_checks::lifecycle {
namespace {
using guest::Cpu;
namespace word {  // the control block
constexpr std::uint32_t log_count = 0, objects = 4, specs = 8, phase = 12, items_read = 16, items_written = 20,
                        body_transfers = 24, spec_active = 0x1c, spec_failures = 0x20;
}
constexpr std::uint32_t history_name = 0x10746280, history_construct = 0x10748bb0,
                        object_allocate = 0x10773520, allocate = 0x10775130, open = 0x107475b0, read = 0x10747a80,
                        write = 0x10747cc0, close = 0x10747900;
constexpr int not_found = -43;
constexpr std::size_t log_limit = 0x300;

enum Kind : std::uint32_t { spec_kind = 1, name_kind, open_kind, read_kind, write_kind, close_kind, history_kind };

std::uint32_t bump(Cpu& c, std::uint32_t at) {
    c.put32(at, c.u32(at) + 1);
    return c.u32(at);
}
void log(Cpu& c, Kind kind, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e, std::uint32_t f, std::uint32_t g) {
    const auto n = c.u32(control + word::log_count);
    c.put32(control + word::log_count, n + 1);
    if (n >= log_limit) return;
    const auto at = log_base + n * 0x20;
    for (const auto& [offset, value] : {std::pair<std::uint32_t, std::uint32_t>{0, kind}, {4, a}, {8, b}, {12, d}, {16, e}, {20, f}, {24, g}})
        c.put32(at + offset, value);
}
bool in_module(const Cpu& c) { return c.lr() > 0x100de410 && c.lr() <= 0x100e92d0; }
std::uint8_t pattern(std::uint32_t at, std::uint32_t salt) { return std::uint8_t(at * 131 + salt * 17 + 7); }
// A checksum of what a write receives (the first 0x4000 bytes, and the length).
std::uint32_t checksum(const Cpu& c, std::uint32_t at, std::uint32_t length) {
    std::uint32_t hash = 2166136261u ^ length;
    for (std::uint32_t i = 0; i < std::min<std::uint32_t>(length, 0x4000); ++i) hash = (hash ^ c.u8(at + i)) * 16777619u;
    return hash;
}
void fill(Cpu& c, std::uint32_t at, std::uint32_t length, std::uint32_t salt) {
    for (std::uint32_t i = 0; i < std::min<std::uint32_t>(length, 0x400); ++i) c.put8(at + i, pattern(i, salt));
}

// The part of the file a transfer is, told by the destination (or source) and the shape of the request.
Role role_of(const Cpu& c, const Script& s, std::uint32_t at, std::uint32_t size, std::uint32_t count) {
    const auto phase = c.u32(control + word::phase);
    if (size == 0x2c && count == s.people && at == c.u32(s.manager + 0x14)) return Role::table;
    if (size == 2 && count == 1 && at == s.manager + 0x0a) return Role::record_count;
    if (size == 2 && count == 1 && at == s.manager + 0x12) return Role::index_count;
    if (size == 2 && count == 1 && phase == 2) return Role::pending_count;
    if (size == 0x18 && at == c.u32(s.manager + 4) && phase == 1) return Role::records;
    if (size == 8 && at == c.u32(s.manager + 0xc) && phase == 2) return Role::index;
    if (size == 0x10 && count == 1 && phase == 3) return Role::item;
    return Role::body;
}
// The file's parts come in order: the table, the bodies' own, the record count (phase 1), the records, the index
// count (2), the index, the pending count (3), the pending items.
void advance(Cpu& c, Role role) {
    if (role == Role::record_count) c.put32(control + word::phase, 1);
    if (role == Role::index_count) c.put32(control + word::phase, 2);
    if (role == Role::pending_count) c.put32(control + word::phase, 3);
}
// A load: the count of items "read" and the bytes put in the destination.
std::uint32_t serve_read(Cpu& c, const Script& s, std::uint32_t at, std::uint32_t size, std::uint32_t count) {
    const auto role = role_of(c, s, at, size, count);
    advance(c, role);
    std::uint32_t result = count;
    switch (role) {
    case Role::table: fill(c, at, size * count, 1); if (s.fail == role) result = count - 1; break;
    case Role::record_count: c.put16(at, std::uint16_t(s.records)); if (s.fail == role) result = 0; break;
    case Role::records: fill(c, at, size * count, 2); if (s.fail == role) result = count - 1; break;
    case Role::index_count: c.put16(at, std::uint16_t(s.index)); if (s.fail == role) result = 0; break;
    case Role::index: fill(c, at, size * count, 3); if (s.fail == role) result = count - 1; break;
    case Role::pending_count: c.put16(at, std::uint16_t(s.pending)); if (s.fail == role) result = 0; break;
    case Role::item: {
        const auto n = c.u32(control + word::items_read);
        c.put32(control + word::items_read, n + 1);
        fill(c, at, 0x10, 4 + n);
        if (s.fail == role && n == s.fail_item) result = 0;
        break;
    }
    case Role::body:
        for (std::uint32_t i = 0; i < std::min<std::uint32_t>(size * count, 0x1000); ++i) c.put8(at + i, 0);
        if (s.fail == role && bump(c, control + word::body_transfers) == s.fail_body) result = count - 1;
        break;
    case Role::none: break;
    }
    log(c, read_kind, at, size, count, c.r(6), std::uint32_t(role), result);
    return result;
}
std::uint32_t serve_write(Cpu& c, const Script& s, std::uint32_t at, std::uint32_t size, std::uint32_t count) {
    const auto role = role_of(c, s, at, size, count);
    advance(c, role);
    std::uint32_t result = count;
    switch (role) {
    case Role::table: case Role::records: case Role::index:
        if (s.fail == role) result = count - 1;
        break;
    case Role::record_count: case Role::pending_count: case Role::index_count:
        if (s.fail == role) result = 0;
        break;
    case Role::item: {
        const auto n = c.u32(control + word::items_written);
        c.put32(control + word::items_written, n + 1);
        if (s.fail == role && n == s.fail_item) result = 0;
        break;
    }
    case Role::body:
        if (s.fail == role && bump(c, control + word::body_transfers) == s.fail_body) result = count - 1;
        break;
    case Role::none: break;
    }
    log(c, write_kind, at, size, count, c.r(6), checksum(c, at, size * count) ^ std::uint32_t(role), result);
    return result;
}

// FSMakeFSSpec is an import the generated code reaches through its import descriptor, not through the dispatcher,
// so a case answers it where every import ends: a fixture on the State, active only while a case's control block
// says so. It keeps the folder's Pascal name (up to 4 lookups: its bytes are what the lookup receives) and
// answers "not found" for the first `spec_failures` lookups.
struct FolderLookup final : CodeFragmentBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) != "CarbonLib::FSMakeFSSpec") return false;
        Cpu c(s);
        if (c.u32(control + word::spec_active) == 0) return false;
        const auto n = c.u32(control + word::specs);
        c.put32(control + word::specs, n + 1);
        const auto folder = c.r(5);
        if (n < 4)
            for (std::uint32_t i = 0; i <= c.u8(folder); ++i) c.put8(name_log + n * 0x100 + i, c.u8(folder + i));
        const auto answer = n < c.u32(control + word::spec_failures) ? std::uint32_t(not_found) : 0;
        log(c, spec_kind, c.r(3), c.r(4), c.r(6), c.u8(folder), 0, answer);
        // The glue has saved the caller's TOC at 0x14(sp) and loaded the import's: nothing to do here.
        c.set(3, answer);
        s.pc = c.lr();
        return true;
    }
};
FolderLookup folder_lookup;
}

void install(const Scenario& scenario) { scenario.s.code_fragment_observer = &folder_lookup; }

std::vector<Write> memory(const Script& script) {
    return {{4, control + word::spec_active, 1}, {4, control + word::spec_failures, script.spec_failures}};
}

std::function<bool(Cpu&, std::uint32_t)> service(Script script) {
    return [script](Cpu& c, std::uint32_t entry) {
        switch (entry) {
        case history_name:
            log(c, name_kind, c.r(3), c.r(4), 0, 0, 0, 0);
            c.set(3, scratch_base + 0x50000);
            return true;
        case history_construct:
            log(c, history_kind, c.r(3), c.r(4), c.r(5), 0, 0, 0);
            return true;
        case object_allocate:
            if (script.fail_object != 0 && in_module(c) && (script.fail_object == every_object || bump(c, control + word::objects) == script.fail_object)) {
                c.set(3, 0);
                return true;
            }
            return false;
        case allocate:
            if (script.fail_size != 0 && in_module(c) && c.r(3) == script.fail_size) {
                c.set(3, 0);
                return true;
            }
            return false;
        case open:
            log(c, open_kind, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), script.open_fails ? 0 : handle_value);
            c.set(3, script.open_fails ? 0 : handle_value);
            return true;
        case read: c.set(3, serve_read(c, script, c.r(3), c.r(4), c.r(5))); return true;
        case write: c.set(3, serve_write(c, script, c.r(3), c.r(4), c.r(5))); return true;
        case close: log(c, close_kind, c.r(3), c.u32(c.r(3)), 0, 0, 0, 0); return true;
        default: return false;
        }
    };
}
}
