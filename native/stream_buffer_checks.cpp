#include "stream_buffer_adapter.hpp"
#include "allocator_adapter.hpp"
#include "semantic_io_checks.hpp"
using namespace cm::semantic_io_checks;
namespace {
constexpr std::uint32_t root=fixture+0x1000, stream=fixture+0x2000, second=fixture+0x2200,
    vectors=fixture+0x4000, buffer=fixture+0x6000, count=fixture+0x7000, alternate_toc=fixture+0xc000;
constexpr std::array<std::uint32_t,4> leaf{0x10775fd0,0x10777f90,0x10777fc0,0x10777f68};
std::uint32_t reg(const State&s,unsigned n){return std::uint32_t(s.r<4>(n*4));}
void args(State&s,std::uint32_t entry,std::initializer_list<std::uint32_t> values){
    unsigned n=3;for(auto v:values)s.wr<4>(n++*4,v);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+100000;
}
void descriptor(State&s,std::uint32_t p,unsigned cls=2,unsigned state=0){
    s.memory.clear(p,84);s.memory.write<4>(p,p+84);s.memory.write<2>(p+4,cls<<6);s.memory.write<1>(p+8,state<<5);
    s.memory.write<4>(p+28,7);s.memory.write<4>(p+32,buffer);s.memory.write<4>(p+36,16);s.memory.write<4>(p+40,buffer+4);
    for(unsigned offset:{60u,64u,68u,72u})s.memory.write<4>(p+offset,vectors);
    s.memory.write<4>(p+76,0x12345678);
}
std::unique_ptr<State> base(const char*program,unsigned variation=0){
    auto s=std::make_unique<State>();load_program(*s,program);s->memory.map(fixture,0x10000);s->memory.copy_in(fixture,std::vector<std::uint8_t>(0x10000,0xa5));
    s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));s->registers.fill(0x5b);s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000+n);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);
    s->wr<1>(2304,variation%2?0xf9:0xa6);s->wr<1>(1024,variation%4);s->wr<1>(1027,(variation/2)%2);
    descriptor(*s,root);descriptor(*s,stream);descriptor(*s,second);s->memory.write<4>(root+248,stream);
    s->memory.write<4>(toc-0x5a68,root);s->wr<4>(120,root);
    for(unsigned i=0;i<4;++i){s->memory.write<4>(vectors+i*8,leaf[i]);s->memory.write<4>(vectors+i*8+4,alternate_toc);}
    for(unsigned i=0;i<4;++i)s->memory.write<4>(toc-0x5a6c-i*4,vectors);
    s->memory.copy_in(buffer,{'x',13,10,0x80});s->memory.write<4>(count,4);s->budget=100000;return s;
}
using Execute=std::function<void(State&)>;
using Expect=std::function<void(const State&)>;
struct Observer:StreamBufferBoundaryObserver{
    struct Import{std::string name;decltype(State::registers) registers;std::uint32_t pc,heap;std::uint64_t calls;
        std::int16_t error;decltype(State::allocated) allocated,available;std::vector<std::pair<std::uint32_t,std::uint64_t>> ranges;bool operator==(const Import&)const=default;};
    std::vector<Import> imports;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool)override{}
    void capture(const State&s,const char*name){imports.push_back({name,s.registers,s.pc,s.heap_end,s.native_calls,s.memory_error,s.allocated,s.available,s.memory.mapped_ranges()});}
    void import_enter(const State&s,const char*n)override{capture(s,n);}
    void import_leave(const State&s,const char*n)override{capture(s,n);}
};
struct CallbackPlan{
    std::uint32_t actual=3;bool mutate=true,clobber=true;
    void attach(Effects&e)const{
        e.on_read=[plan=*this](State&s,Effects&effects,std::uint32_t address,unsigned size){
            if(s.pc==0x10781320&&size==4&&address>=vectors&&address<vectors+32&&(address-vectors)%8==0){
                const auto link=std::uint32_t(s.r<4>(4128));
                // Supplement the common callback capture with refcon and TOC,
                // which a callback may clobber before final-state comparison.
                effects.number(5,1);effects.number(reg(s,6),4);effects.number(reg(s,2),4);
                if(link==0x10775820||link==0x10775900)effects.number(s.memory.read<4>(reg(s,5)),4);
                if(plan.mutate&&(link==0x10775820||link==0x10775900)){
                    const auto length=reg(s,5),payload=reg(s,4);s.memory.write<4>(length,plan.actual);
                    if(link==0x10775820){std::vector<std::uint8_t> bytes{'x',10,13};bytes.resize(std::min(plan.actual,3u));s.memory.copy_in(payload,bytes);}
                }
            }
            if(plan.clobber&&size==4&&address==reg(s,1)+20&&(s.pc==0x10775820||s.pc==0x10775900||s.pc==0x10776740)){
                for(unsigned n=4;n<=12;++n)s.wr<4>(n*4,0xdead0000+n);
                s.wr<4>(0,0xdead0000);s.wr<1>(2304,0x87);s.wr<1>(1024,0x1);s.wr<1>(1027,0x1);
            }
        };
    }
};
unsigned cases=0,faults=0,negatives=0;
std::array<std::uint64_t,13>witnesses{},direct{};
void compare_pair(State&a,State&b,const std::string&label,bool fault=false,bool mixed=false,const CallbackPlan&plan={},const Execute&execute=[](State&s){run(s);}){
    a.stream_buffer_original=true;Effects ea,eb;ea.state=&a;eb.state=&b;Observer oa,ob;plan.attach(ea);plan.attach(eb);
    a.memory.date_observer=&ea;b.memory.date_observer=&eb;a.stream_buffer_observer=&oa;b.stream_buffer_observer=&ob;
    std::string ax,bx;try{execute(a);}catch(const std::exception&e){ax=e.what();}try{execute(b);}catch(const std::exception&e){bx=e.what();}
    a.memory.date_observer=b.memory.date_observer=nullptr;a.stream_buffer_observer=b.stream_buffer_observer=nullptr;
    require(ax==bx,label+" error mismatch original="+ax+" semantic="+bx);require(fault==!ax.empty(),label+" fault expectation "+ax);
    if(fault)require(ax.starts_with("Unmapped data access at "),label+" budget exhaustion is not mapping-fault acceptance");
    Image(a).compare(b,label);require(ea.bytes==eb.bytes,label+" ordered guest effects");require(ea.callbacks==eb.callbacks,label+" CFM arguments/links");require(oa.imports==ob.imports,label+" imports and native storage history");
    require(!b.stream_buffer_dispatch_escapes,label+" normal escape");if(!mixed)require(b.stream_buffer_original_calls==std::array<std::uint64_t,13>{},label+" original selection");
    require(!a.allocator_original&&!b.allocator_original&&!a.memory_primitives_original&&!b.memory_primitives_original,"Semantic dependencies");
    ++cases;if(fault)++faults;if(!fault&&!mixed)for(unsigned i=0;i<13;++i)witnesses[i]+=b.stream_buffer_calls[i];
}
void check(std::unique_ptr<State>a,const std::string&label,const Expect&expected={},bool fault=false,const CallbackPlan&plan={},const Execute&execute=[](State&s){run(s);}){
    auto b=clone(*a);compare_pair(*a,*b,label,fault,false,plan,execute);if(expected)expected(*b);
}
std::unique_ptr<State> setup(const char*program,unsigned i){
    auto s=base(program,i);
    switch(i){
    case 0:s->memory.write<2>(stream+4,0);args(*s,stream_buffer_entries[i],{});break;
    case 1:args(*s,stream_buffer_entries[i],{stream,0x00400035,buffer,16});break;
    case 2:{
        args(*s,0x10775130,{84});run(*s);const auto p=reg(*s,3);require(p!=0,"Dynamic stream fixture allocation");descriptor(*s,p,0);s->memory.write<1>(p+16,1);s->memory.write<4>(root+80,p);args(*s,stream_buffer_entries[i],{});break;}
    case 3:descriptor(*s,root,2,1);args(*s,stream_buffer_entries[i],{});break;
    case 4:descriptor(*s,root,2,1);s->memory.write<2>(root+4,0x280);args(*s,stream_buffer_entries[i],{});break;
    case 5:case 6:args(*s,stream_buffer_entries[i],{buffer,count});break;
    case 7:args(*s,stream_buffer_entries[i],{stream});s->memory.write<4>(stream+48,511);break;
    case 8:args(*s,stream_buffer_entries[i],{stream,count,0});break;
    case 9:args(*s,stream_buffer_entries[i],{stream,count});break;
    case 10:args(*s,stream_buffer_entries[i],{stream,buffer,2,512});break;
    case 11:case 12:descriptor(*s,stream,2,1);args(*s,stream_buffer_entries[i],{stream});break;
    }
    reset_stream_buffer_diagnostics(*s);reset_allocator_diagnostics(*s);s->executed=s->native_calls=0;s->budget=100000;return s;
}
struct Negative:Observer{
    std::uint32_t target;std::unique_ptr<Image> expected;bool mutated=false;unsigned parent_returns=0;Effects*effects=nullptr;
    std::vector<std::uint8_t> prefix;std::vector<Import> import_prefix;
    explicit Negative(std::uint32_t entry):target(entry){}
    void leave(State&s,std::uint32_t entry,bool original)override{
        if(entry!=target){if(mutated)++parent_returns;return;}
        if(original){expected=std::make_unique<Image>(s);prefix=effects->bytes;import_prefix=imports;return;}
        require(bool(expected),"Missing negative reference boundary");expected->compare(s,"Unmutated stream boundary");
        require(prefix==effects->bytes&&import_prefix==imports,"Negative effects/import prefix");s.wr<4>(12,s.r<4>(12)^1);mutated=true;expected->compare(s,"Own stream boundary "+hex_address(entry));
    }
};
void negative(std::unique_ptr<State>a,std::uint32_t target){
    auto b=clone(*a);a->stream_buffer_original=true;Negative observer(target);Effects ea,eb;ea.state=a.get();eb.state=b.get();CallbackPlan plan;plan.attach(ea);plan.attach(eb);
    observer.effects=&ea;a->memory.date_observer=&ea;a->stream_buffer_observer=&observer;run(*a);
    observer.parent_returns=0;observer.imports.clear();observer.effects=&eb;b->memory.date_observer=&eb;b->stream_buffer_observer=&observer;bool rejected=false;
    try{run(*b);}catch(const std::runtime_error&e){rejected=std::string(e.what()).starts_with("Own stream boundary "+hex_address(target));}
    a->memory.date_observer=b->memory.date_observer=nullptr;a->stream_buffer_observer=b->stream_buffer_observer=nullptr;
    require(rejected&&observer.mutated&&!observer.parent_returns,"Negative escaped own/child boundary");++negatives;
}
std::unique_ptr<State> sparse(const State&initial,std::uint64_t omitted=UINT64_MAX){
    auto probe=clone(initial);probe->stream_buffer_original=true;Effects effects;effects.state=probe.get();CallbackPlan{}.attach(effects);probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
    auto result=std::make_unique<State>();result->registers=initial.registers;result->unique=initial.unique;result->pc=initial.pc;result->budget=initial.budget;
    result->allocated=initial.allocated;result->available=initial.available;result->heap_end=initial.heap_end;result->memory_error=initial.memory_error;
    for(const auto&[begin,length]:effects.accesses)for(const auto&[address,size]:initial.memory.mapped_ranges()){
        const auto low=std::max(std::uint64_t(begin),std::uint64_t(address)),high=std::min(std::uint64_t(begin)+length,std::uint64_t(address)+size);if(low>=high)continue;
        if(low<=omitted&&omitted<high){if(low<omitted)result->memory.map(std::uint32_t(low),omitted-low);if(omitted+1<high)result->memory.map(std::uint32_t(omitted+1),high-omitted-1);}
        else result->memory.map(std::uint32_t(low),high-low);
    }
    for(const auto&[address,size]:result->memory.mapped_ranges())result->memory.copy_in(address,initial.memory.copy_out(address,size));return result;
}
}
int main(int argc,char**argv){
    if(argc!=2)return 2;
    try{
        for(unsigned i=0;i<13;++i){check(setup(argv[1],i),"direct entry "+hex_address(stream_buffer_entries[i]));++direct[i];}
        for(unsigned i=0;i<13;++i){auto s=setup(argv[1],i);s->memory.write<4>(fixture,stream_buffer_entries[i]);s->memory.write<4>(fixture+4,toc);s->wr<4>(48,fixture);s->pc=0x10781320;check(std::move(s),"generated indirect entry "+std::to_string(i));}
        for(unsigned i=0;i<13;++i)negative(setup(argv[1],i),stream_buffer_entries[i]);
        negative(setup(argv[1],1),0x10775960);negative(setup(argv[1],8),0x10775790);negative(setup(argv[1],9),0x107756f0);negative(setup(argv[1],11),0x107767b0);
        for(unsigned i=0;i<13;++i){
            auto s=setup(argv[1],i);check(sparse(*s),"minimal mapping "+std::to_string(i));auto probe=clone(*s);probe->stream_buffer_original=true;Effects e;e.state=probe.get();CallbackPlan{}.attach(e);probe->memory.date_observer=&e;run(*probe);probe->memory.date_observer=nullptr;
            std::vector<std::uint32_t> bytes;const auto ranges=s->memory.mapped_ranges();
            for(const auto&x:e.log)if(x.kind==2||x.kind==4){const auto p=x.address+std::uint32_t(x.size)-1;if(std::any_of(ranges.begin(),ranges.end(),[=](const auto&r){return r.first<=p&&p<std::uint64_t(r.first)+r.second;}))bytes.push_back(p);}
            require(!bytes.empty(),"Entry has no fault bytes");for(auto n:{0u,unsigned(bytes.size()/2),unsigned(bytes.size()-1)})check(sparse(*s,bytes[n]),"access-prefix fault "+std::to_string(i)+"/"+std::to_string(n),{},true);
        }
        for(unsigned i:{5u,6u})for(unsigned length:{0u,1u,2u,4u}){
            auto s=setup(argv[1],i);s->memory.write<4>(count,length);check(std::move(s),"newline length "+std::to_string(i)+"/"+std::to_string(length),[=](const State&s){
                auto expected=std::vector<std::uint8_t>{'x',13,10,0x80};for(unsigned n=0;n<length;++n){if(expected[n]==13)expected[n]=10;else if(expected[n]==10)expected[n]=13;}
                require(s.memory.copy_out(buffer,4)==expected&&reg(s,3)==buffer+length&&s.r<4>(4132)==0,"Captured newline extent and CTR");
            });
        }
        for(unsigned mode:{0u,1u,2u,3u,UINT32_MAX})for(unsigned capacity:{0u,1u,511u,512u}){
            auto s=base(argv[1]);args(*s,0x10775960,{stream,buffer,mode,capacity});check(std::move(s),"configuration mode/capacity "+std::to_string(mode)+"/"+std::to_string(capacity),[=](const State&s){
                require(reg(s,3)==(mode>2?UINT32_MAX:0),"Mode result");if(mode<=2){const auto expected=!mode||!capacity?stream+17:buffer;require(s.memory.read<4>(stream+32)==expected&&s.memory.read<4>(stream+40)==expected,"Buffer location");require(s.memory.read<4>(stream+36)==(!mode||!capacity?1:capacity),"Capacity");}
            });
        }
        for(unsigned cls:{0u,1u,2u,7u})for(unsigned state:{0u,1u,2u,3u}){
            auto s=base(argv[1]);descriptor(*s,stream,cls,state);args(*s,0x10775960,{stream,buffer,2,512});check(std::move(s),"configuration class/state "+std::to_string(cls)+"/"+std::to_string(state),[=](const State&s){require(reg(s,3)==(!cls||state?UINT32_MAX:0),"Class/state rejection");if(cls==1&&!state)require(s.memory.read<4>(stream+48)==511,"512-byte alignment mask");});
        }
        for(unsigned status=0;status<4;++status)for(bool binary:{false,true})for(unsigned force:{0u,1u})for(unsigned i:{8u,9u}){
            auto s=setup(argv[1],i);s->memory.write<4>(stream+(i==8?64:68),vectors+status*8);s->memory.write<2>(stream+4,0x80|(binary?8:0));if(i==8)s->wr<4>(20,force);
            check(std::move(s),"callback status/text/force "+std::to_string(i)+"/"+std::to_string(status)+"/"+std::to_string(binary)+"/"+std::to_string(force),[=](const State&s){
                const auto result=status==3?UINT32_MAX:status;require(reg(s,3)==result,"Callback result");require(reg(s,2)==toc,"Callback TOC restore");
                require(s.memory.read<4>(count)==(i==8&&status==2?0:3),"Callback amount/EOF");
                require(s.memory.read<4>(stream+28)==(status?7:10),"Success-only position advance");
                if(i==8&&!status)require(s.memory.copy_out(buffer,3)==std::vector<std::uint8_t>{'x',std::uint8_t(binary?10:13),std::uint8_t(binary?13:10)},"Read text translation");
                if(i==9)require(s.memory.copy_out(buffer,3)==std::vector<std::uint8_t>{'x',std::uint8_t(binary?13:10),std::uint8_t(binary?10:13)},"Write conversion prefix including errors");
            });
        }
        for(unsigned state:{0u,1u,2u,3u,7u})for(unsigned status:{0u,1u}){
            auto s=base(argv[1]);descriptor(*s,stream,2,state);s->memory.write<4>(stream+68,vectors+status*8);args(*s,0x107767b0,{stream});check(std::move(s),"flush state/status "+std::to_string(state)+"/"+std::to_string(status),[=](const State&s){require(reg(s,3)==(state==1&&status?UINT32_MAX:0),"Flush outcome");require(s.memory.read<1>(stream+13)==(state==1&&status?1:0),"Sticky write failure");require((s.memory.read<1>(stream+8)>>5)==(state==1&&status?1:0),"Flush state transition");});
        }
        for(unsigned cls:{0u,1u,2u})for(unsigned status=0;status<4;++status){
            auto s=base(argv[1]);descriptor(*s,stream,cls);s->memory.write<4>(stream+72,vectors+status*8);args(*s,0x107766f0,{stream});check(std::move(s),"close class/status "+std::to_string(cls)+"/"+std::to_string(status),[=](const State&s){require(reg(s,3)==(cls&&status?UINT32_MAX:0),"Close status");if(cls)require(!s.memory.read<4>(stream)&&!(s.memory.read<2>(stream+4)&0x1c0),"Close descriptor clearing");});
        }
        {auto s=base(argv[1]);args(*s,0x107766f0,{0});check(std::move(s),"null close",[](const State&s){require(reg(s,3)==UINT32_MAX,"Null close contract");});}
        {auto s=base(argv[1]);args(*s,0x107767b0,{0});check(std::move(s),"null flush all",[](const State&s){require(!reg(s,3),"Null flush facade");});}
        for(unsigned nodes:{0u,1u,2u}){
            auto s=base(argv[1]);s->memory.write<4>(root+248,nodes?stream:0);s->memory.write<2>(stream+4,nodes==2?0x80:0);s->memory.write<4>(stream+80,nodes==2?second:0);s->memory.write<2>(second+4,0);args(*s,0x10775240,{});
            check(std::move(s),"acquire nodes "+std::to_string(nodes),[=](const State&s){const auto p=reg(s,3);require(p!=0,"Stream allocation");if(nodes)require(p==(nodes==1?stream:second),"Reusable stream traversal");else require(s.memory.read<4>(root+80)==p&&s.memory.read<1>(p+16)==1&&s.memory.copy_out(p,16)==std::vector<std::uint8_t>(16,0),"Dynamic stream zeroing/linking");});
        }
        {auto s=base(argv[1]);s->memory.write<4>(root+248,0);s->heap_end=0xfffffff0;args(*s,0x10775240,{});check(std::move(s),"acquire failure",[](const State&s){require(!reg(s,3)&&s.memory_error==-108,"Acquisition failure");});}
        for(bool fail:{false,true}){
            auto s=base(argv[1]);if(fail)s->heap_end=0xfffffff0;args(*s,0x10775960,{stream,0,2,64});check(std::move(s),"owned buffer allocation "+std::to_string(fail),[=](const State&s){require(reg(s,3)==(fail?UINT32_MAX:0),"Owned buffer result");require(bool(s.memory.read<1>(stream+8)&0x10)==!fail,"Owned buffer flag");if(fail)require(s.memory.read<4>(stream+32)==stream+17,"Failure fallback descriptor retained");});
        }
        for(bool closing:{false,true}){
            auto s=base(argv[1]);check(std::move(s),"owned buffer release "+std::to_string(closing),{},false,{},[=](State&s){
                args(s,0x10775960,{stream,0,2,64});run(s);const auto owned=s.memory.read<4>(stream+32);require(owned&&bool(s.memory.read<1>(stream+8)&0x10),"Owned buffer setup");
                s.memory.copy_in(owned,std::vector<std::uint8_t>(64,0x6b));s.memory.copy_in(buffer,std::vector<std::uint8_t>(8,0x7d));
                if(closing)args(s,0x107766f0,{stream});else args(s,0x10775960,{stream,buffer,2,512});run(s);
                require(!reg(s,3)&&s.allocated.empty(),"Owned buffer storage not released");require(s.memory.copy_out(buffer,8)==std::vector<std::uint8_t>(8,0x7d),"Neighbor payload damaged");
                if(!closing)require(s.memory.read<4>(stream+32)==buffer&&!(s.memory.read<1>(stream+8)&0x10),"Replacement buffer ownership");
            });
        }
        {auto s=base(argv[1]);s->memory.write<2>(stream+4,0x80);s->memory.write<4>(stream+80,second);args(*s,0x10775240,{});check(std::move(s),"append after occupied dynamic chain",[](const State&s){const auto p=reg(s,3);require(p&&p!=stream&&p!=second&&s.memory.read<4>(second+80)==p&&s.memory.read<1>(p+16)==1,"Dynamic descriptor append");});}
        {auto s=base(argv[1]);check(std::move(s),"static and dynamic list teardown",{},false,{},[](State&s){
            args(s,0x10775130,{84});run(s);const auto p=reg(s,3);descriptor(s,p,0);s.memory.write<1>(p+16,1);s.memory.write<4>(p+80,second);s.memory.write<4>(root+80,p);
            args(s,0x107753b0,{});run(s);require(s.allocated.empty()&&!s.memory.read<4>(root+80),"Dynamic nodes not detached/released");require(((s.memory.read<2>(root+4)>>6)&7)==3&&((s.memory.read<2>(second+4)>>6)&7)==3,"Static descriptor retention");
        });}
        {auto s=base(argv[1]);descriptor(*s,root,2,1);descriptor(*s,stream,2,1);s->memory.write<4>(root+80,stream);s->memory.write<4>(root+68,vectors+8);args(*s,0x10775440,{});check(std::move(s),"flush-all failure still visits next stream",[](const State&s){require(reg(s,3)==UINT32_MAX&&s.memory.read<1>(root+13)==1&&!s.memory.read<1>(stream+13)&&(s.memory.read<1>(stream+8)>>5)==0,"Flush aggregate/continuation");});}
        for(unsigned cls:{0u,2u})for(bool line:{false,true})for(unsigned state:{0u,1u,2u}){
            auto s=base(argv[1]);descriptor(*s,root,cls,state);s->memory.write<2>(root+4,(cls<<6)|(line?0x200:0));args(*s,0x107754b0,{});
            check(std::move(s),"line-buffer selection "+std::to_string(cls)+"/"+std::to_string(line)+"/"+std::to_string(state),[=](const State&s){require(s.stream_buffer_calls[12]==unsigned(cls&&line&&state==1),"Line-buffer predicate called an ineligible stream");require(!reg(s,3),"Line-buffer aggregate success");});
        }
        for(unsigned status:{0u,1u}){
            auto s=base(argv[1]);descriptor(*s,stream,1,1);s->memory.write<4>(stream+56,40);s->memory.write<4>(stream+68,vectors+status*8);args(*s,0x107767b0,{stream});
            check(std::move(s),"file-class flush preserves tell position "+std::to_string(status),[=](const State&s){require(reg(s,3)==(status?UINT32_MAX:0)&&s.memory.read<4>(stream+28)==(status?7:44),"File flush uses original tell before drain");});
        }
        for(bool external:{false,true}){
            auto s=base(argv[1]);args(*s,0x107752d0,{stream,0x00400000,buffer,external?16u:0u});
            check(std::move(s),"initialize file-class live callbacks "+std::to_string(external),[=](const State&s){for(unsigned offset:{60u,64u,68u,72u})require(s.memory.read<4>(stream+offset)==vectors,"Default callback TOC slot");require(s.memory.read<4>(stream+32)==(external?buffer:stream+17)&&!s.memory.read<4>(stream+44)&&!s.memory.read<4>(stream+76),"Initialized external/inline descriptor");});
        }
        for(bool closed:{false,true}){
            auto s=base(argv[1]);descriptor(*s,stream,closed?0:2);if(!closed)s->memory.write<1>(stream+13,1);args(*s,0x107767b0,{stream});check(std::move(s),"flush closed/sticky error "+std::to_string(closed),[](const State&s){require(reg(s,3)==UINT32_MAX,"Invalid descriptor flush");});
        }
        for(unsigned i:{5u,6u}){auto s=setup(argv[1],i);s->memory.write<4>(count,13);s->wr<4>(12,count);check(std::move(s),"newline count/payload alias "+std::to_string(i),[](const State&s){require(s.memory.read<4>(count)==10&&reg(s,3)==count+13&&s.r<4>(4132)==0,"Aliased length must be captured before newline stores");});}
        {auto s=setup(argv[1],7);s->memory.write<4>(stream+48,UINT32_MAX);s->memory.write<4>(stream+28,UINT32_MAX);check(std::move(s),"prepare word32 subtraction wrap",[](const State&s){require(s.memory.read<4>(stream+44)==17,"Word32 aligned subtraction");});}
        {auto s=setup(argv[1],8);s->wr<4>(16,stream+28);check(std::move(s),"count aliases stream position",[](const State&s){require(s.memory.read<4>(stream+28)==6,"Out count written before position reread");});}
        {auto s=setup(argv[1],9);s->memory.write<4>(stream+40,buffer);check(std::move(s),"empty drain",[](const State&s){require(!reg(s,3)&&s.memory.read<4>(count)==4,"Empty drain skips out count");});}
        for(unsigned i:{1u,8u,9u,10u,11u,12u}){
            auto s=setup(argv[1],i);s->wr<4>(4,fixture+0xe000);check(std::move(s),"relocated frame "+std::to_string(i));
            s=setup(argv[1],i);s->wr<4>(8,alternate_toc);s->memory.write<4>(alternate_toc-0x5a68,root);for(unsigned n=0;n<4;++n)s->memory.write<4>(alternate_toc-0x5a6c-n*4,vectors);check(std::move(s),"relocated live TOC "+std::to_string(i));
        }
        for(unsigned i:{1u,8u,9u,11u,12u})for(bool parent:{false,true}){
            auto s=setup(argv[1],i);auto t=clone(*s);if(parent)t->stream_buffer_entry_original[i]=true;else for(unsigned child:{5u,6u,7u,9u,10u,12u})if(child!=i)t->stream_buffer_entry_original[child]=true;
            compare_pair(*s,*t,"mixed dispatch "+std::to_string(i)+"/"+std::to_string(parent),false,true);
        }
        {auto s=base(argv[1]);s->stream_buffer_original=true;s->stream_buffer_entry_original.fill(true);s->stream_buffer_calls.fill(7);s->stream_buffer_original_calls.fill(9);s->stream_buffer_original_resume=stream_buffer_entries[0];s->stream_buffer_dispatch_escapes=3;Observer o;s->stream_buffer_observer=&o;s->allocator_calls[0]=42;reset_stream_buffer_diagnostics(*s);
            require(!s->stream_buffer_original&&!s->stream_buffer_observer&&!s->stream_buffer_original_resume&&!s->stream_buffer_dispatch_escapes&&s->stream_buffer_calls==std::array<std::uint64_t,13>{}&&s->stream_buffer_original_calls==std::array<std::uint64_t,13>{}&&s->stream_buffer_entry_original==std::array<bool,13>{}&&s->allocator_calls[0]==42,"Independent diagnostic reset");}
        for(unsigned i=0;i<13;++i)require(witnesses[i]>0,"Missing normal entry witness");
        std::cout<<cases<<" stream buffer comparisons, "<<faults<<" expected faults, "<<negatives<<" rejected negatives\n";
        for(unsigned i=0;i<13;++i)std::cout<<hex_address(stream_buffer_entries[i])<<" modern="<<witnesses[i]<<" direct="<<direct[i]<<" original=0 escapes=0\n";
        return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
