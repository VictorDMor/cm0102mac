// Original builders and commands in disposable copies of an existing capture.
// Final submission is hit-tested, but only safe dismissal handlers are run.
void negotiation_dialog_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist");
    for(unsigned scenario=0;scenario<8;++scenario) {
        std::vector<std::uint8_t> baseline;
        for(bool adaptive:{false,true}) {
            try {
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
                require(s.platform->resize_gui_canvas(s,{801,601}),"Dialog pixel initialization");
                const auto managed=c.u32(c.u32(manager+4)+0x24),clubs=c.u32(c.toc(-0x7be8));
                unsigned person=0,contract=0;
                for(unsigned k=0;k<c.u32(c.toc(-0x7b50))&&!person;++k) {
                    const auto club=clubs+k*0x248;
                    if(club==managed || c.u32(club+0x30)!=c.u32(managed+0x30))continue;
                    for(unsigned j=0;j<50&&!person;++j) {
                        const auto candidate=c.u32(club+0x98+j*4);
                        if(!candidate || !c.u32(candidate+0x14) || c.u32(candidate+0x24)!=club)continue;
                        if(!invoke(0x103fa290,{c.u32(c.toc(-0x7b40)),managed,candidate,c.toc(-0x7bb8)}))continue;
                        contract=invoke(0x10082f20,{c.u32(c.toc(-0x7b7c)),candidate});
                        if(contract)person=candidate;
                    }
                }
                require(person,"Dialog fixture needs an eligible loan player");
                const auto display=c.u32(manager+0x24);
                const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
                const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
                const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
                const auto history=[&]{return c.u32(manager+0x10);};
                const auto value=[&](unsigned slot){return c.u32(history()+0x14+slot*8);};
                const auto set_value=[&](unsigned slot,unsigned v){c.put32(history()+0x14+slot*8,v);};
                const auto find=[&](unsigned action) {
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i)if(c.u32(control(i)+8)==action)return i;
                    throw std::runtime_error("Missing dialog action "+std::to_string(action));
                };
                unsigned offer=0,link=0,draft=0;
                const auto make_offer=[&](bool incoming) {
                    offer=allocate(s,0x54,true);
                    invoke(0x10411ea0,{offer,person,incoming?c.u32(person+0x24):managed,0,0xffffffffu,1});
                    c.put32(offer+0x28,0xffffffffu); c.put32(offer+0x2c,0);
                    link=allocate(s,4,true); c.put32(link,offer);
                };
                if(scenario<3) {
                    require(invoke(0x1042c2c0,{person}),"Original loan opener rejected fixture");
                    if(scenario) {
                        make_offer(false); set_value(7,offer); set_value(9,0);
                        for(unsigned i=0;i<5;++i) {
                            set_value(16+i,value(11+i)); set_value(21+i,value(11+i));
                        }
                        // A controlled proposed-terms marker enables the
                        // original second restore button and outgoing footer.
                        if(scenario==2) { set_value(10,contract); set_value(21,50); }
                    }
                } else if(scenario==3) {
                    invoke(0x100960d0,{person,1,11,1});
                    invoke(0x10719560,{display,1,0}); invoke(0x10096c60,{});
                    draft=invoke(0x101487d0,{c.u32(c.toc(-0x7a54)),0});
                    require(std::int32_t(invoke(0x10097b70,{find(1)}))==-4,"Contract did not open confirmation");
                } else if(scenario==4 || scenario==7) {
                    make_offer(false);
                    invoke(0x1041ea60,{scenario==7?0:link,scenario==7?27u:1u,scenario==7?link:0});
                } else if(scenario==5) {
                    make_offer(true);
                    invoke(0x1042ee90,{person,0,2003,50,1,1,offer});
                } else invoke(0x1042ed80,{person,2});
                const auto meanings=[&] {
                    std::vector<std::uint8_t> bytes;
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                        for(const auto [offset,size]:{std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                            const auto part=s.memory.copy_out(control(i)+offset,size); bytes.insert(bytes.end(),part.begin(),part.end());
                        }
                    return bytes;
                };
                const auto records=[&] {
                    std::vector<std::uint8_t> bytes;
                    const auto append=[&](unsigned at,unsigned count) {
                        if(at) { const auto part=s.memory.copy_out(at,count); bytes.insert(bytes.end(),part.begin(),part.end()); }
                    };
                    append(person,0x70); append(contract,0x64); append(managed,0x248); append(c.u32(person+0x24),0x248);
                    append(history(),0x110); append(offer,0x54); append(link,4); append(draft,0x64);
                    const auto rng=random_state(s); bytes.insert(bytes.end(),rng.begin(),rng.end()); return bytes;
                };
                const auto hit=[&](unsigned i) {
                    const auto box=rect(control(i)+0x10);
                    c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
                    c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
                    return std::int16_t(invoke(0x1071a280,{display}));
                };
                const GuiCanvas canvas=adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600};
                const auto build=[&](GuiCanvas size,bool overflow=false) {
                    const auto builder=c.u32(c.u32(history()));
                    require(adaptive_screen_available(s,builder)==adaptive,"Dialog gate disagrees with preference");
                    if(adaptive) {
                        c.put16(manager+0x2c58,1); require(!adaptive_screen_available(s,builder),"Secondary dialog passed gate");
                        c.put16(manager+0x2c58,0); c.put32(manager+0x2c64,1);
                        require(!adaptive_screen_available(s,builder),"Network dialog passed gate"); c.put32(manager+0x2c64,0);
                    }
                    require(s.platform->resize_gui_canvas(s,size),"Dialog resize failed");
                    invoke(0x10719560,{display,1,0}); invoke(builder,{});
                    int body=-1,footer=-1;
                    for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                        const auto box=rect(grid(i));
                        if(box==ScreenRect{110,140,780,545} || box==ScreenRect{110,165,780,545}) {
                            body=int(i);
                            if(overflow) {
                                c.put32(grid(i)+0x14,c.u8(grid(i)+0xbb1)+3); c.put32(grid(i)+0x1fc,2);
                                require(!c.u8(grid(i)+0xbef),"Overflow was already refreshed");
                                continue;
                            }
                        }
                        if(box==ScreenRect{100,555,790,590})footer=int(i);
                        invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
                    }
                    require(body>=0 && footer>=0,"Original dialog panels absent");
                    const auto semantics=meanings(),saved=records(),old=s.memory.copy_out(display,0x129da8);
                    const auto old_word=[&](unsigned offset) {
                        return std::int32_t((unsigned(old[offset])<<24)|(unsigned(old[offset+1])<<16)|
                                            (unsigned(old[offset+2])<<8)|old[offset+3]);
                    };
                    const auto registers=s.registers;
                    const auto unique=s.unique;
                    const auto pc=s.pc,stack_base=c.sp()-0x8000; const auto stack=s.memory.copy_out(stack_base,0x8100);
                    layout_built_screen(s,display,builder);
                    require(meanings()==semantics,"Dialog changed control order, actions, payload, fonts, colors or text");
                    require(records()==saved,"Dialog layout changed history, records, terms or RNG");
                    require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                            "Dialog layout clobbered guest ABI/stack");
                    const bool narrow_confirmation=builder!=0x1042ca40 && overflow && size.width<822;
                    if(!adaptive || size.original() || narrow_confirmation)require(s.memory.copy_out(display,0x129da8)==old,"Legacy dialog changed");
                    else {
                        const auto panel=rect(grid(body)),buttons=rect(grid(footer));
                        require(panel.left>=110 && panel.width()<=960 && panel.right<size.width && panel.bottom<buttons.top &&
                                buttons.right==panel.right && buttons.bottom==size.height-10 && buttons.width()<=570,
                                "Dialog panel/footer bounds invalid");
                        require(panel!=ScreenRect{110,140,780,545} && panel!=ScreenRect{110,165,780,545},"Dialog recipe did not run");
                        if(overflow)require(c.u32(grid(body)+0x1fc)==2 && c.u8(grid(body)+0xbef),"Dialog lost original scrolling");
                        for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                            if(c.i16(control(i)+0x72)!=body)continue;
                            const auto box=rect(control(i)+0x10);
                            if(box.top==0 && box.bottom==0)continue;
                            require(box.left>=panel.left && box.right<=panel.right-(overflow?22:0) &&
                                    box.top>=panel.top && box.bottom<=panel.bottom,"Dialog control outside content or under scrollbar");
                            // Links must stay on the same baseline and inside
                            // their text run, including the centered message.
                            if(c.u32(control(i)+8)>=2000 || (builder==0x1042ca40 && c.u32(control(i)+8)>=7 && c.u32(control(i)+8)<=9)) {
                                bool matched=false;
                                for(unsigned j=0;j<c.u16(display+0x129da6);++j) {
                                    if(c.i16(control(j)+0x72)!=body || c.u32(control(j)+8) ||
                                       c.u32(control(j)+0x24)!=c.u32(control(i)+0x24))continue;
                                    const auto text_box=rect(control(j)+0x10);
                                    if(box.top==text_box.top && box.bottom==text_box.bottom &&
                                       box.left>=text_box.left && box.right<=text_box.right) {
                                        const auto link_offset=0xba864+i*0x17c,text_offset=0xba864+j*0x17c;
                                        require(c.i32(control(i)+0x28)-c.i32(control(j)+0x28)==
                                                old_word(link_offset+0x28)-old_word(text_offset+0x28),
                                                "Dialog link moved relative to its rendered word");
                                        if(builder!=0x1042ca40)require(c.i32(control(j)+0x2c)-c.i32(control(j)+0x28)==
                                            old_word(text_offset+0x2c)-old_word(text_offset+0x28),
                                            "Confirmation changed measured text width/centering");
                                        matched=true;
                                    }
                                }
                                require(matched,"Dialog rich link detached from its text");
                            }
                        }
                    }
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                        if(c.i16(control(i)+0x72)==footer && c.u32(control(i)+8) && !(c.u32(control(i)+0x3c)&0x20))
                            require(hit(i)==int(i),"Dialog footer hit missed original control");
                    }
                };
                build(canvas);
                if(adaptive)for(GuiCanvas size:{GuiCanvas{800,600},{801,601},{800,900},{1440,600},{3200,1800},canvas})build(size);
                if(adaptive && (scenario<2 || scenario==3 || scenario==5))
                    for(GuiCanvas size:{GuiCanvas{801,601},canvas})build(size,true);
                build(canvas);
                std::vector<std::uint8_t> outcome;
                const auto checkpoint=[&](int status) {
                    const auto append=[&](unsigned word){for(unsigned shift:{24u,16u,8u,0u})outcome.push_back(word>>shift);};
                    append(status); append(c.u32(c.u32(history())));
                    if(c.u32(c.u32(history()))==0x1042ca40)for(unsigned i=11;i<16;++i)append(value(i));
                    if(c.u32(c.u32(history()))==0x1042efa0)for(unsigned i=1;i<7;++i)append(value(i));
                };
                const auto action=[&](unsigned command) {
                    const auto i=find(command); require(hit(i)==int(i),"Dialog command hit missed");
                    const auto handler=c.u32(c.u32(history()+8));
                    const int result=std::int32_t(invoke(handler,{i})); checkpoint(result); return result;
                };
                const auto edit=[&](unsigned command) {
                    require(action(command)==-11,"Loan edit unexpectedly navigated away"); build(canvas);
                };
                if(scenario<3) {
                    for(unsigned command:{2u,1u,4u,3u,7u,8u,9u})edit(command);
                    if(scenario)edit(14);
                    if(scenario==2)edit(13);
                    // Cancel is inert; the caller would navigate afterwards.
                    const auto saved=records(); require(action(5)==-9 && records()==saved,"Loan Back changed records");
                    if(scenario==1)edit(8); // Changed counteroffer enables action 10.
                    require(action(scenario==1?10:6)==-4 && c.u32(c.u32(history()))==0x1042efa0,
                            "Loan Offer failed to open its original confirmation");
                    build(canvas);
                }
                const auto builder=c.u32(c.u32(history()));
                const unsigned cancel=builder==0x1009dc00?21:builder==0x1041eb10?2:1;
                const unsigned submit=builder==0x1009dc00?20:builder==0x1041eb10?3:2;
                require(hit(find(submit))==int(find(submit)),"Dialog submit hit missed");
                const auto saved=records(); require(action(cancel)==-9 && records()==saved,"Confirmation Cancel changed records");
                if(!adaptive)baseline=outcome;
                else require(outcome==baseline,"Adaptive dialog changed original command outcomes");
            } catch(const std::exception& e) {
                throw std::runtime_error("Dialog scenario "+std::to_string(scenario)+(adaptive?" adaptive: ":" original: ")+e.what());
            }
        }
        std::cout<<"dialog state "<<scenario<<": original commands, input, records/RNG and guest ABI preserved"<<std::endl;
    }
    initialize_new_screens();
}
