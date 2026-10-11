#pragma once
#include "file_controller_internal.hpp"
#include "cpp_object_storage_adapter.hpp"

namespace cm::aot::cpp_object_storage {
using Abi = file_controller::Abi;
inline constexpr auto entries = cpp_object_storage_entries;
void construct_array(Abi&);
void cleanup_partial_array(Abi&);
void construct_range(Abi&);
void destroy_range(Abi&);
void destroy_array(Abi&);
void allocate_object(Abi&);
void release_object(Abi&);
void allocation_facade(Abi&, bool);
void allocation_failure(Abi&);
void destroy_failure(Abi&);
void failure_type(Abi&);
}
