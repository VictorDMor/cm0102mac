#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_club_information(State& s,std::uint32_t display,GuiCanvas canvas) {
    guest::Cpu c(s);
    const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
    const auto read=[&](std::uint32_t at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
    const auto write=[&](std::uint32_t at,ScreenRect box){
        c.put32(at,box.left); c.put32(at+4,box.top); c.put32(at+8,box.right); c.put32(at+12,box.bottom);
    };
    const unsigned grids=std::min<unsigned>(250,c.u16(display+0x129da4));
    int backing=-1,ledger=-1,summary=-1,staff=-1,statistics=-1;
    for(unsigned i=0;i<grids;++i) {
        const auto at=grid(i);
        const auto box=read(at);
        const auto columns=c.u8(at+0xbb0),rows=c.u8(at+0xbb1);
        if(columns==5 && rows==14) {
            if(box==ScreenRect{110,190,780,500} && c.u32(at+0x18)==2)backing=int(i);
            if(box==ScreenRect{110,190,770,500} && c.u32(at+0x18)==1)ledger=int(i);
        } else if(columns==2 && c.u32(at+0x18)==2) {
            if(box==ScreenRect{110,190,780,332} && rows==6)summary=int(i);
            if(box==ScreenRect{110,337,780,500} && rows==7)staff=int(i);
            if(box==ScreenRect{110,190,780,500} && rows==17)statistics=int(i);
        }
    }
    // Identify the actual original branch, including a General Info fallback.
    // Never reinterpret popup grids or fields based only on requested history.
    const bool finance=backing>=0 && ledger>=0, general=summary>=0 && staff>=0;
    if(!finance && !general && statistics<0)return;
    const ScreenLayout shell{canvas};
    auto available=shell.content(190); available.bottom-=45; // lower section tabs
    const int width=std::min(1600,available.width());
    available.left+=(available.width()-width)/2; available.right=available.left+width;
    ScreenRect panel=available,summary_box=available,staff_box=available;
    if(general) {
        // Keep the six metadata rows compact. Extra height reveals more of
        // the original populated staff list, within AutoGrid's 30-row limit.
        const std::array<FormTrack,3> rows{{{142},{5},{163,1,30*21+16}}};
        const auto sections=form_tracks(available,rows,false);
        summary_box=sections[0]; staff_box=sections[2]; panel.bottom=staff_box.bottom;
    } else {
        const std::array<FormTrack,1> rows{{{310,1,(finance?14:17)*28}}};
        panel=form_tracks(available,rows,false).front();
    }
    auto cells=panel; cells.right-=10; // original inset of the financial text layer
    const int gutter=finance && c.i32(grid(ledger)+0x14)+1>14?22:0;
    GuiGuestScope preserve(s);
    c.set(1,c.sp()-0x400);
    for(unsigned i=0,count=std::min<unsigned>(1200,c.u16(display+0x129da6));i<count;++i) {
        const auto at=display+0xba864+i*0x17c;
        if(finance && c.i16(at+0x72)==ledger) {
            // The header separator spans all five columns with explicit X;
            // ordinary cells retain AutoGrid's original 2:1:1:1:1 weights.
            if(c.i32(at+0x28)==125 && c.i32(at+0x2c)==765) {
                c.put32(at+0x28,panel.left+15); c.put32(at+0x2c,cells.right-5-gutter);
            }
        } else if(general && c.i16(at+0x72)==staff) {
            // The staff heading and underline use measured text widths.
            // Translate their explicit X coordinates without stretching text.
            if(c.i32(at+0x28)!=0 || c.i32(at+0x2c)!=0) {
                c.put32(at+0x28,c.i32(at+0x28)+panel.left-110);
                c.put32(at+0x2c,c.i32(at+0x2c)+panel.left-110);
            }
        } else if(c.i16(at+0x72)<0) {
            const auto before=read(at+0x10); auto after=before;
            if(before==ScreenRect{100,10,790,70})after=shell.title();
            else if(before==ScreenRect{110,150,780,185})after={panel.left,150,panel.right,185};
            else if(before==ScreenRect{110,125,235,145})after={panel.left,125,panel.left+125,145};
            else if(before==ScreenRect{710,15,785,35} || before==ScreenRect{660,4,785,24} ||
                    before==ScreenRect{620,4,785,24})after=shell.anchor(before,LayoutAxis::trailing,LayoutAxis::fixed);
            else continue;
            write(at+0x10,after);
            c.put32(at+0x28,after.left); c.put32(at+0x2c,after.right);
            c.put32(at+0x30,after.top); c.put32(at+0x34,after.bottom);
        }
    }
    for(unsigned i=0;i<grids;++i) {
        auto box=read(grid(i));
        if(int(i)==backing || int(i)==statistics)box=panel;
        else if(int(i)==ledger)box=cells;
        else if(int(i)==summary)box=summary_box;
        else if(int(i)==staff) {
            box=staff_box;
            const auto rows=std::clamp((box.height()-16)/21,7,30);
            // The original constructor initializes weights only for its seven
            // visible rows. New rows need that same unit weight before refresh.
            for(int row=c.u8(grid(i)+0xbb1);row<rows;++row)c.put8(grid(i)+0xbd0+row,1);
            c.put8(grid(i)+0xbb1,rows);
            c.set({{3,grid(i)},{4,c.u32(grid(i)+0x1fc)}}); c.call(0x107145f0,0xfffffff0);
        }
        else if(box==ScreenRect{100,80,790,115})box.right=canvas.width-10;
        else if(box==ScreenRect{100,510,790,545})box={100,canvas.height-90,canvas.width-10,canvas.height-55};
        else continue;
        write(grid(i),box);
        c.set(3,grid(i)); c.call(0x10714630,0xfffffff0);
        c.set({{3,grid(i)},{4,0}}); c.call(0x10714110,0xfffffff0);
    }
}
}
