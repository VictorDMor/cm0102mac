#include "transfer_synthesis_detail.hpp"

namespace cm::aot::transfer_synthesis::detail {
bool finalize_offer(Call& a){
    a.at(0x10415b2c);a.compare(19,-1);
    if(!a.equal()){a.compare(19,5000);if(!a.at_least()){a.load(0,16,8);a.compare(0,0);if(!a.nonpositive())a.value(19,5000);}}
    a.at(0x10415b4c);a.compare(19,-1);bool over=!a.nonpositive();if(over){a.load(0,15,0x20);a.compare_registers(0,19);over=!a.nonpositive();}
    if(over){a.compare(20,0,false);if(!a.equal()){a.load(3,20,0);a.call(0x107735a0);a.value(0,0);a.store(20,0,0);}a.at(0x10415b78);a.value(3,0);a.jump(0x10415e34);return false;}
    a.at(0x10415b80);a.load(3,2,-0x7b80);a.compare(20,0,false);a.load<2>(5,3,0,true);a.load<2>(4,3,2);a.load<1>(0,3,4);a.store<2>(1,0x44,5);a.store<2>(1,0x46,4);a.store<1>(1,0x48,0);
    bool pending=!a.equal();if(pending){a.load(0,20,0);a.compare(0,0,false);pending=!a.equal();}if(pending){a.load<1>(3,16,0x62);a.call(0x100c7a20);a.sign8(0,3,true);pending=!a.equal();}if(pending){a.compare(21,0,false);pending=!a.equal();}
    if(pending){a.load<1>(0,15,0x38);a.value(3,3);a.insert_mask(0,3,0xf0,4);a.move(4,17);a.store<1>(15,0x38,0);a.move(3,21);a.value(5,0);a.value(6,0);a.load<2>(0,21,0x32);a.store<2>(1,0x46,0);a.load<2>(0,21,0x30,true);a.store<2>(1,0x44,0);a.load<1>(0,21,0x34);a.store<1>(1,0x48,0);a.call(0x10380f20);a.sign8(0,3,true);
        if(a.equal()){a.load(3,26,0);a.move(4,16);a.move(5,17);a.value(6,0);a.call(0x103fac70);a.store(15,0x20,3);a.value(3,5);a.load<1>(0,15,0x38);a.insert_mask(0,3,0xf0,4);a.store<1>(15,0x38,0);}}
    a.at(0x10415c34);a.load(3,27,0);a.move(4,16);a.value(13,0);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
    if(!a.equal()){a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,5);if(!a.equal())a.value(13,1);}
    a.at(0x10415c70);a.compare(13,0);if(!a.equal()){a.load(3,2,-0x7b78);a.load(4,3,0);a.load(3,2,-0x7be8);a.load(0,4,4);a.load(3,3,0);a.multiply(0,0,0x248);a.add(7,3,0);a.jump(0x10415c9c);}else{a.at(0x10415c98);a.value(7,0);}
    a.at(0x10415c9c);a.load(3,26,0);a.move(4,17);a.move(6,16);a.offset(5,1,0x44);a.offset(9,1,0x38);a.value(8,1);a.call(0x103f8820);a.move(13,3);a.sign8(0,13,true);
    bool valid=!a.equal();if(!valid){a.load<2>(0,1,0x38,true);a.compare(0,9);valid=a.equal();}
    if(!valid){a.compare(20,0,false);if(!a.equal()){a.load(3,20,0);a.call(0x107735a0);a.value(0,0);a.store(20,0,0);}a.at(0x10415ce8);a.value(3,0);a.jump(0x10415e34);return false;}
    a.at(0x10415cf0);a.compare(20,0,false);bool supplied=!a.equal();if(supplied){a.load(0,20,0);a.compare(0,0,false);supplied=!a.equal();}
    bool cleared=false;if(!supplied){a.at(0x10415d04);a.load<1>(3,16,0x62);a.call(0x100c7a20);a.sign8(0,3,true);bool clear=!a.equal();if(clear){a.sign8(0,13,true);clear=!a.equal();}if(clear){a.load<1>(0,15,0x38);a.value(3,0);a.insert_mask(0,3,0xf0,4);a.store<1>(15,0x38,0);a.jump(0x10415d7c);cleared=true;}}
    if(!cleared){a.at(0x10415d30);a.load<1>(3,16,0x62);a.call(0x100c7a20);a.sign8(0,3,true);if(!a.equal()){a.load<1>(3,15,0x38);a.mask(0,3,0xf8000000,false,24);a.shift_right(4,0,28);a.sign8(0,4,true);if(a.equal()){a.value(0,1);a.insert_mask(3,0,0xf0,4);a.store<1>(15,0x38,3);a.jump(0x10415d7c);}else{a.at(0x10415d64);a.sign8(0,4);a.compare(0,3);if(a.equal()){a.value(0,4);a.insert_mask(3,0,0xf0,4);a.store<1>(15,0x38,3);}}}}
    a.at(0x10415d7c);a.load(3,27,0);a.move(4,16);a.value(13,0);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
    if(!a.equal()){a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,5);if(!a.equal())a.value(13,1);}
    a.at(0x10415db8);a.compare(13,0);if(!a.equal()){a.load(3,2,-0x7b78);a.load(4,3,0);a.load(3,2,-0x7be8);a.load(0,4,4);a.load(3,3,0);a.multiply(0,0,0x248);a.add(0,3,0);a.jump(0x10415de4);}else{a.at(0x10415de0);a.value(0,0);}
    a.at(0x10415de4);a.compare(0,0,false);if(a.equal()){a.value(0,1);a.store<1>(15,0x37,0);}
    a.at(0x10415df4);a.load<1>(0,15,0x37);a.sign8(0,0,true);bool value=!a.equal();if(value){a.load<1>(0,15,0x38);a.mask(0,0,0xf8000000,false,24);a.shift_right(0,0,28);a.sign8(0,0);a.compare(0,5);value=!a.equal();}if(value){a.load(3,26,0);a.move(4,16);a.move(5,17);a.value(6,1);a.call(0x103fac70);a.store(15,0x20,3);}
    a.at(0x10415e30);a.value(3,1);return true;
}
}
