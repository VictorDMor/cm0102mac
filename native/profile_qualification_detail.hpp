#pragma once
#include "profile_qualification.hpp"
#include "semantic_guest_call.hpp"
namespace cm::aot::profile_qualification::detail {
using Call=SemanticGuestCall;
void scan_primary_qualifications(Call&);
void scan_secondary_qualifications(Call&);
void report_missing_profile(Call&);
void category_four_reference(Call&);
}
