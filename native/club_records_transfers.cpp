#include "club_records_detail.hpp"

namespace cm::aot::club_records {
using namespace detail;
namespace {
void copy_date(Cpu& c,std::uint32_t destination,std::uint32_t source) {
    c.put16(destination+2,c.u16(source+2));c.put16(destination,c.u16(source));c.put8(destination+4,c.u8(source+4));
}
void queue_record(Cpu& c,const Program& p,std::uint32_t manager,std::uint32_t club,unsigned kind,
                  std::uint32_t check_back,std::uint32_t append_back) {
    if(!nonzero_byte(c,p.call(0x1001be60,check_back,{manager,c.u32(club),kind})))
        p.call(0x1001bd60,append_back,{manager,c.u32(club),kind});
}
}

std::uint32_t record_transfer(Cpu& c,std::uint32_t manager,std::uint32_t offer) {
    Frame frame(c,0x10012fb0,0x60,{.first_gpr=24});Program p{c};
    keep(c,{{24,manager},{25,offer},{28,c.toc(slot::clubs)},{30,c.toc(-0x7b78)},{29,c.toc(-0x7b80)}});
    Kept<27> player(c,c.u32(c.toc(-0x7be0))+c.u32(offer+12)*0x70);
    Kept<26> buyer(c,c.u32(c.r(28))+c.u32(offer+8)*0x248);
    c.set(5,c.u32(c.r(28)));
    const auto contract=p.call(0x10082f20,0x10013008,{c.u32(c.toc(-0x7b7c)),player});
    c.put32(c.r(30),contract);
    if(compare_logical(c,contract)!=0) {c.set(3,c.u32(c.r(28)));c.set(28,c.r(3)+c.u32(contract+4)*0x248);}
    else c.set(28,0);
    const auto seller=c.r(28);
    Kept<31> season(c,p.call(0x1001b9c0,0x1001303c,{manager,buyer,1}));
    const auto modern_result=p.call(0x1001b830,0x10013050,{manager,buyer,1});
    compare_logical(c,seller);Kept<30> modern(c,modern_result);
    if(seller && compare(c,c.i32(offer+0x20))!=0) {
        c.set(3,c.u32(offer+0x20));
        if(compare(c,c.r(3),c.i32(season+0x378))>0) {
            c.put32(season+0x378,c.r(3));p.call(0x1030d6e0,0x10013084,{player,season+0x37c});
            c.put32(season+0x394,c.u32(seller));copy_date(c,season+0x398,c.r(29));
            c.set(3,c.u32(offer+4));c.put8(season+0x3a0,c.r(3)!=0);
            c.set(3,c.u32(offer+0x20));
            if(compare(c,c.r(3),c.i32(modern+0x378))>0) {
                assign(c,modern+0x378,season+0x378,RecordKind::transfer);
                c.set(4,c.u32(modern+0x378));c.set(3,std::uint32_t(c.i16(buyer+0x1ec)*1000)+0x50000);
                if(compare(c,c.r(4),c.r(3)-0x6c20)>=0) {
                    queue_record(c,p,manager,buyer,28,0x1001315c,0x10013174);
                    p.call(0x100d9960,0x10013184,{c.u32(buyer+0x80),player,1});
                }
            }
        }
    }
    if(compare_logical(c,seller)!=0) {
        const auto seller_season=p.call(0x1001b9c0,0x1001319c,{manager,seller,1});c.set(30,seller_season);
        const auto seller_modern=p.call(0x1001b830,0x100131b0,{manager,seller,1});c.set(31,seller_modern);
        c.set(4,c.u32(offer+0x20));
        if(compare(c,c.r(4))!=0 && compare(c,c.r(4),c.i32(seller_season+0x3a4))>0) {
            c.put32(seller_season+0x3a4,c.r(4));p.call(0x1030d6e0,0x100131dc,{player,seller_season+0x3a8});
            c.put32(seller_season+0x3c0,c.u32(buyer));copy_date(c,seller_season+0x3c4,c.r(29));
            c.set(3,c.u32(offer+4));c.put8(seller_season+0x3cc,c.r(3)!=0);
            c.set(3,c.u32(offer+0x20));
            if(compare(c,c.r(3),c.i32(seller_modern+0x3a4))>0) {
                assign(c,seller_modern+0x3a4,seller_season+0x3a4,RecordKind::transfer);
                c.set(4,c.u32(seller_modern+0x3a4));c.set(3,std::uint32_t(c.i16(seller+0x1ec)*1000)+0x50000);
                if(compare(c,c.r(4),c.r(3)-0x6c20)>=0)
                    queue_record(c,p,manager,seller,29,0x100132b4,0x100132cc);
            }
        }
    }
    c.set(3,1);return 1;
}

std::uint32_t record_transfer_totals(Cpu& c,std::uint32_t manager,std::uint32_t amount,std::uint32_t seller,std::uint32_t buyer) {
    Frame frame(c,0x100132f0,0x60,{.first_gpr=25});Program p{c};
    keep(c,{{31,seller},{29,manager},{30,amount},{25,buyer}});
    if(compare(c,seller)==0 || compare_logical(c,buyer)==0){c.set(3,1);return 1;}
    Kept<26> buyer_season(c,p.call(0x1001b9c0,0x10013330,{manager,buyer,1}));
    Kept<28> modern(c,p.call(0x1001b830,0x10013344,{manager,buyer,1}));
    const auto spent=c.u32(buyer_season+0x3d8);
    c.put32(buyer_season+0x3d8,compare(c,spent,-1)==0?amount:spent+amount);
    c.put16(buyer_season+0x3dc,0);
    Kept<27> buyer_year(c,p.call(0x10192610,0x10013374,{buyer}));
    c.set(4,c.u32(buyer_season+0x3d8));
    if(compare(c,c.r(4),c.i32(modern+0x3d8))>0) {
        c.set(3,std::uint32_t(c.i16(buyer+0x1ec)*1000)+0x80000);
        if(compare(c,c.r(4),c.r(3)-0x5ee0)>=0) {
            c.set(3,std::uint32_t(buyer_year)&0xffff);
            if(compare_logical(c,c.r(3),c.u16(modern+0x3dc))!=0)
                queue_record(c,p,manager,buyer,31,0x100133c0,0x100133d8);
        }
        c.put32(modern+0x3d8,c.u32(buyer_season+0x3d8));c.put16(modern+0x3dc,buyer_year);
    }
    const auto seller_season_result=p.call(0x1001b9c0,0x100133f4,{manager,seller,1});
    c.set(27,seller_season_result);const auto seller_season=c.r(27);
    modern=p.call(0x1001b830,0x10013408,{manager,seller,1});
    const auto income=c.u32(seller_season+0x3d0);
    c.put32(seller_season+0x3d0,compare(c,income,-1)==0?amount:income+amount);
    c.put16(seller_season+0x3d4,0);
    c.set(30,p.call(0x10192610,0x10013438,{seller}));c.set(4,c.u32(modern+0x3d0));
    if(compare(c,c.i32(seller_season+0x3d0),c.r(4))>0) {
        c.set(3,std::uint32_t(c.i16(seller+0x1ec)*1000)+0x80000);
        // The original tests the previous modern-day income for this threshold.
        if(compare(c,c.r(4),c.r(3)-0x5ee0)>=0)
            queue_record(c,p,manager,seller,30,0x10013474,0x1001348c);
        c.put32(modern+0x3d0,c.u32(seller_season+0x3d0));c.put16(modern+0x3d4,c.r(30));
    }
    c.set(3,1);return 1;
}

std::uint32_t record_player_totals(Cpu& c,std::uint32_t manager,std::uint32_t player,std::uint32_t club,
                                   std::uint32_t appearances,std::uint32_t goals) {
    Frame frame(c,0x100134b0,0xb0,{.first_gpr=23});Program p{c};
    keep(c,{{24,player},{28,c.toc(slot::success)},{29,c.toc(slot::message)},{23,manager},
            {30,c.toc(slot::pool)},{25,club},{26,appearances},{27,goals}});
    if(compare(c,player)==0 || compare_logical(c,club)==0) {
        assertion(c,frame,0x10013500,0x68,0x40,0x6fa);c.set(3,0);return 0;
    }
    Kept<31> modern(c,p.call(0x1001b830,0x10013550,{manager,club,1}));
    if(compare(c,modern)==0){assertion(c,frame,0x1001356c,0x48,0x38,0x702);c.set(3,0);return 0;}
    if(nonzero_byte(c,p.call(0x10013cc0,0x100135c4,{manager,23,club,1}))) {
        const auto previous=c.i16(modern+0x29c);c.set(3,std::uint32_t(previous));
        if(compare(c,previous,-1)==0 || compare(c,std::int16_t(appearances),previous)>0) {
            c.put16(modern+0x29c,appearances);p.call(0x1030d6e0,0x100135f4,{player,modern+0x2a0});
        }
    }
    if(nonzero_byte(c,p.call(0x10013cc0,0x10013608,{manager,24,club,1}))) {
        const auto previous=c.i16(modern+0x2b8);c.set(3,std::uint32_t(previous));
        if(compare(c,previous,-1)==0 || compare(c,std::int16_t(goals),previous)>0) {
            c.put16(modern+0x2b8,goals);p.call(0x1030d6e0,0x10013638,{player,modern+0x2bc});
        }
    }
    c.set(3,1);return 1;
}
}
