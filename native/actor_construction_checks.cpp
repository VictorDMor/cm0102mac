#include "actor_construction_random.hpp"
#include "semantic_io_checks.hpp"
#include <map>
#include <set>

namespace cm::aot { bool actor_construction_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace actor = cm::aot::actor_construction;
constexpr auto match = fixture + 0x1000, object = fixture + 0x11000, staff = fixture + 0x12000;
constexpr auto player = fixture + 0x13000, game = fixture + 0x14000, home = fixture + 0x15000, away = fixture + 0x16000;
constexpr auto nation = fixture + 0x17000, competition = fixture + 0x18000, sidecar = fixture + 0x19000;
constexpr auto service = fixture + 0x1a000, history = fixture + 0x1b000, coach = fixture + 0x1c000;
constexpr auto cursor = fixture + 0x1d000, pool_end = cursor + 4, epoch = cursor + 8, pool = cursor + 0x100, doubles = cursor + 0x400;
struct Options {
    std::uint32_t index=8, slot=8, side=0, staff_id=0, staff_count=10;
    std::uint32_t match_gate=0, availability=0, role=8, fitness=100, lookup=sidecar;
    std::uint32_t favorites=0, rivals=0, national=0, special=0, attribute_gate=1, eligibility=1, context=0;
    std::uint32_t favorite_club=0, relationship=20, history_result=history, history_status=1, staff_disabled=0;
    std::uint32_t seed=17, rng=65535;
    double result_f1=1.5, power_f1=1.0;
    bool original_player=true, names=true, report_exit=false, current_date=true, negative=false, child_negative=false;
    unsigned fault_at=0;
};
struct Call {
    std::uint32_t target, continuation;
    decltype(State::registers) before, after;
    bool operator==(const Call&) const = default;
};
std::set<std::uint32_t> observed, accepted_addresses;
struct Model final : actor::DependencyDispatch {
    Options o; bool original;
    std::vector<Call> calls;
    Model(Options input,bool old):o(input),original(old){}
    void reference(State& s,std::uint32_t end) {
        bool first=true;
        while(s.pc!=end) {
            if((!first && s.pc==actor::entries[1]) || !actor_construction_reference(s)) call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            first=false;
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation) override {
        const auto number=calls.size();calls.push_back({target,continuation,s.registers,{}});
        if(target==actor::entries[1]) {
            if(original) reference(s,continuation);else actor::initialize_actor(s,this);
            if(!original && o.child_negative) s.memory.write<1>(object+0x19,s.memory.read<1>(object+0x19)^1);
        } else {
            const auto fourth=std::uint32_t(s.r<4>(16)), sixth=std::uint32_t(s.r<4>(24));
            std::uint32_t result=0;
            switch(target) {
            case 0x10082f90:result=o.lookup;break;
            case 0x101878d0:result=o.fitness;break;
            case 0x10187680:result=o.availability;break;
            case 0x103d0990:result=0;break;
            case 0x10289560:result=o.role;break;
            case 0x100d9480:result=o.favorites;break;
            case 0x100d94d0:result=o.rivals;break;
            case 0x100ce250:result=o.national;break;
            case 0x100c7d00:result=o.special;break;
            case 0x104646d0:result=o.attribute_gate;break;
            case 0x10306a80:s.wr<8>(0x4010,float_bits<8>(o.result_f1));break;
            case 0x10305ae0:result=11;break;
            case 0x10305700:result=o.eligibility;break;
            case 0x103058f0:result=15;break;
            case 0x100d9440:result=o.current_date;break;
            case 0x10303560:s.wr<8>(0x4010,float_bits<8>(sixth==1?o.result_f1:7.0));break;
            case 0x1014c2e0:result=o.context;break;
            case 0x101fce40:result=o.match_gate;break;
            case 0x10452ed0:result=o.eligibility;break;
            case 0x10780fc0:s.wr<8>(0x4010,float_bits<8>(o.power_f1));s.memory.write<4>(s.r<4>(4)+0x14,s.r<4>(8));break;
            case 0x100d9580:result=o.favorite_club;break;
            case 0x100d2f50:result=o.relationship;break;
            case 0x10459b50:result=o.history_result;break;
            case 0x102de3c0:result=o.history_status;break;
            case 0x100d3150:result=o.staff_disabled;break;
            case 0x100c7a50:s.memory.write<2>(s.r<4>(20),0x1234);s.memory.write<2>(s.r<4>(24),0x4321);break;
            case 0x10226520:result=17;break;
            case 0x1077b770:result=0xabcdfff0;break;
            case 0x1077b720:case 0x10739890:case 0x10768810:case 0x10769b60:case 0x1076f7d0:result=0x12345678;break;
            default:throw std::runtime_error("Unknown fixture dependency "+hex_address(target));
            }
            (void)fourth;
            for(unsigned n=3;n<=12;++n)s.wr<4>(n*4,0x0bad0000+n*37);
            s.wr<4>(0,0xbeef0055);s.wr<4>(12,result);s.pc=continuation;
        }
        calls[number].after=s.registers;
        if(o.fault_at && calls.size()>=o.fault_at){s.pc=target;throw std::runtime_error("Fixture actor-construction dependency interruption");}
    }
};
std::unique_ptr<State> seed(const Options& o,unsigned entry=1) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned offset=1280;offset<1312;++offset)s->wr<1>(offset,0);
    s->memory.map(fixture,0x30000);s->memory.map(stack-0x300,0x400);s->memory.map(toc-0x8000,0x4000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=actor::entries[entry];
    unsigned reg=3;
    if(entry) for(auto value:{object,match,staff,o.side,o.index,o.slot})s->wr<4>(reg++*4,value);
    else for(auto value:{match+0x7aac,o.index,staff,o.slot})s->wr<4>(reg++*4,value);
    s->memory.write<4>(match+0x7aac+0x28,object-o.index*0x1c4);s->memory.write<4>(match+0x7aac+0x14,match);s->memory.write<1>(match+0x7aac+0x1a,o.side);
    s->memory.write<4>(staff,o.staff_id);s->memory.write<4>(staff+0x14,o.original_player?player:0);s->memory.write<4>(staff+0x24,o.current_date?fixture+0x200:0);
    for(auto source:{0x28u,0x2cu,0x30u}){s->memory.write<4>(staff+source,o.names?fixture+0x200+source*4:0);s->memory.write<4>(fixture+0x220+source*4,fixture+0x500+source);}
    for(auto field:{0x65u,0x66u,0x67u,0x69u})s->memory.write<1>(staff+field,20);
    s->memory.write<1>(staff+0x5e,0);s->memory.write<1>(staff+0x5f,7);
    s->memory.write<4>(match+0x3118,game);s->memory.write<1>(match+0x7931,1);s->memory.write<4>(match+0x7c5c,20000);
    s->memory.write<4>(game,0);s->memory.write<4>(game+0x14,competition);s->memory.write<4>(game+0x18,fixture+0x600);
    s->memory.write<4>(fixture+0x604,40000);s->memory.write<4>(game+0x1c,home);s->memory.write<4>(game+0x20,away);
    s->memory.write<1>(game+0x40,1);s->memory.write<1>(game+0x41,1);s->memory.write<1>(object+0x27,o.side);
    for(auto club:{home,away}){s->memory.write<4>(club+0x2c,nation);s->memory.write<4>(club+0x30,competition);s->memory.write<4>(club+0x90,fixture+0x700);s->memory.write<2>(club+0x1ec,5751);}
    s->memory.write<1>(nation+0x120,1);s->memory.write<4>(nation+0x68,5);s->memory.write<2>(competition+0x18,15);
    s->memory.write<1>(sidecar+0x60,1);s->memory.write<1>(sidecar+0x4c,30);s->memory.write<1>(sidecar+0x44,30);
    s->memory.write<4>(service,service+0x100);s->memory.write<4>(service+0x100,service+0x200);s->memory.write<4>(service+0x208,history);
    s->memory.write<4>(history+0x1c,7);s->memory.write<4>(history+0x44,11);
    for(unsigned side=0;side<2;++side){s->memory.write<4>(match+0x7aac+side*0xd8+0x20,coach);s->memory.write<4>(coach+0x1c,coach+0x100);s->memory.write<1>(coach+0x13b,10);}
    for(auto source:{-0x7b3c,-0x7b80,-0x7b5c,-0x7ba0,-0x7be4,-0x7b7c,-0x7a7c,-0x7a24,-0x7a50,-0x7bbc,-0x7b28,-0x7984,-0x74d4,-0x7b38})s->memory.write<4>(toc+std::uint32_t(source),service);
    s->memory.write<4>(toc-0x7be4,fixture+0x800);s->memory.write<4>(fixture+0x800,o.staff_count);
    s->memory.write<4>(toc-0x7b80,fixture+0x900);s->memory.write<2>(fixture+0x902,2001);
    s->memory.write<4>(toc-0x74d4,fixture+0xa00);s->memory.write<4>(fixture+0xa00,1);
    s->memory.write<4>(toc-0x7b38,fixture+0xb00);s->memory.write<4>(fixture+0xb00,player);
    s->memory.write<4>(toc-0x7a24,fixture+0xc00);s->memory.write<4>(fixture+0xc00,6);
    s->memory.write<4>(toc-0x51e8,doubles);s->memory.write<4>(toc-0x51e0,fixture+0xd00);s->memory.write<4>(fixture+0xd00,float_bits<4>(100.0));s->memory.write<4>(fixture+0xd04,float_bits<4>(1.0));
    s->memory.write<4>(toc-0x7b90,epoch);s->memory.write<4>(toc-0x7b8c,pool);s->memory.write<4>(toc-0x7b88,pool_end);s->memory.write<4>(toc-0x7b84,cursor);
    s->memory.write<4>(cursor,pool);s->memory.write<4>(pool_end,pool+0x3e);s->memory.write<4>(epoch,o.seed);
    for(unsigned n=0;n<32;++n)s->memory.write<2>(pool+n*2,o.rng+n*53);
    const double values[]{65535,0.0,1.0,0.8,1.0,1.2,1.4,100,20,2.0,30.0,15.0,1.5,0.6,2.5,200};
    for(unsigned n=0;n<16;++n)s->memory.write<8>(doubles+n*8,float_bits<8>(values[n]));s->memory.write<8>(doubles+0x80,float_bits<8>(4503601774854144.0));
    s->memory.write<2>(player+4,180);s->memory.write<2>(player+8,180);s->memory.write<2>(player+10,180);s->memory.write<2>(player+12,180);
    for(auto field:{0x21u,0x25u,0x2cu,0x30u,0x3bu,0x45u})s->memory.write<1>(player+field,15);
    s->memory.write<1>(player+0xf,10);
    s->memory.write<4>(toc-0x7ba8,fixture+0xe00);s->memory.write<4>(toc-0x5274,fixture+0xf00);s->memory.write<4>(toc-0x7bb8,fixture+0xe00);s->memory.write<4>(toc-0x7bac,fixture+0xe80);
    return s;
}
unsigned comparisons=0,interruptions=0,negatives=0,normal=0;
std::vector<Effects::Access> compare(const State& input,const Options& o,unsigned entry=1) {
    auto original=clone(input),modern=clone(input);Model old(o,true),next(o,false);Effects x,y;x.state=original.get();y.state=modern.get();
    original->memory.date_observer=&x;modern->memory.date_observer=&y;observed.clear();
    auto run=[&](State& s,Model& model,bool previous){try{if(previous)model.reference(s,return_sentinel);else if(entry)actor::initialize_actor(s,&model);else actor::prepare_actor(s,&model);}
        catch(const std::exception& error){return std::string(error.what());}return std::string{};};
    const auto expected=run(*original,old,true),actual=run(*modern,next,false);original->memory.date_observer=modern->memory.date_observer=nullptr;
    const auto label="construction component "+std::to_string(comparisons)+" entry="+hex_address(actor::entries[entry]);
    require(!expected.starts_with("Unknown fixture dependency"),label+" incomplete dependency model: "+expected);
    require(expected==actual,label+" exception "+expected+"/"+actual);
    if(o.negative)modern->wr<4>(12,modern->r<4>(12)^1);
    bool rejected=false;try{Image(*original).compare(*modern,label);}catch(const std::runtime_error&){if(!o.negative&&!o.child_negative)throw;rejected=true;}
    require(rejected==(o.negative||o.child_negative),label+" own/child negative observability own="+std::to_string(o.negative)+" child="+std::to_string(o.child_negative)+" exception="+expected+" calls="+std::to_string(next.calls.size()));
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");
        if(old.calls!=next.calls){for(unsigned n=0;n<std::min(old.calls.size(),next.calls.size());++n)if(old.calls[n]!=next.calls[n]){const auto&a=old.calls[n];const auto&b=next.calls[n];const auto p=std::mismatch(a.before.begin(),a.before.end(),b.before.begin());throw std::runtime_error(label+" dependency "+std::to_string(n)+" "+hex_address(a.target)+"/"+hex_address(b.target)+" continue="+hex_address(a.continuation)+"/"+hex_address(b.continuation)+" register-byte="+std::to_string(p.first-a.before.begin()));}throw std::runtime_error(label+" dependency count");}
        require(original->executed==modern->executed,label+" instruction accounting "+std::to_string(original->executed)+"/"+std::to_string(modern->executed));}
    if(expected.empty()&&!rejected){++normal;accepted_addresses.insert(observed.begin(),observed.end());}
    ++comparisons;interruptions+=!expected.empty();negatives+=rejected;return x.log;
}
}
namespace cm::aot { void actor_construction_reference_step_seen(State&,std::uint32_t pc){observed.insert(pc);} }
int main(){
    try{
        Options standard;auto base=seed(standard);const auto accesses=compare(*base,standard);compare(*seed(standard,0),standard,0);
        require(normal==2,"Baseline construction/preparation did not return normally");
        for(auto side:{0u,1u,255u}){auto o=standard;o.side=side;compare(*seed(o),o);}
        for(auto slot:{0u,10u,11u,127u,128u,255u,0x12345608u}){auto o=standard;o.slot=slot;compare(*seed(o),o);}
        for(auto& [name,value]:std::map<std::string,std::uint32_t>{{"fitness",0xffff8000},{"availability",2},{"role",1},{"lookup",0},{"favorites",1},{"rivals",1},{"national",1},{"special",1},{"attribute",0},{"eligibility",0},{"context",1},{"clubfavorite",1},{"relationship",10},{"history",0},{"status",0},{"disabled",1},{"staffid",20},{"matchgate",1}}){
            auto o=standard;
            if(name=="fitness")o.fitness=value;else if(name=="availability")o.availability=value;else if(name=="role")o.role=value;else if(name=="lookup")o.lookup=value;else if(name=="favorites")o.favorites=value;else if(name=="rivals")o.rivals=value;else if(name=="national")o.national=value;else if(name=="special")o.special=value;else if(name=="attribute")o.attribute_gate=value;else if(name=="eligibility")o.eligibility=value;else if(name=="context")o.context=value;else if(name=="clubfavorite")o.favorite_club=value;else if(name=="relationship")o.relationship=value;else if(name=="history")o.history_result=value;else if(name=="status")o.history_status=value;else if(name=="disabled")o.staff_disabled=value;else if(name=="staffid")o.staff_id=value;else if(name=="matchgate")o.match_gate=value;
            compare(*seed(o),o);
        }
        for(bool present:{false,true}){auto o=standard;o.original_player=present;o.names=present;o.current_date=present;compare(*seed(o),o);}
        for(auto condition:{0u,1u,2u,3u,4u,15u}){auto input=seed(standard);input->memory.write<1>(sidecar+0x60,condition);compare(*input,standard);}
        for(auto pressure:{0u,5u,10u,14u,15u,20u,30u,128u,255u}){auto input=seed(standard);for(auto f:{0x65u,0x66u,0x67u})input->memory.write<1>(staff+f,pressure);compare(*input,standard);}
        for(auto reputation:{0u,1u,7u,0x80000000u,0xffffffffu})for(auto random:{0u,127u,0xffffu}){auto o=standard;o.seed=reputation;o.rng=random;compare(*seed(o),o);}
        for(double value:{0.0,0.9,1.0,2.0,100.0,std::numeric_limits<double>::quiet_NaN()}){auto o=standard;o.result_f1=value;o.power_f1=value;compare(*seed(o),o);}
        for(auto pair:{std::pair{0x7931u,0u},{0x14000u+0x40u-0x1000u,0u},{0x14000u+0x41u-0x1000u,0u},{0x14000u+0x3au-0x1000u,1u}}){auto input=seed(standard);input->memory.write<1>(match+pair.first,pair.second);compare(*input,standard);}
        for(auto reputation:{5750u,5751u})for(auto role_attribute:{10u,20u})for(auto attribute:{0u,15u,35u,128u,255u}) {
            auto input=seed(standard);input->memory.write<2>(home+0x1ec,reputation);input->memory.write<1>(player+0xf,role_attribute);
            input->memory.write<1>(player+0x21,attribute);compare(*input,standard);
        }
        for(auto standing:{2u,4u,6u})for(auto points:{4u,6u,8u,10u}) {
            auto input=seed(standard);input->memory.write<4>(history+0x1c,standing);input->memory.write<4>(history+0x44,points);compare(*input,standard);
        }
        for(auto field:{0x74u,0x78u,0x7cu}){auto input=seed(standard);input->memory.write<4>(away+field,home);input->memory.write<4>(home+field,away);compare(*input,standard);}
        auto missing_descriptor=seed(standard);missing_descriptor->memory.write<4>(service+0x100,0);
        missing_descriptor->memory.write<4>(toc-0x7a50,fixture+0xd80);missing_descriptor->memory.write<4>(fixture+0xd80,0);
        missing_descriptor->memory.write<4>(toc-0x7bbc,fixture+0xda0);missing_descriptor->memory.write<4>(fixture+0xda0,competition);compare(*missing_descriptor,standard);
        auto date_gate=seed(standard);date_gate->memory.write<4>(nation+0x68,6);compare(*date_gate,standard);
        date_gate->memory.write<2>(fixture+0x902,2002);compare(*date_gate,standard);
        auto no_coach=seed(standard);no_coach->memory.write<4>(match+0x7aac+0x20,0);compare(*no_coach,standard);
        no_coach->memory.write<4>(match+0x7aac+0x20,coach);no_coach->memory.write<4>(coach+0x1c,0);compare(*no_coach,standard);
        auto refill=seed(standard);refill->memory.write<4>(cursor,pool+0x3e);compare(*refill,standard);
        for(auto control:{1287u,1294u,1308u}){auto input=seed(standard);input->wr<1>(control,1);compare(*input,standard);}
        // The original special-club path tests a wrapped controller address
        // against zero. A valid split mapping reaches that rare legacy branch.
        auto wrapped=seed(standard);constexpr auto wrapped_match=0xffff8554u;
        const auto match_bytes=wrapped->memory.copy_out(match,0xe0bc);
        wrapped->memory.map(wrapped_match,0x7aac);wrapped->memory.map(0,0x6610);
        wrapped->memory.copy_in(wrapped_match,{match_bytes.begin(),match_bytes.begin()+0x7aac});
        wrapped->memory.copy_in(0,{match_bytes.begin()+0x7aac,match_bytes.end()});wrapped->wr<4>(16,wrapped_match);
        wrapped->memory.write<1>(nation+0x120,0);wrapped->memory.write<4>(0x20,0);compare(*wrapped,standard);
        for(unsigned budget=0;budget<=1000;++budget){auto input=seed(standard);input->budget=budget;compare(*input,standard);}
        for(unsigned fault=1;fault<=30;++fault){auto o=standard;o.fault_at=fault;compare(*seed(o),o);}
        std::set<std::uint32_t> bytes;for(auto access:accesses)for(unsigned n=0;n<access.size;++n)bytes.insert(access.address+n);
        for(auto hole:bytes){auto sparse=std::make_unique<State>();sparse->registers=base->registers;sparse->unique=base->unique;sparse->pc=base->pc;
            for(auto byte:bytes)if(byte!=hole){sparse->memory.map(byte,1);sparse->memory.write<1>(byte,base->memory.read<1>(byte));}compare(*sparse,standard);}
        for(unsigned entry:{0u,1u}){auto o=standard;o.negative=true;compare(*seed(o,entry),o,entry);}
        auto child=standard;child.child_negative=true;compare(*seed(child,0),child,0);
        require(normal>=60,"Complete component corpus lacks successful normal execution");
        std::cout<<"PASS: complete actor construction/preparation component comparisons="<<comparisons<<" normal="<<normal<<" interrupted="<<interruptions<<" own/child-negatives="<<negatives<<" original-addresses-in-normal-cases="<<accepted_addresses.size()<<"; full ABI/memory/ordered effects/dependency boundaries/accounting\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
