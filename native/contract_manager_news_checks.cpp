#include "contract_manager_detail.hpp"
#include "contract_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <map>
#include <set>

namespace cm::aot {
bool contract_manager_news_reference(State&);
std::vector<std::uint32_t> contract_manager_news_calls();
void contract_manager_news_constants(State&, std::uint32_t, std::uint32_t);
void contract_manager_offer_constants(State&, std::uint32_t, std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr std::uint32_t message=fixture+0x100, proposal=fixture+0x200, globals=fixture+0x1000;
constexpr std::uint32_t clubs=fixture+0x4000, other_clubs=fixture+0x6000, staff=fixture+0x8000;
constexpr std::uint32_t competitions=fixture+0xa000, divisions=fixture+0xb000, nodes=fixture+0xc000;
constexpr std::uint32_t buffer=fixture+0x10000, output=fixture+0x11000, pool=fixture+0x14000;
constexpr std::uint32_t table=fixture+0x18000, registry=fixture+0x19000;
constexpr std::uint32_t manager=fixture+0x1d000,rows=fixture+0x1d100,indices=fixture+0x1d500;
constexpr std::uint32_t own_manager=fixture+0x1d040,own_rows=fixture+0x1e000,ui=fixture+0x20000,ui_manager=fixture+0x24000,pending=fixture+0x30000,pending_nodes=fixture+0x31000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,20> ranges{{
    {0x1008a350,0x1008ab40},
    {0x1008ab40,0x1008bfc0},
    {0x1008cd20,0x1008d450},
    {0x1008d450,0x1008e350},
    {0x1008e350,0x1008ec00},
    {0x1008ec00,0x1008f620},
    {0x1008c090,0x1008c270},{0x1008c270,0x1008c4c0},{0x1008c4c0,0x1008c8d0},{0x1008ca50,0x1008cd20},
    {0x1008f7b0,0x1008fa40},{0x1008fa40,0x1008fcf0},{0x1008fcf0,0x10090310},
    {0x100913f0,0x10091630},{0x10091630,0x100918b0},{0x100918b0,0x10091bf0},
    {0x10090310,0x10090560},{0x10090560,0x10090a00},{0x10090a00,0x10090f20},{0x10090f20,0x10091320}}};
struct Options {
    std::uint32_t entry=0x1008c4c0,mode=1,period=1,bosman=1,player=1,related=3,spread=1;
    std::uint32_t current_eligible=1,other_eligible=1,name_available=1;
    std::uint32_t request_flags=0,playing=1,changed=1,before=0,after=1,confused=1;
    std::uint32_t guard_first=0,guard_second=0,guard_third=0,record_id=2,person_count=3,index_row=0;
    int contract_flags=8,contract_status=0,age=25;
    std::uint32_t request_role=2,validation=0,agent=1,day=100,year=2002,wage=1000;
    std::uint32_t current_day=100,current_year=2002,current_wage=1000,club_id=1,lender_id=3,current_club=1;
    std::uint32_t departure_kind=2,important=0,nation_scope=4,selected_context=0,pending_kind=1;
    int first_trait=0,second_trait=0,profile_c=5000,profile_a=6000;
    bool context_present=true,context_club=true,context_matches=true,shared_scopes=false,mutate_departure=false;
    int first_gender=-1,second_gender=2;
    bool contract=true,division=true,empty_list=false,mutate_lookup=false,corrupt=false,corrupt_period=false,corrupt_extra=false;
    bool mutate_contract=false,corrupt_guard=false,corrupt_loan_spill=false,corrupt_date=false,corrupt_ui=false;
    std::uint32_t offer_notify=0,offer_outcome=2,offer_reason=0,reason_known=0,offer_origin=0xffffffff;
    std::uint32_t offer_days=180,offer_age=30,recipient_count=3,random_salt=7,random_word=10,index_flags=0xab;
    std::uint32_t cutoff_day=100,cutoff_year=2002,later_day=200,later_year=2002;
    int keeper=20,defender=15,midfielder=15,attacker=15;
    bool offer_primary=true,origin_present=true,club_manager=true,random_wrap=false,corrupt_random=false,corrupt_index_bit=false;
    std::uint32_t offer_person=2,offer_kind=2,offer_flags=8,offer_manager=1,offer_timing=0,offer_marker=0x420e9,offer_variation=0;
    std::uint32_t offer_renewal=1,offer_bonus=200000,offer_start_day=80,offer_start_year=2001,offer_career=0,offer_previous=0,fp_pattern=0;
    bool corrupt_offer_pair=false,corrupt_offer_spill=false,enable_inexact_exception=false;
    int short_gender=127;
    std::uint32_t unrest_human=0,unrest_listed=0,unrest_quote=1000000,unrest_fee=900000,unrest_position=2;
    std::uint32_t unrest_wrap=0,unrest_human_mask=0,unrest_account_mask=0xfff,unrest_reputations=0,unrest_random_stride=3;
    std::vector<std::uint32_t> unrest_candidates{1,2,3};
    double unrest_balance=2000000;
    bool unrest_club=true,unrest_mutate_table=false,unrest_mutate_candidates=false;
    bool corrupt_unrest_finance=false,corrupt_unrest_sort=false,corrupt_unrest_spill=false;
    bool unrest_render_mutate_staff=false;
    std::uint32_t unrest_variation=0,unrest_not_listed=1,unrest_gender=127;
    int nation_gender=-128,nationality_gender=127;
    std::uint32_t request_registered=1,request_value=1000000,request_quote=1500000,request_status=0;
    int request_reaction=0,request_refined=7;
    unsigned request_force_sampler=0;std::int32_t request_bound=70000;
    bool request_nation=true,request_mutate_reaction=false,request_clear_profile=false,corrupt_request_status=false,corrupt_request_quote=false;
    int request_reputation=6000;
    bool corrupt_request_render_spill=false,request_render_mutate_tables=false;
    unsigned interrupt=0;
};
struct Event {
    std::uint32_t target,back,extra;
    std::array<std::uint32_t,32> gpr;
    std::array<std::uint64_t,32> fpr;
    std::array<std::uint8_t,11> conditions;
    std::uint32_t ctr;
    std::array<std::uint32_t,6> stack_arguments;
    std::vector<std::uint8_t> emitted_text;
    bool operator==(const Event&) const=default;
};
struct Model {
    Options o;std::vector<Event> events;std::uint32_t current_row=0;bool mutated=false,bound_injected=false;
    explicit Model(Options value):o(value){}
    void call(State& s,std::uint32_t target,std::uint32_t back) {
        Cpu c(s);
        if(o.corrupt_extra&&target==0x10727890&&c.r(4)==pool+0x3190)c.put32(c.sp()+0x38,c.u32(c.sp()+0x38)^4);
        if(o.corrupt_loan_spill&&target==0x10727890&&c.r(4)==pool+0x6ff6)c.put32(c.sp()+0x44,c.u32(c.sp()+0x44)^4);
        if(o.corrupt_index_bit&&target==0x102cb4b0&&c.r(4)==27)c.set(5,o.index_flags!=0);
        if(o.corrupt_offer_spill&&target==0x10727890&&c.r(4)==pool+0x15d6)c.put32(c.sp()+0x44,c.u32(c.sp()+0x44)^4);
        if(o.corrupt_unrest_spill&&target==0x10727890&&c.r(4)==pool+0x410c)c.put32(c.sp()+0x4c,c.u32(c.sp()+0x4c)^4);
        if(o.corrupt_request_render_spill&&target==0x10727890&&(c.r(4)==pool+0x4f4d||c.r(4)==pool+0x51bf))c.put32(c.sp()+0x38,c.u32(c.sp()+0x38)^4);
        Event event{};event.target=target;event.back=back;event.ctr=std::uint32_t(s.r<4>(guest::reg::ctr));
        for(unsigned n=0;n<32;++n){event.gpr[n]=c.r(n);event.fpr[n]=s.r<8>(guest::reg::fpr(n));}
        for(unsigned n=0;n<8;++n)event.conditions[n]=c.cr(n);
        event.conditions[8]=std::uint8_t(s.r<1>(guest::reg::xer_ca));event.conditions[9]=std::uint8_t(s.r<1>(guest::reg::xer_so));event.conditions[10]=std::uint8_t(s.r<1>(guest::reg::xer_ov));
        if(target==0x10727890){unsigned words=0;
            if(c.r(4)==pool+0x3190||c.r(4)==pool+0x5507||c.r(4)==pool+0x4f4d||c.r(4)==pool+0x51bf)words=1;
            if(c.r(4)==pool+0x6eff||c.r(4)==pool+0x70ca)words=2;
            if(c.r(4)==pool+0x6ff6)words=4;
            if(c.r(4)==pool+0xa00||c.r(4)==pool+0xafc||c.r(4)==pool+0xfe3||c.r(4)==pool+0x112b)words=1;
            if(c.r(4)==pool+0x14b7)words=3;
            if(c.r(4)==pool+0x15d6)words=4;
            if(o.entry==0x1008d450){switch(c.r(4)-pool){
            case 0x3706:case 0x384f:case 0x3e61:words=4;break;
            case 0x39a1:case 0x3a82:case 0x3b4c:case 0x3c36:case 0x46e1:words=2;break;
            case 0x3d9c:words=1;break;
            case 0x4008:case 0x4599:words=5;break;
            case 0x410c:words=6;break;
            case 0x4231:case 0x430d:case 0x4415:words=3;break;
            default:break;}}
            for(unsigned n=0;n<words;++n)event.stack_arguments[n]=c.u32(c.sp()+0x38+n*4);
            if(words)event.extra=event.stack_arguments[0];}
        if(target==0x1074b830||target==0x1074b960)event.emitted_text=s.memory.copy_out(c.r(4),32);
        events.push_back(event);
        std::uint32_t answer=0;
        switch(target) {
        case 0x102cb570: {
            std::array<std::uint32_t,28> parameters{1,11,2,13,17,19,
                o.entry==0x1008ca50?o.related:o.period,o.bosman,o.player,0,0,0};
            if(o.entry==0x1008c090||o.entry==0x100913f0)parameters={2,11,13,17,1,0,0,0,0,0,0,0};
            if(o.entry==0x100918b0)parameters={2,11,13,17,1,3,o.related,0,0,0,0,0};
            if(o.entry==0x1008f7b0){parameters[6]=o.request_flags;parameters[7]=o.playing;}
            if(o.entry==0x1008fcf0){parameters[6]=o.changed;parameters[7]=o.period;parameters[8]=o.before;parameters[9]=o.after;parameters[10]=o.confused;parameters[11]=std::uint32_t(o.age);}
            if(o.entry==0x10090560)parameters={o.club_id,0,2,13,17,19,o.request_role,o.validation,o.day,o.year,o.wage,o.agent};
            if(o.entry==0x10090f20)parameters={o.club_id,0,2,13,17,19,o.departure_kind,o.lender_id,o.important,0,0,0};
            if(o.entry==0x1008ab40)parameters={o.offer_person,13,17,19,o.club_id,o.offer_outcome,o.day,o.year,0,0,o.offer_kind,0,o.offer_flags,o.offer_manager,11,13,17,o.offer_timing,o.offer_marker,o.offer_variation,o.offer_renewal,o.offer_reason,o.wage,o.offer_bonus,o.offer_start_day,o.offer_start_year,o.offer_career,o.offer_previous};
            if(o.entry==0x1008ec00)parameters={o.club_id,0,o.offer_person,13,17,19,o.offer_flags,o.changed,o.validation,o.agent,o.nation_scope,o.important,o.request_status};
            if(o.entry==0x1008d450){
                parameters={o.club_id,o.offer_person,13,17,19,o.offer_manager,11,13,17,o.offer_flags,o.request_role,o.unrest_position,o.unrest_variation,o.unrest_fee,std::uint32_t(o.unrest_candidates.size())};
                require(o.unrest_candidates.size()<=3,"Publisher caps renderer candidate count at three");
                for(unsigned n=0;n<o.unrest_candidates.size();++n)parameters[15+n]=o.unrest_candidates[n];
                parameters[15+o.unrest_candidates.size()]=o.unrest_not_listed;
            }
            if(o.corrupt_ui&&o.entry==0x10090f20)parameters[8]+=256;
            require(c.r(4)<parameters.size(),"Original news parameter index");
            answer=parameters[c.r(4)]+(o.corrupt_period&&c.r(4)==6?1:0);break;
        }
        case 0x100cb590:
            c.put32(c.r(6),0x4e414d45);c.put32(c.r(6)+4,c.r(9));c.put32(c.r(6)+8,c.r(8));break;
        case 0x100cbde0:
            answer=o.name_available;
            if(o.unrest_render_mutate_staff&&!mutated){c.put32(globals+4,fixture+0x34000);mutated=true;}break;
        case 0x100d6300:
            c.put32(c.r(4),0x504f5349);c.put32(c.r(4)+4,c.r(3));c.put32(c.r(4)+8,c.r(6));answer=o.unrest_gender;break;
        case 0x10727890:case 0x1077b720: {
            // An intentionally observable formatting stub: every supplied
            // register and all supplied stack variadic words influence its output.
            std::uint32_t hash=0x811c9dc5;
            for(unsigned n=4;n<11;++n)hash=(hash^c.r(n))*0x1000193;
            for(auto word:event.stack_arguments)hash=(hash^word)*0x1000193;c.put32(c.r(3),hash);c.put32(c.r(3)+4,c.r(4));break;
        }
        case 0x1074b830:case 0x1074b960:
            c.put32(c.r(3),c.u32(c.r(3))+1);s.memory.copy_in(c.r(3)+0x10,s.memory.copy_out(c.r(4),32));
            if(o.entry==0x1008ec00&&o.request_render_mutate_tables&&!mutated){c.put32(globals,other_clubs);c.put32(globals+4,fixture+0x34000);mutated=true;}
            if(o.entry==0x1008d450&&o.unrest_mutate_table&&!mutated){c.put32(globals,other_clubs);mutated=true;}break;
        case 0x1074bab0: {
            const auto count=c.u32(c.r(3)+4);const auto at=c.r(3)+0x80+count*12;
            c.put32(at,c.r(4));c.put32(at+4,c.r(5));c.put32(at+8,c.r(6));c.put32(c.r(3)+4,count+1);break;
        }
        case 0x102bdf90:
            s.memory.copy_in(c.r(3),std::vector<std::uint8_t>(0xd0,0));c.put32(c.r(3),c.r(4));break;
        case 0x102cb4b0:
            require(c.r(4)<=(o.entry==0x1008a350?27u:o.entry==0x1008cd20?18u:o.entry==0x1008e350?12u:11u),"Original news field index");c.put32(c.r(3)+0x20+c.r(4)*4,c.r(5));
            if(o.entry==0x1008e350&&o.request_clear_profile&&c.r(4)==8){c.put32(staff+2*112+0x14,0);mutated=true;}
            if(o.entry==0x1008cd20&&c.r(4)==14&&!mutated&&(o.unrest_mutate_candidates||o.corrupt_unrest_sort)){
                const auto first=c.sp()+0x48+(o.corrupt_unrest_sort?4:0),second=first+4;const auto a=c.u32(first),b=c.u32(second);c.put32(first,b);c.put32(second,a);mutated=true;}
            if(o.mutate_contract&&o.entry==0x10091630){
                if(c.r(4)==4)c.put32(indices+o.record_id*100,1);
                if(c.r(4)==6)c.put32(indices+o.record_id*100,2);
                mutated=true;
            }
            if(o.mutate_departure&&o.entry==0x10090a00&&c.r(4)==6){c.put32(indices+o.record_id*100,1);mutated=true;}
            break;
        case 0x102cc140:case 0x102cc400:case 0x102cc8e0: {
            const auto count=c.u32(globals+0x200);const auto at=globals+0x300+count*0xe0;
            c.put32(at,target==0x102cc8e0?0xffffffffu:c.r(5));s.memory.copy_in(at+4,s.memory.copy_out(c.r(4),0xd0));c.put32(at+0xd4,target);c.put32(globals+0x200,count+1);break;
        }
        case 0x100d33d0:
            answer=back==0x1008c3d8?o.current_eligible:o.other_eligible;
            if(o.mutate_lookup&&back==0x1008c460){require(current_row!=0,"Active shortlist row");c.put32(current_row+4,c.u32(current_row+4)==0?3:0);c.put32(globals,other_clubs);mutated=true;}break;
        case 0x10738c00:c.put32(c.r(3),0);c.put32(c.r(3)+4,0);break;
        case 0x10362ef0:c.put32(c.r(5)+4,o.empty_list?0:nodes);break;
        case 0x10738e00: {
            if(c.r(3)==pending){answer=c.u32(pending+0xc);if(answer)c.put32(pending+0xc,answer==pending_nodes+32?0:answer+16);}
            else{const auto index=c.u32(c.r(3));const auto count=o.entry==0x1008cd20?o.unrest_candidates.size():4u;if(!o.empty_list&&index<count){answer=nodes+index*16;c.put32(c.r(3),index+1);}}
            current_row=answer;break;
        }
        case 0x10738c20:c.put32(c.r(3),0);c.put32(c.r(3)+4,0);break;
        case 0x10739890:c.put32(c.r(6),0x46494c45);c.put32(c.r(7),0x45585400);break;
        case 0x10768810:answer=pool+0x100;break;
        case 0x10769b60:break;
        case 0x103e6470:answer=o.guard_first+(o.corrupt_guard?1:0);break;
        case 0x103eaea0:answer=o.guard_second;break;
        case 0x103eb020:answer=o.guard_third;break;
        case 0x107156b0:
            answer=c.r(3);c.put16(answer,c.r(4)+(o.corrupt_date?1:0));c.put16(answer+2,c.r(5));c.put8(answer+4,c.r(5)%4==0);break;
        case 0x102cd0c0:
            require(c.r(4)!=0&&c.r(5)==c.u32(c.r(8)+0xdc)&&c.r(6)==7&&c.r(7)==1,"Valid direct request-flag update");
            c.put32(c.r(8)+8+c.r(6)*4,c.r(7));break;
        case 0x10082f90:answer=o.offer_primary?rows:0;break;
        case 0x100c7a20:answer=o.reason_known;break;
        case 0x103eb440:answer=globals+0x820;break;
        case 0x10369080:c.put32(c.r(9),0x12345);break;
        case 0x1077b770:answer=0x1234abcd+(o.corrupt_random?1u:0u);break;
        case 0x100c6a10:answer=o.offer_age;break;
        case 0x10374d10:answer=o.offer_days;break;
        case 0x103fc710:
            for(unsigned n=0;n<30;++n)c.put32(c.r(6)+n*4,clubs+(n%4)*0x248);
            answer=o.recipient_count;break;
        case 0x10009f20:answer=0x12345678;break;
        case 0x1000b410:
            c.put32(c.r(3),c.r(5));c.put32(c.r(3)+4,c.r(6));c.put32(c.r(3)+8,c.r(7));break;
        case 0x10716eb0:
            c.put32(c.r(4),c.u32(c.r(3)));c.put32(c.r(4)+4,0x44415445);break;
        case 0x10117490:answer=o.unrest_position;break;
        case 0x100d3040:answer=o.unrest_listed;break;
        case 0x10085860:answer=o.entry==0x1008e350?o.request_quote+(o.corrupt_request_quote?1u:0u):o.unrest_quote;break;
        case 0x102cdbf0:answer=o.request_registered;break;
        case 0x10084be0:answer=o.request_value;break;
        case 0x100d3570:
            c.put32(globals+0x880,c.u32(globals+0x880)+1);c.put32(globals+0x884,c.r(4));
            if(o.request_mutate_reaction&&c.r(4)==0xfffffff4){c.put8(c.r(3)+0x65,127);c.put8(proposal+0x4c,200);mutated=true;}break;
        case 0x10363ca0:c.put32(globals+0x888,c.r(4));break;
        case 0x1008cd20:c.put32(globals+0x88c,c.r(5));break;
        case 0x1037fa80:c.put32(c.r(3)+0x50,c.u32(c.r(3)+0x50)|c.r(4));break;
        case 0x1037fa70:answer=o.request_status+(o.corrupt_request_status?1u:0u);break;
        case 0x100788f0:answer=std::uint32_t(o.request_refined);break;
        case 0x1014c2e0: {
            if(back==0x1008cf40)answer=o.unrest_human;
            else {const auto table=c.u32(globals);const auto id=(std::int64_t(c.r(3))-table)/0x248;answer=id>=0&&id<12&&((o.unrest_human_mask>>id)&1);
                if(o.unrest_mutate_table&&!mutated){c.put32(globals,other_clubs);mutated=true;}}
            break;
        }
        case 0x10103510: {
            const auto table=c.u32(globals);const auto id=(std::int64_t(c.r(4))-table)/0x248;
            require(id>=0&&id<12,"Valid candidate financial-account lookup");answer=((o.unrest_account_mask>>id)&1)?fixture+0x32000+std::uint32_t(id)*32:0;break;
        }
        case 0x100f5770:answer=0x12345678;break;
        case 0x10773c80:break;
        default:throw std::runtime_error("Unmodelled original news dependency "+hex_address(target));
        }
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)c.set(n,0xdd000000+n*0x101);
        for(unsigned n=0;n<14;++n)c.set_f(n,-20.25-n);
        c.set_cr(0,5);s.wr<1>(guest::reg::xer_ca,events.size()&1);c.set(3,answer);s.pc=back;
        if(target==0x10009f20)c.set(4,0x89abcdefu+(o.corrupt_offer_pair?1u:0u));
        if(target==0x100f5770)c.set(4,0x89abcdef);
        if(target==0x10773c80)c.set_f(1,o.unrest_balance+(o.corrupt_unrest_finance?1:0));
        if(o.entry==0x1008e350&&target==0x1077b770){
            const unsigned phase=back==0x1008e568?1:back==0x1008e880?2:back==0x1008e9e4?3:0;
            if(phase&&phase==o.request_force_sampler){c.set(23,std::uint32_t(o.request_bound));bound_injected=true;}
        }
        if(o.interrupt&&events.size()==o.interrupt)throw std::runtime_error("News dependency interruption");
    }
};
thread_local Model* active_model=nullptr;
struct Hook final:ContractManagerCallHook {bool call(State& s)override{active_model->call(s,s.pc,std::uint32_t(s.r<4>(guest::reg::lr)));return true;}} hook;
std::unique_ptr<State> seed(const Options& o) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->memory.map(fixture,0x40000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);Cpu c(*s);
    for(unsigned n=0;n<0x5000;++n)c.put8(stack-0x4000+n,n*17+3);
    for(unsigned n=0;n<32;++n)c.set_f(n,n+0.5);for(unsigned n=0;n<8;++n)c.set_cr(n,5);
    s->wr<1>(guest::reg::xer_so,1);s->wr<1>(guest::reg::xer_ov,0);s->wr<1>(guest::reg::xer_ca,1);
    c.set(1,stack);c.set(2,toc);c.set_lr(return_sentinel);c.set(3,fixture+0x80);
    c.set(4,o.entry==0x1008c270||o.entry==0x1008fa40?(o.contract?proposal:0):o.entry==0x10091630?(o.contract?staff+2*112:0):message);c.set(5,o.entry==0x1008c270?o.period:o.entry==0x10091630?clubs+o.related*0x248:o.mode);
    c.set(6,o.entry==0x1008c270?o.bosman:output);c.set(7,o.spread);s->pc=o.entry;s->budget=1000000;
    if(o.entry==0x10090310)c.set(4,o.contract?proposal:0);
    if(o.entry==0x10090560)c.set(3,own_manager);
    if(o.entry==0x10090a00){c.set(3,own_manager);c.set(4,o.contract?staff+2*112:0);c.set(5,o.departure_kind);c.set(6,o.important);}
    if(o.entry==0x1008e350){c.set(3,manager);c.set(4,o.contract?proposal:0);}
    if(o.entry==0x1008a350){c.set(3,manager);c.set(4,proposal);c.set(5,o.offer_notify);}
    if(o.entry==0x1008cd20){c.set(3,manager);c.set(4,o.contract?staff+2*112:0);c.set(5,o.unrest_club?clubs+0x248:0);}
    c.put32(toc-0x7be8,globals);c.put32(globals,clubs);c.put32(toc-0x7be0,globals+4);c.put32(globals+4,staff);
    c.put32(toc-0x7bb8,buffer);c.put32(toc-0x5718,pool);c.put32(toc-0x7ba8,globals+0x100);
    c.put32(toc-0x7bf0,globals+8);c.put32(globals+8,globals+0x180);
    c.put32(toc-0x7bac,globals+0x120);c.put8(globals+0x120,1);
    c.put32(toc-0x7b40,globals+12);c.put32(globals+12,registry);c.put32(registry+0x988,registry+0x1000);
    c.put32(toc-0x7b7c,globals+16);c.put32(globals+16,manager);c.put32(manager,rows);
    c.put32(toc-0x7b9c,globals+20);c.put32(globals+20,indices);c.put32(indices+o.record_id*100,o.index_row);
    c.put32(toc-0x7be4,globals+24);c.put32(globals+24,o.person_count);c.put32(toc-0x7b78,globals+28);
    c.put32(own_manager,own_rows);c.put32(own_manager+0x34,o.pending_kind?pending:0);
    c.put32(pending+4,o.pending_kind==3?0:pending_nodes);
    for(unsigned n=0;n<3;++n){c.put32(pending_nodes+n*16,n==1&&(o.pending_kind==1||o.pending_kind==4)?o.record_id:1);c.put32(pending_nodes+n*16+4,n==1&&o.pending_kind==4?3:o.club_id);
        c.put32(rows+n*100+4,n+1);c.put8(rows+n*100+0x38,std::uint32_t(o.contract_flags));c.put32(rows+n*100+0xc,o.current_wage);c.put16(rows+n*100+0x30,o.current_day);c.put16(rows+n*100+0x32,o.current_year);
        c.put32(own_rows+n*100+4,(n+2)%4);}
    c.put32(toc-0x7bb0,ui);c.put16(ui+0x2c58,o.selected_context);
    for(unsigned n=0;n<2;++n){c.put32(ui+n*0x2c0+4,o.context_present?ui_manager+n*0x100:0);const auto team=(n==0?o.context_matches:!o.context_matches)?(o.club_id<4?o.club_id:1):3;c.put32(ui_manager+n*0x100+0x24,o.context_club?clubs+team*0x248:0);}
    c.put32(buffer,0x5052494f);c.put32(buffer+4,0x52000000);c.put32(proposal,2);c.put32(proposal+4,1);
    c.put32(message+0xdc,41);
    if(o.entry==0x10090560)c.put32(message+0x24,o.validation);
    for(unsigned n=0;n<4;++n){const auto club=clubs+n*0x248;c.put32(club,n);c.put32(club+0x2c,n==2||(!o.division&&n==1)?0:divisions+n*0x100);c.put8(club+0x23b,std::uint32_t(n==3?o.second_gender:o.first_gender));c.put8(club+0x242,std::uint32_t(n==3?o.second_gender:o.first_gender));c.put32(nodes+n*16+4,n);}
    s->memory.copy_in(other_clubs,s->memory.copy_out(clubs,4*0x248));
    c.put8(proposal+0x5c,std::uint32_t(o.contract_status));
    c.put8(proposal+0x60,o.request_role);c.put16(proposal+0x30,o.day);c.put16(proposal+0x32,o.year);c.put32(proposal+0xc,o.wage);
    const auto person=staff+2*112;c.put32(person,o.record_id);c.put32(person+0x14,o.player?fixture+0x1c000:0);c.put32(person+0x24,clubs+o.current_club*0x248);c.put16(person+0x5c,std::uint32_t(o.age));
    c.put8(person+0x65,std::uint32_t(o.first_trait));c.put8(person+0x67,std::uint32_t(o.second_trait));c.put16(fixture+0x1c000+0xc,std::uint32_t(o.profile_c));c.put16(fixture+0x1c000+0xa,std::uint32_t(o.profile_a));
    c.put32(person+0xc,divisions+o.nation_scope*0x100);
    if(o.entry==0x10090a00){for(unsigned n=0;n<4;++n)c.put32(clubs+n*0x248+0x2c,divisions+(o.shared_scopes?0:n)*0x100);s->memory.copy_in(other_clubs,s->memory.copy_out(clubs,4*0x248));}
    for(unsigned n=0;n<3;++n){c.put32(person+0x28+n*4,competitions+n*0x100);c.put32(competitions+n*0x100+0x20,11+n);}
    if(o.entry==0x1008a350){
        c.put8(proposal+0x38,std::uint32_t(o.contract_flags));c.put8(proposal+0x64,o.offer_outcome);c.put8(proposal+0x78,o.offer_reason);c.put32(proposal+0x70,o.offer_origin);
        c.put32(proposal+8,200000);c.put16(proposal+0x28,80);c.put16(proposal+0x2a,2001);c.put32(rows+0x34,0x5a6b7c8d);
        c.put32(clubs+0x248+0x90,o.club_manager?person:0);c.put8(indices+o.record_id*100+12,o.index_flags);
        c.put8(fixture+0x1c000+0xf,std::uint32_t(o.keeper));c.put8(fixture+0x1c000+0x11,std::uint32_t(o.defender));c.put8(fixture+0x1c000+0x13,std::uint32_t(o.midfielder));c.put8(fixture+0x1c000+0x15,std::uint32_t(o.attacker));
        c.put32(toc-0x7b90,globals+0x800);c.put32(globals+0x800,o.random_salt);
        c.put32(toc-0x7b84,globals+0x804);c.put32(globals+0x804,globals+0x900);
        c.put32(toc-0x7b88,globals+0x808);c.put32(globals+0x808,globals+0x900+(o.random_wrap?0:10));
        c.put32(toc-0x7b8c,globals+0x920);c.put16(globals+0x902,o.random_word);c.put16(globals+0x920,o.random_word);
        c.put32(toc-0x79a8,globals+0x810);c.put16(globals+0x810,o.cutoff_day);c.put16(globals+0x812,o.cutoff_year);
        c.put32(toc-0x79b8,globals+0x818);c.put16(globals+0x818,o.later_day);c.put16(globals+0x81a,o.later_year);
        c.put32(globals+0x820,o.origin_present?globals+0x840:0);c.put32(globals+0x840+0x2c,0x45678);
    }
    contract_manager_news_constants(*s,toc,table);contract_manager_offer_constants(*s,toc,table+0x400);
    if(o.entry==0x1008cd20){
        c.put32(toc-0x7ab8,globals+0x860);c.put32(globals+0x860,fixture+0x32000);
        c.put32(person+8,o.unrest_fee);c.put32(rows+0x50,o.offer_flags);c.put8(rows+0x60,o.request_role);
        for(unsigned n=0;n<12;++n){const auto club=clubs+n*0x248;c.put32(club,n);c.put32(club+0x90,o.club_manager?person:0);
            const int reputation=o.unrest_reputations==1?100:o.unrest_reputations==2?12000-int(n)*100:o.unrest_reputations==3?(n%2?32767:-32768):o.unrest_reputations==4?(n==3?200:100):int(n)*1000;
            c.put16(club+0x1ec,std::uint32_t(reputation));}
        s->memory.copy_in(other_clubs,s->memory.copy_out(clubs,12*0x248));
        require(o.unrest_candidates.size()<=40,"Bounded initialized candidate list");
        for(unsigned n=0;n<o.unrest_candidates.size();++n)c.put32(nodes+n*16+4,o.unrest_candidates[n]);
        c.put32(toc-0x7b90,globals+0x800);c.put32(globals+0x800,o.random_salt);
        c.put32(toc-0x7b84,globals+0x804);c.put32(globals+0x804,globals+0x900+(o.unrest_wrap==1?4:o.unrest_wrap==2?2:0));
        c.put32(toc-0x7b88,globals+0x808);c.put32(globals+0x808,globals+0x900+(o.unrest_wrap==3?0:4));
        c.put32(toc-0x7b8c,globals+0x900);for(unsigned n=0;n<3;++n)c.put16(globals+0x900+n*2,o.random_word+n*o.unrest_random_stride);
    }
    if(o.entry==0x1008e350){
        c.put8(proposal+0x4c,std::uint32_t(o.request_reaction));c.put32(proposal+0x50,o.offer_flags);
        c.put32(person+0xc,o.request_nation?fixture+0x33200:0);c.put32(fixture+0x33200+0x68,3);
        c.put32(toc-0x7b90,globals+0x800);c.put32(globals+0x800,o.random_salt);
        c.put32(toc-0x7b84,globals+0x804);c.put32(globals+0x804,globals+0x900+(o.unrest_wrap==1?4:o.unrest_wrap==2?2:0));
        c.put32(toc-0x7b88,globals+0x808);c.put32(globals+0x808,globals+0x900+(o.unrest_wrap==3?0:4));
        c.put32(toc-0x7b8c,globals+0x900);for(unsigned n=0;n<3;++n)c.put16(globals+0x900+n*2,o.random_word+n*o.unrest_random_stride);
        for(unsigned n=0;n<32;++n)s->wr<1>(1280+n,n<24&&o.fp_pattern?((n+o.fp_pattern)&1):0);s->wr<1>(1308,o.enable_inexact_exception);
    }
    if(o.entry==0x1008ec00){
        const auto copy=s->memory.copy_out(person,112);s->memory.copy_in(staff+o.offer_person*112,copy);c.put32(staff+o.offer_person*112,o.offer_person);
        c.put16(clubs+o.club_id*0x248+0x1ec,std::uint32_t(o.request_reputation));c.put8(clubs+o.club_id*0x248+0x242,std::uint32_t(o.short_gender));
        c.put32(clubs+o.club_id*0x248+0x2c,o.division?fixture+0x33000:0);c.put8(fixture+0x33000+0x115,std::uint32_t(o.nation_gender));
        c.put32(toc-0x7b98,globals+0x864);c.put32(globals+0x864,fixture+0x36000);
        for(unsigned n=0;n<5;++n){c.put32(fixture+0x36000+n*0x128+0x68,n);c.put8(fixture+0x36000+n*0x128+0x116,std::uint32_t(o.nationality_gender));}
        s->memory.copy_in(fixture+0x34000,s->memory.copy_out(staff,12*112));
    }
    if(o.entry==0x1008d450){
        const auto copy=s->memory.copy_out(person,112);s->memory.copy_in(staff+o.offer_person*112,copy);c.put32(staff+o.offer_person*112,o.offer_person);
        c.put32(clubs+o.club_id*0x248+0x2c,o.division?fixture+0x33000:0);c.put8(fixture+0x33000+0x115,std::uint32_t(o.nation_gender));
        c.put32(staff+o.offer_person*112+0xc,fixture+0x33200);c.put8(fixture+0x33200+0x116,std::uint32_t(o.nationality_gender));
        c.put8(clubs+o.club_id*0x248+0x242,std::uint32_t(o.short_gender));
        s->memory.copy_in(fixture+0x34000,s->memory.copy_out(staff,12*112));
    }
    if(o.entry==0x1008ab40){const auto copy=s->memory.copy_out(person,112);s->memory.copy_in(staff+o.offer_person*112,copy);c.put32(staff+o.offer_person*112,o.offer_person);c.put8(clubs+o.club_id*0x248+0x242,std::uint32_t(o.short_gender));
        for(unsigned n=0;n<32;++n)s->wr<1>(1280+n,n<24&&o.fp_pattern?((n+o.fp_pattern)&1):0);
        s->wr<1>(1308,o.enable_inexact_exception);}
    if(o.entry==0x1008cd20){for(unsigned n=0;n<32;++n)s->wr<1>(1280+n,n<24&&o.fp_pattern?((n+o.fp_pattern)&1):0);s->wr<1>(1308,o.enable_inexact_exception);}
    return s;
}
void original(State& s,Model& m) {
    while(s.pc!=return_sentinel){bool owned=false;for(auto [begin,end]:ranges)if(begin==m.o.entry)owned|=s.pc>=begin&&s.pc<end;
        if(owned)require(contract_manager_news_reference(s),"Complete original news body");
        else m.call(s,s.pc,std::uint32_t(s.r<4>(guest::reg::lr)));}
}
void modern(State& s,Model& m) {
    Cpu c(s);const auto previous=contract_manager_call_hook;active_model=&m;contract_manager_call_hook=&hook;
    try{switch(s.pc){
    case 0x1008ec00:contract_manager::render_transfer_request(c,c.r(4),c.r(5),c.r(6));break;
    case 0x1008e350:contract_manager::review_transfer_request(c,c.r(3),c.r(4));break;
    case 0x1008d450:contract_manager::render_transfer_unrest(c,c.r(4),c.r(5),c.r(6));break;
    case 0x1008cd20:contract_manager::publish_transfer_unrest(c,c.r(3),c.r(4),c.r(5));break;
    case 0x1008ab40:contract_manager::render_contract_offer_result(c,c.r(4),c.r(5),c.r(6));break;
    case 0x1008a350:contract_manager::publish_contract_offer_result(c,c.r(3),c.r(4),c.r(5));break;
    case 0x1008c090:contract_manager::render_cancelled_offer(c,c.r(4),c.r(5),c.r(6));break;
    case 0x1008c270:contract_manager::publish_expiring_contract(c,c.r(4),c.r(5),c.r(6),c.r(7));break;
    case 0x1008c4c0:contract_manager::render_expiring_contract(c,c.r(4),c.r(5),c.r(6));break;
    case 0x1008ca50:contract_manager::render_bosman_departure(c,c.r(4),c.r(5),c.r(6));break;
    case 0x1008f7b0:contract_manager::render_withdrawn_request(c,c.r(4),c.r(5),c.r(6));break;
    case 0x1008fa40:contract_manager::publish_assurance_request(c,c.r(4));break;
    case 0x1008fcf0:contract_manager::render_assurance_request(c,c.r(4),c.r(5),c.r(6));break;
    case 0x100913f0:contract_manager::render_cancelled_trial(c,c.r(4),c.r(5),c.r(6));break;
    case 0x10091630:contract_manager::publish_loan_return(c,c.r(4),c.r(5));break;
    case 0x100918b0:contract_manager::render_loan_return(c,c.r(4),c.r(5),c.r(6));break;
    case 0x10090310:contract_manager::publish_new_contract_request(c,c.r(4));break;
    case 0x10090560:contract_manager::render_new_contract_request(c,c.r(3),c.r(4),c.r(5),c.r(6));break;
    case 0x10090a00:contract_manager::publish_departure_status(c,c.r(3),c.r(4),c.r(5),c.r(6));break;
    case 0x10090f20:contract_manager::render_departure_status(c,c.r(4),c.r(5),c.r(6));break;
    default:throw std::runtime_error("Unknown modern news entry");}c.finish();}
    catch(...){contract_manager_call_hook=previous;active_model=nullptr;throw;}
    contract_manager_call_hook=previous;active_model=nullptr;
}
void same(const State& a,const State& b,const std::string& label) {
    for(unsigned n=0;n<32;++n){require(a.r<4>(n*4)==b.r<4>(n*4),label+" r"+std::to_string(n));require(a.r<8>(guest::reg::fpr(n))==b.r<8>(guest::reg::fpr(n)),label+" f"+std::to_string(n));}
    for(unsigned n=1280;n<1312;++n)require(a.r<1>(n)==b.r<1>(n),label+" FPSCR");
    for(unsigned n=2304;n<2312;++n)require(a.r<1>(n)==b.r<1>(n),label+" CR"+std::to_string(n-2304));
    for(auto reg:{guest::reg::xer_ca,guest::reg::xer_so,guest::reg::xer_ov})require(a.r<1>(reg)==b.r<1>(reg),label+" XER");
    for(auto reg:{guest::reg::lr,guest::reg::ctr})require(a.r<4>(reg)==b.r<4>(reg),label+" LR/CTR");require(a.pc==b.pc,label+" PC");
    for(auto [at,size]:a.memory.mapped_ranges()){
        const auto x=a.memory.copy_out(at,size),y=b.memory.copy_out(at,size);
        if(x!=y){const auto pos=std::mismatch(x.begin(),x.end(),y.begin());throw std::runtime_error(label+" memory "+hex_address(at+std::uint32_t(pos.first-x.begin())));}}
}
unsigned cases=0,returned=0,interruptions=0,fp_guards=0,negative=0,injected_returns=0;std::set<std::uint32_t> continuations,normal_continuations,injected_continuations;std::map<std::uint32_t,unsigned> per_entry,per_injected;
void compare(Options o={}) {
    auto source=seed(o),a=clone(*source),b=clone(*source);Model old(o),current(o);
    old.o.corrupt=old.o.corrupt_period=old.o.corrupt_extra=old.o.corrupt_guard=old.o.corrupt_loan_spill=old.o.corrupt_date=old.o.corrupt_ui=old.o.corrupt_random=old.o.corrupt_index_bit=old.o.corrupt_offer_pair=old.o.corrupt_offer_spill=old.o.corrupt_unrest_finance=old.o.corrupt_unrest_sort=old.o.corrupt_unrest_spill=old.o.corrupt_request_status=old.o.corrupt_request_quote=old.o.corrupt_request_render_spill=false;
    const auto run=[](State& s,Model& m,bool reference){try{if(reference)original(s,m);else modern(s,m);return std::string{};}catch(const std::exception& e){return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,current,false),label="news "+hex_address(o.entry)+" case"+std::to_string(cases);
    const bool fp_guard=o.enable_inexact_exception&&expected=="Original enabled floating inexact exception requires a handler";
    if(o.enable_inexact_exception)require(fp_guard,label+" requested inexact exception must interrupt");
    require(expected.empty()||expected=="News dependency interruption"||fp_guard,label+" invalid original: "+expected);
    require(expected==actual,label+" outcomes "+expected+"/"+actual);
    if(o.interrupt)require(!expected.empty()&&old.events.size()==o.interrupt,label+" interruption reached");
    else if(fp_guard){
        // Explicit comparisons let the generator isolate these watched PCs.
        if(o.entry==0x1008e350){
            if(o.request_force_sampler==1)require(a->pc==0x1008e620,label+" explicit tolerance FP guard reached");
            else if(o.request_force_sampler==2)require(a->pc==0x1008e938,label+" explicit first-agent FP guard reached");
            else require(a->pc==0x1008ea9c,label+" explicit second-agent FP guard reached");
        }else if(o.entry==0x1008cd20)require(a->pc==0x1008d028,label+" explicit unrest FP guard reached");
        else require(a->pc==0x1008ae68,label+" explicit offer FP guard reached");
    }else require(expected.empty()&&a->pc==return_sentinel,label+" original completed");
    if(o.mutate_lookup&&!o.interrupt){
        require(old.mutated&&current.mutated,label+" callback mutation reached");
        const auto published=std::find_if(old.events.begin(),old.events.end(),[](const Event& e){return e.back==0x1008c484;});
        require(published!=old.events.end()&&published->gpr[5]==other_clubs+3*0x248,label+" original rereads mutated row ID and club table");
    }
    if(o.corrupt_extra)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.extra!=0;}),label+" stack variadic control reached");
    if(o.corrupt_loan_spill)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.stack_arguments[3]!=0;}),label+" fourth loan stack argument control reached");
    if(o.corrupt_guard)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x102cc140;}),label+" unmasked assurance guard control");
    if(o.corrupt_date){const auto count=[](const Model& m){return std::count_if(m.events.begin(),m.events.end(),[](const Event& e){return e.target==0x102cd0c0;});};require(count(old)==0&&count(current)==1,label+" date control changes stale-notice decision");}
    if(o.corrupt_ui)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x10727890&&e.gpr[4]==pool+0x6b24;}),label+" unmasked whole-word UI control");
    if(o.corrupt_index_bit){const auto field=std::find_if(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x102cb4b0&&e.gpr[4]==27;});require(field!=old.events.end()&&field->gpr[5]==0,label+" index flag control changes a zero low bit");}
    if(o.corrupt_offer_spill)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x10727890&&e.gpr[4]==pool+0x15d6&&e.stack_arguments[3]!=0;}),label+" fourth offer stack argument reached");
    if(o.entry==0x1008ec00&&!o.interrupt){
        if(o.request_render_mutate_tables){require(old.mutated&&current.mutated,label+" renderer callback table mutations reached");
            const auto offset=o.offer_person*112;require(std::any_of(old.events.begin(),old.events.end(),[&](const Event& e){return e.target==0x100cbde0&&e.gpr[3]==fixture+0x34000+offset;}),label+" renderer rereads changed staff table");
            if(std::int8_t(o.mode)!=0)require(std::any_of(old.events.begin(),old.events.end(),[&](const Event& e){return e.back==0x1008f548&&e.gpr[6]==other_clubs+o.club_id*0x248;}),label+" renderer rereads changed primary club table");}
        if(std::int8_t(o.mode)!=0&&(o.offer_flags==0x800||o.offer_flags==0x807||o.offer_flags==0x907)&&o.request_status==0){
            std::vector<std::uint32_t> repeated;for(const auto& e:old.events)if(e.target==0x10727890&&(e.gpr[4]==pool+0x4c93||e.gpr[4]==pool+0x4cda))repeated.push_back(e.gpr[4]);
            const auto expected=o.offer_flags==0x907?std::vector<std::uint32_t>{pool+0x4c93}:std::vector<std::uint32_t>{pool+0x4c93,pool+0x4cda};
            require(repeated==expected,label+" original duplicate reason flag order and fifth-reason boundary");}
    }
    if(o.corrupt_request_render_spill)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x10727890&&(e.gpr[4]==pool+0x4f4d||e.gpr[4]==pool+0x51bf)&&e.stack_arguments[0]!=0;}),label+" required agent story stack argument reached");
    if(o.entry==0x1008e350&&!o.interrupt&&!fp_guard){
        if(o.request_force_sampler){require(old.bound_injected&&current.bound_injected,label+" explicit retained-bound injection reached");
            if(o.request_bound>65535||o.request_bound<-65535)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x100788f0;}),label+" otherwise unreachable wide sampler reached");}
        if(o.request_mutate_reaction)require(old.mutated&&current.mutated&&a->memory.read<1>(proposal+0x4c)==std::uint8_t(-100),label+" reaction reread after status callback");
    }
    if(o.entry==0x1008d450&&o.unrest_mutate_table&&!o.interrupt){
        require(old.mutated&&current.mutated,label+" club callback mutation reached");
        require(std::any_of(old.events.begin(),old.events.end(),[&](const Event& e){return e.back==0x1008e240&&e.gpr[6]==other_clubs+o.club_id*0x248;}),label+" primary link rereads changed club table");
        require(std::any_of(old.events.begin(),old.events.end(),[&](const Event& e){return e.back==0x1008e314&&e.gpr[6]==clubs+o.unrest_candidates.front()*0x248;}),label+" interested links retain cached original club pointers");
    }
    if(o.unrest_render_mutate_staff&&!o.interrupt){require(old.mutated&&current.mutated,label+" staff callback mutation reached");require(std::any_of(old.events.begin(),old.events.end(),[&](const Event& e){return e.back==0x1008e288&&e.gpr[6]==fixture+0x34000+o.offer_person*112;}),label+" name link rereads changed staff table");}
    if(o.corrupt_unrest_spill)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x10727890&&e.gpr[4]==pool+0x410c&&e.stack_arguments[5]!=0;}),label+" sixth unrest stack argument reached");
    if(o.corrupt_offer_pair)require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x1000b410&&e.gpr[6]==0x89abcdef;}),label+" currency return pair reached");
    const auto field_value=[](const Model& m,unsigned field){const auto at=std::find_if(m.events.begin(),m.events.end(),[&](const Event& e){return e.target==0x102cb4b0&&e.gpr[4]==field;});require(at!=m.events.end(),"Required unrest metadata field");return at->gpr[5];};
    if(o.corrupt_request_quote){const auto automatic=[](const Model& m){return std::any_of(m.events.begin(),m.events.end(),[](const Event& e){return e.target==0x1008cd20;});};require(automatic(old)&&!automatic(current),label+" one-unit quote control changes automatic acceptance");}
    if(o.request_clear_profile&&!o.interrupt)require(old.mutated&&current.mutated&&field_value(old,9)==0&&a->memory.read<4>(staff+2*112+0x14)==0,label+" missing profile reread after metadata callback");
    if(o.corrupt_request_status){require(field_value(old,12)==o.request_status&&field_value(current,12)==o.request_status+1,label+" status control changes published metadata");}
    if(o.corrupt_unrest_finance){require(field_value(old,14)==0&&field_value(current,14)==1,label+" finance control changes affordability at integer boundary");}
    if(o.corrupt_unrest_sort){require(field_value(old,15)==3&&field_value(old,16)==2&&field_value(old,17)==1&&field_value(current,15)==3&&field_value(current,16)==1&&field_value(current,17)==2,label+" sort control distinguishes equal-reputation order");}
    if(o.entry==0x1008cd20&&!o.interrupt&&!fp_guard){
        if(o.unrest_mutate_table){require(old.mutated&&current.mutated,label+" candidate table mutation reached");require(std::any_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x10103510&&e.gpr[4]==other_clubs+0x248;}),label+" financial lookup rereads changed club table");}
        if(o.unrest_mutate_candidates){require(old.mutated&&current.mutated&&field_value(old,15)==2&&field_value(old,16)==1,label+" metadata callbacks reread candidate array");}
        if(o.unrest_reputations==4&&!o.corrupt_unrest_sort)require(field_value(old,15)==3&&field_value(old,16)==2&&field_value(old,17)==1,label+" original nonstable reputation swap order");
        if(o.unrest_candidates.size()==40&&o.unrest_human_mask==0)require(std::count_if(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x10738e00;})==31,label+" thirty-first row fetched before cap test");
    }
    if(o.mutate_departure&&!o.interrupt){
        require(old.mutated&&current.mutated,label+" departure mutation reached");require(a->memory.read<4>(globals+28)==rows+100,label+" global cache preserved by direct-manager lookup");
        const auto field=std::find_if(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x102cb4b0&&e.gpr[4]==7;});
        require(field!=old.events.end()&&field->gpr[5]==3,label+" fourth lookup uses distinct direct-manager rows");}
    if(o.entry==0x10090560&&!o.interrupt&&!o.corrupt_date){
        const bool row=std::int32_t(o.record_id)<std::int32_t(o.person_count)&&std::int32_t(o.index_row)>=0;
        const bool matching=row&&o.wage==o.current_wage&&std::uint16_t(o.year)==std::uint16_t(o.current_year)&&std::int16_t(o.day)==std::int16_t(o.current_day);
        const bool invalid=std::int8_t(o.mode)!=0&&std::int8_t(o.validation)==0&&(!matching||o.pending_kind==1);
        const auto observed=std::count_if(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x102cd0c0;});require(observed==unsigned(invalid),label+" original stale-notice path characterized");}
    if(o.mutate_contract&&!o.interrupt){
        require(old.mutated&&current.mutated,label+" loan index mutations reached");
        require(a->memory.read<4>(globals+28)==rows+200,label+" third lookup refreshes cached contract");
        const auto published=std::find_if(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x102cc140;});
        require(published!=old.events.end()&&published->gpr[5]==clubs+3*0x248,label+" original uses freshly selected recipient");}
    if(o.entry==0x1008c4c0&&std::int8_t(o.mode)!=0&&(o.period==0||o.period>7)&&!o.interrupt){
        require(std::none_of(old.events.begin(),old.events.end(),[](const Event& e){return e.target==0x10727890;}),label+" original deliberately skips formatting");
        require(a->memory.read<4>(output+0x10)==0x5052494f,label+" prior shared text is emitted");}
    if(o.corrupt)b->memory.write<1>(output+0x200,1);
    bool rejected=false;
    try{same(*a,*b,label);if(old.events!=current.events){const auto at=std::mismatch(old.events.begin(),old.events.end(),current.events.begin(),current.events.end());std::string detail;
        if(at.first!=old.events.end()&&at.second!=current.events.end()){
            detail=" at "+hex_address(at.first->back)+"/"+hex_address(at.second->back);
            for(unsigned n=0;n<32;++n)if(at.first->gpr[n]!=at.second->gpr[n])detail+=" r"+std::to_string(n)+"="+hex_address(at.first->gpr[n])+"/"+hex_address(at.second->gpr[n]);}
        throw std::runtime_error(label+" call"+std::to_string(at.first-old.events.begin())+detail);}}
    catch(const std::runtime_error&){if(!o.corrupt&&!o.corrupt_period&&!o.corrupt_extra&&!o.corrupt_guard&&!o.corrupt_loan_spill&&!o.corrupt_date&&!o.corrupt_ui&&!o.corrupt_random&&!o.corrupt_index_bit&&!o.corrupt_offer_pair&&!o.corrupt_offer_spill&&!o.corrupt_unrest_finance&&!o.corrupt_unrest_sort&&!o.corrupt_unrest_spill&&!o.corrupt_request_status&&!o.corrupt_request_quote&&!o.corrupt_request_render_spill)throw;rejected=true;}
    require(rejected==(o.corrupt||o.corrupt_period||o.corrupt_extra||o.corrupt_guard||o.corrupt_loan_spill||o.corrupt_date||o.corrupt_ui||o.corrupt_random||o.corrupt_index_bit||o.corrupt_offer_pair||o.corrupt_offer_spill||o.corrupt_unrest_finance||o.corrupt_unrest_sort||o.corrupt_unrest_spill||o.corrupt_request_status||o.corrupt_request_quote||o.corrupt_request_render_spill),label+" negative control");
    ++cases;returned+=!rejected&&expected.empty();interruptions+=expected=="News dependency interruption";fp_guards+=fp_guard;negative+=rejected;
    if(!rejected&&expected.empty()){++per_entry[o.entry];injected_returns+=o.request_force_sampler!=0;per_injected[o.entry]+=o.request_force_sampler!=0;
        for(const auto& e:old.events){continuations.insert(e.back);(o.request_force_sampler?injected_continuations:normal_continuations).insert(e.back);}}
}
void interrupt_each(Options o) {
    auto s=seed(o);Model m(o);original(*s,m);require(s->pc==return_sentinel,"Original interruption schedule returned");
    for(unsigned n=1;n<=m.events.size();++n){o.interrupt=n;compare(o);}
}
}
int main(){try{
    compare(Options{.entry=0x1008ec00});
    constexpr std::array<std::uint32_t,23> request_masks{1,2,4,0x100,8,0x10,0x20000000,0x20,0x40,0x80,0x200,0x4000,0x400,0x800,0x2000,0x20000,0x400000,0x2000000,0x4000000,0x200000,0x40000,0x01000002,0x80000001};
    for(auto flags:{0x807u,0x907u}){Options o;o.entry=0x1008ec00;o.offer_flags=flags;compare(o);interrupt_each(o);}
    for(auto flags:request_masks)for(auto person:{0u,1u,2u})for(auto agent:{0u,1u}){Options o;o.entry=0x1008ec00;o.offer_flags=flags;o.offer_person=person;o.agent=agent;compare(o);}
    for(unsigned i=0;i<request_masks.size();++i)for(unsigned j=i+1;j<request_masks.size();++j){Options o;o.entry=0x1008ec00;o.offer_flags=request_masks[i]|request_masks[j];o.offer_person=(i+j)%3;o.agent=i&1;compare(o);}
    for(unsigned i=0;i<request_masks.size();++i)for(unsigned j=i+1;j<request_masks.size();++j)for(unsigned k=j+1;k<request_masks.size();++k){Options o;o.entry=0x1008ec00;o.offer_flags=request_masks[i]|request_masks[j]|request_masks[k];o.offer_person=(i+j+k)%3;o.agent=(i+k)&1;o.request_status=(j%7==0)?0x80:0;compare(o);}
    for(auto flags:{0u,7u,15u,23u,29u,31u,0x800u,0x3fffffffu,0x80000001u,0xffffffffu})for(auto person:{0u,1u,2u,6u,10u})for(auto status:{0u,0x80u,0x100u,0x80000000u,0xffffffffu})for(auto agent:{0u,1u,128u,256u}){Options o;o.entry=0x1008ec00;o.offer_flags=flags;o.offer_person=person;o.request_status=status;o.agent=agent;compare(o);}
    for(auto mode:{0u,1u,128u,256u,0xffffffffu})for(auto changed:{0u,1u,128u,256u})for(auto accepted:{0u,1u,128u,256u,257u})for(auto available:{0u,1u,128u,256u}){Options o;o.entry=0x1008ec00;o.mode=mode;o.changed=changed;o.validation=accepted;o.name_available=available;o.offer_flags=7;compare(o);}
    for(int reputation:{-32768,6749,6750,6751,32767,65535})for(bool country:{false,true})for(auto retiring:{0u,1u,128u,256u,0xffffffffu})for(auto nation:{0u,1u,4u,0xffffffffu})for(int gender:{-128,-1,0,127}){Options o;o.entry=0x1008ec00;o.offer_flags=0x20020020;o.request_reputation=reputation;o.division=country;o.important=retiring;o.nation_scope=nation;o.short_gender=gender;o.first_gender=-gender-1;o.nation_gender=gender;o.nationality_gender=-gender-1;compare(o);}
    for(auto flags:request_masks){Options o;o.entry=0x1008ec00;o.offer_flags=flags;interrupt_each(o);}
    for(auto flags:{0u,7u,23u,29u,31u,0xffffffffu})for(auto person:{0u,1u,2u})for(auto agent:{0u,1u}){Options o;o.entry=0x1008ec00;o.offer_flags=flags;o.offer_person=person;o.agent=agent;compare(o);interrupt_each(o);}
    for(auto mode:{0u,1u}){Options o;o.entry=0x1008ec00;o.mode=mode;o.offer_flags=7;o.request_render_mutate_tables=true;compare(o);interrupt_each(o);}
    for(auto person:{0u,1u}){Options o;o.entry=0x1008ec00;o.offer_flags=7;o.offer_person=person;o.agent=1;o.corrupt_request_render_spill=true;compare(o);}
    compare(Options{.entry=0x1008e350});
    for(auto registered:{0u,1u,128u,256u,0xffffffffu})for(bool contract:{false,true})for(bool player:{false,true}){Options o;o.entry=0x1008e350;o.request_registered=registered;o.contract=contract;o.player=player;compare(o);}
    for(auto role:{0u,1u,2u,17u,255u})for(auto value:{0u,1u,1000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto quote:{0u,1u,1499999u,1500000u,1500001u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto pattern:{0u,1u}){
        Options o;o.entry=0x1008e350;o.request_registered=0;o.request_role=role;o.request_value=value;o.request_quote=quote;o.fp_pattern=pattern;compare(o);
    }
    for(int ambition:{-128,-6,-5,0,1,5,20,127,128,255})for(int reaction:{-128,-101,-100,-1,0,99,100,127,128,255})for(auto word:{0u,1u,49u,50u,65535u}){
        Options o;o.entry=0x1008e350;o.request_registered=0;o.request_role=1;o.first_trait=ambition;o.request_reaction=reaction;o.random_word=word;compare(o);
    }
    for(auto registered:{0u,1u})for(auto wrap:{0u,1u,2u,3u})for(auto salt:{0u,1u,65535u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto word:{0u,1u,65535u}){
        Options o;o.entry=0x1008e350;o.request_registered=registered;o.unrest_wrap=wrap;o.random_salt=salt;o.random_word=word;o.first_trait=7;o.second_trait=-3;compare(o);
    }
    for(int ambition:{-128,-1,0,1,2,3,127,128,255})for(int trait:{-128,-1,0,1,3,127,128,255})for(int profile:{-32768,4750,4751,32767})for(int age:{32,33,127,128,255,288})for(bool nation:{false,true}){
        Options o;o.entry=0x1008e350;o.first_trait=ambition;o.second_trait=trait;o.profile_c=profile;o.age=age;o.request_nation=nation;compare(o);
    }
    for(auto first:{0u,1u,128u,256u})for(auto second:{0u,1u,256u})for(auto third:{0u,1u,256u}){Options o;o.entry=0x1008e350;o.guard_first=first;o.guard_second=second;o.guard_third=third;compare(o);}
    for(auto registered:{0u,1u})for(auto wrap:{0u,1u,2u,3u})for(int trait:{0,1,7,-128}){Options o;o.entry=0x1008e350;o.request_registered=registered;o.unrest_wrap=wrap;o.first_trait=trait;o.second_trait=3;interrupt_each(o);}
    for(auto flags:{0u,2u,8u,10u,255u}){Options o;o.entry=0x1008e350;o.request_registered=0;o.request_quote=1499999;o.contract_status=int(flags);compare(o);interrupt_each(o);}
    Options request_null;request_null.entry=0x1008e350;request_null.contract=false;interrupt_each(request_null);
    Options reaction;reaction.entry=0x1008e350;reaction.request_registered=0;reaction.request_role=1;reaction.request_mutate_reaction=true;compare(reaction);interrupt_each(reaction);
    Options cleared_profile;cleared_profile.entry=0x1008e350;cleared_profile.request_clear_profile=true;compare(cleared_profile);interrupt_each(cleared_profile);
    Options request_status;request_status.entry=0x1008e350;request_status.request_status=0x81020304;request_status.corrupt_request_status=true;compare(request_status);
    Options request_quote;request_quote.entry=0x1008e350;request_quote.request_registered=0;request_quote.request_quote=1499999;request_quote.corrupt_request_quote=true;compare(request_quote);
    Options request_rng;request_rng.entry=0x1008e350;request_rng.unrest_wrap=3;request_rng.first_trait=7;request_rng.second_trait=3;request_rng.corrupt_random=true;compare(request_rng);
    for(unsigned phase:{1u,2u,3u})for(int bound:{65535,65536,70000,-65535,-65536,-70000})for(int refined:{-100,0,100}){
        Options o;o.entry=0x1008e350;o.request_registered=phase==1?0:1;o.request_role=1;o.first_trait=3;o.second_trait=3;o.unrest_wrap=3;o.request_force_sampler=phase;o.request_bound=bound;o.request_refined=refined;compare(o);
    }
    for(unsigned phase:{1u,2u,3u}){Options o;o.entry=0x1008e350;o.request_registered=phase==1?0:1;o.request_role=1;o.first_trait=3;o.second_trait=3;o.unrest_wrap=3;o.request_force_sampler=phase;o.request_bound=70000;interrupt_each(o);}
    for(unsigned phase:{1u,2u,3u}){Options o;o.entry=0x1008e350;o.request_registered=phase==1?0:1;o.request_role=1;o.first_trait=3;o.second_trait=3;o.unrest_wrap=3;o.request_force_sampler=phase;o.request_bound=70000;o.enable_inexact_exception=true;compare(o);}
    compare(Options{.entry=0x1008d450});
    for(unsigned bits=0;bits<128;++bits)for(auto person:{1u,2u})for(auto fee:{0u,1u,2u}){
        Options o;o.entry=0x1008d450;o.offer_flags=0;constexpr std::array<std::uint32_t,7> masks{1,2,0x100,0x10,0x20,0x4000,0x20000};
        for(unsigned n=0;n<7;++n)if((bits>>n)&1)o.offer_flags|=masks[n];o.offer_person=person;o.unrest_fee=fee;compare(o);
    }
    for(auto count:{0u,1u,2u,3u})for(auto variation:{0u,1u,256u,0xffffffffu})for(auto flag:{0u,1u,128u,256u})for(auto role:{1u,2u,257u})for(auto reasons:{0u,2u,0x01000002u,0x3ffffu}){
        Options o;o.entry=0x1008d450;o.unrest_candidates.resize(count);for(unsigned n=0;n<count;++n)o.unrest_candidates[n]=n+1;
        o.unrest_variation=variation;o.unrest_not_listed=flag;o.request_role=role;o.offer_flags=reasons;compare(o);
    }
    for(auto mode:{0u,256u})for(auto person:{0u,1u,2u,6u,10u})for(auto flags:{0u,1u,4u,5u,6u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto variation:{0u,1u,2u,3u,0x80000000u,0xffffffffu})for(auto flag:{0u,1u,128u,256u}){
        Options o;o.entry=0x1008d450;o.mode=mode;o.offer_person=person;o.offer_flags=flags;o.unrest_variation=variation;o.unrest_not_listed=flag;compare(o);
    }
    for(auto mode:{1u,128u})for(auto fee:{0u,1u,2u,3u,4u,5u,6u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto person:{0u,1u,2u,6u,10u})for(auto count:{0u,1u,2u,3u}){
        Options o;o.entry=0x1008d450;o.mode=mode;o.unrest_fee=fee;o.offer_person=person;o.unrest_candidates.resize(count);o.offer_flags=0x3ffff;for(unsigned n=0;n<count;++n)o.unrest_candidates[n]=n+1;compare(o);
        o.offer_flags=0;o.unrest_variation=1;compare(o);
    }
    for(int gender:{-128,-1,0,127})for(auto position:{0u,32767u,32768u,65535u,65536u,0x80000000u,0xffffffffu})for(bool nation:{false,true})for(auto available:{0u,1u,128u,256u}){
        Options o;o.entry=0x1008d450;o.unrest_position=position;o.unrest_gender=std::uint32_t(gender);o.first_gender=-gender-1;o.short_gender=gender;o.nation_gender=gender;o.nationality_gender=-gender-1;o.division=nation;o.name_available=available;o.offer_flags=0x20020;compare(o);
    }
    for(auto count:{0u,1u,2u,3u})for(auto variation:{0u,1u})for(auto flag:{0u,1u})for(auto reasons:{0u,0x3ffffu}){
        Options o;o.entry=0x1008d450;o.unrest_candidates.resize(count);for(unsigned n=0;n<count;++n)o.unrest_candidates[n]=n+1;o.unrest_variation=variation;o.unrest_not_listed=flag;o.offer_flags=reasons;interrupt_each(o);
    }
    for(auto count:{0u,1u,2u,3u}){Options o;o.entry=0x1008d450;o.unrest_variation=1;o.offer_flags=8;o.unrest_candidates.resize(count);for(unsigned n=0;n<count;++n)o.unrest_candidates[n]=n+1;compare(o);interrupt_each(o);}
    for(auto mode:{0u,1u}){Options o;o.entry=0x1008d450;o.mode=mode;o.unrest_mutate_table=true;compare(o);interrupt_each(o);}
    Options staff_change;staff_change.entry=0x1008d450;staff_change.unrest_render_mutate_staff=true;compare(staff_change);interrupt_each(staff_change);
    Options six;six.entry=0x1008d450;six.unrest_not_listed=0;six.unrest_variation=1;six.unrest_fee=1;six.offer_flags=0;compare(six);interrupt_each(six);six.corrupt_unrest_spill=true;compare(six);
    Options pair;pair.entry=0x1008d450;pair.corrupt_offer_pair=true;compare(pair);
    compare(Options{.entry=0x1008cd20});
    for(auto position:{0u,32767u,32768u,65535u,65536u,0x80000000u,0xffffffffu})for(auto flags:{0u,1u,0x80000000u,0xffffffffu}){Options o;o.entry=0x1008cd20;o.unrest_position=position;o.offer_flags=flags;compare(o);}
    for(bool person:{false,true})for(bool attributes:{false,true})for(bool club:{false,true})for(bool manager:{false,true})for(bool contract:{false,true}){Options o;o.entry=0x1008cd20;o.contract=person;o.player=attributes;o.unrest_club=club;o.club_manager=manager;o.offer_primary=contract;compare(o);}
    for(auto listed:{0u,1u,128u,256u,0xffffffffu})for(auto human:{0u,1u,128u,256u})for(auto role:{0u,1u,2u,15u,255u})for(auto fee:{499999u,500000u,500001u}){Options o;o.entry=0x1008cd20;o.unrest_listed=listed;o.unrest_human=human;o.request_role=role;o.unrest_fee=fee;compare(o);}
    for(auto wrap:{0u,1u,2u,3u})for(auto salt:{0u,1u,20u,65535u,0x80000000u,0xffffffffu})for(auto word:{0u,1u,65535u}){Options o;o.entry=0x1008cd20;o.unrest_wrap=wrap;o.random_salt=salt;o.random_word=word;compare(o);}
    for(auto quote:{0u,10000u,10001u,199999u,200000u,499999u,500000u,999999u,1000000u,4999999u,5000000u,10000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto word:{0u,20u,40u})for(auto pattern:{0u,1u,2u}){Options o;o.entry=0x1008cd20;o.unrest_quote=quote;o.random_salt=0;o.random_word=word;o.fp_pattern=pattern;compare(o);}
    for(unsigned count:{0u,1u,2u,3u,4u,29u,30u,31u,40u})for(unsigned pattern:{0u,1u,2u,3u}){Options o;o.entry=0x1008cd20;o.unrest_reputations=pattern;o.unrest_candidates.clear();for(unsigned n=0;n<count;++n)o.unrest_candidates.push_back(n%11+1);compare(o);}
    for(auto ids:{std::vector<std::uint32_t>{1,1,2},std::vector<std::uint32_t>{1,2,1},std::vector<std::uint32_t>{1,2,2},std::vector<std::uint32_t>{0xffffffff,0,1},std::vector<std::uint32_t>{1,1,1}}){Options o;o.entry=0x1008cd20;o.unrest_reputations=1;o.unrest_candidates=ids;compare(o);interrupt_each(o);}
    for(auto mask:{0u,2u,0xfffu})for(auto accounts:{0u,2u,0xfffu})for(double balance:{-1.0,0.0,989999.0,990000.0,990001.0,2000000.0}){Options o;o.entry=0x1008cd20;o.unrest_human=1;o.unrest_listed=1;o.unrest_human_mask=mask;o.unrest_account_mask=accounts;o.unrest_balance=balance;compare(o);}
    for(auto count:{0u,1u,30u,128u,256u,0xffffffffu}){Options o;o.entry=0x1008cd20;o.recipient_count=count;compare(o);}
    Options swaps;swaps.entry=0x1008cd20;swaps.unrest_reputations=4;compare(swaps);swaps.corrupt_unrest_sort=true;compare(swaps);
    Options funds;funds.entry=0x1008cd20;funds.unrest_human=1;funds.unrest_listed=1;funds.unrest_human_mask=2;funds.unrest_candidates={1};funds.unrest_balance=990000;funds.corrupt_unrest_finance=true;compare(funds);
    Options table_change;table_change.entry=0x1008cd20;table_change.unrest_human_mask=2;table_change.unrest_mutate_table=true;compare(table_change);interrupt_each(table_change);
    Options row_change;row_change.entry=0x1008cd20;row_change.unrest_candidates={1,2};row_change.unrest_reputations=1;row_change.unrest_mutate_candidates=true;compare(row_change);interrupt_each(row_change);
    for(auto wrap:{0u,1u,2u,3u})for(bool direct:{false,true}){Options o;o.entry=0x1008cd20;o.unrest_wrap=wrap;o.unrest_human=direct;o.unrest_listed=direct;o.unrest_human_mask=0xfff;interrupt_each(o);}
    for(auto quote:{10000u,10001u,199999u,200000u,500000u,1000000u,5000000u}){Options o;o.entry=0x1008cd20;o.unrest_quote=quote;o.random_salt=20;o.random_word=0;o.unrest_random_stride=0;interrupt_each(o);}
    for(auto pattern:{0u,1u,2u}){Options o;o.entry=0x1008cd20;o.unrest_quote=1000001;o.fp_pattern=pattern;o.enable_inexact_exception=true;compare(o);}
    compare(Options{.entry=0x1008ab40});
    for(int long_gender:{-128,-1,0,127})for(int short_gender:{-128,-1,0,127})for(auto outcome:{1u,2u}){Options o;o.entry=0x1008ab40;o.first_gender=long_gender;o.short_gender=short_gender;o.offer_outcome=outcome;compare(o);}
    for(auto mode:{1u,128u}){Options o;o.entry=0x1008ab40;o.mode=mode;o.offer_marker=0;compare(o);interrupt_each(o);}
    for(auto pattern:{0u,1u,2u}){Options o;o.entry=0x1008ab40;o.fp_pattern=pattern;o.enable_inexact_exception=true;compare(o);}
    for(auto outcome:{0u,1u,2u,3u})for(auto kind:{2u,5u})for(auto marker:{0u,0x420e9u})for(auto reason:{0u,11u})for(auto renewed:{0u,1u})for(auto previous:{0u,1u})for(auto variation:{0u,1u,2u,0xffffffffu})for(bool own:{false,true}){Options o;o.entry=0x1008ab40;o.mode=0;o.offer_outcome=outcome;o.offer_kind=kind;o.offer_marker=marker;o.offer_reason=reason;o.offer_renewal=renewed;o.offer_previous=previous;o.offer_variation=variation;o.context_matches=own;compare(o);}
    for(auto flags:{0u,8u,64u,72u})for(auto manager:{1u,0xffffffffu})for(int age:{23,24,128})for(auto career:{0u,1u})for(auto renewed:{0u,1u})for(auto previous:{0u,1u})for(auto variation:{0u,1u,2u,0xffffffffu})for(auto bonus:{0u,3000000u,10000000u})for(auto year:{2002u,2003u}){Options o;o.entry=0x1008ab40;o.offer_flags=flags;o.offer_manager=manager;o.age=age;o.offer_career=career;o.offer_renewal=renewed;o.offer_previous=previous;o.offer_variation=variation;o.offer_bonus=bonus;o.year=year;compare(o);}
    for(auto flags:{0u,4u,256u,260u})for(auto manager:{1u,0xffffffffu})for(auto timing:{0u,999u,1000u})for(bool own:{false,true}){Options o;o.entry=0x1008ab40;o.offer_outcome=1;o.offer_flags=flags;o.offer_manager=manager;o.offer_timing=timing;o.context_matches=own;compare(o);interrupt_each(o);}
    for(auto mode:{0u,1u,128u,256u})for(auto outcome:{1u,2u,3u,4u,128u,256u})for(auto kind:{2u,5u})for(auto available:{0u,1u,128u,256u}){Options o;o.entry=0x1008ab40;o.mode=mode;o.offer_outcome=outcome;o.offer_kind=kind;o.name_available=available;compare(o);}
    for(auto person:{0u,2u,10u})for(auto wage:{4999u,5000u,7999u,8000u,0x80000000u,0xffffffffu})for(auto flags:{8u,64u}){Options o;o.entry=0x1008ab40;o.offer_person=person;o.wage=wage;o.offer_flags=flags;o.age=23;o.offer_renewal=0;o.offer_previous=1;o.offer_variation=2;compare(o);}
    for(auto bonus:{0u,944999u,945000u,945001u,2944999u,2945000u,2945001u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto pattern:{0u,1u,2u}){Options o;o.entry=0x1008ab40;o.offer_bonus=bonus;o.fp_pattern=pattern;compare(o);}
    for(auto day:{0u,100u,32768u,65535u})for(auto year:{0u,2001u,2002u,65535u})for(auto from:{0u,80u,32768u,65535u}){Options o;o.entry=0x1008ab40;o.day=day;o.year=year;o.offer_start_day=from;compare(o);}
    for(auto mode:{0u,1u})for(auto outcome:{0u,1u,2u,3u})for(auto flags:{8u,72u}){Options o;o.entry=0x1008ab40;o.mode=mode;o.offer_outcome=outcome;o.offer_flags=flags;interrupt_each(o);}
    for(auto selected:{0u,1u})for(bool present:{false,true})for(bool club:{false,true})for(auto manager:{0u,1u,0xffffffffu}){Options o;o.entry=0x1008ab40;o.selected_context=selected;o.context_present=present;o.context_club=club;o.offer_manager=manager;compare(o);}
    for(auto mode:{0u,1u})for(auto status:{1u,2u})for(auto renewal:{128u,256u})for(auto previous:{128u,256u})for(auto career:{128u,256u})for(auto reason:{11u,267u,0xffffffffu}){Options o;o.entry=0x1008ab40;o.mode=mode;o.offer_outcome=status;o.offer_renewal=renewal;o.offer_previous=previous;o.offer_career=career;o.offer_reason=reason;o.offer_variation=2;compare(o);}
    Options offer_spill;offer_spill.entry=0x1008ab40;offer_spill.offer_flags=8;offer_spill.offer_variation=2;offer_spill.offer_previous=1;offer_spill.year=2003;offer_spill.corrupt_offer_spill=true;compare(offer_spill);
    Options offer_pair;offer_pair.entry=0x1008ab40;offer_pair.corrupt_offer_pair=true;compare(offer_pair);
    compare(Options{.entry=0x1008a350});
    for(auto notify:{0u,1u,128u,256u,0xffffffffu})for(auto outcome:{0u,1u,2u,128u,256u})for(auto days:{0u,179u,180u,181u,32768u,65535u}){Options o;o.entry=0x1008a350;o.offer_notify=notify;o.offer_outcome=outcome;o.offer_days=days;compare(o);}
    for(auto day:{0u,99u,100u,101u,200u,201u,32768u,65535u})for(auto year:{2001u,2002u,2003u})for(auto cutoff:{0u,100u,32768u,65535u}){Options o;o.entry=0x1008a350;o.offer_notify=1;o.current_day=day;o.current_year=year;o.cutoff_day=cutoff;compare(o);}
    for(bool primary:{false,true})for(bool manager:{false,true})for(bool player:{false,true})for(bool empty:{false,true})for(auto known:{0u,1u,128u,256u}){Options o;o.entry=0x1008a350;o.offer_primary=primary;o.club_manager=manager;o.player=player;o.empty_list=empty;o.reason_known=known;compare(o);}
    for(auto origin:{0u,1u,0xffffffffu})for(bool present:{false,true}){Options o;o.entry=0x1008a350;o.offer_origin=origin;o.origin_present=present;compare(o);interrupt_each(o);}
    for(unsigned flags=0;flags<256;++flags){Options o;o.entry=0x1008a350;o.contract_flags=int(flags);compare(o);}
    for(unsigned position=0;position<4;++position)for(auto age:{34u,35u,36u,37u,38u,128u,256u}){Options o;o.entry=0x1008a350;o.keeper=o.defender=o.midfielder=o.attacker=0;o.offer_age=age;if(position==0)o.keeper=20;if(position==1)o.defender=15;if(position==2)o.midfielder=15;if(position==3)o.attacker=15;compare(o);interrupt_each(o);}
    for(bool wrap:{false,true})for(auto salt:{0u,1u,65535u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto word:{0u,1u,65535u}){Options o;o.entry=0x1008a350;o.random_wrap=wrap;o.random_salt=salt;o.random_word=word;compare(o);}
    for(auto count:{0u,1u,30u,128u,256u,0xffffffffu}){Options o;o.entry=0x1008a350;o.offer_notify=1;o.recipient_count=count;compare(o);}
    for(auto flags:{0u,2u,0x80u,0xabu,0xffu})for(auto reason:{0u,1u,127u,128u,255u,256u}){Options o;o.entry=0x1008a350;o.index_flags=flags;o.offer_reason=reason;compare(o);}
    for(auto notify:{0u,1u})for(auto date:{99u,150u,201u})for(bool wrap:{false,true}){Options o;o.entry=0x1008a350;o.offer_notify=notify;o.current_day=date;o.random_wrap=wrap;interrupt_each(o);}
    Options rng_control;rng_control.entry=0x1008a350;rng_control.random_wrap=true;rng_control.corrupt_random=true;compare(rng_control);
    Options bit_control;bit_control.entry=0x1008a350;bit_control.index_flags=2;bit_control.corrupt_index_bit=true;compare(bit_control);
    for(auto entry:{0x1008c090u,0x1008c4c0u,0x1008ca50u})for(auto mode:{0u,1u,128u,256u,0xffffffffu})for(auto name:{0u,1u,128u,256u}){Options o;o.entry=entry;o.mode=mode;o.name_available=name;compare(o);}
    for(auto mode:{0u,1u})for(int gender:{-128,-1,0,127}){Options o;o.entry=0x1008c090;o.mode=mode;o.first_gender=gender;compare(o);}
    for(auto period:{0u,1u,2u,3u,4u,5u,6u,7u,8u,0x80000000u,0xffffffffu})for(auto first:{0u,1u,128u,256u,0xffffffffu})for(auto second:{0u,1u,128u,256u}){Options o;o.period=period;o.bosman=first;o.player=second;compare(o);}
    for(auto mode:{0u,1u,128u,256u})for(auto related:{0u,1u,3u,0xfffffffeu,0xffffffffu})for(int first:{-128,-1,0,127})for(int second:{-128,0,127}){Options o;o.entry=0x1008ca50;o.mode=mode;o.related=related;o.first_gender=first;o.second_gender=second;compare(o);}
    for(auto period:{0u,1u,7u,128u,256u,0xffffffffu})for(auto bosman:{0u,1u,128u,256u})for(auto spread:{0u,1u,128u,256u})for(bool player:{false,true}){Options o;o.entry=0x1008c270;o.period=period;o.bosman=bosman;o.spread=spread;o.player=player;compare(o);}
    for(bool contract:{false,true})for(bool division:{false,true})for(bool empty:{false,true})for(auto current:{0u,1u,128u,256u})for(auto other:{0u,1u,128u,256u}){Options o;o.entry=0x1008c270;o.contract=contract;o.division=division;o.empty_list=empty;o.current_eligible=current;o.other_eligible=other;compare(o);}
    Options changed;changed.entry=0x1008c270;changed.mutate_lookup=true;compare(changed);
    for(auto entry:{0x1008c090u,0x1008c4c0u,0x1008ca50u,0x1008c270u})for(auto mode:{0u,1u}){Options o;o.entry=entry;o.mode=mode;interrupt_each(o);}
    Options null;null.entry=0x1008c270;null.contract=false;interrupt_each(null);
    Options mutation;mutation.corrupt=true;compare(mutation);mutation.corrupt=false;mutation.corrupt_period=true;compare(mutation);
    Options extra;extra.entry=0x1008ca50;extra.corrupt_extra=true;compare(extra);
    for(auto entry:{0x1008f7b0u,0x1008fcf0u,0x100913f0u,0x100918b0u})for(auto mode:{0u,1u,128u,256u,0xffffffffu})for(auto available:{0u,1u,128u,256u}){Options o;o.entry=entry;o.mode=mode;o.name_available=available;compare(o);}
    for(auto flags:{0u,1u,2u,4u,0x10u,0x14u,0x15u,0xffffffffu})for(auto playing:{0u,1u,128u,256u})for(int gender:{-128,0,127}){Options o;o.entry=0x1008f7b0;o.request_flags=flags;o.playing=playing;o.first_gender=gender;compare(o);}
    for(auto role:{0u,1u,2u,3u,4u,5u,6u,7u,8u,0x80000000u,0xffffffffu})
        for(auto before:{0u,1u,2u,3u,4u,0xffffffffu})for(auto after:{0u,1u,2u,3u,4u,0xffffffffu})
        for(auto changed:{0u,1u,256u})for(auto confused:{0u,1u,256u}){Options o;o.entry=0x1008fcf0;o.period=role;o.before=before;o.after=after;o.changed=changed;o.confused=confused;compare(o);}
    for(auto entry:{0x100913f0u,0x100918b0u})for(auto mode:{0u,1u})for(int first:{-128,-1,0,127})for(int second:{-128,0,127})for(auto related:{0u,1u,3u,256u,0xffffffffu}){Options o;o.entry=entry;o.mode=mode;o.first_gender=first;o.second_gender=second;o.related=related;compare(o);}
    for(auto entry:{0x1008fa40u,0x10091630u})for(bool contract:{false,true})for(auto count:{0u,2u,3u,4u})for(auto index:{0u,1u,2u,0xffffffffu})for(auto related:{0u,1u,3u}){Options o;o.entry=entry;o.contract=contract;o.person_count=count;o.index_row=index;o.related=related;compare(o);}
    for(auto first:{0u,1u,128u,256u})for(auto second:{0u,1u,256u})for(auto third:{0u,1u,256u}){Options o;o.entry=0x1008fa40;o.guard_first=first;o.guard_second=second;o.guard_third=third;compare(o);}
    for(unsigned flags=0;flags<256;++flags){Options o;o.entry=0x1008fa40;o.contract_flags=int(flags);compare(o);}
    for(int age:{-128,0,25,127,128,255,280})for(int status:{-128,-1,0,127,256}){Options o;o.entry=0x1008fa40;o.age=age;o.contract_status=status;compare(o);}
    Options refreshed;refreshed.entry=0x10091630;refreshed.related=1;refreshed.mutate_contract=true;compare(refreshed);interrupt_each(refreshed);
    for(auto entry:{0x1008f7b0u,0x1008fa40u,0x1008fcf0u,0x100913f0u,0x10091630u,0x100918b0u})for(auto mode:{0u,1u}){Options o;o.entry=entry;o.mode=mode;interrupt_each(o);}
    Options assert_path;assert_path.entry=0x1008fa40;assert_path.contract=false;interrupt_each(assert_path);
    Options guard;guard.entry=0x1008fa40;guard.corrupt_guard=true;compare(guard);
    Options spill;spill.entry=0x100918b0;spill.related=3;spill.corrupt_loan_spill=true;compare(spill);
    for(auto entry:{0x10090310u,0x10090560u,0x10090a00u,0x10090f20u})compare(Options{.entry=entry});
    for(bool contract:{false,true})for(auto first:{0u,1u,128u,256u})for(auto second:{0u,1u,256u})for(auto third:{0u,1u,256u}){Options o;o.entry=0x10090310;o.contract=contract;o.guard_first=first;o.guard_second=second;o.guard_third=third;compare(o);}
    for(int first:{-128,9,10,127,255})for(int second:{-128,4,5,127,255})for(int score:{-32768,4750,4751,7000,32767})for(bool player:{false,true}){Options o;o.entry=0x10090310;o.first_trait=first;o.second_trait=second;o.profile_c=score;o.player=player;o.request_role=255;compare(o);}
    for(auto mode:{0u,1u,128u,256u})for(auto validation:{0u,1u,128u,256u})for(auto pending:{0u,1u,2u,3u,4u})for(auto index:{0u,1u,0xffffffffu})for(auto count:{0u,2u,3u}){Options o;o.entry=0x10090560;o.mode=mode;o.validation=validation;o.pending_kind=pending;o.index_row=index;o.person_count=count;compare(o);}
    for(auto role:{0u,1u,2u,3u,4u,0x80000000u,0xffffffffu})for(auto agent:{0u,1u,128u,256u}){Options o;o.entry=0x10090560;o.request_role=role;o.agent=agent;compare(o);}
    for(auto day:{0u,100u,128u,32768u,65535u,65636u})for(auto year:{0u,2002u,65535u,67538u})for(auto wage:{0u,1000u,0x80000000u,0xffffffffu}){Options o;o.entry=0x10090560;o.day=day;o.year=year;o.wage=wage;o.pending_kind=0;compare(o);}
    for(auto kind:{0u,1u,5u,6u,128u,256u})for(auto important:{0u,1u,128u,256u})for(int profile:{6999,7000,7001,32767})for(int regional:{6999,7000,7001})for(bool shared:{false,true}){Options o;o.entry=0x10090a00;o.departure_kind=kind;o.important=important;o.profile_c=profile;o.profile_a=regional;o.shared_scopes=shared;compare(o);}
    for(auto kind:{2u,6u})for(auto nation:{0u,1u,2u,3u,4u})for(auto club:{0u,1u,3u})for(auto index:{0u,1u,2u}){Options o;o.entry=0x10090a00;o.departure_kind=kind;o.important=1;o.profile_c=7000;o.profile_a=7001;o.nation_scope=nation;o.current_club=club;o.index_row=index;compare(o);}
    for(bool contract:{false,true})for(auto index:{0u,0xffffffffu})for(auto count:{0u,3u}){Options o;o.entry=0x10090a00;o.contract=contract;o.index_row=index;o.person_count=count;compare(o);}
    for(auto mode:{0u,1u,128u,256u})for(auto kind:{0u,1u,5u,6u,128u,256u})for(auto important:{0u,1u,128u,256u,257u})for(bool context:{false,true})for(bool own:{false,true}){Options o;o.entry=0x10090f20;o.mode=mode;o.departure_kind=kind;o.important=important;o.context_present=context;o.context_matches=own;compare(o);}
    for(auto mode:{0u,1u})for(auto kind:{1u,5u})for(auto club:{1u,0xffffffffu})for(auto lender:{3u,0xffffffffu}){Options o;o.entry=0x10090f20;o.mode=mode;o.departure_kind=kind;o.club_id=club;o.lender_id=lender;o.important=0;o.context_matches=false;compare(o);}
    for(auto selected:{0u,1u})for(bool club:{false,true})for(int gender:{-128,0,127}){Options o;o.entry=0x10090f20;o.mode=0;o.context_matches=false;o.context_club=club;o.important=1;o.selected_context=selected;o.first_gender=gender;compare(o);}
    Options reread;reread.entry=0x10090a00;reread.departure_kind=6;reread.mutate_departure=true;compare(reread);interrupt_each(reread);
    for(auto entry:{0x10090310u,0x10090560u,0x10090a00u,0x10090f20u})for(auto mode:{0u,1u}){Options o;o.entry=entry;o.mode=mode;interrupt_each(o);}
    for(int score:{7000,7001}){Options o;o.entry=0x10090a00;o.important=1;o.profile_c=score;o.profile_a=7001;interrupt_each(o);}
    Options date;date.entry=0x10090560;date.pending_kind=0;date.corrupt_date=true;compare(date);
    Options ui_control;ui_control.entry=0x10090f20;ui_control.mode=0;ui_control.important=1;ui_control.context_matches=false;ui_control.corrupt_ui=true;compare(ui_control);
    Options new_guard;new_guard.entry=0x10090310;new_guard.corrupt_guard=true;compare(new_guard);
    const auto original_calls=contract_manager_news_calls();
    for(auto back:original_calls){
        require(continuations.contains(back),"Unreached complete news continuation "+hex_address(back));
        const bool wide=back==0x1008e630||back==0x1008e948||back==0x1008eaac;
        if(wide)require(!normal_continuations.contains(back)&&injected_continuations.contains(back),"Wide request sampler requires explicit injection "+hex_address(back));
        else require(normal_continuations.contains(back),"Missing normal-input continuation "+hex_address(back));
    }
    require(per_entry.size()==ranges.size(),"Every complete news body checked");
    for(auto [begin,end]:ranges){
        const auto count=std::count_if(continuations.begin(),continuations.end(),[&](auto back){return back>=begin&&back<end;});
        std::cout<<"news "<<hex_address(begin)<<": "<<per_entry[begin]<<" complete returns, "<<count<<" semantic continuations\n";
    }
    std::cout<<injected_returns<<" explicitly injected-bound returns (separate from "<<returned-injected_returns<<" normal returns); "<<normal_continuations.size()<<" normal continuations; "<<injected_continuations.size()<<" injected continuations.\n";
    for(auto [entry,count]:per_injected)if(count)std::cout<<"injected "<<hex_address(entry)<<": "<<count<<" returns\n";
    std::cout<<cases<<" news comparisons; "<<returned<<" complete returns; "<<interruptions<<" explicit interruptions; "<<fp_guards<<" explicit FP guards; "<<negative<<" negative controls; "<<continuations.size()<<"/"<<original_calls.size()<<" continuations. Scripted dependencies do not prove callees.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
