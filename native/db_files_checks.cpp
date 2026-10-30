// Independent generated reference with scripted stream, allocation and path
// routines. No game file, career or window is opened.
#include "db_files_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include "guest_values.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <unistd.h>
#if defined(CM_DB_FILES_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif
using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool db_files_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, header=data+0x1000, shared=data+0x2000, message=data+0x3000,
    success=data+0x3800, version_text=data+0x3900, path_text=data+0x3a00, records=data+0x10000,
    file_source=data+0x30000, allocated=data+0x50000, trace_at=data+0x80000, stack=0x71010000,
    sentinel=0xfffffff0, module_end=0x105e0240, pool=0x108435dc;
constexpr std::uint32_t file_handle=0x60001800, cached_handle=0x60001900, written_handle=0x60001a00;
enum class Kind{open,load,convert,old,save};
struct Routine{std::uint32_t entry;Kind kind;std::uint32_t record_size;std::int32_t current;};
constexpr std::array<Routine,59> routines{{
    {0x105d71b0,Kind::open,0,0},{0x105d7200,Kind::open,0,0},
    {0x105d7250,Kind::load,0xc8,1},{0x105d7530,Kind::convert,0xc8,0},{0x105d76e0,Kind::save,0xc8,0},
    {0x105d7750,Kind::load,0x128,2},{0x105d7a30,Kind::convert,0x128,0},{0x105d8560,Kind::save,0x128,0},
    {0x105d85d0,Kind::load,0x38,2},{0x105d88b0,Kind::convert,0x38,0},{0x105d8ad0,Kind::save,0x38,0},
    {0x105d8b40,Kind::load,0x50,1},{0x105d8e20,Kind::convert,0x50,0},{0x105d8ef0,Kind::save,0x50,0},
    {0x105d8f60,Kind::load,0x248,2},{0x105d9290,Kind::convert,0x248,0},{0x105d9710,Kind::old,0x248,0},{0x105da670,Kind::save,0x248,0},
    {0x105da6e0,Kind::load,0x70,2},{0x105da9c0,Kind::convert,0x70,0},{0x105dac10,Kind::save,0x70,0},
    {0x105dac80,Kind::load,0x9d,1},{0x105daf60,Kind::convert,0x9d,0},
    {0x105db230,Kind::load,0x44,2},{0x105db530,Kind::convert,0x44,0},{0x105db6c0,Kind::old,0x44,0},{0x105dc160,Kind::save,0x44,0},
    {0x105dc1d0,Kind::load,0x48,2},{0x105dc4d0,Kind::convert,0x48,0},{0x105dc710,Kind::old,0x48,0},{0x105dd910,Kind::save,0x48,0},
    {0x105dd980,Kind::load,0x34,1},{0x105ddc60,Kind::convert,0x34,0},{0x105ddd20,Kind::save,0x34,0},
    {0x105ddd90,Kind::load,0x14,1},{0x105de070,Kind::convert,0x14,0},{0x105de0e0,Kind::save,0x14,0},
    {0x105de150,Kind::load,0x6c,2},{0x105de450,Kind::convert,0x6c,0},{0x105de550,Kind::old,0x6c,0},{0x105de9b0,Kind::save,0x6c,0},
    {0x105dea20,Kind::load,0x1c,1},{0x105ded00,Kind::convert,0x1c,0},{0x105ded70,Kind::save,0x1c,0},
    {0x105dede0,Kind::load,0x68,2},{0x105df0c0,Kind::convert,0x68,0},{0x105df1a0,Kind::save,0x68,0},
    {0x105df210,Kind::load,0x3c,1},{0x105df4f0,Kind::convert,0x3c,0},{0x105df5c0,Kind::save,0x3c,0},
    {0x105df630,Kind::load,0x2c,1},{0x105df910,Kind::convert,0x2c,0},{0x105dfa10,Kind::save,0x2c,0},
    {0x105dfa80,Kind::load,0x3c,1},{0x105dfd60,Kind::convert,0x3c,0},{0x105dfdf0,Kind::save,0x3c,0},
    {0x105dfe60,Kind::load,0x2c,1},{0x105e0140,Kind::convert,0x2c,0},{0x105e01d0,Kind::save,0x2c,0},
}};
struct Case {
    Routine routine=routines[2];
    std::uint32_t count=2, packed=0, marker=0xf1fffff1, pattern=0x93;
    std::int32_t version=0;
    bool cached=false, no_allocation=false, no_written_copy=false, null_open=false, clobber=false;
    unsigned short_read_at=0, short_write_at=0;
    bool zero_bytes=false;
};
Case active;
bool original_mode=false, production_mode=false, ownership_mode=false;
unsigned calls=0, reads=0, writes=0;
std::uint8_t fill(std::uint32_t seed,std::uint32_t i){return std::uint8_t(seed+i*7+(i>>8)*13);}
// The all-zero and all-one patterns stay flat; the others vary per read.
std::uint8_t stream_byte(std::uint32_t read,std::uint32_t i){
    const auto pattern=active.pattern;
    return pattern==0||pattern==0xff?std::uint8_t(pattern):fill(pattern+read,i);
}
void trace(Cpu& c,std::uint32_t target,bool registers=true) {
    if(++calls>1500)throw std::runtime_error("fixture call bound");
    auto size=c.u32(trace_at);
    const auto add=[&](std::uint32_t value){c.put32(trace_at+4+size++*4,value);};
    add(target);add(c.lr());
    if(registers){
        for(unsigned r=3;r<=10;++r)add(c.r(r));
        for(unsigned r=13;r<32;++r)add(c.r(r));
        for(unsigned f=0;f<8;++f)add(c.cr(f));
    } else for(unsigned r=3;r<=5;++r)add(c.r(r));
    c.put32(trace_at,size);
}
void clobber(Cpu& c) {
    if(!active.clobber)return;
    for(unsigned r=4;r<=10;++r)c.set(r,0x41000000+calls*0x100+r);
    c.compare(0,17,29);c.compare(1,3,3);
}
std::uint32_t finish_stream(Cpu& c,std::uint32_t answer){clobber(c);c.set(3,answer);c.s.pc=c.lr();return answer;}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if(target>=routines.front().entry && target<module_end) {
        if(original_mode)return db_files_reference(s);
        if(production_mode)return false;
        if(db_files_lists(target)){db_files_run_rewrite(s,target);return true;}
        return false;
    }
    if(target==0x1077fc28) {
        // BlockMoveData through its vector: the TOC save and the copy. The
        // bridge has no guest frame, so only its arguments are observable.
        if(ownership_mode)s.budget=s.executed;
        trace(c,target,false);
        c.put32(c.sp()+20,c.r(2));
        std::vector<std::uint8_t> bytes(c.r(5));
        for(std::uint32_t i=0;i<c.r(5);++i)bytes[i]=c.u8(c.r(3)+i);
        for(std::uint32_t i=0;i<c.r(5);++i)c.put8(c.r(4)+i,bytes[i]);
        // The vector leaves the import's TOC in r2; the caller reloads its own.
        c.set(2,0x77000000+calls);
        if(active.clobber)for(unsigned r=3;r<=5;++r)c.set(r,0x42000000+calls*0x100+r);
        c.s.pc=c.lr();return true;
    }
    if(run_leaf(s))return true;
    constexpr std::array program{0x10739890u,0x10739b40u,0x10739de0u,0x10746280u,0x10746470u,0x107464d0u,
        0x107465a0u,0x10747a80u,0x10747cc0u,0x10768810u,0x10769b60u,0x1076f7d0u,0x10775130u,0x10775170u,
        0x10775fe0u,0x10776360u,0x107766f0u,0x1077b720u,0x1077dbf0u,0x1077dc10u,0x1077dc60u};
    if(std::ranges::find(program,target)==program.end())return false;
    if(ownership_mode)s.budget=s.executed;
    trace(c,target);
    const auto a3=c.r(3),a4=c.r(4),a5=c.r(5),a6=c.r(6);
    switch(target) {
    case 0x10739de0: {
        if(active.null_open)return finish_stream(c,0),true;
        const bool write=a4==pool+3;
        finish_stream(c,write?(active.no_written_copy?0:written_handle):(active.cached?cached_handle:0));
        return true;
    }
    case 0x10775fe0: case 0x10747a80: {
        const auto total=a4*a5;
        ++reads;
        const bool small=active.short_read_at==reads;
        const auto done=small&&a5!=0?a5-1:a5;
        // One-byte reads can all return zero, so that the version 1 readers'
        // zero-byte substitution is taken.
        const bool zero=total==1 && active.zero_bytes;
        for(std::uint32_t i=0;i<done*a4;++i)c.put8(a3+i,zero?0:stream_byte(reads,i));
        return finish_stream(c,done),true;
    }
    case 0x10776360: case 0x10747cc0: {
        ++writes;
        std::uint32_t sum=0;
        for(std::uint32_t i=0;i<a4*a5;++i)sum=sum*31+c.u8(a3+i);
        c.put32(trace_at+0x3f000+writes%64*4,sum);
        const auto done=active.short_write_at==writes?(a5==0?0:a5-1):a5;
        return finish_stream(c,done),true;
    }
    case 0x107464d0: return finish_stream(c,active.marker),true;
    case 0x10775130: return finish_stream(c,active.no_allocation?0:allocated),true;
    case 0x10746280: return finish_stream(c,path_text),true;
    case 0x10768810: return finish_stream(c,version_text),true;
    case 0x10739890: c.put32(a6,0x4e414d45);c.put8(a6+4,0);if(c.r(7))c.put32(c.r(7),0x2e657874);return finish_stream(c,0),true;
    case 0x10739b40: c.put32(a3,0x50415448);c.put8(a3+4,0);return finish_stream(c,a3),true;
    case 0x1077b720: c.put32(a3,c.r(8));return finish_stream(c,4),true;
    default: return finish_stream(c,0),true;
    }
}
void initialize(Cpu& c) {
    // The real exit does not come back after a failed allocation. The stub
    // does, so both sides continue over a mapped null buffer and are compared.
    if(active.no_allocation)c.s.memory.map(0,0x4000);
    c.s.memory.map(data,0xc0000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0xc0000,0xa5));
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    for(unsigned f=0;f<32;++f)c.set_f(f,1.25+f);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    c.put32(c.r(2)-0x7ba8,message);c.put32(c.r(2)-0x7bac,success);c.put32(c.r(2)-0x7bb8,shared);c.put32(c.r(2)-0x42e8,pool);
    c.put8(success,1);c.put32(version_text,0x76312e30);c.put8(version_text+4,0);
    c.put32(path_text,0x444154);c.put8(header,0x48);c.put8(header+1,0);
    c.put32(header+0x38,active.count);c.put32(header+0x40,std::uint32_t(active.version));c.put32(header+0x3c,0x12345678);
    for(std::uint32_t i=0;i<0x4000;++i){c.put8(file_source+i,active.pattern==0||active.pattern==0xff?std::uint8_t(active.pattern):fill(active.pattern,i));c.put8(records+i,fill(active.pattern^0x5a,i));}
    c.put32(trace_at,0);
    switch(active.routine.kind) {
    case Kind::open: c.set(3,data+0x1100);c.set(4,data+0x1200);break;
    case Kind::load: c.set(3,file_handle);c.set(4,active.packed);c.set(5,records);c.set(6,header);break;
    case Kind::convert: c.set(3,records);c.set(4,file_source);c.set(5,active.count);break;
    case Kind::old: c.set(3,file_handle);c.set(4,active.packed);c.set(5,records);c.set(6,active.count);break;
    case Kind::save: c.set(3,file_handle);c.set(4,active.packed);c.set(5,records);c.set(6,active.count);break;
    }
    c.s.budget=c.s.executed+2000000;
}
void original(State& s,std::uint32_t entry) {
    original_mode=true;calls=reads=writes=0;s.pc=entry;run_until(s,sentinel);
}
void rewrite(State& s,std::uint32_t entry) {
    original_mode=false;calls=reads=writes=0;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}
    else db_files_run_rewrite(s,entry);
#if defined(CM_DB_FILES_COVERAGE)
    const auto profile=std::string(CM_DB_FILES_PROFILE_DIR)+"/check-"+std::to_string(getpid())+".profraw";
    __llvm_profile_set_filename(profile.c_str());
    if(__llvm_profile_write_file()!=0)throw std::runtime_error("Cannot flush DB_FILES child coverage");
#endif
}
void corrupt(State& s,std::uint32_t entry){rewrite(s,entry);Cpu c(s);c.put8(records,c.u8(records)^1);}
struct Observed final:MemoryObserver {
    void mapped(std::uint32_t,std::uint64_t)override{}
    void written(std::uint32_t,const std::uint8_t*,std::size_t)override{}
    void cleared(std::uint32_t,std::uint64_t)override{}
};
std::vector<Case> build_cases() {
    std::vector<Case> cases;
    const auto add=[&](const Case& t){cases.push_back(t);};
    for(const auto& routine:routines) {
        Case base;base.routine=routine;base.version=routine.current;
        switch(routine.kind) {
        case Kind::open:
            add(base);{Case t=base;t.null_open=true;add(t);t=base;t.cached=true;add(t);}
            break;
        case Kind::convert:
            for(std::uint32_t count:{0u,1u,3u})for(std::uint32_t pattern:{0x93u,0u,0xffu,0x7fu}) {
                Case t=base;t.count=count;t.pattern=pattern;add(t);
            }
            break;
        case Kind::save:
            for(std::uint32_t packed:{0u,1u,0x80u,0x100u})for(std::uint32_t count:{0u,1u,3u})for(unsigned bad:{0u,1u}) {
                Case t=base;t.packed=packed;t.count=count;t.short_write_at=bad;add(t);
            }
            break;
        case Kind::load: {
            // Routes selected by version, for a Mac file and a packed one.
            for(std::int32_t version:{-1,0,1,2,3,0x7fffffff})for(std::uint32_t packed:{0u,1u,0x80u,0x100u}) {
                Case t=base;t.version=version;t.packed=packed;add(t);
            }
            // A Windows file with and without a converted copy, every failure.
            for(std::uint32_t count:{0u,1u,3u}) for(std::uint32_t marker:{0xf1fffff1u,0u,0xf1fffff0u}) {
                Case t=base;t.count=count;t.marker=marker;add(t);
                t.cached=true;add(t);
                t=base;t.count=count;t.marker=marker;t.no_allocation=true;add(t);
                t=base;t.count=count;t.marker=marker;t.no_written_copy=true;add(t);
                t=base;t.count=count;t.marker=marker;t.short_read_at=1;add(t);
                t=base;t.count=count;t.marker=marker;t.cached=true;t.short_read_at=1;add(t);
                t=base;t.count=count;t.marker=marker;t.short_write_at=1;add(t);
            }
            // Every version from a Windows file: the allocation or the open
            // copy is left behind on the older and error routes.
            for(std::int32_t version:{-1,0,1,2,3})for(std::uint32_t marker:{0u,0xf1fffff0u}) {
                Case t=base;t.version=version;t.marker=marker;add(t);
                t.cached=true;add(t);
                t=base;t.version=version;t.marker=marker;t.no_allocation=true;add(t);
                t=base;t.version=version;t.marker=marker;t.count=0;add(t);
            }
            for(std::uint32_t pattern:{0u,0xffu,0x7fu}) {
                Case t=base;t.pattern=pattern;t.marker=0;add(t);
                t=base;t.pattern=pattern;add(t);
            }
            break;
        }
        case Kind::old: {
            for(std::uint32_t packed:{0u,1u,0x80u,0x100u})for(std::uint32_t count:{0u,1u,2u})for(std::uint32_t marker:{0xf1fffff1u,0u}) {
                Case t=base;t.packed=packed;t.count=count;t.marker=marker;add(t);
                t.zero_bytes=true;add(t);
            }
            // Fail each read of one record, then the second record's first.
            unsigned fields=routine.record_size==0x248?133:routine.record_size==0x44?36:routine.record_size==0x48?62:11;
            for(unsigned at=1;at<=fields+1;++at)for(std::uint32_t packed:{0u,1u}) {
                Case t=base;t.packed=packed;t.count=2;t.short_read_at=at;add(t);
            }
            for(std::uint32_t pattern:{0u,0xffu,0x7fu}) {
                Case t=base;t.pattern=pattern;t.count=2;add(t);
            }
            break;
        }
        }
    }
    return cases;
}
}
int main(int argc,char** argv) {
    if(argc<2)return 2;
    const bool production_available=argc<3 || std::string(argv[2])!="--no-production";
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        const auto cases=build_cases();
        unsigned completed=0,failures=0;std::map<std::uint32_t,std::array<unsigned,2>> counts;
        for(bool production:{false,true}) {
            if(production&&!production_available)break;
            production_mode=production;
            for(const auto& test:cases)for(bool clobbered:{false,true}) {
                if(failures>=8)break;
                active=test;active.clobber=clobbered;
                auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c);
                Observed observer;if(production)s->memory.observer=&observer;
                auto differences=compare_routine_call(*s,test.routine.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                if(!differences.empty()) {
                    std::cout<<"DIFFERS "<<hex_address(test.routine.entry)<<" production="<<production<<" count="<<test.count<<" packed="<<test.packed<<" version="<<test.version<<" marker="<<test.marker<<" pattern="<<test.pattern<<" cached="<<test.cached<<" noalloc="<<test.no_allocation<<" nowrite="<<test.no_written_copy<<" shortr="<<test.short_read_at<<" shortw="<<test.short_write_at<<" zero="<<test.zero_bytes<<" clobber="<<clobbered<<'\n';
                    unsigned shown=0;for(const auto& d:differences)if(shown++<12)std::cout<<"  "<<d<<'\n';
                    ++failures;
                } else {++completed;++counts[test.routine.entry][production?1:0];}
            }
            if(failures)break;
        }
        production_mode=false;active=Case{};active.version=active.routine.current;
        auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c);
        if(compare_routine_call(*s,active.routine.entry,original,corrupt,RoutineChecker::result_r3).empty()){++failures;std::cout<<"FAIL corruption control\n";}
        else std::cout<<"ok deliberate memory corruption detected\n";
        unsigned ownership=0;
        if(production_available)for(const auto& routine:routines) {
            active=Case{};active.routine=routine;active.version=routine.current;
            auto normal=cm::semantic_io_checks::clone(*base);Cpu nc(*normal);initialize(nc);
            normal->budget=normal->executed;Observed observer;normal->memory.observer=&observer;production_mode=true;ownership_mode=true;
            try{rewrite(*normal,routine.entry);++ownership;}catch(const std::exception& e){++failures;std::cout<<"FAIL production ownership "<<hex_address(routine.entry)<<' '<<e.what()<<'\n';}
            ownership_mode=false;production_mode=false;
            auto ref=cm::semantic_io_checks::clone(*base);Cpu rc(*ref);initialize(rc);ref->budget=ref->executed;bool rejected=false;
            try{original(*ref,routine.entry);}catch(const std::exception& e){rejected=std::string(e.what())=="Native instruction budget exhausted" && ref->pc==routine.entry;}
            if(!rejected){++failures;std::cout<<"FAIL original budget control "<<hex_address(routine.entry)<<'\n';}
        }
        for(const auto& [entry,n]:counts)std::cout<<hex_address(entry)<<" returned "<<n[0]<<" controlled + "<<n[1]<<" production\n";
        std::cout<<ownership<<"/59 observed production controls; "<<completed<<" returned comparisons; "<<failures<<" failures\n";
        return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
