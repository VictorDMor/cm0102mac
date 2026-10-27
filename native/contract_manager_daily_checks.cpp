#include "contract_manager.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {
bool contract_manager_daily_reference(State&);
std::vector<std::uint32_t> contract_manager_daily_calls();
void contract_manager_daily_constants(State&, std::uint32_t, std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
using cm::aot::guest::Cpu;
constexpr auto entry=0x10079050u, end=0x1007bb70u;
constexpr auto object=fixture+0x1000, transfer=fixture+0x7000, shortlist=fixture+0x7800, division=fixture+0x7900;
constexpr auto globals=fixture+0x8000, proxy=fixture+0x9000, people=fixture+0x10000;
constexpr auto contracts=fixture+0x20000, attributes=fixture+0x40000, clubs=fixture+0x50000;
constexpr auto indices=fixture+0x60000, constants=fixture+0x70000, today=fixture+0x72000, dates=fixture+0x73000;
constexpr auto random=fixture+0x75000, pool=fixture+0x76000, relations=fixture+0x78000;
struct Options {
    unsigned phase=2, scan=1, count=1, category=1, role=2, role_bits=0, secondary_role=2, type=11, interrupt=0, cursor=0, seed=0, entropy=0;
    int day=100, expiry=100, start=-200, secondary_expiry=100, renewal=0, status=0, age=25, ambition=15;
    int reputation=6000, international=2000, ability=120, club_quality=5000, value=123456, maximum=120000;
    unsigned offer=1, flags=0, keyed=0; std::vector<int> statuses;
    bool annual=false, running=true, player=true, no_primary=false, no_employer=false, has_secondary=false;
    bool human=false, human_value=false, national=false, playing=true, selected=true, registered=false;
    bool relation=false, ready=false, possible=true, pending=false, other_offer=false, bid=false, listing=false;
    bool risk=false, division_work=false, no_club=false, no_division=false, owner=true, active=false, cap=false;
    bool dirty=false, index_secondary=true, special_day=false, retire=false, corrupt=false, corrupt_offer=false;
};
int ordinal(Cpu& c,std::uint32_t date) {return int(c.u16(date+2))*365+int(c.i16(date));}
void put_date(Cpu& c,std::uint32_t date,int value) {
    int year=value/365,day=value%365;if(day<0){day+=365;--year;}
    c.put16(date,std::uint32_t(day));c.put16(date+2,std::uint32_t(year));c.put8(date+4,0);
}
unsigned arity(std::uint32_t target) {
    switch(target) {
    case 0x100c7a20:case 0x10380520:case 0x1037f110:case 0x10380710:case 0x100d33d0:case 0x103721f0:case 0x10715880:case 0x10080840:return 1;
    case 0x10727890:case 0x10082f20:case 0x10083110:case 0x100845b0:case 0x100d94d0:case 0x100d9580:case 0x102de3c0:case 0x102cdbf0:case 0x100c6a10:case 0x103faff0:case 0x103e6470:case 0x100d3040:case 0x10084a80:return 2;
    case 0x10717890:case 0x10717b20:case 0x10739ef0:case 0x10718130:case 0x103eb020:case 0x103f43b0:case 0x10363ca0:case 0x1008a350:case 0x10080bb0:case 0x1008c8d0:return 3;
    case 0x10768300:case 0x10090a00:case 0x10380f20:case 0x103eaea0:case 0x103e6290:return 4;
    case 0x107156f0:case 0x10743120:case 0x1014c2e0:case 0x10080d30:case 0x1008c270:case 0x1036d240:case 0x103afb90:case 0x100c8930:return 5;
    case 0x10084be0:case 0x103e68c0:return 6;
    case 0x10308350:return 7;
    case 0x1077b770:return 0;
    default:throw std::runtime_error("Unknown daily dependency "+hex_address(target));
    }
}
struct Event {std::uint32_t target,back;std::vector<std::uint32_t> args,kept;std::vector<std::uint64_t> fp;bool operator==(const Event&)const=default;};
struct Model {
    Options o;std::vector<Event> calls;unsigned queried_status=0;
    explicit Model(Options options):o(std::move(options)){}
    void call(State& s,std::uint32_t target,std::uint32_t back) {
        Cpu c(s);Event e{target,back,{},{},{}};
        for(unsigned n=0;n<arity(target);++n)e.args.push_back(c.r(n+3));
        if(target==0x10080d30)e.args.push_back(c.r(10));
        for(unsigned n=13;n<32;++n)e.kept.push_back(c.r(n));
        e.fp.push_back(s.r<8>(guest::reg::fpr(1)));for(unsigned n=14;n<32;++n)e.fp.push_back(s.r<8>(guest::reg::fpr(n)));
        calls.push_back(e);const auto first=c.r(3),second=c.r(4),third=c.r(5);std::uint32_t result=0;
        if(target==0x107156f0){put_date(c,first,int(c.r(6))*365+364);c.put32(first+4,0);result=first;}
        if(target==0x10717890||target==0x10717b20){const auto delta=std::int32_t(third);put_date(c,first,ordinal(c,second)+(target==0x10717890?delta:-delta));result=first;}
        if(target==0x10739ef0)s.memory.copy_in(first,s.memory.copy_out(second,third));
        if(target==0x10082f20||target==0x10083110){const auto id=c.u32(second);const auto index=c.i32(indices+id*100+(target==0x10083110?4:0));result=index<0?0:c.u32(first)+std::uint32_t(index)*100;}
        if(target==0x100845b0)c.put32(indices+c.u32(second)*100+4,0xffffffff);
        if(target==0x10084a80)c.put32(indices+c.u32(second)*100,0xffffffff);
        if(target==0x1014c2e0)result=second==1?o.human_value:o.human;
        if(target==0x100c7a20)result=o.playing;
        if(target==0x10084be0)result=std::uint32_t(o.value);
        if(target==0x10380520)result=o.cap;
        if(target==0x10718130){const auto other=int(std::uint16_t(second))*365+int(std::int16_t(second>>16));result=std::uint32_t(ordinal(c,first)-other);}
        if(target==0x100d94d0||target==0x100d9580)result=o.relation;
        if(target==0x10080d30)result=o.corrupt_offer?0:o.offer;
        if(target==0x10380f20){c.put8(third,o.possible);result=0;}
        if(target==0x102de3c0)result=o.national;
        if(target==0x102cdbf0)result=o.registered;
        if(target==0x100c6a10)result=std::uint32_t(o.age);
        if(target==0x103faff0)result=o.risk;
        if(target==0x1037f110)result=o.ready;
        if(target==0x103eaea0)result=o.other_offer;
        if(target==0x103eb020)result=o.bid;
        if(target==0x103e6470)result=o.pending;
        if(target==0x103f43b0)result=o.listing;
        if(target==0x10380710){result=std::uint32_t(queried_status<o.statuses.size()?o.statuses[queried_status]:o.status);++queried_status;}
        if(target==0x100d33d0)result=o.division_work;
        if(target==0x1036d240)result=o.selected?attributes:0;
        if(target==0x103721f0){for(unsigned n=0;n<100;++n)c.put8(first+n,n*31+7);result=first;}
        if(target==0x10715880){put_date(c,first,2001*365+100);result=first;}
        if(target==0x100d3040)result=o.retire;
        if(target==0x1077b770){c.set(0,0);c.set(4,globals+0x100);c.set(3,o.entropy);s.pc=back;if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Daily dependency interrupted");return;}
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);
        for(unsigned n=0;n<14;++n)c.set_f(n,-100.0-n);c.set_cr(0,2);c.set(3,result);s.pc=back;
        if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Daily dependency interrupted");
    }
};
thread_local Model* active=nullptr;
struct Hook final:ContractManagerCallHook {bool call(State& s)override{active->call(s,s.pc,std::uint32_t(s.r<4>(4128)));return true;}}hook;
std::unique_ptr<State> seed(const Options& o) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x100000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);
    Cpu c(*s);c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,object);c.set(4,today);c.set(5,o.phase);c.set(6,o.scan);s->pc=entry;s->budget=10000000;
    unsigned global=0;
    const auto holder=[&](int slot,std::uint32_t value){const auto at=globals+global++*4;c.put32(toc+slot,at);c.put32(at,value);};
    holder(-0x7be0,people);holder(-0x7be4,o.count);holder(-0x7b40,transfer);holder(-0x7bf0,attributes+0x8000);holder(-0x7b28,attributes+0x8100);
    holder(-0x7b78,0);holder(-0x7be8,clubs);holder(-0x7b9c,indices);holder(-0x7b7c,proxy);holder(-0x7b84,random+o.cursor);holder(-0x7b88,random+8);holder(-0x7b90,o.seed);
    holder(-0x7b3c,attributes+0x8200);holder(-0x7a6c,attributes+0x8300);
    c.put32(toc-0x7b8c,random);c.put32(toc-0x79e4,globals+0x200);c.put8(globals+0x200,o.running);
    c.put32(toc-0x7bb8,globals+0x300);c.put32(toc-0x7bb0,globals+0x400);c.put32(toc-0x5718,pool);
    const auto now=2001*365+(o.annual?364:o.day);put_date(c,today,now);
    const std::pair<int,int> horizons[]={{-0x79e8,o.renewal},{-0x79d8,91},{-0x79b8,365},{-0x79b0,152},{-0x79ac,121},{-0x79a4,60},{-0x79a0,30},{-0x799c,7},{-0x79a8,91},{-0x79b4,182},{-0x79bc,550},{-0x79c0,1095},{-0x79c4,7},{-0x79c8,30},{-0x79cc,42},{-0x79d0,60},{-0x79d4,70},{-0x7a9c,o.special_day?0:-1}};
    unsigned date_index=0;for(auto [slot,delta]:horizons){const auto at=dates+date_index++*16;c.put32(toc+slot,at);put_date(c,at,now+delta);}
    c.put32(object,contracts);c.put32(object+4,o.count*2+2);c.put32(object+8,o.count*2);c.put32(proxy,o.no_employer?0:contracts);
    c.put32(object+0x28,relations);c.put32(object+0x2c,o.keyed);c.put32(object+0x1c,o.dirty);c.put32(object+0x18,1);c.put32(object+0x14,3);c.put32(object+12,relations+0x1000);
    for(unsigned n=0;n<o.keyed;++n){const auto at=relations+n*24;c.put32(at,n);c.put32(at+8,0);put_date(c,at+12,now-(n?60:0));}
    c.put32(transfer+0x988,shortlist);
    for(unsigned n=0;n<o.count;++n){const auto at=people+n*112,row=contracts+n*200,extra=row+100,stats=attributes+n*0x100;
        c.put32(at,n);c.put32(at+8,99999);c.put32(at+0x14,o.player?stats:0);c.put32(at+0x24,o.no_club?0:clubs+0x248);c.put16(at+0x5c,std::uint32_t(o.age));c.put8(at+0x62,o.type);c.put8(at+0x65,std::uint32_t(o.ambition));
        c.put32(row,o.no_primary?0xffffffff:n);c.put32(row+4,1);put_date(c,row+0x28,now+o.start);put_date(c,row+0x30,now+o.expiry);c.put8(row+0x38,(o.role<<2)|o.role_bits);c.put8(row+0x54,o.active);c.put8(row+0x5c,o.flags);c.put8(row+0x60,o.category);c.put32(row+0x24,std::uint32_t(o.maximum));
        c.put32(extra,o.has_secondary?n:0xffffffff);c.put32(extra+4,2);put_date(c,extra+0x30,now+o.secondary_expiry);c.put8(extra+0x38,o.secondary_role<<2);
        c.put32(indices+n*100,o.no_primary?0xffffffff:n*2);c.put32(indices+n*100+4,o.has_secondary&&o.index_secondary?n*2+1:0xffffffff);c.put16(stats+6,std::uint32_t(o.ability));c.put16(stats+10,std::uint32_t(o.reputation));c.put16(stats+12,std::uint32_t(o.international));
    }
    for(unsigned n=0;n<4;++n){const auto club=clubs+n*0x248;c.put32(club,n);c.put32(club+0x90,o.owner?attributes:0);c.put32(club+0x2c,o.no_division?0:division);c.put16(club+0x1ec,std::uint32_t(o.club_quality));for(unsigned r=0;r<50;++r)c.put32(club+0x98+r*4,r&1?people:attributes);}
    for(unsigned n=0;n<16;++n)c.put16(random+n*2,n);
    contract_manager_daily_constants(*s,toc,constants);return s;
}
void reference(State& s,Model& model){while(s.pc!=return_sentinel){if(s.pc>=entry&&s.pc<end)require(contract_manager_daily_reference(s),"Complete original daily body");else model.call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
void same_registers(const State& a,const State& b,const std::string& label) {
    for(unsigned n=1;n<32;++n)if(n!=3&&n!=11&&n!=12)require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n)+" "+hex_address(a.r<4>(n*4))+"/"+hex_address(b.r<4>(n*4)));
    for(unsigned n=1;n<32;++n)if(n==1||n>=14)require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));
    for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR"+std::to_string(n-1280));
    for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR");
    require(a.r<1>(guest::reg::xer_ca)==b.r<1>(guest::reg::xer_ca),label+" carry");require(a.r<4>(guest::reg::lr)==b.r<4>(guest::reg::lr),label+" LR");
}
unsigned cases=0,returned=0,stopped=0,negative=0;std::set<std::uint32_t> reached;
void compare(Options o={}) {
    auto source=seed(o),a=clone(*source),b=clone(*source);Model old(o),modern(o);old.o.corrupt_offer=false;
    const auto run=[&](State& s,Model& model,bool original){try{if(original)reference(s,model);else{const auto previous=contract_manager_call_hook;contract_manager_call_hook=&hook;active=&model;try{Cpu c(s);cm::aot::contract_manager::process_daily_contracts(c,c.r(3),c.r(4),c.r(5),c.r(6));c.finish();}catch(...){contract_manager_call_hook=previous;active=nullptr;throw;}contract_manager_call_hook=previous;active=nullptr;}return std::string{};}catch(const std::exception& e){return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false),label="daily case "+std::to_string(cases);
    require(expected.empty()||expected=="Daily dependency interrupted",label+" invalid reference "+expected);require(expected==actual,label+" outcomes "+expected+"/"+actual);
    if(o.interrupt)require(expected=="Daily dependency interrupted"&&old.calls.size()==o.interrupt,label+" interruption reached");else require(expected.empty(),label+" complete return");
    if(o.corrupt)b->memory.write<1>(object,b->memory.read<1>(object)^0x5a);
    bool rejected=false;
    try {same_registers(*a,*b,label);require(a->pc==b->pc,label+" PC");
        for(auto [address,size]:a->memory.mapped_ranges()){const auto x=a->memory.copy_out(address,size),y=b->memory.copy_out(address,size);if(x!=y){const auto at=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(address+std::uint32_t(at.first-x.begin()))+" "+std::to_string(*at.first)+"/"+std::to_string(*at.second));}}
        if(old.calls!=modern.calls){const auto at=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());std::string detail;
            if(at.first!=old.calls.end()&&at.second!=modern.calls.end()){const auto& x=*at.first;const auto& y=*at.second;detail=" targets "+hex_address(x.target)+"/"+hex_address(y.target)+" backs "+hex_address(x.back)+"/"+hex_address(y.back);for(unsigned n=0;n<std::min(x.args.size(),y.args.size());++n)if(x.args[n]!=y.args[n])detail+=" arg"+std::to_string(n+3)+" "+hex_address(x.args[n])+"/"+hex_address(y.args[n]);for(unsigned n=0;n<19;++n)if(x.kept[n]!=y.kept[n])detail+=" r"+std::to_string(n+13)+" "+hex_address(x.kept[n])+"/"+hex_address(y.kept[n]);for(unsigned n=0;n<std::min(x.fp.size(),y.fp.size());++n)if(x.fp[n]!=y.fp[n])detail+=" fp"+std::to_string(n);}
            throw std::runtime_error(label+" call "+std::to_string(at.first-old.calls.begin())+detail);}
    }catch(const std::runtime_error&){if(!o.corrupt&&!o.corrupt_offer)throw;rejected=true;}
    require(rejected==(o.corrupt||o.corrupt_offer),label+" negative control");++cases;returned+=expected.empty()&&!rejected;stopped+=!expected.empty();negative+=rejected;
    if(expected.empty()&&!rejected)for(const auto& e:old.calls)reached.insert(e.back);
}
}
int main(){try {
    compare();
    for(unsigned phase:{0u,1u,2u,255u})for(unsigned scan:{0u,1u,255u}){Options o;o.phase=phase;o.scan=scan;compare(o);}
    for(unsigned count:{0u,2u,251u}){Options o;o.count=count;o.annual=true;o.running=false;compare(o);}
    for(int state:{1,3,5,7,9,11,13})for(unsigned category=0;category<=7;++category)for(bool selected:{false,true})for(unsigned offer:{0u,1u}){Options o;o.status=state;o.category=category;o.selected=selected;o.offer=offer;compare(o);}
    for(int expiry:{-40,-31,-1,0,1,100,182,183,365,550,1095})for(unsigned role:{1u,2u,5u}){Options o;o.expiry=expiry;o.role=role;compare(o);}
    for(int renewal:{7,30,42,60,70,91})for(unsigned category:{1u,2u,3u,5u}){Options o;o.renewal=renewal;o.expiry=200;o.category=category;compare(o);}
    for(unsigned cursor:{0u,6u,8u})for(unsigned salt:{0u,19u,20u,65535u}){Options o;o.cursor=cursor;o.seed=salt;o.statuses={1,1,14};compare(o);}
    for(bool human:{false,true})for(bool player:{false,true})for(bool national:{false,true}){Options o;o.human=human;o.player=player;o.national=national;compare(o);}
    for(bool human:{false,true})for(bool playing:{false,true})for(unsigned day:{100u,101u}){Options o;o.human_value=human;o.playing=playing;o.day=int(day);o.flags=4;o.cap=true;compare(o);}
    for(bool absent:{false,true})for(bool club:{false,true}){Options o;o.no_primary=absent;o.no_club=!club;o.has_secondary=true;o.secondary_expiry=0;compare(o);}
    for(int expiry:{0,365,550})for(bool selected:{false,true})for(unsigned offer:{0u,1u}){Options o;o.has_secondary=true;o.expiry=expiry;o.selected=selected;o.offer=offer;compare(o);}
    for(int reputation:{6750,6751})for(int age:{23,24})for(unsigned kind:{2u,3u}){Options o;o.renewal=7;o.expiry=200;o.category=kind;o.reputation=reputation;o.age=age;o.registered=true;compare(o);}
    for(unsigned type:{11u,12u,13u,15u})for(bool human:{false,true}){Options o;o.role=1;o.expiry=-40;o.ready=true;o.type=type;o.human=human;o.registered=true;compare(o);}
    for(int state:{1,7,13})for(bool player:{false,true})for(bool registered:{false,true}){Options o;o.status=state;o.player=player;o.registered=registered;o.risk=true;compare(o);}
    for(int state=0;state<=14;++state){Options o;o.active=true;o.status=state;compare(o);}
    for(unsigned category:{1u,3u,6u,7u})for(bool selected:{false,true}){Options o;o.has_secondary=true;o.expiry=0;o.category=category;o.selected=selected;o.retire=true;compare(o);}
    for(unsigned category:{1u,2u,3u,5u,6u}){Options o;o.player=false;o.expiry=0;o.category=category;compare(o);}
    for(bool ready:{false,true}){Options o;o.expiry=0;o.role=1;o.active=true;o.ready=ready;compare(o);}
    for(int age:{18,19,22,23,30,31}){Options o;o.status=1;o.ambition=14;o.age=age;o.division_work=true;o.ready=true;compare(o);}
    for(unsigned flag:{0u,8u}){Options o;o.expiry=0;o.flags=flag;compare(o);}
    for(int day:{36,200}){Options o;o.day=day;compare(o);}
    Options special;special.has_secondary=true;special.special_day=true;special.expiry=300;special.offer=0;compare(special);
    Options no_employer;no_employer.no_employer=true;compare(no_employer);Options no_division;no_division.no_division=true;no_division.status=1;compare(no_division);
    for(unsigned role:{7u,32u,63u}){Options o;o.has_secondary=true;o.secondary_expiry=0;o.secondary_role=role;compare(o);o.has_secondary=false;o.role=role;o.role_bits=2;compare(o);}
    for(unsigned guard=0;guard<5;++guard){Options o;o.role=1;o.expiry=-40;o.ready=true;if(guard==0)o.other_offer=true;if(guard==1)o.bid=true;if(guard==2)o.pending=true;if(guard==3)o.listing=true;if(guard==4)o.possible=false;compare(o);}
    for(unsigned count:{249u,250u}){Options o;o.count=count;o.annual=true;o.running=false;compare(o);}
    Options keyed;keyed.keyed=3;compare(keyed);keyed.dirty=true;compare(keyed);
    for(unsigned cut:{1u,4u,8u,20u}){Options o;o.interrupt=cut;compare(o);}
    Options bad;bad.corrupt=true;compare(bad);bad.corrupt=false;bad.corrupt_offer=true;bad.status=1;compare(bad);
    const auto calls=contract_manager_daily_calls();for(auto back:calls)require(reached.contains(back),"Unreached daily continuation "+hex_address(back));
    std::cout<<cases<<" daily comparisons; "<<returned<<" returned; "<<stopped<<" partial model stops; "<<negative<<" negative controls detected; "<<reached.size()<<"/"<<calls.size()<<" original dependency continuations reached\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
