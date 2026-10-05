#include "string_core_adapter.hpp"
#include "collection_lookup_adapter.hpp"
#include "background_selection_adapter.hpp"
#include "callbacks.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture=0x30000000, stack=0x700fff00, toc=0x108bba90;
constexpr std::uint32_t left=fixture, right=fixture+0x100, destination=fixture+0x200;
constexpr std::uint32_t table=fixture+0x400, mapper=fixture+0x500, unmapped=0xffff0000;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> bytes;
    struct Access { unsigned kind; std::uint32_t address; std::uint64_t size, value; };
    std::vector<Access> log;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> accesses;
    State* state = nullptr;
    std::function<void(State&, Effects&, std::uint32_t, unsigned)> after_read;
    std::vector<std::uint32_t> allocator_requests;
    unsigned callback_reads = 0;
    bool mutated = false;
    void number(std::uint64_t value, unsigned width) {
        for (unsigned n = width; n; --n) bytes.push_back(std::uint8_t(value >> ((n - 1) * 8)));
    }
    void event(unsigned kind, std::uint32_t address, std::uint64_t size, const std::uint8_t* data = nullptr) {
        number(kind, 1); number(address, 4); number(size, 8); accesses.emplace_back(address, size);
        std::uint64_t value = 0;
        if (data) {
            bytes.insert(bytes.end(), data, data + size);
            for (unsigned i = 0; i < std::min(size, std::uint64_t(8)); ++i) value = (value << 8) | data[i];
        }
        log.push_back({kind,address,size,value});
    }
    void mapped(std::uint32_t address, std::uint64_t size) override { event(1, address, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override { event(2, address, size, data); }
    void cleared(std::uint32_t address, std::uint64_t size) override { event(3, address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override {
        event(4, address, size, data);
        if (state && state->pc==0x10774b18) allocator_requests.push_back(std::uint32_t(state->r<4>(124)));
        if (after_read) after_read(*state, *this, address, size);
    }
};
// Instruction accounting and process-local hook diagnostics are intentionally
// excluded. Every architectural byte, including generated temporary residues,
// and every mapped memory byte is compared at returns and faults.
struct Image {
    decltype(State::registers) registers;
    decltype(State::unique) unique;
    std::uint32_t pc, heap_end;
    std::uint64_t native_calls;
    std::int16_t memory_error;
    decltype(State::allocated) allocated, available;
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> memory;
    explicit Image(const State& s) : registers(s.registers), unique(s.unique), pc(s.pc), heap_end(s.heap_end),
        native_calls(s.native_calls), memory_error(s.memory_error), allocated(s.allocated), available(s.available) {
        for (const auto& [address, size] : s.memory.mapped_ranges()) memory.emplace_back(address, s.memory.copy_out(address, size));
    }
    void compare(const State& s, const std::string& label) const {
        if (registers != s.registers) {
            const auto p = std::mismatch(registers.begin(), registers.end(), s.registers.begin());
            throw std::runtime_error(label + " register byte=" + std::to_string(p.first - registers.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        if (unique != s.unique) {
            const auto p = std::mismatch(unique.begin(), unique.end(), s.unique.begin());
            throw std::runtime_error(label + " temporary byte=" + std::to_string(p.first - unique.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        require(pc == s.pc, label + " PC expected=" + hex_address(pc) + " actual=" + hex_address(s.pc));
        require(heap_end == s.heap_end && native_calls == s.native_calls && memory_error == s.memory_error &&
                allocated == s.allocated && available == s.available, label + " allocator/import mismatch");
        const auto ranges = s.memory.mapped_ranges();
        require(ranges.size() == memory.size(), label + " mapping count mismatch");
        for (unsigned i = 0; i < memory.size(); ++i) {
            const auto& [address, bytes] = memory[i];
            require(ranges[i].first == address && ranges[i].second == bytes.size(), label + " mapping mismatch");
            const auto actual = s.memory.copy_out(address, bytes.size());
            if (bytes != actual) {
                const auto p = std::mismatch(bytes.begin(), bytes.end(), actual.begin());
                throw std::runtime_error(label + " memory mismatch at " + hex_address(address + std::uint32_t(p.first - bytes.begin())));
            }
        }
    }
};
std::unique_ptr<State> clone(const State& s) {
    auto t = std::make_unique<State>();
    t->registers = s.registers; t->unique = s.unique; t->pc = s.pc;
    t->executed = s.executed; t->budget = s.budget; t->native_calls = s.native_calls;
    t->allocated = s.allocated; t->available = s.available; t->heap_end = s.heap_end; t->memory_error = s.memory_error;
    for (const auto& [address, size] : s.memory.mapped_ranges()) {
        t->memory.map(address, size); t->memory.copy_in(address, s.memory.copy_out(address, size));
    }
    return t;
}
std::unique_ptr<State> setup(const char* program, unsigned entry, unsigned variation=0) {
    auto s=std::make_unique<State>(); load_program(*s,program);
    s->memory.map(fixture,0x2000);
    s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000u+n);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);
    s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);
    s->wr<1>(2304,variation%2?0xf9:0xa6);s->wr<1>(1024,variation%4);s->wr<1>(1027,(variation/2)%2);
    s->memory.copy_in(left,{'a','b','a',0});s->memory.copy_in(right,{'a','b','a',0});
    s->memory.copy_in(destination,std::vector<std::uint8_t>(64,0xa5));
    s->wr<4>(12,left);s->wr<4>(16,right);s->wr<4>(20,2);
    if(entry==6||entry==7||entry==8){s->wr<4>(12,destination);s->wr<4>(16,left);}
    if(entry==8)s->memory.copy_in(destination,{'x',0});
    if(entry==11||entry==12)s->wr<4>(16,'a');
    s->pc=string_core_entries[entry];s->budget=200000;s->executed=s->native_calls=0;
    return s;
}
unsigned cases=0,faults=0;
std::array<std::uint64_t,14> witnesses{};
struct Comparison { Effects effects; };
using Mutation=std::function<void(State&,Effects&,std::uint32_t,unsigned)>;
Comparison compare_pair(State& original,State& modern,const std::string& label,bool expect_fault=false,
    const Mutation& mutation={},const std::function<void(State&)>& execute=[](State& s){run(s);}) {
    require(!original.collection_original&&!modern.collection_original&&!original.background_original&&
        !modern.background_original&&!original.calendar_original&&!modern.calendar_original&&
        !original.rng_original&&!modern.rng_original,"Unrelated modules must remain semantic");
    original.string_original=true;
    Effects expected,actual;expected.state=&original;actual.state=&modern;
    expected.after_read=actual.after_read=mutation;
    original.memory.date_observer=&expected;modern.memory.date_observer=&actual;
    std::string a,b;
    try{execute(original);}catch(const std::exception& e){a=e.what();}
    try{execute(modern);}catch(const std::exception& e){b=e.what();}
    original.memory.date_observer=modern.memory.date_observer=nullptr;
    require(a==b,label+" fault mismatch original="+a+" semantic="+b);
    if(!expect_fault&&!a.empty())throw std::runtime_error(label+" original: "+a+" pc="+hex_address(original.pc));
    require(expect_fault==!a.empty(),label+" fault expectation mismatch");
    if(expect_fault)require(a.starts_with("Unmapped data access at "),label+" wrong fault type");
    Image(original).compare(modern,label);
    require(expected.bytes==actual.bytes,label+" ordered access/effect mismatch");
    require(original.rng_calls==modern.rng_calls&&original.background_calls==modern.background_calls&&
        original.collection_calls==modern.collection_calls,label+" unrelated dependency dispatch mismatch");
    ++cases;if(expect_fault)++faults;
    if(modern.string_original_calls==std::array<std::uint64_t,14>{})
        for(unsigned i=0;i<14;++i)witnesses[i]+=modern.string_calls[i];
    return {std::move(actual)};
}
using Expectation=std::function<void(const State&,const Effects&)>;
void check(std::unique_ptr<State> a,const std::string& label,const Expectation& expect={},bool fault=false,const Mutation& mutation={}) {
    auto b=clone(*a);auto result=compare_pair(*a,*b,label,fault,mutation);
    if(expect)expect(*b,result.effects);
}
std::vector<std::uint32_t> addresses(const Effects& e,unsigned kind,std::uint32_t begin,std::uint32_t end) {
    std::vector<std::uint32_t> result;
    for(const auto& x:e.log)if(x.kind==kind&&begin<=x.address&&x.address<end)result.push_back(x.address);
    return result;
}
std::uint32_t mac_result(const State& s,const std::vector<std::uint8_t>& a,const std::vector<std::uint8_t>& b,int limit=32767) {
    const auto mapping=std::uint32_t(s.memory.read<4>(toc-0x62bc));
    auto mapped=[&](std::uint8_t v){return v==255?-1:int(signed_value<1>(s.memory.read<1>(mapping+v)));};
    for(int i=0;i<limit;++i) {
        if(!a[i]||!b[i])return !a[i]?(!b[i]?0:UINT32_MAX):1;
        if(mapped(a[i])!=mapped(b[i]))return mapped(a[i])<mapped(b[i])?UINT32_MAX:1;
    }
    return 0;
}
std::unique_ptr<State> accessed_mapping(const State& initial) {
    auto probe=clone(initial);Effects effects;probe->string_original=true;effects.state=probe.get();
    probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
    auto result=std::make_unique<State>();result->registers=initial.registers;result->unique=initial.unique;
    result->pc=initial.pc;result->budget=initial.budget;
    for(const auto& [address,size]:effects.accesses)result->memory.map(address,size);
    for(const auto& [address,size]:result->memory.mapped_ranges())result->memory.copy_in(address,initial.memory.copy_out(address,size));
    return result;
}
struct Boundaries final:StringBoundaryObserver {
    struct Boundary { std::uint32_t entry;Image image;Boundary(std::uint32_t e,const State& s):entry(e),image(s){} };
    std::vector<Boundary> expected;
    unsigned cursor=0;std::uint32_t perturb=0;bool mutated=false;
    void enter(const State&,std::uint32_t,bool) override {}
    void leave(State& s,std::uint32_t entry,bool original) override {
        if(original){expected.emplace_back(entry,s);return;}
        require(cursor<expected.size()&&expected[cursor].entry==entry,"Missing nested string boundary");
        if(entry==perturb&&!mutated){s.wr<4>(12,s.r<4>(12)^1);mutated=true;}
        expected[cursor++].image.compare(s,"Own string boundary "+hex_address(entry));
    }
};
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    try {
        const std::vector<std::vector<std::uint8_t>> texts{
            {0},{'a',0},{'a','b',0},{'a','b','c',0},{'A','B','c',0},
            {'a','b','d',0},{255,0},{128,0},{129,0},{'x',255,0}};
        // Selected pairs cover equality, prefixes and both comparison signs.
        for(unsigned entry:{0u,1u,2u,9u,10u})for(unsigned i=0;i<texts.size();++i)
            for(unsigned j:{i,(i+1)%unsigned(texts.size())}) {
                auto a=setup(argv[1],entry,i);a->memory.copy_in(left,texts[i]);a->memory.copy_in(right,texts[j]);
                const unsigned count=entry==1||entry==10?2:32767;a->wr<4>(20,count);
                std::uint32_t expected=0;
                if(entry<3)expected=mac_result(*a,texts[i],texts[j],int(count));
                else for(unsigned n=0;n<count;++n){
                    if(texts[i][n]!=texts[j][n]){expected=std::uint32_t(int(texts[i][n])-int(texts[j][n]));break;}
                    if(!texts[i][n])break;
                }
                check(std::move(a),"comparison pair "+std::to_string(entry)+"/"+std::to_string(i)+"/"+std::to_string(j),
                    [=](const State& s,const Effects&){require(s.r<4>(12)==expected,"Independent comparison result");});
            }
        for(unsigned entry:{0u,1u,2u})for(unsigned nulls:{1u,2u,3u}) {
            auto a=setup(argv[1],entry);if(nulls&1)a->wr<4>(12,0);if(nulls&2)a->wr<4>(16,0);
            check(std::move(a),"null Mac comparison",[](const State& s,const Effects& e){
                require(s.r<4>(12)==0,"Null comparison result");
                require(addresses(e,4,toc-0x62bc,toc-0x62b8).size()==1,"Null comparison must load table slot");
                require(addresses(e,4,fixture,fixture+0x1000).empty(),"Null comparison read strings");});
        }
        for(unsigned entry:{0u,1u,2u,4u}) {
            auto a=setup(argv[1],entry);a->wr<4>(8,unmapped);if(entry<3)a->wr<4>(12,0);
            check(std::move(a),"mandatory TOC slot fault",{},true);
        }
        for(unsigned entry:{0u,1u})for(std::uint32_t count:{0u,1u,2u,3u,0xffffu,0x8000u,0x7fffu,0x10001u,0xffff0002u}) {
            if(entry==0&&count!=0)continue;
            auto a=setup(argv[1],entry);a->memory.copy_in(left,{'a','b','x',0});a->memory.copy_in(right,{'a','b','z',0});
            a->wr<4>(20,count);const int limit=entry==0?32767:int(signed_value<2>(count&65535));
            const auto expected=mac_result(*a,{'a','b','x',0},{'a','b','z',0},limit);
            check(std::move(a),"signed16 Mac limit "+std::to_string(count),[=](const State& s,const Effects& e){
                require(s.r<4>(12)==expected,"Signed16 limit result");
                require(!addresses(e,4,left,left+4).empty(),"Nonpositive Mac limit avoided first read");
                if(entry==1&&limit>=0&&limit<=2)require(addresses(e,4,left,left+4).back()==left+limit,"Mac limit lookahead missing");});
        }
        for(unsigned entry:{0u,1u,2u}) {
            auto a=setup(argv[1],entry);a->memory.copy_in(table,std::vector<std::uint8_t>(256,0));
            a->memory.write<1>(table+'a',128);a->memory.write<1>(table+'b',127);a->memory.write<1>(table+'x',0);
            a->memory.write<4>(toc-0x62bc,table);a->memory.copy_in(left,{'x','a',0});a->memory.copy_in(right,{'x','b',0});
            check(std::move(a),"signed mutable table and mapped zero",[](const State& s,const Effects& e){
                require(s.r<4>(12)==UINT32_MAX,"Mapped zero terminated early or signed mapping lost");
                require(addresses(e,4,table,table+256)==std::vector<std::uint32_t>{table+'x',table+'x',table+'a',table+'b'},"Live table lookup order");});
            a=setup(argv[1],entry);a->memory.copy_in(left,{255,0});a->memory.copy_in(right,{255,0});a->memory.write<4>(toc-0x62bc,unmapped);
            check(std::move(a),"ff bypasses table",[](const State& s,const Effects&){require(!s.r<4>(12),"ff equality");});
        }
        for(unsigned entry:{0u,1u,2u,4u,5u,9u,10u,11u,12u,13u}) {
            auto a=setup(argv[1],entry);a->wr<4>(12,unmapped);check(std::move(a),"invalid first string "+std::to_string(entry),{},true);
        }
        for(unsigned entry:{0u,1u,9u,10u,13u}) {
            auto a=setup(argv[1],entry);a->wr<4>(16,unmapped);check(std::move(a),"invalid second string",{},true);
        }
        for(auto count:{0u,1u,2u,3u,4u,UINT32_MAX,0x80000000u}) {
            auto a=setup(argv[1],10);a->wr<4>(20,count);if(!count){a->wr<4>(12,unmapped);a->wr<4>(16,unmapped);}
            a->memory.copy_in(left,{255,0});a->memory.copy_in(right,{0,0});
            check(std::move(a),"word32 byte limit",[=](const State& s,const Effects& e){
                require(s.r<4>(12)==(count?255:0),"Unsigned difference or word32 limit");
                if(!count)require(addresses(e,4,left,right+4).empty(),"Zero byte limit read pointers");});
        }
        for(unsigned entry:{5u,6u,7u,8u})for(const auto& text:texts) {
            auto a=setup(argv[1],entry,unsigned(text.size()));a->memory.copy_in(left,text);
            if(entry==7)a->wr<4>(20,unsigned(text.size())+2);
            check(std::move(a),"length/copy/pad/concatenate "+std::to_string(entry),[=](const State& s,const Effects& e){
                if(entry==5){require(s.r<4>(12)==text.size()-1&&s.r<4>(16)==left+text.size()-1,"Length/cursor");return;}
                auto expected=text;if(entry==7)expected.insert(expected.end(),2,0);
                const auto out=destination+(entry==8?1:0);
                require(s.r<4>(12)==destination&&s.memory.copy_out(out,expected.size())==expected,"Independent copy bytes/return");
                if(entry==7)require(addresses(e,4,left,left+0x100).size()==text.size(),"Padding continued reading source");
                if(entry==8)require(e.log[0].address==destination,"Concat read source before destination scan");
            });
        }
        for(unsigned entry:{6u,7u,8u})for(int offset:{-1,0,1}) {
            auto a=setup(argv[1],entry);a->memory.map(left-1,1);a->wr<4>(12,left+offset);a->wr<4>(16,left);a->wr<4>(20,3);
            // Bounded buffers guarantee quick original faults for forward
            // overlaps that overwrite the terminator, without a huge scan.
            if((entry==6&&offset==1)||entry==8) {
                a->wr<4>(12,unmapped-8+offset);a->wr<4>(16,unmapped-8);
                a->memory.map(unmapped-9,9);a->memory.copy_in(unmapped-9,{'x','a','b','a',0,0,0,0,0});
                check(std::move(a),"finite destructive overlap",{},true);
            } else check(std::move(a),"progressive overlap "+std::to_string(entry)+"/"+std::to_string(offset));
        }
        for(auto count:{0u,1u,3u,4u,6u}) {
            auto a=setup(argv[1],7);a->wr<4>(20,count);if(!count){a->wr<4>(12,unmapped);a->wr<4>(16,unmapped);}
            check(std::move(a),"bounded copy count",[=](const State& s,const Effects& e){
                require(s.r<4>(20)==0&&s.r<1>(1027)==1,"Bounded copy count/carry");
                if(!count)require(e.log.empty(),"Zero copy accessed bytes");
                if(count==3)require(s.memory.copy_out(destination,4)==std::vector<std::uint8_t>{'a','b','a',0xa5},"Truncated copy appended NUL");
            });
        }
        for(unsigned entry:{5u,6u,7u,8u,9u,10u,11u,12u,13u,4u}) {
            auto a=setup(argv[1],entry);a->memory.map(unmapped-2,2);a->memory.copy_in(unmapped-2,{'a','a'});
            if(entry==6||entry==7){a->wr<4>(16,unmapped-2);if(entry==7)a->wr<4>(20,UINT32_MAX);}
            else a->wr<4>(12,unmapped-2);
            if(entry==9||entry==10)a->memory.copy_in(right,{'a','a',0});
            if(entry==10)a->wr<4>(20,UINT32_MAX);
            if(entry==11||entry==12)a->wr<4>(16,'z');
            if(entry==13)a->memory.copy_in(right,{'a','a','b',0});
            check(std::move(a),"finite unterminated/count wrap fault "+std::to_string(entry),{},true);
        }
        for(unsigned entry:{6u,7u,8u}) {
            auto a=setup(argv[1],entry);a->wr<4>(12,unmapped);check(std::move(a),"destination fault",{},true);
        }
        for(unsigned entry:{11u,12u})for(std::uint32_t value:{0u,unsigned('a'),unsigned('b'),unsigned('z'),255u,0x123461u}) {
            auto a=setup(argv[1],entry);a->wr<4>(16,value);
            const auto wanted=value&255;const auto expected=wanted==0?left+3:wanted=='a'?left+(entry==12?2:0):wanted=='b'?left+1:0;
            check(std::move(a),"byte search "+std::to_string(value),[=](const State& s,const Effects&){require(s.r<4>(12)==expected,"Independent first/last byte pointer");});
        }
        for(unsigned entry:{11u,12u}) {
            auto a=setup(argv[1],entry);a->memory.copy_in(left,{'a',255,'b',255,0});a->wr<4>(16,0x1234ff);
            check(std::move(a),"high byte occurrence",[=](const State& s,const Effects&){require(s.r<4>(12)==left+(entry==11?1:3),"High-byte search pointer");});
        }
        for(unsigned entry:{3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,13u}) {
            auto a=setup(argv[1],entry);if(entry==6||entry==7)a->wr<4>(16,0);else a->wr<4>(12,0);
            check(std::move(a),"null pointer without guard "+std::to_string(entry),{},true);
        }
        {
            auto a=setup(argv[1],13);a->memory.map(unmapped-2,2);a->memory.copy_in(unmapped-2,{'a','a'});
            a->wr<4>(16,unmapped-2);a->memory.copy_in(left,{'a','a','b',0});
            check(std::move(a),"inner needle fault after haystack read",[](const State& s,const Effects& e){
                require(s.pc==0x1077ddd4&&e.log.back().address==left+2,"Substring fault read order");
            },true);
        }
        for(const auto& needle:std::vector<std::vector<std::uint8_t>>{{0},{'a',0},{'b','a',0},{'a','b','a',0},{'z',0},{'a','b','a','a',0}}) {
            auto a=setup(argv[1],13);a->memory.copy_in(right,needle);
            const auto expected=needle[0]==0||needle==std::vector<std::uint8_t>{'a',0}||needle==std::vector<std::uint8_t>{'a','b','a',0}?left:
                needle==std::vector<std::uint8_t>{'b','a',0}?left+1:0;
            check(std::move(a),"substring",[=](const State& s,const Effects&){require(s.r<4>(12)==expected,"Independent substring pointer");});
        }
        for(bool null_needle:{false,true}) {
            auto a=setup(argv[1],13);a->wr<4>(12,unmapped);if(null_needle)a->wr<4>(16,0);else a->memory.write<1>(right,0);
            check(std::move(a),"exceptional needle ignores haystack",[](const State& s,const Effects& e){
                require(s.r<4>(12)==unmapped&&s.r<4>(20)==unmapped-1,"Exceptional needle return/residue");
                require(addresses(e,4,unmapped,UINT32_MAX).empty(),"Exceptional needle read haystack");});
        }
        for(bool mutable_table:{false,true}) {
            auto a=setup(argv[1],4);a->memory.copy_in(left,{'a',255,'b',0});
            const auto original=std::uint32_t(a->memory.read<4>(toc-0x618c));
            a->memory.copy_in(mapper,a->memory.copy_out(original,256));a->memory.write<4>(toc-0x618c,mapper);
            a->memory.write<1>(mapper+'a',0);a->memory.write<1>(mapper+'b',128);
            if(mutable_table)a->memory.write<4>(toc-0x62bc,unmapped);
            check(std::move(a),"mapper independent table/raw termination",[](const State& s,const Effects& e){
                require(s.memory.copy_out(left,4)==std::vector<std::uint8_t>{0,255,128,0}&&s.r<4>(12)==left+3,"Mapper bytes/advanced return");
                require(addresses(e,4,mapper,mapper+256)==std::vector<std::uint32_t>{mapper+'a',mapper+'b'},"Mapper ff/NUL lookup");
                require(addresses(e,2,left,left+4)==std::vector<std::uint32_t>{left,left+1,left+2},"Mapper wrote terminator");});
        }
        // The string aliases its own transformation table. Writing the first
        // result changes the lookup used by the second raw byte.
        {
            auto a=setup(argv[1],4);a->memory.write<4>(toc-0x618c,left);a->wr<4>(12,left+'a');a->memory.copy_in(left+'a',{'b','a',0});
            check(std::move(a),"mapper table alias",[](const State& s,const Effects&){
                require(s.r<4>(12)==left+'a'+2&&s.memory.copy_out(left+'a',3)==std::vector<std::uint8_t>{'a','a',0},"Mapper alias must change future lookup");});
        }
        for(unsigned entry:{0u,1u,4u}) {
            auto a=setup(argv[1],entry);const auto slot=entry==4?0x618cu:0x62bcu;
            a->memory.map(0x31000000,0x10000);a->memory.write<4>(0x31008000-slot,a->memory.read<4>(toc-slot));a->wr<4>(8,0x31008000);
            check(std::move(a),"relocated TOC");
            a=setup(argv[1],entry);a->memory.write<4>(toc-slot,unmapped);check(std::move(a),"invalid table",{},true);
        }
        for(unsigned entry:{0u,1u,2u,9u,10u}) {
            auto a=setup(argv[1],entry);a->wr<4>(16,left);
            check(std::move(a),"identical pointer retains repeated reads",[](const State&,const Effects& e){
                auto reads=addresses(e,4,left,left+4);require(reads.size()>=4&&reads[0]==reads[1],"Same pointer reads deduplicated");});
        }
        for(auto alias:{stack+8,stack-64}) {
            auto a=setup(argv[1],2);a->wr<4>(12,alias);a->memory.copy_in(alias,{'a',0});a->memory.copy_in(right,{'b',0});
            check(std::move(a),"descriptor string aliases wrapper spill");
        }
        for(unsigned entry:{2u,3u})for(bool own_original:{false,true}) {
            auto a=setup(argv[1],entry);auto b=clone(*a);b->string_entry_original.fill(!own_original);b->string_entry_original[entry]=own_original;
            compare_pair(*a,*b,"mixed wrapper/dependencies");
            const auto child=entry==2?0u:5u;
            require((own_original?b->string_original_calls[entry]:b->string_calls[entry])&&
                (own_original?b->string_calls[child]:b->string_original_calls[child]),"Mixed dispatch missing");
            if(entry==3)require((own_original?b->string_calls[6]:b->string_original_calls[6])!=0,"Mixed copy dependency missing");
        }
        for(const auto& text:std::vector<std::vector<std::uint8_t>>{{0},{'a','b','c',0}}) {
            auto a=setup(argv[1],3);a->memory.copy_in(left,text);
            check(std::move(a),"real original allocator duplication",[=](const State& s,const Effects& e){
                const auto result=std::uint32_t(s.r<4>(12));
                require(e.allocator_requests==std::vector<std::uint32_t>{std::uint32_t(text.size())},"Original allocator requested size");
                require(result&&result!=left&&s.memory.copy_out(result,text.size())==text,"Independent allocated copy");
                require(s.string_calls[5]&&s.string_calls[6],"Duplication dependencies not semantic");});
        }
        {
            auto a=setup(argv[1],3);a->wr<4>(12,unmapped);check(std::move(a),"duplicate length fault",{},true);
        }
        // Date helpers stay semantic; only the newly reconstructed strings
        // select the original reference. Small synthetic fallback fixtures.
        const std::array<std::uint32_t,6> dates{0x107184d0,0x10718570,0x10718700,0x107187d0,0x107188a0,0x107189d0};
        const std::array<unsigned,6> slots{0x3fc4,0x3fc8,0x3fcc,0x3fd4,0x3fdc,0x3fe4};
        for(unsigned i=0;i<dates.size();++i) {
            auto a=setup(argv[1],6);a->pc=dates[i];a->wr<4>(12,255);
            a->memory.write<4>(toc-slots[i],destination);a->memory.write<4>(toc-0x3fa0,fixture+0x800);
            a->memory.copy_in(fixture+0xf40,{'?',0});
            check(std::move(a),"Date fallback "+hex_address(dates[i]),[](const State& s,const Effects&){
                require(s.memory.copy_out(destination,2)==std::vector<std::uint8_t>{'?',0}&&s.string_calls[6]==1,"Date fallback copy witness");});
        }
        for(const auto& format:std::vector<std::string>{"%.2f units","%08.2f!"}) {
            auto a=setup(argv[1],0);a->pc=0x1077b680;
            for(unsigned flag=1280;flag<1312;++flag)a->wr<1>(flag,0);
            a->memory.copy_in(right,{format.begin(),format.end()});a->memory.write<1>(right+format.size(),0);
            a->memory.write<8>(fixture+0x1000,float_bits<8>(7.25));
            a->wr<4>(12,destination);a->wr<4>(16,64);a->wr<4>(20,right);a->wr<4>(24,fixture+0x1000);
            check(std::move(a),"normal formatter composition",[=](const State& s,const Effects&){
                const std::string expected=format=="%.2f units"?"7.25 units":"00007.25!";
                require(s.memory.copy_out(destination,expected.size())==std::vector<std::uint8_t>(expected.begin(),expected.end()),"Formatter output");
                require(s.string_calls[5]&&s.string_calls[11],"Formatter dispatch length="+std::to_string(s.string_calls[5])+" byte-search="+std::to_string(s.string_calls[11]));});
        }
        // Two descriptor selections combine the semantic collection query,
        // wrapper and Mac comparison with the original mutable vector and RNG.
        for(bool altered_table:{false,true}) {
            auto a=setup(argv[1],0);constexpr auto catalog=fixture+0x800,root=fixture+0x900;
            constexpr auto list=fixture+0xa00,node=fixture+0xb00,image=fixture+0xc00,pool=fixture+0x1100;
            a->memory.write<1>(toc-0x35d4,1);a->memory.write<4>(toc-0x5c3c,fixture+0x700);
            a->memory.write<4>(fixture+0x704,catalog);a->memory.write<4>(catalog,root);
            a->memory.write<4>(catalog+4,a->memory.read<4>(toc-0x5c38));a->memory.write<4>(catalog+12,1);
            a->memory.write<4>(root,left);a->memory.write<4>(root+4,list);
            a->memory.write<4>(root+8,0);a->memory.write<4>(root+12,0);
            a->memory.write<4>(list,node);a->memory.write<4>(list+4,node+16);a->memory.write<4>(list+8,2);
            a->memory.write<4>(node,image);a->memory.write<4>(node+4,node+16);
            a->memory.write<4>(node+16,image);a->memory.write<4>(node+20,0);
            a->memory.copy_in(image,{'p','i','c',0});
            a->memory.write<4>(toc-0x7b90,pool+32);a->memory.write<4>(toc-0x7b8c,pool);
            a->memory.write<4>(toc-0x7b88,pool+36);a->memory.write<4>(toc-0x7b84,pool+40);
            a->memory.write<4>(pool+32,0);a->memory.write<4>(pool+36,pool+14);a->memory.write<4>(pool+40,pool);
            for(unsigned i=0;i<8;++i)a->memory.write<2>(pool+i*2,i);
            if(altered_table){
                const auto original=std::uint32_t(a->memory.read<4>(toc-0x62bc));
                a->memory.copy_in(table,a->memory.copy_out(original,256));a->memory.write<1>(table+'b',128);
                a->memory.write<4>(toc-0x62bc,table);
            }
            a->pc=0x107139c0;a->wr<4>(12,1);a->wr<4>(16,1);a->wr<4>(20,right);a->wr<4>(24,0);
            check(std::move(a),"background/collection/descriptor/table composition",[](const State& s,const Effects&){
                require(s.r<4>(12)==image&&s.memory.read<4>(pool+40)==pool+2,"Nonempty picture/RNG consumption");
                require(s.background_calls[3]&&s.background_calls[1]&&s.collection_calls[2]&&s.collection_calls[4]&&
                    s.string_calls[2]&&s.string_calls[0],"Semantic selection/query/comparison chain");
            });
        }
        for(unsigned entry=0;entry<14;++entry)if(entry!=3) {
            auto a=setup(argv[1],entry);check(accessed_mapping(*a),"exact accessed byte mappings "+std::to_string(entry));
        }
        for(unsigned entry:{0u,1u}) {
            auto a=setup(argv[1],entry);a->memory.map(unmapped-1,1);a->memory.write<1>(unmapped-1,'a');
            a->wr<4>(12,unmapped-1);a->wr<4>(20,1);
            check(std::move(a),"Mac limit lookahead fault",{},true);
            a=setup(argv[1],entry);a->memory.write<1>(left,0);a->wr<4>(16,unmapped);a->wr<4>(20,1);
            check(std::move(a),"right terminator reread fault",{},true);
        }
        for(unsigned entry:{2u,3u}) {
            auto a=setup(argv[1],entry);a->wr<4>(4,unmapped);check(std::move(a),"wrapper first spill fault",{},true);
            a=setup(argv[1],entry);auto minimal=entry==2?accessed_mapping(*a):clone(*a);
            minimal->wr<4>(4,unmapped-8);minimal->memory.map(unmapped-16,24);
            check(std::move(minimal),"wrapper backchain fault after spill",[=](const State&,const Effects& e){
                require(addresses(e,2,unmapped-16,unmapped+8).size()==(entry==2?1:3),"Missing stores before frame fault");
            },true);
        }
        // Intervene only at the real allocator's final LR reload. This proves
        // the duplication ABI's null-result and bad-copy branches; it does not
        // claim a naturally occurring allocator failure.
        for(bool null_result:{false,true}) {
            auto a=setup(argv[1],3);
            check(std::move(a),null_result?"controlled allocator zero return":"controlled allocated-copy fault",
                null_result?Expectation([](const State& s,const Effects& e){
                    require(!s.r<4>(12)&&!s.string_calls[6]&&e.mutated,"Controlled zero must skip copy");
                }):Expectation{},!null_result,[=](State& s,Effects& e,std::uint32_t,unsigned){
                    if(s.pc==0x10775150&&!e.mutated){e.mutated=true;s.wr<4>(12,null_result?0:unmapped);}
                });
        }
        {
            auto a=setup(argv[1],3);
            a->memory.write<4>(toc-0x3b74,unmapped);
            check(std::move(a),"allocator dependency access fault",{},true);
        }
        // Mutations distinguish a captured table pointer from live contents.
        // Changing the slot during the first key read must not replace it.
        for(unsigned entry:{0u,1u,4u}) {
            auto a=setup(argv[1],entry);const auto slot=entry==4?0x618cu:0x62bcu;
            const auto mapping=std::uint32_t(a->memory.read<4>(toc-slot));
            check(std::move(a),"captured table pointer with live contents",{},false,
                [=](State& s,Effects& e,std::uint32_t address,unsigned){
                    if(address==left&&!e.mutated){e.mutated=true;s.memory.write<4>(toc-slot,unmapped);s.memory.write<1>(mapping+'b',128);}
                });
        }
        // Repeated candidate prefixes retain the inner haystack-before-needle
        // order and restart from the next candidate rather than skipping it.
        {
            auto a=setup(argv[1],13);a->memory.copy_in(left,{'a','a','a','b',0});a->memory.copy_in(right,{'a','a','b',0});
            check(std::move(a),"repeated substring prefixes",[](const State& s,const Effects& e){
                require(s.r<4>(12)==left+1,"Repeated-prefix first match");
                auto reads=addresses(e,4,left,right+4);
                require(reads.size()>6&&reads[0]==right&&reads[1]==left&&reads[2]==left&&reads[3]==right,"Substring inner access order");
            });
        }
        for(unsigned entry=0;entry<14;++entry) {
            auto a=setup(argv[1],entry);auto b=clone(*a);Boundaries boundaries;
            a->string_original=true;a->string_observer=&boundaries;run(*a);
            b->string_observer=&boundaries;run(*b);require(boundaries.cursor==boundaries.expected.size(),"Missing own boundaries");
            auto c=setup(argv[1],entry);c->string_observer=&boundaries;boundaries.cursor=0;
            boundaries.perturb=string_core_entries[entry];bool rejected=false;
            try{run(*c);}catch(const std::runtime_error& e){rejected=std::string(e.what()).starts_with("Own string boundary "+hex_address(boundaries.perturb));}
            require(rejected&&boundaries.mutated,"Own negative missed its return boundary");
            c->string_original=true;c->string_entry_original.fill(true);c->string_original_resume=string_core_entries[entry];
            reset_string_core_diagnostics(*c);
            require(!c->string_original&&!c->string_observer&&!c->string_original_resume&&c->string_entry_original==std::array<bool,14>{}&&
                c->string_calls==std::array<std::uint64_t,14>{}&&c->string_original_calls==std::array<std::uint64_t,14>{},"String diagnostics reset");
        }
        // Real generated transition vector, with wrapper and comparator both
        // semantic. The target and TOC remain guest-owned mutable data.
        {
            auto a=setup(argv[1],2);a->memory.write<4>(fixture+0x700,0x1074de10);a->memory.write<4>(fixture+0x704,toc);
            a->memory.write<4>(toc-0x7ee4,fixture+0x700);a->pc=0x10780900;auto b=clone(*a);
            compare_pair(*a,*b,"generated descriptor vector transition");
            require(b->string_calls[2]&&b->string_calls[0]&&b->string_original_calls==std::array<std::uint64_t,14>{},"Semantic vector dispatch");
        }
        require(std::all_of(witnesses.begin(),witnesses.end(),[](auto n){return n!=0;}),"Empty production string dispatch");
        std::cout<<"PASS: string core ABI/memory/ordered effects cases="<<cases<<" faults="<<faults
            <<" own negatives=14; nonempty production entries=14; Date fallbacks=6; formatting compositions=2; background compositions=2; mixed modes=4; vector transitions=1\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
