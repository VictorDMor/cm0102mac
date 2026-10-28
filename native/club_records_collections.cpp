#include "club_records_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::club_records {
using namespace detail;

std::uint32_t flush_pending(Cpu& c,std::uint32_t manager) {
    Frame frame(c,0x10013650,0x140,{.first_gpr=24});Program p{c};Manager records{c,manager};
    keep(c,{{31,c.toc(slot::news)},{29,manager},{28,c.toc(slot::clubs)}});
    if(compare_logical(c,records.pending())!=0) {
        Kept<30> index(c,0);Kept<27> offset(c,0);
        while(compare(c,index,records.pending_count())<0) {
            c.set(4,c.u32(c.r(28)));c.set(5,records.pending().get()+std::uint32_t(offset));
            Kept<26> club_id(c,c.u32(c.r(5)));Kept<25> kind(c,c.u8(c.r(5)+4));
            Kept<24> club(c,c.r(4)+std::uint32_t(club_id)*0x248);
            bool deliver=nonzero_byte(c,p.call(0x102cdbf0,0x100136ac,{c.u32(c.r(31)),club}));
            if(!deliver)deliver=nonzero_byte(c,p.call(0x100c7d00,0x100136bc,{club}));
            if(deliver) {
                p.call(0x102bdf90,0x100136d4,{frame.at(0x38),1000,0});
                const auto period=c.i8(records.pending().get()+std::uint32_t(offset)+5);
                std::uint32_t record;
                if(compare(c,period,2)==0) {
                    record=p.call(0x1001b9c0,0x100136fc,{manager,club,0});
                    p.call(0x1001c290,0x10013710,{club_id,record,kind,frame.at(0x38)});
                }else {
                    record=p.call(0x1001b830,0x10013724,{manager,club,0});
                    p.call(0x1001c290,0x10013738,{club_id,record,kind,frame.at(0x38)});
                }
                if(nonzero_byte(c,p.call(0x100c7d00,0x10013740,{club})))
                    p.call(0x102cc400,0x10013758,{c.u32(c.r(31)),frame.at(0x38),c.u32(club+0x2c)});
                else p.call(0x102cc140,0x1001376c,{c.u32(c.r(31)),frame.at(0x38),club});
            }
            offset=std::uint32_t(offset)+8;index=std::uint32_t(index)+1;
        }
        if(compare(c,records.pending_capacity(),100)>=0) {
            p.call(0x10775170,0x10013794,{records.pending()});records.pending()=0;records.pending_capacity()=0;
        }
        records.pending_count()=0;
    }
    c.set(3,1);return 1;
}

std::uint32_t format_period(Cpu& c,std::uint32_t manager,std::uint32_t club,std::uint32_t period,
                             std::uint32_t kind,std::uint32_t output) {
    Frame frame(c,0x10013ee0,0xb0,{.first_gpr=24});Program p{c};Fpu fp(c);
    keep(c,{{28,output},{29,c.toc(slot::success)},{30,c.toc(slot::message)},{24,manager},
            {31,c.toc(slot::pool)},{25,club},{26,period},{27,kind}});
    if(compare(c,output)==0){assertion(c,frame,0x10013f28,0x68,0x40,0x95f);c.set(3,0);return 0;}
    p.call(0x1074c570,0x10013f74,{output});p.call(0x1074b830,0x10013f80,{output,c.toc(slot::pool)+0x36c});
    if(compare_logical(c,club)==0){assertion(c,frame,0x10013f9c,0x48,0x38,0x96b);c.set(3,0);return 0;}
    if(compare(c,std::int8_t(period),1)==0)c.set(29,p.call(0x1001b830,0x10013ffc,{manager,club,0}));
    else {
        const auto record=p.call(0x1001b9c0,0x10014014,{manager,club,0});c.set(29,record);
        if(compare(c,std::int8_t(kind),20)==0 && compare_logical(c,record)!=0) {
            c.set(3,c.toc(slot::doubles));c.set_f(1,c.f64(c.r(3)+0x48));
            fp.fcmp(0,c.f(1),c.f32(record+0x23c));
            if(c.cr(0)&2)p.call(0x1001bb50,0x1001404c,{manager,club});
        }
    }
    return p.call(0x10014080,0x10014064,{manager,club,c.r(29),kind,output});
}

std::uint32_t read_history(Cpu& c,std::uint32_t manager,std::uint32_t club,std::uint32_t count_out) {
    Frame frame(c,0x10019720,0xb0,{.first_gpr=23});Program p{c};Fpu fp(c);
    keep(c,{{28,club},{24,c.toc(slot::success)},{25,c.toc(slot::message)},
            {29,count_out},{26,c.toc(slot::pool)},{27,manager}});c.put32(count_out,0);
    if(compare(c,club)==0){assertion(c,frame,0x10019768,0x68,0x40,0x14c1);c.set(3,0);return 0;}
    const auto season_result=p.call(0x1001b9c0,0x100197b4,{manager,club,0});Kept<31> season(c,season_result);
    Kept<23> modern(c,p.call(0x1001b830,0x100197cc,{manager,club,0}));
    c.set(4,0x80000);
    Kept<30> rows(c,p.call(0x10775130,0x100197dc,{0x7d000}));
    if(compare(c,rows)==0){assertion(c,frame,0x100197f8,0x48,0x38,0x14cd);c.set(3,0);return 0;}
    c.put16(std::uint32_t(rows)+(c.u32(count_out)<<10),0);
    if(compare_logical(c,modern)!=0) {
        Kept<24> destination(c,std::uint32_t(rows)+(c.u32(count_out)<<10)+4);
        copy_archive_record(c,destination,modern,0x10019870);
    }else p.call(0x1001c160,0x100199fc,{std::uint32_t(rows)+(c.u32(count_out)<<10)+4});
    c.set(3,c.u32(count_out));c.put32(count_out,c.r(3)+1);
    if(compare_logical(c,season)!=0) {
        c.set(3,c.toc(slot::doubles));c.set_f(1,c.f64(c.r(3)+0x48));fp.fcmp(0,c.f(1),c.f32(season+0x23c));
        if(c.cr(0)&2)p.call(0x1001bb50,0x10019a30,{manager,club});
    }
    const auto year=p.call(0x10192610,0x10019a38,{club});c.put16(std::uint32_t(rows)+(c.u32(count_out)<<10),year);
    if(compare_logical(c,season)!=0) {
        Kept<24> destination(c,std::uint32_t(rows)+(c.u32(count_out)<<10)+4);
        copy_archive_record(c,destination,season,0x10019a68);
    }else p.call(0x1001c160,0x10019bf4,{std::uint32_t(rows)+(c.u32(count_out)<<10)+4});
    c.set(3,c.u32(count_out));c.put32(count_out,c.r(3)+1);
    const auto index=c.u32(manager+28)+c.u32(club)*index_size;c.set(4,c.u32(manager+28));
    c.set(23,p.call(0x10749fd0,0x10019c20,{c.u32(manager),c.u32(index+8),c.u32(index+12)}));
    if(compare(c,c.r(23))!=0) {
        c.put32(c.r(23)+12,c.u32(c.r(23)+8));
        for(;;) {
            const auto row=p.call(0x10738e30,0x10019c70,{c.r(23)});c.set(5,row);
            if(compare(c,row)==0)break;
            c.set(4,c.u32(count_out));if(compare(c,c.r(4),500)==0)break;
            c.set(3,c.r(4)+1);c.put32(count_out,c.r(3));c.set(6,std::uint32_t(rows)+(c.r(4)<<10));
            c.put16(c.r(6),c.u16(row));p.call(0x10019ca0,0x10019c68,{c.r(6)+4,row+4});
        }
        p.call(0x10738c20,0x10019c84,{c.r(23),1});
    }
    c.set(3,rows);return rows;
}
}
