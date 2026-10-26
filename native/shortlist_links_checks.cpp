#include "shortlist_links.hpp"
#include "semantic_io_checks.hpp"
#include "shortlist_rewrite_fixture.hpp"
#include <bit>
#include <set>
namespace cm::aot {std::vector<std::uint32_t> shortlist_links_expected_sites();bool shortlist_links_reference(State&);void shortlist_links_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace rf=cm::shortlist_rewrite_fixture;
namespace links=cm::aot::shortlist_links;
constexpr std::uint32_t object=fixture+0x1000,pairs=fixture+0x4000,reverse_pairs=fixture+0x8000,club_table=fixture+0xc000,clubs=fixture+0x10000,rows=fixture+0x14000,staff=fixture+0x18000,contract=fixture+0x1c000,metadata=fixture+0x20000,list=fixture+0x24000,list_data=fixture+0x25000,globals=fixture+0x27000,random=fixture+0x28000,tables=fixture+0x2a000,output=fixture+0x2d000,pool=fixture+0x30000;
constexpr std::array<std::uint32_t,10> ends{0x10362df4,0x10362eec,0x10363128,0x103632e4,0x103636ac,0x10363c98,0x10365c28,0x10365c78,0x10365cbc,0x10365d08};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{links::contains_staff_club_link,links::collect_club_links,links::collect_staff_links,links::mark_primary_contract,links::add_staff_club_link,links::reset_linked_club_rows,links::transition_staff_links,links::clear_primary_contract_flag_one,links::set_primary_contract_flag_two,links::clear_primary_contract_flag_two};
struct Control{std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options{std::vector<Control> controls;unsigned interrupt=0,fail_allocation=0,search_bias=1;std::uint32_t club_result=0,reserved_club_result=0,reserved_staff_result=1,primary=contract,secondary=contract,blocked=0,pending=0,react=0,renewal=65535;bool own_negative=false,child_negative=false,structural=false,compose=false,force_search_miss=false,replace_rows=false;};
struct Trace:SemanticGuestObserver{Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==2)s.wr<2>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(links::entries.begin(),links::entries.end(),root);require(i!=links::entries.end(),"Known shortlist-links root "+hex_address(root));return unsigned(i-links::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace& trace;std::vector<Event> calls;rf::Side side;unsigned allocations=0;std::uint32_t used=0;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(shortlist_links_reference(s),"Complete shortlist-links reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    std::uint32_t reserve(unsigned n){const auto result=pool+used;used+=(n+15)&~15u;require(used<0x1f000,"Bounded fixture allocation");return result;}
    static std::int32_t difference(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a-b);}
    static int order(std::array<std::uint32_t,2>a,std::array<std::uint32_t,2>b,bool club){const unsigned key=club?1:0,tie=club?0:1;if(a[key]!=b[key])return difference(a[key],b[key]);if(a[tie]==0xffffffff||b[tie]==0xffffffff)return 0;return difference(a[tie],b[tie]);}
    static std::array<std::uint32_t,2> pair(State& s,std::uint32_t at){return {std::uint32_t(s.memory.read<4>(at)),std::uint32_t(s.memory.read<4>(at+4))};}
    static void put_pair(State& s,std::uint32_t at,std::array<std::uint32_t,2> p){s.memory.write<4>(at,p[0]);s.memory.write<4>(at+4,p[1]);}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t at){std::vector<std::uint8_t>b;for(unsigned n=0;n<8192;++n){auto c=std::uint8_t(s.memory.read<1>(at+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated shortlist-links fixture text");}
    void append(State& s,std::uint32_t destination,std::array<std::uint32_t,2> value){const auto count=unsigned(s.memory.read<2>(destination));require(count<128,"Bounded fixture list");auto data=std::uint32_t(s.memory.read<4>(destination+4));if(!data){data=reserve(1024);s.memory.write<4>(destination+4,data);s.memory.write<4>(destination+12,data);}put_pair(s,data+count*8,value);s.memory.write<2>(destination,count+1);}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override{
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});const bool counts=side.enter(target,stop);
        try{
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;
            if(!side.composes(target,stop)){s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);}
            const bool own=std::find(links::entries.begin(),links::entries.end(),target)!=links::entries.end();
            if(options.compose&&own){
                if(original)reference(s,stop);else if(side.rewrite)rf::compose(s,target,rf::answer_of(*this));else modern_root(s,this,&trace);
                require(s.pc==stop,"Composed shortlist-links child return");
            }else if(side.composes(target,stop)){
                if(own)reference(s,stop);else rf::compose(s,target,rf::answer_of(*this));
                require(s.pc==stop,"Composed shortlist-links child return");
            }else{
                if(target==0x10775600){
                    require(arg(6)==8,"Fixture eight-byte pair search ABI");const auto comparator=std::uint32_t(s.memory.read<4>(arg(7)));require(comparator==0x10366410||comparator==0x103663b0,"Pinned owned pair comparator descriptor");
                    if(!options.force_search_miss){const auto key=pair(s,first);const auto count=arg(5);require(count<=128,"Bounded fixture search");std::vector<std::uint32_t> found;for(unsigned n=0;n<count;++n)if(order(key,pair(s,second+n*8),comparator==0x103663b0)==0)found.push_back(second+n*8);if(!found.empty())result=found[options.search_bias==0?0:options.search_bias==2?found.size()-1:found.size()/2];}
                }else if(target==0x10768300){
                    require(arg(5)==8,"Fixture eight-byte pair sort ABI");const auto comparator=std::uint32_t(s.memory.read<4>(arg(6)));require(comparator==0x10366410||comparator==0x103663b0,"Pinned owned pair sort descriptor");const auto count=arg(4);require(count<=128,"Bounded fixture sort");
                    // A deterministic dependency fixture, not the original sort
                    // implementation. Insertion is bounded even for sentinel ties.
                    for(unsigned n=1;n<count;++n){const auto value=pair(s,first+n*8);unsigned j=n;while(j&&order(value,pair(s,first+(j-1)*8),comparator==0x103663b0)<0){put_pair(s,first+j*8,pair(s,first+(j-1)*8));--j;}put_pair(s,first+j*8,value);}
                }else if(target==0x10773520||target==0x10775130||target==0x107751b0){
                    ++allocations;if(allocations!=options.fail_allocation){const auto size=target==0x107751b0?arg(4):first;require(size<=4096,"Bounded fixture resize");result=reserve(size);if(target==0x107751b0&&first)s.memory.copy_in(result,s.memory.copy_out(first,std::min(size,800u)));}
                }else if(target==0x10738c00){s.memory.clear(first,16);result=first;}
                else if(target==0x10738ca0){s.memory.write<2>(first,0);s.memory.write<4>(first+12,s.memory.read<4>(first+4));result=first;}
                else if(target==0x10738c20){(void)s.memory.read<2>(first);s.memory.write<2>(first,0);result=0;}
                else if(target==0x10738d10){require(arg(5)==8,"Fixture list append ABI");append(s,first,pair(s,second));result=first;}
                else if(target==0x10738e00){const auto cursor=std::uint32_t(s.memory.read<4>(first+12)),begin=std::uint32_t(s.memory.read<4>(first+4));const auto count=std::int16_t(s.memory.read<2>(first));if(count>0&&cursor>=begin&&cursor<begin+std::uint32_t(count)*8){result=cursor;s.memory.write<4>(first+12,cursor+8);}}
                else if(target==links::collect_staff_links_entry){const auto destination=arg(5);s.memory.write<2>(destination,0);append(s,destination,{std::uint32_t(s.memory.read<4>(second)),1});append(s,destination,{std::uint32_t(s.memory.read<4>(second)),2});result=destination;}
                else if(target==links::transition_staff_links_entry){s.memory.write<4>(globals+0x610,second);s.memory.write<4>(globals+0x614,arg(5));result=1;}
                else if(target==0x1014c2e0)result=options.club_result;
                else if(target==0x1014c490)result=options.reserved_club_result;
                else if(target==0x1030fbf0)result=options.reserved_staff_result;
                else if(target==0x10082f20)result=options.primary;
                else if(target==0x10082f90)result=options.secondary;
                else if(target==0x100d3040)result=options.blocked;
                else if(target==0x103f43b0)result=options.pending;
                else if(target==0x100da0b0)result=options.react;
                else if(target==0x1077b770)result=options.renewal;
                else if(target==0x10717890||target==0x10717b20){const auto days=arg(5);const auto current=std::uint32_t(s.memory.read<2>(second));s.memory.write<2>(first,target==0x10717890?current+days:current-days);s.memory.write<2>(first+2,s.memory.read<2>(second+2));s.memory.write<1>(first+4,3);result=first;if(options.replace_rows)s.memory.write<4>(club_table+12,rows+40*44*2);}
                else if(target==0x100d9b80||target==0x10363ca0||target==0x1008cd20){s.memory.write<4>(globals+0x618,target);result=1;}
                else if(target==0x1035e9f0){s.memory.write<4>(second+0xc,s.memory.read<4>(second+0xc)+1);result=second;}
                else if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
                else if(target==0x10768810)result=output+0x800;
                else if(target==0x1077b720){const auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);result=first;}
                else if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
                else if(target==0x1076f7d0)throw std::runtime_error("Fixture shortlist-links requested exit");
                else throw std::runtime_error("Unknown shortlist-links dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2&&side.clobbers(target,n))s.wr<4>(n*4,0xdd000000+n*0x101);s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;if(side.interrupts(counts,options.interrupt,calls.size()))throw std::runtime_error("Fixture shortlist-links dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x50000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=links::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=1000000;s->wr<4>(12,object);s->wr<4>(16,staff+0x70);s->wr<4>(20,clubs+0x248);s->wr<4>(24,1);s->wr<4>(28,2);s->wr<4>(32,0);
    for(auto [slot,ptr]:{std::pair{-0x7b80,globals+0x100},std::pair{-0x7b7c,globals},std::pair{-0x7b78,globals+4},std::pair{-0x7b40,globals+8},std::pair{-0x7be8,globals+12},std::pair{-0x7be4,globals+16},std::pair{-0x7be0,globals+20},std::pair{-0x7b1c,globals+24},std::pair{-0x7b9c,globals+28},std::pair{-0x7bac,globals+32},std::pair{-0x7ba8,output},std::pair{-0x7b90,globals+36},std::pair{-0x7b8c,random},std::pair{-0x7b88,globals+40},std::pair{-0x7b84,globals+44}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,globals+0x200);s->memory.write<4>(globals+8,globals+0x300);s->memory.write<4>(globals+12,clubs);s->memory.write<4>(globals+16,20);s->memory.write<4>(globals+20,staff);s->memory.write<4>(globals+24,globals+0x400);s->memory.write<4>(globals+28,metadata);s->memory.write<1>(globals+32,1);s->memory.write<4>(globals+36,0);s->memory.write<4>(globals+40,random+0x20);s->memory.write<4>(globals+44,random);s->memory.write<2>(globals+0x100,100);s->memory.write<2>(globals+0x102,2001);for(unsigned k=0;k<18;++k)s->memory.write<2>(random+k*2,10+k*13);
    s->memory.write<4>(object,pairs);s->memory.write<4>(object+4,reverse_pairs);s->memory.write<4>(object+8,100);s->memory.write<4>(object+12,100);s->memory.write<4>(object+16,5);s->memory.write<4>(object+20,5);s->memory.write<4>(object+0xdc,club_table);
    constexpr std::array<std::array<unsigned,2>,5> first{{{0,1},{1,0},{1,1},{1,2},{2,1}}},second{{{1,0},{0,1},{1,1},{2,1},{1,2}}};for(unsigned k=0;k<5;++k){Model::put_pair(*s,pairs+k*8,first[k]);Model::put_pair(*s,reverse_pairs+k*8,second[k]);}
    for(unsigned c=0;c<4;++c){s->memory.write<4>(clubs+c*0x248,c);s->memory.write<4>(clubs+c*0x248+0x90,staff+5*0x70);s->memory.write<4>(club_table+c*12,rows+c*40*44);s->memory.write<1>(club_table+c*12+9,80);for(unsigned k=0;k<40;++k){const auto row=rows+(c*40+k)*44;s->memory.write<1>(row,k%16);s->memory.write<4>(row+8,100);s->memory.write<4>(row+0xc,12);s->memory.write<4>(row+0x10,k<3?1:0xffffffff);s->memory.write<4>(row+0x14,k==0?1:2);s->memory.write<4>(row+0x18,k<3?1:0xffffffff);s->memory.write<1>(row+0x28,0xda);}}
    for(unsigned k=0;k<20;++k){const auto p=staff+k*0x70;s->memory.write<4>(p,k);s->memory.write<4>(p+8,50);s->memory.write<4>(p+0x24,k>=4?clubs+0x248:clubs);s->memory.write<1>(p+0x6b,k==19?1:0);s->memory.write<1>(metadata+k*100+12,0x21);}
    s->memory.write<1>(contract+0x5c,0xac);s->memory.write<1>(contract+0x60,0xe3);s->memory.write<1>(contract+0x4c,10);s->memory.write<2>(list,0);s->memory.write<4>(list+4,list_data);s->memory.write<4>(list+12,list_data);shortlist_links_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});
    if(n==1){s->wr<4>(16,clubs+0x248);s->wr<4>(20,list);}if(n==2){s->wr<4>(20,list);s->wr<4>(24,1);s->wr<4>(28,3);}if(n==3||n==5)s->wr<4>(20,1);return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0,builder_differences=0;std::set<std::uint32_t> sites,completed_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
// The one known difference of the builder owner (no longer in production):
// since add-link inserts a new pair in place instead of re-sorting
// (shortlist_links_storage.cpp, TASKS.md), it skips some of the reference's
// qsort calls (0x10768300). In add-link's cases other than the negative
// controls, a builder whose calls are the reference's without some qsort
// calls may differ in what follows from that: the model's call counter
// (globals+0x600), the access order, the link register the skipped call
// leaves, and an interruption or a fault the reference meets inside a sort.
bool skipped_sort(const State& input,const Options& options,const std::vector<Event>& reference,const std::vector<Event>& builder,const std::string& message){
    if(input.pc!=links::add_staff_club_link_entry||options.own_negative||options.child_negative)return false;
    std::size_t matched=0;
    for(const auto& call:reference){if(matched<builder.size()&&builder[matched].target==call.target&&builder[matched].stop==call.stop){++matched;continue;}if(call.target!=0x10768300)return false;}
    const auto has=[&](const char* text){return message.find(text)!=std::string::npos;};
    // The reference stopped at its last call, a sort (an interruption after
    // it, or a fault in the sorted index); the builder, not sorting, went on.
    const bool stopped_in_sort=!reference.empty()&&reference.back().target==0x10768300&&
        (options.interrupt||has(" original=Unmapped data access at "));
    if(matched!=builder.size()&&!stopped_in_sort)return false;
    // Or an interruption after the n-th call never came for a builder that
    // made fewer calls.
    const bool never_interrupted=options.interrupt&&builder.size()<options.interrupt&&
        has(" original=Fixture shortlist-links dependency interruption modern=")&&message.ends_with(" modern=");
    return (stopped_in_sort&&message.ends_with(" modern="))||never_interrupted||has("memory mismatch at 0x300276")||message.ends_with(" effects")||message.ends_with(" dependency ABI")||
        message.ends_with(" source order")||message.ends_with(" accounting")||
        has("register byte=4128 ")||has("register byte=4129 ")||has("register byte=4130 ")||has("register byte=4131 ");
}
// The same case for the C++ SHORTLIST_MANAGER.CPP (shortlist_rewrite_fixture.hpp).
void rewrite_side(const State& input,const Options& options,const std::string& expected,const std::string& label){
    const char* not_run=!options.controls.empty()||options.structural?"register written at an instruction address":nullptr;
    static Trace unused{Options{}};
    rf::rewrite_side(input,expected,not_run,{label,nullptr,rf::negative(options.own_negative,options.child_negative),{{tables,output}},{}},
        [&](bool reference){return std::make_unique<Model>(reference,options,unused);},[](Model& m,State& s){m.reference(s);});
}
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Shortlist links root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture shortlist-links dependency interruption"||expected=="Fixture shortlist-links requested exit"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First shortlist-links site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    bool rejected=false;try{
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    }catch(const std::runtime_error& e){if(!skipped_sort(input,options,old.calls,modern.calls,e.what()))throw;++builder_differences;}
    rewrite_side(input,options,expected,label);
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=options.structural||!options.controls.empty();if(!rejected&&expected!="Native instruction budget exhausted")sites.insert(ot.sites.begin(),ot.sites.end());if(!rejected&&expected.empty())completed_sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed shortlist-links boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n:std::set<unsigned>{1,2,unsigned(base.calls.size()/2),unsigned(base.calls.size())})if(n&&n<=base.calls.size()){auto o=options;o.interrupt=n;compare(input,o);}
}


void baseline_cases(){for(unsigned n=0;n<10;++n){auto a=seed(n);boundaries(*a);Options o;o.own_negative=true;compare(*a,o);o.own_negative=false;o.child_negative=true;compare(*a,o);}}
void initial_cases(){
    for(unsigned n:{7u,8u,9u})for(unsigned flag=0;flag<256;++flag)for(bool present:{false,true}){auto a=seed(n);a->memory.write<1>(contract+0x5c,flag);Options o;o.primary=present?contract:0;compare(*a,o);}
    for(unsigned n:{0u,1u,2u})for(unsigned bias:{0u,1u,2u}){auto a=seed(n);Options o;o.search_bias=bias;boundaries(*a,o);}
    for(unsigned mode=0;mode<6;++mode)for(unsigned flag:{0u,1u,2u}){auto a=seed(6);a->wr<4>(24,mode);a->wr<4>(28,flag);boundaries(*a);}
    for(unsigned fail=0;fail<=4;++fail){auto a=seed(4);a->memory.write<4>(object,0);a->memory.write<4>(object+4,0);a->memory.write<4>(object+16,0);a->memory.write<4>(object+20,0);Options o;o.fail_allocation=fail;compare(*a,o);}
    for(unsigned n:{2u,5u,6u}){auto a=seed(n);Options o;o.compose=true;compare(*a,o);}
}
void query_cases(){
    for(auto person:{0u,1u,2u,3u})for(auto club:{0u,clubs,clubs+0x248,clubs+3*0x248}){auto a=seed(0);a->wr<4>(16,staff+person*0x70);a->wr<4>(20,club);compare(*a);}
    {auto a=seed(0);a->memory.write<4>(object,0);boundaries(*a);}
    for(auto club:{clubs,clubs+0x248,clubs+3*0x248}){auto a=seed(1);a->wr<4>(16,club);boundaries(*a);}
    for(auto kind:{0u,1u,2u,3u,255u,256u})for(auto reserved:{0u,1u,256u,255u})for(auto qualified:{0u,1u}){auto a=seed(2);a->wr<4>(28,kind);Options o;o.reserved_club_result=reserved;o.reserved_staff_result=qualified;compare(*a,o);}
    for(auto out:{0u,list})for(auto cached:{0u,1u,256u,255u}){auto a=seed(2);a->wr<4>(20,out);a->wr<4>(32,cached);a->wr<4>(28,1);compare(*a);}
    for(auto stored:{0u,clubs,clubs+0x248}){auto a=seed(2);a->memory.write<4>(staff+19*0x70+0x24,stored);a->wr<4>(28,1);boundaries(*a);}
    for(auto force:{0u,1u})for(auto status:{0u,99u,100u,255u})for(auto score:{0u,49u,50u,51u,0xffffffffu}){auto a=seed(2);a->wr<4>(24,force);a->memory.write<1>(club_table+12+9,status);a->memory.write<4>(rows+40*44+8,score);compare(*a);}
    {auto a=seed(2);for(unsigned n=0;n<40;++n)a->memory.write<4>(rows+(40+n)*44+16,0xffffffff);boundaries(*a);}
    {auto a=seed(0);a->wr<4>(20,0);require(compare(*a).state->r<4>(12)==1,"Independent absent-club key matches an existing staff link");}
    {auto a=seed(1);const auto result=compare(*a);require(result.state->memory.read<2>(list)==3,"Independent club lookup collects all three adjacent links");require(Model::pair(*result.state,list_data)==std::array<std::uint32_t,2>{0,1}&&Model::pair(*result.state,list_data+16)==std::array<std::uint32_t,2>{2,1},"Independent club lookup rewinds to the first matching pair");}
    {auto a=seed(2);const auto result=compare(*a);require(result.state->memory.read<2>(list)==2&&Model::pair(*result.state,list_data)==std::array<std::uint32_t,2>{1,1}&&Model::pair(*result.state,list_data+8)==std::array<std::uint32_t,2>{1,2},"Independent staff lookup excludes its stored club");}
}
void storage_cases(){
    for(auto nullable:{0u,1u,2u,3u}){auto a=seed(4);if(nullable&1){a->memory.write<4>(object,0);a->memory.write<4>(object+16,0);}if(nullable&2){a->memory.write<4>(object+4,0);a->memory.write<4>(object+20,0);}a->wr<4>(16,staff+3*0x70);a->wr<4>(20,clubs+3*0x248);boundaries(*a);}
    for(unsigned fail:{0u,1u,2u}){auto a=seed(4);for(unsigned n=0;n<100;++n){Model::put_pair(*a,pairs+n*8,{2,0});Model::put_pair(*a,reverse_pairs+n*8,{2,0});}a->memory.write<4>(object+8,100);a->memory.write<4>(object+12,100);a->memory.write<4>(object+16,100);a->memory.write<4>(object+20,100);Options o;o.fail_allocation=fail;compare(*a,o);}
    for(auto allowed:{0u,1u,256u,255u})for(auto nibble:{0u,0x21u,0xf5u}){auto a=seed(4);a->wr<4>(16,staff+3*0x70);a->wr<4>(20,clubs+3*0x248);a->memory.write<1>(metadata+300+12,nibble);Options o;o.club_result=allowed;compare(*a,o);}
    {auto a=seed(4);a->wr<4>(20,0);compare(*a);a->wr<4>(16,staff+3*0x70);compare(*a);}
    for(auto flag:{0u,1u,128u,256u}){auto a=seed(5);a->wr<4>(20,flag);boundaries(*a);}
    {auto a=seed(4);a->wr<4>(16,staff+3*0x70);a->wr<4>(20,clubs+3*0x248);const auto result=compare(*a);require(result.state->memory.read<4>(object+16)==6&&result.state->memory.read<4>(object+20)==6,"Independent insertion grows both used counts");require(Model::pair(*result.state,pairs+40)==std::array<std::uint32_t,2>{3,3}&&Model::pair(*result.state,reverse_pairs+40)==std::array<std::uint32_t,2>{3,3},"Independent insertion populates both sorted indices");require(result.state->memory.read<1>(metadata+300+12)==0x31,"Independent insertion changes only the upper metadata nibble");}
    {auto a=seed(5);const auto result=compare(*a);require(result.state->memory.read<1>(club_table+12+9)==100&&result.state->memory.read<4>(rows+40*44+16)==1&&result.state->memory.read<4>(rows+40*44+24)==0xffffffff,"Independent linked-row reset retains the primary reference");}
    // Deliberate receiver replacement reaches the post-allocation null-index
    // branch without pretending a fatal allocator failure returned naturally.
    {auto a=seed(4);Options o;o.controls={{0x10363424,25*4,fixture+0x3000,4}};compare(*a,o);}
}
void marker_cases(){
    for(auto blocked:{0u,1u,256u,255u})for(auto primary:{0u,contract})for(auto pending:{0u,1u}){auto a=seed(3);Options o;o.blocked=blocked;o.primary=primary;o.pending=pending;compare(*a,o);}
    for(auto flag:{0u,1u,128u,256u})for(auto threshold:{0u,10u,127u,128u,255u})for(auto react:{0u,1u,256u,255u}){auto a=seed(3);a->wr<4>(20,flag);a->memory.write<1>(contract+0x4c,threshold);Options o;o.react=react;compare(*a,o);}
    for(auto end:{random,random+2,random+4,random+0x20})for(auto salt:{0u,65535u,0xffffffffu}){auto a=seed(3);a->memory.write<4>(globals+40,end);a->memory.write<4>(globals+36,salt);boundaries(*a);}
    {auto a=seed(3);Options o;o.pending=1;const auto result=compare(*a,o);require(result.state->memory.read<1>(contract+0x5c)==0xad&&result.state->memory.read<1>(contract+0x60)==0xe7,"Independent primary marker preserves the high contract status bits");}
    for(unsigned n:{7u,8u,9u}){auto a=seed(n);const auto result=compare(*a);require(result.state->r<4>(12)==(n==8?contract:1),"Independent marker return convention");}
}
void transition_cases(){
    for(auto club:{0u,clubs,clubs+0x248})for(auto mode:{0u,1u,2u,3u,4u,5u,6u,127u,128u,255u,256u}){auto a=seed(6);a->wr<4>(20,club);a->wr<4>(24,mode);compare(*a);}
    for(auto field:{0u,4u,16u,20u}){auto a=seed(6);a->memory.write<4>(object+field,0);boundaries(*a);}
    for(auto type:{0u,11u,128u,255u})for(auto flag:{0u,1u,2u,128u,255u})for(auto mode:{1u,2u,3u,4u,5u}){auto a=seed(6);a->wr<4>(24,mode);a->wr<4>(28,flag);a->memory.write<1>(rows+40*44,type);compare(*a);}
    for(auto mode:{2u,3u,4u,5u})for(auto primary:{0u,1u,0xffffffffu})for(auto previous:{1u,2u,0xffffffffu})for(bool present:{false,true}){auto a=seed(6);a->wr<4>(24,mode);a->memory.write<4>(rows+40*44+16,primary);a->memory.write<4>(rows+40*44+20,previous);Options o;o.secondary=present?contract:0;compare(*a,o);}
    for(auto mode:{1u,2u,3u,4u,5u}){auto a=seed(6);a->wr<4>(24,mode);Options o;o.replace_rows=true;boundaries(*a,o);}
    for(auto allowed:{0u,1u,256u,255u})for(auto nibble:{0x01u,0x21u,0xf5u}){auto a=seed(6);a->memory.write<1>(metadata+100+12,nibble);Options o;o.club_result=allowed;compare(*a,o);}
    for(auto club:{0u,clubs}){auto a=seed(6);a->wr<4>(20,club);a->wr<4>(24,4);Options o;o.compose=true;boundaries(*a,o);}
    {auto a=seed(6);a->wr<4>(24,3);const auto result=compare(*a);require(result.state->memory.read<4>(object+16)==4&&result.state->memory.read<4>(object+20)==4,"Independent transition removes exactly one pair from each index");require(result.state->memory.read<1>(metadata+100+12)==0x11,"Independent paired removal decreases the metadata nibble");for(unsigned n=0;n<3;++n)require(result.state->memory.read<4>(rows+(40+n)*44+16)==0xffffffff&&result.state->memory.read<4>(rows+(40+n)*44+24)==0xffffffff,"Independent withdrawal clears both row references");}
    {auto a=seed(6);a->wr<4>(24,5);const auto result=compare(*a);require(result.state->memory.read<4>(rows+40*44+16)==1&&result.state->memory.read<4>(rows+40*44+20)==1&&result.state->memory.read<4>(rows+40*44+24)==0xffffffff,"Independent deferred reference retains the primary staff ID");require((result.state->memory.read<1>(contract+0x60)&15)==11,"Independent deferred reference copies the row's contract nibble");}
    // This late null-club key is structurally unreachable after the ordinary
    // early return. Restore the club before the subsequent row operation.
    {auto a=seed(6);Options o;o.controls={{0x103656c0,23*4,0,4},{0x10365894,23*4,clubs+0x248,4}};compare(*a,o);}
}
void relocation_cases(){for(unsigned n:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u}){auto a=seed(n);constexpr std::uint32_t moved=0x18008000;a->memory.map(moved-0x8000,0x9000);a->memory.copy_in(moved-0x8000,a->memory.copy_out(toc-0x8000,0x9000));a->wr<4>(8,moved);shortlist_links_fixture_tables(*a,moved,fixture+0x45000);compare(*a);}}
}
namespace cm::aot {void shortlist_links_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{rf::tally.name="Shortlist links";baseline_cases();initial_cases();query_cases();storage_cases();marker_cases();transition_cases();relocation_cases();const auto expected=shortlist_links_expected_sites();for(auto pc:expected)if(!sites.contains(pc))std::cerr<<"Uncovered shortlist-links site "<<hex_address(pc)<<std::endl;std::cout<<"Shortlist links comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/1122 completed_sites="<<completed_sites.size()<<" builder_differences="<<builder_differences<<std::endl;require(sites.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[](auto pc){return sites.contains(pc);}),"Every original shortlist-links site required");rf::finish();}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
