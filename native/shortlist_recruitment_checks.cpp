#include "shortlist_recruitment.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>
#include <map>
namespace cm::aot {std::vector<std::uint32_t> shortlist_recruitment_expected_sites();bool shortlist_recruitment_reference(State&);void shortlist_recruitment_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace recruitment=cm::aot::shortlist_recruitment;
constexpr std::uint32_t object=fixture+0x1000,pairs=fixture+0x4000,club_table=fixture+0x8000,clubs=fixture+0xc000,staff=fixture+0x10000,profiles=fixture+0x12000,contracts=fixture+0x14000,rows=fixture+0x16000,metadata=fixture+0x1a000,globals=fixture+0x1c000,random=fixture+0x1d000,tables=fixture+0x20000,countries=fixture+0x24000,club_service=fixture+0x25000,attributes=fixture+0x27000;
constexpr std::array<std::uint32_t,3> ends{0x10363f5c,0x10364d00,0x10365588};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{recruitment::distribute_staff_interest,recruitment::register_club_interest,recruitment::consider_requested_club_interest};
struct Control{std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options {
    std::vector<Control> controls;unsigned interrupt=0;
    bool own_negative=false,child_negative=false,structural=false,compose=false,primary=true,old_contract=true,service_present=true,current_service_present=true,attribute_present=true,force_search_miss=false,commit_bit=false,change_row_value=false,floating_stop=false;
    std::uint32_t unavailable=0,future_result=0,pending=0,roles=2,target_excluded=0,target_reserved=0,current_kind=0,domestic_allowed=1,domestic_mode=0,qualified=1,eligible=1,transfer_allowed=1,offer=0,blocked=1,quote=120,old_cost=100,new_cost=60,old_estimate=80,new_estimate=200,contract_limit=0,contract_action=0,renewal=65535,risk_budget=100,child_result=1;
    std::int32_t finance=1000000;
    double score=4,secondary_score=4,risk=20;
    std::vector<double> scores;
};
struct Trace:SemanticGuestObserver{Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==2)s.wr<2>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(recruitment::entries.begin(),recruitment::entries.end(),root);require(i!=recruitment::entries.end(),"Known recruitment root "+hex_address(root));return unsigned(i-recruitment::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned score_calls=0;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(shortlist_recruitment_reference(s),"Complete recruitment reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try{
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;double real=700.25;bool bank=false;
            s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x508,s.memory.read<4>(globals+0x508)+1);
            if(options.compose&&std::find(recruitment::entries.begin(),recruitment::entries.end(),target)!=recruitment::entries.end()){
                if(original)reference(s,stop);else modern_root(s,this,&trace);require(s.pc==stop,"Composed recruitment child return");
            }else{
                if(target==0x10082f20){const auto id=std::uint32_t(s.memory.read<4>(second));require(id<32,"Bounded contract fixture staff");result=(id==1?options.primary:options.old_contract)?contracts+id*0x80:0;}
                else if(target==0x103f43b0)result=options.pending;
                else if(target==0x10117490){(void)s.memory.read<2>(first+10);result=options.roles;}
                else if(target==0x100c7d00){(void)s.memory.read<4>(first);result=options.unavailable;}
                else if(target==recruitment::register_club_interest_entry){const auto id=std::uint32_t(s.memory.read<4>(second));s.memory.write<4>(globals+0x510,s.memory.read<4>(globals+0x510)+1);s.memory.write<4>(globals+0x514,id);s.memory.write<4>(globals+0x518,s.memory.read<4>(arg(5)));result=options.child_result;}
                else if(target==0x10717b20){const auto day=std::int16_t(s.memory.read<2>(second));const auto year=std::uint16_t(s.memory.read<2>(second+2));std::int64_t ordinal=std::int64_t(year)*365+day-arg(5);const auto y=std::int64_t(std::floor(double(ordinal)/365));s.memory.write<2>(first,ordinal-y*365);s.memory.write<2>(first+2,y);s.memory.write<1>(first+4,3);s.memory.write<1>(first+5,0x6d);s.memory.write<2>(first+6,0x1234);result=first;}
                else if(target==0x103721f0){s.memory.clear(first,0x40);result=first;}
                else if(target==0x1014c2e0)result=first==clubs+0x248?(second==0?options.target_excluded:options.target_reserved):options.current_kind;
                else if(target==0x100c96e0){result=std::uint32_t(s.memory.read<4>(first+0x2c));}
                else if(target==0x10147e70)result=options.domestic_allowed;
                else if(target==0x1030fbf0)result=options.qualified;
                else if(target==0x10149df0)result=options.domestic_mode;
                else if(target==0x10775600){require(arg(6)==8&&s.memory.read<4>(arg(7))==0x10366410,"Original pair descriptor and eight-byte fixture search");if(!options.force_search_miss){const auto key=s.memory.read<4>(first),club=s.memory.read<4>(first+4);const auto count=arg(5);require(count<64,"Bounded fixture pair count");for(unsigned n=0;n<count;++n){const auto at=second+n*8;if(s.memory.read<4>(at)==key&&(club==0xffffffff||s.memory.read<4>(at+4)==club)){result=at;break;}}}}
                else if(target==0x1036f590)result=options.eligible;
                else if(target==0x103f83c0)result=options.transfer_allowed;
                else if(target==0x10103510){const auto id=std::uint32_t(s.memory.read<4>(second));require(id<8,"Bounded fixture club service");result=options.service_present&&(id!=0||options.current_service_present)?club_service+id*0x200:0;}
                else if(target==0x100f5770){bank=true;result=1;}
                else if(target==0x1036fbf0){s.memory.write<4>(arg(7),options.contract_limit);result=options.offer;}
                else if(target==0x100d3040)result=options.blocked;
                else if(target==0x1077b770)result=options.renewal;
                else if(target==0x100cdef0)result=10000;
                else if(target==0x103ecef0)result=options.contract_action;
                else if(target==0x10361490){const auto person=arg(5),id=std::uint32_t(s.memory.read<4>(person));s.memory.write<4>(arg(7),id==1?options.new_estimate:options.old_estimate);if(options.change_row_value&&id!=1)s.memory.write<4>(arg(6)+4,200);result=id==1?options.new_cost:options.old_cost;}
                else if(target==0x10089a40||target==0x10085860)result=options.quote;
                else if(target==0x1018c340){const auto id=std::uint32_t(s.memory.read<4>(second));require(id<32,"Bounded fixture attribute lookup");result=options.attribute_present?attributes+id*0x80:0;}
                else if(target==0x10084be0)result=options.quote;
                else if(target==0x1036f170)result=options.future_result;
                else if(target==0x10369080){real=score_calls<options.scores.size()?options.scores[score_calls]:(arg(6)==1?options.score:options.secondary_score);++score_calls;result=11;if(options.commit_bit&&arg(6)==1){const auto id=std::uint32_t(s.memory.read<4>(arg(5))),entry=std::uint32_t(s.memory.read<4>(object+0xdc))+id*12,row=std::uint32_t(s.memory.read<4>(entry));s.memory.write<1>(row+0x28,s.memory.read<1>(row+0x28)|0x80);}}
                else if(target==0x1037b6f0){real=options.risk;result=12;}
                else if(target==0x100f2680)result=options.risk_budget;
                else if(target==0x10365590){s.memory.write<4>(globals+0x520,second);s.memory.write<4>(globals+0x524,arg(5));result=1;}
                else if(target==0x103632f0){const auto count=std::uint32_t(s.memory.read<4>(first+16));auto data=std::uint32_t(s.memory.read<4>(first));if(!data){data=pairs+0x1000;s.memory.write<4>(first,data);s.memory.write<4>(first+8,100);}require(count<64,"Fixture link insertion capacity");s.memory.write<4>(data+count*8,s.memory.read<4>(second));s.memory.write<4>(data+count*8+4,s.memory.read<4>(arg(5)));s.memory.write<4>(first+16,count+1);result=1;}
                else throw std::runtime_error("Unknown recruitment dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<8>(0x4010,float_bits<8>(real));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);if(bank)s.wr<4>(16,std::uint32_t(options.finance));s.pc=stop;
            }
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture recruitment dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned offset=1280;offset<1312;++offset)s->wr<1>(offset,0);s->memory.map(fixture,0x40000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=recruitment::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=1000000;s->wr<4>(12,object);s->wr<4>(16,staff+0x70);s->wr<4>(20,n==0?1:clubs+0x248);s->wr<4>(24,n==1?0xffff:1000);s->wr<4>(28,1);
    for(auto [slot,ptr]:{std::pair{-0x7b9c,globals+4},std::pair{-0x7b7c,globals},std::pair{-0x7b78,globals+8},std::pair{-0x7be8,globals+12},std::pair{-0x7b50,globals+16},std::pair{-0x765c,globals+20},std::pair{-0x7588,globals+24},std::pair{-0x7b40,globals+28},std::pair{-0x7b80,globals+0x80},std::pair{-0x7b1c,globals+32},std::pair{-0x7a54,globals+36},std::pair{-0x7ab8,globals+48},std::pair{-0x7ba0,globals+52},std::pair{-0x7be0,globals+56},std::pair{-0x7b90,globals+60},std::pair{-0x7b84,globals+64},std::pair{-0x7b88,globals+68},std::pair{-0x7b8c,random}})s->memory.write<4>(toc+slot,ptr);
    for(auto [at,value]:{std::pair{0u,globals+0x200},std::pair{4u,metadata},std::pair{12u,clubs},std::pair{16u,8u},std::pair{20u,globals+0x100},std::pair{24u,99u},std::pair{28u,globals+0x204},std::pair{32u,globals+0x208},std::pair{36u,globals+0x20c},std::pair{48u,globals+0x210},std::pair{52u,globals+0x214},std::pair{56u,staff},std::pair{60u,0u},std::pair{64u,random},std::pair{68u,random+0x20}})s->memory.write<4>(globals+at,value);
    s->memory.write<2>(globals+0x80,100);s->memory.write<2>(globals+0x82,2001);for(unsigned k=0;k<18;++k)s->memory.write<2>(random+k*2,10+k*13);
    s->memory.write<4>(object,pairs);s->memory.write<4>(object+8,100);s->memory.write<4>(object+16,0);s->memory.write<4>(object+0xdc,club_table);
    for(unsigned c=0;c<8;++c){s->memory.write<4>(clubs+c*0x248,c);s->memory.write<4>(clubs+c*0x248+0x2c,countries+0x100);s->memory.write<4>(clubs+c*0x248+0x90,staff+3*0x70);s->memory.write<2>(clubs+c*0x248+0x1ec,2500);s->memory.write<4>(club_table+c*12,rows+c*40*44);s->memory.write<1>(club_table+c*12+4,1);s->memory.write<1>(club_table+c*12+9,0);s->memory.write<4>(globals+0x100+c*4,c);s->memory.write<1>(club_service+c*0x200+0x165,1);
        for(unsigned k=0;k<40;++k){const auto row=rows+(c*40+k)*44;s->memory.write<1>(row,11);s->memory.write<2>(row+2,2);s->memory.write<4>(row+4,100);s->memory.write<4>(row+8,10000);s->memory.write<4>(row+0x10,k==0?2:0xffffffff);s->memory.write<4>(row+0x14,0xffffffff);s->memory.write<4>(row+0x18,0xffffffff);s->memory.write<2>(row+0x1c,1);s->memory.write<2>(row+0x1e,2000);s->memory.write<1>(row+0x28,0);}}
    for(unsigned k=0;k<32;++k){const auto person=staff+k*0x70,record=contracts+k*0x80;s->memory.write<4>(person,k);s->memory.write<4>(person+8,k==1?11000:1000);s->memory.write<4>(person+12,countries);s->memory.write<4>(person+0x14,profiles+k*0x40);s->memory.write<4>(person+0x24,clubs);s->memory.write<2>(person+0x4c,20);s->memory.write<2>(person+0x4e,2000);s->memory.write<2>(person+0x5c,20);s->memory.write<1>(person+0x62,11);s->memory.write<2>(profiles+k*0x40+10,8000);s->memory.write<4>(record+4,0);s->memory.write<1>(record+0x60,7);s->memory.write<4>(record+0x50,1);s->memory.write<2>(attributes+k*0x80+0x34,15);}
    s->memory.write<4>(countries+0x68,1);s->memory.write<4>(countries+0x100+0x68,2);shortlist_recruitment_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites,completed_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Shortlist recruitment root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture recruitment dependency interruption"||expected=="Fixture recruitment requested exit"||expected.starts_with("Unmapped data access at ")||(options.floating_stop&&expected=="Original enabled floating inexact exception requires a handler"),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First recruitment site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=options.structural||!options.controls.empty();if(!rejected&&expected!="Native instruction budget exhausted")sites.insert(ot.sites.begin(),ot.sites.end());if(!rejected&&expected.empty())completed_sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed recruitment boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n:std::set<unsigned>{1,2,unsigned(base.calls.size()/2),unsigned(base.calls.size())})if(n&&n<=base.calls.size()){auto o=options;o.interrupt=n;compare(input,o);}
}



void baseline_cases(){for(unsigned n=0;n<3;++n){auto a=seed(n);boundaries(*a);Options o;o.own_negative=true;compare(*a,o);o.own_negative=false;o.child_negative=true;compare(*a,o);}}
void initial_cases(){
    for(unsigned n:{1u,2u}){auto a=seed(n);const auto result=compare(*a);require(result.state->r<4>(12)==1,"Independent recruitment baseline returns success");}
    {auto a=seed(0);Options o;o.compose=true;compare(*a,o);}
}
void row_field(State& s,unsigned offset,unsigned width,std::uint32_t value){for(unsigned n=0;n<40;++n){const auto at=rows+(40+n)*44+offset;if(width==1)s.memory.write<1>(at,value);else if(width==2)s.memory.write<2>(at,value);else s.memory.write<4>(at,value);}}
void distribution_cases(){
    for(auto force:{0u,1u,128u,255u,256u})for(auto ability:{0u,5499u,5500u,8000u,0xffffu}){auto a=seed(0);a->wr<4>(20,force);a->memory.write<2>(profiles+0x40+10,ability);compare(*a);}
    for(auto flags:{0u,4u,8u,12u,0x40u,0x50u,0xf0u}){auto a=seed(0);a->memory.write<1>(metadata+100+12,flags);boundaries(*a);}
    for(auto status:{0u,10u,11u,255u})for(auto retired:{0u,1u})for(auto kind:{0u,5u}){auto a=seed(0);a->memory.write<1>(staff+0x70+0x62,status);a->memory.write<4>(staff+0x70+0x20,retired?profiles:0);a->memory.write<1>(staff+0x70+0x60,kind);compare(*a);}
    for(auto nationality:{0u,countries})for(auto excluded:{1u,99u})for(auto pending:{0u,1u,255u,256u}){auto a=seed(0);a->memory.write<4>(staff+0x70+12,nationality);a->memory.write<4>(globals+24,excluded);Options o;o.pending=pending;compare(*a,o);}
    for(auto year:{1999u,2000u,2001u,2002u})for(auto day:{0u,20u,265u,300u,0xffffu})for(auto status:{0u,7u}){auto a=seed(0);a->memory.write<2>(staff+0x70+0x4e,year);a->memory.write<2>(staff+0x70+0x4c,day);a->memory.write<1>(contracts+0x80+0x60,status);a->memory.write<4>(contracts+0x80+0x50,0);compare(*a);}
    for(auto role:{0u,1u,2u,8u,32u,128u,2048u,0xffffu})for(auto result:{0u,1u,255u,256u}){auto a=seed(0);Options o;o.roles=role;o.child_result=result;compare(*a,o);}
    for(auto count:{0u,1u,8u})for(bool primary:{false,true})for(auto unavailable:{0u,1u,255u,256u}){auto a=seed(0);a->memory.write<4>(globals+16,count);Options o;o.primary=primary;o.unavailable=unavailable;compare(*a,o);}
    {auto a=seed(0);a->memory.write<4>(staff+0x70+0x14,0);boundaries(*a);}
    {auto a=seed(0);a->memory.write<4>(staff+0x70+0x24,0);Options o;o.primary=false;boundaries(*a,o);}
    {auto a=seed(0);Options o;o.compose=true;boundaries(*a,o);}
}
void preparation_cases(){
    for(unsigned n:{1u,2u}){
        for(auto status:{0u,1u,2u,255u})for(auto used:{0u,99u,100u,255u}){auto a=seed(n);a->memory.write<1>(club_table+12+4,status);a->memory.write<1>(club_table+12+9,used);compare(*a);}
        for(auto array:{0u,pairs})for(auto id:{1u,2u}){auto a=seed(n);a->memory.write<4>(object,array);a->memory.write<4>(object+16,1);a->memory.write<4>(pairs,id);a->memory.write<4>(pairs+4,1);boundaries(*a);}
        for(bool primary:{false,true})for(bool old_contract:{false,true})for(auto allowed:{0u,1u,255u,256u}){auto a=seed(n);Options o;o.primary=primary;o.old_contract=old_contract;o.eligible=allowed;compare(*a,o);o.eligible=1;o.transfer_allowed=allowed;compare(*a,o);}
        for(auto roles:{0u,1u,2u,8u,32u,128u,2048u,0xffffu,0x8000u}){auto a=seed(n);Options o;o.roles=roles;if(n==1)a->wr<4>(24,roles);compare(*a,o);}
        for(bool present:{false,true})for(bool current:{false,true}){auto a=seed(n);Options o;o.service_present=present;o.current_service_present=current;compare(*a,o);}
    }
    for(auto reserved:{0u,1u,256u,255u})for(auto available:{0u,1u})for(auto mode:{0u,1u,2u,3u,255u,256u})for(auto qualified:{0u,1u,256u}){auto a=seed(1);Options o;o.target_reserved=reserved;o.domestic_allowed=available;o.domestic_mode=mode;o.qualified=qualified;compare(*a,o);}
    {auto a=seed(1);Options o;o.target_excluded=1;boundaries(*a,o);}
    for(unsigned n:{1u,2u}){auto a=seed(n);Options o;o.controls=n==1?std::vector<Control>{{0x10364104,27*4,0,4},{0x10364164,27*4,clubs+0x248,4}}:std::vector<Control>{{0x10364e14,17*4,0,4},{0x10364e74,17*4,clubs+0x248,4}};compare(*a,o);}
}
void row_filter_cases(){
    for(unsigned n:{1u,2u}){
        for(auto field:{0u,2u,0x10u,0x14u,0x18u,0x28u})for(auto value:{0u,1u,2u,11u,128u,0xffffu,0xffffffffu}){auto a=seed(n);row_field(*a,field,field==0||field==0x28?1:field==2?2:4,value);compare(*a);}
        for(auto year:{2000u,2001u,2002u})for(auto day:{0u,100u,300u,0xffffu}){auto a=seed(n);row_field(*a,0x1e,2,year);row_field(*a,0x1c,2,day);compare(*a);}
        for(auto age:{20u,24u,25u,128u,256u,0xffffu})for(auto ability:{999u,1000u,4999u,5000u})for(auto same:{0u,1u,2u}){auto a=seed(n);row_field(*a,0x28,1,5*8);a->memory.write<2>(staff+0x70+0x5c,age);a->memory.write<2>(profiles+0x40+10,ability);if(same==1)a->memory.write<4>(staff+0x70+12,countries+0x100);if(same==2)a->memory.write<4>(clubs+0x2c,countries+0x100);compare(*a);}
        {auto a=seed(n);row_field(*a,0x10,4,2);row_field(*a,0x14,4,2);boundaries(*a);}
        {auto a=seed(n);row_field(*a,0x10,4,0xffffffff);boundaries(*a);}
        {auto a=seed(n);a->memory.write<4>(staff+2*0x70+0x14,0);boundaries(*a);}
    }
    for(unsigned n:{1u,2u})for(auto end:{random,random+2,random+0x20})for(auto salt:{0u,65535u,0xffffffffu}){auto a=seed(n);row_field(*a,0x28,1,40);a->memory.write<4>(globals+68,end);a->memory.write<4>(globals+60,salt);boundaries(*a);}
}
void pricing_cases(){
    for(unsigned n:{1u,2u}){
        for(auto value:{0u,1u,100u,10000u,0x7fffffffu})for(auto finance:{-1,0,1,10000,1000000}){auto a=seed(n);row_field(*a,8,4,value);Options o;o.finance=finance;compare(*a,o);}
        for(auto reputation:{0u,1u,1249u,1250u,2500u,6999u,7000u,0x7fffu,0x8000u,0xffffu}){auto a=seed(n);a->memory.write<2>(clubs+0x248+0x1ec,reputation);compare(*a);}
        for(auto price:{0u,50u,100u,1000u,0xffffffffu})for(auto quote:{0u,60u,120u,20000u}){auto a=seed(n);Options o;o.old_cost=price;o.new_cost=price;o.quote=quote;compare(*a,o);}
        for(auto old_estimate:{0u,50u,100u,1000u})for(auto new_estimate:{0u,50u,200u,2000u}){auto a=seed(n);Options o;o.old_estimate=old_estimate;o.new_estimate=new_estimate;compare(*a,o);}
        for(auto age:{0u,14u,15u,16u,30u,0xffffu})for(bool present:{false,true}){auto a=seed(n);a->memory.write<2>(attributes+0x80+0x34,age);a->memory.write<2>(attributes+0x100+0x34,age);Options o;o.attribute_present=present;compare(*a,o);}
        for(auto kind:{0u,1u,255u,256u})for(bool contract:{false,true}){auto a=seed(n);Options o;o.current_kind=kind;o.primary=contract;compare(*a,o);}
    }
    for(auto offer:{0u,1u,255u,256u})for(auto limit:{0u,50u,120u,300u})for(auto future:{0u,1u,2u,255u,256u}){auto a=seed(1);Options o;o.offer=offer;o.contract_limit=limit;o.future_result=future;compare(*a,o);}
    for(unsigned n:{1u,2u})for(auto action:{0u,1u,255u,256u})for(auto kind:{0u,1u,2u,3u,5u}){auto a=seed(n);row_field(*a,0x28,1,kind*8);Options o;o.contract_action=action;compare(*a,o);}
    {auto a=seed(1);Options o;o.new_estimate=1;const auto result=compare(*a,o);require(result.state->r<4>(12)==0&&result.state->memory.read<4>(rows+40*44+8)==10000,"Independent rejected registration restores its tentative row threshold");}
    {auto a=seed(1);Options o;o.old_estimate=1000;o.risk=1000;const auto result=compare(*a,o);require(result.state->memory.read<4>(rows+40*44+8)!=10000,"Independent earlier rejection retains the original partial threshold effect");}
}
void completion_cases(){
    for(unsigned n:{1u,2u})for(auto score:{-1.0,0.0,2.749,2.75,3.149,3.15,3.151,4.0}){auto a=seed(n);Options o;o.score=score;compare(*a,o);}
    for(auto secondary:{0.05,0.1,0.1001,3.0,3.888888888888889,4.0,100.0})for(auto risk:{0.0,20.0,120.0,1000.0})for(auto budget:{0u,15u,100u}){auto a=seed(1);Options o;o.secondary_score=secondary;o.risk=risk;o.risk_budget=budget;compare(*a,o);}
    for(bool commit:{false,true}){auto a=seed(1);Options o;o.commit_bit=commit;boundaries(*a,o);}
    for(auto apply:{0u,1u,128u,255u,256u}){auto a=seed(2);a->wr<4>(28,apply);const auto result=compare(*a);require(result.state->r<4>(12)==1,"Independent explicit-interest approval");require(result.state->memory.read<4>(rows+40*44+0x10)==(std::int8_t(apply)?1u:2u),"Independent apply flag controls the row publication");}
    for(unsigned n:{0u,1u,2u}){auto a=seed(n);constexpr std::uint32_t moved=0x18008000;a->memory.map(moved-0x8000,0x9000);a->memory.copy_in(moved-0x8000,a->memory.copy_out(toc-0x8000,0x9000));a->wr<4>(8,moved);shortlist_recruitment_fixture_tables(*a,moved,fixture+0x34000);compare(*a);}
}

void remaining_paths(){
    for(unsigned n:{1u,2u})for(auto ability:{0u,999u,1000u,4999u,5000u,8000u})for(auto end:{random,random+0x20}){
        auto a=seed(n);a->memory.write<4>(clubs+0x2c,countries);a->memory.write<2>(profiles+0x40+10,ability);a->memory.write<4>(globals+68,end);row_field(*a,0x28,1,40);boundaries(*a);
    }
    for(auto blocked:{0u,1u,255u,256u}){auto a=seed(1);a->memory.write<4>(staff+0x70+8,1500050);row_field(*a,8,4,100);Options o;o.current_kind=1;o.blocked=blocked;boundaries(*a,o);}
    {auto a=seed(1);row_field(*a,8,4,10);Options o;o.finance=100;boundaries(*a,o);}
    for(auto threshold:{0u,1u}){auto a=seed(1);row_field(*a,8,4,threshold);Options o;o.primary=false;o.finance=0;boundaries(*a,o);}
    for(unsigned n:{1u,2u}){auto a=seed(n);Options o;o.change_row_value=true;o.old_estimate=150;boundaries(*a,o);}
    for(unsigned n:{1u,2u})for(auto price:{120u,121u,1000u}){auto a=seed(n);Options o;o.current_kind=1;o.new_cost=price;boundaries(*a,o);}
    for(auto limit:{0xffffffffu,0x80000000u}){auto a=seed(1);Options o;o.offer=1;o.contract_limit=limit;boundaries(*a,o);}
    {auto a=seed(2);a->memory.write<2>(clubs+0x248+0x1ec,10);Options o;o.old_cost=1000;o.quote=0;boundaries(*a,o);}
    for(auto amount:{0u,10u,0xfff00000u}){auto a=seed(2);a->wr<4>(24,amount);a->memory.write<2>(clubs+0x248+0x1ec,10);Options o;o.new_cost=1000;boundaries(*a,o);}
}

void floating_cases(){
    for(unsigned n:{1u,2u})for(auto value:{std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){auto a=seed(n);Options o;o.score=value;compare(*a,o);}
    for(auto value:{std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){auto a=seed(1);Options o;o.secondary_score=value;compare(*a,o);o.secondary_score=4;o.risk=value;compare(*a,o);}
    for(unsigned n:{1u,2u})for(auto status:{1286u,1287u,1294u,1299u}){auto a=seed(n);a->wr<1>(status,1);compare(*a);}
    for(unsigned n:{1u,2u}){auto a=seed(n);a->wr<1>(1308,1);Options o;o.floating_stop=true;compare(*a,o);}
}

}
namespace cm::aot {void shortlist_recruitment_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{baseline_cases();initial_cases();distribution_cases();preparation_cases();row_filter_cases();pricing_cases();completion_cases();remaining_paths();floating_cases();const auto expected=shortlist_recruitment_expected_sites();for(auto pc:expected)if(!sites.contains(pc))std::cerr<<"Uncovered recruitment site "<<hex_address(pc)<<std::endl;std::cout<<"Shortlist recruitment comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/1591 completed_sites="<<completed_sites.size()<<std::endl;require(sites.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[](auto pc){return sites.contains(pc);}),"Every original recruitment site required");}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
