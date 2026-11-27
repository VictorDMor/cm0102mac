// Paired builds and original commands in disposable existing captures. Reading
// news can acknowledge real records; compare that effect, not just geometry.
void news_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist");
    std::vector<std::vector<std::uint8_t>> baseline;
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
        const auto scratch=allocate(s,16384,true); world(s,scratch,scratch+16);
        require(s.platform->resize_gui_canvas(s,{801,601}),"News pixel initialization");
        const auto person=c.u32(manager+4),queue=c.u32(c.toc(-0x7bf0));
        const auto text=[&](unsigned at,const std::string& value) {
            for(unsigned i=0;i<=value.size();++i)c.put8(at+i,i<value.size()?std::uint8_t(value[i]):0);
        };
        // Original publication initializes owned text, unread counts and IDs.
        // Include a long article to exercise wrapping and the body scrollbar.
        invoke(0x102bdf90,{scratch+64,30,0}); // An unanswered squad call-up.
        for (unsigned field = 0; field < 3; ++field) invoke(0x102cb4b0,{scratch+64,field,0});
        invoke(0x102cbc10,{queue,scratch+64,person,0});
        const auto decision_id = c.u32(queue+16);
        unsigned newest_filtered = 0;
        for(unsigned n=0;n<36;++n) {
            invoke(0x102bdf90,{scratch+64,0x1388,0});
            invoke(0x102cb4b0,{scratch+64,0,c.u32(person)});
            invoke(0x102cb4b0,{scratch+64,1,1});
            invoke(0x102cb4b0,{scratch+64,2,n});
            std::string message="Zebra "+std::to_string(n)+" ";
            for(unsigned j=0;j<50;++j)message+="A controlled news article keeps its original words and links. ";
            text(scratch+512,message);
            invoke(0x102cb630,{scratch+64,3,scratch+512,0});
            invoke(0x102cbc10,{queue,scratch+64,person,0});
            if (n == 19) newest_filtered = c.u32(queue+16);
        }
        invoke(0x102ce710,{person,0});
        const auto history=c.u32(manager+0x10),display=c.u32(manager+0x24);
        const auto value=[&](unsigned slot){return c.u32(history+0x14+slot*8);};
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
        const auto records=[&](bool full_history) {
            std::vector<std::uint8_t> bytes;
            const auto append=[&](unsigned at,unsigned count) {
                const auto part=s.memory.copy_out(at,count); bytes.insert(bytes.end(),part.begin(),part.end());
            };
            append(queue,20); append(c.u32(queue),c.u32(queue+4)*0xe4);
            append(c.u32(queue+8),c.u32(queue+12)*12); append(person,0x70);
            if(full_history)append(history,0x118);
            else {
                for(unsigned slot:{0u,1u,3u,4u,6u,7u})append(history+0x14+slot*8,4);
                for(unsigned tab=0;tab<8;++tab)append(history+0x14+(9+3*tab)*8,4);
                if(value(5))append(value(5)+0xdc,5);
                append(value(8),30);
            }
            const auto rng=random_state(s); bytes.insert(bytes.end(),rng.begin(),rng.end()); return bytes;
        };
        unsigned checkpoint=0;
        const auto compare=[&] {
            auto current=records(false);
            // Real headlines retain their action, ID, style, font and color.
            const auto list=value(11+3*value(1));
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i);
                if(c.i16(at+0x72)!=int(list) || !c.u8(at+0x78))continue;
                for(const auto [offset,size]:{std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x78u,256u}}) {
                    const auto part=s.memory.copy_out(at+offset,size); current.insert(current.end(),part.begin(),part.end());
                }
            }
            // Wrapping may split a line or a link into several controls.
            // Compare its words and unique destinations independently of that.
            for(int owner:{1,8,9}) {
                std::vector<std::pair<unsigned,unsigned>> destinations;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                    const auto at=control(i);
                    if(c.i16(at+0x72)!=owner)continue;
                    for(unsigned j=0;j<256 && c.u8(at+0x78+j);++j) {
                        const auto ch=c.u8(at+0x78+j);
                        if(ch!=' ' && ch!='\n' && ch!='\r' && ch!='\t')current.push_back(ch);
                    }
                    if(c.u32(at+8))destinations.emplace_back(c.u32(at+8),c.u32(at+0x48));
                }
                current.push_back(0);
                std::sort(destinations.begin(),destinations.end());
                destinations.erase(std::unique(destinations.begin(),destinations.end()),destinations.end());
                for(const auto [action,payload]:destinations)for(unsigned word:{action,payload})
                    for(unsigned shift:{24u,16u,8u,0u})current.push_back((word>>shift)&255);
                current.push_back(0);
            }
            if(!adaptive)baseline.push_back(std::move(current));
            else if(current!=baseline.at(checkpoint))throw std::runtime_error("News records/headlines differ at checkpoint "+std::to_string(checkpoint));
            ++checkpoint;
        };
        const auto build=[&](GuiCanvas canvas, bool list_at_top = false) {
            require(s.platform->resize_gui_canvas(s,canvas),"News resize failed");
            require(adaptive_screen_available(s,0x102cea30)==adaptive,"News preference gate failed");
            invoke(0x10719560,{display,1,0}); invoke(0x102cea30,{});
            const auto semantic=meanings(),saved=records(true),before=s.memory.copy_out(display,0x129da8);
            const auto registers=s.registers;
            const auto unique=s.unique;
            const auto pc=s.pc,stack_base=c.sp()-0x8000; const auto stack=s.memory.copy_out(stack_base,0x8100);
            layout_built_screen(s,display,0x102cea30);
            require(meanings()==semantic && records(true)==saved,"News post-layout changed semantics, history, inbox or RNG");
            require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                    "News post-layout changed guest ABI/stack");
            if(!adaptive || canvas.original())require(s.memory.copy_out(display,0x129da8)==before,"Classic news post-layout changed");
            const auto list=value(11+3*value(1));
            require(list==6 && c.u8(grid(list)+0xbb1)<=30,"News grid index or row capacity invalid");
            const bool expanded=adaptive && !canvas.original();
            require(rect(grid(list)).right==(expanded?canvas.width-20:780),"News width ignored gate");
            if(expanded) {
                require(rect(grid(4)).bottom==canvas.height-55 && rect(grid(5)).right==canvas.width-10,"News bottom tabs misplaced");
                require(rect(grid(9)).bottom==canvas.height-100,"News reading region did not reach tabs");
                if(canvas.height>=900)require(c.u8(grid(list)+0xbb1)>5 && c.u8(grid(9)+0xbb1)>9,"Taller news did not add rows");
            }
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
            }
            if(list_at_top)
                require(value(10 + 3*value(1)) == 0 && c.i32(grid(list) + 0x1fc) == 0, "Read All did not scroll the displayed table to the top");
            // During normal navigation, the selected headline remains visible after the
            // builder restores scrolling with its own visible-row capacity.
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i);
                if(c.i16(at+0x72)==int(list) && c.u32(at+0x38)==0x210) {
                    const auto box=rect(at+0x10),panel=rect(grid(list));
                    if (box.top < panel.top || box.bottom > panel.bottom || box.height() <= 0)
                        throw std::runtime_error("Selected news outside viewport at checkpoint " + std::to_string(checkpoint) +
                            ", tab " + std::to_string(value(1)) + ", offset " + std::to_string(value(10+3*value(1))) +
                            ", first " + std::to_string(c.i32(grid(list)+0x1fc)) + ", row top " + std::to_string(box.top));
                }
            }
            // Query the original hit router without executing story callbacks.
            std::vector<unsigned> checked;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i),action=c.u32(at+8);
                // Edit fields focus first; their action is dispatched when
                // editing completes, covered by command(9) below.
                if(action!=1 && action!=2 && action!=3 && action!=5 && action!=news_screens::read_all_action && action!=2007)continue;
                if(std::find(checked.begin(),checked.end(),action)!=checked.end())continue;
                const auto box=rect(at+0x10);
                if(box.width()<=0 || box.height()<=0 || box.top<10)continue;
                c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
                c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
                const auto hit=std::int16_t(invoke(0x1071a280,{display}));
                if(hit!=int(i))throw std::runtime_error("News hit mismatch: adaptive="+std::to_string(adaptive)+
                    " checkpoint="+std::to_string(checkpoint)+" action="+std::to_string(action)+
                    " expected="+std::to_string(i)+" got="+std::to_string(hit));
                checked.push_back(action);
            }
            compare();
        };
        const auto find=[&](unsigned action,unsigned payload=0xffffffffu) {
            for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                if(c.u32(control(i)+8)==action && (payload==0xffffffffu || c.u32(control(i)+0x48)==payload))return i;
            throw std::runtime_error("Missing news action "+std::to_string(action));
        };
        const auto command=[&](unsigned action,unsigned payload=0xffffffffu) {
            require(std::int32_t(invoke(0x102cfcd0,{find(action,payload)}))==-11,"News command did not request redraw");
        };
        for(auto size:{GuiCanvas{800,600},GuiCanvas{801,601},GuiCanvas{800,900},GuiCanvas{1440,600},GuiCanvas{1280,900},GuiCanvas{3200,1800}})build(size);
        const GuiCanvas canvas{1280,900};
        for(unsigned tab=1;tab<8;++tab) { command(1,tab); build(canvas); }
        command(1,0); build(canvas);
        require(value(3)>30,"News fixture must overflow the list");
        command(2,c.u32(value(2)+(value(3)-1)*0xe4+0xdc)); build(canvas);
        if (value(9) != decision_id) { command(2,decision_id); build(canvas); }
        const auto intervention = rect(control(find(3)) + 0x10);
        require(intervention.left == 110 && intervention.right <= rect(grid(7)).left, "The intervention overlaps the filter");
        require(value(10) > 0, "Read All fixture must start scrolled away from the top");
        const auto newest_id = c.u32(value(2) + 0xdc);
        require(newest_id != decision_id, "Fixture needs a newer story than the pending decision");
        const auto read_all = rect(control(find(news_screens::read_all_action)) + 0x10);
        const auto next = rect(control(find(5)) + 0x10);
        require(next.right <= read_all.left && next.top == read_all.top, "Read All must follow Read Next on the toolbar");
        command(news_screens::read_all_action);
        build(canvas, true);
        require(value(9) == newest_id && c.u32(value(5) + 0xdc) == newest_id, "Read All did not open the newest article");

        build(canvas, true);
        command(2,decision_id); build(canvas);
        require(rect(control(find(3)) + 0x10).left == intervention.left, "Read All removed the pending intervention");
        text(value(8),"Zebra 1"); command(9); build(canvas);
        command(news_screens::read_all_action); build(canvas, true);
        require(value(9) == newest_filtered && c.u32(value(5) + 0xdc) == newest_filtered,
                "Read All opened a story outside the filter or missed the newest match");
        for(const auto* filter:{"Zebra","zzzzqqq",""}) {
            text(value(8),filter); command(9); build(canvas);
            if(std::string_view(filter)=="zzzzqqq")require(!value(5),"Unmatched filter retained a selection");
        }
        command(5); build(canvas);
        // Gate exclusions remain legacy without depending on club ownership.
        if(adaptive) {
            c.put16(manager+0x2c58,1); require(!adaptive_screen_available(s,0x102cea30),"Secondary news adapted"); c.put16(manager+0x2c58,0);
            c.put32(manager+0x2c64,1); require(!adaptive_screen_available(s,0x102cea30),"Network news adapted"); c.put32(manager+0x2c64,0);
        }
    }
    initialize_new_screens();
    std::cout<<"News paired records/RNG, original selection/filter/unread commands and presentation ABI passed\n";
}
