#include "injury_record_adapter.hpp"
#include "actor_attributes_io_checks.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "process_bridge.hpp"
#include <chrono>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace injury=cm::aot::injury_record;
enum class Path { direct, generated, cfm };
enum class Mode { modern, original, entry_original, profile };
struct Clock:CodeFragmentBoundaryObserver {
    std::uint64_t base=native_microseconds();unsigned count=0;
    ActorAttributesIoPair* io=nullptr;bool modern=false;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override {
        if(io && io->dispatch(s,name,modern))return true;
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Injury record await native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
std::array<std::set<std::uint32_t>,3> runtime_sites;
void record_site(State& s,std::uint32_t pc) {if(s.executed>=s.budget)return;for(unsigned n=0;n<3;++n)if(pc>=injury::entries[n] && pc<(n==0?0x1018aec4u:n==1?0x1018b2b0u:0x1018b9ecu))runtime_sites[n].insert(pc);}
struct Witness:InjuryRecordBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,InjuryRecordOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Capture:InjuryRecordBoundaryObserver {
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
    std::array<std::unique_ptr<State>,3> inputs;
    std::array<std::uint32_t,3> callsites{};
    void enter(const State& state,std::uint32_t entry,bool original)override {
        if(original)return;auto it=std::find(injury::entries.begin(),injury::entries.end(),entry);require(it!=injury::entries.end(),"Unknown injury capture");const auto index=unsigned(it-injury::entries.begin());
        if(inputs[index])return;callsites[index]=std::uint32_t(state.r<4>(4128))-4;inputs[index]=clone(state);inputs[index]->platform=state.platform;inputs[index]->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,InjuryRecordOutcome)override{}
};
struct Negative:InjuryRecordBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,InjuryRecordOutcome result)override {
        if(entry==injury::entries[index] && !original && result==InjuryRecordOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
    }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(directory,e)||e||std::filesystem::is_symlink(directory,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(directory,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(directory,e);}
};
unsigned cases=0,returns=0,budgets=0,negatives=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_injury_record(s,injury::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,injury::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false) {
    Clock probe_clock;probe_clock.base=1000000000;
    ActorAttributesIoPair io=[&]() {
        try {return ActorAttributesIoPair(input,index,injury::entries[index],[&](State& x){injury_record_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    injury_record_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;auto& d=injury_record_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production injury-record entry="+hex_address(injury::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!expected.empty()),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    if(index==2 && mode==Mode::modern && limit==50000000 && !negative)std::cout<<"Propagation actual path="<<unsigned(path)<<" nestedRegistrationCalls="<<d.calls[0]<<std::endl;
    ++cases;returns+=expected.empty();budgets+=!expected.empty();
}

unsigned fault_cases=0;
void actual_fault(const State& input,unsigned index,unsigned argument) {
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_record_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {s->wr<4>(argument*4,0);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    injury_record_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();io.between();const auto actual=execute(*modern,Path::generated,index);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual injury pointer fault original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual injury pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual injury fault effects/accounting/import ABI");io.compare();
    const auto& d=injury_record_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual injury fault counters");++fault_cases;
}

void matrix(const State& input,unsigned index,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}) {
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode);
    }
    if(!profile) {
        compare(input,index,Path::generated,Mode::modern,50000000,true);
        for(auto cut:{0u,1u,2u,3u,8u,12u,16u})compare(input,index,Path::generated,Mode::modern,cut);
    }
}
std::uint32_t invoke_original(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    auto& own=injury_record_diagnostics(s);auto& actor=actor_attributes_diagnostics(s);const auto old_own=own.original,old_actor=actor.original;own.original=actor.original=true;
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){std::cerr<<"Original injury setup entry="<<hex_address(entry)<<" pc="<<hex_address(s.pc)<<" reason="<<e.what()<<std::endl;own.original=old_own;actor.original=old_actor;throw;}
    own.original=old_own;actor.original=old_actor;return std::uint32_t(s.r<4>(12));
}
std::unique_ptr<State> complete_match(const State& input,std::uint32_t match) {
    auto old=clone(input),modern=clone(input);old->platform=modern->platform=input.platform;injury_record_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;
    Clock x,y;x.base=y.base=1000000000;old->code_fragment_observer=&x;modern->code_fragment_observer=&y;unsigned advances=0;
    for(;advances<1400;++advances) {
        for(auto* state:{old.get(),modern.get()}) {state->wr<4>(12,match);state->wr<4>(16,1);state->wr<4>(4128,return_sentinel);state->pc=0x101d3f60;state->budget=state->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*state,return_sentinel);}
        require(old->executed==modern->executed && old->r<4>(12)==modern->r<4>(12),"Injury paired match progression/accounting");if(advances%50==0)Image(*old).compare(*modern,"Injury paired intermediate match");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    require(advances<1400 && modern->r<4>(12)==2,"Injury paired match reaches full time");Image(*old).compare(*modern,"Injury paired complete match");require(x.events==y.events && x.count==y.count,"Injury paired imports");
    const auto& a=injury_record_diagnostics(*old);const auto& b=injury_record_diagnostics(*modern);
    for(unsigned n=0;n<3;++n)require(a.original_calls[n]==b.calls[n] && b.calls[n]==b.returns[n] && !b.faults[n] && !b.dispatch_escapes,"Injury paired natural counts");
    std::cout<<"PASS: injury paired full match advances="<<advances<<" registrationCalls="<<b.calls[0]<<" noticeCalls="<<b.calls[1]<<" propagationCalls="<<b.calls[2]<<"; complete state/import ABI/per-tick accounting"<<std::endl;
    old->code_fragment_observer=modern->code_fragment_observer=nullptr;return modern;
}
std::unique_ptr<State> prepare_post_match(const State& full,std::uint32_t match) {
    auto s=clone(full);s->platform=full.platform;const auto toc=std::uint32_t(s->r<4>(8)),manager=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7ba0))),context=std::uint32_t(s->memory.read<4>(match+0x3118));
    const auto stats=invoke_original(*s,0x10773520,{0x1000}),service=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7b4c)));require(stats!=0,"Original statistics allocation");invoke_original(*s,0x1029f920,{service,match,stats,0});
    const auto id=std::uint32_t(s->memory.read<4>(stats+0x194)),staff=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be0)))+id*0x70;require(id!=0xffffffff && s->memory.read<4>(staff)==id,"Actual post-match player");
    const auto sp=std::uint32_t(s->r<4>(4));s->wr<4>(4,sp-0x80);s->memory.write<4>(sp-0x80+0x38,0);invoke_original(*s,injury::registration_entry,{manager,0,0,staff,0,0,0xffffffff,0});s->wr<4>(4,sp);
    unsigned kind=0;bool eligible=false;for(;kind<8;++kind) {auto probe=clone(*s);probe->platform=s->platform;if(signed_value<1>(invoke_original(*probe,0x10188930,{manager,kind,2,staff}))>=0){eligible=true;break;}}
    require(eligible,"Original tables have an eligible queued injury");const auto queue=invoke_original(*s,0x10773520,{16});invoke_original(*s,0x10738c00,{queue});s->memory.write<4>(manager+0x28,queue);
    const auto item=stats+0xf00;s->memory.write<4>(item,id);s->memory.write<4>(item+4,(kind<<24)|0x00630000);invoke_original(*s,0x10738d10,{queue,item,8});s->memory.write<1>(stats+0x1b3,1);
    s->wr<4>(12,manager);s->wr<4>(16,context);s->wr<4>(20,stats);s->wr<4>(4128,return_sentinel);s->pc=actor_attributes::post_match_entry;return s;
}
constexpr std::array<std::pair<unsigned,const char*>,14> variants{{
    {0,"registration-no-context.snapshot"},{0,"registration-backdate.snapshot"},{0,"registration-deferred.snapshot"},
    {0,"registration-deferred-negative.snapshot"},{0,"registration-replacement.snapshot"},{0,"registration-allocation.snapshot"},
    {0,"registration-reallocation.snapshot"},{0,"registration-risk.snapshot"},{0,"registration-risk-negative.snapshot"},{0,"registration-guard.snapshot"},
    {1,"notice-long.snapshot"},{1,"notice-broadcast.snapshot"},{1,"notice-no-context.snapshot"},{2,"propagation-body.snapshot"}}};
void extra_cases(const std::array<std::unique_ptr<State>,3>& inputs,const std::filesystem::path& snapshots) {
    const auto manager=std::uint32_t(inputs[0]->r<4>(12)),staff=std::uint32_t(inputs[0]->r<4>(24)),toc=std::uint32_t(inputs[0]->r<4>(8));
    const auto id=std::uint32_t(inputs[0]->memory.read<4>(staff)),row=std::uint32_t(inputs[0]->memory.read<4>(manager))+id*0x24;
    for(auto [index,name]:variants) {
        const auto label=std::string_view(name);auto input=clone(*inputs[index]);input->platform=inputs[index]->platform;
        if(label=="registration-no-context.snapshot")input->wr<4>(28,0);
        if(label=="registration-backdate.snapshot")input->wr<4>(20,1);
        if(label=="registration-deferred.snapshot" || label=="registration-deferred-negative.snapshot") {input->wr<4>(16,11);input->wr<4>(40,label=="registration-deferred.snapshot"?1:236);input->memory.write<4>(input->r<4>(4)+0x38,1);}
        if(label=="registration-replacement.snapshot") {
            const auto registers=input->registers;input->memory.write<4>(input->r<4>(4)+0x38,0);
            invoke_original(*input,actor_attributes::reset_entry,{manager,id});
            invoke_original(*input,injury::registration_entry,{manager,0,0,staff,0,0,0xffffffff,0});input->registers=registers;
            require(signed_value<4>(input->memory.read<4>(row+0x10))>=0,"Original setup creates a replaceable active injury");input->wr<4>(16,77);
        }
        if(label=="registration-allocation.snapshot") {input->memory.write<4>(manager+4,0);input->memory.write<4>(manager+8,0);}
        if(label=="registration-reallocation.snapshot") {
            const auto base=std::uint32_t(input->memory.read<4>(manager+4)),count=std::uint32_t(input->memory.read<4>(manager+8));require(base && count>0 && count<10000,"Actual injury allocation is bounded");
            for(unsigned n=0;n<count;++n)input->memory.write<1>(base+n*0x38,0);
        }
        if(label=="registration-risk.snapshot" || label=="registration-risk-negative.snapshot") {input->wr<4>(16,11);input->wr<4>(40,label=="registration-risk.snapshot"?1:236);}
        if(label=="registration-guard.snapshot") {input->memory.write<4>(staff,input->memory.read<4>(input->memory.read<4>(toc-0x7be4)));input->wr<4>(12,0);}
        if(label=="notice-long.snapshot" || label=="notice-broadcast.snapshot") {
            input->memory.write<2>(input->r<4>(20)+0x30,101);
            if(label=="notice-broadcast.snapshot") {const auto profile=input->memory.read<4>(input->r<4>(16)+0x14);require(profile!=0,"Actual notice player has a profile");input->memory.write<2>(profile+12,9000);}
        }
        if(label=="notice-no-context.snapshot")input->memory.write<1>(input->r<4>(20)+0x20,255);
        if(label=="propagation-body.snapshot") {
            input->memory.write<1>(input->r<4>(16)+0x14,255);input->memory.write<1>(input->r<4>(20),11);
            const auto pool=input->memory.read<4>(toc-0x7b8c),salt=input->memory.read<4>(toc-0x7b90),cursor=input->memory.read<4>(toc-0x7b84),end=input->memory.read<4>(input->memory.read<4>(toc-0x7b88));
            require(pool+6<=end,"Actual random pool supports controlled risk words");input->memory.write<4>(salt,0);input->memory.write<4>(cursor,pool);input->memory.write<2>(pool+2,0);input->memory.write<2>(pool+4,100);input->memory.write<2>(pool+6,0);
            const auto profile=input->memory.read<4>(staff+0x14);if(profile)input->memory.write<1>(profile+0x2d,0);

        }
        input->pc=injury::entries[index];input->wr<4>(4128,return_sentinel);save_runtime_snapshot(*input,snapshots/name);matrix(*input,index,false);
        std::cout<<"PASS: controlled actual-data injury input="<<name<<std::endl;
    }
}

void capture_parent(const State& input,const std::filesystem::path& snapshots) {
    Clock probe_clock;probe_clock.base=1000000000;
    ActorAttributesIoPair io(input,0,actor_attributes::post_match_entry,[&](State& s){injury_record_diagnostics(s).original=true;s.code_fragment_observer=&probe_clock;});auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    injury_record_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;Capture capture;injury_record_diagnostics(*modern).observer=&capture;
    Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    for(auto* state:{old.get(),modern.get()})state->budget=state->executed+50000000;
    run_until(*old,return_sentinel);old->platform.reset();io.between();run_until(*modern,return_sentinel);old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;injury_record_diagnostics(*modern).observer=nullptr;
    Image(*old).compare(*modern,"Complete prepared original post-match parent");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Post-match injury effects/accounting/import ABI");io.compare();require(io.expected!=io.baseline,"Actual post-match history bytes change");
    for(unsigned n=0;n<3;++n) {require(bool(capture.inputs[n]),"Post-match parent captures each injury root");capture.inputs[n]->platform=input.platform;const auto& d=injury_record_diagnostics(*modern);require(d.calls[n]>0 && d.calls[n]==d.returns[n] && !d.faults[n] && !d.original_calls[n],"Dedicated actual-parent injury counts");
        const auto expected=n==0?(capture.callsites[n]==0x101873c4 || capture.callsites[n]==0x101873f4):capture.callsites[n]==(n==1?0x1018a028:0x1018a408);require(expected,"Captured original injury BL");
        save_runtime_snapshot(*capture.inputs[n],snapshots/(hex_address(injury::entries[n])+".snapshot"));matrix(*capture.inputs[n],n,false);
        std::cout<<"PASS: complete prepared post-match injury entry="<<hex_address(injury::entries[n])<<" original BL="<<hex_address(capture.callsites[n])<<" calls="<<d.calls[n]<<" returns="<<d.returns[n]<<std::endl;
    }
    extra_cases(capture.inputs,snapshots);
    actual_fault(*capture.inputs[0],0,3);actual_fault(*capture.inputs[1],1,4);
    State gate;load_match_snapshot(gate,snapshots/"propagation-body.snapshot");gate.platform=input.platform;actual_fault(gate,2,3);
}
}
int main(int argc,char** argv){try {
    const bool focused=argc==6 && std::string_view(argv[5])=="--variants";
    require(argc==4 || argc==5 || focused,"Expected original program/game/private backing and optional captured profile directory");const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Injury profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    if(focused) {std::array<std::unique_ptr<State>,3> inputs;for(unsigned n=0;n<3;++n){inputs[n]=std::make_unique<State>();load_program(*inputs[n],argv[1]);inputs[n]->platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(*inputs[n],std::filesystem::path(argv[4])/(hex_address(injury::entries[n])+".snapshot"));}extra_cases(inputs,argv[4]);}
    else if(profile) {for(unsigned n=0;n<3;++n){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(injury::entries[n])+".snapshot"));matrix(input,n,true);}
        for(auto [index,name]:variants) {State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/name);matrix(input,index,true);}}
    else {auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto full=complete_match(*game.state,game.match);auto input=prepare_post_match(*full,game.match);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"injury-record-live-profile";std::filesystem::create_directories(snapshots);save_runtime_snapshot(*input,snapshots/"post-match-parent.snapshot");capture_parent(*input,snapshots);}
    if(!profile){std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"injury-record-runtime-sites.txt");for(unsigned n=0;n<3;++n)for(auto pc:runtime_sites[n])log<<n<<" "<<hex_address(pc)<<std::endl;}
    std::cout<<"Runtime injury source sites="<<runtime_sites[0].size()<<","<<runtime_sites[1].size()<<","<<runtime_sites[2].size()<<std::endl;
    std::cout<<"Actual injury pointer faults="<<fault_cases<<std::endl;
    std::cout<<"PASS: injury production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
