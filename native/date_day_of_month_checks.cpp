#include "calendar.hpp"
#include "calendar_adapter.hpp"
#include "callbacks.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <unistd.h>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, entry = 0x107182f0;
constexpr std::array<std::int16_t,12> common{-1,30,58,89,119,150,180,211,242,272,303,333};
constexpr std::array<std::int16_t,12> leap{-1,30,59,90,120,151,181,212,243,273,304,334};
void require(bool test, const std::string& message) { if (!test) throw std::runtime_error(message); }
struct NoWrites final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Day-of-month mapped memory"); }
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override { throw std::runtime_error("Day-of-month wrote memory"); }
    void cleared(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Day-of-month cleared memory"); }
};
void same_memory(const State& a, const State& b) {
    require(a.memory.mapped_ranges() == b.memory.mapped_ranges(), "Day-of-month mapped ranges differ");
    for (const auto& [start,size] : a.memory.mapped_ranges())
        for (std::uint64_t offset=0; offset<size; offset+=65536) {
            const auto length=std::size_t(std::min<std::uint64_t>(size-offset,65536));
            require(a.memory.copy_out(start+std::uint32_t(offset),length)==
                    b.memory.copy_out(start+std::uint32_t(offset),length), "Day-of-month entire memory differs");
        }
}
void initialize(State& s, unsigned year, int day, unsigned flag, unsigned index) {
    const auto stack=s.r<4>(4);
    constexpr std::uint32_t toc=0x108bba90;
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    for (unsigned r=0;r<32;++r) s.wr<4>(r*4,0x12340000+r);
    s.wr<4>(4,stack); s.wr<4>(8,toc); s.wr<4>(4128,return_sentinel);
    s.memory.copy_in(fixture,std::vector<std::uint8_t>(256,0xa5));
    s.memory.write<2>(fixture+16,std::uint16_t(day)); s.memory.write<2>(fixture+18,year);
    s.memory.write<1>(fixture+20,flag); s.wr<4>(12,fixture+16);
    s.wr<1>(2304,index%2?0xf9:0x06); s.wr<1>(1024,index%2); s.wr<1>(1027,(index/2)%2);
    s.pc=entry; s.executed=0; s.budget=160; s.native_calls=0;
    s.calendar_calls={}; s.date_difference_calls=0; s.date_ordering_calls=0; s.date_before_calls=0; s.date_day_of_month_calls=0;
}
}
int main(int argc,char** argv) {
    if (argc!=2) return 2;
    try {
        auto a=std::make_unique<State>(), b=std::make_unique<State>();
        load_program(*a,argv[1]); load_program(*b,argv[1]); a->calendar_original=true;
        for (auto* s:{a.get(),b.get()}) s->memory.map(fixture,256);
        unsigned count=0, oracle=0;
        auto equivalent=[&] {
            if(a->registers!=b->registers) {
                for(unsigned i=0;i<a->registers.size();++i) if(a->registers[i]!=b->registers[i])
                    std::cerr<<"register byte "<<i<<" original="<<unsigned(a->registers[i])<<" modern="<<unsigned(b->registers[i])<<'\n';
                throw std::runtime_error("Day-of-month direct ABI mismatch case="+std::to_string(count));
            }
            require(a->unique==b->unique && a->pc==b->pc,"Day-of-month temporaries/PC differ"); same_memory(*a,*b);
            require(a->allocated==b->allocated && a->available==b->available && a->heap_end==b->heap_end &&
                    a->memory_error==b->memory_error && !a->native_calls && !b->native_calls,"Day-of-month allocator/import effects differ");
        };
        auto check=[&](unsigned year,int day,unsigned flag,unsigned mutated=0,bool relocated=false) {
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,year,day,flag,count);
                if(relocated) {
                    s->wr<4>(8,fixture+0x4000);
                    s->memory.write<4>(fixture+0x4000-0x3fa8,0x108e14ac);
                    s->memory.write<4>(fixture+0x4000-0x3fa4,0x108e14c4);
                }
            }
            std::array<std::int16_t,12> thresholds{};
            const auto slot=std::uint32_t(a->r<4>(8))-(flag?0x3fa4u:0x3fa8u);
            const auto table=std::uint32_t(a->memory.read<4>(slot));
            if(mutated) for(auto* s:{a.get(),b.get()}) {
                s->memory.write<4>(slot,fixture+64);
                for(unsigned i=0;i<12;++i) s->memory.write<2>(fixture+64+i*2,std::uint16_t(mutated==1?(i%3?int(i)*7-8:300-int(i)*2):(i%2?32767:-32768)));
            }
            const auto effective_table=std::uint32_t(a->memory.read<4>(slot));
            for(unsigned i=0;i<12;++i) thresholds[i]=cm::calendar::narrow_day(int(a->memory.read<2>(effective_table+i*2)));
            if(!mutated) require(thresholds==(flag?leap:common),"Original table values changed");
            NoWrites observer; std::vector<std::pair<std::uint32_t,unsigned>> ar,br;
            a->memory.read_trace=&ar; b->memory.read_trace=&br;
            a->memory.observer=&observer; b->memory.observer=&observer;
            run(*a); run(*b);
            a->memory.read_trace=nullptr; b->memory.read_trace=nullptr;
            a->memory.observer=nullptr; b->memory.observer=nullptr;
            if(ar!=br) {
                for(const auto& [address,width]:ar) std::cerr<<"original read "<<hex_address(address)<<"/"<<width<<'\n';
                for(const auto& [address,width]:br) std::cerr<<"modern read "<<hex_address(address)<<"/"<<width<<'\n';
                throw std::runtime_error("Day-of-month ordered reads differ case="+std::to_string(count));
            }
            require(ar.size()>=4 && ar[0]==std::pair(fixture+20,1u) && ar[1]==std::pair(slot,4u) &&
                    ar[2]==std::pair(effective_table+22,2u) && ar[3]==std::pair(fixture+16,2u),"Day-of-month initial read order differs");
            const auto recognized=cm::calendar::month_index(std::int16_t(day),thresholds);
            std::vector<std::pair<std::uint32_t,unsigned>> expected_reads{{fixture+20,1},{slot,4}};
            for(int i=11;i>=0;--i) {
                expected_reads.emplace_back(effective_table+unsigned(i)*2,2);
                expected_reads.emplace_back(fixture+16,2);
                if(i==recognized) { expected_reads.emplace_back(effective_table+unsigned(i)*2,2); break; }
            }
            require(ar==expected_reads,"Day-of-month complete lazy reads/reload differ");
            require(b->r<4>(20)==effective_table && b->r<4>(24)==std::uint32_t(day) &&
                    b->r<4>(16)==effective_table+(recognized<0?std::uint32_t(-2):unsigned(recognized)*2) &&
                    b->r<4>(28)==std::uint32_t(recognized) && b->r<4>(4132)==unsigned(recognized+1),
                    "Day-of-month reverse loop ABI differs");
            equivalent();
            require(b->date_day_of_month_calls==1 && !b->date_difference_calls && !b->date_ordering_calls && !b->date_before_calls &&
                    b->calendar_calls==std::array<std::uint64_t,2>{},"Day-of-month dispatch/counters differ");
            const auto result=std::bit_cast<std::int32_t>(std::uint32_t(a->r<4>(12)));
            require(result==cm::calendar::day_of_month(std::int16_t(day),thresholds),"Day-of-month semantic result differs");
            if(!mutated && year>=1 && year<=2100 && flag==unsigned(cm::calendar::is_leap(std::uint16_t(year))) &&
               day>=0 && day<int(cm::calendar::year_length(std::uint16_t(year)))) {
                using namespace std::chrono;
                const year_month_day date{sys_days(std::chrono::year(int(year))/January/1)+days(day)};
                require(result==int(unsigned(date.day())),"Day-of-month independent Gregorian oracle differs"); ++oracle;
            }
            // Pointer and contents are mutable mapped data, restored privately.
            for(auto* s:{a.get(),b.get()}) s->memory.write<4>(slot,table);
            ++count;
        };
        for(unsigned year:{0u,1u,4u,1900u,1999u,2000u,2001u,2004u,2100u,32767u,65535u})
            for(unsigned flag:{0u,1u,128u,255u}) {
                for(int day:{-32768,-2,-1,0,1,58,59,60,364,365,366,367,32767}) check(year,day,flag);
                for(auto threshold:(flag?leap:common))
                    for(int delta:{0,1}) check(year,int(threshold)+delta,flag);
            }
        for(unsigned year:{1u,1900u,2000u,2100u})
            for(int day=0;day<int(cm::calendar::year_length(std::uint16_t(year)));++day)
                check(year,day,unsigned(cm::calendar::is_leap(std::uint16_t(year))));
        for(unsigned flag:{0u,1u,128u,255u})
            for(int day:{-32768,-1,0,59,100,299,32767}) {check(2000,day,flag,true); check(1900,day,flag,false,true); check(2000,day,flag,2);}
        for(const auto& thresholds:{common,leap})
            for(unsigned bits=0;bits<=65535;++bits) {
                const auto day=cm::calendar::narrow_day(bits); int expected=-1;
                for(int i=0;i<12;++i) if(day>thresholds[i]) expected=i;
                require(cm::calendar::day_of_month(day,thresholds)==(expected<0?-1:cm::calendar::day_from_threshold(day,thresholds[expected])),"Day-of-month pure signed domain differs");
            }
        // Map only the fields and threshold actually read. Year, stack,
        // other table entries and the unused flag-selected slot stay unmapped.
        {
            auto sparse_a=std::make_unique<State>(), sparse_b=std::make_unique<State>();
            sparse_a->calendar_original=true;
            constexpr std::uint32_t date=0x31000000, slot=0x32000000, table=0x33000000;
            std::vector<std::pair<std::uint32_t,unsigned>> ar,br;
            NoWrites observer;
            for(auto* s:{sparse_a.get(),sparse_b.get()}) {
                s->registers.fill(0x39); s->unique.fill(0x82);
                s->memory.map(date,2); s->memory.map(date+4,1);
                s->memory.map(slot,4); s->memory.map(table+22,2);
                s->memory.write<2>(date,1); s->memory.write<1>(date+4,0);
                s->memory.write<4>(slot,table); s->memory.write<2>(table+22,0);
                s->wr<4>(4,0x700fff00); s->wr<4>(8,slot+0x3fa8);
                s->wr<4>(12,date); s->wr<4>(4128,return_sentinel);
                s->pc=entry; s->budget=160; s->memory.observer=&observer;
                s->memory.read_trace=s==sparse_a.get()?&ar:&br;
                run(*s); s->memory.read_trace=nullptr; s->memory.observer=nullptr;
            }
            const std::vector<std::pair<std::uint32_t,unsigned>> reads{
                {date+4,1},{slot,4},{table+22,2},{date,2},{table+22,2}};
            require(ar==reads && br==reads,"Sparse day-of-month reads differ");
            require(sparse_a->registers==sparse_b->registers && sparse_a->unique==sparse_b->unique &&
                    sparse_a->pc==sparse_b->pc && sparse_b->r<4>(12)==1 &&
                    sparse_b->date_day_of_month_calls==1,"Sparse day-of-month ABI differs");
            same_memory(*sparse_a,*sparse_b);
        }
        for(unsigned day:{0u,59u,334u,65535u}) {
            std::array<std::uint32_t,2> results{}; unsigned index=0;
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,2000,int(cm::calendar::narrow_day(day)),1,count);
                s->memory.write<4>(fixture,entry); s->memory.write<4>(fixture+4,s->r<4>(8));
                const auto registers=s->registers; const auto unique=s->unique;
                results[index++]=call_original(*s,fixture,std::array<std::uint32_t,1>{fixture+16});
                require(s->registers==registers && s->unique==unique,"Day-of-month callback restoration differs");
            }
            require(results[0]==results[1] && b->date_day_of_month_calls==1,"Day-of-month callback return differs"); same_memory(*a,*b);
        }
        // Actual caller instructions with controlled inputs: synthetic evidence.
        for(auto pc:{0x10097f84u,0x10097fa0u,0x101450ccu,0x10363940u,0x1036395cu,0x103a76f8u,0x1000a494u}) {
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,2000,59,1,count); s->pc=pc; s->wr<4>(4132,entry);
                require(dispatch_page(*s) && s->pc==entry,"Day-of-month caller missed dispatch");
                require(dispatch_page(*s),"Day-of-month target unavailable");
            }
            require(b->pc==(pc==0x1000a494u?return_sentinel:pc+4) && b->r<4>(12)==29 && b->date_day_of_month_calls==1,"Day-of-month caller result differs"); equivalent();
        }
        auto pattern=(std::filesystem::temp_directory_path()/"cm-day-of-month-checks-XXXXXX").string();
        require(mkdtemp(pattern.data())!=nullptr,"Cannot create private day-of-month stream directory");
        const auto stream=std::filesystem::path(pattern)/"original-day-of-month.gz";
        initialize(*a,2000,59,1,count); open_day_of_month_audit(stream,true); run(*a); close_day_of_month_audit();
        initialize(*b,2000,59,1,count); open_day_of_month_audit(stream,false); run(*b); close_day_of_month_audit(true); equivalent();
        initialize(*b,2000,59,1,count); open_day_of_month_audit(stream,false,true);
        bool rejected=false;
        try {run(*b);} catch(const std::runtime_error& error) {
            rejected=std::string(error.what()).find("day-of-month edge 0 at 0x107182f0 field=day-of-month return byte=48")!=std::string::npos;
        }
        close_day_of_month_audit(); require(rejected,"Day-of-month first-return negative was accepted");
        initialize(*b,2000,59,1,count); run(*b);
        require(b->r<4>(12)==29,"Private day-of-month perturbation leaked into normal execution");
        open_day_of_month_audit(stream,false); rejected=false;
        try {close_day_of_month_audit(true);} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Unread day-of-month edges were accepted");
        const auto corrupted=std::filesystem::path(pattern)/"corrupted.gz";
        std::filesystem::copy_file(stream,corrupted);
        {
            std::fstream file(corrupted,std::ios::in|std::ios::out|std::ios::binary);
            file.seekg(-8,std::ios::end); char byte{}; file.read(&byte,1); byte^=1;
            file.seekp(-8,std::ios::end); file.write(&byte,1);
        }
        rejected=false;
        try {initialize(*b,2000,59,1,count); open_day_of_month_audit(corrupted,false); run(*b); close_day_of_month_audit(true);}
        catch(const std::runtime_error&) {rejected=true;}
        try {close_day_of_month_audit();} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Day-of-month checksum damage was accepted");
        std::filesystem::resize_file(stream,std::filesystem::file_size(stream)/2); rejected=false;
        try {initialize(*b,2000,59,1,count); open_day_of_month_audit(stream,false); run(*b); close_day_of_month_audit(true);}
        catch(const std::runtime_error&) {rejected=true;}
        try {close_day_of_month_audit();} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Day-of-month truncated body was accepted");
        std::filesystem::resize_file(stream,4); rejected=false;
        try {open_day_of_month_audit(stream,false);} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Truncated day-of-month header was accepted");
        std::filesystem::remove_all(pattern);
        std::cout<<count<<" direct original/modern day-of-month cases; "<<oracle<<" Gregorian oracle cases; 131072 pure signed-day checks; sparse mapped fields; full registers/temporaries/PC/mapped memory/allocator, ordered reads, no writes/imports/RNG; mutable tables/TOC, synthetic callers, indirect dispatch and callbacks\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
