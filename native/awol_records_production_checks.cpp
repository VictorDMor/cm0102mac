#include "awol_records_adapter.hpp"
#include "contract_query_adapter.hpp"
#include "contract_storage_adapter.hpp"
#include "contract_terms_adapter.hpp"
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
namespace injury=cm::aot::awol;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("AWOL awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
constexpr std::array<std::uint32_t,21> ends{0x100032f8,0x100033b8,0x100036dc,0x10003dc0,0x10003ec0,0x10003fa0,0x10004068,0x100066d0,0x100067d0,0x10006828,0x10006900,0x100072e0,0x100065f8,0x10006074,0x10007100,0x10008b58,0x10007a4c,0x1000909c,0x10009ee8,0x10008770,0x10009b54};
std::array<std::set<std::uint32_t>,21> runtime_sites;
void record_site(State& s,std::uint32_t pc) {
    if(s.executed>=s.budget)return;
    for(unsigned n=0;n<injury::record_entries.size();++n)
        if(pc>=injury::record_entries[n] && pc<ends[n])runtime_sites[n].insert(pc);
}
struct Witness:AwolRecordsBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,AwolRecordsOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Negative:AwolRecordsBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,AwolRecordsOutcome result)override {
        if(entry==injury::record_entries[index] && !original && result==AwolRecordsOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
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
    require(relative.parent_path().empty() && name.starts_with("prepared-backing-") && std::filesystem::is_directory(snapshots/relative) && !std::filesystem::is_symlink(snapshots/relative),"Retained original AWOL history backing");
    ActorAttributesIoPair::copy(snapshots/relative,root);
}
unsigned cases=0,returns=0,budgets=0,negatives=0;
unsigned released_forks=0;
void between_runs(ActorAttributesIoPair& io) {
    // A budget/fault can leave an original native write fork open. Release
    // those host resources before restoring the same private file for the
    // modern run; otherwise Carbon rejects its second exclusive writer.
    // This cleanup performs no guest access or guest import and does not
    // change the architectural state/effects being compared.
    for(const auto& [guest,native]:io.old_forks.references) {
        (void)guest;require(io.old_forks.forks.close(native)==0,"Release original native fork at comparison boundary");++released_forks;
    }
    io.old_forks.references.clear();io.between();
}
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_awol_records(s,injury::record_entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,injury::record_entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool budget_expected=true) {
    Clock probe_clock;probe_clock.base=1000000000;
    ActorAttributesIoPair io=[&]() {
        try {return ActorAttributesIoPair(input,index,injury::record_entries[index],[&](State& x){awol_records_diagnostics(x).original=true;contract_query_diagnostics(x).original=true;contract_storage_diagnostics(x).original=true;contract_terms_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::record_entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    awol_records_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;injury_readouts_diagnostics(*a).original=true;injury_record_diagnostics(*a).original=true;contract_query_diagnostics(*a).original=true;contract_storage_diagnostics(*a).original=true;contract_terms_diagnostics(*a).original=true;auto& d=awol_records_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();between_runs(io);const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production AWOL entry="+hex_address(injury::record_entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::record_entries[index],[&](State& x){awol_records_diagnostics(x).original=true;contract_query_diagnostics(x).original=true;contract_storage_diagnostics(x).original=true;contract_terms_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {
        constexpr std::uint32_t bad=0xf0000000;
        if(index==12)s->memory.write<4>(s->r<4>(8)-0x7b84,bad);
        else s->wr<4>((index==11 || index==15 || (index>=16 && index<=18))?16:12,bad);
        s->pc=injury::record_entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;
    }
    awol_records_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;contract_query_diagnostics(*old).original=true;contract_storage_diagnostics(*old).original=true;contract_terms_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();between_runs(io);const auto actual=execute(*modern,Path::generated,index);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual AWOL pointer fault original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual AWOL pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual AWOL fault effects/accounting/import ABI");io.compare();
    const auto& d=awol_records_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual AWOL fault counters");++fault_cases;
}

std::uint32_t invoke_setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);
    s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    run_until(s,return_sentinel);return std::uint32_t(s.r<4>(12));
}
std::uint32_t original_setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    auto& selected=awol_records_diagnostics(s).original;
    auto& query=contract_query_diagnostics(s).original;auto& storage=contract_storage_diagnostics(s).original;auto& terms=contract_terms_diagnostics(s).original;
    struct Restore {bool& value;bool& query;bool& storage;bool& terms;bool previous,old_query,old_storage,old_terms;
        ~Restore(){value=previous;query=old_query;storage=old_storage;terms=old_terms;}} restore{selected,query,storage,terms,selected,query,storage,terms};
    selected=query=storage=terms=true;
    return invoke_setup(s,entry,args);
}
void arguments(State& s,std::initializer_list<std::uint32_t> args) {
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);
    s.wr<4>(4128,return_sentinel);
}
void matrix(const State& input,unsigned index,bool profile) {
    if(profile) {
        compare(input,index,Path::generated,Mode::profile);
        compare(input,index,Path::cfm,Mode::profile);
        return;
    }
    std::uint64_t steps=0;
    for(auto path:{Path::direct,Path::generated,Path::cfm}) {
        const auto elapsed=compare(input,index,path,Mode::modern);
        if(path==Path::generated)steps=elapsed;
    }
    compare(input,index,Path::generated,Mode::original);
    compare(input,index,Path::generated,Mode::entry_original);
    compare(input,index,Path::generated,Mode::modern,50000000,true);
    require(steps>3,"Measure AWOL operation before focused budget boundaries");
    for(auto cut:{std::uint64_t(0),steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    actual_fault(input,index);
    std::cout<<"PASS: actual AWOL root="<<hex_address(injury::record_entries[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
struct World {std::uint32_t manager,player,club;};
World prepare_world(State& s) {
    const auto toc=std::uint32_t(s.r<4>(8));
    // Reproduce the original career's allocation/construction/publication at
    // 0x10344658..0x10344678 and 0x10345218..0x10345238.
    for(auto [offset,size,constructor]:{std::array<unsigned,3>{0x7bf0,20,0x102cb930},std::array<unsigned,3>{0x7ba4,12,0x101094d0}}) {
        const auto slot=std::uint32_t(s.memory.read<4>(toc-offset));
        if(!s.memory.read<4>(slot)) {
            const auto object=original_setup(s,0x10773520,{size});require(object!=0,"Original AWOL management-service allocation");
            original_setup(s,constructor,{object,0});s.memory.write<4>(slot,object);
        }
    }
    // The same original AWOL constructor/publication used by the career.
    const auto manager=original_setup(s,0x10773520,{16});require(manager!=0,"Original AWOL controller allocation");
    require(original_setup(s,injury::constructor_entry,{manager,0})==manager,"Original AWOL controller construction");
    s.memory.write<4>(s.memory.read<4>(toc-0x7b24),manager);
    const auto count=std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc-0x7be4)));
    const auto staff_base=std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc-0x7be0)));
    std::uint32_t player=0;
    for(unsigned id=0;id<count;++id) {
        const auto staff=staff_base+id*0x70,club=std::uint32_t(s.memory.read<4>(staff+0x24));
        if(club && s.memory.read<4>(club+0x90) && s.memory.read<4>(staff+0x14) &&
           s.memory.read<4>(staff+0x28) && s.memory.read<4>(staff+0x2c) && s.memory.read<4>(staff+0x30)) {player=staff;break;}
    }
    require(player!=0 && count==153796,"Prepared AWOL world retains the actual staff population and a managed player");
    const auto club=std::uint32_t(s.memory.read<4>(player+0x24));
    const auto contracts=std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc-0x7b7c)));
    require(contracts!=0,"Original contract controller exists");
    if(!original_setup(s,0x10082f20,{contracts,player})) {
        // Selected-player portion of the original contract population:
        // 0x10078500..0x10078528 and 0x1007882c..0x10078838.
        const auto data=original_setup(s,0x10084be0,{contracts,player,club,1,0xffffffff,0});
        require(data!=0,"Original actual-player contract preparation");
        original_setup(s,0x10078e70,{contracts,player,data});
    }
    const auto contract=original_setup(s,0x10082f20,{contracts,player});
    require(contract && s.memory.read<4>(contract+4)==s.memory.read<4>(club),"Original contract binds the actual player and club");
    std::cout<<"Prepared original AWOL world staff="<<count<<" player="<<s.memory.read<4>(player)<<" club="<<s.memory.read<4>(club)<<std::endl;
    return {manager,player,club};
}
std::uint32_t first_item(State& s,std::uint32_t list) {
    require(list && s.memory.read<2>(list)>0,"Actual populated AWOL list");
    // The original iterator reset and next protocol used by AWOL callers.
    s.memory.write<4>(list+12,s.memory.read<4>(list+4));
    const auto item=original_setup(s,0x10738e00,{list});require(item!=0,"Original AWOL list iterator");return item;
}
std::uint32_t name_index(const State& s,std::uint32_t staff,unsigned n) {
    return std::uint32_t(s.memory.read<4>(s.memory.read<4>(staff+0x28+n*4)+0x20));
}
void message_case(const State& source,const World& w,unsigned index,bool profile) {
    auto input=clone(source);input->platform=source.platform;
    const auto message=original_setup(*input,0x10773520,{0x400}),output=original_setup(*input,0x10773520,{16});
    require(message && output,"Original AWOL message and rich-text allocations");
    const auto id=std::uint32_t(input->memory.read<4>(w.player)),club=std::uint32_t(input->memory.read<4>(w.club));
    const auto reporter=std::uint32_t(input->memory.read<4>(w.club+0x90));require(reporter!=0,"Actual AWOL reporter");
    const auto first=name_index(*input,w.player,0),middle=name_index(*input,w.player,1),last=name_index(*input,w.player,2);
    std::vector<std::uint32_t> fields;
    std::uint32_t type=0;
    if(index==16){type=0xbc5;fields={id,first,middle,last,0xffffffff,club,4,7,0xffffffff};}
    else if(index==17){type=0xbc9;fields={id,first,middle,last,club,0,0,std::uint32_t(input->memory.read<4>(reporter)),name_index(*input,reporter,0),name_index(*input,reporter,1),name_index(*input,reporter,2)};}
    else {require(index==18,"Known AWOL message renderer");type=0x138f;fields={id,club,0,first,middle,last,std::uint32_t(input->memory.read<4>(reporter)),name_index(*input,reporter,0),name_index(*input,reporter,1),name_index(*input,reporter,2)};}
    original_setup(*input,0x102bdf90,{message,type,0});
    for(unsigned n=0;n<fields.size();++n)original_setup(*input,0x102cb4b0,{message,n,fields[n]});
    original_setup(*input,0x1074b6e0,{output,0});
    arguments(*input,{w.manager,message,1,output});matrix(*input,index,profile);
}
}
int main(int argc,char** argv){try {
    require(argc==5 || (argc==6 && std::string_view(argv[5])=="--profile"),"Expected original program, game, private backing and prepared world snapshot, optionally --profile");
    const bool profile=argc==6;require(profile==match_pc_profile_enabled(),"AWOL profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    require(!std::filesystem::exists(root),"Fresh private AWOL backing");OwnedBacking cleanup{root};
    copy_retained_backing(std::filesystem::path(argv[4]).parent_path(),root);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(source,argv[4]);
    Clock clock;clock.base=1000000000;source.code_fragment_observer=&clock;
    const auto w=prepare_world(source);source.memory.map(fixture,0x1000);
    const auto check=[&](const State& base,unsigned index,std::initializer_list<std::uint32_t> args){
        auto input=clone(base);input->platform=base.platform;arguments(*input,args);matrix(*input,index,profile);
    };
    {
        auto fresh=clone(source);fresh->platform=source.platform;
        const auto object=original_setup(*fresh,0x10773520,{16});require(object!=0,"Actual constructor input allocation");
        arguments(*fresh,{object,0});matrix(*fresh,0,profile);
    }
    check(source,12,{10});check(source,14,{w.manager,w.player,0,4});
    check(source,19,{w.manager,w.player});check(source,20,{w.manager,w.player,w.club});
    original_setup(source,injury::registration_entry,{w.manager,w.player,0,4});
    original_setup(source,injury::training_event_entry,{w.manager,w.player});
    original_setup(source,injury::fitness_event_entry,{w.manager,w.player,w.club});
    require(source.memory.read<2>(source.memory.read<4>(w.manager))==1 && source.memory.read<2>(source.memory.read<4>(w.manager+4))==1 &&
            source.memory.read<2>(source.memory.read<4>(w.manager+8))==1,"Original AWOL preparation populates all three real lists");
    for(unsigned index:{1u,4u,5u,6u,7u,10u,13u})check(source,index,{w.manager,w.player,0});
    check(source,8,{w.manager,first_item(source,std::uint32_t(source.memory.read<4>(w.manager)))});
    check(source,9,{w.manager,first_item(source,std::uint32_t(source.memory.read<4>(w.manager+4)))});
    check(source,11,{w.manager,w.player,w.club,4,7});check(source,15,{w.manager,w.player,w.club});
    for(unsigned index:{16u,17u,18u})message_case(source,w,index,profile);
    constexpr auto path=fixture+0x200,save_path=fixture+0x240;
    const auto string=[&](std::uint32_t address,const std::string& text){source.memory.copy_in(address,std::vector<std::uint8_t>(text.begin(),text.end()));source.memory.write<1>(address+text.size(),0);};
    string(path,"CM User Data:awol-load.dat");string(save_path,"CM User Data:awol-save.dat");
    source.memory.write<4>(source.memory.read<4>(source.r<4>(8)-0x7b74),0x3e024);
    require(original_setup(source,injury::save_entry,{w.manager,path})==1,"Original AWOL archive creation");
    require(std::filesystem::is_regular_file(root/"awol-load.dat") && std::filesystem::file_size(root/"awol-load.dat")>153796,"Actual AWOL archive exists");
    // Original component archives reject duplicate component names. Preserve a
    // complete original header with zero components as the save baseline,
    // separately from the populated archive used by the loader.
    constexpr unsigned header_size=0x40748;
    std::ifstream archive(root/"awol-load.dat",std::ios::binary);std::vector<std::uint8_t> header(header_size);
    archive.read(reinterpret_cast<char*>(header.data()),header_size);require(bool(archive),"Complete original AWOL archive header");
    const auto buffer=original_setup(source,0x10773520,{header_size});require(buffer!=0,"Original empty AWOL header allocation");
    source.memory.copy_in(buffer,header);source.memory.write<4>(buffer+4,0);string(fixture+0x280,"wb");
    const auto stream=original_setup(source,0x107768f0,{save_path,fixture+0x280});require(stream!=0,"Original empty AWOL archive stream");
    original_setup(source,0x10748b80,{buffer,stream});require(original_setup(source,0x107766f0,{stream})==0,"Original empty AWOL archive close");
    original_setup(source,0x107735a0,{buffer});
    require(std::filesystem::file_size(root/"awol-save.dat")==header_size,"Original writer creates the empty AWOL archive header");
    check(source,3,{w.manager,save_path});
    {
        auto load=clone(source);load->platform=source.platform;
        const auto object=original_setup(*load,0x10773520,{16});require(object!=0,"Actual AWOL archive load object allocation");
        for(unsigned n=0;n<4;++n)load->memory.write<4>(object+n*4,0);
        arguments(*load,{object,path});matrix(*load,2,profile);
    }
    if(!profile) {
        std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"awol-records-runtime-sites.txt");
        for(unsigned n=0;n<runtime_sites.size();++n){require(!runtime_sites[n].empty(),"Every AWOL root has actual-data runtime coverage");for(auto pc:runtime_sites[n])log<<n<<" "<<hex_address(pc)<<std::endl;}
    }
    std::cout<<"PASS: AWOL production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<fault_cases<<" releasedNativeForks="<<released_forks<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
