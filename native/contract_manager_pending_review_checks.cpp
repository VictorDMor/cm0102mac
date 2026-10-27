#include "contract_manager.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <map>
#include <set>
#include <limits>

namespace cm::aot {
bool contract_manager_pending_review_reference(State&);
std::vector<std::uint32_t> contract_manager_pending_review_calls();
void contract_manager_pending_review_constants(State&,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;using cm::aot::guest::Cpu;
constexpr auto entry=0x10081bd0u,end=0x10082bf0u;
constexpr auto object=fixture+0x1000,person=fixture+0x2000,clubs=fixture+0x3000,contract=fixture+0x5000;
constexpr auto attributes=fixture+0x6000,transfers=fixture+0x7000,shortlist=fixture+0x8000,national=fixture+0x9000;
constexpr auto globals=fixture+0xa000,random=fixture+0xb000,rows=fixture+0xc000,owned=fixture+0xd000,item=fixture+0xe000;
constexpr auto errors=fixture+0x10000,nations=fixture+0x14000,constants=fixture+0x18000,today=fixture+0x1a000,arena=fixture+0x20000;
struct Row {int person_id=0,club_id=1,role=2,state=0,active=1,origin=-1,value=100;unsigned kind=11;double attraction=4,extra=2;};
struct Options {
    std::vector<Row> records{Row{}};int baseline=100,trait=15,adapt=21,ambition=15,nation=0,refined=0;
    // The original constants make the wide sampler unreachable. An explicit
    // constant-edge fixture checks its preserved defensive branch separately.
    double minimum_bound=5;
    unsigned cursor=0,seed=0,entropy=0x1234,expiry_days=365,interrupt=0;
    bool blocked=false,primary=true,profile=false,player=true,national_kind=false,national_available=true;
    bool mix=false,veto=false,exclude=false,accepted=true,human=false,low_reputation=false,no_owner=false;
    bool fail_allocation=false,expect_null_deferred=false,corrupt=false,corrupt_decision=false;
};
struct Event {std::uint32_t target,back;std::vector<std::uint32_t> args,kept;std::vector<std::uint64_t> fp;bool operator==(const Event&)const=default;};
unsigned arity(std::uint32_t target) {
    switch(target){
    case 0x103721f0:case 0x10715880:case 0x10738c00:case 0x10738e00:case 0x100c7a20:case 0x100c7a00:case 0x102dbe90:case 0x10773520:case 0x100788f0:return 1;
    case 0x10738c20:case 0x10738e60:case 0x103f4c00:case 0x10082f20:case 0x10417260:case 0x103eb440:case 0x101b1620:case 0x10698e80:case 0x103ea250:case 0x10082c50:case 0x10082bf0:case 0x103e8bb0:case 0x1030f950:case 0x103ea270:return 2;
    case 0x10717890:case 0x1037e660:case 0x10738d10:case 0x10718130:case 0x1008a350:case 0x10082c10:case 0x100d9960:case 0x100da110:case 0x103d2b30:case 0x10769b60:return 3;
    case 0x1036d510:case 0x103e76c0:case 0x103ecef0:case 0x102dbeb0:return 4;
    case 0x1014c2e0:case 0x10739890:case 0x100808d0:return 5;
    case 0x10366470:case 0x10084be0:case 0x103e70f0:case 0x1077b720:return 6;
    case 0x10369080:return 7;
    case 0x10374f50:return 8;
    case 0x1077b770:case 0x10768810:return 0;
    default:throw std::runtime_error("Unknown pending review dependency "+hex_address(target));
    }
}
struct Node {std::uint32_t node,data;};
struct Model {
    Options o;std::vector<Event> calls;std::map<std::uint32_t,std::vector<Node>> lists;std::map<std::uint32_t,std::uint32_t> last;
    unsigned allocated=0;std::uint32_t next=arena;
    explicit Model(Options options):o(std::move(options)){}
    void seed_lists(State& s) {
        auto& list=lists[owned];for(unsigned n=0;n<o.records.size();++n)list.push_back({owned+0x100+n*16,rows+n*0x100});
        links(s,owned);
    }
    void links(State& s,std::uint32_t list) {
        const auto& nodes=lists[list];s.memory.write<2>(list,nodes.size());s.memory.write<4>(list+4,nodes.empty()?0:nodes.front().node);
        for(unsigned n=0;n<nodes.size();++n)s.memory.write<4>(nodes[n].node,n+1<nodes.size()?nodes[n+1].node:0);
    }
    const Row& current(State& s)const {
        const auto row=std::uint32_t(s.memory.read<4>(std::uint32_t(s.r<4>(4))+0x44));
        const auto n=(row-rows)/0x100;require(row>=rows&&n<o.records.size(),"Current modeled offer");return o.records[n];
    }
    void call(State& s,std::uint32_t target,std::uint32_t back) {
        Cpu c(s);
        // These leaf helpers only save/restore FPR28..31. guest::Frame
        // performs the same stores/reloads in the C++ run, without a call.
        if(target==0x10773808||target==0x10773858){for(unsigned n=28;n<32;++n){const auto at=c.sp()-32+(n-28)*8;if(target==0x10773808)c.put64(at,s.r<8>(guest::reg::fpr(n)));else s.wr<8>(guest::reg::fpr(n),c.u64(at));}s.pc=back;return;}
        Event e{target,back,{},{},{}};for(unsigned n=0;n<arity(target);++n)e.args.push_back(c.r(n+3));for(unsigned n=13;n<32;++n)e.kept.push_back(c.r(n));for(unsigned n=28;n<32;++n)e.fp.push_back(s.r<8>(guest::reg::fpr(n)));calls.push_back(e);
        const auto first=c.r(3),second=c.r(4),third=c.r(5);std::uint32_t result=0;bool floating=false;double answer=0;
        if(target==0x10738c00){s.memory.clear(first,16);lists[first]={};result=first;}
        if(target==0x10738c20){lists[first].clear();s.memory.clear(first,16);}
        if(target==0x103721f0){for(unsigned n=0;n<100;++n)c.put8(first+n,n*31+7);c.put32(first+4,0xffffffff);c.put32(first+8,0);c.put32(first+0x50,0);c.put8(first+0x54,0);c.put32(first+0x58,0xffffffff);result=first;}
        if(target==0x10715880){c.put16(first,100);c.put16(first+2,2001);c.put8(first+4,0);result=first;}
        if(target==0x103f4c00)result=o.blocked;
        if(target==0x10717890){c.put16(first,102);c.put16(first+2,2001);c.put8(first+4,0);result=first;}
        if(target==0x100808d0){c.put32(globals+0x100,first);c.put32(globals+0x104,second);}
        if(target==0x10738e00){const auto cursor=c.u32(first+12);if(cursor){const auto& nodes=lists.at(first);const auto at=std::find_if(nodes.begin(),nodes.end(),[&](Node n){return n.node==cursor;});require(at!=nodes.end(),"Valid modeled iterator");last[first]=cursor;result=at->data;c.put32(first+12,c.u32(cursor));}}
        if(target==0x10738e60){auto& nodes=lists.at(first);const auto at=std::find_if(nodes.begin(),nodes.end(),[&](Node n){return n.data==second;});require(at!=nodes.end(),"Current modeled removal");nodes.erase(at);links(s,first);}
        if(target==0x10738d10){if(!first)throw std::runtime_error("Null deferred dependency");require(third<=136,"Bounded pending review append");const auto node=next,data=next+0x20;next+=0x100;s.memory.copy_in(data,s.memory.copy_out(second,third));lists[first].push_back({node,data});links(s,first);result=data;}
        if(target==0x10082f20)result=o.primary?contract:0;
        if(target==0x10369080){floating=true;answer=o.corrupt_decision?0:current(s).attraction;}
        if(target==0x10366470){floating=true;answer=current(s).extra;}
        if(target==0x1036d510)result=2;
        if(target==0x103eb440){c.put32(item,item+0x100);result=item;}
        if(target==0x10417260)result=item+0x200;
        if(target==0x10374f50){for(unsigned n=0;n<100;++n)c.put8(first+n,n*7+3);result=1;}
        if(target==0x1037e660)result=first==c.sp()+0xc0?std::uint32_t(o.baseline):std::uint32_t(current(s).value);
        if(target==0x100c7a20)result=o.mix;
        if(target==0x100c7a00)result=o.national_kind;
        if(target==0x102dbe90)result=o.national_available;
        if(target==0x103ecef0)result=o.veto;
        if(target==0x10718130)result=o.expiry_days;
        if(target==0x101b1620)result=o.exclude;
        if(target==0x10082c50){s.memory.copy_in(first,s.memory.copy_out(second,100));result=first;}
        if(target==0x10082c10){c.put16(second+2,c.u16(third+2));c.put16(second,c.u16(third));c.put8(second+4,c.u8(third+4));c.put32(first,second);result=first;}
        if(target==0x10082bf0){s.memory.copy_in(first,s.memory.copy_out(second,12));result=first;}
        if(target==0x100788f0)result=std::uint32_t(o.refined);
        if(target==0x1077b770){c.set(0,0);c.set(4,globals+0x108);c.set(3,o.entropy);s.pc=back;if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Pending review interrupted");return;}
        if(target==0x10773520){++allocated;result=o.fail_allocation?0:arena+0x18000+allocated*0x100;}
        if(target==0x103e8bb0)result=o.accepted;
        if(target==0x102dbeb0){c.put32(second,std::uint32_t(o.nation));c.put32(third,11);c.put32(c.r(6),22);}
        if(target==0x1014c2e0)result=o.human;
        if(target==0x10084be0)result=123456;
        if(target==0x10739890){c.put32(c.r(6),123);c.put32(c.r(7),456);}
        if(target==0x10768810)result=0x1234;
        if(target==0x1077b720)for(unsigned n=0;n<6;++n)c.put32(first+n*4,c.r(n+3));
        if(target==0x1008a350||target==0x103e70f0){const auto at=target==0x1008a350?second:c.r(6);s.memory.copy_in(fixture+0x50000,s.memory.copy_out(at,100));}
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);
        for(unsigned n=0;n<14;++n)c.set_f(n,-100.0-n);if(floating)c.set_f(1,answer);
        c.set_cr(0,2);c.set(3,result);s.pc=back;
        if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Pending review interrupted");
    }
};
thread_local Model* active=nullptr;struct Hook final:ContractManagerCallHook{bool call(State& s)override{active->call(s,s.pc,std::uint32_t(s.r<4>(4128)));return true;}}hook;
std::unique_ptr<State> seed(const Options& o) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x60000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);Cpu c(*s);c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,object);c.set(4,person);s->pc=entry;s->budget=1000000;
    for(auto [slot,pointer]:{std::pair{-0x7b40,globals},{-0x7b28,globals+4},{-0x7b7c,globals+8},{-0x7b78,globals+12},{-0x7be8,globals+16},{-0x7b84,globals+20},{-0x7b9c,globals+24},{-0x7ba8,errors},{-0x7b80,today},{-0x5718,constants+0x400},{-0x79ec,globals+28},{-0x7b90,globals+32},{-0x7b88,globals+36},{-0x7b8c,random},{-0x79f0,globals+40},{-0x7b94,globals+44},{-0x79f4,globals+48},{-0x79f8,globals+52},{-0x7bac,globals+56},{-0x7a7c,globals+60},{-0x7b1c,globals+64}})c.put32(toc+slot,pointer);
    c.put32(globals,transfers);c.put32(globals+4,national);c.put32(globals+8,object);c.put32(globals+16,clubs);c.put32(globals+20,random+o.cursor);c.put32(globals+24,globals+0x200);c.put8(globals+28,1);c.put32(globals+32,o.seed);c.put32(globals+36,random+8);c.put32(globals+40,globals+0x300);c.put32(globals+44,globals+0x400);c.put32(globals+48,10);c.put32(globals+52,nations);c.put8(globals+56,1);c.put32(globals+60,globals+0x500);c.put32(globals+64,globals+0x600);
    c.put32(object,contract);c.put32(object+0x34,owned);c.put32(person,0);c.put32(person+0x14,o.player?attributes:0);c.put32(person+0x1c,o.profile?nations:0);c.put32(person+0x24,clubs+0x248);c.put8(person+0x63,std::uint32_t(o.adapt));c.put8(person+0x65,std::uint32_t(o.ambition));c.put8(person+0x67,std::uint32_t(o.trait));
    c.put32(contract+4,1);c.put32(transfers+0x988,shortlist);c.put8(globals+0x208,o.records.size());
    for(unsigned n=0;n<8;++n){c.put32(clubs+n*0x248,n);c.put32(clubs+n*0x248+0x90,o.no_owner?0:attributes+0x200);c.put16(clubs+n*0x248+0x1ec,o.low_reputation?4500:5000);}
    for(unsigned n=0;n<16;++n)c.put16(random+n*2,n&3);
    for(unsigned n=0;n<o.records.size();++n){const auto at=rows+n*0x100;const auto& row=o.records[n];for(unsigned j=0;j<136;++j)c.put8(at+j,j*17+n);c.put32(at,std::uint32_t(row.person_id));c.put32(at+4,std::uint32_t(row.club_id));c.put8(at+0x38,std::uint32_t(row.role)<<2);c.put8(at+0x60,1);c.put8(at+0x64,std::uint32_t(row.state));c.put32(at+0x70,std::uint32_t(row.origin));c.put32(at+0x74,11+n);c.put8(at+0x78,row.kind);c.put8(at+0x79,std::uint32_t(row.active));c.put32(at+0x7c,1);c.put16(at+0x68,100);c.put16(at+0x6a,2001);c.put8(at+0x6c,0);}
    c.put16(today,100);c.put16(today+2,2001);c.put8(today+4,0);contract_manager_pending_review_constants(*s,toc,constants);c.put_f64(constants+0x188,o.minimum_bound);return s;
}
void reference(State& s,Model& model){while(s.pc!=return_sentinel){if(s.pc>=entry&&s.pc<end)require(contract_manager_pending_review_reference(s),"Complete original pending review");else model.call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
void equal_registers(const State& a,const State& b,const std::string& label){for(unsigned n=1;n<32;++n)if(n!=3&&n!=11&&n!=12)require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n)+" "+hex_address(a.r<4>(n*4))+"/"+hex_address(b.r<4>(n*4)));for(unsigned n=1;n<32;++n)if(n==1||n>=14)require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR"+std::to_string(n-1280));for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR");require(a.r<1>(guest::reg::xer_ca)==b.r<1>(guest::reg::xer_ca),label+" carry");require(a.r<4>(guest::reg::lr)==b.r<4>(guest::reg::lr),label+" LR");}
unsigned cases=0,returned=0,stopped=0,negatives=0;std::set<std::uint32_t> reached;
void compare(Options o={}) {
    auto source=seed(o);auto a=clone(*source),b=clone(*source);Model old(o),modern(o);old.o.corrupt_decision=false;old.seed_lists(*a);modern.seed_lists(*b);
    const auto run=[&](State& s,Model& model,bool original){try{if(original)reference(s,model);else{const auto previous=contract_manager_call_hook;contract_manager_call_hook=&hook;active=&model;try{Cpu c(s);cm::aot::contract_manager::review_pending_contracts(c,c.r(3),c.r(4));c.finish();}catch(...){contract_manager_call_hook=previous;active=nullptr;throw;}contract_manager_call_hook=previous;active=nullptr;}return std::string{};}catch(const std::exception& e){return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false),label="pending review case "+std::to_string(cases);
    require(expected.empty()||expected=="Pending review interrupted"||expected=="Null deferred dependency",label+" invalid reference "+expected);require(expected==actual,label+" outcomes "+expected+"/"+actual);
    if(o.interrupt)require(expected=="Pending review interrupted"&&old.calls.size()==o.interrupt,label+" interruption reached");
    else if(o.expect_null_deferred)require(expected=="Null deferred dependency",label+" null deferred boundary reached");
    else require(expected.empty(),label+" expected a complete return");
    if(o.corrupt)b->memory.write<1>(person+8,b->memory.read<1>(person+8)^0x5a);bool rejected=false;
    try{equal_registers(*a,*b,label);require(a->pc==b->pc,label+" PC");for(auto [address,size]:a->memory.mapped_ranges()){const auto x=a->memory.copy_out(address,size),y=b->memory.copy_out(address,size);if(x!=y){const auto at=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(address+std::uint32_t(at.first-x.begin()))+" "+std::to_string(*at.first)+"/"+std::to_string(*at.second));}}if(old.calls!=modern.calls){const auto at=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());std::string detail;if(at.first!=old.calls.end()&&at.second!=modern.calls.end()){const auto& x=*at.first;const auto& y=*at.second;detail=" targets "+hex_address(x.target)+"/"+hex_address(y.target)+" back "+hex_address(x.back)+"/"+hex_address(y.back);for(unsigned n=0;n<std::min(x.args.size(),y.args.size());++n)if(x.args[n]!=y.args[n])detail+=" arg"+std::to_string(n+3)+" "+hex_address(x.args[n])+"/"+hex_address(y.args[n]);for(unsigned n=0;n<std::min(x.kept.size(),y.kept.size());++n)if(x.kept[n]!=y.kept[n])detail+=" r"+std::to_string(n+13)+" "+hex_address(x.kept[n])+"/"+hex_address(y.kept[n]);for(unsigned n=0;n<std::min(x.fp.size(),y.fp.size());++n)if(x.fp[n]!=y.fp[n])detail+=" f"+std::to_string(n+28)+" "+std::to_string(x.fp[n])+"/"+std::to_string(y.fp[n]);}throw std::runtime_error(label+" call "+std::to_string(at.first-old.calls.begin())+detail);}}
    catch(const std::runtime_error&){if(!o.corrupt&&!o.corrupt_decision)throw;rejected=true;}
    require(rejected==(o.corrupt||o.corrupt_decision),label+" negative control");++cases;returned+=expected.empty()&&!rejected;stopped+=!expected.empty();negatives+=rejected;if(expected.empty()&&!rejected)for(const auto& e:old.calls)reached.insert(e.back);
}
}
int main(){try{
    compare();Options empty;empty.records.clear();compare(empty);Options blocked;blocked.blocked=true;compare(blocked);
    for(int role:{-1,2,5,6})for(double score:{2.5,2.75,3.15,4.0}){Options o;o.records[0].role=role;o.records[0].attraction=score;compare(o);}
    for(int value:{0,-100,50,60,70,80,90,99,100,150}){Options o;o.records[0].value=value;compare(o);}
    for(int baseline:{-100,0}){Options o;o.baseline=baseline;compare(o);}
    for(int attribute:{-128,-1,0,15,127})for(unsigned changed:{0u,1u,2u}){Options o;if(changed==0)o.trait=attribute;else if(changed==1)o.adapt=attribute;else o.ambition=attribute;compare(o);}
    for(int value:{59,60,61,69,70,71,79,80,81,89,90,91,98,99,100})for(double score:{std::nextafter(3.15,0.0),3.15,std::nextafter(3.15,4.0),std::nextafter(3.5,0.0),3.5,std::nextafter(3.5,4.0)}){Options o;o.records[0].value=value;o.records[0].attraction=score;compare(o);}
    for(double score:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()})for(int role:{2,5,6}){Options o;o.records[0].role=role;o.records[0].attraction=score;compare(o);}
    for(int state:{1,2,3}){Options o;o.records[0].state=state;compare(o);}
    for(bool primary:{false,true}){Options o;o.primary=primary;o.records[0].club_id=2;compare(o);}
    Options ignored;ignored.records[0].person_id=1;compare(ignored);ignored.records[0].person_id=0;ignored.records[0].active=0;compare(ignored);
    for(bool profile:{false,true})for(bool available:{false,true}){Options o;o.profile=profile;o.national_kind=true;o.national_available=available;compare(o);}
    Options mixed;mixed.profile=true;mixed.mix=true;mixed.records[0].kind=2;compare(mixed);
    Options nonplayer;nonplayer.player=false;compare(nonplayer);nonplayer.profile=true;nonplayer.mix=true;compare(nonplayer);
    Options no_owner;no_owner.no_owner=true;no_owner.records[0].attraction=11;compare(no_owner);
    Options veto;veto.veto=true;veto.expiry_days=89;compare(veto);Options excluded;excluded.profile=true;excluded.exclude=true;compare(excluded);
    for(bool accepted:{false,true})for(bool human:{false,true}){Options o;o.accepted=accepted;o.human=human;o.national_kind=true;compare(o);}
    for(int nation:{-1,0,10,11}){Options o;o.national_kind=true;o.nation=nation;compare(o);}
    for(unsigned cursor:{0u,8u})for(unsigned salt:{0u,65535u}){Options o;o.cursor=cursor;o.seed=salt;o.low_reputation=true;compare(o);}
    for(double score:{4.0,11.0,40.0,1000000000.0}){Options o;o.records[0].attraction=score;o.cursor=8;compare(o);}
    for(unsigned cursor:{0u,2u,4u,6u,8u})for(unsigned salt:{0u,1u,2u,3u})for(unsigned entropy:{0u,1u,2u,3u})for(bool low:{false,true}){Options o;o.records[0].attraction=11;o.cursor=cursor;o.seed=salt;o.entropy=entropy;o.low_reputation=low;compare(o);}
    for(int refined:{-100,0,100}){Options o;o.records[0].attraction=11;o.minimum_bound=70000;o.refined=refined;compare(o);}
    Options two;two.records.push_back(Row{});two.records[1].club_id=2;two.records[1].origin=42;two.records[1].value=150;compare(two);two.fail_allocation=true;two.expect_null_deferred=true;compare(two);
    Options loser=two;loser.fail_allocation=false;loser.expect_null_deferred=false;loser.records[1].value=50;compare(loser);
    Options national_transfer;national_transfer.records[0].origin=42;national_transfer.national_kind=true;compare(national_transfer);
    for(unsigned cut:{1u,4u,8u,12u}){Options o;o.interrupt=cut;compare(o);}
    Options negative;negative.corrupt=true;compare(negative);
    negative.corrupt=false;negative.corrupt_decision=true;compare(negative);
    const auto calls=contract_manager_pending_review_calls();for(auto back:calls)require(reached.contains(back),"Unreached original dependency continuation "+hex_address(back));std::cout<<cases<<" pending-review comparisons; "<<returned<<" returned (including 3 injected-constant edge cases for the otherwise unreachable wide sampler); "<<stopped<<" model stops (partial); "<<negatives<<" mutations detected; "<<reached.size()<<"/"<<calls.size()<<" original dependency continuations reached\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
