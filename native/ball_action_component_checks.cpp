#include "ball_action.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include "execution-message-table.hpp"

namespace cm::aot { bool ball_action_preparation_reference(State&);bool ball_action_orientation_reference(State&);bool ball_action_facing_reference(State&);bool ball_action_area_reference(State&);bool ball_action_elevated_reference(State&);bool ball_action_low_reference(State&);bool ball_action_selected_reference(State&);bool ball_action_contact_reference(State&);bool ball_action_contact_resolution_reference(State&);bool ball_action_contact_outcome_reference(State&);bool ball_action_execution_reference(State&);bool ball_action_execution_message_reference(State&);bool ball_action_receiver_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace action=cm::aot::ball_action;
constexpr auto match=fixture+0x1000,actor=fixture+0x11000,doubles=fixture+0x12000;
constexpr auto singles=fixture+0x12400,stream=fixture+0x12800,globals=fixture+0x12a00;
constexpr auto recipient=actor+0x600,teams=fixture+0x800,home=fixture+0x900,away=fixture+0xa00,staff=fixture+0xb00;
constexpr auto execution_table=fixture+0x12d00;
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
    bool mutate_execution=false,mutate_table=false;
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
        if(phase==21) {
            for(;;) {
                if(s.pc>=action::execution_message_phase&&s.pc<action::receiver_resolution_phase)
                    require(ball_action_execution_message_reference(s),"Missing original ball-action execution message");
                else if(s.pc==0x10201180&&s.r<4>(4128)>action::execution_message_phase&&s.r<4>(4128)<=action::receiver_resolution_phase)
                    call(s,s.pc,std::uint32_t(s.r<4>(4128)));
                else return;
            }
        }
        const auto done=phase>=23?action::receiver_outcome_phase:phase>=21?action::receiver_resolution_phase:phase>=19?action::execution_message_phase:phase>=17?action::return_phase:phase>=15?action::contact_outcome_phase:phase>=13?action::contact_resolution_phase:phase>=11?action::contact_selection_phase:phase>=9?action::selected_action_phase:phase>=7?action::low_ball_selection_phase:phase>=5?action::ball_state_selection_phase:phase>=3?action::area_selection_phase:phase?action::action_bias_phase:action::orientation_phase;
        while(s.pc!=done&&!(phase>=5&&phase<=10&&s.pc==action::selected_action_phase)&&!(phase>=13&&phase<=18&&s.pc==action::action_execution_phase)
              &&!(phase>=19&&(s.pc==action::return_phase||s.pc==action::execution_result_phase||s.pc==action::no_receiver_phase))) {
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
            result=options.random;
            if(options.mutate_contact){s.memory.write<1>(std::uint32_t(s.r<4>(4))+0x41,0);s.memory.write<4>(singles+0x3c,float_bits<4>(-2));}
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
            s.memory.write<2>(std::uint32_t(s.r<4>(12))+0x19e,((s.r<4>(24)&255)<<8)|(s.r<4>(28)&255));
            break;
        case 0x101e3d50:
            result=options.area_status;if(options.child_negative&&!original)result^=1;
            if(options.mutate_area){s.memory.write<1>(actor+0x105,0);s.memory.write<1>(actor+0x27,1);}
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
        case 0x1022ba10:s.memory.write<4>(std::uint32_t(s.r<4>(12))+0x54,(0x12340000|s.r<4>(16))^unsigned(options.child_negative&&!original&&phase==23));break;
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
            s.memory.write<1>(std::uint32_t(s.r<4>(40)),options.execution_blocked);result=options.execution_contact;break;
        case 0x101f5110:
            s.memory.write<1>(std::uint32_t(s.r<4>(24)),options.resolved_contact);
            s.memory.write<4>(std::uint32_t(s.r<4>(40)),float_bits<4>(12.5));
            if(options.mutate_execution){s.memory.write<1>(std::uint32_t(s.r<4>(4))+0x48,1);s.memory.write<4>(toc-0x5204,execution_table+0x200);}
            break;
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
std::array<unsigned,25> phase_cases{},phase_completed{};
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
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_trace(options),new_trace(options);
    Model old(true,options,phase),modern(false,options,phase);
    const auto run=[&](State& s,Model& m,bool original){try{if(original){original_trace=&old_trace;m.reference(s);}else {
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
        require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");
        require(old.calls==modern.calls,label+" dependency ABI");
        if(old_trace.order!=new_trace.order)for(unsigned n=0;n<std::min(old_trace.order.size(),new_trace.order.size());++n)if(old_trace.order[n]!=new_trace.order[n]){
            std::cerr<<"First site "<<hex_address(old_trace.order[n])<<" / "<<hex_address(new_trace.order[n])<<'\n';break;}
        require(old_trace.order==new_trace.order,label+" original-site order");
    }
    ++cases;++phase_cases[phase];phase_completed[phase]+=expected.empty()&&!rejected;
    negatives+=rejected;interruptions+=!expected.empty();structural+=options.force_wide!=0||options.facing_interval!=200||options.zero_elevated_threshold||options.adjustment_wide!=0;
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
    for(auto pc=action::entry;pc<action::receiver_outcome_phase;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    require(sites.size()==3643,"All original ball-action preparation through receiver-resolution sites covered");
    for(unsigned p=0;p<phase_cases.size();++p){require(phase_completed[p]>0,"Each ball-action phase completed");
        std::cout<<"Phase "<<p<<": "<<phase_cases[p]<<" comparisons, "<<phase_completed[p]<<" completions\n";}
    std::cout<<"PASS: "<<cases<<" ball-action component comparisons; "<<completed<<" phase completions, "<<interruptions
        <<" interruptions, "<<negatives<<" own/child negatives; "<<structural<<" dormant-branch structural cases; all "<<sites.size()
        <<" original sites; complete state/effects/ABI/accounting; production unchanged\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
