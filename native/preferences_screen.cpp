#include "preferences_screen.hpp"
#include "preferences_screen_detail.hpp"
#include "match_debugging.hpp"

namespace cm::aot::preferences_screen::detail {
namespace {
void count_languages(Call& a){
    while(true){
        a.at(0x1033d110);a.sign8(0,13);a.compare(0,11);
        if(!a.less())break;
        a.at(0x1033d0ec);
        a.move(3,13);a.call(0x10726da0);a.sign8(0,3,true);
        if(a.equal()){
        }else{
            a.at(0x1033d0fc);
            a.offset(0,20,0x1);a.sign8(20,0);
        }
        a.at(0x1033d104);
        a.offset(0,13,0x1);a.sign8(0,0);a.move(13,0);
    }
}
}

void initialize_resources(Call& a){
    a.at(0x1033d030);
    a.link();a.multiple(13);a.load(25,2,-0x7bd4);a.load(26,2,-0x7bcc);a.load(27,2,-0x7bd0);a.load(17,2,-0x7bc4);
    a.load(28,2,-0x7bb8);a.load(29,2,-0x7bb0);a.load(30,2,-0x4e18);a.store(1,0x8,0);a.frame(0x1b0);
    a.load<1>(0,2,-0x3892);a.sign8(0,0,true);
    if(!a.equal()){
    }else{
        a.at(0x1033d068);
        a.value(3,0x0);a.value(4,0xc0);a.value(5,0x0);a.value(6,0x0);a.call(0x1076bb20);a.value(0,0x1);
        a.store<2>(2,-0x3890,3);a.store<1>(2,-0x3892,0);
    }
}

void capture_session(Call& a){
    a.at(0x1033d088);
    a.load(5,2,-0x4ebc);a.move(3,29);a.load(6,2,-0x4ec0);a.value(4,0x0);a.load<2>(9,2,-0x3894);a.load(8,5,0x0);
    a.load<1>(7,5,0x4);a.load(5,6,0x0);a.load<1>(0,6,0x4);a.store<2>(1,0x68,9);a.store(1,0x84,8);a.store<1>(1,0x88,7);
    a.store(1,0x7c,5);a.store<1>(1,0x80,0);a.call(0x1073ef70);a.move(0,3);a.move(3,29);a.move(21,0);a.value(4,0x1);
    a.call(0x1073ef70);a.mask(31,3,0xff,false,0);a.value(20,0x0);a.value(13,0x0);a.jump(0x1033d110);
    count_languages(a);
}

void create_settings_frame(Call& a){
    a.at(0x1033d11c);
    a.value(3,0x0);a.call(0x10712e60);a.compare(31,0,false);
    if(a.equal()){
    }else{
        a.at(0x1033d12c);
        a.value(9,0x0);a.value(3,0x2);a.store(1,0x38,9);a.value(0,0xffffffff);a.value(4,0x0);a.value(5,0x0);
        a.store(1,0x3c,3);a.value(6,0x31f);a.value(7,0x257);a.value(8,0x1);a.store(1,0x40,9);a.value(9,0x0);
        a.value(10,0x1);a.store(1,0x44,0);a.load<2>(0,29,0x2c58,true);a.multiply(0,0,0x2c0);a.add(3,29,0);
        a.load(3,3,0x24);a.call(0x107198c0);
    }
    a.at(0x1033d178);
    a.sign8(0,20);a.value(3,0x1f4);a.compare(0,2);
    if(a.at_least()){
    }else{
        a.at(0x1033d188);
        a.value(3,0x1db);
    }
    a.at(0x1033d18c);
    a.subtract_from(4,3,0x258);a.offset(18,3,-0x1);a.mask(0,4,0x1,false,1);a.value(3,0x0);a.add(0,0,4);
    a.value(4,0x38);a.shift_right(19,0,1);a.value(5,0x0);a.add(18,19,18);a.value(6,0x0);a.call(0x1076bb20);
    a.value(0,0x0);a.value(4,0x30);a.store(1,0x38,0);a.value(0,0xffffffff);a.offset(23,29,0x2c58);
    a.offset(22,29,0x24);a.store(1,0x3c,4);a.move(5,19);a.move(7,18);a.value(4,0x91);a.store(1,0x40,3);
    a.value(6,0x2e8);a.value(8,0x1);a.value(9,0x0);a.store(1,0x44,0);a.value(10,0x1);a.load<2>(0,29,0x2c58,true);
    a.multiply(0,0,0x2c0);a.indexed_load(3,22,0);a.call(0x107198c0);a.move(3,28);a.offset(4,30,0x9b);
    a.call(0x10727890);a.value(0,0x4);a.load(6,2,-0x7bc8);a.store(1,0x38,0);a.move(3,29);a.offset(5,19,0xa);
    a.offset(7,19,0x37);a.load<2>(0,6,0x0);a.value(4,0x9b);a.value(6,0x2de);a.value(8,0x30);a.store(1,0x3c,0);
    a.value(10,0xc);a.store(1,0x40,28);a.load<2>(9,25,0x0);a.call(0x10744390);a.call(0x10768810);a.move(5,3);
    a.move(3,28);a.offset(4,30,0x891);a.call(0x10727890);a.value(0,0x1);a.load(6,2,-0x7b10);a.store(1,0x38,0);
    a.move(3,29);a.offset(5,19,0x41);a.offset(7,19,0x55);a.load<2>(0,6,0x0);a.value(4,0xa5);a.value(6,0x22b);
    a.value(8,0x1);a.store(1,0x3c,0);a.value(9,0x0);a.value(10,0x1);a.store(1,0x40,28);a.call(0x10744390);
    a.move(3,28);a.offset(4,30,0x8a8);a.call(0x10727890);a.value(0,0x1);a.load(4,2,-0x7bc8);a.store(1,0x38,0);
    a.value(9,0x27);a.value(0,0x0);a.move(3,29);a.load<2>(8,4,0x0);a.offset(5,19,0x41);a.offset(7,19,0x55);
    a.value(4,0x261);a.store(1,0x3c,8);a.value(6,0x2de);a.value(8,0x30);a.value(10,0xc);a.store(1,0x40,28);
    a.store(1,0x44,9);a.store(1,0x48,0);a.load<2>(9,17,0x0);a.call(0x10744430);a.value(8,0x0);a.value(3,0x1);
    a.store(1,0x38,8);a.value(0,0xffffffff);a.offset(5,18,-0x28);a.offset(7,18,-0xa);a.store(1,0x3c,3);
    a.offset(9,1,0x68);a.value(4,0x9b);a.value(6,0x2de);a.store(1,0x40,8);a.value(8,0x2);a.value(10,0x1);
    a.store(1,0x44,0);a.load<2>(0,23,0x0,true);a.multiply(0,0,0x2c0);a.indexed_load(3,22,0);a.call(0x107198c0);
    a.move(13,3);a.move(3,28);a.offset(4,30,0x544);a.call(0x10727890);a.load(3,2,-0x7bc8);a.value(8,0x28);
    a.value(0,0x0);a.value(5,0x0);a.load<2>(4,3,0x0);a.move(3,29);a.value(6,0x0);a.value(7,0x30);a.store(1,0x38,4);
    a.move(4,13);a.value(9,0xc);a.value(10,0x3);a.store(1,0x3c,28);a.store(1,0x40,8);a.store(1,0x44,0);
    a.load<2>(8,17,0x0);a.call(0x107444e0);a.move(3,28);a.offset(4,30,0x785);a.call(0x10727890);a.load(3,2,-0x7bc8);
    a.value(8,0x29);a.value(0,0x0);a.value(5,0x1);a.load<2>(4,3,0x0);a.move(3,29);a.value(6,0x0);a.value(7,0x30);
    a.store(1,0x38,4);a.move(4,13);a.value(9,0xc);a.value(10,0x3);a.store(1,0x3c,28);a.store(1,0x40,8);
    a.store(1,0x44,0);a.load<2>(8,17,0x0);a.call(0x107444e0);a.value(0,0x1);a.load(3,2,-0x4ec4);a.store(1,0x38,0);
    a.offset(4,19,0x5f);a.floating_load(1,3,0,true);a.offset(6,18,-0x3c);a.store(1,0x3c,13);a.offset(7,1,0x78);
    a.offset(8,1,0x74);a.offset(9,1,0x70);a.offset(10,1,0x6c);a.value(3,0xa5);a.value(5,0x2d4);a.call(0x103cbb30);
    a.value(7,0x0);a.value(3,0x1);a.store(1,0x38,7);a.sign8(4,20);a.combine(0,4,3,false,true);a.value(6,0xffffffff);
    a.store(1,0x3c,3);a.shift_right(3,0,1);a.intersect(0,0,4);a.offset(5,19,0x64);a.store(1,0x40,7);a.subtract(0,0,3);
    a.mask(3,0,0x1,false,1);a.offset(7,18,-0x41);a.store(1,0x44,6);a.offset(0,3,native_debugging_available() ? 0xf : 0xe);a.offset(9,1,0x84);
    a.value(4,0xaa);a.load<2>(3,23,0x0,true);a.mask(10,0,0xff,false,0);a.value(6,0x2cf);a.value(8,0x5);
    a.multiply(0,3,0x2c0);a.indexed_load(3,22,0);a.call(0x107198c0);
}

void return_to_caller(Call& a){
    a.at(0x1033f82c);
    a.load(0,1,0x1b8);a.offset(1,1,0x1b0);a.restore_link();a.multiple(13,true);a.finish();
}
}

namespace cm::aot::preferences_screen {
void build_screen(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    SemanticGuestCall a(s,build_screen_entry,d,o);
    detail::initialize_resources(a);
    detail::capture_session(a);
    detail::create_settings_frame(a);
    detail::build_language_selector(a);
    detail::build_currency_selector(a);
    detail::build_font_selector(a);
    detail::create_options_grid(a);
    detail::build_wage_choices(a);
    detail::build_screen_choices(a);
    detail::build_background_change_choices(a);
    detail::build_sound_choices(a);
    detail::build_database_choices(a);
    detail::build_match_storage_choices(a);
    detail::build_autosave_choices(a);
    detail::build_compression_choices(a);
    detail::build_safe_save_choices(a);
    detail::build_flashing_choices(a);
    detail::build_background_match_choices(a);
    detail::build_print_choices(a);
    append_debugging_option(s);
    detail::return_to_caller(a);
}
}
