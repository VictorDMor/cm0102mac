#include "engine_initialization_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "tree_mutation_adapter.hpp"
#include "aggregate_object_adapter.hpp"
#include "record_group_adapter.hpp"

namespace cm::aot {
// Owned process-local metadata. Extending this private definition does not
// alter State or the layout seen by the 1,922 generated translation units.
struct RuntimeDiagnostics {
    EngineInitializationDiagnostics engine_initialization;
    UtilityObjectDiagnostics utility_object;
    TreeMutationDiagnostics tree_mutation;
    AggregateObjectDiagnostics aggregate_object;
    RecordGroupDiagnostics record_group;
};
State::State() = default;
State::~State() = default;
EngineInitializationDiagnostics& engine_initialization_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->engine_initialization;
}
const EngineInitializationDiagnostics& engine_initialization_diagnostics(const State& state) {
    static const EngineInitializationDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->engine_initialization : empty;
}
UtilityObjectDiagnostics& utility_object_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->utility_object;
}
const UtilityObjectDiagnostics& utility_object_diagnostics(const State& state) {
    static const UtilityObjectDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->utility_object : empty;
}
TreeMutationDiagnostics& tree_mutation_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->tree_mutation;
}
const TreeMutationDiagnostics& tree_mutation_diagnostics(const State& state) {
    static const TreeMutationDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->tree_mutation : empty;
}

AggregateObjectDiagnostics& aggregate_object_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->aggregate_object;
}
const AggregateObjectDiagnostics& aggregate_object_diagnostics(const State& state) {
    static const AggregateObjectDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->aggregate_object : empty;
}

RecordGroupDiagnostics& record_group_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->record_group;
}
const RecordGroupDiagnostics& record_group_diagnostics(const State& state) {
    static const RecordGroupDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->record_group : empty;
}

}
