#include "match_debugger_database.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>

namespace cm::aot::match_debugger {
namespace {
constexpr std::size_t club_stride = 584, name_stride = 79;
constexpr std::uint32_t table_marker = 0xf1fffff1;
struct IndexEntry {
    std::string filename;
    std::uint32_t id, count, stored_offset, type;
};
std::uint32_t word(std::span<const std::uint8_t> data,std::size_t offset,bool little=false) {
    if(offset>data.size()||data.size()-offset<4)throw std::runtime_error("Truncated original database word");
    std::uint32_t value=0;
    for(unsigned n=0;n<4;++n)value=(value<<8)|data[offset+(little?3-n:n)];
    return value;
}
std::span<const std::uint8_t> field(std::span<const std::uint8_t> data,std::size_t offset,std::size_t size) {
    if(offset>data.size()||size>data.size()-offset)throw std::runtime_error("Truncated original database field");
    return data.subspan(offset,size);
}
std::string filename(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);
    if(end==bytes.end()||end==bytes.begin())throw std::runtime_error("Invalid database table filename");
    std::string result(bytes.begin(),end);
    if(result=="."||result==".."||result.find_first_of("/\\:")!=std::string::npos)
        throw std::runtime_error("Unsafe database table filename");
    return result;
}
std::vector<IndexEntry> index_entries(std::span<const std::uint8_t> bytes) {
    if(bytes.size()<8||(bytes.size()-8)%68||word(bytes,0)||word(bytes,4))
        throw std::runtime_error("Invalid original Mac database index");
    std::vector<IndexEntry> result;std::set<std::uint32_t> ids;std::set<std::string> names;
    for(std::size_t offset=8;offset<bytes.size();offset+=68) {
        IndexEntry entry{filename(field(bytes,offset,52)),word(bytes,offset+52),word(bytes,offset+56),word(bytes,offset+60),word(bytes,offset+64)};
        if(!ids.insert(entry.id).second||!names.insert(entry.filename).second)
            throw std::runtime_error("Duplicate original database table identity");
        result.push_back(std::move(entry));
    }
    return result;
}
const IndexEntry& table(const std::vector<IndexEntry>& entries,std::uint32_t id) {
    const auto found=std::find_if(entries.begin(),entries.end(),[id](const auto& entry){return entry.id==id;});
    if(found==entries.end())throw std::runtime_error("Original database club table is missing");
    if(found->stored_offset||found->type!=2)throw std::runtime_error("Unsupported original Mac club table representation");
    return *found;
}
void validate_records(std::span<const std::uint8_t> data,std::uint32_t count) {
    if(std::uint64_t(count)*club_stride+4!=data.size()||word(data,data.size()-4)!=table_marker)
        throw std::runtime_error("Original club records or trailing marker are invalid");
}
std::string display_name(std::span<const std::uint8_t> bytes) {
    bytes=bytes.first(std::size_t(std::find(bytes.begin(),bytes.end(),0)-bytes.begin()));
    // The supplied .lng records use Windows Latin-1, including F6 for the
    // German club name Koeln. File records remain unchanged for guest loading.
    CFStringRef value=CFStringCreateWithBytes(nullptr,bytes.data(),CFIndex(bytes.size()),kCFStringEncodingWindowsLatin1,false);
    if(!value)throw std::runtime_error("Invalid original club name encoding");
    const auto maximum=CFStringGetMaximumSizeForEncoding(CFStringGetLength(value),kCFStringEncodingUTF8)+1;
    std::string result(std::size_t(maximum),0);
    const bool converted=CFStringGetCString(value,result.data(),maximum,kCFStringEncodingUTF8);
    CFRelease(value);
    if(!converted)throw std::runtime_error("Cannot decode original club name");
    result.resize(std::char_traits<char>::length(result.c_str()));return result;
}
std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
    const auto size=std::filesystem::file_size(path);
    if(size>64*1024*1024)throw std::runtime_error("Original club catalog input exceeds its supported size");
    std::ifstream stream(path,std::ios::binary);std::vector<std::uint8_t> bytes(size);
    if(!stream.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size)))
        throw std::runtime_error("Cannot read original club catalog input");
    return bytes;
}
std::string digest(std::span<const std::uint8_t> bytes) {
    std::array<unsigned char,CC_SHA256_DIGEST_LENGTH> result;
    CC_SHA256(bytes.data(),CC_LONG(bytes.size()),result.data());
    constexpr char alphabet[]="0123456789abcdef";std::string value;
    for(auto byte:result){value+=alphabet[byte>>4];value+=alphabet[byte&15];}return value;
}
void align_supplied_english(std::vector<std::uint8_t>& names,std::span<const std::uint8_t> clubs) {
    constexpr auto original="f305bc9731a06098f24dc860f9156f539d4d965e610f90196e3e2792be1ef194";
    constexpr auto aligned="d949309b50c822c337546e04b7eb893507aba3cf0bec9aa95d4ef7f9d8f6ffa1";
    const auto hash=digest(names);
    if(hash==aligned)return;
    if(hash!=original||digest(clubs)!="1fb0859d587afb684667eb614a043b5b73234b36cabec9b803989f5231e32ece")
        throw std::runtime_error("Club names require the supported original database release");
    // Same established compatibility correction as language_compat.py. Only
    // this audited record is moved in memory; the original file is never written.
    const auto first=32+5791*name_stride,last=32+8649*name_stride;
    if(last+name_stride>names.size())throw std::runtime_error("Truncated original English club names");
    std::rotate(names.begin()+first,names.begin()+last,names.begin()+last+name_stride);
    if(digest(names)!=aligned)throw std::runtime_error("Aligned original club names failed verification");
}
}

DatabaseCatalog DatabaseCatalog::parse(std::span<const std::uint8_t> index,
    std::span<const std::uint8_t> clubs,std::span<const std::uint8_t> national,
    std::span<const std::uint8_t> english) {
    const auto entries=index_entries(index);const auto& club_table=table(entries,0);const auto& national_table=table(entries,1);
    validate_records(clubs,club_table.count);validate_records(national,national_table.count);
    const auto total=std::uint64_t(club_table.count)+national_table.count;
    if(total>UINT32_MAX||word(english,0,true)!=total||32+total*name_stride>english.size())
        throw std::runtime_error("Original club and language counts do not agree");
    DatabaseCatalog catalog;catalog.clubs_.reserve(std::size_t(total));catalog.records_.reserve(std::size_t(total)*club_stride);
    std::uint32_t expected=0;
    for(auto [data,count,kind]:std::array<std::tuple<std::span<const std::uint8_t>,std::uint32_t,ClubKind>,2>{{
        {clubs,club_table.count,ClubKind::club},{national,national_table.count,ClubKind::national_team}}}) {
        for(std::uint32_t row=0;row<count;++row,++expected) {
            const auto record=field(data,std::size_t(row)*club_stride,club_stride);const auto id=word(record,0);
            if(id!=expected)throw std::runtime_error("Original club record ID is missing, duplicated or out of order");
            const auto label=field(english,32+std::size_t(id)*name_stride,name_stride);
            Club club{id,kind,display_name(label.first(52)),display_name(label.subspan(52,26))};
            if(club.name.empty())throw std::runtime_error("Original English club name is missing");
            catalog.clubs_.push_back(std::move(club));catalog.records_.insert(catalog.records_.end(),record.begin(),record.end());
        }
    }
    return catalog;
}
DatabaseCatalog DatabaseCatalog::load(const std::filesystem::path& original_game) {
    const auto data=original_game/"Data";const auto index=read_file(data/"index.dam");const auto entries=index_entries(index);
    const auto clubs=read_file(data/table(entries,0).filename),national=read_file(data/table(entries,1).filename);
    auto english=read_file(data/"eng.lng");align_supplied_english(english,clubs);
    return parse(index,clubs,national,english);
}
const Club& DatabaseCatalog::club(std::uint32_t id) const {
    if(id>=clubs_.size())throw std::out_of_range("Unknown original club ID");return clubs_[id];
}
std::span<const std::uint8_t> DatabaseCatalog::original_record(std::uint32_t id) const {
    (void)club(id);return std::span<const std::uint8_t>(records_).subspan(std::size_t(id)*club_stride,club_stride);
}
}
