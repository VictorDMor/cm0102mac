#include "staff_contracts.hpp"
#include "guest_range_sampler.hpp"

namespace cm::aot::staff_contracts {
std::uint32_t sample_range(guest::Cpu& c, std::int32_t bound) {
    return guest::RandomRangeSampler<0x10374570, -0x4dac, 0xc0>::sample(c, bound);
}
}
