#include "floating_input_adapter.hpp"
#include "formatted_input_adapter.hpp"
#include "semantic_io_checks.hpp"
using namespace cm::semantic_io_checks;
namespace {
std::unique_ptr<State> setup(const char* program, const std::string& input = "-1.5!", unsigned width = 100) {
    auto s=std::make_unique<State>();load_program(*s,program);s->memory.map(fixture,0x2000);
    s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));s->registers.fill(0x5b);s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000u+n);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);
    s->wr<1>(2304,0xa6);s->wr<1>(1024,1);s->wr<1>(1027,1);s->wr<1>(1309,0);s->wr<1>(1310,0);s->wr<1>(1311,0);
    s->memory.copy_in(left,{input.begin(),input.end()});s->memory.write<1>(left+input.size(),0);
    s->memory.write<4>(right,left);s->memory.write<4>(right+4,0);
    s->memory.write<4>(fixture+0x700,0x1077d960);s->memory.write<4>(fixture+0x704,toc);
    s->wr<4>(12,width);s->wr<4>(16,fixture+0x700);s->wr<4>(20,right);s->wr<4>(24,destination);s->wr<4>(28,destination+4);
    s->pc=0x1077de10;s->budget=20000;return s;
}
unsigned cases = 0, faults = 0;
std::array<std::uint64_t, 1> witnesses{};
using Mutation = std::function<void(State&, Effects&, std::uint32_t, unsigned)>;
Effects compare_pair(State& original, State& modern, const std::string& label, bool fault = false, const Mutation& mutation = {}, bool mixed = false) {
    if (!mixed) original.floating_input_original = true;
    Effects expected, actual; expected.state = &original; actual.state = &modern; expected.on_read = actual.on_read = mutation;
    original.memory.date_observer = &expected; modern.memory.date_observer = &actual;
    std::string a, b;
    try { run(original); } catch (const std::exception& e) { a = e.what(); }
    try { run(modern); } catch (const std::exception& e) { b = e.what(); }
    original.memory.date_observer = modern.memory.date_observer = nullptr;
    require(a == b, label + " fault mismatch original=" + a + " semantic=" + b);
    require(fault == !a.empty(), label + " fault expectation: " + a);
    Image(original).compare(modern, label);
    require(expected.bytes == actual.bytes, label + " ordered memory effects");
    require(expected.callbacks == actual.callbacks, label + " callbacks");
    require(!original.string_original && !modern.string_original && !original.memory_primitives_original && !modern.memory_primitives_original, "Semantic dependencies");
    ++cases; if (fault) ++faults;
    if (!fault && modern.floating_input_original_calls == std::array<std::uint64_t, 1>{})
        for (unsigned i = 0; i < 1; ++i) witnesses[i] += modern.floating_input_calls[i];
    return actual;
}
using Expectation = std::function<void(const State&)>;
void check(std::unique_ptr<State> a, const std::string& label, const Expectation& expected = {}, bool fault = false, const Mutation& mutation = {}) {
    auto b = clone(*a); compare_pair(*a, *b, label, fault, mutation); if (expected) expected(*b);
}
struct Boundaries final : FloatingInputBoundaryObserver {
    struct Boundary { std::uint32_t entry; Image image; Boundary(std::uint32_t e, const State& s) : entry(e), image(s) {} };
    std::vector<Boundary> expected; unsigned cursor = 0; std::uint32_t perturb = 0; bool mutated = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (original) { expected.emplace_back(entry, s); return; }
        require(cursor < expected.size() && expected[cursor].entry == entry, "Boundary order");
        if (entry == perturb) { s.wr<8>(16400, s.r<8>(16400) ^ 1); mutated = true; }
        expected[cursor++].image.compare(s, "Own floating boundary " + hex_address(entry));
    }
};
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX) {
    auto probe = clone(initial); Effects effects; effects.state = probe.get(); probe->floating_input_original = true;
    probe->memory.date_observer = &effects; run(*probe); probe->memory.date_observer = nullptr;
    auto result = std::make_unique<State>(); result->registers = initial.registers; result->unique = initial.unique; result->pc = initial.pc; result->budget = initial.budget;
    for (const auto& [address, size] : effects.accesses) {
        if (address <= omitted && omitted < std::uint64_t(address) + size) {
            if (omitted > address) result->memory.map(address, omitted - address);
            if (omitted + 1 < std::uint64_t(address) + size) result->memory.map(std::uint32_t(omitted + 1), std::uint64_t(address) + size - omitted - 1);
        } else result->memory.map(address, size);
    }
    for (const auto& [address, size] : result->memory.mapped_ranges()) result->memory.copy_in(address, initial.memory.copy_out(address, size));
    return result;
}
}
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: cm-floating-input-checks <original-segments>\n";return 2;}
 try{
  for(const auto& input:std::vector<std::string>{"1.5!","-1.5!","+1.5!","  1.5!","0!","-0!",".5!","1.!","0012.3!","1e2!","1e-2!","1e+2!","1e!","1e+!",".!","-!","!","","inf!","infinity!","INF!","-inf!","infi!","nan!","nan(32)!","-nan(255)!","nan(x)!","nan(999999999999999999999999)!","nan(!","123456789012345678901234567!","1e99999!","1e-99999!","1e309!","1e-309!","0x0!","0x1p1!","0x1.8p1!"}){
   check(setup(argv[1],input),"Floating token "+input,[=](const State& s){
    if(input=="1.5!"||input=="-1.5!"||input=="+1.5!"||input=="  1.5!"){
     require(s.r<8>(16400)==(input[0]=='-'?UINT64_C(0xbff8000000000000):UINT64_C(0x3ff8000000000000)),"Independent decimal bits");
     require(s.memory.read<4>(destination)==input.size()-1&&s.memory.read<4>(destination+4)==0,"Independent consumed/flag");
    }
    if(input=="nan(32)!")require(s.r<8>(16400)==UINT64_C(0x7ff8040000000000),"Independent PPC NaN payload");
    if(input=="0!"||input=="-0!")require(s.r<8>(16400)==(input[0]=='-'?UINT64_C(0x8000000000000000):0),"Independent signed zero");
   });
  }
  for(unsigned width:{0u,1u,2u,3u,4u,5u})check(setup(argv[1],"12.5!",width),"Field width");
  for(unsigned mode=0;mode<4;++mode){auto a=setup(argv[1],"0.1!");a->wr<1>(1310,mode>>1);a->wr<1>(1311,mode&1);check(std::move(a),"Guest rounding");}
  for(unsigned off:{0u,4u,56u,64u,116u,120u,168u,172u,176u,296u,308u}){
   auto a=setup(argv[1]);a->wr<4>(24,stack-272+off);check(std::move(a),"Count/frame alias");
  }
  auto initial=setup(argv[1]);check(sparse(*initial),"Minimal mapping");
  auto probe=clone(*initial);Effects effects;effects.state=probe.get();probe->floating_input_original=true;probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
  std::vector<std::uint32_t> omitted;for(const auto& e:effects.log){const auto byte=e.address+std::uint32_t(e.size)-1;if(std::find(omitted.begin(),omitted.end(),byte)==omitted.end())omitted.push_back(byte);}
  for(auto byte:omitted)check(sparse(*initial,byte),"Floating fault "+hex_address(byte),{},true);
  Boundaries b;auto a=setup(argv[1]);a->floating_input_original=true;a->floating_input_observer=&b;run(*a);auto m=setup(argv[1]);m->floating_input_observer=&b;run(*m);require(b.cursor==b.expected.size(),"Boundary visited");
  auto n=setup(argv[1]);n->floating_input_observer=&b;b.cursor=0;b.perturb=0x1077de10;bool rejected=false;
  try{run(*n);}catch(const std::exception& e){rejected=std::string(e.what()).starts_with("Own floating boundary 0x1077de10");}require(rejected&&b.mutated,"Own floating negative");
  n->floating_input_original=true;n->floating_input_dispatch_escapes=1;n->floating_input_original_resume=0x1077de10;n->string_calls[0]=77;reset_floating_input_diagnostics(*n);require(!n->floating_input_original&&!n->floating_input_dispatch_escapes&&!n->floating_input_original_resume&&!n->floating_input_observer&&n->string_calls[0]==77,"Independent reset");
  auto indirect=setup(argv[1]);indirect->pc=0x10780900;indirect->memory.write<4>(fixture+0x710,0x1077de10);indirect->memory.write<4>(fixture+0x714,toc);indirect->memory.write<4>(toc-0x7ee4,fixture+0x710);check(std::move(indirect),"Generated indirect transition");
  check(setup(argv[1],"1.5!"),"Mutable callback",{},false,[](State& s,Effects& e,std::uint32_t address,unsigned){if(!e.mutated&&address==right){e.mutated=true;s.wr<4>(44,0x13572468);}});
  {
   auto live=setup(argv[1],"~1.5!");const auto classification=std::uint32_t(live->memory.read<4>(toc-0x5a48));live->memory.write<1>(classification+'~',live->memory.read<1>(classification+'~')|2);check(std::move(live),"Live whitespace classification",[](const State&s){require(s.r<8>(16400)==UINT64_C(0x3ff8000000000000),"Live classification decimal bits");});
   auto point=setup(argv[1],"1,5!");const auto slot=std::uint32_t(point->memory.read<4>(toc-0x5a2c));const auto decimal_point=std::uint32_t(point->memory.read<4>(slot));point->memory.write<1>(decimal_point,',');check(std::move(point),"Live decimal separator",[](const State&s){require(s.r<8>(16400)==UINT64_C(0x3ff8000000000000),"Live separator decimal bits");});
   auto record=setup(argv[1]);const auto template_address=std::uint32_t(record->memory.read<4>(toc-0x3b2c));record->memory.write<1>(template_address+41,0x3e);check(std::move(record),"Live decimal template padding");
   auto relocated=setup(argv[1]);constexpr auto new_toc=fixture+0x10000;const auto ranges=relocated->memory.mapped_ranges();for(const auto&[address,size]:ranges){const auto lo=std::max(std::uint64_t(address),std::uint64_t(toc-0x8000));const auto hi=std::min(std::uint64_t(address)+size,std::uint64_t(toc+0x1000));if(lo<hi){const auto target=std::uint32_t(new_toc+lo-toc);relocated->memory.map(target,hi-lo);relocated->memory.copy_in(target,relocated->memory.copy_out(std::uint32_t(lo),hi-lo));}}relocated->wr<4>(8,new_toc);relocated->memory.write<4>(fixture+0x704,new_toc);check(std::move(relocated),"Relocated floating TOC");
  }
  struct CallbackBoundary final:FormattedInputBoundaryObserver{
   std::vector<Image> expected;unsigned cursor=0;bool perturb=false,mutated=false;
   void enter(const State&,std::uint32_t,bool)override{}
   void leave(State&s,std::uint32_t entry,bool original)override{if(entry!=0x1077d960)return;if(original){expected.emplace_back(s);return;}require(cursor<expected.size(),"Floating callback boundary order");if(perturb){s.wr<4>(12,s.r<4>(12)^1);mutated=true;}expected[cursor++].compare(s,"Floating callback child boundary");}
  } child;
  for(bool inverse:{false,true}){auto a=setup(argv[1]);auto m=clone(*a);m->floating_input_original=inverse;m->formatted_input_entry_original[2]=!inverse;compare_pair(*a,*m,"Mixed floating/callback",false,{},true);}
  {
   auto original=setup(argv[1]);original->floating_input_original=true;original->formatted_input_entry_original[2]=true;original->formatted_input_observer=&child;run(*original);
   auto normal=setup(argv[1]);normal->formatted_input_observer=&child;run(*normal);require(child.cursor==child.expected.size(),"Floating callback child boundaries");
   child.cursor=0;child.perturb=true;auto negative=setup(argv[1]);Boundaries parent;negative->formatted_input_observer=&child;negative->floating_input_observer=&parent;bool rejected=false;try{run(*negative);}catch(const std::exception&e){rejected=std::string(e.what()).starts_with("Floating callback child boundary");}require(rejected&&child.mutated&&parent.cursor==0,"Floating child rejected before parent");
  }
  // A known legacy hex exponent phase cannot terminate. Bound each attempt;
  // report the original and semantic limits separately, never as equivalence.
  for(bool original:{true,false}){
   auto h=setup(argv[1],"0x1p0!");h->floating_input_original=original;h->budget=1500;bool limited=false;
   try{run(*h);}catch(const std::exception& e){limited=std::string(e.what()).find("budget")!=std::string::npos;}
   require(limited,"Known hexadecimal zero-exponent limit");std::cout<<"Legacy hex diagnostic mode="<<(original?"original":"semantic")<<" bounded, excluded from PASS counts\n";
  }
  require(witnesses[0]!=0,"Nonempty floating normal witness");std::cout<<"Normal witness 0x1077de10 calls="<<witnesses[0]<<" original=0 escapes=0\n";
  std::cout<<"PASS: floating full architecture/memory/ordered effects cases="<<cases<<" faults="<<faults<<" own negative=1 child negative=1 indirect transition=1\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
