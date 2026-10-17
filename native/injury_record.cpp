#include "injury_record.hpp"
#include "injury_record_random.hpp"

namespace cm::aot::injury_record {
namespace {
using namespace detail;

void reject_registration(Call& a,std::uint32_t start,std::int32_t scratch,std::int32_t other,std::uint32_t line) {
    a.at(start);a.offset(3,19,0x53);a.offset(6,1,scratch);a.offset(7,1,other);a.value(4,0);a.value(5,0);
    a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,20);a.offset(4,19,0x5e);
    a.offset(6,1,scratch);a.offset(7,1,other);a.value(8,line);a.call(0x1077b720);a.move(4,20);a.offset(3,19,0x6b);
    a.value(5,0);a.call(0x10769b60);a.value(0,0);a.value(3,0xffffffff);a.store<1>(22,0,0);a.jump(0x1018aeb0);
}

bool valid_registration(Call& a) {
    if(!a.equal()) {
        a.at(0x10188d7c);bool eligible=a.equal();
        if(!eligible) {a.load(3,2,-0x7be4);a.load(4,17,0);a.load(0,3,0);a.compare_registers(4,0);eligible=a.less();}
        if(!eligible) {a.value(3,0xffffffff);a.jump(0x1018aeb0);return false;}
        a.at(0x10188d9c);a.load(0,17,0x14);a.compare(0,0,false);
        if(a.equal()) {reject_registration(a,0x10188da8,0xa0,0x50,0x87f);return false;}
        a.at(0x10188e00);a.sign8(0,16,true);bool invalid=a.less();
        if(!invalid) {a.sign8(0,16);a.compare(0,82);invalid=!a.less();}
        if(invalid) {
            a.at(0x10188e14);a.load(3,2,-0x7bb8);a.offset(4,19,0xc1);a.sign8(5,16);a.call(0x1077b720);
            reject_registration(a,0x10188e24,0x80,0x48,0x887);return false;
        }
        return true;
    }
    reject_registration(a,0x10188d24,0xc0,0x58,0x872);return false;
}

void store_risk_type(Call& a,std::uint32_t start,bool positive) {
    a.at(start);a.load(0,17,0);if(!positive)a.value(4,1);a.load(3,15,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x14,16);
    a.load(0,17,0);a.load(3,15,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x15,positive?18:4);
}

bool deferred_registration(Call& a) {
    a.at(0x10188ea4);a.sign8(0,23,true);if(a.equal())return false;
    a.sign8(0,18,true);
    if(!a.equal()) {
        a.sign8(0,16);a.multiply(3,0,12);a.load(0,2,-0x64d4);a.add(3,0,3);a.load<1>(0,3,9);a.sign8(0,0,true);
        if(!a.equal()) {
            a.sign8(0,18,true);
            if(!a.at_least()) {
                a.value(3,0x66660000);a.sign8(0,18);a.offset(3,3,0x6667);a.multiply_high(0,3,0);a.shift_right(0,0,2);
                a.mask(3,0,1,false,1);a.add_record(13,0,3);
                if(a.equal()) {a.value(14,0);a.jump(0x10189048);}
                else bounded_draw<13,14,true,false,13,-1,0xf0>(a,0x10188f00);
                a.at(0x10189048);a.compare(14,0);
                if(a.equal()) {store_risk_type(a,0x10189050,false);a.jump(0x101890b0);}
                else {a.at(0x10189080);a.sign8(0,18,true);if(!a.nonpositive())store_risk_type(a,0x10189088,true);}
            } else {
                a.at(0x10189080);a.sign8(0,18,true);if(!a.nonpositive())store_risk_type(a,0x10189088,true);
            }
        }
    }
    a.at(0x101890b0);a.value(3,0xffffffff);a.jump(0x1018aeb0);return true;
}

bool retire_replaced_record(Call& a) {
    a.at(0x101890b8);a.load(0,17,0);a.load(5,15,0);a.multiply(4,0,0x24);a.add(3,5,4);a.load(0,3,0x10);a.compare(0,-1);
    if(a.equal())return true;
    a.offset(0,16,-0x4d);a.mask(0,0,0xff);a.compare(0,2,false);bool replace=a.nonpositive();
    if(!replace) {a.sign8(0,16);a.compare(0,0x51);replace=a.equal();}
    if(!replace) {a.at(0x1018913c);a.value(3,0xffffffff);a.jump(0x1018aeb0);return false;}
    a.at(0x101890f0);a.load(0,3,0x10);a.add(5,5,4);a.load(6,15,4);a.offset(4,5,0x18);a.multiply(0,0,0x38);
    a.load(3,15,12);a.offset(5,5,0x1c);a.add(23,6,0);a.load<2>(13,23,0x34,true);a.move(6,23);a.call(0x10748ce0);
    a.value(4,0xffffffff);a.store<1>(23,0,4);a.load(0,17,0);a.load(3,15,0);a.multiply(0,0,0x24);a.add(3,3,0);
    a.store(3,0x10,4);a.jump(0x10189144);return true;
}

bool allocate_injury_slot(Call& a) {
    a.at(0x10189144);a.load(0,17,0);a.load(3,15,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x21,16);
    a.load(3,15,4);a.compare(3,0,false);
    if(!a.equal()) {
        a.move(4,3);a.value(5,0);a.jump(0x10189190);
        for(;;) {
            a.at(0x10189190);a.load(0,15,8);a.compare_registers(5,0);if(!a.less())break;
            a.at(0x10189170);a.load<1>(0,4,0);a.sign8(0,0);a.compare(0,-1);
            if(a.equal()) {a.move(21,5);a.jump(0x1018919c);break;}
            a.at(0x10189188);a.offset(4,4,0x38);a.offset(5,5,1);
        }
    }
    a.at(0x1018919c);a.compare(21,-1);if(!a.equal())return true;
    a.load(4,15,8);a.compare(3,0,false);a.offset(23,4,100);
    if(!a.equal()) {a.multiply(4,23,0x38);a.call(0x107751b0);a.store(15,4,3);a.jump(0x101891d0);}
    else {a.at(0x101891c4);a.multiply(3,23,0x38);a.call(0x10775130);a.store(15,4,3);}
    a.at(0x101891d0);a.load(0,15,4);a.compare(0,0,false);
    if(a.equal()) {reject_registration(a,0x101891dc,0x60,0x40,0x8f2);return false;}
    a.at(0x10189234);a.load(0,15,8);a.value(5,0xffffffff);a.compare_registers(0,23);a.subtract(3,0,23);a.multiply(6,0,0x38);
    if(!a.at_least()) {
        a.mask(0,3,0x1fffffff,true,29);a.count_destination(0);
        if(!a.equal()) {
            do {
                a.at(0x10189258);
                for(unsigned n=0;n<8;++n) {a.load(4,15,4);a.indexed_store<1>(5,4,6);a.offset(6,6,0x38);}
            } while(a.count_nonzero());
            a.mask(3,3,7,true);if(a.equal()) {a.at(0x101892d8);a.load(21,15,8);a.store(15,8,23);return true;}
        }
        a.at(0x101892c4);a.count_destination(3);
        do {a.at(0x101892c8);a.load(4,15,4);a.indexed_store<1>(5,4,6);a.offset(6,6,0x38);} while(a.count_nonzero());
    }
    a.at(0x101892d8);a.load(21,15,8);a.store(15,8,23);return true;
}

void attach_match_context(Call& a) {
    a.at(0x101892e0);a.multiply(24,21,0x38);a.load(3,15,4);a.compare(14,0,false);a.indexed_store<1>(16,3,24);
    if(!a.equal()) {
        a.value(4,0);a.value(7,0);a.value(5,0);a.jump(0x10189348);
        for(;;) {
            a.at(0x10189348);a.sign8(0,7);a.compare(0,50);if(!a.less())break;
            a.at(0x10189308);a.load(6,14,0x1c);a.offset(3,5,0x98);a.indexed_load(0,6,3);a.compare_registers(0,17,false);
            if(a.equal()) {a.move(4,6);a.jump(0x10189354);break;}
            a.at(0x10189328);a.load(6,14,0x20);a.indexed_load(0,6,3);a.compare_registers(0,17,false);
            if(a.equal()) {a.move(4,6);a.jump(0x10189354);break;}
            a.at(0x10189340);a.offset(5,5,4);a.offset(7,7,1);
        }
        a.at(0x10189354);a.compare(4,0,false);
        if(!a.equal()) {a.at(0x10189370);a.load(0,15,4);a.offset(5,24,4);a.move(3,14);a.add(5,0,5);a.call(0x1030d4a0);a.jump(0x10189398);return;}
        a.load(0,15,4);a.value(4,0xffffffff);a.add(3,0,24);a.store<1>(3,0x20,4);a.jump(0x10189398);
    } else {a.at(0x10189388);a.load(0,15,4);a.value(4,0xffffffff);a.add(3,0,24);a.store<1>(3,0x20,4);}
}

void medical_duration_adjustment(Call& a) {
    a.at(0x10189398);a.load(0,17,0x24);a.value(19,0);a.compare(0,0,false);
    if(!a.equal()) {
        a.value(20,0);a.value(14,0);a.jump(0x10189410);
        for(;;) {
            a.at(0x10189410);a.sign8(0,20);a.compare(0,3);if(!a.less())break;
            a.at(0x101893b4);a.load(3,17,0x24);a.offset(0,14,0x190);a.indexed_load(3,3,0);a.compare(3,0,false);
            if(!a.equal()) {
                a.load(3,3,0x1c);a.compare(3,0,false);
                if(!a.equal()) {
                    a.load<1>(4,3,0x3e);a.call(0x100d2f50);a.sign8(3,3);a.sign8(0,19);a.compare_registers(3,0);
                    if(!a.nonpositive()) {
                        a.load(3,17,0x24);a.offset(0,14,0x190);a.indexed_load(3,3,0);a.load(3,3,0x1c);
                        a.load<1>(4,3,0x3e);a.call(0x100d2f50);a.move(19,3);
                    }
                }
            }
            a.at(0x10189408);a.offset(14,14,4);a.offset(20,20,1);
        }
    }
    a.at(0x1018941c);a.sign8(0,16);a.value(3,0x66660000);a.multiply(14,0,12);a.sign8(19,19);a.load(0,2,-0x64d4);
    a.offset(5,3,0x6667);a.subtract_from(4,19,35);a.add(3,0,14);a.load<2>(0,3,6,true);a.product(0,4,0);
    a.multiply_high(0,5,0);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.offset_record(22,19,5);a.add(0,0,3);a.sign16(20,0);
    if(a.equal()) {a.value(22,0);a.jump(0x101895b0);}
    else bounded_draw<22,22,false,false,19,4>(a,0x10189468);
    advance_pool(a,0x101895b0);
    a.at(0x101895d8);a.load(3,31,0);a.value(4,0x66660000);a.load(5,28,0);a.offset(0,4,0x6667);a.load<2>(4,3,0);
    a.add(5,4,5);a.multiply_high(0,0,5);a.shift_right(0,0,3);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,20);
    a.subtract(0,0,5);a.compare_registers(22,0);if(a.nonpositive())return;
    a.sign16(19,20,true);
    if(a.equal()) {a.value(22,0);a.jump(0x10189764);}
    else bounded_draw<19,22,false,false,19,-1,0xe0,true>(a,0x10189620);
    a.at(0x10189764);a.sign16(0,22);a.subtract(20,0,20);
}

void extend_duration_from_traits(Call& a) {
    a.at(0x1018976c);a.sign8(0,16);a.compare(0,0x51);if(a.equal())a.value(20,0);
    a.at(0x1018977c);a.sign8(0,16);a.load<2>(3,17,0x5c,true);a.compare(0,0x51);a.add(20,20,13);a.sign8(19,3);
    if(a.equal())return;
    a.load(0,2,-0x64d4);a.add(3,0,14);a.load<2>(0,3,4,true);a.compare(0,10);if(a.nonpositive())return;
    a.compare(19,30);
    if(!a.nonpositive()) {
        a.load(3,17,0x14);a.offset(4,19,-35);a.load<1>(0,3,0x2d);a.sign8(0,0);a.add(4,0,4);a.sign16(0,4,true);
        if(!a.nonpositive()) {
            a.sign16(13,4,true);
            if(a.equal()) {a.value(22,0);a.jump(0x10189924);}
            else bounded_draw<13,22>(a,0x101897dc);
            a.at(0x10189924);a.sign16(0,22);a.add(20,20,0);
        }
    }
    a.at(0x1018992c);a.load(3,17,0x14);a.load<1>(0,3,0x2d);a.sign8(3,0);a.compare(3,10);if(a.nonpositive())return;
    a.offset(0,3,-10);a.sign16(13,0,true);
    if(a.equal()) {a.value(22,0);a.jump(0x10189a9c);}
    else bounded_draw<13,22>(a,0x10189954);
    a.at(0x10189a9c);a.sign16(0,22);a.add(20,20,0);
}

void accumulate_injury_days(Call& a) {
    a.at(0x10189aa4);a.load(0,2,-0x64d4);a.value(23,0);a.add(13,0,14);a.load_update<2>(0,13,4,true);
    a.add(0,20,0);a.sign16(20,0);a.jump(0x10189c50);
    for(;;) {
        a.at(0x10189c50);a.compare_registers(23,20);if(a.at_least())break;
        a.load(0,17,0);a.load(4,26,0);a.multiply(3,0,100);a.offset(0,3,10);a.indexed_load<1>(0,4,0);a.compare(0,100,false);if(!a.less())break;
        a.at(0x10189ac0);a.load(3,17,0x14);a.load<1>(0,3,0x34);a.sign8(0,0);a.multiply(3,0,5);a.offset_record(25,3,12);
        if(a.equal()) {a.value(22,0);a.jump(0x10189c28);}
        else bounded_draw<25,22,false,true>(a,0x10189ae0);
        a.at(0x10189c28);a.compare(22,0);
        if(a.equal()) {a.load(0,17,0);a.load(5,26,0);a.multiply(3,0,100);a.offset(4,3,10);a.indexed_load<1>(3,5,4);a.offset(0,3,1);a.indexed_store<1>(0,5,4);}
        a.at(0x10189c4c);a.offset(23,23,1);
    }
}

void update_club_injury_counter(Call& a) {
    a.at(0x10189c74);a.load<1>(0,1,0x173);a.sign8(0,0,true);if(!a.equal())return;
    a.load(3,17,0x24);a.compare(3,0,false);if(a.equal())return;
    a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(!a.equal())return;
    a.compare(20,100);if(a.less())return;
    a.move(3,17);a.value(4,0);a.call(0x100d3040);a.sign8(0,3,true);if(!a.equal())return;
    a.load(3,2,-0x7b40);a.load(4,3,0);a.compare(4,0,false);if(a.equal())return;
    a.load(3,17,0x24);a.value(5,100);a.load(4,4,0x988);a.load(0,3,0);a.load(4,4,0xdc);a.multiply(3,0,12);a.offset(0,3,9);a.indexed_store<1>(5,4,0);
}

void prepare_long_injury_history(Call& a) {
    a.at(0x10189cf4);a.load<1>(0,1,0x173);a.sign8(0,0,true);if(!a.equal())return;
    advance_pool(a,0x10189d00);
    a.at(0x10189d28);a.load(5,31,0);a.value(3,0x51ec0000);a.load(4,28,0);a.offset(0,3,-0x7ae1);a.load<2>(3,5,0);a.add(4,3,4);
    a.multiply_high(0,0,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,50);a.subtract(3,0,4);
    a.offset(0,3,30);a.compare_registers(20,0);if(a.less())return;
    a.load(3,2,-0x7b40);a.load(3,3,0);a.compare(3,0,false);if(a.equal())return;
    a.load(3,3,0x988);a.compare(3,0,false);if(a.equal())return;
    a.move(4,17);a.value(5,0);a.call(0x10363b80);
}

void store_injury_duration(Call& a) {
    a.at(0x10189d8c);a.load(6,15,4);a.value(4,0x43300000);a.load(0,2,-0x64d4);a.exclusive_flags(5,20,0x80000000);
    a.add(6,6,24);a.store(1,0xe4,5);a.add(3,0,14);a.floating_load(3,27,8);a.store<2>(6,0x30,20);a.floating_load(0,27,0x70);
    a.load(0,15,4);a.store(1,0xe0,4);a.add(5,0,24);a.store<2>(5,0x34,20);a.floating_load(1,1,0xe0);a.load<1>(0,3,8);
    a.store(1,0xe8,4);a.floating_subtract(2,1,3,true);a.sign8(3,0);a.load(0,15,4);a.exclusive_flags(3,3,0x80000000);
    a.store(1,0xec,3);a.add(3,0,24);a.floating_load(1,1,0xe8);a.floating_subtract(1,1,3);a.floating_difference(0,1,0,true);
    a.floating_round_single(0,0);a.floating_product_single(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xf0);a.load(0,1,0xf4);a.store<2>(3,0x32,0);
    a.load(0,15,4);a.add(5,0,24);a.load<2>(3,5,0x32,true);a.load<2>(4,5,0x30,true);a.offset(0,3,60);a.compare_registers(0,4);
    if(!a.at_least()) {a.offset(0,4,-60);a.store<2>(5,0x32,0);}
    a.at(0x10189e30);a.load(3,15,4);a.load<1>(0,1,0x173);a.add(3,3,24);a.sign8(0,0,true);a.load<2>(0,3,0x32,true);a.store<2>(3,0x36,0);
}

void date_injury_record(Call& a) {
    if(!a.equal()) {
        a.compare(20,0);
        if(a.equal()) {a.value(22,0);a.jump(0x10189fa4);}
        else bounded_draw<20,22>(a,0x10189e5c);
        a.at(0x10189fa4);a.offset(0,22,1);a.load(4,2,-0x7b80);a.sign16(5,0);a.offset(3,1,0x38);a.call(0x10717b20);
        a.load(3,15,4);a.offset(4,24,0x28);a.load<2>(0,1,0x3a);a.add(4,3,4);a.store<2>(4,2,0);a.load<2>(0,1,0x38,true);
        a.store<2>(4,0,0);a.load<1>(0,1,0x3c);a.store<1>(4,4,0);a.jump(0x1018a008);
    } else {
        a.at(0x10189fe0);a.load(3,2,-0x7b80);a.offset(5,24,0x28);a.load(4,15,4);a.load<2>(0,3,2);a.add(5,4,5);
        a.store<2>(5,2,0);a.load<2>(0,3,0,true);a.store<2>(5,0,0);a.load<1>(0,3,4);a.store<1>(5,4,0);
    }
}

void remember_injury_risk(Call& a) {
    a.at(0x1018a02c);a.load(3,17,0);a.sign8(0,18,true);a.load(4,15,0);a.multiply(0,3,0x24);a.add(3,4,0);a.store(3,0x10,21);
    if(a.equal())return;
    a.load(0,2,-0x64d4);a.add(3,0,14);a.load<1>(0,3,9);a.sign8(0,0,true);if(a.equal())return;
    a.sign8(0,18,true);
    if(!a.at_least()) {
        a.value(3,0x66660000);a.sign8(20,18);a.offset(0,3,0x6667);a.multiply_high(0,0,20);a.shift_right(0,0,2);
        a.mask(3,0,1,false,1);a.add_record(14,0,3);
        if(a.equal()) {a.value(22,0);a.jump(0x1018a1d4);}
        else bounded_draw<14,22>(a,0x1018a08c);
        a.at(0x1018a1d4);a.compare(22,0);
        if(a.equal()) {
            a.load(3,17,0);a.sign8(0,18,true);a.load(4,15,0);a.multiply(0,3,0x24);a.add(3,4,0);a.store<1>(3,0x14,16);
            if(a.at_least())return;
            a.compare(20,0);
            if(a.equal()) {a.value(14,0);a.jump(0x1018a350);}
            else bounded_draw<20,14>(a,0x1018a208);
            a.at(0x1018a350);a.compare(14,0);if(!a.equal())return;
            a.load(0,17,0);a.value(4,1);a.load(3,15,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x15,4);a.jump(0x1018a3a4);return;
        }
    }
    a.at(0x1018a374);a.sign8(0,18,true);if(!a.nonpositive())store_risk_type(a,0x1018a37c,true);
}

void notify_player_injury(Call& a) {
    a.at(0x1018a3a4);a.load(0,17,0x20);a.compare(0,0,false);if(a.equal())return;
    a.load(3,2,-0x7a64);a.load(3,3,0);a.compare(3,0,false);if(a.equal())return;
    a.load(0,15,4);a.move(4,17);a.add(5,0,24);a.load<2>(5,5,0x30,true);a.call(0x102b8460);
}

void request_related_injury(Call& a) {
    a.at(0x1018a3d4);a.load(0,17,0);a.value(5,0);a.load(4,15,0);a.move(3,15);a.multiply(0,0,0x24);a.add(4,4,0);a.store<1>(4,0x20,5);
    a.load(4,17,0);a.load(0,15,4);a.multiply(4,4,0x24);a.load(6,15,0);a.add(5,0,24);a.add(4,6,4);a.call(propagation_entry);
}

void notify_retirement_risk(Call& a) {
    a.at(0x1018a40c);a.load(0,17,0);a.sign8(3,16);a.load(4,15,0);a.multiply(0,0,0x24);a.add(4,4,0);a.load<1>(0,4,0x14);
    a.sign8(0,0);a.compare_registers(3,0);if(!a.equal())return;
    a.sign8(0,18,true);if(a.less())return;
    a.load<1>(3,4,0x15);a.offset(0,3,1);a.store<1>(4,0x15,0);a.load(0,17,0);a.load(3,15,0);a.multiply(0,0,0x24);a.add(3,3,0);
    a.load<1>(4,3,0x15);a.sign8(0,4);a.compare(0,1);if(a.nonpositive())return;
    a.load<2>(0,13,0,true);a.value(3,5);a.subtract_from(0,0,200);a.compare(0,5);if(!a.nonpositive())a.move(3,0);
    a.at(0x1018a47c);a.sign8(0,18,true);a.move(20,3);if(!a.nonpositive()) {a.sign8(0,18);a.divide(20,3,0);}
    a.at(0x1018a490);a.sign8(13,4,true);
    if(a.equal()) {a.value(14,0);a.jump(0x1018a5e8);}
    else bounded_draw<13,14,true,true>(a,0x1018a4a0);
    a.at(0x1018a5e8);a.compare(20,0);
    if(a.equal()) {a.value(13,0);a.jump(0x1018a740);}
    else bounded_draw<20,13,true>(a,0x1018a5f8);
    a.at(0x1018a740);a.compare_registers(14,13);if(a.nonpositive())return;
    a.load(3,2,-0x7b28);a.load(3,3,0);a.compare(3,0,false);if(a.equal())return;
    a.load(0,15,4);a.move(4,17);a.add(5,0,24);a.load<2>(5,5,0x32,true);a.call(0x102de470);
}

template<bool Profile>
bool duration_risk_band(Call& a,std::uint32_t start,std::uint32_t pool,std::uint32_t join,
                       std::int32_t threshold,std::int32_t duration,std::int32_t draw_limit) {
    a.at(start);a.compare(19,threshold);if(a.nonpositive())return false;
    if constexpr(Profile) {
        a.load(3,17,0x14);a.load(5,15,4);a.load<2>(4,17,0x5c,true);a.load<1>(0,3,0x2d);a.add(5,5,24);
        a.sign8(3,4);a.load<2>(13,5,0x32,true);a.sign8(0,0);a.add(0,3,0);a.subtract_from(0,0,duration);
    } else {
        a.load(3,15,4);a.load<2>(0,17,0x5c,true);a.add(3,3,24);a.sign8(0,0);a.load<2>(13,3,0x32,true);a.subtract_from(0,0,duration);
    }
    a.compare_registers(13,0);if(a.nonpositive())return false;
    a.compare(13,0);
    if(a.equal()) {a.value(14,0);a.jump(join);}
    else bounded_draw<13,14,true,true>(a,pool);
    a.at(join);a.load<2>(0,17,0x5c,true);a.sign8(0,0);a.subtract_from(0,0,draw_limit);a.compare_registers(14,0);if(a.nonpositive())return false;
    a.load(3,2,-0x7b28);a.load(3,3,0);a.compare(3,0,false);if(a.equal())return true;
    a.load(0,15,4);a.move(4,17);a.add(5,0,24);a.load<2>(5,5,0x32,true);a.call(0x102de470);a.jump(0x1018ae64);return true;
}

void severe_duration_risk(Call& a) {
    if(duration_risk_band<true>(a,0x1018a76c,0x1018a7b4,0x1018a8fc,36,120,90))return;
    if(duration_risk_band<true>(a,0x1018a938,0x1018a980,0x1018aac8,33,150,90))return;
    if(duration_risk_band<false>(a,0x1018ab04,0x1018ab3c,0x1018ac84,31,300,200))return;
    a.at(0x1018acc0);a.load(0,15,4);a.add(3,0,24);a.load<2>(0,3,0x32,true);a.compare(0,500);if(a.nonpositive())return;
    a.load(3,17,0x14);a.load<1>(0,3,0x34);a.sign8(3,0);a.offset_record(13,3,5);
    if(a.equal()) {a.value(14,0);a.jump(0x1018ae38);}
    else bounded_draw<13,14,true,true>(a,0x1018acf0);
    a.at(0x1018ae38);a.compare(14,0);if(!a.equal())return;
    a.load(3,2,-0x7b28);a.load(3,3,0);a.compare(3,0,false);if(a.equal())return;
    a.load(0,15,4);a.move(4,17);a.add(5,0,24);a.load<2>(5,5,0x32,true);a.call(0x102de470);
}

void record_minor_injury_history(Call& a) {
    a.at(0x1018ae64);a.load<1>(0,1,0x17f);a.sign8(0,0,true);if(a.equal())return;
    a.mask(0,16,0xff);a.compare(0,1,false);
    if(!a.nonpositive()) {a.sign8(0,16);a.compare(0,2);if(!a.equal())return;}
    a.at(0x1018ae88);a.load(3,17,0x24);a.compare(3,0,false);if(a.equal())return;
    a.load(6,3,0);a.move(3,15);a.load(4,2,-0x7b80);a.move(5,16);a.value(7,0);a.call(0x1018bb50);
}
}

void register_injury(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,registration_entry,dependencies,observer);
    a.link();a.multiple(13);a.move(17,6,true);a.load(26,2,-0x7b9c);a.load(27,2,-0x543c);a.move(15,3);a.load(28,2,-0x7b90);
    a.move(16,4);a.load(29,2,-0x7b8c);a.move(14,7);a.load(30,2,-0x7b88);a.move(18,10);a.load(31,2,-0x7b84);a.value(21,0xffffffff);
    a.load(22,2,-0x7bac);a.value(13,0);a.load(20,2,-0x7ba8);a.load(19,2,-0x5438);a.save_link();a.frame(336);
    a.load<1>(23,1,0x18b);a.store<1>(1,0x173,5);a.store<1>(1,0x17f,8);a.store(1,0x180,9);
    if(valid_registration(a)) {
        a.at(0x10188e7c);a.move(3,15);a.move(4,17);a.call(0x10187a40);a.load(3,2,-0x7a7c);a.load(3,3,0);a.compare(3,0,false);
        if(!a.equal()) {a.move(4,17);a.value(5,0);a.call(0x103d0990);}
        if(!deferred_registration(a) && retire_replaced_record(a) && allocate_injury_slot(a)) {
            attach_match_context(a);medical_duration_adjustment(a);extend_duration_from_traits(a);accumulate_injury_days(a);
            update_club_injury_counter(a);prepare_long_injury_history(a);store_injury_duration(a);date_injury_record(a);
            a.at(0x1018a008);a.move(3,15);a.move(4,17);a.call(0x10187a40);a.load(0,15,4);a.move(3,15);a.load(6,1,0x180);
            a.move(4,17);a.add(5,0,24);a.call(notice_entry);
            remember_injury_risk(a);notify_player_injury(a);request_related_injury(a);notify_retirement_risk(a);severe_duration_risk(a);record_minor_injury_history(a);
            a.at(0x1018aeac);a.move(3,21);
        }
    }
    a.at(0x1018aeb0);a.pop(336);a.multiple(13,true);a.finish();
}
}
