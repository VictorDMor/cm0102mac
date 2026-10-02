#include "calendar.hpp"
#include "calendar_adapter.hpp"
#include "callbacks.hpp"
#include <chrono>
#include <iostream>
#include <set>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, entry = 0x10718130;
struct Writes final : MemoryObserver {
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> sequence;
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Difference mapped memory"); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
        sequence.emplace_back(address, std::vector<std::uint8_t>(bytes, bytes + size));
    }
    void cleared(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Difference cleared memory"); }
};
void require(bool test, const std::string& message) { if (!test) throw std::runtime_error(message); }
void initialize(State& s, unsigned year, unsigned day, unsigned second_year, unsigned second_day,
                unsigned alias, unsigned index) {
    s.memory.observer = nullptr;
    const auto stack = std::uint32_t(s.r<4>(4)), toc = std::uint32_t(s.r<4>(8));
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    for (unsigned r = 0; r < 32; ++r) s.wr<4>(r * 4, 0x12340000 + r);
    s.wr<4>(4, stack); s.wr<4>(8, toc); s.wr<4>(4128, return_sentinel);
    s.memory.copy_in(fixture, std::vector<std::uint8_t>(256, 0xa5));
    s.memory.copy_in(stack - 4096, std::vector<std::uint8_t>(4160, 0xa7));
    const auto scratch = fixture + 64;
    s.memory.write<4>(0x108b7ad0, scratch);
    s.memory.write<1>(0x108b84d0, index % 3 == 0 ? 0 : index % 3 == 1 ? 1 : 128);
    const auto first = alias == 0 ? fixture + 16 : scratch + alias - 3;
    s.memory.write<2>(first, day); s.memory.write<2>(first + 2, year);
    s.memory.write<1>(first + 4, index % 2 ? 255 : 0);
    s.wr<4>(12, first);
    s.wr<4>(16, (second_day << 16) | second_year);
    s.wr<4>(20, index % 2 ? 0xff123456 : 0x0089abcd);
    s.wr<1>(2304, 0xf9); s.wr<1>(1024, index % 2); s.wr<1>(1027, 1);
    s.pc = entry; s.executed = 0; s.native_calls = 0; s.budget = 6000000;
    s.calendar_calls = {}; s.date_difference_calls = 0;
}
void same_memory(const State& a, const State& b) {
    require(a.memory.mapped_ranges() == b.memory.mapped_ranges(), "Difference mapped ranges differ");
    for (const auto& [start, size] : a.memory.mapped_ranges())
        for (std::uint64_t offset = 0; offset < size; offset += 65536) {
            const auto length = std::size_t(std::min<std::uint64_t>(size - offset, 65536));
            require(a.memory.copy_out(start + std::uint32_t(offset), length) ==
                    b.memory.copy_out(start + std::uint32_t(offset), length),
                    "Difference full memory mismatch at " + hex_address(start + std::uint32_t(offset)));
        }
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        auto a = std::make_unique<State>(), b = std::make_unique<State>();
        load_program(*a, argv[1]); load_program(*b, argv[1]);
        a->calendar_original = true;
        for (auto* s : {a.get(), b.get()}) s->memory.map(fixture, 256);
        unsigned count = 0, oracle = 0;
        auto check = [&](unsigned y, unsigned d, unsigned sy, unsigned sd, unsigned alias) {
            for (auto* s : {a.get(), b.get()}) initialize(*s, y, d, sy, sd, alias, count);
            const auto first = std::uint32_t(a->r<4>(12));
            const auto before = a->memory.copy_out(first, 5);
            Writes aw, bw; a->memory.observer = &aw; b->memory.observer = &bw;
            run(*a); run(*b); a->memory.observer = nullptr; b->memory.observer = nullptr;
            const auto context = " case=" + std::to_string(count) + " y=" + std::to_string(y) +
                " d=" + std::to_string(d) + " sy=" + std::to_string(sy) + " sd=" + std::to_string(sd) +
                " alias=" + std::to_string(alias);
            if (a->registers != b->registers) {
                for (unsigned i = 0; i < a->registers.size(); ++i)
                    if (a->registers[i] != b->registers[i]) std::cerr << "register byte " << i << " original=" << unsigned(a->registers[i])
                        << " modern=" << unsigned(b->registers[i]) << '\n';
                throw std::runtime_error("Difference direct ABI mismatch" + context);
            }
            require(a->unique == b->unique && a->pc == b->pc, "Difference temporary/PC mismatch" + context);
            require(aw.sequence == bw.sequence, "Difference ordered writes/footprint mismatch" + context);
            same_memory(*a, *b);
            require(a->allocated == b->allocated && a->available == b->available && a->heap_end == b->heap_end &&
                    a->memory_error == b->memory_error, "Difference allocator mismatch" + context);
            require(!a->native_calls && !b->native_calls && b->date_difference_calls == 1 &&
                    b->calendar_calls == std::array<std::uint64_t, 2>{}, "Difference semantic entry/import counter mismatch" + context);
            if (alias == 0) {
                require(a->memory.copy_out(first, 5) == before, "Difference modified separate input" + context);
                const auto expected = cm::calendar::difference({cm::calendar::narrow_day(d), std::uint16_t(y)},
                                                              {cm::calendar::narrow_day(sd), std::uint16_t(sy)});
                require(a->r<4>(12) == std::uint32_t(expected), "Pure semantic result mismatch" + context);
                // Independent oracle only where applicable: all same-year days,
                // and Gregorian cross-year dates where displaced lengths agree.
                if (y == sy || ((y > sy || cm::calendar::year_length(std::uint16_t(y - 1)) ==
                                          cm::calendar::year_length(std::uint16_t(sy - 1))) && y <= 2107 && sy <= 2107 &&
                                d < cm::calendar::year_length(y) && sd < cm::calendar::year_length(sy))) {
                    using namespace std::chrono;
                    const auto result = y == sy ? int(cm::calendar::narrow_day(d)) - int(cm::calendar::narrow_day(sd)) :
                        int((sys_days(year(int(y))/January/1) + days(d) -
                             (sys_days(year(int(sy))/January/1) + days(sd))).count());
                    require(expected == result, "Independent Gregorian domain oracle mismatch" + context); ++oracle;
                }
            }
            ++count;
        };
        for (unsigned y : {0u, 1u, 4u, 100u, 400u, 1900u, 1999u, 2000u, 2001u, 2004u, 2100u, 32767u, 65535u})
            for (int delta : {-7, -2, -1, 0, 1, 2, 7}) {
                const auto sy = int(y) + delta; if (sy < 0 || sy > 65535) continue;
                for (const auto& [d, sd] : std::array<std::pair<unsigned, unsigned>, 12>{{
                    {0,0}, {0,1}, {1,0}, {58,59}, {59,60}, {60,59}, {364,0}, {365,366},
                    {32767,32768}, {32768,32767}, {65535,0}, {0,65535}}})
                    for (unsigned alias = 0; alias < 7; ++alias) check(y, d, unsigned(sy), sd, alias);
            }
        for (const auto& [y, sy] : std::array<std::pair<unsigned,unsigned>, 4>{{{0,65535},{65535,0},{1900,2100},{2100,1900}}})
            check(y, 59, sy, 60, 0);
        require(cm::calendar::difference({0,1999},{0,2000}) == -365 &&
                cm::calendar::difference({0,2000},{0,2001}) == -365 &&
                cm::calendar::difference({0,2001},{0,2000}) == 366,
                "Ascending leap quirk was silently corrected");
        // Callback marshalling is distinct from the unmasked direct ABI checks.
        initialize(*b, 2000, 59, 2000, 60, 0, 1);
        b->memory.write<4>(fixture, entry); b->memory.write<4>(fixture + 4, b->r<4>(8));
        const auto registers = b->registers; const auto unique = b->unique;
        const auto result = call_original(*b, fixture, std::array<std::uint32_t,3>{fixture + 16, (60u << 16) | 2000, 0xff123456});
        require(result == 0xffffffff && b->registers == registers && b->unique == unique && b->date_difference_calls == 1,
                "Difference callback argument/restoration mismatch");
        // Real translated inter-page bl and indirect bctr entry paths.
        for (const auto pc : {0x1002bbb0u, 0x1000a494u}) {
            initialize(*b, 2000, 59, 2000, 60, 0, 1);
            b->pc = pc; b->wr<4>(4132, entry);
            require(dispatch_page(*b) && b->pc == entry, "Difference caller did not dispatch to entry");
            const auto return_pc = std::uint32_t(b->r<4>(4128));
            require(dispatch_page(*b) && b->pc == return_pc && b->r<4>(12) == 0xffffffff && b->date_difference_calls == 1,
                    "Difference caller missed semantic entry");
        }
        std::cout << count << " direct original/modern difference cases: full registers, temporaries, PC, ordered writes, entire mapped memory, allocator and no imports/RNG; "
                  << oracle << " independent domain oracle cases; callback dispatch; preserved ascending leap quirk\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
