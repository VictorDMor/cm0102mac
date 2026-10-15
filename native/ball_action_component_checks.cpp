#include "ball_action.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include "execution-message-table.hpp"
#include "result-message-table.hpp"
#include "failed-message-tables.hpp"

namespace cm::aot { bool ball_action_preparation_reference(State&);bool ball_action_orientation_reference(State&);bool ball_action_facing_reference(State&);bool ball_action_area_reference(State&);bool ball_action_elevated_reference(State&);bool ball_action_low_reference(State&);bool ball_action_selected_reference(State&);bool ball_action_contact_reference(State&);bool ball_action_contact_resolution_reference(State&);bool ball_action_contact_outcome_reference(State&);bool ball_action_execution_reference(State&);bool ball_action_execution_message_reference(State&);bool ball_action_receiver_reference(State&);bool ball_action_receiver_outcome_reference(State&);bool ball_action_result_reference(State&);bool ball_action_result_message_reference(State&);bool ball_action_result_resolution_reference(State&); }
namespace cm::aot { bool ball_action_result_outcome_reference(State&);bool ball_action_unopposed_reference(State&); }
namespace cm::aot { bool ball_action_final_preparation_reference(State&);bool ball_action_special_effects_reference(State&);bool ball_action_regular_effects_reference(State&);bool ball_action_success_reaction_reference(State&); }
namespace cm::aot { bool ball_action_failed_preparation_reference(State&);bool ball_action_failed_message_reference(State&);bool ball_action_failed_comment_reference(State&);bool ball_action_failed_fallback_reference(State&); }
namespace cm::aot { bool ball_action_receiver_selection_reference(State&);bool ball_action_receiver_sampling_reference(State&);bool ball_action_receiver_marker_reference(State&);bool ball_action_receiver_position_reference(State&);bool ball_action_receiver_assessment_reference(State&);bool ball_action_receiver_effects_reference(State&);bool ball_action_receiver_final_reaction_reference(State&);bool ball_action_missing_receiver_reference(State&);bool ball_action_return_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace action=cm::aot::ball_action;
constexpr auto match=fixture+0x1000,actor=fixture+0x11000,doubles=fixture+0x12000;
constexpr auto singles=fixture+0x12400,stream=fixture+0x12800,globals=fixture+0x12a00;
constexpr auto recipient=actor+0x600,teams=fixture+0x800,home=fixture+0x900,away=fixture+0xa00,staff=fixture+0xb00;
constexpr auto execution_table=fixture+0x12d00;
constexpr auto result_table=fixture+0xc00;
constexpr auto restart_player=actor+0xa00;
constexpr auto actor_profile=fixture+0x400,receiver_profile=fixture+0x500;
constexpr auto failed_table=fixture+0x10000,failed_comment_table=failed_table+0x100,failed_fallback_table=failed_table+0x200;
constexpr auto diagnostics=fixture+0x700,error_buffer=fixture+0x9000,error_flag=fixture+0x9400,error_text=fixture+0x9500;
struct Options {
    unsigned interrupt=0,force_wide=0;
    unsigned adjustment=0;int adjustment_wide=0;
    bool own_negative=false,child_negative=false,mutate_fitness=false,mutate_owner=false,mutate_facing=false,mutate_area=false;
    bool mutate_elevated=false,zero_elevated_threshold=false,mutate_low=false,mutate_contact=false,mutate_resolution=false;
    std::int32_t sampled=9;
    std::int32_t facing_interval=200;
    std::uint32_t random=0xabcd5678,facing_gate=0,area_status=0,lane_clear=0,elevated_preference=0,low_alignment=1;
    std::uint32_t contact_result=1,contact_x=7,contact_y=5;
    std::uint32_t resolved_contact=1,reaction_result=1;
    std::uint32_t recipient_result=recipient,recipient_gate=1,recipient_second=1,actor_gate=1,contact_end_gate=0;
    bool mutate_outcome=false;
    std::uint32_t execution_receiver=recipient,execution_kind=45,execution_rating=200,execution_contact=1,execution_blocked=0,execution_disabled=0;
    std::uint32_t result_contact=1;
    std::uint32_t area_status_second=0;
    bool separate_area_status=false,mutate_unopposed=false;
    std::array<std::uint32_t,3> receiver_kinds{};
    std::uint32_t final_gate=1,regular_gate=1,regular_second_gate=1;
    bool mutate_final=false,zero_special_interval=false;
    std::uint32_t failed_result=1;
    std::int32_t mutate_failed_slot=0;
    std::uint32_t selection_status=0;
    bool zero_receiver_interval=false,force_missing_receiver=false,mutate_receiver_sampling=false,mutate_receiver_selection=false;
    unsigned zero_receiver_reaction=0;
    bool mutate_execution=false,mutate_table=false,mutate_result=false,mutate_result_table=false;
    double distance=25.25;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> order;
    explicit Trace(Options o):options(o){}
    void before_step(State& s,std::uint32_t pc)override {
        order.push_back(pc);
        // Signed-halfword fitness cannot naturally select the wide interval.
        // These paired boundary injections cover its dormant original body;
        // they are structural evidence, not gameplay reachability evidence.
        if(options.force_wide==1&&pc==0x1023ce80)s.wr<4>(0,65536);
        if(options.force_wide==2&&pc==0x1023ced4)s.wr<4>(0,std::uint32_t(-65536));
        if(options.force_wide==3&&pc==0x1023d3b0)s.wr<4>(0,65536);
        if(options.force_wide==4&&pc==0x1023d3c4)s.wr<4>(0,std::uint32_t(-65536));
        if(options.force_wide==5&&pc==0x1023d7ac)s.wr<4>(56,65536);
        if(options.force_wide==6&&pc==0x1023d7bc)s.wr<4>(56,std::uint32_t(-65536));
        if(options.force_wide==7&&pc==0x1023db30)s.wr<4>(68,65536);
        if(options.force_wide==8&&pc==0x1023db40)s.wr<4>(68,std::uint32_t(-65536));
        if(options.force_wide==9&&pc==0x1023dcb4)s.wr<4>(68,65536);
        if(options.force_wide==10&&pc==0x1023dcc4)s.wr<4>(68,std::uint32_t(-65536));
        if(options.force_wide==11&&pc==0x1023df40)s.wr<4>(56,65536);
        if(options.force_wide==12&&pc==0x1023df50)s.wr<4>(56,std::uint32_t(-65536));
        if(options.force_wide==13&&pc==0x1023e0b0)s.wr<4>(68,65536);
        if(options.force_wide==14&&pc==0x1023e0c0)s.wr<4>(68,std::uint32_t(-65536));
        if(options.force_wide==15&&pc==0x1023f024)s.wr<4>(72,65536);
        if(options.force_wide==16&&pc==0x1023f034)s.wr<4>(72,std::uint32_t(-65536));
        if(options.force_wide==17&&pc==0x1023f30c)s.wr<4>(72,65536);
        if(options.force_wide==18&&pc==0x1023f31c)s.wr<4>(72,std::uint32_t(-65536));
        if(options.force_wide==19&&pc==0x1023fc9c)s.wr<4>(76,65536);
        if(options.force_wide==20&&pc==0x1023fcac)s.wr<4>(76,std::uint32_t(-65536));
        if(options.force_wide==21&&pc==0x10240448)s.wr<4>(72,65536);
        if(options.force_wide==22&&pc==0x10240458)s.wr<4>(72,std::uint32_t(-65536));
        if(options.force_wide==23&&pc==0x102407c4)s.wr<4>(72,65536);
        if(options.force_wide==24&&pc==0x102407d4)s.wr<4>(72,std::uint32_t(-65536));
        if(options.force_wide==25&&pc==0x102411b8)s.wr<4>(76,65536);
        if(options.force_wide==26&&pc==0x102411c8)s.wr<4>(76,std::uint32_t(-65536));
        if(options.force_wide==27&&pc==0x10241960)s.wr<4>(76,65536);
        if(options.force_wide==28&&pc==0x10241970)s.wr<4>(76,std::uint32_t(-65536));
        if(options.force_wide==29&&pc==0x10241c48)s.wr<4>(76,65536);
        if(options.force_wide==30&&pc==0x10241c58)s.wr<4>(76,std::uint32_t(-65536));
        if(options.force_wide==31&&pc==0x10242ad8)s.wr<4>(64,65536);
        if(options.force_wide==32&&pc==0x10242ae8)s.wr<4>(64,std::uint32_t(-65536));
        if(options.force_wide==33&&pc==0x10242f8c)s.wr<4>(56,65536);
        if(options.force_wide==34&&pc==0x10242f9c)s.wr<4>(56,std::uint32_t(-65536));
        if(options.force_wide==35&&pc==0x10243258)s.wr<4>(52,65536);
        if(options.force_wide==36&&pc==0x10243268)s.wr<4>(52,std::uint32_t(-65536));
        if(options.force_wide==37&&pc==0x10243440)s.wr<4>(64,65536);
        if(options.force_wide==38&&pc==0x10243450)s.wr<4>(64,std::uint32_t(-65536));
        if(options.force_wide==39&&pc==0x10243e04)s.wr<4>(0,65536);
        if(options.force_wide==40&&pc==0x10243e4c)s.wr<4>(0,std::uint32_t(-65536));
        if(options.force_wide==41&&pc==0x10244678)s.wr<4>(64,65536);
        if(options.force_wide==42&&pc==0x10244688)s.wr<4>(64,std::uint32_t(-65536));
        if(options.force_wide==43&&pc==0x102449a8)s.wr<4>(64,65536);
        if(options.force_wide==44&&pc==0x102449b8)s.wr<4>(64,std::uint32_t(-65536));
        if(options.zero_receiver_interval&&pc==0x10243d8c)s.wr<4>(0,s.r<4>(12));
        if(options.force_missing_receiver&&pc==action::unsuccessful_action_phase)s.wr<4>(56,0);
        if((options.zero_receiver_reaction==1&&pc==0x10244638)||(options.zero_receiver_reaction==2&&pc==0x10244968))s.wr<4>(12,std::uint32_t(-70));
        if(options.zero_special_interval&&pc==0x10242a98)s.wr<4>(12,std::uint32_t(-70));
        if(options.zero_elevated_threshold&&pc==0x1023dc74)s.wr<4>(68,std::uint32_t(-70));
        if(options.adjustment_wide) {
            const auto base=action::selected_action_phase+options.adjustment*0x188;
            if(options.adjustment_wide>0&&pc==base+0x44)s.wr<4>(0,65536);
            if(options.adjustment_wide<0&&pc==base+0x58)s.wr<4>(0,std::uint32_t(-65536));
        }
        if(options.facing_interval!=200&&pc==0x1023d364)s.wr<4>(76,std::uint32_t(options.facing_interval));
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;unsigned phase;std::vector<Event> calls;
    Model(bool old,Options value,unsigned part):original(old),options(value),phase(part){}
    void reference(State& s) {
        if(phase==69){require(ball_action_return_reference(s),"Missing original ball-action return");return;}
        if(phase==21||phase==29||phase==47||phase==49||phase==51) {
            auto start=action::execution_message_phase,finish=action::receiver_resolution_phase;auto reference=ball_action_execution_message_reference;
            if(phase==29){start=action::result_message_phase;finish=action::result_resolution_phase;reference=ball_action_result_message_reference;}
            if(phase==47){start=action::failed_message_phase;finish=action::failed_comment_phase;reference=ball_action_failed_message_reference;}
            if(phase==49){start=action::failed_comment_phase;finish=action::failed_fallback_phase;reference=ball_action_failed_comment_reference;}
            if(phase==51){start=action::failed_fallback_phase;finish=action::failed_outcome_phase;reference=ball_action_failed_fallback_reference;}
            for(;;) {
                if(s.pc>=start&&s.pc<finish)
                    require(reference(s),"Missing original ball-action message");
                else if((s.pc==0x10201180||s.pc==0x1077b770||s.pc==0x10270320)&&s.r<4>(4128)>start&&s.r<4>(4128)<=finish)
                    call(s,s.pc,std::uint32_t(s.r<4>(4128)));
                else return;
            }
        }
        const auto done=phase>=69?return_sentinel:phase>=65?action::return_phase:phase>=63?action::receiver_final_reaction_phase:phase>=61?action::receiver_effects_phase:phase>=59?action::receiver_assessment_phase:phase>=55?action::receiver_effects_phase:phase>=53?action::receiver_sampling_phase:phase>=49?action::failed_outcome_phase:phase>=47?action::failed_comment_phase:phase>=45?action::failed_message_phase:phase>=43?action::return_phase:phase>=39?action::success_reaction_phase:phase>=37?action::final_effects_phase:phase>=35?action::no_receiver_phase:phase>=33?action::return_phase:phase>=31?action::result_outcome_phase:phase>=29?action::result_resolution_phase:phase>=27?action::result_message_phase:phase>=25?action::return_phase:phase>=23?action::receiver_outcome_phase:phase>=21?action::receiver_resolution_phase:phase>=19?action::execution_message_phase:phase>=17?action::return_phase:phase>=15?action::contact_outcome_phase:phase>=13?action::contact_resolution_phase:phase>=11?action::contact_selection_phase:phase>=9?action::selected_action_phase:phase>=7?action::low_ball_selection_phase:phase>=5?action::ball_state_selection_phase:phase>=3?action::area_selection_phase:phase>=1?action::action_bias_phase:action::orientation_phase;
        while(s.pc!=done&&!(phase>=5&&phase<=10&&s.pc==action::selected_action_phase)&&!(phase>=13&&phase<=18&&s.pc==action::action_execution_phase)
              &&!(phase>=19&&((phase<=68&&s.pc==action::return_phase)||(phase<=36&&s.pc==action::no_receiver_phase)||(phase<=26&&s.pc==action::execution_result_phase)||(phase>=27&&phase<=34&&s.pc==action::unopposed_result_phase)))
              &&!(phase>=37&&((phase<=44&&s.pc==action::unsuccessful_action_phase)||(phase<=40&&s.pc==action::regular_effects_phase)))
              &&!(phase>=45&&((phase<=66&&s.pc==action::missing_receiver_phase)||(phase<=52&&s.pc==action::failed_outcome_phase)||(phase<=50&&s.pc==action::failed_fallback_phase)))
              &&!(phase>=53&&((phase<=56&&s.pc==action::receiver_marker_phase)||(phase<=58&&s.pc==action::receiver_position_phase)||(phase<=62&&s.pc==action::receiver_effects_phase)))) {
            if(s.pc>=action::entry&&s.pc<action::orientation_phase)
                require(ball_action_preparation_reference(s),"Missing original ball-action preparation");
            else if(s.pc>=action::orientation_phase&&s.pc<action::action_bias_phase)
                require(ball_action_orientation_reference(s),"Missing original ball-action orientation");
            else if(s.pc>=action::action_bias_phase&&s.pc<action::area_selection_phase)
                require(ball_action_facing_reference(s),"Missing original ball-action facing");
            else if(s.pc>=action::area_selection_phase&&s.pc<action::ball_state_selection_phase)
                require(ball_action_area_reference(s),"Missing original ball-action area selection");
            else if(s.pc>=action::ball_state_selection_phase&&s.pc<action::low_ball_selection_phase)
                require(ball_action_elevated_reference(s),"Missing original ball-action elevated selection");
            else if(s.pc>=action::low_ball_selection_phase&&s.pc<action::selected_action_phase)
                require(ball_action_low_reference(s),"Missing original ball-action low-ball selection");
            else if(s.pc>=action::selected_action_phase&&s.pc<action::contact_selection_phase)
                require(ball_action_selected_reference(s),"Missing original ball-action selected-action sampling");
            else if(s.pc>=action::contact_selection_phase&&s.pc<action::contact_resolution_phase)
                require(ball_action_contact_reference(s),"Missing original ball-action contact preparation");
            else if(s.pc>=action::contact_resolution_phase&&s.pc<action::contact_outcome_phase)
                require(ball_action_contact_resolution_reference(s),"Missing original ball-action contact resolution");
            else if(s.pc>=action::contact_outcome_phase&&s.pc<action::action_execution_phase)
                require(ball_action_contact_outcome_reference(s),"Missing original ball-action contact outcome");
            else if(s.pc>=action::action_execution_phase&&s.pc<action::execution_message_phase)
                require(ball_action_execution_reference(s),"Missing original ball-action execution preparation");
            else if(s.pc>=action::execution_message_phase&&s.pc<action::receiver_resolution_phase)
                require(ball_action_execution_message_reference(s),"Missing original ball-action execution message");
            else if(s.pc>=action::receiver_resolution_phase&&s.pc<action::receiver_outcome_phase)
                require(ball_action_receiver_reference(s),"Missing original ball-action receiver resolution");
            else if(s.pc>=action::receiver_outcome_phase&&s.pc<action::execution_result_phase)
                require(ball_action_receiver_outcome_reference(s),"Missing original ball-action receiver outcome");
            else if(s.pc>=action::execution_result_phase&&s.pc<action::result_message_phase)
                require(ball_action_result_reference(s),"Missing original ball-action result preparation");
            else if(s.pc>=action::result_message_phase&&s.pc<action::result_resolution_phase)
                require(ball_action_result_message_reference(s),"Missing original ball-action result message");
            else if(s.pc>=action::result_resolution_phase&&s.pc<action::result_outcome_phase)
                require(ball_action_result_resolution_reference(s),"Missing original ball-action result resolution");
            else if(s.pc>=action::result_outcome_phase&&s.pc<action::unopposed_result_phase)
                require(ball_action_result_outcome_reference(s),"Missing original ball-action result outcome");
            else if(s.pc>=action::unopposed_result_phase&&s.pc<action::no_receiver_phase)
                require(ball_action_unopposed_reference(s),"Missing original ball-action unopposed execution");
            else if(s.pc>=action::no_receiver_phase&&s.pc<action::final_effects_phase)
                require(ball_action_final_preparation_reference(s),"Missing original ball-action final preparation");
            else if(s.pc>=action::final_effects_phase&&s.pc<action::regular_effects_phase)
                require(ball_action_special_effects_reference(s),"Missing original ball-action special effects");
            else if(s.pc>=action::regular_effects_phase&&s.pc<action::success_reaction_phase)
                require(ball_action_regular_effects_reference(s),"Missing original ball-action regular effects");
            else if(s.pc>=action::success_reaction_phase&&s.pc<action::unsuccessful_action_phase)
                require(ball_action_success_reaction_reference(s),"Missing original ball-action success reaction");
            else if(s.pc>=action::unsuccessful_action_phase&&s.pc<action::failed_message_phase)
                require(ball_action_failed_preparation_reference(s),"Missing original ball-action failed preparation");
            else if(s.pc>=action::failed_message_phase&&s.pc<action::failed_comment_phase)
                require(ball_action_failed_message_reference(s),"Missing original ball-action failed message");
            else if(s.pc>=action::failed_comment_phase&&s.pc<action::failed_fallback_phase)
                require(ball_action_failed_comment_reference(s),"Missing original ball-action failed comment");
            else if(s.pc>=action::failed_fallback_phase&&s.pc<action::failed_outcome_phase)
                require(ball_action_failed_fallback_reference(s),"Missing original ball-action failed fallback");
            else if(s.pc>=action::failed_outcome_phase&&s.pc<action::receiver_sampling_phase)
                require(ball_action_receiver_selection_reference(s),"Missing original ball-action receiver selection");
            else if(s.pc>=action::receiver_sampling_phase&&s.pc<action::receiver_marker_phase)
                require(ball_action_receiver_sampling_reference(s),"Missing original ball-action receiver sampling");
            else if(s.pc>=action::receiver_marker_phase&&s.pc<action::receiver_position_phase)
                require(ball_action_receiver_marker_reference(s),"Missing original ball-action receiver marker");
            else if(s.pc>=action::receiver_position_phase&&s.pc<action::receiver_assessment_phase)
                require(ball_action_receiver_position_reference(s),"Missing original ball-action receiver position");
            else if(s.pc>=action::receiver_assessment_phase&&s.pc<action::receiver_effects_phase)
                require(ball_action_receiver_assessment_reference(s),"Missing original ball-action receiver assessment");
            else if(s.pc>=action::receiver_effects_phase&&s.pc<action::receiver_final_reaction_phase)
                require(ball_action_receiver_effects_reference(s),"Missing original ball-action receiver effects");
            else if(s.pc>=action::receiver_final_reaction_phase&&s.pc<action::missing_receiver_phase)
                require(ball_action_receiver_final_reaction_reference(s),"Missing original ball-action receiver final reaction");
            else if(s.pc>=action::missing_receiver_phase&&s.pc<action::return_phase)
                require(ball_action_missing_receiver_reference(s),"Missing original ball-action missing receiver");
            else if(s.pc>=action::return_phase&&s.pc<action::end)
                require(ball_action_return_reference(s),"Missing original ball-action frame return");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event event{target,continuation,s.registers,{}};std::uint32_t result=0;
        switch(target) {
        case 0x10245160:
            if(options.mutate_owner)s.memory.write<4>(actor+0x1a4,match+1);
            break;
        case 0x1077b770:
            result=options.random^unsigned(options.child_negative&&!original&&phase==55);
            if(options.mutate_contact){s.memory.write<1>(std::uint32_t(s.r<4>(4))+0x41,0);s.memory.write<4>(singles+0x3c,float_bits<4>(-2));}
            if(options.mutate_receiver_sampling){s.memory.write<1>(recipient+0x10f,255);s.memory.write<1>(recipient+0x110,128);}
            break;
        case 0x10226520:result=std::uint32_t(options.sampled);break;
        case 0x10775540: {
            const auto input=std::uint32_t(s.r<4>(12));result=std::bit_cast<std::int32_t>(input)<0?0u-input:input;
            if(options.child_negative&&!original)result^=1;
            break;
        }
        case 0x101d8d70:
            result=options.facing_gate;if(options.child_negative&&!original)result^=1;
            if(options.mutate_facing){s.memory.write<1>(actor+0x106,10);s.memory.write<1>(match+0x791f,3);}
            break;
        case 0x10237830:
            s.memory.write<2>(std::uint32_t(s.r<4>(12))+0x19e,(((s.r<4>(24)&255)<<8)|(s.r<4>(28)&255))^unsigned(options.child_negative&&!original&&phase==59));
            break;
        case 0x101e3d50:
            result=continuation==0x102425cc&&options.separate_area_status?options.area_status_second:options.area_status;
            if(options.child_negative&&!original)result^=phase==35?0x10u:1u;
            if(options.mutate_area){s.memory.write<1>(actor+0x105,0);s.memory.write<1>(actor+0x27,1);}
            if(options.mutate_unopposed&&continuation==0x102425b8){s.memory.write<4>(actor+0x1a4,match+4);s.memory.write<2>(recipient+0x36,65535);}
            break;
        case 0x101e4ed0:
            result=options.lane_clear;
            if(options.mutate_elevated){s.memory.write<1>(actor+0x10f,128);s.memory.write<4>(actor+0x1a4,match+1);}
            break;
        case 0x102875f0:
            result=options.elevated_preference;if(options.child_negative&&!original)result^=1;
            if(options.mutate_elevated){s.memory.write<1>(actor+0x24,128);s.memory.write<1>(actor+0x106,10);}
            break;
        case 0x10244c60:
            result=options.low_alignment;if(options.child_negative&&!original)result^=1;
            if(options.mutate_low){s.memory.write<2>(actor+0x194,32768);s.memory.write<2>(actor+0x136,0);}
            break;
        case 0x10239590: {
            auto bits=std::uint32_t(float_bits<4>(float_value<8>(s.r<8>(0x4010))));
            if(options.child_negative&&!original)bits^=1;
            s.memory.write<4>(std::uint32_t(s.r<4>(12))+0x50,bits);break;
        }
        case 0x101f50c0:
            s.memory.write<1>(std::uint32_t(s.r<4>(24)),options.contact_x);s.memory.write<1>(std::uint32_t(s.r<4>(28)),options.contact_y);
            result=options.contact_result;if(options.child_negative&&!original)result^=1;
            break;
        case 0x101d68f0:
            s.memory.write<4>(globals+0x20,s.r<4>(16));s.memory.write<4>(globals+0x24,s.r<4>(32));break;
        case 0x101f5180:
            s.memory.write<1>(std::uint32_t(s.r<4>(20)),options.resolved_contact);
            s.memory.write<4>(std::uint32_t(s.r<4>(36)),std::uint32_t(float_bits<4>(12.5))^unsigned(options.child_negative&&!original));
            break;
        case 0x10201180:
            s.memory.write<4>(globals+0x40+std::uint32_t(calls.size())*4,s.r<4>(16)^unsigned(options.child_negative&&!original&&phase>=21));
            if(options.mutate_resolution&&s.r<4>(16)==0x1fe8) {
                s.memory.write<1>(std::uint32_t(s.r<4>(4))+0x41,6);s.memory.write<4>(actor+0x1a4,match+4);
                s.memory.write<1>(actor+0x27,1);s.memory.write<1>(actor+0x105,4);s.memory.write<1>(actor+0x106,10);
            }
            if(options.mutate_outcome) {
                s.memory.write<1>(std::uint32_t(s.r<4>(4))+0x41,7);s.memory.write<1>(actor+0x27,1);
                s.memory.write<1>(recipient+0x106,0);s.memory.write<4>(actor+0x1a4,match+4);
            }
            if(options.mutate_execution)s.memory.write<1>(std::uint32_t(s.r<4>(4))+0x40,0);
            break;
        case 0x10288f30:result=options.reaction_result;break;
        case 0x1022ba10:s.memory.write<4>(std::uint32_t(s.r<4>(12))+0x54,(0x12340000|s.r<4>(16))^unsigned(options.child_negative&&!original&&(phase==23||phase==31||phase==63)));break;
        case 0x101f1530:s.memory.write<4>(std::uint32_t(s.r<4>(16))+0x5c,0x34560000|s.r<4>(20));break;
        case 0x101f3a30:result=options.recipient_result;break;
        case 0x102720a0:result=options.actor_gate;break;
        case 0x10272150:
            result=std::any_of(calls.begin(),calls.end(),[](const Event& e){return e.target==0x10272150;})?options.recipient_second:options.recipient_gate;
            break;
        case 0x101ee530:
            result=options.contact_end_gate;s.memory.write<4>(globals+0x200,s.r<4>(16)^unsigned(options.child_negative&&!original));break;
        case 0x101d6780:s.memory.write<4>(globals+0x208,float_bits<4>(float_value<8>(s.r<8>(0x4010))));break;
        case 0x102884d0:s.memory.write<4>(std::uint32_t(s.r<4>(12))+0x58,0x56780000|s.r<4>(16));break;
        case 0x101eaf60:
            s.memory.write<1>(std::uint32_t(s.r<4>(28)),7u^unsigned(options.child_negative&&!original));
            s.memory.write<1>(std::uint32_t(s.r<4>(32)),5);s.memory.write<2>(std::uint32_t(s.r<4>(36)),options.execution_rating);
            s.memory.write<1>(std::uint32_t(s.r<4>(40)),options.execution_kind);
            s.memory.write<1>(std::uint32_t(s.memory.read<4>(std::uint32_t(s.r<4>(4))+0x38)),options.execution_disabled);
            result=options.execution_receiver;break;
        case 0x101f5350:
            s.memory.write<4>(globals+0x220,s.r<4>(24));
            if(options.mutate_execution){s.memory.write<4>(actor+0x1a4,match+4);s.memory.write<1>(actor+0x27,1);s.memory.write<1>(recipient+0x106,1);}
            break;
        case 0x101f5050:
            s.memory.write<1>(std::uint32_t(s.r<4>(32)),options.contact_x);s.memory.write<1>(std::uint32_t(s.r<4>(36)),options.contact_y);
            s.memory.write<1>(std::uint32_t(s.r<4>(40)),options.execution_blocked);
            result=continuation>=action::execution_result_phase?options.result_contact:options.execution_contact;
            if(options.child_negative&&!original&&phase==27)result^=1;
            if(options.mutate_result){s.memory.write<1>(actor+0x27,1);s.memory.write<1>(actor+0x106,10);s.memory.write<2>(recipient+0x32,65516);s.memory.write<1>(match+0x7931,1);}
            break;
        case 0x101f5110:
            s.memory.write<1>(std::uint32_t(s.r<4>(24)),options.resolved_contact);
            s.memory.write<4>(std::uint32_t(s.r<4>(40)),float_bits<4>(12.5));
            if(options.mutate_execution){s.memory.write<1>(std::uint32_t(s.r<4>(4))+0x48,1);s.memory.write<4>(toc-0x5204,execution_table+0x200);}
            if(options.mutate_result)s.memory.write<4>(toc-0x5208,result_table+0x200);
            break;
        case 0x1022e3a0:
            s.memory.write<4>(globals+0x260,s.r<4>(12)^unsigned(options.child_negative&&!original&&(phase==37||phase==53)));
            if(options.mutate_final){s.memory.write<4>(actor+0x1a4,match+4);s.memory.write<2>(actor+0x36,65535);s.memory.write<1>(recipient+0x2c,255);}
            break;
        case 0x10289560: {
            const auto index=std::min<std::size_t>(2,std::count_if(calls.begin(),calls.end(),[](const Event& e){return e.target==0x10289560;}));
            result=options.receiver_kinds[index];s.memory.write<4>(globals+0x264,s.r<4>(12)^unsigned(options.child_negative&&!original&&(phase==39||phase==63)));
            if(options.mutate_final){s.memory.write<4>(actor+0x70,receiver_profile+1);s.memory.write<2>(recipient+0x36,65535);}
            break;
        }
        case 0x10270ba0:result=options.final_gate;break;
        case 0x10272050:result=options.regular_gate;s.memory.write<4>(globals+0x268,s.r<4>(12)^unsigned(options.child_negative&&!original&&phase==41));break;
        case 0x102721a0:result=options.regular_second_gate;break;
        case 0x102775e0:s.memory.write<4>(globals+0x26c,s.r<4>(12)^unsigned(options.child_negative&&!original&&phase==43));break;
        case 0x102865a0:s.memory.write<4>(globals+0x270,float_bits<4>(float_value<8>(s.r<8>(0x4010))));break;
        case 0x10270320:result=options.failed_result^unsigned(options.child_negative&&!original&&phase==51);break;
        case 0x101f1100:
            s.memory.write<1>(std::uint32_t(s.r<4>(12))+0x7928,options.selection_status);
            if(options.mutate_receiver_selection){s.memory.write<4>(actor+0x1a4,match+4);s.memory.write<1>(match+4+0x7928,1);}
            break;
        case 0x10288230:s.memory.write<4>(globals+0x278,s.r<4>(12)^unsigned(options.child_negative&&!original&&phase==57));break;
        case 0x10739890:
            s.memory.write<4>(std::uint32_t(s.r<4>(24)),0x12345678u^unsigned(options.child_negative&&!original&&phase==67));s.memory.write<4>(std::uint32_t(s.r<4>(28)),0x87654321);break;
        case 0x10768810:result=error_text;break;
        case 0x1077b720:s.memory.write<1>(std::uint32_t(s.r<4>(12)),0x45);break;
        case 0x10769b60:s.memory.write<4>(globals+0x27c,s.r<4>(16));break;
        default:throw std::runtime_error("Unknown ball-action dependency "+hex_address(target));
        }
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
        if(target==0x10245160)s.wr<8>(0x4010,float_bits<8>(options.distance+unsigned(options.child_negative&&!original)));
        s.pc=continuation;event.after=s.registers;calls.push_back(std::move(event));
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture ball-action dependency interrupted");
    }
};
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x13000);s->memory.map(stack-0x800,0xa00);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,actor);s->wr<4>(16,700);s->wr<4>(20,0x123456ff);s->wr<4>(24,8);
    s->wr<4>(4128,return_sentinel);s->pc=action::entry;
    for(auto [offset,value]:std::array<std::pair<int,unsigned>,6>{{{-0x51e8,doubles},{-0x51e0,singles},
        {-0x7b90,globals},{-0x7b8c,stream},{-0x7b88,globals+4},{-0x7b84,globals+8}}})s->memory.write<4>(toc+offset,value);
    s->memory.write<4>(actor+0x1a4,match);s->memory.write<4>(actor+0x50,float_bits<4>(7500));
    s->memory.write<4>(actor+0x6c,actor+0x300);s->memory.write<1>(actor+0x368,2);
    s->memory.write<1>(match+0x791d,4);s->memory.write<1>(match+0x791e,7);s->memory.write<1>(match+0x791f,2);
    s->memory.write<4>(globals,17);s->memory.write<4>(globals+4,stream+126);s->memory.write<4>(globals+8,stream);
    for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,100+n);
    s->memory.write<8>(doubles,float_bits<8>(65535));s->memory.write<8>(doubles+0x80,float_bits<8>(4503601774854144.0));
    return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;
std::array<unsigned,71> phase_cases{},phase_completed{};
std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={},unsigned phase=0) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    if(options.mutate_fitness) {
        auto edit=[](State& s,Effects& e,std::uint32_t at,unsigned){if(at==actor+0x50&&!e.mutated){e.mutated=true;s.memory.write<4>(at,float_bits<4>(-32768));}};
        x.on_read=edit;y.on_read=edit;
    }
    if(options.mutate_table) {
        auto edit=[](State& s,Effects& e,std::uint32_t at,unsigned){if(at==toc-0x5204&&!e.mutated){e.mutated=true;s.memory.write<4>(at,execution_table+0x200);}};
        x.on_read=edit;y.on_read=edit;
    }
    if(options.mutate_result_table) {
        auto edit=[](State& s,Effects& e,std::uint32_t at,unsigned){if(at==toc-0x5208&&!e.mutated){e.mutated=true;s.memory.write<4>(at,result_table+0x200);}};
        x.on_read=edit;y.on_read=edit;
    }
    if(options.mutate_failed_slot) {
        auto edit=[slot=options.mutate_failed_slot](State& s,Effects& e,std::uint32_t at,unsigned){if(at==toc+slot&&!e.mutated){e.mutated=true;s.memory.write<4>(at,(slot==-0x520c?failed_table:slot==-0x5210?failed_comment_table:failed_fallback_table)+0x400);}};
        x.on_read=edit;y.on_read=edit;
    }
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_trace(options),new_trace(options);
    Model old(true,options,phase),modern(false,options,phase);
    const auto run=[&](State& s,Model& m,bool original){try{if(original){original_trace=&old_trace;m.reference(s);}else {
            if(phase==70)action::act(s,&m,&new_trace);
            else {
            const bool prefix=phase>0&&phase%2==0;
            if(phase==0||prefix)action::prepare(s,&m,&new_trace);
            if(phase==1||prefix)action::orient(s,&m,&new_trace);
            if(phase==3||(prefix&&phase>=4))action::face_action(s,&m,&new_trace);
            if(phase==5||(prefix&&phase>=6))action::select_area_action(s,&m,&new_trace);
            if((phase==7||(prefix&&phase>=8))&&s.pc!=action::selected_action_phase)action::select_elevated_action(s,&m,&new_trace);
            if(phase==9||(prefix&&phase>=10&&s.pc==action::low_ball_selection_phase))action::select_low_ball_action(s,&m,&new_trace);
            if(phase==11||(prefix&&phase>=12))action::sample_selected_action(s,&m,&new_trace);
            if(phase==13||(prefix&&phase>=14))action::prepare_contact(s,&m,&new_trace);
            if((phase==15||(prefix&&phase>=16))&&s.pc!=action::action_execution_phase)action::resolve_contact(s,&m,&new_trace);
            if((phase==17||(prefix&&phase>=18))&&s.pc==action::contact_outcome_phase)action::complete_contact(s,&m,&new_trace);
            if((phase==19||(prefix&&phase>=20))&&s.pc==action::action_execution_phase)action::prepare_execution(s,&m,&new_trace);
            if(phase==21||(prefix&&phase>=22&&s.pc==action::execution_message_phase))action::describe_execution(s,&m,&new_trace);
            if((phase==23||(prefix&&phase>=24))&&s.pc==action::receiver_resolution_phase)action::resolve_receiver(s,&m,&new_trace);
            if((phase==25||(prefix&&phase>=26))&&s.pc==action::receiver_outcome_phase)action::complete_receiver_contact(s,&m,&new_trace);
            if((phase==27||(prefix&&phase>=28))&&s.pc==action::execution_result_phase)action::prepare_execution_result(s,&m,&new_trace);
            if(phase==29||(prefix&&phase>=30&&s.pc==action::result_message_phase))action::describe_execution_result(s,&m,&new_trace);
            if((phase==31||(prefix&&phase>=32))&&s.pc==action::result_resolution_phase)action::resolve_execution_result(s,&m,&new_trace);
            if((phase==33||(prefix&&phase>=34))&&s.pc==action::result_outcome_phase)action::complete_execution_result(s,&m,&new_trace);
            if((phase==35||(prefix&&phase>=36))&&s.pc==action::unopposed_result_phase)action::complete_unopposed_execution(s,&m,&new_trace);
            if((phase==37||(prefix&&phase>=38))&&s.pc==action::no_receiver_phase)action::prepare_final_action(s,&m,&new_trace);
            if((phase==39||(prefix&&phase>=40))&&s.pc==action::final_effects_phase)action::complete_special_action(s,&m,&new_trace);
            if((phase==41||(prefix&&phase>=42))&&s.pc==action::regular_effects_phase)action::complete_regular_action(s,&m,&new_trace);
            if((phase==43||(prefix&&phase>=44))&&s.pc==action::success_reaction_phase)action::react_to_completed_action(s,&m,&new_trace);
            if((phase==45||(prefix&&phase>=46))&&s.pc==action::unsuccessful_action_phase)action::prepare_failed_action(s,&m,&new_trace);
            if(phase==47||(prefix&&phase>=48&&s.pc==action::failed_message_phase))action::describe_failed_action(s,&m,&new_trace);
            if(phase==49||(prefix&&phase>=50&&s.pc==action::failed_comment_phase))action::describe_failed_comment(s,&m,&new_trace);
            if(phase==51||(prefix&&phase>=52&&s.pc==action::failed_fallback_phase))action::describe_failed_fallback(s,&m,&new_trace);
            if((phase==53||(prefix&&phase>=54))&&s.pc==action::failed_outcome_phase)action::prepare_receiver_outcome(s,&m,&new_trace);
            if((phase==55||(prefix&&phase>=56))&&s.pc==action::receiver_sampling_phase)action::sample_receiver_counter(s,&m,&new_trace);
            if((phase==57||(prefix&&phase>=58))&&s.pc==action::receiver_marker_phase)action::handle_receiver_marker(s,&m,&new_trace);
            if((phase==59||(prefix&&phase>=60))&&s.pc==action::receiver_position_phase)action::position_receiver(s,&m,&new_trace);
            if((phase==61||(prefix&&phase>=62))&&s.pc==action::receiver_assessment_phase)action::assess_receiver(s,&m,&new_trace);
            if((phase==63||(prefix&&phase>=64))&&s.pc==action::receiver_effects_phase)action::complete_receiver_outcome(s,&m,&new_trace);
            if((phase==65||(prefix&&phase>=66))&&s.pc==action::receiver_final_reaction_phase)action::react_to_receiver_outcome(s,&m,&new_trace);
            if((phase==67||(prefix&&phase>=68))&&s.pc==action::missing_receiver_phase)action::report_missing_receiver(s,&m,&new_trace);
            if((phase==69||(prefix&&phase>=70))&&s.pc==action::return_phase)action::return_action_frame(s,&m,&new_trace);
            }
        }}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Ball-action phase="+std::to_string(phase)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown ball-action dependency"),expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative){
        std::cerr<<"Input "<<hex_address(input.pc)<<" budget "<<input.budget<<"; final "<<hex_address(a->pc)<<" / "<<hex_address(b->pc)
            <<"; steps "<<old_trace.order.size()<<" / "<<new_trace.order.size()<<"; calls "<<old.calls.size()<<" / "<<modern.calls.size()<<'\n';
        throw;}rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected) {
        require(x.bytes==y.bytes,label+" ordered effects");
        if(a->executed!=b->executed) {
            std::cerr<<label<<" accounting "<<a->executed<<" / "<<b->executed<<"; sites "<<old_trace.order.size()<<" / "<<new_trace.order.size()<<'\n';
            for(unsigned n=0;n<std::min(old_trace.order.size(),new_trace.order.size());++n)if(old_trace.order[n]!=new_trace.order[n]){
                std::cerr<<"First accounting site "<<hex_address(old_trace.order[n])<<" / "<<hex_address(new_trace.order[n])<<'\n';break;}
        }
        require(a->executed==b->executed,label+" accounting");
        require(old.calls==modern.calls,label+" dependency ABI");
        if(old_trace.order!=new_trace.order)for(unsigned n=0;n<std::min(old_trace.order.size(),new_trace.order.size());++n)if(old_trace.order[n]!=new_trace.order[n]){
            std::cerr<<"First site "<<hex_address(old_trace.order[n])<<" / "<<hex_address(new_trace.order[n])<<'\n';break;}
        require(old_trace.order==new_trace.order,label+" original-site order");
    }
    ++cases;++phase_cases[phase];phase_completed[phase]+=expected.empty()&&!rejected;
    negatives+=rejected;interruptions+=!expected.empty();structural+=options.force_wide!=0||options.facing_interval!=200||options.zero_elevated_threshold||options.adjustment_wide!=0||options.zero_special_interval||options.zero_receiver_interval||options.zero_receiver_reaction||options.force_missing_receiver;
    completed+=expected.empty()&&!rejected;sites.insert(old_trace.order.begin(),old_trace.order.end());
    return {std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={},unsigned phase=0) {
    const auto baseline=compare(input,options,phase);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,options,phase);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes) {
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}
        compare(*s,options,phase);
    }
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o,phase);}
}
std::unique_ptr<State> orientation_seed() {
    auto s=seed();s->pc=action::orientation_phase;s->wr<4>(4,stack-0x150);s->wr<4>(60,actor);s->wr<4>(52,117);
    s->wr<4>(64,700);s->wr<4>(88,2);s->wr<4>(104,doubles);s->wr<4>(108,globals);s->wr<4>(112,stream);
    s->wr<4>(116,globals+4);s->wr<4>(120,globals+8);s->wr<4>(124,singles);
    s->memory.write<2>(actor+0x19e,30);s->memory.write<2>(match+0x7950,10);s->memory.write<1>(match+0x7920,2);
    s->memory.write<1>(match+1,2);return s;
}
std::unique_ptr<State> facing_seed() {
    auto s=orientation_seed();s->pc=action::action_bias_phase;s->wr<4>(52,20);s->wr<4>(68,5);s->wr<4>(72,8);
    s->wr<8>(0x4000+29*16,float_bits<8>(25.25));s->memory.write<8>(doubles+0xc8,float_bits<8>(10));
    s->memory.write<1>(actor+0x10f,2);s->memory.write<1>(actor+0x108,3);s->memory.write<1>(actor+0x27,0);
    s->memory.write<1>(actor+0x105,4);s->memory.write<1>(actor+0x106,6);s->memory.write<4>(match+0xdff8,actor+0x200);
    return s;
}
std::unique_ptr<State> area_seed() {
    auto s=facing_seed();s->pc=action::area_selection_phase;s->memory.write<1>(actor+0x106,1);
    s->memory.write<4>(actor+0x6c,actor+0x300);s->memory.write<1>(actor+0x368,2);
    s->memory.write<2>(actor + 0x11e + 37*2,0x8765);return s;
}
std::unique_ptr<State> elevated_seed() {
    auto s=area_seed();s->pc=action::ball_state_selection_phase;s->wr<4>(88,4);s->memory.write<1>(actor+0x24,2);
    for(auto kind:{12u,21u,46u})s->memory.write<2>(actor + 0x11e + kind*2,0x8000+kind);
    return s;
}
std::unique_ptr<State> low_seed() {
    auto s=elevated_seed();s->pc=action::low_ball_selection_phase;s->wr<4>(88,2);
    s->memory.write<1>(actor+0x106,6);s->memory.write<2>(actor+0x194,200);s->memory.write<2>(actor+0x136,2);
    s->memory.write<2>(actor+0x138,0xcfc7);s->memory.write<4>(actor+0x9c,float_bits<4>(20));
    s->memory.write<8>(doubles+0x328,float_bits<8>(50));s->memory.write<4>(singles,float_bits<4>(1));return s;
}
std::unique_ptr<State> selected_seed() {
    auto s=low_seed();s->pc=action::selected_action_phase;s->wr<4>(52,12);s->wr<4>(68,200);s->wr<4>(64,700);
    s->wr<4>(84,117);s->wr<4>(80,88);s->memory.write<4>(singles+0xe0,float_bits<4>(0.5));return s;
}
std::unique_ptr<State> contact_seed() {
    auto s=selected_seed();s->pc=action::contact_selection_phase;s->memory.write<4>(singles,float_bits<4>(0));
    s->memory.write<4>(singles+0x3c,float_bits<4>(0));s->memory.write<4>(singles+0x2c,float_bits<4>(20));return s;
}
std::unique_ptr<State> resolution_seed() {
    auto s=contact_seed();s->pc=action::contact_resolution_phase;s->memory.write<1>(actor+0x106,1);
    s->memory.write<1>(actor+0x2f,12);s->memory.write<2>(actor+0x36,250);s->memory.write<1>(actor+0x366,2);
    s->memory.write<4>(match+0x79f4,globals+0x100);s->memory.write<4>(match+0x79f8,globals+0x180);
    s->memory.write<1>(stack-0x150+0x41,1);s->memory.write<1>(stack-0x150+0x43,1);s->memory.write<1>(stack-0x150+0x44,7);
    return s;
}
std::unique_ptr<State> outcome_seed() {
    auto s=resolution_seed();s->pc=action::contact_outcome_phase;s->wr<4>(64,0xffffffff);s->wr<4>(68,0x1f79);
    s->memory.write<1>(stack-0x150+0x42,0);s->memory.write<4>(stack-0x150+0x4c,float_bits<4>(12.5));
    s->memory.write<1>(recipient+0x2f,23);s->memory.write<1>(recipient+0x105,4);s->memory.write<1>(recipient+0x106,6);
    s->memory.write<4>(match+0x3118,teams);s->memory.write<4>(match+0x311c,teams);
    s->memory.write<4>(teams+0x1c,home);s->memory.write<4>(teams+0x20,away);
    s->memory.write<4>(home+0x2c,staff);s->memory.write<4>(away+0x2c,staff);s->memory.write<1>(staff+0x119,8);
    return s;
}
std::unique_ptr<State> execution_seed() {
    auto s=outcome_seed();s->pc=action::action_execution_phase;s->wr<4>(68,200);s->wr<4>(52,0xabcd002d);
    s->memory.write<1>(match+0x7931,1);s->memory.write<1>(match+0x7935,1);
    s->memory.write<1>(recipient+0x27,1);s->memory.write<1>(recipient+0x106,10);s->memory.write<2>(recipient+0x32,20);
    s->memory.write<4>(toc-0x5204,execution_table);
    for(unsigned n=0;n<47;++n){s->memory.write<4>(execution_table+n*4,original_execution_message_table[n]);s->memory.write<4>(execution_table+0x200+n*4,original_execution_message_table[46-n]);}
    return s;
}
std::unique_ptr<State> execution_message_seed() {
    auto s=execution_seed();s->pc=action::execution_message_phase;s->wr<4>(56,recipient);s->memory.write<1>(stack-0x150+0x48,45);return s;
}
std::unique_ptr<State> receiver_seed() {
    auto s=execution_message_seed();s->pc=action::receiver_resolution_phase;s->memory.write<1>(actor+0x106,10);
    s->memory.write<1>(stack-0x150+0x40,2);s->memory.write<1>(stack-0x150+0x43,1);
    s->memory.write<2>(recipient+0x36,400);s->memory.write<4>(recipient+0x6c,recipient+0x300);s->memory.write<1>(recipient+0x366,0);
    return s;
}
std::unique_ptr<State> receiver_outcome_seed() {
    auto s=receiver_seed();s->pc=action::receiver_outcome_phase;s->wr<4>(64,0xffffffff);s->wr<4>(68,0x1f79);
    s->memory.write<1>(restart_player+0x2f,34);s->memory.write<1>(restart_player+0x27,2);s->memory.write<1>(recipient+0x16,0x5a);
    return s;
}
Options receiver_outcome_options() {Options o;o.recipient_result=restart_player;return o;}
std::unique_ptr<State> result_seed() {
    auto s=receiver_outcome_seed();s->pc=action::execution_result_phase;s->wr<4>(52,45);s->wr<4>(68,200);
    s->memory.write<1>(actor+0x106,1);s->memory.write<1>(stack-0x150+0x40,0);s->memory.write<2>(stack-0x150+0x4a,100);
    s->memory.write<2>(recipient+0x32,65516);s->memory.write<4>(toc-0x5208,result_table);
    for(unsigned n=0;n<57;++n){s->memory.write<4>(result_table+n*4,original_result_message_table[n]);s->memory.write<4>(result_table+0x200+n*4,original_result_message_table[56-n]);}
    return s;
}
std::unique_ptr<State> result_message_seed() {
    auto s=result_seed();s->pc=action::result_message_phase;return s;
}
std::unique_ptr<State> result_resolution_seed() {
    auto s=result_seed();s->pc=action::result_resolution_phase;s->memory.write<1>(stack-0x150+0x40,2);s->memory.write<1>(actor+0x366,0);return s;
}
std::unique_ptr<State> result_outcome_seed() {
    auto s=result_resolution_seed();s->pc=action::result_outcome_phase;s->wr<4>(64,0x1ff9);s->wr<4>(68,0x1f79);return s;
}
std::unique_ptr<State> unopposed_seed() {
    auto s=result_seed();s->pc=action::unopposed_result_phase;s->wr<4>(88,2);s->memory.write<1>(stack-0x150+0x42,1);
    s->memory.write<1>(stack-0x150+0x45,0);s->memory.write<1>(stack-0x150+0xd0,0x5a);s->memory.write<1>(recipient+4,255);return s;
}
std::unique_ptr<State> final_seed() {
    auto s=result_seed();s->pc=action::no_receiver_phase;s->wr<4>(64,200);s->wr<4>(68,300);s->wr<4>(88,2);s->wr<4>(92,0);s->wr<4>(96,actor);
    s->memory.write<8>(doubles+0x330,float_bits<8>(10));s->memory.write<4>(match+0x3108,0x7ffffffe);s->memory.write<4>(match+0x310c,0xfffffffe);
    s->memory.write<1>(stack-0x150+0x46,10);s->memory.write<1>(recipient+0x2c,250);s->memory.write<2>(actor+0x1a0,0);
    s->memory.write<2>(actor+0x32,65516);s->memory.write<1>(actor+7,255);s->memory.write<1>(actor+8,255);s->memory.write<1>(actor+9,255);s->memory.write<1>(recipient+7,255);
    s->memory.write<4>(actor+0x70,actor_profile);s->memory.write<4>(recipient+0x70,receiver_profile);s->memory.write<1>(actor_profile+0x2e,40);s->memory.write<1>(receiver_profile+0x2e,40);
    s->memory.write<1>(actor+0x106,1);s->memory.write<4>(singles+0x18,float_bits<4>(0.75));return s;
}
std::unique_ptr<State> special_effects_seed() {auto s=final_seed();s->pc=action::final_effects_phase;s->wr<4>(88,4);return s;}
std::unique_ptr<State> regular_effects_seed() {auto s=final_seed();s->pc=action::regular_effects_phase;s->wr<4>(88,1);return s;}
std::unique_ptr<State> success_reaction_seed() {auto s=final_seed();s->pc=action::success_reaction_phase;s->wr<4>(88,4);s->memory.write<2>(actor+0x1a0,200);return s;}
std::unique_ptr<State> failed_seed() {
    auto s=final_seed();s->pc=action::unsuccessful_action_phase;s->memory.write<1>(recipient+0x105,4);s->memory.write<1>(recipient+0x106,10);
    s->memory.write<2>(recipient+0x32,65516);s->memory.write<4>(globals,18);s->memory.write<1>(stack-0x150+0x48,45);
    s->memory.write<4>(toc-0x520c,failed_table);s->memory.write<4>(toc-0x5210,failed_comment_table);s->memory.write<4>(toc-0x5214,failed_fallback_table);
    const auto copy=[&](auto table,const auto& values){for(unsigned n=0;n<values.size();++n){s->memory.write<4>(table+n*4,values[n]);s->memory.write<4>(table+0x400+n*4,values[values.size()-1-n]);}};
    copy(failed_table,original_failed_action_table);copy(failed_comment_table,original_failed_comment_table);copy(failed_fallback_table,original_failed_fallback_table);
    s->memory.write<4>(singles+0xe4,float_bits<4>(1));s->memory.write<4>(recipient+0xb0,float_bits<4>(2));s->memory.write<4>(actor+0xb0,float_bits<4>(5));return s;
}
std::unique_ptr<State> failed_message_seed() {auto s=failed_seed();s->pc=action::failed_message_phase;return s;}
std::unique_ptr<State> failed_comment_seed() {auto s=failed_seed();s->pc=action::failed_comment_phase;return s;}
std::unique_ptr<State> failed_fallback_seed() {auto s=failed_seed();s->pc=action::failed_fallback_phase;return s;}
std::unique_ptr<State> receiver_selection_seed() {
    auto s=failed_seed();s->pc=action::failed_outcome_phase;s->memory.write<4>(recipient+0x1a4,match);s->memory.write<1>(recipient+0x10f,3);s->memory.write<1>(recipient+0x110,4);
    s->memory.write<1>(stack-0x150+0xd0,1);s->memory.write<1>(stack-0x150+0x47,7);s->memory.write<1>(actor+0x2c,250);
    s->memory.write<1>(recipient+4,10);s->memory.write<1>(recipient+5,8);s->memory.write<1>(recipient+6,2);s->memory.write<1>(recipient+7,10);s->memory.write<1>(recipient+8,8);s->memory.write<1>(recipient+9,2);
    s->memory.write<2>(recipient+0x36,7000);s->memory.write<1>(recipient+0x100,0);s->memory.write<2>(match+0x7948,1100);s->memory.write<8>(doubles+0xf0,float_bits<8>(4503599627370496.0));
    s->memory.write<8>(doubles+0x288,float_bits<8>(0.5));s->memory.write<1>(match+0xe03d,0);s->memory.write<1>(match+0xe073,0);s->memory.write<1>(match+0x791f,2);s->memory.write<1>(match+0x7928,0);
    s->memory.write<4>(toc-0x51e4,diagnostics);s->memory.write<4>(toc-0x7ba8,error_buffer);s->memory.write<4>(toc-0x7bac,error_flag);s->memory.write<1>(error_flag,1);return s;
}
std::unique_ptr<State> receiver_sampling_seed() {auto s=receiver_selection_seed();s->pc=action::receiver_sampling_phase;return s;}
std::unique_ptr<State> receiver_marker_seed() {auto s=receiver_selection_seed();s->pc=action::receiver_marker_phase;return s;}
std::unique_ptr<State> receiver_position_seed() {auto s=receiver_selection_seed();s->pc=action::receiver_position_phase;return s;}
std::unique_ptr<State> receiver_assessment_seed() {auto s=receiver_selection_seed();s->pc=action::receiver_assessment_phase;return s;}
std::unique_ptr<State> receiver_effects_seed() {auto s=receiver_selection_seed();s->pc=action::receiver_effects_phase;s->wr<4>(68,0);return s;}
std::unique_ptr<State> receiver_final_seed() {auto s=receiver_selection_seed();s->pc=action::receiver_final_reaction_phase;s->wr<4>(68,1);s->memory.write<4>(globals,17);return s;}
std::unique_ptr<State> missing_receiver_seed() {auto s=receiver_selection_seed();s->pc=action::missing_receiver_phase;return s;}
std::unique_ptr<State> return_seed() {
    auto s=receiver_selection_seed();s->pc=action::return_phase;s->memory.write<4>(stack+8,return_sentinel);
    for(unsigned n=13;n<=31;++n)s->memory.write<4>(stack-0x6c+(n-13)*4,0x11223300u+n);
    for(unsigned n=29;n<=31;++n)s->memory.write<8>(stack-(32-n)*8,float_bits<8>(double(n)+0.25));return s;
}

void check_receiver_tail() {
    for(auto state:{0u,2u,4u,255u})for(auto side:{0u,1u})for(auto column:{0u,1u,3u,6u,10u,11u})for(auto contact:{0u,1u})for(auto execution:{0u,1u})for(auto result:{0u,1u})for(auto receiver:{0u,recipient}) {
        auto s=receiver_selection_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,column);
        Options o;o.contact_result=contact;o.execution_contact=execution;o.result_contact=result;o.execution_receiver=receiver;o.failed_result=1;auto r=compare(*s,o,70);require(r.state->pc==return_sentinel,"Complete coordinator matrix returns through its original caller frame");
    }
    for(bool child:{false,true}){auto s=receiver_selection_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.execution_contact=0;o.result_contact=0;o.execution_rating=32767;o.contact_end_gate=1;o.failed_result=1;o.own_negative=!child;o.child_negative=child;compare(*s,o,70);}
    for(auto kind:{0u,19u,21u,33u,45u,127u,128u,255u})for(auto score:{0u,100u,300u,32767u,32768u,65535u})for(auto rating:{0u,100u,300u,32767u,32768u,65535u})for(auto status:{0u,1u}) {
        auto s=receiver_selection_seed();s->wr<4>(68,0xabcd0000|score);s->memory.write<2>(stack-0x150+0x4a,rating);s->memory.write<1>(stack-0x150+0x48,kind);Options o;o.selection_status=status;
        auto result=compare(*s,o,53);const auto balance=std::clamp<std::int64_t>(signed_value<2>(rating-score-200),-200,200);
        const auto threshold=kind==19?400:kind==33||kind==45?250:100;
        require(result.state->memory.read<2>(recipient+0x1a0)==std::uint16_t(balance)&&result.state->r<4>(68)==0,"Receiver selection narrows the score difference before clamping and clears its flag");
        require(result.state->pc==(balance>=threshold?action::receiver_position_phase:status?action::receiver_marker_phase:action::receiver_sampling_phase),"Receiver balance and live selection status select the original continuation");
    }
    for(auto kind:{0u,19u,33u,45u})for(auto state:{0u,3u,4u,5u,128u,255u})for(auto status:{0u,1u}) {
        auto s=receiver_selection_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<1>(match+0x791f,state);Options o;o.selection_status=status;boundaries(*s,o,53);
    }
    {Options o;o.mutate_receiver_selection=true;auto r=compare(*receiver_selection_seed(),o,53);require(r.state->pc==action::receiver_marker_phase,"Receiver selection reloads its owner after the dependency");boundaries(*receiver_selection_seed(),o,53);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*receiver_selection_seed(),o,53);}
    for(unsigned first=0;first<256;++first)for(unsigned second=0;second<256;++second) {
        auto s=receiver_sampling_seed();s->memory.write<1>(recipient+0x10f,first);s->memory.write<1>(recipient+0x110,second);auto result=compare(*s,{},55);
        const auto interval=3-int(signed_value<1>(first)>0)-int(signed_value<1>(second)>2);
        require(result.state->memory.read<1>(recipient+0x2c)==unsigned(124%interval)&&result.state->r<4>(68)==1,"Both signed receiver traits select the exact three-draw counter sample");
    }
    for(auto first:{0u,1u,127u,128u,255u})for(auto second:{0u,2u,3u,127u,128u,255u}){auto s=receiver_sampling_seed();s->memory.write<1>(recipient+0x10f,first);s->memory.write<1>(recipient+0x110,second);boundaries(*s,{},55);}
    for(auto random:{0u,17u,18u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto cursor:{stream+120,stream+122,stream+124,stream+126}) {
        auto s=receiver_sampling_seed();s->memory.write<4>(globals,random);s->memory.write<4>(globals+8,cursor);Options o;o.mutate_receiver_sampling=true;boundaries(*s,o,55);
    }
    for(auto wide:{39u,40u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*receiver_sampling_seed(),o,55);}
    {Options o;o.zero_receiver_interval=true;boundaries(*receiver_sampling_seed(),o,55);}
    for(bool child:{false,true}){auto s=receiver_sampling_seed();s->memory.write<4>(globals+8,stream+126);Options o;o.own_negative=!child;o.child_negative=child;compare(*s,o,55);}
    for(unsigned flag=0;flag<256;++flag){auto s=receiver_marker_seed();s->memory.write<1>(stack-0x150+0xd0,flag);auto r=compare(*s,{},57);require(r.calls.size()==unsigned(flag!=0)&&r.state->pc==action::receiver_effects_phase,"Any nonzero receiver marker invokes its actor dependency");}
    for(auto flag:{0u,1u,128u}){auto s=receiver_marker_seed();s->memory.write<1>(stack-0x150+0xd0,flag);boundaries(*s,{},57);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*receiver_marker_seed(),o,57);}
    for(unsigned state=0;state<256;++state)for(auto side:{0u,1u,128u,255u})for(auto column:{0u,3u,4u,8u,9u,11u,127u,128u,255u}) {
        auto s=receiver_position_seed();s->memory.write<1>(match+0x791f,state);s->memory.write<1>(recipient+0x27,side);s->memory.write<1>(actor+0x106,column);auto r=compare(*s,{},59);
        const auto normalized=state<4?0u:state;const bool right=side==0&&normalized!=255;
        const auto col=std::int32_t(signed_value<1>(column)),destination=right?(col<9?col+3:11):(col>3?col-3:0);
        require(r.state->memory.read<1>(match+0x791f)==normalized&&r.calls.size()==1&&read_be<4>(r.calls[0].before.data()+28)==std::uint32_t(signed_value<1>(destination)),"Receiver positioning preserves signed state normalization and destination clamping");
    }
    for(auto state:{0u,3u,4u,128u,255u})for(auto side:{0u,1u})for(auto column:{3u,4u,8u,9u}){auto s=receiver_position_seed();s->memory.write<1>(match+0x791f,state);s->memory.write<1>(recipient+0x27,side);s->memory.write<1>(actor+0x106,column);boundaries(*s,{},59);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*receiver_position_seed(),o,59);}
    for(auto kind:{0u,21u,127u,128u,255u})for(auto metric:{0u,604u,605u,714u,715u,1100u,1407u,1408u,32767u,32768u,65535u})for(auto denominator:{0u,1u,10u,255u})for(auto numerator:{0u,1u,8u,255u}) {
        auto s=receiver_assessment_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<2>(match+0x7948,metric);s->memory.write<1>(recipient+(kind==21?7:4),denominator);s->memory.write<1>(recipient+(kind==21?8:5),numerator);compare(*s,{},61);
    }
    for(auto kind:{0u,21u})for(auto side:{0u,1u,128u})for(auto flag:{0u,1u,128u})for(auto balance:{6750u,6751u,32767u,32768u})for(auto reported:{0u,1u}) {
        auto s=receiver_assessment_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<1>(recipient+0x27,side);s->memory.write<1>(match+(side?0xe03d:0xe073),flag);s->memory.write<2>(recipient+0x36,balance);s->memory.write<1>(recipient+0x100,reported);
        auto r=compare(*s,{},61);const bool emit=!flag&&balance>6750&&balance<=32767&&!reported;
        require(r.calls.size()==unsigned(emit)&&r.state->memory.read<1>(recipient+0x100)==unsigned(reported||emit),"Receiver performance report requires an eligible club, signed balance and unused report flag");
        if(emit)require(read_be<4>(r.calls.back().before.data()+16)==(kind==21?0x21ceu:0x21cdu),"Receiver assessment chooses the action-specific message");
    }
    for(auto kind:{0u,21u})for(auto denom:{0u,10u})for(auto count:{0u,1u,2u,3u,4u}){auto s=receiver_assessment_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<1>(recipient+(kind==21?7:4),denom);s->memory.write<1>(recipient+(kind==21?8:6),count);boundaries(*s,{},61);}
    for(auto bits:{0ull,0x3fe0000000000000ull,0x3ff0000000000000ull,0x7ff0000000000000ull,0x7ff8000000001234ull})for(auto kind:{0u,21u}){auto s=receiver_assessment_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<8>(doubles+0x288,bits);boundaries(*s,{},61);}
    {auto s=receiver_assessment_seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},61);}
    {auto s=receiver_assessment_seed();s->memory.write<1>(stack-0x150+0x48,21);s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},61);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*receiver_assessment_seed(),o,61);}
    for(unsigned state=0;state<256;++state)for(auto flag:{0u,1u,0x100u,0x180u,0x1ffu})for(auto kind:{0u,2u,4u,6u,0x10000u}) {
        auto s=receiver_effects_seed();s->wr<4>(88,0xabcd0000|state);s->wr<4>(68,0xabcd0000|flag);Options o;o.final_gate=0;o.receiver_kinds.fill(kind);auto r=compare(*s,o,63);
        const auto base=std::uint16_t(7000+((flag&255)?120:200));const auto extra=state==4?40:((flag&255)?120:200)*((kind&6)?1:2);
        require(r.state->memory.read<1>(actor+0x2c)==1&&r.state->memory.read<2>(recipient+0x36)==std::uint16_t(base+extra),"Receiver effects retain wrapped actor increments and ordered signed receiver rewards");
        if(state==4)require(r.state->memory.read<2>(recipient+0x19c)==123&&r.state->memory.read<1>(recipient+7)==11&&r.state->memory.read<1>(recipient+8)==9&&r.state->memory.read<2>(actor+0x36)==240,"Special receiver effects update both players independently");
    }
    for(auto first:{0u,2u,4u})for(auto second:{0u,2u,4u}){Options o;o.receiver_kinds={first,second,0};boundaries(*receiver_effects_seed(),o,63);}
    for(auto state:{0u,4u})for(auto side:{0u,1u,128u})for(auto row:{2u,3u,4u,5u,6u})for(auto column:{0u,1u,11u})for(auto gate:{0u,1u})for(auto alignment:{0u,1u}) {
        auto s=receiver_effects_seed();s->wr<4>(88,state);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);Options o;o.final_gate=gate;o.low_alignment=alignment;compare(*s,o,63);
    }
    for(auto state:{0u,4u})for(auto trait:{0u,1u,127u,128u,255u})for(auto response:{0u,1u,0x100u,0x1ffu}) {
        auto s=receiver_effects_seed();s->wr<4>(88,state);s->memory.write<1>(recipient+0x17,trait);s->memory.write<4>(globals,14);s->memory.write<2>(stream+2,0);Options o;o.reaction_result=response;boundaries(*s,o,63);
    }
    for(auto wide:{41u,42u,43u,44u})for(auto sampled:{-100000,0,10000000}){auto s=receiver_effects_seed();s->wr<4>(88,wide>=43?4u:0u);Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*s,o,63);}
    for(auto state:{0u,4u}){auto s=receiver_effects_seed();s->wr<4>(88,state);Options o;o.zero_receiver_reaction=state==4?2:1;boundaries(*s,o,63);}
    for(auto state:{0u,4u})for(auto cursor:{stream+124,stream+126}){auto s=receiver_effects_seed();s->wr<4>(88,state);s->memory.write<4>(globals+8,cursor);boundaries(*s,{},63);}
    for(auto state:{0u,4u}){auto s=receiver_effects_seed();s->wr<4>(88,state);s->memory.write<4>(globals+8,stream+124);s->memory.write<2>(stream+126,52);boundaries(*s,{},63);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*receiver_effects_seed(),o,63);}
    for(unsigned flag=0;flag<256;++flag)for(auto random:{0u,1u,17u,18u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=receiver_final_seed();s->wr<4>(68,0xabcd0000|flag);s->memory.write<4>(globals,random);auto r=compare(*s,{},65);const bool emit=flag&&std::int32_t(102u+random)%7==0;
        require(r.calls.size()==unsigned(emit),"Final receiver report uses the signed seventh and low-byte flag");
    }
    for(auto flag:{0u,1u,128u})for(auto random:{17u,18u}){auto s=receiver_final_seed();s->wr<4>(68,flag);s->memory.write<4>(globals,random);boundaries(*s,{},65);}
    {auto s=receiver_final_seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},65);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*receiver_final_seed(),o,65);}
    {auto r=compare(*missing_receiver_seed(),{},67);require(r.calls.size()==4&&r.state->memory.read<1>(error_flag)==0&&r.state->memory.read<1>(error_buffer)==0x45,"Missing receiver retains all four diagnostics and clears the original error flag");boundaries(*missing_receiver_seed(),{},67);}
    for(auto slot:{-0x51e4,-0x7ba8,-0x7bac})for(auto pointer:{0u,actor+1,stack-0x150}){auto s=missing_receiver_seed();s->memory.write<4>(toc+slot,pointer);compare(*s,{},67);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*missing_receiver_seed(),o,67);}
    {auto s=return_seed();const auto r=compare(*s,{},69);require(r.state->pc==return_sentinel&&r.state->r<4>(4)==stack,"Ball-action return restores its caller and complete frame");for(unsigned n=13;n<=31;++n)require(r.state->r<4>(n*4)==0x11223300u+n,"Ball-action return restores every nonvolatile general register");for(unsigned n=29;n<=31;++n)require(r.state->r<8>(0x4000+n*16)==float_bits<8>(double(n)+0.25),"Ball-action return restores every saved floating register");boundaries(*s,{},69);Options o;o.own_negative=true;compare(*s,o,69);}
    for(auto phase:{54u,56u,58u,60u,62u,64u,66u,68u,70u})for(auto status:{0u,1u})for(auto state:{2u,4u}) {
        auto s=receiver_selection_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=0;o.execution_contact=0;o.result_contact=0;o.execution_rating=32767;o.contact_end_gate=1;o.failed_result=1;o.selection_status=status;
        auto r=compare(*s,o,phase);require(std::any_of(r.calls.begin(),r.calls.end(),[](const Event& e){return e.continuation==0x10243bac;}),"Original-root prefix reaches the reconstructed receiver tail");if(phase==70)require(r.state->pc==return_sentinel,"Complete failed-action root returns to the caller");
        if(status==0&&state==2)boundaries(*s,o,phase);
    }
    for(auto phase:{68u,70u}){auto s=receiver_selection_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.execution_contact=0;o.result_contact=0;o.execution_rating=32767;o.contact_end_gate=1;o.failed_result=1;o.force_missing_receiver=true;auto r=compare(*s,o,phase);require(std::any_of(r.calls.begin(),r.calls.end(),[](const Event& e){return e.target==0x10769b60;}),"Original-root prefix reaches the missing-receiver diagnostic");boundaries(*s,o,phase);}
    for(auto state:{0u,2u,4u}){auto s=receiver_selection_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=0;o.execution_receiver=0;auto r=compare(*s,o,70);require(r.state->pc==return_sentinel,"Complete successful-action root returns to the caller");boundaries(*s,o,70);}
}
}
namespace cm::aot {void ball_action_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    auto initial=seed();boundaries(*initial);
    for(auto fitness:{0.0,1.0,7499.99,7500.0,9999.0,10000.0,10001.0,32767.0,32768.0,65535.0,65536.0,-1.0,-32768.0,-32769.0,1.0e30,-1.0e30})
        for(auto random:{0u,17u,0x7fffffffu,0x80000000u,0xffffffffu}) {
            auto s=seed();s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<4>(globals,random);
            compare(*s);
        }
    for(auto bits:{0x7fc12345u,0x7f812345u,0xffc12345u,0x7f800000u,0xff800000u,0x80000000u}){
        auto s=seed();s->memory.write<4>(actor+0x50,bits);compare(*s);
    }
    for(unsigned coord=0;coord<256;++coord){auto s=seed();s->wr<4>(20,0xabcd0000|coord);s->wr<4>(24,0xfedc0000|(255-coord));
        auto result=compare(*s);require(result.state->r<4>(68)==(coord==255?4:(0xabcd0000|coord)),"Low-byte coordinate sentinel");}
    for(auto cursor:{stream,stream+124,stream+126,stream+128,0xfffffffeu})for(auto random:{0u,0xabcd5678u,0xffffffffu}){
        auto s=seed();s->memory.write<4>(globals+8,cursor);Options o;o.random=random;compare(*s,o);
    }
    {auto s=seed();s->memory.write<4>(actor+0x50,float_bits<4>(10000));boundaries(*s);
        require(compare(*s).calls.size()==1,"Zero fitness interval consumes no random sample");}
    {auto s=seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s);}
    for(unsigned wide:{1u,2u})for(auto sampled:{-100000,0,9,1000000}) {
        auto s=seed();Options o;o.force_wide=wide;o.sampled=sampled;compare(*s,o);
        if(sampled==1000000)boundaries(*s,o);
    }
    for(auto value:{0.0,-1.0,1.0e100}){auto s=seed();s->memory.write<8>(doubles,float_bits<8>(value));Options o;o.force_wide=1;compare(*s,o);}
    for(auto fitness:{-32768.0,7500.0,32767.0}){auto s=seed();s->memory.write<4>(actor+0x50,float_bits<4>(fitness));Options o;o.mutate_fitness=true;boundaries(*s,o);}
    {Options o;o.mutate_owner=true;boundaries(*initial,o);}
    for(auto ptr:{0u,actor+1,stack-0x108,stack-0x150,stack+8}){auto s=seed();s->wr<4>(12,ptr);compare(*s);}
    for(auto sp:{stack+1,stack+8,globals+0x150,actor+0x6c}){auto s=seed();s->wr<4>(4,sp);compare(*s);}
    for(auto offset:{-0x51e8,-0x51e0,-0x7b90,-0x7b8c,-0x7b88,-0x7b84})for(auto ptr:{0u,stack-0x150,globals+8}){
        auto s=seed();s->memory.write<4>(toc+offset,ptr);compare(*s);
    }
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*initial,o);}
    for(auto surface:{0u,1u,2u,31u,32u,33u,127u,128u,255u})for(auto height:{0u,1u,2u,3u,127u,128u,129u,255u})
        for(auto heading:{0u,20u,89u,90u,91u,180u,181u,359u,360u,32767u,32768u,65535u}) {
            auto s=orientation_seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,height);
            s->memory.write<2>(actor+0x19e,heading);s->memory.write<2>(match+0x7950,0);
            const auto result=compare(*s,{},1);
            auto angle=std::int32_t(signed_value<2>(heading));if(angle<0)angle=-angle;
            angle=std::int32_t(signed_value<2>(std::uint32_t(angle)));
            if(angle>180)angle=std::int32_t(signed_value<2>(std::uint32_t(360-angle)));
            auto bias=std::int32_t(signed_value<2>(std::uint32_t(360-2*angle)))/3;
            bias=std::int32_t(signed_value<2>(std::uint32_t(bias)));
            bias=std::int32_t(signed_value<2>(std::uint32_t(bias*(angle<90?11-angle/10:2))));
            require(result.state->r<4>(80)==std::uint32_t(bias),"Independent orientation bias with signed narrowing");
            const auto h=std::int32_t(signed_value<1>(height-1));
            const auto cube=std::int32_t(signed_value<2>(std::uint32_t(h*h*h*20)));
            const auto surface_penalty=std::int32_t(signed_value<2>(std::uint32_t(h*(surface==2?119%3+1:surface==32?119%8+3:0))));
            require(result.state->r<4>(64)==std::uint32_t(700-cube-surface_penalty),"Independent height and surface adjustment");
        }
    for(unsigned kind=0;kind<256;++kind){auto s=orientation_seed();s->wr<4>(88,kind|0xabcdef00);compare(*s,{},1);}
    for(auto surface:{0u,2u,32u,33u})for(auto heading:{20u,90u,270u})for(bool wrap:{false,true}){
        auto s=orientation_seed();s->memory.write<1>(match+1,surface);s->memory.write<2>(actor+0x19e,heading);
        if(wrap)s->memory.write<4>(globals+8,stream+126);
        boundaries(*s,{},1);
    }
    for(auto surface:{2u,32u})for(auto random:{0u,0x7fffffffu,0x80000000u,0xfffffff0u,0xffffffffu}) {
        auto s=orientation_seed();s->memory.write<1>(match+1,surface);s->memory.write<4>(globals,random);compare(*s,{},1);
    }
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=orientation_seed();s->wr<4>(60,ptr);compare(*s,{},1);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*orientation_seed(),o,1);}
    for(auto surface:{0u,2u,32u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));compare(*s,{},2);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);boundaries(*s,{},2);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto column:{0u,2u,3u,4u,8u,9u,10u,11u,127u,128u,255u})
        for(auto trait:{0u,2u,20u,127u,128u,255u})for(auto gate:{0u,1u,0x100u,0x1ffu}) {
            auto s=facing_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,column);
            s->memory.write<1>(actor+0x10f,trait);Options o;o.facing_gate=gate;
            auto result=compare(*s,o,3);
            const bool attack=std::int32_t(signed_value<1>(trait))*3>119||(gate&255)!=0;
            const auto destination=attack?(side?std::max(std::int32_t(signed_value<1>(column))-3,0):
                std::int32_t(signed_value<1>(column))>=9?11:std::int32_t(signed_value<1>(column))+3):8;
            require(result.state->memory.read<2>(actor+0x19e)==(((attack?4u:5u)<<8)|(std::uint32_t(destination)&255)),"Independent facing destination and low-byte gate");
        }
    for(auto angle:{0u,90u,91u,180u,32767u,32768u,0xffffu}){auto s=facing_seed();s->wr<4>(52,angle);compare(*s,{},3);}
    for(auto height:{0u,2u,3u,4u,5u,127u,128u,255u})for(auto distance:{-1.0,9.99,10.0,10.01,1.0e100}){
        auto s=facing_seed();s->memory.write<1>(match+0x791f,height);s->wr<8>(0x4000+29*16,float_bits<8>(distance));compare(*s,{},3);
    }
    for(auto bits:{0x7ff8000000001234ull,0x7ff0000000001234ull,0x7ff0000000000000ull,0xfff0000000000000ull}){
        auto s=facing_seed();s->wr<8>(0x4000+29*16,bits);compare(*s,{},3);
    }
    for(auto side:{0u,1u})for(auto column:{2u,6u,10u})for(bool wrap:{false,true}) {
        auto s=facing_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,column);
        if(wrap)s->memory.write<4>(globals+8,stream+126);
        Options o;o.facing_gate=1;boundaries(*s,o,3);
    }
    for(unsigned wide:{3u,4u})for(auto sampled:{-10000,0,100000}){
        Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*facing_seed(),o,3);
    }
    for(auto interval:{0,-1,-32768,32767}){Options o;o.facing_interval=interval;boundaries(*facing_seed(),o,3);}
    {auto s=facing_seed();s->memory.write<1>(match+0x791f,4);boundaries(*s,{},3);}
    for(auto gate:{0u,1u}){Options o;o.facing_gate=gate;o.mutate_facing=true;boundaries(*facing_seed(),o,3);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=facing_seed();s->wr<4>(60,ptr);compare(*s,{},3);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*facing_seed(),o,3);}
    for(auto surface:{0u,2u,32u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));compare(*s,{},4);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);boundaries(*s,{},4);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,4u,6u,7u,127u,128u,255u})
        for(auto column:{0u,1u,2u,9u,10u,11u,12u,127u,128u,255u})for(auto trait:{0u,1u,127u,128u,253u,255u}) {
            auto s=area_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
            s->memory.write<1>(actor+0x106,column);s->memory.write<1>(actor+0x368,trait);
            auto result=compare(*s,{},5);
            const bool inside=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);
            const auto interval=std::int32_t(signed_value<1>(trait))*200+int(inside)*500+100;
            const auto sample=interval==0?0:interval>0?119%interval:-(119%interval);
            require(result.state->pc==(sample==0?action::selected_action_phase:action::ball_state_selection_phase),"Independent area reaction selection");
            if(sample==0)require(result.state->r<4>(52)==37&&result.state->r<4>(68)==0xffff8765,"Selected action reads its signed attribute");
            else require(result.state->r<4>(52)==std::uint32_t(sample),"Independent signed area sample");
        }
    for(auto side:{0u,1u})for(auto status:{0u,1u,2u,0xffffu,0x10001u}){
        auto s=area_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,side?10:1);
        Options o;o.area_status=status;boundaries(*s,o,5);
    }
    for(auto row:{1u,4u,7u})for(auto col:{0u,2u,10u,12u}){
        auto s=area_seed();s->memory.write<1>(actor+0x27,1);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,col);boundaries(*s,{},5);
    }
    for(auto trait:{0u,128u,253u})for(auto draw:{0u,100u,65535u}) {
        auto s=area_seed();s->memory.write<1>(actor+0x368,trait);s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,draw);boundaries(*s,{},5);
    }
    for(unsigned wide:{5u,6u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*area_seed(),o,5);}
    {auto s=area_seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},5);}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=area_seed();s->memory.write<4>(globals,random);compare(*s,{},5);}
    {Options o;o.mutate_area=true;boundaries(*area_seed(),o,5);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=area_seed();s->wr<4>(60,ptr);compare(*s,{},5);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*area_seed(),o,5);}
    for(auto surface:{0u,2u,32u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));compare(*s,{},6);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);boundaries(*s,{},6);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,6u,7u})for(auto column:{0u,1u,2u,9u,10u,11u,12u})
        for(auto trait:{0u,2u,128u,255u})for(auto preference:{0u,1u,20u,127u,128u,255u}) {
            auto s=elevated_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
            s->memory.write<1>(actor+0x106,column);s->memory.write<1>(actor+0x24,trait);Options o;o.elevated_preference=preference;
            auto result=compare(*s,o,7);
            const bool inside=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);
            const auto first=std::int32_t(signed_value<1>(preference));
            const auto first_sample=first==0?0:first>0?119%first:-(119%first);
            const auto interval=int(inside)*1500+std::int32_t(signed_value<1>(trait))*80+70;
            const auto draw=first==0?119:121;
            const auto second_sample=interval>0?draw%interval:-(draw%interval);
            const auto kind=first_sample>second_sample?46u:21u;
            require(result.state->r<4>(52)==kind&&result.state->r<4>(68)==0xffff8000u+kind,"Independent elevated preference and threshold selection");
        }
    for(unsigned state=0;state<256;++state){auto s=elevated_seed();s->wr<4>(88,state|0x12340000);
        auto result=compare(*s,{},7);require(result.state->pc==(std::int32_t(signed_value<1>(state))<4?action::low_ball_selection_phase:action::selected_action_phase),"Signed ball-state selection");}
    for(auto side:{0u,1u,2u,255u})for(auto col:{0u,1u,6u,10u,11u})for(auto control:{0u,125u,126u,127u,0xffffu})for(auto gate:{0u,1u,0x100u,0x1ffu}) {
        auto s=elevated_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,col);s->wr<4>(64,control);
        Options o;o.lane_clear=gate;auto result=compare(*s,o,7);
        const bool opposite=side==0?col>=10&&col<=11:col<=1;
        const bool clear=!opposite&&std::int32_t(signed_value<2>(control))>126&&(gate&255)!=0;
        require((result.state->r<4>(52)==12)==clear,"Independent clear-lane action gate");
        if(clear)require(result.state->r<4>(92)==1&&result.state->r<4>(88)==2&&result.state->memory.read<1>(match+0x791f)==2,"Clear-lane action updates live ball state");
    }
    for(auto side:{0u,1u})for(auto col:{0u,6u,10u})for(auto preference:{0u,20u,255u}){
        auto s=elevated_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,col);
        Options o;o.elevated_preference=preference;boundaries(*s,o,7);
    }
    for(auto gate:{0u,1u})for(bool mutation:{false,true}){Options o;o.lane_clear=gate;o.mutate_elevated=mutation;boundaries(*elevated_seed(),o,7);}
    for(unsigned wide:{7u,8u,9u,10u})for(auto sampled:{-100000,0,10000000}){
        Options o;o.force_wide=wide;o.sampled=sampled;o.elevated_preference=20;boundaries(*elevated_seed(),o,7);
    }
    {Options o;o.zero_elevated_threshold=true;boundaries(*elevated_seed(),o,7);}
    for(auto cursor:{stream+124,stream+126}){auto s=elevated_seed();s->memory.write<4>(globals+8,cursor);Options o;o.elevated_preference=20;boundaries(*s,o,7);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*elevated_seed(),o,7);}
    for(auto state:{2u,4u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<1>(match+0x791f,state);compare(*s,{},8);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);
        s->memory.write<1>(match+0x791f,4);boundaries(*s,{},8);}
    for(unsigned state=0;state<256;++state){auto s=low_seed();s->wr<4>(88,state|0xabcd0000);auto result=compare(*s,{},9);
        if(state!=2&&state!=3)require(result.state->r<4>(52)==13&&result.state->r<4>(68)==0xffffcfc7,"Other low-ball states select action 13");}
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{2u,3u,4u,5u,6u,128u})for(auto col:{0u,1u,6u,11u,12u,128u})
        for(auto gate:{0u,1u,0x100u,0x1ffu}) {
            auto s=low_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,col);
            Options o;o.low_alignment=gate;auto result=compare(*s,o,9);
            const bool goal=row>=3&&row<=5&&col==(side?0u:11u);
            require(result.state->r<4>(52)==(goal||(gate&255)?59u:12u),"Goal cell or low-byte alignment permits the attempt");
        }
    for(auto first:{0,1,2,119,120,200,32767,-1,-32768})for(auto second:{0,1,2,119,120,200,32767,-1,-32768})for(auto rating:{0,1,2,4,5,20}){
        auto s=low_seed();s->memory.write<2>(actor+0x194,std::uint32_t(first));s->memory.write<2>(actor+0x136,std::uint32_t(second));
        s->memory.write<4>(actor+0x9c,float_bits<4>(rating));auto result=compare(*s,{},9);
        int draws=0;
        const auto draw=[&](int interval){if(!interval)return 0;const int number=117+2*++draws;return interval>0?number%interval:-(number%interval);};
        const auto a=draw(first),b=draw(second);unsigned kind=12;
        if(a>b){const auto sampled=draw(rating);const auto threshold=(117+2*++draws)%5;if(sampled>=threshold)kind=59;}
        require(result.state->r<4>(52)==kind,"Independent low-ball competing samples and attempt threshold");
        require(result.state->r<4>(68)==std::uint32_t(kind==59?first:second),"Low-ball selected attribute aliases its earlier interval");
    }
    for(auto distance:{-1.0,0.0,49.999,50.0,50.001})for(auto limit:{-1.0,0.0,50.0}){
        auto s=low_seed();s->wr<8>(0x4000+29*16,float_bits<8>(distance));s->memory.write<8>(doubles+0x328,float_bits<8>(limit));compare(*s,{},9);
    }
    for(auto bits:{0x7ff8000000001234ull,0x7ff0000000001234ull,0x7ff0000000000000ull,0xfff0000000000000ull}){
        auto s=low_seed();s->wr<8>(0x4000+29*16,bits);compare(*s,{},9);s->wr<8>(0x4000+29*16,float_bits<8>(25));s->memory.write<8>(doubles+0x328,bits);compare(*s,{},9);
    }
    for(auto side:{0u,1u})for(auto row:{2u,4u,6u})for(auto col:{0u,6u,11u}){
        auto s=low_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,col);boundaries(*s,{},9);
    }
    for(auto first:{0,200,-1})for(auto second:{0,2,-1}){auto s=low_seed();s->memory.write<2>(actor+0x194,std::uint32_t(first));
        s->memory.write<2>(actor+0x136,std::uint32_t(second));boundaries(*s,{},9);}
    for(auto rating:{0.0,0.5,-1.0,2.0,65535.0,65536.0,-65535.0,-65536.0,2147483520.0,-2147483648.0,1.0e30}){
        auto s=low_seed();s->memory.write<4>(actor+0x9c,float_bits<4>(rating));boundaries(*s,{},9);
    }
    for(auto bits:{0x7fc12345u,0x7f812345u,0xffc12345u,0x7f800000u,0xff800000u}){auto s=low_seed();s->memory.write<4>(actor+0x9c,bits);compare(*s,{},9);}
    for(unsigned wide:{11u,12u,13u,14u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*low_seed(),o,9);}
    for(auto sampled:{-100000,10000000}){auto s=low_seed();s->memory.write<4>(actor+0x9c,float_bits<4>(65536));Options o;o.sampled=sampled;boundaries(*s,o,9);}
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126}){auto s=low_seed();s->memory.write<4>(globals+8,cursor);boundaries(*s,{},9);}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=low_seed();s->memory.write<4>(globals,random);compare(*s,{},9);}
    {Options o;o.mutate_low=true;boundaries(*low_seed(),o,9);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*low_seed(),o,9);}
    for(auto state:{0u,2u,3u,4u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);
        s->memory.write<2>(actor+0x19e,heading);s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<1>(match+0x791f,state);compare(*s,{},10);
    }
    {auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);boundaries(*s,{},10);}
    for(auto first:{0u,1u,200u,32768u,65535u})for(auto control:{0u,1u,700u,32768u,65535u})
        for(auto fatigue:{0u,1u,117u,65535u})for(auto orientation:{0u,1u,88u,65535u}) {
            auto s=selected_seed();s->wr<4>(68,first|0xabcd0000);s->wr<4>(64,control|0xef120000);
            s->wr<4>(84,fatigue|0x34560000);s->wr<4>(80,orientation|0x789a0000);auto result=compare(*s,{},11);
            int draws=0;const auto draw=[&](unsigned value){const auto interval=std::int32_t(signed_value<2>(value));
                if(!interval)return 0;const auto number=117+2*++draws;return interval>0?number%interval:-(number%interval);};
            const auto a=draw(first),b=draw(control),c=draw(fatigue),d=draw(orientation);
            const auto expected=std::uint32_t(signed_value<2>(std::uint32_t(a+b-c-d)));
            require(result.state->r<4>(68)==expected,"Independent selected-action four-term adjustment");
            require(result.state->memory.read<4>(actor+0x50)==float_bits<4>(0.5),"Selected-action fitness dependency consumes the live constant");
        }
    for(unsigned kind=0;kind<256;++kind){auto s=selected_seed();s->wr<4>(52,kind|0xabcd0000);compare(*s,{},11);}
    for(unsigned condition=0;condition<256;++condition)for(unsigned overflow:{0u,1u})for(auto interval:{0u,1u,0x8000u}){
        auto s=selected_seed();s->wr<1>(2304,condition);s->wr<1>(1024,overflow);s->wr<4>(68,interval);
        s->budget=1;compare(*s,{},11);
    }
    for(auto reg:{17u,16u,21u,20u})for(auto value:{0u,1u,0xffffu,0x8000u,0x10000u,0x80000000u,0xffffffffu}){
        auto s=selected_seed();s->wr<4>(reg*4,value);boundaries(*s,{},11);
    }
    {auto s=selected_seed();for(auto reg:{17u,16u,21u,20u})s->wr<4>(reg*4,0);boundaries(*s,{},11);}
    for(unsigned adjustment=0;adjustment<4;++adjustment)for(auto wide:{-1,1})for(auto sampled:{-100000,0,10000000}){
        Options o;o.adjustment=adjustment;o.adjustment_wide=wide;o.sampled=sampled;boundaries(*selected_seed(),o,11);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126}){auto s=selected_seed();s->memory.write<4>(globals+8,cursor);boundaries(*s,{},11);}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=selected_seed();s->memory.write<4>(globals,random);compare(*s,{},11);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=selected_seed();s->wr<4>(60,ptr);compare(*s,{},11);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*selected_seed(),o,11);}
    for(auto state:{0u,2u,3u,4u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);
        s->memory.write<2>(actor+0x19e,heading);s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<1>(match+0x791f,state);compare(*s,{},12);
    }
    {auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);boundaries(*s,{},12);}
    for(auto flag:{0u,1u,2u,127u,128u,255u,0x100u,0x1ffu})for(auto interval:{0,1,2,7,20,119,-1,-20,-119,32767,-32768}){
        auto s=contact_seed();s->memory.write<4>(singles+0x2c,float_bits<4>(interval));Options o;o.contact_result=flag;
        auto result=compare(*s,o,13);const auto sampled=interval==0?0:interval>0?119%interval:-(119%interval);
        require(result.state->pc==((flag&255)==0||sampled==0?action::action_execution_phase:action::contact_resolution_phase),"Independent contact screening decision");
        const auto reports=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101d68f0;});
        require(reports==int((flag&255)!=0&&sampled==0),"Immediate contact action only for an active zero sample");
    }
    for(unsigned byte=0;byte<256;++byte){auto s=contact_seed();s->memory.write<4>(singles+0x2c,float_bits<4>(0));Options o;o.contact_x=byte;o.contact_y=255-byte;
        auto result=compare(*s,o,13);require(result.state->memory.read<4>(stack-0x150+0x38)==byte&&result.state->memory.read<4>(stack-0x150+0x3c)==255-byte,"Contact coordinate output bytes reach ordered call spills");}
    for(auto constant:{0.0,0.25,1.0,20.0,65535.0,65536.0,-65535.0,-65536.0,2147483520.0,-2147483648.0})
        for(auto multiplier:{0.0,0.2,-2.0,10000.0}){
            auto s=contact_seed();s->memory.write<4>(singles+0x2c,float_bits<4>(constant));s->memory.write<4>(singles+0x3c,float_bits<4>(multiplier));compare(*s,{},13);
        }
    for(auto flag:{0u,1u,255u})for(auto interval:{0.0,0.25,1.0,20.0,-20.0,65536.0,-65536.0}){
        auto s=contact_seed();s->memory.write<4>(singles+0x2c,float_bits<4>(interval));Options o;o.contact_result=flag;boundaries(*s,o,13);
    }
    for(auto interval:{65536.0,-65536.0})for(auto sampled:{-100000,0,10000000}){
        auto s=contact_seed();s->memory.write<4>(singles+0x2c,float_bits<4>(interval));Options o;o.sampled=sampled;boundaries(*s,o,13);
    }
    for(auto bits:{0x7fc12345u,0x7f812345u,0xffc12345u,0x7f800000u,0xff800000u})for(auto offset:{0u,0x2cu,0x3cu}){
        auto s=contact_seed();s->memory.write<4>(singles+offset,bits);compare(*s,{},13);
    }
    for(auto distance:{-1.0e100,-1.0,0.0,1.0,100.0,1.0e100}){auto s=contact_seed();s->wr<8>(0x4000+29*16,float_bits<8>(distance));
        s->memory.write<4>(singles+0x3c,float_bits<4>(2));compare(*s,{},13);}
    {auto s=contact_seed();s->memory.write<4>(singles,float_bits<4>(20));auto result=compare(*s,{},13);
        require(result.state->memory.read<4>(globals+8)==stream&&result.state->pc==action::action_execution_phase,"Mutable float equality baseline skips random advancement");}
    for(bool mutation:{false,true}){auto s=contact_seed();s->memory.write<4>(globals+8,stream+126);Options o;o.mutate_contact=mutation;boundaries(*s,o,13);}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=contact_seed();s->memory.write<4>(globals,random);compare(*s,{},13);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=contact_seed();s->wr<4>(60,ptr);compare(*s,{},13);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*contact_seed(),o,13);}
    for(auto state:{0u,2u,3u,4u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=contact_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);
        s->memory.write<2>(actor+0x19e,heading);s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<1>(match+0x791f,state);compare(*s,{},14);
    }
    {auto s=contact_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);boundaries(*s,{},14);}
    for(unsigned flag=0;flag<256;++flag)for(auto coordinate:{0u,1u,2u,3u,127u,128u,255u}) {
        auto s=resolution_seed();s->memory.write<1>(stack-0x150+0x43,coordinate);Options o;o.resolved_contact=flag;
        auto result=compare(*s,o,15);const auto message=flag==2?0x1ff9u:flag==3?0x1ff7u:flag==4?0x1ff8u:flag==6?0x1ff5u:
            std::int32_t(signed_value<1>(coordinate))>2?0x210au:0xffffffffu;
        require(result.state->r<4>(64)==message,"Independent signed contact classification and coordinate threshold");
        require(result.state->memory.read<1>(match+0x7928)==(flag==7?7u:5u),"Raw contact seven bypasses the area reaction");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{0u,1u,2u,4u,6u,7u,127u,128u,255u})
        for(auto column:{0u,1u,2u,9u,10u,11u,12u,127u,128u,255u})for(auto flag:{1u,2u,7u}) {
            auto s=resolution_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
            s->memory.write<1>(actor+0x106,column);Options o;o.resolved_contact=flag;auto result=compare(*s,o,15);
            const bool inside=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);
            const auto status=flag==7?7u:inside?5u:6u;
            require(result.state->memory.read<1>(match+0x7928)==status,"Independent live contact area status");
            require(result.state->r<4>(68)==(status==5?0x1f7au:0x1f79u),"Contact outcome message follows the resulting area status");
            require(result.state->memory.read<2>(actor+0x36)==(status==5?0xff06u:250u),"Contact halfword decrement wraps without saturation");
        }
    for(auto trait:{0u,1u,2u,7u,20u,119u,127u,128u,255u})for(auto random:{0u,17u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=resolution_seed();s->memory.write<1>(actor+0x366,trait);s->memory.write<4>(globals,random);
        Options o;o.resolved_contact=2;auto result=compare(*s,o,15);const auto interval=std::int32_t(signed_value<1>(trait));
        const auto number=std::bit_cast<std::int32_t>(random+102u);
        const auto sample=interval==0?0:interval>0?number%interval:-(number%interval);
        require(result.state->r<4>(56)==std::uint32_t(sample),"Independent contact reaction signed sample");
        const auto reactions=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x10288f30;});
        require(reactions==int(sample==0),"Only a zero reaction sample calls the actor reaction dependency");
    }
    for(auto flag:{1u,2u,3u,4u,6u,7u,128u,255u}){Options o;o.resolved_contact=flag;boundaries(*resolution_seed(),o,15);}
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=resolution_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
        s->memory.write<1>(actor+0x106,column);boundaries(*s,{},15);
    }
    for(auto random:{0u,1u,2u,0x80000000u,0x80000001u,0xfffffffdu,0xffffffffu}){auto s=resolution_seed();s->memory.write<4>(globals,random);boundaries(*s,{},15);}
    for(auto trait:{0u,1u,2u,128u,255u})for(auto result:{0u,1u,0x100u,0x1ffu}) {
        auto s=resolution_seed();s->memory.write<1>(actor+0x366,trait);Options o;o.reaction_result=result;
        auto observed=compare(*s,o,15);const auto reactions=std::count_if(observed.calls.begin(),observed.calls.end(),[](const Event& e){return e.target==0x10288f30;});
        const auto transitions=std::count_if(observed.calls.begin(),observed.calls.end(),[](const Event& e){return e.target==0x1022ba10;});
        require(transitions==int(reactions!=0&&(result&255)!=0),"Actor transition uses only the reaction result low byte");
        boundaries(*s,o,15);
    }
    for(unsigned wide:{15u,16u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*resolution_seed(),o,15);}
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126}){auto s=resolution_seed();s->memory.write<4>(globals+8,cursor);boundaries(*s,{},15);}
    {auto s=resolution_seed();s->memory.write<4>(globals+8,stream+126);s->memory.write<1>(actor+0x366,0);Options o;o.resolved_contact=2;boundaries(*s,o,15);}
    {auto s=resolution_seed();s->memory.write<4>(globals,0x80000001);s->memory.write<1>(stack-0x150+0x43,128);boundaries(*s,{},15);}
    {Options o;o.mutate_resolution=true;boundaries(*resolution_seed(),o,15);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=resolution_seed();s->wr<4>(60,ptr);compare(*s,{},15);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*resolution_seed(),o,15);}
    for(auto state:{0u,2u,3u,4u})for(auto flag:{1u,2u,7u})for(auto column:{1u,6u,10u}) {
        auto s=resolution_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);
        s->memory.write<1>(actor+0x106,column);s->memory.write<1>(match+0x791f,state);Options o;o.resolved_contact=flag;compare(*s,o,16);
    }
    for(auto flag:{0u,1u}){auto s=resolution_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);
        Options o;o.contact_result=flag;boundaries(*s,o,16);}
    for(unsigned flag=0;flag<256;++flag)for(auto message:{0u,0x1ff9u,0x7fffu,0x8000u,0xffffu}) {
        auto s=outcome_seed();s->wr<4>(64,0xabcd0000|message);s->wr<4>(68,0x12341f7a);s->memory.write<1>(stack-0x150+0x41,flag);
        auto result=compare(*s,{},17);const auto responses=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101f1530;});
        require(responses==int(std::int32_t(signed_value<2>(message))>-1&&flag>=3&&flag<=6),"Independent contact response gate uses signed message and raw flag");
        require(result.state->memory.read<2>(actor+0x36)==225,"Contact outcome always subtracts twenty-five from the actor halfword");
        require(result.state->pc==action::return_phase,"Contact outcome leaves through the shared original return");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{0u,4u,128u,255u})for(auto column:{0u,1u,10u,128u,255u}) {
        auto s=outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);
        auto result=compare(*s,{},17);
        require(result.state->memory.read<1>(match+0x791d)==row&&result.state->memory.read<1>(match+0x791e)==column,"Ball receives raw actor coordinates");
        require(result.state->memory.read<1>(match+0x7924)==unsigned(side==0)&&result.state->memory.read<4>(match+0xe000)==0,"Contact ownership inversion and destination clearing");
    }
    for(auto side:{0u,1u,2u,255u})for(auto flag:{1u,7u})for(auto message:{0x1f79u,0x1f7au,0x1234u})
        for(auto result:{0u,recipient})for(auto disabled:{0u,1u,128u,255u}) {
            auto s=outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(stack-0x150+0x41,flag);
            s->memory.write<1>(stack-0x150+0x42,disabled);s->wr<4>(68,message);Options o;o.recipient_result=result;compare(*s,o,17);
        }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,6u,7u,128u})for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u}) {
        auto s=outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(stack-0x150+0x41,7);
        s->memory.write<1>(recipient+0x105,row);s->memory.write<1>(recipient+0x106,column);auto result=compare(*s,{},17);
        const bool inside=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);
        const auto comments=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x10201180&&read_be<4>(e.before.data()+16)==0x21e1;});
        require(comments==int(!inside),"Eligible contact-seven recipient outside the actor-side area receives the extra message");
    }
    for(auto actor_gate:{0u,1u,0x100u,0x1ffu})for(auto recipient_gate:{0u,1u,0x100u,0x1ffu})for(auto second:{0u,1u}) {
        Options o;o.actor_gate=actor_gate;o.recipient_gate=recipient_gate;o.recipient_second=second;boundaries(*outcome_seed(),o,17);
    }
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(stack-0x150+0x41,7);
        s->memory.write<1>(recipient+0x105,row);s->memory.write<1>(recipient+0x106,column);boundaries(*s,{},17);
    }
    for(auto message:{0x1f79u,0x1f7au,0x1234u})for(auto result:{0u,recipient})for(auto disabled:{0u,1u}) {
        auto s=outcome_seed();s->wr<4>(68,message);s->memory.write<1>(stack-0x150+0x42,disabled);Options o;o.recipient_result=result;boundaries(*s,o,17);
    }
    for(auto side:{0u,1u})for(auto pointer:{0u,staff})for(auto role:{0u,8u,255u}) {
        auto s=outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<1>(stack-0x150+0x43,127);
        s->memory.write<1>(actor+0x27,side);s->memory.write<4>((side?home:away)+0x2c,pointer);s->memory.write<1>(staff+0x119,role);boundaries(*s,{},17);
    }
    for(auto interval:{0u,1u,2u,127u,128u,255u}) {
        auto s=outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<1>(stack-0x150+0x43,interval);boundaries(*s,{},17);
    }
    for(auto wide:{17u,18u})for(auto sampled:{-100000,0,10000000}) {
        auto s=outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*s,o,17);
    }
    for(auto gate:{0u,1u,0x100u,0x1ffu}){Options o;o.contact_end_gate=gate;boundaries(*outcome_seed(),o,17);}
    for(auto message:{0x1f79u,0x1f7au})for(auto flag:{1u,3u,4u,5u,6u,7u,128u,255u})for(auto second:{2u,3u})for(auto third:{2u,3u}) {
        auto s=outcome_seed();s->wr<4>(68,message);s->memory.write<1>(stack-0x150+0x41,flag);s->memory.write<4>(globals,0);
        s->memory.write<2>(stream+2,18);s->memory.write<2>(stream+4,second);s->memory.write<2>(stream+6,third);boundaries(*s,{},17);
    }
    for(auto random:{0u,1u,0x7fffffffu,0x80000000u,0x80000001u,0xffffffffu}){
        auto s=outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,random);compare(*s,{},17);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126}) {
        auto s=outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<4>(globals+8,cursor);boundaries(*s,{},17);
    }
    {auto s=outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<4>(globals+8,stream+124);
        s->memory.write<1>(stack-0x150+0x43,0);boundaries(*s,{},17);}
    {auto s=outcome_seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},17);}
    for(auto limit:{stream+2,stream+4}) {
        auto s=outcome_seed();s->wr<4>(68,0x1f7a);s->memory.write<4>(globals,0);s->memory.write<4>(globals+4,limit);
        s->memory.write<2>(stream,3);s->memory.write<2>(stream+2,18);s->memory.write<2>(stream+4,3);Options o;o.random=0;boundaries(*s,o,17);
    }
    {Options o;o.mutate_outcome=true;auto s=outcome_seed();s->wr<4>(64,0x1ff9);boundaries(*s,o,17);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=outcome_seed();s->wr<4>(60,ptr);compare(*s,{},17);}
    for(auto value:{0u,1u,24u,25u,32768u,65535u}){auto s=outcome_seed();s->memory.write<2>(actor+0x36,value);compare(*s,{},17);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*outcome_seed(),o,17);}
    for(auto state:{0u,2u,3u,4u})for(auto flag:{1u,2u,7u})for(auto column:{1u,6u,10u}) {
        auto s=outcome_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);
        s->memory.write<1>(actor+0x106,column);s->memory.write<1>(match+0x791f,state);Options o;o.resolved_contact=flag;compare(*s,o,18);
    }
    for(auto flag:{0u,1u}){auto s=outcome_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);
        Options o;o.contact_result=flag;boundaries(*s,o,18);}
    for(auto rating:{0u,1u,199u,200u,201u,32767u,32768u,65535u})for(auto bound:{0u,200u,32767u,32768u,65535u})
        for(auto random:{0u,17u,18u,0x80000000u,0xffffffffu})for(auto flag:{0u,1u,0x100u,0x1ffu}) {
            auto s=execution_seed();s->wr<4>(68,0xabcd0000|bound);s->memory.write<4>(globals,random);s->memory.write<1>(match+0x7931,0);
            Options o;o.execution_rating=rating;o.execution_contact=flag;auto result=compare(*s,o,19);
            unsigned draws=0;const auto draw=[&](){return std::bit_cast<std::int32_t>(random+100u+2*++draws);};
            const bool attempt=signed_value<2>(rating)>=signed_value<2>(bound)||draw()%5==0;
            const bool immediate=attempt&&(flag&255)!=0&&draw()%30==0;
            require(result.state->pc==(attempt&&(flag&255)&&!immediate?action::execution_message_phase:action::execution_result_phase),"Independent receiver-contact attempt and immediate-action decision");
            require(result.state->memory.read<4>(match+0xe000)==0,"Selected actor ownership is cleared after the notification");
            require(result.state->memory.read<4>(globals+8)==stream+draws*2,"Receiver-contact random consumption follows the live gates");
        }
    for(auto target:{0u,recipient,actor,actor+1,stack-0x150}) {
        Options o;o.execution_receiver=target;auto result=compare(*execution_seed(),o,19);
        if(!target){require(result.state->pc==action::no_receiver_phase&&result.calls.size()==2,"Null receiver is reported before the distinct fallback");boundaries(*execution_seed(),o,19);}
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,6u,7u,128u,255u})for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u}) {
        auto s=execution_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(recipient+0x105,row);s->memory.write<1>(recipient+0x106,column);
        auto result=compare(*s,{},19);const bool inside=row>=2&&row<=6&&(side==0?(column>=10&&column<=11):column<=1);
        const auto checks=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101ee530;});
        require(checks==int(inside),"Receiver warning checks use the inverted actor side and live receiver coordinates");
    }
    for(auto trait:{0u,1u,20u,127u,32767u,32768u,65516u,65535u})for(auto random:{0u,17u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=execution_seed();s->memory.write<2>(recipient+0x32,trait);s->memory.write<4>(globals,random);compare(*s,{},19);
    }
    for(auto enabled:{0u,1u,128u,255u})for(auto gate:{0u,1u,0x100u,0x1ffu})for(auto blocked:{0u,1u,128u,255u}) {
        auto s=execution_seed();s->memory.write<1>(match+0x7931,enabled);Options o;o.contact_end_gate=gate;o.execution_blocked=blocked;compare(*s,o,19);
    }
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=execution_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(recipient+0x105,row);s->memory.write<1>(recipient+0x106,column);boundaries(*s,{},19);
    }
    for(auto trait:{0u,20u,32768u,65516u}){auto s=execution_seed();s->memory.write<2>(recipient+0x32,trait);boundaries(*s,{},19);}
    for(auto wide:{19u,20u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*execution_seed(),o,19);}
    for(auto gate:{0u,1u})for(auto blocked:{0u,1u}){Options o;o.contact_end_gate=gate;o.execution_blocked=blocked;boundaries(*execution_seed(),o,19);}
    for(auto disabled:{0u,1u,128u,255u})for(auto contact:{0u,1u,0x100u,0x1ffu}) {
        auto s=execution_seed();s->memory.write<1>(match+0x7931,0);s->memory.write<4>(globals,18);Options o;o.execution_disabled=disabled;o.execution_contact=contact;boundaries(*s,o,19);
    }
    for(unsigned kind=0;kind<256;++kind){auto s=execution_seed();s->memory.write<1>(match+0x7931,0);Options o;o.execution_kind=kind;compare(*s,o,19);}
    for(auto cursor:{stream+122,stream+124,stream+126})for(auto gate:{0u,1u}) {
        auto s=execution_seed();s->memory.write<4>(globals+8,cursor);Options o;o.contact_end_gate=gate;boundaries(*s,o,19);
    }
    {auto s=execution_seed();s->memory.write<4>(globals+8,stream+126);Options o;o.execution_rating=0;boundaries(*s,o,19);}
    {auto s=execution_seed();s->memory.write<4>(globals+8,stream+126);s->memory.write<1>(match+0x7931,0);boundaries(*s,{},19);}
    for(auto gate:{0u,1u}){Options o;o.mutate_execution=true;o.contact_end_gate=gate;boundaries(*execution_seed(),o,19);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=execution_seed();s->wr<4>(60,ptr);compare(*s,{},19);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*execution_seed(),o,19);}
    for(auto state:{0u,2u,3u,4u})for(auto contact:{0u,1u})for(auto target:{0u,recipient}) {
        auto s=execution_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=contact;o.execution_receiver=target;compare(*s,o,20);
    }
    for(auto target:{0u,recipient}){auto s=execution_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);
        Options o;o.contact_result=0;o.execution_receiver=target;boundaries(*s,o,20);}
    const std::array<std::pair<std::uint32_t,unsigned>,12> execution_messages{{
        {0x1023ff6c,0x1fea},{0x1023ff98,0x1feb},{0x1023ffc4,0x1fec},{0x1023fff0,0x1fed},
        {0x1024001c,0x1fee},{0x10240048,0x1ff2},{0x10240074,0x1fef},{0x102400a0,0x1ff1},
        {0x102400cc,0x1ff0},{0x102400f8,0x1ff3},{0x10240124,0x1ff4},{0x10240150,0x1fe9}}};
    for(unsigned kind=0;kind<256;++kind) {
        auto s=execution_message_seed();s->memory.write<1>(stack-0x150+0x48,kind);auto result=compare(*s,{},21);
        const auto target=kind<=46?original_execution_message_table[kind]:0x10240150;
        const auto expected=std::find_if(execution_messages.begin(),execution_messages.end(),[&](const auto& entry){return entry.first==target;})->second;
        require(result.calls.size()==1&&read_be<4>(result.calls[0].before.data()+16)==expected,"Original action table and signed-to-unsigned bounds select the message");
    }
    for(unsigned index=0;index<47;++index)for(auto [target,message]:execution_messages) {
        auto s=execution_message_seed();s->memory.write<1>(stack-0x150+0x48,index);s->memory.write<4>(execution_table+index*4,target);
        auto result=compare(*s,{},21);require(read_be<4>(result.calls[0].before.data()+16)==message,"Live table replacement overrides the original action mapping");
    }
    for(auto [target,message]:execution_messages) {
        auto s=execution_message_seed();s->memory.write<4>(execution_table+45*4,target);boundaries(*s,{},21);
        s->pc=target;auto result=compare(*s,{},21);require(read_be<4>(result.calls[0].before.data()+16)==message,"Direct destination does not replay table access");boundaries(*s,{},21);
    }
    for(auto target:{0u,action::receiver_resolution_phase,action::execution_result_phase,action::no_receiver_phase,action::return_phase,return_sentinel,0x12345679u}) {
        auto s=execution_message_seed();s->memory.write<4>(execution_table+45*4,target);auto result=compare(*s,{},21);
        require(result.state->pc==target&&result.calls.empty(),"Non-message live target remains an explicit continuation");boundaries(*s,{},21);
    }
    for(auto pointer:{0u,execution_table+1,execution_table+0x200,toc-0x5204,stack-0x150}) {
        auto s=execution_message_seed();s->memory.write<4>(toc-0x5204,pointer);compare(*s,{},21);
    }
    {Options o;o.mutate_table=true;boundaries(*execution_message_seed(),o,21);}
    for(auto kind:{47u,128u,255u}){auto s=execution_message_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<4>(toc-0x5204,0);boundaries(*s,{},21);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*execution_message_seed(),o,21);}
    for(auto state:{0u,2u,3u,4u})for(auto contact:{0u,1u})for(auto target:{0u,recipient}) {
        auto s=execution_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=contact;o.execution_receiver=target;compare(*s,o,22);
    }
    for(auto target:{0u,recipient}){auto s=execution_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);
        Options o;o.contact_result=0;o.execution_receiver=target;boundaries(*s,o,22);}
    {auto s=execution_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.mutate_execution=true;boundaries(*s,o,22);}
    for(unsigned flag=0;flag<256;++flag)for(auto coordinate:{0u,1u,2u,3u,127u,128u,255u}) {
        auto s=receiver_seed();s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<1>(stack-0x150+0x43,coordinate);
        auto result=compare(*s,{},23);const auto message=flag==2?0x1ff9u:flag==3?0x1ff7u:flag==4?0x1ff8u:flag==5?0x1ff6u:
            std::int32_t(signed_value<1>(coordinate))>2?0x210au:0xffffffffu;
        require(result.state->r<4>(64)==message,"Receiver-contact classification distinguishes five from six");
        require(result.state->memory.read<1>(match+0x7928)==(flag==7?7u:5u),"Receiver-contact seven bypasses area effects");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{0u,1u,2u,4u,6u,7u,128u,255u})
        for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u,255u})for(auto flag:{1u,2u,7u}) {
            auto s=receiver_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
            s->memory.write<1>(actor+0x106,column);s->memory.write<1>(stack-0x150+0x40,flag);auto result=compare(*s,{},23);
            const bool inside=row>=2&&row<=6&&(side==0?(column>=10&&column<=11):column<=1);
            const auto status=flag==7?7u:inside?5u:6u;
            require(result.state->memory.read<1>(match+0x7928)==status&&result.state->r<4>(68)==(status==5?0x1f7au:0x1f79u),"Independent receiver-contact inverted area decision");
            require(result.state->memory.read<2>(recipient+0x36)==(status==5?0xff9cu:400u),"Receiver-contact decrement applies to the selected recipient");
            require(result.state->memory.read<2>(actor+0x36)==250,"Receiver-contact leaves the initiating actor halfword intact");
            require(result.state->memory.read<4>(match+0x3110)==(status==5?actor:0u),"Major receiver contact retains the initiating actor link");
        }
    for(auto trait:{0u,1u,2u,7u,20u,119u,127u,128u,255u})for(auto random:{0u,17u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=receiver_seed();s->memory.write<1>(recipient+0x366,trait);s->memory.write<4>(globals,random);auto result=compare(*s,{},23);
        const auto interval=std::int32_t(signed_value<1>(trait)),number=std::bit_cast<std::int32_t>(random+102u);
        const auto sample=interval==0?0:interval>0?number%interval:-(number%interval);
        require(result.state->r<4>(52)==std::uint32_t(sample),"Independent receiver reaction sample replaces the earlier action register");
    }
    for(auto flag:{1u,2u,3u,4u,5u,6u,7u,128u,255u}){auto s=receiver_seed();s->memory.write<1>(stack-0x150+0x40,flag);boundaries(*s,{},23);}
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=receiver_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
        s->memory.write<1>(actor+0x106,column);boundaries(*s,{},23);
    }
    for(auto random:{0u,1u,2u,0x80000000u,0x80000001u,0xffffffffu}) {
        auto s=receiver_seed();s->memory.write<1>(stack-0x150+0x40,1);s->memory.write<1>(stack-0x150+0x43,128);s->memory.write<4>(globals,random);boundaries(*s,{},23);
    }
    for(auto trait:{0u,1u,2u,128u,255u})for(auto result:{0u,1u,0x100u,0x1ffu}) {
        auto s=receiver_seed();s->memory.write<1>(recipient+0x366,trait);Options o;o.reaction_result=result;boundaries(*s,o,23);
    }
    for(auto wide:{21u,22u})for(auto sampled:{-100000,0,10000000}) {
        auto s=receiver_seed();s->memory.write<1>(recipient+0x366,2);Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*s,o,23);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126})for(auto flag:{1u,2u})for(auto trait:{0u,2u}) {
        auto s=receiver_seed();s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<1>(recipient+0x366,trait);s->memory.write<4>(globals+8,cursor);boundaries(*s,{},23);
    }
    {auto s=receiver_seed();s->memory.write<1>(stack-0x150+0x40,1);Options o;o.mutate_outcome=true;boundaries(*s,o,23);}
    for(auto ptr:{0u,actor,recipient+1,stack-0x150,globals}){auto s=receiver_seed();s->wr<4>(56,ptr);compare(*s,{},23);}
    for(auto value:{0u,1u,499u,500u,32768u,65535u}){auto s=receiver_seed();s->memory.write<2>(recipient+0x36,value);compare(*s,{},23);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*receiver_seed(),o,23);}
    for(auto state:{0u,2u,3u,4u})for(auto contact:{0u,1u})for(auto target:{0u,recipient})for(auto flag:{1u,2u,7u}) {
        auto s=receiver_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=contact;o.execution_receiver=target;o.resolved_contact=flag;compare(*s,o,24);
    }
    for(auto flag:{1u,2u,7u}){auto s=receiver_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);
        Options o;o.contact_result=0;o.resolved_contact=flag;boundaries(*s,o,24);}
    for(unsigned flag=0;flag<256;++flag)for(auto message:{0u,0x1ff9u,0x7fffu,0x8000u,0xffffu})for(auto kind:{44u,45u,47u}) {
        auto s=receiver_outcome_seed();s->wr<4>(64,0xabcd0000|message);s->memory.write<1>(stack-0x150+0x40,flag);
        s->memory.write<1>(stack-0x150+0x42,1);s->memory.write<1>(stack-0x150+0x48,kind);auto o=receiver_outcome_options();o.contact_end_gate=1;
        auto result=compare(*s,o,25);const bool selected=signed_value<2>(message)>-1;
        const auto responses=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101f1530;});
        require(responses==int(selected&&flag>=3&&flag<=6),"Receiver response gate uses the signed message and raw contact flag");
        const bool marked=selected&&(kind==44||kind==47)&&flag>=3&&flag<=6;
        require(result.state->memory.read<1>(recipient+0x16)==(marked?1u:0x5au),"Blocked action kinds mark only the contacted receiver after their live contact gate");
        require(result.state->memory.read<2>(recipient+0x36)==375&&result.state->memory.read<2>(actor+0x36)==250,"Receiver outcome subtracts twenty-five only from the contacted player");
        for(const auto& event:result.calls)if(event.target==0x101f1530)require(read_be<4>(event.before.data()+16)==recipient,"Contact response receives the contacted player, not the actor or restart player");
    }
    for(unsigned flag=0;flag<256;++flag)for(auto message:{0x1f79u,0x1f7au}) {
        auto s=receiver_outcome_seed();s->wr<4>(68,message);s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<1>(stack-0x150+0x42,1);
        auto result=compare(*s,receiver_outcome_options(),25);
        require(result.state->memory.read<1>(recipient+0x16)==(flag==4||flag==5?1u:0x5au),"Blocked receiver ending marks raw contact four or five independently of restart type");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{0u,4u,128u,255u})for(auto column:{0u,1u,10u,128u,255u}) {
        auto s=receiver_outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);
        auto result=compare(*s,receiver_outcome_options(),25);
        require(result.state->memory.read<1>(match+0x791d)==row&&result.state->memory.read<1>(match+0x791e)==column&&result.state->memory.read<1>(match+0x7924)==side,"Receiver outcome keeps the initiating actor raw coordinates and side");
        for(const auto& event:result.calls)if(event.target==0x101f3a30)require(read_be<4>(event.before.data()+16)==side,"Restart selection receives the raw initiating side");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,6u,7u,128u})for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u}) {
        auto s=receiver_outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
        s->memory.write<1>(actor+0x106,column);s->memory.write<1>(stack-0x150+0x40,7);auto result=compare(*s,receiver_outcome_options(),25);
        const bool inside=row>=2&&row<=6&&(side==0?(column>=10&&column<=11):column<=1);
        const auto comments=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x10201180&&read_be<4>(e.before.data()+16)==0x21e1;});
        require(comments==int(!inside),"Restart follow-up checks the initiating actor inverted area");
        for(const auto& event:result.calls) {
            if(event.target==0x10272150)require(read_be<4>(event.before.data()+12)==actor,"Restart eligibility checks the initiating actor");
            if(event.target==0x10201180&&read_be<4>(event.before.data()+16)==0x21e1)
                require(read_be<4>(event.before.data()+24)==34,"Restart commentary identifies the separately selected restart player");
        }
    }
    for(auto kind:{0u,44u,45u,47u,128u,255u})for(auto disabled:{0u,1u,128u,255u})for(auto message:{0x1f79u,0x1f7au}) {
        auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->wr<4>(68,message);s->memory.write<1>(stack-0x150+0x48,kind);
        s->memory.write<1>(stack-0x150+0x42,disabled);boundaries(*s,receiver_outcome_options(),25);
    }
    for(auto target:{0u,restart_player})for(auto message:{0x1f79u,0x1f7au,0x1234u})for(auto flag:{1u,7u}) {
        auto s=receiver_outcome_seed();s->wr<4>(68,message);s->memory.write<1>(stack-0x150+0x40,flag);auto o=receiver_outcome_options();o.recipient_result=target;boundaries(*s,o,25);
    }
    for(auto actor_gate:{0u,1u,0x100u,0x1ffu})for(auto player_gate:{0u,1u,0x100u,0x1ffu})for(auto second:{0u,1u}) {
        auto o=receiver_outcome_options();o.actor_gate=actor_gate;o.recipient_gate=player_gate;o.recipient_second=second;boundaries(*receiver_outcome_seed(),o,25);
    }
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=receiver_outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);
        s->memory.write<1>(stack-0x150+0x40,7);boundaries(*s,receiver_outcome_options(),25);
    }
    for(auto side:{0u,1u})for(auto pointer:{0u,staff})for(auto role:{0u,8u,255u}) {
        auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<1>(stack-0x150+0x43,127);
        s->memory.write<1>(recipient+0x27,side);s->memory.write<4>((side?home:away)+0x2c,pointer);s->memory.write<1>(staff+0x119,role);boundaries(*s,receiver_outcome_options(),25);
    }
    for(auto interval:{0u,1u,2u,127u,128u,255u}) {
        auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<1>(stack-0x150+0x43,interval);boundaries(*s,receiver_outcome_options(),25);
    }
    for(auto wide:{23u,24u})for(auto sampled:{-100000,0,10000000}) {
        auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);auto o=receiver_outcome_options();o.force_wide=wide;o.sampled=sampled;boundaries(*s,o,25);
    }
    for(auto gate:{0u,1u,0x100u,0x1ffu}){auto o=receiver_outcome_options();o.contact_end_gate=gate;boundaries(*receiver_outcome_seed(),o,25);}
    for(auto message:{0x1f79u,0x1f7au})for(auto flag:{1u,3u,4u,5u,6u,7u,128u,255u})for(auto second:{2u,3u})for(auto third:{2u,3u}) {
        auto s=receiver_outcome_seed();s->wr<4>(68,message);s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<4>(globals,0);
        s->memory.write<2>(stream+2,18);s->memory.write<2>(stream+4,second);s->memory.write<2>(stream+6,third);boundaries(*s,receiver_outcome_options(),25);
    }
    for(auto random:{0u,1u,0x7fffffffu,0x80000000u,0x80000001u,0xffffffffu}) {
        auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,random);compare(*s,receiver_outcome_options(),25);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126}) {
        auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<4>(globals+8,cursor);boundaries(*s,receiver_outcome_options(),25);
    }
    {auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<4>(globals,2);s->memory.write<4>(globals+8,stream+124);
        s->memory.write<1>(stack-0x150+0x43,0);boundaries(*s,receiver_outcome_options(),25);}
    for(auto blocked:{0u,1u}){auto s=receiver_outcome_seed();s->wr<4>(68,0x1f7a);s->memory.write<1>(stack-0x150+0x42,blocked);
        s->memory.write<4>(globals+8,stream+126);boundaries(*s,receiver_outcome_options(),25);}
    for(auto limit:{stream+2,stream+4}) {
        auto s=receiver_outcome_seed();s->wr<4>(68,0x1f7a);s->memory.write<4>(globals,0);s->memory.write<4>(globals+4,limit);
        s->memory.write<2>(stream,3);s->memory.write<2>(stream+2,18);s->memory.write<2>(stream+4,3);auto o=receiver_outcome_options();o.random=0;boundaries(*s,o,25);
    }
    for(bool execution:{false,true}){auto s=receiver_outcome_seed();s->wr<4>(64,0x1ff9);s->memory.write<1>(stack-0x150+0x42,1);s->memory.write<1>(stack-0x150+0x48,44);
        auto o=receiver_outcome_options();o.mutate_execution=execution;o.mutate_outcome=!execution;boundaries(*s,o,25);}
    for(auto ptr:{0u,actor,recipient+1,stack-0x150,globals}){auto s=receiver_outcome_seed();s->wr<4>(56,ptr);compare(*s,receiver_outcome_options(),25);}
    for(auto value:{0u,1u,24u,25u,32768u,65535u}){auto s=receiver_outcome_seed();s->memory.write<2>(recipient+0x36,value);compare(*s,receiver_outcome_options(),25);}
    for(bool child:{false,true}){auto o=receiver_outcome_options();o.own_negative=!child;o.child_negative=child;compare(*receiver_outcome_seed(),o,25);}
    for(auto state:{0u,2u,3u,4u})for(auto contact:{0u,1u})for(auto target:{0u,recipient})for(auto flag:{1u,2u,7u}) {
        auto s=receiver_outcome_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);auto o=receiver_outcome_options();
        o.contact_result=contact;o.execution_receiver=target;o.resolved_contact=flag;compare(*s,o,26);
    }
    for(auto flag:{1u,2u,7u}){auto s=receiver_outcome_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);
        auto o=receiver_outcome_options();o.contact_result=0;o.resolved_contact=flag;boundaries(*s,o,26);}
    for(unsigned flag=0;flag<256;++flag) {
        auto s=result_seed();s->memory.write<1>(stack-0x150+0x40,flag);auto result=compare(*s,{},27);
        if(flag)require(result.state->pc==action::unopposed_result_phase&&result.calls.empty()&&result.state->memory.read<1>(stack-0x150+0x40)==0,
            "Existing execution contact clears without retrying or drawing random");
    }
    for(auto score:{0u,100u,101u,32767u,32768u,65535u})for(auto rating:{0u,100u,32767u,32768u,65535u})for(auto random:{0u,17u,18u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=result_seed();s->wr<4>(68,0xabcd0000|score);s->memory.write<2>(stack-0x150+0x4a,rating);s->memory.write<4>(globals,random);
        auto result=compare(*s,{},27);const auto count=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101f5050;});
        require(count==int(signed_value<2>(score)>signed_value<2>(rating)||std::bit_cast<std::int32_t>(random+102u)%5==0),
            "Execution contact attempt uses signed score and rating, then the original random fifth");
    }
    for(auto enabled:{0u,1u,128u,255u})for(auto gate:{0u,1u,0x100u,0x1ffu})for(auto blocked:{0u,1u,128u,255u}) {
        auto s=result_seed();s->memory.write<1>(match+0x7931,enabled);Options o;o.contact_end_gate=gate;o.execution_blocked=blocked;compare(*s,o,27);
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,6u,7u})for(auto column:{0u,1u,2u,9u,10u,11u,12u}) {
        auto s=result_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);compare(*s,{},27);
    }
    for(auto trait:{0u,1u,20u,32767u,32768u,65516u,65535u})for(auto random:{0u,17u,0x80000000u,0xffffffffu}) {
        auto s=result_seed();s->memory.write<2>(recipient+0x32,trait);s->memory.write<4>(globals,random);compare(*s,{},27);
    }
    for(auto trait:{0u,20u,32768u,65516u}){auto s=result_seed();s->memory.write<2>(recipient+0x32,trait);boundaries(*s,{},27);}
    for(auto wide:{25u,26u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*result_seed(),o,27);}
    for(auto contact:{0u,1u,0x100u,0x1ffu})for(auto blocked:{0u,1u}){Options o;o.result_contact=contact;o.execution_blocked=blocked;boundaries(*result_seed(),o,27);}
    for(auto score:{0u,200u})for(auto random:{0u,17u,18u}) {
        auto s=result_seed();s->wr<4>(68,score);s->memory.write<4>(globals,random);s->memory.write<1>(match+0x7931,0);boundaries(*s,{},27);
    }
    {auto s=result_seed();s->memory.write<4>(globals,18);s->memory.write<1>(match+0x7931,0);auto result=compare(*s,{},27);
        require(result.state->pc==action::unopposed_result_phase&&std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101d68f0;})==1,
            "Random tenth reports and clears the newly attempted contact before resolution");}
    for(auto flag:{1u,128u,255u}){auto s=result_seed();s->memory.write<1>(stack-0x150+0x40,flag);boundaries(*s,{},27);}
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=result_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);boundaries(*s,{},27);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126})for(auto score:{0u,200u})for(auto gate:{0u,1u}) {
        auto s=result_seed();s->wr<4>(68,score);s->memory.write<4>(globals+8,cursor);Options o;o.contact_end_gate=gate;boundaries(*s,o,27);
    }
    {Options o;o.mutate_result=true;boundaries(*result_seed(),o,27);}
    for(auto ptr:{0u,actor,recipient+1,stack-0x150,globals}){auto s=result_seed();s->wr<4>(56,ptr);compare(*s,{},27);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*result_seed(),o,27);}
    for(auto state:{0u,2u,3u,4u})for(auto contact:{0u,1u})for(auto target:{0u,recipient})for(auto flag:{0u,1u,7u}) {
        auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;
        o.contact_result=contact;o.execution_receiver=target;o.resolved_contact=flag;compare(*s,o,28);
    }
    for(auto contact:{0u,1u}){auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.execution_contact=contact;boundaries(*s,o,28);}
    const std::array<std::pair<std::uint32_t,unsigned>,12> result_messages{{
        {0x10241474,0x1fea},{0x102414a0,0x1feb},{0x102414cc,0x1fec},{0x102414f8,0x1fed},
        {0x10241524,0x1fee},{0x10241550,0x1ff2},{0x1024157c,0x1fef},{0x102415a8,0x1ff1},
        {0x102415d4,0x1ff0},{0x10241600,0x1ff3},{0x1024162c,0x1ff4},{0x10241658,0x1fe9}}};
    for(unsigned kind=0;kind<256;++kind) {
        auto s=result_message_seed();s->wr<4>(52,0xabcd0000|kind);auto result=compare(*s,{},29);
        const auto target=kind<=56?original_result_message_table[kind]:0x10241658;
        const auto expected=std::find_if(result_messages.begin(),result_messages.end(),[&](const auto& entry){return entry.first==target;})->second;
        require(result.calls.size()==1&&read_be<4>(result.calls[0].before.data()+16)==expected,"Second live table uses the saved action register and its own 57-word extent");
        const auto& before=result.calls[0].before;
        require(read_be<4>(before.data()+24)==12&&read_be<4>(before.data()+32)==23,"Execution-result messages put the initiating player before the receiver");
    }
    for(unsigned index=0;index<57;++index)for(auto [target,message]:result_messages) {
        auto s=result_message_seed();s->wr<4>(52,index);s->memory.write<4>(result_table+index*4,target);auto result=compare(*s,{},29);
        require(read_be<4>(result.calls[0].before.data()+16)==message,"Live result-table edits override the original mapping");
    }
    for(auto [target,message]:result_messages) {
        auto s=result_message_seed();s->memory.write<4>(result_table+45*4,target);boundaries(*s,{},29);s->pc=target;auto result=compare(*s,{},29);
        require(read_be<4>(result.calls[0].before.data()+16)==message,"Direct result-message entry skips table access");boundaries(*s,{},29);
    }
    for(auto target:{0u,action::result_resolution_phase,action::unopposed_result_phase,action::return_phase,return_sentinel,0x12345679u}) {
        auto s=result_message_seed();s->memory.write<4>(result_table+45*4,target);auto result=compare(*s,{},29);
        require(result.state->pc==target&&result.calls.empty(),"Non-message result-table target remains an explicit continuation");boundaries(*s,{},29);
    }
    for(auto pointer:{0u,result_table+1,result_table+0x200,toc-0x5208,stack-0x150}){auto s=result_message_seed();s->memory.write<4>(toc-0x5208,pointer);compare(*s,{},29);}
    {Options o;o.mutate_result_table=true;boundaries(*result_message_seed(),o,29);}
    for(auto kind:{57u,128u,255u}){auto s=result_message_seed();s->wr<4>(52,kind);s->memory.write<4>(toc-0x5208,0);boundaries(*s,{},29);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*result_message_seed(),o,29);}
    for(auto state:{0u,2u,3u,4u})for(auto contact:{0u,1u})for(auto flag:{0u,1u,7u}) {
        auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=contact;o.resolved_contact=flag;compare(*s,o,30);
    }
    {auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.mutate_result=true;boundaries(*s,o,30);}
    for(unsigned flag=0;flag<256;++flag)for(auto coordinate:{0u,1u,2u,3u,127u,128u,255u}) {
        auto s=result_resolution_seed();s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<1>(stack-0x150+0x43,coordinate);auto result=compare(*s,{},31);
        const auto message=flag==2?0x1ff9u:flag==3?0x1ff7u:flag==4?0x1ff8u:flag==5?0x1ff6u:std::int32_t(signed_value<1>(coordinate))>2?0x210au:0xffffffffu;
        require(result.state->r<4>(64)==message,"Execution-result classification preserves contact five versus six and the signed coordinate gate");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{0u,1u,2u,4u,6u,7u,128u,255u})for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u,255u})for(auto flag:{1u,2u,7u}) {
        auto s=result_resolution_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);s->memory.write<1>(stack-0x150+0x40,flag);
        auto result=compare(*s,{},31);const bool inside=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);const auto status=flag==7?7u:inside?5u:6u;
        require(result.state->memory.read<1>(match+0x7928)==status&&result.state->r<4>(68)==(status==5?0x1f7au:0x1f79u),"Execution-result area uses the initiating side without inversion");
        require(result.state->memory.read<2>(actor+0x36)==(status==5?0xff06u:250u)&&result.state->memory.read<2>(recipient+0x36)==400,"Execution-result decrement applies only to the initiating actor");
        require(result.state->memory.read<4>(match+0x3110)==(status==5?recipient:0u),"Execution-result major contact retains the selected receiver link");
    }
    for(auto trait:{0u,1u,2u,7u,20u,119u,127u,128u,255u})for(auto random:{0u,17u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=result_resolution_seed();s->memory.write<1>(actor+0x366,trait);s->memory.write<4>(globals,random);compare(*s,{},31);
    }
    for(auto flag:{1u,2u,3u,4u,5u,6u,7u,128u,255u}){auto s=result_resolution_seed();s->memory.write<1>(stack-0x150+0x40,flag);boundaries(*s,{},31);}
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=result_resolution_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);boundaries(*s,{},31);
    }
    for(auto random:{0u,1u,2u,0x80000000u,0x80000001u,0xffffffffu}) {
        auto s=result_resolution_seed();s->memory.write<1>(stack-0x150+0x40,1);s->memory.write<1>(stack-0x150+0x43,128);s->memory.write<4>(globals,random);boundaries(*s,{},31);
    }
    for(auto trait:{0u,1u,2u,128u,255u})for(auto response:{0u,1u,0x100u,0x1ffu}) {
        auto s=result_resolution_seed();s->memory.write<1>(actor+0x366,trait);Options o;o.reaction_result=response;boundaries(*s,o,31);
    }
    for(auto wide:{27u,28u})for(auto sampled:{-100000,0,10000000}) {
        auto s=result_resolution_seed();s->memory.write<1>(actor+0x366,2);Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*s,o,31);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126})for(auto flag:{1u,2u})for(auto trait:{0u,2u}) {
        auto s=result_resolution_seed();s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<1>(actor+0x366,trait);s->memory.write<4>(globals+8,cursor);boundaries(*s,{},31);
    }
    {auto s=result_resolution_seed();s->memory.write<1>(stack-0x150+0x40,1);Options o;o.mutate_outcome=true;boundaries(*s,o,31);}
    for(auto value:{0u,1u,499u,500u,32768u,65535u}){auto s=result_resolution_seed();s->memory.write<2>(actor+0x36,value);compare(*s,{},31);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*result_resolution_seed(),o,31);}
    for(auto state:{0u,2u,3u,4u})for(auto contact:{0u,1u})for(auto flag:{0u,1u,2u,7u}) {
        auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=contact;o.resolved_contact=flag;compare(*s,o,32);
    }
    for(auto flag:{0u,1u,7u}){auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.resolved_contact=flag;boundaries(*s,o,32);}
    for(auto phase:{28u,30u,32u}) {
        auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x7931,0);Options o;
        o.contact_result=0;o.execution_contact=0;o.execution_rating=0;o.resolved_contact=2;const auto result=compare(*s,o,phase);
        require(std::any_of(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101f5110&&e.continuation==action::result_message_phase;}),
            "Original-root prefix reaches the second contact and result resolver");
        boundaries(*s,o,phase);
    }
    for(unsigned flag=0;flag<256;++flag)for(auto message:{0u,0x1ff9u,0x7fffu,0x8000u,0xffffu}) {
        auto s=result_outcome_seed();s->wr<4>(64,0xabcd0000|message);s->memory.write<1>(stack-0x150+0x40,flag);Options o;o.contact_end_gate=1;
        auto result=compare(*s,o,33);const auto count=std::count_if(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x101f1530;});
        require(count==int(signed_value<2>(message)>-1&&flag>=3&&flag<=6),"Execution-result response uses the signed message and raw contact class");
        for(const auto& event:result.calls)if(event.target==0x101f1530)require(read_be<4>(event.before.data()+16)==actor,"Execution-result response targets the initiating player");
    }
    for(unsigned flag=0;flag<256;++flag)for(auto restart:{0x1f79u,0x1f7au}) {
        auto s=result_outcome_seed();s->wr<4>(68,restart);s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<1>(stack-0x150+0x42,1);
        auto result=compare(*s,{},33);
        require(result.state->memory.read<1>(recipient+0x16)==(flag==4||flag==5||flag==6?1u:0x5au),"Blocked execution-result ending marks receiver contact six as well as four and five");
        require(result.state->memory.read<2>(actor+0x36)==225&&result.state->memory.read<2>(recipient+0x36)==400,"Execution-result outcome subtracts twenty-five only from the initiating player");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{0u,4u,128u,255u})for(auto column:{0u,1u,10u,128u,255u}) {
        auto s=result_outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);
        auto result=compare(*s,{},33);require(result.state->memory.read<1>(match+0x791d)==row&&result.state->memory.read<1>(match+0x791e)==column&&result.state->memory.read<1>(match+0x7924)==unsigned(side==0),
            "Execution-result outcome publishes the raw initiating coordinates and inverted side");
        for(const auto& event:result.calls)if(event.target==0x101f3a30)require(read_be<4>(event.before.data()+16)==unsigned(side==0),"Result restart selection receives the inverted initiating side");
    }
    for(auto restart:{0x1f79u,0x1f7au})for(auto flag:{1u,2u,3u,4u,5u,6u,7u,128u,255u})for(auto blocked:{0u,1u}) {
        auto s=result_outcome_seed();s->wr<4>(68,restart);s->memory.write<1>(stack-0x150+0x40,flag);s->memory.write<1>(stack-0x150+0x42,blocked);boundaries(*s,{},33);
    }
    for(auto player:{0u,restart_player})for(auto gate:{0u,1u,0x100u,0x1ffu})for(auto again:{0u,1u})for(auto actor_gate:{0u,1u}) {
        auto s=result_outcome_seed();Options o;o.recipient_result=player;o.recipient_gate=gate;o.recipient_second=again;o.actor_gate=actor_gate;compare(*s,o,33);
    }
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u})for(auto flag:{1u,7u}) {
        auto s=result_outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(restart_player+0x105,row);s->memory.write<1>(restart_player+0x106,column);s->memory.write<1>(stack-0x150+0x40,flag);
        auto o=receiver_outcome_options();boundaries(*s,o,33);
        for(const auto& event:compare(*s,o,33).calls)if(event.target==0x10272150)require(read_be<4>(event.before.data()+12)==restart_player,"Result followup queries the selected restart player");
    }
    for(auto player:{0u,restart_player}){auto o=receiver_outcome_options();o.recipient_result=player;boundaries(*result_outcome_seed(),o,33);}
    for(auto gate:{0u,1u})for(auto again:{0u,1u})for(auto actor_gate:{0u,1u}) {
        auto o=receiver_outcome_options();o.recipient_gate=gate;o.recipient_second=again;o.actor_gate=actor_gate;boundaries(*result_outcome_seed(),o,33);
    }
    for(auto side:{0u,1u,2u})for(auto pointer:{0u,staff})for(auto role:{0u,8u,9u}) {
        auto s=result_outcome_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<4>(home+0x2c,pointer);s->memory.write<4>(away+0x2c,pointer);s->memory.write<1>(staff+0x119,role);
        s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,4);s->memory.write<2>(stream+4,99);s->memory.write<2>(stream+6,0);s->memory.write<1>(stack-0x150+0x43,100);boundaries(*s,{},33);
    }
    for(auto wide:{29u,30u})for(auto sampled:{-100000,0,10000000}) {
        auto s=result_outcome_seed();s->memory.write<4>(globals,18);Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*s,o,33);
    }
    for(auto coordinate:{0u,1u,127u,128u,255u})for(auto random:{0u,1u,2u,17u,18u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=result_outcome_seed();s->memory.write<1>(stack-0x150+0x43,coordinate);s->memory.write<4>(globals,random);compare(*s,{},33);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126})for(auto random:{17u,18u}) {
        auto s=result_outcome_seed();s->memory.write<4>(globals+8,cursor);s->memory.write<4>(globals,random);boundaries(*s,{},33);
    }
    for(auto coordinate:{0u,1u}) {
        auto s=result_outcome_seed();s->memory.write<4>(globals,18);s->memory.write<4>(globals+8,stream+(coordinate?122:124));
        s->memory.write<2>(stream+124,2);s->memory.write<1>(stack-0x150+0x43,coordinate);boundaries(*s,{},33);
    }
    for(auto blocked:{0u,1u})for(auto limit:{stream-2,stream,stream+2}) {
        auto s=result_outcome_seed();s->wr<4>(64,0xffffffff);s->wr<4>(68,0x1f7a);s->memory.write<1>(stack-0x150+0x42,blocked);
        s->memory.write<4>(globals,0);s->memory.write<4>(globals+4,limit);s->memory.write<4>(globals+8,stream+126);
        for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,9);Options o;o.random=0;boundaries(*s,o,33);
    }
    for(bool mutate:{false,true}){Options o;o.mutate_outcome=mutate;o.mutate_execution=!mutate;boundaries(*result_outcome_seed(),o,33);}
    for(auto value:{0u,1u,24u,25u,32768u,65535u}){auto s=result_outcome_seed();s->memory.write<2>(actor+0x36,value);compare(*s,{},33);}
    for(auto pointer:{0u,actor,recipient+1,stack-0x150,globals}){auto s=result_outcome_seed();s->wr<4>(56,pointer);compare(*s,{},33);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*result_outcome_seed(),o,33);}
    for(auto contact:{0u,1u})for(auto flag:{0u,1u,2u,7u}) {
        auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.execution_contact=contact;o.resolved_contact=flag;compare(*s,o,34);
    }
    {auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x7931,0);Options o;o.contact_result=0;o.execution_contact=0;o.execution_rating=0;
        auto result=compare(*s,o,34);require(result.state->pc==action::return_phase&&std::any_of(result.calls.begin(),result.calls.end(),[](const Event& e){return e.continuation==0x102421cc;}),
            "Root prefix reaches the complete execution-result outcome");boundaries(*s,o,34);}
    for(unsigned kind=0;kind<256;++kind)for(auto state:{0u,4u,0x104u,0x180u})for(auto status:{0u,0x10u,0x880u,0x890u,0x8b0u,0xffffu}) {
        auto s=unopposed_seed();s->wr<4>(88,state);s->memory.write<1>(stack-0x150+0x48,kind);Options o;o.area_status=status;auto result=compare(*s,o,35);
        const bool skip=(state&255)==4,smaller=(status&0x30)&&(status&0x880);
        require(result.state->memory.read<2>(recipient+0x19c)==std::uint16_t(signed_value<1>(kind)),"Unopposed action stores the signed action byte in the receiver halfword");
        require(result.state->memory.read<1>(recipient+4)==(skip?255u:0u)&&result.state->memory.read<2>(recipient+0x36)==(skip?400u:smaller?341u:330u),
            "Unopposed receiver counter wraps and status gates select the original halfword decrement");
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,4u,6u,7u,128u})for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u})for(auto blocked:{0u,1u})for(auto disabled:{0u,1u,128u}) {
        auto s=unopposed_seed();s->wr<4>(88,4);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);
        s->memory.write<1>(stack-0x150+0x42,blocked);s->memory.write<1>(stack-0x150+0x45,disabled);auto result=compare(*s,{},35);
        const bool area=row>=2&&row<=6&&(side==0?(column>=10&&column<=11):column<=1);
        require(result.state->memory.read<1>(stack-0x150+0xd0)==(blocked&&(area||disabled)?1u:0x5au),"Unopposed blocked marker combines the inverted area and live disabled byte");
    }
    for(auto status:{0u,0x10u,0x880u,0x890u})for(auto second:{0u,0x10u,0x880u,0x890u}) {
        Options o;o.area_status=status;o.area_status_second=second;o.separate_area_status=true;auto result=compare(*unopposed_seed(),o,35);
        require(result.calls.size()==((status&0x30)?2u:1u)&&result.state->memory.read<2>(recipient+0x36)==((status&0x30)&&(second&0x880)?341u:330u),
            "Unopposed status queries are repeated and use independent live results");boundaries(*unopposed_seed(),o,35);
    }
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}) {
        auto s=unopposed_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);boundaries(*s,{},35);
    }
    for(auto pointer:{0u,actor,recipient+1,stack-0x150,globals}){auto s=unopposed_seed();s->wr<4>(56,pointer);compare(*s,{},35);}
    for(auto value:{0u,1u,58u,59u,69u,70u,32768u,65535u})for(auto status:{0u,0x890u}){auto s=unopposed_seed();s->memory.write<2>(recipient+0x36,value);Options o;o.area_status=status;compare(*s,o,35);}
    {Options o;o.mutate_unopposed=true;o.area_status=0x890;boundaries(*unopposed_seed(),o,35);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;o.area_status=0x890;compare(*unopposed_seed(),o,35);}
    for(auto contact:{0u,1u})for(auto flag:{0u,1u,2u,7u}) {
        auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.execution_contact=contact;o.resolved_contact=flag;o.result_contact=0;compare(*s,o,36);
    }
    {auto s=result_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x7931,0);Options o;o.contact_result=0;o.execution_contact=0;o.execution_rating=0;o.result_contact=0;
        auto result=compare(*s,o,36);require(result.state->pc==action::no_receiver_phase,"Root prefix reaches the unopposed continuation");boundaries(*s,o,36);}
    for(auto receiver:{0u,recipient})for(auto score:{0u,100u,101u,32767u,32768u,65535u})for(auto rating:{0u,100u,32767u,32768u,65535u}) {
        auto s=final_seed();s->wr<4>(56,receiver);s->wr<4>(68,0xabcd0000|score);s->memory.write<2>(stack-0x150+0x4a,rating);auto result=compare(*s,{},37);
        const bool successful=!receiver||signed_value<2>(score)>signed_value<2>(rating);require(result.state->pc==(successful?action::final_effects_phase:action::unsuccessful_action_phase),"Final action gate uses the signed score only when a receiver exists");
        if(successful){const auto delta=std::int32_t(signed_value<2>(score))/10;
            require(result.state->memory.read<4>(match+0x3108)==0x7ffffffeu+std::uint32_t(delta)&&result.state->memory.read<4>(match+0x310c)==0xfffffffeu+std::uint32_t(delta),"Final action adds both independently converted score fractions with word wrap");
            require(result.state->memory.read<2>(actor+0x36)==254&&result.state->memory.read<2>(actor+0x19c)==45,"Final preparation retains the action byte and adds four to the initiating halfword");
            if(receiver)require(result.state->memory.read<1>(receiver+0x2c)==4,"Final receiver byte wraps after the live stack increment");}
    }
    for(auto divisor:{0.0,-1.0,1.0,10.0,1.0e100}){auto s=final_seed();s->memory.write<8>(doubles+0x330,float_bits<8>(divisor));boundaries(*s,{},37);}
    for(auto bits:{0x7ff8000000001234ull,0x7ff0000000001234ull,0x7ff0000000000000ull,0xfff0000000000000ull}){auto s=final_seed();s->memory.write<8>(doubles+0x330,bits);compare(*s,{},37);}
    for(auto receiver:{0u,recipient}){auto s=final_seed();s->wr<4>(56,receiver);boundaries(*s,{},37);}
    {auto s=final_seed();s->wr<4>(68,0);boundaries(*s,{},37);}
    {Options o;o.mutate_final=true;boundaries(*final_seed(),o,37);}
    for(auto receiver:{actor,recipient+1,stack-0x150,globals}){auto s=final_seed();s->wr<4>(56,receiver);compare(*s,{},37);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*final_seed(),o,37);}
    for(auto phase:{38u,40u,42u,44u})for(auto state:{0u,2u,4u}) {
        auto s=final_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,state);Options o;o.contact_result=0;o.execution_receiver=0;compare(*s,o,phase);
    }
    for(unsigned state=0;state<256;++state){auto s=special_effects_seed();s->wr<4>(88,0xabcd0000|state);auto result=compare(*s,{},39);
        if(state!=4)require(result.state->pc==action::regular_effects_phase&&result.calls.empty(),"Only raw state four enters the special action effects");}
    for(auto first:{0u,1u,40u,127u,128u,255u})for(auto second:{0u,1u,40u,127u,128u,255u})for(auto kind:{0u,1u,0x10001u,0xffffu}) {
        auto s=special_effects_seed();s->memory.write<1>(actor_profile+0x2e,first);s->memory.write<1>(receiver_profile+0x2e,second);Options o;o.receiver_kinds.fill(kind);auto result=compare(*s,o,39);
        require(result.state->memory.read<2>(actor+0x19c)==123&&result.state->memory.read<1>(actor+7)==0&&result.state->memory.read<1>(actor+8)==0&&result.state->memory.read<1>(recipient+7)==0,"Special effects preserve the ordered action replacement and wrapped actor/receiver counters");
        require(result.state->memory.read<2>(recipient+0x36)==(signed_value<2>(kind)==1?65336u:390u),"Special receiver decrement uses the live signed classification");
        require(result.state->memory.read<2>(actor+0x36)==590&&result.state->memory.read<1>(actor+9)==0,"Eligible special action applies separate minus-ten, plus-fifty and plus-three-hundred writes");
    }
    for(auto first:{0u,actor_profile})for(auto second:{0u,receiver_profile})for(auto kind:{0u,1u})for(auto random:{16u,17u,18u}) {
        auto s=special_effects_seed();s->memory.write<4>(actor+0x70,first);s->memory.write<4>(recipient+0x70,second);s->memory.write<4>(globals,random);Options o;o.receiver_kinds.fill(kind);boundaries(*s,o,39);
    }
    for(auto side:{0u,1u,2u})for(auto row:{1u,3u,4u,5u,6u})for(auto column:{0u,1u,11u})for(auto gate:{0u,1u})for(auto alignment:{0u,1u}) {
        auto s=special_effects_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);Options o;o.final_gate=gate;o.low_alignment=alignment;compare(*s,o,39);
    }
    for(auto trait:{0u,1u,127u,128u,255u})for(auto response:{0u,1u,0x100u,0x1ffu}) {
        auto s=special_effects_seed();s->memory.write<1>(recipient+0x17,trait);s->memory.write<4>(globals,14);Options o;o.reaction_result=response;boundaries(*s,o,39);
    }
    for(auto wide:{31u,32u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*special_effects_seed(),o,39);}
    for(auto cursor:{stream,stream+122}){auto s=special_effects_seed();s->memory.write<4>(globals+8,cursor);Options o;o.zero_special_interval=true;boundaries(*s,o,39);}
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126})for(auto kind:{0u,1u}) {
        auto s=special_effects_seed();s->memory.write<4>(globals+8,cursor);Options o;o.receiver_kinds.fill(kind);boundaries(*s,o,39);
    }
    for(auto balance:{0u,199u,200u,201u,32767u,32768u,65335u,65336u,65535u})for(auto score:{0u,300u,1000u,32768u,65535u}) {
        auto s=special_effects_seed();s->wr<4>(56,0);s->wr<4>(68,0xabcd0000|score);s->memory.write<2>(actor+0x1a0,balance);auto result=compare(*s,{},39);
        const auto delta=signed_value<2>(std::uint32_t(score-100-200));const auto updated=signed_value<2>(std::uint32_t(signed_value<2>(balance)+delta));
        require(result.state->memory.read<2>(actor+0x1a0)==std::uint16_t(std::clamp<std::int64_t>(updated,-200,200)),"Special action narrows before applying both balance clamps");
    }
    for(auto balance:{0u,199u,65335u}){auto s=special_effects_seed();s->wr<4>(56,0);s->memory.write<2>(actor+0x1a0,balance);s->wr<4>(68,balance==199?1000u:0u);boundaries(*s,{},39);}
    {Options o;o.receiver_kinds={0,1,0};o.mutate_final=true;boundaries(*special_effects_seed(),o,39);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*special_effects_seed(),o,39);}
    for(unsigned flag=0;flag<256;++flag){auto s=regular_effects_seed();s->wr<4>(92,0xabcd0000|flag);auto result=compare(*s,{},41);
        if(flag)require(result.calls.size()==1&&read_be<4>(result.calls[0].before.data()+16)==0x216e&&result.state->memory.read<2>(actor+0x1a0)==0,"Recorded regular flag reports without applying the balance update");}
    for(auto owner:{actor,recipient})for(auto state:{0u,1u,2u,4u,127u,128u,255u})for(auto row:{0u,2u,3u,4u,5u,6u,128u})for(auto column:{0u,1u,11u}) {
        auto s=regular_effects_seed();s->wr<4>(96,owner);s->wr<4>(88,state);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);compare(*s,{},41);
    }
    for(auto score:{0u,500u,65035u,65036u,32768u,65535u})for(auto first:{0u,1u,0x100u,0x1ffu})for(auto second:{0u,1u,0x100u,0x1ffu}) {
        auto s=regular_effects_seed();s->wr<4>(64,score);Options o;o.regular_gate=first;o.regular_second_gate=second;compare(*s,o,41);
    }
    for(auto score:{200u,65035u})for(auto first:{0u,1u})for(auto second:{0u,1u})for(auto random:{17u,18u}) {
        auto s=regular_effects_seed();s->wr<4>(64,score);s->memory.write<4>(globals,random);s->memory.write<2>(stream+4,103);Options o;o.regular_gate=first;o.regular_second_gate=second;boundaries(*s,o,41);
    }
    for(auto owner:{actor,recipient})for(auto balance:{199u,65335u}){auto s=regular_effects_seed();s->wr<4>(96,owner);s->wr<4>(68,balance==199?1000u:0u);s->memory.write<2>(actor+0x1a0,balance);boundaries(*s,{},41);}
    for(auto cursor:{stream+124,stream+126}){auto s=regular_effects_seed();s->memory.write<4>(globals+8,cursor);s->memory.write<4>(globals,18);boundaries(*s,{},41);}
    for(auto side:{0u,1u})for(auto row:{2u,4u,6u})for(auto column:{0u,1u,11u}){auto s=regular_effects_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);boundaries(*s,{},41);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*regular_effects_seed(),o,41);}
    for(unsigned state=0;state<256;++state) {
        auto s=success_reaction_seed();s->wr<4>(88,0xabcd0000|state);s->memory.write<2>(actor+0x1a0,1);s->memory.write<2>(actor+0x32,0);
        s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,0);s->memory.write<2>(stream+4,0);auto result=compare(*s,{},43);
        require(std::any_of(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x102775e0;}),"Completed-action trigger is reached for the forced zero threshold");
        require(result.state->memory.read<1>(match+0x1c0)==unsigned(signed_value<1>(state)<4),"Completed-action state gate controls the match flag independently of its trigger call");
    }
    for(auto state:{0u,1u,4u,5u,128u,255u})for(auto balance:{0u,1u,199u,200u,65336u,65535u})for(auto trait:{0u,1u,20u,32767u,32768u,65516u}) {
        auto s=success_reaction_seed();s->wr<4>(88,state);s->memory.write<2>(actor+0x1a0,balance);s->memory.write<2>(actor+0x32,trait);compare(*s,{},43);
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,4u,6u,7u,128u})for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u}) {
        auto s=success_reaction_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);compare(*s,{},43);
    }
    for(auto wide:{33u,34u,35u,36u})for(auto sampled:{-100000,0,10000000}) {
        auto s=success_reaction_seed();if(wide<=34)s->wr<4>(88,1);Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*s,o,43);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126})for(auto state:{1u,4u})for(auto balance:{0u,200u}) {
        auto s=success_reaction_seed();s->wr<4>(88,state);s->memory.write<2>(actor+0x1a0,balance);s->memory.write<4>(globals+8,cursor);boundaries(*s,{},43);
    }
    for(auto side:{0u,1u})for(auto row:{1u,4u,7u})for(auto column:{0u,2u,10u,12u}){auto s=success_reaction_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,column);boundaries(*s,{},43);}
    for(auto trait:{0u,1u,32768u,65516u}){auto s=success_reaction_seed();s->memory.write<2>(actor+0x32,trait);boundaries(*s,{},43);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*success_reaction_seed(),o,43);}
    for(auto phase:{38u,40u,42u,44u}) {
        auto s=final_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+0x791f,4);Options o;o.contact_result=0;o.execution_receiver=0;const auto result=compare(*s,o,phase);
        require(std::any_of(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x1022e3a0;}),"Original-root prefix enters final preparation");
        if(phase==44)require(result.state->pc==action::return_phase&&std::any_of(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x102775e0;}),"Original-root prefix reaches completed special-action reaction");
        boundaries(*s,o,phase);
    }
    for(auto gate:{0u,1u,0x100u,0x1ffu})for(auto blocked:{0u,1u,128u,255u})for(auto trait:{0u,1u,20u,32768u,65516u,65535u}) {
        auto s=failed_seed();s->memory.write<1>(stack-0x150+0x42,blocked);s->memory.write<2>(recipient+0x32,trait);Options o;o.contact_end_gate=gate;compare(*s,o,45);
    }
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{0u,1u,2u,4u,6u,7u,128u})for(auto column:{0u,1u,2u,9u,10u,11u,12u,128u}) {
        auto s=failed_seed();s->memory.write<1>(recipient+0x27,side);s->memory.write<1>(recipient+0x105,row);s->memory.write<1>(recipient+0x106,column);Options o;o.contact_end_gate=1;
        auto result=compare(*s,o,45);const bool area=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);
        require(result.state->pc==(area?action::failed_message_phase:action::failed_fallback_phase),"Failed area report uses the receiver's raw side and coordinates before its even random gate");
    }
    for(auto gate:{0u,1u})for(auto blocked:{0u,1u})for(auto trait:{0u,32768u,65516u})for(auto random:{16u,17u,18u}) {
        auto s=failed_seed();s->memory.write<1>(stack-0x150+0x42,blocked);s->memory.write<2>(recipient+0x32,trait);s->memory.write<4>(globals,random);Options o;o.contact_end_gate=gate;boundaries(*s,o,45);
    }
    for(auto wide:{37u,38u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*failed_seed(),o,45);}
    for(auto cursor:{stream+124,stream+126})for(auto gate:{0u,1u}){auto s=failed_seed();s->memory.write<4>(globals+8,cursor);Options o;o.contact_end_gate=gate;boundaries(*s,o,45);}
    {auto s=failed_seed();s->wr<4>(56,0);auto result=compare(*s,{},45);require(result.calls.empty()&&result.state->pc==action::missing_receiver_phase,"Null failed receiver transfers to the original diagnostic tail");boundaries(*s,{},45);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*failed_seed(),o,45);}
    const std::array<std::pair<std::uint32_t,unsigned>,12> failed_messages{{
        {0x10243690,0x1fea},{0x102436bc,0x1feb},{0x102436e8,0x1fec},{0x10243714,0x1fed},
        {0x10243740,0x1fee},{0x1024376c,0x1ff2},{0x10243798,0x1fef},{0x102437c4,0x1ff1},
        {0x102437f0,0x1ff0},{0x1024381c,0x1ff3},{0x10243848,0x1ff4},{0x10243874,0x1fe9}}};
    for(unsigned kind=0;kind<256;++kind) {
        auto s=failed_message_seed();s->memory.write<1>(stack-0x150+0x48,kind);auto result=compare(*s,{},47);const auto target=kind<=46?original_failed_action_table[kind]:0x10243874;
        const auto expected=std::find_if(failed_messages.begin(),failed_messages.end(),[&](const auto& v){return v.first==target;})->second;
        require(result.calls.size()==1&&read_be<4>(result.calls[0].before.data()+16)==expected,"Failed action table selects its original receiver-first message");
    }
    for(unsigned index=0;index<47;++index)for(auto [target,message]:failed_messages){auto s=failed_message_seed();s->memory.write<1>(stack-0x150+0x48,index);s->memory.write<4>(failed_table+index*4,target);auto result=compare(*s,{},47);require(read_be<4>(result.calls[0].before.data()+16)==message,"Mutable failed action table overrides canonical mapping");}
    for(auto [target,message]:failed_messages){auto s=failed_message_seed();s->memory.write<4>(failed_table+45*4,target);boundaries(*s,{},47);s->pc=target;auto result=compare(*s,{},47);require(read_be<4>(result.calls[0].before.data()+16)==message,"Direct failed message skips its table read");boundaries(*s,{},47);}
    for(auto target:{0u,action::failed_outcome_phase,action::return_phase,action::missing_receiver_phase,0x12345679u}){auto s=failed_message_seed();s->memory.write<4>(failed_table+45*4,target);auto result=compare(*s,{},47);require(result.calls.empty()&&result.state->pc==target,"Unknown failed message target remains a continuation");boundaries(*s,{},47);}
    for(auto pointer:{0u,failed_table+1,failed_table+0x400,toc-0x520c,stack-0x150}){auto s=failed_message_seed();s->memory.write<4>(toc-0x520c,pointer);compare(*s,{},47);}
    for(auto kind:{47u,128u,255u}){auto s=failed_message_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<4>(toc-0x520c,0);boundaries(*s,{},47);}
    {Options o;o.mutate_failed_slot=-0x520c;boundaries(*failed_message_seed(),o,47);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*failed_message_seed(),o,47);}
    const std::array<std::pair<std::uint32_t,unsigned>,3> failed_comments{{{0x1024391c,0x20eb},{0x10243958,0x20ea},{0x10243994,0x20e8}}};
    for(unsigned state=0;state<256;++state)for(unsigned draw=0;draw<50;++draw) {
        auto s=failed_comment_seed();s->wr<4>(88,0xabcd0000|state);s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,draw);auto result=compare(*s,{},49);
        if(signed_value<1>(state)>=4)require(result.calls.empty()&&result.state->pc==action::failed_outcome_phase,"Signed state four or higher skips the failed comment and random consumption");
        else {const auto target=draw<=9?original_failed_comment_table[draw]:0x10243994;const auto expected=std::find_if(failed_comments.begin(),failed_comments.end(),[&](const auto& v){return v.first==target;})->second;require(result.calls.size()==1&&read_be<4>(result.calls[0].before.data()+16)==expected,"Random fiftieth selects the ten-word failed comment table or default");}
    }
    for(unsigned index=0;index<10;++index)for(auto [target,message]:failed_comments){auto s=failed_comment_seed();s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,index);s->memory.write<4>(failed_comment_table+index*4,target);auto result=compare(*s,{},49);require(read_be<4>(result.calls[0].before.data()+16)==message,"Mutable failed-comment table selects each message at every live index");boundaries(*s,{},49);}
    for(auto [target,message]:failed_comments){auto s=failed_comment_seed();s->pc=target;auto result=compare(*s,{},49);require(read_be<4>(result.calls[0].before.data()+16)==message,"Direct failed comment skips its random and table reads");boundaries(*s,{},49);}
    for(auto side:{0u,1u,2u,128u,255u}){auto s=failed_comment_seed();s->memory.write<1>(recipient+0x27,side);auto result=compare(*s,{},49);require(read_be<4>(result.calls[0].before.data()+28)==unsigned(side==0),"Failed comment uses the receiver's inverted side");}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=failed_comment_seed();s->memory.write<4>(globals,random);boundaries(*s,{},49);}
    {auto s=failed_comment_seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},49);}
    for(auto pointer:{0u,failed_comment_table+1,failed_comment_table+0x400,toc-0x5210,stack-0x150}){auto s=failed_comment_seed();s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,0);s->memory.write<4>(toc-0x5210,pointer);compare(*s,{},49);}
    {auto s=failed_comment_seed();s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,0);Options o;o.mutate_failed_slot=-0x5210;boundaries(*s,o,49);}
    for(auto target:{0u,action::failed_outcome_phase,action::failed_fallback_phase,action::return_phase,0x12345679u}){auto s=failed_comment_seed();s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,0);s->memory.write<4>(failed_comment_table,target);auto result=compare(*s,{},49);require(result.state->pc==target&&result.calls.empty(),"Unknown failed comment target remains a continuation");boundaries(*s,{},49);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*failed_comment_seed(),o,49);}
    const std::array<std::uint32_t,9> fallback_targets{{0x102439f4,0x10243a14,0x10243a40,0x10243a6c,0x10243a98,0x10243ac4,0x10243b50,0x10243b7c,action::failed_outcome_phase}};
    for(unsigned kind=0;kind<256;++kind){auto s=failed_fallback_seed();s->memory.write<1>(stack-0x150+0x48,kind);compare(*s,{},51);}
    for(unsigned index=0;index<47;++index)for(auto target:fallback_targets){auto s=failed_fallback_seed();s->memory.write<1>(stack-0x150+0x48,index);s->memory.write<4>(failed_fallback_table+index*4,target);compare(*s,{},51);}
    for(auto target:fallback_targets){auto s=failed_fallback_seed();s->pc=target;boundaries(*s,{},51);if(target!=action::failed_outcome_phase){s->pc=action::failed_fallback_phase;s->memory.write<4>(failed_fallback_table+45*4,target);boundaries(*s,{},51);}}
    for(auto result_value:{0u,1u,0x100u,0x1ffu})for(auto score:{0u,32767u,32768u,65535u})for(auto rating:{0u,32767u,32768u,65535u}) {
        auto s=failed_fallback_seed();s->pc=0x102439f4;s->wr<4>(68,0xabcd0000|score);s->memory.write<2>(stack-0x150+0x4a,rating);Options o;o.failed_result=result_value;auto result=compare(*s,o,51);
        require(result.state->pc==((result_value&255)?action::failed_outcome_phase:action::return_phase),"Failed fallback respects the low-byte receiver result and its early return");
        require(read_be<4>(result.calls[0].before.data()+16)==std::uint32_t(signed_value<2>(rating-score)),"Failed receiver dependency receives the signed narrowed rating-minus-score difference");
    }
    for(auto field:{0u,1u,2u})for(auto bits:{0u,0x80000000u,0x3f800000u,0x40000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u}) {
        auto s=failed_fallback_seed();s->pc=0x10243ac4;s->memory.write<4>(field==0?singles+0xe4:field==1?recipient+0xb0:actor+0xb0,bits);boundaries(*s,{},51);
    }
    for(auto random:{0u,1u,17u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=failed_fallback_seed();s->pc=0x10243ac4;s->memory.write<4>(globals,random);compare(*s,{},51);}
    {auto s=failed_fallback_seed();s->pc=0x10243ac4;s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},51);}
    for(auto pointer:{0u,failed_fallback_table+1,failed_fallback_table+0x400,toc-0x5214,stack-0x150}){auto s=failed_fallback_seed();s->memory.write<4>(toc-0x5214,pointer);compare(*s,{},51);}
    {Options o;o.mutate_failed_slot=-0x5214;boundaries(*failed_fallback_seed(),o,51);}
    for(auto kind:{47u,128u,255u}){auto s=failed_fallback_seed();s->memory.write<1>(stack-0x150+0x48,kind);s->memory.write<4>(toc-0x5214,0);boundaries(*s,{},51);}
    for(auto target:{0u,action::missing_receiver_phase,action::return_phase,0x12345679u}){auto s=failed_fallback_seed();s->memory.write<4>(failed_fallback_table+45*4,target);auto result=compare(*s,{},51);require(result.state->pc==target&&result.calls.empty(),"Unknown failed fallback target remains a continuation");boundaries(*s,{},51);}
    for(bool child:{false,true}){auto s=failed_fallback_seed();s->pc=0x102439f4;Options o;o.own_negative=!child;o.child_negative=child;compare(*s,o,51);}
    for(auto phase:{46u,48u,50u,52u})for(auto gate:{0u,1u}) {
        auto s=failed_seed();s->pc=action::entry;s->wr<4>(4,stack);Options o;o.contact_result=0;o.execution_contact=0;o.result_contact=0;o.execution_rating=32767;o.contact_end_gate=gate;auto result=compare(*s,o,phase);
        require(std::any_of(result.calls.begin(),result.calls.end(),[](const Event& e){return e.continuation==0x102433e8;}),"Original-root prefix enters failed-action receiver handling");boundaries(*s,o,phase);
    }
    check_receiver_tail();
    for(auto pc=action::entry;pc<action::end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    require(sites.size()==8142,"All original ball-action instructions covered");
    for(unsigned p=0;p<phase_cases.size();++p){require(phase_completed[p]>0,"Each ball-action phase completed");
        std::cout<<"Phase "<<p<<": "<<phase_cases[p]<<" comparisons, "<<phase_completed[p]<<" completions\n";}
    std::cout<<"PASS: "<<cases<<" ball-action component comparisons; "<<completed<<" phase completions, "<<interruptions
        <<" interruptions, "<<negatives<<" own/child negatives; "<<structural<<" dormant-branch structural cases; all "<<sites.size()
        <<" original sites; complete state/effects/ABI/accounting; component oracle complete\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
