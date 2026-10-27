#include "contract_manager.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <map>
#include <set>

namespace cm::aot {
bool contract_manager_offer_policy_reference(State&);
std::vector<std::uint32_t> contract_manager_offer_policy_calls();
void contract_manager_offer_policy_constants(State&, std::uint32_t, std::uint32_t, std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto allowed_entry=0x10089ea0u, rebalance_entry=0x1008a160u;
constexpr auto object=fixture+0x1000,people=fixture+0x2000,proposal=fixture+0x3000,records=fixture+0x4000;
constexpr auto globals=fixture+0x5000,indices=fixture+0x6000,clubs=fixture+0x8000,owner=fixture+0xa000;
constexpr auto list=fixture+0xb000,nodes=fixture+0xc000,rows=fixture+0xd000;
constexpr auto constants=fixture+0x10000,singles=fixture+0x12000,today=fixture+0x14000;
struct Row {int person_id=0,club_id=1; unsigned kind=3;};
struct Options {
    std::vector<Row> pending{{0,1,3}};
    int person_id=0,person_count=3,primary=0,contract_person=0,proposal_club=1,contract_club=1;
    int age=26,locked=0,role=2,low_bits=0,role_state=2,discontent=-51,owner_day=49,owner_year=2001;
    unsigned terms=0,status=0,squad_role=0,active=0,interrupt=0,fp_state=0;
    int baseline=1000,wage=100,assessment_offset=0;
    bool null_person=false,player=true,nonplayer=false,club_owner=true,protected_person=false,special=false;
    bool has_list=true,corrupt=false,corrupt_copy=false,expect_fp_guard=false;
};
struct Event {
    std::uint32_t target,back;std::vector<std::uint32_t> args,kept;
    std::vector<std::uint64_t> fp;std::vector<std::uint8_t> fpscr;
    bool operator==(const Event&)const=default;
};
unsigned arity(std::uint32_t target){switch(target){
    case 0x1037f110:case 0x10738e00:return 1;
    case 0x100d3150:return 2;
    case 0x10717b20:case 0x1037e660:case 0x1077fc28:return 3;
    default:throw std::runtime_error("Unknown offer policy dependency "+hex_address(target));
}}
struct Model {
    Options o;std::vector<Event> calls;unsigned assessments=0,copies=0;
    explicit Model(Options options):o(std::move(options)){}
    void call(State& s,std::uint32_t target,std::uint32_t back){
        Cpu c(s);Event e{target,back,{},{},{},{}};
        for(unsigned n=0;n<arity(target);++n)e.args.push_back(c.r(n+3));
        for(unsigned n=13;n<32;++n)e.kept.push_back(c.r(n));
        for(unsigned n=14;n<32;++n)e.fp.push_back(s.r<8>(guest::reg::fpr(n)));
        for(unsigned n=1280;n<1312;++n)e.fpscr.push_back(std::uint8_t(s.r<1>(n)));
        calls.push_back(e);require(calls.size()<500,"Bounded offer policy fixture");
        const auto first=c.r(3),second=c.r(4),third=c.r(5);std::uint32_t value=0;
        if(target==0x100d3150)value=o.protected_person;
        if(target==0x1037f110)value=o.special;
        if(target==0x10717b20){
            int date=int(c.u16(second+2))*365+int(c.i16(second))-std::int32_t(third);
            int year=date/365,day=date%365;if(day<0){day+=365;--year;}
            c.put16(first,std::uint32_t(day));c.put16(first+2,std::uint32_t(year));c.put8(first+4,0);value=first;
        }
        if(target==0x10738e00){
            const auto cursor=c.u32(first+12);
            if(cursor){require(cursor>=nodes&&cursor<nodes+o.pending.size()*16,"Valid pending iterator");value=c.u32(cursor+4);c.put32(first+12,c.u32(cursor));}
        }
        if(target==0x1037e660){
            // Deliberately independent dependency model: first assessment is
            // the requested baseline; later assessments react to wage edits.
            value=assessments++?c.u32(first+12)+std::uint32_t(o.assessment_offset):std::uint32_t(o.baseline);
        }
        if(target==0x1077fc28){
            s.memory.copy_in(first,s.memory.copy_out(second,third));value=first;
            c.put32(c.sp()+0x14,c.r(2));
            if(o.corrupt_copy&&copies++==0)c.put8(first,c.u8(first)^0x80);
        }
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);
        for(unsigned n=0;n<14;++n)c.set_f(n,-100.0-n);
        c.set_cr(0,2);c.set(3,value);s.pc=back;
        if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Offer policy dependency interrupted");
    }
};
thread_local Model* active_model=nullptr;
struct Hook final:ContractManagerCallHook {
    bool call(State& s)override{active_model->call(s,s.pc,std::uint32_t(s.r<4>(4128)));return true;}
}hook;
std::unique_ptr<State> seed(std::uint32_t entry,const Options& o){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    s->memory.map(fixture,0x20000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    Cpu c(*s);for(unsigned n=0;n<0x5000;++n)c.put8(stack-0x4000+n,n*13+7);
    for(unsigned n=0;n<32;++n)c.set_f(n,double(n)+0.25);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,o.fp_state==3?1:n>=1304?0:o.fp_state==1?1:o.fp_state==2?n&1:0);
    c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,object);c.set(4,proposal);s->pc=entry;s->budget=1000000;
    c.put32(toc-0x7be0,globals);c.put32(globals,o.null_person?0:people);
    c.put32(toc-0x7be4,globals+4);c.put32(globals+4,std::uint32_t(o.person_count));
    c.put32(toc-0x7b9c,globals+8);c.put32(globals+8,indices);
    c.put32(toc-0x7be8,globals+12);c.put32(globals+12,clubs);c.put32(toc-0x7b80,today);
    c.put16(today,200);c.put16(today+2,2001);c.put8(today+4,0);
    contract_manager_offer_policy_constants(*s,toc,constants,singles);
    c.put32(object,records);c.put32(object+4,1);c.put32(object+8,1);c.put32(object+0x34,o.has_list?list:0);
    for(int n=-1;n<4;++n){
        const auto at=people+std::uint32_t(n*112);c.put32(at,std::uint32_t(n));
        c.put32(at+0x14,o.player?owner+0x100:0);c.put32(at+0x1c,o.nonplayer?owner+0x200:0);c.put16(at+0x5c,std::uint32_t(o.age));
        c.put32(indices+std::uint32_t(n*100),std::uint32_t(o.primary));c.put8(indices+std::uint32_t(n*100)+12,o.status<<4);
    }
    for(unsigned n=0;n<100;++n){c.put8(proposal+n,n*31+7);c.put8(records+n,n*13+5);}
    c.put32(proposal,std::uint32_t(o.person_id));c.put32(proposal+4,std::uint32_t(o.proposal_club));c.put32(proposal+12,std::uint32_t(o.wage));c.put8(proposal+0x38,(std::uint32_t(o.role)<<2)|std::uint32_t(o.low_bits));
    c.put32(records,std::uint32_t(o.contract_person));c.put32(records+4,std::uint32_t(o.contract_club));c.put32(records+0x50,o.terms);
    c.put8(records+0x54,std::uint32_t(o.locked));c.put8(records+0x60,o.squad_role);c.put8(records+0x5c,o.active);
    c.put8(records+0x46,std::uint32_t(o.role_state));c.put8(records+0x44,std::uint32_t(o.discontent));
    c.put32(clubs+std::uint32_t(o.contract_club)*0x248+0x90,o.club_owner?owner:0);c.put16(owner+0x4c,std::uint32_t(o.owner_day));c.put16(owner+0x4e,std::uint32_t(o.owner_year));
    c.put32(list+4,o.pending.empty()?0:nodes);c.put32(list+12,0x98765432);
    for(unsigned n=0;n<o.pending.size();++n){
        const auto row=rows+n*0x100,node=nodes+n*16;for(unsigned j=0;j<0x80;++j)c.put8(row+j,n*19+j*11+3);
        c.put32(node,n+1<o.pending.size()?node+16:0);c.put32(node+4,row);
        c.put32(row,std::uint32_t(o.pending[n].person_id));c.put32(row+4,std::uint32_t(o.pending[n].club_id));c.put8(row+0x64,o.pending[n].kind);
        c.put16(row+0x30,100+n);c.put16(row+0x32,2004+n);c.put8(row+0x34,n&1);
    }
    return s;
}
bool within(std::uint32_t pc){return pc>=allowed_entry&&pc<0x1008a350;}
void reference(State& s,Model& model){while(s.pc!=return_sentinel){if(within(s.pc))require(contract_manager_offer_policy_reference(s),"Complete original offer policy body");else model.call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
struct Totals {unsigned cases=0,returned=0,partial=0,negative=0;std::set<std::uint32_t> returned_calls;};
std::map<std::uint32_t,Totals> totals;
void same(const State& a,const State& b,std::uint32_t entry,const std::string& label,bool fp_guard){
    for(unsigned n=1;n<32;++n)if(n!=11&&n!=12&&(n!=3||entry==allowed_entry))require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n)+" "+hex_address(a.r<4>(n*4))+"/"+hex_address(b.r<4>(n*4)));
    for(unsigned n=1;n<32;++n)if(n==1||n>=14)require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));
    for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR "+std::to_string(n));
    for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR");
    require(a.r<1>(guest::reg::xer_ca)==b.r<1>(guest::reg::xer_ca),label+" carry");require(a.r<4>(4128)==b.r<4>(4128)&&(fp_guard||a.pc==b.pc),label+" LR/PC");
    for(auto [address,size]:a.memory.mapped_ranges()){
        const auto x=a.memory.copy_out(address,size),y=b.memory.copy_out(address,size);
        if(x!=y){const auto at=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(address+std::uint32_t(at.first-x.begin()))+" "+std::to_string(*at.first)+"/"+std::to_string(*at.second));}
    }
}
void compare(std::uint32_t entry,Options o={}){
    auto source=seed(entry,o),a=clone(*source),b=clone(*source);Model old(o),modern(o);old.o.corrupt_copy=false;
    const auto run=[&](State& s,Model& model,bool original){try{
        if(original)reference(s,model);
        else {const auto previous=contract_manager_call_hook;active_model=&model;contract_manager_call_hook=&hook;
            try {Cpu c(s);if(entry==allowed_entry)c.set(3,contract_manager::contract_offer_allowed(c,c.r(3),c.r(4)));else contract_manager::rebalance_pending_offer(c,c.r(3),c.r(4));c.finish();}
            catch(...){contract_manager_call_hook=previous;active_model=nullptr;throw;}
            contract_manager_call_hook=previous;active_model=nullptr;
        }return std::string{};
    }catch(const std::exception& e){return std::string(e.what());}};
    auto& t=totals[entry];const auto expected=run(*a,old,true),actual=run(*b,modern,false),label=hex_address(entry)+" offer policy case "+std::to_string(t.cases);
    const bool fp_guard=expected=="Original enabled floating invalid-conversion exception requires a handler"||expected=="Original enabled floating inexact exception requires a handler";
    require(expected.empty()||expected=="Offer policy dependency interrupted"||(o.expect_fp_guard&&fp_guard),label+" invalid reference: "+expected);
    require(expected==actual,label+" outcomes: "+expected+"/"+actual);
    if(o.interrupt)require(expected=="Offer policy dependency interrupted"&&old.calls.size()==o.interrupt,label+" interruption reached");else if(o.expect_fp_guard)require(fp_guard,label+" enabled FP guard reached");else require(expected.empty(),label+" completed return");
    if(o.corrupt)b->memory.write<1>(proposal,b->memory.read<1>(proposal)^0x80);
    bool rejected=false;try{
        same(*a,*b,entry,label,fp_guard);
        if(old.calls!=modern.calls){
            const auto at=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());std::string detail;
            if(at.first!=old.calls.end()&&at.second!=modern.calls.end()){
                detail=" targets "+hex_address(at.first->target)+"/"+hex_address(at.second->target)+" returns "+hex_address(at.first->back)+"/"+hex_address(at.second->back);
                for(unsigned n=0;n<std::min(at.first->args.size(),at.second->args.size());++n)if(at.first->args[n]!=at.second->args[n])detail+=" arg"+std::to_string(n+3)+" "+hex_address(at.first->args[n])+"/"+hex_address(at.second->args[n]);
                for(unsigned n=0;n<at.first->kept.size();++n)if(at.first->kept[n]!=at.second->kept[n])detail+=" r"+std::to_string(n+13)+" "+hex_address(at.first->kept[n])+"/"+hex_address(at.second->kept[n]);
                for(unsigned n=0;n<at.first->fp.size();++n)if(at.first->fp[n]!=at.second->fp[n])detail+=" f"+std::to_string(n+14);
            }
            throw std::runtime_error(label+" call "+std::to_string(at.first-old.calls.begin())+detail);
        }
    }catch(const std::runtime_error&){if(!o.corrupt&&!o.corrupt_copy)throw;rejected=true;}
    require(rejected==(o.corrupt||o.corrupt_copy),label+" mutation control");++t.cases;t.returned+=expected.empty()&&!rejected;t.partial+=!expected.empty();t.negative+=rejected;
    if(expected.empty()&&!rejected)for(const auto& e:old.calls)t.returned_calls.insert(e.back);
}
}
int main(){try{
    compare(allowed_entry);compare(rebalance_entry);
    for(int index:{-1,0})for(int id:{-1,0,2,3}){Options o;o.primary=index;o.person_id=id;compare(allowed_entry,o);}
    Options absent;absent.null_person=true;compare(allowed_entry,absent);
    for(unsigned terms:{0u,1u,2u,4u,8u,16u,32u,64u,128u,256u,512u,1024u})for(int age:{24,25}){Options o;o.terms=terms;o.age=age;compare(allowed_entry,o);}
    for(int club:{0,1,2})for(bool player:{false,true})for(bool nonplayer:{false,true}){Options o;o.proposal_club=club;o.player=player;o.nonplayer=nonplayer;compare(allowed_entry,o);}
    for(int role:{0,1,2,31,32,63})for(int bits:{0,1,2,3}){Options o;o.role=role;o.low_bits=bits;compare(allowed_entry,o);}
    for(int age:{-128,-1,23,24,25,127,128,255,280})for(unsigned status:{0u,1u}){Options o;o.age=age;o.status=status;compare(allowed_entry,o);}
    for(int day:{-32768,-1,49,50,51,32767})for(int year:{0,2000,2001,2002,65535}){Options o;o.owner_day=day;o.owner_year=year;compare(allowed_entry,o);}
    for(int state:{-1,2,3})for(int discontent:{-51,-50,-49})for(bool owner_present:{false,true}){Options o;o.role_state=state;o.discontent=discontent;o.club_owner=owner_present;compare(allowed_entry,o);}
    for(int lock:{-128,0,1,255})for(bool protected_person:{false,true})for(bool special:{false,true}){Options o;o.locked=lock;o.protected_person=protected_person;o.special=special;compare(allowed_entry,o);}
    for(unsigned squad_role:{0u,7u,0xf7u})for(unsigned active:{0u,8u}){Options o;o.squad_role=squad_role;o.active=active;compare(allowed_entry,o);}
    for(bool has_list:{false,true})for(const auto& pending:std::vector<std::vector<Row>>{{},{{0,1,2}},{{0,1,3}},{{1,1,3},{0,2,3}},{{1,1,3},{0,2,3},{0,1,3},{0,1,3}}}){Options o;o.has_list=has_list;o.pending=pending;compare(rebalance_entry,o);}
    for(int id:{-1,1,2})for(int club:{0,2}){Options o;o.person_id=id;o.proposal_club=club;o.pending={{id+1,club,3},{id,club+1,3},{id,club,3},{id,club,3}};compare(rebalance_entry,o);}
    for(int baseline:{-2147483647-1,-1,0,1,100,1000,1000000,16777215,16777216,16777217,2147483647})for(int wage:{0,100,5000}){Options o;o.baseline=baseline;o.wage=wage;compare(rebalance_entry,o);}
    for(int wage:{-1000,-1,960,970,1000,1030,1040})for(int offset:{-100,0,100}){Options o;o.wage=wage;o.assessment_offset=offset;compare(rebalance_entry,o);}
    for(unsigned state:{1u,2u})for(int baseline:{0,1000}){Options o;o.fp_state=state;o.baseline=baseline;o.wage=0;compare(rebalance_entry,o);}
    Options guard;guard.fp_state=3;guard.expect_fp_guard=true;compare(rebalance_entry,guard);
    guard.baseline=2147483647;guard.wage=2147483647;guard.assessment_offset=-2147483647;compare(rebalance_entry,guard);
    for(unsigned at:{1u,2u,3u}){Options o;o.interrupt=at;compare(allowed_entry,o);}
    for(unsigned at:{1u,2u,3u,4u,5u,6u}){Options o;o.interrupt=at;compare(rebalance_entry,o);}
    for(auto entry:{allowed_entry,rebalance_entry}){Options o;o.corrupt=true;compare(entry,o);}
    Options copy;copy.corrupt_copy=true;compare(rebalance_entry,copy);
    const auto calls=contract_manager_offer_policy_calls();
    for(const auto& [entry,t]:totals){unsigned expected=0;const auto end=entry==allowed_entry?rebalance_entry:0x1008a350u;
        for(auto back:calls)if(back>entry&&back<end){++expected;require(t.returned_calls.contains(back),"Unreached completed offer policy continuation "+hex_address(back));}
        std::cout<<hex_address(entry)<<": "<<t.cases<<" comparisons; "<<t.returned<<" returned; "<<t.partial<<" explicitly partial interruptions/FP guards; "<<t.negative<<" mutation controls detected; "<<t.returned_calls.size()<<"/"<<expected<<" original continuations in completed calls\n";
    }return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
