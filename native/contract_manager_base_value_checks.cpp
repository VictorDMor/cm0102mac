#include "contract_manager.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <cmath>
#include <limits>
#include <set>

namespace cm::aot {
bool contract_manager_base_value_reference(State&);
std::vector<std::uint32_t> contract_manager_base_value_calls();
void contract_manager_base_value_constants(State&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;using guest::Cpu;
constexpr auto entry=0x10084be0u,end=0x10085860u;
constexpr auto object=fixture+0x1000,proxy=fixture+0x1100,person=fixture+0x2000,clubs=fixture+0x3000;
constexpr auto records=fixture+0x5000,attributes=fixture+0x6000,globals=fixture+0x7000,indices=fixture+0x8000;
constexpr auto division=fixture+0x9000,competition=fixture+0xa000,owner=fixture+0xb000,origin=fixture+0xc000;
constexpr auto random=fixture+0xd000,constants=fixture+0x10000,singles=fixture+0x12000,ages=fixture+0x13000,today=fixture+0x14000;
constexpr auto conversion_constants=fixture+0x15000;
struct Options {
    int position=0,age=24,ability=7000,reputation=6000,potential=7000,ambition=10,trait=15;
    int club_reputation=5000,requested_score=-1,calculated_score=2,division_level=2,division_standing=10,competition_standing=10,club_score=5000,club_kind=0;
    int person_id=0,person_count=3,primary=0,expiry=365,mood=0,role=2,role_bits=0,ceiling=500000;
    int country=2,national_country=1,alternate_country=3,origin_modifier=18;
    unsigned adjusted=0,international=0,cursor=0,salt=0,entropy=0x1234,interrupt=0,fp_state=0;
    std::int64_t conversion=-1;
    double conversion_input=0;
    bool original_conversion=false,force_conversion_input=false;
    bool target=true,same_club=true,player=true,club_contract=true,has_division=true,has_competition=true,has_origin=true;
    bool contract_policy=true,release_ceiling=false,null_person=false,expect_fp_guard=false,corrupt=false,corrupt_assessment=false,corrupt_conversion=false;
};
struct Event {
    std::uint32_t target,back;std::vector<std::uint32_t> args,kept;std::vector<std::uint64_t> fp;std::vector<std::uint8_t> fpscr;
    bool operator==(const Event&)const=default;
};
unsigned arity(std::uint32_t target){switch(target){
    case 0x1077b770:case 0x10773780:return 0;
    case 0x10775540:case 0x10380520:return 1;
    case 0x103faff0:return 2;
    case 0x10082f90:case 0x100cdef0:return 3;
    case 0x103fac70:return 4;
    case 0x10219e40:return 5;
    default:throw std::runtime_error("Unknown base-value dependency "+hex_address(target));
}}
struct Model {
    Options o;std::vector<Event> calls;unsigned original_conversions=0;bool conversion_faulted=false;
    explicit Model(Options options):o(std::move(options)){}
    void call(State& s,std::uint32_t target,std::uint32_t back){
        Cpu c(s);Event e{target,back,{},{},{},{}};
        for(unsigned n=0;n<arity(target);++n)e.args.push_back(c.r(n+3));
        for(unsigned n=13;n<32;++n)e.kept.push_back(c.r(n));
        e.fp.push_back(s.r<8>(guest::reg::fpr(1)));for(unsigned n=14;n<32;++n)e.fp.push_back(s.r<8>(guest::reg::fpr(n)));
        for(unsigned n=1280;n<1312;++n)e.fpscr.push_back(std::uint8_t(s.r<1>(n)));
        calls.push_back(e);std::uint32_t value=0;
        if(target==0x10773780&&o.original_conversion){
            // Exercise the caller with the complete original helper, including
            // its CR6/7, FPSCR and below-SP stores. Forced inputs are explicitly
            // injected helper boundaries, not observed normal-game arguments.
            if(o.force_conversion_input)c.set_f(1,o.conversion_input);
            ++original_conversions;
            try{while(s.pc>=0x10773780&&s.pc<0x107737d0)require(contract_manager_base_value_reference(s),"Complete original conversion helper");}
            catch(...){conversion_faulted=true;throw;}
            require(s.pc==back,"Original converter returned to caller");
            if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Base-value dependency interrupted");return;
        }
        if(target==0x10082f90)value=o.club_contract?records:0;
        if(target==0x100cdef0)value=std::uint32_t(o.club_reputation+(o.corrupt_assessment?1000:0));
        if(target==0x10219e40)value=std::uint32_t(o.calculated_score);
        if(target==0x103fac70)value=0x12345678;
        if(target==0x103faff0)value=o.contract_policy;
        if(target==0x10380520)value=o.release_ceiling;
        if(target==0x10775540){
            const auto input=std::int32_t(c.r(3));const auto sign=input<0?0xffffffffu:0u;
            c.s.wr<1>(guest::reg::xer_ca,input<0&&(std::uint32_t(input)&0x7fffffff));
            c.set(4,sign);c.set(0,sign^std::uint32_t(input));c.set(3,(sign^std::uint32_t(input))-sign);s.pc=back;
            if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Base-value dependency interrupted");return;
        }
        if(target==0x10773780){
            const auto input=c.f(1);
            value=std::isnan(input)||input>=4294967296.0?0xffffffffu:input<0?0:std::uint32_t(std::trunc(input));
            if(o.conversion>=0)value=std::uint32_t(o.conversion);
            if(o.corrupt_conversion)value+=100;
        }
        if(target==0x1077b770){c.set(0,0);c.set(4,globals+0x100);c.set(3,o.entropy);s.pc=back;
            if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Base-value dependency interrupted");return;}
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);
        for(unsigned n=0;n<14;++n)c.set_f(n,-100.0-n);c.set_cr(0,2);c.set(3,value);s.pc=back;
        if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Base-value dependency interrupted");
    }
};
thread_local Model* active_model=nullptr;struct Hook final:ContractManagerCallHook{
    bool call(State& s)override{active_model->call(s,s.pc,std::uint32_t(s.r<4>(4128)));return true;}
}hook;
void date(Cpu& c,std::uint32_t at,int ordinal){int year=ordinal/365,day=ordinal%365;if(day<0){day+=365;--year;}c.put16(at,std::uint32_t(day));c.put16(at+2,std::uint32_t(year));c.put8(at+4,0);}
std::unique_ptr<State> seed(const Options& o){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x20000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);Cpu c(*s);
    for(unsigned n=0;n<0x5000;++n)c.put8(stack-0x4000+n,n*13+7);for(unsigned n=0;n<32;++n)c.set_f(n,double(n)+0.25);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,o.fp_state==3?1:n>=1304?0:o.fp_state==1?1:o.fp_state==2?n&1:0);
    c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,object);c.set(4,o.null_person?0:person);c.set(5,o.target?clubs+(o.same_club?0:0x248):0);c.set(6,o.adjusted);c.set(7,std::uint32_t(o.requested_score));c.set(8,0xabcddcba);s->pc=entry;s->budget=1000000;
    unsigned slot_index=0;const auto holder=[&](int slot,std::uint32_t value){const auto at=globals+slot_index++*4;c.put32(toc+slot,at);c.put32(at,value);};
    holder(-0x7b7c,proxy);holder(-0x7be4,std::uint32_t(o.person_count));holder(-0x7b9c,indices);holder(-0x7b78,0);holder(-0x7be8,clubs);holder(-0x79fc,std::uint32_t(o.national_country));holder(-0x7a00,std::uint32_t(o.alternate_country));holder(-0x7b40,owner+0x1000);holder(-0x7b90,o.salt);holder(-0x7b84,random+o.cursor);holder(-0x7b88,random+8);
    c.put32(toc-0x7b8c,random);c.put32(toc-0x7b80,today);
    contract_manager_base_value_constants(*s,toc,constants,singles,ages,conversion_constants);
    c.put32(object,records);c.put32(object+4,1);c.put32(object+8,1);c.put32(proxy,records);c.put32(proxy+4,1);c.put32(proxy+8,1);
    c.put32(person,std::uint32_t(o.person_id));c.put32(person+0x14,o.player?attributes:0);c.put16(person+0x5c,std::uint32_t(o.age));c.put8(person+0x5e,o.international);c.put32(person+12,o.has_origin?origin:0);c.put16(origin+0xac,std::uint32_t(o.origin_modifier));
    c.put32(indices+std::uint32_t(o.person_id*100),std::uint32_t(o.primary));
    for(unsigned n=0;n<100;++n)c.put8(records+n,n*19+3);c.put32(records,std::uint32_t(o.person_id));c.put32(records+4,0);c.put32(records+0x24,std::uint32_t(o.ceiling));c.put8(records+0x44,std::uint32_t(o.mood));c.put8(records+0x38,(std::uint32_t(o.role)<<2)|std::uint32_t(o.role_bits));
    const auto now=2001*365+100;date(c,today,now);date(c,records+0x30,now+o.expiry);
    for(unsigned n=0;n<2;++n){const auto club=clubs+n*0x248;c.put32(club,n);c.put32(club+0x90,owner);c.put32(club+0x2c,o.has_division?division:0);c.put32(club+0x30,o.has_competition?competition:0);c.put16(club+0x1ec,std::uint32_t(o.club_score));c.put8(club+0x23d,std::uint32_t(o.club_kind));}
    c.put32(division+0x68,std::uint32_t(o.country));c.put8(division+0x11c,std::uint32_t(o.division_level));c.put8(division+0x11f,std::uint32_t(o.division_standing));c.put16(competition+0x18,std::uint32_t(o.competition_standing));
    for(unsigned n=0;n<0x80;++n)c.put8(attributes+n,std::uint32_t(o.trait));
    for(unsigned offset:{0xfu,0x10u,0x11u,0x12u,0x13u,0x14u,0x15u})c.put8(attributes+offset,0);
    if(o.position<4)c.put8(attributes+0x15-unsigned(o.position),15);
    else if(o.position==4)c.put8(attributes+15,20);
    else if(o.position==5)c.put8(attributes+16,15);
    else if(o.position==6)c.put8(attributes+17,1);
    c.put16(attributes+8,std::uint32_t(o.ability));c.put16(attributes+10,std::uint32_t(o.reputation));c.put16(attributes+12,std::uint32_t(o.potential));c.put8(attributes+0x2d,std::uint32_t(o.ambition));
    for(unsigned n=0;n<16;++n)c.put16(random+n*2,n*101+7);return s;
}
void reference(State& s,Model& model){while(s.pc!=return_sentinel){if(s.pc>=entry&&s.pc<end)require(contract_manager_base_value_reference(s),"Complete original base-value body");else model.call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
struct Totals{unsigned cases=0,returned=0,partial=0,negative=0,null_faults=0,fp_guards=0;std::set<std::uint32_t> returned_calls;}totals;
void same(const State& a,const State& b,const std::string& label,bool diagnostic_pc){
    for(unsigned n=1;n<32;++n)if(n!=11&&n!=12)require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n)+" "+hex_address(a.r<4>(n*4))+"/"+hex_address(b.r<4>(n*4)));
    for(unsigned n=1;n<32;++n)if(n==1||n>=14)require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));
    for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR "+std::to_string(n));
    for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR "+std::to_string(n));
    require(a.r<1>(guest::reg::xer_ca)==b.r<1>(guest::reg::xer_ca),label+" carry");require(a.r<4>(4128)==b.r<4>(4128)&&(diagnostic_pc||a.pc==b.pc),label+" LR/PC");
    for(auto [address,size]:a.memory.mapped_ranges()){const auto x=a.memory.copy_out(address,size),y=b.memory.copy_out(address,size);if(x!=y){const auto at=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(address+std::uint32_t(at.first-x.begin()))+" "+std::to_string(*at.first)+"/"+std::to_string(*at.second));}}
}
void compare(Options o={}){
    auto source=seed(o),a=clone(*source),b=clone(*source);Model old(o),modern(o);old.o.corrupt_assessment=old.o.corrupt_conversion=false;
    const auto run=[&](State& s,Model& model,bool original){try{
        if(original)reference(s,model);else{
            const auto previous=contract_manager_call_hook;active_model=&model;contract_manager_call_hook=&hook;
            try{Cpu c(s);c.set(3,contract_manager::base_player_value(c,c.r(3),c.r(4),c.r(5),c.r(6),c.r(7)));c.finish();}catch(...){contract_manager_call_hook=previous;active_model=nullptr;throw;}
            contract_manager_call_hook=previous;active_model=nullptr;
        }return std::string{};
    }catch(const std::exception& e){return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false),label="base value case "+std::to_string(totals.cases);
    const bool null_fault=o.null_person&&expected=="Unmapped data access at 0x00000014";
    const bool fp_guard=expected=="Original enabled floating inexact exception requires a handler"||expected=="Original enabled floating invalid-conversion exception requires a handler";
    require(expected.empty()||expected=="Base-value dependency interrupted"||null_fault||(o.expect_fp_guard&&fp_guard),label+" invalid reference: "+expected);require(expected==actual,label+" outcomes: "+expected+"/"+actual);
    if(o.interrupt)require(expected=="Base-value dependency interrupted"&&old.calls.size()==o.interrupt,label+" interruption reached");
    else if(o.null_person)require(null_fault,label+" exact original null-person read reached");else if(o.expect_fp_guard)require(fp_guard,label+" enabled FP guard reached");else require(expected.empty(),label+" completed return");
    if(o.original_conversion)require(old.original_conversions&&modern.original_conversions,label+" complete conversion helper entered");
    if(o.original_conversion&&o.force_conversion_input&&o.expect_fp_guard)require(old.conversion_faulted&&modern.conversion_faulted,label+" FP guard arose inside the conversion helper");
    if(o.corrupt)b->memory.write<1>(person,b->memory.read<1>(person)^0x80);bool rejected=false;
    try{same(*a,*b,label,null_fault||fp_guard);if(old.calls!=modern.calls){
        const auto at=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());std::string detail;
        if(at.first!=old.calls.end()&&at.second!=modern.calls.end()){
            detail=" targets "+hex_address(at.first->target)+"/"+hex_address(at.second->target)+" backs "+hex_address(at.first->back)+"/"+hex_address(at.second->back);
            for(unsigned n=0;n<std::min(at.first->args.size(),at.second->args.size());++n)if(at.first->args[n]!=at.second->args[n])detail+=" arg"+std::to_string(n+3)+" "+hex_address(at.first->args[n])+"/"+hex_address(at.second->args[n]);
            for(unsigned n=0;n<at.first->kept.size();++n)if(at.first->kept[n]!=at.second->kept[n])detail+=" r"+std::to_string(n+13)+" "+hex_address(at.first->kept[n])+"/"+hex_address(at.second->kept[n]);
            for(unsigned n=0;n<at.first->fp.size();++n)if(at.first->fp[n]!=at.second->fp[n])detail+=" fp-slot"+std::to_string(n)+" "+hex_address(at.first->fp[n])+"/"+hex_address(at.second->fp[n]);
        }throw std::runtime_error(label+" call "+std::to_string(at.first-old.calls.begin())+detail);
    }}catch(const std::runtime_error&){if(!o.corrupt&&!o.corrupt_assessment&&!o.corrupt_conversion)throw;rejected=true;}
    require(rejected==(o.corrupt||o.corrupt_assessment||o.corrupt_conversion),label+" mutation control");++totals.cases;totals.returned+=expected.empty()&&!rejected;totals.partial+=!expected.empty();totals.negative+=rejected;totals.null_faults+=null_fault;totals.fp_guards+=fp_guard;
    if(expected.empty()&&!rejected)for(const auto& e:old.calls)totals.returned_calls.insert(e.back);
}
}
int main(){try{
    compare();
    for(int position=0;position<8;++position)for(int age:{17,23,24,28,29,32,33,34,40}){Options o;o.position=position;o.age=age;compare(o);}
    for(int age:{-128,-1,0,127,128,255,280})for(bool target:{false,true}){Options o;o.age=age;o.target=target;compare(o);}
    for(int trait:{-128,-1,0,14,15,20,127})for(int ambition:{-128,0,10,20,127}){Options o;o.position=7;o.trait=trait;o.ambition=ambition;compare(o);}
    for(int ability:{-32768,-1,0,5250,32767})for(int potential:{4999,5000,5001,8000,8001,32767}){Options o;o.ability=ability;o.potential=potential;compare(o);}
    for(int score:{-65537,-32768,-1,0,1,655,656,32767,65535})for(int reputation:{-32768,-1,0,4999,5000,5001,32767}){Options o;o.requested_score=score;o.club_reputation=reputation;compare(o);}
    for(int result:{0,1,12000,12001}){Options o;o.conversion=result;compare(o);}
    Options maximum;maximum.conversion=0xffffffffu;compare(maximum);
    for(bool target:{false,true})for(bool division_present:{false,true})for(bool competition_present:{false,true})for(int level:{-1,0,1,3,4}){Options o;o.target=target;o.has_division=division_present;o.has_competition=competition_present;o.division_level=level;compare(o);}
    for(int kind:{-128,0,1,2,3,4,127})for(int standing:{-32768,0,10,9268,32767}){Options o;o.club_kind=kind;o.competition_standing=standing;compare(o);}
    for(unsigned international:{0u,1u,2u,255u})for(int modifier:{-32768,-18,0,18,180,32767})for(bool origin_present:{false,true}){Options o;o.international=international;o.origin_modifier=modifier;o.has_origin=origin_present;compare(o);}
    for(unsigned cursor:{0u,6u,8u})for(unsigned salt:{0u,1u,2u,65535u,0x7fffffffu,0xffffffffu}){Options o;o.target=false;o.cursor=cursor;o.salt=salt;compare(o);}
    for(unsigned adjusted:{0u,1u,128u,256u})for(int country:{1,2,3})for(bool capped:{false,true})for(int ceiling:{-1,0,500000,2147483647}){Options o;o.adjusted=adjusted;o.country=country;o.release_ceiling=capped;o.ceiling=ceiling;compare(o);}
    for(unsigned adjusted:{0u,1u})for(int standing:{0,80,250}){Options o;o.original_conversion=true;o.adjusted=adjusted;o.country=1;o.contract_policy=false;o.division_level=1;o.competition_standing=standing;o.club_score=0;if(!standing){o.potential=0;o.club_reputation=0;o.requested_score=0;o.ability=0;}compare(o);}
    for(int days:{-1,0,1,349,350,351,1049,1050,1750,32768})for(bool same_club:{false,true})for(int mood:{-1,0,1,100}){Options o;o.expiry=days;o.same_club=same_club;o.mood=mood;compare(o);}
    for(int role:{0,1,2,31,32,63})for(int bits:{0,1,2,3}){Options o;o.role=role;o.role_bits=bits;o.mood=1;compare(o);}
    for(bool player:{false,true})for(bool primary:{false,true})for(bool own:{false,true})for(int id:{-1,0,3}){Options o;o.player=player;o.primary=primary?0:-1;o.club_contract=own;o.person_id=id;compare(o);}
    for(bool policy:{false,true})for(bool contract:{false,true}){Options o;o.contract_policy=policy;o.club_contract=contract;compare(o);}
    for(unsigned state:{1u,2u})for(bool target:{false,true}){Options o;o.fp_state=state;o.target=target;compare(o);}
    for(int reputation:{-1,0,4999,5000,32767})for(int potential:{7000,9000})for(unsigned state:{0u,1u,2u}){Options o;o.original_conversion=true;o.club_reputation=reputation;o.potential=potential;o.fp_state=state;compare(o);}
    for(double value:{-1.0,0.0,0.5,1.0,2147483647.5,2147483648.0,2147483648.5,4294967295.5,4294967296.0,std::numeric_limits<double>::quiet_NaN()}){Options o;o.original_conversion=true;o.force_conversion_input=true;o.conversion_input=value;compare(o);}
    Options conversion_guard;conversion_guard.original_conversion=true;conversion_guard.force_conversion_input=true;conversion_guard.conversion_input=0.5;conversion_guard.fp_state=3;conversion_guard.expect_fp_guard=true;compare(conversion_guard);
    conversion_guard.conversion_input=2147483648.5;compare(conversion_guard);
    for(unsigned at:{1u,2u,3u,4u,5u,6u,7u}){Options o;o.interrupt=at;compare(o);}
    Options absent;absent.null_person=true;compare(absent);
    Options guard;guard.fp_state=3;guard.expect_fp_guard=true;guard.age=17;compare(guard);
    guard.age=24;guard.adjusted=1;guard.division_level=1;guard.competition_standing=9268;guard.club_score=0;compare(guard);
    Options mutation;mutation.corrupt=true;compare(mutation);mutation.corrupt=false;mutation.corrupt_assessment=true;mutation.club_reputation=1000;compare(mutation);mutation.corrupt_assessment=false;mutation.corrupt_conversion=true;compare(mutation);
    const auto continuations=contract_manager_base_value_calls();for(auto back:continuations)require(totals.returned_calls.contains(back),"Unreached completed base-value continuation "+hex_address(back));
    std::cout<<totals.cases<<" comparisons; "<<totals.returned<<" returned; "<<totals.partial<<" explicitly partial checks ("<<totals.null_faults<<" exact null-person faults,"<<totals.fp_guards<<" enabled-FP guards); "<<totals.negative<<" mutation controls detected; "<<totals.returned_calls.size()<<"/"<<continuations.size()<<" original continuations in completed calls\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
