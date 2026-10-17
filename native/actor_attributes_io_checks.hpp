#pragma once
#include "actor_attributes_adapter.hpp"
#include "platform.hpp"
#include "semantic_io_checks.hpp"
#include "data_forks.hpp"
#include "file_reference_bridge.hpp"
#include "fork_io_bridge.hpp"
#include <chrono>
#include <fstream>
#include <map>

namespace cm::semantic_io_checks {
// Carbon allocates process-global fork numbers monotonically. Supply stable
// guest numbers at this host-input boundary while performing actual native IO.
// Every guest read/write still reaches the ordinary effect observers.
struct ActorAttributesForkInputs {
    NativeDataForks forks;
    std::map<std::int16_t,std::int16_t> references;
    std::int16_t next=121;
    bool dispatch(State& s,std::string_view name,const std::filesystem::path& working) {
        const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>((n+3)*4));};
        std::int16_t result=0;
        if(name=="CarbonLib::FSOpenFork") {
            require(arg(1)==0,"Lifecycle history uses the unnamed data fork");
            NativeFileReference bytes{};const auto input=s.memory.copy_out(arg(0),bytes.size());std::copy(input.begin(),input.end(),bytes.begin());
            std::filesystem::path path;result=native_reference_path(bytes,path);
            if(!result) {
                const auto relative=path.lexically_relative(working);
                require(!relative.empty() && *relative.begin()!="..","History fork must stay in its private fixture");
                std::int16_t native=0;result=forks.open(path,std::int8_t(arg(3)),native);
                if(!result){const auto guest=next++;references.emplace(guest,native);s.memory.write<2>(arg(4),std::uint16_t(guest));}
            }
        } else if(name=="CarbonLib::FSCloseFork") {
            const auto guest=std::int16_t(arg(0));require(references.contains(guest),"Closing an unknown fixture fork");
            result=forks.close(references.at(guest));if(!result)references.erase(guest);
        } else if(name=="CarbonLib::PBGetForkSizeSync" || name=="CarbonLib::PBSetForkPositionSync" ||
                  name=="CarbonLib::PBReadForkSync" || name=="CarbonLib::PBWriteForkSync") {
            const auto pb=arg(0);s.memory.write<4>(pb+12,0);
            const auto guest=std::int16_t(s.memory.read<2>(pb+24));require(references.contains(guest),"Unknown fixture parameter-block fork");const auto native=references.at(guest);
            if(name=="CarbonLib::PBGetForkSizeSync") {
                std::int64_t size=0;result=forks.size(native,size);if(!result)s.memory.write<8>(pb+46,std::uint64_t(size));
            } else {
                const auto mode=std::uint16_t(s.memory.read<2>(pb+44));const auto offset=std::bit_cast<std::int64_t>(s.memory.read<8>(pb+46));
                if(name=="CarbonLib::PBSetForkPositionSync")result=forks.seek(native,mode,offset);
                else {
                    const auto count=std::uint32_t(s.memory.read<4>(pb+36)),buffer=std::uint32_t(s.memory.read<4>(pb+32));std::uint32_t actual=0;
                    result=name=="CarbonLib::PBReadForkSync"?read_guest_fork(s,forks,native,mode,offset,buffer,count,actual):write_guest_fork(s,forks,native,mode,offset,buffer,count,actual);
                    s.memory.write<4>(pb+40,actual);
                }
            }
            s.memory.write<2>(pb+16,std::uint16_t(result));
        } else return false;
        s.wr<4>(12,std::uint32_t(std::int32_t(result)));s.pc=s.r<4>(4128);return true;
    }
};
// Opaque native FSRefs contain filesystem identities. Use one private location
// sequentially, with identical bytes restored between fresh platform instances.
// Pre-existing empty history files keep their identities stable for both runs.
struct ActorAttributesIoPair {
    using Files=std::map<std::string,std::vector<char>>;
    std::filesystem::path root,working;
    Files baseline,expected;
    std::map<std::string,std::filesystem::file_time_type> times;
    std::shared_ptr<NativePlatform> original,modern;
    ActorAttributesForkInputs old_forks,new_forks;
    bool dispatch(State& s,std::string_view name,bool modern_run) {
        return !root.empty() && (modern_run?new_forks:old_forks).dispatch(s,name,working);
    }
    static Files files(const std::filesystem::path& base) {
        Files result;
        for(const auto& item:std::filesystem::recursive_directory_iterator(base)) {
            require(!item.is_symlink(),"Private IO pair rejects symbolic links");if(!item.is_regular_file())continue;
            std::ifstream stream(item.path(),std::ios::binary);require(bool(stream),"Cannot read private IO output");
            std::vector<char> bytes(std::filesystem::file_size(item.path()));
            if(!bytes.empty())stream.read(bytes.data(),std::streamsize(bytes.size()));require(bool(stream),"Cannot read complete private IO output");
            result.emplace(item.path().lexically_relative(base).generic_string(),std::move(bytes));
        }
        return result;
    }
    static void copy(const std::filesystem::path& from,const std::filesystem::path& to) {
        std::filesystem::create_directories(to);
        for(const auto& item:std::filesystem::recursive_directory_iterator(from)) {
            require(!item.is_symlink(),"Private IO input rejects symbolic links");const auto path=to/item.path().lexically_relative(from);
            if(item.is_directory())std::filesystem::create_directories(path);
            else if(item.is_regular_file()) {std::filesystem::copy_file(item.path(),path);std::filesystem::last_write_time(path,std::filesystem::last_write_time(item.path()));}
        }
    }
    ActorAttributesIoPair(const State& input,unsigned index,std::uint32_t root_entry=0,
                          std::function<void(State&)> original_control={}):original(input.platform),modern(input.platform) {
        if(!root_entry && index!=12 && index!=13 && index!=17)return;
        require(bool(input.platform),"Actual lifecycle IO requires a platform");static unsigned serial=0;
        root=input.platform->writable_directory().parent_path()/("lifecycle-io-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial));
        require(!std::filesystem::exists(root),"Private IO pair must be new");working=root/"working";
        copy(input.platform->writable_directory(),working);copy(input.platform->writable_directory(),root/"probe");
        {
            auto probe=clone(input);probe->platform=std::make_shared<NativePlatform>(input.platform->game_directory(),root/"probe",AudioMode::offline);
            actor_attributes_diagnostics(*probe).original=true;
            if(original_control)original_control(*probe);
            probe->pc=root_entry?root_entry:actor_attributes::entries[index];probe->wr<4>(4128,return_sentinel);
            probe->budget=probe->executed+50000000;
            try {run_until(*probe,return_sentinel);}
            catch(const std::exception& error) {
                std::cerr<<"Original private-IO probe entry="<<hex_address(root_entry?root_entry:actor_attributes::entries[index])
                    <<" pc="<<hex_address(probe->pc)<<" reason="<<error.what()<<std::endl;
                throw;
            }
        }
        const auto initial=files(working);
        for(const auto& [name,bytes]:files(root/"probe")) {
            (void)bytes;if(initial.contains(name))continue;
            const auto path=working/name;std::filesystem::create_directories(path.parent_path());std::ofstream empty(path,std::ios::binary);require(bool(empty),"Cannot prepare empty private history file");
        }
        baseline=files(working);for(const auto& [name,bytes]:baseline) {(void)bytes;times.emplace(name,std::filesystem::last_write_time(working/name));}
        original=std::make_shared<NativePlatform>(input.platform->game_directory(),working,AudioMode::offline);
        modern=std::make_shared<NativePlatform>(input.platform->game_directory(),working,AudioMode::offline);
    }
    void between() {
        if(root.empty())return;original.reset();expected=files(working);
        for(const auto& [name,bytes]:expected) {(void)bytes;require(baseline.contains(name),"History probe must discover every created private file");}
        for(const auto& [name,bytes]:baseline) {
            std::ofstream stream(working/name,std::ios::binary|std::ios::trunc);require(bool(stream),"Cannot restore private history baseline");
            if(!bytes.empty())stream.write(bytes.data(),std::streamsize(bytes.size()));require(bool(stream),"Cannot restore complete private history baseline");
            stream.close();std::filesystem::last_write_time(working/name,times.at(name));
        }
        require(files(working)==baseline,"Private history baseline restored exactly");
    }
    void compare()const {if(!root.empty())require(files(working)==expected,"Original/modern private history file bytes");}
    ~ActorAttributesIoPair() {
        original.reset();modern.reset();if(root.empty())return;std::error_code error;
        if(std::filesystem::is_symlink(root,error)||error)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(root,error))if(error||p.is_symlink(error)||error)return;
        std::filesystem::remove_all(root,error);
    }
};
}
