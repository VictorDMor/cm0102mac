#include "club_shortlist_storage.hpp"
#include "semantic_io_checks.hpp"
#include <set>
namespace cm::aot {std::vector<std::uint32_t> club_shortlist_storage_expected_sites();bool club_shortlist_storage_reference(State&);void club_shortlist_storage_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace shortlists=cm::aot::club_shortlist_storage;
constexpr std::uint32_t object=fixture+0x1000,club_table=fixture+0x2000,clubs=fixture+0x4000,staff=fixture+0x8000,careers=fixture+0xa000,contract=fixture+0xe000,globals=fixture+0x10000,tables=fixture+0x14000,output=fixture+0x1a000,pool=fixture+0x1c000,arena=fixture+0x20000;
constexpr std::array<std::uint32_t,7> ends{0x103567a8,0x103568d4,0x10356e3c,0x103573b8,0x10358af0,0x10358b24,0x10358f14};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{shortlists::construct_shortlist_storage,shortlists::destroy_shortlist_storage,shortlists::reset_shortlist_storage,shortlists::sample_shortlist_range,shortlists::load_shortlist_storage,shortlists::initialize_auxiliary_record,shortlists::save_shortlist_storage};
struct Control {std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options {std::vector<Control> controls;unsigned interrupt=0,short_io=0,fail_allocation=0;std::uint32_t version=0x3e017,seed_return=12345,eligible=1,primary=contract,secondary=contract,clubs=2,header_count=2,legacy_count=2;bool partial=false,own_negative=false,child_negative=false,terminate=false,flip_eligibility=false,empty_club=false,mutate_after_date=false;};
struct Trace:SemanticGuestObserver {Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==8)s.wr<8>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(shortlists::entries.begin(),shortlists::entries.end(),root);require(i!=shortlists::entries.end(),"Known shortlist root "+hex_address(root));return unsigned(i-shortlists::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event {std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned allocations=0,io_calls=0,eligibility_calls=0,date_calls=0;std::size_t position=0;std::vector<std::uint8_t> input,written;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){
        const auto append=[&](unsigned size,std::uint32_t value){for(unsigned n=size;n;--n)input.push_back(std::uint8_t(value>>((n-1)*8)));};
        append(4,options.header_count);for(unsigned n=0;n<options.header_count;++n){append(4,n+10);append(4,n+20);}
        if(std::bit_cast<std::int32_t>(options.version)<0x3e016){append(4,options.legacy_count);for(unsigned n=0;n<options.legacy_count*24;++n)input.push_back(std::uint8_t(n));}
        for(unsigned n=0;n<(std::bit_cast<std::int32_t>(options.version)<=0x3e016?15:16);++n)append(4,1);
        append(4,2);
        for(unsigned bank=0;bank<2;++bank)for(unsigned n=0;n<(std::bit_cast<std::int32_t>(options.version)<0x3e017?15:16);++n){append(4,n);append(4,100+bank);append(4,200+n);}
        for(unsigned n=0;n<options.clubs;++n){append(4,options.empty_club?0:1);append(4,0x01000001);append(4,0x01000000);}
        if(!options.empty_club)for(unsigned n=0;n<options.clubs*(std::bit_cast<std::int32_t>(options.version)>0x3e016?40:25);++n)for(unsigned off=0;off<44;++off)input.push_back(std::uint8_t(1+n+off));
    }
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(club_shortlist_storage_reference(s),"Complete shortlist reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t at){std::vector<std::uint8_t> b;for(unsigned n=0;n<8192;++n){auto c=std::uint8_t(s.memory.read<1>(at+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated shortlist fixture text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const bool own=std::find(shortlists::entries.begin(),shortlists::entries.end(),target)!=shortlists::entries.end();
        constexpr std::array allowed{0x10775130u,0x10775170u,0x107735a0u,0x107156b0u,0x10715880u,0x10747a80u,0x10747cc0u,0x1077fc28u,0x10768300u,0x10775550u,0x10773320u,0x10778300u,0x10365d10u,0x10082f20u,0x10082f90u,0x1077b770u,0x10739890u,0x10768810u,0x1077b720u,0x10769b60u,0x1076f7d0u};
        require(own||std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known shortlist dependency "+hex_address(target));const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try{
            if(own){if(original)reference(s,stop);else modern_root(s,this,&trace);}
            else{
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;
                s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);
                if(target==0x10775130){++allocations;require(first<0x800,"Bounded shortlist fixture allocation");result=allocations==options.fail_allocation?0:arena+allocations*0x800;if(result)s.memory.copy_in(result,std::vector<std::uint8_t>(std::max(1u,first),0x6b));}
                if(target==0x10775170||target==0x107735a0)s.memory.write<4>(globals+0x608,first);
                if(target==0x107156b0||target==0x10715880){s.memory.copy_in(first,{0,1,7,205,3});result=first;if(target==0x107156b0&&++date_calls==1&&options.mutate_after_date){const auto previous=std::uint32_t(s.memory.read<4>(object+0xdc));s.memory.copy_in(fixture+0x3000,s.memory.copy_out(previous,24));s.memory.write<4>(object+0xdc,fixture+0x3000);}}
                if(target==0x10775550)result=options.version;
                if(target==0x1077b770)result=options.seed_return;
                if(target==0x10365d10){++eligibility_calls;result=options.flip_eligibility?(eligibility_calls>18*16):options.eligible;}
                if(target==0x10082f20)result=options.primary;
                if(target==0x10082f90)result=options.secondary;
                if(target==0x10778300)s.memory.clear(first,arg(5));
                if(target==0x1077fc28){s.memory.copy_in(second,s.memory.copy_out(first,arg(5)));s.memory.write<4>(arg(1)+0x14,arg(2));}
                if(target==0x10768300){require(arg(5)==8,"Shortlist sort item width");if(second==2){const auto a=s.memory.copy_out(first,8),b=s.memory.copy_out(first+8,8);s.memory.copy_in(first,b);s.memory.copy_in(first+8,a);}}
                if(target==0x10773320){require(arg(6)==44&&arg(7)==25,"Legacy shortlist record-array shape");const auto callback=std::uint32_t(s.memory.read<4>(second));require(callback==shortlists::initialize_auxiliary_record_entry,"Pinned original shortlist constructor vector");for(unsigned n=0;n<25;++n){s.wr<4>(12,first+n*44);s.wr<4>(4128,stop);s.pc=callback;if(original)reference(s,stop);else modern_root(s,this,&trace);}}
                if(target==0x10747a80||target==0x10747cc0){const auto count=arg(5),width=second*count;require(width<0x10000,"Bounded shortlist IO");++io_calls;const bool shorted=options.short_io==io_calls;const auto bytes=shorted?(options.partial?width/2:0):width;
                    if(target==0x10747a80){require(position+bytes<=input.size(),"Available shortlist stream bytes");s.memory.copy_in(first,std::vector<std::uint8_t>(input.begin()+std::ptrdiff_t(position),input.begin()+std::ptrdiff_t(position+bytes)));position+=bytes;}
                    else {auto b=s.memory.copy_out(first,bytes);written.insert(written.end(),b.begin(),b.end());}result=shorted?(count?count-1:1):count;
                }
                if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
                if(target==0x10768810)result=output+0x800;
                if(target==0x1077b720){const auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);result=first;}
                if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
                if(target==0x1076f7d0&&options.terminate)throw std::runtime_error("Fixture shortlist termination");
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-70.0-n));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture club-shortlist-storage dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned b=1280;b<1312;++b)s->wr<1>(b,0);s->memory.map(fixture,0x70000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=shortlists::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=500000;s->wr<4>(12,object);s->wr<4>(16,0);
    for(auto [slot,ptr]:{std::pair{-0x7b50,globals},std::pair{-0x7be8,globals+4},std::pair{-0x7b7c,globals+8},std::pair{-0x7b78,globals+12},std::pair{-0x7b90,globals+16},std::pair{-0x7b84,globals+20},std::pair{-0x7b88,globals+24},std::pair{-0x7b8c,pool},std::pair{-0x7bac,globals+28},std::pair{-0x7be4,globals+32},std::pair{-0x7be0,globals+36},std::pair{-0x7b98,globals+40},std::pair{-0x7b04,globals+44},std::pair{-0x7b74,globals+48},std::pair{-0x7ba8,output}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,2);s->memory.write<4>(globals+4,clubs);s->memory.write<4>(globals+8,globals+0x100);s->memory.write<4>(globals+16,12345);s->memory.write<4>(globals+20,pool);s->memory.write<4>(globals+24,pool+0x100);s->memory.write<1>(globals+28,1);s->memory.write<4>(globals+32,18);s->memory.write<4>(globals+36,staff);s->memory.write<4>(globals+40,careers);s->memory.write<4>(globals+44,10);s->memory.write<4>(globals+48,globals+0x200);
    s->memory.write<4>(object,arena);s->memory.write<4>(object+4,arena+0x100);s->memory.write<4>(object+8,2);s->memory.write<4>(object+12,2);s->memory.write<4>(object+16,2);s->memory.write<4>(object+20,2);s->memory.write<4>(object+0xd8,2);s->memory.write<4>(object+0xdc,club_table);
    for(unsigned k=0;k<32;++k){s->memory.write<4>(object+0x18+k*4,arena+0x200+k*0x40);for(unsigned b=0;b<24;++b)s->memory.write<1>(arena+0x200+k*0x40+b,k+b);}
    for(unsigned k=0;k<16;++k)s->memory.write<4>(object+0x98+k*4,1);
    for(unsigned k=0;k<2;++k){s->memory.write<4>(club_table+k*12,arena+0x1000+k*0x800);s->memory.write<2>(clubs+k*0x248+0x1ec,100);}
    for(unsigned k=0;k<18;++k){s->memory.write<4>(staff+k*0x70,k);s->memory.write<4>(staff+k*0x70+8,10000+k*100);s->memory.write<4>(staff+k*0x70+12,careers+(k%10)*0x128);}
    for(unsigned k=0;k<10;++k)s->memory.write<4>(careers+k*0x128+0x68,200+k);
    s->memory.write<4>(contract+4,1);s->memory.write<1>(contract+0x44,0xfe);for(unsigned k=0;k<256;++k)s->memory.write<2>(pool+k*2,(12345+k*1009)&65535);club_shortlist_storage_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});if(n==3)s->wr<4>(12,10);return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites,completed_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::vector<std::uint8_t> written;std::size_t position;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Club shortlist storage root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture shortlist termination"||expected=="Fixture club-shortlist-storage dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First club-shortlist-storage site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");require(old.written==modern.written&&old.position==modern.position,label+" stream effects");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();if(!rejected&&expected!="Native instruction budget exhausted")sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty()&&!rejected)completed_sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls,old.written,old.position};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed club-shortlist-storage boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n:std::set<unsigned>{1,2,unsigned(base.calls.size()/2),unsigned(base.calls.size())})if(n&&n<=base.calls.size()){auto o=options;o.interrupt=n;compare(input,o);}
}

Control gpr(std::uint32_t pc,unsigned reg,std::uint32_t value){return {pc,reg*4,value,4};}
void sampler_cases(){
    for(auto bound:{0u,1u,7u,65535u,65536u,0x7fffffffu,0x80000000u,0xffff0000u,0xffff0001u,0xffffffffu}){auto a=seed(3);a->wr<4>(12,bound);boundaries(*a);}
    constexpr std::array points{0x10356f58u,0x10357044u,0x10357130u};
    for(unsigned depth=1;depth<=3;++depth)for(auto value:{65536u,0xffff0000u}){auto a=seed(3);a->wr<4>(12,65536);Options o;for(unsigned n=0;n<depth;++n)o.controls.push_back(gpr(points[n],22-n,value));compare(*a,o);}
    for(unsigned depth=1;depth<=2;++depth){auto a=seed(3);a->wr<4>(12,65536);Options o;o.controls.push_back(gpr(points[0],22,depth==1?0x7fffffffu:65536u));if(depth==2)o.controls.push_back(gpr(points[1],21,0x7fffffffu));compare(*a,o);}
    {auto a=seed(3);a->wr<4>(12,65536);a->memory.write<4>(globals+24,pool);Options o;for(unsigned n=0;n<3;++n)o.controls.push_back(gpr(points[n],22-n,65536));boundaries(*a,o);}
    for(auto bound:{10u,65536u,0xffff0000u})for(auto reset:{0u,65535u,65536u,0xffffffffu}){auto a=seed(3);a->wr<4>(12,bound);a->memory.write<4>(globals+20,pool+0x100);Options o;o.seed_return=reset;boundaries(*a,o);}
    for(auto cursor:{pool-2,pool,pool+0xfe,pool+0x100,pool+0x102}){auto a=seed(3);a->memory.write<4>(globals+20,cursor);boundaries(*a);}
    for(auto bound:{1u,7u,65535u,0xffff0001u,0xfffffff9u})for(auto sample:{0u,1u,65535u})for(auto added:{0u,1u,65535u,0x7fffffffu,0x80000001u,0xffffffffu}){auto a=seed(3);a->wr<4>(12,bound);a->memory.write<2>(pool+2,sample);a->memory.write<4>(globals+16,added);const auto result=compare(*a);const auto sum=std::bit_cast<std::int32_t>(sample+added);const auto limit=std::bit_cast<std::int32_t>(bound);const auto remainder=std::int64_t(sum)%limit;const auto expected=std::uint32_t(limit>0?remainder:-remainder);require(result.state->r<4>(12)==expected,"Independent narrow signed range remainder");}
    for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x3ff0000000000000),UINT64_C(0x7fefffffffffffff),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000001234)}){auto a=seed(3);a->wr<4>(12,65536);a->memory.write<8>(tables+0x1000,bits);boundaries(*a);}
}
void lifecycle_cases(){
    for(auto count:{0u,1u,2u}){auto a=seed(0);a->memory.write<4>(globals,count);boundaries(*a);}
    {auto a=seed(0);a->wr<4>(16,globals+0x300);boundaries(*a);}
    for(unsigned n:{0u,2u})for(unsigned fail:{1u,2u,16u,32u})for(bool terminate:{false,true}){auto a=seed(n);Options o;o.fail_allocation=fail;o.terminate=terminate;compare(*a,o);}
    for(auto value:{0u,1u,0x7fffu,0x8000u,0xffffu,0x10001u}){auto a=seed(1);a->wr<4>(16,value);boundaries(*a);}
    for(unsigned variant=0;variant<5;++variant){auto a=seed(1);if(variant==0)a->wr<4>(12,0);if(variant==1)a->memory.write<4>(object+0xdc,0);if(variant==2)a->memory.write<4>(globals,0);if(variant==3)for(unsigned n=0;n<32;++n)a->memory.write<4>(object+0x18+n*4,0);if(variant==4){a->memory.write<4>(object,0);a->memory.write<4>(object+4,0);}boundaries(*a);}
    for(auto score:{0u,1u,32767u,32768u,65535u}){auto a=seed(0);a->memory.write<4>(globals,1);a->memory.write<2>(clubs+0x1ec,score);compare(*a);}
    for(auto pc:{0x103564b8u,0x10356508u}){auto a=seed(0);Options o;o.controls={{pc,2304,4,1}};compare(*a,o);}
}
void persistence_cases(){
    for(auto version:{0x3e015u,0x3e016u,0x3e017u,0xffffffffu}){auto a=seed(4);Options o;o.version=version;boundaries(*a,o);}
    for(auto root:{4u,6u})for(unsigned cut=1;cut<=54;++cut)for(bool partial:{false,true}){auto a=seed(root);Options o;o.short_io=cut;o.partial=partial;compare(*a,o);}
    for(auto version:{0x3e015u,0x3e016u})for(unsigned cut=1;cut<=54;++cut){auto a=seed(4);Options o;o.version=version;o.short_io=cut;o.partial=true;compare(*a,o);}
    for(unsigned fail:{1u,2u,3u,18u,34u,35u,36u})for(bool terminate:{false,true}){auto a=seed(4);Options o;o.fail_allocation=fail;o.terminate=terminate;compare(*a,o);}
    {auto a=seed(4);Options o;o.header_count=0;compare(*a,o);}
    for(auto count:{0u,1u,2u})for(unsigned fail:{0u,3u}){auto a=seed(4);Options o;o.version=0x3e015;o.legacy_count=count;o.fail_allocation=fail;compare(*a,o);}
    {auto a=seed(4);Options o;o.empty_club=true;boundaries(*a,o);}
    {auto a=seed(6);a->memory.write<4>(club_table,0);a->memory.write<4>(club_table+12,0);boundaries(*a);}
    for(auto count:{0u,1u}){auto a=seed(6);a->memory.write<4>(globals,count);boundaries(*a);}
    for(auto score:{0u,1u,256u,32767u,32768u,65535u}){auto a=seed(4);a->memory.write<2>(clubs+0x1ec,score);compare(*a);}
}
void reset_cases(){
    for(auto staff_count:{0u,1u,15u,16u,17u,18u}){auto a=seed(2);a->memory.write<4>(globals+32,staff_count);boundaries(*a);}
    for(auto eligible:{0u,1u,127u,128u,255u,256u}){auto a=seed(2);Options o;o.eligible=eligible;compare(*a,o);}
    for(auto primary:{0u,contract})for(auto secondary:{0u,contract}){auto a=seed(2);Options o;o.primary=primary;o.secondary=secondary;boundaries(*a,o);}
    for(auto status:{0u,127u,128u,255u}){auto a=seed(2);a->memory.write<1>(contract+0x44,status);compare(*a);}
    for(auto bound:{0u,1u,10u,65535u,65536u,0xffff0000u}){auto a=seed(2);a->memory.write<4>(staff+12,0);a->memory.write<4>(globals+44,bound);Options o;if(bound>65535)o.controls={gpr(0x10356d10,17,0)};compare(*a,o);}
    {auto a=seed(2);a->memory.write<4>(staff+12,0);a->memory.write<4>(globals+20,pool+0x100);boundaries(*a);}
    {auto a=seed(2);Options o;o.flip_eligibility=true;o.terminate=true;compare(*a,o);}
    {auto a=seed(2);a->memory.write<4>(staff+12,0);a->memory.write<4>(globals+44,65536);Options o;o.controls={gpr(0x10356cb0,3,0x7fffffff),gpr(0x10356d10,17,0)};compare(*a,o);}
}
void independent_cases(){
    {auto a=seed(0);a->memory.write<4>(globals,1);const auto result=compare(*a);const auto& m=result.state->memory;const auto table=std::uint32_t(m.read<4>(object+0xdc)),rows=std::uint32_t(m.read<4>(table));require(m.read<2>(table+6)==100&&m.read<1>(table+4)==0&&m.read<1>(table+9)==0,"Independent per-club header defaults");for(unsigned n=0;n<40;++n){const auto row=rows+n*44;require(m.read<1>(row)==11&&m.read<2>(row+2)==0&&m.read<4>(row+4)==0&&m.read<4>(row+8)==0&&m.read<4>(row+12)==0,"Independent forty shortlist records");for(unsigned off:{16u,20u,24u})require(m.read<4>(row+off)==0xffffffff,"Independent identity sentinels");require(m.read<1>(row+1)==0x6b&&m.read<1>(row+0x28)==0x7b&&m.read<1>(row+0x24)==255,"Independent preserved bytes and masked flags");require(m.copy_out(row+0x1c,5)==std::vector<std::uint8_t>({0,1,7,205,3}),"Independent initial record date");}}
    {auto a=seed(0);Options o;o.mutate_after_date=true;boundaries(*a,o);}
    {auto a=seed(2);const auto result=compare(*a);const auto& m=result.state->memory;require(m.read<4>(object+0xd8)==18,"Independent capacity includes every staff member");for(unsigned n=0;n<16;++n){require(m.read<4>(object+0x98+n*4)==2,"Independent populated lists exclude last sixteen staff");const auto first=std::uint32_t(m.read<4>(object+0x18+n*4)),second=std::uint32_t(m.read<4>(object+0x58+n*4));require(m.read<4>(first)==0&&m.read<4>(first+4)==200&&m.read<4>(first+8)==10000,"Independent first bank record fields");require(m.read<4>(second+8)==0xfffffffe,"Independent signed contract status");}}
    {auto a=seed(6);const auto result=compare(*a);require(result.written.size()==4016&&std::equal(result.written.begin(),result.written.begin()+4,std::array<std::uint8_t,4>{0,0,0,2}.begin()),"Independent exported stream header and length");}
    {auto a=seed(4);const auto result=compare(*a);const auto& m=result.state->memory;require(result.position==4016,"Independent current-format input length");require(m.read<4>(m.read<4>(object))==10&&m.read<4>(m.read<4>(object+4))==11,"Independent copied and sorted pair arrays");const auto table=m.read<4>(object+0xdc);require(m.read<2>(table+6)==100,"Independent loaded club score floor");}
    {auto a=seed(4);Options o;o.version=0x3e016;const auto result=compare(*a,o);const auto& m=result.state->memory;const auto rows=m.read<4>(m.read<4>(object+0xdc));require(m.read<1>(rows)==11&&m.read<2>(rows+2)==0x0102,"Independent legacy row conversion");require(m.read<1>(rows+25*44)==11&&m.read<4>(rows+25*44+16)==0xffffffff,"Independent added legacy tail defaults");}
}

void relocation_cases(){
    for(auto root:{0u,2u,3u,4u,5u,6u}){auto a=seed(root);constexpr std::uint32_t moved=0x18008000;a->memory.map(moved-0x8000,0x9000);a->memory.copy_in(moved-0x8000,a->memory.copy_out(toc-0x8000,0x9000));a->wr<4>(8,moved);club_shortlist_storage_fixture_tables(*a,moved,tables+0x40000);compare(*a);}
}
}
namespace cm::aot {void club_shortlist_storage_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{
    for(unsigned n=0;n<7;++n){auto s=seed(n);boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.child_negative=true;if(n==3)s->memory.write<4>(globals+20,pool+0x100);compare(*s,o);}
    sampler_cases();lifecycle_cases();persistence_cases();reset_cases();independent_cases();relocation_cases();
    const auto expected=club_shortlist_storage_expected_sites();for(auto pc:expected)if(!sites.contains(pc))std::cerr<<"Uncovered shortlist site "<<hex_address(pc)<<std::endl;require(sites.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[](auto pc){return sites.contains(pc);}),"Every original shortlist site required");
    std::cout<<"Club shortlist storage comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/1945 completed_sites="<<completed_sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
