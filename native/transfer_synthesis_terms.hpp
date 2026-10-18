#pragma once
#include "transfer_synthesis_detail.hpp"

namespace cm::aot::transfer_synthesis::detail {
// These are complete repeated arithmetic blocks. The caller retains the
// original random-pool advances, including advances interleaved with a draw.
void choose_ordinary_period(Call&,std::uint32_t pc,bool thirds,unsigned increment);
void calculate_ordinary_amount(Call&,std::uint32_t pc,unsigned increment,bool alternate_registers);
void choose_percentage_pair(Call&,std::uint32_t pc,bool fifths,bool compare_delta,bool adjust_ordinary,std::uint32_t join);
void sample_payment_range(Call&,std::uint32_t pc);
void sample_source_component(Call&);
}
