#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>
#include <map>

namespace cm::aot {
void layout_next_match(State& s,std::uint32_t display,GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const unsigned grids=std::min<unsigned>(250,c.u16(display+0x129da4));
    int body=-1; bool empty=false,lower_tabs=false;
    for(unsigned i=0;i<grids;++i) {
        const auto at=screen.grid(i); const auto box=screen.rect(at);
        if(box==ScreenRect{100,510,790,545})lower_tabs=true;
        if(box.left!=110 || box.right!=780 || (box.bottom!=500 && box.bottom!=545) || c.u32(at+0x18)!=2)continue;
        if(box.top==190 && screen.columns(at)==2 && c.u8(at+0xbb2)==3 && c.u8(at+0xbb3)==5)body=int(i);
        else if(box.top==125 && screen.columns(at)==1) {body=int(i); empty=true;}
    }
    if(body<0)return;
    const auto before=screen.rect(screen.grid(body));
    const auto original_rows=screen.rows(screen.grid(body));
    if(!original_rows || original_rows>30)return;
    const ScreenLayout shell{canvas};
    auto available=shell.content(before.top);
    if(lower_tabs || before.bottom==500)available.bottom-=45;
    const int width=std::min(1600,available.width());
    available.left+=(available.width()-width)/2; available.right=available.left+width;
    auto panel=available;
    if(!empty) {
        const std::array<FormTrack,1> tracks{{{before.height(),1,30*21+16}}};
        panel=form_tracks(available,tracks,false).front();
    }
    const int rows=empty?original_rows:std::clamp((panel.height()-16)/21,int(original_rows),30);
    const auto last_row=c.i32(screen.grid(body)+0x14);
    // The original rich-text formatter uses a 20px gutter and a one-pixel
    // leading inset relative to the plain 3:5 AutoGrid value column.
    const auto value_start=[](ScreenRect box,bool overflow) {
        const int end=box.right-2-(overflow?20:0);
        return box.left+3+3*(end-box.left-2)/8;
    };
    const int new_value=value_start(panel,last_row+1>rows);
    const unsigned controls=std::min<unsigned>(1200,c.u16(display+0x129da6));
    std::map<unsigned,int> shifts;
    GuiGuestScope preserve(s);
    c.set(1,c.sp()-0x400);
    for(unsigned i=0;i<controls;++i) {
        const auto at=screen.control(i);
        if(c.i16(at+0x72)==body) {
            if(!c.i32(at+0x28) && !c.i32(at+0x2c))continue;
            const int left=c.i32(at+0x28);
            auto [group,inserted]=shifts.try_emplace(c.u32(at+0x24),panel.left-before.left);
            (void)inserted;
            // Rich text emits a kind-1 base before its kind-2 links. A left
            // heading and a value can share the same row, both in column zero.
            // Establish the shift at each base; a long heading's links must
            // not become value links just because their X crosses the column.
            if(c.u32(at+0xc)==1) {
                if(left==before.left+3)group->second=panel.left-before.left;
                else if(left==value_start(before,false) || left==value_start(before,true))
                    group->second=new_value-left;
            }
            const int shift=empty?(panel.left+panel.right-before.left-before.right)/2:group->second;
            c.put32(at+0x28,left+shift); c.put32(at+0x2c,c.i32(at+0x2c)+shift);
        } else if(c.i16(at+0x72)<0) {
            const auto old=screen.rect(at+0x10); auto box=old;
            if(old==ScreenRect{100,10,790,70})box=shell.title();
            else if(old==ScreenRect{110,150,780,185})box={panel.left,150,panel.right,185};
            else if(old==ScreenRect{710,15,785,35} || old==ScreenRect{660,4,785,24} || old==ScreenRect{620,4,785,24})
                box=shell.anchor(old,LayoutAxis::trailing,LayoutAxis::fixed);
            else continue;
            screen.write(at+0x10,box);
            c.put32(at+0x28,box.left); c.put32(at+0x2c,box.right);
            c.put32(at+0x30,box.top); c.put32(at+0x34,box.bottom);
        }
    }
    for(unsigned i=0;i<grids;++i) {
        auto box=screen.rect(screen.grid(i));
        if(int(i)==body) {
            box=panel;
            screen.show_rows(screen.grid(i), original_rows, rows);
            screen.set_rows(screen.grid(i), rows);
            if(!empty) {
                screen.resize_rows(screen.grid(i));
            }
        } else if(box==ScreenRect{110,125,360,145})box={panel.left,125,panel.left+250,145};
        else if(box==ScreenRect{530,125,780,145})box={panel.right-250,125,panel.right,145};
        else if(box==ScreenRect{100,80,790,115})box.right=canvas.width-10;
        else if(box==ScreenRect{100,510,790,545})box={100,canvas.height-90,canvas.width-10,canvas.height-55};
        else continue;
        screen.write(screen.grid(i),box);
        screen.measure(screen.grid(i));
        screen.publish(screen.grid(i));
    }
}
}
