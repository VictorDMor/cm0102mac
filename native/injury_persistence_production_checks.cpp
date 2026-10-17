#include "injury_persistence_adapter.hpp"
#include "injury_readouts_adapter.hpp"
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
namespace injury=cm::aot::injury_persistence;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Injury reporting awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
constexpr std::array<std::uint32_t,2> ends{0x10184e58,0x10185404};
std::array<std::set<std::uint32_t>,2> runtime_sites;
void record_site(State& s,std::uint32_t pc) {if(s.executed>=s.budget)return;for(unsigned n=0;n<2;++n)if(pc>=injury::entries[n] && pc<ends[n])runtime_sites[n].insert(pc);}
struct Witness:InjuryPersistenceBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,InjuryPersistenceOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Capture:InjuryPersistenceBoundaryObserver {
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
    std::array<std::unique_ptr<State>,2> inputs;
    std::array<std::uint32_t,2> callsites{};
    void enter(const State& state,std::uint32_t entry,bool original)override {
        if(original)return;auto it=std::find(injury::entries.begin(),injury::entries.end(),entry);require(it!=injury::entries.end(),"Unknown injury capture");const auto index=unsigned(it-injury::entries.begin());
        if(inputs[index])return;callsites[index]=std::uint32_t(state.r<4>(4128))-4;inputs[index]=clone(state);inputs[index]->platform=state.platform;inputs[index]->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,InjuryPersistenceOutcome)override{}
};
struct Negative:InjuryPersistenceBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,InjuryPersistenceOutcome result)override {
        if(entry==injury::entries[index] && !original && result==InjuryPersistenceOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
    }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(directory,e)||e||std::filesystem::is_symlink(directory,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(directory,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(directory,e);}
};
void copy_retained_backing(const std::filesystem::path& snapshots,const std::filesystem::path& root) {
    std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);const auto relative=std::filesystem::path(name);
    require(relative.parent_path().empty() && name.starts_with("prepared-backing-") && std::filesystem::is_directory(snapshots/relative) && !std::filesystem::is_symlink(snapshots/relative),"Retained original reporting history backing");
    ActorAttributesIoPair::copy(snapshots/relative,root);
}
unsigned cases=0,returns=0,budgets=0,negatives=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_injury_persistence(s,injury::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,injury::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool budget_expected=true) {
    Clock probe_clock;probe_clock.base=1000000000;
    ActorAttributesIoPair io=[&]() {
        try {return ActorAttributesIoPair(input,index,injury::entries[index],[&](State& x){injury_persistence_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    injury_persistence_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;injury_readouts_diagnostics(*a).original=true;injury_record_diagnostics(*a).original=true;auto& d=injury_persistence_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production injury-persistence entry="+hex_address(injury::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000 || !budget_expected?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected measured boundary");
    require(expected.empty()?a->executed<=a->budget:a->executed==a->budget+1,label+" exact original budget position");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!expected.empty()),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    ++cases;returns+=expected.empty();budgets+=!expected.empty();
    return a->executed-input.executed;
}

unsigned fault_cases=0;
void actual_fault(const State& input,unsigned index) {
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_persistence_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {
        const auto toc=s->r<4>(8);constexpr std::uint32_t bad=0xf0000000;
        (void)toc;s->wr<4>(12,bad);
        s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;
    }
    injury_persistence_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();
    // A guest fault deliberately leaves its fork open. Release only the host
    // fixture handle before replay so its write lock cannot affect the next run.
    // Guest memory, import trace and the open-reference map remain unchanged.
    for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release abandoned original fixture fork after outcome");}
    io.between();const auto actual=execute(*modern,Path::generated,index);
    require(io.old_forks.references.size()==io.new_forks.references.size(),"Fault preserves guest open-fork count");
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual persistence pointer fault entry="+hex_address(injury::entries[index])+" original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual injury bootstrap pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual reporting fault effects/accounting/import ABI");io.compare();
    const auto& d=injury_persistence_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual reporting fault counters");++fault_cases;
}

void matrix(const State& input,unsigned index,bool profile) {
    std::uint64_t full_steps=0;
    for(auto path:{Path::direct,Path::generated,Path::cfm}) {
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original}){const auto elapsed=compare(input,index,path,mode);if(path==Path::generated && mode==Mode::modern)full_steps=elapsed;}
    }
    if(!profile) {
        compare(input,index,Path::generated,Mode::modern,50000000,true);
        require(full_steps>0,"Original reporting length measured before budget cuts");
        for(auto cut:{0u,1u,2u,3u,8u,12u,16u})compare(input,index,Path::generated,Mode::modern,cut,false,cut<full_steps);
    }
}
std::uint32_t invoke_original(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args,std::uint64_t limit=50000000) {
    auto& own=injury_persistence_diagnostics(s);auto& actor=actor_attributes_diagnostics(s);auto& record=injury_record_diagnostics(s);auto& readouts=injury_readouts_diagnostics(s);
    const auto old_own=own.original,old_actor=actor.original,old_record=record.original,old_readouts=readouts.original;own.original=actor.original=record.original=readouts.original=true;
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+limit;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){std::cerr<<"Original injury setup entry="<<hex_address(entry)<<" pc="<<hex_address(s.pc)<<" reason="<<e.what()<<std::endl;own.original=old_own;actor.original=old_actor;record.original=old_record;readouts.original=old_readouts;throw;}
    own.original=old_own;actor.original=old_actor;record.original=old_record;readouts.original=old_readouts;return std::uint32_t(s.r<4>(12));
}
std::unique_ptr<State> complete_match(const State& input,std::uint32_t match) {
    auto old=clone(input),modern=clone(input);old->platform=modern->platform=input.platform;injury_persistence_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;
    Clock x,y;x.base=y.base=1000000000;old->code_fragment_observer=&x;modern->code_fragment_observer=&y;unsigned advances=0;
    for(;advances<1400;++advances) {
        for(auto* state:{old.get(),modern.get()}) {state->wr<4>(12,match);state->wr<4>(16,1);state->wr<4>(4128,return_sentinel);state->pc=0x101d3f60;state->budget=state->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*state,return_sentinel);}
        require(old->executed==modern->executed && old->r<4>(12)==modern->r<4>(12),"Injury paired match reporting/accounting");if(advances%50==0)Image(*old).compare(*modern,"Injury paired intermediate match");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    require(advances<1400 && modern->r<4>(12)==2,"Injury paired match reaches full time");Image(*old).compare(*modern,"Injury paired complete match");require(x.events==y.events && x.count==y.count,"Injury paired imports");
    const auto& a=injury_persistence_diagnostics(*old);const auto& b=injury_persistence_diagnostics(*modern);
    for(unsigned n=0;n<2;++n)require(a.original_calls[n]==b.calls[n] && b.calls[n]==b.returns[n] && !b.faults[n] && !b.dispatch_escapes,"Injury paired natural counts");
    std::cout<<"PASS: injury persistence paired full match advances="<<advances<<" calls=";for(unsigned n=0;n<2;++n)std::cout<<(n?",":"")<<b.calls[n];std::cout<<"; complete state/import ABI/per-tick accounting"<<std::endl;
    old->code_fragment_observer=modern->code_fragment_observer=nullptr;return modern;
}

constexpr std::array<std::uint32_t,2> callsites{0x101842d4,0x1034d308};
void capture_caller(const State& input,unsigned index,Capture& capture) {
    Clock probe;probe.base=1000000000;
    ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_persistence_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;injury_persistence_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;
    capture.inputs[index].reset();capture.callsites[index]=0;injury_persistence_diagnostics(*modern).observer=&capture;
    Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    for(auto* s:{old.get(),modern.get()}){s->pc=callsites[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    std::vector<std::uint32_t> old_sites,new_sites;
    x.on_read=[&](State& state,Effects&,std::uint32_t,unsigned){if(state.pc>=injury::entries[index] && state.pc<ends[index])old_sites.push_back(state.pc);};
    y.on_read=[&](State& state,Effects&,std::uint32_t,unsigned){if(state.pc>=injury::entries[index] && state.pc<ends[index])new_sites.push_back(state.pc);};
    const auto run=[&](State& state,Clock& clock,const std::vector<std::uint32_t>& sites,const char* mode){try{run_until(state,callsites[index]+4);}catch(const std::exception& error){std::cerr<<"Caller "<<mode<<" entry="<<hex_address(injury::entries[index])<<" pc="<<hex_address(state.pc)<<" reason="<<error.what()<<" root reads=";for(auto pc:sites)std::cerr<<hex_address(pc)<<",";std::cerr<<std::endl;for(auto it=clock.events.rbegin();it!=clock.events.rend() && it<clock.events.rbegin()+std::min<std::size_t>(8,clock.events.size());++it)std::cerr<<"Import "<<it->name<<" leave="<<it->leave<<std::endl;throw;}};
    run(*old,a,old_sites,"original");old->platform.reset();io.between();run(*modern,b,new_sites,"modern");
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;injury_persistence_diagnostics(*modern).observer=nullptr;
    Image(*old).compare(*modern,"Complete persistence through original BL");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Persistence caller effects/accounting/import ABI");io.compare();
    const auto& d=injury_persistence_diagnostics(*modern);require(d.calls[index]==1 && d.returns[index]==1 && !d.original_calls[index] && !d.faults[index] && !d.dispatch_escapes,"Dedicated productive persistence caller counter");
    if(index==0)require(modern->r<4>(12)==1,"Actual controller load succeeds");
}
constexpr std::array<std::pair<unsigned,const char*>,3> variants{{{0,"load-legacy.snapshot"},{1,"save-extended.snapshot"},{0,"load-extended.snapshot"}}};
void archive_fixture(State& state,std::uint32_t manager,std::uint32_t path,bool legacy) {
    const auto strings=std::uint32_t(state.memory.read<4>(state.r<4>(8)-0x5438));
    const auto file=invoke_original(state,0x107475b0,{path,strings+0x71,1,0x16,4});require(file!=0,"Original variant archive component open");
    state.memory.write<4>(fixture+0x80,file);
    const auto write=[&](std::uint32_t input,unsigned width,unsigned count){require(invoke_original(state,0x10747cc0,{input,width,count,file})==count,"Original variant archive record write");};
    const auto staff_count=std::uint32_t(state.memory.read<4>(state.memory.read<4>(state.r<4>(8)-0x7be4)));
    write(std::uint32_t(state.memory.read<4>(manager)),0x24,staff_count);write(manager+8,4,1);
    const auto count=std::uint32_t(state.memory.read<4>(manager+8)),records=std::uint32_t(state.memory.read<4>(manager+4));
    if(legacy){for(unsigned n=0;n<count;++n)write(records+n*0x38,0x34,1);}else write(records,0x38,count);
    write(manager+0x18,4,1);const auto first=std::uint32_t(state.memory.read<4>(manager+0x18));if(first)write(std::uint32_t(state.memory.read<4>(manager+0x10)),0x18,first);
    write(manager+0x24,4,1);const auto second=std::uint32_t(state.memory.read<4>(manager+0x24));if(second)write(std::uint32_t(state.memory.read<4>(manager+0x1c)),0x14,second);
    const auto list=std::uint32_t(state.memory.read<4>(manager+0x28));state.memory.write<2>(fixture+0x88,list?state.memory.read<2>(list):0);write(fixture+0x88,2,1);
    if(list && state.memory.read<2>(list)){state.memory.write<4>(list+12,state.memory.read<4>(list+4));for(auto row=invoke_original(state,0x10738e00,{list});row;row=invoke_original(state,0x10738e00,{list}))write(row,8,1);}
    invoke_original(state,0x10747900,{fixture+0x80});
}
void extra_cases(const std::array<std::unique_ptr<State>,2>& inputs,const std::filesystem::path& snapshots) {
    auto legacy=clone(*inputs[0]);legacy->platform=inputs[0]->platform;
    const auto path=fixture+0x280;const auto text=std::string("CM User Data:injury-legacy.dat");legacy->memory.copy_in(path,std::vector<std::uint8_t>(text.begin(),text.end()));legacy->memory.write<1>(path+text.size(),0);
    const auto saved_manager=std::uint32_t(inputs[1]->r<4>(12));archive_fixture(*legacy,saved_manager,path,true);
    legacy->memory.write<4>(legacy->memory.read<4>(legacy->r<4>(8)-0x7b74),0x3e023);legacy->wr<4>(12,inputs[0]->r<4>(12));legacy->wr<4>(16,path);legacy->wr<4>(4128,return_sentinel);legacy->pc=injury::load_entry;
    save_runtime_snapshot(*legacy,snapshots/variants[0].second);matrix(*legacy,0,false);require(runtime_sites[0].contains(0x1018493c),"Actual legacy records perform both date conversions");
    auto extended=clone(*inputs[1]);extended->platform=inputs[1]->platform;const auto manager=std::uint32_t(extended->r<4>(12)),record=std::uint32_t(extended->memory.read<4>(manager+4));
    for(auto [pointer,count,size]:{std::array<unsigned,3>{0x10,0x18,0x18},std::array<unsigned,3>{0x1c,0x24,0x14}}){const auto buffer=invoke_original(*extended,0x10775130,{size});require(buffer!=0,"Original optional-array fixture allocation");extended->memory.copy_in(buffer,extended->memory.copy_out(record,size));extended->memory.write<4>(manager+pointer,buffer);extended->memory.write<4>(manager+count,1);}
    const auto list=invoke_original(*extended,0x10773520,{16});require(list!=0,"Original archive-list fixture allocation");invoke_original(*extended,0x10738c00,{list});
    for(unsigned n=0;n<2;++n)invoke_original(*extended,0x10738d10,{list,record+n*0x38,8});extended->memory.write<4>(manager+0x28,list);
    extended->wr<4>(12,manager);extended->wr<4>(16,fixture+0x240);extended->wr<4>(4128,return_sentinel);extended->pc=injury::save_entry;save_runtime_snapshot(*extended,snapshots/variants[1].second);matrix(*extended,1,false);
    const auto extended_path=fixture+0x2c0;const auto extended_text=std::string("CM User Data:injury-extended.dat");extended->memory.copy_in(extended_path,std::vector<std::uint8_t>(extended_text.begin(),extended_text.end()));extended->memory.write<1>(extended_path+extended_text.size(),0);archive_fixture(*extended,manager,extended_path,false);
    const auto object=invoke_original(*extended,0x10773520,{0x2c});require(object!=0,"Original extended load object");extended->memory.copy_in(object,extended->memory.copy_out(manager,0x2c));extended->wr<4>(12,object);extended->wr<4>(16,extended_path);extended->wr<4>(4128,return_sentinel);extended->pc=injury::load_entry;save_runtime_snapshot(*extended,snapshots/variants[2].second);matrix(*extended,0,false);
    require(runtime_sites[0].contains(0x10184e20) && runtime_sites[1].contains(0x10185378),"Actual optional arrays and list append/iterator records reached");
    const auto backing="prepared-backing-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());ActorAttributesIoPair::copy(extended->platform->writable_directory(),snapshots/backing);
    {std::ofstream pointer(snapshots/"prepared-backing.txt");require(bool(pointer),"Retain legacy and extended original archive inputs");pointer<<backing<<std::endl;}
}

void capture_prepared(const State& original_source,const std::filesystem::path& snapshots) {
    auto owned=clone(original_source);owned->platform=original_source.platform;auto& source=*owned;
    const auto registers=source.registers;
    const auto load_file=source.platform->writable_directory()/"injury-load.dat",save_file=source.platform->writable_directory()/"injury-save.dat";
    constexpr unsigned header_size=0x40748;
    std::ifstream archive(load_file,std::ios::binary);std::vector<std::uint8_t> header(header_size);archive.read(reinterpret_cast<char*>(header.data()),header_size);require(bool(archive),"Complete original archive header template");
    const auto buffer=invoke_original(source,0x10773520,{header_size});require(buffer!=0,"Original empty archive header allocation");source.memory.copy_in(buffer,header);source.memory.write<4>(buffer+4,0);
    source.memory.copy_in(fixture+0x2c0,{'w','b',0});const auto stream=invoke_original(source,0x107768f0,{fixture+0x240,fixture+0x2c0});require(stream!=0,"Original empty archive stream");
    invoke_original(source,0x10748b80,{buffer,stream});require(invoke_original(source,0x107766f0,{stream})==0,"Original empty archive close");invoke_original(source,0x107735a0,{buffer});source.registers=registers;
    require(std::filesystem::file_size(save_file)==header_size,"Original writer creates complete empty archive header");
    const auto backing="prepared-backing-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());ActorAttributesIoPair::copy(source.platform->writable_directory(),snapshots/backing);
    {std::ofstream pointer(snapshots/"prepared-backing.txt");require(bool(pointer),"Retain initialized empty save archive");pointer<<backing<<std::endl;}
    save_runtime_snapshot(source,snapshots/"prepared-persistence.snapshot");
    const auto toc=std::uint32_t(source.r<4>(8)),manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7ba0))),load_object=std::uint32_t(source.r<4>(12));
    Capture capture;
    for(unsigned index=0;index<2;++index) {
        auto input=clone(source);input->platform=source.platform;input->code_fragment_observer=nullptr;input->wr<4>(12,index?manager:load_object);input->wr<4>(16,fixture+(index?0x240:0x200));input->wr<4>(4128,return_sentinel);input->pc=injury::entries[index];
        injury_persistence_diagnostics(*input).original=false;actor_attributes_diagnostics(*input).original=false;injury_readouts_diagnostics(*input).original=false;injury_record_diagnostics(*input).original=false;
        capture_caller(*input,index,capture);require(capture.inputs[index] && capture.callsites[index]==callsites[index],"Actual original caller captures persistence root");capture.inputs[index]->platform=source.platform;
        save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(injury::entries[index])+".snapshot"));matrix(*capture.inputs[index],index,false);actual_fault(*capture.inputs[index],index);
    }
    extra_cases(capture.inputs,snapshots);
}
void prepare_and_capture(const State& full,const std::filesystem::path& snapshots) {
    auto source=clone(full);source->platform=full.platform;source->memory.map(fixture,0x1000);
    const auto toc=std::uint32_t(source->r<4>(8)),manager=std::uint32_t(source->memory.read<4>(source->memory.read<4>(toc-0x7ba0)));
    // Retain actual initialized rows while bounding the archive fixture. The
    // full factory's roster is outside this block's proportional runtime scope.
    const auto count_slot=std::uint32_t(source->memory.read<4>(toc-0x7be4));
    const auto original_staff_count=std::uint32_t(source->memory.read<4>(count_slot));
    const auto record_count=std::uint32_t(source->memory.read<4>(manager+8));
    require(original_staff_count>=2 && record_count>=1,"Actual initialized injury rows available for bounded archive");
    const auto version_slot=std::uint32_t(source->memory.read<4>(toc-0x7b74));
    std::cout<<"Original prepared archive format discriminator="<<source->memory.read<4>(version_slot)<<std::endl;
    source->memory.write<4>(version_slot,0x3e024);
    source->memory.write<4>(count_slot,2);source->memory.write<4>(manager+8,std::min(record_count,2u));
    for(auto offset:{0x18u,0x24u})source->memory.write<4>(manager+offset,std::min(std::uint32_t(source->memory.read<4>(manager+offset)),2u));
    std::cout<<"PASS: bounded actual archive staff=2 injuryRecords="<<std::min(record_count,2u)<<" originalStaff="<<original_staff_count<<std::endl;
    const auto string=[&](std::uint32_t address,const std::string& text){source->memory.copy_in(address,std::vector<std::uint8_t>(text.begin(),text.end()));source->memory.write<1>(address+text.size(),0);};
    string(fixture+0x200,"CM User Data:injury-load.dat");string(fixture+0x240,"CM User Data:injury-save.dat");
    Clock clock;clock.base=1000000000;source->code_fragment_observer=&clock;
    invoke_original(*source,injury::save_entry,{manager,fixture+0x200});source->code_fragment_observer=nullptr;
    require(std::filesystem::is_regular_file(source->platform->writable_directory()/"injury-load.dat") && std::filesystem::file_size(source->platform->writable_directory()/"injury-load.dat")>0,"Original injury archive written to private backing");
    const auto load_object=invoke_original(*source,0x10773520,{0x2c});require(load_object!=0,"Original load controller allocation");source->memory.copy_in(load_object,source->memory.copy_out(manager,0x2c));
    const auto backing="prepared-backing-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());ActorAttributesIoPair::copy(source->platform->writable_directory(),snapshots/backing);
    {std::ofstream pointer(snapshots/"prepared-backing.txt");require(bool(pointer),"Retain private persistence backing");pointer<<backing<<std::endl;}
    save_runtime_snapshot(*source,snapshots/"prepared-persistence.snapshot");
    capture_prepared(*source,snapshots);
}
}
int main(int argc,char** argv){try {
    const bool prepared=argc==6 && std::string_view(argv[5])=="--prepared";
    require(argc==4 || argc==5 || prepared,"Expected original program/game/private backing and optional persistence profile directory");const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Persistence profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(root),"Fresh private persistence backing");OwnedBacking cleanup{root};
    if(prepared){const auto snapshots=std::filesystem::path(argv[4]).parent_path();copy_retained_backing(snapshots,root);State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(source,argv[4]);capture_prepared(source,snapshots);}
    else if(profile) {
        copy_retained_backing(argv[4],root);
        for(unsigned n=0;n<2;++n){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(injury::entries[n])+".snapshot"));matrix(input,n,true);}
        for(auto [index,name]:variants){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/name);matrix(input,index,true);}
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto full=complete_match(*game.state,game.match);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"injury-persistence-live-profile";std::filesystem::create_directories(snapshots);prepare_and_capture(*full,snapshots);
    }
    if(!profile){std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"injury-persistence-runtime-sites.txt");for(unsigned n=0;n<2;++n)for(auto pc:runtime_sites[n])log<<n<<" "<<hex_address(pc)<<std::endl;}
    std::cout<<"Runtime injury persistence source sites=";for(unsigned n=0;n<2;++n)std::cout<<(n?",":"")<<runtime_sites[n].size();std::cout<<std::endl;
    std::cout<<"Actual injury persistence pointer faults="<<fault_cases<<std::endl;
    std::cout<<"PASS: injury persistence production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
