// Existing capture, disposable manager drafts and original negotiation
// commands. No contract is submitted to the career's persistent ledger.
void contract_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist");
    for(unsigned scenario=0;scenario<4;++scenario) {
        std::vector<std::uint8_t> original;
        for(bool adaptive:{false,true}) {
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
            require(s.platform->resize_gui_canvas(s,{801,601}),"Contract pixel initialization");
            const auto managed=c.u32(c.u32(manager+4)+0x24),clubs=c.u32(c.toc(-0x7be8));
            unsigned person=0,contract=0;
            for(unsigned i=0;i<c.u32(c.toc(-0x7b50))&&!person;++i) {
                const auto club=clubs+i*0x248;
                if((scenario<2)!=(club==managed) || c.u32(club+0x30)!=c.u32(managed+0x30))continue;
                for(unsigned j=0;j<50&&!person;++j) {
                    const auto candidate=c.u32(club+0x98+j*4);
                    if(!candidate || c.u32(candidate+0x24)!=club)continue;
                    const auto profile=c.u32(candidate+0x14);
                    if(!profile || (c.u8(profile+0xf)==20)!=(scenario==1))continue;
                    contract=invoke(0x10082f20,{c.u32(c.toc(-0x7b7c)),candidate});
                    if(contract && c.u32(contract+4)==c.u32(club) && (c.i8(contract+0x38)>>2)!=5)person=candidate;
                }
            }
            require(person,"Contract fixture needs the requested playing role");
            const auto display=c.u32(manager+0x24);
            unsigned transfer=0,link=0;
            if(scenario==3) {
                // An isolated original offer record exercises the linked
                // opener without borrowing a proposal screen's owned draft
                // or registering an offer in the career's transfer ledger.
                transfer=allocate(s,0x54,true);
                invoke(0x10411ea0,{transfer,person,managed,0,0xffffffffu,1});
                c.put32(transfer+0x28,0xffffffffu); c.put32(transfer+0x2c,0);
                link=allocate(s,4,true); c.put32(link,transfer);
                invoke(0x100965c0,{link,person,1,11,1});
            } else invoke(0x100960d0,{person,1,11,1});
            const auto history=c.u32(manager+0x10);
            require(c.u32(c.u32(history))==0x10096c60 && c.u32(c.u32(history+8))==0x10097b70,
                    "Original contract opener/handler changed");
            require(adaptive_screen_available(s,0x10096c60)==adaptive,"Contract ignored New Screens");
            if(adaptive) {
                c.put16(manager+0x2c58,1); require(!adaptive_screen_available(s,0x10096c60),"Secondary contract passed gate");
                c.put16(manager+0x2c58,0); c.put32(manager+0x2c64,1);
                require(!adaptive_screen_available(s,0x10096c60),"Network contract passed gate"); c.put32(manager+0x2c64,0);
                for(unsigned tab:{0u,4u}) {
                    c.put32(history+0x1c,tab); require(!adaptive_screen_available(s,0x10096c60),"Unknown contract tab passed gate");
                }
                c.put32(history+0x1c,1);
                c.put32(history+0x14+8*8,14);
                require(!adaptive_screen_available(s,0x10096c60),"Tabless staff contract passed playing gate");
                c.put32(history+0x14+8*8,11);
            }
            const auto draft=invoke(0x101487d0,{c.u32(c.toc(-0x7a54)),0});
            const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
            const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
            const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
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
                for(const auto [at,size]:{std::pair{person,0x70u},{contract,0x64u},{managed,0x248u},
                        {c.u32(person+0x24),0x248u},{draft,0x64u},{c.u32(history+0x24),0x64u},{history,0x110u}}) {
                    const auto part=s.memory.copy_out(at,size); bytes.insert(bytes.end(),part.begin(),part.end());
                }
                if(transfer) {
                    const auto part=s.memory.copy_out(transfer,0x54); bytes.insert(bytes.end(),part.begin(),part.end());
                }
                const auto rng=random_state(s); bytes.insert(bytes.end(),rng.begin(),rng.end()); return bytes;
            };
            const auto find=[&](unsigned action,unsigned payload=0xffffffffu) {
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.u32(control(i)+8)==action && (payload==0xffffffffu || c.u32(control(i)+0x48)==payload))return i;
                throw std::runtime_error("Contract missing action "+std::to_string(action)+"/"+std::to_string(payload));
            };
            const auto hit=[&](unsigned i) {
                const auto box=rect(control(i)+0x10);
                c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
                c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
                return std::int16_t(invoke(0x1071a280,{display}));
            };
            const auto build=[&](GuiCanvas canvas,bool overflow=false) {
                require(s.platform->resize_gui_canvas(s,canvas),"Contract resize failed");
                invoke(0x10719560,{display,1,0}); invoke(0x10096c60,{});
                std::vector<unsigned> fields;
                for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                    if(rect(grid(i))==ScreenRect{110,185,780,545}) {
                        fields.push_back(i);
                        // Controlled trailing empty rows exercise the real
                        // AutoGrid scrollbar and a nonzero scroll position.
                        if(overflow) {
                            c.put32(grid(i)+0x14,c.u8(grid(i)+0xbb1)+3); c.put32(grid(i)+0x1fc,2);
                            require(!c.u8(grid(i)+0xbef),"Overflow fixture already has a published scrollbar");
                            continue; // Exercise the layout before its first AutoGrid refresh.
                        }
                    }
                    invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
                }
                const auto before=meanings(),saved=records(),old=s.memory.copy_out(display,0x129da8);
                const auto registers=s.registers; const auto unique=s.unique; const auto pc=s.pc,stack_base=c.sp()-0x8000;
                const auto stack=s.memory.copy_out(stack_base,0x8100);
                layout_built_screen(s,display,0x10096c60);
                require(meanings()==before,"Contract layout changed texts, fonts, colors, actions, control indices or popups");
                require(records()==saved,"Contract layout changed draft, requested terms, records, history or RNG");
                require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                        "Contract layout clobbered guest ABI/stack");
                if(!adaptive || canvas.original())require(s.memory.copy_out(display,0x129da8)==old,"Legacy contract changed");
                else {
                    require(!fields.empty(),"Contract field grid absent");
                    const auto body=rect(grid(fields.front()));
                    const auto footer=rect(grid(c.i16(control(find(27))+0x72)));
                    require(body.left>=110 && body.width()<=960 && body.right<canvas.width && body.bottom<footer.top &&
                            footer.left>=100 && footer.right<=body.right && footer.bottom<canvas.height,
                            "Contract panels overlap or escape canvas/navigation");
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                        const auto at=control(i); const auto owner=c.i16(at+0x72);
                        if(std::find(fields.begin(),fields.end(),unsigned(owner))==fields.end())continue;
                        const auto box=rect(at+0x10);
                        if(box.top==0 && box.bottom==0) {
                            const auto row=c.u32(at+0x24),scroll=c.u32(grid(owner)+0x1fc);
                            require(row<scroll || row>=scroll+c.u8(grid(owner)+0xbb1),"Visible contract row unexpectedly hidden");
                            continue;
                        }
                        if(!(box.left>=body.left && box.right<=body.right && box.top>=body.top && box.bottom<=body.bottom))
                            throw std::runtime_error("Contract bounds at tab "+std::to_string(c.u32(history+0x1c))+" size "+
                                std::to_string(canvas.width)+"x"+std::to_string(canvas.height)+" overflow "+std::to_string(overflow)+
                                " control "+std::to_string(i)+" row "+std::to_string(c.u32(at+0x24))+" box "+
                                std::to_string(box.left)+","+std::to_string(box.top)+","+std::to_string(box.right)+","+std::to_string(box.bottom));
                        if(overflow)require(box.right<=body.right-22,"Contract field overlaps scrollbar gutter");
                    }
                    if(overflow)for(auto i:fields) {
                        require(c.u32(grid(i)+0x1fc)==2 && c.u8(grid(i)+0xbef),"Contract layout changed scroll or lost its scrollbar");
                        const int first=c.i16(grid(i)+0xb76);
                        require(first>=0 && c.u32(control(c.u16(grid(i)+0x212+first*2))+0x24)>=2 &&
                                (!first || c.u32(control(c.u16(grid(i)+0x210+first*2))+0x24)<2),
                                "Contract scrollbar selected a different first visible row");
                    }
                    if(fields.size()==2) {
                        const auto amount=rect(grid(fields.back()));
                        require(amount.top==body.top+8 && amount.bottom==body.bottom-8,"Clause overlay insets disagree");
                        // AutoGrid scrolls from each grid's first visible
                        // control, not a shared row origin. At scroll zero
                        // both overlays must align; independent scrolling
                        // retains the original offset/visibility semantics.
                        for(unsigned i=0;!overflow && i<c.u16(display+0x129da6);++i) {
                            if(c.i16(control(i)+0x72)!=int(fields.back()))continue;
                            for(unsigned j=0;j<c.u16(display+0x129da6);++j) {
                                if(c.i16(control(j)+0x72)!=int(fields.front()) || c.u32(control(j)+0x24)!=c.u32(control(i)+0x24))continue;
                                const auto amount_box=rect(control(i)+0x10),label_box=rect(control(j)+0x10);
                                if(amount_box.top!=label_box.top || amount_box.bottom!=label_box.bottom)
                                    throw std::runtime_error("Clause alignment "+std::to_string(canvas.width)+"x"+std::to_string(canvas.height)+
                                        " overflow "+std::to_string(overflow)+" row "+std::to_string(c.u32(control(i)+0x24))+
                                        " amount "+std::to_string(amount_box.top)+","+std::to_string(amount_box.bottom)+
                                        " label "+std::to_string(label_box.top)+","+std::to_string(label_box.bottom));
                            }
                        }
                    }
                }
                for(unsigned command:{1u,27u})require(hit(find(command))==int(find(command)),"Contract footer input missed");
                for(unsigned tab:{1u,2u,3u})require(hit(find(12,tab))==int(find(12,tab)),"Contract tab input missed");
            };
            const GuiCanvas canvas=adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600};
            build(canvas);
            if(adaptive && scenario==0)for(GuiCanvas size:{GuiCanvas{800,600},{801,601},{800,900},{1440,600},{3200,1800},canvas})
                for(unsigned tab:{1u,2u,3u}) { c.put32(history+0x1c,tab); build(size); }
            if(adaptive && scenario==0)for(GuiCanvas size:{GuiCanvas{801,601},canvas})
                for(unsigned tab:{1u,2u,3u}) { c.put32(history+0x1c,tab); build(size,true); }
            c.put32(history+0x1c,1); build(canvas);
            std::vector<std::uint8_t> outcome;
            const auto checkpoint=[&] {
                for(const auto [at,size]:{std::pair{draft,0x64u},{history+0x14+9*8,4*8u}}) {
                    const auto part=s.memory.copy_out(at,size); outcome.insert(outcome.end(),part.begin(),part.end());
                }
                // Opening confirmation releases the old screen's requested
                // terms. Compare its lifetime as well as its live contents.
                const auto requested=c.u32(history+0x24);
                outcome.push_back(requested!=0);
                if(requested) {
                    const auto part=s.memory.copy_out(requested,0x64); outcome.insert(outcome.end(),part.begin(),part.end());
                }
            };
            const auto action=[&](unsigned command,unsigned payload=0xffffffffu) {
                try {
                    const auto i=find(command,payload); require(hit(i)==int(i),"Contract action missed its displayed control");
                    const auto result=std::int32_t(invoke(0x10097b70,{i})); checkpoint(); return result;
                } catch(const std::exception& error) {
                    throw std::runtime_error("Contract scenario "+std::to_string(scenario)+(adaptive?" adaptive":" original")+
                        " command "+std::to_string(command)+": "+error.what());
                }
            };
            const auto edit=[&](unsigned command,unsigned payload=0xffffffffu) {
                std::vector<ScreenRect> bounds;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)bounds.push_back(rect(control(i)+0x10));
                const auto status=action(command,payload);
                require(status==0 || status==-11,"Contract edit unexpectedly navigated away");
                if(status==-11)build(canvas);
                else for(unsigned i=0;i<bounds.size();++i)require(rect(control(i)+0x10)==bounds[i],
                                                                 "Incremental contract edit lost its drawing/input bounds");
            };
            // Values that refresh in place must retain both rendering and
            // input positions; handlers use adjacent original control indices.
            for(unsigned command:{5u,4u,24u,23u})edit(command);
            for(unsigned offset:{0u,2u}) {
                const auto command=c.u32(control(c.u32(history+0x14+17*8)+offset)+8);
                if(command) {
                    require(command==9 || command==10 || command==29,"Unexpected original duration action");
                    edit(command);
                }
            }
            edit(11); edit(30);
            const auto menu=[&](unsigned action_code) {
                const auto choice=find(action_code);
                const auto popup=c.i16(control(choice)+0x72);
                unsigned trigger=0xffffffffu;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.i16(control(i)+0x74)==popup) { trigger=i; break; }
                require(trigger!=0xffffffffu && hit(trigger)==-1 && c.u8(display)==1 && c.i16(display+0x12a930)==popup,
                        "Contract amount menu lost its trigger");
                const auto box=rect(grid(popup));
                require(box.left>=0 && box.right<canvas.width && box.top>=0 && box.bottom<canvas.height,
                        "Contract popup escaped canvas");
                require(std::int32_t(invoke(0x10097b70,{choice}))==-11,"Contract amount choice failed");
                checkpoint(); build(canvas);
            };
            menu(8); menu(25);
            edit(12,2);
            // The chosen bonus differs for keepers, but activation remains
            // the original per-role command and menu.
            unsigned bonus=0xffffffffu;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                if(c.u32(control(i)+8)==13) { bonus=c.u32(control(i)+0x48); break; }
            if(bonus==0xffffffffu) {
                bonus=c.u32(control(find(14))+0x48);
                edit(14,bonus);
            }
            require(bonus<3,"Contract bonus fixture has no supported bonus");
            edit(13,bonus); edit(15,bonus); edit(16,bonus); menu(42+bonus); edit(14,bonus);
            edit(12,3);
            if(c.u8(draft+0x1d)==1)edit(19,1);
            edit(18,1); edit(39); edit(40); menu(38); edit(19,1);
            edit(12,1);
            const auto final_result=action(scenario%2?1:27);
            if(scenario%2) {
                require(final_result==-4 && c.u32(c.u32(c.u32(manager+0x10)))==0x1009dc00,
                        "Contract Offer failed to open original confirmation");
            } else require(final_result==-10 && c.u32(draft)==0xffffffffu,"Cancel did not invalidate the original draft");
            if(!adaptive)original=outcome;
            else require(outcome==original,"Adaptive contract changed original edit/offer/cancel results");
        }
        std::cout<<"contract state "<<scenario<<": three tabs, original commands/popups/outcome, ABI/draft/records/RNG"<<std::endl;
    }
    initialize_new_screens();
}
