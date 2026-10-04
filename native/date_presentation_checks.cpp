#include "date_subsystem_adapter.hpp"
#include "match_replay.hpp"
#include <fstream>
#include <functional>
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture=0x30000000, stack=0x700fff00, toc=0x108bba90;
void require(bool value,const std::string& message) {if(!value)throw std::runtime_error(message);}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> events;
    void number(std::uint64_t value, unsigned width) {
        for (unsigned i=width;i;--i) events.push_back(std::uint8_t(value >> ((i-1)*8)));
    }
    void mapped(std::uint32_t start, std::uint64_t size) override {
        number(1,1); number(start,4); number(size,8);
    }
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        number(2,1); number(start,4); number(size,8);
        events.insert(events.end(),bytes,bytes+size);
    }
    void cleared(std::uint32_t start, std::uint64_t size) override {
        number(3,1); number(start,4); number(size,8);
    }
};
}
int main(int argc,char** argv) {
    if(argc!=3 && argc!=4)return 2;
    try {
        auto s=std::make_unique<State>();load_match_snapshot(*s,argv[1]);s->memory.map(fixture,32768);
        auto semantic=std::make_unique<State>();load_match_snapshot(*semantic,argv[1]);semantic->memory.map(fixture,32768);
        const bool compare_semantic=argc==4;
        std::ofstream evidence(argv[2],std::ios::binary);require(bool(evidence),"Cannot open private presentation characterization");
        const bool composed=argc==4&&std::string_view(argv[3])=="--composed";
        unsigned cases=0;
        auto check=[&](std::uint32_t entry,std::uint32_t first,std::uint32_t second,std::uint32_t phase,std::function<void(State&)> setup={}) {
            for(auto* state:{s.get(),semantic.get()}) {
                state->memory.observer=nullptr;state->memory.read_trace=nullptr;
                state->registers.fill(0x5b);state->unique.fill(0x6d);
                for(unsigned reg=0;reg<32;++reg)state->wr<4>(reg*4,0x12340000+reg);
                state->wr<4>(4,stack);state->wr<4>(8,toc);state->wr<4>(2440,0xbadc0ffe);state->wr<4>(4128,return_sentinel);
                state->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));
                state->memory.copy_in(fixture,std::vector<std::uint8_t>(4096,0xa5));
                state->memory.write<2>(fixture+16,243);state->memory.write<2>(fixture+18,2002);state->memory.write<1>(fixture+20,0);
                state->wr<4>(12,first);state->wr<4>(16,second);state->wr<4>(20,100);state->wr<4>(24,phase);
                // Isolate the original diagnostic logging branch. Modal OS
                // dialogs are characterized separately in the real app.
                state->memory.write<1>(toc-0x3550,1);
                state->calendar_original=state==s.get()||!composed;state->executed=0;state->budget=2000000;state->native_calls=0;state->pc=entry;
                if(setup)setup(*state);
            }
            const auto inputs=s->registers;
            Effects effects,actual;std::vector<std::pair<std::uint32_t,unsigned>> reads,actual_reads;
            s->memory.observer=&effects;s->memory.read_trace=&reads;
            try {run(*s);}catch(const std::runtime_error& error){throw std::runtime_error("Original presentation entry="+hex_address(entry)+" case="+std::to_string(cases)+": "+error.what());}s->memory.observer=nullptr;s->memory.read_trace=nullptr;require(s->pc==return_sentinel,"Original presentation did not return");
            if(compare_semantic) {
                semantic->memory.observer=&actual;semantic->memory.read_trace=&actual_reads;
                if(composed)run(*semantic);
                else {semantic->step(entry);require(execute_semantic_date(*semantic,entry),"Presentation semantic entry is pending at "+hex_address(entry));}
                semantic->memory.observer=nullptr;semantic->memory.read_trace=nullptr;
                require(reads==actual_reads,"Presentation ordered reads differ at "+hex_address(entry)+" case="+std::to_string(cases));
                require(effects.events==actual.events,"Presentation ordered effects differ at "+hex_address(entry)+" case="+std::to_string(cases));
                if(s->registers!=semantic->registers) {
                    for(unsigned byte=0;byte<s->registers.size();++byte)if(s->registers[byte]!=semantic->registers[byte])
                        std::cerr<<"register byte="<<byte<<" original="<<unsigned(s->registers[byte])<<" semantic="<<unsigned(semantic->registers[byte])<<'\n';
                    throw std::runtime_error("Presentation ABI differs at "+hex_address(entry)+" case="+std::to_string(cases));
                }
                require(s->pc==semantic->pc && s->unique==semantic->unique,"Presentation PC/temporaries differ");
                require(s->memory.mapped_ranges()==semantic->memory.mapped_ranges(),"Presentation mapped ranges differ");
                for(const auto& [start,size]:s->memory.mapped_ranges())for(std::uint64_t offset=0;offset<size;offset+=65536) {
                    const auto length=std::size_t(std::min<std::uint64_t>(size-offset,65536));
                    require(s->memory.copy_out(start+std::uint32_t(offset),length)==semantic->memory.copy_out(start+std::uint32_t(offset),length),"Presentation entire mapped memory differs at "+hex_address(entry)+" case="+std::to_string(cases)+" range="+hex_address(start+std::uint32_t(offset)));
                }
                require(s->allocated==semantic->allocated && s->available==semantic->available && s->heap_end==semantic->heap_end &&
                        s->memory_error==semantic->memory_error && s->native_calls==semantic->native_calls,"Presentation allocator/import effects differ");
            }
            auto bytes=[&](const auto& value){evidence.write(reinterpret_cast<const char*>(&value),sizeof(value));};
            bytes(entry);bytes(inputs);bytes(s->registers);bytes(s->pc);bytes(s->native_calls);
            const auto ne=std::uint32_t(effects.events.size());bytes(ne);evidence.write(reinterpret_cast<const char*>(effects.events.data()),ne);
            const auto nr=std::uint32_t(reads.size());bytes(nr);for(const auto& [address,width]:reads){bytes(address);bytes(width);}
            auto output=s->memory.copy_out(fixture,4096);evidence.write(reinterpret_cast<const char*>(output.data()),output.size());
            ++cases;
        };
        for(auto entry:{0x107184d0u,0x10718570u})for(unsigned phase:{0u,1u,2u,3u,255u})for(unsigned mode:{0u,1u,128u})check(entry,0xa5a50000|phase,mode,0);
        for(auto entry:{0x10718700u,0x107187d0u})for(unsigned day:{0u,1u,2u,3u,4u,5u,6u,7u,65535u})check(entry,0xa5a50000|day,0,0);
        for(auto entry:{0x107188a0u,0x107189d0u})for(unsigned month:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,255u})check(entry,0xa5a50000|month,0,0);
        for(unsigned language:{0u,3u,4u,5u,9u,10u}) {
            s->memory.write<1>(toc-0x35ac,language);semantic->memory.write<1>(toc-0x35ac,language);
            for(unsigned phase:{0u,1u,2u,3u,255u})check(0x10718650,0xa5a50000|phase,0,0);
        }
        for(auto entry:{0x10715bc0u,0x10715e00u,0x10716130u,0x107164f0u,0x107168a0u,0x10716b00u,0x10716eb0u,0x10717270u,0x10717620u})
                for(unsigned phase:{0u,1u,2u,3u,255u})check(entry,fixture+16,fixture+256,phase);
        const std::array<std::uint32_t,9> format_entries{0x10715bc0,0x10715e00,0x10716130,0x107164f0,0x107168a0,0x10716b00,0x10716eb0,0x10717270,0x10717620};
        for(auto entry:format_entries)for(unsigned year:{0u,1599u,1600u,1601u,1900u,2000u,2002u,2100u,65535u})
            for(int day:{-1,0,1,58,59,60,243,365,366})for(unsigned flag:{0u,1u,128u,255u})
                check(entry,fixture+16,fixture+256,year%4,[&](State& a){a.memory.write<2>(fixture+16,day);a.memory.write<2>(fixture+18,year);a.memory.write<1>(fixture+20,flag);});
        for(auto entry:format_entries)for(unsigned capacity:{0u,8u,9u,17u,18u,49u,50u,32767u,32768u,65535u})
            for(unsigned buffer:{0u,fixture+16,fixture+17,fixture+18,fixture+256,stack-300})
                check(entry,fixture+16,buffer,1,[&](State& a){a.wr<4>(20,0xa5a50000|capacity);});
        for(auto entry:{0x107156f0u,0x107158a0u})for(int day:{-32768,-1,0,1,31,32,32767})for(int month:{-128,-1,0,1,11,12,127})
            check(entry,fixture+32,0xa5a50000|std::uint16_t(day),0,[&](State& a){a.wr<4>(20,0xa5a50000|std::uint8_t(month));a.wr<4>(24,2000);a.wr<4>(28,0xffff);});
        for(auto entry:format_entries)for(unsigned flag:{0u,1u,128u,255u})for(unsigned phase:{0u,2u,255u})
            check(entry,fixture+16,fixture+256,phase,[&](State& a){
                a.memory.write<1>(fixture+20,flag);
                for(auto offset:{0x3fa8u,0x3fa4u}){
                    const auto old=std::uint32_t(a.memory.read<4>(toc-offset));auto target=fixture+4096+(offset==0x3fa4?64:0);
                    a.memory.copy_in(target,a.memory.copy_out(old,24));a.memory.write<2>(target+14,220);a.memory.write<4>(toc-offset,target);
                }
            });
        for(auto entry:{0x10718700u,0x107187d0u,0x107188a0u,0x107189d0u})for(unsigned selected:{0u,1u,6u})for(unsigned fallback:{0u,1u})
            check(entry,selected,0,0,[&](State& a){
                const auto slot=entry==0x10718700?0x3fd0u:entry==0x107187d0?0x3fd8u:entry==0x107188a0?0x3fe0u:0x3fe8u;
                const auto size=entry<0x107188a0?28:48;
                const auto old=std::uint32_t(a.memory.read<4>(toc-slot));const auto table=fixture+6144+((slot-0x3fd0)/8)*64;a.memory.copy_in(table,a.memory.copy_out(old,size));
                const auto fall=entry==0x10718700?0x107187a8u:entry==0x107187d0?0x10718878u:entry==0x107188a0?0x107189a0u:0x10718accu;
                a.memory.write<4>(table+selected*4,fallback?fall:a.memory.read<4>(old+4));a.memory.write<4>(toc-slot,table);
                a.memory.write<4>(toc-(slot-4),fixture+256);
            });
        const std::array<unsigned,6> scratch_slots{0x3fc4,0x3fc8,0x3fcc,0x3fd4,0x3fdc,0x3fe4};
        for(auto entry:format_entries)for(unsigned slot:scratch_slots)for(int overlap:{-1,0,1,2,4})for(unsigned phase:{1u,255u})
            check(entry,fixture+16,fixture+256,phase,[&](State& a){
                for(unsigned i=0;i<scratch_slots.size();++i)a.memory.write<4>(toc-scratch_slots[i],fixture+12288+i*256);
                a.memory.write<4>(toc-slot,fixture+16+overlap);
            });
        require(bool(evidence),"Cannot finalize private presentation characterization");
        std::cout<<cases<<(compare_semantic?" original/semantic helper":" original presentation")<<" cases: phase/mode widths, invalid name indices and isolated language classifications; raw ABI and private output evidence retained\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
