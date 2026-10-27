#pragma once
#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager::news {
using namespace detail;
// An inline primary lookup, with the original R3 result and scratch image.
// A direct manager deliberately bypasses the global holder and cache.
inline std::uint32_t primary(Cpu& c,std::uint32_t person,std::uint32_t holder,
    std::uint32_t index_holder,std::uint32_t count_holder,std::uint32_t cache,
    bool already_tested=false,std::uint32_t direct_manager=0) {
    if(!already_tested)compare_logical(c,person);
    auto manager=direct_manager;
    if(holder){manager=c.u32(holder);c.set(4,manager);}
    std::uint32_t row=0;
    if(person){const auto id=c.u32(person);c.set(3,id);const auto count=c.u32(count_holder);c.set(0,count);
        if(compare(c,std::int32_t(id),std::int32_t(count))<0){
            const auto fresh_id=c.u32(person);c.set(0,fresh_id);const auto indices=c.u32(index_holder);c.set(3,indices);
            const auto offset=fresh_id*100;c.set(0,offset);const auto index=c.u32(indices+offset);c.set(0,index);
            if(compare(c,std::int32_t(index))>=0){const auto displacement=index*100;c.set(0,displacement);const auto rows=c.u32(manager);c.set(3,rows);row=rows+displacement;}
        }
    }
    c.set(3,row);
    if(cache){compare_logical(c,row);c.put32(cache,row);}
    return row;
}
}
