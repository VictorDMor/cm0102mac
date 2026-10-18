#include "shortlist_category.hpp"
#include "semantic_io_checks.hpp"
#include <set>
namespace cm::aot {std::vector<std::uint32_t> shortlist_category_expected_sites();bool shortlist_category_reference(State&);void shortlist_category_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace category=cm::aot::shortlist_category;
constexpr std::uint32_t object=fixture+0x1000,club_table=fixture+0x2000,club=fixture+0x4000,rows=fixture+0x8000,other_rows=fixture+0x9000,staff=fixture+0x10000,profile=fixture+0x20000,nonplayer=fixture+0x24000,contract=fixture+0x25000,stamp=fixture+0x26000,other_stamp=fixture+0x26100,date=fixture+0x26200,globals=fixture+0x30000,tables=fixture+0x34000,output=fixture+0x3c000,pair=fixture+0x3e000,other_pair=fixture+0x3e100;
constexpr std::array<std::uint32_t,5> ends{0x103660a4,0x10366250,0x103663b0,0x10366410,0x10366470};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{category::eligible_for_category,category::count_and_prune_club_rows,category::refresh_unassigned_rows,category::compare_pairs_by_second,category::compare_pairs_by_first};
struct Control{std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options {std::vector<Control> controls;unsigned interrupt=0;std::uint32_t club_result=1,transfer_result=1,primary=contract,day=80,year=2001;bool own_negative=false,child_negative=false,structural=false,replace_rows=false,replace_stamp=false;};
struct Trace:SemanticGuestObserver{Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(category::entries.begin(),category::entries.end(),root);require(i!=category::entries.end(),"Known shortlist category root "+hex_address(root));return unsigned(i-category::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(shortlist_category_reference(s),"Complete shortlist category reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t at){std::vector<std::uint8_t>b;for(unsigned n=0;n<8192;++n){auto c=std::uint8_t(s.memory.read<1>(at+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated category fixture text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override{
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try{
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;
            s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);
            if(target==0x1014c2e0)result=options.club_result;
            else if(target==0x10717b20){require(arg(5)==20,"Original twenty-day subtraction argument");(void)s.memory.read<4>(second);s.memory.write<2>(first,options.day);s.memory.write<2>(first+2,options.year);s.memory.write<1>(first+4,3);result=first;if(options.replace_stamp)s.memory.write<4>(club+0x248+0x90,other_stamp);}
            else if(target==0x103eaea0){result=options.transfer_result;if(options.replace_rows)s.memory.write<4>(club_table+12,other_rows);}
            else if(target==0x10082f20){result=options.primary;if(options.replace_rows)s.memory.write<4>(club_table+12,other_rows);}
            else if(target==0x10365590){s.memory.write<4>(globals+0x610,second);s.memory.write<4>(globals+0x614,arg(7));s.memory.write<4>(globals+0x618,arg(8));s.memory.write<4>(globals+0x61c,s.memory.read<4>(globals+0x61c)+1);}
            else if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
            else if(target==0x10768810)result=output+0x800;
            else if(target==0x1077b720){const auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);result=first;}
            else if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
            else if(target==0x12345678)result=42;
            else throw std::runtime_error("Unknown category dependency "+hex_address(target));
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture shortlist-category dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x50000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=category::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,object);s->wr<4>(16,n==0?staff:club+0x248);s->wr<4>(20,n==0?0:0xffffffff);
    for(auto [slot,ptr]:{std::pair{-0x7b80,date},std::pair{-0x7b40,globals},std::pair{-0x7be0,globals+4},std::pair{-0x7b7c,globals+8},std::pair{-0x7b78,globals+12},std::pair{-0x7be4,globals+16},std::pair{-0x7be8,globals+20},std::pair{-0x7bac,globals+24},std::pair{-0x7ba8,output}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,globals+0x100);s->memory.write<4>(globals+4,staff);s->memory.write<4>(globals+8,globals+0x200);s->memory.write<4>(globals+16,20);s->memory.write<4>(globals+20,club);s->memory.write<1>(globals+24,1);s->memory.write<4>(object+0xdc,club_table);s->memory.write<4>(club_table+12,rows);s->memory.write<1>(club_table+16,2);s->memory.write<4>(club+0x248,1);s->memory.write<4>(club+0x248+0x90,stamp);
    s->memory.write<2>(date,100);s->memory.write<2>(date+2,2001);s->memory.write<2>(stamp+0x4c,70);s->memory.write<2>(stamp+0x4e,2001);s->memory.write<2>(other_stamp+0x4c,90);s->memory.write<2>(other_stamp+0x4e,2001);
    for(unsigned k=0;k<20;++k){s->memory.write<4>(staff+k*0x70,k);s->memory.write<4>(staff+k*0x70+0x14,profile+k*0x40);s->memory.write<4>(staff+k*0x70+0x1c,nonplayer+k*16);s->memory.write<4>(staff+k*0x70+0x24,club);s->memory.write<1>(staff+k*0x70+0x62,11);for(unsigned off=15;off<=25;++off)s->memory.write<1>(profile+k*0x40+off,20);}
    for(unsigned k=0;k<40;++k){s->memory.write<1>(rows+k*44,k%16);s->memory.write<4>(rows+k*44+0x10,k<3?k:0xffffffff);s->memory.write<4>(rows+k*44+0x14,0xabcdef01);s->memory.write<4>(rows+k*44+0x18,n==2?0xffffffff:1);}
    s->memory.copy_in(other_rows,s->memory.copy_out(rows,40*44));s->memory.write<4>(contract+4,0);s->memory.write<1>(contract+0x38,0);s->memory.write<4>(pair,1);s->memory.write<4>(pair+4,100);s->memory.write<4>(other_pair,2);s->memory.write<4>(other_pair+4,100);shortlist_category_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});
    if(n==2)s->wr<4>(20,club_table+12);if(n>=3){s->wr<4>(12,pair);s->wr<4>(16,other_pair);}return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites,completed_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Shortlist category root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture shortlist-category dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First shortlist-category site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=options.structural||!options.controls.empty();if(!rejected&&expected!="Native instruction budget exhausted")sites.insert(ot.sites.begin(),ot.sites.end());if(!rejected&&expected.empty())completed_sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed shortlist-category boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n:std::set<unsigned>{1,2,unsigned(base.calls.size()/2),unsigned(base.calls.size())})if(n&&n<=base.calls.size()){auto o=options;o.interrupt=n;compare(input,o);}
}

void baseline_cases(){for(unsigned n=0;n<5;++n){auto a=seed(n);boundaries(*a);Options o;o.own_negative=true;compare(*a,o);if(n<3){o.own_negative=false;o.child_negative=true;if(n==0)a->wr<4>(20,16);compare(*a,o);}}}
void eligibility_cases(){
    for(unsigned code=0;code<256;++code)for(bool present:{false,true}){auto a=seed(0);a->wr<4>(20,code);if(!present)a->memory.write<4>(staff+0x14,0);compare(*a);}
    for(unsigned code=0;code<15;++code)for(auto value:{0u,14u,15u,127u,128u,255u})for(unsigned off=15;off<=25;++off){auto a=seed(0);a->wr<4>(20,code);a->memory.write<1>(profile+off,value);compare(*a);}
    for(auto code:{15u,16u,255u,256u,271u,0x80000000u,0xffffffffu}){auto a=seed(0);a->wr<4>(20,code);boundaries(*a);}
    for(auto role:{0u,10u,11u,12u,13u,14u,15u,16u,127u,128u,255u})for(auto active:{0u,1u,256u,255u})for(auto ptr:{0u,club}){auto a=seed(0);a->wr<4>(20,15);a->memory.write<1>(staff+0x62,role);a->memory.write<4>(staff+0x24,ptr);Options o;o.club_result=active;compare(*a,o);}
    for(auto person:{0u,3u,4u,19u,20u,0x80000000u,0xffffffffu})for(auto ptr:{0u,nonplayer}){auto a=seed(0);a->wr<4>(20,15);a->memory.write<4>(staff,person);a->memory.write<4>(staff+0x1c,ptr);a->memory.write<4>(staff+0x24,0);compare(*a);}
    {auto a=seed(0);a->memory.write<1>(profile+15,14);require(compare(*a).state->r<4>(12)==0,"Independent category-zero threshold below fifteen");a->memory.write<1>(profile+15,15);require(compare(*a).state->r<4>(12)==1,"Independent category-zero threshold inclusive");a->memory.write<1>(profile+15,255);require(compare(*a).state->r<4>(12)==0,"Independent signed qualification byte");}
    for(auto target:{0x10365d74u,0x10365d78u,0x10365d7cu,0x10365d80u,0x10365d84u,0x10365d88u,0x10365d9cu,0x10365da0u,0x10365da4u,0x10365da8u,0x10365dacu,0x10365db0u,0x10366084u,0x10366088u,0x12345678u}){auto a=seed(0);a->memory.write<4>(tables+0x1000,target);Options o;o.structural=true;compare(*a,o);}
}
void pruning_cases(){
    for(auto code:{0u,1u,2u,11u,127u,128u,255u,256u,0xffffffffu}){auto a=seed(1);a->wr<4>(20,code);boundaries(*a);}
    for(auto year:{0u,2000u,2001u,2002u,65535u})for(auto day:{0u,69u,70u,71u,32767u,32768u,65535u}){auto a=seed(1);Options o;o.year=year;o.day=day;compare(*a,o);}
    for(auto flag:{0u,1u,2u,3u,128u,255u})for(auto allowed:{0u,1u,256u,255u})for(auto found:{0u,1u,256u,0xffffffffu}){auto a=seed(1);a->memory.write<1>(club_table+16,flag);Options o;o.club_result=allowed;o.transfer_result=found;compare(*a,o);}
    {auto a=seed(1);Options o;o.transfer_result=0;const auto r=compare(*a,o);require(r.state->r<4>(12)==0,"Independent pruning count");for(unsigned n=0;n<3;++n)require(r.state->memory.read<4>(rows+n*44+16)==0xffffffff&&r.state->memory.read<4>(rows+n*44+24)==0xffffffff&&r.state->memory.read<4>(rows+n*44+20)==0xabcdef01,"Independent two-column invalidation");}
    for(auto field:{16u,24u})for(auto value:{0u,0xffffffffu,0x80000000u,0x7fffffffu}){auto a=seed(1);a->memory.write<4>(rows+field,value);Options o;o.club_result=0;compare(*a,o);}
    for(bool swap:{false,true}){auto a=seed(1);Options o;o.replace_rows=swap;o.transfer_result=0;boundaries(*a,o);o.replace_stamp=true;boundaries(*a,o);}
}
void refresh_cases(){
    for(auto id:{0u,19u,20u,21u,0xffffffffu,0x80000000u,0x7fffffffu}){auto a=seed(2);a->memory.write<4>(rows+16,id);boundaries(*a);}
    for(unsigned kind=0;kind<256;++kind){auto a=seed(2);a->memory.write<1>(contract+0x38,kind);compare(*a);}
    for(auto target:{0u,club,club+0x248})for(auto owner:{0u,club,club+0x248})for(auto primary:{0u,contract}){auto a=seed(2);a->wr<4>(16,target);a->memory.write<4>(staff+0x24,owner);Options o;o.primary=primary;compare(*a,o);}
    for(auto code:{0u,10u,11u,12u,127u,128u,255u}){auto a=seed(2);a->memory.write<1>(rows,code);const auto r=compare(*a);const auto event=std::find_if(r.calls.begin(),r.calls.end(),[](auto& x){return x.target==0x10365590;});require(event!=r.calls.end(),"Independent assignment dependency reached");std::uint32_t value=0;for(unsigned n=28;n<32;++n)value=(value<<8)|event->before[n];require(value==(code==11?0u:1u),"Independent category-eleven flag");}
    {auto a=seed(2);a->memory.write<4>(rows+16,20);const auto r=compare(*a);require(r.state->memory.read<4>(rows+16)==0xffffffff,"Independent out-of-range staff index invalidation");}
    {auto a=seed(2);Options o;o.replace_rows=true;boundaries(*a,o);}
}
void relocation_cases(){
    for(unsigned n:{0u,1u,2u}){auto a=seed(n);constexpr std::uint32_t moved=0x18008000;a->memory.map(moved-0x8000,0x9000);a->memory.copy_in(moved-0x8000,a->memory.copy_out(toc-0x8000,0x9000));a->wr<4>(8,moved);shortlist_category_fixture_tables(*a,moved,tables+0x10000);if(n==0)a->wr<4>(20,16);compare(*a);}
    {auto a=seed(2);a->memory.write<4>(rows+16,20);a->memory.write<4>(rows+24,123);const auto r=compare(*a);require(r.state->memory.read<4>(rows+16)==0xffffffff&&r.state->memory.read<4>(rows+24)==123,"Independent invalid-index cleanup preserves the other status column");}
}
void comparison_cases(){
    for(unsigned n:{3u,4u})for(auto x:{0u,1u,0xffffffffu,0x7fffffffu,0x80000000u})for(auto y:{0u,1u,0xffffffffu,0x7fffffffu,0x80000000u})for(auto k:{0u,4u}){auto a=seed(n);a->memory.write<4>(pair+k,x);a->memory.write<4>(other_pair+k,y);compare(*a);}
    for(unsigned n:{3u,4u})for(auto left:{0u,pair})for(auto right:{0u,pair,other_pair}){auto a=seed(n);a->wr<4>(12,left);a->wr<4>(16,right);const auto r=compare(*a);if(!left)require(r.state->r<4>(12)==0xffffffff,"Independent null-first ordering includes both-null");}
    for(unsigned n:{3u,4u}){auto a=seed(n);const auto key=n==3?4u:0u,tie=n==3?0u:4u;a->memory.write<4>(pair+key,0x80000000);a->memory.write<4>(other_pair+key,1);require(compare(*a).state->r<4>(12)==0x7fffffff,"Independent wrapping primary subtraction");a->memory.write<4>(other_pair+key,0x80000000);a->memory.write<4>(pair+tie,0xffffffff);require(compare(*a).state->r<4>(12)==0,"Independent secondary sentinel tie");}
}
}
namespace cm::aot {void shortlist_category_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{baseline_cases();eligibility_cases();pruning_cases();refresh_cases();relocation_cases();comparison_cases();const auto expected=shortlist_category_expected_sites();for(auto pc:expected)if(!sites.contains(pc))std::cerr<<"Uncovered category site "<<hex_address(pc)<<std::endl;require(sites.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[](auto pc){return sites.contains(pc);}),"Every original shortlist category site required");std::cout<<"Shortlist category comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/467 completed_sites="<<completed_sites.size()<<std::endl;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
