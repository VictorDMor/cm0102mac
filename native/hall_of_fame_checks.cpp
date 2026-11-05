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

bool dependencies(State& s) {
    Cpu c(s);
    const auto pc = s.pc, lr = c.lr(), frame = c.sp();
    Call call{pc, lr};
    for (unsigned reg = 0; reg < 32; ++reg) call[reg + 2] = c.r(reg);
    switch (pc) {
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
        require(c.r(3) == frame + 0x270 && c.r(4) == source + 0x2a, "fallback affiliation name");
        put_text(c, c.r(3), "No Club/Nation");
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
    case 0x10768300: {
        const auto bucket = c.r(3);
        require(bucket >= table && bucket < table + 27 * bucket_size && c.r(4) == 100 &&
                    c.r(5) == row_size && c.r(6) == rank_descriptor, "ranking sort arguments");
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

void compare_call(const State& initial, Stop expected, Corrupt corrupt = Corrupt::none) {
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
        }
        effects.state = &s;
        s.memory.date_observer = &effects;
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
    const auto label = "Hall of Fame " + hex_address(initial.pc);
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
    if (std::ranges::any_of(original_calls, [](const Call& call) { return call[0] == 0x10768300; })) {
        Cpu result(*rewrite);
        if (result.u8(real_sort)) {
            const auto sort = std::ranges::find_if(original_calls, [](const Call& call) { return call[0] == 0x10768300; });
            const auto bucket = (*sort)[5];
            require(text_at(result, bucket) == "Fixture Manager" && result.u32(bucket + 0x68) == 120,
                    "shared sorter places the new high score first");
        }
    }
    Image(*original).compare(*rewrite, corrupt == Corrupt::result ? "corrupted comparator" : "Hall of Fame " + hex_address(initial.pc), expected == Stop::unmapped);
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

}

int main() try {
    check_rank();
    check_record_initialization();
    check_records();
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
