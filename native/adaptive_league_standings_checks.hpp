// Existing Brazilian leagues exercise short tables, retained overflow and an
// overflow-to-full-table transition. All navigation stays in disposable memory.
void league_standings_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist"); select_new_screens(true);
    for (unsigned id : {262u, 65u, 79u}) {
        Clock clock; clock.idle_input = true;
        auto state = std::make_unique<State>(); auto& s = *state;
        load_match_snapshot(s, path); s.code_fragment_observer = &clock;
        guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
        const auto manager = c.toc(-0x7bb0);
        c.put16(manager + 0x2c58, 0);
        const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
            unsigned reg = 3; for (auto arg : args) c.set(reg++, arg);
            s.budget = s.executed + 400000000;
            try { c.call(entry, return_sentinel); }
            catch (const std::exception& error) {
                throw std::runtime_error("Standings " + std::to_string(id) + " entry=" + hex_address(entry) +
                    " pc=" + hex_address(s.pc) + ": " + error.what());
            }
            return c.r(3);
        };
        s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
        const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
        require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}),
                "Standings pixel initialization");
        const auto definition = c.u32(c.toc(-0x7bbc)) + id * 0x6c;
        invoke(0x100648e0, {definition, 0xfffffffeu, 0xffffffffu, 0});
        const auto history = c.u32(manager + 0x10), display = c.u32(manager + 0x24);
        const auto setting = [history](unsigned slot) { return history + 0x14 + slot * 8; };
        const auto competition = c.u32(setting(0));
        require(c.u32(c.u32(history)) == 0x10065340 && competition && c.u32(setting(1)) == 1,
                "Original competition opener did not establish table history");
        require(adaptive_screen_available(s, 0x10065340), "Initial current-stage standings gate failed");
        const auto grid = [display](unsigned i) { return display + 4 + i * 0xbf0; };
        const auto control = [display](unsigned i) { return display + 0xba864 + i * 0x17c; };
        const auto rect = [&](unsigned at) { return ScreenRect{c.i32(at), c.i32(at+4), c.i32(at+8), c.i32(at+12)}; };
        const auto refresh = [&] {
            for (unsigned i=0; i<c.u16(display+0x129da4); ++i) {
                invoke(0x10714630, {grid(i)}); invoke(0x10714110, {grid(i),0});
            }
        };
        const auto hit = [&](unsigned index) {
            const auto box=rect(control(index)+0x10);
            c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
            c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
            return std::int16_t(invoke(c.u8(display)==1 ? 0x1071aa10 : 0x1071a280, {display}));
        };
        const auto meanings = [&] {
            std::vector<std::uint8_t> bytes;
            for (unsigned i=0; i<c.u16(display+0x129da6); ++i)
                for (const auto [offset,size] : {std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                    const auto part=s.memory.copy_out(control(i)+offset,size);
                    bytes.insert(bytes.end(),part.begin(),part.end());
                }
            return bytes;
        };
        const auto records = [&] {
            std::vector<std::pair<unsigned,std::vector<std::uint8_t>>> saved;
            const auto append = [&](unsigned at,unsigned size) {
                if(at && size) saved.emplace_back(at,s.memory.copy_out(at,size));
            };
            append(history,0x1f4); append(definition,0x6c); append(competition,0xec);
            append(c.toc(-0x7b78),4); append(c.toc(-0x7b80),8);
            append(c.r(2)-0x3750,4); append(c.r(2)-0x3754,4);
            const auto count=c.i16(competition+0x3e), days=c.i16(competition+0xa4);
            const auto table=c.u32(competition+0xb0);
            require(count>0 && count<=64 && days>=0,"Standings fixture record sizes");
            append(table,unsigned(count)*0x44);
            append(c.u32(competition+0xbc),unsigned(c.u16(competition+0xa6))*0x4c);
            for(int i=0;i<count;++i) {
                const auto row=table+unsigned(i)*0x44;
                append(c.u32(row),0x248);
                append(c.u32(row+8),unsigned(count-1)*0x10);
                append(c.u32(row+12),unsigned(days));
            }
            return saved;
        };
        const auto unchanged = [&](const auto& saved) {
            for (const auto& [at,bytes] : saved)
                require(s.memory.copy_out(at,bytes.size())==bytes,"Standings layout changed competition/club/history/date records");
        };
        int body=-1;
        const auto build = [&](GuiCanvas size,bool adaptive,unsigned scroll) {
            select_new_screens(adaptive);
            require(s.platform->resize_gui_canvas(s,size),"Standings resize failed");
            c.put32(setting(6),scroll);
            invoke(0x10719560,{display,1,0}); invoke(0x10065340,{});
            require(c.i32(setting(2))==-1,"Original builder did not resolve current stage");
            body=-1;
            for(unsigned i=0;i<c.u16(display+0x129da4);++i)
                if(c.u8(grid(i)+0xbb0)==11 && c.u32(grid(i)+0x18)==2) body=int(i);
            require(body>=0 && c.u32(setting(7))==unsigned(body),"Original standings scroll-grid ownership changed");
            const auto area=grid(body); const auto before=rect(area);
            const auto old_rows=c.u8(area+0xbb1);
            const auto last_row=c.i32(area+0x14), old_scroll=c.i32(area+0x1fc);
            require(before==ScreenRect{110,190,780,500},"Unexpected captured standings geometry");
            // Production applies layout before the first refresh; preserve that
            // boundary so a removed scrollbar cannot inherit a stale draw flag.
            const auto original=s.memory.copy_out(display,0x129da8); const auto saved=records();
            const auto rng=random_state(s), semantic=meanings();
            const auto registers=s.registers; const auto unique=s.unique;
            const auto pc=s.pc, stack_base=c.sp()-0x8000;
            const auto stack=s.memory.copy_out(stack_base,0x8100);
            layout_built_screen(s,display,0x10065340);
            unchanged(saved); require(random_state(s)==rng,"Standings layout changed RNG");
            require(s.registers==registers && s.unique==unique && s.pc==pc &&
                    s.memory.copy_out(stack_base,0x8100)==stack,"Standings layout changed guest ABI/stack");
            require(meanings()==semantic,"Standings layout changed original text, style, action or payload");
            if(!adaptive || size.original()) {
                require(s.memory.copy_out(display,0x129da8)==original,"Classic standings changed");
                refresh(); return;
            }
            const auto panel=rect(area); const int rows=c.u8(area+0xbb1);
            require(panel.left>=110 && panel.right<=size.width-20 && panel.width()<=1600 &&
                    panel.top==190 && panel.bottom<=size.height-100 && panel.height()<=646,
                    "Standings panel exceeds available bounds");
            require(rows>=old_rows && rows<=30 && (size.height<900 || rows>old_rows),"Standings capacity failed to grow safely");
            for(int row=old_rows;row<rows;++row)require(c.u8(area+0xbd0+row)==1,"New standings row lacks a unit weight");
            const int expected_scroll=std::clamp(old_scroll,0,std::max(0,last_row+1-rows));
            require(c.i32(area+0x1fc)==expected_scroll,"Standings scroll was not clamped to expanded capacity");
            require(bool(c.u8(area+0xbef))==(last_row+1>rows),"Standings scrollbar does not match row capacity");
            refresh();
            unsigned clubs=0,positions=0,separators=0;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i); const auto box=rect(at+0x10);
                if(c.i16(at+0x72)!=body)continue;
                if(c.i32(at+0x28)) {
                    ++separators;
                    require(c.i32(at+0x28)==panel.left+2 && c.i32(at+0x2c)==panel.right-2-(last_row+1>rows?22:0),
                            "Standings separator lost its full-row bounds");
                }
                if(box.bottom<=panel.top || box.top>=panel.bottom)continue;
                require(box.left>=panel.left && box.right<=panel.right && box.top>=panel.top && box.bottom<=panel.bottom &&
                        box.right>=box.left && box.bottom>=box.top,"Standings control outside its panel");
                const auto action=c.u32(at+8);
                if(action==2005 || action==7) {
                    require(hit(i)==int(i),"Expanded standings link missed its input bounds");
                    if(action==2005)++clubs; else ++positions;
                }
            }
            require(clubs && positions && separators,"Standings fixture did not exercise clubs, positions and separators");
            unsigned top_tabs=0, lower_tabs=0, print=0, view_menu=0;
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                const auto at=grid(i); const auto box=rect(at);
                if(box.top==80 && box.left==100) {
                    ++top_tabs; require(box.right==size.width-10,"Standings top tabs failed to expand");
                }
                if(box.top==size.height-90 && box.left==100) {
                    ++lower_tabs; require(box.right==size.width-10,"Standings lower tabs failed to expand");
                }
            }
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i); const auto box=rect(at+0x10);
                if(c.i16(at+0x72)>=0)continue;
                if(box.top==15 && box.width()==75) {
                    ++print; require(box.right==size.width-15,"Standings Print failed to follow the window");
                }
                if(box.top==125 && box.width()==125 && c.i16(at+0x74)>=0) {
                    ++view_menu; require(box.left==panel.left,"Standings View failed to follow its panel");
                }
            }
            require(top_tabs==2 && lower_tabs==2 && print==1 && view_menu==1,"Standings shell controls were not verified");
        };
        // All exclusions are pure reads; unresolved stages must not call the
        // current-stage virtual predicate, which can finalize league records.
        const auto gate_registers=s.registers; const auto gate_unique=s.unique; const auto gate_pc=s.pc;
        const auto gate_record=s.memory.copy_out(competition,0xec), gate_rng=random_state(s);
        require(adaptive_screen_available(s,0x10065340),"Unresolved simple league is unavailable");
        require(s.registers==gate_registers && s.unique==gate_unique && s.pc==gate_pc &&
                s.memory.copy_out(competition,0xec)==gate_record && random_state(s)==gate_rng,"Standings gate ran guest code");
        for(unsigned section:{0u,2u,3u,4u,7u,8u}) {
            c.put32(setting(1),section); require(!adaptive_screen_available(s,0x10065340),"Non-table competition tab adapted");
        }
        c.put32(setting(1),1);
        c.put32(setting(4),3); require(!adaptive_screen_available(s,0x10065340),"Unknown standings view adapted"); c.put32(setting(4),0);
        c.put16(manager+0x2c58,1); require(!adaptive_screen_available(s,0x10065340),"Secondary standings adapted"); c.put16(manager+0x2c58,0);
        const auto network=c.u32(manager+0x2c64);
        c.put32(manager+0x2c64,1); require(!adaptive_screen_available(s,0x10065340),"Network standings adapted"); c.put32(manager+0x2c64,network);
        c.put8(competition+0x43,1); require(!adaptive_screen_available(s,0x10065340),"Cup stage adapted as a league"); c.put8(competition+0x43,2);
        const auto clubs=c.u16(competition+0x3e);
        c.put16(competition+0x3e,8); require(!adaptive_screen_available(s,0x10065340),"Compact group adapted as a full league"); c.put16(competition+0x3e,clubs);
        // Controlled topology is only read by eligibility, never dispatched as
        // a competition. It covers the mixed-stage, selected-stage distinction.
        const auto child=allocate(s,0xec,true), children=allocate(s,4,true);
        s.memory.copy_in(child,s.memory.copy_out(competition,0xec)); c.put32(children,child);
        const auto old_children=c.u32(competition+0xc), old_last=c.u32(competition+0x30);
        c.put32(competition+0xc,children); c.put32(competition+0x30,0);
        require(adaptive_screen_available(s,0x10065340),"Unresolved all-league stages rejected");
        c.put8(child+0x43,1);
        require(!adaptive_screen_available(s,0x10065340),"Unresolved mixed-stage competition adapted");
        c.put32(setting(2),0); require(!adaptive_screen_available(s,0x10065340),"Selected cup child adapted");
        c.put8(child+0x43,2); require(adaptive_screen_available(s,0x10065340),"Selected league child rejected");
        c.put8(competition+0x43,1); require(adaptive_screen_available(s,0x10065340),"League child under cup parent rejected");
        c.put8(competition+0x43,2);
        for(auto index:{0xfffffffdu,1u}) {
            c.put32(setting(2),index); require(!adaptive_screen_available(s,0x10065340),"Invalid stage index adapted");
        }
        c.put32(competition+0xc,old_children); c.put32(competition+0x30,old_last); c.put32(setting(2),0xfffffffeu);
        for(unsigned view:{0u,1u,2u}) {
            c.put32(setting(4),view);
            for(auto size:{GuiCanvas{800,600},GuiCanvas{801,601},GuiCanvas{1280,900},GuiCanvas{1600,1000},GuiCanvas{3200,1800},GuiCanvas{800,900}})
                build(size,true,view==1?8u:0u);
            build({1280,900},false,0);
        }
        for(bool adaptive:{false,true}) {
            const auto size=adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600};
            build(size,adaptive,0);
            for(unsigned view:{1u,2u,0u}) {
                unsigned parent=0,choice=0;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                    const auto at=control(i);
                    if(c.i16(at+0x72)<0 && c.i32(at+0x14)==125 && c.i32(at+0x18)-c.i32(at+0x10)==125 && c.i16(at+0x74)>=0)parent=i;
                    if(c.u32(at+8)==6 && c.u32(at+0x48)==view)choice=i;
                }
                require(parent && choice,"Original standings View menu missing");
                const auto rng=random_state(s), record=s.memory.copy_out(competition,0xec);
                require(hit(parent)==-1 && c.u8(display)==1,"Standings View popup did not open");
                require(hit(choice)==int(choice),"Standings View option missed its input bounds");
                require(std::int32_t(invoke(0x1006d500,{choice}))==-11 && c.u32(setting(4))==view,
                        "Original standings View command changed");
                require(random_state(s)==rng && s.memory.copy_out(competition,0xec)==record,"Standings View command changed records/RNG");
                build(size,adaptive,0);
            }
        }
        if(id==262) {
            // No captured league currently exposes a Stage menu. Borrow one
            // GUIO solely to exercise its original action5 decoding/reset.
            build({1280,900},true,3);
            unsigned choice=0;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                if(c.u32(control(i)+8)==6)choice=i;
            require(choice && !c.u32(setting(5)),"Controlled stage command requires an empty fixture cache");
            const auto old_control=s.memory.copy_out(control(choice),0x17c);
            c.put32(control(choice)+8,5); c.put32(control(choice)+0x48,0);
            c.put32(setting(3),0xffffffffu); c.put32(setting(4),2);
            const auto rng=random_state(s), record=s.memory.copy_out(competition,0xec);
            require(std::int32_t(invoke(0x1006d500,{choice}))==-11 && c.i32(setting(2))==-1 &&
                    !c.u32(setting(3)) && !c.u32(setting(4)) && !c.u32(setting(6)),
                    "Original stage command failed its history/view/scroll reset");
            require(random_state(s)==rng && s.memory.copy_out(competition,0xec)==record,
                    "Stage command changed competition records/RNG");
            s.memory.copy_in(control(choice),old_control);
            // Existing roots are copied into unavailable/future UI states.
            // Only the original message builders run; no season is advanced.
            const auto active=c.u8(competition+0x45); const auto name=c.u16(competition+0x3a);
            const auto start=c.u16(competition+0xb6), year_offset=c.u16(competition+0xb8);
            for(unsigned scenario:{0u,1u,2u}) for(auto size:{GuiCanvas{800,600},GuiCanvas{1280,900},GuiCanvas{3200,1800}}) {
                c.put8(competition+0x45,scenario ? active : 0);
                c.put16(competition+0xb6,scenario ? 0 : 0xffff);
                c.put16(competition+0xb8,1);
                c.put16(competition+0x3a,scenario==2 ? 1 : 0);
                require(s.platform->resize_gui_canvas(s,size),"Standings message resize");
                invoke(0x10719560,{display,1,0}); invoke(0x10065340,{});
                int message=-1;
                for(unsigned i=0;i<c.u16(display+0x129da4);++i)
                    if(c.u32(grid(i)+0x18)==2 && c.u8(grid(i)+0xbb0)==1 && c.i32(grid(i))==110)message=int(i);
                require(message>=0,"Original standings message branch missing");
                const auto area=grid(message); const auto before=rect(area);
                require(before.top==(scenario==2 ? 165 : 125),"Standings message heading branch missing");
                const auto capacity=c.u8(area+0xbb1);
                const auto original=s.memory.copy_out(display,0x129da8), semantic=meanings(), message_rng=random_state(s);
                const auto saved=records(); const auto registers=s.registers; const auto unique=s.unique;
                const auto pc=s.pc, stack_base=c.sp()-0x8000; const auto stack=s.memory.copy_out(stack_base,0x8100);
                std::vector<std::array<int,3>> spans;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.i16(control(i)+0x72)==message && c.i32(control(i)+0x28))
                        spans.push_back({int(i),c.i32(control(i)+0x28),c.i32(control(i)+0x2c)});
                layout_built_screen(s,display,0x10065340);
                unchanged(saved);
                require(random_state(s)==message_rng && meanings()==semantic,"Standings message changed text, commands or RNG");
                require(s.registers==registers && s.unique==unique && s.pc==pc &&
                        s.memory.copy_out(stack_base,0x8100)==stack,"Standings message changed guest ABI/stack");
                if(size.original())require(s.memory.copy_out(display,0x129da8)==original,"Classic standings message changed");
                else {
                    const auto panel=rect(area);
                    require(panel.width()>before.width() && panel.bottom<=size.height-100 && c.u8(area+0xbb1)==capacity,
                            "Standings message panel/capacity failed to adapt");
                    require(!spans.empty(),"Standings message lacks measured text");
                    const int shift=(panel.left+panel.right-before.left-before.right)/2;
                    for(const auto span:spans)
                        require(c.i32(control(span[0])+0x28)==span[1]+shift && c.i32(control(span[0])+0x2c)==span[2]+shift,
                                "Standings message lost measured text/link widths");
                    require(rect(control(0)+0x10)==ScreenLayout{size}.title(),"Message title failed to expand");
                    unsigned top_tabs=0,lower_tabs=0,print=0,heading=0;
                    for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                        const auto box=rect(grid(i));
                        if(box==ScreenRect{100,80,size.width-10,115})++top_tabs;
                        if(box==ScreenRect{100,size.height-90,size.width-10,size.height-55})++lower_tabs;
                    }
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                        const auto at=control(i); if(c.i16(at+0x72)>=0)continue;
                        const auto box=rect(at+0x10);
                        if(box==ScreenRect{size.width-90,15,size.width-15,35})++print;
                        if(box==ScreenRect{panel.left,125,panel.right,160})++heading;
                    }
                    require(top_tabs==2 && lower_tabs==2 && print==1 && heading==(scenario==2?1u:0u),
                            "Standings message tabs, Print or optional heading did not adapt");
                }
            }
            c.put8(competition+0x45,active); c.put16(competition+0x3a,name);
            c.put16(competition+0xb6,start); c.put16(competition+0xb8,year_offset);
        }
    }
    initialize_new_screens();
    std::cout << "Standings views, original links/menus, scrolling, records/RNG, classic fallback and presentation ABI passed\n";
}
