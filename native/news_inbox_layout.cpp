#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
// The builder owns row capacity and rich-text wrapping. This pass only sizes
// the shared tab strips and assigns a compact date column to the headline list.
void layout_news_inbox(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const auto history=c.u32(c.toc(-0x7bb0)+0x10);
    const auto tab=c.u32(history+0x1c);
    const auto list=c.u32(history+0x14+(11+tab*3)*8);
    const unsigned count=std::min<unsigned>(250,c.u16(display+0x129da4));
    if(list>=count)return;
    const auto panel=screen.rect(screen.grid(list));
    const auto rows=screen.rows(screen.grid(list));
    if(panel.left!=110 || panel.top!=125 || screen.columns(screen.grid(list))!=2 || !rows || rows>30)return;
    auto usable=panel;
    if(c.i32(screen.grid(list)+0x14)+1>rows)usable.right-=22;
    const std::array<FormTrack,2> columns{{{130},{518,1}}};
    const auto tracks=form_tracks(usable,columns,true);
    GuiGuestScope preserve(s);
    c.set(1,c.sp()-0x400);
    const unsigned controls=std::min<unsigned>(1200,c.u16(display+0x129da6));
    for(unsigned i=0;i<controls;++i) {
        const auto at=screen.control(i);
        if(c.i16(at+0x72)==int(list)) {
            const auto col=c.u32(at+0x20);
            if(col<tracks.size()) {
                c.put32(at+0x28,tracks[col].left);
                c.put32(at+0x2c,tracks[col].right-(col==0?2:0));
            }
        } else if(c.i16(at+0x72)<0 && screen.rect(at+0x10)==ScreenRect{100,10,790,70}) {
            const auto title=ScreenLayout{canvas}.title();
            screen.write(at+0x10,title);
            c.put32(at+0x28,title.left); c.put32(at+0x2c,title.right);
            c.put32(at+0x30,title.top); c.put32(at+0x34,title.bottom);
        }
    }
    for(unsigned i=0;i<count;++i) {
        auto box=screen.rect(screen.grid(i));
        if(box.left==100 && box.right==790 && box.height()==35 &&
           (box.top==80 || box.top==canvas.height-90)) {
            box.right=canvas.width-10;
            screen.write(screen.grid(i),box);
        } else if(i!=list)continue;
        screen.measure(screen.grid(i));
        screen.publish(screen.grid(i));
    }
}
}
