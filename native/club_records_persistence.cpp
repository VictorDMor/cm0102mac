#include "club_records_detail.hpp"
#include <array>

namespace cm::aot::club_records {
using namespace detail;
namespace {
struct StoredField {
    unsigned offset, size, count_offset;
    bool collection, skip_empty;
    std::uint32_t returned, error_site;
    unsigned name, extension, line;
};
struct ErrorSite {std::uint32_t site;unsigned name,extension,line;};
// File order is capacity/count/payload for the two record collections, then
// the per-club indexes and pending notification collection. Raw guest bytes
// include the original padding; unused capacity is never serialized.
constexpr std::array save_fields{
    StoredField{8,4,0,false,false,0x1001b244,0x1001b260,0x1e4,0x8c,0x1609},
    StoredField{12,4,0,false,false,0x1001b2c8,0x1001b2e4,0x1c4,0x84,0x1611},
    StoredField{4,record_size,12,true,true,0x1001b354,0x1001b374,0x1a4,0x7c,0x161c},
    StoredField{20,4,0,false,false,0x1001b3dc,0x1001b3f8,0x184,0x74,0x1625},
    StoredField{24,4,0,false,false,0x1001b460,0x1001b47c,0x164,0x6c,0x162d},
    StoredField{16,record_size,24,true,true,0x1001b4ec,0x1001b50c,0x144,0x64,0x1638},
    StoredField{32,4,0,false,false,0x1001b574,0x1001b590,0x124,0x5c,0x1641},
    StoredField{28,index_size,32,true,false,0x1001b5f8,0x1001b618,0x104,0x54,0x1649},
    StoredField{40,4,0,false,false,0x1001b680,0x1001b69c,0xe4,0x4c,0x1651},
    StoredField{44,4,0,false,false,0x1001b704,0x1001b720,0xc4,0x44,0x1659},
    StoredField{36,8,44,true,true,0x1001b790,0x1001b7b0,0xa4,0x3c,0x1664},
};
}

std::uint32_t save(Cpu& c,std::uint32_t manager,std::uint32_t file) {
    Frame frame(c,0x1001b120,0x260,{.first_gpr=28});Program p{c};
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(slot::message)},{29,c.toc(slot::success)},{28,manager}});
    if(compare_logical(c,file)==0){assertion(c,frame,0x1001b168,0x224,0x9c,0x15fa);c.set(3,0);return 0;}
    const auto opened=p.call(0x107475b0,0x1001b1c4,{file,c.toc(slot::pool)+0x275c,1,0x16,4});
    compare_logical(c,opened);c.put32(frame.at(0x38),opened);
    if(!opened) {
        assertion(c,frame,0x1001b1e4,0x204,0x94,0x1602);p.call(0x1076f7d0,0x1001b228,{missing});c.set(3,0);return 0;
    }
    for(const auto& field:save_fields) {
        auto count=field.collection?c.u32(manager+field.count_offset):1u;
        if(field.collection && field.skip_empty) {
            c.set(5,count);if(compare(c,count)<=0)continue;
        }
        const auto address=field.collection?c.u32(manager+field.offset):manager+field.offset;
        const auto written=p.call(0x10747cc0,field.returned,{address,field.size,count,c.u32(frame.at(0x38))});
        // The original reloads a collection count after the write.
        if(field.collection)count=c.u32(manager+field.count_offset);
        if((field.collection?compare(c,count,written):compare(c,written,1))!=0) {
            assertion(c,frame,field.error_site,field.name,field.extension,field.line);
            p.call(0x1076f7d0,field.error_site+0x44,{missing});
            p.call(0x10747900,field.error_site+0x4c,{frame.at(0x38)});c.set(3,0);return 0;
        }
    }
    p.call(0x10747900,0x1001b80c,{frame.at(0x38)});c.set(3,1);return 1;
}

std::uint32_t load(Cpu& c,std::uint32_t manager,std::uint32_t file) {
    Frame frame(c,0x1001a690,0x340,{.first_gpr=22});Program p{c};
    keep(c,{{29,c.toc(slot::success)},{30,c.toc(slot::message)},{25,manager},{31,c.toc(slot::pool)},{26,0}});
    if(compare_logical(c,file)==0){assertion(c,frame,0x1001a6d0,0x2ec,0xc4,0x1515);c.set(3,0);return 0;}
    const auto opened=p.call(0x107475b0,0x1001a72c,{file,c.toc(slot::pool)+0x275c,0,0x16,4});
    compare_logical(c,opened);c.put32(frame.at(0x38),opened);
    if(!opened){assertion(c,frame,0x1001a74c,0x2cc,0xbc,0x151d);c.set(3,0);return 0;}
    const auto failed=[&](ErrorSite error) {
        assertion(c,frame,error.site,error.name,error.extension,error.line);
        p.call(0x1076f7d0,error.site+0x44,{missing});p.call(0x10747900,error.site+0x4c,{frame.at(0x38)});
        c.set(3,0);return false;
    };
    const auto scalar=[&](unsigned offset,std::uint32_t back,ErrorSite error) {
        const auto read=p.call(0x10747a80,back,{manager+offset,4,1,c.u32(frame.at(0x38))});
        return compare(c,read,1)==0 || failed(error);
    };
    const auto collection=[&](unsigned pointer,unsigned capacity,unsigned count,unsigned size,
                              std::uint32_t allocated,ErrorSite allocation_error,
                              std::uint32_t read_back,ErrorSite read_error,bool required=false) {
        const auto available=c.u32(manager+capacity);
        if(!required && compare(c,available)<=0){c.put32(manager+pointer,0);return true;}
        const auto address=p.call(0x10775130,allocated,{available*size});c.put32(manager+pointer,address);c.set(3,c.u32(manager+pointer));
        if(compare_logical(c,c.r(3))==0)return failed(allocation_error);
        const auto items=c.u32(manager+count);c.set(5,items);
        if(!required && compare(c,items)<=0)return true;
        const auto read=p.call(0x10747a80,read_back,{address,size,items,c.u32(frame.at(0x38))});
        return compare(c,c.u32(manager+count),read)==0 || failed(read_error);
    };
    if(!scalar(8,0x1001a7a4,{0x1001a7c0,0x2ac,0xb4,0x1524}) ||
       !scalar(12,0x1001a828,{0x1001a844,0x28c,0xac,0x152c}) ||
       !collection(4,8,12,record_size,0x1001a8ac,{0x1001a8d0,0x26c,0xa4,0x1538},
                   0x1001a93c,{0x1001a95c,0x24c,0x9c,0x1543}) ||
       !scalar(20,0x1001a9cc,{0x1001a9e8,0x22c,0x94,0x1552}) ||
       !scalar(24,0x1001aa50,{0x1001aa6c,0x20c,0x8c,0x155a}) ||
       !collection(16,20,24,record_size,0x1001aad4,{0x1001aaf8,0x1ec,0x84,0x1566},
                   0x1001ab64,{0x1001ab84,0x1cc,0x7c,0x1571}) ||
       !scalar(32,0x1001abf4,{0x1001ac10,0x1ac,0x74,0x1580}) ||
       !collection(28,32,32,index_size,0x1001ac70,{0x1001ac94,0x18c,0x6c,0x1589},
                   0x1001acf8,{0x1001ad18,0x16c,0x64,0x1591},true))return 0;
    Kept<27> club_index(c,0);Kept<28> offset(c,0);
    while(compare(c,club_index,c.i32(manager+32))<0) {
        const auto index=c.u32(manager+28)+std::uint32_t(offset);c.set({{3,c.u32(index+4)},{4,index}});
        const bool invalid=compare(c,c.r(3),c.i32(manager+20))>=0 || compare(c,c.r(3),-1)<0 ||
            (c.set(3,c.u32(index)),compare(c,c.r(3),c.i32(manager+8))>=0) || compare(c,c.r(3),-1)<0 ||
            compare(c,c.i32(index+8),-1)<0 || compare(c,c.i32(index+12),-1)<0;
        if(invalid) {
            if(compare(c,std::int8_t(c.r(26)))==0) {
                c.set(26,1);assertion(c,frame,0x1001ade8,0x14c,0x5c,0x15a2);
            }
            const auto indexes=c.u32(manager+28);c.set({{22,missing},{23,indexes},{24,indexes}});
            for(auto field:{0u,4u,8u,12u})c.put32(index+field,missing);
            c.put8(index+16,0);c.put8(index+17,0);c.put16(index+18,0);
            for(auto field:{20u,24u,44u,64u,84u})c.put32(index+field,0);
            c.put16(index+104,0);
            c.set({{3,indexes},{4,indexes},{5,indexes},{6,indexes},{7,indexes},{8,indexes},{9,indexes},{10,indexes}});
        }
        offset=std::uint32_t(offset)+index_size;club_index=std::uint32_t(club_index)+1;
    }
    if(!scalar(40,0x1001aeec,{0x1001af08,0x12c,0x54,0x15b9}) ||
       !scalar(44,0x1001af70,{0x1001af8c,0x10c,0x4c,0x15c1}) ||
       !collection(36,40,44,8,0x1001aff4,{0x1001b018,0xec,0x44,0x15cd},
                   0x1001b084,{0x1001b0a4,0xcc,0x3c,0x15d8}))return 0;
    p.call(0x10747900,0x1001b108,{frame.at(0x38)});c.set(3,1);return 1;
}
}
