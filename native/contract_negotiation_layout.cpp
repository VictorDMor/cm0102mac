#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
void layout_contract_negotiation(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
    const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
    const auto read=[&](std::uint32_t at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
    const auto write=[&](std::uint32_t at,ScreenRect box){
        c.put32(at,box.left); c.put32(at+4,box.top); c.put32(at+8,box.right); c.put32(at+12,box.bottom);
    };
    int title=-1,tab_background=-1,tabs=-1,fields=-1,amounts=-1,footer=-1;
    const unsigned grid_count=std::min<unsigned>(250,c.u16(display+0x129da4));
    const unsigned control_count=std::min<unsigned>(1200,c.u16(display+0x129da6));
    for(unsigned i=0;i<grid_count;++i) {
        const auto box=read(grid(i)); const unsigned columns=c.u8(grid(i)+0xbb0);
        if(box==ScreenRect{100,10,790,70})title=int(i);
        if(box==ScreenRect{100,140,790,175}) {
            if(columns==1)tab_background=int(i);
            if(columns==3)tabs=int(i);
        }
        if(box==ScreenRect{110,185,780,545}) {
            if(fields<0)fields=int(i);
            else amounts=int(i);
        }
        if(box==ScreenRect{100,555,790,590} && c.u8(grid(i)+0xbb1)==1)footer=int(i);
    }
    if(title<0 || tabs<0 || tab_background<0 || fields<0 || footer<0)return;
    const auto history=c.u32(c.toc(-0x7bb0)+0x10);
    const unsigned tab=c.u32(history+0x1c),columns=c.u8(grid(fields)+0xbb0),rows=c.u8(grid(fields)+0xbb1);
    if(!rows || rows>30 || (tab==1 && columns!=7) || (tab==2 && columns!=9) ||
       (tab==3 && (columns!=3 || amounts<0 || c.u8(grid(amounts)+0xbb0)!=9 || c.u8(grid(amounts)+0xbb1)!=rows)))return;
    const unsigned button_count=c.u8(grid(footer)+0xbb0);
    if(button_count<2 || button_count>3)return;
    const ScreenLayout shell{canvas};
    auto available=shell.content(185);
    const int width=std::min(960,available.width());
    available.left+=(available.width()-width)/2; available.right=available.left+width;
    // Preserve visible rows and scroll state. Sparse tabs keep their legend
    // at the original last row, while row spacing has a finite upper bound.
    const std::array<FormTrack,1> sections{{{360,1,int(rows)*30+16}}};
    const auto body=form_tracks(available,sections,false).front();
    const ScreenRect tab_bar{body.left-10,140,body.right+10,175};
    // AutoGrid reserves 22px when its last populated row exceeds capacity.
    // Explicit X overrides must reserve it too, including before the first
    // AutoGrid refresh has created the scrollbar.
    const int gutter=(c.i32(grid(fields)+0x14)+1>int(rows) ||
                     (amounts>=0 && c.i32(grid(amounts)+0x14)+1>int(rows)))?22:0;
    auto field_space=body; field_space.right-=gutter;
    const std::array<FormTrack,7> basic{{{32},{335-gutter,1},{32},{168},{32},{32},{32}}};
    const std::array<FormTrack,9> detailed{{{32},{18},{236-gutter,1},{32},{144},{32},{32},{16},{128}}};
    const auto tracks=tab==1?form_tracks(field_space,basic,true):form_tracks(field_space,detailed,true);
    auto buttons=shell.buttons(std::min(body.width(),int(button_count)*190));
    buttons.left-=buttons.right-body.right; buttons.right=body.right;
    GuiGuestScope preserve(s);
    c.set(1,c.sp()-0x400);
    for(unsigned i=0;i<control_count;++i) {
        const auto at=control(i); const int owner=c.i16(at+0x72);
        const int left=c.i32(at+0x28),right=c.i32(at+0x2c);
        if(owner==title && left && right) {
            const auto box=DetailsLayout::centered_text({left,0,right,0},read(grid(title)),shell.title());
            c.put32(at+0x28,box.left); c.put32(at+0x2c,box.right);
        } else if(owner==fields || (amounts>=0 && owner==amounts)) {
            if(left && right) {
                // Paragraphs and their separately measured links move by the
                // same amount. Expanding available width never scales glyphs.
                c.put32(at+0x28,left+body.left-110);
                c.put32(at+0x2c,!c.u32(at+8) && right==770?field_space.right-10:right+body.left-110);
            } else {
                const unsigned col=c.u32(at+0x20);
                if(tab==3 && owner==fields && col==2) {
                    // Clause labels span the remaining row except on the
                    // amount row, which shares its space with another grid.
                    bool amount_row=false;
                    for(unsigned j=0;j<control_count && !amount_row;++j)
                        amount_row=c.i16(control(j)+0x72)==amounts && c.u32(control(j)+0x24)==c.u32(at+0x24);
                    c.put32(at+0x28,tracks[2].left+2);
                    c.put32(at+0x2c,amount_row?tracks[2].right-2:field_space.right-2);
                } else if(col<tracks.size()) {
                    c.put32(at+0x28,tracks[col].left+2); c.put32(at+0x2c,tracks[col].right-2);
                }
            }
        } else if(owner<0) {
            const auto before=read(at+0x10); auto after=before;
            if(before==ScreenRect{100,80,790,125})after=shell.subtitle();
            else if(before==ScreenRect{730,85,785,105})after=shell.anchor(before,LayoutAxis::trailing,LayoutAxis::fixed);
            else continue;
            write(at+0x10,after);
            c.put32(at+0x28,after.left); c.put32(at+0x2c,after.right);
            c.put32(at+0x30,after.top); c.put32(at+0x34,after.bottom);
        }
    }
    for(unsigned i=0;i<button_count;++i)c.put8(grid(footer)+0xbb2+i,1);
    std::vector<std::pair<int,ScreenRect>> panels{{title,shell.title()},{tab_background,tab_bar},
        {tabs,tab_bar},{fields,body},{footer,buttons}};
    // Style 1 has no inset; the clauses' backing grid has 8px vertically.
    // Equal inner rectangles align corresponding rows. Each grid retains
    // the original independently managed scroll position.
    if(amounts>=0)panels.push_back({amounts,{body.left,body.top+8,body.right,body.bottom-8}});
    for(const auto& [i,box]:panels) {
        write(grid(i),box);
        c.set(3,grid(i)); c.call(0x10714630,0xfffffff0);
        c.set({{3,grid(i)},{4,0}}); c.call(0x10714110,0xfffffff0);
    }
}
}
