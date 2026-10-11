#pragma once
#include "file_controller_internal.hpp"
#include "file_path_adapter.hpp"

namespace cm::aot::file_path {
using Abi = file_controller::Abi;
inline constexpr auto entries = file_path_entries;
void copy_pascal(Abi&);
void classic_spec(Abi&);
void compatible_spec(Abi&);
void script_encoding(Abi&);
void reference_from_path(Abi&);
void unicode_name(Abi&);
void parent_and_leaf(Abi&);
}
