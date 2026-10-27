#include "contract_manager.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <cmath>
#include <limits>
#include <set>

namespace cm::aot {
bool contract_manager_preparation_reference(State&);
std::vector<std::uint32_t> contract_manager_preparation_calls();
void contract_manager_preparation_constants(State&, std::uint32_t, std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
using cm::aot::guest::Cpu;
constexpr auto entry=0x10080d30u, end=0x10081b10u;
constexpr auto object=fixture+0x1000,person=fixture+0x2000,club=fixture+0x3000,output=fixture+0x4000;
constexpr auto contract=fixture+0x5000,attributes=fixture+0x6000,transfer=fixture+0x7000,shortlist=fixture+0x8000;
constexpr auto indices=fixture+0x9000,globals=fixture+0xa000,national=fixture+0xb000,origin_offer=fixture+0xc000;
constexpr auto injury=fixture+0xd000,table=fixture+0x18000;
struct Options {
    unsigned guard=0,role=2,ability=4000,valuation=6000000,weekly=1000,maximum=1000,proposal_year=2006;
    unsigned duration=2000,interrupt=0,mode=0;
    unsigned current_year=2000,squad_role=0;
    int age=35,bonus=100,guarantee=0,trait=15;
    std::int32_t proposal_result=1;
    bool derive=false,no_primary=false,existing=false,player=false,source=false,copy=true,limited=false,injured=false;
    bool alternative=false,secondary_floor=false,corrupt=false;
    bool special_position=false;
    bool injury_absent=false;
    double scale=1;
};
struct Event {
    std::uint32_t target,back;
    std::vector<std::uint32_t> arguments,kept;
    std::vector<std::uint64_t> fp;
    bool operator==(const Event&) const=default;
};
unsigned arity(std::uint32_t target) {
    switch(target) {
    case 0x103721f0: case 0x100c7d00: case 0x100c7a20: case 0x100d3c40: return 1;
    case 0x10082f20: case 0x103eb440: case 0x10103510: case 0x100f4790: case 0x1018c340: case 0x1037f830: return 2;
    case 0x10082f90: case 0x103f43b0: case 0x100f2680: case 0x10718130: case 0x10717890: case 0x10771110: return 3;
    case 0x1036d510:return 4;
    case 0x1014c2e0:return 5;
    case 0x10416840:return 6;
    case 0x10374f50:case 0x1007f200:return 8;
    case 0x10780fc0:return 0;
    default:throw std::runtime_error("Unknown preparation dependency "+hex_address(target));
    }
}
struct Model {
    Options o;
    std::vector<Event> calls;
    explicit Model(Options options):o(options){}
    void call(State& s,std::uint32_t target,std::uint32_t back) {
        Cpu c(s); Event e{target,back,{},{},{}};
        for(unsigned n=0;n<arity(target);++n)e.arguments.push_back(c.r(3+n));
        for(unsigned n=19;n<32;++n)e.kept.push_back(c.r(n));
        for(unsigned n=29;n<32;++n)e.fp.push_back(s.r<8>(guest::reg::fpr(n)));
        if(target==0x10780fc0){e.fp.push_back(s.r<8>(guest::reg::fpr(1)));e.fp.push_back(s.r<8>(guest::reg::fpr(2)));}
        calls.push_back(e);
        const auto first=c.r(3),second=c.r(4),third=c.r(5);std::uint32_t result=0;
        if(target==0x103721f0) {
            for(unsigned n=0;n<100;++n)c.put8(first+n,n*31+11);
            c.put32(first+4,0xffffffff);c.put8(first+0x54,0);c.put32(first+0x58,0xffffffff);
            c.put16(first+0x42,0);c.put8(first+0x5c,0);c.put32(first+0x48,0);c.put32(first+0x50,0);c.put32(first+8,0);result=first;
        }
        if(target==0x10082f20)result=o.no_primary?0:contract;
        if(target==0x100c7d00)result=o.guard==1;
        if(target==0x100c7a20)result=o.guard==3||o.guard==4?0:1;
        if(target==0x100d3c40)result=o.guard==4;
        if(target==0x1014c2e0)result=o.guard==5;
        if(target==0x103f43b0)result=o.guard==6;
        if(target==0x103eb440)result=origin_offer;
        if(target==0x1036d510){c.put8(c.r(6),o.role);result=o.guard==7?3:2;}
        if(target==0x10374f50) {
            c.put32(first,0);c.put32(first+4,1);c.put32(first+8,200);c.put32(first+12,o.weekly);
            for(unsigned n=0;n<3;++n)c.put32(first+0x10+n*4,std::uint32_t(o.bonus));
            c.put8(first+0x1c,o.alternative);c.put8(first+0x1d,o.guarantee);c.put8(first+0x1f,o.secondary_floor);
            c.put32(first+0x24,0);c.put16(first+0x30,150);c.put16(first+0x32,o.proposal_year);c.put8(first+0x34,0);
            c.put8(first+0x60,4);result=o.guard==8?0:std::uint32_t(o.proposal_result);
        }
        if(target==0x10416840)result=o.valuation;
        if(target==0x10103510)result=national;
        if(target==0x100f2680||target==0x100f4790)result=o.maximum;
        if(target==0x10082f90)result=o.existing?contract:0;
        if(target==0x10718130)result=first==table+0x800?std::uint32_t(o.age*365):o.duration;
        if(target==0x10780fc0){const auto value=std::pow(c.f(1),c.f(2));c.set_f(1,value);c.put32(c.sp()+0x14,c.r(2));s.pc=back;return;}
        if(target==0x1018c340)result=o.injury_absent?0:injury;
        if(target==0x10717890){c.put16(first,100);c.put16(first+2,2001);c.put8(first+4,0);result=first;}
        if(target==0x10771110)s.memory.copy_in(first,s.memory.copy_out(second,third));
        if(target==0x1007f200)s.memory.copy_in(globals+0x1000,s.memory.copy_out(second,100));
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);
        for(unsigned n=0;n<14;++n)c.set_f(n,-100.0-n);
        c.set_cr(0,2);c.set(3,result);s.pc=back;
        if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Preparation dependency interrupted");
    }
};
thread_local Model* active=nullptr;
struct Hook final:ContractManagerCallHook {bool call(State& s)override{active->call(s,s.pc,std::uint32_t(s.r<4>(4128)));return true;}}hook;
std::unique_ptr<State> seed(Options o) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x30000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);
    Cpu c(*s);c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,object);c.set(4,person);
    c.set(5,o.derive?0:club);c.set(6,o.copy?output:0);c.set(7,o.source?42:0xffffffff);c.set(10,o.mode);c.set_f(1,o.scale);s->pc=entry;s->budget=100000;
    for(auto [slot,pointer]:{std::pair{-0x7ba0,globals},{-0x7ab8,globals+4},{-0x7b40,globals+8},{-0x7b78,globals+12},{-0x7b80,table+0x800},{-0x7b7c,globals+16},{-0x7be8,globals+20},{-0x7b9c,globals+24},{-0x7a68,globals+28},{-0x79b8,table+0x800}})c.put32(toc+slot,pointer);
    c.put32(globals,1);c.put32(globals+4,national);c.put32(globals+8,transfer);c.put32(globals+16,object);c.put32(globals+20,club-0x248);c.put32(globals+24,indices);c.put8(globals+28,o.limited);
    c.put32(object,contract);c.put32(person,0);c.put32(person+0x14,o.player?attributes:0);c.put32(person+0x24,o.guard==3||o.guard==4?club:0);
    c.put8(person+0x62,11);c.put8(person+0x67,std::uint32_t(o.trait));c.put32(club,1);c.put32(club+0x90,o.guard==4?0:1);c.put16(club+0x1ec,5000);
    c.put32(contract+4,1);c.put16(contract+0x32,o.guard==9?2010:o.current_year);c.put16(contract+0x30,100);c.put8(contract+0x60,o.squad_role);
    c.put8(indices+12,o.guard==2?4:0);c.put32(transfer+0x988,shortlist);c.put32(shortlist+0xdc,globals+0x100);c.put8(globals+0x100+12+4,o.guard==3?2:0);
    c.put32(origin_offer,origin_offer+0x100);c.put16(attributes+10,o.ability);c.put16(attributes+12,1000);c.put8(attributes+0x34,std::uint32_t(o.trait));c.put8(attributes+0x3c,std::uint32_t(o.trait));c.put8(attributes+15,o.special_position?20:0);
    c.put16(injury+0x34,o.injured?71:70);c.put16(table+0x800,100);c.put16(table+0x802,2001);c.put8(table+0x804,0);
    contract_manager_preparation_constants(*s,toc,table);return s;
}
void original(State& s,Model& model) {
    while(s.pc!=return_sentinel) {
        if(s.pc>=entry&&s.pc<end)require(contract_manager_preparation_reference(s),"Complete original preparation body");
        else model.call(s,s.pc,std::uint32_t(s.r<4>(4128)));
    }
}
void same_registers(const State& a,const State& b,const std::string& label) {
    for(unsigned n=1;n<32;++n)if(n!=11&&n!=12)require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n)+" "+hex_address(a.r<4>(n*4))+"/"+hex_address(b.r<4>(n*4)));
    for(unsigned n=1;n<32;++n)if(n==1||n>=14)require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));
    for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR"+std::to_string(n-1280));
    for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR");
    require(a.r<1>(guest::reg::xer_ca)==b.r<1>(guest::reg::xer_ca),label+" XER carry");
    require(a.r<4>(guest::reg::lr)==b.r<4>(guest::reg::lr),label+" LR");
}
unsigned cases=0,returned=0,stopped=0,negatives=0;std::set<std::uint32_t> reached;
void compare(Options o={}) {
    auto source=seed(o),a=clone(*source),b=clone(*source);Model old(o),modern(o);
    const auto run=[&](State& s,Model& model,bool reference){try{if(reference)original(s,model);else{
        const auto previous=contract_manager_call_hook;active=&model;contract_manager_call_hook=&hook;
        try{Cpu c(s);const auto answer=cm::aot::contract_manager::prepare_contract_offer(c,c.r(3),c.r(4),c.r(5),c.r(6),c.r(7),c.r(10),c.f(1));c.set(3,answer);c.finish();}
        catch(...){contract_manager_call_hook=previous;active=nullptr;throw;}contract_manager_call_hook=previous;active=nullptr;
    }return std::string{};}catch(const std::exception& e){return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false),label="preparation case "+std::to_string(cases);
    require(expected.empty()||expected=="Preparation dependency interrupted",label+" invalid reference "+expected);
    require(expected==actual,label+" outcomes "+expected+"/"+actual);
    if(o.interrupt)require(expected=="Preparation dependency interrupted"&&old.calls.size()==o.interrupt,label+" requested interruption reached");
    if(o.injury_absent)require(std::count_if(old.calls.begin(),old.calls.end(),[](const Event& e){return e.target==0x1018c340;})==1,label+" null injury lookup reached");
    if(o.corrupt)b->memory.write<1>(output,b->memory.read<1>(output)^0x5a);
    bool rejected=false;
    try{
        same_registers(*a,*b,label);require(a->pc==b->pc,label+" PC");
        for(auto [address,size]:a->memory.mapped_ranges()){
            const auto x=a->memory.copy_out(address,size),y=b->memory.copy_out(address,size);
            if(x!=y){const auto at=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(address+std::uint32_t(at.first-x.begin()))+" "+std::to_string(*at.first)+"/"+std::to_string(*at.second));}
        }
        if(old.calls!=modern.calls){auto at=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());throw std::runtime_error(label+" call "+std::to_string(at.first-old.calls.begin())+(at.first==old.calls.end()?" extra":" target "+hex_address(at.first->target)));}
    }catch(const std::runtime_error&){if(!o.corrupt)throw;rejected=true;}
    require(rejected==o.corrupt,label+" negative control");++cases;returned+=expected.empty()&&!rejected;stopped+=!expected.empty();negatives+=rejected;
    if(expected.empty()&&!rejected)for(const auto& e:old.calls)reached.insert(e.back);
}
}
int main(){try{
    compare();
    for(unsigned guard=1;guard<=9;++guard){Options o;o.guard=guard;if(guard==7)o.player=true;if(guard==9)o.existing=true;compare(o);}
    for(bool primary:{false,true}){Options o;o.derive=true;o.no_primary=!primary;compare(o);}
    for(int age:{25,29,30,31,32,34,35,40})for(unsigned wage:{1000u,12000u,25000u}){
        Options o;o.player=true;o.age=age;o.weekly=wage;o.maximum=50000;compare(o);
    }
    for(unsigned value:{0u,50000u,50001u,1000000u,1000001u,5000000u,5000001u,12000000u}){
        Options o;o.source=true;o.guarantee=1;o.valuation=value;compare(o);
    }
    for(bool source:{false,true})for(int flag:{0,1}){Options o;o.guarantee=flag;o.source=source;o.alternative=true;o.secondary_floor=true;compare(o);}
    for(unsigned ability:{2000u,5000u,8000u,12000u}){Options o;o.player=true;o.age=35;o.weekly=25000;o.maximum=50000;o.ability=ability;compare(o);}
    for(double factor:{-1.0,0.0,0.5,1.0,2.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){Options o;o.scale=factor;compare(o);}
    for(bool copy:{false,true})for(unsigned mode:{0u,1u,2u}){Options o;o.copy=copy;o.mode=mode;o.existing=mode==1;o.limited=true;compare(o);}
    for(int bonus:{-2,-1,0,100}){Options o;o.player=true;o.bonus=bonus;compare(o);}
    for(double scale:{0.5,1.0,2.0}){Options o;o.role=11;o.scale=scale;compare(o);}
    Options promotion;promotion.guard=7;promotion.player=true;promotion.ability=6000;compare(promotion);
    Options short_term;short_term.player=true;short_term.existing=true;short_term.age=25;short_term.squad_role=6;compare(short_term);
    Options publish;publish.copy=false;publish.mode=1;publish.existing=true;publish.current_year=2005;compare(publish);
    Options limit;limit.limited=true;limit.proposal_year=2010;compare(limit);
    Options keeper;keeper.player=true;keeper.special_position=true;compare(keeper);
    for(int trait:{-128,0,15,127}){Options o;o.player=true;o.trait=trait;compare(o);}
    Options no_injury;no_injury.existing=true;no_injury.injury_absent=true;compare(no_injury);
    for(auto amount:{INT32_MIN,-1,123456789,INT32_MAX}){Options o;o.proposal_result=amount;compare(o);}
    Options injury;injury.existing=true;injury.injured=true;compare(injury);
    for(unsigned at:{1u,2u,5u,8u}){Options o;o.interrupt=at;compare(o);}
    Options negative;negative.corrupt=true;compare(negative);
    const auto calls=contract_manager_preparation_calls();
    require(reached==std::set<std::uint32_t>(calls.begin(),calls.end()),"Every original preparation call continuation reached by returned cases");
    std::cout<<cases<<" preparation comparisons; "<<returned<<" returned; "<<stopped<<" model stops (partial); "<<negatives<<" mutations detected; "<<reached.size()<<"/"<<calls.size()<<" original call continuations reached\n";
    for(auto back:calls)if(!reached.contains(back))std::cout<<"unreached "<<hex_address(back)<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
