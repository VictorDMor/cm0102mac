#include "date_subsystem_audit.hpp"
#include "date_subsystem_adapter.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <zlib.h>

namespace cm::aot {
namespace {
using Bytes=std::vector<std::uint8_t>;
void number(Bytes& bytes,std::uint64_t value,unsigned width) {
    for(unsigned i=width;i;--i)bytes.push_back(std::uint8_t(value>>((i-1)*8)));
}
void raw(Bytes& bytes,const auto& value) {bytes.insert(bytes.end(),value.begin(),value.end());}
struct Frame {std::uint32_t entry;std::uint64_t index;Bytes effects;};
struct DateObserver final:MemoryObserver {
    std::vector<Frame*> frames;
    void event(unsigned kind,std::uint32_t address,std::uint64_t size,const std::uint8_t* bytes=nullptr) {
        for(auto* frame:frames){number(frame->effects,kind,1);number(frame->effects,address,4);number(frame->effects,size,8);
            if(bytes)frame->effects.insert(frame->effects.end(),bytes,bytes+size);}
    }
    void mapped(std::uint32_t address,std::uint64_t size) override {event(1,address,size);}
    void written(std::uint32_t address,const std::uint8_t* bytes,std::size_t size) override {event(2,address,size,bytes);}
    void cleared(std::uint32_t address,std::uint64_t size) override {event(3,address,size);}
    void read(std::uint32_t address,const std::uint8_t* bytes,unsigned size) override {event(4,address,size,bytes);}
};
DateObserver observer;
gzFile stream=nullptr;
bool recording=false;
std::uint32_t perturb=0;
bool perturbed=false;
std::uint64_t next_index=0;
constexpr std::array<char,8> magic{'C','M','D','A','T','0','0','1'};
std::size_t entry_index(std::uint32_t entry) {
    const auto found=std::find(date_subsystem_entries.begin(),date_subsystem_entries.end(),entry);
    if(found==date_subsystem_entries.end())throw std::runtime_error("Unknown semantic Date entry "+hex_address(entry));
    return std::size_t(found-date_subsystem_entries.begin());
}
// Each begin/end chunk is length-framed and CRC-protected by gzip. Nesting is
// explicit and strict. Historical streams keep their original encodings.
void chunk(unsigned kind,const Frame& frame,const Bytes& actual,const char* field) {
    Bytes prefix;number(prefix,kind,1);number(prefix,frame.entry,4);number(prefix,frame.index,8);number(prefix,actual.size(),8);
    if(recording) {
        if(gzwrite(stream,prefix.data(),unsigned(prefix.size()))!=int(prefix.size()) ||
           gzwrite(stream,actual.data(),unsigned(actual.size()))!=int(actual.size()))throw std::runtime_error("Cannot record private Date edge");
    } else {
        Bytes expected_prefix(prefix.size());
        if(gzread(stream,expected_prefix.data(),unsigned(expected_prefix.size()))!=int(expected_prefix.size()))
            throw std::runtime_error("First divergence: Date edge "+std::to_string(frame.index)+" at "+hex_address(frame.entry)+" field=truncated header");
        const auto expected_size=read_be<8>(expected_prefix.data()+13);
        if(expected_prefix!=prefix || expected_size>128*1024*1024)
            throw std::runtime_error("First divergence: Date edge "+std::to_string(frame.index)+" at "+hex_address(frame.entry)+" field="+field+" header/size offset=0");
        Bytes expected(actual.size());
        if(gzread(stream,expected.data(),unsigned(expected.size()))!=int(expected.size()))
            throw std::runtime_error("First divergence: Date edge "+std::to_string(frame.index)+" at "+hex_address(frame.entry)+" field=truncated body");
        const auto mismatch=std::mismatch(expected.begin(),expected.end(),actual.begin());
        if(mismatch.first!=expected.end()) {
            if(const auto* directory=std::getenv("CM_AOT_DATE_DIFF_DIRECTORY")) {
                std::filesystem::create_directories(directory);
                for(const auto& [name,bytes]:std::array<std::pair<const char*,const Bytes*>,2>{{{"expected",&expected},{"actual",&actual}}}) {
                    std::ofstream file(std::filesystem::path(directory)/(std::string(name)+".bin"),std::ios::binary);
                    file.write(reinterpret_cast<const char*>(bytes->data()),bytes->size());
                }
            }
        }
        if(mismatch.first!=expected.end())throw std::runtime_error("First divergence: Date edge "+std::to_string(frame.index)+" at "+hex_address(frame.entry)+
            " field="+field+" offset="+std::to_string(mismatch.first-expected.begin())+
            " expected="+std::to_string(*mismatch.first)+" actual="+std::to_string(*mismatch.second));
    }
}
std::uint32_t result_pointer(std::uint32_t entry,const std::array<std::uint32_t,5>& args,const State& s) {
    if(entry==0x10715a30)return args[2];
    if(entry==0x107156b0||entry==0x107156f0||entry==0x10715880||entry==0x107158a0||
       entry==0x10717a10||entry==0x10717cb0||entry==0x10717dd0)return args[0];
    if(entry>=0x10715bc0&&entry<=0x10717620)return args[1];
    if(entry==0x107184d0||entry==0x10718570||(entry>=0x10718700&&entry<=0x107189d0))return std::uint32_t(s.r<4>(12));
    return 0;
}
Bytes result_bytes(std::uint32_t entry,const std::array<std::uint32_t,5>& args,const State& s) {
    Bytes result;number(result,s.r<4>(12),4);const auto pointer=result_pointer(entry,args,s);
    if(!pointer)return result;
    if(entry<0x10715bc0||entry==0x10717a10||entry==0x10717cb0||entry==0x10717dd0)raw(result,s.memory.copy_out(pointer,5));
    else {
        // Diagnostics use copy_out, so they add no traced legacy reads. A
        // terminated string is captured byte-for-byte, including its NUL.
        for(unsigned i=0;i<65536;++i){const auto byte=s.memory.copy_out(pointer+i,1)[0];result.push_back(byte);if(!byte)return result;}
        throw std::runtime_error("Private Date output lacks a bounded NUL");
    }
    return result;
}
void allocator(Bytes& bytes,const State& s) {
    number(bytes,s.heap_end,4);number(bytes,std::uint16_t(s.memory_error),2);number(bytes,s.native_calls,8);
    for(const auto* map:{&s.allocated,&s.available}){number(bytes,map->size(),4);for(const auto& [address,size]:*map){number(bytes,address,4);number(bytes,size,4);}}
    const auto ranges=s.memory.mapped_ranges();number(bytes,ranges.size(),4);for(const auto& [address,size]:ranges){number(bytes,address,4);number(bytes,size,8);}
}
struct ObservationScope {
    State& state;MemoryObserver* previous;bool active;
    ObservationScope(State& s,Frame& frame):state(s),previous(s.memory.date_observer),active(bool(stream)) {
        if(active){observer.frames.push_back(&frame);state.memory.date_observer=&observer;}
    }
    ~ObservationScope(){state.memory.date_observer=previous;if(active)observer.frames.pop_back();}
};
}
bool try_modern_date_subsystem(State& s,std::uint32_t entry) {
    // A same-page goto can reach this destination while the dispatcher's PC
    // still names its previous return. Publish the actual destination before
    // recording, just as the next generated step does in original execution.
    s.pc=entry;
    if(s.date_original_resume==entry){s.date_original_resume=0;return false;}
    const auto index=entry_index(entry);
    const bool original=s.calendar_original||s.date_subsystem_original||s.date_entry_original[index];
    if(stream&&recording&&!s.calendar_original)throw std::runtime_error("Date reference recording requires all 34 original entries");
    auto& calls=original?s.date_original_calls[index]:s.date_subsystem_calls[index];
    const bool sample=calls<16&&std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
    ++calls;
    if(original&&!stream&&!sample)return false;
    const std::array<std::uint32_t,5> args{std::uint32_t(s.r<4>(12)),std::uint32_t(s.r<4>(16)),std::uint32_t(s.r<4>(20)),std::uint32_t(s.r<4>(24)),std::uint32_t(s.r<4>(28))};
    const auto link=std::uint32_t(s.r<4>(4128));Frame frame{entry,next_index++,{}};
    // Snapshot reads never enter the observed body of this entry or its parent.
    auto* parent_observer=s.memory.date_observer;s.memory.date_observer=nullptr;
    if(stream){Bytes input;raw(input,s.registers);raw(input,s.unique);number(input,s.pc,4);allocator(input,s);chunk(1,frame,input,"input/ABI");}
    s.memory.date_observer=parent_observer;
    {
        ObservationScope scope(s,frame);
        if(original){s.date_original_resume=entry;s.pc=entry;try{run_until(s,link);}catch(...){s.date_original_resume=0;throw;}}
        else {s.step(entry);if(!execute_semantic_date(s,entry))throw std::runtime_error("Missing semantic Date body");}
    }
    s.memory.date_observer=nullptr;
    if(!original&&stream&&perturb==entry&&!perturbed) {
        perturbed=true;const auto pointer=result_pointer(entry,args,s);
        if(entry<0x10715bc0||entry==0x10717a10||entry==0x10717cb0||entry==0x10717dd0)
            s.memory.write<1>(pointer,s.memory.copy_out(pointer,1)[0]^1);
        else s.wr<4>(12,s.r<4>(12)^1);
    }
    if(sample) {
        const auto output=result_bytes(entry,args,s);std::fprintf(stderr,"date-subsystem.%s entry=%08x sample=%llu caller=%08x args=%08x,%08x,%08x,%08x,%08x return=%08x bytes=",
            original?"original":"modern",entry,static_cast<unsigned long long>(calls),link,args[0],args[1],args[2],args[3],args[4],unsigned(s.r<4>(12)));
        for(unsigned i=0;i<std::min<std::size_t>(output.size(),160);++i)std::fprintf(stderr,"%02x",unsigned(output[i]));std::fprintf(stderr,"\n");
    }
    if(stream) {
        Bytes output;const auto result=result_bytes(entry,args,s);number(output,result.size(),4);raw(output,result);
        raw(output,s.registers);raw(output,s.unique);number(output,s.pc,4);allocator(output,s);number(output,frame.effects.size(),8);raw(output,frame.effects);
        chunk(2,frame,output,"result/ABI/effects");
    }
    s.memory.date_observer=parent_observer;return true;
}
void open_date_subsystem_audit(const std::filesystem::path& path,bool record,std::uint32_t perturb_entry) {
    if(stream||(record&&perturb_entry))throw std::runtime_error("Invalid private Date audit mode");
    if(perturb_entry)entry_index(perturb_entry);
    stream=gzopen(path.c_str(),record?"wb1":"rb");if(!stream)throw std::runtime_error("Cannot open private Date stream");
    auto header=magic;const auto count=record?gzwrite(stream,header.data(),header.size()):gzread(stream,header.data(),header.size());
    if(count!=int(header.size())||header!=magic){gzclose(stream);stream=nullptr;throw std::runtime_error("Unrecognized private Date stream");}
    recording=record;perturb=perturb_entry;perturbed=false;next_index=0;
}
void close_date_subsystem_audit(bool require_end) {
    if(!stream)return;bool extra=false;
    if(require_end&&!recording){std::uint8_t byte;extra=gzread(stream,&byte,1)!=0||!gzeof(stream);}
    const auto result=gzclose(stream);stream=nullptr;perturb=0;perturbed=false;next_index=0;
    if(extra||result!=Z_OK)throw std::runtime_error("First divergence: Date unconsumed edge/EOF/checksum");
}
std::string date_subsystem_call_summary(const State& s) {
    std::ostringstream out;
    for(unsigned i=0;i<date_subsystem_entries.size();++i)out<<"date.entry="<<hex_address(date_subsystem_entries[i])
        <<" mode="<<(s.calendar_original||s.date_subsystem_original||s.date_entry_original[i]?"original":"modern")
        <<" modern="<<s.date_subsystem_calls[i]<<" original="<<s.date_original_calls[i]<<'\n';
    return out.str();
}
}
