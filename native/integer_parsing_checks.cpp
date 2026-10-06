#include "integer_parsing_adapter.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture=0x30000000, stack=0x700fff00, toc=0x108bba90;
constexpr std::uint32_t left=fixture, right=fixture+0x100, destination=fixture+0x200;
constexpr std::uint32_t unmapped=0xffff0000;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> bytes;
    struct Access { unsigned kind; std::uint32_t address; std::uint64_t size, value; };
    std::vector<Access> log;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> accesses;
    State* state = nullptr;
    std::function<void(State&, Effects&, std::uint32_t, unsigned)> on_read;
    bool mutated = false;
    std::vector<std::array<std::uint32_t,5>> callbacks;
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
        if (state->pc==0x10781320) callbacks.push_back({std::uint32_t(state->r<4>(12)),std::uint32_t(state->r<4>(16)),std::uint32_t(state->r<4>(20)),std::uint32_t(state->r<4>(48)),std::uint32_t(state->r<4>(4128))});
        if (on_read) on_read(*state, *this, address, size);
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
    s->memory.map(fixture,0x2000); s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000u+n);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);
    s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);
    s->wr<1>(2304,variation%2?0xf9:0xa6);s->wr<1>(1024,variation%4);s->wr<1>(1027,(variation/2)%2);
    s->memory.copy_in(left,{'1','2','!',0});
    s->memory.write<4>(fixture+0x700,0x1077d960);s->memory.write<4>(fixture+0x704,toc);
    s->memory.write<4>(right,left);s->memory.write<4>(right+4,0);
    s->memory.write<4>(destination,0xdeadbeef);s->memory.write<4>(destination+4,0xdeadbeef);s->memory.write<4>(destination+8,0xdeadbeef);
    s->memory.write<4>(s->memory.read<4>(toc-0x5a58),0x12345678);
    if(entry<2) {
        s->wr<4>(12,10);s->wr<4>(16,0x7fffffff);s->wr<4>(20,fixture+0x700);s->wr<4>(24,right);
        s->wr<4>(28,destination);s->wr<4>(32,destination+4);s->wr<4>(36,destination+8);
    } else {s->wr<4>(12,left);s->wr<4>(16,destination);s->wr<4>(20,10);}
    s->pc=integer_parsing_entries[entry];s->budget=200000;s->executed=s->native_calls=0;
    return s;
}
unsigned cases=0,faults=0;
std::array<std::uint64_t,5> witnesses{};
struct Comparison { Effects effects; };
using Mutation=std::function<void(State&,Effects&,std::uint32_t,unsigned)>;
Comparison compare_pair(State& original,State& modern,const std::string& label,bool expect_fault=false,
    const Mutation& mutation={},const std::function<void(State&)>& execute=[](State& s){run(s);}) {
    require(!original.collection_original&&!modern.collection_original&&!original.background_original&&
        !modern.background_original&&!original.calendar_original&&!modern.calendar_original&&
        !original.rng_original&&!modern.rng_original&&!original.string_original&&!modern.string_original&&
        !original.memory_primitives_original&&!modern.memory_primitives_original,"Unrelated modules must remain semantic");
    original.integer_parsing_original=true;
    Effects expected,actual;expected.state=&original;actual.state=&modern;
    expected.on_read=actual.on_read=mutation;
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
    require(expected.callbacks==actual.callbacks,label+" callback protocol mismatch");
    require(original.rng_calls==modern.rng_calls&&original.background_calls==modern.background_calls&&
        original.collection_calls==modern.collection_calls&&original.string_calls==modern.string_calls,label+" unrelated dependency dispatch mismatch");
    require(original.memory_primitives_calls==modern.memory_primitives_calls,label+" memory dependency dispatch mismatch");
    require(modern.integer_parsing_phase_escapes==std::array<std::uint64_t,2>{},label+" escaped characterized phases");
    ++cases;if(expect_fault)++faults;
    if(modern.integer_parsing_original_calls==std::array<std::uint64_t,5>{})
        for(unsigned i=0;i<witnesses.size();++i)witnesses[i]+=modern.integer_parsing_calls[i];
    return {std::move(actual)};
}
using Expectation=std::function<void(const State&,const Effects&)>;
void check(std::unique_ptr<State> a,const std::string& label,const Expectation& expect={},bool fault=false,const Mutation& mutation={}) {
    auto b=clone(*a);auto result=compare_pair(*a,*b,label,fault,mutation);
    if(expect)expect(*b,result.effects);
}
struct Boundaries final:IntegerParsingBoundaryObserver {
    struct Boundary { std::uint32_t entry;Image image;Boundary(std::uint32_t e,const State& s):entry(e),image(s){} };
    std::vector<Boundary> expected;
    unsigned cursor=0;std::uint32_t perturb=0;bool mutated=false;
    void enter(const State&,std::uint32_t,bool) override {}
    void leave(State& s,std::uint32_t entry,bool original) override {
        if(original){expected.emplace_back(entry,s);return;}
        require(cursor<expected.size()&&expected[cursor].entry==entry,"Missing nested integer parsing boundary");
        if(entry==perturb&&!mutated){s.wr<4>(12,s.r<4>(12)^1);mutated=true;}
        expected[cursor++].image.compare(s,"Own integer parsing boundary "+hex_address(entry));
    }
};

// Minimal mappings retain exactly the original's observed guest accesses.
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted=UINT64_C(1)<<32) {
    auto probe=clone(initial);Effects effects;effects.state=probe.get();
    probe->integer_parsing_original=true;probe->memory.date_observer=&effects;
    run(*probe);probe->memory.date_observer=nullptr;
    auto result=std::make_unique<State>();result->registers=initial.registers;result->unique=initial.unique;
    result->pc=initial.pc;result->budget=initial.budget;
    result->allocated=initial.allocated;result->available=initial.available;
    result->heap_end=initial.heap_end;result->native_calls=initial.native_calls;result->memory_error=initial.memory_error;
    auto map=[&](std::uint32_t address,std::uint64_t size){
        if(address<=omitted&&omitted<std::uint64_t(address)+size){
            if(omitted>address)result->memory.map(address,omitted-address);
            if(omitted+1<std::uint64_t(address)+size)
                result->memory.map(std::uint32_t(omitted+1),std::uint64_t(address)+size-omitted-1);
        }else result->memory.map(address,size);
    };
    for(const auto& [address,size]:effects.accesses)map(address,size);
    for(const auto& [address,size]:result->memory.mapped_ranges())
        result->memory.copy_in(address,initial.memory.copy_out(address,size));
    return result;
}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: cm-integer-parsing-checks <original-segments>\n";return 2;}
    try {
        struct CoreCase { std::string text;unsigned base;std::uint64_t magnitude;unsigned consumed,sign;bool overflow32=false,overflow64=false; };
        const std::vector<CoreCase> core{
            {"",0,0,0,0},{" \t",0,0,0,0},{"+",10,0,0,0},{"-",10,0,0,1},
            {"  -12!",0,12,5,1},{"+15!",10,15,3,0},{"0",0,0,1,0},{"0x",0,0,0,0},{"0XG",16,0,0,0},
            {"0x2a!",0,42,4,0},{"0X2A!",16,42,4,0},{"09",0,0,1,0},{"077!",0,63,3,0},
            {"1012",2,5,3,0},{"17!",8,15,2,0},{"8",8,0,0,0},{"zZ!",36,1295,2,0},
            {"aF!",16,175,2,0},{"Z",10,0,0,0},{std::string("\x80",1),36,0,0,0},{std::string("\xff",1),10,0,0,0},
            {"2147483647!",10,2147483647,10,0},{"-2147483648!",10,2147483648,11,1},
            {"4294967295!",10,UINT32_MAX,10,0},{"4294967296!",10,UINT64_C(4294967296),10,0,true},
            {"-4294967297",10,UINT64_C(4294967297),11,1,true},
            {"18446744073709551615",10,UINT64_MAX,20,0,true},
            {"18446744073709551616",10,0,20,0,true,true},
            {"18446744073709551617!",10,1,20,0,true,true},
            {"ffffffffffffffff!",16,UINT64_MAX,16,0,true},
            {"10000000000000000!",16,0,17,0,true,true}
        };
        for(unsigned entry:{0u,1u})for(const auto& c:core) {
            auto a=setup(argv[1],entry,c.consumed);a->memory.copy_in(left,{c.text.begin(),c.text.end()});
            a->memory.write<1>(left+c.text.size(),0);a->wr<4>(12,c.base);
            check(std::move(a),"core base/text "+std::to_string(entry)+"/"+c.text,[=](const State& s,const Effects& e){
                const auto result=entry? (s.r<4>(12)<<32)|s.r<4>(16):s.r<4>(12);
                require(result==(entry?c.magnitude:std::uint32_t(c.magnitude)),"Independent magnitude/wrap");
                require(s.memory.read<4>(destination)==c.consumed&&s.memory.read<4>(destination+4)==c.sign&&
                    s.memory.read<4>(destination+8)==(entry?c.overflow64:c.overflow32),"Independent count/sign/overflow");
                require(!e.callbacks.empty()&&e.callbacks.back()[2]==1,"Final unread callback required");
                for(unsigned i=0;i+1<e.callbacks.size();++i)require(e.callbacks[i][2]==0,"Read callback mode");
                require(s.integer_parsing_phase_escapes==std::array<std::uint64_t,2>{},"Normal phases escaped semantic body");
            });
        }
        // Small limits expose lookahead around each sign/prefix/digit boundary.
        struct Limited {const char* text;unsigned limit,value,count,sign,cursor;};
        for(unsigned entry:{0u,1u})for(const auto& c:std::vector<Limited>{{"12!",1,1,1,0,1},{"12!",2,12,2,0,2},
            {"+12!",1,0,0,0,1},{"+12!",2,1,2,0,2},{"-12!",1,0,0,1,1},{"-12!",2,1,2,1,2},
            {"0x12!",1,0,1,0,1},{"0x12!",2,0,0,0,2},{"0x12!",3,1,3,0,3},{"0x12!",4,18,4,0,4},
            {"  12!",1,1,3,0,3}}) {
            auto a=setup(argv[1],entry);const std::string text=c.text;a->memory.copy_in(left,{text.begin(),text.end()});a->memory.write<1>(left+text.size(),0);
            a->wr<4>(12,0);a->wr<4>(16,c.limit);
            check(std::move(a),"field boundary",[=](const State& s,const Effects& e){
                require(s.r<4>(entry?16:12)==c.value&&(!entry||s.r<4>(12)==0),"Limited magnitude");
                require(s.memory.read<4>(destination)==c.count&&s.memory.read<4>(destination+4)==c.sign,"Limited count/sign");
                require(s.memory.read<4>(right)==left+c.cursor,"Unread lookahead cursor "+text+"/"+std::to_string(c.limit));
                require(e.callbacks.back()[2]==1,"Limit still unreads");
            });
        }
        for(unsigned entry:{0u,1u})for(std::uint32_t base:{0u,1u,37u,UINT32_MAX,0x80000000u})for(std::uint32_t limit:{0u,UINT32_MAX,1u}) {
            if(base==0&&limit==1)continue;
            auto a=setup(argv[1],entry);a->wr<4>(12,base);a->wr<4>(16,limit);
            check(std::move(a),"invalid base/limit",[](const State& s,const Effects& e){
                require(s.memory.read<4>(destination)==0&&s.memory.read<4>(destination+4)==0&&s.memory.read<4>(destination+8)==0,"Invalid initializes outputs");
                require(e.callbacks.size()==1&&e.callbacks[0][2]==1,"Invalid must still unread without read");
                require(s.memory.read<4>(right)==left-1,"Invalid callback context effect");
            });
        }
        for(unsigned entry:{0u,1u})for(std::uint32_t limit:{0x7fffffffu,0x80000000u}) {
            auto a=setup(argv[1],entry);a->memory.write<1>(left,0);a->wr<4>(16,limit);
            check(std::move(a),"extreme limit finite EOF");
        }
        struct SignedCase {const char* text;unsigned base;std::uint32_t value;unsigned count;bool range;};
        const std::vector<SignedCase> signed_cases{{"",10,0,0,false},{"  +",10,0,0,false},{"-",10,0,0,false},
            {" -12!",10,std::uint32_t(-12),4,false},{"0x",0,0,0,false},{"09",0,0,1,false},{"0x7fffffff!",0,0x7fffffff,10,false},
            {"2147483647",10,0x7fffffff,10,false},{"2147483648",10,0x7fffffff,10,true},
            {"-2147483648",10,0x80000000,11,false},{"-2147483649",10,0x80000000,11,true},
            {"4294967296!",10,0x7fffffff,10,true},{"-4294967296!",10,0x80000000,11,true},
            {"12",1,0,0,false},{"12",37,0,0,false}};
        for(unsigned entry:{2u,3u,4u})for(const auto& c:signed_cases)for(bool end:{false,true}) {
            if(entry!=2&&(c.base!=10||end))continue;
            auto a=setup(argv[1],entry);const std::string text=c.text;a->memory.copy_in(left,{text.begin(),text.end()});a->memory.write<1>(left+text.size(),0);
            a->wr<4>(16,end?destination:0);a->wr<4>(20,c.base);
            check(std::move(a),"signed wrapper "+std::to_string(entry)+"/"+text,[=](const State& s,const Effects&){
                require(s.r<4>(12)==c.value,"Signed result/saturation");
                if(end)require(s.memory.read<4>(destination)==left+c.count,"End pointer including no conversion");
                require(s.memory.read<4>(s.memory.read<4>(toc-0x5a58))==(c.range?34u:0x12345678u),"Range error without clearing success");
                require(s.integer_parsing_calls[0]&&s.integer_parsing_original_calls==std::array<std::uint64_t,5>{},"Wrapper production child dispatch");
            });
        }
        for(unsigned entry:{0u,1u,2u,3u,4u}) {
            auto a=setup(argv[1],entry);check(sparse(*a),"exact accessed mapping "+std::to_string(entry));
            auto probe=clone(*a);Effects effects;effects.state=probe.get();probe->integer_parsing_original=true;
            probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
            // One last byte per distinct accessed word/byte: frame, TOC, phase,
            // ctype, descriptor, input/context and output faults with prefixes.
            std::vector<std::uint32_t> missing;
            for(const auto& x:effects.log){const auto byte=x.address+std::uint32_t(x.size)-1;
                if(std::find(missing.begin(),missing.end(),byte)==missing.end())missing.push_back(byte);}
            for(auto byte:missing)check(sparse(*a,byte),"missing accessed byte "+std::to_string(entry)+"/"+hex_address(byte),{},true);
        }
        for(unsigned entry:{0u,1u}) {
            for(unsigned which:{28u,32u,36u}) {
                auto a=setup(argv[1],entry);a->wr<4>(which,unmapped);check(std::move(a),"invalid core output",{},true);
            }
            auto a=setup(argv[1],entry);a->wr<4>(24,unmapped);check(std::move(a),"invalid callback context",{},true);
            auto b=setup(argv[1],entry);b->wr<4>(20,unmapped);check(std::move(b),"invalid callback descriptor",{},true);
            auto c=setup(argv[1],entry);c->memory.write<4>(fixture+0x700,0x1077d9e0);c->memory.write<4>(right+4,UINT32_MAX);
            check(std::move(c),"alternate generated callback",[](const State& s,const Effects& e){
                require(s.memory.read<4>(destination)==0&&e.callbacks.size()==2,"Alternative EOF callback");
            });
            auto d=setup(argv[1],entry);const auto relocated=fixture+0x9000;
            for(unsigned offset:{0x62bcu,0x5a48u,0x3b24u,0x3b28u}) {d->memory.map(relocated-offset,4);d->memory.write<4>(relocated-offset,d->memory.read<4>(toc-offset));}
            d->wr<4>(8,relocated);
            check(std::move(d),"relocated parser TOC",[=](const State& s,const Effects&){require(s.r<4>(8)==relocated,"Callback restores relocated TOC");});
            auto f=setup(argv[1],entry);const auto table=std::uint32_t(f->memory.read<4>(toc-(entry?0x3b28:0x3b24)));
            f->memory.write<4>(table+4,entry?0x1077f394:0x1077f01c);
            check(std::move(f),"discriminant phase redirection",[](const State& s,const Effects&){require(s.r<4>(12)==12||s.r<4>(16)==12,"Redirected first phase parses digits");});
            auto g=setup(argv[1],entry);g->memory.write<4>(table+4,unmapped);
            auto h=clone(*g);g->integer_parsing_original=true;std::string x,y;
            Effects before,after;before.state=g.get();after.state=h.get();g->memory.date_observer=&before;h->memory.date_observer=&after;
            try{run(*g);}catch(const std::exception& e){x=e.what();}try{run(*h);}catch(const std::exception& e){y=e.what();}
            require(x==y&&x=="Uncompiled original address: "+hex_address(unmapped),"Unknown phase preserves runtime dispatch failure");
            g->memory.date_observer=h->memory.date_observer=nullptr;
            require(before.bytes==after.bytes&&before.callbacks==after.callbacks,"Unknown phase ordered effects");
            Image(*g).compare(*h,"Unknown phase transfer");require(h->integer_parsing_phase_escapes[entry]==1,"Unknown phase diagnostic");
            for(std::uint32_t alias:{destination,destination+4,stack-4,stack-112,stack-128}) {
                auto v=setup(argv[1],entry);v->wr<4>(28,alias);v->wr<4>(32,alias);v->wr<4>(36,alias);
                check(std::move(v),"output/output/spill alias");
            }
            auto k=setup(argv[1],entry);
            check(std::move(k),"mutable descriptor and clobber",[](const State& s,const Effects& e){
                require(e.mutated&&s.r<4>(44)==0x13572468,"Observable callback clobber");
            },false,[](State& s,Effects& e,std::uint32_t,unsigned){
                if(s.pc==0x1077d988&&!e.mutated){e.mutated=true;s.memory.write<4>(fixture+0x700,0x1077d9e0);s.memory.write<4>(right+4,UINT32_MAX);s.wr<4>(44,0x13572468);}
            });
            auto m=setup(argv[1],entry);const auto map=std::uint32_t(m->memory.read<4>(toc-0x62bc));
            m->memory.copy_in(left,{'a','!',0});m->memory.write<1>(map+'a','B');m->wr<4>(12,16);
            check(std::move(m),"mutable letter mapping",[=](const State& s,const Effects&){require(s.r<4>(entry?16:12)==11,"Guest mapping selects digit");});
            auto n=setup(argv[1],entry);n->memory.copy_in(left,{'a','!',0});n->memory.write<1>(map+'a','B');n->wr<4>(12,16);
            check(std::move(n),"mapping reread mutation",[](const State& s,const Effects& e){require(e.mutated&&s.r<4>(s.integer_parsing_calls[1]?16:12)==12,"Second letter mapping is live");},false,
                [=](State& s,Effects& e,std::uint32_t address,unsigned){if(address==map+'a'&&!e.mutated&&std::count_if(e.log.begin(),e.log.end(),[=](const auto& x){return x.kind==4&&x.address==address;})==2){e.mutated=true;s.memory.write<1>(address,'C');}});
            auto sequence=setup(argv[1],entry);sequence->memory.write<4>(fixture+0x700,0x1077d9e0);
            sequence->memory.write<4>(right+4,'1');
            check(std::move(sequence),"noncontiguous generated callback sequence",[=](const State& s,const Effects& e){
                require(s.r<4>(entry?16:12)==12&&s.memory.read<4>(destination)==2&&e.callbacks.size()==4,"Callback sequence magnitude/count/events entry="+std::to_string(entry)+" result="+std::to_string(s.r<4>(entry?16:12))+" count="+std::to_string(s.memory.read<4>(destination))+" events="+std::to_string(e.callbacks.size()));
                require(s.memory.read<4>(right)==left,"Alternative callback does not traverse text");
            },false,[](State& s,Effects& e,std::uint32_t address,unsigned){
                if(s.pc==0x1077d9e0&&address==right+4)
                    s.memory.write<4>(right+4,e.callbacks.size()==1?'1':e.callbacks.size()==2?'2':UINT32_MAX);
            });
            auto unusual=setup(argv[1],entry);unusual->memory.copy_in(left,{'!',0});
            const auto unusual_ctype=std::uint32_t(unusual->memory.read<4>(toc-0x5a48));
            unusual->memory.write<1>(unusual_ctype+'!',16);
            check(std::move(unusual),"guest classification creates negative digit",[=](const State& s,const Effects&){
                require(s.r<4>(entry?16:12)==std::uint32_t(-15)&&(!entry||s.r<4>(12)==UINT32_MAX),"Live signed digit extension");
                require(s.memory.read<4>(destination)==1&&s.memory.read<4>(destination+8)==0,"Negative digit count/overflow");
            });
            auto p=setup(argv[1],entry);const auto ctype=std::uint32_t(p->memory.read<4>(toc-0x5a48));
            p->memory.write<1>(ctype+'1',0);
            check(std::move(p),"mutable classification",[](const State& s,const Effects&){require(s.memory.read<4>(destination)==0,"Guest classification rejects digit");});
        }
        for(std::uint32_t alias:{stack-4,stack-8,stack-40,stack-36,stack-32}) {
            auto a=setup(argv[1],2);a->wr<4>(16,alias);check(std::move(a),"wrapper end/frame/flag alias");
        }
        for(bool range:{false,true}) {
            auto a=setup(argv[1],2);if(range)a->memory.copy_in(left,{'9','9','9','9','9','9','9','9','9','9',0});
            a->wr<4>(16,unmapped);check(std::move(a),"wrapper end write fault",{},true);
            if(range) {auto b=setup(argv[1],2);b->memory.copy_in(left,{'9','9','9','9','9','9','9','9','9','9',0});b->memory.write<4>(toc-0x5a58,unmapped);check(std::move(b),"wrapper range-error store fault",{},true);}
        }
        for(bool own_original:{false,true}) {
            auto a=setup(argv[1],2);auto b=clone(*a);b->integer_parsing_entry_original[2]=own_original;b->integer_parsing_entry_original[0]=!own_original;
            compare_pair(*a,*b,"mixed wrapper/parser");require((own_original?b->integer_parsing_original_calls[2]:b->integer_parsing_calls[2])&&
                (own_original?b->integer_parsing_calls[0]:b->integer_parsing_original_calls[0]),"Mixed dependency selection");
        }
        for(unsigned entry=0;entry<5;++entry) {
            auto a=setup(argv[1],entry);auto b=clone(*a);Boundaries boundaries;
            a->integer_parsing_original=true;a->integer_parsing_observer=&boundaries;run(*a);b->integer_parsing_observer=&boundaries;run(*b);
            require(boundaries.cursor==boundaries.expected.size(),"Missing own return boundaries");
            if(entry>=2)require(boundaries.expected.front().entry==integer_parsing_entries[0]&&boundaries.expected.back().entry==integer_parsing_entries[entry],"Child before parent");
            auto c=setup(argv[1],entry);c->integer_parsing_observer=&boundaries;boundaries.cursor=0;boundaries.perturb=integer_parsing_entries[entry];bool rejected=false;
            try{run(*c);}catch(const std::runtime_error& e){rejected=std::string(e.what()).starts_with("Own integer parsing boundary "+hex_address(boundaries.perturb));}
            require(rejected&&boundaries.mutated&&boundaries.cursor==boundaries.expected.size(),"Own negative missed first own return");
            c->integer_parsing_original=true;c->integer_parsing_entry_original.fill(true);c->integer_parsing_original_resume=integer_parsing_entries[entry];c->integer_parsing_phase_escapes.fill(1);
            reset_integer_parsing_diagnostics(*c);require(!c->integer_parsing_original&&!c->integer_parsing_observer&&!c->integer_parsing_original_resume&&
                c->integer_parsing_entry_original==std::array<bool,5>{}&&c->integer_parsing_calls==std::array<std::uint64_t,5>{}&&
                c->integer_parsing_original_calls==std::array<std::uint64_t,5>{}&&c->integer_parsing_phase_escapes==std::array<std::uint64_t,2>{},"Integer diagnostics reset");
            auto indirect=setup(argv[1],entry);indirect->pc=0x10780900;indirect->memory.write<4>(fixture+0x700,integer_parsing_entries[entry]);
            indirect->memory.write<4>(toc-0x7ee4,fixture+0x700);
            // Parser callback must have its own descriptor after the transition.
            indirect->memory.write<4>(fixture+0x710,0x1077d960);indirect->memory.write<4>(fixture+0x714,toc);
            if(entry<2)indirect->wr<4>(20,fixture+0x710);
            check(std::move(indirect),"generated indirect parser transition",[=](const State& s,const Effects&){require(s.integer_parsing_calls[entry]&&s.integer_parsing_original_calls==std::array<std::uint64_t,5>{},"Indirect normal dispatch");});
        }
        {
            auto a=setup(argv[1],3);Boundaries boundaries;a->integer_parsing_original=true;a->integer_parsing_observer=&boundaries;run(*a);
            auto b=setup(argv[1],3);b->integer_parsing_observer=&boundaries;boundaries.perturb=integer_parsing_entries[0];bool rejected=false;
            try{run(*b);}catch(const std::runtime_error& e){rejected=std::string(e.what()).starts_with("Own integer parsing boundary "+hex_address(boundaries.perturb));}
            require(rejected&&boundaries.cursor==1&&boundaries.mutated,"Child negative must fail before parent");
        }
        // The generated string facade takes text/format/guest varargs in r3/4/5.
        // Use an explicit destination and %n to independently check consumption.
        for(bool wide:{false,true}) {
            auto a=setup(argv[1],2);a->pc=0x1077da70;
            const std::string text=wide?"18446744073709551615!":"-123!",format=wide?"%llu%n":"%d%n";
            a->memory.copy_in(left,{text.begin(),text.end()});a->memory.write<1>(left+text.size(),0);
            a->memory.copy_in(right,{format.begin(),format.end()});a->memory.write<1>(right+format.size(),0);
            a->memory.write<4>(fixture+0x1000,destination);a->memory.write<4>(fixture+0x1004,destination+16);
            a->wr<4>(12,left);a->wr<4>(16,right);a->wr<4>(20,fixture+0x1000);
            check(std::move(a),"generated scanner composition",[=](const State& s,const Effects&){
                require(s.r<4>(12)==1,"Scanner assignment count");
                if(wide)require(s.memory.read<8>(destination)==UINT64_MAX,"Scanner uint64 destination");
                else require(s.memory.read<4>(destination)==std::uint32_t(-123),"Scanner signed32 destination");
                require(s.memory.read<4>(destination+16)==(wide?20:4),"Scanner consumed count");
                require(s.memory.read<4>(stack-64+56)==left+(wide?20:4),"Facade callback cursor");
                require(s.integer_parsing_calls[wide?1:0]&&s.integer_parsing_original_calls==std::array<std::uint64_t,5>{},"Scanner real parser dispatch");
            });
        }
        require(std::all_of(witnesses.begin(),witnesses.end(),[](auto n){return n!=0;}),"Empty production integer dispatch");
        std::cout<<"PASS: integer parsing ABI/memory/ordered effects cases="<<cases<<" faults="<<faults
            <<" own negatives=5; child negative=1; nonempty production entries=5; mixed modes=2; indirect transitions=5; scanner compositions=2\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
