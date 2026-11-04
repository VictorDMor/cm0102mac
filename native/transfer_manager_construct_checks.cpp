#include "transfer_manager_construct.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t constructor_entry = 0x103df0d0, array_constructor = 0x10773320, list_constructor = 0x10738c00,
    media_lookup = 0x1077fee0, ordinal = 0x107156b0, nation_init = 0x103fd420, archive_load = 0x103df790,
    staff_init = 0x103ec6d0, shortlist_init = 0x10356410, allocate_table = 0x10775200,
    allocate_shortlist = 0x10773520, split = 0x10739890, format = 0x1077b720, text = 0x10727890, dialog = 0x10769b60;
constexpr std::uint32_t manager = fixture, tables = fixture + 0x1000, shortlist = fixture + 0x2000,
    archive = fixture + 0x2200, globals = fixture + 0x3000, vectors = fixture + 0x4000, pool = fixture + 0x5000,
    title = fixture + 0x6000, body = fixture + 0x6100, message = fixture + 0x6200, trace = fixture + 0x7000;
constexpr std::uint32_t parent_frame = stack - 0x3e0;
constexpr std::uint32_t loader_frame = parent_frame - 0x770;
constexpr std::array media_bytes{0x61u, 0x42u, 0x63u, 0x44u, 0xe5u, 0x46u};

enum class Failure { none, staff, club, shortlist };
struct Case {
    Failure failure = Failure::none;
    unsigned retries = 0, media_length = 3;
    std::uint32_t load_result = 0;
    bool from_archive = false, real_staff = false, failed_open = false;
};
Case active;
bool original_route = false, production_route = false;

void run_original(State&, std::uint32_t);
bool observe(State&);

// The parent calls the loader directly. Reattach the external-service
// fixtures when the real loader establishes its own guest frame.
struct ArchiveBoundary final : MemoryObserver {
    State& state;
    bool entered = false;
    explicit ArchiveBoundary(State& s) : state(s) { restrict_write_notifications(loader_frame, loader_frame + 4); }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {
        require(state.pc == archive_load && dispatch_probe == nullptr, "Transfer direct load boundary");
        dispatch_probe = observe;
        entered = true;
    }
};

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 40, "Transfer constructor trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 64;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    add(c.r(2));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    add(c.r(23));
    add(c.r(25));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 64) == target;
    return count;
}

bool external(State& s, unsigned allowance) {
    if (!production_route || original_route) return false;
    const auto back = Cpu(s).lr();
    s.budget = s.executed + allowance;
    require(dispatch_page(s), "Missing transfer constructor service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

struct Diagnostic {
    std::uint32_t split_return, name, extension, line, reason;
};
Diagnostic diagnostic() {
    if (active.failed_open) return {0x103df818, 0x6a8, 0x184, 0x163, 0x264};
    switch (active.failure) {
    case Failure::staff: return {0x103df390, 0xa8, 0x50, 0xa3, 0x5e};
    case Failure::club: return {0x103df428, 0x88, 0x48, 0xab, 0x1ce};
    case Failure::shortlist: return {0x103df4f4, 0x68, 0x40, 0xc0, 0x215};
    default: throw std::runtime_error("Unexpected transfer constructor allocation diagnostic");
    }
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (target) {
    case array_constructor:
        require(back == 0x103df11c && a == manager + 0x18 && b == vectors && d == vectors + 8 &&
                c.r(6) == 16 && c.r(7) == 150, "Transfer embedded list array");
        record(c);
        if (external(s, 25000)) {
            // Default date and two embedded list leaves follow the array.
            s.budget = s.executed + 7 + 2 * 6;
            return true;
        }
        return false;
    case media_lookup:
        require(back == 0x103df1ec && !a && !b && d == parent_frame + 0x124 &&
                c.r(6) == parent_frame + 0xdc && c.r(2) == toc && c.u8(d) == active.media_length * 2,
                "Transfer media request");
        for (unsigned i = 0; i < active.media_length * 2; ++i)
            require(c.u8(d + 1 + i) == media_bytes[i], "Transfer media name interleave");
        c.put32(c.r(6), 0x4d454449);
        c.put32(c.sp() + 0x14, toc);
        answer = calls(c, media_lookup) < active.retries ? 0x0000ffdd : 0;
        if (production_route && !original_route) s.budget = s.executed;
        break;
    case ordinal:
        require(back == 0x103df318 && a == parent_frame + 0x38 && !b && !d && c.r(2) == toc,
                "Transfer start date request");
        record(c);
        return external(s, 200);
    case nation_init:
        require(back == 0x103df350 && a == manager && c.u32(manager + 0x97c) == 0 &&
                c.u8(manager + 0x980) == 1 && c.u32(manager + 0xa6c) == 0, "Transfer nation initialization state");
        c.put32(a + 0x9e0, 0x12345678);
        break;
    case allocate_table:
        require(c.r(2) == toc, "Transfer allocation TOC");
        if (back == 0x103df360) {
            require(a == 2 && b == 10, "Transfer staff status allocation");
            answer = active.failure == Failure::staff ? 0 : tables;
        } else {
            require(back == 0x103df3f8 && a == 4 && b == 16 && c.u32(manager + 0x9a0) == tables,
                    "Transfer club status allocation");
            answer = active.failure == Failure::club ? 0 : tables + 0x100;
        }
        break;
    case archive_load:
        require(back == 0x103df49c && a == manager && b == archive && c.u32(manager + 0x990) == 0xffffffff,
                "Transfer saved section request");
        if (active.failed_open && original_route) {
            run_original(s, target);
            return true;
        }
        answer = active.load_result;
        if (std::int8_t(answer)) c.put32(manager + 0x988, shortlist + 0x100);
        break;
    case 0x103721f0:
        require(active.failed_open && back == 0x103df7cc && a == loader_frame + 0x6c8, "Transfer load scratch");
        c.put32(a, 0);
        break;
    case 0x107475b0:
        require(active.failed_open && back == 0x103df7f4 && a == archive && b == pool + 0x257 &&
                !d && c.r(6) == 22 && c.r(7) == 4, "Transfer archive open");
        break;
    case allocate_shortlist:
        require(back == 0x103df4b4 && a == 0xe0, "Transfer shortlist allocation");
        answer = active.failure == Failure::shortlist ? 0 : shortlist;
        break;
    case shortlist_init:
        require(back == 0x103df4c4 && a == shortlist && !b, "Transfer shortlist initialization");
        c.put32(a, 0x534c4953);
        answer = a;
        break;
    case staff_init:
        require(back == 0x103df550 && a == manager && c.u32(manager + 0x988) == shortlist,
                "Transfer staff bootstrap state");
        if (active.real_staff && original_route) {
            run_original(s, target);
            return true;
        }
        c.put16(tables + 4, 0x3fff);
        break;
    case split: {
        const auto info = diagnostic();
        const auto frame = active.failed_open ? loader_frame : parent_frame;
        require(back == info.split_return && a == pool + 0x37 && !b && !d && c.r(6) == frame + info.name &&
                c.r(7) == frame + info.extension, "Transfer diagnostic split");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        if (production_route && !original_route) s.budget = s.executed + 2;
        break;
    }
    case format: {
        const auto info = diagnostic();
        require(back == info.split_return + 0x24 && a == message && b == pool + 0x4c && d == globals + 0x100 &&
                c.r(8) == info.line && c.r(9) == pool + info.reason, "Transfer allocation diagnostic format");
        c.put32(message, info.line);
        break;
    }
    case text:
        if (back == 0x103df210 || back == 0x103df21c) {
            require((back == 0x103df210 && a == parent_frame + 0x2ec && b == pool) ||
                    (back == 0x103df21c && a == parent_frame + 0x224 && b == pool + 10), "Transfer media diagnostic");
        } else {
            const auto first = diagnostic().split_return;
            require((back == first + 0x30 && a == title && b == pool + (active.failed_open ? 0x2a2 : 0xa6)) ||
                    (back == first + 0x40 && a == body && b == pool + (active.failed_open ? 0x2b2 : 0xbe) && d == message),
                    "Transfer diagnostic text");
        }
        c.put32(a, b);
        break;
    case dialog:
        if (back == 0x103df22c)
            require(a == parent_frame + 0x2ec && b == parent_frame + 0x224 && d == 1, "Transfer media retry dialog");
        else
            require(back == diagnostic().split_return + 0x50 && a == title && b == body && !d,
                    "Transfer allocation dialog");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xad000000 + r);
    if (target == media_lookup) c.set(2, toc + 0x40);
    c.set(3, answer);
    c.set_cr(0, 4);
    c.finish();
    if (target == allocate_table && back == 0x103df3f8 && active.failed_open && production_route && !original_route)
        dispatch_probe = nullptr;
    if (target == shortlist_init && active.real_staff && production_route && !original_route) {
        dispatch_probe = nullptr;
        s.budget = s.executed + 200;
    }
    return true;
}

std::unique_ptr<State> make_state() {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0x9000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = constructor_entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, active.from_archive ? archive : 0}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x6214, vectors);
    c.put32(toc - 0x6314, vectors + 8);
    c.put32(vectors, list_constructor);
    c.put32(vectors + 4, toc);
    c.put32(vectors + 8, 0x10738c20);
    c.put32(vectors + 12, toc);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x4b98, pool + 0x800);
    c.put32(toc - 0x4b94, pool + 0x820);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x5b20, globals + 0x100);
    c.put32(toc - 0x7be4, globals);
    c.put32(toc - 0x7b50, globals + 4);
    c.put32(toc - 0x7be0, globals + 8);
    c.put32(globals, 2);
    c.put32(globals + 4, 4);
    c.put32(globals + 8, globals + 0x200);
    c.put8(globals + 0x200 + 0x5e, 14);
    c.put8(pool + 0x800, active.media_length);
    c.put8(pool + 0x820, active.media_length);
    for (unsigned i = 0; i < active.media_length; ++i) {
        c.put8(pool + 0x821 + i, media_bytes[i * 2]);
        c.put8(pool + 0x801 + i, media_bytes[i * 2 + 1]);
    }
    for (unsigned i = 0; i < 150; ++i) c.put32(manager + 0x18 + 16 * i + 8, 0xdeadcafe);
    return s;
}

void run_original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    while (s.pc != stop) {
        if (entry == constructor_entry && (s.pc == nation_init || s.pc == archive_load || s.pc == staff_init)) {
            require(observe(s), "Missing transfer constructor dependency fixture");
            continue;
        }
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t) {
    Cpu c(s);
    construct_transfer_manager(c, c.r(3), c.r(4));
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    ArchiveBoundary boundary(s);
    if (active.failed_open) s.memory.observer = &boundary;
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    run_until(s, return_sentinel);
    s.memory.observer = nullptr;
    require(!active.failed_open || boundary.entered, "Transfer loader was not entered directly");
    production_route = false;
    dispatch_probe = observe;
}

unsigned checked = 0;
void check(const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = make_state();
    auto baseline = clone(*s);
    run_original(*baseline, constructor_entry);
    Cpu c(*baseline);
    const bool tables_ok = test.failure != Failure::staff && test.failure != Failure::club;
    const bool loaded = tables_ok && test.from_archive && std::int8_t(test.load_result);
    const bool new_shortlist = tables_ok && !loaded && test.failure != Failure::shortlist;
    require(c.r(3) == manager && c.r(2) == toc && calls(c, media_lookup) == test.retries + 1 &&
            calls(c, test.failed_open ? 0x107475b0 : archive_load) == unsigned(tables_ok && test.from_archive) &&
            calls(c, dialog) == test.retries + (test.failure != Failure::none) + test.failed_open,
            "Original transfer constructor route");
    require(c.u32(manager + 0x9e0) == 0x12345678 &&
            c.u32(manager + 0x988) == (loaded ? shortlist + 0x100 : new_shortlist ? shortlist : 0) &&
            c.u16(tables + 4) == (new_shortlist ? 0x3fff : 0), "Original transfer constructor state");
    for (unsigned i = 0; i < 150; ++i)
        require(c.u32(manager + 0x18 + 16 * i + 8) == 0, "Original transfer list initialization");
    const auto differences = compare_routine_call(*s, constructor_entry, run_original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer manager constructor case " + std::to_string(checked));
    ++checked;
}

void wrong_shortlist(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put32(manager + 0x988, 0);
}
}

int cm::aot::transfer_manager::checks::construct(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check({}, production);
            check({.from_archive = true, .real_staff = true}, production);
            check({.failure = Failure::staff, .retries = 1}, production);
            check({.load_result = 1, .from_archive = true}, production);
            check({.from_archive = true, .failed_open = true}, production);
            std::cout << checked << " transfer constructor production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer constructor check mode");
        for (const auto failure : {Failure::none, Failure::staff, Failure::club, Failure::shortlist}) {
            check({.failure = failure});
            check({.failure = failure, .retries = 2, .from_archive = true});
        }
        check({.retries = 1, .media_length = 0});
        check({.load_result = 0x100, .from_archive = true});
        check({.load_result = 0xffffff80, .from_archive = true});
        check({.load_result = 1, .from_archive = true});
        active = {};
        const auto s = make_state();
        const auto differences = compare_routine_call(*s, constructor_entry, run_original, wrong_shortlist,
            RoutineChecker::result_r3, false, true);
        require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30000988"); }),
                "The constructor comparison missed a missing shortlist");
        std::cout << checked << " transfer constructor comparisons and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
