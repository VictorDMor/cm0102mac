#include "calendar.hpp"
#include "calendar_adapter.hpp"
#include "callbacks.hpp"
#include <chrono>
#include <iostream>
#include <utility>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, entry = 0x10331860;
struct Writes final : MemoryObserver {
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> sequence;
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Ordering mapped memory"); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
        sequence.emplace_back(address, std::vector<std::uint8_t>(bytes, bytes + size));
    }
    void cleared(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Ordering cleared memory"); }
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
    const auto first = alias ? stack + 23 + alias : fixture + 16;
    s.memory.write<2>(first, day); s.memory.write<2>(first + 2, year);
    s.memory.write<1>(first + 4, index % 2 ? 255 : 0);
    s.wr<4>(12, first);
    s.wr<4>(16, (second_day << 16) | second_year);
    constexpr std::array<unsigned,4> tails{0x00123456,0x0189abcd,0x80000000,0xffffffff};
    s.wr<4>(20, tails[index % tails.size()]);
    s.wr<1>(2304, index % 2 ? 0xf9 : 0x06); s.wr<1>(1024, index % 2);
    s.wr<1>(1027, (index / 2) % 2);
    s.pc = entry; s.executed = 0; s.native_calls = 0; s.budget = 100;
    s.calendar_calls = {}; s.date_difference_calls = 0; s.date_ordering_calls = 0;
}
void same_memory(const State& a, const State& b) {
    require(a.memory.mapped_ranges() == b.memory.mapped_ranges(), "Ordering mapped ranges differ");
    for (const auto& [start, size] : a.memory.mapped_ranges())
        for (std::uint64_t offset = 0; offset < size; offset += 65536) {
            const auto length = std::size_t(std::min<std::uint64_t>(size - offset, 65536));
            require(a.memory.copy_out(start + std::uint32_t(offset), length) ==
                    b.memory.copy_out(start + std::uint32_t(offset), length),
                    "Ordering full memory mismatch at " + hex_address(start + std::uint32_t(offset)));
        }
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        auto a = std::make_unique<State>(), b = std::make_unique<State>();
        load_program(*a, argv[1]); load_program(*b, argv[1]); a->calendar_original = true;
        for (auto* s : {a.get(), b.get()}) s->memory.map(fixture, 256);
        unsigned count = 0, oracle = 0;
        auto check = [&](unsigned y, unsigned d, unsigned sy, unsigned sd, unsigned alias) {
            for (auto* s : {a.get(), b.get()}) initialize(*s, y, d, sy, sd, alias, count);
            const auto first = std::uint32_t(a->r<4>(12)), stack = std::uint32_t(a->r<4>(4));
            const auto before = a->memory.copy_out(first, 5);
            const auto second = a->r<4>(16), tail = a->r<4>(20);
            Writes aw, bw; a->memory.observer = &aw; b->memory.observer = &bw;
            run(*a); run(*b); a->memory.observer = nullptr; b->memory.observer = nullptr;
            const auto context = " case=" + std::to_string(count) + " y=" + std::to_string(y) +
                " d=" + std::to_string(d) + " sy=" + std::to_string(sy) +
                " sd=" + std::to_string(sd) + " alias=" + std::to_string(alias);
            if (a->registers != b->registers) {
                for (unsigned i = 0; i < a->registers.size(); ++i)
                    if (a->registers[i] != b->registers[i]) std::cerr << "register byte " << i << " original=" << unsigned(a->registers[i])
                        << " modern=" << unsigned(b->registers[i]) << '\n';
                throw std::runtime_error("Ordering direct ABI mismatch" + context);
            }
            require(a->unique == b->unique && a->pc == b->pc, "Ordering temporary/PC mismatch" + context);
            require(aw.sequence == bw.sequence && aw.sequence.size() == 2 &&
                    aw.sequence[0].first == stack + 28 && aw.sequence[0].second.size() == 4 &&
                    aw.sequence[1].first == stack + 32 && aw.sequence[1].second.size() == 4,
                    "Ordering exact spill sequence mismatch" + context);
            require(a->memory.read<4>(stack + 28) == second && a->memory.read<4>(stack + 32) == tail,
                    "Ordering by-value padding lost" + context);
            same_memory(*a, *b);
            require(a->allocated == b->allocated && a->available == b->available && a->heap_end == b->heap_end &&
                    a->memory_error == b->memory_error, "Ordering allocator mismatch" + context);
            require(!a->native_calls && !b->native_calls && b->date_ordering_calls == 1 && !b->date_difference_calls &&
                    b->calendar_calls == std::array<std::uint64_t, 2>{}, "Ordering entry/import counter mismatch" + context);
            // No writes follow the spills. Read the effective aliased first
            // fields and compare with independent lexicographic ordering.
            const auto effective_year = unsigned(a->memory.read<2>(first + 2));
            const auto effective_day = int(cm::calendar::narrow_day(int(a->memory.read<2>(first))));
            const auto expected = std::pair(sy, int(cm::calendar::narrow_day(sd))) > std::pair(effective_year, effective_day);
            require(a->r<4>(12) == unsigned(expected), "Ordering tuple oracle mismatch" + context);
            if (!alias) {
                require(a->memory.copy_out(first, 5) == before, "Ordering modified separate date" + context);
                require(cm::calendar::is_after({cm::calendar::narrow_day(sd), std::uint16_t(sy)},
                                              {cm::calendar::narrow_day(d), std::uint16_t(y)}) == expected,
                        "Ordering pure result mismatch" + context);
                if (y >= 1 && y <= 2100 && sy >= 1 && sy <= 2100 &&
                    d < cm::calendar::year_length(y) && sd < cm::calendar::year_length(sy)) {
                    using namespace std::chrono;
                    const auto independent = sys_days(year(int(sy))/January/1) + days(sd) >
                                             sys_days(year(int(y))/January/1) + days(d);
                    require(expected == independent, "Ordering Gregorian oracle mismatch" + context); ++oracle;
                }
            }
            ++count;
        };
        for (unsigned y : {0u,1u,4u,1900u,1999u,2000u,2001u,2004u,2100u,32767u,65535u})
            for (int delta : {-1,0,1}) {
                const auto sy = int(y) + delta; if (sy < 0 || sy > 65535) continue;
                for (const auto& [d, sd] : std::array<std::pair<unsigned,unsigned>,14>{{
                    {0,0},{0,1},{1,0},{58,59},{59,60},{60,59},{364,0},{365,366},
                    {32767,32768},{32768,32767},{65535,0},{0,65535},{32768,32768},{65535,65535}}})
                    for (unsigned alias = 0; alias <= 12; ++alias) check(y,d,unsigned(sy),sd,alias);
            }
        for (const auto& [y,sy] : std::array<std::pair<unsigned,unsigned>,4>{{{0,65535},{65535,0},{1900,2100},{2100,1900}}})
            check(y,59,sy,60,0);
        // Every signed day value against zero and each signed extreme in the
        // pure predicate; ABI checks above cover ordered spill/alias effects.
        for (unsigned bits = 0; bits <= 65535; ++bits)
            for (int reference : {-32768,0,32767}) {
                const auto day = cm::calendar::narrow_day(bits);
                require(cm::calendar::is_after({day,2000},{std::int16_t(reference),2000}) == (int(day) > reference),
                        "Ordering signed day domain mismatch");
            }
        initialize(*b,2000,59,2000,60,0,1);
        b->memory.write<4>(fixture,entry); b->memory.write<4>(fixture+4,b->r<4>(8));
        const auto registers = b->registers; const auto unique = b->unique;
        const auto result = call_original(*b,fixture,std::array<std::uint32_t,3>{fixture+16,(60u<<16)|2000,0xff123456});
        require(result == 1 && b->registers == registers && b->unique == unique && b->date_ordering_calls == 1,
                "Ordering callback argument/restoration mismatch");
        for (const auto pc : {0x10331794u,0x10332c24u,0x10717f50u,0x1000a494u}) {
            initialize(*b,2000,59,2000,60,0,1); b->pc = pc; b->wr<4>(4132,entry);
            require(dispatch_page(*b), "Ordering caller did not dispatch");
            if (b->pc == entry) require(dispatch_page(*b), "Ordering inter-page entry unavailable");
            const auto return_pc = pc == 0x1000a494u ? return_sentinel : pc+4;
            require(b->pc == return_pc && b->r<4>(12) == 1 && b->date_ordering_calls == 1,
                    "Ordering caller missed semantic entry");
        }
        std::cout << count << " direct original/modern ordering cases: full registers, temporaries, PC, exact spills, entire mapped memory, allocator and no imports/RNG; "
                  << oracle << " Gregorian oracle cases; 196608 pure signed-day checks; real same/inter-page calls, indirect dispatch and callback\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
