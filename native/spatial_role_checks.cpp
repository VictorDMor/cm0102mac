#include "spatial_role.hpp"
#include "semantic_io_checks.hpp"
#include "match_actor.hpp"
#include "application_exit.hpp"
#include <set>

namespace cm::aot { bool spatial_role_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace spatial=cm::aot::spatial_role;
constexpr auto match=fixture+0x1000, first=match+0x311c, table=fixture+0x20000, secondary=table+0x200;
constexpr auto actor=first, center=match+0x2eb0;
struct Options { std::uint32_t reason=0xffffffff, gate=0, relation=1; bool negative=false, child_negative=false; unsigned fault_at=0; std::vector<std::uint32_t> positions; };
struct Call { std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Call&)const=default; };
struct Model final:spatial::DependencyDispatch {
    Options options;bool old;unsigned positions=0;std::vector<Call> calls;
    Model(Options o,bool original):options(o),old(original){}
    void reference(State& s,std::uint32_t end){
        bool first_call=true;
        while(s.pc!=end){
            const bool child=!first_call && std::find(spatial::entries.begin(),spatial::entries.end(),s.pc)!=spatial::entries.end();
            if(child || !spatial_role_reference(s))call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            first_call=false;
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation) override {
        const auto number=calls.size();calls.push_back({target,continuation,s.registers,{}});
        const auto found=std::find(spatial::entries.begin(),spatial::entries.end(),target);
        if(found!=spatial::entries.end()){
            const auto entry=spatial::Entry(found-spatial::entries.begin());
            if(entry==spatial::Entry::current_position && !options.positions.empty()){
                s.memory.write<1>(s.r<4>(12)+0x19,options.positions[std::min(positions++,unsigned(options.positions.size()-1))]);
            }
            if(old)reference(s,continuation);else spatial::invoke(s,entry,this);
            if(!old && options.child_negative && entry==spatial::Entry::current_position)s.wr<4>(12,s.r<4>(12)^1);
        }else if(target==0x101f1870){cm::aot::match_actor::write_position(s);}
        else{
            const auto first=std::uint32_t(s.r<4>(12));
            if(target==0x10283590)s.memory.write<1>(first+0x104,0);
            if(target==0x1076f7d0)throw ApplicationExit{};
            require(target==0x10283540 || target==0x10283590 || target==0x101da5a0 || target==0x10739890 || target==0x10768810 || target==0x1077b720 || target==0x10769b60,
                "Unknown spatial fixture dependency "+hex_address(target));
            for(unsigned n=3;n<=12;++n)s.wr<4>(n*4,0x0bad0000+n*37);
            s.wr<4>(0,0xbeef0055);s.wr<4>(12,target==0x101da5a0?options.gate:target==0x10283540?options.relation:0x12345678);s.pc=continuation;
        }
        calls[number].after=s.registers;
        if(options.fault_at && calls.size()>=options.fault_at){s.pc=target;throw std::runtime_error("Fixture spatial dependency interruption");}
    }
};
std::unique_ptr<State> seed(spatial::Entry entry,const Options& options={}){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    s->memory.map(fixture,0x60000);s->memory.map(stack-0x300,0x400);s->memory.map(toc-0x8000,0x4000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=spatial::entries[unsigned(entry)];
    s->wr<4>(12,actor);s->wr<4>(16,0);s->wr<4>(20,options.reason);
    if(entry==spatial::Entry::register_center_cell || entry==spatial::Entry::remove_from_cell){s->wr<4>(12,match);s->wr<4>(16,actor);}
    if(entry==spatial::Entry::current_role || entry==spatial::Entry::secondary_role || entry==spatial::Entry::previous_role){s->wr<4>(12,match);s->wr<4>(16,actor);}
    if(entry==spatial::Entry::clear_goalkeeper || entry==spatial::Entry::resolve_goalkeeper)s->wr<4>(12,match);
    for(unsigned side=0;side<2;++side){
        s->memory.write<4>(match+0x7934+side*4,table+side*0x80);s->memory.write<4>(match+0x793c+side*4,secondary+side*0x80);
        for(unsigned slot=0;slot<11;++slot){
            const std::uint16_t roles[]{1,64,32,16,4,2,8,128,2048,16,64};
            s->memory.write<2>(table+side*0x80+slot*2,roles[slot]);s->memory.write<2>(secondary+side*0x80+slot*2,roles[slot]^0x8000);
        }
        for(unsigned index=0;index<20;++index){
            const auto p=first+(side*20+index)*0x1c4;s->memory.write<1>(p+0x19,index<11?index:255);s->memory.write<1>(p+0x1a,index<11?index:255);
            s->memory.write<1>(p+0x27,side);s->memory.write<1>(p+0x105,8);s->memory.write<1>(p+0x106,6);s->memory.write<1>(p+0x20,255);
            s->memory.write<4>(p+0x1a4,match);s->memory.write<2>(p+0x1c2,index<11?index:0);
        }
        s->memory.write<4>(match+0xe01c+side*4,first+side*20*0x1c4);
        s->memory.write<4>(center+side*0x2c,first+side*20*0x1c4);s->memory.write<4>(center+side*0x2c+4,first+(side*20+1)*0x1c4);
        s->memory.write<1>(center+0x58+side,2);s->memory.write<1>(first+(side*20+1)*0x1c4+0x28,1);
    }
    s->memory.write<4>(actor+0x1b0,first+2*0x1c4);s->memory.write<1>(actor+0x104,2);
    s->memory.write<4>(actor+0x1b4,first+3*0x1c4);s->memory.write<4>(actor+0x1b8,first+4*0x1c4);
    s->memory.write<4>(first+3*0x1c4+0x1b0,actor);s->memory.write<4>(first+4*0x1c4+0x1b0,actor);
    s->memory.write<4>(toc-0x52cc,fixture+0x200);s->memory.write<4>(toc-0x7ba8,fixture+0x300);s->memory.write<4>(toc-0x7bac,fixture+0x400);
    return s;
}
unsigned comparisons=0,normal=0,interruptions=0,negatives=0;
std::array<unsigned,23> normal_entries{};
std::vector<Effects::Access> compare_observation(const State& input,spatial::Entry entry,const Options& options,bool observed){
    auto original=clone(input),modern=clone(input);Model old(options,true),next(options,false);Effects x,y;x.state=original.get();y.state=modern.get();
    if(observed){original->memory.date_observer=&x;modern->memory.date_observer=&y;}
    std::vector<std::pair<std::uint32_t,unsigned>> old_trace,new_trace;
    if(!observed){original->memory.read_trace=&old_trace;modern->memory.read_trace=&new_trace;}
    auto run=[&](State& s,Model& model,bool previous){try{if(previous)model.reference(s,return_sentinel);else spatial::invoke(s,entry,&model);}
        catch(const ApplicationExit&){return std::string("ApplicationExit");}catch(const std::exception& error){return std::string(error.what());}return std::string{};};
    const auto expected=run(*original,old,true),actual=run(*modern,next,false);original->memory.date_observer=modern->memory.date_observer=nullptr;
    original->memory.read_trace=modern->memory.read_trace=nullptr;
    const auto label="spatial-role component "+std::to_string(comparisons)+" root="+hex_address(spatial::entries[unsigned(entry)])+" observed="+std::to_string(observed);
    require(!expected.starts_with("Unknown spatial fixture"),label+" incomplete model "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.negative)modern->wr<4>(12,modern->r<4>(12)^1);
    bool rejected=false;try{Image(*original).compare(*modern,label);}catch(const std::runtime_error&){if(!options.negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.negative||options.child_negative),label+" observable own/child negative");
    if(!rejected){
        require(x.bytes==y.bytes,label+" ordered effects");
        require(old_trace==new_trace,label+" independent read trace");
        if(old.calls!=next.calls){
            for(unsigned n=0;n<std::min(old.calls.size(),next.calls.size());++n)if(old.calls[n]!=next.calls[n]){
                const auto&a=old.calls[n];const auto&b=next.calls[n];const auto p=std::mismatch(a.before.begin(),a.before.end(),b.before.begin());
                throw std::runtime_error(label+" dependency "+std::to_string(n)+" target="+hex_address(a.target)+"/"+hex_address(b.target)+" continue="+hex_address(a.continuation)+"/"+hex_address(b.continuation)+" register-byte="+std::to_string(p.first-a.before.begin()));
            }throw std::runtime_error(label+" dependency count");
        }
        require(original->executed==modern->executed,label+" accounting "+std::to_string(original->executed)+"/"+std::to_string(modern->executed));
    }
    ++comparisons;negatives+=rejected;interruptions+=!expected.empty();if(expected.empty()&&!rejected){++normal;++normal_entries[unsigned(entry)];}return x.log;
}
std::vector<Effects::Access> compare(const State& input,spatial::Entry entry,const Options& options={}){
    compare_observation(input,entry,options,false);
    return compare_observation(input,entry,options,true);
}
}
int main(){
    try{
        for(unsigned index=0;index<spatial::entries.size();++index){
            const auto entry=spatial::Entry(index);auto input=seed(entry);const auto accesses=compare(*input,entry);require(normal_entries[index]>0,"Entry lacks a normal component path");
            for(unsigned budget=0;budget<=60;++budget){auto limited=clone(*input);limited->budget=budget;compare(*limited,entry);}
            std::set<std::uint32_t> bytes;for(auto access:accesses)for(unsigned n=0;n<access.size;++n)bytes.insert(access.address+n);
            for(auto hole:bytes){auto sparse=std::make_unique<State>();sparse->registers=input->registers;sparse->unique=input->unique;sparse->pc=input->pc;
                for(auto byte:bytes)if(byte!=hole){sparse->memory.map(byte,1);sparse->memory.write<1>(byte,input->memory.read<1>(byte));}compare(*sparse,entry);}
            Options negative;negative.negative=true;compare(*input,entry,negative);
        }
        // Unframed field addresses wrap at 32 bits, including multi-byte values.
        // Compare their successful and partial-mapping fault prefixes against
        // the independent original, with and without callback observation.
        struct Field {unsigned index,offset,width;};
        for(auto field:{Field{0,0x19,1},{1,0x106,1},{2,0x105,1},{3,0x1b0,4},
            {4,0x1b0,4},{5,0x104,1},{6,0x104,1},{10,0x1a,1},{12,0x106,1},
            {13,0x105,1},{18,0x1a,1},{19,0x1c2,2}}){
            for(unsigned mapped=0;mapped<=field.width;++mapped){
                State input;input.registers.fill(0xa5);input.unique.fill(0xb7);
                input.wr<4>(12,0xfffffff0);input.wr<4>(16,0xabcdef80);input.wr<4>(4128,return_sentinel);
                input.pc=spatial::entries[field.index];const auto wrapped=field.offset-0x10;
                if(mapped){input.memory.map(wrapped,mapped);for(unsigned n=0;n<mapped;++n)input.memory.write<1>(wrapped+n,0x80+n);}
                compare(input,spatial::Entry(field.index));
            }
        }
        for(auto entry:{spatial::Entry::current_role,spatial::Entry::secondary_role,spatial::Entry::previous_role}){
            for(auto value:{0u,10u,11u,127u,128u,255u}){auto input=seed(entry);input->memory.write<1>(actor+(entry==spatial::Entry::previous_role?0x1a:0x19),value);compare(*input,entry);}
            for(auto sequence:{std::vector<std::uint32_t>{0,10,255}, {10,0,11}, {0,11}, {255}}){Options o;o.positions=sequence;compare(*seed(entry,o),entry,o);}
        }
        for(auto entry:{spatial::Entry::actor_current_role,spatial::Entry::actor_previous_role,spatial::Entry::actor_secondary_role}){
            auto input=seed(entry);input->memory.write<4>(actor+0x1a4,0);compare(*input,entry);
        }
        for(auto reason:{0u,0xffu,0xfdu,0x100u,0xabcdefedu})for(auto count:{0u,1u,2u,127u,128u,255u}){
            Options o;o.reason=reason;auto input=seed(spatial::Entry::remove_from_cell,o);input->memory.write<1>(actor+0x104,count);compare(*input,spatial::Entry::remove_from_cell,o);
        }
        for(auto gate:{0u,1u})for(auto role:{1u,2u,4u,8u,16u,32u,64u,128u,2048u,0xffffu}){
            Options o;o.reason=0xfdu;o.gate=gate;auto input=seed(spatial::Entry::remove_from_cell,o);input->memory.write<2>(table,role);compare(*input,spatial::Entry::remove_from_cell,o);
        }
        for(auto flags:{0u,1u,2u,4u,16u,32u,64u,128u,2048u}){
            Options o;o.reason=0xfdu;auto input=seed(spatial::Entry::remove_from_cell,o);input->memory.write<2>(table+2,flags);compare(*input,spatial::Entry::remove_from_cell,o);
        }
        for(auto entry:{spatial::Entry::register_center_cell,spatial::Entry::clear_goalkeeper,spatial::Entry::resolve_goalkeeper,spatial::Entry::marking_member})
            for(auto raw:{0u,1u,2u,127u,128u,255u,0xabcdef01u}){auto input=seed(entry);input->wr<4>(16,raw);if(entry==spatial::Entry::register_center_cell){input->wr<4>(16,actor);input->memory.write<1>(actor+0x27,raw);}compare(*input,entry);}
        for(unsigned side=0;side<2;++side)for(auto unavailable:{254u,255u})for(auto cache:{0u,first+side*20*0x1c4}){
            auto input=seed(spatial::Entry::resolve_goalkeeper);input->memory.write<4>(match+0xe01c+side*4,cache);input->memory.write<1>(first+side*20*0x1c4+0x19,unavailable);compare(*input,spatial::Entry::resolve_goalkeeper);
        }
        for(auto entry:{spatial::Entry::remove_from_cell,spatial::Entry::resolve_goalkeeper})for(unsigned fault=1;fault<=20;++fault){Options o;o.fault_at=fault;compare(*seed(entry,o),entry,o);}
        Options child;child.child_negative=true;compare(*seed(spatial::Entry::current_role),spatial::Entry::current_role,child);
        std::cout<<"PASS: spatial-role component comparisons="<<comparisons<<" normal="<<normal<<" interrupted="<<interruptions<<" own/child-negatives="<<negatives<<"; all 23 complete entries, observed and unobserved, full ABI/memory/effects/dependency boundaries/accounting\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
