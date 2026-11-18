// February's career contains both a human-managed match and a computer match
// with a complete commentary archive. Read both using the original tab handler.
void archived_commentary_builder(const char* snapshot, const std::filesystem::path& user) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>();
    auto& s = *state;
    load_match_snapshot(s, snapshot);
    s.code_fragment_observer = &clock;
    guest::Cpu c(s);
    c.set(2, 0x108bba90);
    const auto manager = c.toc(-0x7bb0);
    c.put16(manager + 0x2c58, 0);
    s.platform = std::make_shared<NativePlatform>(temp.path/"game", user, AudioMode::offline);
    const auto scratch = allocate(s, 32, true);
    world(s, scratch, scratch+16);
    const auto call = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3;
        for (auto arg : args) c.set(reg++,arg);
        s.budget = s.executed + 400000000;
        c.call(entry,return_sentinel);
        return c.r(3);
    };
    require(s.platform->resize_gui_canvas(s,{801,601}), "Initialize commentary canvas");
    const auto display = c.u32(manager+0x24);
    const GuestScreen screen{c,display};
    const auto rebuild = [&] {
        call(0x10719560,{display,1,0});
        call(0x1028f280,{});
    };
    const auto label = [&](unsigned control) {
        const auto data = s.memory.copy_out(control+0x78,0x100);
        return std::string(data.begin(),std::find(data.begin(),data.end(),0));
    };
    const auto click = [&](unsigned i) {
        const auto box = screen.rect(screen.control(i)+0x10);
        c.put32(c.toc(-0x5c10),(box.left+box.right)/2);
        c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
        c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
        return std::int16_t(call(c.u8(display)==1 ? 0x1071aa10 : 0x1071a280,{display}));
    };
    const auto change_tab = [&](unsigned tab) {
        for (unsigned i = 0; i < c.u16(display+0x129da6); ++i) {
            const auto control = screen.control(i);
            if (c.u32(control+8) != 1 || c.u32(control+0x48) != tab || c.i16(control+0x72)<0) continue;
            const auto area = screen.grid(c.i16(control+0x72));
            if (screen.rows(area)!=1 || screen.columns(area)!=(tab<=4 ? 4 : 3)) continue;
            screen.measure(area); screen.publish(area);
            require(click(i)==int(i), "Archived report tab hit bounds");
            call(0x1028fa50,{i});
            return;
        }
        throw std::runtime_error("Missing archived report tab");
    };
    for (unsigned key : {1,5000}) {
        call(0x1028f070,{key});
        const auto history = c.u32(manager+0x10);
        const auto slot = [&](unsigned n) { return c.u32(history+0x14+8*n); };
        rebuild();
        change_tab(4);
        require(slot(2)==4 && slot(30)==key, "Wrong archived commentary");
        for (const auto [size, enabled] : {std::pair{GuiCanvas{800,600},true}, {{1280,900},false},
                  {{801,601},true}, {{1280,900},true}, {{3200,1800},true}}) {
            select_new_screens(enabled);
            require(s.platform->resize_gui_canvas(s,size), "Resize commentary canvas");
            rebuild();
            unsigned index = 0;
            while (index < c.u16(display+0x129da4) &&
                   (screen.rect(screen.grid(index))!=ScreenRect{110,125,780,500} ||
                    screen.columns(screen.grid(index))!=2)) ++index;
            require(index<c.u16(display+0x129da4) && slot(45), "Archived event file did not load");
            const auto area = screen.grid(index);
            const int last = c.i32(area+0x14);
            require(last==(key==1 ? 168 : 97), "Incomplete original archived commentary");
            const auto cell = [&](int row,int column) {
                for (unsigned i=0; i<c.u16(display+0x129da6); ++i) {
                    const auto control = screen.control(i);
                    if (c.i16(control+0x72)==int(index) && c.i32(control+0x24)==row &&
                        c.i32(control+0x20)==column) return control;
                }
                throw std::runtime_error("Missing archived commentary cell");
            };
            const auto archive = s.memory.copy_out(slot(45),0x4074), nav = s.memory.copy_out(history,0x1f4);
            const auto original = s.memory.copy_out(display,0x129da8), rng = random_state(s);
            const auto regs = s.registers;
            layout_built_screen(s,display,0x1028f280);
            require(s.registers==regs && rng==random_state(s) && nav==s.memory.copy_out(history,nav.size()) &&
                    archive==s.memory.copy_out(slot(45),archive.size()), "Commentary layout changed game state");
            if (!enabled || size.original()) {
                require(original==s.memory.copy_out(display,original.size()), "Classic commentary changed");
                continue;
            }
            const auto panel = screen.rect(area);
            require(panel.width()==std::min(1400,size.width-130) && panel.top==125 &&
                    screen.rows(area)==(size.height>=900 ? 28 : 20), "Commentary panel or visible row count");
            const int visible = screen.rows(area);
            const auto check_page = [&](int first) {
                for (int row=first; row<first+visible; ++row) {
                    const auto minute = screen.rect(cell(row,0)+0x10), text = screen.rect(cell(row,1)+0x10);
                    require(minute.left==panel.left && minute.width()>=77 && minute.width()<=81 &&
                            text.left==minute.right+2 && text.right==panel.right-22 &&
                            minute.top==text.top && minute.bottom==text.bottom &&
                            text.top>=panel.top && text.bottom<=panel.bottom, "Minute or text separated from its row");
                }
            };
            for (unsigned i = 0; i < c.u16(display+0x129da6); ++i) {
                const auto control = screen.control(i);
                if (c.i16(control+0x72) != int(index)) continue;
                for (const auto [offset,length] : {std::pair{0x20u,8u},{0x68u,10u},{0x78u,256u}}) {
                    const auto field = s.memory.copy_out(control+offset,length);
                    require(std::equal(field.begin(),field.end(),original.begin()+(control-display)+offset),
                            "Commentary layout altered event order, style or text");
                }
            }
            check_page(0);
            const auto final = cell(last,1);
            require(label(final)==" Soou o apito final" && screen.rect(final+0x10)==ScreenRect{0,0,0,0},
                    "Unexpected final archived event");
            // Click below the thumb, then page to the last line using the
            // original scrollbar. Verify that the column geometry follows it.
            c.put32(c.toc(-0x5c10),panel.right-10);
            c.put32(c.toc(-0x5c14),c.i32(area+0xb90));
            c.put8(c.toc(-0x7bac),1);
            for (int i=0; i<=last/visible && c.i32(area+0xb98)<last+1-visible; ++i)
                require(call(0x1071bd00,{display,index})==1, "Commentary scrollbar did not page");
            require(c.i32(area+0xb98)==last+1-visible, "Commentary scroll did not reach final whistle");
            check_page(last+1-visible);
        }
        if (key==1) {
            // Limit only the disposable in-memory event log. A short report
            // can overflow the original 20 rows and fit the enlarged page.
            const auto count_addr = slot(45)+6;
            const auto count = c.u16(count_addr);
            c.put16(count_addr,24);
            rebuild();
            bool checked = false;
            for (unsigned i=0; i<c.u16(display+0x129da4); ++i) {
                const auto area = screen.grid(i);
                if (screen.columns(area)!=2 || screen.rect(area)!=ScreenRect{110,125,780,500}) continue;
                const int total = c.i32(area+0x14)+1;
                require(total>20 && total<=28, "Expected a short overflowing commentary");
                layout_built_screen(s,display,0x1028f280);
                require(screen.rows(area)==28 && !c.u8(area+0xbef), "Obsolete commentary scrollbar");
                checked = true;
                break;
            }
            require(checked, "Missing short commentary");
            c.put16(count_addr,count);
        }
        change_tab(3);
        for (const auto [size, enabled] : {std::pair{GuiCanvas{800,600},true}, {{1280,900},false},
                                           {{800,900},true}, {{1280,600},true}, {{1280,900},true}, {{3200,1800},true}}) {
            select_new_screens(enabled);
            require(s.platform->resize_gui_canvas(s,size), "Resize archived zones");
            for (unsigned next : {0,1,3}) {
                rebuild();
                require(slot(2)==3 && slot(30)==key, "Unexpected archived zones");
                const unsigned first = slot(11); // first third's percentage
                const auto box = [&](unsigned index) { return screen.rect(screen.control(index)+0x10); };
                const auto original = s.memory.copy_out(display,0x129da8);
                const auto stats = s.memory.copy_out(slot(32),0xdfc), nav = s.memory.copy_out(history,0x1f4);
                const auto regs = s.registers;
                const auto rng = random_state(s);
                layout_built_screen(s,display,0x1028f280);
                require(s.registers==regs && rng==random_state(s) && nav==s.memory.copy_out(history,nav.size()) &&
                        stats==s.memory.copy_out(slot(32),stats.size()), "Zones layout changed match data");
                if (!enabled || size.original()) {
                    require(original==s.memory.copy_out(display,original.size()), "Classic zones changed");
                    break;
                }
                // The pitch shapes precede the percentage controls. The
                // possession strip and its two labels follow the three bars.
                require(first>=22 && first+23<c.u16(display+0x129da6), "Missing pitch components");
                const auto pitch = box(first-22), circle = box(first-16), possession = box(first+19);
                require(pitch.left>=110 && pitch.right<=size.width-20 && possession.bottom<=size.height-100 &&
                        pitch.bottom<possession.top && possession.left==pitch.left && possession.right==pitch.right &&
                        std::abs(pitch.width()*285-pitch.height()*520)<=650 && std::abs(circle.width()-circle.height())<=1,
                        "Pitch or possession strip distorted");
                if (size.height>=900) require(pitch.width()>520, "Pitch did not grow");
                for (unsigned j=0; j<3; ++j) {
                    const auto third = box(first+j), home = box(first+6+5*j), away = box(first+7+5*j);
                    require(third.left>=pitch.left && third.right<=pitch.right &&
                            (j==0 || third.left==box(first+j-1).right) &&
                            std::abs(third.width()-box(first).width())<=1 &&
                            third.left<=home.left && home.right==away.left && away.right<=third.right,
                            "Thirds or possession bars lost alignment");
                }
                for (unsigned i=first-22; i<=first+23; ++i) {
                    const auto control = screen.control(i);
                    const auto style_and_text = s.memory.copy_out(control+0x68,0x110);
                    require(std::equal(style_and_text.begin(),style_and_text.end(),original.begin()+(control-display)+0x68),
                            "Pitch styles or archived values changed");
                }
                if (key==1 && slot(10)==3) {
                    require(label(screen.control(first))=="32%" && label(screen.control(first+1))=="47%" &&
                            label(screen.control(first+2))=="21%" && label(screen.control(first+21))=="46%" &&
                            label(screen.control(first+22))=="54%", "Unexpected Cruzeiro archive percentages");
                }
                for (unsigned i=0; i<c.u16(display+0x129da4); ++i) {
                    screen.measure(screen.grid(i)); screen.publish(screen.grid(i));
                }
                const auto first_option = slot(35);
                require(!c.u32(screen.control(first_option)+8) && !c.u32(screen.control(first_option+3)+8),
                        "Archived regulation match enabled live or extra-time selection");
                const auto option = first_option+1+next;
                const auto popup = c.i16(screen.control(option)+0x72);
                require(popup>=0, "Missing period menu");
                const auto opener = c.i16(screen.grid(popup)+0x20e);
                require(opener>=0, "Missing period opener");
                require(click(opener)==-1 && click(option)==int(option), "Period menu missed its hit area");
                require(std::int32_t(call(0x1028fa50,{option}))==-11 && slot(10)==next, "Period selection failed");
            }
        }
        for (unsigned tab : {5,7}) {
            change_tab(tab);
            for (const auto [size, enabled] : {std::pair{GuiCanvas{800,600},true}, {{1280,900},false},
                                               {{801,601},true}, {{1280,900},true}, {{3200,1800},true}}) {
                select_new_screens(enabled);
                require(s.platform->resize_gui_canvas(s,size), "Resize archived team table");
                for (unsigned page : {1,2}) {
                    rebuild();
                    require(slot(2)==tab && slot(28)==page && slot(30)==key, "Unexpected archived team page");
                    const int header = c.i16(screen.control(slot(26))+0x72);
                    int players = 0;
                    while (players<c.u16(display+0x129da4) && screen.columns(screen.grid(players))!=21) ++players;
                    require(header>=0 && players<c.u16(display+0x129da4), "Missing team statistics grids");
                    const auto original = s.memory.copy_out(display,0x129da8);
                    const auto stats = s.memory.copy_out(slot(32),0xdfc), nav = s.memory.copy_out(history,0x1f4);
                    const auto regs = s.registers;
                    const auto rng = random_state(s);
                    layout_built_screen(s,display,0x1028f280);
                    require(regs==s.registers && rng==random_state(s) && stats==s.memory.copy_out(slot(32),stats.size()) &&
                            nav==s.memory.copy_out(history,nav.size()), "Team layout changed match data or navigation");
                    if (!enabled || size.original()) {
                        require(original==s.memory.copy_out(display,original.size()), "Classic team table changed");
                        break;
                    }
                    const auto cell = [&](int grid,int col,int row) {
                        for (unsigned i=0; i<c.u16(display+0x129da6); ++i) {
                            const auto at = screen.control(i);
                            if (c.i16(at+0x72)==grid && c.i32(at+0x20)==col && c.i32(at+0x24)==row) return i;
                        }
                        throw std::runtime_error("Missing team statistics cell");
                    };
                    const auto box = [&](int grid,int col,int row) {
                        return screen.rect(screen.control(cell(grid,col,row))+0x10);
                    };
                    for (unsigned i=0; i<c.u16(display+0x129da6); ++i) {
                        const auto at = screen.control(i);
                        if (c.i16(at+0x72)!=header && c.i16(at+0x72)!=players) continue;
                        for (const auto [offset,length] : {std::pair{8u,4u},{0x20u,8u},{0x48u,4u},{0x68u,0x110u}}) {
                            const auto data = s.memory.copy_out(at+offset,length);
                            require(std::equal(data.begin(),data.end(),original.begin()+(at-display)+offset),
                                    "Team layout changed a statistic, label, player link or row");
                        }
                    }
                    require(box(header,4,0).bottom<box(players,4,1).top, "Header overlaps the first player");
                    if (size.width>=1280)
                        require(box(players,4,1).width()>240 && box(players,0,1).width()<42, "Player name or shirt width");
                    for (int col=4; col<=16; ++col) {
                        const auto heading = box(header,col,0), stat = box(players,col+2,1);
                        require(heading.left==stat.left && heading.right==stat.right, "Statistic outside its heading");
                    }
                    if (key==1 && tab==7) {
                        const auto name = cell(players,4,9);
                        require(label(screen.control(name))=="Kanu" && c.u32(screen.control(name)+8)==2007 &&
                                click(name)==int(name), "Kanu link missed its row");
                        require(label(screen.control(cell(players,19,9)))=="10" &&
                                label(screen.control(cell(players,20,9)))=="4", "Unexpected rating or goals");
                        if (page==1)
                            require(label(screen.control(cell(players,6,9)))=="38" &&
                                    label(screen.control(cell(players,7,9)))=="25", "Unexpected Kanu passing totals");
                    }
                    const auto arrow = slot(26)+(page==1 ? 1 : 0);
                    require(click(arrow)==int(arrow) && std::int32_t(call(0x1028fa50,{arrow}))==-11 &&
                            slot(28)==3-page, "Team statistics page control failed");
                }
            }
        }
        change_tab(2);
        require(slot(2)==2 && slot(30)==key, "Leaving commentary changed match");
    }
    initialize_new_screens();
}
