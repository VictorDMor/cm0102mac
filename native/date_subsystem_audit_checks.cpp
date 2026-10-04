#include "date_subsystem_audit.hpp"
#include "match_replay.hpp"
#include "callbacks.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <zlib.h>
using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture=0x30000000,stack=0x700fff00,toc=0x108bba90;
void require(bool test,const std::string& message){if(!test)throw std::runtime_error(message);}
void reset(State& s,std::uint32_t entry,bool original) {
    s.registers.fill(0x5b);s.unique.fill(0x6d);
    for(unsigned i=0;i<32;++i)s.wr<4>(i*4,0x12340000+i);
    s.wr<4>(4,stack);s.wr<4>(8,toc);s.wr<4>(4128,return_sentinel);s.wr<4>(2440,0xbadc0ffe);
    s.memory.copy_in(fixture,std::vector<std::uint8_t>(4096,0xa5));s.memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));
    s.memory.write<2>(fixture+16,243);s.memory.write<2>(fixture+18,2002);s.memory.write<1>(fixture+20,0);
    s.memory.write<2>(fixture+32,59);s.memory.write<2>(fixture+34,2003);s.memory.write<1>(fixture+36,0);
    s.wr<4>(12,fixture+64);s.wr<4>(16,fixture+16);s.wr<4>(20,1);s.wr<4>(24,2002);s.wr<4>(28,0xffff);
    if(entry==0x107156b0){s.wr<4>(16,243);s.wr<4>(20,2002);}
    if(entry==0x107156f0||entry==0x107158a0){s.wr<4>(16,1);s.wr<4>(20,8);}
    if(entry==0x10715a30){s.wr<4>(12,fixture+16);s.wr<4>(16,1);s.wr<4>(20,fixture+64);}
    if(entry>=0x10715bc0&&entry<=0x10717620){s.wr<4>(12,fixture+16);s.wr<4>(16,fixture+256);s.wr<4>(20,100);s.wr<4>(24,1);}
    if(entry==0x10717dd0){s.wr<4>(20,s.memory.read<4>(fixture+32));s.wr<4>(24,s.memory.read<4>(fixture+36));}
    if(entry==0x10718440)s.wr<4>(12,2000);
    if(entry>=0x107184d0){s.wr<4>(12,1);s.wr<4>(16,1);}
    if(entry==0x10718af0){s.wr<4>(12,9);s.wr<4>(16,5);}
    for(auto [offset,flag]:{std::pair{0x3facu,0x35bbu},{0x3fb0u,0x35bcu},{0x3fb4u,0x35bdu},{0x3fb8u,0x35beu},{0x3fbcu,0x35bfu},{0x3fc0u,0x35c0u}}) {
        s.memory.write<4>(toc-offset,fixture+1024+(offset&31)*8);s.memory.write<1>(toc-flag,0);
    }
    s.calendar_original=original;s.date_subsystem_original=false;s.date_entry_original.fill(false);
    s.pc=entry;s.executed=0;s.native_calls=0;s.budget=2000000;
}
// Every comparison starts from identical complete legacy memory, including
// stateful language-service bookkeeping. Retain only the actual diagnostic
// counts from prior attempts to prove that perturbation reset is independent
// of an entry's lifetime call count. This is a synthetic contract fixture.
std::unique_ptr<State> clone_input(const State& input,const State* diagnostics=nullptr) {
    auto copy=std::make_unique<State>();copy->registers=input.registers;copy->unique=input.unique;
    copy->pc=input.pc;copy->executed=input.executed;copy->budget=input.budget;copy->native_calls=input.native_calls;
    copy->allocated=input.allocated;copy->available=input.available;copy->heap_end=input.heap_end;copy->memory_error=input.memory_error;
    copy->platform=input.platform;
    for(const auto& [start,size]:input.memory.mapped_ranges()) {
        copy->memory.map(start,size);
        for(std::uint64_t offset=0;offset<size;offset+=65536)copy->memory.copy_in(start+std::uint32_t(offset),input.memory.copy_out(start+std::uint32_t(offset),std::min<std::uint64_t>(65536,size-offset)));
    }
    if(diagnostics) {
        copy->date_subsystem_calls=diagnostics->date_subsystem_calls;copy->date_original_calls=diagnostics->date_original_calls;
        copy->calendar_calls=diagnostics->calendar_calls;copy->date_weekday_calls=diagnostics->date_weekday_calls;
    }
    return copy;
}
std::vector<std::uint8_t> inflate(const std::filesystem::path& path) {
    auto file=gzopen(path.c_str(),"rb");require(file,"Cannot read test Date stream");std::vector<std::uint8_t> result;
    std::array<std::uint8_t,65536> block;int n;
    while((n=gzread(file,block.data(),block.size()))>0)result.insert(result.end(),block.begin(),block.begin()+n);
    require(gzclose(file)==Z_OK&&n==0,"Invalid test Date oracle");return result;
}
void deflate(const std::filesystem::path& path,const std::vector<std::uint8_t>& bytes) {
    auto file=gzopen(path.c_str(),"wb1");require(file,"Cannot write test Date stream");
    require(gzwrite(file,bytes.data(),unsigned(bytes.size()))==int(bytes.size())&&gzclose(file)==Z_OK,"Cannot finalize test Date stream");
}
}
int main(int argc,char** argv) {
    if(argc!=3)return 2;
    try {
        const std::filesystem::path output(argv[2]);std::filesystem::create_directories(output);
        auto original=std::make_unique<State>(),modern=std::make_unique<State>();
        load_match_snapshot(*original,argv[1]);load_match_snapshot(*modern,argv[1]);original->memory.map(fixture,4096);modern->memory.map(fixture,4096);
        unsigned negatives=0,positives=0;
        for(auto entry:date_subsystem_entries) {
            const auto oracle=output/(hex_address(entry)+"-original.gz");reset(*original,entry,true);
            const auto input=clone_input(*original);
            open_date_subsystem_audit(oracle,true);run(*original);close_date_subsystem_audit(true);
            for(unsigned attempt=0;attempt<6;++attempt) {
                modern=clone_input(*input,modern.get());reset(*modern,entry,false);const bool negative=attempt<4&&attempt%2;
                if(attempt==4)modern->date_subsystem_original=true;
                if(attempt==5)modern->date_entry_original[std::find(date_subsystem_entries.begin(),date_subsystem_entries.end(),entry)-date_subsystem_entries.begin()]=true;
                const auto weekday_before=modern->date_weekday_calls;const auto shifts_before=modern->calendar_calls;
                open_date_subsystem_audit(oracle,false,negative?entry:0);bool rejected=false;
                try {run(*modern);close_date_subsystem_audit(true);}
                catch(const std::runtime_error& error) {
                    close_date_subsystem_audit();const std::string text(error.what());
                    require(negative&&text.starts_with("First divergence: Date edge 0 at "+hex_address(entry)+" field=result/ABI/effects"),"Unexpected Date rejection entry="+hex_address(entry)+" attempt="+std::to_string(attempt)+": "+text);rejected=true;
                }
                require(rejected==negative,"Date negative escaped or failed to reset");
                if(attempt==4&&entry==0x10715a30)require(modern->date_weekday_calls==weekday_before+1&&modern->calendar_calls[0]==shifts_before[0]+1&&modern->calendar_calls[1]==shifts_before[1]+1,"Original adjustment parent disabled delivered semantic children");if(negative)++negatives;else ++positives;
            }
        }
        unsigned callbacks=0;
        for(auto entry:date_subsystem_entries) {
            const auto path=output/(hex_address(entry)+"-callback-original.gz");std::uint32_t expected_return=0;
            reset(*original,entry,true);const auto input=clone_input(*original);modern=clone_input(*input,modern.get());
            for(auto* state:{original.get(),modern.get()}) {
                const bool reference=state==original.get();reset(*state,entry,reference);
                state->memory.write<4>(fixture+3000,entry);state->memory.write<4>(fixture+3004,toc);
                const auto registers=state->registers;const auto temporaries=state->unique;
                const std::array<std::uint32_t,5> args{std::uint32_t(state->r<4>(12)),std::uint32_t(state->r<4>(16)),std::uint32_t(state->r<4>(20)),std::uint32_t(state->r<4>(24)),std::uint32_t(state->r<4>(28))};
                open_date_subsystem_audit(path,reference);const auto result=call_original(*state,fixture+3000,args);close_date_subsystem_audit(true);
                require(state->registers==registers&&state->unique==temporaries&&state->pc==entry,"Date callback did not restore its caller ABI");
                if(reference)expected_return=result;else require(result==expected_return,"Date callback return differs");
            }
            ++callbacks;
        }
        // A selected original parent must still call the modern delivered
        // weekday/subtraction children in the eight-entry baseline.
        require(modern->date_weekday_calls&&modern->calendar_calls[0]&&modern->calendar_calls[1],"Original Date parents disabled semantic delivered children");
        constexpr std::uint32_t entry=0x10715880;
        const auto good=output/(hex_address(entry)+"-original.gz"),bad=output/"malformed.gz";
        const auto bytes=inflate(good);
        for(unsigned kind=0;kind<6;++kind) {
            auto altered=bytes;
            if(kind==0)altered.resize(4);
            if(kind==1)altered.resize(18);
            if(kind==2)altered.resize(61);
            if(kind==3)altered[7]='2';
            if(kind==4)altered.push_back('X');
            deflate(bad,altered);
            if(kind==5) {
                std::ifstream in(bad,std::ios::binary);std::vector<char> compressed{std::istreambuf_iterator<char>(in),{}};compressed[compressed.size()-8]^=1;
                std::ofstream out(bad,std::ios::binary);out.write(compressed.data(),compressed.size());
            }
            reset(*modern,entry,false);bool rejected=false;
            try {open_date_subsystem_audit(bad,false);run(*modern);close_date_subsystem_audit(true);}
            catch(const std::runtime_error&){close_date_subsystem_audit();rejected=true;}
            require(rejected,"Malformed/truncated/unconsumed/checksum Date stream accepted");
        }
        reset(*modern,entry,false);open_date_subsystem_audit(output/"candidate-record-rejected.gz",true);bool rejected=false;
        try{run(*modern);}catch(const std::runtime_error& error){rejected=std::string(error.what()).find("all 34 original")!=std::string::npos;}
        close_date_subsystem_audit();require(rejected,"Candidate was recorded as Date oracle");
        std::cout<<positives<<" composed stream agreements; "<<negatives<<" isolated own-entry result rejections across 26 entries; "<<callbacks<<" original/modern indirect callbacks; reset, header/body truncation, version, checksum, EOF/unconsumed and candidate-record enforcement passed\n";
    }catch(const std::exception& error){try{close_date_subsystem_audit();}catch(...){}std::cerr<<error.what()<<'\n';return 1;}
}
