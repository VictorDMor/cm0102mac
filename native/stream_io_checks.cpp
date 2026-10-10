#include "stream_io_adapter.hpp"
#include "stream_buffer_adapter.hpp"
#include "semantic_io_checks.hpp"
using namespace cm::semantic_io_checks;
namespace {
constexpr std::uint32_t root=fixture+0x1000, stream=fixture+0x2000, vectors=fixture+0x4000,
    buffer=fixture+0x6000, alternate_buffer=fixture+0x6200, source=fixture+0x7000, output=fixture+0x8000, alternate_toc=fixture+0xc000;
constexpr std::array<std::uint32_t,4> leaf{0x10775fd0,0x10777f90,0x10777fc0,0x10777f68};
std::uint32_t reg(const State&s,unsigned n){return std::uint32_t(s.r<4>(n*4));}
void args(State&s,std::uint32_t entry,std::initializer_list<std::uint32_t> values){unsigned n=3;for(auto v:values)s.wr<4>(n++*4,v);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+100000;}
void descriptor(State&s,std::uint32_t p,unsigned cls=1,unsigned state=2,unsigned mode=2,bool binary=true){
    s.memory.clear(p,84);s.memory.write<4>(p,p+84);s.memory.write<2>(p+4,0x1800|(mode<<9)|(cls<<6)|(binary?8:0));s.memory.write<1>(p+8,state<<5);
    s.memory.write<4>(p+28,7);s.memory.write<4>(p+32,buffer);s.memory.write<4>(p+36,16);s.memory.write<4>(p+40,buffer);s.memory.write<4>(p+44,state==1?16:4);s.memory.write<4>(p+52,4);
    for(unsigned offset:{60u,64u,68u,72u})s.memory.write<4>(p+offset,vectors);s.memory.write<4>(p+76,0x12345678);
}
std::unique_ptr<State> base(const char*program,unsigned variation=0){
    auto s=std::make_unique<State>();load_program(*s,program);s->memory.map(fixture,0x10000);s->memory.copy_in(fixture,std::vector<std::uint8_t>(0x10000,0xa5));s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));s->registers.fill(0x5b);s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000+n);s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);
    s->wr<1>(2304,variation%2?0xf9:0xa6);s->wr<1>(1024,variation%4);s->wr<1>(1027,(variation/2)%2);
    descriptor(*s,root,0);descriptor(*s,stream);s->memory.write<4>(root+248,stream);s->memory.write<4>(toc-0x5a68,root);
    s->memory.write<4>(toc-0x5a58,fixture+0x1800);s->memory.write<4>(toc-0x5a80,fixture+0x1810);s->memory.write<4>(toc-0x5a54,0x01020304);
    for(unsigned i=0;i<4;++i){s->memory.write<4>(vectors+i*8,leaf[i]);s->memory.write<4>(vectors+i*8+4,alternate_toc);}
    s->memory.copy_in(buffer,{'x',10,13,'y'});s->memory.copy_in(source,{'A',10,'B',13,'C','D'});s->budget=100000;return s;
}
using Execute=std::function<void(State&)>;
using Expect=std::function<void(const State&)>;
struct Observer:StreamIoBoundaryObserver{
    struct Import{std::string name;decltype(State::registers) registers;std::uint32_t pc,heap;std::uint64_t calls;std::int16_t error;decltype(State::allocated) allocated,available;std::vector<std::pair<std::uint32_t,std::uint64_t>> ranges;bool operator==(const Import&)const=default;};
    std::vector<Import> imports;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool)override{}
    void capture(const State&s,const char*n){imports.push_back({n,s.registers,s.pc,s.heap_end,s.native_calls,s.memory_error,s.allocated,s.available,s.memory.mapped_ranges()});}
    void import_enter(const State&s,const char*n)override{capture(s,n);}
    void import_leave(const State&s,const char*n)override{capture(s,n);}
};
struct CallbackPlan{
    std::uint32_t actual=3;unsigned eof_after=0;bool clobber=true,replace_descriptor=false;std::uint32_t flags=UINT32_MAX;
    void attach(Effects&e)const{
        e.on_read=[plan=*this,reads=0u](State&s,Effects&effects,std::uint32_t address,unsigned size)mutable{
            if(s.pc==0x10781320&&size==4&&address>=vectors&&address<vectors+32&&(address-vectors)%8==0){
                const auto link=std::uint32_t(s.r<4>(4128));effects.number(5,1);effects.number(reg(s,6),4);effects.number(reg(s,2),4);
                if(link==0x10775820||link==0x10775900){
                    const auto length=reg(s,5),payload=reg(s,4),requested=std::uint32_t(s.memory.read<4>(length));effects.number(requested,4);
                    if(link==0x10775820)++reads;const auto actual=plan.eof_after&&reads>plan.eof_after?0:std::min(requested,plan.actual);s.memory.write<4>(length,actual);
                    if(link==0x10775820){const std::vector<std::uint8_t> data{'x',10,13};std::vector<std::uint8_t> bytes(actual);for(unsigned n=0;n<actual;++n)bytes[n]=data[n%data.size()];s.memory.copy_in(payload,bytes);if(plan.eof_after&&reads>plan.eof_after)s.memory.write<4>(address,leaf[2]);}
                    if(plan.flags!=UINT32_MAX)s.memory.write<2>(stream+4,plan.flags);
                    if(plan.replace_descriptor){s.memory.write<4>(stream+32,alternate_buffer);s.memory.write<4>(stream+36,5);}
                }
            }
            if(plan.clobber&&size==4&&address==reg(s,1)+20&&(s.pc==0x10775820||s.pc==0x10775900||s.pc==0x10777ea4)){
                for(unsigned n=4;n<=12;++n)s.wr<4>(n*4,0xdead0000+n);s.wr<4>(0,0xdead0000);s.wr<1>(2304,0x87);s.wr<1>(1024,1);s.wr<1>(1027,1);
            }
        };
    }
};
unsigned cases=0,faults=0,negatives=0;std::array<std::uint64_t,9>witnesses{},direct{};
void compare_pair(State&a,State&b,const std::string&label,bool fault=false,bool mixed=false,const CallbackPlan&plan={},const Execute&execute=[](State&s){run(s);}){
    a.stream_io_original=true;Effects ea,eb;ea.state=&a;eb.state=&b;Observer oa,ob;plan.attach(ea);plan.attach(eb);a.memory.date_observer=&ea;b.memory.date_observer=&eb;a.stream_io_observer=&oa;b.stream_io_observer=&ob;
    std::string ax,bx;try{execute(a);}catch(const std::exception&e){ax=e.what();}try{execute(b);}catch(const std::exception&e){bx=e.what();}
    a.memory.date_observer=b.memory.date_observer=nullptr;a.stream_io_observer=b.stream_io_observer=nullptr;
    require(ax==bx,label+" error mismatch original="+ax+" semantic="+bx);require(fault==!ax.empty(),label+" fault expectation "+ax);if(fault)require(ax.starts_with("Unmapped data access at "),label+" budget exhaustion is not mapping-fault acceptance");
    Image(a).compare(b,label);require(ea.bytes==eb.bytes,label+" ordered guest effects");require(ea.callbacks==eb.callbacks,label+" generated CFM arguments/links");require(oa.imports==ob.imports,label+" import/native storage history");require(!b.stream_io_dispatch_escapes,label+" normal escape");if(!mixed)require(b.stream_io_original_calls==std::array<std::uint64_t,9>{},label+" original selection");
    require(!a.stream_buffer_original&&!b.stream_buffer_original&&a.stream_buffer_original_calls==std::array<std::uint64_t,13>{}&&b.stream_buffer_original_calls==std::array<std::uint64_t,13>{}&&!a.allocator_original&&!b.allocator_original&&!a.memory_primitives_original&&!b.memory_primitives_original,"Established semantic dependencies");
    ++cases;if(fault)++faults;if(!fault&&!mixed)for(unsigned i=0;i<9;++i)witnesses[i]+=b.stream_io_calls[i];
}
void check(std::unique_ptr<State>a,const std::string&label,const Expect&expected={},bool fault=false,const CallbackPlan&plan={},const Execute&execute=[](State&s){run(s);}){auto b=clone(*a);compare_pair(*a,*b,label,fault,false,plan,execute);if(expected)expected(*b);}
std::unique_ptr<State> setup(const char*program,unsigned i){
    auto s=base(program,i);switch(i){
    case 0:s->memory.write<4>(stream+44,0);args(*s,stream_io_entries[i],{stream});break;
    case 1:args(*s,stream_io_entries[i],{stream});break;
    case 2:args(*s,stream_io_entries[i],{output,8,stream});break;
    case 3:args(*s,stream_io_entries[i],{0x1a5,stream});break;
    case 4:case 5:args(*s,stream_io_entries[i],{output,2,3,stream});break;
    case 6:case 7:descriptor(*s,stream,1,1);args(*s,stream_io_entries[i],{source,2,3,stream});break;
    case 8:args(*s,stream_io_entries[i],{stream,1});break;
    }return s;
}
struct Negative:Observer{
    struct Boundary{Image image;std::vector<std::uint8_t> effects;std::vector<Import> imports;};
    std::uint32_t target;std::vector<Boundary> expected;bool mutated=false;unsigned parent_returns=0;Effects*effects=nullptr;
    explicit Negative(std::uint32_t entry):target(entry){}
    void leave(State&s,std::uint32_t entry,bool original)override{
        if(entry!=target){if(mutated)++parent_returns;return;}if(original){expected.push_back({Image(s),effects->bytes,imports});return;}
        require(!expected.empty(),"Missing negative reference boundary");expected.front().image.compare(s,"Unmutated stream I/O boundary");require(expected.front().effects==effects->bytes&&expected.front().imports==imports,"Negative effects/import prefix");s.wr<4>(12,s.r<4>(12)^1);mutated=true;expected.front().image.compare(s,"Own stream I/O boundary "+hex_address(entry));
    }
};
void negative(std::unique_ptr<State>a,std::uint32_t target){
    auto b=clone(*a);a->stream_io_original=true;Negative observer(target);Effects ea,eb;ea.state=a.get();eb.state=b.get();CallbackPlan{}.attach(ea);CallbackPlan{}.attach(eb);observer.effects=&ea;a->memory.date_observer=&ea;a->stream_io_observer=&observer;run(*a);observer.imports.clear();observer.effects=&eb;b->memory.date_observer=&eb;b->stream_io_observer=&observer;bool rejected=false;
    try{run(*b);}catch(const std::runtime_error&e){rejected=std::string(e.what()).starts_with("Own stream I/O boundary "+hex_address(target));}a->memory.date_observer=b->memory.date_observer=nullptr;a->stream_io_observer=b->stream_io_observer=nullptr;require(rejected&&observer.mutated&&!observer.parent_returns,"Negative escaped its own/child boundary");++negatives;
}
std::unique_ptr<State> sparse(const State&initial,std::uint64_t omitted=UINT64_MAX){
    auto probe=clone(initial);probe->stream_io_original=true;Effects effects;effects.state=probe.get();Observer imports;CallbackPlan{}.attach(effects);probe->memory.date_observer=&effects;probe->stream_io_observer=&imports;run(*probe);probe->memory.date_observer=nullptr;probe->stream_io_observer=nullptr;
    auto result=std::make_unique<State>();result->registers=initial.registers;result->unique=initial.unique;result->pc=initial.pc;result->budget=initial.budget;result->allocated=initial.allocated;result->available=initial.available;result->heap_end=initial.heap_end;result->memory_error=initial.memory_error;
    auto map=[&](std::uint32_t begin,std::uint64_t length){for(const auto&[address,size]:initial.memory.mapped_ranges()){
        const auto low=std::max(std::uint64_t(begin),std::uint64_t(address)),high=std::min(std::uint64_t(begin)+length,std::uint64_t(address)+size);if(low>=high)continue;
        if(low<=omitted&&omitted<high){if(low<omitted)result->memory.map(std::uint32_t(low),omitted-low);if(omitted+1<high)result->memory.map(std::uint32_t(omitted+1),high-omitted-1);}else result->memory.map(std::uint32_t(low),high-low);
    }};
    for(const auto&[address,size]:effects.accesses)map(address,size);
    // Carbon copies snapshot bulk sources; they produce no scalar read events.
    for(const auto&i:imports.imports)if(i.name=="CarbonLib::BlockMoveData")map(std::uint32_t(read_be<4>(i.registers.data()+12)),read_be<4>(i.registers.data()+20));
    for(const auto&[address,size]:result->memory.mapped_ranges())result->memory.copy_in(address,initial.memory.copy_out(address,size));return result;
}
}
int main(int argc,char**argv){
    if(argc!=2)return 2;try{
        for(unsigned i=0;i<9;++i){check(setup(argv[1],i),"direct entry "+hex_address(stream_io_entries[i]));++direct[i];}
        for(unsigned i=0;i<9;++i){auto s=setup(argv[1],i);s->memory.write<4>(fixture,stream_io_entries[i]);s->memory.write<4>(fixture+4,toc);s->wr<4>(48,fixture);s->pc=0x10781320;check(std::move(s),"generated indirect entry "+std::to_string(i));}
        for(unsigned i=0;i<9;++i)negative(setup(argv[1],i),stream_io_entries[i]);negative(setup(argv[1],1),0x1077fa50);
        {auto s=setup(argv[1],1);s->memory.write<4>(stream+44,0);negative(std::move(s),0x10775ad0);}
        negative(setup(argv[1],4),0x10776000);negative(setup(argv[1],6),0x10776380);
        for(unsigned i=0;i<9;++i){auto s=setup(argv[1],i);check(sparse(*s),"minimal mapping "+std::to_string(i));auto probe=clone(*s);probe->stream_io_original=true;Effects e;e.state=probe.get();CallbackPlan{}.attach(e);probe->memory.date_observer=&e;run(*probe);probe->memory.date_observer=nullptr;std::vector<std::uint32_t> bytes;const auto ranges=s->memory.mapped_ranges();
            for(const auto&x:e.log)if(x.kind==2||x.kind==4){const auto p=x.address+std::uint32_t(x.size)-1;if(std::any_of(ranges.begin(),ranges.end(),[=](const auto&r){return r.first<=p&&p<std::uint64_t(r.first)+r.second;}))bytes.push_back(p);}
            require(!bytes.empty(),"Entry has no fault bytes");for(auto n:{0u,unsigned(bytes.size()/2),unsigned(bytes.size()-1)})check(sparse(*s,bytes[n]),"access-prefix fault "+std::to_string(i)+"/"+std::to_string(n),{},true);
        }
        for(unsigned mode=0;mode<4;++mode)for(unsigned request:{0u,1u,UINT32_MAX,0x80000000u}){
            auto s=base(argv[1]);s->memory.write<1>(stream+5,0x48|(mode<<4));args(*s,0x1077fa50,{stream,request});check(std::move(s),"orientation "+std::to_string(mode)+"/"+std::to_string(request),[=](const State&s){const auto expected=mode==1?UINT32_MAX:mode==2?1:mode==3?stream:request;require(reg(s,3)==expected,"Original orientation result");if(!mode&&request)require(((s.memory.read<1>(stream+5)>>4)&3)==(std::bit_cast<std::int32_t>(request)<0?1:2),"Orientation becomes sticky");});
        }
        {auto s=base(argv[1]);args(*s,0x1077fa50,{0,1});check(std::move(s),"null orientation",[](const State&s){require(!reg(s,3),"Null orientation");});}
        for(unsigned state:{0u,1u,2u,3u,4u,7u}){
            auto s=base(argv[1]);descriptor(*s,stream,1,state);s->memory.write<4>(stream+44,0);s->memory.write<1>(stream+16+state,0x91);args(*s,0x10775ad0,{stream});check(std::move(s),"slow byte state "+std::to_string(state),[=](const State&s){require(reg(s,3)==(state==1?UINT32_MAX:state>=3?0x91:'x'),"Slow byte/unget value");if(state>=3)require((s.memory.read<1>(stream+8)>>5)==state-1,"Pop unget state");});
        }
        for(unsigned state:{0u,1u,2u,3u,4u,7u})for(unsigned value:{0u,0x1a5u,UINT32_MAX}){
            auto s=base(argv[1]);descriptor(*s,stream,1,state);s->memory.write<1>(stream+12,1);args(*s,0x10775d80,{value,stream});check(std::move(s),"pushback state/value "+std::to_string(state)+"/"+std::to_string(value),[=](const State&s){const bool rejected=state==1||state==4||value==UINT32_MAX;require(reg(s,3)==(rejected?UINT32_MAX:value&255),"Pushback rejection/truncation");if(!rejected)require(!s.memory.read<1>(stream+12),"Pushback clears EOF");});
        }
        {auto s=base(argv[1]);check(std::move(s),"pushback then slow read",{},false,{},[](State&s){args(s,0x10775d80,{0x1a5,stream});run(s);args(s,0x10775ad0,{stream});run(s);require(reg(s,3)==0xa5&&s.memory.read<4>(stream+44)==4&&(s.memory.read<1>(stream+8)>>5)==2,"Pushback restores saved buffer count");});}
        for(unsigned bound:{0u,1u,2u,8u,0x80000000u,UINT32_MAX}){
            auto s=base(argv[1]);args(*s,0x10775c80,{output,bound,stream});check(std::move(s),"line bound "+std::to_string(bound),[=](const State&s){if(!bound||bound==UINT32_MAX)require(!reg(s,3)&&s.memory.read<1>(output)==0xa5,"Nonpositive line extent");else require(reg(s,3)==output&&s.memory.read<1>(output+(bound==1?0:bound==2?1:2))==0,"Line terminator");});
        }
        for(unsigned status:{1u,2u}){
            auto s=base(argv[1]);s->memory.copy_in(buffer,{'A','B'});s->memory.write<4>(stream+44,2);s->memory.write<4>(stream+64,vectors+status*8);args(*s,0x10775c80,{output,8,stream});check(std::move(s),"line partial error/EOF "+std::to_string(status),[=](const State&s){require(s.memory.copy_out(output,2)==std::vector<std::uint8_t>{'A','B'},"Partial line prefix");require(reg(s,3)==(status==2?output:0)&&s.memory.read<1>(output+2)==(status==2?0:0xa5),"EOF terminates partial line; error does not");},false,CallbackPlan{.actual=0});
        }
        for(unsigned i:{4u,5u,6u,7u})for(unsigned size:{0u,1u,2u})for(unsigned amount:{0u,3u}){
            auto s=setup(argv[1],i);s->wr<4>(16,size);s->wr<4>(20,amount);check(std::move(s),"item size/count "+std::to_string(i)+"/"+std::to_string(size)+"/"+std::to_string(amount),[=](const State&s){require(reg(s,3)==(size&&amount?amount:0),"Zero/full item result");});
        }
        for(unsigned i:{5u,7u}){
            auto s=setup(argv[1],i);s->wr<4>(16,0x10000000);s->wr<4>(20,16);check(std::move(s),"word32 product wraps to zero "+std::to_string(i),[](const State&s){require(!reg(s,3)&&((s.memory.read<1>(stream+5)>>4)&3)==1,"Wrapped zero product still orients stream");});
        }
        for(unsigned i:{5u,7u})for(bool allowed:{false,true}){
            auto s=base(argv[1]);descriptor(*s,stream,1,0);if(!allowed)s->memory.write<1>(stream+4,i==5?0x14:0x0c);args(*s,stream_io_entries[i],{i==5?output:source,2,3,stream});
            check(std::move(s),"idle access transition "+std::to_string(i)+"/"+std::to_string(allowed),[=](const State&s){require(reg(s,3)==(allowed?3:0)&&s.memory.read<1>(stream+13)==unsigned(!allowed),"Idle permission and sticky rejection");});
        }
        for(unsigned i:{5u,7u})for(bool sticky:{false,true}){
            auto s=setup(argv[1],i);if(sticky)s->memory.write<1>(stream+13,1);else s->memory.write<2>(stream+4,0);check(std::move(s),"sticky/closed item rejection "+std::to_string(i)+"/"+std::to_string(sticky),[](const State&s){require(!reg(s,3),"Rejected item transfer");});
        }
        {auto s=setup(argv[1],7);s->memory.write<2>(stream+4,0x1c88);check(std::move(s),"class two output initialization",[](const State&s){require(reg(s,3)==3&&s.memory.read<4>(fixture+0x1810)==0x01020304,"Live standard-output initialization dependency");});}
        for(unsigned status:{0u,1u}){
            auto s=base(argv[1]);descriptor(*s,stream,1,0);s->memory.write<1>(stream+4,0x3c);s->memory.write<4>(stream+60,vectors+status*8);args(*s,0x10776380,{source,2,3,stream});check(std::move(s),"append seek before output transition "+std::to_string(status),[=](const State&s){require(reg(s,3)==(status?0:3),"Append seek result controls write");});
        }
        {auto s=setup(argv[1],0);check(std::move(s),"successful zero-byte refill becomes EOF",[](const State&s){require(reg(s,3)==UINT32_MAX&&s.memory.read<1>(stream+12)==1&&!s.memory.read<1>(stream+13),"Zero-byte success is EOF for a byte read");},false,CallbackPlan{.actual=0});}
        {auto s=setup(argv[1],5);s->memory.write<4>(stream+44,0);check(std::move(s),"buffered partial item before EOF",[](const State&s){require(reg(s,3)==1&&s.memory.read<1>(stream+12)==1,"Three bytes before EOF yield one complete two-byte item: result="+std::to_string(reg(s,3))+" eof="+std::to_string(s.memory.read<1>(stream+12)));},false,CallbackPlan{.eof_after=1});}
        for(unsigned i:{1u,2u,3u}){auto s=setup(argv[1],i);s->memory.write<1>(stream+5,0x68);check(std::move(s),"wide-oriented byte operation rejection "+std::to_string(i),[=](const State&s){require(reg(s,3)==(i==2?0:UINT32_MAX),"Wide stream rejects byte access");});}
        for(unsigned i:{5u,7u})for(unsigned mode:{0u,1u,2u})for(bool binary:{false,true})for(unsigned status=0;status<4;++status){
            auto s=base(argv[1]);descriptor(*s,stream,1,i==5?2:1,mode,binary);s->memory.write<4>(stream+44,i==5?0:16);s->memory.write<4>(stream+(i==5?64:68),vectors+status*8);if(i==7&&mode==2)s->memory.write<4>(stream+36,2);args(*s,stream_io_entries[i],{i==5?output:source,2,3,stream});
            check(std::move(s),"item mode/text/status "+std::to_string(i)+"/"+std::to_string(mode)+"/"+std::to_string(binary)+"/"+std::to_string(status),[=](const State&s){require(reg(s,2)==toc,"Item callback TOC restored");require(s.memory.read<4>(stream+32)==buffer,"Direct buffer restored");if(i==7)require(s.memory.read<1>(stream+13)==unsigned(status!=0),"Write failure is sticky");if(i==5&&status==1)require(s.memory.read<1>(stream+13)==1,"Read status one is an error");if(i==5&&status>=2)require(s.memory.read<1>(stream+12)==1,"Other read failures set EOF");});
        }
        for(unsigned i:{5u,7u})for(unsigned status:{0u,1u}){
            auto s=base(argv[1]);descriptor(*s,stream,1,i==5?2:1,0,true);s->memory.write<4>(stream+44,0);s->memory.write<4>(stream+(i==5?64:68),vectors+status*8);args(*s,stream_io_entries[i],{i==5?output:source,2,3,stream});check(std::move(s),"direct partial item credit "+std::to_string(i)+"/"+std::to_string(status),[=](const State&s){require(reg(s,3)==(i==5?0:1),"Reads floor partial items; writes credit final partial item");require(s.memory.read<4>(stream+32)==buffer&&s.memory.read<4>(stream+36)==16&&!s.memory.read<4>(stream+44),"Direct partial descriptor restored");},false,CallbackPlan{.actual=1});
        }
        for(unsigned state:{3u,4u}){
            auto s=base(argv[1]);descriptor(*s,stream,1,state);s->memory.write<1>(stream+5,0x68);s->memory.write<4>(stream+44,0);s->memory.write<2>(stream+22,0x1122);s->memory.write<2>(stream+24,0x3344);args(*s,0x10776000,{output,2,1,stream});check(std::move(s),"wide pushback item "+std::to_string(state),[=](const State&s){require(reg(s,3)==1&&s.memory.read<2>(output)==(state==3?0x1122:0x3344)&&(s.memory.read<1>(stream+8)>>5)==state-1,"Wide pending character uses two-byte slot");});
        }
        for(unsigned i:{5u,7u}){
            auto s=base(argv[1]);descriptor(*s,stream,1,i==5?2:1,0,true);s->memory.write<4>(stream+44,0);args(*s,stream_io_entries[i],{i==5?output:source,1,3,stream});check(std::move(s),"direct callback descriptor replacement "+std::to_string(i),[](const State&s){require(reg(s,3)==3&&s.memory.read<4>(stream+32)==buffer&&s.memory.read<4>(stream+36)==16,"Saved descriptor restored after live callback changes");},false,CallbackPlan{.replace_descriptor=true});
        }
        {auto s=setup(argv[1],7);s->memory.write<4>(stream+36,2);check(std::move(s),"buffer mode changes during callback",[](const State&s){require(reg(s,3)==3&&!s.memory.read<4>(stream+44),"Cached strategy and final live buffering mode");},false,CallbackPlan{.flags=0x1848});}
        {auto s=setup(argv[1],5);s->memory.copy_in(buffer,{'A','B','C','D'});s->wr<4>(12,buffer+1);s->wr<4>(16,1);s->wr<4>(20,4);check(std::move(s),"overlapping buffered read bulk snapshot",[](const State&s){require(s.memory.copy_out(buffer,5)==std::vector<std::uint8_t>{'A','A','B','C','D'},"Read bulk source snapshot");});}
        {auto s=setup(argv[1],7);s->memory.copy_in(buffer,{'A','B','C','D'});s->memory.write<4>(stream+40,buffer+1);s->wr<4>(12,buffer);s->wr<4>(16,1);s->wr<4>(20,4);check(std::move(s),"overlapping buffered write bulk snapshot",[](const State&s){require(s.memory.copy_out(buffer,5)==std::vector<std::uint8_t>{'A','A','B','C','D'},"Write bulk source snapshot");});}
        {auto s=setup(argv[1],5);s->wr<4>(12,stream+44);check(std::move(s),"destination aliases descriptor count");}
        for(unsigned i:{0u,2u,5u,7u}){
            auto s=setup(argv[1],i);s->wr<4>(4,fixture+0xe000);check(std::move(s),"relocated frame "+std::to_string(i));s=setup(argv[1],i);s->wr<4>(8,alternate_toc);for(unsigned offset:{0x5a68u,0x5a58u,0x5a80u,0x5a54u,0x7e64u})s->memory.write<4>(alternate_toc-offset,s->memory.read<4>(toc-offset));check(std::move(s),"relocated live TOC "+std::to_string(i));
        }
        for(unsigned i:{1u,2u,4u,5u,6u,7u})for(bool parent:{false,true}){auto s=setup(argv[1],i);auto t=clone(*s);if(parent)t->stream_io_entry_original[i]=true;else for(unsigned child:{0u,5u,7u,8u})if(child!=i)t->stream_io_entry_original[child]=true;compare_pair(*s,*t,"mixed dispatch "+std::to_string(i)+"/"+std::to_string(parent),false,true);}
        {auto s=base(argv[1]);s->stream_io_original=true;s->stream_io_entry_original.fill(true);s->stream_io_calls.fill(7);s->stream_io_original_calls.fill(9);s->stream_io_original_resume=stream_io_entries[0];s->stream_io_dispatch_escapes=3;Observer o;s->stream_io_observer=&o;s->stream_buffer_calls[0]=42;reset_stream_io_diagnostics(*s);require(!s->stream_io_original&&!s->stream_io_observer&&!s->stream_io_original_resume&&!s->stream_io_dispatch_escapes&&s->stream_io_calls==std::array<std::uint64_t,9>{}&&s->stream_io_original_calls==std::array<std::uint64_t,9>{}&&s->stream_io_entry_original==std::array<bool,9>{}&&s->stream_buffer_calls[0]==42,"Independent diagnostic reset");}
        for(unsigned i=0;i<9;++i)require(witnesses[i]>0,"Missing normal entry witness");std::cout<<cases<<" stream I/O comparisons, "<<faults<<" expected faults, "<<negatives<<" rejected negatives\n";
        for(unsigned i=0;i<9;++i)std::cout<<hex_address(stream_io_entries[i])<<" modern="<<witnesses[i]<<" direct="<<direct[i]<<" original=0 escapes=0\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
