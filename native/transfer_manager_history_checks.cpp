#include "transfer_manager_history.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <random>
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t append = 0x103f2d50, archive = 0x103e92b0, shift_back = 0x10717cb0,
    format = 0x1077b720, make_path = 0x10746280, open_file = 0x10739de0, write_file = 0x10776360,
    close_file = 0x107766f0, move_data = 0x1077fc28, row_size = 40;
constexpr std::uint32_t manager = fixture, rows = fixture + 0x1000, expanded = fixture + 0x2000,
    input = fixture + 0x3800, globals = fixture + 0x4000, vector = fixture + 0x4200,
    filename = fixture + 0x4300, path = fixture + 0x4400, handles = fixture + 0x4500,
    archived = fixture + 0x4600, trace = fixture + 0x5000, pool = fixture + 0x7000,
    message = fixture + 0x9000, title = fixture + 0x9100, body = fixture + 0x9200;
constexpr unsigned move_import = 0x10bf7bbc, archive_frame = stack - 0x110;

struct Case {
    unsigned count = 5, capacity = 8, bad_year = 0, failed_row = 0xffffffff;
    bool has_array = true, allocation_fails = false, all_new = false, zero_year = false,
         cutoff_before_first_day = false;
};
Case active;
bool original_route = false, production_route = false;

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 64, "History trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 96;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    for (unsigned r = 17; r <= 22; ++r) add(c.r(r));
    add(c.cr(0));
}

unsigned calls(Cpu& c, std::uint32_t target) {
    unsigned count = 0;
    for (unsigned i = 0; i < c.u32(trace); ++i) count += c.u32(trace + 4 + i * 96) == target;
    return count;
}

bool run_service(State& s, std::uint32_t back) {
    if (!production_route || original_route) return false;
    s.budget = s.executed + 15000;
    require(dispatch_page(s), "Missing real archive dependency");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

bool observe(State& s) {
    Cpu c(s);
    const auto target = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (target) {
    case shift_back:
        require(back == 0x103e92f8 && a == archive_frame + 0x40 && b == globals && d == 52,
                "Archive date cutoff");
        record(c);
        return run_service(s, back);
    case format:
        if (back == 0x103e93a8) {
            require(a == filename && b == pool + 0x1838 && d >= 2001 && d <= 2003, "Archive year filename");
            c.put32(filename, d);
        } else {
            require(a == message && d == globals + 0x40 && c.u32(c.r(6)) == 0x5452414e &&
                    c.u32(c.r(7)) == 0x43505000, "History diagnostic format");
            const auto line = c.r(8);
            if (back == 0x103e93fc || back == 0x103e9474)
                require(b == pool + 0x6ff && line == (back == 0x103e93fc ? 0x1331u : 0x133bu), "Archive I/O diagnostic");
            else
                require(b == pool + 0x4c && ((back == 0x103e9518 && line == 0x135c && c.r(9) == pool + 0x1855) ||
                         (back == 0x103f2e08 && line == 0x2793 && c.r(9) == pool + 0x1b1b)), "History allocation diagnostic");
            c.put32(message, line);
        }
        break;
    case make_path:
        require(back == 0x103e93b4 && a == pool + 0x1849 && b == filename, "Archive path resolution");
        c.put32(path, c.u32(filename));
        answer = path;
        break;
    case open_file:
        require(back == 0x103e93bc && a == path && b == pool + 0x1852, "Archive append stream");
        if (c.u32(path) != active.bad_year) answer = handles + (c.u32(path) - 2000) * 4;
        break;
    case write_file: {
        const auto index = (a - rows) / row_size;
        require(back == 0x103e9434 && a >= rows && index < active.count && (a - rows) % row_size == 0 &&
                b == row_size && d == 1 && c.r(6) == handles + (c.u16(a + 0x22) - 2000) * 4,
                "Archive transfer row write");
        if (index != active.failed_row) {
            const auto written = c.u32(archived);
            s.memory.copy_in(archived + 4 + written * row_size, s.memory.copy_out(a, row_size));
            c.put32(archived, written + 1);
            answer = 1;
        }
        break;
    }
    case close_file:
        require((back == 0x103e9398 || back == 0x103e94b8) && a >= handles + 4 && a <= handles + 12,
                "Archive close handle");
        break;
    case 0x10775130:
        require((back == 0x103f2da8 && !active.has_array && a == (active.capacity + 100) * row_size) ||
                (back == 0x103e94d4 && a == (active.count - c.r(20) + 100) * row_size), "History allocation size");
        answer = active.allocation_fails ? 0 : expanded;
        break;
    case 0x107751b0:
        require(back == 0x103f2dbc && active.has_array && a == rows && b == (active.capacity + 100) * row_size,
                "History growth size");
        if (!active.allocation_fails) {
            s.memory.copy_in(expanded, s.memory.copy_out(rows, active.count * row_size));
            answer = expanded;
        }
        break;
    case 0x10775170:
        require(back == 0x103e9574 && a == rows && c.r(2) == toc, "Archive old array release and restored TOC");
        break;
    case move_data:
        require(back == 0x103e9568 && a == rows + c.r(20) * row_size && b == expanded &&
                d == (active.count - c.r(20)) * row_size, "Archive compacted history move");
        record(c);
        return run_service(s, back);
    case 0x10739890:
        require(a == pool + 0x37 && !b && !d, "History diagnostic source split");
        require((back == 0x103e93dc && c.r(6) == archive_frame + 0xa8 && c.r(7) == archive_frame + 0x58) ||
                (back == 0x103e9454 && c.r(6) == archive_frame + 0x88 && c.r(7) == archive_frame + 0x50) ||
                (back == 0x103e94f4 && c.r(6) == archive_frame + 0x68 && c.r(7) == archive_frame + 0x48) ||
                (back == 0x103f2de4 && c.r(6) == c.sp() + 0x40 && c.r(7) == c.sp() + 0x38),
                "History diagnostic buffers");
        c.put32(c.r(6), 0x5452414e);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x10727890:
        require((a == title && b == pool + 0xa6) || (a == body && b == pool + 0xbe && d == message),
                "History allocation dialog text");
        c.put32(a, b);
        break;
    case 0x10769b60:
        require(!d && ((a == title && b == body) || (a == pool + 0x70c && b == message)), "History diagnostic display");
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xa4000000 + r);
    c.set_cr(0, 4);
    c.set(3, answer);
    c.finish();
    return true;
}

std::unique_ptr<State> fixture_state(std::uint32_t entry, const Case& test) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xa000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x2000, 0x3000);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, input}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7b80, globals);
    c.put32(toc - 0x7bb8, filename);
    c.put32(toc - 0x7bac, globals + 8);
    c.put32(toc - 0x5b20, globals + 0x40);
    c.put32(toc - 0x4b9c, pool);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x79dc, title);
    c.put32(toc - 0x79e0, body);
    c.put32(toc - 0x3fb8, globals + 0x80);
    c.put32(toc - 0x7e64, vector);
    const auto* import = import_name(move_import);
    require(import && std::string_view(import) == "CarbonLib::BlockMoveData", "Wrong history move import");
    c.put32(vector, move_import);
    c.put32(vector + 4, toc + 0x4000);
    c.put16(globals, test.cutoff_before_first_day ? 98 : 100);
    c.put16(globals + 2, 2003);
    c.put8(globals + 8, 1);
    c.put32(manager + 0x994, test.has_array ? rows : 0);
    c.put32(manager + 0x998, test.capacity);
    c.put32(manager + 0x99c, test.count);
    for (unsigned i = 0; i <= test.count; ++i) {
        const auto row = rows + i * row_size;
        for (unsigned byte = 0; byte < row_size; ++byte) c.put8(row + byte, 0x40 + i + byte);
        c.put16(row + 0x20, i == 3 ? 101 : 100);
        c.put16(row + 0x22, test.all_new ? 2003 : test.zero_year && !i ? 0 : 2001 + i / 2);
    }
    for (unsigned byte = 0; byte < row_size; ++byte) c.put8(input + byte, 0x90 + byte);
    c.put16(input + 0x20, 210);
    c.put16(input + 0x22, 2004);
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    require(transfer_manager_reference(s), "Missing transfer history reference");
    while (s.pc != stop) {
        if (!transfer_manager_reference(s)) run_until(s, Cpu(s).lr());
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    if (entry == append) append_past_transfer(c, c.r(3), c.r(4));
    else archive_past_transfers(c, c.r(3));
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    run_until(s, return_sentinel);
    production_route = false;
}

unsigned checked = 0;
void compare(State& s, std::uint32_t entry, RoutineRunner first, RoutineRunner second) {
    const auto differences = compare_routine_call(s, entry, first, second,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Transfer history case " + std::to_string(checked) + " at " + hex_address(entry));
    ++checked;
}

void check_append(const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(append, test);
    auto control = clone(*s);
    original(*control, append);
    Cpu c(*control);
    const bool grows = test.count == test.capacity;
    const auto storage = grows ? (test.allocation_fails ? 0 : expanded) : rows;
    require(c.u32(manager + 0x994) == storage && c.u32(manager + 0x998) == test.capacity + (grows ? 100 : 0),
            "Original past transfer capacity");
    if (storage) {
        const auto row = storage + test.count * row_size;
        require(c.u32(manager + 0x99c) == test.count + 1 && c.u32(row) == c.u32(input) &&
                c.u16(row + 0x20) == 210 && c.u16(row + 0x22) == 2004 &&
                c.u8(row + 0x24) == c.u8(input + 0x24), "Original past transfer row");
        require(c.u8(row + 0x1d) == (grows ? 0 : 0x40 + test.count + 0x1d), "Original past transfer padding");
    } else {
        require(c.u32(manager + 0x99c) == test.count && calls(c, 0x10769b60) == 1, "Original past transfer allocation failure");
    }
    compare(*s, append, original, selected);
}

void check_archive(const Case& test, unsigned moved, unsigned writes, bool success, RoutineRunner selected = rewrite) {
    active = test;
    auto s = fixture_state(archive, test);
    auto control = clone(*s);
    original(*control, archive);
    Cpu c(*control);
    const auto removed = test.allocation_fails ? 0 : moved;
    require(c.u32(manager + 0x99c) == test.count - removed && c.u32(archived) == writes &&
            c.u8(globals + 8) == success, "Original archive counts and success flag");
    require(c.u32(manager + 0x994) == (removed ? expanded : rows) &&
            c.u32(manager + 0x998) == (removed ? test.count - removed + 100 : test.capacity), "Original archive compaction");
    if (removed)
        require(c.u32(expanded) == c.u32(rows + removed * row_size) && c.u32(archive_frame + 0x14) == toc,
                "Original archive payload and saved TOC");
    compare(*s, archive, original, selected);
}

void sequence(State& s, RoutineRunner run) {
    Cpu c(s);
    run(s, append);
    c.set(3, manager);
    run(s, archive);
    require(c.u32(manager + 0x99c) == 3 && c.u32(archived) == 3 &&
            c.u16(expanded + 2 * row_size + 0x22) == 2004, "Combined append and archive result");
}
void original_sequence(State& s, std::uint32_t) { sequence(s, original); }
void production_sequence(State& s, std::uint32_t) { sequence(s, production); }

void change_archived_year(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put16(archived + 4 + 0x22, 2000);
}

void check_negative_control() {
    active = {};
    auto s = fixture_state(archive, active);
    const auto differences = compare_routine_call(*s, archive, original, change_archived_year,
        RoutineChecker::result_r3, false, true);
    require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30004627"); }),
            "The archive comparison missed an altered year");
}
}

int cm::aot::transfer_manager::checks::history(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check_append({}, production);
            check_archive({}, 3, 3, true, production);
            active = {};
            auto s = fixture_state(append, active);
            // The post-append archive sees one additional row.
            ++active.count;
            compare(*s, append, original_sequence, production_sequence);
            std::cout << checked << " transfer history production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown transfer history check mode");
        check_append({});
        check_append({.count = 3, .capacity = 3});
        check_append({.count = 0, .capacity = 0, .has_array = false});
        check_append({.count = 0, .capacity = 0, .has_array = false, .allocation_fails = true});
        check_append({.count = 3, .capacity = 3, .allocation_fails = true});
        check_archive({}, 3, 3, true);
        check_archive({.count = 0}, 0, 0, true);
        check_archive({.bad_year = 2001}, 2, 1, false);
        check_archive({.failed_row = 1}, 2, 2, false);
        check_archive({.allocation_fails = true}, 3, 3, true);
        check_archive({.all_new = true}, 0, 0, true);
        check_archive({.zero_year = true}, 3, 2, true);
        check_archive({.cutoff_before_first_day = true}, 2, 2, true);
        constexpr auto seed = 0x54484953u;
        std::mt19937 random(seed);
        for (unsigned i = 0; i < 20; ++i) {
            const auto count = unsigned(random() % 6), spare = unsigned(random() % 3);
            check_append({.count = count, .capacity = count + spare});
        }
        check_negative_control();
        std::cout << checked << " transfer history comparisons (seed " << seed << ") and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
