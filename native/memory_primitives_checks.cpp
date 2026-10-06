#include "memory_primitives_adapter.hpp"
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
    std::function<void(State&, Effects&, std::uint32_t, unsigned)> after_read;
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
    if(entry==0){s->wr<4>(12,destination);s->wr<4>(16,left);}
    if(entry==1||entry==5){s->wr<4>(12,destination);s->wr<4>(16,0xa6);s->wr<4>(20,33);}
    if(entry==2||entry==3)s->wr<4>(16,'a');
    s->pc=memory_primitives_entries[entry];s->budget=200000;s->executed=s->native_calls=0;
    return s;
}
unsigned cases=0,faults=0;
std::array<std::uint64_t,6> witnesses{};
struct Comparison { Effects effects; };
using Mutation=std::function<void(State&,Effects&,std::uint32_t,unsigned)>;
Comparison compare_pair(State& original,State& modern,const std::string& label,bool expect_fault=false,
    const Mutation& mutation={},const std::function<void(State&)>& execute=[](State& s){run(s);}) {
    require(!original.collection_original&&!modern.collection_original&&!original.background_original&&
        !modern.background_original&&!original.calendar_original&&!modern.calendar_original&&
        !original.rng_original&&!modern.rng_original&&!original.string_original&&!modern.string_original,"Unrelated modules must remain semantic");
    original.memory_primitives_original=true;
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
        original.collection_calls==modern.collection_calls&&original.string_calls==modern.string_calls,label+" unrelated dependency dispatch mismatch");
    ++cases;if(expect_fault)++faults;
    if(modern.memory_primitives_original_calls==std::array<std::uint64_t,6>{})
        for(unsigned i=0;i<6;++i)witnesses[i]+=modern.memory_primitives_calls[i];
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
struct Boundaries final:MemoryPrimitivesBoundaryObserver {
    struct Boundary { std::uint32_t entry;Image image;Boundary(std::uint32_t e,const State& s):entry(e),image(s){} };
    std::vector<Boundary> expected;
    unsigned cursor=0;std::uint32_t perturb=0;bool mutated=false;
    void enter(const State&,std::uint32_t,bool) override {}
    void leave(State& s,std::uint32_t entry,bool original) override {
        if(original){expected.emplace_back(entry,s);return;}
        require(cursor<expected.size()&&expected[cursor].entry==entry,"Missing nested memory primitive boundary");
        if(entry==perturb&&!mutated){s.wr<4>(12,s.r<4>(12)^1);mutated=true;}
        expected[cursor++].image.compare(s,"Own memory primitive boundary "+hex_address(entry));
    }
};

// Minimal mappings retain exactly the observed scalar accesses. Carbon's bulk
// source interval is added explicitly: copy_out does not emit read events.
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted=UINT64_C(1)<<32) {
    auto probe=clone(initial);Effects effects;effects.state=probe.get();
    probe->memory_primitives_original=true;probe->memory.date_observer=&effects;
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
    if(initial.pc==memory_primitives_entries[0]&&initial.r<4>(20))
        map(std::uint32_t(initial.r<4>(16)),initial.r<4>(20));
    for(const auto& [address,size]:result->memory.mapped_ranges())
        result->memory.copy_in(address,initial.memory.copy_out(address,size));
    return result;
}
void expect_fill(const State& s,const Effects& e,std::uint32_t base,unsigned count,
                 std::uint32_t value,bool wrapper) {
    require(s.memory.copy_out(base,count)==std::vector<std::uint8_t>(count,std::uint8_t(value)),"Independent filled bytes");
    require(s.memory.read<1>(base-1)==0xa5&&s.memory.read<1>(base+count)==0xa5,"Fill guard bytes");
    std::vector<std::pair<std::uint32_t,std::uint64_t>> actual,expected;
    for(const auto& access:e.log)if(access.kind==2&&base<=access.address&&access.address<base+count)
        actual.emplace_back(access.address,access.size);
    unsigned offset=0;
    if(count>=32){
        const auto aligned=(base+3u)&~3u;
        while(base+offset<aligned)expected.emplace_back(base+offset++,1);
        while(count-offset>=4){expected.emplace_back(base+offset,4);offset+=4;}
    }
    while(offset<count)expected.emplace_back(base+offset++,1);
    require(actual==expected,"Fill store widths/order");
    require(s.r<4>(12)==(wrapper||count<32?base:0),"Leaf versus wrapper return");
    require(s.r<4>(20)==0&&s.r<4>(24)==base+count-1,"Fill remaining count/cursor");
    if(count)require(s.r<1>(1027)==1,"Fill final carry");
    if(wrapper)require(s.memory_primitives_calls[1]&&s.memory_primitives_calls[5],"Wrapper real leaf dispatch");
}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: cm-memory-primitives-checks <original-segments>\n";return 2;}
    try {
        const std::vector<std::uint8_t> block{0,254,1,254,128,0,127,254};
        for(unsigned entry:{2u,3u})for(std::uint32_t wanted:{0u,254u,0x1234feu,128u,77u})
            for(unsigned count:{0u,1u,3u,4u,8u}) {
                auto a=setup(argv[1],entry,count);a->memory.copy_in(left,block);
                a->wr<4>(16,wanted);a->wr<4>(20,count);
                if(!count)a->wr<4>(12,unmapped);
                std::uint32_t expected=0;unsigned consumed=count;
                for(unsigned i=0;i<count;++i){const auto index=entry==2?i:count-i-1;
                    if(block[index]==std::uint8_t(wanted)){expected=left+index;consumed=i+1;break;}}
                check(std::move(a),"bounded search "+std::to_string(entry)+"/"+std::to_string(count),
                    [=](const State& s,const Effects& e){
                        require(s.r<4>(12)==expected&&s.r<4>(16)==std::uint8_t(wanted),"Independent search pointer/byte");
                        auto reads=addresses(e,4,left,left+block.size());
                        std::vector<std::uint32_t> ordered;
                        for(unsigned i=0;i<consumed;++i)ordered.push_back(left+(entry==2?i:count-i-1));
                        require(reads==ordered,"Search direction and early termination");
                        require(s.r<4>(20)==(expected?count-consumed+1:0),"Search count residue");
                        if(!count)require(e.log.empty()&&s.r<1>(1027)==1,"Zero search accessed data/carry");
                    });
            }
        for(unsigned entry:{2u,3u})for(std::uint32_t count:{0x7fffffffu,0x80000000u,UINT32_MAX}) {
            auto a=setup(argv[1],entry);const auto end=unmapped;
            a->memory.map(end-2,2);a->memory.copy_in(end-2,{1,2});
            a->wr<4>(12,entry==2?end-2:end-count);a->wr<4>(16,99);a->wr<4>(20,count);
            check(std::move(a),"extreme search finite fault",[=](const State& s,const Effects&){
                require(s.r<4>(12)==(entry==2?end-1:end-2),"Failed search must retain cursor");
                require(s.r<4>(20)==count-2,"Extreme search word32 count");
            },true);
        }
        for(unsigned entry:{2u,3u}) {
            auto a=setup(argv[1],entry);a->memory.map(UINT32_MAX,1);a->memory.map(0,1);
            a->memory.write<1>(UINT32_MAX,7);a->memory.write<1>(0,254);
            a->wr<4>(12,UINT32_MAX);a->wr<4>(16,entry==2?254:7);a->wr<4>(20,2);
            check(std::move(a),"search address wrap",[=](const State& s,const Effects&){
                require(s.r<4>(12)==(entry==2?0:UINT32_MAX)&&s.r<4>(20)==1,"Wrapped match address/count");
                require((s.r<1>(2304)&2)!=0,"Wrapped zero pointer is still a found match");
            });
        }
        // Explicit binary comparisons include internal NULs and unsigned bytes.
        for(unsigned count:{0u,1u,4u,8u})for(unsigned difference:{0u,1u,4u,8u})for(bool greater:{false,true}) {
            auto a=setup(argv[1],4,count);auto lhs=block,rhs=block;
            if(difference){lhs[difference-1]=greater?255:0;rhs[difference-1]=greater?0:255;}
            a->memory.copy_in(left,lhs);a->memory.copy_in(right,rhs);a->wr<4>(20,count);
            if(!count){a->wr<4>(12,unmapped);a->wr<4>(16,unmapped);}
            const auto expected=difference&&difference<=count?(greater?1u:UINT32_MAX):0u;
            check(std::move(a),"binary block comparison",[=](const State& s,const Effects& e){
                require(s.r<4>(12)==expected,"Independent normalized block result");
                if(!count)require(e.log.empty()&&s.r<4>(16)==0&&s.r<1>(1027)==1,"Zero comparison ABI");
                if(expected)require(addresses(e,4,left,right+8).size()==2*difference+2,"Divergence reread count");
            });
        }
        for(bool become_equal:{false,true}) {
            auto a=setup(argv[1],4);a->memory.write<1>(left,1);a->memory.write<1>(right,2);
            check(std::move(a),"controlled divergence reread",[=](const State& s,const Effects& e){
                require(e.mutated&&s.r<4>(12)==1,"Comparison must decide from reread bytes");
                require(addresses(e,4,left,right+1)==std::vector<std::uint32_t>{left,right,left,right},"Exact reread order");
            },false,[=](State& s,Effects& e,std::uint32_t,unsigned){
                if(s.pc==0x107783b0&&!e.mutated){e.mutated=true;s.memory.write<1>(left,become_equal?2:255);}
            });
        }
        for(std::uint32_t count:{0x7fffffffu,0x80000000u,UINT32_MAX}) {
            auto a=setup(argv[1],4);a->memory.map(unmapped-2,2);a->memory.copy_in(unmapped-2,{0,0});
            a->memory.copy_in(right,{0,0});a->wr<4>(12,unmapped-2);a->wr<4>(20,count);
            check(std::move(a),"extreme comparison finite fault",[](const State& s,const Effects&){
                require(s.pc==0x107783a0&&s.r<4>(24)==unmapped-1,"Comparison failed updating load cursor");
            },true);
        }
        std::vector<std::pair<unsigned,unsigned>> fills{{0,0},{1,1},{2,4},{3,7},{0,63},{0,64},{1,67},{2,97}};
        for(unsigned alignment=0;alignment<4;++alignment)for(unsigned count:{31u,32u,33u})fills.emplace_back(alignment,count);
        for(unsigned entry:{1u,5u})for(unsigned i=0;i<fills.size();++i) {
            const auto [alignment,count]=fills[i];const auto base=destination+alignment;
            const std::array<std::uint32_t,4> values{0,128,0x1234a6,255};const auto value=values[i%values.size()];
            auto a=setup(argv[1],entry,i);a->memory.copy_in(destination-1,std::vector<std::uint8_t>(128,0xa5));
            a->wr<4>(12,base);a->wr<4>(16,value);a->wr<4>(20,count);
            check(std::move(a),"fill alignment/boundary "+std::to_string(entry)+"/"+std::to_string(i),
                [=](const State& s,const Effects& e){expect_fill(s,e,base,count,value,entry==1);});
        }
        for(unsigned entry:{1u,5u}) {
            auto a=setup(argv[1],entry);a->wr<4>(12,unmapped);a->wr<4>(20,0);
            check(std::move(a),"zero fill invalid destination",[=](const State& s,const Effects& e){
                require(s.r<4>(12)==unmapped&&addresses(e,2,unmapped,unmapped+1).empty(),"Zero fill wrote data");
                if(entry==5)require(e.log.empty(),"Zero leaf fill accessed data");
            });
        }
        for(unsigned count:{1u,31u,32u,33u,64u,67u})for(unsigned offset:{0u,1u,3u}) {
            auto a=setup(argv[1],5);const auto base=destination+offset;a->wr<4>(12,base);a->wr<4>(20,count);
            for(unsigned missing:{0u,count-1}) {
                auto minimal=sparse(*a,base+missing);
                check(std::move(minimal),"fill partial scalar fault",{},true);
            }
        }
        for(std::uint32_t count:{0x80000000u,UINT32_MAX}) {
            auto a=setup(argv[1],5);a->memory.map(unmapped-8,8);a->wr<4>(12,unmapped-8);a->wr<4>(20,count);
            check(std::move(a),"extreme fill finite fault",[](const State& s,const Effects&){
                require(s.pc==0x1077844c&&s.r<4>(16)==unmapped-12,"Group store failure keeps base");
                require(s.memory.copy_out(unmapped-8,8)==std::vector<std::uint8_t>(8,0xa6),"Group store prefix");
            },true);
        }
        for(unsigned count:{0u,1u,6u,32u})for(int overlap:{-2,0,2,256}) {
            auto a=setup(argv[1],0);const auto src=left+16,dst=std::uint32_t(src+overlap);
            a->memory.copy_in(left,std::vector<std::uint8_t>(320,0xa5));
            std::vector<std::uint8_t> payload(32);for(unsigned i=0;i<payload.size();++i)payload[i]=std::uint8_t(i);
            a->memory.copy_in(src,payload);auto expected=a->memory.copy_out(left,320);
            std::copy_n(payload.begin(),count,expected.begin()+dst-left);
            a->wr<4>(12,count?dst:unmapped);a->wr<4>(16,count?src:unmapped);a->wr<4>(20,count);
            check(std::move(a),"Carbon block copy overlap",[=](const State& s,const Effects& e){
                require(s.r<4>(12)==(count?dst:unmapped)&&s.native_calls==1,"Copy return and real zero-count import");
                require(s.memory.copy_out(left,320)==expected,"Independent captured-source copy");
                require(addresses(e,4,left,left+320).empty(),"Bulk copy invented scalar source reads");
                unsigned writes=0;for(const auto& x:e.log)if(x.kind==2&&x.address==dst){++writes;require(x.size==count,"Copy must write one bulk destination");}
                require(writes==(count?1u:0u),"Copy data write count");
            });
        }
        for(bool source_fault:{false,true})for(std::uint32_t count:{1u,3u,UINT32_MAX}) {
            auto a=setup(argv[1],0);a->memory.map(unmapped-2,2);a->memory.copy_in(unmapped-2,{7,8});
            a->wr<4>(source_fault?16:12,count==1?unmapped:unmapped-2);a->wr<4>(20,count);
            check(std::move(a),"copy bulk validation fault",[](const State& s,const Effects& e){
                require(s.native_calls==0,"Failed import must not increment native calls");
                require(addresses(e,2,left,left+0x2000).empty(),"Failed bulk copy wrote a partial destination");
            },true);
        }
        for(std::uint32_t alias:{stack-4,stack+8,stack-64,stack-44})for(bool source_alias:{false,true}) {
            auto a=setup(argv[1],0);a->wr<4>(20,4);a->memory.write<4>(left,return_sentinel);
            a->wr<4>(source_alias?16:12,alias);
            check(std::move(a),"copy frame/spill/TOC alias",[=](const State& s,const Effects&){
                require(s.r<4>(12)==(source_alias?destination:alias),"Copy alias destination return");
                const auto expected=source_alias?(alias==stack-4?0xcafe001fu:
                    alias==stack+8?return_sentinel:alias==stack-64?stack:toc):return_sentinel;
                require(s.memory.read<4>(source_alias?destination:alias)==expected,"Copy alias captures spill/frame bytes");
                if(!source_alias&&alias==stack-4)require(s.r<4>(124)==return_sentinel,"Copy restores overwritten r31 spill");
                if(!source_alias&&alias==stack-44)require(s.r<4>(8)==return_sentinel,"Copy reloads mutable TOC spill");
            });
        }
        for(const auto& [base,count]:std::array<std::pair<std::uint32_t,unsigned>,2>{{{stack-4,4},{stack-64,33}}}) {
            auto a=setup(argv[1],1);a->wr<4>(12,base);a->wr<4>(20,count);
            check(std::move(a),"fill wrapper spill/frame alias",[=](const State& s,const Effects&){
                require(s.r<4>(12)==base&&s.memory.copy_out(base,count)==std::vector<std::uint8_t>(count,0xa6),"Aliased wrapper return/payload");
                require(s.r<4>(124)==(base==stack-4?0xa6a6a6a6u:0xcafe001fu),"Fill restores mutable r31 spill");
            });
        }
        {
            auto a=setup(argv[1],0);const auto relocated=fixture+0x9000;
            a->memory.map(relocated-0x7e64,4);
            a->memory.write<4>(relocated-0x7e64,a->memory.read<4>(toc-0x7e64));a->wr<4>(8,relocated);
            check(std::move(a),"relocated copy TOC",[=](const State& s,const Effects&){require(s.r<4>(8)==relocated&&s.r<4>(2440)==relocated,"Relocated TOC ABI");});
        }
        {
            auto a=setup(argv[1],0);
            check(std::move(a),"controlled dependency clobber",[](const State& s,const Effects& e){
                require(e.mutated&&s.r<4>(12)==right,"Dependency r31 clobber remains visible");
            },false,[](State& s,Effects& e,std::uint32_t,unsigned){
                if(s.pc==0x1077fc34&&!e.mutated){e.mutated=true;s.wr<4>(124,right);}
            });
        }
        // Per-access missing bytes discriminate scalar fault prefixes, including
        // partially mapped vector/TOC/frame slots. They do not change Memory.
        for(unsigned entry=0;entry<6;++entry) {
            auto null=setup(argv[1],entry);null->wr<4>(12,0);null->wr<4>(20,1);
            check(std::move(null),"nonempty null destination/base without guard "+std::to_string(entry),{},true);
            auto a=setup(argv[1],entry);check(sparse(*a),"exact accessed mapping "+std::to_string(entry));
            auto probe=clone(*a);Effects effects;effects.state=probe.get();probe->memory_primitives_original=true;
            probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
            std::vector<std::uint32_t> missing;
            for(const auto& x:effects.log){const auto byte=x.address+std::uint32_t(x.size)-1;
                if(std::find(missing.begin(),missing.end(),byte)==missing.end())missing.push_back(byte);}
            if(entry==0)missing.push_back(left+1); // Carbon bulk source, no read event.
            for(auto byte:missing)check(sparse(*a,byte),"missing accessed byte "+std::to_string(entry),{},true);
        }
        for(bool own_original:{false,true}) {
            auto a=setup(argv[1],1);auto b=clone(*a);
            b->memory_primitives_entry_original[1]=own_original;b->memory_primitives_entry_original[5]=!own_original;
            compare_pair(*a,*b,"mixed fill wrapper/leaf");
            require((own_original?b->memory_primitives_original_calls[1]:b->memory_primitives_calls[1])&&
                (own_original?b->memory_primitives_calls[5]:b->memory_primitives_original_calls[5]),"Mixed fill dependency dispatch");
        }
        for(unsigned entry=0;entry<6;++entry) {
            auto a=setup(argv[1],entry);auto b=clone(*a);Boundaries boundaries;
            a->memory_primitives_original=true;a->memory_primitives_observer=&boundaries;run(*a);
            b->memory_primitives_observer=&boundaries;run(*b);
            require(boundaries.cursor==boundaries.expected.size(),"Missing own return boundaries");
            if(entry==1)require(boundaries.expected.size()==2&&boundaries.expected.front().entry==memory_primitives_entries[5],"Nested leaf before parent");
            auto c=setup(argv[1],entry);c->memory_primitives_observer=&boundaries;boundaries.cursor=0;
            boundaries.perturb=memory_primitives_entries[entry];bool rejected=false;
            try{run(*c);}catch(const std::runtime_error& e){rejected=std::string(e.what()).starts_with("Own memory primitive boundary "+hex_address(boundaries.perturb));}
            require(rejected&&boundaries.mutated&&boundaries.cursor==boundaries.expected.size(),"Own negative missed first own return");
            c->memory_primitives_original=true;c->memory_primitives_entry_original.fill(true);c->memory_primitives_original_resume=memory_primitives_entries[entry];
            reset_memory_primitives_diagnostics(*c);
            require(!c->memory_primitives_original&&!c->memory_primitives_observer&&!c->memory_primitives_original_resume&&
                c->memory_primitives_entry_original==std::array<bool,6>{}&&c->memory_primitives_calls==std::array<std::uint64_t,6>{}&&
                c->memory_primitives_original_calls==std::array<std::uint64_t,6>{},"Memory diagnostics reset");
            auto indirect=setup(argv[1],entry);indirect->pc=0x10780900;
            indirect->memory.write<4>(fixture+0x700,memory_primitives_entries[entry]);indirect->memory.write<4>(fixture+0x704,toc);
            indirect->memory.write<4>(toc-0x7ee4,fixture+0x700);
            check(std::move(indirect),"generated indirect primitive transition",[=](const State& s,const Effects&){
                require(s.memory_primitives_calls[entry]&&s.memory_primitives_original_calls==std::array<std::uint64_t,6>{},"Indirect normal dispatch");
            });
        }
        {
            auto a=setup(argv[1],1);Boundaries boundaries;
            a->memory_primitives_original=true;a->memory_primitives_observer=&boundaries;run(*a);
            auto b=setup(argv[1],1);b->memory_primitives_observer=&boundaries;
            boundaries.perturb=memory_primitives_entries[5];bool rejected=false;
            try{run(*b);}catch(const std::runtime_error& e){rejected=std::string(e.what()).starts_with("Own memory primitive boundary "+hex_address(boundaries.perturb));}
            require(rejected&&boundaries.cursor==1&&boundaries.mutated,"Child negative must fail before parent return");
        }
        for(const auto& text:std::vector<std::vector<std::uint8_t>>{{0},{'a','b','c',0}}) {
            auto a=setup(argv[1],0);a->pc=0x107784c0;a->wr<4>(12,destination);a->wr<4>(16,left);
            a->memory.copy_in(left,text);
            check(std::move(a),"generated Pascal conversion",[=](const State& s,const Effects&){
                require(s.r<4>(12)==destination+1&&s.memory.read<1>(destination)==text.size()-1,"Pascal length/return");
                require(s.memory.copy_out(destination+1,text.size()-1)==std::vector<std::uint8_t>(text.begin(),text.end()-1),"Pascal payload");
                require(s.memory.read<1>(destination+text.size())==0xa5,"Pascal guard");
                require(s.string_calls[5]==1&&s.memory_primitives_calls[0]==1&&s.native_calls==1,"Pascal real length/copy/import chain");
            });
        }
        for(const std::string format:{"%.8s!","%.3s!"}) {
            auto a=setup(argv[1],2);a->pc=0x1077b680;
            for(unsigned flag=1280;flag<1312;++flag)a->wr<1>(flag,0);
            a->memory.copy_in(left,{'a','l','p','h','a',0});
            a->memory.copy_in(right,{format.begin(),format.end()});a->memory.write<1>(right+format.size(),0);
            a->memory.write<4>(fixture+0x1000,left);
            a->wr<4>(12,destination);a->wr<4>(16,64);a->wr<4>(20,right);a->wr<4>(24,fixture+0x1000);
            check(std::move(a),"generated precision string formatter",[=](const State& s,const Effects&){
                const std::string expected=format=="%.8s!"?"alpha!":"alp!";
                require(s.r<4>(12)==expected.size()&&s.memory.copy_out(destination,expected.size())==std::vector<std::uint8_t>(expected.begin(),expected.end()),"Precision string formatting output");
                require(s.memory.read<1>(destination+expected.size())==0&&s.memory.read<1>(destination+expected.size()+1)==0xa5,"Formatter terminator/guard");
                require(s.memory_primitives_calls[2]!=0,"Formatter must reach limited byte search");
            });
        }
        require(std::all_of(witnesses.begin(),witnesses.end(),[](auto n){return n!=0;}),"Empty production memory dispatch");
        std::cout<<"PASS: memory primitives ABI/memory/ordered effects cases="<<cases<<" faults="<<faults
            <<" own negatives=6; nonempty production entries=6; mixed modes=2; indirect transitions=6; Pascal compositions=2; precision string formats=2\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
