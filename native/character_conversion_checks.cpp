#include "character_conversion_adapter.hpp"
#include "semantic_io_checks.hpp"
using namespace cm::semantic_io_checks;
namespace {
std::unique_ptr<State> setup(const char* program, unsigned entry, std::uint32_t value = 'A') {
    auto s = std::make_unique<State>(); load_program(*s, program); s->memory.map(fixture, 0x2000);
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7)); s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000u + n);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4128, return_sentinel); s->wr<4>(4132, 0x31415926); s->wr<4>(2440, 0xbadc0ffe);
    s->wr<1>(2304, 0xa6); s->wr<1>(1024, 1); s->wr<1>(1027, 1);
    s->memory.copy_in(destination, std::vector<std::uint8_t>(128, 0xdd));
    s->memory.write<1>(left, value); s->memory.write<1>(left + 1, 0); s->memory.write<1>(left + 2, 0);
    if (entry == 0) { s->wr<4>(12, left); s->wr<4>(16, 3); }
    else if (entry < 3) { s->wr<4>(12, destination); s->wr<4>(16, left); s->wr<4>(20, 3); }
    else { s->wr<4>(12, destination); s->wr<4>(16, value); s->wr<4>(20, 12); }
    if (entry == 5) { s->wr<4>(16, left); s->memory.write<2>(left, value); s->memory.write<2>(left + 2, 0); }
    s->pc = character_conversion_entries[entry]; s->budget = 20000; return s;
}
unsigned cases = 0, faults = 0;
std::array<std::uint64_t, 6> witnesses{};
using Mutation = std::function<void(State&, Effects&, std::uint32_t, unsigned)>;
Effects compare_pair(State& original, State& modern, const std::string& label, bool fault = false, const Mutation& mutation = {}, bool mixed = false) {
    if (!mixed) original.character_conversion_original = true;
    Effects expected, actual; expected.state = &original; actual.state = &modern; expected.on_read = actual.on_read = mutation;
    original.memory.date_observer = &expected; modern.memory.date_observer = &actual;
    std::string a, b;
    try { run(original); } catch (const std::exception& e) { a = e.what(); }
    try { run(modern); } catch (const std::exception& e) { b = e.what(); }
    original.memory.date_observer = modern.memory.date_observer = nullptr;
    require(a == b, label + " fault mismatch original=" + a + " semantic=" + b);
    require(fault == !a.empty(), label + " fault expectation: " + a);
    Image(original).compare(modern, label);
    require(expected.bytes == actual.bytes, label + " ordered memory effects");
    require(expected.callbacks == actual.callbacks, label + " callbacks");
    require(!original.string_original && !modern.string_original && !original.memory_primitives_original && !modern.memory_primitives_original, "Semantic dependencies");
    ++cases; if (fault) ++faults;
    if (!fault && modern.character_conversion_original_calls == std::array<std::uint64_t, 6>{})
        for (unsigned i = 0; i < 6; ++i) witnesses[i] += modern.character_conversion_calls[i];
    return actual;
}
using Expectation = std::function<void(const State&)>;
void check(std::unique_ptr<State> a, const std::string& label, const Expectation& expected = {}, bool fault = false, const Mutation& mutation = {}) {
    auto b = clone(*a); compare_pair(*a, *b, label, fault, mutation); if (expected) expected(*b);
}
struct Boundaries final : CharacterConversionBoundaryObserver {
    struct Boundary { std::uint32_t entry; Image image; Boundary(std::uint32_t e, const State& s) : entry(e), image(s) {} };
    std::vector<Boundary> expected; unsigned cursor = 0; std::uint32_t perturb = 0; bool mutated = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (original) { expected.emplace_back(entry, s); return; }
        require(cursor < expected.size() && expected[cursor].entry == entry, "Boundary order");
        if (entry == perturb) { s.wr<4>(12, s.r<4>(12) ^ 1); mutated = true; }
        expected[cursor++].image.compare(s, "Own character boundary " + hex_address(entry));
    }
};
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX) {
    auto probe = clone(initial); Effects effects; effects.state = probe.get(); probe->character_conversion_original = true;
    probe->memory.date_observer = &effects; run(*probe); probe->memory.date_observer = nullptr;
    auto result = std::make_unique<State>(); result->registers = initial.registers; result->unique = initial.unique; result->pc = initial.pc; result->budget = initial.budget;
    for (const auto& [address, size] : effects.accesses) {
        if (address <= omitted && omitted < std::uint64_t(address) + size) {
            if (omitted > address) result->memory.map(address, omitted - address);
            if (omitted + 1 < std::uint64_t(address) + size) result->memory.map(std::uint32_t(omitted + 1), std::uint64_t(address) + size - omitted - 1);
        } else result->memory.map(address, size);
    }
    for (const auto& [address, size] : result->memory.mapped_ranges()) result->memory.copy_in(address, initial.memory.copy_out(address, size));
    return result;
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-character-conversion-checks <original-segments>\n"; return 2; }
    try {
        const std::vector<std::vector<std::uint8_t>> sequences{{0},{0x7f},{0x80},{0xbf},{0xc0,0x80},{0xc1,0xbf},{0xc2,0x80},{0xdf,0xbf},{0xe0,0xa0,0x80},{0xef,0xbf,0xbf},{0xed,0xa0,0x80},{0xe0,0x80,0x80},{0xc2,0xff},{0xe1,0xff,0xff},{0xe1,0x7f,0x80},{0xe1,0x80,0x7f},{0xf0,0x80,0x80},{0xff}};
        for (unsigned entry = 0; entry < 3; ++entry) for (const auto& bytes : sequences) for (unsigned limit : {0u,1u,2u,3u}) {
            auto a = setup(argv[1], entry); a->memory.copy_in(left, bytes); a->wr<4>(entry ? 20 : 16, limit);
            check(std::move(a), "sequence entry=" + std::to_string(entry) + " prefix=" + std::to_string(bytes[0]) + " limit=" + std::to_string(limit));
        }
        for (unsigned entry : {3u,4u,5u}) for (auto value : {0u,0x7fu,0x80u,0x7ffu,0x800u,0xffffu,0x10041u}) {
            auto a = setup(argv[1], entry, value);
            check(std::move(a), "encode boundary", [=](const State& s) {
                const unsigned length = (value & 0xffff) < 0x80 ? 1 : (value & 0xffff) < 0x800 ? 2 : 3;
                require(s.r<4>(12) == (entry == 5 && !value ? 0 : length), "Independent encoded length");
                if (value == 0x800) require(s.memory.copy_out(destination,3) == std::vector<std::uint8_t>{0xe0,0xa0,0x80}, "Independent 0x800 encoding");
                if (value == 0x7f) require(s.memory.read<1>(destination) == 0x7f, "Independent ASCII");
            });
        }
        for (unsigned entry = 0; entry < 6; ++entry) {
            for (bool null_input : {false,true}) {
                auto a = setup(argv[1], entry); a->wr<4>(null_input && entry < 3 ? (entry ? 16 : 12) : 12, 0);
                check(std::move(a), "null contract", {}, entry == 0);
            }
            auto a = setup(argv[1], entry); a->wr<4>(12, entry == 0 ? stack - 8 : stack - 8);
            if (entry == 0) a->memory.write<1>(stack - 8, 'X');
            check(std::move(a), "red zone overlap");
            auto initial = setup(argv[1], entry); check(sparse(*initial), "minimal mapping");
            auto probe = clone(*initial); Effects effects; effects.state = probe.get(); probe->character_conversion_original = true; probe->memory.date_observer = &effects; run(*probe); probe->memory.date_observer = nullptr;
            std::vector<std::uint32_t> omitted;
            for (const auto& e : effects.log) { const auto byte = e.address + std::uint32_t(e.size) - 1; if (std::find(omitted.begin(),omitted.end(),byte) == omitted.end()) omitted.push_back(byte); }
            for (auto byte : omitted) check(sparse(*initial,byte), "fault " + hex_address(byte), {}, true);
            Boundaries boundaries; auto original = setup(argv[1],entry); original->character_conversion_original = true; original->character_conversion_observer = &boundaries; run(*original);
            auto modern = setup(argv[1],entry); modern->character_conversion_observer = &boundaries; run(*modern); require(boundaries.cursor == boundaries.expected.size(), "All boundaries");
            auto negative = setup(argv[1],entry); boundaries.cursor = 0; boundaries.perturb = character_conversion_entries[entry]; negative->character_conversion_observer = &boundaries;
            bool rejected = false; try { run(*negative); } catch (const std::exception& e) { rejected = std::string(e.what()).starts_with("Own character boundary " + hex_address(boundaries.perturb)); }
            require(rejected && boundaries.mutated, "Own negative");
            negative->character_conversion_original = true; negative->character_conversion_entry_original.fill(true); negative->character_conversion_original_resume = character_conversion_entries[entry]; negative->string_calls[0] = 77; reset_character_conversion_diagnostics(*negative);
            require(!negative->character_conversion_original && negative->character_conversion_calls == std::array<std::uint64_t,6>{} && negative->character_conversion_original_calls == std::array<std::uint64_t,6>{} && negative->character_conversion_entry_original == std::array<bool,6>{} && !negative->character_conversion_original_resume && !negative->character_conversion_observer && negative->string_calls[0] == 77, "Independent diagnostic reset");
            auto indirect = setup(argv[1],entry); indirect->pc = 0x10780900; indirect->memory.write<4>(fixture+0x710,character_conversion_entries[entry]); indirect->memory.write<4>(fixture+0x714,toc); indirect->memory.write<4>(toc-0x7ee4,fixture+0x710); check(std::move(indirect),"Generated indirect transition");
        }
        for (unsigned capacity : {0u,1u,2u,3u,4u,5u,7u}) {
            auto a = setup(argv[1],5); a->memory.write<2>(left,0x80); a->memory.write<2>(left+2,0x800); a->memory.write<2>(left+4,0); a->wr<4>(20,capacity);
            check(std::move(a),"String small capacity",[=](const State& s){ require(s.r<4>(12) == (capacity < 2 ? 0 : capacity < 5 ? 2 : 5),"Independent partial string count"); });
        }
        for (unsigned entry : {3u,4u,5u}) {
            auto a = setup(argv[1],entry,0x800); check(std::move(a),"Live prefix",{},false,[](State& s,Effects& e,std::uint32_t address,unsigned){ if (!e.mutated && address==toc-0x34f8) { s.memory.write<2>(toc-0x34f6,0xb1f2); e.mutated=true; } });
            auto relocated = setup(argv[1],entry,0x80); relocated->memory.map(fixture+0x4000,0x10000); relocated->wr<4>(8,fixture+0xc000); relocated->memory.write<2>(fixture+0xc000-0x34f8,0x0022); relocated->memory.write<2>(fixture+0xc000-0x34f6,0xc3e4); check(std::move(relocated),"Relocated prefix TOC");
        }
        {
            Boundaries b; auto a=setup(argv[1],5);a->character_conversion_original=true;a->character_conversion_observer=&b;run(*a);
            auto n=setup(argv[1],5);n->character_conversion_observer=&b;b.perturb=character_conversion_entries[3];bool rejected=false;
            try{run(*n);}catch(const std::exception& e){rejected=std::string(e.what()).starts_with("Own character boundary "+hex_address(b.perturb));}
            require(rejected&&b.cursor<b.expected.size(),"Child rejected before parent");
        }
        for(const auto& choice:std::vector<std::pair<unsigned,unsigned>>{{2,1},{1,0},{5,4},{4,3}})for(bool inverse:{false,true}){
            auto a=setup(argv[1],choice.first);auto b=clone(*a);b->character_conversion_entry_original[choice.first]=inverse;b->character_conversion_entry_original[choice.second]=!inverse;compare_pair(*a,*b,"Mixed chain",false,{},true);
        }
        for(unsigned i=0;i<6;++i){require(witnesses[i]!=0,"Nonempty normal witness");std::cout<<"Normal witness "<<hex_address(character_conversion_entries[i])<<" calls="<<witnesses[i]<<" original=0 escapes=0\n";}
        std::cout<<"PASS: character full architecture/memory/ordered effects cases="<<cases<<" faults="<<faults<<" own negatives=6 child negative=1 indirect transitions=6 mixed modes=8\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
