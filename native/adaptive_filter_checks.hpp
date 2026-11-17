// Included in adaptive_gui_checks.cpp's anonymous namespace. Work only in a
// disposable captured world: no live game, persistent search or career save.
void filter_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    for (unsigned preset : {0u,1u,2u,3u,8u}) {
        for (bool commit : {false,true}) {
            std::vector<std::uint8_t> original_outcome;
            for (bool adaptive : {false,true}) {
                select_new_screens(adaptive);
                Clock clock; clock.idle_input = true;
                auto state = std::make_unique<State>(); auto& s = *state;
                load_match_snapshot(s, path); s.code_fragment_observer = &clock;
                guest::Cpu c(s); c.set(2, c.u32(c.sp()+0x14));
                const auto manager = c.toc(-0x7bb0); c.put16(manager+0x2c58,0);
                const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
                    unsigned reg = 3; for (auto arg : args) c.set(reg++,arg);
                    s.budget = s.executed + 400000000;
                    c.call(entry, return_sentinel); return c.r(3);
                };
                s.platform = std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
                const auto scratch = allocate(s,32,true); world(s,scratch,scratch+16);
                require(s.platform->resize_gui_canvas(s,{801,601}) && s.platform->resize_gui_canvas(s,{800,600}),
                        "Filter fixture pixel initialization");
                invoke(0x1031ad90,{preset});
                const auto parent_history = c.u32(manager+0x10), display = c.u32(manager+0x24);
                const auto engine = c.u32(parent_history+0x14), config = engine+4+preset*0x2b0;
                const auto search_manager = c.u32(c.toc(-0x7b1c));
                const auto human = c.u32(config)-c.u32(c.toc(-0x7be4))+16;
                require(human < 16, "Filter fixture requires a human search owner");
                const auto saved_at = c.u32(search_manager)+human*10*0x2b0;
                const auto saved = s.memory.copy_out(saved_at,10*0x2b0);
                invoke(0x10329ea0,{engine});
                const auto history = c.u32(manager+0x10), draft = c.u32(history+0x1c);
                require(c.u32(c.u32(history)) == 0x10329fd0 && c.u32(c.u32(history+8)) == 0x103308c0 && draft,
                        "Original filter opener/handler contract changed");
                require(adaptive_screen_available(s,0x10329fd0) == adaptive, "Filter ignored New Screens");
                if (adaptive) {
                    c.put16(manager+0x2c58,1);
                    require(!adaptive_screen_available(s,0x10329fd0), "Secondary filter group passed the gate");
                    c.put16(manager+0x2c58,0);
                    const auto network = c.u32(manager+0x2c64); c.put32(manager+0x2c64,1);
                    require(!adaptive_screen_available(s,0x10329fd0), "Network filter passed the gate");
                    c.put32(manager+0x2c64,network);
                    c.put32(history+0x1c,0);
                    require(!adaptive_screen_available(s,0x10329fd0), "Absent filter draft passed the gate");
                    c.put32(history+0x1c,draft);
                    for (unsigned tab : {0u,3u,255u}) {
                        c.put32(history+0x24,tab);
                        require(!adaptive_screen_available(s,0x10329fd0), "Unknown filter tab passed the gate");
                    }
                    c.put32(history+0x24,1);
                }
                const auto grid = [display](unsigned i) { return display+4+i*0xbf0; };
                const auto control = [display](unsigned i) { return display+0xba864+i*0x17c; };
                const auto rect = [&](unsigned at) { return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)}; };
                const auto meanings = [&] {
                    std::vector<std::uint8_t> result;
                    for (unsigned i=0;i<c.u16(display+0x129da6);++i) {
                        const auto at = control(i);
                        // Actions/kind, column/row, style/alignment/payload,
                        // indices/font/colors/popup and the localized text.
                        for (const auto [offset,size] : {std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                            const auto bytes = s.memory.copy_out(at+offset,size);
                            result.insert(result.end(),bytes.begin(),bytes.end());
                        }
                    }
                    return result;
                };
                const auto find = [&](unsigned action,unsigned payload=0xffffffffu) {
                    for (unsigned i=0;i<c.u16(display+0x129da6);++i)
                        if(c.u32(control(i)+8)==action && (payload==0xffffffffu || c.u32(control(i)+0x48)==payload)) return i;
                    throw std::runtime_error("Filter missing action "+std::to_string(action)+"/"+std::to_string(payload));
                };
                const auto hit = [&](unsigned i) {
                    const auto box = rect(control(i)+0x10);
                    const auto x=c.u32(c.toc(-0x5c10)),y=c.u32(c.toc(-0x5c14));
                    const auto primary=c.u8(c.toc(-0x7bac)),secondary=c.u8(c.toc(-0x5c0c));
                    c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
                    c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
                    const auto result=std::int16_t(invoke(0x1071a280,{display}));
                    c.put32(c.toc(-0x5c10),x); c.put32(c.toc(-0x5c14),y);
                    c.put8(c.toc(-0x7bac),primary); c.put8(c.toc(-0x5c0c),secondary);
                    return result;
                };
                const auto build = [&](GuiCanvas canvas) {
                    require(s.platform->resize_gui_canvas(s,canvas), "Filter resize failed");
                    invoke(0x10719560,{display,1,0}); invoke(0x10329fd0,{});
                    for (unsigned i=0;i<c.u16(display+0x129da4);++i) {
                        invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
                    }
                    const auto before=meanings(),query=s.memory.copy_out(engine,0x35fc),copy=s.memory.copy_out(draft,0x9c);
                    const auto rng=random_state(s);
                    const auto registers=s.registers;
                    const auto unique=s.unique;
                    const auto pc=s.pc,stack_base=c.sp()-0x8000;
                    const auto stack=s.memory.copy_out(stack_base,0x8100);
                    const auto old_display=s.memory.copy_out(display,0x129da8);
                    layout_built_screen(s,display,0x10329fd0);
                    require(meanings()==before, "Filter layout changed text, style, action or popup ownership");
                    require(s.memory.copy_out(engine,0x35fc)==query && s.memory.copy_out(draft,0x9c)==copy && random_state(s)==rng,
                            "Filter layout changed the query, editable copy or RNG");
                    require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                            "Filter layout clobbered the original ABI/stack");
                    if (!adaptive || canvas.original())
                        require(s.memory.copy_out(display,0x129da8)==old_display,"Compatibility filter changed geometry or state");
                    else {
                        const auto panel=rect(grid(1)),body=rect(grid(4)),footer=rect(grid(3));
                        require(panel.left>=107 && panel.right<canvas.width && panel.top>=50 && panel.bottom<canvas.height &&
                                panel.width()<=1320 && body.bottom<footer.top && footer.bottom<panel.bottom,
                                "Yoga form escaped the canvas or overlapped its footer");
                        if(canvas.width>=1280)require(body.width()>624,"Wide filter did not gain field space");
                        for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                            const int owner=c.i16(control(i)+0x72);
                            if(owner<4 || owner>10)continue;
                            const auto box=rect(control(i)+0x10);
                            require(box.left>=body.left && box.right<=body.right && box.top>=body.top && box.bottom<=body.bottom,
                                    "Filter field drawing/input escaped its body");
                        }
                    }
                    for(unsigned action:{8u,9u})require(hit(find(action))==int(find(action)),"Filter footer hit test missed");
                };
                if(adaptive && !commit) {
                    for(GuiCanvas canvas: {GuiCanvas{800,600},{801,601},{1280,900},{800,900},{1440,600},{3200,1800},{800,600}})
                        for(unsigned tab:{1u,2u}) {
                            c.put32(history+0x24,tab); build(canvas);
                            const auto index=find(tab==1?10:5,tab==1?(preset==2?13:8):1);
                            require(hit(index)==int(index),"Filter cell hit test missed after resize");
                        }
                }
                c.put32(history+0x24,1); build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                const auto parent=s.memory.copy_out(config,0x29c);
                const auto action = [&](unsigned category,unsigned payload=0xffffffffu) {
                    const auto i=find(category,payload);
                    require(hit(i)==int(i),"Filter action misses its displayed control");
                    return std::int32_t(invoke(0x103308c0,{i}));
                };
                require(action(10,preset==2?13:8)==-11,"Original age filter action failed");
                build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                require(s.memory.copy_out(config,0x29c)==parent,"Editing a filter changed its parent before OK");
                // Original tab switching and attribute increments edit the same
                // transient record; no replacement handler implements them.
                require(action(1,2)==-11 && c.u32(history+0x24)==2,"Attribute tab transition failed");
                build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                require(action(5,1)==-11,"Attribute increment failed");
                build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                require(action(1,1)==-11 && c.u32(history+0x24)==1,"General tab transition failed");
                build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                const auto edited=s.memory.copy_out(draft,0x9c);
                require(action(2)==-11,"Repositioned Clear button failed");
                build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                require(s.memory.copy_out(draft,0x9c)!=edited && s.memory.copy_out(config,0x29c)==parent,
                        "Clear failed to reset the draft independently of its parent");
                require(action(10,preset==2?13:8)==-11,"Age filter after Clear failed");
                build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                // Original conditional widgets: nationality/base menus, value
                // range or staff job, and (outside Scout mode) editable text.
                for(unsigned payload:{6u,25u,preset==2?44u:27u,54u}) {
                    if(payload==54 && preset>=3)continue;
                    require(action(10,payload)==-11,"Conditional filter activation failed");
                    build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                }
                bool opened=false;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                    const auto at=control(i); const int owner=c.i16(at+0x72),popup=c.i16(at+0x74);
                    if(owner<4 || owner>10 || popup<0)continue;
                    require(hit(i)==-1 && c.u8(display)==1 && c.i16(display+0x12a930)==popup,
                            "Filter popup did not follow its repositioned trigger");
                    const auto menu=rect(grid(popup));
                    require(menu.left>=0 && menu.top>=0 && menu.right<gui_canvas(s).width && menu.bottom<gui_canvas(s).height,
                            "Filter popup escaped the canvas");
                    opened=true; break;
                }
                require(opened,"Other nationality did not emit its original menu");
                build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600});
                require(action(commit?9:8)==-9,"Original Apply/Cancel transition failed");
                const auto outcome=s.memory.copy_out(config,0x29c);
                require(commit ? outcome!=parent : outcome==parent,"Filter transaction did not apply/discard the editable copy");
                if(!adaptive)original_outcome=outcome;
                else require(outcome==original_outcome,"Yoga changed the original Apply/Cancel result");
                require(s.memory.copy_out(saved_at,saved.size())==saved,"Filter fixture modified persistent recruitment settings");
            }
        }
        std::cout<<"filter preset "<<preset<<": both tabs, seven sizes, input/popups, ABI/query/RNG, original Apply/Cancel equivalence"<<std::endl;
    }
    initialize_new_screens();
}
