// Controlled proposals in an existing disposable capture. Compare the
// original commands and transaction outcomes with and without the layout.
void proposal_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist");
    for (unsigned scenario=0;scenario<7;++scenario) {
        std::vector<std::uint8_t> original;
        for (bool adaptive:{false,true}) {
            select_new_screens(adaptive);
            Clock clock; clock.idle_input=true;
            auto state=std::make_unique<State>(); auto& s=*state;
            load_match_snapshot(s,path); s.code_fragment_observer=&clock;
            guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
            const auto manager=c.toc(-0x7bb0); c.put16(manager+0x2c58,0);
            const auto invoke=[&](unsigned entry,std::initializer_list<unsigned> args) {
                unsigned reg=3; for(auto arg:args)c.set(reg++,arg);
                s.budget=s.executed+400000000; c.call(entry,return_sentinel); return c.r(3);
            };
            s.platform=std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
            const auto scratch=allocate(s,32,true); world(s,scratch,scratch+16);
            require(s.platform->resize_gui_canvas(s,{801,601}),"Proposal pixel initialization");
            const auto managed=c.u32(c.u32(manager+4)+0x24),clubs=c.u32(c.toc(-0x7be8));
            unsigned target=0,contract=0;
            for(unsigned i=0;i<c.u32(c.toc(-0x7b50))&&!target;++i) {
                const auto employer=clubs+i*0x248;
                if(employer==managed || c.u32(employer+0x30)!=c.u32(managed+0x30))continue;
                for(unsigned j=0;j<50&&!target;++j) {
                    const auto candidate=c.u32(employer+0x98+j*4);
                    if(!candidate || !c.u32(candidate+0x14) || c.u32(candidate+0x24)!=employer)continue;
                    contract=invoke(0x10082f20,{c.u32(c.toc(-0x7b7c)),candidate});
                    if(contract && c.u32(contract+4)==c.u32(employer) && (c.i8(contract+0x38)>>2)!=5)target=candidate;
                }
            }
            require(target,"Proposal requires a contracted AI player");
            invoke(0x10417590,{target,0});
            const auto display=c.u32(manager+0x24),history=c.u32(manager+0x10);
            require(c.u32(c.u32(history))==0x10417b60 && c.u32(c.u32(history+8))==0x1041b600,
                    "Original proposal opener/handler changed");
            require(adaptive_screen_available(s,0x10417b60)==adaptive,"Proposal ignored New Screens");
            if(adaptive) {
                c.put16(manager+0x2c58,1);
                require(!adaptive_screen_available(s,0x10417b60),"Secondary proposal passed gate");
                c.put16(manager+0x2c58,0); c.put32(manager+0x2c64,1);
                require(!adaptive_screen_available(s,0x10417b60),"Network proposal passed gate");
                c.put32(manager+0x2c64,0);
            }
            const auto draft_slot=invoke(0x10148930,{c.u32(c.toc(-0x7a54)),0});
            const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
            const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
            const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
            const auto meanings=[&] {
                std::vector<std::uint8_t> bytes;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    for(const auto [offset,size]:{std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                        const auto part=s.memory.copy_out(control(i)+offset,size);
                        bytes.insert(bytes.end(),part.begin(),part.end());
                    }
                return bytes;
            };
            const auto records=[&] {
                std::vector<std::uint8_t> bytes;
                for(const auto [at,size]:{std::pair{target,0x70u},{contract,0x40u},{managed,0x248u},
                        {c.u32(target+0x24),0x248u},{c.u32(draft_slot),0x54u},{history,0xd0u}}) {
                    const auto part=s.memory.copy_out(at,size); bytes.insert(bytes.end(),part.begin(),part.end());
                }
                const auto list=c.u32(c.u32(draft_slot)+4);
                if(list) {
                    const auto part=s.memory.copy_out(list,16); bytes.insert(bytes.end(),part.begin(),part.end());
                }
                const auto rng=random_state(s); bytes.insert(bytes.end(),rng.begin(),rng.end());
                return bytes;
            };
            const auto find=[&](unsigned action) {
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.u32(control(i)+8)==action)return i;
                throw std::runtime_error("Proposal missing action "+std::to_string(action));
            };
            const auto hit=[&](unsigned i) {
                const auto box=rect(control(i)+0x10);
                c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
                c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
                return std::int16_t(invoke(0x1071a280,{display}));
            };
            const auto build=[&](GuiCanvas canvas) {
                require(s.platform->resize_gui_canvas(s,canvas),"Proposal resize failed");
                invoke(0x10719560,{display,1,0}); invoke(0x10417b60,{});
                for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                    invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
                }
                const auto before=meanings(),saved=records(),old_display=s.memory.copy_out(display,0x129da8);
                const auto registers=s.registers;
                const auto unique=s.unique;
                const auto pc=s.pc,stack_base=c.sp()-0x8000;
                const auto stack=s.memory.copy_out(stack_base,0x8100);
                layout_built_screen(s,display,0x10417b60);
                require(meanings()==before,"Proposal changed localized text, actions, fonts, colors or popup ownership");
                require(records()==saved,"Proposal layout changed draft, records, history or RNG");
                require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                        "Proposal layout clobbered guest ABI/stack");
                if(!adaptive || canvas.original())require(s.memory.copy_out(display,0x129da8)==old_display,"Legacy proposal changed");
                else {
                    // Explicit bounds must survive the original incremental
                    // redraw path, not just the post-builder layout pass.
                    const auto fee=rect(control(c.u32(history+0x14+15*8)+2)+0x10);
                    for(unsigned i:{unsigned(c.i16(control(c.u32(history+0x14+15*8))+0x72)),
                                    unsigned(c.i16(control(c.u32(history+0x14+20*8))+0x72))}) {
                        invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
                    }
                    require(rect(control(c.u32(history+0x14+15*8)+2)+0x10)==fee,"Proposal redraw lost value bounds");
                    const auto footer=rect(grid(c.i16(control(find(33))+0x72)));
                    require(footer.left>=100 && footer.right<canvas.width && footer.top==canvas.height-45 && footer.bottom<canvas.height,
                            "Proposal footer escaped canvas/navigation");
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                        const auto at=control(i); const int owner=c.i16(at+0x72);
                        if(owner!=2 && owner!=c.i16(control(c.u32(history+0x14+20*8))+0x72))continue;
                        const auto box=rect(at+0x10),panel=rect(grid(owner));
                        require(box.left>=panel.left && box.right<=panel.right && box.top>=panel.top && box.bottom<=panel.bottom,
                                "Proposal terms escaped section bounds");
                    }
                }
                require(hit(find(33))==int(find(33)),"Proposal Back button hit test failed");
            };
            const GuiCanvas canvas=adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600};
            build(canvas);
            const auto draft=c.u32(draft_slot);
            if(scenario>=2 && scenario<=5) {
                // Existing outgoing, pending and incoming footer states. The
                // original offer remains separate from its editable draft.
                const auto active=allocate(s,0x54,true),link=allocate(s,4,true);
                s.memory.copy_in(active,s.memory.copy_out(draft,0x54)); c.put32(link,active);
                c.put32(history+0x1c,link); c.put32(history+0x14+3*8,c.u32(active+0x2c));
                c.put32(history+0x14+6*8,scenario<4?1:0); c.put32(history+0x14+7*8,scenario==3?1:0);
                if(scenario==5)c.put8(draft+0x36,3);
                build(canvas);
            }
            if(scenario==6) {
                const auto list=invoke(0x10773520,{16}); invoke(0x10738c00,{list});
                unsigned count=0;
                for(unsigned i=0;i<50 && count<5;++i) {
                    const auto player=c.u32(managed+0x98+i*4);
                    if(!player || !c.u32(player+0x14))continue;
                    c.put32(scratch,c.u32(player)); invoke(0x10738d10,{list,scratch,4}); ++count;
                }
                require(count==5,"Proposal exchange fixture requires five players");
                c.put32(draft+4,list); build(canvas);
            }
            if(adaptive)for(GuiCanvas size:{GuiCanvas{800,600},{801,601},{800,900},{1440,600},{3200,1800},canvas})build(size);
            std::vector<std::uint8_t> outcome;
            const auto checkpoint=[&] {
                const auto part=s.memory.copy_out(draft+8,0x4c); outcome.insert(outcome.end(),part.begin(),part.end());
                outcome.push_back(c.u32(draft+4)?c.u16(c.u32(draft+4)):0);
            };
            const auto action=[&](unsigned command) {
                const auto i=find(command); require(hit(i)==int(i),"Proposal action missed its displayed control");
                const auto result=std::int32_t(invoke(0x1041b600,{i})); checkpoint(); return result;
            };
            if(scenario<2) {
                for(unsigned command:{3u,4u,7u,8u,9u,10u,11u,12u,13u,15u,14u,18u,20u,19u,34u}) {
                    const auto status=action(command);
                    require(status==0 || status==-11,"Proposal edit left original screen");
                    if(status==-11)build(canvas);
                }
                if(adaptive)for(GuiCanvas size:{GuiCanvas{801,601},{800,900},canvas}) {
                    build(size);
                    require(hit(find(14))==int(find(14)) && hit(find(19))==int(find(19)),
                            "Conditional appearance links lost their text alignment");
                }
                // Open the original amount menu at its moved trigger, then
                // dispatch one of its existing choices through the handler.
                const auto fee=c.u32(history+0x14+15*8)+2;
                const int popup=c.i16(control(fee)+0x74);
                require(popup>=0 && hit(fee)==-1 && c.u8(display)==1 && c.i16(display+0x12a930)==popup,
                        "Proposal fee menu lost its anchor");
                const auto menu=rect(grid(popup));
                require(menu.left>=0 && menu.right<canvas.width && menu.top>=0 && menu.bottom<canvas.height,
                        "Proposal menu escaped canvas");
                require(std::int32_t(invoke(0x1041b600,{find(6)}))==-11,"Proposal amount choice failed");
                checkpoint(); build(canvas);
            }
            if(scenario==6) {
                require(action(30)==-11,"Exchange removal mode failed"); build(canvas);
                require(action(31)==-11 && c.u16(c.u32(draft+4))==4,"Exchange removal changed the wrong list");
                build(canvas);
            }
            if(!adaptive)original=outcome; else require(outcome==original,"Adaptive proposal changed original command results");
            if(scenario==1) {
                require(action(1)==-4,"Offer did not open original confirmation");
                const auto next=c.u32(manager+0x10);
                require(c.u32(c.u32(next))==0x1041eb10 && c.u32(next+0x14)==draft_slot,
                        "Offer confirmation lost the draft");
            } else {
                const auto i=find(33);
                require(std::int32_t(invoke(0x1041b600,{i}))==-9 && !c.u32(draft_slot),"Back did not discard proposal draft");
            }
        }
        std::cout<<"proposal state "<<scenario<<": layout gate, ABI/records/RNG, sizes, original controls and transaction outcome"<<std::endl;
    }
    initialize_new_screens();
}
