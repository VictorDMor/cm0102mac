#include "hall_of_fame_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "localization_format_adapter.hpp"
#include "semantic_io_checks.hpp"

#include <random>

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto entry = 0x10142100u, initialize = 0x101407a0u, done = fixture + 0x80;
constexpr auto first = fixture + 0x100, second = fixture + 0x200;
constexpr auto absent = fixture + 0x4000;

constexpr auto read_record = 0x101407e0u, add_default = 0x101409c0u, add_country = 0x10140a90u;
constexpr auto insert = 0x10140b80u, category = 0x10141e80u;
constexpr unsigned row_size = 0x78, bucket_size = 100 * row_size;
constexpr auto table = fixture + 0x10000, person = fixture + 0x500;
constexpr auto source = fixture + 0x600, message = fixture + 0x680, success = fixture + 0x700;
constexpr auto today = fixture + 0x710, game_id = fixture + 0x718, country_count = fixture + 0x71c;
constexpr auto staff_count = fixture + 0x720, nation_values = fixture + 0x800;
constexpr auto club_name = fixture + 0x900, nation_name = fixture + 0x920, rank_descriptor = fixture + 0x940;
constexpr auto real_sort = fixture + 0x980;
constexpr auto construct_table = 0x10140730u, read_file = 0x10141190u, write_file = 0x10141990u;
constexpr auto merge_record = 0x10140f60u, import_file = 0x10141500u, validate_file = 0x10143050u;
constexpr unsigned table_size = 27 * bucket_size;
constexpr auto file_path = fixture + 0xa00, file_handle = fixture + 0xa80;
constexpr auto file_position = fixture + 0xa90, file_size = fixture + 0xa94;
constexpr auto fail_open = fixture + 0xa98, short_at = fixture + 0xa9c;
constexpr auto file_contents = fixture + 0x100000;
constexpr auto record_constructor = fixture + 0xb00, active_table_slot = fixture + 0xb10;
constexpr auto imported_table = fixture + 0x180000;
constexpr unsigned file_capacity = table_size + 12;
constexpr auto category_name = 0x10142a10u, sorted_categories = 0x101430a0u, compare_categories = 0x10143180u;
constexpr auto category_switch = fixture + 0x2000, category_order = fixture + 0x2100;
constexpr auto category_descriptor = fixture + 0x2120, characters = fixture + 0x2200;
constexpr auto category_output = fixture + 0x2600;
constexpr int category_mode = -0x3953;
constexpr auto read_item_entry = 0x10775fe0u, write_item_entry = 0x10776360u;
using Call = std::array<std::uint32_t, 34>;
std::vector<Call>* call_trace = nullptr;

void put_text(Cpu& c, std::uint32_t to, std::string_view value) {
    for (unsigned i = 0; i < value.size(); ++i) c.put8(to + i, value[i]);
    c.put8(to + value.size(), 0);
}

std::string text_at(Cpu& c, std::uint32_t from) {
    std::string result;
    for (unsigned i = 0; i < 200; ++i) {
        const auto ch = c.u8(from + i);
        if (!ch) return result;
        result.push_back(char(ch));
    }
    throw std::runtime_error("Fixture name is missing its terminator");
}

std::uint32_t category_label(unsigned id) {
    if (id == 0) return 0x10142acc;
    if (id == 1) return 0x10142af8;
    return 0x10142b28 + (id - 2) * 0x30;
}

std::string fixture_category_phrase(unsigned offset, bool compact) {
    // Reverse the compact order and keep the full labels ascending. This
    // makes changing or losing the format flag observable in the actual sort.
    return std::to_string(compact ? 9000 - offset : 1000 + offset);
}

bool dependencies(State& s) {
    Cpu c(s);
    const auto pc = s.pc, lr = c.lr(), frame = c.sp();
    Call call{pc, lr};
    for (unsigned reg = 0; reg < 32; ++reg) call[reg + 2] = c.r(reg);
    switch (pc) {
    case 0x10746280:
        require(c.r(3) == source + 0x39 && c.r(4) == source + 0x43, "Hall of Fame path");
        c.set(3, file_path);
        break;
    case 0x10739de0: {
        const auto mode = c.r(4) - source;
        require(c.r(3) == file_path && (mode == 0x54 || mode == 0x57), "Hall of Fame open arguments");
        const bool write = mode == 0x57;
        const bool missing = (c.u8(fail_open) & (write ? 2 : 1)) != 0;
        if (!missing) {
            c.put32(file_position, 0);
            if (write) c.put32(file_size, 0);
        }
        c.set(3, missing ? 0 : file_handle);
        break;
    }
    case read_item_entry: case write_item_entry: {
        const auto buffer = c.r(3), bytes = c.r(4), position = c.u32(file_position);
        const bool write = pc == write_item_entry;
        require(c.r(5) == 1 && c.r(6) == file_handle &&
                    (position == 0 ? bytes == 8 : position == 8 ? bytes == table_size :
                     position == 8 + table_size && bytes == 4), "Hall of Fame stream transfer");
        const auto available = c.u32(file_size) - position;
        const auto transferred = write ? bytes - (c.u32(short_at) == position) : std::min(bytes, available);
        const auto file = file_contents + position;
        const auto from = write ? buffer : file, to = write ? file : buffer;
        if (transferred) s.memory.copy_in(to, s.memory.copy_out(from, transferred));
        c.put32(file_position, position + transferred);
        if (write) c.put32(file_size, position + transferred);
        c.set(3, transferred == bytes);
        break;
    }
    case 0x107766f0:
        require(c.r(3) == file_handle, "Hall of Fame close argument");
        c.set(3, UINT32_MAX); // The original disregards a close error.
        break;
    case 0x10773320:
        require(c.r(3) == table && c.r(4) == record_constructor && !c.r(5) && c.r(6) == row_size &&
                    c.r(7) == table_size / row_size, "Hall of Fame table constructor arguments");
        break;
    case 0x10778300:
        require(c.r(3) == table && !c.r(4) && c.r(5) == table_size, "Hall of Fame table clear arguments");
        s.memory.copy_in(table, std::vector<std::uint8_t>(table_size));
        break;
    case 0x10739890:
        require(c.r(3) == source && !c.r(4) && !c.r(5), "diagnostic filename");
        c.put8(c.r(6), 'H'); c.put8(c.r(7), 'C');
        break;
    case 0x1077b720:
        if (lr == 0x10140d00) {
            require(c.r(3) == frame + 0x270 && c.r(4) == source + 0x24, "club/nation format");
            put_text(c, c.r(3), text_at(c, c.r(5)) + "/" + text_at(c, c.r(6)));
        } else {
            require(c.r(3) == message && c.r(4) == source + 0x11 && c.r(5) == source + 0x50 &&
                        c.u8(c.r(6)) == 'H' && c.u8(c.r(7)) == 'C', "assertion format");
            c.put32(message, c.r(8));
        }
        c.set(3, 5);
        break;
    case 0x10769b60:
        if (lr == 0x101419f4)
            require(c.r(3) == frame + 0x10c && c.r(4) == frame + 0x170 && !c.r(5), "open error display");
        else
            require(c.r(3) == source + 0x1e && c.r(4) == message && !c.r(5), "assertion display");
        break;
    case 0x100caeb0:
        require(c.r(3) == person && c.r(4) == frame + 0x270 && c.r(5) == 200 && c.r(6) == 1,
                "manager name arguments");
        put_text(c, c.r(4), "Fixture Manager");
        c.set(3, c.r(4));
        break;
    case 0x1077dc10: {
        require(c.r(5) == 50 && c.r(4) == frame + 0x270, "bounded name copy");
        const auto value = text_at(c, c.r(4));
        const auto target = c.r(3);
        put_text(c, target, value);
        for (unsigned i = unsigned(value.size()) + 1; i < c.r(5); ++i) c.put8(target + i, 0);
        break;
    }
    case 0x100d0940:
        require((c.r(3) == club_name || c.r(3) == nation_name) && !c.r(4) && c.r(6) == 200 &&
                    c.r(7) == 1 && c.r(8) == 1, "team or nation name arguments");
        put_text(c, c.r(5), text_at(c, c.r(3)));
        c.set(3, c.r(5));
        break;
    case 0x1077dbf0:
        require(c.r(3) == frame + 0x270, "single-affiliation copy");
        put_text(c, c.r(3), text_at(c, c.r(4)));
        break;
    case 0x10727890:
        if (lr >= 0x10142ae0 && lr <= 0x10142fd0) {
            const auto offset = c.r(4) - source;
            const bool compact = std::int8_t(c.r(6)) != 0;
            require(offset >= 0x113 && offset <= 0x711 && c.r(3), "category localization arguments");
            if (compact && offset != 0x113)
                require(c.r(5) == source + 0x15c && !c.u8(c.r(5)), "compact category marker substitution");
            put_text(c, c.r(3), fixture_category_phrase(offset, compact));
        } else if (lr == 0x101419d8 || lr == 0x101419e4) {
            const bool title = lr == 0x101419d8;
            require(c.r(3) == frame + (title ? 0x10c : 0x170) && c.r(4) == source + (title ? 0x5a : 0x6f),
                    "open error formatting");
            put_text(c, c.r(3), title ? "Hall of Fame" : "Could not open the file");
        } else {
            require(c.r(3) == frame + 0x270 && c.r(4) == source + 0x2a, "fallback affiliation name");
            put_text(c, c.r(3), "No Club/Nation");
        }
        break;
    case 0x1077f900:
        require(!c.r(3), "ranking timestamp ABI");
        c.set(3, 0x80000001);
        break;
    case 0x1077dc90: {
        const auto left = text_at(c, c.r(3)), right = text_at(c, c.r(4));
        c.set(3, std::uint32_t(left.compare(right)));
        break;
    }
    case 0x10778390:
        require(c.r(3) >= table && c.r(3) < table + table_size && c.r(5) == row_size,
                "import duplicate comparison arguments");
        call_trace->push_back(call);
        return false;
    case 0x10768300: {
        if (lr == 0x1014315c) {
            require(c.r(3) == category_order && c.r(4) == 27 && c.r(5) == 1 && c.r(6) == category_descriptor,
                    "category sort arguments");
            for (unsigned i = 0; i < 27; ++i)
                require(c.u8(category_order + i) == i, "category list initialized before sort");
            call_trace->push_back(call);
            return false;
        }
        const auto bucket = c.r(3);
        require(bucket >= table && bucket < table + 27 * bucket_size && c.r(4) == 100 &&
                    c.r(5) == row_size && c.r(6) == rank_descriptor, "ranking sort arguments");
        if (lr == 0x10141170) {
            call_trace->push_back(call);
            return false;
        }
        const auto last = bucket + 99 * row_size;
        require(text_at(c, last) == "Fixture Manager" && c.u32(last + 0x64) == 0x80000001 &&
                    c.u32(last + 0x68) == 120 && c.u32(last + 0x6c) == 0x6789 &&
                    c.u16(last + 0x70) == 55 && c.u16(last + 0x72) == 2001 &&
                    c.u8(last + 0x74) == 0, "ranking record fields at the sort boundary");
        if (c.u8(real_sort)) {
            call_trace->push_back(call);
            return false;
        }
        break;
    }
    default:
        return false;
    }
    call_trace->push_back(call);
    c.finish();
    return true;
}

struct Model {
    decltype(dispatch_probe) previous = dispatch_probe;
    Model() { dispatch_probe = dependencies; }
    ~Model() { dispatch_probe = previous; call_trace = nullptr; }
};

enum class Stop { returned, unmapped, budget };
enum class Corrupt { none, result, dependency };

std::unique_ptr<State> compare_call(const State& initial, Stop expected, Corrupt corrupt = Corrupt::none,
                  bool trace_memory = true) {
    auto original = clone(initial), rewrite = clone(initial);
    Effects original_effects, rewrite_effects;
    std::vector<Call> original_calls, rewrite_calls;
    const auto invoke = [&](State& s, Effects& effects, std::vector<Call>& calls, bool old) {
        call_trace = &calls;
        // The generated caller tries these owners in place. Decline them
        // so both fixture paths reach the same dependency models.
        if (dispatch_probe) {
            localization_format_diagnostics(s).original_body = 0x10727890;
            s.time_runtime_original_body = 0x1077f900;
            s.stream_io_original = s.stream_buffer_original = true;
        }
        effects.state = &s;
        s.memory.date_observer = trace_memory ? &effects : nullptr;
        try {
            if (old) hall_of_fame_run_original(s, initial.pc);
            else run_until(s, done);
        } catch (const std::runtime_error& error) {
            const std::string_view message = error.what();
            s.memory.date_observer = nullptr;
            if (message == "Native instruction budget exhausted") return std::pair{Stop::budget, std::string(message)};
            if (message.starts_with("Unmapped data access at "))
                return std::pair{Stop::unmapped, std::string(message)};
            throw;
        }
        s.memory.date_observer = nullptr;
        require(s.pc == done, "Hall of Fame routine returned to a different address");
        return std::pair{Stop::returned, std::string{}};
    };
    const auto a = invoke(*original, original_effects, original_calls, true);
    const auto b = invoke(*rewrite, rewrite_effects, rewrite_calls, false);
    call_trace = nullptr;
    const auto label = "Hall of Fame " + hex_address(initial.pc) + " arg4 " +
        std::to_string(initial.r<4>(guest::reg::gpr(4)));
    require(a.first == expected && a == b, label + " stop or fault changed (" + hex_address(original->pc) + " " + a.second + " / " + hex_address(rewrite->pc) + " " + b.second + ")");
    if (corrupt == Corrupt::dependency) {
        require(!rewrite_calls.empty(), "dependency corruption did not reach a call");
        ++rewrite_calls.front()[6];
    }
    if (original_calls != rewrite_calls) {
        const auto difference = std::mismatch(original_calls.begin(), original_calls.end(), rewrite_calls.begin(), rewrite_calls.end());
        std::string detail = label + " ordered dependency arguments at call " + std::to_string(difference.first - original_calls.begin());
        if (difference.first != original_calls.end() && difference.second != rewrite_calls.end()) {
            const auto word = std::mismatch(difference.first->begin(), difference.first->end(), difference.second->begin());
            detail += " for " + hex_address(difference.first->front()) + " word " +
                std::to_string(word.first - difference.first->begin()) + " expected " +
                hex_address(*word.first) + " actual " + hex_address(*word.second);
        }
        throw std::runtime_error(detail);
    }
    if (original_effects.bytes != rewrite_effects.bytes) {
        const auto difference = std::mismatch(original_effects.log.begin(), original_effects.log.end(),
            rewrite_effects.log.begin(), rewrite_effects.log.end(), [](const auto& x, const auto& y) {
                return x.kind == y.kind && x.address == y.address && x.size == y.size && x.value == y.value;
            });
        const auto address = [](const auto& i, const auto& end) { return i == end ? "end" : hex_address(i->address); };
        throw std::runtime_error(label + " ordered memory effects at " +
            address(difference.first, original_effects.log.end()) + " / " + address(difference.second, rewrite_effects.log.end()));
    }
    if (corrupt == Corrupt::result) {
        Cpu c(*rewrite);
        c.set(3, c.r(3) ^ 1);
    }
    const auto sort = std::ranges::find_if(original_calls, [](const Call& call) {
        return call[0] == 0x10768300 && call[1] == 0x10140f34;
    });
    Cpu result(*rewrite);
    if (sort != original_calls.end() && result.u8(real_sort)) {
        const auto bucket = (*sort)[5];
        require(text_at(result, bucket) == "Fixture Manager" && result.u32(bucket + 0x68) == 120,
                "shared sorter places the new high score first");
    }
    Image(*original).compare(*rewrite, corrupt == Corrupt::result ? "corrupted comparator" :
        label, expected == Stop::unmapped);
    return rewrite;
}

std::unique_ptr<State> seed(std::mt19937& random) {
    auto state = std::make_unique<State>();
    state->memory.map(fixture, 0x1000);
    Cpu c(*state);
    for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
    state->wr<1>(guest::reg::xer_so, random() & 1);
    state->wr<1>(guest::reg::xer_ca, random() & 1);
    c.set({{3, first}, {4, second}});
    c.set_lr(done);
    state->pc = entry;
    state->budget = 100;
    return state;
}

void check_rank() {
    std::mt19937 random(0x484f464d);
    constexpr std::array values{0u, 1u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xfffffffeu, 0xffffffffu};
    unsigned completed = 0;
    for (auto a : values) {
        for (auto b : values) {
            for (const bool tie : {false, true}) {
                auto s = seed(random);
                Cpu c(*s);
                c.put32(first + 0x68, tie ? 10 : a);
                c.put32(second + 0x68, tie ? 10 : b);
                c.put32(first + 0x64, a);
                c.put32(second + 0x64, b);
                compare_call(*s, Stop::returned);
                ++completed;
            }
        }
    }
    for (auto left : {0u, first, absent}) {
        for (auto right : {0u, first, absent}) {
            auto s = seed(random);
            Cpu c(*s);
            c.set({{3, left}, {4, right}});
            const auto faults = left && right && (left == absent || right == absent);
            compare_call(*s, faults ? Stop::unmapped : Stop::returned);
            completed += !faults;
        }
    }
    auto s = seed(random);
    bool rejected = false;
    try { compare_call(*s, Stop::returned, Corrupt::result); }
    catch (const std::runtime_error& error) {
        rejected = std::string_view(error.what()).starts_with("corrupted comparator register byte=");
    }
    require(rejected, "wrong comparator result was not detected");
    s->budget = 1;
    compare_call(*s, Stop::budget);
    std::cout << completed << " Hall of Fame ranking comparisons, three mapped-range faults, "
                 "one budget stop and one corruption control\n";
}

void check_record_initialization() {
    std::mt19937 random(0x52454344);
    for (unsigned test = 0; test < 32; ++test) {
        auto s = seed(random);
        Cpu c(*s);
        s->memory.map(stack - 0x1000, 0x2000);
        for (unsigned offset = 0; offset < 0x100; offset += 4) c.put32(stack - offset, random());
        for (unsigned offset = 0; offset < 0x78; offset += 4) c.put32(first + offset, random());
        c.set({{1, stack}, {3, first}});
        s->pc = initialize;
        compare_call(*s, Stop::returned);
    }
    std::cout << "32 Hall of Fame record initialization comparisons with the shared date constructor\n";
}


constexpr std::array nation_slots{
    -0x7508, -0x7650, -0x7510, -0x7b68, -0x7590, -0x750c, -0x7a10, -0x7594, -0x7a00,
    -0x7af4, -0x7598, -0x7a28, -0x7574, -0x7a14, -0x7a24, -0x7578, -0x7504, -0x75a8,
    -0x79fc, -0x758c, -0x7a1c, -0x7a18, -0x7514, -0x75b0, -0x7a20, -0x757c};

std::unique_ptr<State> score_seed(std::mt19937& random) {
    auto state = seed(random);
    state->memory.map(table, 27 * bucket_size);
    state->memory.map(stack - 0x4000, 0x5000);
    state->memory.map(toc - 0x8000, 0x8000);
    Cpu c(*state);
    for (unsigned offset = 0; offset < 0x800; offset += 4) c.put32(stack - offset, random());
    c.set({{1, stack}, {2, toc}});
    c.put32(toc - 0x54cc, source);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x5b20, source + 0x50);
    c.put32(toc - 0x7bac, success);
    c.put32(toc - 0x7b80, today);
    c.put32(toc - 0x7450, game_id);
    c.put32(toc - 0x7b04, country_count);
    c.put32(toc - 0x7be4, staff_count);
    c.put32(toc - 0x54d0, rank_descriptor);
    c.put32(rank_descriptor, entry);
    c.put32(rank_descriptor + 4, toc);
    c.put8(success, 1);
    c.put16(today, 55);
    c.put16(today + 2, 2001);
    c.put32(game_id, 0x6789);
    c.put32(person, 195);
    c.put32(staff_count, 200);
    c.put32(country_count, 80);
    for (unsigned i = 0; i < nation_slots.size(); ++i) {
        c.put32(toc + std::uint32_t(nation_slots[i]), nation_values + i * 4);
        c.put32(nation_values + i * 4, i + 10);
    }
    put_text(c, club_name, "Fixture Club");
    put_text(c, nation_name, "Fixture Nation");
    state->budget = 200000;
    return state;
}

void check_records() {
    std::mt19937 random(0x46414d45);
    {
        Model model;
        for (unsigned id : {0u, 5u, 10u, 11u, 20u, 34u, 35u, 36u, 0xffffffffu}) {
            auto s = score_seed(random);
            Cpu c(*s);
            c.set(3, id);
            s->pc = category;
            compare_call(*s, Stop::returned);
        }
        for (unsigned id = 10; id <= 35; ++id) {
            auto s = score_seed(random);
            Cpu(*s).set(3, id);
            s->pc = category;
            compare_call(*s, Stop::returned);
        }
        for (const auto [cat, row] : std::array<std::pair<unsigned, unsigned>, 9>{{
                 {0u, 0u}, {0u, 99u}, {26u, 0u}, {26u, 99u},
                 {0x100u, 0x163u}, {27u, 0u}, {0xffu, 0u}, {0u, 100u}, {0u, 0x80u}}}) {
            for (unsigned alias : {0u, 1u, 2u}) {
                auto s = score_seed(random);
                Cpu c(*s);
                const bool valid = std::int8_t(cat) >= 0 && std::int8_t(cat) <= 26 &&
                                   std::int8_t(row) >= 0 && std::int8_t(row) <= 99;
                const auto address = valid ? table + (cat & 0xff) * bucket_size + (row & 0xff) * row_size : table;
                for (unsigned offset = 0; offset < row_size; offset += 4) c.put32(address + offset, random());
                c.set({{3, table}, {4, cat}, {5, row}, {6, alias ? address + (alias == 2 ? 2 : 0) : second}});
                s->pc = read_record;
                compare_call(*s, Stop::returned);
            }
        }
        for (const auto target : {add_default, add_country, insert}) {
            for (unsigned variant = 0; variant < 12; ++variant) {
                auto s = score_seed(random);
                Cpu c(*s);
                const auto person_arg = variant == 1 ? 0u : person;
                const auto kind = target == insert ? (variant == 2 ? 0xffu : variant == 3 ? 27u : 0u)
                                                   : (variant == 2 ? 0xffffffffu : variant == 3 ? 80u : 10u);
                const auto score = variant == 4 ? 0u : 120u;
                const auto bucket = target == add_country ? table + bucket_size : table;
                if (variant == 5) c.put32(person, 183);
                if (variant == 6) c.put32(bucket + 99 * row_size + 0x68, score);
                if (variant >= 7) {
                    put_text(c, bucket, "Fixture Manager");
                    c.put32(bucket + 0x6c, (variant == 7 || variant == 11) ? 0x678a : 0x6789);
                    c.put16(bucket + 0x70, variant == 8 ? 56 : 55);
                    c.put16(bucket + 0x72, variant == 9 ? 2002 : 2001);
                }
                if (variant & 1) c.put32(person + 0x24, club_name);
                if (variant & 2) c.put32(person + 0x20, nation_name);
                if (variant == 11) c.put8(real_sort, 1);
                c.set({{3, table}, {4, person_arg}, {5, kind}, {6, score}});
                if (target == add_default) c.set(5, score);
                if (target == insert) c.set({{4, kind}, {5, person_arg}});
                s->pc = target;
                try { compare_call(*s, Stop::returned); }
                catch (const std::exception& error) {
                    throw std::runtime_error("Scoring variant " + std::to_string(variant) + ": " + error.what());
                }
            }
        }
        auto corrupted = score_seed(random);
        Cpu(*corrupted).set({{3, table}, {4, person}, {5, 120}});
        corrupted->pc = add_default;
        bool rejected = false;
        try { compare_call(*corrupted, Stop::returned, Corrupt::dependency); }
        catch (const std::runtime_error& error) {
            rejected = std::string_view(error.what()).find("ordered dependency arguments") != std::string_view::npos;
        }
        require(rejected, "wrong scoring dependency argument was not detected");
    }
    for (const auto target : {add_default, add_country}) {
        // Exercise direct owned calls with every dispatcher probe disabled.
        auto s = score_seed(random);
        Cpu(*s).set({{3, table}, {4, person}, {5, target == add_country ? 10u : 0u}, {6, 0}});
        s->pc = target;
        compare_call(*s, Stop::returned);
    }
    std::cout << "100 Hall of Fame record and scoring comparisons, including aliased copies, duplicate keys and direct calls\n";
}


constexpr auto encode = 0x10142180u, decode = 0x101426c0u;
constexpr auto codec_data = fixture + 0x20000, codec_keys = fixture + 0x5000;
constexpr auto checksum_out = fixture + 0x990;

std::unique_ptr<State> codec_seed(std::mt19937& random, unsigned size) {
    auto state = seed(random);
    state->memory.map(stack - 0x2000, 0x3000);
    state->memory.map(toc - 0x8000, 0x8000);
    state->memory.map(codec_keys, 480 * 4);
    state->memory.map(codec_data - 0x1000, size + 0x2000);
    Cpu c(*state);
    for (unsigned offset = 0; offset < 0x100; offset += 4) c.put32(stack - offset, random());
    for (unsigned i = 0; i < 480; ++i) c.put32(codec_keys + i * 4, random() & 0x7fffffff);
    for (unsigned i = 0; i < size; ++i) c.put8(codec_data + i, random());
    c.set({{1, stack}, {2, toc}, {3, codec_data}, {4, size}});
    c.put32(toc - 0x54d8, codec_keys);
    c.put32(toc - 0x54cc, source);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x5b20, source + 0x50);
    c.put32(toc - 0x7bac, success);
    c.put8(success, 1);
    state->wr<4>(guest::reg::ctr, random());
    state->budget = 50000000;
    return state;
}

void check_codec() {
    std::mt19937 random(0x46414d43);
    unsigned comparisons = 0;
    for (unsigned size : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 15u, 16u, 17u,
                          23u, 24u, 25u, 479u, 480u, 481u, 487u, 488u, 489u, 960u, 961u, 324000u}) {
        auto state = codec_seed(random, size);
        Cpu c(*state);
        const auto plain = state->memory.copy_out(codec_data, size);
        std::uint32_t sum = 0;
        for (auto byte : plain) sum += byte;
        c.set(5, checksum_out);
        state->pc = encode;
        compare_call(*state, Stop::returned, Corrupt::none, size < 10000);
        ++comparisons;

        // Feed the independently generated original bytes into both decoders.
        hall_of_fame_run_original(*state, encode);
        require(c.r(3) == 1 && c.u32(checksum_out) == sum, "Hall of Fame plaintext checksum");
        c.set({{3, codec_data}, {4, size}, {5, sum}});
        c.set_lr(done);
        state->pc = decode;
        compare_call(*state, Stop::returned, Corrupt::none, size < 10000);
        ++comparisons;
        if (size < 10000) {
            c.set(5, sum + 1);
            compare_call(*state, Stop::returned);
            ++comparisons;
            c.set(5, sum);
        }
        run_until(*state, done);
        require(c.r(3) == 1 && state->memory.copy_out(codec_data, size) == plain,
                "Hall of Fame original/C++ round trip");
    }

    for (const auto target : {encode, decode}) {
        for (unsigned size : {1u, 3u, 4u, 8u, 9u, 16u, 17u}) {
            auto state = codec_seed(random, size);
            Cpu c(*state);
            c.put32(toc - 0x54d8, codec_data);
            // The key words can be edited by the same byte operations.
            // Padding around the fixture covers signed negative offsets.
            for (unsigned i = 0; i < 480; ++i) c.put32(codec_data + i * 4, random());
            c.set(5, target == encode ? codec_data + 1 : 0);
            state->pc = target;
            compare_call(*state, Stop::returned);
            ++comparisons;
        }
    }

    {
        Model model;
        for (auto target : {encode, decode}) {
            for (const auto [address, size] : std::array<std::pair<unsigned, unsigned>, 2>{{
                     {0, 4}, {codec_data, UINT32_MAX}}}) {
                auto state = codec_seed(random, 4);
                Cpu(*state).set({{3, address}, {4, size}, {5, checksum_out}});
                state->pc = target;
                compare_call(*state, Stop::returned);
                ++comparisons;
            }
        }
    }

    auto state = codec_seed(random, 0);
    Cpu(*state).set(5, 0);
    state->pc = encode;
    compare_call(*state, Stop::unmapped);
    state->budget = 1;
    compare_call(*state, Stop::budget);
    std::cout << comparisons << " Hall of Fame file transform comparisons, including 324000 bytes, "
                 "table and checksum aliasing, a bad checksum, one fault and one budget stop\n";
}

void check_persistence() {
    std::mt19937 random(0x46494c45);
    auto state = score_seed(random);
    state->memory.map(codec_keys, 480 * 4);
    state->memory.map(file_contents, file_capacity);
    state->budget = 50000000;
    Cpu c(*state);
    c.put32(toc - 0x54d8, codec_keys);
    c.put32(toc - 0x7404, record_constructor);
    for (unsigned i = 0; i < 480; ++i) c.put32(codec_keys + i * 4, random() & 0x7fffffff);
    for (unsigned i = 0; i < table_size; ++i) c.put8(table + i, random());
    const auto plain = state->memory.copy_out(table, table_size);
    const std::vector<std::uint8_t> empty(table_size);
    c.set({{3, table}, {4, table_size}, {5, checksum_out}});
    state->pc = encode;
    auto encoded_state = clone(*state);
    hall_of_fame_run_original(*encoded_state, encode);
    const auto encoded = encoded_state->memory.copy_out(table, table_size);
    const auto checksum = Cpu(*encoded_state).u32(checksum_out);
    require(encoded != plain, "persistence fixture must transform the table");

    Model model;
    struct ReadCase {
        unsigned bytes, magic, version, sum, line;
        bool missing, clears;
    };
    for (const auto test : std::array{
            ReadCase{file_capacity, 0x003abfb2, 1, checksum, 0, false, false},
            ReadCase{file_capacity, 0x003abfb2, 1, checksum, 0, true, false},
            ReadCase{7, 0x003abfb2, 1, checksum, 0x1bc, false, false},
            ReadCase{file_capacity, 0x003abfb3, 1, checksum, 0x1c4, false, false},
            ReadCase{file_capacity, 0x003abfb2, 2, checksum, 0x1cc, false, false},
            ReadCase{8 + table_size - 1, 0x003abfb2, 1, checksum, 0x1d4, false, true},
            ReadCase{file_capacity - 1, 0x003abfb2, 1, checksum, 0x1dd, false, true},
            ReadCase{file_capacity, 0x003abfb2, 1, checksum ^ 1, 0x1e6, false, true}}) {
        state->memory.copy_in(file_contents + 8, encoded);
        c.put32(file_contents, test.magic);
        c.put8(file_contents + 4, test.version);
        c.put32(file_contents + 8 + table_size, test.sum);
        c.put32(file_size, test.bytes);
        c.put8(fail_open, test.missing);
        state->memory.copy_in(table, std::vector<std::uint8_t>(table_size, 0xa5));
        const auto initial = state->memory.copy_out(table, table_size);
        c.set(3, table);
        state->pc = read_file;
        auto result = compare_call(*state, Stop::returned, Corrupt::none, false);
        Cpu after(*result);
        const bool ok = !test.missing && !test.line;
        require(after.r(3) == ok && after.u32(message) == test.line &&
                    after.u8(success) == !test.line, "load result and diagnostic");
        require(result->memory.copy_out(table, table_size) == (ok ? plain : test.clears ? empty : initial),
                "load table result, including the clear-on-failure boundary");
    }

    struct WriteCase { unsigned short_position, line; bool missing, null_table; };
    for (const auto test : std::array{
            WriteCase{UINT32_MAX, 0, false, false},
            WriteCase{UINT32_MAX, 0, true, false},
            WriteCase{0, 0x288, false, false},
            WriteCase{UINT32_MAX, 0x290, false, true},
            WriteCase{8, 0x298, false, false},
            WriteCase{8 + table_size, 0x2a0, false, false}}) {
        c.put32(file_size, 0);
        c.put32(short_at, test.short_position);
        c.put8(fail_open, test.missing ? 2 : 0);
        state->memory.copy_in(table, plain);
        c.set(3, test.null_table ? 0 : table);
        state->pc = write_file;
        auto result = compare_call(*state, Stop::returned, Corrupt::none, false);
        Cpu after(*result);
        const bool ok = !test.missing && !test.line;
        const bool leaves_encoded = test.line == 0x298 || test.line == 0x2a0;
        require(after.r(3) == ok && after.u32(message) == test.line &&
                    after.u8(success) == !test.line, "save result and diagnostic");
        require(result->memory.copy_out(table, table_size) == (leaves_encoded ? encoded : plain),
                "save table result, including the encoded short-write state");
        if (ok)
            require(after.u32(file_size) == file_capacity && after.u32(file_contents) == 0x003abfb2 &&
                        after.u8(file_contents + 4) == 1 &&
                        result->memory.copy_out(file_contents + 8, table_size) == encoded &&
                        after.u32(file_contents + 8 + table_size) == checksum, "save bytes");
    }

    c.put32(short_at, UINT32_MAX);
    c.put8(fail_open, 0);
    c.set(3, table);
    state->pc = write_file;
    bool rejected = false;
    try { compare_call(*state, Stop::returned, Corrupt::dependency, false); }
    catch (const std::runtime_error& error) {
        rejected = std::string_view(error.what()).find("ordered dependency arguments") != std::string_view::npos;
    }
    require(rejected, "file dependency trace corruption must fail");

    for (const unsigned open_mask : {0u, 1u, 3u}) {
        state->memory.copy_in(file_contents + 8, encoded);
        c.put32(file_contents, 0x003abfb2);
        c.put8(file_contents + 4, 1);
        c.put32(file_contents + 8 + table_size, checksum);
        c.put32(file_size, file_capacity);
        c.put32(short_at, UINT32_MAX);
        c.put8(fail_open, open_mask);
        c.set(3, table);
        state->pc = construct_table;
        auto result = compare_call(*state, Stop::returned, Corrupt::none, false);
        Cpu after(*result);
        require(after.r(3) == table && result->memory.copy_out(table, table_size) == (open_mask ? empty : plain),
                "table construction initializes before loading");
        require(after.u32(file_contents + 8 + table_size) == (open_mask == 1 ? 0u : checksum),
                "table construction writes a new empty table only when required");
    }
    std::cout << "17 Hall of Fame persistence and table construction comparisons with ordered dependencies and file bytes\n";
}

void check_import() {
    std::mt19937 random(0x494d5054);
    auto state = score_seed(random);
    state->memory.map(codec_keys, 480 * 4);
    state->memory.map(file_contents, file_capacity);
    state->memory.map(imported_table, table_size);
    state->budget = 50000000;
    Cpu c(*state);
    c.put32(toc - 0x54d8, codec_keys);
    c.put32(toc - 0x54d4, imported_table);
    c.put32(toc - 0x7414, active_table_slot);
    c.put32(active_table_slot, table);
    for (unsigned i = 0; i < 480; ++i) c.put32(codec_keys + i * 4, random() & 0x7fffffff);

    const auto set_row = [](Cpu& cpu, std::uint32_t row, unsigned score, std::int32_t stamp) {
        cpu.put32(row + 0x64, std::uint32_t(stamp));
        cpu.put32(row + 0x68, score);
    };
    const auto candidate = imported_table + 5 * row_size;
    Model model;
    unsigned comparisons = 0;
    struct RankCase {
        unsigned category, score, cutoff, duplicate;
        bool null_record, qualifies, different_padding = false;
    };
    for (const auto test : std::array{
            RankCase{0, 120, 0, 100, false, true},
            RankCase{0, 120, 0, 0, false, false},
            RankCase{0, 120, 0, 98, false, false},
            RankCase{0, 120, 0, 98, false, true, true},
            RankCase{0, 120, 0, 99, false, false},
            RankCase{0, 120, 120, 100, false, false},
            RankCase{0, 120, 121, 100, false, false},
            RankCase{0, 0, 0, 100, false, false},
            RankCase{0x1234001a, 0x80000001, 0, 100, false, true},
            RankCase{0x80, 120, 0, 100, false, false},
            RankCase{27, 120, 0, 100, false, false},
            RankCase{0, 120, 0, 100, true, false}}) {
        const auto bucket = table + ((test.category & 0xff) == 26 ? 26 : 0) * bucket_size;
        const auto bottom = bucket + 99 * row_size;
        state->memory.copy_in(table, std::vector<std::uint8_t>(table_size));
        set_row(c, bottom, test.cutoff, 0);
        set_row(c, candidate, test.score, 4);
        // The comparison includes trailing padding that the subsequent copy omits.
        c.put8(candidate + 0x77, 0xad);
        if (test.duplicate < 100) {
            const auto existing = bucket + test.duplicate * row_size;
            state->memory.copy_in(existing, state->memory.copy_out(candidate, row_size));
            if (test.different_padding) c.put8(existing + 0x77, 0);
        }
        c.set({{3, table}, {4, test.category}, {5, test.null_record ? 0 : candidate}});
        state->pc = merge_record;
        auto result = compare_call(*state, Stop::returned);
        ++comparisons;
        Cpu after(*result);
        const bool invalid = test.null_record || (test.category & 0xff) > 26;
        require(after.r(3) == test.qualifies && after.u8(success) == !invalid,
                "rank import duplicate and cutoff result");
        if (test.qualifies)
            require(after.u32(bucket + 0x68) == test.score && after.u32(bucket + 0x64) == 4,
                    "shared sorter places imported rank");
    }

    state->memory.copy_in(table, std::vector<std::uint8_t>(table_size));
    state->memory.copy_in(imported_table, std::vector<std::uint8_t>(table_size));
    set_row(c, table, 140, 4);
    set_row(c, candidate, 140, 4); // duplicate of the live first row
    set_row(c, candidate + row_size, 240, 5);
    set_row(c, candidate + 2 * row_size, 900, -1); // nonpositive timestamps are skipped
    set_row(c, candidate + 3 * row_size, 950, 0);
    const auto final_bucket = table + 26 * bucket_size;
    set_row(c, imported_table + table_size - row_size, 300, 9);
    const auto live = state->memory.copy_out(table, table_size);
    const auto plain_import = state->memory.copy_out(imported_table, table_size);
    auto encoded_state = clone(*state);
    Cpu(*encoded_state).set({{3, imported_table}, {4, table_size}, {5, checksum_out}});
    encoded_state->pc = encode;
    hall_of_fame_run_original(*encoded_state, encode);
    const auto sum = Cpu(*encoded_state).u32(checksum_out);
    state->memory.copy_in(file_contents + 8, encoded_state->memory.copy_out(imported_table, table_size));
    const std::vector<std::uint8_t> blank_import(table_size, 0xf2);

    struct ImportCase {
        unsigned bytes, magic, version, checksum, line;
        bool has_path, opens;
    };
    const std::array cases{
        ImportCase{file_capacity, 0x003abfb2, 1, sum, 0, true, true},
        ImportCase{file_capacity, 0x003abfb2, 1, sum, 0x20b, false, true},
        ImportCase{file_capacity, 0x003abfb2, 1, sum, 0x214, true, false},
        ImportCase{7, 0x003abfb2, 1, sum, 0x21c, true, true},
        ImportCase{file_capacity, 0x003abfb3, 1, sum, 0x225, true, true},
        ImportCase{file_capacity, 0x003abfb2, 2, sum, 0x22e, true, true},
        ImportCase{8 + table_size - 1, 0x003abfb2, 1, sum, 0x237, true, true},
        ImportCase{file_capacity - 1, 0x003abfb2, 1, sum, 0x240, true, true},
        ImportCase{file_capacity, 0x003abfb2, 1, sum ^ 1, 0x249, true, true}};
    const auto prepare = [&](ImportCase test) {
        c.put32(file_contents, test.magic);
        c.put8(file_contents + 4, test.version);
        c.put32(file_contents + 8 + table_size, test.checksum);
        c.put32(file_size, test.bytes);
        c.put8(fail_open, !test.opens);
        state->memory.copy_in(imported_table, blank_import);
    };
    for (const auto test : cases) {
        for (unsigned mode : {0x100u, 0x180u}) {
            prepare(test);
            c.set({{3, table}, {4, test.has_path ? file_path : 0}, {5, mode}});
            state->pc = import_file;
            auto result = compare_call(*state, Stop::returned, Corrupt::none, false);
            ++comparisons;
            Cpu after(*result);
            const bool validation_only = (mode & 0xff) != 0;
            const auto line = validation_only ? 0 : test.line;
            require(after.r(3) == !test.line && after.u32(message) == line && after.u8(success) == !line,
                    "file import result and silent-validation diagnostic");
            if (test.line || validation_only)
                require(result->memory.copy_out(table, table_size) == live, "validation never changes live ranks");
            else
                require(after.u32(table + 0x68) == 240 && after.u32(table + row_size + 0x68) == 140 &&
                            !after.u32(table + 2 * row_size + 0x68) && after.u32(final_bucket + 0x68) == 300,
                        "file merges positive, nonduplicate ranks across categories");
            if (!test.line || test.line == 0x249)
                require(result->memory.copy_out(imported_table, table_size) == plain_import,
                        "import scratch retains the decoded bytes even if the checksum mismatches");
        }
    }

    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        prepare(scenario == 1 ? cases.back() : cases.front());
        c.put32(active_table_slot, scenario == 2 ? 0 : table);
        c.set(3, file_path);
        state->pc = validate_file;
        auto result = compare_call(*state, Stop::returned, Corrupt::none, false);
        ++comparisons;
        Cpu after(*result);
        require(after.r(3) == (scenario == 0 ? 1u : 2u) && after.u8(success) == 1 &&
                    result->memory.copy_out(table, table_size) == live,
                "file picker callback reports validation without merging or displaying errors");
    }
    std::cout << comparisons << " Hall of Fame import and ranked merge comparisons with the shared comparator and sorter\n";
}

void check_categories() {
    std::mt19937 random(0x4c414245);
    auto state = score_seed(random);
    state->memory.map(category_switch, 0x800);
    Cpu c(*state);
    c.put32(toc - 0x54dc, category_switch);
    c.put32(toc - 0x54e0, category_order);
    c.put32(toc - 0x54e4, category_descriptor);
    c.put32(toc - 0x3e3c, characters);
    c.put8(toc - 0x35ac, 1);
    c.put32(category_descriptor, compare_categories);
    c.put32(category_descriptor + 4, toc);
    for (unsigned i = 0; i < 27; ++i) c.put32(category_switch + 4 * i, category_label(i));
    for (unsigned i = 0; i < 256; ++i)
        for (unsigned column = 0; column < 3; ++column) c.put8(characters + 3 * i + column, i);

    Model model;
    unsigned comparisons = 0;
    const auto check_name = [&](std::uint32_t id, std::uint32_t out, std::uint32_t size, std::uint32_t mode,
                                bool valid) {
        put_text(c, category_output, "unchanged");
        c.set({{3, id}, {4, out}, {5, size}, {6, mode}});
        state->pc = category_name;
        auto result = compare_call(*state, Stop::returned);
        ++comparisons;
        Cpu after(*result);
        require(after.r(3) == valid && after.u8(success) == valid &&
                    after.u32(message) == (valid ? 0u : 0x3d8u), "category label result and assertion");
        const auto rendered = text_at(after, category_output);
        if (valid) require(rendered.size() == 4 && rendered != "unchanged", "category label uses formatter output");
        else require(rendered == "unchanged", "bad label leaves output buffer alone");
        return rendered;
    };
    for (unsigned i = 0; i < 27; ++i)
        for (unsigned mode : {0x100u, 0x180u})
            check_name(0xabc00000 | i, category_output, 50, mode, true);
    require(check_name(0, category_output, 200, 1, true) == fixture_category_phrase(0x113, true) &&
                check_name(26, category_output, 200, 0, true) == fixture_category_phrase(0x711, false),
            "boundary category names use the right language key");
    c.put32(category_switch, category_label(26));
    require(check_name(0, category_output, 50, 0, true) == fixture_category_phrase(0x711, false),
            "category selection reads the live jump table");
    c.put32(category_switch, category_label(0));
    check_name(0x80, category_output, 50, 0, false);
    check_name(27, category_output, 50, 0, false);
    check_name(0, 0, 50, 0, false);
    check_name(0, category_output, 49, 0, false);
    check_name(0, category_output, UINT32_MAX, 0, false);

    constexpr unsigned null = 256;
    struct CompareCase { unsigned left, right; int full, compact; };
    for (auto test : std::array{
            CompareCase{null, null, 1, 1}, CompareCase{null, 1, 1, 1}, CompareCase{1, null, -1, -1},
            CompareCase{0, 1, -1, -1}, CompareCase{1, 0, 1, 1}, CompareCase{0, 0, 0, 0},
            CompareCase{2, 26, -1, 1}, CompareCase{26, 2, 1, -1}, CompareCase{17, 17, 0, 0}}) {
        for (unsigned mode : {0u, 1u}) {
            c.put8(first, test.left);
            c.put8(second, test.right);
            c.put8(toc + std::uint32_t(category_mode), mode);
            c.set({{3, test.left == null ? 0 : first}, {4, test.right == null ? 0 : second}});
            state->pc = compare_categories;
            auto result = compare_call(*state, Stop::returned);
            ++comparisons;
            require(std::int32_t(Cpu(*result).r(3)) == (mode ? test.compact : test.full),
                    "category order uses special cases and shared text comparison");
        }
    }

    for (unsigned mode : {0x100u, 0x180u}) {
        c.set(3, mode);
        state->pc = sorted_categories;
        auto result = compare_call(*state, Stop::returned);
        ++comparisons;
        Cpu after(*result);
        require(after.r(3) == category_order && after.u8(category_order) == 0 &&
                    after.u8(toc + std::uint32_t(category_mode)) == (mode & 0xff),
                "sort returns the category list and keeps the selected format");
        for (unsigned i = 1; i < 27; ++i)
            require(after.u8(category_order + i) == ((mode & 0xff) ? 27 - i : i),
                    "actual sorter uses the selected category names after the fixed first row");
    }
    std::cout << comparisons << " Hall of Fame category comparisons with localization arguments and actual sorting\n";
}

void check_category_fallback(const char* program) {
    auto state = std::make_unique<State>();
    load_program(*state, program);
    state->memory.map(fixture, 0x3000);
    state->budget = 200000;
    Cpu c(*state);
    c.set({{1, stack}, {2, toc}});
    c.set_lr(done);
    c.put32(toc - 0x54cc, source);
    c.put8(toc - 0x35ac, 0xff);
    put_text(c, source + 0x130, "Aster<%s - COMMENT - menu>");
    put_text(c, source + 0x15d, "Aster League");
    for (unsigned mode : {0u, 1u}) {
        c.set({{3, 1}, {4, category_output}, {5, 200}, {6, mode}});
        state->pc = category_name;
        auto result = compare_call(*state, Stop::returned);
        Cpu after(*result);
        require(after.r(3) == 1 && text_at(after, category_output) == (mode ? "Aster" : "Aster League"),
                "shared formatter renders category labels when translations are unavailable");
    }
    std::cout << "2 Hall of Fame category comparisons with the actual formatter fallback\n";
}

}

int main(int argc, char** argv) try {
    if (argc == 2 && std::string_view(argv[1]) == "--persistence") {
        check_persistence();
        return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--import") {
        check_import();
        return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--categories") {
        check_categories();
        return 0;
    }
    if (argc == 3 && std::string_view(argv[1]) == "--category-fallback") {
        check_category_fallback(argv[2]);
        return 0;
    }
    require(argc == 1, "unknown Hall of Fame checker option");
    check_rank();
    check_record_initialization();
    check_records();
    check_codec();
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
