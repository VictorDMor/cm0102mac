#pragma once
#include <string_view>

namespace cm::aot {
struct State;
// Run the native bridge for an import by name. Names are compared as views:
// imports such as pow are called tens of thousands of times a second.
// bridge_import(State&, const std::string&) in aot_runtime.hpp wraps this;
// that header is included by every generated page, so it keeps its form.
bool bridge_import_name(State& state, std::string_view name);
}
