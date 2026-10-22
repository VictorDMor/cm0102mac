#include "preferences_actions.hpp"
#include "semantic_guest_call.hpp"
#include "match_debugging.hpp"

namespace cm::aot::preferences_actions {

void open_with_mode(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    if (native_debugging_available()) begin_debugging_options();
    SemanticGuestCall a(s,open_with_mode_entry,d,o);
    a.at(0x1033ce70);
    a.link();a.store(1,-0x4,31);a.load(31,2,-0x7bb0);a.value(6,0x1);a.store(1,-0x8,30);a.move(30,3);
    a.load(4,2,-0x4eb4);a.move(3,31);a.store(1,-0xc,29);a.value(7,0x0);a.load(5,2,-0x4eb8);a.value(8,0x0);
    a.store(1,0x8,0);a.frame(0x50);a.call(0x1073e4b0);a.sign8(0,3,true);
    if(a.equal()){
    }else{
        a.at(0x1033ceb4);
        a.value(3,0x150);a.call(0x10773520);a.move(29,3,true);
        if(a.equal()){
        }else{
            a.at(0x1033cec4);
            a.load(4,2,-0x7ac8);a.load(4,4,0x0);a.call(0x1013f410);
        }
        a.at(0x1033ced0);
        a.move(3,31);a.move(5,29);a.value(4,0x0);a.value(6,0x0);a.call(0x1073f1e0);a.move(3,31);
        a.mask(5,30,0xff,false,0);a.value(4,0x1);a.value(6,0x0);a.call(0x1073f1e0);
    }
    a.at(0x1033cef8);
    a.load(0,1,0x58);a.offset(1,1,0x50);a.restore_link();a.load(31,1,-0x4);a.load(30,1,-0x8);a.load(29,1,-0xc);
    a.finish();
}

void open_from_game_state(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    if (native_debugging_available()) begin_debugging_options();
    SemanticGuestCall a(s,open_from_game_state_entry,d,o);
    a.at(0x1033cf20);
    a.link();a.store(1,-0x4,31);a.load(5,2,-0x4eb8);a.store(1,-0x8,30);a.load(4,2,-0x4eb4);a.store(1,-0xc,29);
    a.load(30,2,-0x7ac8);a.store(1,0x8,0);a.sign8(0,3,true);a.load(31,2,-0x7bb0);a.frame(0x50);
    if(!a.equal()){
        a.at(0x1033cfb0);
        a.move(3,31);a.value(6,0x1);a.value(7,0x0);a.value(8,0x0);a.call(0x1073e4b0);a.sign8(0,3,true);
        if(a.equal()){
        }else{
            a.at(0x1033cfcc);
            a.value(3,0x150);a.call(0x10773520);a.move(29,3,true);
            if(a.equal()){
            }else{
                a.at(0x1033cfdc);
                a.load(4,30,0x0);a.call(0x1013f410);
            }
            a.at(0x1033cfe4);
            a.move(3,31);a.move(5,29);a.value(4,0x0);a.value(6,0x0);a.call(0x1073f1e0);a.move(3,31);a.value(4,0x1);
            a.value(5,0x3);a.value(6,0x0);a.call(0x1073f1e0);
        }
    }else{
        a.at(0x1033cf50);
        a.move(3,31);a.value(6,0x1);a.value(7,0x0);a.value(8,0x0);a.call(0x1073e4b0);a.sign8(0,3,true);
        if(a.equal()){
        }else{
            a.at(0x1033cf6c);
            a.value(3,0x150);a.call(0x10773520);a.move(29,3,true);
            if(a.equal()){
            }else{
                a.at(0x1033cf7c);
                a.load(4,30,0x0);a.call(0x1013f410);
            }
            a.at(0x1033cf84);
            a.move(3,31);a.move(5,29);a.value(4,0x0);a.value(6,0x0);a.call(0x1073f1e0);a.move(3,31);a.value(4,0x1);
            a.value(5,0x2);a.value(6,0x0);a.call(0x1073f1e0);a.jump(0x1033d00c);
        }
    }
    a.at(0x1033d00c);
    a.load(0,1,0x58);a.offset(1,1,0x50);a.restore_link();a.load(31,1,-0x4);a.load(30,1,-0x8);a.load(29,1,-0xc);
    a.finish();
}
}
