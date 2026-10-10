#include "formatted_output_adapter.hpp"
#include "semantic_io_checks.hpp"
using namespace cm::semantic_io_checks;
namespace {
constexpr std::uint32_t format_address=fixture+0x500, arguments=fixture+0x1000,
    descriptor=fixture+0x700, stream=fixture+0x1200, buffer=fixture+0x1400;
void text(State& s,std::uint32_t address,const std::string& value){s.memory.copy_in(address,{value.begin(),value.end()});s.memory.write<1>(address+value.size(),0);}
std::unique_ptr<State> setup(const char* program,unsigned entry,const std::string& format="%d",std::uint64_t value=123){
    auto s=std::make_unique<State>();load_program(*s,program);s->memory.map(fixture,0x8000);s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));s->registers.fill(0x5b);s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000u+n);s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);s->wr<1>(2304,0xa6);s->wr<1>(1024,1);s->wr<1>(1027,1);
    s->wr<1>(1309,0);s->wr<1>(1310,0);s->wr<1>(1311,0);s->wr<8>(16400,value);text(*s,format_address,format);text(*s,left,"abc");s->memory.copy_in(destination,std::vector<std::uint8_t>(512,0xdd));
    s->memory.write<4>(arguments,std::uint32_t(value));s->memory.write<4>(right,arguments);s->memory.write<4>(right+4,64);s->memory.write<4>(right+8,0);s->memory.write<4>(descriptor,0x1077b520);s->memory.write<4>(descriptor+4,toc);
    s->memory.write<4>(stream+4,0x04400000);s->memory.write<1>(stream+8,0x20);s->memory.write<4>(stream+32,buffer);s->memory.write<4>(stream+36,512);s->memory.write<4>(stream+40,buffer+1);s->memory.write<4>(stream+44,511);s->memory.write<4>(toc-0x5a68,stream-0x54);
    switch(entry){
    case 0:s->wr<4>(12,format_address);s->wr<4>(16,right);s->wr<4>(20,destination);break;
    case 1:s->wr<4>(12,value);s->wr<4>(16,destination+600);for(unsigned n=5;n<=10;++n)s->wr<4>(n*4,0);s->wr<4>(24,std::uint32_t('d')<<16);s->wr<4>(32,1);break;
    case 2:s->wr<4>(12,value>>32);s->wr<4>(16,value);s->wr<4>(20,destination+600);for(unsigned n=6;n<=10;++n)s->wr<4>(n*4,0);s->wr<4>(28,std::uint32_t('d')<<16);s->wr<4>(36,1);break;
    case 3:case 6:s->wr<4>(20,destination+600);for(unsigned n=6;n<=10;++n)s->wr<4>(n*4,0);s->wr<4>(28,std::uint32_t(entry==3?'a':'f')<<16);s->wr<4>(36,2);break;
    case 4:case 5:s->wr<4>(12,left);s->wr<4>(16,2);s->memory.write<2>(left+2,3);s->memory.write<1>(left+4,5);text(*s,left+5,"12345");break;
    case 7:s->wr<4>(12,descriptor);s->wr<4>(16,right);s->memory.write<4>(right,destination);s->wr<4>(20,format_address);s->wr<4>(24,arguments);break;
    case 8:s->wr<4>(12,stream);s->wr<4>(16,left);s->wr<4>(20,3);break;
    case 9:s->wr<4>(12,right);s->memory.write<4>(right,destination);s->wr<4>(16,left);s->wr<4>(20,3);break;
    case 10:s->wr<4>(12,format_address);for(unsigned n=4;n<=10;++n)s->wr<4>(n*4,value+(n-4));break;
    case 11:s->wr<4>(12,stream);s->wr<4>(16,format_address);for(unsigned n=5;n<=10;++n)s->wr<4>(n*4,value+(n-5));break;
    case 12:s->wr<4>(12,destination);s->wr<4>(16,64);s->wr<4>(20,format_address);s->wr<4>(24,arguments);break;
    case 13:s->wr<4>(12,destination);s->wr<4>(16,format_address);s->wr<4>(20,arguments);break;
    case 14:s->wr<4>(12,destination);s->wr<4>(16,format_address);for(unsigned n=5;n<=10;++n)s->wr<4>(n*4,value+(n-5));break;
    }
    s->pc=formatted_output_entries[entry];s->budget=100000;return s;
}
unsigned cases = 0, faults = 0;
std::array<std::uint64_t, 15> witnesses{};
using Mutation = std::function<void(State&, Effects&, std::uint32_t, unsigned)>;
Effects compare_pair(State& original, State& modern, const std::string& label, bool fault = false, const Mutation& mutation = {}, bool mixed = false, bool escape=false) {
    if (!mixed) original.formatted_output_original = true;
    Effects expected, actual; expected.state = &original; actual.state = &modern; expected.on_read = actual.on_read = mutation;
    original.memory.date_observer = &expected; modern.memory.date_observer = &actual;
    std::string a, b;
    try { run(original); } catch (const std::exception& e) { a = e.what(); }
    try { run(modern); } catch (const std::exception& e) { b = e.what(); }
    original.memory.date_observer = modern.memory.date_observer = nullptr;
    require(a == b, label + " fault mismatch original=" + a + " semantic=" + b);
    require(fault == !a.empty(), label + " fault expectation: " + a+" PC="+hex_address(original.pc));
    Image(original).compare(modern, label);
    if(expected.bytes!=actual.bytes){unsigned i=0;while(i<expected.log.size()&&i<actual.log.size()){const auto&a=expected.log[i];const auto&b=actual.log[i];if(a.kind!=b.kind||a.address!=b.address||a.size!=b.size||a.value!=b.value)break;++i;}throw std::runtime_error(label+" ordered memory effects event="+std::to_string(i)+" expected="+(i<expected.log.size()?hex_address(expected.log[i].address):"end")+" actual="+(i<actual.log.size()?hex_address(actual.log[i].address):"end"));}
    require(expected.callbacks == actual.callbacks, label + " callbacks");
    require(!original.string_original && !modern.string_original && !original.memory_primitives_original && !modern.memory_primitives_original, "Semantic dependencies");
    ++cases; if (fault) ++faults;
    require(modern.formatted_output_dispatch_escapes==(escape?1u:0u),label+" dispatch escape accounting");
    if(!mixed)require(modern.formatted_output_original_calls==std::array<std::uint64_t,15>{},label+" normal original selection");
    if (!fault && !escape && modern.formatted_output_original_calls == std::array<std::uint64_t, 15>{})
        for (unsigned i = 0; i < 15; ++i) witnesses[i] += modern.formatted_output_calls[i];
    return actual;
}

using Expectation = std::function<void(const State&)>;
void check(std::unique_ptr<State> a, const std::string& label, const Expectation& expected = {}, bool fault = false, const Mutation& mutation = {}) {
    auto b = clone(*a); compare_pair(*a, *b, label, fault, mutation); if (expected) expected(*b);
}
struct Boundaries final : FormattedOutputBoundaryObserver {
    struct Boundary { std::uint32_t entry; Image image; Boundary(std::uint32_t e, const State& s) : entry(e), image(s) {} };
    std::vector<Boundary> expected; unsigned cursor = 0; std::uint32_t perturb = 0; bool mutated = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (original) { expected.emplace_back(entry, s); return; }
        require(cursor < expected.size() && expected[cursor].entry == entry, "Boundary order actual="+hex_address(entry)+" expected="+(cursor<expected.size()?hex_address(expected[cursor].entry):"end"));
        if (entry == perturb) { s.wr<4>(12, s.r<4>(12) ^ 1); mutated = true; }
        expected[cursor++].image.compare(s, "Own output boundary " + hex_address(entry));
    }
};
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX) {
    auto probe = clone(initial); Effects effects; effects.state = probe.get(); probe->formatted_output_original = true;
    probe->memory.date_observer = &effects; run(*probe); probe->memory.date_observer = nullptr;
    auto result = std::make_unique<State>(); result->registers = initial.registers; result->unique = initial.unique; result->pc = initial.pc; result->budget = initial.budget;
    auto bridge_span=[&](std::uint32_t address,unsigned size){if(address<=omitted&&omitted<std::uint64_t(address)+size){if(omitted>address)result->memory.map(address,omitted-address);if(omitted+1<std::uint64_t(address)+size)result->memory.map(std::uint32_t(omitted+1),std::uint64_t(address)+size-omitted-1);}else result->memory.map(address,size);};
    if(initial.pc==formatted_output_entries[3]){bridge_span(stack-256+96,4);bridge_span(stack-256+108,42);}
    if(initial.pc==formatted_output_entries[6]){bridge_span(stack-160+56,4);bridge_span(stack-160+60,42);}
    if(initial.pc==formatted_output_entries[8]||initial.pc==formatted_output_entries[9])bridge_span(left,3);
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
    if(argc!=2){std::cerr<<"Usage: cm-formatted-output-checks <original-segments>\n";return 2;}
    try{
        check(setup(argv[1],12),"independent basic output",[](const State&s){const auto bytes=s.memory.copy_out(destination,4);require(bytes==std::vector<std::uint8_t>{'1','2','3',0}&&s.r<4>(12)==3,"Independent decimal output and calculated length actual="+hex_address(s.memory.read<4>(destination))+" return="+std::to_string(s.r<4>(12))+"\n"+formatted_output_call_summary(s));});
        for(unsigned entry=0;entry<15;++entry)check(setup(argv[1],entry,entry==3?"%a":entry==6?"%.2f":"%d",entry==3||entry==6?float_bits<8>(7.25):123),"basic entry "+std::to_string(entry));
        for(unsigned entry:{1u,2u})for(auto value:{UINT64_C(0),UINT64_C(1),UINT64_C(0xffffffff),UINT64_C(0x80000000),UINT64_MAX,UINT64_C(0x8000000000000000)})for(char code:{'d','u','o','x','X'}){
            auto a=setup(argv[1],entry,"",value);a->wr<4>(entry==1?24:28,std::uint32_t(code)<<16);check(std::move(a),"integer boundary "+std::to_string(entry)+"/"+code+"/"+std::to_string(value));}
        for(unsigned entry:{3u,6u})for(double value:{0.,-0.,1.5,2.5,99.999,1e-8,1e20,std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})for(unsigned precision:{0u,2u,6u}){
            auto a=setup(argv[1],entry,"",float_bits<8>(value));a->wr<4>(36,precision);check(std::move(a),"floating boundary "+std::to_string(entry)+"/"+std::to_string(value)+"/"+std::to_string(precision));}
        for(const std::string& f:{"%%","%d","%08.2f","%*.*d","%lld","%hhd","%#x","%lc","%ls","%n","%,vd","%@vd","%v@d","%","%Q","%509d","%510d","%g","%a"}){auto a=setup(argv[1],0,f);if(f.find('@')!=std::string::npos)a->memory.write<4>(arguments,left);check(std::move(a),"parse "+f);}
        for(unsigned count:{0u,1u,2u,3u,4u,5u,UINT32_MAX}){auto a=setup(argv[1],4);a->wr<4>(16,count);check(std::move(a),"decimal rounding");auto b=setup(argv[1],5);b->wr<4>(16,count);check(std::move(b),"decimal length");}
        for(unsigned entry:{7u,12u,13u,14u})for(const std::string& f:{"literal","%%","%+08d","%#x","%u","%hhd","%lld","%n","%c","%Q","%"}){auto a=setup(argv[1],entry,f);if(f=="%n"){a->memory.write<4>(arguments,destination+48);if(entry==14)a->wr<4>(20,destination+48);}check(std::move(a),"composition "+std::to_string(entry)+"/"+f);}
        struct DecimalExample{std::string format,expected;double value;};
        for(const auto& e:std::vector<DecimalExample>{{"%.2f","7.25",7.25},{"%.0f","2",1.5},{"%.0f","2",2.5},{"%+.1f","+12.4",12.375},{"%08.2f","00001.25",1.25},{"%.2e","1.23e+03",1234.},{"%.4g","1.235e+06",1234567.},{"%.2f","100.00",99.999},{"%.2f","-0.00",-0.},{"%a","0x1.d000000000000p+2",7.25}})for(unsigned entry:{12u,13u,14u}){
            auto a=setup(argv[1],entry,e.format);const auto bits=float_bits<8>(e.value);a->memory.write<8>(arguments,bits);if(entry==14){a->wr<4>(20,bits>>32);a->wr<4>(24,bits);}check(std::move(a),"independent floating output "+e.format,[=](const State&s){require(s.memory.copy_out(destination,e.expected.size())==std::vector<std::uint8_t>(e.expected.begin(),e.expected.end())&&s.memory.read<1>(destination+e.expected.size())==0&&s.r<4>(12)==e.expected.size(),"Independent floating text/count "+e.format);});
        }
        for(unsigned capacity:{0u,1u,3u,4u,5u,64u}){auto a=setup(argv[1],12,"%.2f");a->memory.write<8>(arguments,float_bits<8>(7.25));a->wr<4>(16,capacity);check(std::move(a),"truncated floating buffer",[=](const State&s){require(s.r<4>(12)==4,"Calculated truncation length");const unsigned length=capacity?std::min(capacity-1,4u):0;const std::string prefix=std::string("7.25").substr(0,length);if(capacity)require(s.memory.copy_out(destination,length)==std::vector<std::uint8_t>(prefix.begin(),prefix.end()),"Independent truncated text");require(s.memory.read<1>(capacity?destination+length:destination-1)==0,"Original NUL position including zero capacity");});}
        for(unsigned entry:{1u,2u})for(const auto& fields:std::vector<std::array<unsigned,3>>{{0,0,0},{0x02010001,8,3},{0x00020001,8,0},{0x01010001,0,4},{0,509,509},{0,510,510}}){auto a=setup(argv[1],entry,"",42);a->wr<4>(entry==1?20:24,fields[0]);a->wr<4>(entry==1?28:32,fields[1]);a->wr<4>(entry==1?32:36,fields[2]);check(std::move(a),"Integer signs/alternate/padding/limit "+std::to_string(entry));}
        for(const std::string& digits:{"1250","1350","1251","9999","0000"})for(unsigned count:{0u,2u,3u}){auto a=setup(argv[1],4);a->memory.write<1>(left+4,digits.size());text(*a,left+5,digits);a->wr<4>(16,count);check(std::move(a),"Decimal tie/carry "+digits+"/"+std::to_string(count));}
        for(unsigned entry:{12u,13u})for(const std::string& f:{"%.2s","%ls","%lc","%s"}){auto a=setup(argv[1],entry,f);a->memory.write<4>(arguments,left);if(f=="%ls"){a->memory.write<2>(left,'A');a->memory.write<2>(left+2,0x80);a->memory.write<2>(left+4,0x800);a->memory.write<2>(left+6,0xffff);a->memory.write<2>(left+8,0);}check(std::move(a),"String/character output composition "+f,[=](const State&s){if(f=="%.2s")require(s.memory.copy_out(destination,3)==std::vector<std::uint8_t>{'a','b',0},"Bounded string prefix");if(f=="%ls")require(s.memory.copy_out(destination,10)==std::vector<std::uint8_t>{'A',0xc2,0x80,0xe0,0xa0,0x80,0xef,0xbf,0xbf,0},"Independent wide16 output chain");});}
        for(unsigned entry:{10u,11u}){auto a=setup(argv[1],entry,"[%d]",42);check(std::move(a),"Buffered artificial stream",[](const State&s){require(s.r<4>(12)==4&&s.memory.copy_out(buffer+1,4)==std::vector<std::uint8_t>{'[','4','2',']'}&&s.memory.read<4>(stream+40)==buffer+5&&s.memory.read<1>(stream+13)==0,"Independent buffered stream bytes/count/cursor/no-error");});for(unsigned orientation:{0x20u,0x30u}){auto b=setup(argv[1],entry);b->memory.write<1>(stream+5,0x40|orientation);check(std::move(b),"Refused stream orientation",[](const State&s){require(s.r<4>(12)==UINT32_MAX,"Refused orientation result");});}}
        for(unsigned count:{0u,1u,4u}){auto a=setup(argv[1],8);a->wr<4>(20,count);a->memory.write<1>(stream+13,1);check(std::move(a),"Stream helper error result",[=](const State&s){require(s.r<4>(12)==(count?0u:stream),"Callback exact count contract including zero");});}
        for(bool failure:{false,true}){auto a=setup(argv[1],7,"[%05d]",42);a->memory.write<4>(descriptor,0x10779ec0);check(std::move(a),"Generic callback chunks/clobbers/failure",[=](const State&s){require(s.r<4>(12)==(failure?UINT32_MAX:7),"Generic callback calculated/error result");},false,[=](State&s,Effects&e,std::uint32_t address,unsigned){if(address==descriptor&&s.pc==0x10781320){s.wr<4>(24,0x24681357);s.wr<8>(16416,UINT64_C(0x4004000000000000));if(failure&&!e.mutated){e.mutated=true;s.wr<4>(12,0);}}});}
        {auto a=setup(argv[1],7,"[%05d]",42);a->memory.write<4>(descriptor,0x10779ec0);check(std::move(a),"Live descriptor entry",{},false,[](State&s,Effects&e,std::uint32_t address,unsigned){if(address==descriptor&&!e.mutated){e.mutated=true;s.memory.write<4>(descriptor,0x1077b520);}});}
        for(unsigned entry:{1u,2u}){auto a=setup(argv[1],entry,"",31);const auto table=std::uint32_t(a->memory.read<4>(toc-(entry==1?0x3b50:0x3b54)));a->memory.write<4>(table+('d'-'X')*4,entry==1?0x107796e4:0x10779980);check(std::move(a),"Live integer radix destination",[](const State&s){require(s.memory.copy_out(std::uint32_t(s.r<4>(12)),3)==std::vector<std::uint8_t>{'1','F',0},"Live radix independent text");});}
        {auto a=setup(argv[1],0,"%d");const auto table=std::uint32_t(a->memory.read<4>(toc-0x3b4c));a->memory.write<4>(table+('d'-'A')*4,0x1077947c);check(std::move(a),"Live conversion destination",[](const State&s){require(s.memory.read<1>(destination+5)=='x'&&s.memory.read<4>(destination+12)==8,"Live pointer conversion record");});}
        for(unsigned entry:{0u,1u,2u}){auto a=setup(argv[1],entry);constexpr auto new_toc=fixture+0x10000;const auto ranges=a->memory.mapped_ranges();for(const auto&[address,size]:ranges){const auto lo=std::max(std::uint64_t(address),std::uint64_t(toc-0x8000));const auto hi=std::min(std::uint64_t(address)+size,std::uint64_t(toc+0x1000));if(lo<hi){const auto target=std::uint32_t(new_toc+lo-toc);a->memory.map(target,hi-lo);a->memory.copy_in(target,a->memory.copy_out(std::uint32_t(lo),hi-lo));}}a->wr<4>(8,new_toc);check(std::move(a),"Relocated output TOC");}
        for(unsigned entry:{0u,9u,12u})for(unsigned offset:{0u,56u,64u}){auto a=setup(argv[1],entry);if(entry==0)a->wr<4>(20,stack-144+offset);else if(entry==9){a->wr<4>(16,right+offset);a->wr<4>(20,4);}else a->wr<4>(12,stack-80+offset);check(std::move(a),"Output representative frame/context alias");}
        for(const std::string& f:{"%,lvx","%,vf","%@lvd"}){auto a=setup(argv[1],12,f);const bool auxiliary=f.find('@')!=std::string::npos;const auto vector=arguments+(auxiliary?16:0);if(auxiliary){text(*a,left,", ");a->memory.write<4>(arguments,left);}for(unsigned i=0;i<4;++i)a->memory.write<4>(vector+i*4,f=="%,vf"?float_bits<4>(double(i+1)):i+1);check(std::move(a),"Packed output "+f,[=](const State&s){const std::string expected=f=="%,vf"?"1.000000,2.000000,3.000000,4.000000":auxiliary?"1, 2, 3, 4":"1,2,3,4";require(s.r<4>(12)==expected.size()&&s.memory.copy_out(destination,expected.size())==std::vector<std::uint8_t>(expected.begin(),expected.end()),"Independent packed lanes/delimiter/alignment");});}
        {auto a=setup(argv[1],0,"%12d");const auto table=std::uint32_t(a->memory.read<4>(toc-0x5a48));a->memory.write<1>(table+'1',a->memory.read<1>(table+'1')&~16);check(std::move(a),"Live width classification");}
        {auto a=setup(argv[1],12,"%*.*d",0);a->memory.write<4>(arguments,std::uint32_t(-6));a->memory.write<4>(arguments+4,std::uint32_t(-2));a->memory.write<4>(arguments+8,42);check(std::move(a),"Negative dynamic fields",[](const State&s){require(s.memory.copy_out(destination,7)==std::vector<std::uint8_t>{'4','2',' ',' ',' ',' ',0}&&s.r<4>(12)==6,"Independent negative width/precision");});}
        for(unsigned entry:{0u,1u,2u}){auto a=setup(argv[1],entry);const auto table=std::uint32_t(a->memory.read<4>(toc-(entry==0?0x3b4c:entry==1?0x3b50:0x3b54)));a->memory.write<4>(table+('d'-(entry==0?'A':'X'))*4,0x10779ec0);auto b=clone(*a);compare_pair(*a,*b,"Unknown output destination boundary",false,{},false,true);}
        for(unsigned entry=0;entry<15;++entry){
            auto a=setup(argv[1],entry);check(sparse(*a),"minimal mapping "+std::to_string(entry));
            auto probe=clone(*a);Effects effects;effects.state=probe.get();probe->formatted_output_original=true;probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
            std::vector<std::uint32_t> omitted;for(const auto&e:effects.log){const auto byte=e.address+std::uint32_t(e.size)-1;if(std::find(omitted.begin(),omitted.end(),byte)==omitted.end())omitted.push_back(byte);}
            for(auto byte:omitted)check(sparse(*a,byte),"output fault "+std::to_string(entry)+"/"+hex_address(byte),{},true);
            Boundaries boundaries;auto original=clone(*a);original->formatted_output_original=true;original->formatted_output_observer=&boundaries;run(*original);
            auto modern=clone(*a);modern->formatted_output_observer=&boundaries;run(*modern);require(boundaries.cursor==boundaries.expected.size(),"All boundaries");
            auto negative=clone(*a);boundaries.cursor=0;boundaries.perturb=formatted_output_entries[entry];negative->formatted_output_observer=&boundaries;bool rejected=false;
            try{run(*negative);}catch(const std::exception&e){rejected=std::string(e.what()).starts_with("Own output boundary "+hex_address(boundaries.perturb));}require(rejected&&boundaries.mutated,"Own output negative");
            negative->formatted_output_original=true;negative->formatted_output_entry_original.fill(true);negative->formatted_output_dispatch_escapes=1;negative->formatted_output_shared_tail_calls=1;negative->formatted_output_original_resume=formatted_output_entries[entry];negative->string_calls[0]=77;reset_formatted_output_diagnostics(*negative);
            require(!negative->formatted_output_original&&negative->formatted_output_calls==std::array<std::uint64_t,15>{}&&negative->formatted_output_original_calls==std::array<std::uint64_t,15>{}&&negative->formatted_output_entry_original==std::array<bool,15>{}&&!negative->formatted_output_dispatch_escapes&&!negative->formatted_output_shared_tail_calls&&!negative->formatted_output_original_resume&&!negative->formatted_output_observer&&negative->string_calls[0]==77,"Independent diagnostic reset");
            auto indirect=clone(*a);indirect->pc=0x10780900;indirect->memory.write<4>(fixture+0x710,formatted_output_entries[entry]);indirect->memory.write<4>(fixture+0x714,toc);indirect->memory.write<4>(toc-0x7ee4,fixture+0x710);check(std::move(indirect),"Generated output indirect transition "+std::to_string(entry));
        }
        {
            auto a=setup(argv[1],5);a->wr<4>(16,0);auto b=clone(*a);b->formatted_output_entry_original[5]=true;compare_pair(*a,*b,"Generated mixed shared reset",false,{},true);require(b->formatted_output_shared_tail_calls==1,"Generated shared reset hook reached");
            auto indirect=setup(argv[1],5);indirect->wr<4>(16,0);indirect->pc=0x10780900;indirect->memory.write<4>(fixture+0x710,0x10779ed8);indirect->memory.write<4>(fixture+0x714,toc);indirect->memory.write<4>(toc-0x7ee4,fixture+0x710);check(std::move(indirect),"Generated indirect shared reset");
            Boundaries boundaries;auto original=setup(argv[1],12);original->formatted_output_original=true;original->formatted_output_observer=&boundaries;run(*original);auto child=setup(argv[1],12);child->formatted_output_observer=&boundaries;boundaries.perturb=formatted_output_entries[1];bool rejected=false;
            try{run(*child);}catch(const std::exception&e){rejected=std::string(e.what()).starts_with("Own output boundary "+hex_address(boundaries.perturb));}require(rejected&&boundaries.cursor<boundaries.expected.size(),"Output child rejected before parent");
        }
        for(const auto& choice:std::vector<std::pair<unsigned,unsigned>>{{12,7},{7,0},{7,1},{14,12},{13,12},{11,8},{3,1},{6,4}})for(bool inverse:{false,true}){
            auto a=setup(argv[1],choice.first);auto b=clone(*a);b->formatted_output_entry_original[choice.first]=inverse;b->formatted_output_entry_original[choice.second]=!inverse;compare_pair(*a,*b,"Mixed output chain",false,{},true);
        }
        for(unsigned i=0;i<15;++i){require(witnesses[i]!=0,"Nonempty normal witness "+std::to_string(i));std::cout<<"Normal witness "<<hex_address(formatted_output_entries[i])<<" calls="<<witnesses[i]<<" original=0 escapes=0\n";}
        std::cout<<"PASS: output full architecture/memory/ordered effects cases="<<cases<<" faults="<<faults<<" own negatives=15 child negative=1 indirect transitions=15 shared reset transitions=2 mixed modes=16 unknown escape diagnostics=3 excluded from normal witnesses\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
