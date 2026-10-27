#include "contract_manager.hpp"
#include "semantic_io_checks.hpp"
#include "routine_check.hpp"
#include "contract_manager_adapter.hpp"
#include <map>
#include <set>
#include <tuple>

namespace cm::aot {
bool contract_manager_persistence_reference(State&);
std::vector<std::uint32_t> contract_manager_persistence_calls();
}
namespace {
using namespace cm::semantic_io_checks;
using cm::aot::guest::Cpu;
constexpr std::uint32_t load_entry = 0x10091bf0, save_entry = 0x10093030;
constexpr auto manager = fixture + 0x1000, rows = fixture + 0x2000, keyed = fixture + 0x3000;
constexpr auto scheduled = fixture + 0x4000, pending = fixture + 0x5000, deferred = fixture + 0x6000;
constexpr auto staff = fixture + 0x7000, indices = fixture + 0x8000, clubs = fixture + 0x9000;
constexpr auto globals = fixture + 0xb000, pool = fixture + 0x10000, diagnostics = fixture + 0x20000;
constexpr auto arena = fixture + 0x30000, stream = fixture + 0x600, file = fixture + 0x700;
struct Options {
    std::uint32_t version = 0x3e033;
    unsigned count = 2, pending_count = 2, deferred_count = 2;
    unsigned fail_io = 0, fail_allocation = 0, interrupt = 0;
    bool open_failure = false, partial = false, flag = true, sort = false;
    bool people = false, missing_primary = false, dates_equal_year = false;
    bool reverse_dates = false, identical_dates = false, no_secondary = false, canonical_ids = false;
    std::int32_t staff_id = 0;
    bool mutate_count = false, corrupt = false;
    bool special_name = false;
};
struct Call {
    std::uint32_t target, back;
    std::vector<std::uint32_t> arguments, kept;
    bool operator==(const Call&) const = default;
};
unsigned arity(std::uint32_t target) {
    switch (target) {
    case 0x107475b0: return 5;
    case 0x10747a80: case 0x10747cc0: case 0x10768300: return 4;
    case 0x10739890: case 0x107156f0: return 5;
    case 0x1077b720: return 7;
    case 0x10727890: return 3;
    case 0x10769b60: case 0x10738d10: return 3;
    case 0x1077dc90: return 2;
    case 0x10768810: return 0;
    default: return 1;
    }
}
struct Model {
    Options options;
    std::vector<Call> calls;
    std::vector<std::uint8_t> input, written;
    std::size_t position = 0;
    unsigned allocations = 0, io = 0, next = 0;
    bool io_failure_injected = false, allocation_failure_injected = false;
    std::map<std::uint32_t, std::vector<std::uint32_t>> lists;
    Model(Options value, bool reading): options(value) {
        const auto append = [&](unsigned width, std::uint32_t n) {
            for (unsigned i = width; i; --i) input.push_back(std::uint8_t(n >> ((i - 1) * 8)));
        };
        const auto payload = [&](unsigned count, unsigned width, unsigned salt) {
            for (unsigned n = 0; n < count; ++n) {
                const auto start = input.size();
                for (unsigned i = 0; i < width; ++i) input.push_back(std::uint8_t(salt + n * 17 + i));
                // Usable current records for the normalization matrix.
                if (salt == 40 && options.people) {
                    for (unsigned i = 0; i < 4; ++i) input[start + i] = 0;
                    input[start + 3] = std::uint8_t(n + (options.canonical_ids ? 0 : 10));
                    const bool later = options.reverse_dates ? !(n & 1) : (n & 1);
                    input[start + 0x30] = 0; input[start + 0x31] = later && !options.identical_dates ? 80 : 40;
                    const auto year = options.dates_equal_year ? 2001u : (later ? 2002u : 2001u);
                    input[start + 0x32] = std::uint8_t(year >> 8); input[start + 0x33] = std::uint8_t(year);
                }
                if (salt == 120) {
                    for (unsigned i = 0; i < 4; ++i) input[start + i] = 0;
                    input[start + 0x32] = 7; input[start + 0x33] = 207;
                }
            }
        };
        if (!reading) return;
        if (std::int32_t(options.version) > 0x3e000) { append(4, options.count); payload(options.count, 24, 10); }
        append(4, options.count); payload(options.count, std::int32_t(options.version) > 0x3e032 ? 100 : 92, 40);
        append(4, options.count); payload(options.count, 16, 80); append(4, 0);
        for (unsigned count : {options.pending_count, options.deferred_count}) {
            append(2, count); payload(count, 136, 120);
        }
        if (options.flag) append(1, 1);
    }
    void call(State& s, std::uint32_t target, std::uint32_t back) {
        constexpr std::array allowed{0x107475b0u,0x10747a80u,0x10747cc0u,0x10747900u,0x10775130u,
            0x10775170u,0x10773520u,0x10775550u,0x10768810u,0x10739890u,0x1077b720u,0x10727890u,
            0x10769b60u,0x103721f0u,0x10715880u,0x10738c00u,0x10738d10u,0x10738e00u,0x10768300u,
            0x1077dc90u,0x107156f0u};
        require(std::ranges::find(allowed, target) != allowed.end(), "Known contract persistence dependency " + hex_address(target));
        Cpu c(s);
        Call event{target, back, {}, {}};
        auto arguments = arity(target);
        // This overload copies a literal title and does not read r5.
        if (target == 0x10727890 && c.r(3) == diagnostics + 0x1000) arguments = 2;
        for (unsigned n = 0; n < arguments; ++n) event.arguments.push_back(c.r(3 + n));
        for (unsigned n = 13; n < 32; ++n) event.kept.push_back(c.r(n));
        calls.push_back(std::move(event));
        const auto first = c.r(3), second = c.r(4), third = c.r(5);
        std::uint32_t answer = 0;
        if (target == 0x107475b0) answer = options.open_failure ? 0 : stream;
        if (target == 0x10775550) answer = options.version;
        if (target == 0x10768810) answer = 0x1234;
        if (target == 0x10775130 || target == 0x10773520) {
            ++allocations; require(first < 0x1000, "Bounded persistence allocation");
            allocation_failure_injected |= allocations == options.fail_allocation;
            answer = allocations == options.fail_allocation ? 0 : arena + allocations * 0x1000;
            if (answer) {
                std::vector<std::uint8_t> data(first);
                for (unsigned i=0; i<first; ++i) data[i]=std::uint8_t(0x6b+i*37+(i>>8));
                s.memory.copy_in(answer, data);
            }
        }
        if (target == 0x10747a80 || target == 0x10747cc0) {
            ++io; const auto width = second * third; require(width < 0x10000, "Bounded persistence IO");
            const bool failed = io == options.fail_io;
            io_failure_injected |= failed;
            const auto bytes = failed ? (options.partial ? width / 2 : 0) : width;
            if (target == 0x10747a80) {
                const auto available = input.size() - position;
                if (available < bytes) {
                    require(!options.flag && width == 1, "Available persistence input");
                    answer = 0;
                } else {
                    s.memory.copy_in(first, std::vector<std::uint8_t>(input.begin() + std::ptrdiff_t(position), input.begin() + std::ptrdiff_t(position + bytes)));
                    position += bytes; answer = failed ? (third ? third - 1 : 1) : third;
                }
            } else {
                auto data = s.memory.copy_out(first, bytes); written.insert(written.end(), data.begin(), data.end());
                answer = failed ? (third ? third - 1 : 1) : third;
            }
            if (options.mutate_count && io == 2) s.memory.write<4>(manager + 0x2c, 1);
        }
        if (target == 0x10747900) s.memory.write<4>(first, 0);
        if (target == 0x10739890) { s.memory.write<4>(c.r(6), 0x12345678); s.memory.write<4>(c.r(7), 0x87654321); }
        if (target == 0x1077b720) {
            for (unsigned n = 0; n < 7; ++n) s.memory.write<4>(first + n * 4, c.r(3 + n));
        }
        if (target == 0x10727890) { s.memory.write<4>(first, second); if (arguments == 3) s.memory.write<4>(first + 4, third); }
        if (target == 0x103721f0) s.memory.clear(first, 136);
        if (target == 0x10715880) s.memory.copy_in(first, {0,0,7,209,0});
        if (target == 0x10738c00) { s.memory.clear(first, 16); lists[first] = {}; answer = first; }
        if (target == 0x10738d10) {
            require(third == 136, "Pending record width");
            // The loader intentionally has no null check for this list.
            // Compare the original's null dependency call as a model stop,
            // separately from completed returns and handled error paths.
            if (!first) throw std::runtime_error("Persistence null pending-list dependency");
            const auto node = arena + 0x10000 + ++next * 0x100;
            s.memory.copy_in(node, s.memory.copy_out(second, 136)); lists[first].push_back(node);
            s.memory.write<2>(first, lists[first].size());
            s.memory.write<4>(first + 4, lists[first].front());
        }
        if (target == 0x10738e00) {
            auto current = std::uint32_t(s.memory.read<4>(first + 12));
            if (!current) answer = 0;
            else {
                answer = current;
                const auto& list = lists.at(first); const auto at = std::ranges::find(list, current);
                const auto following = at + 1 == list.end() ? 0 : *(at + 1);
                s.memory.write<4>(first + 12, following);
            }
        }
        if (target == 0x10768300 && second == 2) {
            const auto a = s.memory.copy_out(first, third), b = s.memory.copy_out(first + third, third);
            s.memory.copy_in(first, b); s.memory.copy_in(first + third, a);
        }
        if (target == 0x1077dc90) answer = options.special_name ? 0 : 1;
        if (target == 0x107156f0) s.memory.copy_in(first, {0,150,std::uint8_t(c.r(6) >> 8),std::uint8_t(c.r(6)),1});
        for (unsigned n = 0; n <= 12; ++n) if (n != 1 && n != 2) c.set(n, 0xdd000000 + n * 0x101);
        c.set_cr(0, 2); c.set(3, answer); s.pc = back;
        if (options.interrupt && calls.size() == options.interrupt) throw std::runtime_error("Persistence dependency interrupted");
    }
};
thread_local Model* active = nullptr;
bool probe(State& s) { require(active != nullptr, "Active persistence fixture"); active->call(s, s.pc, std::uint32_t(s.r<4>(4128))); return true; }
struct Hook final : ContractManagerCallHook { bool call(State& s) override { return probe(s); } } hook;

std::unique_ptr<State> seed(bool reading, const Options& options) {
    auto s = std::make_unique<State>(); s->registers.fill(0xa5); s->unique.fill(0xb7);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    s->memory.map(fixture, 0x60000); s->memory.map(stack - 0x6000, 0x7000); s->memory.map(toc - 0x8000, 0x9000);
    Cpu c(*s); c.set(1, stack); c.set(2, toc); c.set_lr(return_sentinel); c.set(3, manager); c.set(4, file);
    s->pc = reading ? load_entry : save_entry; s->budget = 500000;
    for (auto [slot, pointer] : {std::pair{-0x5718,pool}, {-0x7b04,globals}, {-0x7b50,globals+4}, {-0x7b74,globals+8},
             {-0x7be8,globals+12}, {-0x7b9c,globals+16}, {-0x79e0,diagnostics+0x2000}, {-0x79dc,diagnostics+0x1000},
             {-0x7ba8,diagnostics}, {-0x7be0,globals+20}, {-0x7be4,globals+24}, {-0x5724,globals+28}})
        c.put32(toc + slot, pointer);
    c.put32(globals+4, 2); c.put32(globals+12, clubs); c.put32(globals+16, indices); c.put32(globals+20, staff);
    c.put32(globals+24, options.people ? 1 : 0);
    c.put32(staff, std::uint32_t(options.staff_id)); c.put32(indices, options.missing_primary ? 0xffffffff : 0);
    c.put32(indices+4, options.no_secondary ? 0xffffffff : 1);
    if (options.people) for (unsigned club : {0u,1u}) for (unsigned slot : {0u,1u,49u})
        c.put32(clubs+club*0x248+0x98+slot*4, staff);
    if (!reading) {
        c.put32(manager, rows); c.put32(manager+4, options.count); c.put32(manager+8, options.count);
        c.put32(manager+0x28, keyed); c.put32(manager+0x2c, options.count); c.put32(manager+0x30, options.count);
        c.put32(manager+0x0c, scheduled); c.put32(manager+0x10, options.count); c.put32(manager+0x14, options.count);
        c.put32(manager+0x1c, options.sort);
        for (auto [pointer, width] : {std::pair{rows,100u}, {keyed,24u}, {scheduled,16u}})
            for (unsigned i = 0; i < options.count * width; ++i) c.put8(pointer + i, i * 31 + 7);
        for (auto [offset, pointer, count] : {std::tuple{0x34u,pending,options.pending_count}, {0x38u,deferred,options.deferred_count}}) {
            if (!count) continue;
            c.put32(manager+offset, pointer); c.put16(pointer, count); c.put32(pointer+4, pointer+0x100);
            for (unsigned n = 0; n < count; ++n) for (unsigned i = 0; i < 136; ++i) c.put8(pointer+0x100+n*0x100+i, i+n*17);
        }
    }
    return s;
}
void prepare_lists(Model& model, const Options& options) {
    for (auto [pointer, count] : {std::pair{pending,options.pending_count}, {deferred,options.deferred_count}})
        for (unsigned n = 0; n < count; ++n) model.lists[pointer].push_back(pointer + 0x100 + n * 0x100);
}
void reference(State& s, Model& model) {
    const auto entry = s.pc, end = entry == load_entry ? save_entry : 0x10093860;
    while (s.pc != return_sentinel) {
        if (s.pc >= entry && s.pc < end) require(contract_manager_persistence_reference(s), "Complete persistence reference");
        else model.call(s, s.pc, std::uint32_t(s.r<4>(4128)));
    }
}
void registers_equal(const State& a, const State& b, bool reading, const std::string& label) {
    for (unsigned n = 1; n < 32; ++n) {
        if (n == 3 && !reading) continue;
        if (n == 11 || n == 12) continue;
        require(a.r<4>(n*4) == b.r<4>(n*4), label + " r" + std::to_string(n) + " " + hex_address(a.r<4>(n*4)) + "/" + hex_address(b.r<4>(n*4)));
    }
    for (unsigned i = 1280; i < 1312; ++i) require(a.r<1>(i) == b.r<1>(i), label + " FPSCR");
    for (unsigned i = 2304; i < 2312; ++i) require(a.r<1>(i) == b.r<1>(i), label + " CR");
    require(a.r<1>(guest::reg::xer_ca) == b.r<1>(guest::reg::xer_ca), label + " XER carry");
    for (unsigned n = 1; n < 32; ++n) require(a.r<8>(0x4000+n*16) == b.r<8>(0x4000+n*16), label + " f" + std::to_string(n));
}
unsigned cases = 0, negatives = 0, returned = 0, model_stops = 0;
std::set<std::uint32_t> reached;
void compare(bool reading, Options options = {}) {
    auto source = seed(reading, options), a = clone(*source), b = clone(*source);
    Model old(options, reading), modern(options, reading); prepare_lists(old, options); prepare_lists(modern, options);
    const auto run = [&](State& s, Model& model, bool original) {
        try {
            if (original) reference(s, model);
            else {
                const auto previous = dispatch_probe; const auto previous_hook = contract_manager_call_hook;
                active = &model; dispatch_probe = probe; contract_manager_call_hook = &hook;
                try { Cpu c(s); if (reading) c.set(3, cm::aot::contract_manager::load_contracts(c, c.r(3), c.r(4)));
                      else cm::aot::contract_manager::save_contracts(c, c.r(3), c.r(4)); c.finish(); }
                catch (...) { dispatch_probe = previous; contract_manager_call_hook = previous_hook; active = nullptr; throw; }
                dispatch_probe = previous; contract_manager_call_hook = previous_hook; active = nullptr;
            }
            return std::string{};
        } catch (const std::exception& e) { return std::string(e.what()); }
    };
    const auto expected = run(*a, old, true), actual = run(*b, modern, false);
    const auto label = std::string(reading ? "load" : "save") + " case " + std::to_string(cases);
    require(expected.empty() || expected == "Persistence dependency interrupted" || expected == "Persistence null pending-list dependency", label + " invalid reference: " + expected);
    require(expected == actual, label + " outcomes " + expected + "/" + actual);
    if (options.fail_io) require(old.io_failure_injected && modern.io_failure_injected, label + " requested IO failure reached");
    if (options.fail_allocation) require(old.allocation_failure_injected && modern.allocation_failure_injected, label + " requested allocation failure reached");
    if (options.corrupt) b->memory.write<1>(manager + 8, b->memory.read<1>(manager + 8) ^ 0x5a);
    bool rejected = false;
    try {
        require(a->pc == b->pc, label + " PC"); registers_equal(*a, *b, reading, label);
        require(a->r<4>(4128) == b->r<4>(4128), label + " LR");
        require(a->memory.mapped_ranges() == b->memory.mapped_ranges(), label + " mappings");
        for (auto [address, size] : a->memory.mapped_ranges()) {
            auto left = a->memory.copy_out(address, size), right = b->memory.copy_out(address, size);
            if (left != right) {
                auto at = std::mismatch(left.begin(), left.end(), right.begin());
                if (!options.corrupt) std::cerr << "Memory bytes " << unsigned(*at.first) << "/" << unsigned(*at.second) << '\n';
                for (unsigned k=0; !options.corrupt && k<std::min(old.calls.size(),modern.calls.size()); ++k) {
                    if (old.calls[k]==modern.calls[k]) continue;
                    std::cerr << "Call " << k << " " << hex_address(old.calls[k].target) << " args";
                    for (auto value:old.calls[k].arguments) std::cerr << " " << hex_address(value);
                    std::cerr << " vs";
                    for (auto value:modern.calls[k].arguments) std::cerr << " " << hex_address(value);
                    std::cerr << '\n';
                }
                throw std::runtime_error(label + " memory " + hex_address(address + std::uint32_t(at.first-left.begin())));
            }
        }
        require(old.written == modern.written && old.position == modern.position, label + " stream bytes");
        if (old.calls != modern.calls) {
            const auto at = std::mismatch(old.calls.begin(), old.calls.end(), modern.calls.begin(), modern.calls.end());
            throw std::runtime_error(label + " dependency " + std::to_string(at.first-old.calls.begin()) +
                                     (at.first != old.calls.end() ? " target " + hex_address(at.first->target) : " extra"));
        }
    } catch (const std::runtime_error&) { if (!options.corrupt) throw; rejected = true; }
    require(rejected == options.corrupt, label + " discriminating mutation");
    if (expected.empty() && !options.corrupt) for (const auto& call : old.calls) reached.insert(call.back);
    ++cases; negatives += rejected; returned += expected.empty() && !rejected; model_stops += !expected.empty();
}
}
int main() {
    try {
        for (bool reading : {false, true}) {
            compare(reading);
            Options empty; empty.count=empty.pending_count=empty.deferred_count=0; compare(reading, empty);
            Options open; open.open_failure=true; compare(reading, open);
            for (unsigned fail=1; fail <= 14; ++fail) for (bool partial : {false,true}) {
                Options o; o.fail_io=fail; o.partial=partial; compare(reading, o);
            }
            for (unsigned cut : {1u,2u,5u,8u}) { Options o; o.interrupt=cut; compare(reading,o); }
            Options mutation; mutation.corrupt=true; compare(reading, mutation);
            for (unsigned count : {0u,1u,2u}) for (unsigned pending_rows : {0u,1u,2u}) for (unsigned deferred_rows : {0u,1u,2u}) {
                Options o; o.count=count; o.pending_count=pending_rows; o.deferred_count=deferred_rows; compare(reading,o);
            }
        }
        for (auto version : {0x3e000u,0x3e001u,0x3e032u,0x3e033u,0xffffffffu}) {
            Options o; o.version=version; compare(true,o);
        }
        for (auto version : {0x3e000u,0x3e032u}) for (unsigned fail=1; fail<=(version==0x3e000 ? 12u : 14u); ++fail) {
            Options o; o.version=version; o.fail_io=fail; o.partial=true; compare(true,o);
        }
        for (unsigned fail : {1u,2u,3u,5u}) { Options o; o.fail_allocation=fail; compare(true,o); }
        for (unsigned fail : {3u,6u}) { Options o; o.version=0x3e032; o.fail_allocation=fail; compare(true,o); }
        for (auto version : {0x3e032u,0x3e033u}) {
            Options o; o.version=version; o.fail_allocation=version==0x3e032 ? 5 : 4; compare(true,o);
        }
        for (bool missing : {false,true}) for (bool equal : {false,true}) {
            Options o; o.people=true; o.missing_primary=missing; o.dates_equal_year=equal; compare(true,o);
        }
        for (bool same_year : {false,true}) for (bool identical : {false,true}) {
            Options o; o.people=true; o.reverse_dates=true; o.dates_equal_year=same_year;
            o.identical_dates=identical; compare(true,o);
        }
        for (auto id : {-1,2}) { Options o; o.people=true; o.staff_id=id; compare(true,o); }
        Options no_secondary; no_secondary.people=true; no_secondary.no_secondary=true; compare(true,no_secondary);
        Options canonical; canonical.people=true; canonical.canonical_ids=true; compare(true,canonical);
        Options legacy; legacy.flag=false; compare(true, legacy); legacy.special_name=true; compare(true, legacy);
        Options sorting; sorting.sort=true; compare(false, sorting);
        Options changing; changing.mutate_count=true; compare(false,changing);
        const auto original_calls=contract_manager_persistence_calls();
        const std::set<std::uint32_t> expected_calls(original_calls.begin(),original_calls.end());
        require(reached==expected_calls, "Every original persistence call continuation reached by returned cases");
        std::cout << cases << " contract-manager persistence comparisons; " << negatives
                  << " mutations detected; " << returned << " returned; " << model_stops
                  << " model stops (partial); " << reached.size() << "/" << expected_calls.size()
                  << " original call continuations covered by returned cases\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
