#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
// Loan editing, the three confirmation builders and the message screens of the same shape
// (screen_layout.hpp) share a content panel, title/subtitle and a footer of one to three buttons.
// Original builders retain all state and controls.
void layout_negotiation_dialog(State& s, std::uint32_t display, GuiCanvas canvas, bool loan_terms) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    int title=-1,fields=-1,restore=-1,footer=-1;
    const unsigned grid_count=std::min<unsigned>(250,c.u16(display+0x129da4));
    const unsigned control_count=std::min<unsigned>(1200,c.u16(display+0x129da6));
    for(unsigned i=0;i<grid_count;++i) {
        const auto box=screen.rect(screen.grid(i));
        if(box==ScreenRect{100,10,790,70})title=int(i);
        if(box==ScreenRect{110,140,780,545} || (loan_terms && box==ScreenRect{110,165,780,545}))fields=int(i);
        if(loan_terms && box==ScreenRect{530,140,780,160})restore=int(i);
        if(box==ScreenRect{100,555,790,590})footer=int(i);
    }
    if(fields<0 || footer<0)return;
    const auto old=screen.rect(screen.grid(fields));
    const unsigned rows=screen.rows(screen.grid(fields)),buttons=screen.columns(screen.grid(footer));
    if(!rows || rows>30 || screen.columns(screen.grid(fields))!=(loan_terms?7:1) ||
       c.u32(screen.grid(fields)+0x18)!=2 || screen.rows(screen.grid(footer))!=1 || buttons<1 || buttons>3)return;
    const ScreenLayout shell{canvas};
    auto available=shell.content(old.top);
    const int width=std::min(960,available.width());
    available.left+=(available.width()-width)/2; available.right=available.left+width;
    const std::array<FormTrack,1> sections{{{old.height(),1,int(rows)*28+16}}};
    const auto body=form_tracks(available,sections,false).front();
    const int gutter=c.i32(screen.grid(fields)+0x14)+1>int(rows)?22:0;
    auto field_space=body; field_space.right-=gutter;
    // A wrapped confirmation and its links need the same text origin.
    // At the narrow overflow limit keep the original geometry rather than
    // independently clipping a paragraph and moving its clickable words.
    if(!loan_terms && field_space.width()<old.width())return;
    const std::array<FormTrack,7> columns{{{32},{335-gutter,1},{32},{168},{32},{32},{32}}};
    const auto tracks=loan_terms?form_tracks(field_space,columns,true):std::vector<ScreenRect>{};
    auto footer_box=shell.buttons(std::min(body.width(),int(buttons)*190));
    footer_box.left-=footer_box.right-body.right; footer_box.right=body.right;
    // Confirmation messages keep their original wrapping and measured links
    // together in a centered text column; their glyphs never stretch.
    const int shift=loan_terms?body.left-old.left:(field_space.left+field_space.right-old.left-old.right)/2;
    GuiGuestScope preserve(s);
    c.set(1,c.sp()-0x400);
    for(unsigned i=0;i<control_count;++i) {
        const auto at=screen.control(i); const int owner=c.i16(at+0x72);
        const int left=c.i32(at+0x28),right=c.i32(at+0x2c);
        if(title>=0 && owner==title && left && right) {
            const auto box=DetailsLayout::centered_text({left,0,right,0},screen.rect(screen.grid(title)),shell.title());
            c.put32(at+0x28,box.left); c.put32(at+0x2c,box.right);
        } else if(owner==fields) {
            if(left && right) {
                const int edge=field_space.right-(loan_terms?10:5);
                // Full paragraph backgrounds grow; measured clickable spans
                // move by the same offset and retain their measured widths.
                const int end=std::max(field_space.left+2,
                    loan_terms && !c.u32(at+8) && right==770?edge:std::min(right+shift,edge));
                c.put32(at+0x28,std::clamp(left+shift,field_space.left+2,end));
                c.put32(at+0x2c,end);
            } else if(loan_terms) {
                const unsigned col=c.u32(at+0x20);
                if(col<tracks.size()) {
                    c.put32(at+0x28,tracks[col].left+2); c.put32(at+0x2c,tracks[col].right-2);
                }
            }
        } else if(owner<0) {
            const auto before=screen.rect(at+0x10); auto after=before;
            if(before==ScreenRect{100,10,790,70})after=shell.title();
            else if(before==ScreenRect{100,80,790,125})after=shell.subtitle();
            else continue;
            screen.write(at+0x10,after);
            c.put32(at+0x28,after.left); c.put32(at+0x2c,after.right);
            c.put32(at+0x30,after.top); c.put32(at+0x34,after.bottom);
        }
    }
    for(unsigned i=0;i<buttons;++i)c.put8(screen.grid(footer)+0xbb2+i,1);
    std::vector<std::pair<int,ScreenRect>> panels{{fields,body},{footer,footer_box}};
    if(title>=0)panels.push_back({title,shell.title()});
    if(restore>=0)panels.push_back({restore,{body.right-250,140,body.right,160}});
    for(const auto& [i,box]:panels) {
        screen.write(screen.grid(i),box);
        screen.measure(screen.grid(i));
        screen.publish(screen.grid(i));
    }
}
}
