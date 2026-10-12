#include "player_movement.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <fstream>
#include "contact-action-table.hpp"
#include "repeated-contact-tables.hpp"

namespace cm::aot { bool movement_noncarrier_reference(State&);bool movement_preparation_reference(State&);bool movement_carrier_reference(State&);bool movement_environment_reference(State&);bool movement_interaction_reference(State&);bool movement_contact_reference(State&);bool movement_contact_outcome_reference(State&);bool movement_contact_action_reference(State&);bool movement_contact_followup_reference(State&);bool movement_restart_reference(State&);bool movement_possession_reference(State&);bool movement_play_preparation_reference(State&);bool movement_play_bias_reference(State&);bool movement_kind17_contact_reference(State&);bool movement_kind17_clear_reference(State&);bool movement_kind18_reference(State&);bool movement_repeated_contact_reference(State&);bool movement_transfer_tail_reference(State&);bool player_movement_reference(State&);void movement_noncarrier_step_seen(State&,std::uint32_t); }
namespace {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t owner=fixture+0x1000, subject=owner+0x311c, neighbour=subject+0x1c4;
constexpr std::uint32_t doubles=fixture+0x20000,singles=fixture+0x21000,distances=fixture+0x22000;
constexpr std::uint32_t stream=fixture+0x26000,globals=fixture+0x27000,output=fixture+0x40000;
constexpr std::uint32_t phase_stack=stack-0x180;
struct Trace {std::vector<std::uint32_t> order;std::set<std::uint32_t> sites;};
thread_local Trace* active=nullptr;
struct Probe final:SemanticGuestObserver {void before_step(State& s,std::uint32_t pc)override{movement_noncarrier_step_seen(s,pc);}};
struct Options {std::uint32_t role=0,secondary_role=0,suitability=0,low_flags=0,high_flags=0;bool neighbour_gate=true,override_facing=false,facing_allowed=true,force_wide_control=false,force_negative_limit=false,force_wide_condition=false,force_negative_condition_limit=false;unsigned fault_call=0;std::int32_t sampled=9;std::uint32_t other_actor=neighbour;bool zone_blocked=false;double measurement=0;bool force_wide_contact=false,force_negative_contact_limit=false;unsigned contact_result=1,contact_flag=0,contact_forbidden=0,contact_status=0;bool reaction_allowed=true,force_wide_reaction=false,force_negative_reaction_limit=false;bool report_excluded=false,rating_null_owner=false,force_wide_interest=false,force_negative_interest_limit=false;std::uint32_t restart_receiver=neighbour;bool restart_qualified=true,restart_repeat_qualified=true,restart_position=true;bool force_wide_release=false,force_negative_release_limit=false;bool force_wide_play_condition=false,force_negative_play_condition_limit=false;bool force_wide_success=false,force_negative_success_limit=false;bool force_negative_cell_bias=false;bool next_zone=false,current_zone=false,clear_trait=false,clear_secondary_trait=false,y_gate=false,null_attachment_prefix=false;unsigned gates=0;bool repeat_allowed=true,force_wide_repeat=false,force_negative_repeat_limit=false;bool transfer_trait=false,ratio_blocked=false,outcome_active=false,transfer_pending=false;bool force_wide_transfer=false,force_negative_transfer_limit=false,zero_transfer_bound=false,force_wide_distance=false,force_negative_distance_limit=false,zero_distance_bound=false,force_wide_reward=false,force_negative_reward_limit=false,zero_reward_bound=false;bool force_action_direction=false;};
thread_local const Options* edits=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    Options options;std::vector<Event> events;unsigned qualification_calls=0,zone_calls=0,gate_calls=0;
    explicit Model(Options o={}):options(o){}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        std::array<std::uint32_t,8> args{};for(unsigned n=0;n<8;++n)args[n]=std::uint32_t(s.r<4>((n+3)*4));
        events.push_back({target,continuation,s.registers,{}});const auto index=events.size()-1;std::uint32_t result=0;
        switch(target){
        case 0x10272050:result=options.transfer_trait;break;
        case 0x10237b10:result=options.ratio_blocked;break;
        case 0x10270ba0:result=options.outcome_active;break;
        case 0x101f1100:s.memory.write<1>(owner+0x7928,options.transfer_pending);break;
        case 0x10288230:s.memory.write<1>(subject+0x12,0x82);break;
        case 0x10270320:result=options.repeat_allowed;break;
        case 0x1077b720:s.memory.write<4>(args[0],args[1]);break;
        case 0x10739890:s.memory.write<4>(args[3],0x12345678);s.memory.write<4>(args[4],0x9abcdef0);break;
        case 0x10768810:result=output+0x380;break;
        case 0x10769b60:s.memory.write<4>(output+0x3a0,args[0]);break;
        case 0x10239dc0:result=zone_calls++?options.current_zone:options.next_zone;break;
        case 0x101ee9c0:result=(options.gates>>std::min(gate_calls++,3u))&1;break;
        case 0x10272100:result=options.clear_trait;break;
        case 0x102720d0:result=options.clear_secondary_trait;break;
        case 0x101ee9f0:result=options.y_gate;break;
        case 0x101e3f10:s.wr<8>(0x4010,float_bits<8>(options.measurement));break;
        case 0x102865a0:s.memory.write<4>(output+0x600,std::uint32_t(s.r<8>(0x4010)));break;
        case 0x101f3a30:result=options.restart_receiver;break;
        case 0x10272150:result=qualification_calls++?options.restart_repeat_qualified:options.restart_qualified;break;
        case 0x102720a0:result=options.restart_position;break;
        case 0x101d6780:s.memory.write<4>(output+0x500,std::uint32_t(s.r<8>(0x4010)));break;
        case 0x102884d0:s.memory.write<1>(args[0]+0x11,0);break;
        case 0x10303560:s.wr<8>(0x4010,float_bits<8>(options.measurement));if(options.rating_null_owner)s.memory.write<4>(subject+0x1a4,0);break;
        case 0x1014c2e0:result=options.report_excluded;break;
        case 0x10738d10:for(unsigned n=0;n<args[2];++n)s.memory.write<1>(output+0x400+n,s.memory.read<1>(args[1]+n));break;
        case 0x101f1530:s.memory.write<1>(neighbour+0x12,0x75);break;
        case 0x10288f30:result=options.reaction_allowed;break;
        case 0x1022ba10:s.memory.write<4>(output+0x350,args[0]);break;
        case 0x101e3d50:result=options.contact_status;break;
        case 0x101f5050:s.memory.write<1>(args[5],2);s.memory.write<1>(args[6],3);s.memory.write<1>(args[7],options.contact_flag);result=options.contact_result;break;
        case 0x101ee530:result=options.contact_forbidden;break;
        case 0x101d68f0:s.memory.write<4>(output+0x300,args[1]);break;
        case 0x101f5110:s.memory.write<1>(args[3],0xfe);s.memory.write<2>(args[7],0x7654);break;
        case 0x101eaf60:
            s.memory.write<1>(args[4],0xfd);s.memory.write<1>(args[5],0xfe);s.memory.write<2>(args[6],0x1234);
            s.memory.write<1>(args[7],0xff);s.memory.write<1>(s.memory.read<4>(s.r<4>(4)+0x38),0x7f);
            result=options.other_actor;break;
        case 0x10245160:s.wr<8>(0x4010,float_bits<8>(options.measurement));break;
        case 0x10239d10:result=options.zone_blocked;break;
        case 0x10226520:result=std::uint32_t(options.sampled);break;
        case 0x101f5350:s.memory.write<4>(output+0x200,args[2]);break;
        case 0x10245050:result=options.facing_allowed;break;
        case 0x102775c0:result=s.memory.read<4>(owner+0xe000)==args[0];break;
        case 0x101d8120:result=options.neighbour_gate;break;
        case 0x102836a0:s.memory.write<1>(args[0]+0x11,1);break;
        case 0x1077b770:result=0xabcd5678;break;
        case 0x10775540:result=std::bit_cast<std::int32_t>(args[0])<0?0u-args[0]:args[0];break;
        case 0x10289560:result=options.role;break;
        case 0x102895e0:result=options.secondary_role;break;
        case 0x1028ac50:result=options.suitability;break;
        case 0x10113e10:result=options.high_flags;break;
        case 0x10239590:s.memory.write<4>(subject+0x50,float_bits<4>(float_value<8>(s.r<8>(0x4010))));break;
        case 0x101f0c00:s.memory.write<4>(output+0x100,0x101f0c00);break;
        case 0x10283690:result=s.memory.read<1>(args[0]+0x27);break;
        case 0x10201180:s.memory.write<4>(output+index*4,args[1]);break;
        case 0x1022e3a0:s.memory.write<4>(owner+0xe000,args[0]);break;
        case 0x10237830:s.memory.write<2>(args[0]+0x19e,((args[3]&255)<<8)|(args[4]&255));break;
        case 0x102721a0:result=options.override_facing;break;
        default:throw std::runtime_error("Unknown noncarrier fixture dependency "+hex_address(target));
        }
        for(unsigned reg=0;reg<=12;++reg)if(reg!=1 && reg!=2)s.wr<4>(reg*4,0xbad00000+reg*73);
        s.wr<4>(12,result);if(target==0x10113e10)s.wr<4>(16,options.low_flags);
        s.pc=continuation;events[index].after=s.registers;
        if(options.fault_call && events.size()==options.fault_call)throw std::runtime_error("Fixture movement dependency interruption");
    }
    void whole_reference(State& s){while(s.pc!=return_sentinel){if(player_movement_reference(s))continue;require(s.pc<cm::aot::player_movement::entry || s.pc>=cm::aot::player_movement::end,"Missing original whole movement label");call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void reference(State& s,bool preparing,bool carrier,bool environment,bool interaction,bool contact,bool outcome,bool action,bool followup,bool restart,bool possession,bool play_preparation,bool play_bias,bool kind17_contact,bool kind17_clear,bool kind18,bool simple_play,bool repeated_contact,unsigned tail_mode){while(s.pc!=return_sentinel){
        if(preparing && (s.pc==cm::aot::player_movement::carrier_phase || s.pc==cm::aot::player_movement::noncarrier_phase))break;
        if(carrier && s.pc==cm::aot::player_movement::carrier_decision_phase)break;
        if(environment && s.pc==cm::aot::player_movement::carrier_interaction_phase)break;
        if(interaction && s.pc==cm::aot::player_movement::carrier_contact_phase)break;
        if(tail_mode==1 && (s.pc==cm::aot::player_movement::carrier_distance_phase || s.pc==cm::aot::player_movement::carrier_statistics_phase || s.pc==0x102346e4))break;
        if((tail_mode==2 || tail_mode==3) && s.pc==cm::aot::player_movement::carrier_ratings_phase)break;
        if(tail_mode==4 && s.pc==cm::aot::player_movement::common_return_phase)break;
        if(tail_mode==5 && s.pc==cm::aot::player_movement::noncarrier_phase)break;
        if(simple_play && s.pc==cm::aot::player_movement::noncarrier_phase)break;
        if(repeated_contact && (s.pc==cm::aot::player_movement::carrier_transfer_phase || s.pc==cm::aot::player_movement::carrier_null_contact_phase || s.pc==cm::aot::player_movement::common_return_phase))break;
        if(kind18 && s.pc==cm::aot::player_movement::noncarrier_phase)break;
        if(kind17_clear && s.pc==cm::aot::player_movement::noncarrier_phase)break;
        if(kind17_contact && (s.pc==cm::aot::player_movement::carrier_kind17_clear_phase || s.pc==cm::aot::player_movement::noncarrier_phase))break;
        if(play_bias && (s.pc==cm::aot::player_movement::carrier_kind17_phase || s.pc==cm::aot::player_movement::carrier_kind18_phase || s.pc==cm::aot::player_movement::carrier_kind15_phase || s.pc==cm::aot::player_movement::carrier_other_kind_phase))break;
        if(play_preparation && (s.pc==cm::aot::player_movement::carrier_play_ready_phase || s.pc==cm::aot::player_movement::carrier_repeated_contact_phase))break;
        if(possession && (s.pc==cm::aot::player_movement::carrier_play_decision_phase || s.pc==cm::aot::player_movement::noncarrier_phase))break;
        if(restart && s.pc==cm::aot::player_movement::common_return_phase)break;
        if(followup && s.pc==cm::aot::player_movement::carrier_restart_selection_phase)break;
        if(action && (s.pc==cm::aot::player_movement::carrier_contact_followup_phase || s.pc==return_sentinel))break;
        if(outcome && s.pc==cm::aot::player_movement::carrier_no_contact_phase)break;
        if(contact && (s.pc==cm::aot::player_movement::carrier_contact_decision_phase || s.pc==cm::aot::player_movement::carrier_no_contact_phase || s.pc==cm::aot::player_movement::carrier_failed_contact_phase || s.pc==cm::aot::player_movement::carrier_resolved_contact_phase))break;
        if(!(preparing?movement_preparation_reference(s):carrier?movement_carrier_reference(s):environment?movement_environment_reference(s):interaction?movement_interaction_reference(s):contact?movement_contact_reference(s):outcome?movement_contact_outcome_reference(s):action?movement_contact_action_reference(s):followup?movement_contact_followup_reference(s):restart?movement_restart_reference(s):possession?movement_possession_reference(s):play_preparation?movement_play_preparation_reference(s):play_bias?movement_play_bias_reference(s):kind17_contact?movement_kind17_contact_reference(s):kind17_clear?movement_kind17_clear_reference(s):kind18?movement_kind18_reference(s):(simple_play || repeated_contact)?movement_repeated_contact_reference(s):tail_mode==6?movement_noncarrier_reference(s):tail_mode?movement_transfer_tail_reference(s):movement_noncarrier_reference(s)))call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
};
std::uint32_t cell(unsigned x,unsigned y){return owner+0xa08+x*0x450+y*0x5c;}
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x60000);s->memory.map(stack-0x900,0xb00);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,phase_stack);s->wr<4>(8,toc);s->wr<4>(60,subject);s->wr<4>(64,5);s->wr<4>(68,6);
    s->wr<4>(104,doubles);s->wr<4>(108,globals);s->wr<4>(112,stream);s->wr<4>(116,globals+4);s->wr<4>(120,globals+8);s->wr<4>(124,singles);
    s->wr<4>(4128,return_sentinel);s->pc=cm::aot::player_movement::noncarrier_phase;
    s->memory.write<4>(phase_stack,stack);s->memory.write<4>(phase_stack+0x188,return_sentinel);
    s->memory.write<8>(stack-8,0x0123456789abcdef);s->memory.write<8>(stack-16,0xfedcba9876543210);
    for(unsigned reg=13;reg<32;++reg)s->memory.write<4>(stack-0x5c+(reg-13)*4,0xcafe0000+reg);
    s->memory.write<1>(phase_stack+0x116,4);s->memory.write<1>(phase_stack+0x115,6);
    s->memory.write<4>(toc-0x6424,distances);s->memory.write<4>(globals,17);s->memory.write<4>(globals+4,stream+126);s->memory.write<4>(globals+8,stream);
    for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,100+n);
    for(unsigned n=0;n<0x400;n+=8)s->memory.write<8>(doubles+n,float_bits<8>(2));
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,9>{{{0x28,0.5},{0x38,100},{0x48,4},{0x80,4503601774854144.0},{0xc8,1},{0xd0,2},{0x168,2},{0x190,2},{0x2a0,100}}})
        s->memory.write<8>(doubles+offset,float_bits<8>(value));
    for(unsigned n=0;n<0x200;n+=4)s->memory.write<4>(singles+n,float_bits<4>(1.0+n/1024.0));
    for(unsigned n=0;n<0x1000;n+=4)s->memory.write<4>(distances+n,float_bits<4>(1.25));
    s->memory.write<1>(subject+0x105,4);s->memory.write<1>(subject+0x106,6);s->memory.write<1>(subject+0x19,2);s->memory.write<1>(subject+0x28,0);
    s->memory.write<4>(subject+0x1a4,owner);s->memory.write<1>(subject+0x107,10);s->memory.write<1>(subject+0x10a,10);s->memory.write<4>(subject+0xd8,float_bits<4>(75));
    s->memory.write<4>(subject+0x1b4,neighbour);s->memory.write<1>(neighbour+0x27,1);
    s->memory.write<4>(cell(4,6),subject);s->memory.write<4>(cell(4,6)+4,neighbour);s->memory.write<1>(cell(4,6)+0x58,2);s->memory.write<1>(neighbour+0x28,1);
    s->memory.write<1>(owner+1,1);s->memory.write<1>(owner+0x30d8,3);s->memory.write<1>(owner+0x791d,4);s->memory.write<1>(owner+0x791e,8);
    s->memory.write<1>(owner+0x7924,255);s->memory.write<4>(owner+0x79f4,output+0x1000);
    return s;
}
Probe probe;unsigned cases=0,normal=0,interrupted=0,negative_cases=0,whole_cases=0,whole_normal=0,whole_faults=0;std::set<std::uint32_t> whole_sites;std::set<std::uint32_t> sites;
std::vector<std::uint32_t> last_path;
std::unique_ptr<State> preparation_seed(unsigned x,unsigned y,unsigned kind,bool owns=false) {
    auto s=seed();s->pc=cm::aot::player_movement::entry;s->wr<4>(4,stack);s->wr<4>(12,subject);s->wr<4>(16,x);s->wr<4>(20,y);
    s->wr<4>(24,kind);s->wr<4>(28,0);s->wr<4>(32,0);s->memory.write<4>(owner+0xe000,owns?subject:0);
    for(auto [offset,value]:std::array<std::pair<int,unsigned>,7>{{{-0x51e0,singles},{-0x51e8,doubles},{-0x51e4,doubles},
        {-0x7b90,globals},{-0x7b8c,stream},{-0x7b88,globals+4},{-0x7b84,globals+8}}})s->memory.write<4>(toc+offset,value);
    return s;
}
void compare(const State& input,Options options={},bool negative=false,bool whole=false) {
    auto x=clone(input),y=clone(input);Model old(options),modern(options);Effects a,b;a.state=x.get();b.state=y.get();x->memory.date_observer=&a;y->memory.date_observer=&b;
    Trace old_trace,new_trace;
    const bool preparing=input.pc==cm::aot::player_movement::entry,carrier=input.pc==cm::aot::player_movement::carrier_phase,environment=input.pc==cm::aot::player_movement::carrier_decision_phase,interaction=input.pc==cm::aot::player_movement::carrier_interaction_phase,contact=input.pc==cm::aot::player_movement::carrier_contact_phase,outcome=input.pc==cm::aot::player_movement::carrier_failed_contact_phase || input.pc==cm::aot::player_movement::carrier_resolved_contact_phase,action=input.pc==cm::aot::player_movement::carrier_contact_decision_phase,followup=input.pc==cm::aot::player_movement::carrier_contact_followup_phase,restart=input.pc==cm::aot::player_movement::carrier_restart_selection_phase,possession=input.pc==cm::aot::player_movement::carrier_no_contact_phase,play_preparation=input.pc==cm::aot::player_movement::carrier_play_decision_phase,play_bias=input.pc==cm::aot::player_movement::carrier_play_ready_phase,kind17_contact=input.pc==cm::aot::player_movement::carrier_kind17_phase,kind17_clear=input.pc==cm::aot::player_movement::carrier_kind17_clear_phase,kind18=input.pc==cm::aot::player_movement::carrier_kind18_phase,simple_play=input.pc==cm::aot::player_movement::carrier_kind15_phase || input.pc==cm::aot::player_movement::carrier_other_kind_phase,repeated_contact=input.pc==cm::aot::player_movement::carrier_repeated_contact_phase || input.pc==0x10233900;
    const unsigned tail_mode=input.pc==cm::aot::player_movement::carrier_transfer_phase?1:input.pc==cm::aot::player_movement::carrier_distance_phase || input.pc==0x102346e4?2:input.pc==cm::aot::player_movement::carrier_statistics_phase?3:input.pc==cm::aot::player_movement::carrier_ratings_phase?4:input.pc==cm::aot::player_movement::carrier_null_contact_phase?5:input.pc==cm::aot::player_movement::common_return_phase?6:0;
    const auto execute=[preparing,carrier,environment,interaction,contact,outcome,action,followup,restart,possession,play_preparation,play_bias,kind17_contact,kind17_clear,kind18,simple_play,repeated_contact,tail_mode,whole](State& s,Model& model,bool reference,Trace& trace){active=&trace;edits=&model.options;std::string error;try{if(whole && reference)model.whole_reference(s);else if(whole)cm::aot::player_movement::move(s,&model,&probe);else if(reference)model.reference(s,preparing,carrier,environment,interaction,contact,outcome,action,followup,restart,possession,play_preparation,play_bias,kind17_contact,kind17_clear,kind18,simple_play,repeated_contact,tail_mode);else if(preparing)(void)cm::aot::player_movement::prepare_movement(s,&model,&probe);else if(carrier)cm::aot::player_movement::continue_carrier_sampling(s,&model,&probe);else if(environment)cm::aot::player_movement::continue_carrier_environment(s,&model,&probe);else if(interaction)cm::aot::player_movement::continue_carrier_interaction(s,&model,&probe);else if(contact)cm::aot::player_movement::continue_carrier_contact(s,&model,&probe);else if(outcome)cm::aot::player_movement::continue_carrier_contact_outcome(s,&model,&probe);else if(action)cm::aot::player_movement::continue_carrier_contact_action(s,&model,&probe);else if(followup)cm::aot::player_movement::continue_carrier_contact_followup(s,&model,&probe);else if(restart)cm::aot::player_movement::continue_carrier_restart(s,&model,&probe);else if(possession)cm::aot::player_movement::continue_carrier_possession(s,&model,&probe);else if(play_preparation)cm::aot::player_movement::continue_carrier_play_preparation(s,&model,&probe);else if(play_bias)cm::aot::player_movement::continue_carrier_play_bias(s,&model,&probe);else if(kind17_contact)cm::aot::player_movement::continue_carrier_kind17_contact(s,&model,&probe);else if(kind17_clear)cm::aot::player_movement::continue_carrier_kind17_clear(s,&model,&probe);else if(kind18)cm::aot::player_movement::continue_carrier_kind18(s,&model,&probe);else if(simple_play)cm::aot::player_movement::continue_carrier_simple_play(s,&model,&probe);else if(repeated_contact)cm::aot::player_movement::continue_carrier_repeated_contact(s,&model,&probe);else if(tail_mode==1)cm::aot::player_movement::continue_carrier_transfer(s,&model,&probe);else if(tail_mode==2)cm::aot::player_movement::continue_carrier_distance(s,&model,&probe);else if(tail_mode==3)cm::aot::player_movement::continue_carrier_statistics(s,&model,&probe);else if(tail_mode==4)cm::aot::player_movement::continue_carrier_ratings(s,&model,&probe);else if(tail_mode==5)cm::aot::player_movement::continue_carrier_null_contact(s,&model,&probe);else if(tail_mode==6)cm::aot::player_movement::return_movement_frame(s,&model,&probe);else cm::aot::player_movement::continue_noncarrier(s,&model,&probe);}catch(const std::exception& e){error=e.what();}active=nullptr;edits=nullptr;return error;};
    const auto expected=execute(*x,old,true,old_trace),actual=execute(*y,modern,false,new_trace);
    x->memory.date_observer=y->memory.date_observer=nullptr;const auto label="movement fragment case "+std::to_string(cases);
    require(expected==actual,label+" outcomes original="+expected+" modern="+actual);
    if(old_trace.order!=new_trace.order){unsigned n=0;while(n<std::min(old_trace.order.size(),new_trace.order.size()) && old_trace.order[n]==new_trace.order[n])++n;
        std::cerr<<label<<" first path difference="<<n<<" counts="<<old_trace.order.size()<<'/'<<new_trace.order.size()<<'\n';
        if(n<old_trace.order.size())std::cerr<<"original="<<hex_address(old_trace.order[n])<<'\n';if(n<new_trace.order.size())std::cerr<<"modern="<<hex_address(new_trace.order[n])<<'\n';}
    require(old_trace.order==new_trace.order,label+" original instruction path differs");
    require(a.bytes==b.bytes && x->executed==y->executed && old.events==modern.events,label+" architecture/dependency/effects/accounting differ");
    if(negative)y->wr<4>(12,y->r<4>(12)^1);bool rejected=false;try{Image(*x).compare(*y,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" negative discriminant");++cases;normal+=actual.empty()&&!negative;interrupted+=!actual.empty();negative_cases+=negative;
    sites.insert(old_trace.sites.begin(),old_trace.sites.end());last_path=old_trace.order;
    if(whole){++whole_cases;whole_normal+=expected.empty();whole_faults+=!expected.empty();whole_sites.insert(old_trace.sites.begin(),old_trace.sites.end());}
    if(kind17_contact || kind17_clear){auto next=clone(input);next->pc=cm::aot::player_movement::carrier_kind18_phase;
        if(kind17_clear){next->wr<4>(88,0);next->wr<4>(80,3);}compare(*next,options,negative);}

}
}
namespace cm::aot {
void player_movement_reference_step_seen(State& s,std::uint32_t pc){movement_noncarrier_step_seen(s,pc);}
void player_movement_reference_call_seen(State&,std::uint32_t,std::uint32_t){}
void player_movement_reference_jump_seen(State&,std::uint32_t,std::uint32_t){}
}
namespace cm::aot {void movement_noncarrier_step_seen(State& s,std::uint32_t pc){if(active){active->order.push_back(pc);active->sites.insert(pc);}
    // Matched prefix perturbations exercise the original wide-control fallback;
    // these are distinct from normal entry paths and actual runtime evidence.
    if(edits && edits->force_action_direction && pc==0x102303fc)s.memory.write<1>(s.r<4>(4)+0x46,0);
    if(edits && edits->force_wide_control && pc==0x1022ed14)s.wr<4>(12,0);
    if(edits && edits->force_negative_limit && pc==0x1022ed68)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_wide_condition && pc==0x1022f92c)s.wr<4>(12,0);
    if(edits && edits->force_negative_condition_limit && pc==0x1022f93c)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_wide_contact && pc==0x10230150)s.wr<4>(12,0);
    if(edits && edits->force_negative_contact_limit && pc==0x10230160)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_wide_reaction && pc==0x10230868)s.wr<4>(12,0);
    if(edits && edits->force_negative_reaction_limit && pc==0x10230878)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_wide_interest && pc==0x10230ef8)s.wr<4>(12,0);
    if(edits && edits->force_negative_interest_limit && pc==0x10230f08)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_wide_release && pc==0x10231a54)s.wr<4>(12,0);
    if(edits && edits->force_negative_release_limit && pc==0x10231a64)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_wide_play_condition && pc==0x10231c84)s.wr<4>(12,0);
    if(edits && edits->force_negative_play_condition_limit && pc==0x10231c94)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_wide_success && pc==0x10232108)s.wr<4>(12,0);
    if(edits && edits->force_negative_success_limit && pc==0x10232118)s.wr<4>(12,0x7fffffff);
    if(edits && edits->force_negative_cell_bias && pc==0x10231fa4)s.memory.write<2>(subject+0x1a0,std::uint16_t(-400));
    if(edits && edits->force_wide_transfer && pc==0x10233ff4)s.wr<4>(12,0);
    if(edits && edits->force_negative_transfer_limit && pc==0x10234004)s.wr<4>(12,0x7fffffff);
    if(edits && edits->zero_transfer_bound && pc==0x10233fb4)s.wr<4>(12,std::uint32_t(-20));
    if(edits && edits->force_wide_distance && pc==0x10234428)s.wr<4>(12,0);
    if(edits && edits->force_negative_distance_limit && pc==0x10234470)s.wr<4>(12,0x7fffffff);
    if(edits && edits->zero_distance_bound && pc==0x102343b0)s.wr<4>(0,s.r<4>(12));
    if(edits && edits->force_wide_reward && pc==0x10234b9c)s.wr<4>(12,0);
    if(edits && edits->force_negative_reward_limit && pc==0x10234bac)s.wr<4>(12,0x7fffffff);
    if(edits && edits->zero_reward_bound && pc==0x10234b5c)s.wr<4>(12,std::uint32_t(-50));
    if(edits && edits->force_wide_repeat && pc==0x10233964)s.wr<4>(12,0);
    if(edits && edits->force_negative_repeat_limit && pc==0x10233974)s.wr<4>(12,0x7fffffff);
    if(edits && edits->null_attachment_prefix && (pc==0x10232cb8 || pc==0x102336b8))s.memory.write<4>(subject+0x1b4,0);
}}
int main(int argc,char** argv){try {
    auto base=seed();compare(*base);compare(*base,{},true);
    for(unsigned side:{0u,1u})for(int membership:{-1,0,1})for(bool moved:{false,true}){
        auto s=seed();s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,std::uint8_t(membership));if(!moved)s->wr<4>(64,4);
        s->memory.write<1>(cell(4,6)+0x58+side,2);s->memory.write<4>(cell(4,6)+side*0x2c,subject);s->memory.write<4>(cell(4,6)+side*0x2c+4,neighbour);
        if(membership==1){s->memory.write<4>(cell(4,6)+side*0x2c,neighbour);s->memory.write<4>(cell(4,6)+side*0x2c+4,subject);}compare(*s);
    }
    for(unsigned surface:{0u,1u,2u,3u,4u,5u,8u,16u,17u,32u,64u,65u,128u})for(unsigned weather:{2u,3u,4u,5u}){
        auto s=seed();s->memory.write<1>(owner+1,surface);s->memory.write<1>(owner+0x30d8,weather);compare(*s);
    }
    for(unsigned role:{0u,4u})for(unsigned secondary:{0u,4u})for(unsigned suitability:{0u,2u}){
        Options o;o.role=role;o.secondary_role=secondary;o.suitability=suitability;compare(*base,o);
        for(unsigned flags:{0x8000u,0x2000u}){o.low_flags=flags;auto s=seed();s->memory.write<4>(owner+0x7c6c+0x588,0x100);compare(*s,o);}
    }
    for(unsigned side:{0u,1u})for(int team:{-1,0,1})for(unsigned designation:{0u,subject,neighbour})for(bool special:{false,true}){
        auto s=seed();s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,255);s->memory.write<1>(owner+0x7924,std::uint8_t(team));
        s->memory.write<4>(owner+0xdff8,designation);s->memory.write<1>(neighbour+0x27,side);Options o;o.override_facing=special;compare(*s,o);
    }
    for(unsigned side:{0u,1u})for(unsigned possession:{0u,subject})for(unsigned ball_status:{0u,5u}){
        auto s=seed();s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,255);s->memory.write<4>(owner+0xe000,possession);
        s->memory.write<1>(owner+0x791f,ball_status);s->memory.write<1>(owner+0x791d,5);s->memory.write<1>(owner+0x791e,6);compare(*s);
    }
    for(unsigned side:{0u,1u})for(unsigned count:{0u,1u})for(unsigned cursor:{stream,stream+126}){
        auto s=seed();s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,255);s->memory.write<1>(subject+0x104,count);s->memory.write<4>(globals+8,cursor);compare(*s);
    }
    for(unsigned side:{0u,1u})for(unsigned extent:{0u,neighbour})for(unsigned y:{0u,5u,11u}){
        auto s=seed();s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,255);
        s->memory.write<4>(owner+0xe014+side*4,extent);s->memory.write<1>(neighbour+0x106,y);compare(*s);
    }
    {Options o;o.role=1;compare(*base,o);}
    for(unsigned x:{0u,8u,9u,255u})for(unsigned y:{0u,11u,12u,255u}){
        auto s=seed();s->wr<4>(64,x);s->wr<4>(68,y);s->memory.write<1>(subject+0x28,255);
        s->memory.write<4>(owner+0xe000,subject);compare(*s);
    }
    for(unsigned side:{0u,1u})for(unsigned rival_side:{0u,1u}){
        auto s=seed();s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,255);
        s->memory.write<4>(owner+0xdff8,neighbour);s->memory.write<1>(neighbour+0x27,rival_side);
        s->memory.write<1>(owner+0x791d,5);s->memory.write<1>(owner+0x791e,6);compare(*s);
    }
    for(unsigned side:{0u,1u})for(unsigned x:{2u,3u,4u,5u,6u})for(unsigned y:{0u,1u,2u,9u,10u,11u}){
        auto s=seed();s->wr<4>(64,x);s->wr<4>(68,y);s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,255);
        s->memory.write<1>(owner+0x791d,x);s->memory.write<1>(owner+0x791e,y);s->memory.write<1>(owner+0x1e6,1);compare(*s);
    }
    for(bool owns:{false,true})for(unsigned x:{3u,4u,5u})for(unsigned y:{5u,6u,7u})for(unsigned movement_kind:{0u,15u,256u}){
        auto s=preparation_seed(x,y,movement_kind,owns);
        compare(*s);
        Options o;o.facing_allowed=false;s->memory.write<1>(subject+0x104,1);compare(*s,o);
        o.neighbour_gate=false;compare(*s,o);
        o.neighbour_gate=true;s->memory.write<1>(subject+0x107,120);s->memory.write<4>(globals+8,stream+126);compare(*s,o);
        for(unsigned field:{0x10fu,0x10du,0x108u}){s->memory.write<1>(subject+field,1);compare(*s,o);s->memory.write<1>(subject+field,0);}
    }
    for(unsigned x:{2u,4u,6u})for(unsigned y:{4u,6u,8u}){
        auto s=preparation_seed(x,y,15);s->memory.write<1>(subject+0x107,120);s->memory.write<1>(subject+0x108,1);compare(*s);
    }
    for(unsigned limit:{0u,1u,4u,22u,60u,100u,150u,200u}){auto s=preparation_seed(2,4,15);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=6;++call){Options o;o.fault_call=call;compare(*preparation_seed(2,4,15),o);}
    compare(*preparation_seed(2,4,15),{},true);
    const auto carrier_seed=[](double control,unsigned kind,int affinity,int bias,bool accumulate){auto s=seed();s->pc=cm::aot::player_movement::carrier_phase;
        s->wr<4>(72,kind);s->wr<4>(76,accumulate);s->memory.write<4>(subject+0x50,float_bits<4>(control));s->memory.write<4>(subject+0x9c,float_bits<4>(0.25));
        for(unsigned offset:{0x13cu,0x140u,0x142u})s->memory.write<2>(subject+offset,std::uint16_t(affinity));
        s->memory.write<2>(subject+0x1a0,std::uint16_t(bias));return s;};
    for(double control:{-32768.0,-1.0,0.0,75.25,9999.0,10000.0,32767.0})for(unsigned kind:{0u,15u,18u})
      for(int affinity:{-32768,-1000,0,1000,32767})for(int bias:{-32768,0,32767})for(bool accumulate:{false,true})
        compare(*carrier_seed(control,kind,affinity,bias,accumulate));
    for(unsigned kind:{0u,15u,18u})for(bool negative_limit:{false,true})for(int sample:{-99999,0,99999})for(bool wrap:{false,true}){
        auto s=carrier_seed(75,kind,1500,1,true);if(wrap)s->memory.write<4>(globals+8,stream+126);
        Options o;o.force_wide_control=!negative_limit;o.force_negative_limit=negative_limit;o.sampled=sample;compare(*s,o);
    }
    for(unsigned kind:{0u,15u,18u}){
        auto wrap=carrier_seed(10000,kind,1500,1,true);wrap->memory.write<4>(globals+8,stream+126);compare(*wrap);
        auto positive_bias=carrier_seed(75,kind,1500,199,true);compare(*positive_bias);
        auto wide=carrier_seed(-32768,kind,32767,32767,false);wide->memory.write<2>(stream+2,35000);Options o;o.sampled=2000000;compare(*wide,o);
    }
    for(unsigned limit:{0u,1u,20u,30u,60u,100u,200u,300u}){auto s=carrier_seed(75,15,1500,1,true);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=2;++call){auto s=carrier_seed(75,15,1500,1,true);s->memory.write<4>(globals+8,stream+126);Options o;o.force_wide_control=true;o.fault_call=call;compare(*s,o);}
    compare(*carrier_seed(75,15,1500,1,true),{},true);
    const auto environment_seed=[](unsigned surface,int ball,int condition){auto s=seed();s->pc=cm::aot::player_movement::carrier_decision_phase;
        s->wr<4>(84,150);s->memory.write<1>(owner+1,surface);s->memory.write<1>(owner+0x791f,std::uint8_t(ball));
        s->memory.write<2>(subject+0x2a,std::uint16_t(condition));return s;};
    for(unsigned surface:{0u,1u,2u,3u,8u,16u,32u,64u,255u})for(int ball:{-128,-11,-1,0,1,5,10,11,127})
        for(int condition:{-32768,0,299,300,301,32767})compare(*environment_seed(surface,ball,condition));
    for(bool negative_limit:{false,true})for(int sample:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=environment_seed(8,11,32767);s->memory.write<4>(globals+8,cursor);Options o;o.force_wide_condition=!negative_limit;
        o.force_negative_condition_limit=negative_limit;o.sampled=sample;compare(*s,o);
    }
    for(unsigned surface:{1u,8u,32u}){auto s=environment_seed(surface,0,300);s->memory.write<4>(globals+8,stream+126);compare(*s);}
    {auto s=environment_seed(0,0,32767);s->memory.write<4>(globals+8,stream+126);compare(*s);}
    for(unsigned limit:{0u,1u,20u,40u,80u,120u,200u}){auto s=environment_seed(8,11,32767);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=3;++call){Options o;o.force_wide_condition=true;o.fault_call=call;compare(*environment_seed(8,11,32767),o);}
    compare(*environment_seed(8,11,32767),{},true);
    const auto interaction_seed=[](unsigned side,unsigned x,unsigned y,unsigned kind=0){auto s=seed();s->pc=cm::aot::player_movement::carrier_interaction_phase;
        s->wr<4>(72,17);s->wr<4>(84,150);s->wr<4>(88,32767);s->memory.write<1>(subject+0x27,side);
        s->memory.write<1>(subject+0x105,x);s->memory.write<1>(subject+0x106,y);s->memory.write<2>(subject+0x19c,kind);
        s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,100);s->memory.write<2>(stream+4,101);return s;};
    for(unsigned side:{0u,1u,2u,255u})for(unsigned x:{0u,1u,2u,3u,4u,5u,6u,7u,8u,255u})
        for(unsigned y:{0u,3u,4u,5u,6u,7u,8u,11u,255u})for(double measurement:{0.0,20.0}){
            auto s=interaction_seed(side,x,y);Options o;o.measurement=measurement;compare(*s,o);
        }
    for(unsigned kind:{15u,16u,17u,18u,19u,0xffffu})for(unsigned other:{0u,neighbour}){
        auto s=interaction_seed(0,4,8,kind);Options o;o.other_actor=other;compare(*s,o);
    }
    for(unsigned side:{0u,1u})for(unsigned role:{0u,1u,0x10001u})for(bool blocked:{false,true}){
        auto s=interaction_seed(side,4,side?3:8);Options o;o.role=role;o.zone_blocked=blocked;compare(*s,o);
    }
    for(unsigned x:{2u,3u,4u,5u,6u})for(unsigned y:{4u,5u,6u,7u})for(unsigned parity:{0u,1u}){
        auto s=interaction_seed(0,x,y);s->memory.write<2>(stream+4,100+parity);compare(*s);
    }
    for(unsigned cursor:{stream+124,stream+126}){
        auto s=interaction_seed(0,4,5);s->memory.write<4>(globals+8,cursor);
        s->memory.write<2>(stream,104);s->memory.write<2>(stream+2,101);s->memory.write<2>(stream+126,100);compare(*s);
    }
    for(double measurement:{-1000.0,-1.0,4.0,5.0,100.0,200.0,std::numeric_limits<double>::quiet_NaN()}){
        auto s=interaction_seed(0,0,8);Options o;o.measurement=measurement;compare(*s,o);
    }
    {auto s=interaction_seed(0,4,5);s->memory.write<2>(stream+2,101);compare(*s);}
    for(unsigned limit:{0u,1u,20u,40u,80u,120u,160u}){auto s=interaction_seed(0,4,5);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=7;++call){Options o;o.measurement=20;o.fault_call=call;compare(*interaction_seed(0,4,5),o);}
    compare(*interaction_seed(0,4,5),{},true);
    const auto contact_seed=[](unsigned side,int decision=100,int difficulty=120,int condition=-11){auto s=seed();s->pc=cm::aot::player_movement::carrier_contact_phase;
        s->wr<4>(84,std::uint32_t(decision));s->wr<4>(88,neighbour);s->memory.write<1>(subject+0x27,side);
        s->memory.write<2>(phase_stack+0x48,std::uint16_t(difficulty));s->memory.write<1>(phase_stack+0x46,1);
        s->memory.write<1>(phase_stack+0x43,1);s->memory.write<1>(owner+0x7931,1);
        s->memory.write<2>(neighbour+0x32,std::uint16_t(condition));return s;};
    {auto s=contact_seed(0);s->wr<4>(88,0);compare(*s);}
    for(unsigned side:{0u,1u})for(unsigned role:{0u,1u,0x10001u})for(bool blocked:{false,true})for(unsigned result:{0u,1u,255u})
        for(int condition:{-32768,-11,-10,0,10,32767}){auto s=contact_seed(side,100,120,condition);Options o;o.role=role;o.zone_blocked=blocked;o.contact_result=result;compare(*s,o);}
    for(int decision:{-32768,0,100,32767})for(int difficulty:{-32768,0,100,32767})for(unsigned sample:{100u,101u,65535u}){
        auto s=contact_seed(0,decision,difficulty);s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,sample);compare(*s);
    }
    for(unsigned flag:{0u,1u,128u})for(unsigned forbidden:{0u,1u,128u})for(unsigned owner_flag:{0u,1u})for(unsigned sample:{120u,121u}){
        auto s=contact_seed(0);s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,sample);s->memory.write<1>(owner+0x7931,owner_flag);
        Options o;o.zone_blocked=true;o.contact_flag=flag;o.contact_forbidden=forbidden;compare(*s,o);
    }
    for(unsigned result:{0u,1u,128u,255u})for(unsigned marker:{0u,1u})for(unsigned sample:{120u,121u}){
        auto s=contact_seed(0);s->memory.write<1>(owner+0x7931,0);s->memory.write<1>(phase_stack+0x43,marker);
        s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,sample);Options o;o.contact_result=result;compare(*s,o);
    }
    for(bool negative_limit:{false,true})for(int condition:{-32768,32767})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=contact_seed(0,100,120,condition);s->memory.write<4>(globals+8,cursor);Options o;o.zone_blocked=true;
        o.force_wide_contact=!negative_limit;o.force_negative_contact_limit=negative_limit;o.sampled=sampled;compare(*s,o);
    }
    for(unsigned cursor:{stream+124,stream+126})for(int condition:{0,-11})for(int difficulty:{0,120}){
        auto s=contact_seed(0,100,difficulty,condition);s->memory.write<4>(globals+8,cursor);Options o;o.zone_blocked=true;compare(*s,o);
    }
    for(unsigned limit:{0u,1u,20u,40u,80u,120u,160u,200u}){auto s=contact_seed(0);s->budget=limit;Options o;o.zone_blocked=true;compare(*s,o);}
    for(unsigned call=1;call<=8;++call){Options o;o.zone_blocked=true;o.force_wide_contact=true;o.fault_call=call;compare(*contact_seed(0),o);}
    compare(*contact_seed(0),{},true);
    const auto outcome_seed=[](bool failed){auto s=seed();s->pc=failed?cm::aot::player_movement::carrier_failed_contact_phase:cm::aot::player_movement::carrier_resolved_contact_phase;
        s->wr<4>(88,neighbour);s->memory.write<1>(phase_stack+0x46,0xff);s->memory.write<1>(neighbour+4,255);
        s->memory.write<2>(neighbour+0x36,0x8000);return s;};
    for(bool failed:{false,true})for(unsigned flag:{0u,1u,128u})for(unsigned marker:{0u,1u})for(bool blocked:{false,true})
        for(unsigned status:{0u,0x10u,0x30u,0x880u,0x890u,0x8b0u,0xffffffffu}){auto s=outcome_seed(failed);
            s->memory.write<1>(phase_stack+0x40,flag);s->memory.write<1>(phase_stack+0x43,marker);Options o;o.zone_blocked=blocked;o.contact_status=status;compare(*s,o);}
    for(unsigned limit:{0u,1u,8u,16u,24u,32u}){auto s=outcome_seed(true);s->budget=limit;Options o;o.contact_status=0x8b0;compare(*s,o);}
    for(unsigned call=1;call<=3;++call){auto s=outcome_seed(true);s->memory.write<1>(phase_stack+0x40,1);Options o;o.contact_status=0x8b0;o.fault_call=call;compare(*s,o);}
    {auto s=outcome_seed(false);s->wr<4>(88,0);compare(*s);}
    compare(*outcome_seed(true),{},true);
    constexpr auto action_table=fixture+0x30000,detail=fixture+0x31000;
    const auto action_seed=[](unsigned direction,unsigned result,int factor=0,unsigned level=3){auto s=seed();s->pc=cm::aot::player_movement::carrier_contact_decision_phase;
        s->wr<4>(88,neighbour);s->memory.write<4>(toc-0x51f4,action_table);for(unsigned n=0;n<47;++n)s->memory.write<4>(action_table+n*4,original_contact_action_table[n]);
        s->memory.write<1>(phase_stack+0x46,direction);s->memory.write<1>(phase_stack+0x47,result);s->memory.write<1>(phase_stack+0x41,level);
        s->memory.write<4>(neighbour+0x6c,detail);s->memory.write<1>(detail+0x66,std::uint8_t(factor));s->memory.write<2>(neighbour+0x36,0x8000);return s;};
    for(unsigned direction=0;direction<=255;++direction)for(unsigned result:{0u,2u,3u,4u,5u,6u,7u,128u,255u}){
        auto s=action_seed(direction,result);compare(*s);Options o;o.zone_blocked=true;compare(*s,o);
    }
    for(unsigned destination:{0x10230420u,0x1023044cu,0x10230478u,0x102304a4u,0x102304d0u,0x102304fcu,0x10230528u,0x10230554u,0x10230580u,0x102305acu,0x102305d8u,0x10230604u,cm::aot::player_movement::carrier_contact_followup_phase,return_sentinel}){
        auto s=action_seed(0,2);s->memory.write<4>(action_table,destination);compare(*s);
    }
    for(int factor:{-128,-7,-6,-5,0,127})for(unsigned level:{0u,3u,11u,127u,128u,255u})for(bool allowed:{false,true}){
        auto s=action_seed(45,0,factor,level);Options o;o.zone_blocked=true;o.reaction_allowed=allowed;compare(*s,o);
    }
    for(bool negative_limit:{false,true})for(int factor:{-128,127})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=action_seed(45,2,factor);s->memory.write<4>(globals+8,cursor);Options o;o.zone_blocked=true;o.force_wide_reaction=!negative_limit;
        o.force_negative_reaction_limit=negative_limit;o.sampled=sampled;compare(*s,o);
    }
    for(unsigned random:{0u,1u,2u,3u,4u,65535u}){auto s=action_seed(45,0,-6,0);s->memory.write<4>(globals,0);
        for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);Options o;o.zone_blocked=true;compare(*s,o);}
    for(unsigned cursor:{stream+124,stream+126}){auto s=action_seed(45,0,-6,0);s->memory.write<4>(globals+8,cursor);Options o;o.zone_blocked=true;compare(*s,o);}
    {auto s=action_seed(45,0,-6,0);s->memory.write<4>(globals,0xfffffff0);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,1);Options o;o.zone_blocked=true;compare(*s,o);}
    {auto s=action_seed(45,2,-6,0);s->memory.write<4>(globals+8,stream+126);Options o;o.zone_blocked=true;compare(*s,o);}
    for(unsigned limit:{0u,1u,4u,5u,8u,9u,16u,32u,64u,128u,256u}){auto s=action_seed(45,0,-6,0);s->budget=limit;Options o;o.zone_blocked=true;compare(*s,o);}
    for(unsigned call=1;call<=7;++call){Options o;o.zone_blocked=true;o.force_wide_reaction=true;o.fault_call=call;compare(*action_seed(45,0,127,0),o);}
    {auto s=action_seed(0,0);s->memory.write<4>(toc-0x51f4,0);compare(*s);}
    compare(*action_seed(45,0,-6,0),{},true);
    constexpr auto other_detail=fixture+0x33000,subject_detail=fixture+0x34000,traits=fixture+0x35000,contract=fixture+0x36000;
    constexpr auto metrics=fixture+0x37000,registry=fixture+0x38000,clubs=fixture+0x39000,staff=fixture+0x3a000;
    const auto followup_seed=[](unsigned result=4,unsigned direction=44,int interest=127){auto s=seed();s->pc=cm::aot::player_movement::carrier_contact_followup_phase;
        s->wr<4>(64,0x1ff8);s->wr<4>(68,0x1f79);s->wr<4>(88,neighbour);
        s->memory.write<4>(toc-0x7b3c,metrics);s->memory.write<4>(metrics,metrics+4);s->memory.write<4>(toc-0x7be4,registry);s->memory.write<4>(registry,100);
        s->memory.write<4>(neighbour+0x6c,other_detail);s->memory.write<4>(subject+0x6c,subject_detail);
        s->memory.write<4>(other_detail,10);s->memory.write<4>(subject_detail,11);s->memory.write<1>(other_detail+0x67,0);
        s->memory.write<4>(neighbour+0x70,traits);s->memory.write<1>(traits+0x1c,100);s->memory.write<1>(traits+0x25,100);s->memory.write<2>(traits+10,10000);
        s->memory.write<4>(neighbour+0xfc,contract);s->memory.write<4>(contract,123);s->memory.write<4>(owner+0x3114,output+0x2000);
        s->memory.write<1>(phase_stack+0x47,result);s->memory.write<1>(phase_stack+0x46,direction);s->memory.write<1>(phase_stack+0x42,10);
        s->memory.write<1>(phase_stack+0x40,1);s->memory.write<1>(phase_stack+0x41,std::uint8_t(interest));
        s->memory.write<4>(owner+0x3118,clubs);s->memory.write<4>(clubs+0x1c,clubs+0x100);s->memory.write<4>(clubs+0x20,clubs+0x200);
        s->memory.write<4>(clubs+0x100+0x2c,staff);s->memory.write<4>(clubs+0x200+0x2c,staff+0x200);
        s->memory.write<1>(staff+0x119,8);s->memory.write<1>(staff+0x200+0x119,8);
        s->memory.write<4>(globals,0);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,100);return s;};
    for(unsigned result:{0u,2u,3u,4u,5u,6u,7u,128u,255u})for(unsigned direction:{0u,44u,47u,255u})for(unsigned report_kind:{0u,5u,10u,11u,255u})
        for(unsigned event:{0x1f79u,0x1f7au}){auto s=followup_seed(result,direction);s->wr<4>(68,event);s->memory.write<1>(phase_stack+0x42,report_kind);Options o;o.measurement=15;compare(*s,o);}
    for(unsigned rating_code:{0u,0xffffu,0x8000u}){auto s=followup_seed();s->wr<4>(64,rating_code);compare(*s);}
    for(double rating:{-1000.0,-1.0,0.0,1.0,2.0,3.0,15.0,127.0,128.0,200.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){
        Options o;o.measurement=rating;compare(*followup_seed(),o);
    }
    for(unsigned n=0;n<9;++n){auto s=followup_seed();Options o;o.measurement=15;
        switch(n){case 0:s->memory.write<4>(owner+0x3114,0);break;case 1:s->memory.write<4>(neighbour+0x6c,0);break;
        case 2:s->memory.write<4>(other_detail,100);break;case 3:s->memory.write<4>(subject_detail,100);break;
        case 4:s->memory.write<4>(subject+0x6c,0);break;case 5:s->memory.write<4>(neighbour+0xfc,0);break;
        case 6:o.report_excluded=true;break;case 7:s->memory.write<1>(other_detail+0x67,127);break;case 8:o.rating_null_owner=true;break;}compare(*s,o);
    }
    for(unsigned field:{0x1cu,0x25u,10u})for(int attribute:{-32768,-128,-1,0,1,3,10,127,3000,6500,32767}){
        auto s=followup_seed();if(field==10)s->memory.write<2>(traits+field,std::uint16_t(attribute));else s->memory.write<1>(traits+field,std::uint8_t(attribute));Options o;o.measurement=15;compare(*s,o);
    }
    for(unsigned result:{2u,3u,4u,5u,6u,7u,8u})for(unsigned flag:{0u,1u})for(unsigned event:{0x1f79u,0x1f7au}){
        auto s=followup_seed(result,44);s->wr<4>(68,event);s->memory.write<1>(phase_stack+0x40,flag);compare(*s);
    }
    for(int interest:{-128,-1,0,1,5,127})for(unsigned sample:{0u,1u,2u,3u,4u,65535u})for(unsigned side:{0u,1u}){
        auto s=followup_seed(2,0,interest);s->memory.write<4>(owner+0x3114,0);s->memory.write<1>(neighbour+0x27,side);
        for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,sample);compare(*s);
    }
    for(unsigned side:{0u,1u})for(unsigned tag:{0u,8u})for(unsigned pointer:{0u,staff}){auto s=followup_seed(2,0,127);
        s->memory.write<4>(owner+0x3114,0);s->memory.write<1>(neighbour+0x27,side);s->memory.write<4>(clubs+0x100+0x2c,pointer);s->memory.write<4>(clubs+0x200+0x2c,pointer);s->memory.write<1>(staff+0x119,tag);compare(*s);
    }
    for(bool negative_limit:{false,true})for(int interest:{-128,127})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=followup_seed(2,0,interest);s->memory.write<4>(owner+0x3114,0);s->memory.write<4>(globals+8,cursor);Options o;o.force_wide_interest=!negative_limit;o.force_negative_interest_limit=negative_limit;o.sampled=sampled;compare(*s,o);
    }
    for(unsigned cursor:{stream+116,stream+118,stream+120,stream+122,stream+124,stream+126}){auto s=followup_seed();s->memory.write<4>(globals+8,cursor);Options o;o.measurement=15;compare(*s,o);}
    {auto s=followup_seed(2,0,0);s->memory.write<4>(owner+0x3114,0);s->memory.write<4>(globals+8,stream+124);compare(*s);}
    for(unsigned limit:{0u,1u,16u,32u,64u,128u,192u,256u,384u}){auto s=followup_seed();s->budget=limit;Options o;o.measurement=15;compare(*s,o);}
    for(unsigned call=1;call<=8;++call){Options o;o.measurement=15;o.fault_call=call;compare(*followup_seed(),o);}
    compare(*followup_seed(),{},true);
    const auto restart_seed=[](unsigned result,unsigned event,unsigned flag,unsigned random){auto s=seed();s->pc=cm::aot::player_movement::carrier_restart_selection_phase;
        s->wr<4>(68,event);s->wr<4>(88,neighbour);s->memory.write<1>(phase_stack+0x47,result);s->memory.write<1>(phase_stack+0x40,flag);
        s->memory.write<1>(phase_stack+0x46,44);s->memory.write<4>(phase_stack+0x4c,float_bits<4>(1.25));s->memory.write<2>(neighbour+0x36,0x8000);
        s->memory.write<4>(globals,0);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);return s;};
    for(unsigned result:{0u,2u,3u,4u,5u,6u,7u,255u})for(unsigned event:{0x1f79u,0x1f7au,0x1234u})for(unsigned flag:{0u,1u,128u})for(unsigned receiver:{0u,neighbour}){
        auto s=restart_seed(result,event,flag,18);Options o;o.restart_receiver=receiver;compare(*s,o);
    }
    for(unsigned result:{3u,7u})for(bool qualified:{false,true})for(bool repeat:{false,true})for(bool position:{false,true})for(bool blocked:{false,true}){
        auto s=restart_seed(result,0x1f79,0,18);Options o;o.restart_qualified=qualified;o.restart_repeat_qualified=repeat;o.restart_position=position;o.zone_blocked=blocked;compare(*s,o);
    }
    for(unsigned random:{0u,1u,9u,18u,19u,27u,65535u})for(unsigned flag:{0u,1u})for(unsigned result:{2u,3u,4u,5u,6u,7u}){
        auto s=restart_seed(result,0x1f79,flag,random);compare(*s);
    }
    for(unsigned forbidden:{0u,1u,128u})for(unsigned side:{0u,1u}){auto s=restart_seed(4,0x1f7a,1,18);s->memory.write<1>(subject+0x27,side);Options o;o.contact_forbidden=forbidden;compare(*s,o);}
    for(unsigned cursor:{stream+120,stream+122,stream+124,stream+126})for(unsigned flag:{0u,1u}){auto s=restart_seed(4,0x1f7a,flag,18);s->memory.write<4>(globals+8,cursor);compare(*s);}
    for(unsigned limit:{0u,1u,16u,32u,64u,96u,128u,192u,256u}){auto s=restart_seed(3,0x1f79,0,18);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=8;++call){Options o;o.fault_call=call;compare(*restart_seed(3,0x1f79,0,18),o);}
    compare(*restart_seed(3,0x1f79,0,18),{},true);
    constexpr auto possession_detail=fixture+0x3c000;
    const auto possession_seed=[](double control=0,unsigned mode=3,int decision=0,unsigned opponent=0){auto s=seed();s->pc=cm::aot::player_movement::carrier_no_contact_phase;
        s->wr<4>(80,mode);s->wr<4>(84,std::uint32_t(decision));s->wr<4>(88,opponent);s->memory.write<4>(owner+0xe000,subject);
        s->memory.write<4>(subject+0xe8,float_bits<4>(control));s->memory.write<4>(singles+0x3c,float_bits<4>(1));s->memory.write<4>(singles+0x90,float_bits<4>(0));
        s->memory.write<4>(subject+0x6c,possession_detail);s->memory.write<1>(possession_detail+0x66,0xff);s->memory.write<2>(subject+0x36,0x8000);
        s->memory.write<2>(phase_stack+0x48,0xffff);s->memory.write<4>(globals,0);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,100);return s;};
    for(double control:{-1000000.0,-65536.0,-65535.0,-1.0,-0.0,0.25,1.0,75.25,65535.0,65536.0,1000000.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})
      for(unsigned mode:{3u,4u,255u})for(int decision:{-32768,0,1,32767})for(unsigned opponent:{0u,neighbour})compare(*possession_seed(control,mode,decision,opponent));
    for(unsigned flag:{1u,128u,255u})for(int attribute:{-128,-2,-1,0,1,127})for(unsigned random:{0u,1u,100u,65535u})for(bool allowed:{false,true}){
        auto s=possession_seed();s->memory.write<1>(subject+0x1d,flag);s->memory.write<1>(possession_detail+0x66,std::uint8_t(attribute));
        for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);Options o;o.reaction_allowed=allowed;compare(*s,o);
    }
    for(int difficulty:{-32768,0,32767}){auto s=possession_seed(0,3,0,neighbour);s->memory.write<2>(phase_stack+0x48,std::uint16_t(difficulty));compare(*s);}
    for(double control:{-1000000.0,1000000.0})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=possession_seed(control);s->memory.write<4>(globals+8,cursor);Options o;o.sampled=sampled;compare(*s,o);
    }
    for(bool negative_limit:{false,true})for(int attribute:{-128,127})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=possession_seed();s->memory.write<1>(subject+0x1d,1);s->memory.write<1>(possession_detail+0x66,std::uint8_t(attribute));s->memory.write<4>(globals+8,cursor);
        Options o;o.force_wide_release=!negative_limit;o.force_negative_release_limit=negative_limit;o.sampled=sampled;compare(*s,o);
    }
    {auto s=possession_seed(16777216.0);s->memory.write<4>(singles+0x90,float_bits<4>(1));compare(*s);}
    {auto s=possession_seed();s->memory.write<4>(subject+0x6c,0);compare(*s);}
    for(unsigned cursor:{stream+122,stream+124,stream+126}){auto s=possession_seed();s->memory.write<4>(globals+8,cursor);compare(*s);}
    for(unsigned limit:{0u,1u,9u,12u,13u,14u,32u,64u,128u,200u,256u}){auto s=possession_seed(-1000000.0);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=5;++call){Options o;o.fault_call=call;auto s=possession_seed(1000000.0);s->memory.write<4>(globals+8,stream+126);compare(*s,o);}
    compare(*possession_seed(),{},true);
    const auto play_preparation_seed=[](unsigned side=0,int condition=0,unsigned opponent=0,int decision=100,int difficulty=50){auto s=seed();s->pc=cm::aot::player_movement::carrier_play_decision_phase;
        s->wr<4>(84,std::uint32_t(decision));s->wr<4>(88,opponent);s->memory.write<1>(subject+0x27,side);s->memory.write<2>(subject+0x32,std::uint16_t(condition));
        s->memory.write<2>(phase_stack+0x48,std::uint16_t(difficulty));return s;};
    for(unsigned side:{0u,1u,2u,128u,255u})for(int condition:{-32768,-11,-10,0,10,32767})for(bool blocked:{false,true}){
        Options o;o.zone_blocked=blocked;compare(*play_preparation_seed(side,condition),o);
    }
    for(unsigned opponent:{0u,neighbour})for(int decision:{-32768,0,100,32767})for(int difficulty:{-32768,0,100,32767})compare(*play_preparation_seed(0,0,opponent,decision,difficulty));
    for(unsigned side:{0u,1u})for(unsigned own:{0u,1u,127u,128u,255u})for(unsigned other:{0u,1u,9u,10u,127u,128u,255u}){
        auto s=play_preparation_seed(side);s->memory.write<1>(owner+0x77bc+side,own);s->memory.write<1>(owner+0x77bc+!side,other);compare(*s);
    }
    for(bool negative_limit:{false,true})for(int condition:{-32768,32767})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=play_preparation_seed(0,condition);s->memory.write<4>(globals+8,cursor);Options o;o.force_wide_play_condition=!negative_limit;
        o.force_negative_play_condition_limit=negative_limit;o.sampled=sampled;compare(*s,o);
    }
    for(unsigned cursor:{stream+124,stream+126}){auto s=play_preparation_seed(0,-11);s->memory.write<4>(globals+8,cursor);compare(*s);}
    for(unsigned limit:{0u,1u,8u,16u,32u,64u,100u,128u}){auto s=play_preparation_seed(0,-11);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=4;++call){Options o;o.fault_call=call;compare(*play_preparation_seed(0,-11),o);}
    compare(*play_preparation_seed(0,-11),{},true);
    const auto play_bias_seed=[](unsigned kind=17,unsigned opponent=0,int bias=0,int decision=100,int difficulty=50,double skill=1,double factor=1,unsigned count=1){auto s=seed();s->pc=cm::aot::player_movement::carrier_play_ready_phase;
        s->wr<4>(72,kind);s->wr<4>(84,std::uint32_t(decision));s->wr<4>(88,opponent);s->memory.write<2>(subject+0x1a0,std::uint16_t(bias));
        s->memory.write<2>(phase_stack+0x48,std::uint16_t(difficulty));s->memory.write<4>(subject+0xe8,float_bits<4>(skill));s->memory.write<4>(subject+0x98,float_bits<4>(factor));
        s->memory.write<4>(singles,float_bits<4>(0));s->memory.write<1>(subject+0x17,0xff);s->memory.write<1>(phase_stack+0x44,2);
        s->memory.write<1>(subject+0x11,255);s->memory.write<2>(subject+0x36,0x7fff);s->memory.write<4>(owner+0x3108,0xfffffff0);s->memory.write<4>(owner+0x310c,0x7fffffff);
        s->memory.write<1>(cell(4,6)+0x59,count);return s;};
    for(int bias:{-32768,-250,-200,-199,0,199,200,250,32767})for(int decision:{-32768,0,100,32767})for(int difficulty:{-32768,0,200,32767})for(unsigned count:{0u,1u})compare(*play_bias_seed(17,0,bias,decision,difficulty,1,1,count));
    for(unsigned kind:{0u,15u,16u,17u,18u,19u,0xffffu})for(unsigned opponent:{0u,neighbour})for(unsigned role:{0u,1u,0x10001u})for(int delay:{-128,-1,0,1,2,3,127}){
        auto s=play_bias_seed(kind,opponent);s->memory.write<1>(phase_stack+0x44,std::uint8_t(delay));Options o;o.role=role;compare(*s,o);
    }
    for(double skill:{-10000.0,-1.0,0.0,1.0,10000.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})
        for(double factor:{-1.0,0.0,1.0,16777216.0})compare(*play_bias_seed(17,0,0,100,50,skill,factor));
    for(int attribute:{-128,-2,-1,0,1,127})for(bool allowed:{false,true})for(unsigned random:{0u,1u,100u,65535u}){
        auto s=play_bias_seed(17,neighbour);s->memory.write<1>(subject+0x17,std::uint8_t(attribute));s->memory.write<4>(globals,0);
        for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);Options o;o.reaction_allowed=allowed;compare(*s,o);
    }
    for(bool negative_limit:{false,true})for(int attribute:{-128,127})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=play_bias_seed(17,neighbour);s->memory.write<1>(subject+0x17,std::uint8_t(attribute));s->memory.write<4>(globals+8,cursor);Options o;o.force_wide_success=!negative_limit;o.force_negative_success_limit=negative_limit;o.sampled=sampled;compare(*s,o);
    }
    for(unsigned cursor:{stream+124,stream+126}){auto s=play_bias_seed(17,neighbour);s->memory.write<4>(globals+8,cursor);compare(*s);}
    for(unsigned side:{0u,1u,255u}){auto s=play_bias_seed();s->memory.write<1>(subject+0x27,side);compare(*s);}
    for(unsigned limit:{0u,1u,16u,32u,48u,64u,96u,128u,192u,256u}){auto s=play_bias_seed(17,neighbour);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=4;++call){Options o;o.fault_call=call;compare(*play_bias_seed(17,neighbour),o);}
    compare(*play_bias_seed(),{},true);
    {auto s=play_bias_seed(17,0,0,100,50,1,1,0);Options o;o.force_negative_cell_bias=true;compare(*s,o);}
    const auto kind17_contact_seed=[](unsigned mode=3,unsigned opponent=neighbour,unsigned active=1,double skill=75,unsigned flags=0){auto s=seed();s->pc=cm::aot::player_movement::carrier_kind17_phase;
        s->wr<4>(76,active);s->wr<4>(80,mode);s->wr<4>(88,opponent);s->memory.write<2>(subject+0x19c,15);
        s->memory.write<4>(subject+0x74,float_bits<4>(20));s->memory.write<4>(neighbour+0x74,float_bits<4>(15));s->memory.write<4>(subject+0x98,float_bits<4>(skill));
        s->memory.write<4>(singles+0x94,float_bits<4>(10));s->memory.write<4>(singles+0x5c,float_bits<4>(2));s->memory.write<8>(doubles+0x90,float_bits<8>(3));
        s->memory.write<4>(owner+0x81f4,flags);s->memory.write<4>(owner+0x9ad4,flags);return s;};
    for(unsigned mode:{3u,4u,255u})for(unsigned opponent:{0u,neighbour})for(bool blocked:{false,true})for(bool facing:{false,true})for(unsigned active:{0u,1u,128u}){
        auto s=kind17_contact_seed(mode,opponent,active);Options o;o.zone_blocked=blocked;o.override_facing=facing;compare(*s,o);
        if(mode==4){s->memory.write<2>(subject+0x19c,17);compare(*s,o);}
    }
    for(unsigned role:{0u,1u,0x10001u})for(double space:{-1.0,0.0,3.0,4.0,std::numeric_limits<double>::quiet_NaN()})for(unsigned flags:{0u,0x400u})for(unsigned side:{0u,1u}){
        auto s=kind17_contact_seed(3,neighbour,1,75,flags);s->memory.write<1>(subject+0x27,side);Options o;o.role=role;o.measurement=space;compare(*s,o);
    }
    {auto s=kind17_contact_seed();Options o;o.role=1;o.zone_blocked=true;compare(*s,o);}
    for(double strength:{0.0,10.0,20.0,std::numeric_limits<double>::quiet_NaN()})for(double opponent_strength:{0.0,9.0,20.0})for(double skill:{-1000000.0,-1.0,0.0,75.0,1000000.0}){
        auto s=kind17_contact_seed(3,neighbour,0,skill);s->memory.write<4>(subject+0x74,float_bits<4>(strength));s->memory.write<4>(neighbour+0x74,float_bits<4>(opponent_strength));compare(*s);
    }
    for(double skill:{-1000000.0,1000000.0})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=kind17_contact_seed(3,neighbour,0,skill);s->memory.write<4>(globals+8,cursor);Options o;o.sampled=sampled;compare(*s,o);
    }
    for(unsigned sample:{0u,1u,20u,29u}){auto s=kind17_contact_seed(3,neighbour,0,0);s->memory.write<4>(globals,0);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,sample);compare(*s);}
    for(unsigned cursor:{stream+124,stream+126}){auto s=kind17_contact_seed(3,neighbour,0,0);s->memory.write<4>(globals+8,cursor);compare(*s);}
    for(unsigned limit:{0u,1u,16u,32u,64u,100u,160u,200u,256u}){auto s=kind17_contact_seed();s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=6;++call){Options o;o.fault_call=call;compare(*kind17_contact_seed(),o);}
    compare(*kind17_contact_seed(),{},true);
    const auto kind17_clear_seed=[](unsigned side=0,unsigned target_x=4,unsigned target_y=6,unsigned actual_x=4,unsigned actual_y=6,unsigned active=0){auto s=seed();s->pc=cm::aot::player_movement::carrier_kind17_clear_phase;
        s->wr<4>(64,target_x);s->wr<4>(68,target_y);s->wr<4>(76,active);s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x105,actual_x);s->memory.write<1>(subject+0x106,actual_y);
        s->memory.write<4>(subject+0x1b4,0);s->memory.write<8>(doubles+0x90,float_bits<8>(3));return s;};
    for(unsigned active:{0u,1u,128u})for(unsigned side:{0u,1u})for(double space:{-1.0,3.0,4.0,std::numeric_limits<double>::quiet_NaN()})for(unsigned flags:{0u,0x400u}){
        auto s=kind17_clear_seed(side,4,6,4,6,active);s->memory.write<4>(owner+0x81f4,flags);s->memory.write<4>(owner+0x9ad4,flags);Options o;o.measurement=space;compare(*s,o);
    }
    for(bool next:{false,true})for(bool current:{false,true}){Options o;o.next_zone=next;o.current_zone=current;compare(*kind17_clear_seed(),o);}
    for(unsigned side:{0u,1u,2u,255u})for(unsigned target_y:{0u,5u,6u,11u,255u})for(unsigned actual_y:{0u,4u,5u,6u,7u,10u,11u,255u})for(unsigned gate:{0u,1u}){
        auto s=kind17_clear_seed(side,4,target_y,4,actual_y);Options o;o.gates=gate;compare(*s,o);
        o.override_facing=true;compare(*s,o);
    }
    for(bool first:{false,true})for(bool second:{false,true})for(unsigned gates:{0u,1u,2u,3u}){Options o;o.clear_trait=first;o.clear_secondary_trait=second;o.gates=gates;compare(*kind17_clear_seed(),o);}
    for(unsigned side:{0u,1u})for(unsigned target_x:{0u,1u,2u,4u,6u,7u,8u,255u})for(unsigned actual_x:{0u,1u,2u,4u,6u,7u,8u}){
        auto s=kind17_clear_seed(side,target_x,6,actual_x,6);Options o;o.gates=15;compare(*s,o);
    }
    for(unsigned linkage:{0u,neighbour})for(unsigned x:{4u,5u})for(unsigned y:{6u,7u})for(int trait:{-128,0,1,2,127})for(int delay:{-128,0,1,2,127}){
        auto s=kind17_clear_seed();s->memory.write<4>(subject+0x1b4,linkage);s->memory.write<1>(neighbour+0x105,x);s->memory.write<1>(neighbour+0x106,y);
        s->memory.write<1>(neighbour+0x10d,std::uint8_t(trait));s->memory.write<1>(neighbour+0x2c,std::uint8_t(delay));compare(*s);
    }
    for(unsigned kind:{15u,17u,18u,19u,0xffffu})for(bool blocked:{false,true})for(bool gate:{false,true}){
        auto s=kind17_clear_seed();s->memory.write<2>(subject+0x19c,kind);Options o;o.zone_blocked=blocked;o.y_gate=gate;compare(*s,o);
    }
    for(unsigned cursor:{stream+124,stream+126}){auto s=kind17_clear_seed();s->memory.write<4>(subject+0x1b4,neighbour);s->memory.write<1>(neighbour+0x105,4);s->memory.write<1>(neighbour+0x106,6);s->memory.write<1>(neighbour+0x10d,127);s->memory.write<4>(globals+8,cursor);compare(*s);}
    {auto s=kind17_clear_seed();s->memory.write<4>(subject+0x1b4,neighbour);s->memory.write<1>(neighbour+0x105,4);s->memory.write<1>(neighbour+0x106,6);s->memory.write<1>(neighbour+0x10d,127);Options o;o.null_attachment_prefix=true;compare(*s,o);}
    for(unsigned limit:{0u,1u,16u,32u,64u,128u,192u,256u}){auto s=kind17_clear_seed();s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=8;++call){Options o;o.fault_call=call;compare(*kind17_clear_seed(),o);}
    compare(*kind17_clear_seed(),{},true);
    for(unsigned side:{0u,1u})for(unsigned flags:{0u,0x400u})for(unsigned cursor:{stream,stream+126})for(unsigned random:{0u,1u,65535u}){
        auto s=kind17_contact_seed(3,neighbour,1,0,flags);s->pc=cm::aot::player_movement::carrier_kind18_phase;s->memory.write<1>(subject+0x27,side);
        s->memory.write<4>(globals,0);s->memory.write<4>(globals+8,cursor);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);
        Options o;o.role=1;o.clear_trait=true;o.measurement=4;compare(*s,o);
    }
    for(unsigned mode:{3u,4u})for(bool blocked:{false,true})for(bool trait:{false,true})for(double space:{-1.0,3.0,4.0}){
        auto s=kind17_contact_seed(mode,neighbour,1,75,0x400);s->pc=cm::aot::player_movement::carrier_kind18_phase;Options o;o.zone_blocked=blocked;o.clear_trait=trait;o.measurement=space;compare(*s,o);
    }
    const auto simple_seed=[](bool diagnostics=false,unsigned opponent=neighbour){auto s=seed();s->pc=diagnostics?cm::aot::player_movement::carrier_other_kind_phase:cm::aot::player_movement::carrier_kind15_phase;
        s->wr<4>(88,opponent);s->wr<4>(56,fixture+0x45000);s->wr<4>(72,0x8001);
        s->memory.write<4>(toc-0x7bb8,output+0x800);s->memory.write<4>(toc-0x7ba8,output+0x900);s->memory.write<4>(toc-0x7bac,output+0xa00);s->memory.write<1>(output+0xa00,1);return s;};
    for(unsigned opponent:{0u,neighbour})for(unsigned side:{0u,1u,255u}){auto s=simple_seed(false,opponent);s->memory.write<1>(subject+0x27,side);compare(*s);}
    compare(*simple_seed(true));
    for(bool diagnostics:{false,true})for(unsigned limit:{0u,1u,2u,4u,8u,12u,16u,24u,32u}){auto s=simple_seed(diagnostics);s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=5;++call){Options o;o.fault_call=call;compare(*simple_seed(true),o);}
    compare(*simple_seed(),{},true);compare(*simple_seed(true),{},true);
    constexpr auto repeated_direction_table=fixture+0x46000,repeated_warning_table=fixture+0x47000;
    const auto repeated_seed=[](unsigned direction=45,int condition=-11,unsigned random=2,unsigned flag=0){auto s=seed();s->pc=cm::aot::player_movement::carrier_repeated_contact_phase;
        s->wr<4>(88,neighbour);s->wr<4>(84,100);s->memory.write<1>(phase_stack+0x46,direction);s->memory.write<1>(phase_stack+0x40,flag);s->memory.write<2>(phase_stack+0x48,120);
        s->memory.write<2>(neighbour+0x32,std::uint16_t(condition));s->memory.write<4>(globals,0);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);
        s->memory.write<4>(toc-0x51f8,repeated_direction_table);s->memory.write<4>(toc-0x51fc,repeated_warning_table);
        for(unsigned n=0;n<47;++n)s->memory.write<4>(repeated_direction_table+n*4,original_repeated_direction_table[n]);
        for(unsigned n=0;n<11;++n)s->memory.write<4>(repeated_warning_table+n*4,original_repeated_warning_table[n]);return s;};
    {auto s=repeated_seed();s->wr<4>(88,0);compare(*s);}
    {auto s=repeated_seed();s->pc=0x10233900;compare(*s);}
    for(unsigned direction=0;direction<256;++direction){compare(*repeated_seed(direction));Options o;o.contact_forbidden=1;o.zone_blocked=true;compare(*repeated_seed(direction),o);o.repeat_allowed=false;o.contact_forbidden=0;o.zone_blocked=false;compare(*repeated_seed(direction),o);}
    for(int condition:{-32768,-11,-10,-1,0,1,10,32767})for(unsigned random:{0u,1u,2u,20u,65535u})for(unsigned flag:{0u,1u,128u})for(unsigned forbidden:{0u,1u,128u}){
        Options o;o.contact_forbidden=forbidden;o.zone_blocked=true;compare(*repeated_seed(45,condition,random,flag),o);}
    for(unsigned remainder=0;remainder<50;++remainder){auto s=repeated_seed(45,0,0,1);s->memory.write<2>(stream+4,remainder);Options o;o.zone_blocked=true;compare(*s,o);}
    for(unsigned side:{0u,1u,127u,128u,255u}){auto s=repeated_seed(45,0,0,1);s->memory.write<1>(neighbour+0x27,side);Options o;o.zone_blocked=true;compare(*s,o);}
    for(auto target:original_repeated_direction_table){auto s=repeated_seed(0,0,0,1);s->memory.write<4>(repeated_direction_table,target);Options o;o.zone_blocked=true;compare(*s,o);}
    for(auto target:{0x10233d9cu,0x10233dd8u,0x10233e14u,cm::aot::player_movement::carrier_transfer_phase,return_sentinel}){auto s=repeated_seed(45,0,0,1);s->memory.write<4>(repeated_warning_table,target);Options o;o.zone_blocked=true;compare(*s,o);}
    for(auto target:{cm::aot::player_movement::carrier_transfer_phase,return_sentinel}){auto s=repeated_seed(0,0,0,1);s->memory.write<4>(repeated_direction_table,target);Options o;o.zone_blocked=true;compare(*s,o);}
    for(bool negative_limit:{false,true})for(int condition:{-32768,32767})for(int sampled:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+124,stream+126}){
        auto s=repeated_seed(45,condition);s->memory.write<4>(globals+8,cursor);Options o;o.force_wide_repeat=!negative_limit;o.force_negative_repeat_limit=negative_limit;o.sampled=sampled;o.zone_blocked=true;compare(*s,o);}
    for(unsigned cursor:{stream+124,stream+126}){auto s=repeated_seed(45,-11,0,1);s->memory.write<4>(globals+8,cursor);Options o;o.zone_blocked=true;compare(*s,o);}
    {auto s=repeated_seed(45,0,1,1);s->memory.write<4>(globals,0xfffffff0);Options o;o.zone_blocked=true;compare(*s,o);}
    for(unsigned limit:{0u,1u,2u,8u,16u,24u,32u,48u,64u,80u,100u,128u,192u,256u}){auto s=repeated_seed(45,0,0,1);s->budget=limit;Options o;o.zone_blocked=true;compare(*s,o);}
    for(unsigned call=1;call<=5;++call){Options o;o.zone_blocked=true;o.force_wide_repeat=true;o.fault_call=call;compare(*repeated_seed(45,32767),o);}
    for(int slot:{-0x51f8,-0x51fc}){auto s=repeated_seed(45,0,0,1);s->memory.write<4>(toc+slot,0);Options o;o.zone_blocked=true;compare(*s,o);}
    compare(*repeated_seed(),{},true);
    const auto tail_seed=[](std::uint32_t phase=cm::aot::player_movement::carrier_transfer_phase,unsigned direction=21){auto s=seed();s->pc=phase;s->wr<4>(88,neighbour);s->wr<4>(84,100);s->wr<4>(52,1);s->wr<4>(64,1);s->wr<4>(76,1);
        s->memory.write<1>(phase_stack+0x46,direction);s->memory.write<1>(phase_stack+0x45,255);s->memory.write<2>(phase_stack+0x48,120);
        s->memory.write<4>(subject+0x6c,detail);s->memory.write<1>(detail+0x66,0);s->memory.write<1>(neighbour+0x10f,3);s->memory.write<1>(neighbour+0x110,4);
        s->memory.write<2>(subject+0x36,6750);s->memory.write<2>(neighbour+0x36,6751);s->memory.write<1>(neighbour+0x17,0);
        s->memory.write<1>(neighbour+4,1);s->memory.write<1>(neighbour+5,4);s->memory.write<1>(neighbour+6,2);s->memory.write<1>(neighbour+7,1);s->memory.write<1>(neighbour+8,4);s->memory.write<1>(neighbour+9,1);
        s->memory.write<2>(owner+0x7948,1100);s->memory.write<1>(owner+0x791f,4);s->memory.write<4>(globals,0);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,0);
        s->memory.write<8>(doubles+0xf0,float_bits<8>(4503599627370496.0));s->memory.write<8>(doubles+0x288,float_bits<8>(0.5));s->memory.write<8>(doubles+0x1a0,float_bits<8>(0.5));
        s->wr<4>(56,fixture+0x45000);s->memory.write<4>(toc-0x7ba8,output+0x900);s->memory.write<4>(toc-0x7bac,output+0xa00);s->memory.write<1>(output+0xa00,1);return s;};
    for(bool zone:{false,true})for(bool trait:{false,true})for(int condition:{-128,-2,-1,0,1,127})for(unsigned direction:{0u,19u,21u,33u,45u,255u}){
        auto s=tail_seed(cm::aot::player_movement::carrier_transfer_phase,direction);s->memory.write<1>(detail+0x66,std::uint8_t(condition));Options o;o.zone_blocked=zone;o.transfer_trait=trait;compare(*s,o);}
    for(int decision:{-32768,0,100,32767})for(int difficulty:{-32768,-1,0,100,300,1000,32767})for(unsigned ball:{0u,1u,4u,255u})for(bool pending:{false,true}){
        auto s=tail_seed();s->wr<4>(84,std::uint32_t(decision));s->memory.write<2>(phase_stack+0x48,std::uint16_t(difficulty));s->memory.write<1>(owner+0x791f,ball);Options o;o.transfer_pending=pending;compare(*s,o);}
    for(bool lower:{false,true})for(int sample:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+126}){auto s=tail_seed();s->memory.write<4>(globals+8,cursor);Options o;o.zone_blocked=true;o.force_wide_transfer=!lower;o.force_negative_transfer_limit=lower;o.sampled=sample;compare(*s,o);}
    {Options o;o.zone_blocked=true;o.zero_transfer_bound=true;compare(*tail_seed(),o);}
    for(unsigned random:{0u,1u,2u,3u,4u,10u,65535u})for(int x:{-128,-1,0,3,127})for(int y:{-128,-1,0,4,127}){
        auto s=tail_seed(cm::aot::player_movement::carrier_distance_phase);s->memory.write<1>(neighbour+0x10f,std::uint8_t(x));s->memory.write<1>(neighbour+0x110,std::uint8_t(y));for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);compare(*s);}
    for(bool lower:{false,true})for(int sample:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+126}){auto s=tail_seed(cm::aot::player_movement::carrier_distance_phase);s->memory.write<4>(globals+8,cursor);Options o;o.force_wide_distance=!lower;o.force_negative_distance_limit=lower;o.sampled=sample;compare(*s,o);}
    {Options o;o.zero_distance_bound=true;compare(*tail_seed(cm::aot::player_movement::carrier_distance_phase),o);}
    for(int original_sample:{-128,0,1,127}){auto s=tail_seed(0x102346e4);s->wr<4>(52,std::uint32_t(original_sample));compare(*s);}
    for(unsigned direction:{0u,21u,255u})for(unsigned side:{0u,1u,255u})for(int y:{-128,0,3,4,8,9,11,127})for(unsigned ball:{0u,1u,3u,4u,128u,255u}){
        auto s=tail_seed(cm::aot::player_movement::carrier_statistics_phase,direction);s->memory.write<1>(neighbour+0x27,side);s->memory.write<1>(subject+0x106,std::uint8_t(y));s->memory.write<1>(owner+0x791f,ball);compare(*s);}
    for(unsigned direction:{0u,21u})for(unsigned denominator:{0u,1u,2u,255u})for(unsigned numerator:{0u,1u,3u,4u,255u})for(double comparison:{-1.0,0.5,1.0,4.0,std::numeric_limits<double>::quiet_NaN()}){
        auto s=tail_seed(cm::aot::player_movement::carrier_statistics_phase,direction);s->memory.write<1>(neighbour+(direction==21?7:4),denominator);s->memory.write<1>(neighbour+(direction==21?8:5),numerator);s->memory.write<8>(doubles+0x288,float_bits<8>(comparison));compare(*s);}
    for(unsigned direction:{0u,21u})for(bool blocked:{false,true})for(unsigned marker:{0u,1u,128u})for(int condition:{6750,6751,32767})for(int ticks:{-32768,-11,0,600,715,1100,32767}){
        auto s=tail_seed(cm::aot::player_movement::carrier_statistics_phase,direction);s->memory.write<1>(neighbour+0x100,marker);s->memory.write<2>(neighbour+0x36,std::uint16_t(condition));s->memory.write<2>(owner+0x7948,std::uint16_t(ticks));Options o;o.ratio_blocked=blocked;compare(*s,o);}
    for(unsigned direction:{0u,21u})for(double conversion:{4503599627370497.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){auto s=tail_seed(cm::aot::player_movement::carrier_statistics_phase,direction);s->memory.write<8>(doubles+0xf0,float_bits<8>(conversion));compare(*s);}
    for(unsigned role:{0u,1u,2u,4u,6u})for(unsigned finish:{0u,1u,128u})for(unsigned active:{0u,1u,128u})for(bool trait:{false,true})for(bool secondary:{false,true})for(bool outcome:{false,true}){
        auto s=tail_seed(cm::aot::player_movement::carrier_ratings_phase);s->wr<4>(64,finish);s->wr<4>(76,active);Options o;o.role=role;o.transfer_trait=trait;o.clear_trait=secondary;o.outcome_active=outcome;compare(*s,o);}
    for(int factor:{-128,-2,-1,0,1,127})for(bool reacts:{false,true})for(unsigned random:{0u,1u,2u,5u,7u,65535u}){auto s=tail_seed(cm::aot::player_movement::carrier_ratings_phase);s->memory.write<1>(neighbour+0x17,std::uint8_t(factor));for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,random);Options o;o.reaction_allowed=reacts;compare(*s,o);}
    for(bool lower:{false,true})for(int sample:{-2000000,0,2000000})for(unsigned cursor:{stream,stream+126}){auto s=tail_seed(cm::aot::player_movement::carrier_ratings_phase);s->memory.write<4>(globals+8,cursor);Options o;o.force_wide_reward=!lower;o.force_negative_reward_limit=lower;o.sampled=sample;compare(*s,o);}
    {Options o;o.zero_reward_bound=true;compare(*tail_seed(cm::aot::player_movement::carrier_ratings_phase),o);}
    for(double scale:{-10000.0,0.0,0.5,1.0,10000.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){auto s=tail_seed(cm::aot::player_movement::carrier_ratings_phase);s->memory.write<8>(doubles+0x1a0,float_bits<8>(scale));compare(*s);}
    compare(*tail_seed(cm::aot::player_movement::carrier_transfer_phase,44));
    {auto s=tail_seed();s->memory.write<4>(globals+8,stream+124);Options o;o.zone_blocked=true;compare(*s,o);}
    {auto s=tail_seed(cm::aot::player_movement::carrier_statistics_phase,0);s->memory.write<4>(globals+8,stream+126);compare(*s);}
    for(auto phase:{cm::aot::player_movement::carrier_transfer_phase,cm::aot::player_movement::carrier_distance_phase,cm::aot::player_movement::carrier_statistics_phase,cm::aot::player_movement::carrier_ratings_phase,cm::aot::player_movement::carrier_null_contact_phase,cm::aot::player_movement::common_return_phase}){
        compare(*tail_seed(phase),{},true);
        for(unsigned limit:{0u,1u,4u,8u,16u,32u,64u,128u,192u,256u,512u}){auto s=tail_seed(phase);s->budget=limit;compare(*s);}
        for(unsigned call=1;call<=8;++call){Options o;o.fault_call=call;o.zone_blocked=true;o.force_wide_transfer=true;o.force_wide_reward=true;compare(*tail_seed(phase),o);}
    }
    for(unsigned cursor:{stream+122,stream+124,stream+126})for(auto phase:{cm::aot::player_movement::carrier_transfer_phase,cm::aot::player_movement::carrier_distance_phase,cm::aot::player_movement::carrier_statistics_phase,cm::aot::player_movement::carrier_ratings_phase}){auto s=tail_seed(phase);s->memory.write<4>(globals+8,cursor);compare(*s);}
    {auto s=tail_seed();s->memory.write<4>(subject+0x6c,0);Options o;o.zone_blocked=true;compare(*s,o);}
    {auto s=tail_seed(cm::aot::player_movement::carrier_null_contact_phase);s->memory.write<4>(toc-0x7bac,0);compare(*s);}
    {auto s=seed();s->memory.write<1>(subject+0x19,255);compare(*s);}
    {auto s=seed();s->memory.write<4>(subject+0x1a4,0);compare(*s);}
    for(unsigned limit:{0u,1u,16u,64u,128u,256u}){auto s=seed();s->budget=limit;compare(*s);}
    for(unsigned call=1;call<=6;++call){Options o;o.fault_call=call;compare(*base,o);}
    const auto whole_seed=[](unsigned kind=18,bool owns=true,unsigned x=5,unsigned y=6,unsigned active=1){auto s=preparation_seed(x,y,kind,owns);s->wr<4>(28,active);s->wr<4>(32,0);
        s->memory.write<4>(toc-0x7bb8,output+0x800);s->memory.write<4>(toc-0x7ba8,output+0x900);s->memory.write<4>(toc-0x7bac,output+0xa00);
        s->memory.write<4>(toc-0x7b3c,metrics);s->memory.write<4>(metrics,metrics+4);s->memory.write<4>(toc-0x7be4,registry);s->memory.write<4>(registry,100);
        s->memory.write<4>(subject+0x6c,subject_detail);s->memory.write<4>(subject_detail,11);s->memory.write<1>(subject_detail+0x66,0);
        s->memory.write<4>(neighbour+0x6c,other_detail);s->memory.write<4>(other_detail,10);s->memory.write<1>(other_detail+0x66,0);s->memory.write<1>(other_detail+0x67,0);
        s->memory.write<4>(subject+0x70,traits+0x80);s->memory.write<4>(neighbour+0x70,traits);s->memory.write<1>(traits+0x1c,100);s->memory.write<1>(traits+0x25,100);s->memory.write<2>(traits+10,10000);
        s->memory.write<4>(neighbour+0xfc,contract);s->memory.write<4>(contract,123);s->memory.write<4>(owner+0x3114,output+0x2000);
        s->memory.write<4>(owner+0x3118,clubs);s->memory.write<4>(clubs+0x1c,clubs+0x100);s->memory.write<4>(clubs+0x20,clubs+0x200);s->memory.write<4>(clubs+0x100+0x2c,staff);s->memory.write<4>(clubs+0x200+0x2c,staff+0x200);
        s->memory.write<4>(subject+0x98,float_bits<4>(75));s->memory.write<4>(subject+0xe8,float_bits<4>(1));s->memory.write<4>(subject+0x74,float_bits<4>(20));s->memory.write<4>(neighbour+0x74,float_bits<4>(15));s->memory.write<4>(subject+0x98,float_bits<4>(75));
        s->memory.write<4>(toc-0x51f4,action_table);s->memory.write<4>(toc-0x51f8,repeated_direction_table);s->memory.write<4>(toc-0x51fc,repeated_warning_table);
        for(unsigned n=0;n<47;++n){s->memory.write<4>(action_table+n*4,original_contact_action_table[n]);s->memory.write<4>(repeated_direction_table+n*4,original_repeated_direction_table[n]);}
        for(unsigned n=0;n<11;++n)s->memory.write<4>(repeated_warning_table+n*4,original_repeated_warning_table[n]);
        s->memory.write<8>(doubles+0xf0,float_bits<8>(4503599627370496.0));s->memory.write<8>(doubles+0x288,float_bits<8>(0.5));s->memory.write<8>(doubles+0x1a0,float_bits<8>(0.5));return s;};
    for(unsigned kind:{0u,15u,17u,18u,19u,256u,0xffffu})for(bool owns:{false,true})for(unsigned active:{0u,1u,128u})for(unsigned x:{4u,5u,7u}){auto s=whole_seed(kind,owns,x,6,active);Options o;o.measurement=4;compare(*s,o,false,true);}
    for(unsigned kind:{15u,17u,18u})for(unsigned actor:{0u,neighbour})for(bool blocked:{false,true})for(unsigned contact:{0u,1u,128u})for(unsigned forbidden:{0u,1u}){auto s=whole_seed(kind);Options o;o.other_actor=actor;o.zone_blocked=blocked;o.contact_result=contact;o.contact_forbidden=forbidden;o.measurement=4;compare(*s,o,false,true);}
    for(unsigned destination:{return_sentinel,cm::aot::player_movement::common_return_phase,cm::aot::player_movement::entry,0x10239d10u}){
        auto s=whole_seed();s->budget=5000;s->memory.write<4>(action_table,destination);Options o;o.force_action_direction=true;o.zone_blocked=true;o.measurement=4;
        compare(*s,o,false,true);require(std::find(last_path.begin(),last_path.end(),0x1023041c)!=last_path.end(),"Whole-root live table redirection was not exercised");
    }
    for(auto destination:original_repeated_direction_table){auto s=whole_seed();s->budget=5000;s->memory.write<4>(action_table,destination);Options o;o.force_action_direction=true;o.zone_blocked=true;o.measurement=4;compare(*s,o,false,true);}
    for(auto destination:original_repeated_warning_table){auto s=whole_seed();s->budget=5000;s->memory.write<4>(action_table,destination);Options o;o.force_action_direction=true;o.zone_blocked=true;o.measurement=4;compare(*s,o,false,true);}
    for(unsigned cursor:{stream,stream+124,stream+126})for(double control:{-32768.0,0.0,75.0,10000.0,32767.0,std::numeric_limits<double>::quiet_NaN()}){auto s=whole_seed();s->memory.write<4>(globals+8,cursor);s->memory.write<4>(subject+0x50,float_bits<4>(control));compare(*s,{},false,true);}
    for(unsigned limit:{0u,1u,4u,8u,16u,20u,21u,24u,32u,64u,128u,256u,512u,1024u}){auto s=whole_seed();s->budget=limit;compare(*s,{},false,true);}
    for(unsigned call=1;call<=16;++call){Options o;o.fault_call=call;o.zone_blocked=true;o.measurement=4;compare(*whole_seed(),o,false,true);}
    for(unsigned fault=0;fault<3;++fault){auto s=whole_seed();if(fault==0)s->memory.write<4>(subject+0x1a4,0);else if(fault==1)s->memory.write<4>(toc-0x51e8,0);else s->wr<4>(12,0);compare(*s,{},true,true);}
    compare(*whole_seed(),{},true,true);
    if(argc==2){std::ofstream report(argv[1]);report<<"{\"cases\":"<<cases<<",\"normal\":"<<normal<<",\"interrupted\":"<<interrupted<<",\"negatives\":"<<negative_cases<<",\"wholeCases\":"<<whole_cases<<",\"wholeNormal\":"<<whole_normal<<",\"wholeFaults\":"<<whole_faults<<",\"wholeSitesCount\":"<<whole_sites.size()<<",\"sites\":[";bool first=true;for(auto pc:sites){if(!first)report<<',';first=false;report<<pc;}report<<"],\"productionDispatchTested\":false}\n";require(bool(report),"Continuation receipt failed");}
    require(sites.size()==7537,"Movement fragment source sites are not fully exercised: "+std::to_string(sites.size())+"/7537");
    std::cout<<"PASS: movement fragments cases="<<cases<<" normal="<<normal<<" interrupted="<<interrupted<<" negatives="<<negative_cases<<" sites="<<sites.size()<<"/7537; independent original fragments and complete C++ coordinator; productive dispatch checked separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
