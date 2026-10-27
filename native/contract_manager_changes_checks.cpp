#include "contract_manager.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <map>
#include <set>

namespace cm::aot {
bool contract_manager_changes_reference(State&);
std::vector<std::uint32_t> contract_manager_changes_calls();
}
namespace {
using namespace cm::semantic_io_checks; using cm::aot::guest::Cpu;
constexpr auto primary_entry=0x10083180u, secondary_entry=0x10083e50u, finish_entry=0x100845b0u;
constexpr auto object=fixture+0x1000, proxy=fixture+0x1100, person=fixture+0x2000, proposal=fixture+0x3000;
constexpr auto records=fixture+0x4000, globals=fixture+0x8000, indices=fixture+0x9000, clubs=fixture+0x10000;
constexpr auto context=fixture+0x20000, countries=fixture+0x22000, random=fixture+0x26000, pool=fixture+0x28000;
constexpr auto allocated=fixture+0x30000, today=fixture+0x38000, errors=fixture+0x39000;
struct Options {
    unsigned kind=11, type=11, role=2, category=2, low_bits=0, old_bits=3, count=2, capacity=3;
    int primary_index=0, secondary_index=1, free_index=-1, target_club=1, home_club=-1;
    int start=0, expiry=365, joined=-1000, roster=20, eligible_country=1, origin_country=2;
    unsigned cursor=0, salt=0, entropy=0, interrupt=0, final_state=16;
    bool empty_storage=false, human=false, attached=true, attach=true, kind_allowed=true, detach=true;
    bool final_attach=true, player=true, division=true, country_exempt=false, national_record=true;
    bool club_rule=false, direct=true, initial_affiliate=false, final_affiliate=false, private_flag=false;
    bool shortlist=true, roster_member=false, fail_allocation=false, observe_allocation_fault=false, reset_storage_on_attach=false, corrupt=false, corrupt_copy=false;
};
int ordinal(Cpu& c,std::uint32_t date){return int(c.u16(date+2))*365+int(c.i16(date));}
void put_date(Cpu& c,std::uint32_t date,int value){int year=value/365,day=value%365;if(day<0){day+=365;--year;}c.put16(date,std::uint32_t(day));c.put16(date+2,std::uint32_t(year));c.put8(date+4,0);}
unsigned arity(std::uint32_t target){switch(target){
    case 0x10768810:case 0x1077b770:return 0;
    case 0x10775130:case 0x10715880:case 0x100f4f80:case 0x100d12d0:return 1;
    case 0x107751b0:case 0x1030f950:case 0x100ca6a0:case 0x100ce200:case 0x10103510:case 0x100df6d0:case 0x10084b10:return 2;
    case 0x100c7dd0:case 0x1008bfc0:case 0x103d2b30:case 0x100c9070:case 0x10091320:case 0x10430790:case 0x10718130:case 0x10717890:case 0x10717a10:case 0x10717cb0:case 0x10771110:case 0x1037f050:case 0x100d3570:case 0x100ce320:case 0x10769b60:return 3;
    case 0x1014c2e0:case 0x10739890:case 0x100c8930:case 0x103afb90:case 0x1036d240:case 0x10365590:return 5;
    case 0x100c82a0:case 0x1077b720:return 6;
    case 0x10308350:return 7;
    default:throw std::runtime_error("Unknown change dependency "+hex_address(target));
}}
struct Event {std::uint32_t target,back;std::vector<std::uint32_t> args,kept;std::vector<std::uint64_t> fp;bool operator==(const Event&)const=default;};
struct Model {
    Options o;std::vector<Event> calls;
    explicit Model(Options options):o(std::move(options)){}
    void call(State& s,std::uint32_t target,std::uint32_t back){
        Cpu c(s);Event e{target,back,{},{},{}};for(unsigned n=0;n<arity(target);++n)e.args.push_back(c.r(n+3));
        for(unsigned n=13;n<32;++n)e.kept.push_back(c.r(n));for(unsigned n=14;n<32;++n)e.fp.push_back(s.r<8>(guest::reg::fpr(n)));calls.push_back(e);
        const auto first=c.r(3),second=c.r(4),third=c.r(5);std::uint32_t result=0;
        if(target==0x100c82a0){result=back==0x10083dd4?o.final_attach:o.attach;if(result){c.put32(second+0x24,first);c.put8(second+0x62,third);const auto date=c.r(6);s.memory.copy_in(second+0x4c,s.memory.copy_out(date,5));}if(o.reset_storage_on_attach){c.put32(object,0);c.put32(object+4,0);c.put32(object+8,0);}}
        if(target==0x100c7dd0)result=o.kind_allowed;
        if(target==0x1014c2e0)result=o.human;
        if(target==0x100c9070)result=std::uint32_t(o.roster);
        if(target==0x10718130){const auto start=int(std::uint16_t(second))*365+int(std::int16_t(second>>16));result=std::uint32_t(ordinal(c,first)-start);}
        if(target==0x10717890||target==0x10717a10||target==0x10717cb0){const auto amount=target==0x10717890?std::int32_t(third):int(std::uint8_t(third))*7;put_date(c,first,ordinal(c,second)+(target==0x10717cb0?-amount:amount));result=first;}
        if(target==0x100ce200){const auto flags=c.u8(first+0x6a);const auto mask=(second&3)?12u:3u;c.put8(first+0x6a,(flags&mask)|second);}
        if(target==0x10771110){for(unsigned n=0;n<third;++n)c.put8(first+n,c.u8(second+n));if(o.corrupt_copy)c.put8(first,c.u8(first)^0x40);result=first;}
        if(target==0x100ca6a0)result=o.country_exempt;
        if(target==0x10103510)result=o.national_record?context+0x500:0;
        if(target==0x100c8930){result=o.detach;if(result)c.put32(second+0x24,0);}
        if(target==0x10775130||target==0x107751b0){result=o.fail_allocation?0:allocated;if(result){const auto size=target==0x10775130?first:second;for(unsigned n=0;n<size;++n)c.put8(allocated+n,n*37+11);if(target==0x107751b0)s.memory.copy_in(allocated,s.memory.copy_out(first,o.count*100));}}
        if(target==0x10739890){c.put32(c.r(6),0x12345678);c.put32(c.r(7),0x9abcdef0);}
        if(target==0x10768810)result=0x1234;
        if(target==0x1077b720)for(unsigned n=0;n<6;++n)c.put32(first+n*4,c.r(n+3));
        if(target==0x10715880){put_date(c,first,2001*365+100);result=first;}
        if(target==0x100d12d0)result=o.club_rule;
        if(target==0x100ce320){
            if(second){c.put8(second,o.direct);result=o.initial_affiliate?clubs+3*0x248:0;}
            else result=(first==clubs+2*0x248?o.initial_affiliate:o.final_affiliate)?clubs+3*0x248:0;
        }
        if(target==0x10084b10){const auto index=c.i32(indices+4);if(index!=-1){c.put32(c.u32(first)+std::uint32_t(index)*100,0xffffffff);c.put32(indices+4,0xffffffff);}}
        if(target==0x1036d240)result=o.shortlist?context+0x600:0;
        if(target==0x1077b770){c.set(0,0);c.set(4,globals+0x108);c.set(3,o.entropy);s.pc=back;if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Change dependency interrupted");return;}
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);for(unsigned n=0;n<14;++n)c.set_f(n,-100.0-n);c.set_cr(0,2);c.set(3,result);s.pc=back;
        if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Change dependency interrupted");
        if(o.fail_allocation&&!o.observe_allocation_fault&&target==0x10769b60&&(back==0x10083af8||back==0x10084444))throw std::runtime_error("Allocation assertion boundary");
    }
};
thread_local Model* active=nullptr;struct Hook final:ContractManagerCallHook{bool call(State& s)override{active->call(s,s.pc,std::uint32_t(s.r<4>(4128)));return true;}}hook;
std::unique_ptr<State> seed(std::uint32_t entry,const Options& o){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x80000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);Cpu c(*s);c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,object);c.set(4,entry==finish_entry?person:proposal);c.set(5,o.kind);s->pc=entry;s->budget=1000000;
    unsigned slot_index=0;const auto holder=[&](int slot,std::uint32_t value){const auto at=globals+slot_index++*4;c.put32(toc+slot,at);c.put32(at,value);};
    holder(-0x7be0,person);holder(-0x7be8,clubs);holder(-0x7b98,countries);holder(-0x79fc,std::uint32_t(o.eligible_country));holder(-0x7ab8,context);holder(-0x7b90,o.salt);holder(-0x7b84,random+o.cursor);holder(-0x7b9c,indices);holder(-0x7b7c,proxy);holder(-0x7b78,0);holder(-0x7be4,3);holder(-0x7a6c,context+0x100);holder(-0x7b40,context+0x200);holder(-0x7b3c,context+0x300);holder(-0x7b1c,context+0x400);holder(-0x7a7c,context+0x700);holder(-0x7b20,context+0x800);holder(-0x7aa4,context+0x900);holder(-0x7b88,random+8);holder(-0x7b68,1);
    c.put32(toc-0x7bac,globals+0x200);c.put8(globals+0x200,1);c.put32(toc-0x7ba8,errors);c.put32(toc-0x5718,pool);c.put32(toc-0x7b80,today);c.put32(toc-0x7b8c,random);c.put32(context+0x200+0x988,context+0x1000);
    c.put32(object,o.empty_storage?0:records);c.put32(object+4,o.capacity);c.put32(object+8,o.count);c.put32(proxy,records);c.put32(proxy+4,o.capacity);c.put32(proxy+8,o.count);c.put32(indices,std::uint32_t(o.primary_index));c.put32(indices+4,std::uint32_t(o.secondary_index));
    const auto now=2001*365+100;put_date(c,today,now);
    for(unsigned n=0;n<std::max(o.count,2u);++n){const auto at=records+n*100;for(unsigned j=0;j<100;++j)c.put8(at+j,n*19+j*7+3);const auto owner=int(n)==o.free_index?0xffffffff:int(n)==o.primary_index||int(n)==o.secondary_index?0u:n+1;c.put32(at,owner);c.put32(at+4,n?2:1);c.put8(at+0x38,(2u<<2)|o.old_bits);c.put8(at+0x60,0xd3);c.put16(at+0x42,0x5a5a);}
    c.put32(person,0);c.put32(person+4,0x11111111);c.put32(person+12,countries+0x800);c.put32(person+0x10,0);c.put32(person+0x14,o.player?context+0x2000:0);
    const auto home=o.home_club>=0?o.home_club:entry==finish_entry?2:1;c.put32(person+0x24,o.attached?clubs+std::uint32_t(home)*0x248:0);c.put8(person+0x62,o.type);c.put8(person+0x6a,o.private_flag?1:0);c.put8(person+0x6d,o.final_state);put_date(c,person+0x4c,now+o.joined);
    for(unsigned n=0;n<100;++n)c.put8(proposal+n,n*31+7);c.put32(proposal,0);c.put32(proposal+4,std::uint32_t(o.target_club));c.put32(proposal+12,4321);put_date(c,proposal+0x28,now+o.start);put_date(c,proposal+0x30,now+o.expiry);c.put8(proposal+0x38,(o.role<<2)|o.low_bits);c.put8(proposal+0x60,0xb0|o.category);c.put16(proposal+0x42,0x8765);c.put8(proposal+0x54,1);c.put32(proposal+0x58,42);
    for(unsigned n=0;n<4;++n){const auto at=clubs+n*0x248;c.put32(at,n);c.put32(at+0x90,context+0x3000);c.put32(at+0x2c,o.division?context+0x4000:0);for(unsigned j=0;j<50;++j)c.put32(at+0x98+j*4,o.roster_member&&(j&1)?person:context);}
    c.put32(context+0x4000+0x68,std::uint32_t(o.eligible_country));c.put32(countries+0x800+0x68,std::uint32_t(o.origin_country));for(unsigned n=0;n<16;++n)c.put16(random+n*2,n);return s;
}
bool within(std::uint32_t pc){return pc>=primary_entry&&pc<0x10084a80;}
void reference(State& s,Model& model){while(s.pc!=return_sentinel){if(within(s.pc))require(contract_manager_changes_reference(s),"Complete original change body");else model.call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
void same_registers(const State& a,const State& b,std::uint32_t entry,const std::string& label){
    for(unsigned n=1;n<32;++n)if(n!=11&&n!=12&&(n!=3||entry==primary_entry))require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n)+" "+hex_address(a.r<4>(n*4))+"/"+hex_address(b.r<4>(n*4)));
    for(unsigned n=1;n<32;++n)if(n==1||n>=14)require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR");for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR");require(a.r<1>(guest::reg::xer_ca)==b.r<1>(guest::reg::xer_ca),label+" carry");require(a.r<4>(guest::reg::lr)==b.r<4>(guest::reg::lr),label+" LR");
}
struct Totals{unsigned cases=0,returned=0,partial=0,negative=0,faults=0;std::set<std::uint32_t> returned_calls,partial_calls;};std::map<std::uint32_t,Totals> totals;
void compare(std::uint32_t entry,Options o={}){
    if(entry==primary_entry&&o.free_index==o.secondary_index)o.secondary_index=-1;
    if(entry==secondary_entry&&o.free_index==o.secondary_index)o.free_index=-1;
    auto source=seed(entry,o),a=clone(*source),b=clone(*source);Model old(o),modern(o);old.o.corrupt_copy=false;
    const auto run=[&](State& s,Model& model,bool original){try{if(original)reference(s,model);else{const auto previous=contract_manager_call_hook;active=&model;contract_manager_call_hook=&hook;try{Cpu c(s);if(entry==primary_entry)c.set(3,cm::aot::contract_manager::apply_primary_contract(c,c.r(3),c.r(4),c.r(5)));else if(entry==secondary_entry)cm::aot::contract_manager::apply_secondary_contract(c,c.r(3),c.r(4));else cm::aot::contract_manager::finish_secondary_contract(c,c.r(3),c.r(4));c.finish();}catch(...){contract_manager_call_hook=previous;active=nullptr;throw;}contract_manager_call_hook=previous;active=nullptr;}return std::string{};}catch(const std::exception& e){return std::string(e.what());}};
    auto& t=totals[entry];const auto expected=run(*a,old,true),actual=run(*b,modern,false),label=hex_address(entry)+" change case "+std::to_string(t.cases);
    const bool allocation_fault=o.fail_allocation&&o.observe_allocation_fault&&expected.starts_with("Unmapped data access at ");
    require(expected.empty()||expected=="Change dependency interrupted"||expected=="Allocation assertion boundary"||allocation_fault,label+" invalid reference "+expected);require(expected==actual,label+" outcomes "+expected+"/"+actual);
    if(o.interrupt)require(expected=="Change dependency interrupted"&&old.calls.size()==o.interrupt,label+" interruption reached");
    else if(o.observe_allocation_fault){
        require(allocation_fault,label+" original null-write fault reached");Cpu c(*a);
        const auto previous=o.reset_storage_on_attach||o.empty_storage?0u:o.count;
        require(expected=="Unmapped data access at "+hex_address(previous*100),label+" first null-row store address");
        require(c.u8(c.toc(-0x7bac))==0&&c.u32(object)==0,label+" allocation assertion state");
        require(c.u32(object+8)==previous+1&&c.u32(indices+(entry==primary_entry?0:4))==previous,label+" append state reached before null write");
    }else if(o.fail_allocation)require(expected=="Allocation assertion boundary",label+" allocation boundary reached");else require(expected.empty(),label+" complete return");
    if(o.corrupt)b->memory.write<1>(person,b->memory.read<1>(person)^0x5a);bool rejected=false;
    try{same_registers(*a,*b,entry,label);if(!allocation_fault)require(a->pc==b->pc,label+" PC");for(auto [address,size]:a->memory.mapped_ranges()){const auto x=a->memory.copy_out(address,size),y=b->memory.copy_out(address,size);if(x!=y){const auto at=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(address+std::uint32_t(at.first-x.begin()))+" "+std::to_string(*at.first)+"/"+std::to_string(*at.second));}}
        if(old.calls!=modern.calls){const auto at=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());std::string d;if(at.first!=old.calls.end()&&at.second!=modern.calls.end()){const auto& x=*at.first;const auto& y=*at.second;d=" targets "+hex_address(x.target)+"/"+hex_address(y.target)+" backs "+hex_address(x.back)+"/"+hex_address(y.back);for(unsigned n=0;n<std::min(x.args.size(),y.args.size());++n)if(x.args[n]!=y.args[n])d+=" arg"+std::to_string(n+3)+" "+hex_address(x.args[n])+"/"+hex_address(y.args[n]);for(unsigned n=0;n<x.kept.size();++n)if(x.kept[n]!=y.kept[n])d+=" r"+std::to_string(n+13)+" "+hex_address(x.kept[n])+"/"+hex_address(y.kept[n]);}throw std::runtime_error(label+" call "+std::to_string(at.first-old.calls.begin())+d);}
    }catch(const std::runtime_error&){if(!o.corrupt&&!o.corrupt_copy)throw;rejected=true;}
    require(rejected==(o.corrupt||o.corrupt_copy),label+" negative control");++t.cases;t.returned+=expected.empty()&&!rejected;t.partial+=!expected.empty();t.negative+=rejected;t.faults+=allocation_fault;if(!rejected)for(const auto& e:old.calls)(expected.empty()?t.returned_calls:t.partial_calls).insert(e.back);
}
}
int main(){try{
    for(auto entry:{primary_entry,secondary_entry,finish_entry})compare(entry);
    for(int index:{-1,0})for(int free:{-1,1})for(unsigned capacity:{2u,3u})for(unsigned kind:{0u,1u,2u,3u,4u,5u,6u,7u,8u,15u}){Options o;o.primary_index=index;o.free_index=free;o.capacity=capacity;o.category=kind;compare(primary_entry,o);}
    for(unsigned role:{1u,2u,5u,32u,63u})for(unsigned bits:{0u,1u,2u,3u})for(int target:{1,2}){Options o;o.role=role;o.low_bits=bits;o.target_club=target;compare(primary_entry,o);}
    for(int delta:{-1,0,1,28,365}){Options o;o.expiry=delta;compare(primary_entry,o);}
    for(int roster:{-1,49,50,51}){Options o;o.role=5;o.roster=roster;compare(primary_entry,o);}
    for(bool attached:{false,true})for(bool attach:{false,true})for(bool permitted:{false,true}){Options o;o.attached=attached;o.attach=attach;o.kind_allowed=permitted;o.kind=12;compare(primary_entry,o);}
    for(unsigned cursor:{0u,8u})for(unsigned salt:{0u,1u,2u,65535u})for(bool exempt:{false,true}){Options o;o.cursor=cursor;o.salt=salt;o.country_exempt=exempt;compare(primary_entry,o);}
    for(int joined:{-701,-700,-699})for(int origin:{1,2}){Options o;o.joined=joined;o.origin_country=origin;compare(primary_entry,o);}
    for(bool division:{false,true})for(bool national:{false,true})for(bool player:{false,true}){Options o;o.division=division;o.national_record=national;o.player=player;compare(primary_entry,o);}
    for(int duration:{-32769,-32768,32767,32768,65535,65536}){Options o;o.role=5;o.expiry=duration;compare(primary_entry,o);}
    for(int index:{-1,1})for(int free:{-1,1})for(unsigned capacity:{2u,3u}){Options o;o.secondary_index=index;o.free_index=free;o.capacity=capacity;compare(secondary_entry,o);}
    for(bool rule:{false,true})for(bool private_flag:{false,true})for(bool affiliate:{false,true}){Options o;o.club_rule=rule;o.private_flag=private_flag;o.final_affiliate=affiliate;compare(secondary_entry,o);}
    for(bool attached:{false,true})for(int index:{-1,0})for(bool rule:{false,true})for(bool affiliate:{false,true}){Options o;o.attached=attached;o.primary_index=index;o.club_rule=rule;o.final_affiliate=affiliate;o.roster_member=affiliate;compare(finish_entry,o);}
    for(bool affiliate:{false,true})for(bool direct:{false,true}){Options o;o.initial_affiliate=affiliate;o.direct=direct;compare(finish_entry,o);}
    for(bool selected:{false,true}){Options o;o.shortlist=selected;compare(finish_entry,o);}
    for(unsigned state:{0u,4u,15u,16u,17u,255u}){Options o;o.final_state=state;compare(finish_entry,o);}
    Options human;human.human=true;compare(primary_entry,human);
    Options detach;detach.kind=12;detach.detach=false;compare(primary_entry,detach);detach.detach=true;detach.final_attach=false;compare(primary_entry,detach);
    Options missing;missing.empty_storage=true;missing.count=0;missing.capacity=0;compare(secondary_entry,missing);
    for(unsigned bits:{0u,1u,2u,3u}){Options o;o.club_rule=true;o.old_bits=bits;compare(finish_entry,o);}
    Options fresh;fresh.primary_index=-1;fresh.secondary_index=-1;fresh.count=0;fresh.capacity=0;fresh.empty_storage=true;compare(primary_entry,fresh);
    fresh.primary_index=0;fresh.count=2;fresh.capacity=2;fresh.empty_storage=false;fresh.reset_storage_on_attach=true;compare(secondary_entry,fresh);
    for(auto entry:{primary_entry,secondary_entry})for(bool empty:{false,true}){Options o;o.primary_index=entry==primary_entry?-1:0;o.secondary_index=-1;o.count=empty&&entry==primary_entry?0:2;o.capacity=o.count;o.empty_storage=empty&&entry==primary_entry;o.reset_storage_on_attach=empty&&entry==secondary_entry;o.fail_allocation=true;compare(entry,o);}
    for(auto entry:{primary_entry,secondary_entry})for(bool empty:{false,true}){Options o;o.primary_index=entry==primary_entry?-1:0;o.secondary_index=-1;o.count=empty&&entry==primary_entry?0:2;o.capacity=o.count;o.empty_storage=empty&&entry==primary_entry;o.reset_storage_on_attach=empty&&entry==secondary_entry;o.fail_allocation=true;o.observe_allocation_fault=true;compare(entry,o);}
    for(auto entry:{primary_entry,secondary_entry,finish_entry}){Options o;o.interrupt=1;compare(entry,o);o.interrupt=4;compare(entry,o);o.interrupt=0;o.corrupt=true;compare(entry,o);if(entry!=finish_entry){o.corrupt=false;o.corrupt_copy=true;compare(entry,o);}}
    const auto calls=contract_manager_changes_calls();for(const auto& [entry,t]:totals){unsigned expected=0,completed=0,partial_only=0;const auto limit=entry==primary_entry?secondary_entry:entry==secondary_entry?finish_entry:0x10084a80u;
        for(auto back:calls)if(back>entry&&back<limit){++expected;require(t.returned_calls.contains(back)||t.partial_calls.contains(back),"Unreached change continuation "+hex_address(back));completed+=t.returned_calls.contains(back);partial_only+=!t.returned_calls.contains(back)&&t.partial_calls.contains(back);}
        std::cout<<hex_address(entry)<<": "<<t.cases<<" comparisons; "<<t.returned<<" returned; "<<t.partial<<" partial checks ("<<t.faults<<" expected original null-write faults); "<<t.negative<<" negative controls detected; "<<completed<<"/"<<expected<<" continuations in returned cases, "<<partial_only<<" additional only in explicitly partial cases\n";
    }return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
