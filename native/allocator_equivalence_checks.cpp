#include "allocator_adapter.hpp"
#include "allocator_bridge.hpp"
#include "semantic_io_checks.hpp"
#include <numeric>
using namespace cm::semantic_io_checks;
namespace {
constexpr std::uint32_t context=fixture+0x1000, arena=fixture+0x2000,
    block=arena+24, second_arena=fixture+0x4000, pool=fixture+0x6000,
    slots=pool+28, hint=fixture+0x7000, relocated_toc=fixture+0xb000,
    class_table=fixture+0x8000;
constexpr std::array<std::uint32_t,6> classes{4,12,28,44,60,76};
std::uint32_t reg(const State& s,unsigned n){return std::uint32_t(s.r<4>(n*4));}
void args(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> values){
    unsigned n=3;for(auto value:values)s.wr<4>(n++*4,value);
    s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+100000;
}
std::uint32_t invoke(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> values){args(s,entry,values);run(s);return reg(s,3);}
void raw_arena(State& s,std::uint32_t base,unsigned size=4096){
    s.memory.write<4>(base,base);s.memory.write<4>(base+4,base);s.memory.write<4>(base+16,size-32);
    s.memory.write<4>(base+20,size|3);s.memory.write<4>(base+24,size-32);s.memory.write<4>(base+28,base|1);
    s.memory.write<4>(base+32,base+24);s.memory.write<4>(base+36,base+24);
    s.memory.write<4>(base+size-12,size-32);s.memory.write<4>(base+size-8,size|3);s.memory.write<4>(base+size-4,base+24);
}
std::unique_ptr<State> base(const char* program,unsigned variation=0){
    auto s=std::make_unique<State>();load_program(*s,program);s->memory.map(fixture,0xc000);
    s->memory.copy_in(fixture,std::vector<std::uint8_t>(0xc000,0xa5));s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));
    s->registers.fill(0x5b);s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000+n);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);
    s->wr<1>(2304,variation%2?0xf9:0xa6);s->wr<1>(1024,variation%4);s->wr<1>(1027,(variation/2)%2);
    s->memory.clear(context,52);s->memory.write<4>(toc-0x3b74,context);s->memory.write<1>(toc-0x3510,1);
    s->memory.write<4>(toc-0x3b70,class_table);for(unsigned i=0;i<6;++i)s->memory.write<4>(class_table+i*4,classes[i]);
    raw_arena(*s,arena);s->allocated[arena]=4096;s->memory.write<4>(context,arena);s->budget=100000;return s;
}
std::unique_ptr<State> setup(const char* program,unsigned index,unsigned variation=0){
    auto s=base(program,variation);s->allocator_original=true;const auto entry=allocator_entries[index];
    switch(index){
    case 0:args(*s,entry,{second_arena,4096});break;
    case 1:args(*s,entry,{arena,96});break;
    case 2:{const auto b=invoke(*s,0x10774190,{arena,96});args(*s,entry,{arena,b});break;}
    case 3:args(*s,entry,{arena,block});break;
    case 4:args(*s,entry,{second_arena+24,96,second_arena,0,0});break;
    case 5:args(*s,entry,{block,96});break;
    case 6:case 7:{
        const auto b1=invoke(*s,0x10774190,{arena,96});const auto b2=invoke(*s,0x10774190,{arena,96});
        if(index==6){invoke(*s,0x10774270,{arena,b1});args(*s,entry,{b2,arena+4092});}
        else args(*s,entry,{b2,arena+4092});break;}
    case 8:raw_arena(*s,second_arena);args(*s,entry,{context,second_arena});break;
    case 9:args(*s,entry,{context,arena});break;
    case 10:args(*s,entry,{context,512});break;
    case 11:args(*s,entry,{context,256});break;
    case 12:args(*s,entry,{context,256,hint});break;
    case 13:{const auto p=invoke(*s,0x10774760,{context,256});args(*s,entry,{context,p});break;}
    case 14:args(*s,entry,{pool,pool,pool,3,slots,48*10});break;
    case 15:args(*s,entry,{context});break;
    case 16:args(*s,entry,{});s->memory.write<1>(toc-0x3510,0);break;
    case 17:args(*s,entry,{context,44});break;
    case 18:{const auto p=invoke(*s,0x10774e80,{context,44});args(*s,entry,{context,p,44});break;}
    case 19:args(*s,entry,{context,256});break;
    case 20:{const auto p=invoke(*s,0x10774e80,{context,44});args(*s,entry,{context,p});break;}
    case 21:{const auto p=invoke(*s,0x10774e80,{context,44});s->memory.copy_in(p,std::vector<std::uint8_t>(44,0x5a));args(*s,entry,{context,p,256});break;}
    case 22:args(*s,entry,{context,33});break;
    case 23:args(*s,entry,{33});break;
    case 24:{const auto p=invoke(*s,0x10775130,{44});args(*s,entry,{p});break;}
    case 25:{const auto p=invoke(*s,0x10775130,{44});args(*s,entry,{p,256});break;}
    case 26:args(*s,entry,{3,11});break;
    }
    reset_allocator_diagnostics(*s);s->executed=s->native_calls=0;s->budget=100000;return s;
}
// Import arguments, returns and bridge storage history are observed around
// the unchanged runtime dispatch, independently of final heap comparisons.
struct Observer:AllocatorBoundaryObserver{
    struct Import {std::string name;decltype(State::registers) registers;std::uint32_t pc,heap;
        std::uint64_t calls;std::int16_t error;decltype(State::allocated) allocated,available;
        std::vector<std::pair<std::uint32_t,std::uint64_t>> ranges;
        bool operator==(const Import&)const=default;};
    std::vector<Import> imports;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool)override{}
    void capture(const State&s,const char*name){imports.push_back({name,s.registers,s.pc,s.heap_end,s.native_calls,s.memory_error,s.allocated,s.available,s.memory.mapped_ranges()});}
    void import_enter(const State&s,const char*name)override{capture(s,name);}
    void import_leave(const State&s,const char*name)override{capture(s,name);}
};
unsigned cases=0,faults=0,negatives=0;
std::array<std::uint64_t,27>witnesses{},direct{};
using Execute=std::function<void(State&)>;
using Expect=std::function<void(const State&)>;
void compare_pair(State& original,State& modern,const std::string& label,bool fault=false,bool mixed=false,
                  const Execute& execute=[](State&s){run(s);}){
    original.allocator_original=true;
    Effects expected,actual;expected.state=&original;actual.state=&modern;Observer oi,mi;
    original.memory.date_observer=&expected;modern.memory.date_observer=&actual;original.allocator_observer=&oi;modern.allocator_observer=&mi;
    std::string a,b;try{execute(original);}catch(const std::exception&e){a=e.what();}try{execute(modern);}catch(const std::exception&e){b=e.what();}
    original.memory.date_observer=modern.memory.date_observer=nullptr;original.allocator_observer=modern.allocator_observer=nullptr;
    require(a==b,label+" fault mismatch original="+a+" semantic="+b);require(fault==!a.empty(),label+" fault expectation "+a+" PC="+hex_address(original.pc));
    if(fault)require(a.starts_with("Unmapped data access at "),label+" expected mapping fault; budget exhaustion is not acceptance");
    Image(original).compare(modern,label);
    if(expected.bytes!=actual.bytes){unsigned i=0;while(i<expected.log.size()&&i<actual.log.size()){const auto&x=expected.log[i];const auto&y=actual.log[i];if(x.kind!=y.kind||x.address!=y.address||x.size!=y.size||x.value!=y.value)break;++i;}throw std::runtime_error(label+" ordered effects event="+std::to_string(i));}
    require(oi.imports==mi.imports,label+" import arguments/returns/storage history");
    require(!original.memory_primitives_original&&!modern.memory_primitives_original&&!original.string_original&&!modern.string_original,"Semantic dependencies");
    require(!modern.allocator_dispatch_escapes,label+" unknown transfer");
    if(!mixed)require(modern.allocator_original_calls==std::array<std::uint64_t,27>{},label+" normal original selection");
    ++cases;if(fault)++faults;
    if(!fault&&!mixed)for(unsigned i=0;i<27;++i)witnesses[i]+=modern.allocator_calls[i];
}
void check(std::unique_ptr<State>a,const std::string& label,const Expect& expected={},bool fault=false,const Execute& execute=[](State&s){run(s);}){
    auto b=clone(*a);compare_pair(*a,*b,label,fault,false,execute);if(expected)expected(*b);
}

struct Negative final:Observer{
    std::uint32_t target;std::unique_ptr<Image> expected;bool mutated=false;unsigned parent_returns=0;
    Effects* effects=nullptr;std::vector<std::uint8_t> expected_effects;std::vector<Import> expected_imports;
    explicit Negative(std::uint32_t entry):target(entry){}
    void leave(State&s,std::uint32_t entry,bool original)override{
        if(entry!=target){if(mutated)++parent_returns;return;}
        if(original){expected=std::make_unique<Image>(s);expected_effects=effects->bytes;expected_imports=imports;return;}
        require(bool(expected),"Missing own reference boundary");
        require(effects->bytes==expected_effects&&imports==expected_imports,"Negative boundary effects/import prefix");
        expected->compare(s,"Unmutated allocator boundary "+hex_address(entry));s.wr<4>(12,s.r<4>(12)^1);mutated=true;
        expected->compare(s,"Own allocator boundary "+hex_address(entry));
    }
};
void negative(std::unique_ptr<State>a,std::uint32_t target){
    auto b=clone(*a);a->allocator_original=true;Negative observer(target);Effects original,modern;original.state=a.get();modern.state=b.get();
    observer.effects=&original;a->memory.date_observer=&original;a->allocator_observer=&observer;run(*a);
    observer.parent_returns=0;observer.imports.clear();observer.effects=&modern;b->memory.date_observer=&modern;b->allocator_observer=&observer;bool rejected=false;
    try{run(*b);}catch(const std::runtime_error&e){rejected=std::string(e.what()).starts_with("Own allocator boundary "+hex_address(target));}
    a->allocator_observer=b->allocator_observer=nullptr;a->memory.date_observer=b->memory.date_observer=nullptr;
    require(rejected&&observer.mutated,"Mutation did not reject at own boundary");
    require(observer.parent_returns==0,"Child mutation escaped into its parent return");++negatives;
}
std::unique_ptr<State> sparse(const State& initial,std::uint64_t omitted=UINT64_MAX){
    auto probe=clone(initial);probe->allocator_original=true;Effects effects;effects.state=probe.get();Observer imports;
    probe->memory.date_observer=&effects;probe->allocator_observer=&imports;run(*probe);probe->memory.date_observer=nullptr;probe->allocator_observer=nullptr;
    auto result=std::make_unique<State>();result->registers=initial.registers;result->unique=initial.unique;result->pc=initial.pc;result->budget=initial.budget;
    result->allocated=initial.allocated;result->available=initial.available;result->heap_end=initial.heap_end;result->memory_error=initial.memory_error;
    auto map=[&](std::uint32_t begin,std::uint64_t size){
        for(const auto&[base,length]:initial.memory.mapped_ranges()){
            const auto low=std::max(std::uint64_t(begin),std::uint64_t(base)),high=std::min(std::uint64_t(begin)+size,std::uint64_t(base)+length);
            if(low>=high)continue;
            if(low<=omitted&&omitted<high){if(omitted>low)result->memory.map(std::uint32_t(low),omitted-low);if(omitted+1<high)result->memory.map(std::uint32_t(omitted+1),high-omitted-1);}
            else result->memory.map(std::uint32_t(low),high-low);
        }
    };
    for(const auto&[address,length]:effects.accesses)map(address,length);
    for(const auto&i:imports.imports)if(i.name=="CarbonLib::BlockMoveData")map(std::uint32_t(read_be<4>(i.registers.data()+12)),read_be<4>(i.registers.data()+20));
    for(const auto&[address,length]:result->memory.mapped_ranges())result->memory.copy_in(address,initial.memory.copy_out(address,length));
    return result;
}
void occupied(State&s,std::uint32_t largest){
    s.memory.write<4>(arena+16,largest);s.memory.write<4>(arena+4092,0);
    s.memory.write<4>(arena+24,0xfe2);s.memory.write<4>(arena+4088,0x1007);
}
void ordinary_neighbors(const State&s,std::uint32_t p,std::uint32_t neighbor,unsigned bytes){
    require(p&&((p-8)&15)==8,"Ordinary payload alignment");
    require(s.memory.copy_out(neighbor,bytes)==std::vector<std::uint8_t>(bytes,0x7b),"Live neighbor damaged");
}
}
int main(int argc,char**argv){
    if(argc!=2)return 2;
    try{
        for(unsigned i=0;i<27;++i){auto s=setup(argv[1],i,i);check(std::move(s),"direct entry "+hex_address(allocator_entries[i]));++direct[i];}

        // Existing generated CFM vector dispatch reaches every destination hook.
        for(unsigned i=0;i<27;++i){
            auto s=setup(argv[1],i,3);s->memory.write<4>(fixture,allocator_entries[i]);s->memory.write<4>(fixture+4,toc);
            s->wr<4>(48,fixture);s->pc=0x10781320;check(std::move(s),"indirect entry "+std::to_string(i));
        }
        for(unsigned i=0;i<27;++i)negative(setup(argv[1],i),allocator_entries[i]);
        negative(setup(argv[1],0),0x107743e0); // arena child before initialization parent
        negative(setup(argv[1],17),0x107749b0); // pool child before allocation parent
        negative(setup(argv[1],25),0x10774f40); // reallocation child before global parent
        for(unsigned i=0;i<27;++i){
            auto initial=setup(argv[1],i);auto minimal=sparse(*initial);check(std::move(minimal),"minimal entry "+std::to_string(i));
            auto probe=clone(*initial);probe->allocator_original=true;Effects effects;effects.state=probe.get();probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
            std::vector<std::uint32_t> omitted;
            for(const auto&access:effects.log)if(access.kind==2||access.kind==4){
                const auto address=access.address+std::uint32_t(access.size)-1;
                const auto ranges=initial->memory.mapped_ranges();
                if(std::any_of(ranges.begin(),ranges.end(),[&](const auto&r){return r.first<=address&&address<std::uint64_t(r.first)+r.second;}))omitted.push_back(address);
            }
            require(!omitted.empty(),"Entry has no accessed fault bytes");
            for(auto index:{0u,unsigned(omitted.size()/2),unsigned(omitted.size()-1)})
                check(sparse(*initial,omitted[index]),"access-prefix fault "+std::to_string(i)+"/"+std::to_string(index),{},true);
        }
        check(setup(argv[1],22),"clear requested extent only",[](const State&s){const auto p=reg(s,3);require(s.memory.copy_out(p,33)==std::vector<std::uint8_t>(33,0)&&s.memory.copy_out(p+33,11)==std::vector<std::uint8_t>(11,0xa5),"Clear changed class slack");});
        // Fit and strict split boundary; a null head deliberately keeps its hint.
        for(auto requested:{96u,3968u,3984u,4064u,4080u}){
            auto s=base(argv[1]);args(*s,0x10774190,{arena,requested});
            check(std::move(s),"block fit "+std::to_string(requested),[=](const State&s){
                require(reg(s,3)==(requested<=4064?block:0),"Independent first fit address");
                const unsigned actual=requested+96<=4064?requested:4064;
                if(requested<=4064)require((s.memory.read<4>(block)&0xfffffff0)==actual,"Strict split size");
            });
        }
        {auto s=base(argv[1]);occupied(*s,0x7b0);args(*s,0x10774190,{arena,96});check(std::move(s),"null freelist stale hint",[](const State&s){require(!reg(s,3)&&s.memory.read<4>(arena+16)==0x7b0,"Null head revised original hint");});}
        // Release/coalesce combinations retain both outer occupied neighbors.
        for(unsigned mask=0;mask<4;++mask){
            auto s=base(argv[1]);check(std::move(s),"coalesce neighbors "+std::to_string(mask),{},false,[=](State&s){
                std::array<std::uint32_t,5> p{};for(auto&v:p)v=invoke(s,0x10774760,{context,100});
                s.memory.copy_in(p.front(),std::vector<std::uint8_t>(100,0x7b));s.memory.copy_in(p.back(),std::vector<std::uint8_t>(100,0x7b));
                if(mask&1)invoke(s,0x10774ee0,{context,p[1]});if(mask&2)invoke(s,0x10774ee0,{context,p[3]});
                invoke(s,0x10774ee0,{context,p[2]});
                const auto free=(mask&1?p[1]:p[2])-8;const auto length=112*(1+unsigned(bool(mask&1))+unsigned(bool(mask&2)));
                require((s.memory.read<4>(free)&0xfffffff0)==length&&!(s.memory.read<4>(free)&2)&&s.memory.read<4>(free+length-4)==length,"Independent merged header/boundary tag");
                const auto previous=std::uint32_t(s.memory.read<4>(free+8)),next=std::uint32_t(s.memory.read<4>(free+12));
                require(s.memory.read<4>(previous+12)==free&&s.memory.read<4>(next+8)==free,"Independent free-list links");
                require(s.memory.copy_out(p.front(),100)==std::vector<std::uint8_t>(100,0x7b)&&s.memory.copy_out(p.back(),100)==std::vector<std::uint8_t>(100,0x7b),"Coalescing corrupted occupied neighbors");
            });
        }
        for(unsigned nodes:{0u,1u,2u}){
            auto s=base(argv[1]);if(!nodes)s->memory.write<4>(context,0);
            if(nodes==2){raw_arena(*s,second_arena);s->memory.write<4>(arena,second_arena);s->memory.write<4>(arena+4,second_arena);s->memory.write<4>(second_arena,arena);s->memory.write<4>(second_arena+4,arena);}
            args(*s,0x10774630,{context,pool});check(std::move(s),"arena insert "+std::to_string(nodes));
            if(nodes){s=base(argv[1]);if(nodes==2){raw_arena(*s,second_arena);s->memory.write<4>(arena,second_arena);s->memory.write<4>(arena+4,second_arena);s->memory.write<4>(second_arena,arena);s->memory.write<4>(second_arena+4,arena);}args(*s,0x10774680,{context,arena});check(std::move(s),"arena remove "+std::to_string(nodes));}
        }
        for(unsigned cls=0;cls<6;++cls)for(unsigned count:{1u,2u,8u,9u,10u,17u}){
            auto s=base(argv[1],cls);args(*s,0x107749b0,{pool,pool,pool,cls,slots,(classes[cls]+4)*count+3});
            check(std::move(s),"pool slots "+std::to_string(cls)+"/"+std::to_string(count),[=](const State&s){
                require(s.memory.read<4>(pool+8)==classes[cls]&&s.memory.read<4>(pool+12)==slots&&s.memory.read<4>(pool+16)==0,"Pool header");
                for(unsigned n=0;n<count;++n){const auto slot=slots+n*(classes[cls]+4);require(s.memory.read<4>(slot)==pool,"Slot owner");require(s.memory.read<4>(slot+4)==(n+1<count?slot+classes[cls]+4:0),"Slot link/terminator");}
            });
        }
        {auto s=base(argv[1]);args(*s,0x107749b0,{pool,pool,pool,0,slots,0});check(std::move(s),"zero pool extent retains finite fault prefix",{},true);}
        for(auto size:{0u,1u,4u,5u,12u,13u,28u,29u,44u,45u,60u,61u,76u,77u,0xffffffb7u,0xffffffb8u,UINT32_MAX}){
            auto s=base(argv[1]);if(size>=0xffffffb7u)s->heap_end=0xfffffff0;
            args(*s,0x10774e80,{context,size});check(std::move(s),"allocation threshold "+std::to_string(size),[=](const State&s){
                if(!size||size>=0xffffffb7u)require(!reg(s,3),"Zero/failed/forbidden size returned payload");
                else{require(reg(s,3)!=0,"Allowed size failed");if(size<=76){const auto owner=std::uint32_t(s.memory.read<4>(reg(s,3)-4));const auto found=std::find_if(classes.begin(),classes.end(),[=](auto c){return c>=size;});require(s.memory.read<4>(owner+8)==*found,"Class boundary");}}
            });
        }
        for(auto previous:{10u,11u,40u,256u})for(auto candidate:{0u,9u,previous-1,previous,previous+1})
            require(next_pool_attempt(previous,candidate)==std::min(previous-1,candidate),"Independent retry cap");
        for(auto largest:{0u,0x100u,0x7b0u,0x1010u})for(auto size:{1u,12u,44u,76u}){
            auto s=base(argv[1]);occupied(*s,largest);
            check(std::move(s),"guarded replenishment "+std::to_string(largest)+"/"+std::to_string(size),{},false,[=](State&s){
                const auto first=invoke(s,0x10774e80,{context,size});require(first!=0,"Replenishment failed to progress");s.memory.copy_in(first,std::vector<std::uint8_t>(size,0x5a));
                const auto second=invoke(s,0x10774e80,{context,size});require(second&&(second+size<=first||first+size<=second),"Replenished slots overlap");s.memory.clear(second,size);
                require(s.memory.copy_out(first,size)==std::vector<std::uint8_t>(size,0x5a),"Replenishment damaged live payload");require(s.memory.read<4>(arena+24)==0xfe2,"Occupied block changed");
            });
        }
        {auto s=base(argv[1]);occupied(*s,0x7b0);s->heap_end=0xfffffff0;args(*s,0x10774e80,{context,44});check(std::move(s),"replenishment acquisition failure",[](const State&s){require(!reg(s,3)&&s.memory_error==-108,"Failure fabricated allocation");});}
        // A compact synthetic pool forces full/nonfull rotation and slot reuse.
        for(unsigned cls:{0u,3u,5u}){
            auto s=base(argv[1]);const auto row=context+4+cls*8;const auto stride=classes[cls]+4;
            s->memory.write<4>(row,pool);s->memory.write<4>(row+4,pool);s->memory.write<4>(pool,pool);s->memory.write<4>(pool+4,pool);
            s->memory.write<4>(pool+8,classes[cls]);s->memory.write<4>(pool+12,slots);s->memory.write<4>(pool+16,1);
            s->memory.write<4>(slots,pool);s->memory.write<4>(slots+4,slots+stride);s->memory.write<4>(slots+stride,pool);s->memory.write<4>(slots+stride+4,0);
            check(std::move(s),"full pool rotation/reuse "+std::to_string(cls),{},false,[=](State&s){
                const auto first=invoke(s,0x10774e80,{context,classes[cls]});const auto second=invoke(s,0x10774e80,{context,classes[cls]});
                require(first==slots+4&&second==slots+stride+4,"Pool pop order");invoke(s,0x10774ee0,{context,first});
                require(invoke(s,0x10774e80,{context,classes[cls]})==first,"Freed slot not reused");
            });
        }

        for(unsigned cls=0;cls<6;++cls){
            auto s=base(argv[1]);check(std::move(s),"complete pool refill/disposal "+std::to_string(cls),{},false,[=](State&s){
                std::vector<std::uint32_t> objects;const auto first=invoke(s,0x10774e80,{context,classes[cls]});objects.push_back(first);
                const auto owner=std::uint32_t(s.memory.read<4>(first-4));const auto raw_size=std::uint32_t(s.memory.read<4>(owner-8))&0xfffffff0;
                const auto count=(raw_size-8-28)/(classes[cls]+4);
                for(unsigned n=1;n<=count;++n)objects.push_back(invoke(s,0x10774e80,{context,classes[cls]}));
                require(s.memory.read<4>(objects.back()-4)!=owner,"Full pool failed to refill");
                for(unsigned n=0;n<objects.size();++n){require(objects[n]!=0,"Refilled pool allocation failure");for(unsigned m=0;m<n;++m)require(objects[n]+classes[cls]<=objects[m]||objects[m]+classes[cls]<=objects[n],"Pool slots overlap");}
                invoke(s,0x10774ee0,{context,objects[0]});require(invoke(s,0x10774e80,{context,classes[cls]})==objects[0],"Full pool failed to rejoin nonfull list");
                for(auto object:objects)invoke(s,0x10774ee0,{context,object});
                require(s.memory.read<4>(context+4+cls*8)==0&&s.memory.read<4>(context+8+cls*8)==0,"Empty pools retained class list");require(s.allocated.empty(),"Empty pool arena not disposed");
            });
        }
        // The four facades compose allocate/write/reallocate/free; string
        // duplication uses the same global allocation facade in production.
        {auto s=base(argv[1]);check(std::move(s),"global lifecycle",{},false,[](State&s){
            const auto p=invoke(s,0x10775130,{12});s.memory.copy_in(p,std::vector<std::uint8_t>(12,0x5a));
            const auto q=invoke(s,0x107751b0,{p,256});require(q&&s.memory.copy_out(q,12)==std::vector<std::uint8_t>(12,0x5a),"Global moving payload");
            invoke(s,0x10775170,{q});const auto clear=invoke(s,0x10775200,{3,11});require(clear&&s.memory.copy_out(clear,33)==std::vector<std::uint8_t>(33,0),"Global counted clear");invoke(s,0x10775170,{clear});
        });}
        {auto s=base(argv[1]);s->memory.copy_in(fixture,{'s','t','r','i','n','g',0});args(*s,0x107395f0,{fixture});check(std::move(s),"semantic string duplication",[](const State&s){require(s.memory.copy_out(reg(s,3),7)==std::vector<std::uint8_t>{'s','t','r','i','n','g',0},"Duplicated payload");require(s.allocator_calls[23]>0,"Duplication missed global allocator");});}
        for(auto count:{0u,1u,0x40000000u,0x40000001u,0x80000001u}){
            auto s=base(argv[1]);args(*s,0x10775200,{count,4});check(std::move(s),"word32 count product "+std::to_string(count),[=](const State&s){const auto size=count*4u;if(!size)require(!reg(s,3),"Wrap-to-zero allocated");else require(reg(s,3)&&s.memory.copy_out(reg(s,3),size)==std::vector<std::uint8_t>(size,0),"Low-word product clear");});
        }
        for(auto old_size:{12u,76u,77u,100u,256u})for(auto new_size:{0u,4u,60u,100u,400u,0xffffffb8u}){
            auto s=base(argv[1]);check(std::move(s),"reallocation "+std::to_string(old_size)+"/"+std::to_string(new_size),{},false,[=](State&s){
                const auto p=invoke(s,0x10774e80,{context,old_size});const auto neighbor=invoke(s,0x10774e80,{context,100});s.memory.copy_in(neighbor,std::vector<std::uint8_t>(100,0x7b));s.memory.copy_in(p,std::vector<std::uint8_t>(old_size,0x5a));
                const auto q=invoke(s,0x10774f40,{context,p,new_size});
                if(!new_size)require(!q,"Zero realloc returned pointer");
                else if(new_size>=0xffffffb8u)require(!q&&s.memory.copy_out(p,old_size)==std::vector<std::uint8_t>(old_size,0x5a),"Failed realloc lost old payload");
                else require(q&&s.memory.copy_out(q,std::min(old_size,new_size))==std::vector<std::uint8_t>(std::min(old_size,new_size),0x5a),"Reallocated surviving payload");
                require(s.memory.copy_out(neighbor,100)==std::vector<std::uint8_t>(100,0x7b),"Realloc damaged neighbor");
            });
        }
        for(auto new_size:{100u,200u,300u}){
            auto s=base(argv[1]);check(std::move(s),"in-place growth "+std::to_string(new_size),{},false,[=](State&s){
                const auto p=invoke(s,0x10774e80,{context,100});const auto next=invoke(s,0x10774e80,{context,256});const auto live=invoke(s,0x10774e80,{context,100});s.memory.copy_in(p,std::vector<std::uint8_t>(100,0x5a));s.memory.copy_in(live,std::vector<std::uint8_t>(100,0x7b));invoke(s,0x10774ee0,{context,next});
                const auto q=invoke(s,0x10774f40,{context,p,new_size});require(q==p&&s.memory.copy_out(q,100)==std::vector<std::uint8_t>(100,0x5a),"In-place growth moved/lost content");ordinary_neighbors(s,q,live,100);
            });
        }
        {auto s=base(argv[1]);check(std::move(s),"moving allocation failure preserves old object",{},false,[](State&s){
            const auto p=invoke(s,0x10774e80,{context,12});s.memory.copy_in(p,std::vector<std::uint8_t>(12,0x5a));s.heap_end=0xfffffff0;const auto q=invoke(s,0x10774f40,{context,p,65536});require(!q&&s.memory.copy_out(p,12)==std::vector<std::uint8_t>(12,0x5a),"Failed moving allocation lost object");
        });}
        for(unsigned initialized:{0u,1u}){
            auto s=base(argv[1]);s->wr<4>(8,relocated_toc);s->memory.write<4>(relocated_toc-0x3b74,context);s->memory.write<4>(relocated_toc-0x3b70,class_table);s->memory.write<1>(relocated_toc-0x3510,initialized);s->memory.copy_in(context,std::vector<std::uint8_t>(52,0x5a));args(*s,0x10774b10,{});
            check(std::move(s),"relocated lazy context "+std::to_string(initialized),[=](const State&s){require(reg(s,3)==context&&s.memory.read<1>(relocated_toc-0x3510)==1,"Global guest context/flag");require(s.memory.copy_out(context,52)==std::vector<std::uint8_t>(52,initialized?0x5a:0),"Lazy initialization count");});
        }
        {auto s=base(argv[1]);s->memory.write<4>(class_table,8);args(*s,0x10774e80,{context,5});check(std::move(s),"mutable live class table",[](const State&s){const auto owner=s.memory.read<4>(reg(s,3)-4);require(s.memory.read<4>(owner+8)==8,"Class sizes frozen");});}

        for(unsigned previous:{0u,4u})for(unsigned used:{0u,2u}){
            auto s=base(argv[1]);args(*s,0x107743e0,{pool,0,pool,previous,used});check(std::move(s),"aliased zero-length block header");
        }
        {auto s=base(argv[1]);args(*s,0x107749b0,{pool,pool,pool,0,pool+4,16});check(std::move(s),"pool header/slot alias");}
        {auto s=base(argv[1]);args(*s,0x10774840,{context,96,context});check(std::move(s),"hint aliases context head",[](const State&s){require(!reg(s,3)&&s.memory.read<4>(context)==0,"Live alias did not clear context before traversal");});}
        {auto s=base(argv[1]);s->memory.write<4>(arena+4092,0xffff0000);args(*s,0x10774190,{arena,96});check(std::move(s),"malformed free head",{},true);}
        {auto s=base(argv[1]);s->memory.write<4>(block,80);s->memory.write<4>(block+12,0xffff0000);args(*s,0x10774190,{arena,96});check(std::move(s),"malformed free next",{},true);}
        for(unsigned index:{6u,7u}){
            auto s=setup(argv[1],index);s->wr<4>(16,0xffff0000);check(std::move(s),"malformed coalesce head "+std::to_string(index),{},true);
        }
        {auto s=base(argv[1]);args(*s,0x10774f40,{context,0,44});check(std::move(s),"null realloc allocates",[](const State&s){require(reg(s,3)!=0,"Null-pointer allocation");});}
        {auto s=base(argv[1]);args(*s,0x10774ee0,{context,0});check(std::move(s),"null free no-op",[](const State&s){require(!s.native_calls,"Null free imported disposal");});}
        for(unsigned i:{0u,1u,3u,4u,5u,8u,9u,14u,15u}){
            auto s=setup(argv[1],i);s->wr<4>(4,fixture+0x9000);check(std::move(s),"relocated frame "+std::to_string(i));
        }
        for(unsigned i:{1u,11u,12u,19u,21u,23u,25u})for(bool parent_original:{false,true}){
            auto s=setup(argv[1],i);auto t=clone(*s);
            // Original parent / semantic children and the inverse, separately.
            if(parent_original)s->allocator_entry_original[i]=t->allocator_entry_original[i]=true;
            else for(unsigned child:{1u,4u,5u,17u,19u,21u})if(child!=i)s->allocator_entry_original[child]=t->allocator_entry_original[child]=true;
            compare_pair(*s,*t,"mixed dispatch "+std::to_string(i)+"/"+std::to_string(parent_original),false,true);
        }
        {auto s=setup(argv[1],23);s->allocator_original=true;s->allocator_entry_original.fill(true);s->allocator_calls.fill(7);s->allocator_original_calls.fill(9);s->allocator_dispatch_escapes=3;s->allocator_original_resume=allocator_entries[0];Observer observer;s->allocator_observer=&observer;s->string_calls[0]=42;
            reset_allocator_diagnostics(*s);require(!s->allocator_original&&!s->allocator_observer&&!s->allocator_original_resume&&!s->allocator_dispatch_escapes&&s->allocator_calls==std::array<std::uint64_t,27>{}&&s->allocator_original_calls==std::array<std::uint64_t,27>{}&&s->allocator_entry_original==std::array<bool,27>{}&&s->string_calls[0]==42,"Independent allocator reset");}
        for(unsigned i=0;i<27;++i)require(witnesses[i]>0,"Missing entry witness "+hex_address(allocator_entries[i]));
        std::cout<<cases<<" allocator comparisons, "<<faults<<" expected faults, "<<negatives<<" rejected negatives\n";
        for(unsigned i=0;i<27;++i)std::cout<<hex_address(allocator_entries[i])<<" modern="<<witnesses[i]<<" direct="<<direct[i]<<" original=0 escapes=0\n";
        return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
