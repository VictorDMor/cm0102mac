#include "contract_manager_negotiation_detail.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace cm::aot {
bool contract_manager_negotiation_reference(State&);
std::vector<std::uint32_t> contract_manager_negotiation_calls();
std::vector<std::uint32_t> contract_manager_negotiation_stops();
void contract_manager_negotiation_constants(State&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;using guest::Cpu;
constexpr auto entry=0x10085860u,end=0x10089a40u,implemented_end=end;
const std::vector<std::uint32_t> checkpoints={};
bool is_checkpoint(std::uint32_t pc){return std::find(checkpoints.begin(),checkpoints.end(),pc)!=checkpoints.end();}
constexpr auto object=fixture+0x1000,person=fixture+0x2000,clubs=fixture+0x3000,contract=fixture+0x5000;
constexpr auto attributes=fixture+0x6000,globals=fixture+0x7000,division=fixture+0x8000,competition=fixture+0x9000;
constexpr auto context=fixture+0xa000,traits=fixture+0xb000,details=fixture+0xc000;
constexpr auto constants=fixture+0x10000,singles=fixture+0x12000,squad_role_table=fixture+0x13000,age_limits=fixture+0x14000;
constexpr auto finance_table=fixture+0x15000,finance_record=fixture+0x16000,index_table=fixture+0x17000;
constexpr auto employer_location=fixture+0x18000,buyer_location=fixture+0x19000;
constexpr auto shortlist_record=fixture+0x1a000;
struct Options {
    int age=24,contract_kind=2,contract_kind_bits=0,locked=0,squad_role=2,mood=10,promise=0,terms=0;
    int level=2,standing=10,club_score=5000,club_kind=0,manager_trait=10,ability=100,reputation=6000,suitability=2;
    std::uint32_t base_value=1000000,tenure=1,option=1;unsigned interrupt=0,fp_state=0;
    int elapsed=200,status=0,joined_year=2001,current_year=2001,diagnostic_flags=0x5a5a;std::int64_t balance=1000000;
    int employer_id=0,buyer_id=1,relationship=0,rival_slot=0;
    int first_score=5000,potential_score=7000;
    int first_finance_class=0,next_finance_class=0,secondary_status=0,expiry_days=365,adjustment_protection=-1,price_protection=-1;
    int person_club_score=3000;
    int regional_country=100,person_quote_status=0,employer_quote_status=0,regional_first_class=0,regional_next_class=0;
    int regional_first_predicate=0,regional_second_predicate=0;
    int lookup_reputation=std::numeric_limits<int>::max();
    int first_profile_trait=10,second_profile_trait=10;
    int goalkeeper=0,first_role=20,second_role=20,profile_attribute=-999,first_skill=15,second_skill=15;
    std::array<int,5> profile_values={7,11,13,17,19};
    int manager_bonus_trait=10,manager_day=0,manager_year=2000,club_ambition=10,player_trait=45;
    int contract_policy=0,final_cap_active=0,buyer_reputation=std::numeric_limits<int>::min();
    std::uint32_t final_cap=50000000;
    std::uint32_t stored_value=777777;
    bool employer=true,buyer=true,contract=true,human=false,national=true,national_active=true,listed=false;
    bool special=false,context=true,traits=true,division=true,competition=true,diagnostics=true;
    bool protected_player=false,finance=true;
    bool transfer_status=false,secondary_contract=true,person_club=true,separate_person_club=false,change_club_on_classification=false;
    bool shortlist=true,shortlist_matches=true,shortlist_empty=false;
    bool manager=true;
    bool employer_location=true,buyer_location=true,same_region=false,buyer_competition=true,same_competition=true,excluded_competition=false;
    bool corrupt=false,corrupt_assessment=false,corrupt_power=false,corrupt_finances=false,corrupt_relationship=false,corrupt_quality_power=false,corrupt_transfer_status=false,corrupt_shortlist=false,corrupt_first_skill=false,corrupt_cap_result=false;
};
struct Event{std::uint32_t target,back;std::vector<std::uint32_t> arguments,kept,volatile_arguments;std::vector<std::uint64_t> fp;std::vector<std::uint8_t> fpscr,conditions;bool operator==(const Event&)const=default;};
unsigned arity(std::uint32_t target){switch(target){
    case 0x10780fc0:return 0;
    case 0x100c96e0:case 0x100f55b0:case 0x10775540:case 0x1037fa70:case 0x10380520:return 1;
    case 0x10147e70:case 0x100d3150:case 0x10103510:case 0x10773c80:case 0x10464160:case 0x100d3040:case 0x100d3300:case 0x100d1cb0:return 2;
    case 0x10082f90:case 0x10149df0:case 0x10003dc0:case 0x100ce320:case 0x10718130:return 3;
    case 0x103ecef0:return 4;
    case 0x1014c2e0:case 0x10219e40:case 0x1036d240:return 5;
    case 0x10084be0:return 6;
    default:throw std::runtime_error("Unknown negotiation prefix dependency "+hex_address(target));
}}
struct Model {
    Options o;std::vector<Event> calls;unsigned classification_calls=0;bool club_changed=false,lookup_changed=false;explicit Model(Options options):o(std::move(options)){}
    void call(State& s,std::uint32_t target,std::uint32_t back){
        Cpu c(s);
        // The C++ frame implements the original's two frameless FP leaves.
        // These are ABI stores/reloads, not scripted business dependencies.
        if(target==0x107737f0||target==0x10773840){for(unsigned n=22;n<32;++n){const auto at=c.sp()-0x50+(n-22)*8;if(target==0x107737f0)c.put64(at,s.r<8>(guest::reg::fpr(n)));else s.wr<8>(guest::reg::fpr(n),c.u64(at));}s.pc=back;return;}
        Event e{target,back,{},{},{},{},{},{}};for(unsigned n=0;n<arity(target);++n)e.arguments.push_back(c.r(n+3));for(unsigned n=13;n<32;++n)e.kept.push_back(c.r(n));for(unsigned n=3;n<11;++n)e.volatile_arguments.push_back(c.r(n));
        for(unsigned n=0;n<32;++n)e.fp.push_back(s.r<8>(guest::reg::fpr(n)));
        for(unsigned n=1280;n<1312;++n)e.fpscr.push_back(std::uint8_t(s.r<1>(n)));for(unsigned n=0;n<8;++n)e.conditions.push_back(c.cr(n));e.conditions.push_back(std::uint8_t(s.r<1>(guest::reg::xer_ca)));calls.push_back(e);
        std::uint32_t answer=0;double floating=0;
        if(target==0x10082f90)answer=o.contract?contract:0;
        if(target==0x1014c2e0)answer=o.human;
        if(target==0x100c96e0)answer=o.context?context:0;
        if(target==0x10147e70)answer=o.national;
        if(target==0x10149df0)answer=o.national_active;
        if(target==0x10003dc0)answer=o.special;
        if(target==0x10219e40)answer=std::uint32_t(o.suitability+(o.corrupt_assessment?1:0));
        if(target==0x10084be0)answer=o.base_value;
        if(target==0x100d3150)answer=o.protected_player;
        if(target==0x100d3150&&back==0x10086a68&&o.adjustment_protection>=0)answer=std::uint32_t(o.adjustment_protection);
        if(target==0x100d3150&&back==0x10086c00&&o.price_protection>=0)answer=std::uint32_t(o.price_protection);
        if(target==0x103ecef0)answer=o.corrupt_transfer_status?!o.transfer_status:o.transfer_status;
        if(target==0x100ce320){answer=o.secondary_contract?contract+0x200:0;c.put8(c.r(4),std::uint32_t(o.secondary_status));}
        if(target==0x100f55b0)answer=std::uint32_t(classification_calls++?o.next_finance_class:o.first_finance_class);
        if(target==0x100f55b0&&back==0x100871d8)answer=std::uint32_t(o.regional_first_class);
        if(target==0x100f55b0&&back==0x100871ec)answer=std::uint32_t(o.regional_next_class);
        if(target==0x100f55b0&&back==0x10086df0&&o.change_club_on_classification){c.put32(person+0x24,clubs+0x500);club_changed=true;}
        if(target==0x1036d240){answer=(o.corrupt_shortlist?!o.shortlist:o.shortlist)?shortlist_record:0;if(o.lookup_reputation!=std::numeric_limits<int>::max()){c.put16(clubs+0x1ec,std::uint32_t(o.lookup_reputation));lookup_changed=true;}}
        if(target==0x100d3040)answer=std::uint32_t(o.regional_first_predicate);
        if(target==0x100d3300)answer=std::uint32_t(o.regional_second_predicate);
        if(target==0x10775540){const auto value=std::int32_t(c.r(3));answer=value<0?0u-std::uint32_t(value):std::uint32_t(value);}
        if(target==0x10718130)answer=std::uint32_t(o.expiry_days);
        if(target==0x100d1cb0)answer=std::uint32_t(c.r(4)==11?o.first_skill+(o.corrupt_first_skill?1:0):o.second_skill);
        if(target==0x1037fa70)answer=std::uint32_t(o.contract_policy);
        if(target==0x10380520)answer=std::uint32_t(o.corrupt_cap_result?!o.final_cap_active:o.final_cap_active);
        if(target==0x10103510)answer=o.finance?finance_record:0;
        if(target==0x10464160)answer=std::uint32_t(o.corrupt_relationship?!o.relationship:o.relationship);
        if(target==0x10773c80)floating=double(std::int64_t((std::uint64_t(c.r(3))<<32)|c.r(4)))+(o.corrupt_finances?1000000:0);
        if(target==0x10780fc0){floating=std::pow(c.f(1),c.f(2))+(o.corrupt_power||(o.corrupt_quality_power&&back==0x100868bc)?0.25:0);c.put32(c.sp()+0x14,c.r(2));c.set(2,0xdeadc0de);}
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);for(unsigned n=0;n<14;++n)c.set_f(n,-100.0-n);c.set_cr(0,2);c.set(3,answer);if(target==0x10780fc0||target==0x10773c80)c.set_f(1,floating);s.pc=back;
        if(o.interrupt&&calls.size()==o.interrupt)throw std::runtime_error("Negotiation prefix dependency interrupted");
    }
};
thread_local Model* active_model=nullptr;struct Hook final:ContractManagerCallHook{bool call(State& s)override{active_model->call(s,s.pc,std::uint32_t(s.r<4>(4128)));return true;}}hook;
std::unique_ptr<State> seed(const Options& o){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x20000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);Cpu c(*s);
    for(unsigned n=0;n<0x5000;++n)c.put8(stack-0x4000+n,n*13+7);for(unsigned n=0;n<32;++n)c.set_f(n,double(n)+0.25);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,n>=1304?0:o.fp_state==1?1:o.fp_state==2?n&1:0);
    // CR fields have four architectural bits; poisoned high padding bits
    // are not valid machine input. Keep GPRs, FPRs and stack patterned.
    for(unsigned n=0;n<8;++n)c.set_cr(n,5);
    s->wr<1>(guest::reg::xer_so,1);s->wr<1>(guest::reg::xer_ov,0);s->wr<1>(guest::reg::xer_ca,1);
    c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,object);c.set(4,o.employer?clubs:0);c.set(5,o.buyer?clubs+0x248:0);c.set(6,person);c.set(7,o.diagnostics?details:0);c.set(8,o.tenure);c.set(9,o.option);s->pc=entry;s->budget=1000000;
    unsigned slot_index=0;const auto holder=[&](int slot,std::uint32_t value){const auto at=globals+slot_index++*4;c.put32(toc+slot,at);c.put32(at,value);};
    holder(-0x7a08,7);holder(-0x7a54,context+0x100);holder(-0x79fc,1);holder(-0x7b40,context+0x200);holder(-0x7b24,context+0x300);c.put32(toc-0x7b80,globals+0x100);
    holder(-0x7ab8,context+0x400);holder(-0x7b9c,index_table);
    holder(-0x7a04,6);holder(-0x7a0c,o.excluded_competition?0:999);
    for(unsigned n=0;n<9;++n){constexpr int slots[]={-0x7a10,-0x7a14,-0x7a18,-0x7a00,-0x7af4,-0x7a1c,-0x7a20,-0x7a24,-0x7a28};holder(slots[n],100+n);}
    c.put32(context+0x200+0x988,context+0x800);c.put32(shortlist_record+0x10,o.shortlist_empty?0xffffffffu:o.shortlist_matches?0:1);c.put8(person+0x6d,std::uint32_t(o.person_quote_status));c.put32(division+0x68,std::uint32_t(o.regional_country));
    contract_manager_negotiation_constants(*s,toc,constants,singles,squad_role_table,age_limits,finance_table);
    c.put16(globals+0x100,300);c.put16(globals+0x102,std::uint32_t(o.current_year));c.put16(person+0x4c,std::uint32_t(300-o.elapsed));c.put16(person+0x4e,std::uint32_t(o.joined_year));
    c.put64(finance_record,std::uint64_t(o.balance));c.put8(index_table+12,std::uint32_t(o.status));
    c.put32(person,0);c.put32(person+8,o.stored_value);c.put32(person+0x14,attributes);c.put32(person+0x24,o.person_club?clubs+(o.separate_person_club?0x500:0):0);c.put16(person+0x5c,std::uint32_t(o.age));c.put16(attributes+6,std::uint32_t(o.ability));c.put16(attributes+10,std::uint32_t(o.reputation));c.put16(clubs+0x500+0x1ec,std::uint32_t(o.person_club_score));
    c.put16(attributes+8,std::uint32_t(o.first_score));c.put16(attributes+12,std::uint32_t(o.potential_score));
    c.put16(attributes+4,std::uint32_t(o.player_trait));c.put8(attributes+0xf,std::uint32_t(o.goalkeeper));c.put8(attributes+0x36,std::uint32_t(o.first_role));c.put8(attributes+0x1b,std::uint32_t(o.second_role));for(unsigned n=0;n<5;++n){constexpr unsigned offsets[]={0x3c,0x44,0x3d,0x3f,0x2e};c.put8(attributes+offsets[n],std::uint32_t(o.profile_attribute==-999?o.profile_values[n]:o.profile_attribute));}
    c.put8(person+0x67,std::uint32_t(o.first_profile_trait));c.put8(person+0x65,std::uint32_t(o.second_profile_trait));
    c.put8(contract+0x38,(std::uint32_t(o.contract_kind)<<2)|std::uint32_t(o.contract_kind_bits));c.put8(contract+0x54,std::uint32_t(o.locked));c.put8(contract+0x5c,o.listed?4:0);c.put8(contract+0x60,std::uint32_t(o.squad_role));c.put8(contract+0x44,std::uint32_t(o.mood));c.put8(contract+0x4c,std::uint32_t(o.promise));c.put32(contract+0x50,std::uint32_t(o.terms));
    c.put32(contract+0x24,o.final_cap);
    const auto absolute_day=300+o.expiry_days;const auto years=absolute_day>=0?absolute_day/365:-((-absolute_day+364)/365);c.put16(contract+0x30,std::uint32_t(absolute_day-years*365));c.put16(contract+0x32,std::uint32_t(o.current_year+years));
    c.put32(context+0x1c,o.traits?traits:0);c.put8(traits+0x34,std::uint32_t(o.manager_trait));c.put16(details,std::uint32_t(o.diagnostic_flags));
    c.put8(traits+0x42,std::uint32_t(o.manager_bonus_trait));c.put16(context+0x4c,std::uint32_t(o.manager_day));c.put16(context+0x4e,std::uint32_t(o.manager_year));
    for(unsigned n=0;n<2;++n){const auto club=clubs+n*0x248;c.put32(club,std::uint32_t(n?o.buyer_id:o.employer_id));c.put32(club+0x2c,o.division?division:0);c.put32(club+0x30,o.competition&&(!n||o.buyer_competition)?competition+(n&&!o.same_competition?0x400:0):0);c.put32(club+0x90,o.manager?context:0);const auto reputation=n&&o.buyer_reputation!=std::numeric_limits<int>::min()?o.buyer_reputation:o.club_score;c.put16(club+0x1ec,std::uint32_t(reputation));c.put8(club+0x23d,std::uint32_t(o.club_kind));c.put8(club+0x240,std::uint32_t(o.club_ambition));}
    c.put32(clubs+0x14,std::uint32_t(o.employer_quote_status));
    c.put32(clubs+0x3c,o.employer_location?employer_location:0);c.put32(clubs+0x248+0x3c,o.buyer_location?buyer_location:0);c.put32(employer_location+0x10,7);c.put32(buyer_location+0x10,o.same_region?7:9);
    if(o.rival_slot)c.put32(clubs+0x70+std::uint32_t(o.rival_slot)*4,clubs+0x248);
    c.put8(division+0x11c,std::uint32_t(o.level));c.put8(division+0x11f,std::uint32_t(o.standing));c.put16(competition+0x18,std::uint32_t(o.standing));return s;
}
void reference(State& s,Model& model){while(s.pc!=return_sentinel&&!is_checkpoint(s.pc)){if(s.pc>=entry&&s.pc<end)require(contract_manager_negotiation_reference(s),"Complete original negotiation reference");else model.call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
void same(const State& a,const State& b,const std::string& label){
    for(unsigned n=0;n<32;++n)require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n)+" "+hex_address(a.r<4>(n*4))+"/"+hex_address(b.r<4>(n*4)));
    for(unsigned n=0;n<32;++n)if(a.pc!=return_sentinel||n==1||n==2||n>=14)require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));
    for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR "+std::to_string(n));for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR"+std::to_string(n-2304)+" "+std::to_string(a.r<1>(n))+"/"+std::to_string(b.r<1>(n)));
    require(a.r<1>(guest::reg::xer_ca)==b.r<1>(guest::reg::xer_ca),label+" carry");require(a.r<4>(4128)==b.r<4>(4128)&&a.pc==b.pc,label+" LR/PC");
    require(a.r<4>(guest::reg::ctr)==b.r<4>(guest::reg::ctr),label+" CTR");
    for(auto [address,size]:a.memory.mapped_ranges()){const auto x=a.memory.copy_out(address,size),y=b.memory.copy_out(address,size);if(x!=y){const auto at=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(address+std::uint32_t(at.first-x.begin()))+" "+std::to_string(*at.first)+"/"+std::to_string(*at.second));}}
}
struct Totals{unsigned cases=0,returned=0,checkpoints=0,interruptions=0,negative=0;std::set<std::uint32_t> calls,boundaries;std::map<std::uint32_t,unsigned> boundary_cases;}totals;
void compare(Options o={}){
    auto source=seed(o),a=clone(*source),b=clone(*source);Model old(o),modern(o);old.o.corrupt_assessment=old.o.corrupt_power=old.o.corrupt_finances=old.o.corrupt_relationship=old.o.corrupt_quality_power=old.o.corrupt_transfer_status=old.o.corrupt_shortlist=old.o.corrupt_first_skill=old.o.corrupt_cap_result=false;
    // Frame unwinding preserves an interrupted guest image; completed calls
    // execute the full original epilogue through the public C++ entry.
    Cpu modern_cpu(*b);
    const auto run=[&](State& s,Model& model,bool original){try{
        if(original)reference(s,model);else{
            const auto previous=contract_manager_call_hook;active_model=&model;contract_manager_call_hook=&hook;
            try{auto& c=modern_cpu;const auto answer=contract_manager::negotiated_player_value(c,c.r(3),c.r(4),c.r(5),c.r(6),c.r(7),c.r(8),c.r(9));c.set(3,answer);c.finish();}
            catch(...){contract_manager_call_hook=previous;active_model=nullptr;throw;}contract_manager_call_hook=previous;active_model=nullptr;
        }return std::string{};
    }catch(const std::exception& e){return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false),label="negotiation case "+std::to_string(totals.cases);
    require(expected.empty()||expected=="Negotiation prefix dependency interrupted",label+" invalid reference "+expected);require(expected==actual,label+" outcomes "+expected+"/"+actual);
    if(o.interrupt)require(expected=="Negotiation prefix dependency interrupted"&&old.calls.size()==o.interrupt,label+" interruption reached");else require(expected.empty()&&(a->pc==return_sentinel||is_checkpoint(a->pc)),label+" declared comparison boundary reached");
    if(o.change_club_on_classification){
        require(old.club_changed&&modern.club_changed,label+" classification changes person's club");
        const auto lookup=std::find_if(old.calls.begin(),old.calls.end(),[](const Event& e){return e.back==0x10086cac;});
        const auto changed=std::find_if(old.calls.begin(),old.calls.end(),[](const Event& e){return e.back==0x10086df0;});
        require(lookup!=old.calls.end()&&lookup->arguments[0]==clubs,label+" prior lookup uses previous club");
        require(changed!=old.calls.end()&&std::int32_t(changed->kept[26-13])<3000000,label+" changed club floor branch reached");
        const auto next_lookup=std::find_if(old.calls.begin(),old.calls.end(),[](const Event& e){return e.back==0x100870f4;});
        require(a->memory.read<4>(person+0x24)==clubs+0x500&&next_lookup!=old.calls.end()&&next_lookup->kept[26-13]==2700000,label+" original rereads changed club reputation before regional lookup");
    }
    if(o.lookup_reputation!=std::numeric_limits<int>::max())require(old.lookup_changed&&modern.lookup_changed,label+" shortlist lookup changes reputation before regional tests");
    if(o.corrupt_first_skill){const auto old_second=std::find_if(old.calls.begin(),old.calls.end(),[](const Event& e){return e.back==0x10088d44;});const auto new_second=std::find_if(modern.calls.begin(),modern.calls.end(),[](const Event& e){return e.back==0x10088d44;});require(old_second!=old.calls.end()&&new_second!=modern.calls.end()&&old_second->kept[18-13]!=new_second->kept[18-13],label+" skill mutation reaches retained first-skill state");}
    if(o.corrupt_cap_result)require(a->pc==return_sentinel&&a->r<4>(3*4)>o.final_cap,label+" final cap mutation is not masked");
    if(o.corrupt)b->memory.write<1>(person,b->memory.read<1>(person)^0x80);bool rejected=false;
    try{same(*a,*b,label);if(old.calls!=modern.calls){const auto at=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());std::string detail;
        if(at.first!=old.calls.end()&&at.second!=modern.calls.end()){
            detail=" targets "+hex_address(at.first->target)+"/"+hex_address(at.second->target)+" backs "+hex_address(at.first->back)+"/"+hex_address(at.second->back);
            for(unsigned n=0;n<std::min(at.first->arguments.size(),at.second->arguments.size());++n)if(at.first->arguments[n]!=at.second->arguments[n])detail+=" arg"+std::to_string(n+3)+" "+hex_address(at.first->arguments[n])+"/"+hex_address(at.second->arguments[n]);
            for(unsigned n=0;n<at.first->kept.size();++n)if(at.first->kept[n]!=at.second->kept[n])detail+=" r"+std::to_string(n+13)+" "+hex_address(at.first->kept[n])+"/"+hex_address(at.second->kept[n]);
            for(unsigned n=0;n<at.first->volatile_arguments.size();++n)if(at.first->volatile_arguments[n]!=at.second->volatile_arguments[n])detail+=" volatile-r"+std::to_string(n+3)+" "+hex_address(at.first->volatile_arguments[n])+"/"+hex_address(at.second->volatile_arguments[n]);
            for(unsigned n=0;n<at.first->conditions.size();++n)if(at.first->conditions[n]!=at.second->conditions[n])detail+=" condition"+std::to_string(n)+" "+std::to_string(at.first->conditions[n])+"/"+std::to_string(at.second->conditions[n]);
            for(unsigned n=0;n<at.first->fp.size();++n)if(at.first->fp[n]!=at.second->fp[n])detail+=" fp-slot"+std::to_string(n);
        }throw std::runtime_error(label+" call "+std::to_string(at.first-old.calls.begin())+detail);
    }}catch(const std::runtime_error&){if(!o.corrupt&&!o.corrupt_assessment&&!o.corrupt_power&&!o.corrupt_finances&&!o.corrupt_relationship&&!o.corrupt_quality_power&&!o.corrupt_transfer_status&&!o.corrupt_shortlist&&!o.corrupt_first_skill&&!o.corrupt_cap_result)throw;rejected=true;}
    require(rejected==(o.corrupt||o.corrupt_assessment||o.corrupt_power||o.corrupt_finances||o.corrupt_relationship||o.corrupt_quality_power||o.corrupt_transfer_status||o.corrupt_shortlist||o.corrupt_first_skill||o.corrupt_cap_result),label+" negative control");++totals.cases;totals.returned+=!rejected&&expected.empty()&&a->pc==return_sentinel;totals.checkpoints+=!rejected&&expected.empty()&&is_checkpoint(a->pc);totals.interruptions+=!expected.empty();totals.negative+=rejected;
    if(!rejected&&expected.empty()){for(const auto& e:old.calls)totals.calls.insert(e.back);if(is_checkpoint(a->pc)){totals.boundaries.insert(a->pc);++totals.boundary_cases[a->pc];}}
}
void interrupt_each_call(Options o) {
    // Derive the reachable call sequence from the independent original,
    // then compare both implementations immediately after each dependency.
    auto state=seed(o);Model model(o);reference(*state,model);
    require(state->pc==return_sentinel,"Interruption schedule reaches original return");
    for(unsigned at=1;at<=model.calls.size();++at){o.interrupt=at;compare(o);}
}
}
int main(){try{
    require(contract_manager_negotiation_stops()==checkpoints,"Original and fixture partial boundaries agree");
    compare();
    for(bool employer:{false,true})for(bool contract_present:{false,true})for(int contract_kind:{0,1,2,31,32,63})for(int bits:{0,1,2,3}){Options o;o.employer=employer;o.contract=contract_present;o.contract_kind=contract_kind;o.contract_kind_bits=bits;compare(o);}
    for(int locked:{-128,-1,0,1,255}){Options o;o.locked=locked;compare(o);}
    for(bool human:{false,true})for(bool national:{false,true})for(bool active:{false,true})for(bool listed:{false,true}){Options o;o.human=human;o.national=national;o.national_active=active;o.listed=listed;compare(o);}
    for(int age:{-128,-1,0,24,28,29,40,127,128,255,280})for(unsigned state:{0u,1u,2u}){Options o;o.age=age;o.fp_state=state;compare(o);}
    for(bool division_present:{false,true})for(bool competition_present:{false,true})for(int level:{-1,0,1,3,4})for(int kind:{0,2,3}){Options o;o.division=division_present;o.competition=competition_present;o.level=level;o.club_kind=kind;compare(o);}
    for(int squad_role:{0,1,2,3,4,5,6,7,8,15,255})for(int mood:{-128,-100,-1,0,50,100,127}){Options o;o.squad_role=squad_role;o.mood=mood;compare(o);}
    for(bool special:{false,true})for(bool present:{false,true})for(bool has_traits:{false,true})for(int trait:{-128,0,10,127}){Options o;o.special=special;o.context=present;o.traits=has_traits;o.manager_trait=trait;compare(o);}
    for(int promise:{-128,-1,0,1,127})for(int terms:{0,1,-1}){Options o;o.promise=promise;o.terms=terms;compare(o);}
    for(int ability:{-32768,-1,0,100,32767})for(int reputation:{-32768,-1,0,6000,32767}){Options o;o.ability=ability;o.reputation=reputation;compare(o);}
    Options zero;zero.base_value=0;compare(zero);
    for(unsigned at:{1u,2u,3u,4u,5u}){Options o;o.interrupt=at;compare(o);}
    Options all;all.human=true;all.listed=true;all.special=true;all.age=40;for(unsigned at=1;at<=17;++at){all.interrupt=at;compare(all);}
    for(unsigned tenure:{0u,1u,128u,256u})for(bool protected_player:{false,true})for(bool diagnostics:{false,true})for(int elapsed:{-1,0,120,121,122,364,365,366,1000}){Options o;o.tenure=tenure;o.protected_player=protected_player;o.diagnostics=diagnostics;o.elapsed=elapsed;compare(o);}
    for(bool finance:{false,true})for(std::int64_t balance:{-100000000ll,-1ll,0ll,50000ll,1000000ll,2000000ll,100000000ll})for(int status:{0,15,16,127,240,255}){Options o;o.finance=finance;o.balance=balance;o.status=status;compare(o);}
    for(int mood:{-128,-1,0,20,21,49,50,60,61,100,127})for(int score:{0,499,500,999,10000}){Options o;o.mood=mood;o.club_score=score;compare(o);}
    for(std::int64_t balance:{-4294967297ll,4294967297ll,9007199254740991ll,9007199254740993ll,std::numeric_limits<std::int64_t>::min(),std::numeric_limits<std::int64_t>::max()}){Options o;o.balance=balance;compare(o);}
    for(int joined:{0,2000,2001,2002,65535})for(int current:{0,2000,2001,2002,65535}){Options o;o.joined_year=joined;o.current_year=current;compare(o);}
    for(int flags:{0,0xffff,0xfeff,0xfdff,0xfbff})for(int mood:{20,21,60,61})for(int elapsed:{0,365}){Options o;o.diagnostic_flags=flags;o.mood=mood;o.elapsed=elapsed;compare(o);}
    for(bool buyer:{false,true})for(int employer_id:{0,6,7})for(int buyer_id:{1,6,7})for(int mood:{-100,20,21,60})for(int terms:{0,1}){Options o;o.buyer=buyer;o.employer_id=employer_id;o.buyer_id=buyer_id;o.mood=mood;o.terms=terms;o.diagnostic_flags=0;compare(o);}
    for(int relationship:{0,1,128,256})for(int rival:{0,1,2,3})for(bool employer_region:{false,true})for(bool buyer_region:{false,true})for(bool same_region:{false,true}){Options o;o.relationship=relationship;o.rival_slot=rival;o.employer_location=employer_region;o.buyer_location=buyer_region;o.same_region=same_region;o.diagnostic_flags=0;compare(o);}
    for(bool buyer_competition:{false,true})for(bool same:{false,true})for(bool excluded:{false,true})for(int mood:{-1,0,20,100}){Options o;o.buyer_competition=buyer_competition;o.same_competition=same;o.excluded_competition=excluded;o.mood=mood;compare(o);}
    for(int suitability:{-32768,0,84,85,86,100,32767})for(int age:{-128,24,25,128})for(int squad_role:{0,4,5,6,7,15})for(bool diagnostics:{false,true}){Options o;o.suitability=suitability;o.age=age;o.squad_role=squad_role;o.diagnostics=diagnostics;o.diagnostic_flags=0;compare(o);}
    for(int score:{0,49,50,499,500,999,10000})for(int suitability:{0,1,100,32767}){Options o;o.club_score=score;o.suitability=suitability;o.diagnostics=false;compare(o);}
    for(int score:{20998,20999,21000,32767}){Options o;o.club_score=score;o.suitability=32767;o.diagnostics=false;compare(o);}
    for(int first:{-32768,0,5000,10000,32767})for(int second:{-32768,0,6000,10000,32767})for(int potential:{-32768,0,7000,10000,32767}){Options o;o.first_score=first;o.reputation=second;o.potential_score=potential;compare(o);}
    for(std::uint32_t base:{1u,999u,1000u,1000000u,1000000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(int mood:{-128,0,50,127})for(int squad_role:{0,1,4,7}){Options o;o.base_value=base;o.mood=mood;o.squad_role=squad_role;compare(o);}
    for(bool transfer:{false,true})for(int adjustment:{0,1,128,256})for(int protection:{0,1,128,256}){Options o;o.transfer_status=transfer;o.adjustment_protection=adjustment;o.price_protection=protection;compare(o);}
    for(bool present:{false,true})for(int status:{-128,-1,0,1,256})for(int squad_role:{0,1,2,3,4,5,6,7,15})for(bool diagnostics:{false,true}){Options o;o.secondary_contract=present;o.secondary_status=status;o.squad_role=squad_role;o.diagnostics=diagnostics;compare(o);}
    for(bool human:{false,true})for(int first:{-2,-1,0,1})for(int next:{-2,-1,0,1})for(int flags:{0,0x800,0xffff}){Options o;o.human=human;o.listed=true;o.first_finance_class=first;o.next_finance_class=next;o.diagnostic_flags=flags;compare(o);}
    for(int terms:{0,1,2,4,8,0x10,0x20,0x40,0x80,0x100,0x200,-1})for(int days:{-365,-1,0,1,120,180,364,365,366,730}){Options o;o.terms=terms;o.expiry_days=days;compare(o);}
    for(std::uint32_t value:{0u,1u,1000u,777777u,0x7fffffffu,0x80000000u,0xffffffffu})for(int protection:{0,1}){Options o;o.stored_value=value;o.price_protection=protection;compare(o);}
    for(int reputation:{-32768,0,4249,4250,4251,10000,32767}){Options o;o.separate_person_club=true;o.person_club_score=reputation;compare(o);}
    for(bool present:{false,true})for(bool secondary:{false,true}){Options o;o.person_club=present;o.secondary_contract=secondary;o.diagnostics=false;compare(o);}
    Options reread;reread.change_club_on_classification=true;reread.base_value=10000;reread.first_score=reread.reputation=reread.potential_score=5000;reread.person_club_score=2000;reread.diagnostic_flags=0x800;compare(reread);
    for(unsigned at:{6u,7u,8u,9u,10u}){Options o;o.mood=60;o.interrupt=at;compare(o);}
    Options mutation;mutation.corrupt=true;compare(mutation);mutation.corrupt=false;mutation.corrupt_assessment=true;compare(mutation);mutation.corrupt_assessment=false;mutation.corrupt_power=true;mutation.age=40;compare(mutation);
    mutation.corrupt_power=false;mutation.corrupt_finances=true;compare(mutation);
    mutation.corrupt_finances=false;mutation.corrupt_relationship=true;compare(mutation);
    Options quality;quality.first_score=quality.reputation=quality.potential_score=5000;quality.corrupt_quality_power=true;compare(quality);quality.corrupt_quality_power=false;quality.interrupt=11;compare(quality);
    Options transfer_mutation;transfer_mutation.corrupt_transfer_status=true;compare(transfer_mutation);
    interrupt_each_call({});Options protected_path;protected_path.price_protection=1;interrupt_each_call(protected_path);Options secondary_path;secondary_path.squad_role=4;interrupt_each_call(secondary_path);
    for(int country:{1,100,101,102,103,104,105,106,107,108,999})for(int reputation:{4000,4001,6999,7000})for(int standing:{12,13}){Options o;o.regional_country=country;o.club_score=reputation;o.standing=standing;compare(o);}
    for(bool listed:{false,true})for(bool matches:{false,true})for(int person_status:{0,2,255})for(int club_status:{-2,-1,0,1}){Options o;o.shortlist=listed;o.shortlist_matches=matches;o.person_quote_status=person_status;o.employer_quote_status=club_status;o.club_score=8000;o.standing=13;compare(o);}
    for(int first_class:{-2,-1,0,255,256})for(int next_class:{-2,-1,0,254,256}){Options o;o.regional_first_class=first_class;o.regional_next_class=next_class;o.club_score=8000;o.standing=13;compare(o);}
    for(int first:{0,1,128,256})for(int second:{0,1,128,256}){Options o;o.regional_first_predicate=first;o.regional_second_predicate=second;o.club_score=8000;o.standing=13;compare(o);}
    for(int age:{-128,19,20,24,25})for(int suitability:{160,161,1000,32767})for(int promise:{-128,-5,-4,-1,0,1,4,127}){Options o;o.age=age;o.suitability=suitability;o.promise=promise;o.club_score=8000;o.standing=13;compare(o);}
    for(int squad_role:{0,1,2,3,4,6,7,15})for(bool buyer:{false,true})for(bool competition:{false,true})for(bool same:{false,true}){Options o;o.squad_role=squad_role;o.buyer=buyer;o.competition=competition;o.same_competition=same;o.club_score=8000;o.standing=13;compare(o);}
    for(int reputation:{6750,6751,7000})for(int standing:{13,15,16})for(int promise:{-51,-50,-49})for(int terms:{0,1}){Options o;o.club_score=reputation;o.standing=standing;o.promise=promise;o.terms=terms;compare(o);}
    Options young;young.club_score=8000;young.standing=13;young.suitability=32767;young.promise=-5;interrupt_each_call(young);
    for(std::uint32_t base:{1u,1000u,1000000u,1000000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(int squad_role:{1,2,3})for(bool same:{false,true}){Options o;o.base_value=base;o.squad_role=squad_role;o.same_competition=same;o.club_score=8000;o.standing=13;o.age=25;compare(o);}
    for(int reputation:{4000,4001,6999,7000,8000}){Options o;o.lookup_reputation=reputation;o.standing=13;compare(o);}
    Options shortlist_mutation;shortlist_mutation.club_score=8000;shortlist_mutation.standing=13;shortlist_mutation.corrupt_shortlist=true;compare(shortlist_mutation);
    for(int terms:{1,4,0x100,0x101})for(int days:{224,225,226,424,425,426,730})for(int squad_role:{0,1,2,3,7}){Options o;o.terms=terms;o.expiry_days=days;o.squad_role=squad_role;o.promise=-50;o.club_score=8000;o.standing=13;compare(o);}
    for(int first:{-128,5,6,127})for(int second:{-128,5,6,127})for(int mood:{-128,-6,-5,0,127})for(int age:{22,23,30,31}){Options o;o.first_profile_trait=first;o.second_profile_trait=second;o.mood=mood;o.age=age;o.promise=-50;o.terms=4;o.reputation=6251;o.club_score=8000;o.standing=13;compare(o);}
    for(int reputation:{6250,6251})for(bool shared:{false,true})for(int relationship:{0,1,128,256}){Options o;o.reputation=reputation;o.same_competition=shared;o.relationship=relationship;o.promise=-50;o.terms=4;o.club_score=8000;o.standing=13;compare(o);}
    Options profile;profile.reputation=6251;profile.promise=-50;profile.terms=4;profile.club_score=8000;profile.standing=13;profile.relationship=1;interrupt_each_call(profile);
    for(int squad_role:{0,1,2,3,4,7})for(int promise:{-1,0,1})for(int terms:{0,1,4,0x100})for(bool same:{false,true}){Options o;o.squad_role=squad_role;o.promise=promise;o.terms=terms;o.same_competition=same;o.club_score=6750;o.standing=13;o.expiry_days=426;compare(o);}
    for(bool competition:{false,true})for(bool buyer:{false,true})for(int squad_role:{1,2,3}){Options o;o.competition=competition;o.buyer=buyer;o.squad_role=squad_role;o.club_score=6750;o.standing=13;compare(o);}
    for(int days:{424,425,426})for(int squad_role:{1,2,3}){Options o;o.squad_role=squad_role;o.promise=0;o.terms=1;o.club_score=6750;o.standing=13;o.expiry_days=days;compare(o);}
    Options lower;lower.club_score=6750;lower.standing=13;lower.terms=1;lower.promise=0;lower.expiry_days=426;lower.squad_role=1;interrupt_each_call(lower);
    for(std::uint32_t base:{1u,1000000u,1000000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(int squad_role:{1,2,3})for(int terms:{0,1})for(bool same:{false,true}){Options o;o.base_value=base;o.squad_role=squad_role;o.terms=terms;o.same_competition=same;o.club_score=6750;o.standing=13;o.expiry_days=426;compare(o);}
    for(int age:{-128,19,20,21,22})for(int suitability:{160,161,32767})for(int promise:{-128,-5,-4,-1,0,1,4,127}){Options o;o.shortlist_empty=true;o.age=age;o.suitability=suitability;o.promise=promise;o.club_score=8000;o.standing=13;compare(o);}
    for(int terms:{0,1,4})for(int promise:{-1,0,1})for(int squad_role:{0,1,2,3,4,6,7})for(int reputation:{6750,8000})for(bool same:{false,true})for(bool buyer:{false,true}){Options o;o.shortlist_empty=true;o.terms=terms;o.promise=promise;o.squad_role=squad_role;o.club_score=reputation;o.same_competition=same;o.buyer=buyer;o.standing=13;o.expiry_days=426;compare(o);}
    for(int first:{-128,5,6,10,127})for(int second:{-128,5,6,10,127})for(int mood:{-128,-1,0,5,6,127})for(int age:{22,23,30,31}){Options o;o.shortlist_empty=true;o.first_profile_trait=first;o.second_profile_trait=second;o.mood=mood;o.age=age;o.promise=0;o.terms=4;o.reputation=5251;o.club_score=8000;o.standing=13;compare(o);}
    for(int trait:{-128,-1,0,5,40,127})for(int mood:{-128,-1,0,39,40,127})for(int squad_role:{0,2,3,4,7}){Options o;o.shortlist_empty=true;o.second_profile_trait=trait;o.mood=mood;o.squad_role=squad_role;o.club_score=6750;o.standing=13;compare(o);}
    for(int mood:{-128,-1,0,59,60,61,127})for(std::uint32_t base:{1u,1000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(int reputation:{4000,8000}){Options o;o.shortlist=false;o.mood=mood;o.base_value=base;o.club_score=reputation;o.standing=13;compare(o);}
    for(int days:{224,225,226,424,425,426})for(int reputation:{5250,5251}){Options o;o.shortlist_empty=true;o.terms=1;o.promise=0;o.expiry_days=days;o.reputation=reputation;o.club_score=8000;o.standing=13;compare(o);}
    Options ordinary_young;ordinary_young.shortlist_empty=true;ordinary_young.age=21;ordinary_young.suitability=32767;ordinary_young.club_score=8000;ordinary_young.standing=13;interrupt_each_call(ordinary_young);
    Options ordinary_profile;ordinary_profile.shortlist_empty=true;ordinary_profile.promise=0;ordinary_profile.terms=1;ordinary_profile.expiry_days=365;ordinary_profile.reputation=5251;ordinary_profile.club_score=8000;ordinary_profile.standing=13;interrupt_each_call(ordinary_profile);
    for(std::uint32_t base:{1u,1000000u,1000000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(int reputation:{6750,8000})for(int squad_role:{1,2,3})for(int terms:{0,1})for(bool same:{false,true}){Options o;o.shortlist_empty=true;o.base_value=base;o.club_score=reputation;o.squad_role=squad_role;o.terms=terms;o.same_competition=same;o.standing=13;o.expiry_days=426;compare(o);}
    for(int age:{18,23,24})for(unsigned option:{0u,1u,128u,256u})for(int suitability:{110,111,120,121,150,220,32767}){Options o;o.age=age;o.option=option;o.suitability=suitability;compare(o);}
    for(int age:{18,23})for(int contract_kind:{14,15,25,26,34,35,40,128})for(int profile:{0,7,10,20})for(int first:{15,20,127})for(int second:{15,20,127}){Options o;o.age=age;o.first_role=contract_kind;o.second_role=0;o.profile_attribute=profile;o.first_skill=first;o.second_skill=second;o.suitability=180;compare(o);}
    for(unsigned index=0;index<5;++index)for(int value:{-128,-1,0,20,127}){Options o;o.profile_values[index]=value;o.age=21;o.suitability=180;compare(o);}
    for(bool manager:{false,true})for(bool traits:{false,true})for(bool human:{false,true})for(int year:{2000,2001,2002})for(int day:{99,100,101}){Options o;o.manager=manager;o.traits=traits;o.human=human;o.listed=true;o.manager_year=year;o.manager_day=day;o.age=21;o.suitability=111;o.first_role=o.second_role=0;o.profile_attribute=0;o.ability=200;compare(o);}
    for(int reputation:{0,2499,2500,3999,4000,5249,5250,5251,10000})for(int level:{4,5,10,11,12,13})for(int kind:{0,3}){Options o;o.club_score=reputation;o.standing=level;o.club_kind=kind;o.age=21;o.suitability=220;o.terms=1;o.contract_policy=8;compare(o);}
    for(int terms:{0,1,0x40,0x800000,0x800041})for(int promise:{-51,-50,-1,0,1})for(int trait:{5,6,15,16}){Options o;o.terms=terms;o.promise=promise;o.second_profile_trait=trait;o.first_profile_trait=9;o.age=21;o.suitability=180;o.buyer_reputation=5751;o.contract_policy=8;compare(o);}
    for(int goalkeeper:{0,20})for(int age:{-128,24,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,127,128,255}){Options o;o.goalkeeper=goalkeeper;o.age=age;o.regional_country=1;o.base_value=1000000;compare(o);}
    for(int active:{0,1,128,256})for(std::uint32_t limit:{0u,1u,1000u,1000000u,50000000u,0x80000000u,0xffffffffu}){Options o;o.final_cap_active=active;o.final_cap=limit;compare(o);}
    Options floor_path;floor_path.age=21;floor_path.suitability=180;floor_path.terms=0x800041;floor_path.contract_policy=8;interrupt_each_call(floor_path);
    floor_path.interrupt=0;floor_path.corrupt_first_skill=true;compare(floor_path);Options cap_mutation;cap_mutation.corrupt_cap_result=true;cap_mutation.final_cap=1000;compare(cap_mutation);
    const auto calls=contract_manager_negotiation_calls();unsigned wanted=0;for(auto back:calls)if(back>entry&&back<implemented_end&&back!=0x1008586c&&back!=0x10089a30){++wanted;require(totals.calls.contains(back),"Unreached negotiation semantic continuation "+hex_address(back));}
    for(auto boundary:checkpoints)require(totals.boundaries.contains(boundary),"Unreached declared partial boundary "+hex_address(boundary));
    std::cout<<totals.cases<<" whole-body comparisons; "<<totals.returned<<" complete returns; "<<totals.checkpoints<<" partial checkpoints; "<<totals.interruptions<<" explicit dependency interruptions; "<<totals.negative<<" negative controls detected; "<<totals.calls.size()<<"/"<<wanted<<" semantic continuations. Scripted dependencies are not proofs of their implementations.\n";
    for(auto [boundary,cases]:totals.boundary_cases)std::cout<<"partial boundary "<<hex_address(boundary)<<": "<<cases<<" cases\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
