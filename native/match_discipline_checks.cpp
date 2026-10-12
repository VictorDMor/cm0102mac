#include "match_discipline.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <string_view>

namespace cm::aot { bool match_discipline_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace discipline = cm::aot::match_discipline;
constexpr auto match = fixture+0x1000, context = match+0xe030, actor = fixture+0x20000,
    opponent = fixture+0x20400, outcome = fixture+0x20800, consequence = fixture+0x20810,
    official = fixture+0x20900, constants = fixture+0x20a00, wide_constants = fixture+0x20b00,
    random_words = fixture+0x20c00, seed_slot = fixture+0x20d00, cursor_slot = seed_slot+4,
    limit_slot = seed_slot+8;
struct Options {
    unsigned decision = 1, phase = 0, position = 8, side = 0, yellow = 0, red = 0,
        reason = 1, severity = 6, special = 0, flag = 0, sample = 0, fouls = 0,
        strictness = 20, caution = 0, trait = 0, zone = 1, fault_at = 0;
    std::int16_t clock = 1;
    unsigned refill_after = 100;
    bool null_official = false, negative = false, child_negative = false;
    std::map<std::uint32_t,std::pair<unsigned,std::uint32_t>> registers;
};
void modern(State& state, unsigned index, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    using Function = void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{discipline::match_contact_penalty,discipline::match_single_actor_penalty,
        discipline::match_referee_response,discipline::single_actor_penalty,discipline::contact_penalty,discipline::referee_response};
    methods[index](state,dependencies,observer);
}
struct Seen final : SemanticGuestObserver {
    std::set<std::uint32_t> sites;
    std::vector<std::uint32_t> order;
    const Options* options = nullptr;
    void record(State& s, std::uint32_t pc) {
        sites.insert(pc); order.push_back(pc);
        if (options) if (const auto found=options->registers.find(pc);found!=options->registers.end())
            s.wr<4>(found->second.first*4,found->second.second);
    }
    void before_step(State& s, std::uint32_t pc) override { record(s,pc); }
};
thread_local Seen* original_seen = nullptr;
struct Dependency {
    std::uint32_t target, continuation;
    decltype(State::registers) before, after;
    bool operator==(const Dependency&) const = default;
};
struct Model final : SemanticDependencyDispatch {
    Options options;
    bool original;
    Seen& seen;
    std::vector<Dependency> calls;
    Model(Options o,bool old,Seen& paths):options(o),original(old),seen(paths) {}
    void reference(State& state,std::uint32_t until) {
        bool first = true;
        while (state.pc != until) {
            const auto entry = std::find(discipline::entries.begin(),discipline::entries.end(),state.pc);
            if ((!first && entry != discipline::entries.end()) || !match_discipline_reference(state))
                call(state,state.pc,std::uint32_t(state.r<4>(4128)));
            first = false;
        }
    }
    void call(State& state,std::uint32_t target,std::uint32_t continuation) override {
        const auto number = calls.size(); calls.push_back({target,continuation,state.registers,{}});
        const auto entry = std::find(discipline::entries.begin(),discipline::entries.end(),target);
        if (entry != discipline::entries.end()) {
            if (original) reference(state,continuation);
            else modern(state,unsigned(entry-discipline::entries.begin()),this,&seen);
            if (!original && options.child_negative) state.wr<4>(12,state.r<4>(12)^1);
        } else {
            require(target==0x1077b770 || target==0x101e3d50 || target==0x10283690 || target==0x10239d10 || target==0x1021b2a0,
                "Unknown discipline fixture dependency "+hex_address(target));
            for (unsigned reg=3;reg<=12;++reg) state.wr<4>(reg*4,0x0bad0000+reg*37);
            state.wr<4>(0,0xbeef0055);
            const auto result = target==0x1077b770?0u:target==0x101e3d50?options.trait:
                target==0x10283690?options.side:target==0x10239d10?options.zone:0u;
            state.wr<4>(12,result); state.pc=continuation;
        }
        calls[number].after=state.registers;
        if (options.fault_at && calls.size()>=options.fault_at) {
            state.pc=target; throw std::runtime_error("Fixture discipline dependency interruption");
        }
    }
};
std::unique_ptr<State> seed(unsigned index,const Options& o={}) {
    auto state=std::make_unique<State>(); state->registers.fill(0xa5); state->unique.fill(0xb7);
    for (unsigned byte=1280;byte<1312;++byte) state->wr<1>(byte,0);
    state->memory.map(fixture,0x22000); state->memory.map(stack-0x500,0x1000); state->memory.map(toc-0x8000,0x4000);
    state->wr<4>(4,stack); state->wr<4>(8,toc); state->wr<4>(4128,return_sentinel); state->pc=discipline::entries[index];
    state->memory.write<4>(context,match); state->memory.write<4>(context+4,o.null_official?0:official);
    state->memory.write<1>(match+0x7929,o.phase); state->memory.write<2>(match+0x7948,std::uint16_t(o.clock));
    state->memory.write<1>(official+0x25,o.strictness); state->memory.write<1>(official+0x24,o.caution);
    state->memory.write<1>(actor+0x19,o.position); state->memory.write<1>(actor+0x27,o.side);
    state->memory.write<1>(actor+0x24,o.yellow); state->memory.write<1>(actor+0x23,o.red);
    state->memory.write<1>(actor+0x14,o.fouls); state->memory.write<2>(actor+0x36,1000);
    state->memory.write<1>(opponent+0x15,255); state->memory.write<1>(outcome,o.decision);
    for (unsigned offset=0;offset<=0x20;offset+=4) state->memory.write<4>(constants+offset,float_bits<4>(1.25+offset));
    state->memory.write<8>(wide_constants,float_bits<8>(10.0)); state->memory.write<8>(wide_constants+8,float_bits<8>(4503599627370496.0+2147483648.0));
    state->memory.write<4>(toc-0x5268,constants); state->memory.write<4>(toc-0x5264,wide_constants);
    state->memory.write<4>(toc-0x7b90,seed_slot); state->memory.write<4>(toc-0x7b8c,random_words);
    state->memory.write<4>(toc-0x7b88,limit_slot); state->memory.write<4>(toc-0x7b84,cursor_slot);
    state->memory.write<4>(seed_slot,0); state->memory.write<4>(cursor_slot,random_words);
    state->memory.write<4>(limit_slot,random_words+o.refill_after*2);
    for (unsigned offset=0;offset<0x80;offset+=2) state->memory.write<2>(random_words+offset,o.sample);
    constexpr unsigned methods[]{4,3,5,3,4,5}; const auto method=methods[index];
    state->wr<4>(12,index<3?match:context); state->wr<4>(16,actor);
    if (method==4) {
        state->wr<4>(20,opponent); state->wr<4>(24,outcome); state->wr<4>(28,o.reason);
        state->wr<4>(32,o.severity); state->wr<4>(36,o.special); state->wr<4>(40,consequence);
        state->memory.write<1>(stack+0x3b,o.flag);
    } else if (method==3) {
        state->wr<4>(20,outcome); state->wr<4>(24,o.reason); state->wr<4>(28,o.severity);
        state->wr<4>(32,o.special); state->wr<4>(36,consequence);
    } else { state->wr<4>(20,outcome); state->wr<4>(24,o.reason); state->wr<4>(28,o.flag); }
    return state;
}
unsigned comparisons=0,normal=0,interruptions=0,negatives=0;
std::array<unsigned,6> normal_entries{};
std::set<std::uint32_t> all_sites;
struct Result { std::unique_ptr<State> state; std::vector<Effects::Access> accesses; std::uint64_t instructions; std::set<std::uint32_t> sites; };
Result compare(const State& input,unsigned index,const Options& options={}) {
    auto original=clone(input),next=clone(input); Seen x,y; x.options=y.options=&options; Model before(options,true,x),after(options,false,y);
    Effects first,second; first.state=original.get(); second.state=next.get();
    original->memory.date_observer=&first; next->memory.date_observer=&second;
    auto run=[&](State& state,Model& model,bool old) {
        try { if (old) { original_seen=&x; model.reference(state,return_sentinel); } else modern(state,index,&model,&y); }
        catch (const std::exception& error) { original_seen=nullptr; return std::string(error.what()); }
        original_seen=nullptr; return std::string{};
    };
    const auto expected=run(*original,before,true),actual=run(*next,after,false);
    original->memory.date_observer=next->memory.date_observer=nullptr;
    const auto label="discipline component="+std::to_string(comparisons)+" entry="+hex_address(discipline::entries[index]);
    require(!expected.starts_with("Unknown discipline"),label+" incomplete model");
    require(expected==actual,label+" original="+expected+" modern="+actual);
    if (options.negative) next->wr<4>(12,next->r<4>(12)^1);
    bool rejected=false;
    try { Image(*original).compare(*next,label); }
    catch (const std::runtime_error&) { if (!options.negative && !options.child_negative) throw; rejected=true; }
    require(rejected==(options.negative||options.child_negative),label+" failed negative discriminant");
    if (!rejected) {
        if (x.order!=y.order) {
            auto p=std::mismatch(x.order.begin(),x.order.end(),y.order.begin(),y.order.end());
            std::cerr<<label<<" reason="<<options.reason<<" fouls="<<options.fouls<<" strictness="<<options.strictness<<" first path divergence="<<(p.first-x.order.begin())<<'\n';
            if(p.first!=x.order.end())std::cerr<<"original="<<hex_address(*p.first)<<'\n';
            if(p.second!=y.order.end())std::cerr<<"modern="<<hex_address(*p.second)<<'\n';
        }
        require(first.bytes==second.bytes,label+" ordered access/effect mismatch");
        require(before.calls==after.calls,label+" dependency ABI mismatch");
        require(original->executed==next->executed,label+" instruction mismatch "+std::to_string(original->executed)+"/"+std::to_string(next->executed));
        require(x.sites==y.sites,label+" source-site mismatch");
        require(x.order==y.order,label+" source-step order mismatch");
    }
    ++comparisons; negatives+=rejected; interruptions+=!expected.empty();
    if (expected.empty()&&!rejected) { ++normal; ++normal_entries[index]; }
    all_sites.insert(x.sites.begin(),x.sites.end());
    const auto instructions=original->executed; return {std::move(original),first.log,instructions,x.sites};
}
}
namespace cm::aot { void discipline_step_seen(State& s,std::uint32_t pc) { if (original_seen) original_seen->record(s,pc); } }

int main() {
    try {
        for (unsigned index=0;index<6;++index) {
            Options standard; const auto initial=seed(index,standard); const auto result=compare(*initial,index,standard);
            require(normal_entries[index]>0,"Entry lacks a normal component return");
            Options negative=standard; negative.negative=true; compare(*initial,index,negative);
            if (index<3) { negative=standard; negative.child_negative=true; compare(*initial,index,negative); }
            for (std::uint64_t budget=0;budget<=result.instructions;++budget) {
                auto limited=clone(*initial); limited->budget=budget; compare(*limited,index,standard);
            }
            std::set<std::uint32_t> bytes;
            for (const auto& access:result.accesses) for (unsigned n=0;n<access.size;++n) bytes.insert(access.address+n);
            for (const auto hole:bytes) {
                auto sparse=std::make_unique<State>(); sparse->registers=initial->registers; sparse->unique=initial->unique; sparse->pc=initial->pc;
                for (const auto byte:bytes) if (byte!=hole) { sparse->memory.map(byte,1); sparse->memory.write<1>(byte,initial->memory.read<1>(byte)); }
                compare(*sparse,index,standard);
            }
            for (const auto fault:{1u,2u}) { Options o=standard; o.fault_at=fault; compare(*seed(index,o),index,o); }
        }
        for (const auto index:{3u,4u}) {
            for (const auto severity:{0u,3u,6u,7u,8u,9u,15u,24u,127u,128u,255u})
                for (const auto sample:{0u,5u,14u,49u,50u,375u,376u,475u,476u,2999u,65535u}) {
                    Options o; o.severity=severity; o.sample=sample;
                    for (const auto special:{0u,1u}) { o.special=special; compare(*seed(index,o),index,o); }
                }
            for (const auto position:{0u,255u,253u}) for (const auto yellow:{0u,1u,2u,127u,128u,255u})
                for (const auto clock:{0,495,506,990,32767,-1}) {
                    Options o; o.position=position; o.yellow=yellow; o.clock=std::int16_t(clock); compare(*seed(index,o),index,o);
                }
            for (const auto decision:{0u,7u,255u}) for (const auto phase:{0u,1u,255u}) {
                Options o; o.decision=decision; o.phase=phase; compare(*seed(index,o),index,o);
            }
            for (const auto trait:{0u,1u}) for (const auto zone:{0u,1u}) for (const auto flag:{0u,1u}) {
                Options o; o.trait=trait; o.zone=zone; o.flag=flag; o.severity=127; compare(*seed(index,o),index,o);
            }
        }
        for (const auto kind:{0u,1u,2u,3u,4u,5u,6u,127u,128u,255u})
            for (const auto fouls:{0u,1u,12u,128u,255u}) for (const auto strictness:{0u,2u,3u,6u,7u,12u,13u,18u,19u,127u,128u,255u}) {
                Options o; o.reason=kind; o.fouls=fouls; o.strictness=strictness; compare(*seed(5,o),5,o);
            }
        for (const auto yellow:{0u,1u,2u,127u,128u,255u}) for (const auto caution:{0u,1u,2u,127u,128u,255u})
            for (const auto sample:{0u,1u,14u,65535u}) {
                Options o; o.yellow=yellow; o.caution=caution; o.sample=sample; compare(*seed(5,o),5,o);
            }
        for (const auto index:{3u,4u,5u}) for (const auto refill:{0u,1u,2u}) {
            Options o; o.refill_after=refill; o.fouls=12; compare(*seed(index,o),index,o);
        }
        for (const auto index:{3u,4u}) {
            for (const auto sample:{0u,500u}) {
                Options o; o.special=1; o.yellow=1; o.clock=990; o.sample=sample; o.refill_after=0; compare(*seed(index,o),index,o);
            }
            Options o; o.yellow=1; o.severity=127; compare(*seed(index,o),index,o);
        }
        for(const auto kind:{1u,2u,3u,4u,5u,6u}) for(const auto position:{8u,255u})
            for(const auto red:{0u,1u,128u}) for(const auto flag:{0u,1u}) {
                Options o; o.reason=kind; o.position=position; o.yellow=1; o.red=red; o.flag=flag; o.clock=990; o.refill_after=0;
                compare(*seed(5,o),5,o);
            }
        { Options o; o.yellow=1; o.caution=1; o.refill_after=0; compare(*seed(5,o),5,o); }
        const auto natural_sites=all_sites.size();
        { Options o; o.registers[0x10223610]={3,0xfffffffbu}; compare(*seed(5,o),5,o); }
        // Diagnostic register perturbations cover compiler-emitted wide-count
        // fallbacks that live byte fields cannot reach. These are deliberately
        // separate from natural match and normal field-matrix evidence.
        for (const auto count:{65536u,0xffff0000u,0xfffffffeu}) {
            Options o; o.fouls=1; o.registers[0x10223140]={29,count}; compare(*seed(5,o),5,o);
            o=Options{}; o.yellow=1; o.caution=1; o.registers[0x10223380]={18,count}; compare(*seed(5,o),5,o);
            o=Options{}; o.registers[0x10223650]={18,count}; compare(*seed(5,o),5,o);
        }
        for (const auto index:{3u,4u,5u}) {
            Options o; auto relocated=seed(index,o); const auto relocated_toc=fixture+0x21800;
            relocated->memory.map(relocated_toc-0x8000,0x4000);
            relocated->memory.copy_in(relocated_toc-0x8000,relocated->memory.copy_out(toc-0x8000,0x4000));
            relocated->wr<4>(8,relocated_toc); compare(*relocated,index,o);
            auto alias=seed(index,o); alias->wr<4>(index==4?24:20,actor+0x24); compare(*alias,index,o);
        }
        Options missing; missing.yellow=1; missing.null_official=true; compare(*seed(5,missing),5,missing);
        // Expectations are independent of register equivalence: real yellow,
        // second yellow and direct red, without dismissal being folded into
        // these outcome methods. The caller owns the later dismissal operation.
        for (const auto yellow:{0u,1u}) {
            Options o; o.yellow=yellow; const auto r=compare(*seed(4,o),4,o);
            require(r.state->memory.read<1>(outcome)==2+yellow && r.state->memory.read<1>(actor+0x24)==yellow+1 &&
                r.state->memory.read<1>(actor+0x23)==0 && r.state->memory.read<1>(actor+0x14)==1 &&
                r.state->memory.read<1>(opponent+0x15)==0,"Contact yellow/foul bookkeeping expectation");
        }
        Options red; red.severity=127; const auto r=compare(*seed(4,red),4,red);
        require(r.state->memory.read<1>(outcome)==4 && r.state->memory.read<1>(actor+0x24)==0 &&
            r.state->memory.read<2>(actor+0x36)==0 && r.state->memory.read<1>(actor+0x25)==1,"Direct-red expectation");
        std::cout<<"PASS: discipline comparisons="<<comparisons<<" normal="<<normal<<" interrupted="<<interruptions
            <<" own/child negatives="<<negatives<<" observed source sites="<<all_sites.size()
            <<"; complete ABI/memory/ordered effects/dependencies/accounting, outcomes, typed match facades, thresholds, clocks, referee fields, stream refill, budgets and byte holes\n";
        require(all_sites.size()==1078,"Complete discipline source-site characterization missing");
        std::cout<<"Field/boundary matrix source sites="<<natural_sites<<"; diagnostic register perturbations="<<10
            <<"; combined original sites="<<all_sites.size()<<'\n';
        for(unsigned index=0;index<6;++index) std::cout<<"Component entry="<<hex_address(discipline::entries[index])
            <<" normal="<<normal_entries[index]<<'\n';
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
