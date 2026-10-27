#include "screen_manager_detail.hpp"
#include "screen_manager_sound.hpp"

namespace cm::aot::screen_manager {
using namespace detail;
std::uint32_t sound_construct(Cpu& c,std::uint32_t address) {
    c.set(4,0);c.put8(address+0x28,0);
    for(unsigned i=0;i<10;++i)c.put32(address+i*4,0);
    c.put8(address+0x28,1);return address;
}
std::uint32_t sound_destroy(Cpu& c,std::uint32_t address,std::uint32_t disposal) {
    Frame frame(c,0x107446b0,0x40,{.first_gpr=30});
    Kept<31> mode(c,disposal);Kept<30> object(c,address);
    if(compare(c,object)!=0) {
        enter(c,0x107446d4);result(c,sound_clear(c,object));
        if(compare(c,std::int16_t(std::uint32_t(mode)))>0)Program{c}.call(0x107735a0,0x107446e4,{object});
    }
    return object;
}
std::uint32_t sound_clear(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x10744700,0x40,{.first_gpr=30});Kept<30> object(c,address);
    if(byte_nonzero(c,c.u8(address+0x28))) {
        Kept<31> index(c,0);
        while(compare(c,std::int16_t(std::uint32_t(index)),10)<0) {
            enter(c,0x1074473c,{object,std::uint32_t(std::int8_t(std::uint32_t(index)))});
            result(c,sound_stop(c,object,c.r(4)));index=index+1;
        }
    }
    return c.r(3);
}
std::uint32_t sound_stop(Cpu& c,std::uint32_t address,std::uint32_t index) {
    Frame frame(c,0x10744770,0x50,{.first_gpr=28});Kept<28> object(c,address);Program p{c};
    if(byte_nonzero(c,c.u8(address+0x28))) {
        Kept<31> offset(c,std::uint32_t(std::int8_t(index))*4);
        Kept<30> channel(c,c.u32(object+offset));
        if(compare_logical(c,channel)!=0) {
            Kept<29> data(c,c.u32(channel+12));
            c.set(4,frame.at(0x38));c.put16(frame.at(0x38),3);c.put32(frame.at(0x3c),0);c.put16(frame.at(0x3a),0);
            p.imported(0x10780948,0x107447d4,{channel,c.r(4)});
            c.set(3,c.u32(data+4));if(compare_logical(c,c.r(3))!=0)p.imported(0x1077fc58,0x107447e8);
            c.set(3,c.u32(data));if(compare_logical(c,c.r(3))!=0)p.imported(0x1077fc58,0x107447fc);
            p.imported(0x10780a50,0x10744808,{extended(c.i16(data+12))});
            c.set(3,c.u32(channel+12));if(compare(c,c.r(3))!=0)p.imported(0x1077fc58,0x1074481c);
            p.imported(0x10780a38,0x1074482c,{channel,1});c.put32(object+offset,0);
        }
    }
    return c.r(3);
}
std::uint32_t sound_load(Cpu& c,std::uint32_t address,std::uint32_t side,std::uint32_t filename,std::uint32_t repeat) {
    Frame frame(c,0x10744860,0x2b0,{.first_gpr=25});
    Kept<27> object(c,address);Kept<28> stereo(c,side);Kept<25> source(c,filename);Kept<29> looping(c,repeat);
    Kept<31> index(c,missing);Kept<30> data(c,0);Program p{c};
    c.put32(frame.at(0x3c),0);c.put16(frame.at(0x38),0);
    auto attempt=[&]() -> bool {
        if(!byte_nonzero(c,c.u8(address+0x28)) || compare_logical(c,source)==0)return false;
        c.set(4,0);
        while(compare(c,std::int16_t(c.r(4)),10)<0) {
            if(compare_logical(c,c.u32(c.r(3)))==0){index=c.r(4);break;}
            c.set(3,c.r(3)+4);c.set(4,c.r(4)+1);
        }
        if(compare(c,std::int16_t(std::uint32_t(index)),-1)==0)return false;
        enter(c,0x107448f8,{object,100,stereo});Kept<26> volume(c,sound_volume(c,object,100,stereo));result(c,volume);
        p.call(0x10746470,0x1074490c,{source,frame.at(0x90),c.toc(-0x3e54)});
        p.call(0x1074c5d0,0x10744918,{frame.at(0x190),frame.at(0x90)});
        p.imported(0x1077fee0,0x1074492c,{0,0,frame.at(0x190),frame.at(0x48)});
        if(compare(c,std::int16_t(c.r(3)))!=0)return false;
        p.imported(0x10780918,0x10744948,{frame.at(0x48),1,frame.at(0x38)});
        if(compare(c,std::int16_t(c.r(3)))!=0)return false;
        p.imported(0x10780978,0x1074495c,{c.toc(-0x3e58)});
        p.imported(0x10780990,0x10744978,{frame.at(0x3c),5,0x85,c.r(3)});
        if(compare(c,std::int16_t(c.r(3)))!=0)return false;
        c.put16(frame.at(0x40),0x2e);c.put16(frame.at(0x42),0);c.put32(frame.at(0x44),volume);
        p.imported(0x10780948,0x107449a4,{c.u32(frame.at(0x3c)),frame.at(0x40)});
        if(compare(c,std::int16_t(c.r(3)))!=0)return false;
        data=p.imported(0x1077faf0,0x107449b8,{16});
        if(compare(c,data)==0)return false;
        c.set(4,c.u32(frame.at(0x3c)));c.put32(c.r(4)+12,data);
        c.put8(data+8,looping);c.put8(data+10,stereo);c.put16(data+12,c.u16(frame.at(0x38)));
        enter(c,0x107449e8,{object,c.u32(frame.at(0x3c))});result(c,sound_read(c,object,c.r(4)));
        if(compare(c,std::int16_t(c.r(3)))!=0)return false;
        c.set(4,c.u32(frame.at(0x3c)));c.put32(object+std::uint32_t(std::int16_t(std::uint32_t(index)))*4,c.r(4));return true;
    };
    if(attempt())return std::uint32_t(std::int8_t(std::uint32_t(index)));
    c.set(3,c.u32(frame.at(0x3c)));if(compare_logical(c,c.r(3))!=0)p.imported(0x10780a38,0x10744a1c,{c.r(3),1});
    if(compare_logical(c,data)!=0)p.imported(0x1077fc58,0x10744a30,{data});
    c.set(3,extended(c.i16(frame.at(0x38))));if(compare(c,std::int16_t(c.r(3)))!=0)p.imported(0x10780a50,0x10744a44);
    return missing;
}
std::uint32_t sound_read(Cpu& c,std::uint32_t,std::uint32_t channel_address) {
    Frame frame(c,0x10744a60,0x80,{.first_gpr=28});Kept<30> channel(c,channel_address);
    Kept<31> data(c,c.u32(channel_address+12));Program p{c};
    auto attempt=[&]() -> bool {
        p.imported(0x1077fc70,0x10744a8c,{0x42});c.put32(data,c.r(3));
        if(compare_logical(c,c.u32(data))==0)return false;
        p.call(0x1074eec0,0x10744aac,{extended(c.i16(data+12)),frame.at(0x48)});
        if(compare(c,std::int16_t(c.r(3)))!=0)return false;
        const auto sample_bytes=std::int32_t(c.i16(frame.at(0x56)))/8;
        Kept<28> size(c,c.u32(frame.at(0x52))*std::uint32_t(sample_bytes)*std::uint32_t(c.i16(frame.at(0x50))));
        p.imported(0x1077fc70,0x10744ad8,{size});c.put32(data+4,c.r(3));c.set(4,c.u32(data+4));
        if(compare_logical(c,c.r(4))==0)return false;
        Kept<29> status(c,p.call(0x1074eff0,0x10744af8,{extended(c.i16(data+12)),c.r(4),size}));
        if(compare(c,std::int16_t(std::uint32_t(status)))!=0)return false;
        c.set(4,c.u32(data));c.put32(c.r(4),c.u32(data+4));
        c.set(4,c.u32(data));c.put32(c.r(4)+4,extended(c.i16(frame.at(0x50))));
        p.imported(0x107809f0,0x10744b24,{frame.at(0x58)});p.imported(0x10780a08,0x10744b2c);
        c.set(4,c.u32(data));c.put32(c.r(4)+8,c.r(3));
        c.set({{8,0},{7,size-1},{6,0xff},{5,0x3c},{3,0x51},{10,c.u32(data)}});
        c.set(4,extended(c.i16(frame.at(0x58))));c.put16(c.r(10)+0x1a,c.r(4));
        c.set({{9,c.u32(frame.at(0x5a))},{4,c.u32(frame.at(0x5e))}});
        c.put32(c.r(10)+0x1c,c.r(9));c.put32(c.r(10)+0x20,c.r(4));
        c.set(4,c.u32(data));c.put32(c.r(4)+12,0);c.set(4,c.u32(data));c.put32(c.r(4)+16,c.r(7));
        c.set(4,c.u32(data));c.put8(c.r(4)+20,0xff);c.set(4,c.u32(data));c.put8(c.r(4)+21,c.r(5));
        c.set(5,c.u32(frame.at(0x52)));c.set(4,c.u32(data));c.put32(c.r(4)+22,c.r(5));
        c.set(5,extended(c.i16(frame.at(0x56))));c.set(4,c.u32(data));c.put16(c.r(4)+0x30,c.r(5));
        c.put16(frame.at(0x40),0x51);c.put16(frame.at(0x42),0);c.put32(frame.at(0x44),c.u32(data));
        c.put16(frame.at(0x38),13);c.put16(frame.at(0x3a),0);
        p.imported(0x10780a20,0x10744bc0,{c.u32(data)});c.put32(frame.at(0x3c),c.r(3));
        status=p.imported(0x10780948,0x10744bd4,{channel,frame.at(0x40)});
        if(compare(c,std::int16_t(std::uint32_t(status)))==0)status=p.imported(0x10780960,0x10744bf4,{channel,frame.at(0x38),1});
        c.set(3,status);return true;
    };
    if(attempt())return c.r(3);
    c.set(3,c.u32(data));if(compare_logical(c,c.r(3))!=0)p.imported(0x1077fc58,0x10744c14);
    c.set(3,c.u32(data+4));if(compare_logical(c,c.r(3))!=0)p.imported(0x1077fc58,0x10744c28);
    return c.r(29);
}
std::uint32_t sound_finished(Cpu& c,std::uint32_t address,std::uint32_t index) {
    const auto signed_index=std::int8_t(index);
    if(compare(c,signed_index)<0 || compare(c,signed_index,10)>=0)return 0;
    c.set(3,c.u32(address+std::uint32_t(signed_index)*4));
    if(compare_logical(c,c.r(3))==0)return 0;
    c.set(3,c.u32(c.r(3)+12));return c.i8(c.r(3)+9)==0;
}
std::uint32_t sound_set_volume(Cpu& c,std::uint32_t address,std::uint32_t index,std::uint32_t volume) {
    Frame frame(c,0x10744ca0,0x50,{.first_gpr=31});Program p{c};
    if(byte_nonzero(c,c.u8(address+0x28)) && compare(c,std::int8_t(index))>=0 && compare(c,std::int8_t(index),10)<0 &&
       compare(c,volume)>=0 && compare(c,volume,100)<=0) {
        Kept<31> channel(c,c.u32(address+std::uint32_t(std::int8_t(index))*4));
        if(compare_logical(c,channel)!=0) {
            c.set(6,c.u32(channel+12));enter(c,0x10744d08,{address,volume,c.u8(c.r(6)+10)});
            result(c,sound_volume(c,address,volume,c.r(5)));c.put32(frame.at(0x3c),c.r(3));
            c.set(5,0x2e);c.put16(frame.at(0x38),0x2e);c.put16(frame.at(0x3a),0);
            p.imported(0x10780948,0x10744d28,{channel,frame.at(0x38)});
        }
    }
    return c.r(3);
}
std::uint32_t sound_pending(Cpu& c,std::uint32_t address) {
    c.set({{4,address},{7,0},{6,0}});
    for(unsigned index=0;index<10;++index) {
        if(compare_logical(c,c.u32(address+index*4))!=0) {
            compare(c,std::int8_t(index));compare(c,std::int8_t(index),10);
            c.set(5,c.u32(address+index*4));compare_logical(c,c.r(5));c.set(5,c.u32(c.r(5)+12));
            const bool finished=!byte_nonzero(c,c.u8(c.r(5)+9));
            if(compare(c,std::int8_t(finished))==0)return index;
        }
        c.set(7,index+1);
        if(index%2) {c.set(6,(index+1)*2);c.set(4,address+(index+1)*4);}
    }
    return missing;
}
std::uint32_t sound_volume(Cpu& c,std::uint32_t,std::uint32_t percent,std::uint32_t side) {
    c.set(3,c.toc(-0x5b98));const bool wide=byte_nonzero(c,c.u8(c.r(3)));
    const auto product=std::int64_t(std::int32_t(percent<<8))*(wide?0x1b4e81b5LL:0x51eb851fLL);
    const auto high=std::int32_t(product>>32);
    const auto shifted=high>>(wide?4:5);
    const auto value=std::uint32_t(shifted)+std::uint32_t(shifted<0);
    const auto channel=std::int8_t(side);
    if(compare(c,channel,1)==0)return value<<16;
    if(channel<1 && compare(c,channel)==0)return value;
    return value | (value<<16);
}
std::uint32_t sound_callback(Cpu& c,std::uint32_t channel_address,std::uint32_t command_address) {
    Frame frame(c,0x10744ee0,0x60,{.first_gpr=28});Kept<29> command(c,command_address);Kept<28> channel(c,channel_address);
    Kept<31> data(c,c.u32(channel+12));Program p{c};
    if(compare_logical(c,data)!=0) {
        if(byte_nonzero(c,c.u8(data+8))) {
            p.imported(0x10780930,0x10744f24,{c.u32(command+4)});
            c.set({{6,0},{4,c.r(3)}});Kept<30> previous(c,c.r(4));
            c.put16(frame.at(0x40),0x51);c.put16(frame.at(0x42),0);c.set(5,c.u32(data));c.put32(frame.at(0x44),c.r(5));
            c.put16(frame.at(0x38),13);c.put16(frame.at(0x3a),0);c.put32(frame.at(0x3c),c.u32(command+4));
            p.imported(0x10780948,0x10744f68,{channel,frame.at(0x40)});
            if(compare(c,std::int16_t(c.r(3)))==0)p.imported(0x10780960,0x10744f84,{channel,frame.at(0x38),1});
            p.imported(0x10780930,0x10744f90,{previous});
        } else c.put8(data+9,1);
    }
    return c.r(3);
}
}
