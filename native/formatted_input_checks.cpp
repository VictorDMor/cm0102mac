#include "formatted_input_adapter.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture=0x30000000, stack=0x700fff00, toc=0x108bba90;
constexpr std::uint32_t left=fixture, right=fixture+0x100, destination=fixture+0x200;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> bytes;
    struct Access { unsigned kind; std::uint32_t address; std::uint64_t size, value; };
    std::vector<Access> log;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> accesses;
    State* state = nullptr;
    std::function<void(State&, Effects&, std::uint32_t, unsigned)> on_read;
    bool mutated = false;
    std::vector<std::array<std::uint32_t,5>> callbacks;
    void number(std::uint64_t value, unsigned width) {
        for (unsigned n = width; n; --n) bytes.push_back(std::uint8_t(value >> ((n - 1) * 8)));
    }
    void event(unsigned kind, std::uint32_t address, std::uint64_t size, const std::uint8_t* data = nullptr) {
        number(kind, 1); number(address, 4); number(size, 8); accesses.emplace_back(address, size);
        std::uint64_t value = 0;
        if (data) {
            bytes.insert(bytes.end(), data, data + size);
            for (unsigned i = 0; i < std::min(size, std::uint64_t(8)); ++i) value = (value << 8) | data[i];
        }
        log.push_back({kind,address,size,value});
    }
    void mapped(std::uint32_t address, std::uint64_t size) override { event(1, address, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override { event(2, address, size, data); }
    void cleared(std::uint32_t address, std::uint64_t size) override { event(3, address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override {
        event(4, address, size, data);
        if (state->pc==0x10781320) callbacks.push_back({std::uint32_t(state->r<4>(12)),std::uint32_t(state->r<4>(16)),std::uint32_t(state->r<4>(20)),std::uint32_t(state->r<4>(48)),std::uint32_t(state->r<4>(4128))});
        if (on_read) on_read(*state, *this, address, size);
    }
};
// Instruction accounting and process-local hook diagnostics are intentionally
// excluded. Every architectural byte, including generated temporary residues,
// and every mapped memory byte is compared at returns and faults.
struct Image {
    decltype(State::registers) registers;
    decltype(State::unique) unique;
    std::uint32_t pc, heap_end;
    std::uint64_t native_calls;
    std::int16_t memory_error;
    decltype(State::allocated) allocated, available;
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> memory;
    explicit Image(const State& s) : registers(s.registers), unique(s.unique), pc(s.pc), heap_end(s.heap_end),
        native_calls(s.native_calls), memory_error(s.memory_error), allocated(s.allocated), available(s.available) {
        for (const auto& [address, size] : s.memory.mapped_ranges()) memory.emplace_back(address, s.memory.copy_out(address, size));
    }
    void compare(const State& s, const std::string& label) const {
        if (registers != s.registers) {
            const auto p = std::mismatch(registers.begin(), registers.end(), s.registers.begin());
            throw std::runtime_error(label + " register byte=" + std::to_string(p.first - registers.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        if (unique != s.unique) {
            const auto p = std::mismatch(unique.begin(), unique.end(), s.unique.begin());
            throw std::runtime_error(label + " temporary byte=" + std::to_string(p.first - unique.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        require(pc == s.pc, label + " PC expected=" + hex_address(pc) + " actual=" + hex_address(s.pc));
        require(heap_end == s.heap_end && native_calls == s.native_calls && memory_error == s.memory_error &&
                allocated == s.allocated && available == s.available, label + " allocator/import mismatch");
        const auto ranges = s.memory.mapped_ranges();
        require(ranges.size() == memory.size(), label + " mapping count mismatch");
        for (unsigned i = 0; i < memory.size(); ++i) {
            const auto& [address, bytes] = memory[i];
            require(ranges[i].first == address && ranges[i].second == bytes.size(), label + " mapping mismatch");
            const auto actual = s.memory.copy_out(address, bytes.size());
            if (bytes != actual) {
                const auto p = std::mismatch(bytes.begin(), bytes.end(), actual.begin());
                throw std::runtime_error(label + " memory mismatch at " + hex_address(address + std::uint32_t(p.first - bytes.begin())));
            }
        }
    }
};
std::unique_ptr<State> clone(const State& s) {
    auto t = std::make_unique<State>();
    t->registers = s.registers; t->unique = s.unique; t->pc = s.pc;
    t->executed = s.executed; t->budget = s.budget; t->native_calls = s.native_calls;
    t->allocated = s.allocated; t->available = s.available; t->heap_end = s.heap_end; t->memory_error = s.memory_error;
    for (const auto& [address, size] : s.memory.mapped_ranges()) {
        t->memory.map(address, size); t->memory.copy_in(address, s.memory.copy_out(address, size));
    }
    return t;
}
constexpr std::uint32_t format_address=fixture+0x500, arguments=fixture+0x1000,
    descriptor=fixture+0x700, stream=fixture+0x1200, buffer=fixture+0x1400;
void text(State& s,std::uint32_t address,const std::string& value) {
    s.memory.copy_in(address,{value.begin(),value.end()});s.memory.write<1>(address+value.size(),0);
}
std::unique_ptr<State> setup(const char* program,unsigned entry,const std::string& input="-123!",const std::string& format="%d%n") {
    auto s=std::make_unique<State>();load_program(*s,program);s->memory.map(fixture,0x2000);
    s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));s->registers.fill(0x5b);s->unique.fill(0x6d);
    for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000u+n);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->wr<4>(4132,0x31415926);s->wr<4>(2440,0xbadc0ffe);
    s->wr<1>(2304,0xa6);s->wr<1>(1024,1);s->wr<1>(1027,1);
    text(*s,left,input);text(*s,format_address,format);
    s->memory.copy_in(destination,std::vector<std::uint8_t>(128,0xdd));
    s->memory.write<4>(right,left);s->memory.write<4>(right+4,0);
    s->memory.write<4>(descriptor,0x1077d960);s->memory.write<4>(descriptor+4,toc);
    for(unsigned i=0;i<8;++i)s->memory.write<4>(arguments+i*4,destination+i*16);
    s->memory.write<4>(stream+4,0x00004000);s->memory.write<1>(stream+5,0x40);
    // Byte orientation is initially unset; the generated helper sets bit 0x10.
    s->memory.write<1>(stream+8,0x40);s->memory.write<4>(stream+40,buffer);
    s->memory.write<4>(stream+44,256);s->memory.write<4>(stream+48,buffer);
    text(*s,buffer,input);s->memory.write<4>(stream+32,buffer);
    switch(entry) {
    case 0:s->wr<4>(12,format_address);s->wr<4>(16,right);s->memory.write<4>(right,arguments);s->wr<4>(20,destination);break;
    case 1:s->wr<4>(12,descriptor);s->wr<4>(16,right);s->wr<4>(20,format_address);s->wr<4>(24,arguments);break;
    case 2:s->wr<4>(12,right);s->wr<4>(16,'Z');s->wr<4>(20,0);break;
    case 3:s->wr<4>(12,left);s->wr<4>(16,format_address);s->wr<4>(20,arguments);break;
    case 4:s->wr<4>(12,left);s->wr<4>(16,format_address);for(unsigned n=5;n<=10;++n)s->wr<4>(n*4,destination+(n-5)*16);break;
    case 5:s->wr<4>(12,stream);s->wr<4>(16,'Z');s->wr<4>(20,0);break;
    case 6:s->wr<4>(12,stream);s->wr<4>(16,format_address);for(unsigned n=5;n<=10;++n)s->wr<4>(n*4,destination+(n-5)*16);break;
    }
    s->pc=formatted_input_entries[entry];s->budget=100000;s->executed=s->native_calls=0;return s;
}
unsigned cases=0,faults=0;
std::array<std::uint64_t,7> witnesses{};
using Mutation=std::function<void(State&,Effects&,std::uint32_t,unsigned)>;
Effects compare_pair(State& original,State& modern,const std::string& label,bool expect_fault=false,const Mutation& mutation={},bool selected=false,bool escape=false) {
    if(!selected)original.formatted_input_original=true;
    Effects expected,actual;expected.state=&original;actual.state=&modern;expected.on_read=actual.on_read=mutation;
    original.memory.date_observer=&expected;modern.memory.date_observer=&actual;
    std::string a,b;
    try{run(original);}catch(const std::exception& e){a=e.what();}
    try{run(modern);}catch(const std::exception& e){b=e.what();}
    original.memory.date_observer=modern.memory.date_observer=nullptr;
    require(a==b,label+" fault mismatch original="+a+" semantic="+b);
    if(!expect_fault&&!a.empty())throw std::runtime_error(label+" original: "+a+" pc="+hex_address(original.pc));
    require(expect_fault==!a.empty(),label+" fault expectation mismatch");
    Image(original).compare(modern,label);
    if(expected.bytes!=actual.bytes) {
        unsigned i=0;while(i<std::min(expected.log.size(),actual.log.size())&&expected.log[i].kind==actual.log[i].kind&&expected.log[i].address==actual.log[i].address&&expected.log[i].size==actual.log[i].size&&expected.log[i].value==actual.log[i].value)++i;
        throw std::runtime_error(label+" ordered effect index="+std::to_string(i)+" expected="+(i<expected.log.size()?hex_address(expected.log[i].address):"end")+" actual="+(i<actual.log.size()?hex_address(actual.log[i].address):"end"));
    }
    require(expected.callbacks==actual.callbacks,label+" callback arguments/events mismatch");
    require((modern.formatted_input_dispatch_escapes!=0)==escape,label+" dispatch escape boundary");
    require(!modern.integer_parsing_original&&!modern.string_original&&!modern.memory_primitives_original&&
        !original.integer_parsing_original&&!original.string_original&&!original.memory_primitives_original,"Dependencies must stay semantic");
    ++cases;if(expect_fault)++faults;
    if(!modern.formatted_input_original&&modern.formatted_input_entry_original==std::array<bool,7>{})
        require(modern.formatted_input_original_calls==std::array<std::uint64_t,7>{},label+" unexpected original selection");
    if(!expect_fault&&!escape&&modern.formatted_input_original_calls==std::array<std::uint64_t,7>{})for(unsigned i=0;i<7;++i)witnesses[i]+=modern.formatted_input_calls[i];
    return actual;
}
using Expectation=std::function<void(const State&,const Effects&)>;
void check(std::unique_ptr<State> a,const std::string& label,const Expectation& expect={},bool fault=false,const Mutation& mutation={}) {
    auto b=clone(*a);auto result=compare_pair(*a,*b,label,fault,mutation);if(expect)expect(*b,result);
}
struct Boundaries final:FormattedInputBoundaryObserver {
    struct Boundary {std::uint32_t entry;Image image;Boundary(std::uint32_t e,const State& s):entry(e),image(s){}};
    std::vector<Boundary> expected;unsigned cursor=0;std::uint32_t perturb=0;bool mutated=false;
    void enter(const State&,std::uint32_t,bool) override {}
    void leave(State& s,std::uint32_t entry,bool original) override {
        if(original){expected.emplace_back(entry,s);return;}
        require(cursor<expected.size()&&expected[cursor].entry==entry,"Boundary order");
        if(entry==perturb){s.wr<4>(12,s.r<4>(12)^1);mutated=true;}
        expected[cursor++].image.compare(s,"Own formatted input boundary "+hex_address(entry));
    }
};
std::unique_ptr<State> sparse(const State& initial,std::uint64_t omitted=UINT64_MAX) {
    auto probe=clone(initial);Effects effects;effects.state=probe.get();probe->formatted_input_original=true;
    probe->memory.date_observer=&effects;run(*probe);probe->memory.date_observer=nullptr;
    auto result=std::make_unique<State>();result->registers=initial.registers;result->unique=initial.unique;result->pc=initial.pc;result->budget=initial.budget;
    for(const auto& [address,size]:effects.accesses) {
        if(address<=omitted&&omitted<std::uint64_t(address)+size) {
            if(omitted>address)result->memory.map(address,omitted-address);
            if(omitted+1<std::uint64_t(address)+size)result->memory.map(std::uint32_t(omitted+1),std::uint64_t(address)+size-omitted-1);
        } else result->memory.map(address,size);
    }
    for(const auto& [address,size]:result->memory.mapped_ranges())result->memory.copy_in(address,initial.memory.copy_out(address,size));return result;
}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: cm-formatted-input-checks <original-segments>\n";return 2;}
    try {
        for(const std::string& f:std::vector<std::string>{"%%","%d","%*3hd","%hhd","%lld","%ld","%Lf","%lc","%ls","%p","%n","%0d","%99999999999d","%","%Q","%vd","%hvd","%lvf","%,vd","%@vd","%[a-c]","%[^a-c]","%[]a]","%[z-a]","%[abc","%hhd","%hf","%llf","%Lc","%v@d"}) {
            auto a=setup(argv[1],0,"",f);text(*a,left,"  ,  ");a->memory.write<4>(arguments,left);
            check(std::move(a),"format "+f,[=](const State& s,const Effects&){
                if(f=="%d")require(s.memory.read<1>(destination+3)=='d'&&s.memory.read<4>(destination+4)==0x7fffffff&&s.r<4>(12)==format_address+2,"Independent format record");
                if(f=="%@vd")require(s.memory.read<4>(right)==arguments+4&&s.memory.read<4>(destination+16)==left+2&&s.memory.read<4>(destination+24)==1,"Delimiter cursor/trim");
            });
        }
        for(unsigned entry:{2u,5u})for(std::uint32_t mode:{0u,1u,2u,3u,UINT32_MAX,0x80000000u})for(bool eof:{false,true}) {
            auto a=setup(argv[1],entry);a->wr<4>(20,mode);if(entry==2)a->memory.write<4>(right+4,eof);else a->memory.write<1>(stream+13,eof);
            check(std::move(a),"callback mode/EOF "+std::to_string(entry)+"/"+std::to_string(mode));
        }
        for(unsigned entry:{2u})for(unsigned ch:{0u,128u,255u}){auto a=setup(argv[1],entry);a->memory.write<1>(left,ch);check(std::move(a),"callback high/NUL");}
        for(unsigned flags:{0u,1u,256u,257u}) {auto a=setup(argv[1],5);a->wr<4>(20,2);a->memory.write<1>(stream+12,flags&255);a->memory.write<1>(stream+13,flags>>8);check(std::move(a),"stream flags");}
        {auto a=setup(argv[1],5);a->wr<4>(20,2);a->memory.write<1>(stream+13,1);check(sparse(*a),"stream EOF short circuit",[](const State& s,const Effects& e){require(s.r<4>(12)==1&&std::none_of(e.log.begin(),e.log.end(),[](const auto& x){return x.kind==4&&x.address==stream+12;}),"Error flag must not be read after EOF");});}
        struct Scan {std::string input,format;unsigned count;std::uint64_t value;unsigned width=4;};
        const std::vector<Scan> scans{
            {"-123!","%d%n",1,std::uint32_t(-123)},{"18446744073709551615!","%llu%n",1,UINT64_MAX,8},
            {"12!","%d",1,12},{"!","%d",0,0xdddddddd},{"abc","xyz",0,0xdddddddd},
            {"  12!"," \t%d%n",1,12},{" %12","%%%d",1,12},{"3 7!","%*d%d",1,7},
            {"1234","%2d%n",1,12},{"0x2a!","%i",1,42},{"077!","%o",1,63},
            {"0XAF!","%X",1,175},{"ffffffff!","%p",1,UINT32_MAX},
            {"4294967296!","%u",1,0},{"-4294967297!","%d",1,UINT32_MAX},
            {"18446744073709551616!","%llu",1,0,8},{"-18446744073709551615!","%lld",1,1,8},
            {"-129!","%hhd",1,127,1},{"65537!","%hu",1,1,2},{"-3!","%ld",1,std::uint32_t(-3)},
            {"inf!","%f",1,0x7f800000},{"-inf!","%lf",1,0xfff0000000000000ULL,8},
            {"inf!","%Lg",1,0x7ff0000000000000ULL,8},{"inf!","%a",1,0x7f800000},
            {"abc!","%c%n",1,'a',1},{"abc!","%2c%n",1,0x6162,2},
            {"  abc def","%s%n",1,0x61626300},{"abc!","%[a-c]%n",1,0x61626300},
            {"ab!","%[^!]",1,0x616200dd},{"]aa!","%[]a]",1,0x5d616100},
            {"x","%n%c",1,0},{"12!","%d%d",1,12},{"12!","%d%*d",1,12}
        };
        for(const auto& c:scans)for(unsigned entry:{1u,3u,4u}) {
            auto a=setup(argv[1],entry,c.input,c.format);
            check(std::move(a),"scan "+std::to_string(entry)+" "+c.format+"/"+c.input,[=](const State& s,const Effects&){
                require(s.r<4>(12)==c.count,"Independent assignment count");
                const auto value=c.width==8?s.memory.read<8>(destination):c.width==2?s.memory.read<2>(destination):c.width==1?s.memory.read<1>(destination):s.memory.read<4>(destination);
                require(value==c.value,"Independent destination "+c.format+" expected="+std::to_string(c.value)+" actual="+std::to_string(value));
                if(c.format=="%d%n"||c.format=="%llu%n") {
                    require(s.memory.read<4>(destination+16)==c.input.size()-1,"Independent %n count");
                    const auto context=entry==1?right:entry==3?stack-64+56:stack-128+56;
                    require(s.memory.read<4>(context)==left+c.input.size()-1,"Independent facade callback cursor");
                }
                require(s.memory.read<1>(destination+8)==0xdd,"Destination guard");
            });
        }
        // Decimal float parsing reaches an existing unimplemented OS service.
        // Preserve that exact dependency failure and its complete effect prefix.
        for(const std::string& f:{"%f","%lf","%Lf"})check(setup(argv[1],4,"1.5!",f),"decimal dependency fault "+f,{},true);
        for(unsigned entry:{1u,3u,4u})for(const auto& c:std::vector<std::pair<std::string,std::string>>{{"","%d"},{"","%n"},{"12","%d%d"},{"  ","%d"},{"abc","%*2c%n"},{"abc","%lc%n"},{"abc!","%ls%n"},{"abc!","%l[a-c]%n"},{"12","%0d"},{"","%%"},{"%","%%"},{"abc","%Q"}})check(setup(argv[1],entry,c.first,c.second),"boundary/characters "+c.second);
        for(const std::string& f:{"%3c%hhn","%3c%hn","%3c%ln","%3c%lln"})check(setup(argv[1],4,"abc!",f),"count destination widths",[=](const State& s,const Effects&){
            const auto count=f.ends_with("hhn")?s.memory.read<1>(destination+16):f.ends_with("hn")?s.memory.read<2>(destination+16):f.ends_with("lln")?s.memory.read<8>(destination+16):s.memory.read<4>(destination+16);
            require(s.r<4>(12)==1&&count==3,"Count narrowing/widening");
        });
        for(const auto& c:std::vector<std::pair<std::string,std::string>>{{"abc","%*s%n"},{"abc","%*[a-c]%n"},{"!","%[a-c]"},{"abc","%4294967295c"},{std::string("\x80\x81!",3),std::string("%[\x80-\x82]",7)},{std::string("\xff!",2),"%c"},{"-inf!","%f"},{"nan!","%lf"},{"inf!","%*f"}})check(setup(argv[1],4,c.first,c.second),"text/high-byte/float boundary",{},c.first=="nan!");
        for(unsigned entry:{3u,4u,6u}){auto a=setup(argv[1],entry);a->wr<4>(12,0);check(std::move(a),"null facade");}
        for(unsigned orientation:{0u,0x10u,0x20u,0x30u}){auto a=setup(argv[1],6);a->memory.write<1>(stream+5,0x40|orientation);check(std::move(a),"stream orientation");}
        for(const auto& c:std::vector<std::pair<std::string,std::string>>{{"-123!","%d%n"},{"18446744073709551615!","%llu%n"},{"abc!","%3c%n"},{"ab!","%[a-b]%n"},{"inf!","%f%n"}}) {
            check(setup(argv[1],6,c.first,c.second),"buffered stream composition "+c.second,[=](const State& s,const Effects&){
                require(s.r<4>(12)==1,"Stream count");require(s.memory.read<1>(stream+5)==0x50,"Byte orientation");
                require(s.memory.read<1>(stream+12)==0&&s.memory.read<1>(stream+13)==0,"No stream EOF/error");
                const unsigned consumed=c.second=="%[a-b]%n"?2:c.first.size()-1;
                require(s.memory.read<4>(destination+16)==consumed,"Stream consumed count");
                if(c.second=="%d%n"||c.second=="%llu%n") {
                    require(s.memory.read<4>(stream+40)==buffer+c.first.size()&&s.memory.read<4>(stream+44)==0&&s.memory.read<4>(stream+52)==256-c.first.size(),"Buffered cursor/count after lookahead");
                    require(s.memory.read<1>(stream+8)==0x60&&s.memory.read<1>(stream+19)=='!',"Buffered unread state/character");
                    require(c.second=="%d%n"?s.memory.read<4>(destination)==std::uint32_t(-123):s.memory.read<8>(destination)==UINT64_MAX,"Stream integer destination");
                }
            });
        }
        // Repeated Mac conversion extensions: compact, explicit delimiters
        // avoid the original mode-1 whitespace loop's nontermination.
        for(const auto& c:std::vector<std::pair<std::string,std::string>>{{"1,2,3,4!","%,lvd%n"},{"1,2,3,4!","%,lvu%n"},{"inf,,inf,,inf,,inf!","%,vf%n"},{"a,b,c,d!","%,lvc%n"},{"1!","%*vd"},{"1!","%*vu"},{"inf!","%*vf"},{"a!","%*vc"},{"1,2!","%@lvd%n"}}) {
            auto a=setup(argv[1],4,c.first,c.second);if(c.second=="%@lvd%n"){text(*a,fixture+0x800,",");a->wr<4>(20,fixture+0x800);a->wr<4>(24,destination);a->wr<4>(28,destination+16);}
            check(std::move(a),"repeated "+c.second,[=](const State& s,const Effects&){
                const bool suppressed=c.second.find('*')!=std::string::npos;require(s.r<4>(12)==(suppressed?0u:1u),"Repeated assignment count");
                if(c.second=="%,lvd%n"||c.second=="%,lvu%n")for(unsigned i=0;i<4;++i)require(s.memory.read<4>(destination+i*4)==i+1,"Packed integer lanes");
                if(c.second=="%,vf%n")for(unsigned i=0;i<4;++i)require(s.memory.read<4>(destination+i*4)==0x7f800000,"Packed float lanes lane="+std::to_string(i)+" actual="+std::to_string(s.memory.read<4>(destination+i*4)));
                if(c.second=="%,lvc%n")for(unsigned i=0;i<4;++i)require(s.memory.read<4>(destination+i*4)=='a'+i,"Packed character lanes");
                if(c.second=="%@lvd%n")require(s.memory.read<4>(destination)==1&&s.memory.read<4>(destination+4)==2&&s.memory.read<8>(destination+8)==0,"Delimiter argument and partial vector");
            });
        }
        for(const auto& c:std::vector<std::pair<std::string,std::string>>{{"1,2!","%,vd"},{"1,2!","%,hvd"},{"1,2!","%,hhvd"},{"-1,-2!","%,hvu"},{"inf,,inf!","%,lvf"},{"a,b!","%,hvc"},{"a,b!","%,vc"},{"a,b!","%,*lvc"},{"1,2!","%,*lvd"},{"1,2!","%,*lvu"},{"inf,,inf!","%,*vf"}})check(setup(argv[1],4,c.first,c.second),"partial/repeated "+c.second);
        // Live record template and ctype mutations change the actual operation.
        {
            auto a=setup(argv[1],4,"1234!","%d");const auto t=std::uint32_t(a->memory.read<4>(toc-0x3b40));
            a->memory.write<4>(t+4,2);check(std::move(a),"mutable template width",[](const State& s,const Effects&){require(s.memory.read<4>(destination)==12,"Template field width");});
            auto b=setup(argv[1],0,"","%d");const auto table=std::uint32_t(b->memory.read<4>(toc-0x3b44));
            b->memory.write<4>(table+('d'-'E')*4,0x1077bb28);check(std::move(b),"conversion destination redirection",[](const State& s,const Effects&){require(s.memory.read<1>(destination+3)=='x'&&s.memory.read<1>(destination+2)==3,"Live destination pointer semantic selection");});
            auto c=setup(argv[1],0,"","%d");c->memory.write<4>(table+('d'-'E')*4,0xffff0000);auto d=clone(*c);
            compare_pair(*c,*d,"unknown conversion destination",true,{},false,true);require(d->formatted_input_dispatch_escapes==1,"Private escape diagnostic");
            auto e=setup(argv[1],4,"12!","%d");const auto ctype=std::uint32_t(e->memory.read<4>(toc-0x5a48));
            e->memory.write<1>(ctype+'1',0);check(std::move(e),"classification rejects input",[](const State& s,const Effects&){require(s.r<4>(12)==0&&s.memory.read<4>(destination)==0xdddddddd,"Guest ctype changes parsing");});
            auto f=setup(argv[1],0,"","%q");f->memory.write<1>(ctype+'q',16);check(std::move(f),"format width classification mutation");
        }
        for(unsigned entry:{0u,1u,3u,4u,6u}) {
            auto a=setup(argv[1],entry);const auto relocated=fixture+0x9000;
            for(unsigned offset:{0x3b40u,0x3b44u,0x5a48u,0x5a44u,0x5a40u,0x62bcu,0x3b24u,0x3b28u}){a->memory.map(relocated-offset,4);a->memory.write<4>(relocated-offset,a->memory.read<4>(toc-offset));}
            a->wr<4>(8,relocated);check(std::move(a),"relocated TOC");
        }
        {
            auto a=setup(argv[1],1);check(std::move(a),"mutable callback descriptor/clobber",[](const State&,const Effects& e){require(e.mutated,"Descriptor mutation observed");},false,
                [](State& s,Effects& e,std::uint32_t,unsigned){if(s.pc==0x1077d988&&!e.mutated){e.mutated=true;s.memory.write<4>(descriptor,0x1077d9e0);s.memory.write<4>(right+4,UINT32_MAX);s.wr<4>(44,0x13572468);}});
            auto b=setup(argv[1],1,"","%d%n");b->memory.write<4>(descriptor,0x1077d9e0);b->memory.write<4>(right+4,0);
            check(std::move(b),"generic noncontiguous callback",[](const State& s,const Effects& e){require(s.r<4>(12)==1&&s.memory.read<4>(destination)==12&&s.memory.read<4>(destination+16)==2&&e.mutated,"Generic callback values/count");},false,
                [](State& s,Effects& e,std::uint32_t address,unsigned){if(s.pc==0x1077d9e0&&address==right+4){e.mutated=true;const auto mode=std::uint32_t(s.r<4>(20));unsigned reads=0;for(const auto& c:e.callbacks)if(c[2]==0)++reads;s.memory.write<4>(right+4,mode==2?0:mode==0?(reads==1?'1':reads==2?'2':'!'):s.r<4>(16));}});
        }
        for(unsigned entry:{0u,3u,4u})for(std::uint32_t alias:{stack-4,stack-64+56,stack-176+56}) {
            auto a=setup(argv[1],entry);if(entry==0)a->wr<4>(20,alias);else if(entry==3)a->memory.write<4>(arguments,alias);else a->wr<4>(20,alias);
            check(std::move(a),"record/destination frame alias "+std::to_string(entry)+"/"+hex_address(alias),{},(entry==0&&alias!=stack-176+56)||(entry==4&&alias==stack-176+56));
        }
        for(unsigned mode:{0u,1u}){auto a=setup(argv[1],2);a->wr<4>(20,mode);a->memory.write<4>(right,mode?0u:0xffffffffu);if(mode==0){a->memory.map(0xffffffff,1);a->memory.write<1>(0xffffffff,128);}check(std::move(a),"string cursor wrap");}
        for(unsigned entry:{1u,4u})for(bool inverse:{false,true}) {
            auto a=setup(argv[1],entry);auto b=clone(*a);b->formatted_input_entry_original[entry]=inverse;b->integer_parsing_entry_original[0]=!inverse;
            compare_pair(*a,*b,"mixed scanner/parser");
        }
        for(bool inverse:{false,true}) {
            auto a=setup(argv[1],4);auto b=clone(*a);b->formatted_input_entry_original[2]=inverse;b->integer_parsing_entry_original[0]=!inverse;
            compare_pair(*a,*b,"mixed parser/callback");
        }
        for(unsigned entry=0;entry<7;++entry) {
            auto a=setup(argv[1],entry);check(sparse(*a),"minimal mapping "+std::to_string(entry));
            auto probe=clone(*a);Effects e;e.state=probe.get();probe->formatted_input_original=true;probe->memory.date_observer=&e;run(*probe);probe->memory.date_observer=nullptr;
            // One missing final byte for each distinct accessed scalar.
            std::vector<std::uint32_t> missing;
            for(const auto& x:e.log){const auto byte=x.address+std::uint32_t(x.size)-1;if(std::find(missing.begin(),missing.end(),byte)==missing.end())missing.push_back(byte);}
            for(auto byte:missing)check(sparse(*a,byte),"fault "+std::to_string(entry)+" "+hex_address(byte),{},true);
            auto original=setup(argv[1],entry);auto modern=clone(*original);Boundaries b;original->formatted_input_original=true;original->formatted_input_observer=&b;run(*original);
            modern->formatted_input_observer=&b;run(*modern);require(b.cursor==b.expected.size(),"All boundaries visited");
            auto negative=setup(argv[1],entry);negative->formatted_input_observer=&b;b.cursor=0;b.perturb=formatted_input_entries[entry];bool rejected=false;
            try{run(*negative);}catch(const std::exception& ex){rejected=std::string(ex.what()).starts_with("Own formatted input boundary "+hex_address(b.perturb));}
            require(rejected&&b.mutated,"Own negative");negative->formatted_input_original=true;negative->formatted_input_entry_original.fill(true);negative->formatted_input_original_resume=formatted_input_entries[entry];negative->formatted_input_dispatch_escapes=1;negative->string_calls[0]=77;reset_formatted_input_diagnostics(*negative);
            require(!negative->formatted_input_original&&negative->string_calls[0]==77&&!negative->formatted_input_observer&&!negative->formatted_input_original_resume&&!negative->formatted_input_dispatch_escapes&&negative->formatted_input_calls==std::array<std::uint64_t,7>{}&&negative->formatted_input_original_calls==std::array<std::uint64_t,7>{}&&negative->formatted_input_entry_original==std::array<bool,7>{},"Diagnostics reset");
            auto indirect=setup(argv[1],entry);indirect->pc=0x10780900;indirect->memory.write<4>(fixture+0x710,formatted_input_entries[entry]);indirect->memory.write<4>(fixture+0x714,toc);indirect->memory.write<4>(toc-0x7ee4,fixture+0x710);
            check(std::move(indirect),"generated indirect transition");
        }
        {
            auto a=setup(argv[1],4);Boundaries b;a->formatted_input_original=true;a->formatted_input_observer=&b;run(*a);
            auto n=setup(argv[1],4);n->formatted_input_observer=&b;b.perturb=formatted_input_entries[0];bool rejected=false;
            try{run(*n);}catch(const std::exception& ex){rejected=std::string(ex.what()).starts_with("Own formatted input boundary "+hex_address(b.perturb));}
            require(rejected&&b.cursor<b.expected.size(),"Child negative before parent");
        }
        for(const auto& choice:std::vector<std::pair<unsigned,unsigned>>{{4,1},{1,0},{6,1}})for(bool inverse:{false,true}) {
            auto a=setup(argv[1],choice.first);auto b=clone(*a);b->formatted_input_entry_original[choice.first]=inverse;b->formatted_input_entry_original[choice.second]=!inverse;compare_pair(*a,*b,"mixed modes");
        }
        require(std::all_of(witnesses.begin(),witnesses.end(),[](auto n){return n!=0;}),"Nonempty seven-entry production coverage");
        for(unsigned i=0;i<7;++i)std::cout<<"Normal witness "<<hex_address(formatted_input_entries[i])<<" calls="<<witnesses[i]<<" original=0 escapes=0\n";
        std::cout<<"PASS: formatted input full architecture/memory/ordered effects cases="<<cases<<" faults="<<faults<<" own negatives=7 child negative=1 nonempty entries=7 indirect transitions=7 mixed modes=12 zero normal escapes\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
