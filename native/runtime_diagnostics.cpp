#include "engine_initialization_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "tree_mutation_adapter.hpp"
#include "aggregate_object_adapter.hpp"
#include "record_group_adapter.hpp"
#include "record_query_adapter.hpp"
#include "record_history_adapter.hpp"
#include "match_engine_adapter.hpp"
#include "match_actor_adapter.hpp"
#include "actor_construction_adapter.hpp"
#include "spatial_role_adapter.hpp"
#include "marking_pressure_adapter.hpp"
#include "player_decision_adapter.hpp"
#include "actor_motion_adapter.hpp"
#include "player_movement_adapter.hpp"
#include "ball_action_adapter.hpp"
#include "ball_contact_adapter.hpp"
#include "match_discipline_adapter.hpp"
#include "match_random_adapter.hpp"
#include "actor_attributes_adapter.hpp"
#include "team_spatial_adapter.hpp"
#include "pitch_distance_adapter.hpp"
#include "actor_order_adapter.hpp"
#include "movement_choice_adapter.hpp"
#include "team_line_adapter.hpp"

namespace cm::aot {
// Owned process-local metadata. Extending this private definition does not
// alter State or the layout seen by the 1,922 generated translation units.
struct RuntimeDiagnostics {
    EngineInitializationDiagnostics engine_initialization;
    UtilityObjectDiagnostics utility_object;
    TreeMutationDiagnostics tree_mutation;
    AggregateObjectDiagnostics aggregate_object;
    RecordGroupDiagnostics record_group;
    RecordQueryDiagnostics record_query;
    RecordHistoryDiagnostics record_history;
    MatchEngineDiagnostics match_engine;
    MatchActorDiagnostics match_actor;
    ActorConstructionDiagnostics actor_construction;
    SpatialRoleDiagnostics spatial_role;
    MarkingPressureDiagnostics marking_pressure;
    PlayerDecisionDiagnostics player_decision;
    ActorMotionDiagnostics actor_motion;
    PlayerMovementDiagnostics player_movement;
    BallActionDiagnostics ball_action;
    BallContactDiagnostics ball_contact;
    MatchDisciplineDiagnostics match_discipline;
    MatchRandomDiagnostics match_random;
    ActorAttributesDiagnostics actor_attributes;
    TeamSpatialDiagnostics team_spatial;
    PitchDistanceDiagnostics pitch_distance;
    ActorOrderDiagnostics actor_order;
    MovementChoiceDiagnostics movement_choice;
    TeamLineDiagnostics team_line;
};
TeamLineDiagnostics& team_line_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->team_line;
}
const TeamLineDiagnostics& team_line_diagnostics(const State& state) {
    static const TeamLineDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->team_line : empty;
}
MovementChoiceDiagnostics& movement_choice_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->movement_choice;
}
const MovementChoiceDiagnostics& movement_choice_diagnostics(const State& state) {
    static const MovementChoiceDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->movement_choice : empty;
}
ActorOrderDiagnostics& actor_order_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->actor_order;
}
const ActorOrderDiagnostics& actor_order_diagnostics(const State& state) {
    static const ActorOrderDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->actor_order : empty;
}
PitchDistanceDiagnostics& pitch_distance_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->pitch_distance;
}
const PitchDistanceDiagnostics& pitch_distance_diagnostics(const State& state) {
    static const PitchDistanceDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->pitch_distance : empty;
}
TeamSpatialDiagnostics& team_spatial_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->team_spatial;
}
const TeamSpatialDiagnostics& team_spatial_diagnostics(const State& state) {
    static const TeamSpatialDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->team_spatial : empty;
}
ActorAttributesDiagnostics& actor_attributes_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->actor_attributes;
}
const ActorAttributesDiagnostics& actor_attributes_diagnostics(const State& state) {
    static const ActorAttributesDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->actor_attributes : empty;
}
MatchRandomDiagnostics& match_random_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->match_random;
}
const MatchRandomDiagnostics& match_random_diagnostics(const State& state) {
    static const MatchRandomDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->match_random : empty;
}
MatchDisciplineDiagnostics& match_discipline_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->match_discipline;
}
const MatchDisciplineDiagnostics& match_discipline_diagnostics(const State& state) {
    static const MatchDisciplineDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->match_discipline : empty;
}
BallContactDiagnostics& ball_contact_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->ball_contact;
}
const BallContactDiagnostics& ball_contact_diagnostics(const State& state) {
    static const BallContactDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->ball_contact : empty;
}
BallActionDiagnostics& ball_action_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->ball_action;
}
const BallActionDiagnostics& ball_action_diagnostics(const State& state) {
    static const BallActionDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->ball_action : empty;
}
PlayerMovementDiagnostics& player_movement_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->player_movement;
}
const PlayerMovementDiagnostics& player_movement_diagnostics(const State& state) {
    static const PlayerMovementDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->player_movement : empty;
}
State::State() = default;
State::~State() = default;
ActorMotionDiagnostics& actor_motion_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->actor_motion;
}
const ActorMotionDiagnostics& actor_motion_diagnostics(const State& state) {
    static const ActorMotionDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->actor_motion : empty;
}
PlayerDecisionDiagnostics& player_decision_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->player_decision;
}
const PlayerDecisionDiagnostics& player_decision_diagnostics(const State& state) {
    static const PlayerDecisionDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->player_decision : empty;
}
MarkingPressureDiagnostics& marking_pressure_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->marking_pressure;
}
const MarkingPressureDiagnostics& marking_pressure_diagnostics(const State& state) {
    static const MarkingPressureDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->marking_pressure : empty;
}
SpatialRoleDiagnostics& spatial_role_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->spatial_role;
}
const SpatialRoleDiagnostics& spatial_role_diagnostics(const State& state) {
    static const SpatialRoleDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->spatial_role : empty;
}
ActorConstructionDiagnostics& actor_construction_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->actor_construction;
}
const ActorConstructionDiagnostics& actor_construction_diagnostics(const State& state) {
    static const ActorConstructionDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->actor_construction : empty;
}
EngineInitializationDiagnostics& engine_initialization_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->engine_initialization;
}
MatchActorDiagnostics& match_actor_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->match_actor;
}
const MatchActorDiagnostics& match_actor_diagnostics(const State& state) {
    static const MatchActorDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->match_actor : empty;
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

RecordQueryDiagnostics& record_query_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->record_query;
}
const RecordQueryDiagnostics& record_query_diagnostics(const State& state) {
    static const RecordQueryDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->record_query : empty;
}

RecordHistoryDiagnostics& record_history_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->record_history;
}
const RecordHistoryDiagnostics& record_history_diagnostics(const State& state) {
    static const RecordHistoryDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->record_history : empty;
}

MatchEngineDiagnostics& match_engine_diagnostics(State& state) {
    if (!state.runtime_diagnostics) state.runtime_diagnostics = std::make_unique<RuntimeDiagnostics>();
    return state.runtime_diagnostics->match_engine;
}
const MatchEngineDiagnostics& match_engine_diagnostics(const State& state) {
    static const MatchEngineDiagnostics empty;
    return state.runtime_diagnostics ? state.runtime_diagnostics->match_engine : empty;
}

}
