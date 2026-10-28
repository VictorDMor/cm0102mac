#include "club_records_detail.hpp"

namespace cm::aot::club_records {
using namespace detail;

std::uint32_t clear_record(Cpu& c,std::uint32_t record) {
    Frame frame(c,0x1001c160,0x70,{.first_gpr=30});
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(slot::message)}});
    if(compare_logical(c,record)==0) {
        assertion(c,frame,0x1001c198,0x40,0x38,0x17ed,true);c.set(3,0);return 0;
    }
    c.set(4,c.toc(slot::floats));
    c.put32(record+4,missing);const auto empty_average=c.f32(c.r(4)+4);
    for(auto offset:{0x10u,0x18u,0x40u,0x68u,0x90u,0xa0u,0xc4u,0xe8u,0x10cu,0x130u,0x14cu,0x160u,0x174u,0x188u})
        c.put32(record+offset,missing);
    for(auto offset:{0x19cu,0x1bcu,0x1dcu,0x21cu})c.put8(record+offset,missing);
    c.put_f32(record+0x23c,empty_average);
    for(auto offset:{0x25cu,0x27cu})c.put8(record+offset,missing);
    for(auto offset:{0x29cu,0x2b8u})c.put16(record+offset,missing);
    for(auto offset:{0x2fcu,0x340u})c.put32(record+offset,missing);
    c.put16(record+0x35c,missing);
    for(auto offset:{0x378u,0x3a4u,0x3d0u,0x3d8u,0x3ecu})c.put32(record+offset,missing);
    c.set(3,1);return 1;
}

std::uint32_t modern_record(Cpu& c,std::uint32_t manager,std::uint32_t club,std::uint32_t create) {
    Frame frame(c,0x1001b830,0xa0,{.first_gpr=26});Program p{c};Manager records{c,manager};
    keep(c,{{30,c.toc(slot::success)},{26,c.toc(slot::message)},{29,manager},{27,c.toc(slot::pool)}});
    if(compare_logical(c,club)==0) {
        assertion(c,frame,0x1001b86c,0x68,0x40,0x1680);c.set(3,0);return 0;
    }
    Kept<31> indexes(c,records.indexes());Kept<28> index_offset(c,c.u32(club)*index_size);
    const auto index_address=std::uint32_t(indexes)+std::uint32_t(index_offset);
    if(compare(c,c.i32(index_address),-1)==0 && compare(c,std::int8_t(create))!=0) {
        c.set(3,records.modern_capacity());
        if(compare(c,records.modern_count(),c.r(3))==0) {
            records.modern_capacity()=c.r(3)+50;
            const auto allocation=p.call(0x107751b0,0x1001b8f8,{records.modern(),records.modern_capacity().get()*record_size});
            records.modern()=allocation;
            if(compare_logical(c,records.modern())==0) {
                assertion(c,frame,0x1001b91c,0x48,0x38,0x1694);
                p.call(0x1076f7d0,0x1001b960,{missing});
            }
        }
        const auto next=records.modern_count().get();c.set(3,next);records.modern_count()=next+1;c.put32(index_address,next);
        p.call(0x1001c160,0x1001b984,{records.modern().get()+c.u32(index_address)*record_size});
    }
    const auto index=c.i32(index_address);
    const auto result=compare(c,index,-1)>0?records.modern().get()+std::uint32_t(index)*record_size:0;
    c.set(3,result);return result;
}

std::uint32_t season_record(Cpu& c,std::uint32_t manager,std::uint32_t club,std::uint32_t create) {
    Frame frame(c,0x1001b9c0,0xa0,{.first_gpr=27});Program p{c};Manager records{c,manager};
    keep(c,{{31,c.toc(slot::success)},{27,c.toc(slot::message)},{29,manager},{28,c.toc(slot::pool)}});
    if(compare_logical(c,club)==0) {
        assertion(c,frame,0x1001b9fc,0x68,0x40,0x16b5);c.set(3,0);return 0;
    }
    Kept<30> index_address(c,records.indexes().get()+c.u32(club)*index_size);c.set(3,records.indexes());
    if(compare(c,c.i32(index_address+4),-1)==0 && compare(c,std::int8_t(create))!=0) {
        c.set(3,records.season_capacity());
        if(compare(c,records.season_count(),c.r(3))==0) {
            records.season_capacity()=c.r(3)+50;
            records.season()=p.call(0x107751b0,0x1001ba8c,{records.season(),records.season_capacity().get()*record_size});
            if(compare_logical(c,records.season())==0) {
                assertion(c,frame,0x1001bab0,0x48,0x38,0x16c9);p.call(0x1076f7d0,0x1001baf4,{missing});
            }
        }
        const auto next=records.season_count().get();c.set(3,next);records.season_count()=next+1;c.put32(index_address+4,next);
        p.call(0x1001c160,0x1001bb18,{records.season().get()+c.u32(index_address+4)*record_size});
    }
    const auto index=c.i32(index_address+4);
    const auto result=compare(c,index,-1)>0?records.season().get()+std::uint32_t(index)*record_size:0;
    c.set(3,result);return result;
}

std::uint32_t append_pending(Cpu& c,std::uint32_t manager,std::uint32_t club,std::uint32_t kind) {
    Frame frame(c,0x1001bd60,0x80,{.first_gpr=27});Program p{c};Manager records{c,manager};
    keep(c,{{31,manager},{29,c.toc(slot::message)},{30,c.toc(slot::pool)},{27,club},{28,kind}});
    c.set(3,records.pending_capacity());
    if(compare(c,records.pending_count(),c.r(3))==0) {
        records.pending_capacity()=c.r(3)+5;
        records.pending()=p.call(0x107751b0,0x1001bdac,{records.pending(),records.pending_capacity().get()*8});
        if(compare_logical(c,records.pending())==0) {
            assertion(c,frame,0x1001bdd0,0x40,0x38,0x174f,true);c.set(3,0);return 0;
        }
    }
    c.set({{3,1},{4,records.pending()}});
    c.put32(c.r(4)+records.pending_count().get()*8,club);
    c.set(4,records.pending());c.set(4,c.r(4)+records.pending_count().get()*8);
    c.put8(c.r(4)+4,kind);c.set(4,records.pending_count());records.pending_count()=c.r(4)+1;
    return 1;
}

std::uint32_t destroy(Cpu& c,std::uint32_t manager,std::uint32_t deleting) {
    Frame frame(c,0x1000ea50,0x40,{.first_gpr=30});Program p{c};
    keep(c,{{31,deleting},{30,manager}});
    if(compare(c,manager)!=0) {
        c.set(3,c.u32(manager));
        if(compare_logical(c,c.r(3))!=0) {p.call(0x10748c80,0x1000ea84,{c.r(3),1});c.put32(manager,0);}
        for(auto offset:{4u,0x10u,0x1cu,0x24u}) {
            c.set(3,c.u32(manager+offset));
            if(compare_logical(c,c.r(3))!=0) {
                const auto back=offset==4?0x1000ea9cu:offset==0x10?0x1000eabcu:offset==0x1c?0x1000eadcu:0x1000eaf8u;
                p.call(0x10775170,back);
                c.put32(manager+offset,0);
                c.put32(manager+offset+4,0);
                if(offset!=0x1c)c.put32(manager+offset+8,0);
            }
        }
        if(compare(c,std::int16_t(deleting))>0)p.call(0x107735a0,0x1000eb18,{manager});
    }
    c.set(3,manager);return manager;
}
}
