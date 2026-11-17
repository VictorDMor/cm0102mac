#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
// The builder owns row capacity and rich-text wrapping. This pass only sizes
// the shared tab strips and assigns a compact date column to the headline list.
void layout_news_inbox(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const auto history=c.u32(c.toc(-0x7bb0)+0x10);
    const auto tab=c.u32(history+0x1c);
    const auto list=c.u32(history+0x14+(11+tab*3)*8);
    const unsigned count=std::min<unsigned>(250,c.u16(display+0x129da4));
    if(list>=count)return;
    const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
    const auto read=[&](std::uint32_t at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
    const auto write=[&](std::uint32_t at,ScreenRect box){
        c.put32(at,box.left); c.put32(at+4,box.top); c.put32(at+8,box.right); c.put32(at+12,box.bottom);
    };
    const auto panel=read(grid(list));
    const auto rows=c.u8(grid(list)+0xbb1);
    if(panel.left!=110 || panel.top!=125 || c.u8(grid(list)+0xbb0)!=2 || !rows || rows>30)return;
    auto usable=panel;
    if(c.i32(grid(list)+0x14)+1>rows)usable.right-=22;
    const std::array<FormTrack,2> columns{{{130},{518,1}}};
    const auto tracks=form_tracks(usable,columns,true);
    GuiGuestScope preserve(s);
    c.set(1,c.sp()-0x400);
    const unsigned controls=std::min<unsigned>(1200,c.u16(display+0x129da6));
    for(unsigned i=0;i<controls;++i) {
        const auto at=display+0xba864+i*0x17c;
        if(c.i16(at+0x72)==int(list)) {
            const auto col=c.u32(at+0x20);
            if(col<tracks.size()) {
                c.put32(at+0x28,tracks[col].left);
                c.put32(at+0x2c,tracks[col].right-(col==0?2:0));
            }
        } else if(c.i16(at+0x72)<0 && read(at+0x10)==ScreenRect{100,10,790,70}) {
            const auto title=ScreenLayout{canvas}.title();
            write(at+0x10,title);
            c.put32(at+0x28,title.left); c.put32(at+0x2c,title.right);
            c.put32(at+0x30,title.top); c.put32(at+0x34,title.bottom);
        }
    }
    for(unsigned i=0;i<count;++i) {
        auto box=read(grid(i));
        if(box.left==100 && box.right==790 && box.height()==35 &&
           (box.top==80 || box.top==canvas.height-90)) {
            box.right=canvas.width-10;
            write(grid(i),box);
        } else if(i!=list)continue;
        c.set(3,grid(i)); c.call(0x10714630,0xfffffff0);
        c.set({{3,grid(i)},{4,0}}); c.call(0x10714110,0xfffffff0);
    }
}
}
