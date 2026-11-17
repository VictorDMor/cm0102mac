#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_league_selection(State& s, std::uint32_t display, GuiCanvas canvas, bool selector) {
    guest::Cpu c(s);
    const auto grid = [display](unsigned i) { return display + 4 + i * 0xbf0; };
    const auto rect = [&](unsigned at) { return ScreenRect{c.i32(at), c.i32(at+4), c.i32(at+8), c.i32(at+12)}; };
    const auto write = [&](unsigned at, ScreenRect r) {
        c.put32(at,r.left); c.put32(at+4,r.top); c.put32(at+8,r.right); c.put32(at+12,r.bottom);
    };
    int options=-1, size=-1, countries=-1, footer=-1;
    const ScreenLayout shell{canvas};
    for (unsigned i=0;i<std::min<unsigned>(250,c.u16(display+0x129da4));++i) {
        const auto box=rect(grid(i));
        if (box==ScreenRect{110,145,525,165}) options=int(i);
        if (box==ScreenRect{530,145,780,165}) size=int(i);
        if (box==ScreenRect{110,170,780,535}) countries=int(i);
        if ((box==ScreenRect{100,555,790,590} || box==shell.footer()) && c.u8(grid(i)+0xbb1)==1) footer=int(i);
    }
    if (options<0 || size<0 || countries<0) return;
    auto panel=shell.content(); panel.bottom-=10;
    const int width=std::min(1120,panel.width()); panel.left+=(panel.width()-width)/2; panel.right=panel.left+width;
    const std::array<FormTrack,5> tracks{{{selector?30:0},{selector?10:0},{30},{10},{240,1}}};
    const auto sections=form_tracks(panel,tracks,false);
    const std::array<FormTrack,3> columns{{{415,1},{5},{250,0,320}}};
    const auto config=form_tracks(sections[2],columns,true);
    GuiGuestScope preserve(s); c.set(1,c.sp()-0x400);
    write(grid(options),config[0]); write(grid(size),config[2]); write(grid(countries),sections[4]);
    const int rows=std::clamp(sections[4].height()/25,8,30);
    for (int row=0;row<rows;++row) c.put8(grid(countries)+0xbd0+row,1);
    c.put8(grid(countries)+0xbb1,rows);
    if(c.i32(grid(countries)+0x14)+1<=rows)c.put8(grid(countries)+0xbef,0);
    c.set({{3,grid(countries)},{4,c.u32(grid(countries)+0x1fc)}});c.call(0x107145f0,0xfffffff0);
    if(footer>=0) {
        write(grid(footer),{panel.right-340,canvas.height-45,panel.right,canvas.height-10});
        for(unsigned col=0;col<c.u8(grid(footer)+0xbb0);++col)c.put8(grid(footer)+0xbb2+col,1);
    }
    for(int i:{options,size,countries,footer}) if(i>=0) {
        c.set(3,grid(i));c.call(0x10714630,0xfffffff0);
        c.set({{3,grid(i)},{4,0}});c.call(0x10714110,0xfffffff0);
    }
}
}
