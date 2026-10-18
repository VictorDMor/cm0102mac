#include "money_text_detail.hpp"
namespace cm::aot::money_text {
void selected_symbol_length(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    detail::Call a(s,selected_symbol_length_entry,d,o);
    a.link();
    a.load(3,2,-0x59d0);
    a.store(1,0x8,0);
    a.frame(0x40);
    a.call(0x1077dbd0);
    a.sign8(3,3);
    a.load(0,1,0x48);
    a.offset(1,1,0x40);
    a.restore_link();
    a.finish();
}
void display_setting(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    detail::Call a(s,display_setting_entry,d,o);
    a.load<1>(3,2,-0x3b02);
    a.finish();
}
void currency_label(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    detail::Call a(s,currency_label_entry,d,o);
    a.link();
    a.store(1,-0x4,31);
    a.compare(3,0x0,false);
    a.load(31,2,-0x59d8);
    a.store(1,-0x8,30);
    a.load(30,2,-0x7ba8);
    a.store(1,-0xc,29);
    a.load(29,2,-0x7bac);
    a.store(1,0x8,0);
    a.frame(0xa0);
    bool valid=false;
    if(!a.equal()){a.sign16(0,4);a.compare(0,50);valid=a.at_least();}
    if(!valid){a.at(0x1000b198);
    a.move(3,31);
    a.offset(6,1,0x68);
    a.offset(7,1,0x40);
    a.value(4,0x0);
    a.value(5,0x0);
    a.call(0x10739890);
    a.call(0x10768810);
    a.move(5,3);
    a.move(3,30);
    a.offset(4,31,0x9);
    a.offset(6,1,0x68);
    a.offset(7,1,0x40);
    a.value(8,0x272);
    a.call(0x1077b720);
    a.move(4,30);
    a.offset(3,31,0x16);
    a.value(5,0x0);
    a.call(0x10769b60);
    a.value(0,0x0);
    a.value(3,0x0);
    a.store<1>(29,0x0,0);
    a.jump(0x1000b3f4);
    }else{
        a.at(0x1000b1f0);a.sign8(0,5);a.compare(0,24,false);
        if(a.positive()){a.at(0x1000b3a0);
    a.move(3,31);
    a.offset(6,1,0x48);
    a.offset(7,1,0x38);
    a.value(4,0x0);
    a.value(5,0x0);
    a.call(0x10739890);
    a.call(0x10768810);
    a.move(5,3);
    a.move(3,30);
    a.offset(4,31,0x9);
    a.offset(6,1,0x48);
    a.offset(7,1,0x38);
    a.value(8,0x2c5);
    a.call(0x1077b720);
    a.move(4,30);
    a.offset(3,31,0x16);
    a.value(5,0x0);
    a.call(0x10769b60);
    a.value(0,0x0);
    a.value(3,0x0);
    a.store<1>(29,0x0,0);
        }else{
            a.load(4,2,-0x59f8);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(4,4,0);a.count_destination(4);
            // One bounded table selection; no instruction-dispatch loop.
            switch(a.count_branch()){
            case 0x1000b210: a.at(0x1000b210);a.offset(4,31,0x13b);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b220: a.at(0x1000b220);a.offset(4,31,0x165);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b230: a.at(0x1000b230);a.offset(4,31,0x16a);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b240: a.at(0x1000b240);a.offset(4,31,0x16f);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b250: a.at(0x1000b250);a.offset(4,31,0x173);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b260: a.at(0x1000b260);a.offset(4,31,0x178);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b270: a.at(0x1000b270);a.offset(4,31,0x5d);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b280: a.at(0x1000b280);a.offset(4,31,0x17d);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b290: a.at(0x1000b290);a.offset(4,31,0x182);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b2a0: a.at(0x1000b2a0);a.offset(4,31,0x189);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b2b0: a.at(0x1000b2b0);a.offset(4,31,0x18d);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b2c0: a.at(0x1000b2c0);a.offset(4,31,0x190);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b2d0: a.at(0x1000b2d0);a.offset(4,31,0x194);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b2e0: a.at(0x1000b2e0);a.offset(4,31,0x19c);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b2f0: a.at(0x1000b2f0);a.offset(4,31,0x1a0);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b300: a.at(0x1000b300);a.offset(4,31,0xce);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b310: a.at(0x1000b310);a.offset(4,31,0x1a5);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b320: a.at(0x1000b320);a.offset(4,31,0x1a9);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b330: a.at(0x1000b330);a.offset(4,31,0x1af);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b340: a.at(0x1000b340);a.offset(4,31,0x1b6);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b350: a.at(0x1000b350);a.offset(4,31,0x1bc);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b360: a.at(0x1000b360);a.offset(4,31,0x1c0);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b370: a.at(0x1000b370);a.offset(4,31,0x1c4);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b380: a.at(0x1000b380);a.offset(4,31,0x1c8);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            case 0x1000b390: a.at(0x1000b390);a.offset(4,31,0x1c);a.call(0x1077dbf0);a.value(3,0x1);a.jump(0x1000b3f4);break;
            default:throw std::runtime_error("Unknown currency-label table destination");
            }
        }
    }
    a.at(0x1000b3f4);
    a.load(0,1,0xa8);
    a.offset(1,1,0xa0);
    a.restore_link();
    a.load(31,1,-0x4);
    a.load(30,1,-0x8);
    a.load(29,1,-0xc);
    a.finish();
}
void format_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    detail::Call a(s,format_amount_entry,d,o);
    a.link();
    a.multiple(13);
    a.move(15,3,true);
    a.load(25,2,-0x59fc);
    a.load(14,2,-0x59f0);
    a.move(17,5);
    a.load(26,2,-0x59ec);
    a.move(16,6);
    a.load(27,2,-0x59e8);
    a.load(13,2,-0x59e4);
    a.load(28,2,-0x59e0);
    a.load(18,2,-0x7ba8);
    a.load(29,2,-0x59d8);
    a.load(30,2,-0x59d0);
    a.store(1,0x8,0);
    a.frame(0x110);
    a.store<1>(1,0x13b,7);
    bool valid=false;
    if(!a.equal()){a.sign16(0,4);a.compare(0,25);valid=a.at_least();}
    if(!valid){a.at(0x1000b464);
    a.move(3,29);
    a.offset(6,1,0x80);
    a.offset(7,1,0x78);
    a.value(4,0x0);
    a.value(5,0x0);
    a.call(0x10739890);
    a.call(0x10768810);
    a.move(5,3);
    a.move(3,18);
    a.offset(4,29,0x9);
    a.offset(6,1,0x80);
    a.offset(7,1,0x78);
    a.value(8,0x2dd);
    a.call(0x1077b720);
    a.move(4,18);
    a.offset(3,29,0x16);
    a.value(5,0x0);
    a.call(0x10769b60);
    a.load(4,2,-0x7bac);
    a.value(0,0x0);
    a.value(3,0x0);
    a.store<1>(4,0x0,0);
    a.jump(0x1000c750);
    }else{
        a.at(0x1000b4c0);
    a.value(24,0x0);
    a.offset(31,1,0xa0);
    a.store<1>(15,0x0,24);
    a.move(23,24);
    a.value(18,0x0);
    a.value(19,0x0);
    a.store<1>(1,0xa0,24);
    a.value(20,0x0);
    a.jump(0x1000b588);
        detail::build_decimal_digits(a);
        a.at(0x1000b5b0);a.load<1>(0,1,0x13b);a.mask(0,0,2,true);
        if(!a.equal()){
            a.offset(3,1,0xa0);a.call(0x1077dbd0);a.compare(3,15,false);
            if(a.positive()){a.at(0x1000bcfc);detail::render_grouped(a,0x1000bcfc);}
            else{a.load(4,2,-0x5a00);a.mask(0,3,0xfffffffc,false,2);a.indexed_load(4,4,0);a.count_destination(4);detail::render_grouped(a,a.count_branch());}
        }else{
            a.at(0x1000bd14);a.offset(3,1,0xa0);a.call(0x1077dbd0);a.compare(3,15,false);
            if(a.positive()){a.at(0x1000c708);detail::render_compact(a,0x1000c708);}
            else{a.load(4,2,-0x5a04);a.mask(0,3,0xfffffffc,false,2);a.indexed_load(4,4,0);a.count_destination(4);detail::render_compact(a,a.count_branch());}
        }
        a.at(0x1000c71c);detail::replace_spaces(a);
    }
    a.at(0x1000c750);
    a.load(0,1,0x118);
    a.offset(1,1,0x110);
    a.restore_link();
    a.multiple(13,true);
    a.finish();
}
}
