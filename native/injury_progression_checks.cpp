#include "injury_progression.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool injury_progression_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace injury=cm::aot::injury_progression;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,injuries=fixture+0x8000;
constexpr std::uint32_t profile=fixture+0x14000,details=fixture+0x18000,descriptors=fixture+0x1a000;
constexpr std::uint32_t objects=fixture+0x1c000,globals=fixture+0x1d000,pool=fixture+0x1d100,constants=fixture+0x1d800;
constexpr std::uint32_t stats=fixture+0x1f000,club=fixture+0x23000,queue=fixture+0x26000,dates=fixture+0x28000,contract=objects+0x300;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,4> ranges{{{injury::daily_entry,0x1018b6e0},{injury::notice_entry,0x1018f33c},{injury::end_compare_entry,0x1018ffe0},{injury::start_compare_entry,0x101900b0}}};
void modern_root(State& s,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{injury::process_daily_injuries,injury::prepare_progress_notice,injury::compare_queued_end,injury::compare_queued_start};
    auto it=std::find(injury::entries.begin(),injury::entries.end(),s.pc);require(it!=injury::entries.end(),"Unknown progression component root");methods[it-injury::entries.begin()](s,dependencies,observer);
}
struct Options {
    unsigned interrupt=0;std::uint32_t random=0,correction=0,eligible=1,relationship=1,contract_pointer=contract;
    int search_start=-1,search_end=-1;unsigned date_day=20,date_year=2001;
    std::uint32_t force_pc=0,force_value=65536,wrap_pc=0;unsigned force_register=17;int force_cr=-1;
    bool own_negative=false,child_negative=false,notice_chain=false,callback_mutate=false,reseed_mutate=false,refresh_mutate=false,relationship_mutate=false;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options value):options(value){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(options.wrap_pc==pc)s.memory.write<4>(globals+8,s.memory.read<4>(globals+4));
        if(options.force_pc==pc) {
            s.wr<4>(options.force_register*4,options.force_value);
            if(options.force_cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(options.force_cr)|(s.r<1>(1024)&1));
        }
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned club_queries=0;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Unknown progression reference root");
        while(s.pc!=stop) {
            if(s.pc>=entry && s.pc<range->second)require(injury_progression_reference(s),"Missing independent progression reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event event{target,continuation,s.registers,{}};
        const bool selected=target==injury::end_compare_entry || target==injury::start_compare_entry || (target==injury::notice_entry && options.notice_chain);
        if(selected){if(original)reference(s,continuation);else modern_root(s,this,&trace);}
        else {
            constexpr std::array<std::uint32_t,18> allowed{0x10717890,0x10187a40,0x10715880,0x10775600,0x10717dd0,0x1077fc28,0x102cdbf0,0x102bdf90,0x102cb4b0,0x1077b770,0x10082f20,0x101864a0,0x1037f7d0,0x100d9580,0x100d9e90,0x100d9960,0x100d9520,0x100d9e10};
            require(target==injury::notice_entry || target==0x100d9b80 || target==0x102cc030 || target==0x10006900 || std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Unknown progression dependency "+hex_address(target));
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
            s.memory.write<4>(fixture+0x30000+unsigned(calls.size())*4,target^arg(3));
            std::uint32_t result=0;
            if(target==0x10715880)s.memory.copy_in(arg(3),std::vector<std::uint8_t>(8,0));
            if(target==0x10717890){s.memory.write<2>(arg(3),options.date_day);s.memory.write<2>(arg(3)+2,options.date_year);s.memory.write<1>(arg(3)+4,2);}
            if(target==0x10717dd0){s.memory.write<2>(arg(3),6);s.memory.write<2>(arg(3)+2,1);s.memory.write<1>(arg(3)+4,0);}
            if(target==0x10775600){const auto index=arg(7)==descriptors?options.search_start:options.search_end;result=index<0?0:arg(4)+std::uint32_t(index)*arg(6);}
            if(target==0x1077fc28)s.memory.copy_in(arg(4),s.memory.copy_out(arg(3),arg(5)));
            if(target==0x102bdf90)s.memory.copy_in(arg(3),std::vector<std::uint8_t>(80,0));
            if(target==0x102cb4b0)s.memory.write<4>(arg(3)+arg(4)*4,arg(5));
            if(target==0x1037f7d0)s.memory.write<4>(arg(3)+0x50,s.memory.read<4>(arg(3)+0x50)&~arg(4));
            if(target==0x102cdbf0)result=options.eligible;
            if(target==0x10082f20){result=options.contract_pointer;++club_queries;}
            if(target==0x101864a0)result=options.correction;
            if(target==0x1077b770)result=options.random;
            if(target==0x100d9580 || target==0x100d9520)result=options.relationship;
            if(target==0x1077b770 && options.reseed_mutate){s.memory.write<1>(staff+0x65,7);s.memory.write<4>(staff+0x14,profile+0x100);}
            if(target==0x10187a40 && options.refresh_mutate){s.memory.write<4>(manager,rows+0x400);s.memory.write<4>(rows+0x400+0x10,0xffffffff);}
            if(target==0x1037f7d0 && options.callback_mutate){s.memory.write<1>(staff+0x67,0);s.memory.write<4>(staff+0x14,profile+0x100);}
            if((target==0x100d9e90 || target==0x100d9e10) && options.relationship_mutate){s.memory.write<1>(staff+0x65,0);s.memory.write<1>(staff+0x69,0);}
            for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
            for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        }
        if(options.child_negative && !original)s.wr<4>(12,s.r<4>(12)^1);
        event.after=s.registers;calls.push_back(event);
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture progression dependency interruption");
    }
};

std::unique_ptr<State> seed(std::uint32_t entry=injury::notice_entry) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x40000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->memory.write<4>(stack+0x14,toc);
    for(auto [slot,pointer]:{std::pair{-0x7be0,objects+0x410},std::pair{-0x7be4,objects+0x100},std::pair{-0x7b80,dates},
        std::pair{-0x5458,descriptors},std::pair{-0x5454,descriptors+8},std::pair{-0x7bf0,objects+0x420},std::pair{-0x7b24,objects+0x408},
        std::pair{-0x543c,constants},std::pair{-0x7b90,globals},std::pair{-0x7b8c,pool},std::pair{-0x7b88,globals+4},std::pair{-0x7b84,globals+8},
        std::pair{-0x64d4,details},std::pair{-0x7b7c,objects+0x418},std::pair{-0x7b9c,objects+0x414}})s->memory.write<4>(toc+slot,pointer);
    s->memory.write<4>(objects+0x100,16);s->memory.write<4>(objects+0x410,staff);s->memory.write<4>(objects+0x414,stats);s->memory.write<4>(objects+0x418,objects);s->memory.write<4>(objects+0x420,objects);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,injuries);s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x14,profile);s->memory.write<4>(staff+0x24,club);
    for(auto field:{0x28,0x2c,0x30})s->memory.write<4>(staff+field,club+field*16);
    s->memory.write<4>(rows+0x10,0xffffffff);s->memory.write<1>(rows+0x14,255);s->memory.write<4>(club+0x98,staff);s->memory.write<4>(club+0x90,objects+0x500);
    for(auto field:{0x63,0x65,0x67,0x69})s->memory.write<1>(staff+field,10);s->memory.write<1>(profile+0x44,10);
    s->memory.write<4>(globals+4,pool+0xfe);s->memory.write<4>(globals+8,pool);s->memory.write<8>(constants,float_bits<8>(65535.0));s->memory.write<8>(constants+8,float_bits<8>(4503601774854144.0));
    s->memory.write<2>(dates,20);s->memory.write<2>(dates+2,2001);s->memory.write<1>(dates+4,2);s->memory.write<2>(dates+8,20);s->memory.write<2>(dates+10,2001);s->memory.write<1>(dates+12,2);
    s->memory.write<4>(descriptors,injury::start_compare_entry);s->memory.write<4>(descriptors+4,toc);s->memory.write<4>(descriptors+8,injury::end_compare_entry);s->memory.write<4>(descriptors+12,toc);
    for(unsigned n=0;n<4;++n){s->memory.write<2>(queue+n*24,20);s->memory.write<2>(queue+n*24+2,2001);s->memory.write<2>(queue+n*24+8,20);s->memory.write<2>(queue+n*24+10,2001);s->memory.write<4>(queue+n*24+0x10,0);s->memory.write<1>(queue+n*24+0x15,77);}
    s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,0);s->wr<4>(24,0);s->wr<4>(28,1);s->wr<4>(32,5);
    if(entry==injury::end_compare_entry || entry==injury::start_compare_entry){s->wr<4>(12,dates);s->wr<4>(16,queue);}
    return s;
}

unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;
std::set<std::uint32_t> sites;
struct Result { std::unique_ptr<State> state; std::vector<Effects::Access> accesses; std::vector<Event> calls;unsigned club_queries=0; };
Result compare(const State& input, Options options={}) {
    auto a=clone(input),b=clone(input); Effects x,y; x.state=a.get(); y.state=b.get();
    a->memory.date_observer=&x; b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options); Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference) {
        try {
            if(reference) {original_trace=&old_trace;m.reference(s);}
            else modern_root(s,&m,&new_trace);
            original_trace=nullptr; return std::string{};
        } catch(const std::exception& e) {original_trace=nullptr;return std::string(e.what());}
    };
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);
    a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Injury progression entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown progression"),expected);
    if(expected!=actual) {
        std::cerr<<"Fault PCs "<<hex_address(a->pc)<<" / "<<hex_address(b->pc)<<"; steps "<<old_trace.sites.size()<<" / "<<new_trace.sites.size()<<std::endl;
        for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First divergent site "<<n<<" "<<hex_address(old_trace.sites[n])<<" / "<<hex_address(new_trace.sites[n])<<std::endl;break;}
        for(auto it=new_trace.sites.rbegin();it!=new_trace.sites.rend() && it<new_trace.sites.rbegin()+std::min<std::size_t>(12,new_trace.sites.size());++it)std::cerr<<"Recent site "<<hex_address(*it)<<std::endl;
    }
    require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;
    try {Image(*a).compare(*b,label);} catch(const std::runtime_error&) {
        if(!options.own_negative&&!options.child_negative) {
            std::cerr<<"Final "<<hex_address(a->pc)<<" / "<<hex_address(b->pc)<<"; steps "<<old_trace.sites.size()<<" / "<<new_trace.sites.size()<<'\n';
            for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)
                if(old_trace.sites[n]!=new_trace.sites[n]) {std::cerr<<"First site "<<hex_address(old_trace.sites[n])<<" / "<<hex_address(new_trace.sites[n])<<'\n';break;}
            throw;
        }
        rejected=true;
    }
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating own/child negative");
    if(!rejected) {
        if(x.bytes!=y.bytes) {
            for(unsigned n=0;n<std::min(x.log.size(),y.log.size());++n) {
                const auto& a=x.log[n];const auto& b=y.log[n];
                if(a.kind!=b.kind || a.address!=b.address || a.size!=b.size || a.value!=b.value) {
                    std::cerr<<"Effect "<<n<<" kind "<<a.kind<<"/"<<b.kind<<" pc-address "<<hex_address(a.address)<<"/"<<hex_address(b.address)<<" value "<<a.value<<"/"<<b.value<<'\n';break;
                }
            }
        }
        require(x.bytes==y.bytes,label+" ordered effects"); require(a->executed==b->executed,label+" accounting");
        require(old.calls==modern.calls,label+" dependency ABI"); if(old_trace.sites!=new_trace.sites) {
            for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]) {
                std::cerr<<"Site "<<n<<" expected="<<hex_address(old_trace.sites[n])<<" actual="<<hex_address(new_trace.sites[n])<<std::endl;break;
            }
        }
        require(old_trace.sites==new_trace.sites,label+" original-site order");
    }
    if((options.force_pc || options.wrap_pc) && expected.empty())require(std::find(old_trace.sites.begin(),old_trace.sites.end(),options.force_pc?options.force_pc:options.wrap_pc)!=old_trace.sites.end(),label+" structural control reached");
    ++cases; completed+=expected.empty()&&!rejected; interruptions+=!expected.empty(); negatives+=rejected;
    structural+=options.force_pc!=0 || options.wrap_pc!=0; sites.insert(old_trace.sites.begin(),old_trace.sites.end());
    return {std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}) {
    const auto baseline=compare(input,options); const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n) {auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    std::set<std::uint32_t> bytes;
    for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes) {
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(auto b:bytes)if(b!=hole) {s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}
        compare(*s,options);
    }
    for(unsigned n=1;n<=baseline.calls.size();++n) {auto o=options;o.interrupt=n;compare(input,o);}
}

void sampled_boundaries(const State& input,Options options={}) {
    auto baseline=compare(input,options);const auto steps=baseline.state->executed-input.executed;
    for(auto cut:{std::uint64_t(0),std::uint64_t(1),steps/2,steps?steps-1:0,steps}) {auto x=clone(input);x->budget=x->executed+cut;compare(*x,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes) {auto x=std::make_unique<State>();x->registers=input.registers;x->unique=input.unique;x->pc=input.pc;
        for(auto byte:bytes)if(byte!=hole){x->memory.map(byte,1);x->memory.write<1>(byte,input.memory.read<1>(byte));}compare(*x,options);}
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}


void date_checks() {
    for(auto entry:{injury::end_compare_entry,injury::start_compare_entry}) {
        for(auto year:{0u,1u,2000u,2001u,65535u})for(auto day:{0u,1u,19u,20u,21u,32767u,32768u,65535u}) {
            auto x=seed(entry);const auto field=entry==injury::end_compare_entry?8u:0u;
            x->memory.write<2>(queue+field,day);x->memory.write<2>(queue+field+2,year);auto r=compare(*x);
            const auto signed_day=day<32768?int(day):int(day)-65536;
            const auto expected=year>2001 || (year==2001 && signed_day>20)?0xffffffffu:year<2001 || (year==2001 && signed_day<20)?1u:0u;
            require(r.state->r<4>(12)==expected,"Independent queued date comparison");
            if(year==2001)boundaries(*x);
        }
        for(auto offset:{0u,4u,8u,0xfffffff8u}){auto x=seed(entry);x->wr<4>(16,stack+offset);boundaries(*x);}
        for(unsigned arg:{3u,4u}){auto x=seed(entry);x->wr<4>(arg*4,0);boundaries(*x);}
        {Options o;o.own_negative=true;compare(*seed(entry),o);}
    }
}

void daily_checks() {
    compare(*seed(injury::daily_entry));boundaries(*seed(injury::daily_entry));
    for(auto active:{0u,1u})for(auto duration:{0u,5u,6u,32767u,32768u,65535u})for(auto flagged:{0u,1u,128u}) {
        auto x=seed(injury::daily_entry);x->memory.write<4>(rows+0x10,active?0:0xffffffff);x->memory.write<2>(injuries+0x32,duration);x->memory.write<2>(injuries+0x30,duration+7);x->memory.write<1>(rows+0x20,flagged);boundaries(*x);
    }
    for(auto day:{19u,20u,21u})for(auto year:{2000u,2001u,2002u}) {
        auto x=seed(injury::daily_entry);x->memory.write<4>(rows+0x10,0);Options o;o.date_day=day;o.date_year=year;boundaries(*x,o);
    }
    for(auto id:{0u,15u,16u,0x7fffffffu,0x80000000u,0xffffffffu}){auto x=seed(injury::daily_entry);x->memory.write<4>(staff,id);boundaries(*x);}
    for(auto count:{0u,1u,15u,16u,17u,0xffffffffu}){auto x=seed(injury::daily_entry);x->memory.write<4>(objects+0x100,count);compare(*x);}
    for(auto count:{1u,2u,4u})for(auto begin:{-1,0,1})for(auto end:{-1,0,1}) {
        if(begin>=int(count)||end>=int(count))continue;auto x=seed(injury::daily_entry);x->memory.write<4>(manager+0x10,queue);x->memory.write<4>(manager+0x18,count);Options o;o.search_start=begin;o.search_end=end;boundaries(*x,o);
    }
    for(unsigned expired:{0u,1u,2u,3u,4u}){auto x=seed(injury::daily_entry);x->memory.write<4>(manager+0x10,queue);x->memory.write<4>(manager+0x18,4);for(unsigned n=0;n<expired;++n)x->memory.write<2>(queue+n*24+8,19);Options o;o.search_end=expired<4?int(expired):3;boundaries(*x,o);}
    {auto x=seed(injury::daily_entry);x->memory.write<4>(rows+0x10,0);Options o;o.refresh_mutate=true;boundaries(*x,o);o.refresh_mutate=false;o.notice_chain=true;x->memory.write<2>(injuries+0x32,6);boundaries(*x,o);}
    {Options o;o.own_negative=true;compare(*seed(injury::daily_entry),o);o.own_negative=false;o.child_negative=true;auto x=seed(injury::daily_entry);x->memory.write<4>(rows+0x10,0);compare(*x,o);}
}

void daily_chain_check() {auto x=seed(injury::daily_entry);x->memory.write<4>(rows+0x10,0);x->memory.write<2>(injuries+0x32,6);Options o;o.notice_chain=true;boundaries(*x,o);}

void notice_checks() {
    boundaries(*seed());
    for(auto pointer:{0u,staff})for(auto force:{0u,1u,128u,256u})for(auto eligible:{0u,1u,128u}) {
        auto x=seed();x->wr<4>(16,pointer);x->wr<4>(28,force);Options o;o.eligible=eligible;boundaries(*x,o);
    }
    for(auto type:{0u,11u,76u,77u,78u,79u,80u,81u,82u,127u,128u,255u,256u})for(auto end:{0u,1u,128u}) {
        auto x=seed();x->wr<4>(24,type);x->wr<4>(20,end);x->memory.write<1>(details+type%256*12+1,12);boundaries(*x);
    }
    for(auto type:{77u,78u,79u})for(auto word:{0u,1u,4u,5u,100u,29499u,29500u,29999u,30000u,31999u,32000u,65535u})for(auto end:{0u,1u}) {
        auto x=seed();x->wr<4>(24,type);x->wr<4>(20,end);x->memory.write<1>(details+type*12+1,12);x->memory.write<2>(pool+2,word);compare(*x);
        if(word==30000 || word==0)boundaries(*x);
    }
    for(auto flags:{0u,0x20u,0x40u,0x20000u,0x20060u})for(auto trait:{0u,1u,10u,20u,127u,128u,255u}) {
        auto x=seed();x->wr<4>(24,81);x->memory.write<1>(details+81*12+1,12);x->memory.write<4>(contract+0x50,flags);for(auto field:{0x63,0x65,0x67,0x69})x->memory.write<1>(staff+field,trait);boundaries(*x);
    }
    for(auto type:{77u,81u})for(auto pointer:{0u,club})for(auto related:{0u,objects+0x500}) {
        auto x=seed();x->wr<4>(24,type);x->memory.write<1>(details+type*12+1,12);x->memory.write<4>(staff+0x24,pointer);x->memory.write<4>(club+0x90,related);Options o;o.contract_pointer=0;boundaries(*x,o);o.contract_pointer=contract;boundaries(*x,o);
    }
    for(auto reaction:{0u,85u,100u,127u,128u,141u,156u,255u}){auto x=seed();x->wr<4>(24,81);x->memory.write<1>(details+81*12+1,12);x->memory.write<4>(contract+0x50,0x20);x->memory.write<1>(contract+0x4c,reaction);boundaries(*x);}
    for(auto salt:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto cursor:{pool,pool+0xfc,pool+0xfe,0xfffffffeu}) {
        auto x=seed();x->wr<4>(24,81);x->memory.write<1>(details+81*12+1,12);x->memory.write<4>(globals,salt);x->memory.write<4>(globals+8,cursor);Options o;o.reseed_mutate=true;boundaries(*x,o);
    }
    {auto x=seed();x->wr<4>(24,81);x->memory.write<1>(details+81*12+1,12);x->memory.write<4>(contract+0x50,0x20060);Options o;o.callback_mutate=true;boundaries(*x,o);}
    {Options o;o.own_negative=true;compare(*seed(),o);o.own_negative=false;o.child_negative=true;compare(*seed(),o);}
}

void structural_checks() {
    constexpr std::array<std::uint32_t,9> tests{0x1018ded4,0x1018e0b0,0x1018e288,0x1018e45c,0x1018e64c,0x1018e7dc,0x1018e9e0,0x1018efcc,0x1018f190};
    for(auto pc:tests) {
        auto x=seed();unsigned type=81;
        if(pc==0x1018efcc){type=77;x->memory.write<1>(staff+0x65,11);}
        if(pc==0x1018f190){type=77;x->memory.write<2>(pool+2,5);x->memory.write<1>(staff+0x69,1);}
        x->wr<4>(24,type);x->memory.write<1>(details+type*12+1,12);
        if(pc==0x1018ded4 || pc==0x1018e0b0 || pc==0x1018e288)x->memory.write<4>(contract+0x50,0x20060);
        if(pc==0x1018e64c){x->memory.write<4>(contract+0x50,0x20);x->memory.write<2>(pool+4,9);}
        if(pc==0x1018e7dc)x->memory.write<4>(objects+0x408,objects);
        for(auto value:{65536u,0xffff0000u,0x7fffffffu,0x80000000u}) {
            Options o;o.force_pc=pc;o.force_value=value;
            if(value==65536)sampled_boundaries(*x,o);else compare(*x,o);
        }
        Options o;o.wrap_pc=pc-0x28;sampled_boundaries(*x,o);
    }
    {auto x=seed(injury::daily_entry);Options o;o.force_pc=0x1018b318;o.force_register=0;o.force_value=0;o.force_cr=2;sampled_boundaries(*x,o);}
    for(auto entry:{injury::end_compare_entry,injury::start_compare_entry}){Options o;o.force_pc=entry+0x84;o.force_register=0;o.force_value=0;o.force_cr=4;sampled_boundaries(*seed(entry),o);}
    for(auto entry:injury::entries){auto x=seed(entry);constexpr auto moved=toc+0x10000;x->memory.map(moved-0x8000,0x9000);x->memory.copy_in(moved-0x8000,x->memory.copy_out(toc-0x8000,0x9000));x->wr<4>(8,moved);sampled_boundaries(*x);}
}

std::unique_ptr<State> risk_seed(unsigned type=81) {auto x=seed();x->wr<4>(24,type);x->memory.write<1>(details+type*12+1,12);return x;}
void coverage_checks() {
    {auto x=seed(injury::daily_entry);x->memory.write<4>(manager+0x10,queue);x->memory.write<4>(manager+0x18,3);x->memory.write<2>(queue,19);Options o;o.search_start=2;sampled_boundaries(*x,o);}
    for(auto year:{2000u,2002u}){auto x=seed(injury::daily_entry);x->memory.write<4>(manager+0x10,queue);x->memory.write<4>(manager+0x18,2);x->memory.write<2>(queue+10,year);Options o;o.search_end=1;sampled_boundaries(*x,o);}
    {auto x=risk_seed();x->memory.write<4>(contract+0x50,0x20);x->memory.write<2>(pool+4,9);x->memory.write<2>(pool+8,9);sampled_boundaries(*x);}
    {auto x=risk_seed(77);x->memory.write<1>(staff+0x65,11);x->memory.write<2>(pool+4,9);sampled_boundaries(*x);}
    for(auto reaction:{0u,85u,100u,127u,128u,141u,156u,255u}) {
        auto x=risk_seed();x->memory.write<4>(contract+0x50,0x20060);x->memory.write<1>(contract+0x4c,reaction);
        for(auto offset:{4u,8u,12u})x->memory.write<2>(pool+offset,9);
        auto r=compare(*x);const auto before=reaction<128?int(reaction):int(reaction)-256;const auto expected=std::clamp(before+15,-100,100);
        require(signed_value<1>(r.state->memory.read<1>(contract+0x4c))==expected,"Independent signed contract-reaction clamp");sampled_boundaries(*x);
    }
    for(auto trait:{0u,6u,10u})for(bool mutate:{false,true}) {
        auto x=risk_seed();x->memory.write<4>(contract+0x50,0x20);x->memory.write<2>(pool+4,9);x->memory.write<1>(staff+0x65,trait);
        Options o;o.relationship_mutate=mutate;sampled_boundaries(*x,o);
    }
    {auto x=risk_seed();x->memory.write<4>(objects+0x408,objects);x->memory.write<1>(staff+0x65,0);sampled_boundaries(*x);}
    {auto x=risk_seed();x->memory.write<2>(profile+10,7251);sampled_boundaries(*x);}
    for(auto choice:{0u,1u,2u,3u})for(auto trait:{0u,1u,2u,20u,127u,128u,255u}) {
        auto x=risk_seed();x->memory.write<2>(pool+8,1);x->memory.write<2>(pool+10,choice);
        for(auto field:{0x63,0x65,0x67,0x69})x->memory.write<1>(staff+field,trait);x->memory.write<1>(profile+0x44,trait);sampled_boundaries(*x);
    }
    {auto x=risk_seed();x->memory.write<2>(pool+8,1);Options o;o.force_pc=0x1018eba8;o.force_register=0;o.force_value=0xffffffff;sampled_boundaries(*x,o);}
    for(auto trait:{0u,1u,11u,16u,20u})for(auto word:{0u,1u,5u}) {
        auto x=risk_seed(77);x->memory.write<2>(pool+2,word);x->memory.write<1>(staff+0x65,trait);x->memory.write<1>(staff+0x69,trait);x->memory.write<1>(staff+0x63,11);
        Options o;o.relationship_mutate=true;sampled_boundaries(*x,o);
    }
    {auto x=risk_seed(79);x->wr<4>(20,1);x->memory.write<2>(pool+2,30000);x->memory.write<2>(pool+4,1);sampled_boundaries(*x);}
    for(auto pc:{0x1018dff4u,0x1018e1d0u,0x1018e3a8u}){auto x=risk_seed();x->memory.write<4>(contract+0x50,0x20060);Options o;o.wrap_pc=pc;sampled_boundaries(*x,o);}
    for(auto pc:{0x1018e8fcu,0x1018eb00u}){auto x=risk_seed();x->memory.write<4>(objects+0x408,objects);Options o;o.wrap_pc=pc;sampled_boundaries(*x,o);}
    for(auto pc:{0x1018edbcu,0x1018ee34u}) {auto x=risk_seed(79);x->wr<4>(20,1);x->memory.write<2>(pool+2,30000);Options o;o.force_pc=pc;o.force_register=pc==0x1018edbc?3:5;o.force_value=pool+0xfe;sampled_boundaries(*x,o);}
    {auto x=risk_seed();x->memory.write<2>(pool+8,1);Options o;o.force_pc=0x1018eb60;o.force_register=5;o.force_value=pool+0xfe;sampled_boundaries(*x,o);}
    for(bool owned:{false,true}){auto x=seed();x->memory.write<4>(club+0x2c,objects);x->memory.write<4>(staff+12,objects);if(!owned)x->memory.write<4>(staff+0x24,0);sampled_boundaries(*x);}
}
}
namespace cm::aot {void injury_progression_step_seen(State& s,std::uint32_t pc) {if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty() || mode=="--dates")date_checks();
    if(mode.empty() || mode=="--daily")daily_checks();
    if(mode=="--daily-chain")daily_chain_check();
    if(mode.empty() || mode=="--notice")notice_checks();
    if(mode.empty() || mode=="--structural")structural_checks();
    if(mode.empty() || mode=="--coverage")coverage_checks();
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered progression "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==1815,"Complete injury progression source coverage");
    std::cout<<"Progression comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
