#include "injury_daily_adapter.hpp"
#include "awol_records_adapter.hpp"
#include "contract_query_adapter.hpp"
#include "contract_storage_adapter.hpp"
#include "contract_terms_adapter.hpp"
#include "contract_review_adapter.hpp"
#include "transfer_offer_adapter.hpp"
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
namespace injury=cm::aot::injury_daily;
enum class Path { direct, generated, cfm, first_caller, second_caller };
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Daily injury awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
constexpr std::array<std::uint32_t,1> ends{0x101864a0};
std::array<std::set<std::uint32_t>,1> runtime_sites;
void record_site(State& s,std::uint32_t pc) {if(s.executed<s.budget && pc>=injury::entry && pc<ends[0])runtime_sites[0].insert(pc);}
struct Witness:InjuryDailyBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,InjuryDailyOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Negative:InjuryDailyBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,InjuryDailyOutcome result)override {
        if(entry==injury::entries[index] && !original && result==InjuryDailyOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
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
    require(relative.parent_path().empty() && name.starts_with("prepared-backing-") && std::filesystem::is_directory(snapshots/relative) && !std::filesystem::is_symlink(snapshots/relative),"Retained original daily injury history backing");
    ActorAttributesIoPair::copy(snapshots/relative,root);
}
unsigned cases=0,returns=0,budgets=0,negatives=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_injury_daily(s,injury::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,injury::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            if(path==Path::first_caller || path==Path::second_caller) {
                s.pc=path==Path::first_caller?0x10119250:0x10119810;
                run_until(s,s.pc+4);
            } else run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool budget_expected=true) {
    Clock probe_clock;probe_clock.base=1000000000;
    ActorAttributesIoPair io=[&]() {
        try {return ActorAttributesIoPair(input,index,injury::entries[index],[&](State& x){injury_daily_diagnostics(x).original=true;contract_query_diagnostics(x).original=true;contract_storage_diagnostics(x).original=true;contract_terms_diagnostics(x).original=true;contract_review_diagnostics(x).original=true;transfer_offer_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    injury_daily_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;injury_readouts_diagnostics(*a).original=true;injury_record_diagnostics(*a).original=true;contract_query_diagnostics(*a).original=true;contract_storage_diagnostics(*a).original=true;contract_terms_diagnostics(*a).original=true;contract_review_diagnostics(*a).original=true;transfer_offer_diagnostics(*a).original=true;auto& d=injury_daily_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production injury-daily entry="+hex_address(injury::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_daily_diagnostics(x).original=true;contract_query_diagnostics(x).original=true;contract_storage_diagnostics(x).original=true;contract_terms_diagnostics(x).original=true;contract_review_diagnostics(x).original=true;transfer_offer_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {
        constexpr std::uint32_t bad=0xf0000000;s->wr<4>(12,bad);
        s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;
    }
    injury_daily_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;contract_query_diagnostics(*old).original=true;contract_storage_diagnostics(*old).original=true;contract_terms_diagnostics(*old).original=true;contract_review_diagnostics(*old).original=true;transfer_offer_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();io.between();const auto actual=execute(*modern,Path::generated,index);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual daily injury pointer fault original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual daily injury pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual daily injury fault effects/accounting/import ABI");io.compare();
    const auto& d=injury_daily_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual daily injury fault counters");++fault_cases;
}

std::uint32_t invoke_setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);
    s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    run_until(s,return_sentinel);return std::uint32_t(s.r<4>(12));
}
void prepare_management_services(State& s) {
    const auto toc=std::uint32_t(s.r<4>(8));
    const auto slot=std::uint32_t(s.memory.read<4>(toc-0x7b24));
    if(s.memory.read<4>(slot))return;
    // The isolated match factory omits this management service. Reproduce the
    // original allocation, constructor and publication at 0x1034530c..0x1034532c.
    const auto before=s.executed;const auto object=invoke_setup(s,0x10773520,{16});
    require(object!=0,"Original daily availability-service allocation");
    {
        auto& diagnostics=awol_records_diagnostics(s);
        struct RestoreOriginal {
            bool& selected;bool previous;
            ~RestoreOriginal(){selected=previous;}
        } restore{diagnostics.original,diagnostics.original};
        diagnostics.original=true;
        require(invoke_setup(s,0x100031b0,{object,0})==object,"Original daily availability-service constructor");
    }
    require(s.memory.read<4>(object+12)!=0,"Original constructor allocates full staff availability flags");
    s.memory.write<4>(slot,object);
    std::cout<<"Prepared original availability service instructions="<<s.executed-before<<std::endl;
}
void matrix(const State& input,bool profile) {
    if(profile) {
        compare(input,0,Path::generated,Mode::profile);
        compare(input,0,Path::cfm,Mode::profile);
        return;
    }
    std::uint64_t steps=0;
    for(auto path:{Path::direct,Path::generated,Path::cfm,Path::first_caller,Path::second_caller}) {
        const auto elapsed=compare(input,0,path,Mode::modern);
        if(path==Path::generated)steps=elapsed;
    }
    compare(input,0,Path::generated,Mode::original);
    compare(input,0,Path::generated,Mode::entry_original);
    compare(input,0,Path::generated,Mode::modern,50000000,true);
    require(steps>3,"Measure daily controller before focused budget boundaries");
    for(auto cut:{std::uint64_t(0),steps/2,steps-1})compare(input,0,Path::generated,Mode::modern,unsigned(cut));
    actual_fault(input,0);
}
}
int main(int argc,char** argv){try {
    require(argc==5 || (argc==6 && std::string_view(argv[5])=="--profile"),"Expected original program, game, private backing and prepared world snapshot, optionally --profile");
    const bool profile=argc==6;require(profile==match_pc_profile_enabled(),"Daily profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    require(!std::filesystem::exists(root),"Fresh private daily backing");OwnedBacking cleanup{root};
    copy_retained_backing(std::filesystem::path(argv[4]).parent_path(),root);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(source,argv[4]);
    const auto toc=std::uint32_t(source.r<4>(8));
    const auto manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7ba0)));
    const auto count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4)));
    require(manager && count>2,"Prepared original daily world retains the full staff population");
    prepare_management_services(source);
    source.wr<4>(12,manager);source.wr<4>(4128,return_sentinel);source.pc=injury::entry;
    std::cout<<"Prepared daily actual staff="<<count<<" injury_records="<<source.memory.read<4>(manager+8)<<std::endl;
    matrix(source,profile);
    if(!profile){std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"injury-daily-runtime-sites.txt");for(auto pc:runtime_sites[0])log<<hex_address(pc)<<std::endl;}
    std::cout<<"Runtime daily source sites="<<runtime_sites[0].size()<<std::endl;
    std::cout<<"PASS: daily production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<fault_cases<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
