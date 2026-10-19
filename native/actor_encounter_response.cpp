#include "actor_encounter.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_encounter {
using Call = SemanticGuestCall;
namespace {
void kind_1_probability(Call&);
void kind_2_probability(Call&);
void kind_3_probability(Call&);
void kind_4_probability(Call&);
void kind_5_probability(Call&);
void kind_1_probability(Call& a) {
    a.at(0x102866fc); a.floating_load(31,24,424,true); a.floating_integer(0,31); a.floating_store(0,1,136);
    a.load(0,1,0x8c); a.sign8(0,0); a.subtract_from(0,0,0x6e); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x10286724); a.load(3,29,0x0); a.load(0,28,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(29,0x0,3);
        if (a.positive()) {
            a.at(0x1028673c); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(26,0x0,0);
        }
        a.at(0x1028674c); a.floating_integer(0,31); a.value(3,0x10000); a.offset(3,3,-0x1);
        a.floating_store(0,1,136); a.load(0,1,0x8c); a.sign8(0,0); a.subtract_from(0,0,0x6e);
        a.compare_registers(0,3);
        if (a.positive()) {
            a.at(0x10286794); a.load(4,29,0x0); a.value(3,0x80010000); a.floating_integer(1,31);
            a.value(0,0x43300000); a.load<2>(5,4,0x0); a.floating_integer(0,31); a.load(4,26,0x0);
            a.value(6,0x10000); a.add(9,5,4); a.offset(3,3,-0x7fff); a.multiply_high(7,3,9);
            a.floating_store(1,1,136); a.value(4,0x43300000); a.floating_load(2,25,128); a.load(5,1,0x8c);
            a.value(3,0x80010000); a.add(7,7,9); a.sign8(5,5); a.shift_right(7,7,0xf); a.floating_store(0,1,104);
            a.subtract_from(5,5,0x6e); a.floating_load(0,25,0); a.exclusive_flags(5,5,0x80000000);
            a.store(1,0x80,0); a.load(0,1,0x6c); a.mask(8,7,0x1,false,1); a.store(1,0x84,5); a.add(5,7,8);
            a.offset(6,6,-0x1); a.sign8(0,0); a.floating_load(1,1,128); a.subtract_from(0,0,0x6e);
            a.product(5,5,6); a.store(1,0x78,4); a.floating_subtract(2,1,2); a.floating_load(1,25,128);
            a.offset(3,3,-0x7fff); a.floating_difference(2,2,0,true); a.subtract(25,5,9);
            a.exclusive_flags(4,25,0x80000000); a.store(1,0x7c,4); a.multiply(0,0,0xa); a.floating_load(0,1,120);
            a.floating_subtract(0,0,1); a.multiply_high(3,3,0); a.floating_product(0,2,0); a.add(0,3,0);
            a.shift_right(0,0,0xf); a.floating_integer(0,0); a.mask(3,0,0x1,false,1); a.add(3,0,3);
            a.floating_store(0,1,112); a.load(25,1,0x74); a.call(0x10226520); a.value(4,0x66660000);
            a.floating_integer(0,31); a.offset(0,3,0x5); a.offset(4,4,0x6667); a.multiply_high(3,4,0);
            a.floating_store(0,1,96); a.load(0,1,0x64); a.sign8(0,0); a.shift_right(3,3,0x2);
            a.mask(4,3,0x1,false,1); a.add(4,3,4); a.subtract_from(3,0,0x6e); a.add(25,25,4); a.offset(0,3,-0x1);
            a.compare_registers(25,0);
            if (a.positive()) {
                a.at(0x102868ac); a.floating_integer(0,31); a.floating_store(0,1,88); a.load(0,1,0x5c);
                a.sign8(0,0); a.subtract_from(3,0,0x6e); a.offset(25,3,-0x1); a.jump(0x10286920);
            }
            a.at(0x10286920);
        } else {
            a.at(0x10286770); a.floating_integer(0,31); a.value(3,0xffff0000); a.offset(3,3,0x1);
            a.floating_store(0,1,136); a.load(0,1,0x8c); a.sign8(0,0); a.subtract_from(0,0,0x6e);
            a.compare_registers(0,3);
            if (a.at_least()) {
                a.at(0x102868c8); a.floating_integer(1,31); a.load(3,29,0x0); a.floating_integer(0,31);
                a.load(0,26,0x0); a.load<2>(3,3,0x0); a.floating_store(1,1,88); a.add(5,3,0); a.load(0,1,0x5c);
                a.floating_store(0,1,96); a.sign8(0,0); a.subtract_from(4,0,0x6e); a.load(0,1,0x64);
                a.divide(3,5,4); a.sign8(0,0); a.subtract_from(0,0,0x6e); a.compare(0,0); a.product(0,3,4);
                a.subtract(25,0,5); a.negate(0,25);
                if (a.positive()) {
                    a.at(0x10286918); a.move(0,25);
                }
                a.at(0x1028691c); a.move(25,0);
            } else {
                a.at(0x10286794); a.load(4,29,0x0); a.value(3,0x80010000); a.floating_integer(1,31);
                a.value(0,0x43300000); a.load<2>(5,4,0x0); a.floating_integer(0,31); a.load(4,26,0x0);
                a.value(6,0x10000); a.add(9,5,4); a.offset(3,3,-0x7fff); a.multiply_high(7,3,9);
                a.floating_store(1,1,136); a.value(4,0x43300000); a.floating_load(2,25,128); a.load(5,1,0x8c);
                a.value(3,0x80010000); a.add(7,7,9); a.sign8(5,5); a.shift_right(7,7,0xf);
                a.floating_store(0,1,104); a.subtract_from(5,5,0x6e); a.floating_load(0,25,0);
                a.exclusive_flags(5,5,0x80000000); a.store(1,0x80,0); a.load(0,1,0x6c); a.mask(8,7,0x1,false,1);
                a.store(1,0x84,5); a.add(5,7,8); a.offset(6,6,-0x1); a.sign8(0,0); a.floating_load(1,1,128);
                a.subtract_from(0,0,0x6e); a.product(5,5,6); a.store(1,0x78,4); a.floating_subtract(2,1,2);
                a.floating_load(1,25,128); a.offset(3,3,-0x7fff); a.floating_difference(2,2,0,true);
                a.subtract(25,5,9); a.exclusive_flags(4,25,0x80000000); a.store(1,0x7c,4); a.multiply(0,0,0xa);
                a.floating_load(0,1,120); a.floating_subtract(0,0,1); a.multiply_high(3,3,0);
                a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_integer(0,0);
                a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_store(0,1,112); a.load(25,1,0x74);
                a.call(0x10226520); a.value(4,0x66660000); a.floating_integer(0,31); a.offset(0,3,0x5);
                a.offset(4,4,0x6667); a.multiply_high(3,4,0); a.floating_store(0,1,96); a.load(0,1,0x64);
                a.sign8(0,0); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(4,3,4);
                a.subtract_from(3,0,0x6e); a.add(25,25,4); a.offset(0,3,-0x1); a.compare_registers(25,0);
                if (a.positive()) {
                    a.at(0x102868ac); a.floating_integer(0,31); a.floating_store(0,1,88); a.load(0,1,0x5c);
                    a.sign8(0,0); a.subtract_from(3,0,0x6e); a.offset(25,3,-0x1); a.jump(0x10286920);
                }
                a.at(0x10286920);
            }
            a.at(0x10286920);
        }
        a.at(0x10286920);
    } else {
        a.at(0x1028671c); a.value(25,0x0); a.jump(0x10286920);
    }
    a.at(0x10286920); a.sign8(0,31); a.compare_registers(25,0);
    if (a.at_least()) {
        a.at(0x102875d4); a.value(3,0x0);
    } else {
        a.at(0x1028692c); a.value(3,0x1); a.jump(0x102875d8);
    }
    a.at(0x102875d8);
}
void kind_2_probability(Call& a) {
    a.at(0x10286934); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x5c,3);
    a.floating_load(2,25,128); a.store(1,0x58,0); a.floating_load(3,25,944); a.floating_load(1,1,88);
    a.floating_load(0,25,136); a.floating_subtract(1,1,2); a.floating_difference(1,3,1,true);
    a.floating_compare(1,0);
    if (!a.equal()) {
        a.at(0x1028696c); a.load(3,29,0x0); a.load(0,28,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(29,0x0,3);
        if (a.positive()) {
            a.at(0x10286984); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(26,0x0,0);
        }
        a.at(0x10286994); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x5c,3);
        a.value(3,0x10000); a.floating_load(1,25,128); a.offset(3,3,-0x1); a.store(1,0x58,0);
        a.floating_load(2,25,944); a.floating_load(0,1,88); a.floating_subtract(0,0,1);
        a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,96); a.load(0,1,0x64);
        a.compare_registers(0,3);
        if (a.positive()) {
            a.at(0x10286a14); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x5c,0);
            a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000); a.store(1,0x58,4);
            a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000); a.floating_load(0,1,88);
            a.value(7,0x10000); a.floating_load(2,25,944); a.value(4,0x80010000); a.floating_subtract(0,0,1);
            a.store(1,0x84,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff); a.store(1,0x80,0);
            a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,128);
            a.floating_load(2,25,944); a.offset(4,4,-0x7fff); a.store(1,0x68,6); a.load(9,29,0x0);
            a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
            a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,96);
            a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x64); a.add(6,6,3); a.store(1,0x70,5);
            a.exclusive_flags(0,0,0x80000000); a.store(1,0x6c,0); a.multiply_high(0,8,6); a.floating_integer(0,0);
            a.floating_load(1,25,128); a.floating_load(2,1,104); a.floating_subtract(2,2,4);
            a.floating_store(0,1,136); a.add(3,0,6); a.load(0,1,0x8c); a.shift_right(3,3,0xf);
            a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5); a.product(3,3,7);
            a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x74,3); a.multiply(0,0,0xa);
            a.floating_load(0,1,112); a.multiply_high(3,4,0); a.floating_subtract(0,0,1);
            a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
            a.floating_integer(0,0); a.add(3,0,3); a.floating_store(0,1,120); a.load(26,1,0x7c);
            a.call(0x10226520); a.exclusive_flags(4,30,0x80000000); a.value(0,0x43300000); a.store(1,0x54,4);
            a.value(4,0x66660000); a.floating_load(1,25,128); a.offset(4,4,0x6667); a.store(1,0x50,0);
            a.offset(0,3,0x5); a.multiply_high(0,4,0); a.floating_load(2,25,944); a.floating_load(0,1,80);
            a.floating_subtract(0,0,1); a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true);
            a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(26,26,0); a.floating_integer(0,0);
            a.floating_store(0,1,72); a.load(3,1,0x4c); a.offset(0,3,-0x1); a.compare_registers(26,0);
            if (a.positive()) {
                a.at(0x10286b80); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x44,3);
                a.floating_load(1,25,128); a.store(1,0x40,0); a.floating_load(2,25,944); a.floating_load(0,1,64);
                a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true); a.floating_integer(0,0);
                a.floating_store(0,1,56); a.load(3,1,0x3c); a.offset(26,3,-0x1); a.jump(0x10286c48);
            }
            a.at(0x10286c48);
        } else {
            a.at(0x102869d4); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x5c,3);
            a.value(3,0xffff0000); a.floating_load(1,25,128); a.offset(3,3,0x1); a.store(1,0x58,0);
            a.floating_load(2,25,944); a.floating_load(0,1,88); a.floating_subtract(0,0,1);
            a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,96); a.load(0,1,0x64);
            a.compare_registers(0,3);
            if (a.at_least()) {
                a.at(0x10286bb8); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.load(5,29,0x0); a.store(1,0x38,4);
                a.floating_load(1,25,128); a.floating_load(0,1,56); a.floating_load(2,25,944);
                a.floating_subtract(0,0,1); a.store(1,0x4c,3); a.floating_load(1,25,128); a.store(1,0x48,0);
                a.floating_difference(3,2,0,true); a.floating_load(2,25,944); a.floating_load(0,1,72);
                a.load<2>(3,5,0x0); a.load(0,26,0x0); a.add(4,3,0); a.floating_subtract(0,0,1);
                a.floating_integer(1,3); a.floating_difference(0,2,0,true); a.floating_store(1,1,64);
                a.load(3,1,0x44); a.divide(0,4,3); a.product(0,0,3); a.subtract(26,0,4); a.floating_integer(0,0);
                a.negate(3,26); a.floating_store(0,1,80); a.load(0,1,0x54); a.compare(0,0);
                if (a.positive()) {
                    a.at(0x10286c40); a.move(3,26);
                }
                a.at(0x10286c44); a.move(26,3);
            } else {
                a.at(0x10286a14); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x5c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000);
                a.store(1,0x58,4); a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000);
                a.floating_load(0,1,88); a.value(7,0x10000); a.floating_load(2,25,944); a.value(4,0x80010000);
                a.floating_subtract(0,0,1); a.store(1,0x84,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff);
                a.store(1,0x80,0); a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,128);
                a.floating_load(2,25,944); a.offset(4,4,-0x7fff); a.store(1,0x68,6); a.load(9,29,0x0);
                a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
                a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,96);
                a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x64); a.add(6,6,3);
                a.store(1,0x70,5); a.exclusive_flags(0,0,0x80000000); a.store(1,0x6c,0); a.multiply_high(0,8,6);
                a.floating_integer(0,0); a.floating_load(1,25,128); a.floating_load(2,1,104);
                a.floating_subtract(2,2,4); a.floating_store(0,1,136); a.add(3,0,6); a.load(0,1,0x8c);
                a.shift_right(3,3,0xf); a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5);
                a.product(3,3,7); a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x74,3);
                a.multiply(0,0,0xa); a.floating_load(0,1,112); a.multiply_high(3,4,0); a.floating_subtract(0,0,1);
                a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
                a.floating_integer(0,0); a.add(3,0,3); a.floating_store(0,1,120); a.load(26,1,0x7c);
                a.call(0x10226520); a.exclusive_flags(4,30,0x80000000); a.value(0,0x43300000); a.store(1,0x54,4);
                a.value(4,0x66660000); a.floating_load(1,25,128); a.offset(4,4,0x6667); a.store(1,0x50,0);
                a.offset(0,3,0x5); a.multiply_high(0,4,0); a.floating_load(2,25,944); a.floating_load(0,1,80);
                a.floating_subtract(0,0,1); a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true);
                a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(26,26,0); a.floating_integer(0,0);
                a.floating_store(0,1,72); a.load(3,1,0x4c); a.offset(0,3,-0x1); a.compare_registers(26,0);
                if (a.positive()) {
                    a.at(0x10286b80); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x44,3);
                    a.floating_load(1,25,128); a.store(1,0x40,0); a.floating_load(2,25,944);
                    a.floating_load(0,1,64); a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true);
                    a.floating_integer(0,0); a.floating_store(0,1,56); a.load(3,1,0x3c); a.offset(26,3,-0x1);
                    a.jump(0x10286c48);
                }
                a.at(0x10286c48);
            }
            a.at(0x10286c48);
        }
        a.at(0x10286c48);
    } else {
        a.at(0x10286964); a.value(26,0x0); a.jump(0x10286c48);
    }
    a.at(0x10286c48); a.sign8(0,31); a.compare_registers(26,0);
    if (a.at_least()) {
        a.at(0x102875d4); a.value(3,0x0);
    } else {
        a.at(0x10286c54); a.value(3,0x1); a.jump(0x102875d8);
    }
    a.at(0x102875d8);
}
void kind_3_probability(Call& a) {
    a.at(0x10286c5c); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
    a.floating_load(2,25,128); a.store(1,0x38,0); a.floating_load(3,25,952); a.floating_load(1,1,56);
    a.floating_load(0,25,136); a.floating_subtract(1,1,2); a.floating_difference(1,3,1,true);
    a.floating_compare(1,0);
    if (!a.equal()) {
        a.at(0x10286c94); a.load(3,29,0x0); a.load(0,28,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(29,0x0,3);
        if (a.positive()) {
            a.at(0x10286cac); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(26,0x0,0);
        }
        a.at(0x10286cbc); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
        a.value(3,0x10000); a.floating_load(1,25,128); a.offset(3,3,-0x1); a.store(1,0x38,0);
        a.floating_load(2,25,952); a.floating_load(0,1,56); a.floating_subtract(0,0,1);
        a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,64); a.load(0,1,0x44);
        a.compare_registers(0,3);
        if (a.positive()) {
            a.at(0x10286d3c); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
            a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000); a.store(1,0x38,4);
            a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000); a.floating_load(0,1,56);
            a.value(7,0x10000); a.floating_load(2,25,952); a.value(4,0x80010000); a.floating_subtract(0,0,1);
            a.store(1,0x64,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff); a.store(1,0x60,0);
            a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,96);
            a.floating_load(2,25,952); a.offset(4,4,-0x7fff); a.store(1,0x48,6); a.load(9,29,0x0);
            a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
            a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,64);
            a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x44); a.add(6,6,3); a.store(1,0x50,5);
            a.exclusive_flags(0,0,0x80000000); a.store(1,0x4c,0); a.multiply_high(0,8,6); a.floating_integer(0,0);
            a.floating_load(1,25,128); a.floating_load(2,1,72); a.floating_subtract(2,2,4);
            a.floating_store(0,1,104); a.add(3,0,6); a.load(0,1,0x6c); a.shift_right(3,3,0xf);
            a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5); a.product(3,3,7);
            a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x54,3); a.multiply(0,0,0xa);
            a.floating_load(0,1,80); a.multiply_high(3,4,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
            a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
            a.floating_store(0,1,88); a.load(26,1,0x5c); a.call(0x10226520); a.exclusive_flags(4,30,0x80000000);
            a.value(0,0x43300000); a.store(1,0x74,4); a.value(4,0x66660000); a.floating_load(1,25,128);
            a.offset(4,4,0x6667); a.store(1,0x70,0); a.offset(0,3,0x5); a.multiply_high(0,4,0);
            a.floating_load(2,25,952); a.floating_load(0,1,112); a.floating_subtract(0,0,1);
            a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true); a.mask(3,0,0x1,false,1); a.add(0,0,3);
            a.add(26,26,0); a.floating_integer(0,0); a.floating_store(0,1,120); a.load(3,1,0x7c);
            a.offset(0,3,-0x1); a.compare_registers(26,0);
            if (a.positive()) {
                a.at(0x10286ea8); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x84,3);
                a.floating_load(1,25,128); a.store(1,0x80,0); a.floating_load(2,25,952); a.floating_load(0,1,128);
                a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true); a.floating_integer(0,0);
                a.floating_store(0,1,136); a.load(3,1,0x8c); a.offset(26,3,-0x1); a.jump(0x10286f70);
            }
            a.at(0x10286f70);
        } else {
            a.at(0x10286cfc); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
            a.value(3,0xffff0000); a.floating_load(1,25,128); a.offset(3,3,0x1); a.store(1,0x38,0);
            a.floating_load(2,25,952); a.floating_load(0,1,56); a.floating_subtract(0,0,1);
            a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,64); a.load(0,1,0x44);
            a.compare_registers(0,3);
            if (a.at_least()) {
                a.at(0x10286ee0); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.load(5,29,0x0); a.store(1,0x38,4);
                a.floating_load(1,25,128); a.floating_load(0,1,56); a.floating_load(2,25,952);
                a.floating_subtract(0,0,1); a.store(1,0x4c,3); a.floating_load(1,25,128); a.store(1,0x48,0);
                a.floating_difference(3,2,0,true); a.floating_load(2,25,952); a.floating_load(0,1,72);
                a.load<2>(3,5,0x0); a.load(0,26,0x0); a.add(4,3,0); a.floating_subtract(0,0,1);
                a.floating_integer(1,3); a.floating_difference(0,2,0,true); a.floating_store(1,1,64);
                a.load(3,1,0x44); a.divide(0,4,3); a.product(0,0,3); a.subtract(26,0,4); a.floating_integer(0,0);
                a.negate(3,26); a.floating_store(0,1,80); a.load(0,1,0x54); a.compare(0,0);
                if (a.positive()) {
                    a.at(0x10286f68); a.move(3,26);
                }
                a.at(0x10286f6c); a.move(26,3);
            } else {
                a.at(0x10286d3c); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000);
                a.store(1,0x38,4); a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000);
                a.floating_load(0,1,56); a.value(7,0x10000); a.floating_load(2,25,952); a.value(4,0x80010000);
                a.floating_subtract(0,0,1); a.store(1,0x64,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff);
                a.store(1,0x60,0); a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,96);
                a.floating_load(2,25,952); a.offset(4,4,-0x7fff); a.store(1,0x48,6); a.load(9,29,0x0);
                a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
                a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,64);
                a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x44); a.add(6,6,3);
                a.store(1,0x50,5); a.exclusive_flags(0,0,0x80000000); a.store(1,0x4c,0); a.multiply_high(0,8,6);
                a.floating_integer(0,0); a.floating_load(1,25,128); a.floating_load(2,1,72);
                a.floating_subtract(2,2,4); a.floating_store(0,1,104); a.add(3,0,6); a.load(0,1,0x6c);
                a.shift_right(3,3,0xf); a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5);
                a.product(3,3,7); a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x54,3);
                a.multiply(0,0,0xa); a.floating_load(0,1,80); a.multiply_high(3,4,0); a.floating_subtract(0,0,1);
                a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
                a.floating_integer(0,0); a.add(3,0,3); a.floating_store(0,1,88); a.load(26,1,0x5c);
                a.call(0x10226520); a.exclusive_flags(4,30,0x80000000); a.value(0,0x43300000); a.store(1,0x74,4);
                a.value(4,0x66660000); a.floating_load(1,25,128); a.offset(4,4,0x6667); a.store(1,0x70,0);
                a.offset(0,3,0x5); a.multiply_high(0,4,0); a.floating_load(2,25,952); a.floating_load(0,1,112);
                a.floating_subtract(0,0,1); a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true);
                a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(26,26,0); a.floating_integer(0,0);
                a.floating_store(0,1,120); a.load(3,1,0x7c); a.offset(0,3,-0x1); a.compare_registers(26,0);
                if (a.positive()) {
                    a.at(0x10286ea8); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x84,3);
                    a.floating_load(1,25,128); a.store(1,0x80,0); a.floating_load(2,25,952);
                    a.floating_load(0,1,128); a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true);
                    a.floating_integer(0,0); a.floating_store(0,1,136); a.load(3,1,0x8c); a.offset(26,3,-0x1);
                    a.jump(0x10286f70);
                }
                a.at(0x10286f70);
            }
            a.at(0x10286f70);
        }
        a.at(0x10286f70);
    } else {
        a.at(0x10286c8c); a.value(26,0x0); a.jump(0x10286f70);
    }
    a.at(0x10286f70); a.sign8(0,31); a.compare_registers(26,0);
    if (a.at_least()) {
        a.at(0x102875d4); a.value(3,0x0);
    } else {
        a.at(0x10286f7c); a.value(3,0x1); a.jump(0x102875d8);
    }
    a.at(0x102875d8);
}
void kind_4_probability(Call& a) {
    a.at(0x10286f84); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
    a.floating_load(2,25,128); a.store(1,0x38,0); a.floating_load(3,25,960); a.floating_load(1,1,56);
    a.floating_load(0,25,136); a.floating_subtract(1,1,2); a.floating_difference(1,3,1,true);
    a.floating_compare(1,0);
    if (!a.equal()) {
        a.at(0x10286fbc); a.load(3,29,0x0); a.load(0,28,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(29,0x0,3);
        if (a.positive()) {
            a.at(0x10286fd4); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(26,0x0,0);
        }
        a.at(0x10286fe4); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
        a.value(3,0x10000); a.floating_load(1,25,128); a.offset(3,3,-0x1); a.store(1,0x38,0);
        a.floating_load(2,25,960); a.floating_load(0,1,56); a.floating_subtract(0,0,1);
        a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,64); a.load(0,1,0x44);
        a.compare_registers(0,3);
        if (a.positive()) {
            a.at(0x10287064); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
            a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000); a.store(1,0x38,4);
            a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000); a.floating_load(0,1,56);
            a.value(7,0x10000); a.floating_load(2,25,960); a.value(4,0x80010000); a.floating_subtract(0,0,1);
            a.store(1,0x64,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff); a.store(1,0x60,0);
            a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,96);
            a.floating_load(2,25,960); a.offset(4,4,-0x7fff); a.store(1,0x48,6); a.load(9,29,0x0);
            a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
            a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,64);
            a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x44); a.add(6,6,3); a.store(1,0x50,5);
            a.exclusive_flags(0,0,0x80000000); a.store(1,0x4c,0); a.multiply_high(0,8,6); a.floating_integer(0,0);
            a.floating_load(1,25,128); a.floating_load(2,1,72); a.floating_subtract(2,2,4);
            a.floating_store(0,1,104); a.add(3,0,6); a.load(0,1,0x6c); a.shift_right(3,3,0xf);
            a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5); a.product(3,3,7);
            a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x54,3); a.multiply(0,0,0xa);
            a.floating_load(0,1,80); a.multiply_high(3,4,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
            a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
            a.floating_store(0,1,88); a.load(26,1,0x5c); a.call(0x10226520); a.exclusive_flags(4,30,0x80000000);
            a.value(0,0x43300000); a.store(1,0x74,4); a.value(4,0x66660000); a.floating_load(1,25,128);
            a.offset(4,4,0x6667); a.store(1,0x70,0); a.offset(0,3,0x5); a.multiply_high(0,4,0);
            a.floating_load(2,25,960); a.floating_load(0,1,112); a.floating_subtract(0,0,1);
            a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true); a.mask(3,0,0x1,false,1); a.add(0,0,3);
            a.add(26,26,0); a.floating_integer(0,0); a.floating_store(0,1,120); a.load(3,1,0x7c);
            a.offset(0,3,-0x1); a.compare_registers(26,0);
            if (a.positive()) {
                a.at(0x102871d0); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x84,3);
                a.floating_load(1,25,128); a.store(1,0x80,0); a.floating_load(2,25,960); a.floating_load(0,1,128);
                a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true); a.floating_integer(0,0);
                a.floating_store(0,1,136); a.load(3,1,0x8c); a.offset(26,3,-0x1); a.jump(0x10287298);
            }
            a.at(0x10287298);
        } else {
            a.at(0x10287024); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
            a.value(3,0xffff0000); a.floating_load(1,25,128); a.offset(3,3,0x1); a.store(1,0x38,0);
            a.floating_load(2,25,960); a.floating_load(0,1,56); a.floating_subtract(0,0,1);
            a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,64); a.load(0,1,0x44);
            a.compare_registers(0,3);
            if (a.at_least()) {
                a.at(0x10287208); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.load(5,29,0x0); a.store(1,0x38,4);
                a.floating_load(1,25,128); a.floating_load(0,1,56); a.floating_load(2,25,960);
                a.floating_subtract(0,0,1); a.store(1,0x4c,3); a.floating_load(1,25,128); a.store(1,0x48,0);
                a.floating_difference(3,2,0,true); a.floating_load(2,25,960); a.floating_load(0,1,72);
                a.load<2>(3,5,0x0); a.load(0,26,0x0); a.add(4,3,0); a.floating_subtract(0,0,1);
                a.floating_integer(1,3); a.floating_difference(0,2,0,true); a.floating_store(1,1,64);
                a.load(3,1,0x44); a.divide(0,4,3); a.product(0,0,3); a.subtract(26,0,4); a.floating_integer(0,0);
                a.negate(3,26); a.floating_store(0,1,80); a.load(0,1,0x54); a.compare(0,0);
                if (a.positive()) {
                    a.at(0x10287290); a.move(3,26);
                }
                a.at(0x10287294); a.move(26,3);
            } else {
                a.at(0x10287064); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000);
                a.store(1,0x38,4); a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000);
                a.floating_load(0,1,56); a.value(7,0x10000); a.floating_load(2,25,960); a.value(4,0x80010000);
                a.floating_subtract(0,0,1); a.store(1,0x64,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff);
                a.store(1,0x60,0); a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,96);
                a.floating_load(2,25,960); a.offset(4,4,-0x7fff); a.store(1,0x48,6); a.load(9,29,0x0);
                a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
                a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,64);
                a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x44); a.add(6,6,3);
                a.store(1,0x50,5); a.exclusive_flags(0,0,0x80000000); a.store(1,0x4c,0); a.multiply_high(0,8,6);
                a.floating_integer(0,0); a.floating_load(1,25,128); a.floating_load(2,1,72);
                a.floating_subtract(2,2,4); a.floating_store(0,1,104); a.add(3,0,6); a.load(0,1,0x6c);
                a.shift_right(3,3,0xf); a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5);
                a.product(3,3,7); a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x54,3);
                a.multiply(0,0,0xa); a.floating_load(0,1,80); a.multiply_high(3,4,0); a.floating_subtract(0,0,1);
                a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
                a.floating_integer(0,0); a.add(3,0,3); a.floating_store(0,1,88); a.load(26,1,0x5c);
                a.call(0x10226520); a.exclusive_flags(4,30,0x80000000); a.value(0,0x43300000); a.store(1,0x74,4);
                a.value(4,0x66660000); a.floating_load(1,25,128); a.offset(4,4,0x6667); a.store(1,0x70,0);
                a.offset(0,3,0x5); a.multiply_high(0,4,0); a.floating_load(2,25,960); a.floating_load(0,1,112);
                a.floating_subtract(0,0,1); a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true);
                a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(26,26,0); a.floating_integer(0,0);
                a.floating_store(0,1,120); a.load(3,1,0x7c); a.offset(0,3,-0x1); a.compare_registers(26,0);
                if (a.positive()) {
                    a.at(0x102871d0); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x84,3);
                    a.floating_load(1,25,128); a.store(1,0x80,0); a.floating_load(2,25,960);
                    a.floating_load(0,1,128); a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true);
                    a.floating_integer(0,0); a.floating_store(0,1,136); a.load(3,1,0x8c); a.offset(26,3,-0x1);
                    a.jump(0x10287298);
                }
                a.at(0x10287298);
            }
            a.at(0x10287298);
        }
        a.at(0x10287298);
    } else {
        a.at(0x10286fb4); a.value(26,0x0); a.jump(0x10287298);
    }
    a.at(0x10287298); a.sign8(0,31); a.compare_registers(26,0);
    if (a.at_least()) {
        a.at(0x102875d4); a.value(3,0x0);
    } else {
        a.at(0x102872a4); a.value(3,0x1); a.jump(0x102875d8);
    }
    a.at(0x102875d8);
}
void kind_5_probability(Call& a) {
    a.at(0x102872ac); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
    a.floating_load(2,25,128); a.store(1,0x38,0); a.floating_load(3,25,968); a.floating_load(1,1,56);
    a.floating_load(0,25,136); a.floating_subtract(1,1,2); a.floating_difference(1,3,1,true);
    a.floating_compare(1,0);
    if (!a.equal()) {
        a.at(0x102872e4); a.load(3,29,0x0); a.load(0,28,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(29,0x0,3);
        if (a.positive()) {
            a.at(0x102872fc); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(26,0x0,0);
        }
        a.at(0x1028730c); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
        a.value(3,0x10000); a.floating_load(1,25,128); a.offset(3,3,-0x1); a.store(1,0x38,0);
        a.floating_load(2,25,968); a.floating_load(0,1,56); a.floating_subtract(0,0,1);
        a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,64); a.load(0,1,0x44);
        a.compare_registers(0,3);
        if (a.positive()) {
            a.at(0x1028738c); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
            a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000); a.store(1,0x38,4);
            a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000); a.floating_load(0,1,56);
            a.value(7,0x10000); a.floating_load(2,25,968); a.value(4,0x80010000); a.floating_subtract(0,0,1);
            a.store(1,0x64,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff); a.store(1,0x60,0);
            a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,96);
            a.floating_load(2,25,968); a.offset(4,4,-0x7fff); a.store(1,0x48,6); a.load(9,29,0x0);
            a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
            a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,64);
            a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x44); a.add(6,6,3); a.store(1,0x50,5);
            a.exclusive_flags(0,0,0x80000000); a.store(1,0x4c,0); a.multiply_high(0,8,6); a.floating_integer(0,0);
            a.floating_load(1,25,128); a.floating_load(2,1,72); a.floating_subtract(2,2,4);
            a.floating_store(0,1,104); a.add(3,0,6); a.load(0,1,0x6c); a.shift_right(3,3,0xf);
            a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5); a.product(3,3,7);
            a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x54,3); a.multiply(0,0,0xa);
            a.floating_load(0,1,80); a.multiply_high(3,4,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
            a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
            a.floating_store(0,1,88); a.load(26,1,0x5c); a.call(0x10226520); a.exclusive_flags(4,30,0x80000000);
            a.value(0,0x43300000); a.store(1,0x74,4); a.value(4,0x66660000); a.floating_load(1,25,128);
            a.offset(4,4,0x6667); a.store(1,0x70,0); a.offset(0,3,0x5); a.multiply_high(0,4,0);
            a.floating_load(2,25,968); a.floating_load(0,1,112); a.floating_subtract(0,0,1);
            a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true); a.mask(3,0,0x1,false,1); a.add(0,0,3);
            a.add(26,26,0); a.floating_integer(0,0); a.floating_store(0,1,120); a.load(3,1,0x7c);
            a.offset(0,3,-0x1); a.compare_registers(26,0);
            if (a.positive()) {
                a.at(0x102874f8); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x84,3);
                a.floating_load(1,25,128); a.store(1,0x80,0); a.floating_load(2,25,968); a.floating_load(0,1,128);
                a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true); a.floating_integer(0,0);
                a.floating_store(0,1,136); a.load(3,1,0x8c); a.offset(26,3,-0x1); a.jump(0x102875c0);
            }
            a.at(0x102875c0);
        } else {
            a.at(0x1028734c); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
            a.value(3,0xffff0000); a.floating_load(1,25,128); a.offset(3,3,0x1); a.store(1,0x38,0);
            a.floating_load(2,25,968); a.floating_load(0,1,56); a.floating_subtract(0,0,1);
            a.floating_difference(0,2,0,true); a.floating_integer(0,0); a.floating_store(0,1,64); a.load(0,1,0x44);
            a.compare_registers(0,3);
            if (a.at_least()) {
                a.at(0x10287530); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.load(5,29,0x0); a.store(1,0x38,4);
                a.floating_load(1,25,128); a.floating_load(0,1,56); a.floating_load(2,25,968);
                a.floating_subtract(0,0,1); a.store(1,0x4c,3); a.floating_load(1,25,128); a.store(1,0x48,0);
                a.floating_difference(3,2,0,true); a.floating_load(2,25,968); a.floating_load(0,1,72);
                a.load<2>(3,5,0x0); a.load(0,26,0x0); a.add(4,3,0); a.floating_subtract(0,0,1);
                a.floating_integer(1,3); a.floating_difference(0,2,0,true); a.floating_store(1,1,64);
                a.load(3,1,0x44); a.divide(0,4,3); a.product(0,0,3); a.subtract(26,0,4); a.floating_integer(0,0);
                a.negate(3,26); a.floating_store(0,1,80); a.load(0,1,0x54); a.compare(0,0);
                if (a.positive()) {
                    a.at(0x102875b8); a.move(3,26);
                }
                a.at(0x102875bc); a.move(26,3);
            } else {
                a.at(0x1028738c); a.exclusive_flags(0,30,0x80000000); a.value(4,0x43300000); a.store(1,0x3c,0);
                a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.value(6,0x43300000);
                a.store(1,0x38,4); a.value(8,0x80010000); a.floating_load(1,25,128); a.value(5,0x43300000);
                a.floating_load(0,1,56); a.value(7,0x10000); a.floating_load(2,25,968); a.value(4,0x80010000);
                a.floating_subtract(0,0,1); a.store(1,0x64,3); a.floating_load(1,25,128); a.offset(8,8,-0x7fff);
                a.store(1,0x60,0); a.offset(7,7,-0x1); a.floating_difference(3,2,0,true); a.floating_load(0,1,96);
                a.floating_load(2,25,968); a.offset(4,4,-0x7fff); a.store(1,0x48,6); a.load(9,29,0x0);
                a.floating_integer(5,3); a.floating_load(4,25,128); a.floating_subtract(0,0,1);
                a.floating_load(3,25,0); a.load<2>(6,9,0x0); a.floating_store(5,1,64);
                a.floating_difference(0,2,0,true); a.load(3,26,0x0); a.load(0,1,0x44); a.add(6,6,3);
                a.store(1,0x50,5); a.exclusive_flags(0,0,0x80000000); a.store(1,0x4c,0); a.multiply_high(0,8,6);
                a.floating_integer(0,0); a.floating_load(1,25,128); a.floating_load(2,1,72);
                a.floating_subtract(2,2,4); a.floating_store(0,1,104); a.add(3,0,6); a.load(0,1,0x6c);
                a.shift_right(3,3,0xf); a.floating_difference(2,2,3,true); a.mask(5,3,0x1,false,1); a.add(3,3,5);
                a.product(3,3,7); a.subtract(26,3,6); a.exclusive_flags(3,26,0x80000000); a.store(1,0x54,3);
                a.multiply(0,0,0xa); a.floating_load(0,1,80); a.multiply_high(3,4,0); a.floating_subtract(0,0,1);
                a.floating_product(0,2,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
                a.floating_integer(0,0); a.add(3,0,3); a.floating_store(0,1,88); a.load(26,1,0x5c);
                a.call(0x10226520); a.exclusive_flags(4,30,0x80000000); a.value(0,0x43300000); a.store(1,0x74,4);
                a.value(4,0x66660000); a.floating_load(1,25,128); a.offset(4,4,0x6667); a.store(1,0x70,0);
                a.offset(0,3,0x5); a.multiply_high(0,4,0); a.floating_load(2,25,968); a.floating_load(0,1,112);
                a.floating_subtract(0,0,1); a.shift_right(0,0,0x2); a.floating_difference(0,2,0,true);
                a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(26,26,0); a.floating_integer(0,0);
                a.floating_store(0,1,120); a.load(3,1,0x7c); a.offset(0,3,-0x1); a.compare_registers(26,0);
                if (a.positive()) {
                    a.at(0x102874f8); a.exclusive_flags(3,30,0x80000000); a.value(0,0x43300000); a.store(1,0x84,3);
                    a.floating_load(1,25,128); a.store(1,0x80,0); a.floating_load(2,25,968);
                    a.floating_load(0,1,128); a.floating_subtract(0,0,1); a.floating_difference(0,2,0,true);
                    a.floating_integer(0,0); a.floating_store(0,1,136); a.load(3,1,0x8c); a.offset(26,3,-0x1);
                    a.jump(0x102875c0);
                }
                a.at(0x102875c0);
            }
            a.at(0x102875c0);
        }
        a.at(0x102875c0);
    } else {
        a.at(0x102872dc); a.value(26,0x0); a.jump(0x102875c0);
    }
    a.at(0x102875c0); a.sign8(0,31); a.compare_registers(26,0);
    if (a.at_least()) {
        a.at(0x102875d4); a.value(3,0x0);
    } else {
        a.at(0x102875cc); a.value(3,0x1); a.jump(0x102875d8);
    }
    a.at(0x102875d8);
}
}
void accept_response(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10286650,dependencies,observer);
    a.at(0x10286650); a.link(); a.floating_store(31,1,-8); a.compare(5,0,false); a.multiple(24,false,-16);
    a.move(24,3); a.load(25,2,-0x51e8); a.move(30,4); a.load(26,2,-0x7b90); a.value(3,0x0); a.load(27,2,-0x7b8c);
    a.load(28,2,-0x7b88); a.load(29,2,-0x7b84); a.save_link(); a.frame(192);
    if (!a.equal()) {
        a.at(0x1028668c); a.load(0,24,0x1ac); a.compare_registers(5,0,false);
        if (a.equal()) {
            a.at(0x10286698); a.value(3,0x1);
        }
        a.at(0x1028669c);
    }
    a.at(0x1028669c); a.compare(3,0); a.value(0,0x1);
    if (!a.equal()) {
        a.at(0x102866a8); a.value(0,0x8);
    }
    a.at(0x102866ac); a.floating_load(1,24,424,true); a.sign8(31,0); a.floating_load(2,25,200); a.call(0x10780fc0);
    a.load(2,1,0x14); a.floating_integer(0,1); a.sign8(0,30); a.compare(0,3); a.floating_store(0,1,136);
    a.load(30,1,0x8c);
    if (a.equal()) {
        a.at(0x10286c5c);
        kind_3_probability(a);
        a.at(0x102875d8);
    } else {
        a.at(0x102866d8);
        if (a.at_least()) {
            a.at(0x102866ec); a.compare(0,5);
            if (a.equal()) {
                a.at(0x102872ac);
                kind_5_probability(a);
                a.at(0x102875d8);
            } else {
                a.at(0x102866f4);
                if (a.at_least()) {
                    a.at(0x102875d4); a.value(3,0x0);
                } else {
                    a.at(0x102866f8); a.jump(0x10286f84);
                    kind_4_probability(a);
                    a.at(0x102875d8);
                }
                a.at(0x102875d8);
            }
            a.at(0x102875d8);
        } else {
            a.at(0x102866dc); a.compare(0,1);
            if (a.equal()) {
                a.at(0x102866fc);
                kind_1_probability(a);
                a.at(0x102875d8);
            } else {
                a.at(0x102866e4);
                if (a.at_least()) {
                    a.at(0x10286934);
                    kind_2_probability(a);
                    a.at(0x102875d8);
                } else {
                    a.at(0x102866e8); a.jump(0x102875d4); a.value(3,0x0);
                }
                a.at(0x102875d8);
            }
            a.at(0x102875d8);
        }
        a.at(0x102875d8);
    }
    a.at(0x102875d8); a.load(0,1,0xc8); a.offset(1,1,0xc0); a.floating_load(31,1,-8); a.restore_link();
    a.multiple(24,true,-16); a.finish();
}
}
