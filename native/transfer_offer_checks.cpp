#include "transfer_offer_queries.hpp"
#include "semantic_io_checks.hpp"
#include <set>
namespace cm::aot {bool transfer_offer_reference(State&);void transfer_offer_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace offers=cm::aot::transfer_offer_queries;
constexpr std::uint32_t manager=fixture+0x2000,staff=fixture+0x4000,clubs=fixture+0x8000,contract=fixture+0xa000,query_record=fixture+0xb000,offer_array=fixture+0xc000,secondary_array=fixture+0xc100,records=fixture+0x10000,list_objects=fixture+0x14000,nodes=fixture+0x18000,flags=fixture+0x20000,globals=fixture+0x24000,tables=fixture+0x30000,output=fixture+0x50000,output_list=fixture+0x54000,auxiliary=fixture+0x58000,append_nodes=fixture+0x60000;
constexpr std::array<std::uint32_t,19> ends{0x103e6358,0x103e6464,0x103e64dc,0x103ea26c,0x103ea28c,0x103ea348,0x103ea43c,0x103ea5f8,0x103ea75c,0x103ea83c,0x103eb014,0x103eb1e4,0x103eb358,0x103eb434,0x103eb464,0x103eb494,0x103f43a4,0x103f4594,0x103fb3f8};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{offers::append_staff_entry,offers::count_club_staff_entries,offers::contains_staff_entry,offers::set_staff_contract_flag,offers::clear_staff_contract_flag,offers::query_staff_contract_flag,offers::rejection_reason_from_squad_status,offers::rejection_reason_allowed,offers::describe_rejection_reason,offers::squad_status_for_rejection_reason,offers::collect_staff_offer_indices,offers::collect_club_staff_offer_indices,offers::collect_secondary_staff_offer_indices,offers::collect_club_offer_indices,offers::primary_offer_slot,offers::secondary_offer_slot,offers::best_qualifying_offer,offers::query_special_staff_offer,offers::offer_is_filtered};
struct Options {unsigned interrupt=0,rules0=0,rules1=0,rival=0,already_present=0;bool own_negative=false,child_negative=false,structural=false,missing_contract=false,missing_association=false;std::uint32_t peer=clubs+0x248,changed_staff_id=0xffffffff;std::vector<std::uint32_t> primary_results;};
struct Trace:SemanticGuestObserver {std::vector<std::uint32_t> sites;void before_step(State&,std::uint32_t pc)override{sites.push_back(pc);}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(offers::entries.begin(),offers::entries.end(),root);require(i!=offers::entries.end(),"Known transfer-offer root "+hex_address(root));return unsigned(i-offers::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned primary_calls=0;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(transfer_offer_reference(s),"Complete transfer-offer reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t at){std::vector<std::uint8_t> b;for(unsigned n=0;n<8192;++n){auto c=std::uint8_t(s.memory.read<1>(at+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated offer text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const bool own=std::find(offers::entries.begin(),offers::entries.end(),target)!=offers::entries.end();
        constexpr std::array allowed{0x10083110u,0x100ce320u,0x10738e00u,0x10738d10u,0x1007f7b0u,0x10082f20u,0x10082f90u,0x1014c2e0u,0x10464160u,0x10739890u,0x10768810u,0x1077b720u,0x10769b60u,0x10727890u,0x10416710u,0x10738c00u,0x10738c20u,0x10738ca0u};
        require(own||std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known transfer-offer dependency "+hex_address(target));const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            if(own){if(original)reference(s,stop);else modern_root(s,this,&trace);}
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;
                s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);
                if(target==0x10083110){(void)s.memory.read<4>(second);result=options.already_present?contract:0;}
                if(target==0x100ce320){(void)s.memory.read<4>(first);result=options.peer;}
                if(target==0x10082f20){(void)s.memory.read<4>(second);result=primary_calls<options.primary_results.size()?options.primary_results[primary_calls]:(options.missing_contract?0:contract);++primary_calls;}
                if(target==0x10082f90){(void)s.memory.read<4>(second);result=options.missing_association?0:contract;}
                if(target==0x1007f7b0){result=options.missing_association?0:contract;if(options.changed_staff_id!=0xffffffff)s.memory.write<4>(staff,options.changed_staff_id);}
                if(target==0x1014c2e0){(void)s.memory.read<4>(first);result=first==clubs?options.rules0:options.rules1;}
                if(target==0x10464160){(void)s.memory.read<4>(first);(void)s.memory.read<4>(second);result=options.rival;}
                if(target==0x10416710)result=std::uint32_t(s.memory.read<4>(first+0x30));
                if(target==0x10738e00){const auto node=std::uint32_t(s.memory.read<4>(first+12));s.memory.write<4>(first+12,node?s.memory.read<4>(node+16):0);result=node;}
                if(target==0x10738c00||target==0x10738c20||target==0x10738ca0){s.memory.clear(first,16);result=first;}
                if(target==0x10738d10){
                    const auto size=arg(5);require(size==4||size==12,"Known list element width");const auto value=s.memory.copy_out(second,size);const auto serial_node=std::uint32_t(s.memory.read<4>(globals+0x80));require(serial_node<256,"Bounded offer fixture append arena");const auto node=append_nodes+serial_node*32;s.memory.write<4>(globals+0x80,serial_node+1);s.memory.copy_in(node,value);s.memory.write<4>(node+16,0);
                    const auto tail=std::uint32_t(s.memory.read<4>(first+8));if(tail)s.memory.write<4>(tail+16,node);else s.memory.write<4>(first+4,node);s.memory.write<4>(first+8,node);s.memory.write<4>(first,s.memory.read<4>(first)+1);result=first;
                }
                if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
                if(target==0x10768810)result=output+0x800;
                if(target==0x10727890||target==0x1077b720){const auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);result=first;}
                if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-70.0-n));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture transfer-offer dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
void list(State& s,std::uint32_t object,std::uint32_t arena,std::initializer_list<std::uint32_t> values){s.memory.clear(object,16);unsigned n=0;for(auto v:values){const auto node=arena+n*32;s.memory.write<4>(node,v);s.memory.write<4>(node+16,n+1<values.size()?node+32:0);++n;}s.memory.write<4>(object,n);s.memory.write<4>(object+4,n?arena:0);s.memory.write<4>(object+8,n?arena+(n-1)*32:0);s.memory.write<4>(object+12,n?arena:0);}
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned b=1280;b<1312;++b)s->wr<1>(b,0);s->memory.map(fixture,0x80000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=offers::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,output_list);s->wr<4>(24,0);
    for(auto [slot,ptr]:{std::pair{-0x7be0,globals},std::pair{-0x7be8,globals+4},std::pair{-0x7b7c,globals+8},std::pair{-0x7b78,globals+12},std::pair{-0x7be4,globals+16},std::pair{-0x7ba8,globals+0x400},std::pair{-0x7bac,globals+0x800}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,staff);s->memory.write<4>(globals+4,clubs);s->memory.write<4>(globals+8,manager+0x1000);s->memory.write<4>(globals+16,4);s->memory.write<1>(globals+0x800,1);
    for(unsigned k=0;k<4;++k){s->memory.write<4>(staff+k*0x70,k);s->memory.write<4>(staff+k*0x70+0x24,clubs+(k%2)*0x248);s->memory.write<2>(staff+k*0x70+0x5c,20);s->memory.write<4>(clubs+k*0x248,k);s->memory.write<4>(clubs+k*0x248+0x30,17+k);}
    s->memory.write<4>(manager,offer_array);s->memory.write<4>(manager+4,4);s->memory.write<4>(manager+12,secondary_array);s->memory.write<4>(manager+16,2);s->memory.write<4>(manager+0x9a0,flags);s->memory.write<4>(manager+0xa50,auxiliary);s->memory.write<4>(manager+0xa58,3);
    for(unsigned k=0;k<4;++k){s->memory.write<4>(offer_array+k*4,records+k*0x100);s->memory.write<4>(records+k*0x100+8,k%2);s->memory.write<4>(records+k*0x100+12,0);s->memory.write<4>(records+k*0x100+0x28,k);s->memory.write<4>(records+k*0x100+0x30,k*100);s->memory.write<1>(records+k*0x100+0x34,2);}
    for(unsigned k=0;k<2;++k){s->memory.write<4>(secondary_array+k*4,records+(k+2)*0x100);s->memory.write<4>(records+(k+2)*0x100+4,list_objects+0x100+k*0x20);list(*s,list_objects+0x100+k*0x20,nodes+0x100+k*0x100,{1,0});}
    s->memory.write<4>(records,list_objects);list(*s,list_objects,nodes,{1,2,3});list(*s,manager+0xa5c,nodes+0x400,{1,0,2});s->memory.write<4>(nodes+0x404,0x23);s->memory.write<4>(nodes+0x408,0x4567);
    s->memory.write<1>(flags,0x50);s->memory.write<1>(flags+1,1);s->memory.write<1>(flags+2,2);s->memory.write<4>(contract+4,0);s->memory.write<1>(contract+0x60,2);s->memory.write<4>(query_record+8,1);s->memory.write<4>(query_record+12,0);s->memory.write<4>(auxiliary+8,1);s->memory.write<4>(auxiliary+20,2);s->memory.write<4>(auxiliary+32,3);
    transfer_offer_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});
    if(n==0){s->wr<4>(20,7);s->wr<4>(24,123);}
    if(n==1||n==13||n==16)s->wr<4>(n==16?20:16,clubs);
    if(n==6)s->wr<4>(16,query_record);
    if(n==7){s->wr<4>(16,0x800);s->wr<4>(20,query_record);}
    if(n==8){s->wr<4>(16,0x200);s->wr<4>(20,output);s->wr<4>(24,200);}
    if(n==9)s->wr<4>(20,0x200);
    if(n==14||n==15)s->wr<4>(16,1);
    if(n==17)s->wr<4>(20,output);
    if(n==18)s->wr<4>(16,records);
    return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot,nt;Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Transfer offer root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture transfer-offer dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First transfer-offer site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=options.structural;if(expected.empty()&&!rejected)sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed transfer-offer boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}

void membership_cases(){
    for(unsigned v=0;v<256;++v)for(auto n:{3u,4u}){auto s=seed(n);s->memory.write<1>(flags,v);const auto r=compare(*s);require(r.state->memory.read<1>(flags)==(n==3?(v|0x40):(v&~0x40u)),"Independent staff flag edit");}
    for(auto present:{0u,1u}){auto s=seed(0);Options o;o.already_present=present;boundaries(*s,o);}
    for(auto arg:{0u,clubs,clubs+0x248,clubs+0x490}){auto s=seed(1);s->wr<4>(16,arg);Options o;o.peer=0;boundaries(*s,o);}
    {auto s=seed(1);s->wr<4>(16,clubs+0x490);s->memory.write<4>(staff+0x24,0);Options o;o.peer=0;boundaries(*s,o);}
    for(auto id:{0u,1u,2u,3u,4u,0xffffffffu}){auto s=seed(2);s->memory.write<4>(staff,id);boundaries(*s);}
    {auto s=seed(2);list(*s,manager+0xa5c,nodes+0x400,{});boundaries(*s);}
    for(auto bits:{0u,0x40u,0x41u,255u})for(auto found:{false,true})for(auto pointer:{0u,output,flags}){
        auto s=seed(5);s->memory.write<1>(flags,bits);s->wr<4>(20,pointer);Options o;o.missing_association=!found;boundaries(*s,o);
    }
    {auto s=seed(5);s->memory.write<1>(flags+10,255);Options o;o.missing_association=true;o.changed_staff_id=1;const auto r=compare(*s,o);require(r.state->memory.read<1>(flags)==0x50&&r.state->memory.read<1>(flags+10)==0xbf,"Independent stale marker reload");}
    for(auto n:{3u,4u,5u})for(auto id:{4u,0x80000000u,0xffffffffu}){auto s=seed(n);s->memory.write<4>(staff,id);compare(*s);}
    for(auto n:{14u,15u})for(auto count:{0u,1u,4u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto pos:{0u,1u,3u,4u,5u,0x7fffffffu,0x80000000u,0xffffffffu}){
        auto s=seed(n);s->memory.write<4>(manager+(n==14?4:16),count);s->wr<4>(16,pos);const auto r=compare(*s);const auto base=n==14?offer_array:secondary_array;const auto expected=std::int32_t(pos)<std::int32_t(count)?base+pos*4:base;require(r.state->r<4>(12)==expected,"Independent signed offer slot");
    }
}
constexpr std::array reasons{0u,10u,0x200u,0x400u,0x800u,0x1000u,0x2000u,0xffffu,0x10002000u,0xffff0200u};
void policy_cases(){
    for(unsigned status=0;status<256;++status){auto s=seed(6);s->memory.write<1>(contract+0x60,status);const auto r=compare(*s);const auto low=status&15u,expected=low==1?0x200u:low==2||low==3?0x400u:low==5?0x2000u:0u;require(r.state->r<4>(12)==expected,"Independent squad rejection reason");}
    for(unsigned status=0;status<256;++status)for(auto reason:reasons){auto s=seed(9);s->memory.write<1>(contract+0x60,status);s->wr<4>(20,reason);const auto r=compare(*s);const auto low=std::int16_t(std::uint16_t(reason));const auto expected=low==0x200?1u:low==0x400?2u:low==0x2000?5u:status&15u;require(r.state->r<4>(12)==expected,"Independent reason-specific squad update");}
    for(auto n:{6u,7u,9u})for(auto missing:{0u,1u,2u}){auto s=seed(n);Options o;o.missing_contract=missing==1;o.missing_association=missing==2;boundaries(*s,o);}
    for(auto age:{0u,23u,24u,25u,127u,128u,255u,256u,279u,280u,32767u,32768u,65535u}){auto s=seed(7);s->wr<4>(16,0x2000);s->memory.write<2>(staff+0x5c,age);const auto r=compare(*s);require(r.state->r<4>(12)==unsigned(std::int8_t(std::uint8_t(age))<24),"Independent cached-age truncation");}
    for(auto reason:reasons)for(auto division:{17u,18u})for(auto missing:{false,true}){auto s=seed(7);s->wr<4>(16,reason);s->memory.write<4>(clubs+0x248+0x30,division);Options o;o.missing_contract=missing;boundaries(*s,o);}
    for(auto gate0:{0u,1u,128u,255u,256u})for(auto gate1:{0u,1u,128u,255u,256u})for(auto relation:{0u,1u,256u}){auto s=seed(7);Options o;o.rules0=gate0;o.rules1=gate1;o.rival=relation;compare(*s,o);}
    for(auto side:{0u,1u})for(auto offset:{0x74u,0x78u,0x7cu}){auto s=seed(7);s->memory.write<4>(clubs+(side?0:0x248)+offset,clubs+(side?0x248:0));boundaries(*s);}
    {auto s=seed(7);s->memory.map(0,0x1000);s->memory.copy_in(0,s->memory.copy_out(clubs,0x920));s->memory.write<4>(globals+4,0);s->memory.write<4>(query_record+8,0);Options o;o.structural=true;boundaries(*s,o);}
    for(auto reason:reasons)for(auto size:{0u,1u,49u,50u,51u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(8);s->wr<4>(16,reason);s->wr<4>(24,size);boundaries(*s);}
    for(auto reason:{0x100u,0x600u,0x1800u}){auto s=seed(8);s->wr<4>(16,reason);boundaries(*s);}
    {auto s=seed(8);s->wr<4>(20,manager);boundaries(*s);}
}
void active_offers(State& s,unsigned kind=0){for(unsigned n=0;n<4;++n)s.memory.write<1>(records+n*0x100+0x34,kind);}
void collection_cases(){
    for(auto n:{10u,11u,12u,13u})for(auto kind:{0u,2u,3u,4u,7u,15u,16u,19u,20u,255u})for(auto out:{0u,output_list}){
        auto s=seed(n);active_offers(*s,kind);s->wr<4>(20,out);boundaries(*s);
    }
    for(auto mode:{0u,1u,255u,256u,257u,0x80000000u,0xffffffffu}){auto s=seed(10);s->wr<4>(24,mode);boundaries(*s);}
    for(auto n:{10u,11u,12u})for(auto id:{0u,1u,3u,4u,0x80000000u,0xffffffffu}){auto s=seed(n);s->memory.write<4>(staff,id);compare(*s);}
    for(auto n:{10u,11u,12u,13u})for(auto variant:{0u,1u,2u,3u,4u,5u,6u}){
        auto s=seed(n);active_offers(*s);Options o;
        if(variant==0)s->memory.write<4>(manager+4,0);
        if(variant==1)s->memory.write<4>(offer_array,0);
        if(variant==2)s->memory.write<4>(records+12,3);
        if(variant==3)s->memory.write<4>(records,0);
        if(variant==4){for(unsigned i=0;i<4;++i)s->memory.write<4>(records+i*0x100+8,3);}
        if(variant==5)o.missing_contract=true;
        if(variant==6){s->memory.write<4>(records+0x200+4,0);s->memory.write<4>(records+0x300+4,0);}
        boundaries(*s,o);
    }
    for(auto count:{0u,1u,2u,3u,255u}){auto s=seed(11);active_offers(*s);s->memory.write<1>(flags+2,count);boundaries(*s);}
    for(auto n:{11u,12u}){auto s=seed(n);active_offers(*s);list(*s,list_objects+0x100,nodes+0x100,{8,7});list(*s,list_objects+0x120,nodes+0x200,{});boundaries(*s);}
    {auto s=seed(10);active_offers(*s);s->memory.write<4>(offer_array+4,0);s->memory.write<4>(records+0x200+12,1);boundaries(*s);}
    {auto s=seed(11);Options o;o.primary_results={contract,0};compare(*s,o);}
    {auto s=seed(11);s->memory.map(0,4);Options o;o.primary_results={contract,0};o.structural=true;const auto r=compare(*s,o);require(r.state->r<4>(12)==0,"Independent mapped-zero repeated contract lookup");}
    {auto s=seed(13);active_offers(*s);const auto r=compare(*s);require(r.state->r<4>(12)==2,"Independent club offer count");}
}
void selection_cases(){
    for(unsigned type=0;type<256;++type)for(auto subtype:{0u,1u,2u,3u,4u,255u}){auto s=seed(18);s->memory.write<1>(records+0x34,type);s->memory.write<1>(records+0x36,subtype);const auto r=compare(*s);const bool expected=(type==2||type==3||type==4||type==16||type==20||(type==15&&subtype>=3));require(r.state->r<4>(12)==expected,"Independent offer filter classes");}
    for(auto score1:{0u,1u,100u,0x7fffffffu,0x80000000u,0xfffffffeu,0xffffffffu})for(auto score3:{0u,1u,100u,0x7fffffffu,0x80000000u,0xfffffffeu,0xffffffffu}){
        auto s=seed(16);s->wr<4>(20,clubs+0x248);s->memory.write<4>(records+0x100+0x30,score1);s->memory.write<4>(records+0x300+0x30,score3);const auto r=compare(*s);std::uint32_t expected=0;std::int32_t best=-1;for(auto k:{1u,3u}){auto v=std::int32_t(k==1?score1:score3);if(v>best){best=v;expected=records+k*0x100;}}require(r.state->r<4>(12)==expected,"Independent strict maximum and first tie");
    }
    {auto s=seed(16);s->wr<4>(20,clubs+0x248);list(*s,list_objects,nodes,{1});s->memory.write<4>(records+0x200,list_objects+0x20);list(*s,list_objects+0x20,nodes+0x80,{3});s->memory.write<4>(records+0x300+0x30,5000);const auto r=compare(*s);require(r.state->r<4>(12)==records+0x100,"Independent first matching offer family");}
    for(auto n:{16u,17u})for(auto variant:{0u,1u,2u,3u,4u,5u}){auto s=seed(n);if(variant==0)s->memory.write<1>(flags+(n==16?1:0),0);if(variant==1)s->memory.write<4>(manager+4,0);if(variant==2)s->memory.write<4>(offer_array,0);if(variant==3)s->memory.write<4>(records+12,1);if(variant==4)s->memory.write<4>(records,0);if(variant==5){active_offers(*s);s->memory.write<1>(records+0x35,1);}boundaries(*s);}
    for(auto kind:{0u,1u,2u,19u})for(auto club:{0u,1u})for(auto override_club:{0u,1u,255u}){auto s=seed(16);s->memory.write<4>(records,0);s->memory.write<1>(records+0x34,kind);s->memory.write<4>(records+8,club);s->memory.write<1>(records+0x35,override_club);boundaries(*s);}
    for(auto at:{1u,2u,3u})for(auto pointer:{0u,output})for(auto auxiliary_count:{0u,1u,3u}){auto s=seed(17);active_offers(*s);s->memory.write<1>(records+at*0x100+0x34,19);s->wr<4>(20,pointer);s->memory.write<4>(manager+0xa58,auxiliary_count);const auto r=compare(*s);require(r.state->r<4>(12)==1,"Independent special offer result");}
    {auto s=seed(17);active_offers(*s);s->memory.write<4>(records+0x300+12,1);s->memory.write<4>(records+0x300+8,0);s->memory.write<1>(records+0x300+0x34,19);boundaries(*s);}
}

void relocation_cases(){
    for(auto n:{0u,1u,5u,6u,7u,8u,9u,11u,12u,17u}){auto s=seed(n);constexpr std::uint32_t other_toc=0x18008000;s->memory.map(other_toc-0x8000,0x9000);s->memory.copy_in(other_toc-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,other_toc);transfer_offer_fixture_tables(*s,other_toc,tables+0x10000);Options o;if(n==0)o.already_present=1;if(n==1)s->wr<4>(16,0);boundaries(*s,o);}
}

}
namespace cm::aot {void transfer_offer_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{
    for(unsigned n=0;n<offers::entries.size();++n){auto s=seed(n);boundaries(*s);Options negative;negative.own_negative=true;compare(*s,negative);const auto base=compare(*s);if(!base.calls.empty()){negative={};negative.child_negative=true;compare(*s,negative);}std::cout<<"PASS root="<<hex_address(offers::entries[n])<<std::endl;}
    membership_cases();policy_cases();collection_cases();selection_cases();relocation_cases();
    if(sites.size()!=1111)for(auto pc:sites)std::cout<<"Covered "<<hex_address(pc)<<"\n";
    require(sites.size()==1111,"Every original transfer-offer source site covered: "+std::to_string(sites.size())+"/1111");
    std::cout<<"Transfer offer comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/1111"<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
