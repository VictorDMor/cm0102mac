#!/usr/bin/env python3
"""Translate selected, reachable original PowerPC P-code to build-time C++.

Unresolved indirect destinations stop the native program with an address. They
never fall back to interpreting instructions. Expand roots and rebuild offline.
"""
import argparse
import bisect
from collections import defaultdict, deque
import functools
import json
import hashlib
import inspect
import re
from pathlib import Path
import sqlite3
import struct
from generation_storage import seed_generated
from unsigned_division_audit import require_unsigned_division_routine
from match_presentation_audit import require_match_presentation_context
from match_speed_audit import require_match_speed_context, MATCH_SPEED_PAGES
from local_temporaries import LocalTemporaries
from native_record_fields import CONTRACTS as NATIVE_RECORD_FIELDS, load_leaves, generate_native_record_fields
from native_sort8 import ENTRY as NATIVE_SORT8_ENTRY, load_sort8, require_sort8_page, generate_sort8
from vector_context_lowering import lower_vector_intrinsic
from calendar_audit import require_calendar_contract, DATE_SUBSYSTEM_ENTRIES
from rng_core_audit import require_rng_core_contract, RNG_CORE_ENTRIES
from background_selection_audit import require_background_selection_contract, BACKGROUND_SELECTION_ENTRIES
from collection_lookup_audit import require_collection_lookup_contract, COLLECTION_LOOKUP_ENTRIES
from string_core_audit import require_string_core_contract, STRING_CORE_ENTRIES
from memory_primitives_audit import require_memory_primitives_contract, MEMORY_PRIMITIVES_ENTRIES
from integer_parsing_audit import (require_integer_parsing_contract, require_integer_parsing_index,
                                   require_integer_parsing_hooks,
                                   INTEGER_PARSING_ENTRIES)
from formatted_input_audit import (require_formatted_input_contract, require_formatted_input_index,
                                   require_formatted_input_hooks, FORMATTED_INPUT_ENTRIES)
from character_conversion_audit import (require_character_conversion_contract, require_character_conversion_index,
                                       require_character_conversion_hooks, CHARACTER_CONVERSION_ENTRIES)
from floating_input_audit import (require_floating_input_contract, require_floating_input_index,
                                  require_floating_input_hooks, FLOATING_INPUT_ENTRIES)
from formatted_output_audit import (require_formatted_output_contract, require_formatted_output_index,
                                    require_formatted_output_hooks, FORMATTED_OUTPUT_ENTRIES)
from allocator_audit import (require_allocator_contract, require_allocator_index,
                             require_allocator_hooks, ALLOCATOR_ENTRIES)
from stream_buffer_audit import (require_stream_buffer_contract, require_stream_buffer_index,
                                 require_stream_buffer_hooks, STREAM_BUFFER_ENTRIES)
from stream_io_audit import (require_stream_io_contract, require_stream_io_index,
                             require_stream_io_hooks, STREAM_IO_ENTRIES)
from file_controller_audit import (require_file_controller_contract, require_file_controller_index,
                                   require_file_controller_hooks, FILE_CONTROLLER_ENTRIES)
from file_path_audit import (require_file_path_contract, require_file_path_index,
                             require_file_path_hooks, FILE_PATH_ENTRIES)
from runtime_lifecycle_audit import (require_runtime_lifecycle_contract, require_runtime_lifecycle_index,
                                    require_runtime_lifecycle_hooks, RUNTIME_LIFECYCLE_ENTRIES)
from code_fragment_audit import (require_code_fragment_contract, require_code_fragment_index,
                                require_code_fragment_hooks, CODE_FRAGMENT_ENTRIES)
from cpp_object_storage_audit import (require_cpp_object_storage_contract, require_cpp_object_storage_index,
                                    require_cpp_object_storage_hooks, CPP_OBJECT_STORAGE_ENTRIES)
from exception_runtime_audit import (require_exception_runtime_contract, require_exception_runtime_index,
                                    require_exception_runtime_hooks, EXCEPTION_RUNTIME_ENTRIES)
from compiler_arithmetic_audit import (require_compiler_arithmetic_contract, require_compiler_arithmetic_index,
                                     require_compiler_arithmetic_hooks, compiler_arithmetic_hook, COMPILER_ARITHMETIC_ENTRIES)
from native_drawing_audit import NATIVE_DRAWING_BOUNDARIES, require_native_drawing_contract
from register_context_audit import (require_register_context_contract, require_register_context_index,
                                   require_register_context_hooks, register_context_hook,
                                   REGISTER_CONTEXT_ENTRIES, CAPTURE_OBSERVATIONS)
from stdlib_core_audit import (require_stdlib_core_contract, require_stdlib_core_index,
                               require_stdlib_core_hooks, stdlib_core_hook, STDLIB_CORE_ENTRIES)
from time_runtime_audit import (require_time_runtime_contract, require_time_runtime_index,
                                require_time_runtime_hooks, time_runtime_hook, TIME_RUNTIME_ENTRIES)

from import_descriptor_audit import (require_import_descriptor_contract, require_import_descriptor_index,
                                     require_import_descriptor_hooks, import_descriptor_hook, IMPORT_DESCRIPTOR_ENTRIES)

from native_service_audit import (require_native_service_contract, require_native_service_index,
                                  require_native_service_hooks, native_service_hook, NATIVE_SERVICE_ENTRIES)

from engine_initialization_audit import (require_engine_initialization_contract, require_engine_initialization_index,
                                         require_engine_initialization_hooks, engine_initialization_hook,
                                         ENGINE_INITIALIZATION_ENTRIES)
from utility_object_audit import (require_utility_object_contract, require_utility_object_index,
                                 require_utility_object_hooks, utility_object_hook, UTILITY_OBJECT_ENTRIES)

from tree_mutation_audit import (require_tree_mutation_contract, require_tree_mutation_index,
                                 require_tree_mutation_hooks, tree_mutation_hook, TREE_MUTATION_ENTRIES)

from aggregate_object_audit import (require_aggregate_object_contract, require_aggregate_object_index,
                                    require_aggregate_object_hooks, aggregate_object_hook, AGGREGATE_OBJECT_ENTRIES)

from record_group_audit import (require_record_group_contract, require_record_group_index,
                                require_record_group_hooks, record_group_hook, RECORD_GROUP_ENTRIES)

from record_history_audit import (require_record_history_contract, require_record_history_index,
                                  require_record_history_hooks, record_history_hook, RECORD_HISTORY_ENTRIES)

from record_query_audit import (require_record_query_contract, require_record_query_index,
                                require_record_query_hooks, record_query_hook, RECORD_QUERY_ENTRIES)
from match_engine_audit import (require_match_engine_contract, require_match_engine_index,
                               require_match_engine_hooks, match_engine_hook, MATCH_ENGINE_ENTRIES)
from match_actor_audit import (require_match_actor_contract, require_match_actor_index,
                              require_match_actor_hooks, match_actor_hook, MATCH_ACTOR_ENTRIES)
from actor_construction_audit import (require_actor_construction_contract, require_actor_construction_index,
                                      require_actor_construction_hooks, actor_construction_hook, ACTOR_CONSTRUCTION_ENTRIES)
from spatial_role_audit import (require_spatial_role_contract, require_spatial_role_index,
                                require_spatial_role_hooks, spatial_role_hook, SPATIAL_ROLE_ENTRIES)
from marking_pressure_audit import (require_marking_pressure_contract, require_marking_pressure_index,
                                    require_marking_pressure_hooks, marking_pressure_hook, MARKING_PRESSURE_ENTRIES)
from player_decision_audit import (require_player_decision_contract, require_player_decision_index,
                                    require_player_decision_hooks, player_decision_hook, PLAYER_DECISION_ENTRIES)
from actor_motion_audit import (require_actor_motion_contract, require_actor_motion_index,
                                require_actor_motion_hooks, actor_motion_hook, ACTOR_MOTION_ENTRIES)

CALENDAR_ADAPTER_ENTRIES = (
    0x103317f0, 0x10331860, 0x10715b20, 0x10717890,
    0x10717b20, 0x10718130, 0x107182f0, 0x10718360)
from player_movement_audit import (require_player_movement_contract, require_player_movement_index,
                                   require_player_movement_hooks, player_movement_hook, PLAYER_MOVEMENT_ENTRIES)

from match_discipline_audit import (require_match_discipline_contract, require_match_discipline_index,
                                   require_match_discipline_hooks, match_discipline_hook, MATCH_DISCIPLINE_ENTRIES)

from match_random_audit import (require_match_random_contract, require_match_random_index,
                                require_match_random_hooks, match_random_hook, MATCH_RANDOM_ENTRIES)
from team_spatial_audit import (require_team_spatial_contract, require_team_spatial_index,
                                require_team_spatial_hooks, team_spatial_hook, TEAM_SPATIAL_ENTRIES)

from pitch_distance_audit import (require_pitch_distance_contract, require_pitch_distance_index,
                                  require_pitch_distance_hooks, pitch_distance_hook, PITCH_DISTANCE_ENTRIES)

from actor_order_audit import (require_actor_order_contract, require_actor_order_index,
                                  require_actor_order_hooks, actor_order_hook, ACTOR_ORDER_ENTRIES)

from movement_choice_audit import (require_movement_choice_contract, require_movement_choice_index,
                                  require_movement_choice_hooks, movement_choice_hook, MOVEMENT_CHOICE_ENTRIES)
from team_line_audit import (require_team_line_contract, require_team_line_index,
                                  require_team_line_hooks, team_line_hook, TEAM_LINE_ENTRIES)
from ball_action_audit import (require_ball_action_contract, require_ball_action_index,
                                  require_ball_action_hooks, ball_action_hook, BALL_ACTION_ENTRIES)
from ball_contact_audit import (require_ball_contact_contract, require_ball_contact_index,
                                  require_ball_contact_hooks, ball_contact_hook, BALL_CONTACT_ENTRIES)

from actor_attributes_audit import (require_actor_attributes_contract, require_actor_attributes_index,
                                    require_actor_attributes_hooks, actor_attributes_hook, ACTOR_ATTRIBUTES_ENTRIES)

from injury_record_audit import (require_injury_record_contract, require_injury_record_index,
                                 require_injury_record_hooks, injury_record_hook, INJURY_RECORD_ENTRIES)

from injury_progression_audit import (require_injury_progression_contract, require_injury_progression_index,
    require_injury_progression_hooks, injury_progression_hook, INJURY_PROGRESSION_ENTRIES)
from injury_club_events_audit import (require_injury_club_events_contract, require_injury_club_events_index,
    require_injury_club_events_hooks, injury_club_events_hook, INJURY_CLUB_EVENTS_ENTRIES)
from injury_readouts_audit import (require_injury_readouts_contract, require_injury_readouts_index,
    require_injury_readouts_hooks, injury_readouts_hook, INJURY_READOUTS_ENTRIES)
from injury_reporting_audit import (require_injury_reporting_contract, require_injury_reporting_index,
    require_injury_reporting_hooks, injury_reporting_hook, INJURY_REPORTING_ENTRIES)
from injury_bootstrap_audit import (require_injury_bootstrap_contract, require_injury_bootstrap_index,
    require_injury_bootstrap_hooks, injury_bootstrap_hook, INJURY_BOOTSTRAP_ENTRIES)
from injury_persistence_audit import (require_injury_persistence_contract, require_injury_persistence_index,
    require_injury_persistence_hooks, injury_persistence_hook, INJURY_PERSISTENCE_ENTRIES)
from injury_daily_audit import (require_injury_daily_contract, require_injury_daily_index,
    require_injury_daily_hooks, injury_daily_hook, INJURY_DAILY_ENTRIES)
from awol_records_audit import (require_awol_records_contract, require_awol_records_index,
    require_awol_records_hooks, awol_records_hook, AWOL_RECORDS_ENTRIES)
from contract_query_audit import (require_contract_query_contract, require_contract_query_index,
    require_contract_query_hooks, contract_query_hook, CONTRACT_QUERY_ENTRIES)
from contract_storage_audit import (require_contract_storage_contract, require_contract_storage_index,
    require_contract_storage_hooks, contract_storage_hook, CONTRACT_STORAGE_ENTRIES)
from contract_terms_audit import (require_contract_terms_contract, require_contract_terms_index,
    require_contract_terms_hooks, contract_terms_hook, CONTRACT_TERMS_ENTRIES)
from contract_review_audit import (require_contract_review_contract, require_contract_review_index,
    require_contract_review_hooks, contract_review_hook, CONTRACT_REVIEW_ENTRIES)
from transfer_offer_audit import (require_transfer_offer_contract, require_transfer_offer_index,
    require_transfer_offer_hooks, transfer_offer_hook, TRANSFER_OFFER_ENTRIES)
from transfer_record_audit import (require_transfer_record_contract, require_transfer_record_index,
    require_transfer_record_hooks, transfer_record_hook, TRANSFER_RECORD_ENTRIES)
from transfer_valuation_audit import (require_transfer_valuation_contract, require_transfer_valuation_index,
    require_transfer_valuation_hooks, transfer_valuation_hook, TRANSFER_VALUATION_ENTRIES)
from transfer_synthesis_audit import (require_transfer_synthesis_contract, require_transfer_synthesis_index,
    require_transfer_synthesis_hooks, transfer_synthesis_hook, TRANSFER_SYNTHESIS_ENTRIES)
from club_shortlist_storage_audit import (require_club_shortlist_storage_contract, require_club_shortlist_storage_index,
    require_club_shortlist_storage_hooks, club_shortlist_storage_hook, CLUB_SHORTLIST_STORAGE_ENTRIES)
from shortlist_category_audit import (require_shortlist_category_contract, require_shortlist_category_index,
    require_shortlist_category_hooks, shortlist_category_hook, SHORTLIST_CATEGORY_ENTRIES)
from shortlist_links_audit import (require_shortlist_links_contract, require_shortlist_links_index,
    require_shortlist_links_hooks, shortlist_links_hook, SHORTLIST_LINKS_ENTRIES)
from shortlist_recruitment_audit import (require_shortlist_recruitment_contract, require_shortlist_recruitment_index,
    require_shortlist_recruitment_hooks, shortlist_recruitment_hook, SHORTLIST_RECRUITMENT_ENTRIES)
from shortlist_assessment_audit import (require_shortlist_assessment_contract, require_shortlist_assessment_index,
    require_shortlist_assessment_hooks, shortlist_assessment_hook, SHORTLIST_ASSESSMENT_ENTRIES)
from shortlist_quotation_audit import (require_shortlist_quotation_contract, require_shortlist_quotation_index,
    require_shortlist_quotation_hooks, shortlist_quotation_hook, SHORTLIST_QUOTATION_ENTRIES)
from shortlist_policy_audit import (require_shortlist_policy_contract, require_shortlist_policy_index,
    require_shortlist_policy_hooks, shortlist_policy_hook, SHORTLIST_POLICY_ENTRIES)
from profile_qualification_audit import (require_profile_qualification_contract, require_profile_qualification_index,
    require_profile_qualification_hooks, profile_qualification_hook, PROFILE_QUALIFICATION_ENTRIES)
from contract_draft_audit import (require_contract_draft_contract, require_contract_draft_index,
    require_contract_draft_hooks, contract_draft_hook, CONTRACT_DRAFT_ENTRIES)
from contract_display_audit import (require_contract_display_contract, require_contract_display_index,
    require_contract_display_hooks, contract_display_hook, CONTRACT_DISPLAY_ENTRIES)
from money_value_audit import (require_money_value_contract, require_money_value_index,
    require_money_value_hooks, money_value_hook, MONEY_VALUE_ENTRIES)
from money_text_audit import (require_money_text_contract, require_money_text_index,
    require_money_text_hooks, money_text_hook, MONEY_TEXT_ENTRIES)
from money_configuration_audit import (require_money_configuration_contract, require_money_configuration_index,
    require_money_configuration_hooks, money_configuration_hook, MONEY_CONFIGURATION_ENTRIES)
from preferences_audit import (require_preferences_contract, require_preferences_index,
    require_preferences_hooks, preferences_hook, PREFERENCES_ENTRIES)
from preferences_actions_audit import (require_preferences_actions_contract, require_preferences_actions_index,
    require_preferences_actions_hooks, preferences_actions_hook, PREFERENCES_ACTIONS_ENTRIES)
from preferences_screen_audit import (require_preferences_screen_contract, require_preferences_screen_index,
    require_preferences_screen_hooks, preferences_screen_hook, PREFERENCES_SCREEN_ENTRIES)
from localization_lookup_audit import (require_localization_lookup_contract, require_localization_lookup_index,
    require_localization_lookup_hooks, localization_lookup_hook, LOCALIZATION_LOOKUP_ENTRIES)
from localization_format_audit import (require_localization_format_contract, require_localization_format_index,
    require_localization_format_hooks, localization_format_hook, LOCALIZATION_FORMAT_ENTRIES)
from localization_catalog_audit import (require_localization_catalog_contract, require_localization_catalog_index,
    require_localization_catalog_hooks, localization_catalog_hook, LOCALIZATION_CATALOG_ENTRIES)
from range_sampling_audit import (require_range_sampling_contract, require_range_sampling_index,
    require_range_sampling_hooks, range_sampling_hook, RANGE_SAMPLING_ENTRIES)
from match_target_audit import (require_match_target_contract, require_match_target_index,
    require_match_target_hooks, match_target_hook, MATCH_TARGET_ENTRIES)
from match_selection_audit import (require_match_selection_contract, require_match_selection_index,
    require_match_selection_hooks, match_selection_hook, MATCH_SELECTION_ENTRIES)
from match_selection_support_audit import (require_match_selection_support_contract, require_match_selection_support_index,
    require_match_selection_support_hooks, match_selection_support_hook, MATCH_SELECTION_SUPPORT_ENTRIES)
from actor_encounter_audit import (require_actor_encounter_contract, require_actor_encounter_index,
    require_actor_encounter_hooks, actor_encounter_hook, ACTOR_ENCOUNTER_ENTRIES)
from match_event_audit import (require_match_event_contract, require_match_event_index,
    require_match_event_hooks, match_event_hook, MATCH_EVENT_ENTRIES)
from actor_flags_audit import (require_actor_flags_contract, require_actor_flags_index,
    require_actor_flags_hooks, actor_flags_hook, ACTOR_FLAGS_ENTRIES)
from actor_effects_audit import (require_actor_effects_contract, require_actor_effects_index,
    require_actor_effects_hooks, actor_effects_hook, ACTOR_EFFECTS_ENTRIES)
from target_acceptance_audit import (require_target_acceptance_contract, require_target_acceptance_index,
    require_target_acceptance_hooks, target_acceptance_hook, TARGET_ACCEPTANCE_ENTRIES)
from actor_action_audit import (require_actor_action_contract, require_actor_action_index,
    require_actor_action_hooks, actor_action_hook, ACTOR_ACTION_ENTRIES, load_actor_action_table)
from actor_policy_audit import (require_actor_policy_contract, require_actor_policy_index,
    require_actor_policy_hooks, actor_policy_hook, ACTOR_POLICY_ENTRIES)
from turbo_pages import TURBO_PAGES, bound_turbo_hook

SEMANTIC_REPLACEMENTS = (
    tuple((entry, "calendar_adapter") for entry in CALENDAR_ADAPTER_ENTRIES) +
    tuple((entry, "date_subsystem_adapter") for entry in DATE_SUBSYSTEM_ENTRIES) +
    tuple((entry, "rng_core_adapter") for entry in RNG_CORE_ENTRIES) +
    tuple((entry, "background_selection_adapter") for entry in BACKGROUND_SELECTION_ENTRIES) +
    tuple((entry, "collection_lookup_adapter") for entry in COLLECTION_LOOKUP_ENTRIES) +
    tuple((entry, "string_core_adapter") for entry in STRING_CORE_ENTRIES) +
    tuple((entry, "memory_primitives_adapter") for entry in MEMORY_PRIMITIVES_ENTRIES) +
    tuple((entry, "integer_parsing_adapter") for entry in INTEGER_PARSING_ENTRIES) +
    tuple((entry, "formatted_input_adapter") for entry in FORMATTED_INPUT_ENTRIES) +
    tuple((entry, "formatted_output_adapter") for entry in FORMATTED_OUTPUT_ENTRIES) +
    tuple((entry, "character_conversion_adapter") for entry in CHARACTER_CONVERSION_ENTRIES) +
    tuple((entry, "floating_input_adapter") for entry in FLOATING_INPUT_ENTRIES) +
    tuple((entry, "allocator_adapter") for entry in ALLOCATOR_ENTRIES) +
    tuple((entry, "stream_buffer_adapter") for entry in STREAM_BUFFER_ENTRIES) +
    tuple((entry, "stream_io_adapter") for entry in STREAM_IO_ENTRIES) +
    tuple((entry, "file_controller_adapter") for entry in FILE_CONTROLLER_ENTRIES) +
    tuple((entry, "file_path_adapter") for entry in FILE_PATH_ENTRIES) +
    tuple((entry, "runtime_lifecycle_adapter") for entry in RUNTIME_LIFECYCLE_ENTRIES) +
    tuple((entry, "code_fragment_adapter") for entry in CODE_FRAGMENT_ENTRIES) +
    tuple((entry, "cpp_object_storage_adapter") for entry in CPP_OBJECT_STORAGE_ENTRIES) +
    tuple((entry, "exception_runtime_adapter") for entry in EXCEPTION_RUNTIME_ENTRIES) +
    tuple((entry, "compiler_arithmetic_adapter") for entry in COMPILER_ARITHMETIC_ENTRIES) +
    tuple((entry, "register_context_adapter") for entry in REGISTER_CONTEXT_ENTRIES) +
    tuple((entry, "stdlib_core_adapter") for entry in STDLIB_CORE_ENTRIES) +
    tuple((entry, "time_runtime_adapter") for entry in TIME_RUNTIME_ENTRIES) +
    tuple((entry, "import_descriptor_adapter") for entry in IMPORT_DESCRIPTOR_ENTRIES) +
    tuple((entry, "native_service_adapter") for entry in NATIVE_SERVICE_ENTRIES) +
    tuple((entry, "engine_initialization_adapter") for entry in ENGINE_INITIALIZATION_ENTRIES) +
    tuple((entry, "utility_object_adapter") for entry in UTILITY_OBJECT_ENTRIES) +
    tuple((entry, "tree_mutation_adapter") for entry in TREE_MUTATION_ENTRIES) +
    tuple((entry, "aggregate_object_adapter") for entry in AGGREGATE_OBJECT_ENTRIES) +
    tuple((entry, "record_group_adapter") for entry in RECORD_GROUP_ENTRIES) +
    tuple((entry, "record_query_adapter") for entry in RECORD_QUERY_ENTRIES) +
    tuple((entry, "record_history_adapter") for entry in RECORD_HISTORY_ENTRIES) +
    tuple((entry, "match_engine_adapter") for entry in MATCH_ENGINE_ENTRIES) +
    tuple((entry, "match_actor_adapter") for entry in MATCH_ACTOR_ENTRIES) +
    tuple((entry, "actor_construction_adapter") for entry in ACTOR_CONSTRUCTION_ENTRIES) +
    tuple((entry, "spatial_role_adapter") for entry in SPATIAL_ROLE_ENTRIES) +
    tuple((entry, "marking_pressure_adapter") for entry in MARKING_PRESSURE_ENTRIES) +
    tuple((entry, "player_decision_adapter") for entry in PLAYER_DECISION_ENTRIES) +
    tuple((entry, "actor_motion_adapter") for entry in ACTOR_MOTION_ENTRIES) +
    tuple((entry, "player_movement_adapter") for entry in PLAYER_MOVEMENT_ENTRIES) +
    tuple((entry, "match_discipline_adapter") for entry in MATCH_DISCIPLINE_ENTRIES) +
    tuple((entry, "match_random_adapter") for entry in MATCH_RANDOM_ENTRIES) +
    tuple((entry, "team_spatial_adapter") for entry in TEAM_SPATIAL_ENTRIES) +
    tuple((entry, "pitch_distance_adapter") for entry in PITCH_DISTANCE_ENTRIES) +
    tuple((entry, "actor_order_adapter") for entry in ACTOR_ORDER_ENTRIES) +
    tuple((entry, "movement_choice_adapter") for entry in MOVEMENT_CHOICE_ENTRIES) +
    tuple((entry, "team_line_adapter") for entry in TEAM_LINE_ENTRIES) +
    tuple((entry, "ball_action_adapter") for entry in BALL_ACTION_ENTRIES) +
    tuple((entry, "ball_contact_adapter") for entry in BALL_CONTACT_ENTRIES) +
    tuple((entry, "actor_attributes_adapter") for entry in ACTOR_ATTRIBUTES_ENTRIES) +
    tuple((entry, "injury_record_adapter") for entry in INJURY_RECORD_ENTRIES) +
    tuple((entry, "injury_progression_adapter") for entry in INJURY_PROGRESSION_ENTRIES) +
    tuple((entry, "injury_club_events_adapter") for entry in INJURY_CLUB_EVENTS_ENTRIES) +
    tuple((entry, "injury_readouts_adapter") for entry in INJURY_READOUTS_ENTRIES) +
    tuple((entry, "injury_reporting_adapter") for entry in INJURY_REPORTING_ENTRIES) +
    tuple((entry, "injury_bootstrap_adapter") for entry in INJURY_BOOTSTRAP_ENTRIES) +
    tuple((entry, "injury_persistence_adapter") for entry in INJURY_PERSISTENCE_ENTRIES) +
    tuple((entry, "injury_daily_adapter") for entry in INJURY_DAILY_ENTRIES) +
    tuple((entry, "awol_records_adapter") for entry in AWOL_RECORDS_ENTRIES) +
    tuple((entry, "contract_query_adapter") for entry in CONTRACT_QUERY_ENTRIES) +
    tuple((entry, "contract_storage_adapter") for entry in CONTRACT_STORAGE_ENTRIES) +
    tuple((entry, "contract_terms_adapter") for entry in CONTRACT_TERMS_ENTRIES) +
    tuple((entry, "contract_review_adapter") for entry in CONTRACT_REVIEW_ENTRIES) +
    tuple((entry, "transfer_offer_adapter") for entry in TRANSFER_OFFER_ENTRIES) +
    tuple((entry, "transfer_record_adapter") for entry in TRANSFER_RECORD_ENTRIES) +
    tuple((entry, "transfer_valuation_adapter") for entry in TRANSFER_VALUATION_ENTRIES) +
    tuple((entry, "transfer_synthesis_adapter") for entry in TRANSFER_SYNTHESIS_ENTRIES) +
    tuple((entry, "club_shortlist_storage_adapter") for entry in CLUB_SHORTLIST_STORAGE_ENTRIES) +
    tuple((entry, "shortlist_category_adapter") for entry in SHORTLIST_CATEGORY_ENTRIES) +
    tuple((entry, "shortlist_links_adapter") for entry in SHORTLIST_LINKS_ENTRIES) +
    tuple((entry, "shortlist_recruitment_adapter") for entry in SHORTLIST_RECRUITMENT_ENTRIES) +
    tuple((entry, "shortlist_assessment_adapter") for entry in SHORTLIST_ASSESSMENT_ENTRIES) +
    tuple((entry, "shortlist_quotation_adapter") for entry in SHORTLIST_QUOTATION_ENTRIES) +
    tuple((entry, "shortlist_policy_adapter") for entry in SHORTLIST_POLICY_ENTRIES) +
    tuple((entry, "profile_qualification_adapter") for entry in PROFILE_QUALIFICATION_ENTRIES) +
    tuple((entry, "contract_draft_adapter") for entry in CONTRACT_DRAFT_ENTRIES) +
    tuple((entry, "contract_display_adapter") for entry in CONTRACT_DISPLAY_ENTRIES) +
    tuple((entry, "money_value_adapter") for entry in MONEY_VALUE_ENTRIES) +
    tuple((entry, "money_text_adapter") for entry in MONEY_TEXT_ENTRIES) +
    tuple((entry, "money_configuration_adapter") for entry in MONEY_CONFIGURATION_ENTRIES) +
    tuple((entry, "preferences_adapter") for entry in PREFERENCES_ENTRIES) +
    tuple((entry, "preferences_actions_adapter") for entry in PREFERENCES_ACTIONS_ENTRIES) +
    tuple((entry, "preferences_screen_adapter") for entry in PREFERENCES_SCREEN_ENTRIES) +
    tuple((entry, "localization_lookup_adapter") for entry in LOCALIZATION_LOOKUP_ENTRIES) +
    tuple((entry, "localization_format_adapter") for entry in LOCALIZATION_FORMAT_ENTRIES) +
    tuple((entry, "localization_catalog_adapter") for entry in LOCALIZATION_CATALOG_ENTRIES) +
    tuple((entry, "range_sampling_adapter") for entry in RANGE_SAMPLING_ENTRIES) +
    tuple((entry, "match_target_adapter") for entry in MATCH_TARGET_ENTRIES) +
    tuple((entry, "match_selection_adapter") for entry in MATCH_SELECTION_ENTRIES) +
    tuple((entry, "match_selection_support_adapter") for entry in MATCH_SELECTION_SUPPORT_ENTRIES) +
    tuple((entry, "actor_encounter_adapter") for entry in ACTOR_ENCOUNTER_ENTRIES) +
    tuple((entry, "match_event_adapter") for entry in MATCH_EVENT_ENTRIES) +
    tuple((entry, "actor_flags_adapter") for entry in ACTOR_FLAGS_ENTRIES) +
    tuple((entry, "actor_effects_adapter") for entry in ACTOR_EFFECTS_ENTRIES) +
    tuple((entry, "target_acceptance_adapter") for entry in TARGET_ACCEPTANCE_ENTRIES) +
    tuple((entry, "actor_action_adapter") for entry in ACTOR_ACTION_ENTRIES) +
    tuple((entry, "actor_policy_adapter") for entry in ACTOR_POLICY_ENTRIES))


def require_unique_semantic_replacements(replacements):
    owners = {}
    for entry, owner in replacements:
        if entry in owners:
            raise ValueError(f"Duplicate semantic replacement at {entry:x}: {owners[entry]} and {owner}")
        owners[entry] = owner


require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)


def integer(value):
    return int(value, 0)


def constant(value):
    return f"UINT64_C({value})"


def read(node):
    space, offset, size = node
    offset = int(offset)
    if size == 16:
        access = {'register': 'read_vector_register', 'unique': 'read_vector_temporary',
                  'ram': 'memory.read_vector'}.get(space)
        if access is None:
            raise ValueError(f'Unsupported vector value space: {node}')
        return f's.{access}({offset})'
    if size not in (1, 2, 4, 8):
        raise ValueError(f"Unsupported value width: {node}")
    if space == "const":
        return constant(offset & ((1 << (size * 8)) - 1))
    access = {"register": "r", "unique": "u", "ram": "memory.read"}.get(space)
    if not access:
        raise ValueError(f"Unsupported value space: {node}")
    return f"s.{access}<{size}>({offset})"


def write(node, value):
    space, offset, size = node
    if size == 16:
        access = {'register': 'write_vector_register', 'unique': 'write_vector_temporary',
                  'ram': 'memory.write_vector'}.get(space)
        if access is None:
            raise ValueError(f'Unsupported vector output space: {node}')
        return f's.{access}({int(offset)}, {value});'
    if size not in (1, 2, 4, 8):
        raise ValueError(f"Unsupported output width: {node}")
    access = {"register": "wr", "unique": "wu", "ram": "memory.write"}.get(space)
    if not access:
        raise ValueError(f"Unsupported output space: {node}")
    return f"s.{access}<{size}>({int(offset)}, {value});"


def transfer(address, page, selected):
    if (page is None or address >> 12 == page) and address in selected:
        return f"goto i_{address:x};"
    return f"s.pc = 0x{address:x}; return true;"


FUSED_MNEMONICS = {"fmadd": "FLOAT_FUSED_ADD", "fmsub": "FLOAT_FUSED_SUB",
                   "fnmadd": "FLOAT_FUSED_ADD", "fnmsub": "FLOAT_FUSED_SUB"}
FUSED_OPERATIONS = {f"{name}{suffix}" for name in set(FUSED_MNEMONICS.values()) for suffix in ("", "_SINGLE")}


def fused_mnemonic(item):
    """(base mnemonic, single precision) for a multiply-add, else None."""
    mnemonic = item["assembly"].split()[0].rstrip(".")
    single = mnemonic.endswith("s") and mnemonic[:-1] in FUSED_MNEMONICS
    base = mnemonic[:-1] if single else mnemonic
    return (base, single) if base in FUSED_MNEMONICS else None


def fused_site_kind(item):
    """FusedKind bits (native/fused_multiply_add.hpp) for a multiply-add, else None."""
    fused = fused_mnemonic(item)
    if fused is None: return None
    base, single = fused
    return ("sub" in base) | (base.startswith("fn") << 1) | (single << 2)


def render_fused_sites(sites):
    sites = sorted(set(sites))
    rows = ",".join(f"{{0x{address:x},{kind}}}" for address, kind in sites) or "{0,0}"
    return ('#include "fused_multiply_add.hpp"\nnamespace cm::aot {\n'
            f"const FusedSite fused_sites[] = {{{rows}}};\n"
            f"const std::size_t fused_site_count = {len(sites)};\n}}\n")


def fused_multiply_add_ops(item):
    """Replace Ghidra's rounded product + add/sub with one fused operation.

    PowerPC multiply-add rounds once; Ghidra exports a FLOAT_MULT into a
    unique temporary followed by FLOAT_ADD/FLOAT_SUB. The product stays for
    any flag computation that reads it; only the sum changes.
    """
    fused = fused_mnemonic(item)
    if fused is None:
        return item["ops"]
    base, single = fused
    mnemonic = item["assembly"].split()[0]
    ops = [list(op) for op in item["ops"]]
    products = [i for i, op in enumerate(ops) if op[0] == "FLOAT_MULT"]
    sums = [i for i, op in enumerate(ops) if op[0] in ("FLOAT_ADD", "FLOAT_SUB")]
    expected = "FLOAT_SUB" if FUSED_MNEMONICS[base].endswith("SUB") else "FLOAT_ADD"
    if len(products) != 1 or len(sums) != 1 or sums[0] != products[0] + 1 or \
       ops[products[0]][1][0] != "unique" or ops[sums[0]][0] != expected or \
       ops[sums[0]][2] != ops[products[0]][1]:
        raise ValueError(f"Unrecognized {mnemonic} export at {item['address']:x}")
    product, total = ops[products[0]], ops[sums[0]]
    ops[sums[0]] = [FUSED_MNEMONICS[base] + ("_SINGLE" if single else ""), total[1], product[2], product[3], total[3]]
    return ops


def operation(op, address, index, page, selected, op_count, read_node=read, write_node=write, userops=None):
    name, output, *inputs = op
    if name == "CALLOTHER":
        vector = lower_vector_intrinsic(op, userops, read_node, write_node)
        if vector is not None:
            return vector
        return 'throw std::runtime_error("Unimplemented original intrinsic at ' + hex(address) + '");'
    if any(node is not None and node[2] == 16 for node in (output, *inputs)) \
            and name not in ('LOAD', 'STORE', 'COPY'):
        raise ValueError(f'Unsupported vector operation: {name}')
    if name in ("BRANCH", "CALL", "CBRANCH"):
        dest = inputs[0]
        if dest[0] == "const":
            offset = int(dest[1]) & ((1 << (8 * dest[2])) - 1)
            if offset >= 1 << (8 * dest[2] - 1):
                offset -= 1 << (8 * dest[2])
            target = index + offset
            if not 0 <= target <= op_count:
                raise ValueError(f"P-code branch outside instruction at {address:x}")
            jump = f"goto op_{address:x}_{target};"
        elif dest[0] == "ram":
            jump = transfer(int(dest[1]), page, selected)
        else:
            raise ValueError(f"Unexpected branch space: {dest}")
        if name == "CBRANCH":
            return f"if ({read_node(inputs[1])} != 0) {{ {jump} }}"
        return jump
    if name in ("BRANCHIND", "CALLIND", "RETURN"):
        return f"s.pc = std::uint32_t({read_node(inputs[0])}); return true;"
    if name in ("LOAD", "STORE"):
        if inputs[0] != ["const", "417", 8]:
            raise ValueError(f"Unexpected memory address space: {inputs[0]}")
        if name == "LOAD":
            if output[2] == 16:
                return write_node(output, f's.memory.read_vector({read_node(inputs[1])})')
            return write_node(output, f"s.memory.read<{output[2]}>({read_node(inputs[1])})")
        if inputs[2][2] == 16:
            return f's.memory.write_vector({read_node(inputs[1])}, {read_node(inputs[2])});'
        return f"s.memory.write<{inputs[2][2]}>({read_node(inputs[1])}, {read_node(inputs[2])});"

    args = [read_node(n) for n in inputs]
    a = args[0] if args else None
    b = args[1] if len(args) > 1 else None
    n = inputs[0][2] if inputs else None
    binary = {
        "INT_ADD": "+", "INT_SUB": "-", "INT_MULT": "*", "INT_AND": "&",
        "INT_OR": "|", "INT_XOR": "^", "INT_EQUAL": "==", "INT_NOTEQUAL": "!=",
        "INT_LESS": "<", "INT_LESSEQUAL": "<=", "BOOL_AND": "&&", "BOOL_OR": "||",
    }
    helpers = {"INT_LEFT": "shift_left", "INT_RIGHT": "shift_right",
               "INT_SRIGHT": "shift_signed", "INT_CARRY": "carry", "INT_SDIV": "divide_signed"}
    if name in binary:
        value = f"({a} {binary[name]} {b})"
    elif name in helpers:
        value = f"{helpers[name]}<{n}>({a}, {b})"
    elif name in ("COPY", "INT_ZEXT"):
        value = a
    elif name == "INT_SEXT":
        value = f"std::uint64_t(signed_value<{n}>({a}))"
    elif name == "INT_SLESS":
        value = f"(signed_value<{n}>({a}) < signed_value<{n}>({b}))"
    elif name == "INT_NEGATE":
        value = f"~{a}"
    elif name == "INT_2COMP":
        value = f"(UINT64_C(0) - {a})"
    elif name == "BOOL_NEGATE":
        value = f"({a} == 0)"
    elif name == "LZCOUNT":
        value = f"leading_zeros<{n}>({a})"
    elif name == "INT_DIV":
        value = f"divide_unsigned({a}, {b})"
    elif name == "SUBPIECE":
        value = f"shift_right<{n}>({a}, {b} * 8)"
    elif name.startswith("FLOAT") or name == "TRUNC":
        fa = f"float_value<{n}>({a})"
        fb = f"float_value<{inputs[1][2]}>({b})" if b else None
        math_op = {"FLOAT_ADD": "+", "FLOAT_SUB": "-", "FLOAT_MULT": "*", "FLOAT_DIV": "/"}
        if name in math_op:
            value = f"float_bits<{output[2]}>({fa} {math_op[name]} {fb})"
        elif name in ("FLOAT_EQUAL", "FLOAT_LESS"):
            value = f"({fa} {'==' if name == 'FLOAT_EQUAL' else '<'} {fb})"
        elif name == "FLOAT_NAN":
            value = f"std::isnan({fa})"
        elif name == "FLOAT2FLOAT":
            value = f"float_bits<{output[2]}>({fa})"
        elif name == "FLOAT_ABS":
            value = f"float_bits<{output[2]}>(std::abs({fa}))"
        elif name == "FLOAT_NEG":
            value = f"float_bits<{output[2]}>(-{fa})"
        elif name == "TRUNC":
            value = f"float_truncate<{output[2]}>({fa})"
        elif name in FUSED_OPERATIONS:
            if output[2] != 8 or any(node[2] != 8 for node in inputs):
                raise ValueError(f"Unexpected fused multiply-add width at {address:x}")
            helper = "fused_multiply_add_single" if name.endswith("_SINGLE") else "fused_multiply_add"
            value = f"{helper}({a}, {b}, {args[2]}, {'true' if '_SUB' in name else 'false'})"
        else:
            raise ValueError(f"Unsupported floating operation: {name}")
    else:
        raise ValueError(f"Unsupported operation: {name}")
    return write_node(output, value)


def zero_divisor_quotient_unused(lookup, address, blocked=frozenset()):
    """True when a divw's quotient cannot matter if its divisor is zero.

    PowerPC divw does not trap on a zero divisor; the quotient is undefined.
    The game's modulo idiom multiplies that quotient by the same divisor
    (mullw, giving 0 whatever the quotient) and overwrites it before any other
    read. Prove that on every path: the only reads of the quotient register
    are such multiplies while the divisor is unchanged, until a full
    overwrite. Calls, returns, indirect flow, userops, cycles, record forms,
    partial accesses and `blocked` addresses (watched by host code, owner
    entries, special lowerings) are not proofs. `lookup` maps an address to
    its export record, or None outside the known code.
    """
    item = lookup(address)
    if item is None or address in blocked or len(item["ops"]) != 1 or item["ops"][0][0] != "INT_SDIV":
        return False
    _, quotient, _, divisor = item["ops"][0]
    if any(node[0] != "register" or node[2] != 4 for node in (quotient, divisor)) or quotient == divisor:
        return False

    def overlaps(node, register):
        return node is not None and node[0] == "register" and \
            int(node[1]) < int(register[1]) + register[2] and int(register[1]) < int(node[1]) + node[2]

    proven = set()

    def visit(at, path, divisor_intact):
        # Memoize by the divisor state on entry: the same address can be
        # reached with the divisor intact on one path and changed on another.
        entry = divisor_intact
        if (at, entry) in proven:
            return True
        item = lookup(at)
        if item is None or at in blocked or at in path or len(path) >= 128:
            return False
        for name, output, *inputs in item["ops"]:
            if name == "CALLOTHER":
                return False
            if any(overlaps(node, quotient) for node in inputs):
                if not (divisor_intact and name == "INT_MULT" and sorted(map(tuple, inputs)) ==
                        sorted([tuple(quotient), tuple(divisor)])):
                    return False
            if overlaps(output, divisor):
                divisor_intact = False
            if overlaps(output, quotient):
                if output != quotient:
                    return False
                proven.add((at, entry))
                return True
        if item["flow"] not in ("FALL_THROUGH", "CONDITIONAL_JUMP", "UNCONDITIONAL_JUMP"):
            return False
        targets = list(item.get("flows", []))
        if item["flow"] != "UNCONDITIONAL_JUMP":
            targets.append(item.get("fallthrough"))
        if not targets or None in targets or \
                not all(visit(target, path | {at}, divisor_intact) for target in targets):
            return False
        proven.add((at, entry))
        return True

    return visit(item["fallthrough"], {address}, True) if item.get("fallthrough") else False


def render_divisions(divisions):
    """quotient_free_division: lets rewritten owners continue past a zero
    divisor at exactly the divw sites generated pages do."""
    values = sorted(divisions)
    rows = [", ".join(f"0x{a:x}" for a in values[i:i + 8]) for i in range(0, len(values), 8)]
    return "\n".join([
        '#include "aot_runtime.hpp"', "#include <algorithm>", "#include <array>", "namespace cm::aot {",
        "namespace {", f"constexpr std::array<std::uint32_t, {len(values)}> quotient_free{{",
        ",\n".join(rows), "};", "}",
        "bool quotient_free_division(std::uint32_t address) {",
        "return std::binary_search(quotient_free.begin(), quotient_free.end(), address);", "}", "}", ""])


def render_watched_routines(call_targets, watched=None):
    """routine_watches_pc: owner entries whose routine may hold an instruction
    that the game's host code (not the checks, which run with an instruction
    budget) watches by exact PC. Builder owners of these routines keep the PC
    exact at every instruction (SemanticGuestCall); others count in batches.
    The export has no routine bounds, so for each watched instruction this
    marks the nearest `bl` target and the nearest owner entry before it, and
    every owner entry between the two."""
    watched = observed_instructions(checks=False) if watched is None else watched
    starts = sorted(call_targets)
    owners = sorted({entry for entry, _ in SEMANTIC_REPLACEMENTS})
    marked = set()
    for address in watched:
        start = bisect.bisect_right(starts, address)
        low = starts[start - 1] if start else 0
        if start:
            marked.add(low)
        if (owner := bisect.bisect_right(owners, address)):
            marked.update(owners[min(bisect.bisect_left(owners, low), owner - 1):owner])
    entries = sorted(marked)
    rows = [", ".join(f"0x{a:x}" for a in entries[i:i + 8]) for i in range(0, len(entries), 8)]
    return "\n".join([
        '#include "aot_runtime.hpp"', "#include <algorithm>", "#include <array>", "namespace cm::aot {",
        "namespace {", f"constexpr std::array<std::uint32_t, {len(entries)}> watched{{",
        ",\n".join(rows), "};", "}",
        "bool routine_watches_pc(std::uint32_t entry) {",
        "return std::binary_search(watched.begin(), watched.end(), entry);", "}", "}", ""])


@functools.cache
def program_quotient_free_divisions():
    """The whole-program proof from the current export, for callers that
    compile only part of a page (equivalence references); None without it."""
    path = Path(__file__).resolve().parent.parent / ".local/current/export/instructions.sqlite"
    if not path.is_file():
        return None
    with sqlite3.connect(f"file:{path}?mode=ro", uri=True) as db:
        return quotient_free_divisions(db)


# 0x101b1620 searches a list for an element whose word at +8 equals *key,
# calling the list iterator 0x10738e00 for each element; native/list_search.cpp
# runs both as C++. Pin every audited instruction (assembly, flow and P-code).
LIST_SEARCH_ENTRY = 0x101b1620
LIST_SEARCH_DIGEST = "0b4607be6c5ce2595a0dc7f02c72ef5507c67f74ad743cbc8c2516777e00ee6b"


def list_search_digest(lookup):
    records = []
    for low, high in ((0x101b1620, 0x101b1718), (0x10738e00, 0x10738e24)):
        for address in range(low, high + 4, 4):
            item = lookup(address)
            if item is not None:
                records.append([address, item["assembly"], item["flow"], item.get("flows", []),
                                item.get("fallthrough"), item["ops"]])
    return hashlib.sha256(json.dumps(records, sort_keys=True).encode()).hexdigest()


def require_list_search(db):
    def lookup(address):
        row = db.execute("SELECT body FROM instructions WHERE address = ?", (address,)).fetchone()
        return json.loads(row[0]) if row else None
    if list_search_digest(lookup) != LIST_SEARCH_DIGEST:
        raise ValueError("List search 0x101b1620 / iterator 0x10738e00 no longer match native/list_search.cpp")


# Production pages call these C++ owners in place (DirectOwnerCalls): `bl X`
# runs the owner's hook at once and continues at the return address, instead
# of returning to the dispatcher twice. Only entries whose single hook is the
# standard `try_modern_<family>(s, X)` qualify, which DirectOwnerCalls checks
# once every page exists; when the owner declines, the dispatcher runs X as
# before and the hook is asked again there. Families whose hook counts a call
# before it may decline are left out, so that no call is counted twice.
NONSTANDARD_OWNER_MODULES = frozenset({"register_context_adapter", "cpp_object_storage_adapter",
                                       "compiler_arithmetic_adapter", "aggregate_object_adapter",
                                       "runtime_lifecycle_adapter"})


@functools.cache
def owners_counting_declines(native=Path(__file__).resolve().parent.parent / "native"):
    """Families whose try_modern hook increments something before its last `return false`."""
    found = set()
    hook = re.compile(r"bool try_modern_(\w+)\(State& \w+, std::uint32_t \w+\)\s*\{")
    for path in sorted(native.glob("*.cpp")):
        text = path.read_text(errors="replace")
        for match in hook.finditer(text):
            body = text[match.end():]
            body = body[:body.find("\n}")]
            last = body.rfind("return false")
            if last >= 0 and re.search(r"\+\+|\+=", body[:last]):
                found.add(match.group(1))
    return frozenset(found)


@functools.cache
def direct_owner_entries():
    """Owner entry -> family, for owners generated code may call in place."""
    counts = defaultdict(int)
    for entry, _ in SEMANTIC_REPLACEMENTS:
        counts[entry] += 1
    other = (set(SPECIAL_LOWERINGS) | set(NATIVE_RECORD_FIELDS) | {NATIVE_SORT8_ENTRY} | set(NATIVE_DRAWING_BOUNDARIES) |
             set(CALENDAR_ADAPTER_ENTRIES) | set(DATE_SUBSYSTEM_ENTRIES) | set(RNG_CORE_ENTRIES) |
             set(CAPTURE_OBSERVATIONS) | {LIST_SEARCH_ENTRY, 0x101195e8, 0x10779ed8, 0x1071a828} |
             set(observed_instructions(checks=True)))
    return {entry: module.removesuffix("_adapter") for entry, module in SEMANTIC_REPLACEMENTS
            if module.endswith("_adapter") and module not in NONSTANDARD_OWNER_MODULES and counts[entry] == 1
            and entry not in other and module.removesuffix("_adapter") not in owners_counting_declines()}


def plain_direct_call(item):
    """The callee of a plain `bl`: save the TOC, link to the next instruction, call."""
    if item.get("flow") != "UNCONDITIONAL_CALL" or not item.get("flows"):
        return None
    callee = item["flows"][0]
    plain = [["COPY", ["register", "2440", 4], ["register", "8", 4]],
             ["COPY", ["register", "4128", 4], ["const", str(item["address"] + item["size"]), 4]],
             ["CALL", None, ["ram", str(callee), 4]]]
    return callee if item["ops"] == plain else None


class DirectOwnerCalls:
    """Owners that production pages call in place, and the hooks generation
    put at each owner entry."""
    def __init__(self):
        self.called, self.entry_hooks = {}, {}

    def require_standard(self):
        """Fails if a callee's entry lost its single standard hook."""
        for callee, family in sorted(self.called.items()):
            if self.entry_hooks.get(callee) != [f"if (try_modern_{family}(s, 0x{callee:x})) return true;"]:
                raise ValueError(f"In-place owner call to {callee:x} needs exactly the standard {family} hook there")


# Zero-divisor sites found in play (day advance, two in match play). The
# proof must keep covering them; generation fails otherwise.
REQUIRED_QUOTIENT_FREE_DIVISIONS = frozenset({0x101355d4, 0x1024773c, 0x10289b28})


@functools.cache
def division_blocked_addresses():
    """Instructions whose state the game's host code may read, or that lower
    specially. Checks may stop at any of them: they compare against the same
    generated code."""
    return frozenset(observed_instructions(checks=False) | set(dict(SEMANTIC_REPLACEMENTS)) |
                     set(CAPTURE_OBSERVATIONS) | set(NATIVE_DRAWING_BOUNDARIES) | SPECIAL_LOWERINGS)


def quotient_free_divisions(db):
    """Every divw whose zero-divisor quotient provably cannot matter."""
    blocked = division_blocked_addresses()
    cache = {}

    def lookup(address):
        if address not in cache:
            row = db.execute("SELECT body FROM instructions WHERE address = ?", (address,)).fetchone()
            cache[address] = json.loads(row[0]) if row else None
        return cache[address]

    proven = frozenset(address for (address,) in db.execute(
        "SELECT address FROM instructions WHERE body LIKE '%INT_SDIV%'") if zero_divisor_quotient_unused(lookup, address, blocked))
    if missing := REQUIRED_QUOTIENT_FREE_DIVISIONS - proven:
        raise ValueError("Zero-divisor proof no longer covers " + ", ".join(f"{a:x}" for a in sorted(missing)))
    return proven


def require_control_cursor_block(instructions, selected):
    """Pin every instruction lowered by the direct Mac control-cursor block.

    Assembly text alone cannot establish equivalence: include every P-code
    operation, node width/offset, branch target, size, and flow annotation.
    Fingerprints are of the canonical fields below from the audited Mac export,
    not of newly generated C++. Any export change requires a fresh audit.
    """
    expected = {
        0x1071a828: "b01b96b48f73b1cd63dffb4c079566739c5b6191feb498739364a7e5dd3ec9a6",  # lhax r0,r28,r19
        0x1071a82c: "42f1db371c7fcc6be19a3bac847e9b2242cbadedbab693047636435086c00d0d",  # mulli r3,r0,0xbf0
        0x1071a830: "2fa70bef6f0d94e56c7ed74daeed6bfb1431a523f45b14de3f3d750a6d5e652e",  # addi r4,r3,0x4
        0x1071a834: "454a829f8049bfdd8349f27004f3d02b90c1c4e5e7040011e17766a5e6a4ceec",  # add r4,r28,r4
        0x1071a838: "370f651e17ed4e094cbb7cf6e1d690897f1dba1cde89860bd07714317dcd99f1",  # lha r3,0xb78(r4)
        0x1071a83c: "a54194819c232fa492a29083207f539fea7ecd23bd57f537375930c64a65830b",  # lha r0,0xb74(r4)
        0x1071a840: "bf12c75a4e958713105fd15e118ef7e812e55b90ca65fb6dbf9abb2434329635",  # cmpw r3,r0
        0x1071a844: "bda58ec4a260f102ce2a8a66c1776c73e081d5fd76fd495286bc1c6342fabe4d",  # bge 0x1071a860
        0x1071a848: "2408f30cc1f8fa834a0550c6658995e0344b83ae8aa6b56d014c1e662affc9b9",  # addi r0,r3,0x1
        0x1071a84c: "a8290b488cd97ec5532e3108bcc7f79d9673bb59db3bd2aebf4fe0fb57aa398c",  # rlwinm r3,r3,0x1,0x0,0x1e
        0x1071a850: "400e6fbf9b34dd2eedf2992b14584548b76ac843f9902981125a3d4eeebc8b61",  # sth r0,0xb78(r4)
        0x1071a854: "afcff9c0c0501405d2a803b0cb4bf525318d5ee467fc2279de41c6e1809a9fe6",  # addi r0,r3,0x212
        0x1071a858: "1acd4e7c8a0cfa3a04797ded6f8e8dd1045240bbe4a99556db3cef5fb9ac2b01",  # lhax r23,r4,r0
        0x1071a85c: "37aaa6efa9d31af0f71fe9ee99447d382fbc91fcbf38b9b6f33dfb4c2f4f7083",  # b 0x1071a864
        0x1071a860: "41bc39fcae77b370cb2d1704bab6c8a1fb944b2d7f866415b4981b822f1f2048",  # li r23,-0x1
    }
    fields = ("address", "size", "assembly", "flow", "fallthrough", "flows", "ops")
    required = set(expected) | {0x1071a864}
    records = defaultdict(list)
    for item in instructions:
        if item.get("address") in required:
            records[item["address"]].append(item)
    for address in required:
        if len(records[address]) != 1 or address not in selected:
            raise ValueError(f"Missing or ambiguous Mac control-cursor instruction at {address:x}")
    for address, fingerprint in expected.items():
        record = {key: records[address][0].get(key) for key in fields}
        actual = hashlib.sha256(json.dumps(record, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        if actual != fingerprint:
            raise ValueError(f"Mac control cursor no longer matches the audited instruction at {address:x}")


def ends_run(item):
    """Whether control can leave a straight-line run after this instruction.

    Exports record Ghidra's flow type; without it the P-code decides alone."""
    if item.get("flow", "FALL_THROUGH") != "FALL_THROUGH":
        return True
    return any(op[0] in ("BRANCHIND", "CALLIND", "RETURN") or
               (op[0] in ("BRANCH", "CBRANCH", "CALL") and op[2][0] == "ram") for op in item["ops"])


# Host code and checks watch exact instructions: an instruction budget that
# must end right before one, or a memory observer that reads the PC during its
# access. Each such instruction runs alone, so the PC is its own while it
# executes. Comparisons of the PC with a literal address anywhere in native/
# are found by scanning; these come from address tables instead (the reads
# where native/match_engine_transfer_checks.cpp and
# spatial_role_transfer_checks.cpp inject a longjmp, and the stops that
# day_advance_checks.cpp and match_arithmetic_checks.cpp step to).
OBSERVED_INSTRUCTION_TABLES = {0x101355d4, 0x10135614, 0x1024773c, 0x10247768, 0x10247948,
    0x101ccdf0, 0x101cd5ec, 0x101d3f78,
    0x101d3b50, 0x101d5680, 0x101d5690, 0x101d56c0, 0x101d5730, 0x101d5744, 0x101d7db0,
    0x101e3dc4, 0x101e3e54, 0x101e3ee4, 0x101e3f00, 0x101f189c, 0x101f1930, 0x101f1940,
    0x101f196c, 0x101f1dcc, 0x101f1df4, 0x101f1e2c, 0x101f21c0, 0x10289550, 0x10289570, 0x102895b0, 0x102895f0}
PC_COMPARISON = re.compile(r"\bpc\s*[!=]=\s*(0x[0-9a-fA-F]{8})\b|\b(0x[0-9a-fA-F]{8})\s*[!=]=\s*[\w>.-]*\bpc\b")


def observed_instructions(native=Path(__file__).resolve().parent.parent / "native", checks=True):
    """Addresses the native sources compare the PC with, plus the address
    tables; with checks=False, only those watched by the game's host code."""
    found = set(OBSERVED_INSTRUCTION_TABLES) if checks else set()
    for path in sorted(native.glob("*")):
        if path.suffix in (".cpp", ".hpp", ".mm") and (checks or not re.search(r"checks|fixture|characterization|benchmark", path.name)):
            for match in PC_COMPARISON.finditer(path.read_text(errors="replace")):
                found.add(int(match.group(1) or match.group(2), 16))
    return frozenset(found)


HOST_OBSERVED_INSTRUCTIONS = observed_instructions()

# Leaf routines (at most six straight-line instructions ending in blr, no
# frame, no calls) are inlined at their direct call sites in generated code:
# a call and its return through the dispatcher cost far more than the body.
# The body runs as the original instructions and is charged as executed. A
# leaf whose entry has an owner hook is inlined only when the owner is an
# exact game-state replacement from these families (the owner's own counters
# then miss those calls); generation fails if an inlined instruction carries
# any other hook or special lowering.
INLINE_OWNER_MODULES = frozenset({
    "spatial_role_adapter", "marking_pressure_adapter", "actor_order_adapter", "team_line_adapter",
    "movement_choice_adapter", "actor_effects_adapter", "actor_motion_adapter", "match_actor_adapter",
    "match_target_adapter", "actor_attributes_adapter", "utility_object_adapter", "stdlib_core_adapter",
    "contract_review_adapter", "contract_terms_adapter"})
# run_until still enters these owners' hooks for their leaves: host code (the
# Match Debugger's lineup edits) relies on them seeing direct calls.
RUN_LEAF_EXCLUDED_OWNERS = frozenset({"match_actor_adapter"})
INLINE_LEAF_LIMIT = 6
LEAF_EXCLUDED_OPS = ("CALLOTHER", "BRANCH", "CBRANCH", "CALL", "BRANCHIND", "CALLIND", "RETURN")


# Instructions generate_page lowers specially (beyond owner hooks and
# quotient-free divisions); a leaf containing one is never inlined.
SPECIAL_LOWERINGS = frozenset({
    0x1028d558, 0x10779ed8, 0x101195e8, 0x1071a828, 0x101fefc8, 0x10774c24,
    0x10713aec, 0x10713b3c, 0x10203738, 0x10147cf8, 0x10147e50, 0x101cce50, 0x10203574, 0x102036ac,
    0x10772774, 0x107727d4, 0x10772a94, 0x10772768})
OWNER_HOOK = re.compile(r"if \(try_modern_(\w+)\(s, 0x[0-9a-f]+\)\) return true;")


def inline_leaves(db, divisions=frozenset()):
    """Call target -> instruction records of an inlinable leaf, before its blr."""
    owners = dict(SEMANTIC_REPLACEMENTS)
    host_observed = observed_instructions(checks=False)
    special = (SPECIAL_LOWERINGS | divisions | set(NATIVE_RECORD_FIELDS) | {NATIVE_SORT8_ENTRY} | set(NATIVE_DRAWING_BOUNDARIES) |
               set(CALENDAR_ADAPTER_ENTRIES) | set(DATE_SUBSYSTEM_ENTRIES) | set(RNG_CORE_ENTRIES) | set(CAPTURE_OBSERVATIONS))
    targets = set()
    for (body,) in db.execute("SELECT body FROM instructions WHERE body LIKE '%UNCONDITIONAL_CALL%'"):
        item = json.loads(body)
        if item.get("flow") == "UNCONDITIONAL_CALL" and item.get("flows"):
            targets.add(item["flows"][0])
    leaves = {}
    for target in targets:
        records = [json.loads(row[0]) for row in db.execute(
            "SELECT body FROM instructions WHERE address >= ? AND address < ? ORDER BY address",
            (target, target + 4 * INLINE_LEAF_LIMIT))]
        body = []
        for expected, record in zip(range(target, target + 4 * INLINE_LEAF_LIMIT, 4), records):
            address = record["address"]
            if address != expected or address in host_observed or address in special or \
                    (address in owners and owners[address] not in INLINE_OWNER_MODULES):
                break
            if record["assembly"] == "blr":
                leaves[target] = body
                break
            writes = {int(op[1][1]) for op in record["ops"] if op[1] and op[1][0] == "register"}
            if record.get("flow") != "FALL_THROUGH" or fused_mnemonic(record) or \
                    record["assembly"].startswith("fctiwz ") or writes & {4, 4128} or \
                    any(op[0] in LEAF_EXCLUDED_OPS for op in record["ops"]):
                break
            body.append(record)
    return leaves


def leaf_body(records, userops, local_temporaries=True):
    """C++ for a leaf's instructions before its blr; they never branch."""
    code = []
    for record in records:
        ops = record["ops"]
        local = LocalTemporaries.for_instruction(ops, read, write) if local_temporaries else None
        code.append("{")
        for i, op in enumerate(ops):
            code.append(operation(op, record["address"], i, None, set(), len(ops),
                                  read_node=local.read if local else read,
                                  write_node=local.write if local else write, userops=userops))
        code.append("}")
    return code


def render_leaves(leaves, userops):
    owners = dict(SEMANTIC_REPLACEMENTS)
    leaves = {target: body for target, body in leaves.items()
              if not any(owners.get(target + 4 * n) in RUN_LEAF_EXCLUDED_OWNERS for n in range(len(body) + 1))}
    """run_leaf: runs the leaf at the PC exactly as its page would (its run's
    step and charge, then blr), without a dispatcher scope; run_until uses it
    for calls from C++ owners. Faults and budget stops are the page's."""
    # A page bitmap rejects most PCs before the switch: owners call run_until
    # for many more routines than leaves.
    base, pages = 0x10000000, {target >> 12 for target in leaves}
    if any(target < base for target in leaves):
        raise ValueError("A leaf lies below the run_leaf page bitmap")
    words = (max(pages, default=base >> 12) - (base >> 12)) // 64 + 1
    bitmap = [0] * words
    for page in pages:
        bitmap[(page - (base >> 12)) // 64] |= 1 << ((page - (base >> 12)) % 64)
    code = ['#include "aot_runtime.hpp"', '#include "floating_point.hpp"', "#include <array>", "namespace cm::aot {",
            f"static constexpr std::array<std::uint64_t, {words}> leaf_pages{{" +
            ", ".join(f"0x{word:x}" for word in bitmap) + "};",
            "bool is_leaf_entry(std::uint32_t address) {",
            "switch (address) {"] + [f"case 0x{target:x}:" for target in sorted(leaves)] + [
            "return true;", "default: return false;", "}", "}",
            "bool run_leaf(State& s) {",
            f"const auto page = (s.pc - 0x{base:x}u) >> 12;",
            "if (page >= leaf_pages.size() * 64 || !((leaf_pages[page / 64] >> (page % 64)) & 1)) return false;",
            "switch (s.pc) {"]
    for target in sorted(leaves):
        body = leaves[target]
        code.append(f"case 0x{target:x}: {{")
        # Runs split where the page splits them: a watched instruction runs
        # alone, so the PC is its own (HOST_OBSERVED_INSTRUCTIONS).
        addresses = [target + 4 * n for n in range(len(body) + 1)]
        starts = [n for n, address in enumerate(addresses)
                  if n == 0 or address in HOST_OBSERVED_INSTRUCTIONS or address - 4 in HOST_OBSERVED_INSTRUCTIONS]
        for start, end in zip(starts, starts[1:] + [len(addresses)]):
            code.append(f"s.step(0x{addresses[start]:x});" + (f" s.charge({end - start - 1});" if end - start > 1 else ""))
            code.extend(leaf_body(body[start:min(end, len(body))], userops))
        code += ["s.pc = std::uint32_t(s.r<4>(4128));", "return true;", "}"]
    code += ["default: return false;", "}", "}", "}"]
    return "\n".join(code) + "\n"


class Inlining:
    """Leaves to inline at direct call sites, and what generation saw: the
    inlined leaves and every address with a hook or special lowering."""
    def __init__(self, leaves):
        self.leaves, self.inlined, self.special = leaves, {}, set()

    def require_plain(self):
        """Fails if a leaf (inlined, or run directly by run_leaf) has an
        instruction with a hook or special lowering."""
        for target, body in self.leaves.items():
            addresses = range(target, target + 4 * len(body) + 4, 4)
            special = [address for address in addresses if address in self.special]
            if special:
                raise ValueError(f"Inlined leaf {target:x} has a hook or special lowering at {special[0]:x}")


def account_blocks(code, instructions, counter, forced, isolated=frozenset()):
    """Charge each straight-line run of instructions once, at its first one.

    Instruction counts are diagnostics (decided 2026-10-20), and a step per
    instruction cost 16-29% of world processing. A run starts at the page's
    first instruction, after a gap, after any instruction that can transfer
    control, at a direct branch target, at any goto other than a
    fall-through, and at every instruction with a hook or observation
    (`forced`); an `isolated` instruction (special code after its body, or
    watched by host code) is a run of its own. Its first instruction keeps its
    step, which the hook audits
    look for, and charges the rest of the run with it. Entering a run in the
    middle through the page switch charges the remainder there, so complete
    runs keep exact counts. A fault inside a run leaves the whole run charged
    and the PC at its first instruction; a finite budget can be overrun by
    less than one run, including any leaf inlined into it.
    """
    addresses = [item["address"] for item in instructions]
    index = {address: i for i, address in enumerate(addresses)}
    leaders = {address for address in forced | isolated if address in index}
    leaders.update(address + instructions[index[address]]["size"] for address in isolated if address in index)
    for i, item in enumerate(instructions):
        address = item["address"]
        if i == 0 or addresses[i - 1] + instructions[i - 1]["size"] != address:
            leaders.add(address)
        if ends_run(item) and address + item["size"] in index:
            leaders.add(address + item["size"])
        leaders.update(target for target in item.get("flows", ()) if target in index)
    label = re.compile(r"^i_([0-9a-f]+): //")
    goto = re.compile(r"goto i_([0-9a-f]+);")
    current = None
    for line in code:
        if match := label.match(line):
            current = int(match.group(1), 16)
        elif current is not None:
            fall_through = current + instructions[index[current]]["size"]
            leaders.update(target for target in map(lambda t: int(t, 16), goto.findall(line))
                           if target != fall_through and target in index)
    remaining, run = {}, []
    for address in addresses + [None]:
        if address is None or (address in leaders and run):
            remaining.update((member, len(run) - k) for k, member in enumerate(run))
            run = []
        if address is not None:
            run.append(address)
    step, charge = ("counter.step", "counter.charge") if counter else ("s.step", "s.charge")
    case = re.compile(r"^case 0x([0-9a-f]+): goto i_\1;$")
    result, current, stepped = [], None, set()
    for line in code:
        if match := label.match(line):
            current = int(match.group(1), 16)
        elif current is None and (match := case.match(line)) and int(match.group(1), 16) not in leaders:
            address = int(match.group(1), 16)
            rest = f" {charge}({remaining[address] - 1});" if remaining[address] > 1 else ""
            line = f"case 0x{address:x}: {step}(0x{address:x});{rest} goto i_{address:x};"
        elif current is not None and line == f"{step}(0x{current:x});":
            stepped.add(current)
            if current not in leaders:
                continue
            if remaining[current] > 1:
                line += f" {charge}({remaining[current] - 1});"
        result.append(line)
    if stepped != set(addresses):
        missing = sorted(set(addresses) - stepped)
        raise ValueError(f"Block accounting found no step line for {len(missing)} instructions, first {missing[0]:x}")
    return result


def generate_page(page, instructions, selected, local_temporaries=True, userops=None, guest_context_transfers=True, exception_runtime_hooks=True, compiler_arithmetic_hooks=True, register_context_hooks=True, stdlib_core_hooks=True, time_runtime_hooks=True, import_descriptor_hooks=True, native_service_hooks=True, engine_initialization_hooks=True, utility_object_hooks=True, tree_mutation_hooks=True, aggregate_object_hooks=True, record_group_hooks=True, record_query_hooks=True, record_history_hooks=True, match_engine_hooks=True, match_actor_hooks=True, actor_construction_hooks=True, spatial_role_hooks=True, marking_pressure_hooks=True, player_decision_hooks=True, actor_motion_hooks=True, player_movement_hooks=True, match_discipline_hooks=True, match_random_hooks=True, team_spatial_hooks=True, pitch_distance_hooks=True, actor_order_hooks=True, movement_choice_hooks=True, team_line_hooks=True, ball_action_hooks=True, ball_contact_hooks=True, actor_attributes_hooks=True, injury_record_hooks=True, injury_progression_hooks=True, injury_club_events_hooks=True, injury_readouts_hooks=True, injury_reporting_hooks=True, injury_bootstrap_hooks=True, injury_persistence_hooks=True, injury_daily_hooks=True, awol_records_hooks=True, contract_query_hooks=True, contract_storage_hooks=True, contract_terms_hooks=True, contract_review_hooks=True, transfer_offer_hooks=True, transfer_record_hooks=True, transfer_valuation_hooks=True, transfer_synthesis_hooks=True, club_shortlist_storage_hooks=True, shortlist_category_hooks=True, shortlist_links_hooks=True, shortlist_recruitment_hooks=True, shortlist_assessment_hooks=True, shortlist_quotation_hooks=True, shortlist_policy_hooks=True, profile_qualification_hooks=True, contract_draft_hooks=True, contract_display_hooks=True, money_value_hooks=True, money_text_hooks=True, money_configuration_hooks=True, preferences_hooks=True, preferences_actions_hooks=True, preferences_screen_hooks=True, localization_lookup_hooks=True, localization_format_hooks=True, localization_catalog_hooks=True, range_sampling_hooks=True, match_target_hooks=True, match_selection_hooks=True, match_selection_support_hooks=True, actor_encounter_hooks=True, match_event_hooks=True, actor_flags_hooks=True, actor_effects_hooks=True, target_acceptance_hooks=True, actor_action_hooks=True, actor_policy_hooks=True, native_record_field_hooks=True, native_sort8_hooks=True, native_leaf_lowering=False, block_accounting=False, inlining=None, nontrapping_divisions=None, list_search_hooks=True,
                  direct_owner_calls=None):
    called_families = set()
    require_native_drawing_contract(instructions)
    require_calendar_contract(page, instructions)
    require_rng_core_contract(page, instructions)
    require_background_selection_contract(page, instructions)
    require_collection_lookup_contract(page, instructions)
    require_string_core_contract(page, instructions)
    require_memory_primitives_contract(page, instructions)
    require_integer_parsing_contract(page, instructions)
    require_formatted_input_contract(page, instructions)
    require_character_conversion_contract(page, instructions)
    require_floating_input_contract(page, instructions)
    require_formatted_output_contract(page, instructions)
    require_allocator_contract(page, instructions)
    require_stream_buffer_contract(page, instructions)
    require_stream_io_contract(page, instructions)
    require_file_controller_contract(page, instructions)
    require_file_path_contract(page, instructions)
    require_runtime_lifecycle_contract(page, instructions)
    require_code_fragment_contract(page, instructions)
    require_cpp_object_storage_contract(page, instructions)
    require_exception_runtime_contract(page, instructions)
    if compiler_arithmetic_hooks:
        require_compiler_arithmetic_contract(page, instructions, selected)
    if register_context_hooks:
        require_register_context_contract(page, instructions, selected)
    if stdlib_core_hooks:
        require_stdlib_core_contract(page, instructions, selected)
    if time_runtime_hooks:
        require_time_runtime_contract(page, instructions, selected)
    if import_descriptor_hooks:
        require_import_descriptor_contract(page, instructions, selected)
    if engine_initialization_hooks:
        require_engine_initialization_contract(page, instructions, selected)
    if record_history_hooks:
        require_record_history_contract(page, instructions, selected)
    if match_engine_hooks:
        require_match_engine_contract(page, instructions, selected)
    if match_actor_hooks:
        require_match_actor_contract(page, instructions, selected)
    if actor_construction_hooks:
        require_actor_construction_contract(page, instructions, selected)
    if spatial_role_hooks:
        require_spatial_role_contract(page, instructions, selected)
    if marking_pressure_hooks:
        require_marking_pressure_contract(page, instructions, selected)
    if match_random_hooks:
        require_match_random_contract(page, instructions, selected)
    if actor_attributes_hooks:
        require_actor_attributes_contract(page, instructions, selected)
    if injury_record_hooks:
        require_injury_record_contract(page, instructions, selected)
    if injury_progression_hooks:
        require_injury_progression_contract(page, instructions, selected)
    if injury_club_events_hooks:
        require_injury_club_events_contract(page, instructions, selected)
    if injury_readouts_hooks:
        require_injury_readouts_contract(page, instructions, selected)
    if injury_reporting_hooks:
        require_injury_reporting_contract(page, instructions, selected)
    if injury_bootstrap_hooks:
        require_injury_bootstrap_contract(page, instructions, selected)
    if injury_persistence_hooks:
        require_injury_persistence_contract(page, instructions, selected)
    if injury_daily_hooks:
        require_injury_daily_contract(page, instructions, selected)
    if awol_records_hooks:
        require_awol_records_contract(page, instructions, selected)
    if contract_query_hooks:
        require_contract_query_contract(page, instructions, selected)
    if contract_storage_hooks:
        require_contract_storage_contract(page, instructions, selected)
    if contract_terms_hooks:
        require_contract_terms_contract(page, instructions, selected)
    if contract_review_hooks:
        require_contract_review_contract(page, instructions, selected)
    if transfer_offer_hooks:
        require_transfer_offer_contract(page, instructions, selected)
    if transfer_record_hooks:
        require_transfer_record_contract(page, instructions, selected)
    if transfer_valuation_hooks:
        require_transfer_valuation_contract(page, instructions, selected)
    if transfer_synthesis_hooks:
        require_transfer_synthesis_contract(page, instructions, selected)
    if club_shortlist_storage_hooks:
        require_club_shortlist_storage_contract(page, instructions, selected)
    if shortlist_category_hooks:
        require_shortlist_category_contract(page, instructions, selected)
    if shortlist_links_hooks:
        require_shortlist_links_contract(page, instructions, selected)
    if shortlist_recruitment_hooks:
        require_shortlist_recruitment_contract(page, instructions, selected)
    if shortlist_assessment_hooks:
        require_shortlist_assessment_contract(page, instructions, selected)
    if shortlist_quotation_hooks:
        require_shortlist_quotation_contract(page, instructions, selected)
    if shortlist_policy_hooks:
        require_shortlist_policy_contract(page, instructions, selected)
    if profile_qualification_hooks:
        require_profile_qualification_contract(page, instructions, selected)
    if contract_draft_hooks:
        require_contract_draft_contract(page, instructions, selected)
    if contract_display_hooks:
        require_contract_display_contract(page, instructions, selected)
    if money_value_hooks:
        require_money_value_contract(page, instructions, selected)
    if money_text_hooks:
        require_money_text_contract(page, instructions, selected)
    if money_configuration_hooks:
        require_money_configuration_contract(page, instructions, selected)
    if preferences_hooks:
        require_preferences_contract(page, instructions, selected)
    if preferences_actions_hooks:
        require_preferences_actions_contract(page, instructions, selected)
    if preferences_screen_hooks:
        require_preferences_screen_contract(page, instructions, selected)
    if localization_lookup_hooks:
        require_localization_lookup_contract(page, instructions, selected)
    if localization_format_hooks:
        require_localization_format_contract(page, instructions, selected)
    if localization_catalog_hooks:
        require_localization_catalog_contract(page, instructions, selected)
    if range_sampling_hooks:
        require_range_sampling_contract(page, instructions, selected)
    if match_target_hooks:
        require_match_target_contract(page, instructions, selected)
    if match_selection_hooks:
        require_match_selection_contract(page, instructions, selected)
    if match_selection_support_hooks:
        require_match_selection_support_contract(page, instructions, selected)
    if actor_encounter_hooks:
        require_actor_encounter_contract(page, instructions, selected)
    if match_event_hooks:
        require_match_event_contract(page, instructions, selected)
    if actor_flags_hooks:
        require_actor_flags_contract(page, instructions, selected)
    if actor_effects_hooks:
        require_actor_effects_contract(page, instructions, selected)
    if actor_action_hooks:
        require_actor_action_contract(page, instructions, selected)
    if actor_policy_hooks:
        require_actor_policy_contract(page, instructions, selected)
    if target_acceptance_hooks:
        require_target_acceptance_contract(page, instructions, selected)
    if team_spatial_hooks:
        require_team_spatial_contract(page, instructions, selected)
    if pitch_distance_hooks:
        require_pitch_distance_contract(page, instructions, selected)
    if actor_order_hooks:
        require_actor_order_contract(page, instructions, selected)
    if movement_choice_hooks:
        require_movement_choice_contract(page, instructions, selected)
    if team_line_hooks:
        require_team_line_contract(page, instructions, selected)
    if ball_action_hooks:
        require_ball_action_contract(page, instructions, selected)
    if ball_contact_hooks:
        require_ball_contact_contract(page, instructions, selected)
    if match_discipline_hooks:
        require_match_discipline_contract(page, instructions, selected)
    if player_movement_hooks:
        require_player_movement_contract(page, instructions, selected)
    if actor_motion_hooks:
        require_actor_motion_contract(page, instructions, selected)
    if player_decision_hooks:
        require_player_decision_contract(page, instructions, selected)
    if record_query_hooks:
        require_record_query_contract(page, instructions, selected)
    if record_group_hooks:
        require_record_group_contract(page, instructions, selected)
    if aggregate_object_hooks:
        require_aggregate_object_contract(page, instructions, selected)
    if tree_mutation_hooks:
        require_tree_mutation_contract(page, instructions, selected)
    if utility_object_hooks:
        require_utility_object_contract(page, instructions, selected)
    if native_service_hooks:
        require_native_service_contract(page, instructions, selected)
    if local_temporaries:
        # Reject unsafe exports, including incoming scratch reads. Falling back
        # just for a consumer cannot recover a previous instruction's discarded
        # scratch writes. No completion manifest is emitted if any page fails.
        for item in instructions:
            LocalTemporaries.require_instruction_local_unique(item["ops"])
    # A bounded export may contain unrelated instructions from these pages.
    # Audit only when its commentary call/load is included; once included,
    # require the same complete context and fingerprint as the full export.
    page_addresses = {item["address"] for item in instructions}
    if nontrapping_divisions is None:
        # Equivalence references compile parts of pages: use the whole-program
        # proof, so they continue exactly where production does; without an
        # export, prove within the records generated.
        nontrapping_divisions = program_quotient_free_divisions()
    if nontrapping_divisions is None:
        records = {item["address"]: item for item in instructions}
        nontrapping_divisions = frozenset(
            address for address, item in records.items() if item["ops"] and item["ops"][0][0] == "INT_SDIV" and
            zero_divisor_quotient_unused(records.get, address, division_blocked_addresses()))
    transfer_hooks = guest_context_transfers and bool(page_addresses & {0x10772768, 0x10772774, 0x107727d4, 0x10772a94})
    if transfer_hooks:
        require_exception_runtime_contract(page, instructions)
    presentation_anchor = {0x101cc: 0x101cce38, 0x10203: 0x10203738}.get(page)
    if presentation_anchor in page_addresses or presentation_anchor in selected:
        require_match_presentation_context(page, instructions, selected)
    if page in MATCH_SPEED_PAGES:
        require_match_speed_context(page, instructions, selected)
    # Specialize accounting on selected UI/runtime pages while retaining
    # the exact finite-budget path used by validation and probes.
    match_hot_page = page in TURBO_PAGES
    transfer_page = None if native_leaf_lowering else page
    code = ['#include "aot_runtime.hpp"', "namespace cm::aot {",
            (f"template <bool Turbo, bool Finite> bool page_{page:x}_impl(State& s) {{" if match_hot_page
             else f"bool page_{page:x}(State& s) {{")]
    if native_sort8_hooks:
        require_sort8_page(page, instructions, selected)
        if NATIVE_SORT8_ENTRY in page_addresses:
            code.insert(1, '#include "native_sort8.hpp"')
    if native_record_field_hooks and page_addresses & NATIVE_RECORD_FIELDS.keys():
        code.insert(1, '#include "native_record_fields.hpp"')
    if transfer_hooks:
        code.insert(1, '#include "guest_context_transfer.hpp"')
    if register_context_hooks and page_addresses & set(REGISTER_CONTEXT_ENTRIES):
        code.insert(1, '#include "register_context_adapter.hpp"')
        code.insert(1, '#include "guest_context_transfer.hpp"')
    if stdlib_core_hooks and page_addresses & set(STDLIB_CORE_ENTRIES):
        code.insert(1, '#include "stdlib_core_adapter.hpp"')
    if time_runtime_hooks and page_addresses & set(TIME_RUNTIME_ENTRIES):
        code.insert(1, '#include "time_runtime_adapter.hpp"')
    if import_descriptor_hooks and page_addresses & set(IMPORT_DESCRIPTOR_ENTRIES):
        code.insert(1, '#include "import_descriptor_adapter.hpp"')
    if engine_initialization_hooks and page_addresses & set(ENGINE_INITIALIZATION_ENTRIES):
        code.insert(1, '#include "engine_initialization_adapter.hpp"')
    if record_history_hooks and page_addresses & set(RECORD_HISTORY_ENTRIES):
        code.insert(1, '#include "record_history_adapter.hpp"')
    if match_engine_hooks and page_addresses & set(MATCH_ENGINE_ENTRIES):
        code.insert(1, '#include "match_engine_adapter.hpp"')
    if match_actor_hooks and page_addresses & set(MATCH_ACTOR_ENTRIES):
        code.insert(1, '#include "match_actor_adapter.hpp"')
    if actor_construction_hooks and page_addresses & set(ACTOR_CONSTRUCTION_ENTRIES):
        code.insert(1, '#include "actor_construction_adapter.hpp"')
    if spatial_role_hooks and page_addresses & set(SPATIAL_ROLE_ENTRIES):
        code.insert(1, '#include "spatial_role_adapter.hpp"')
    if marking_pressure_hooks and page_addresses & set(MARKING_PRESSURE_ENTRIES):
        code.insert(1, '#include "marking_pressure_adapter.hpp"')
    if match_random_hooks and page_addresses & set(MATCH_RANDOM_ENTRIES):
        code.insert(1, '#include "match_random_adapter.hpp"')
    if actor_attributes_hooks and page_addresses & set(ACTOR_ATTRIBUTES_ENTRIES):
        code.insert(1, '#include "actor_attributes_adapter.hpp"')
    if injury_record_hooks and page_addresses & set(INJURY_RECORD_ENTRIES):
        code.insert(1, '#include "injury_record_adapter.hpp"')
    if injury_progression_hooks and page_addresses & set(INJURY_PROGRESSION_ENTRIES):
        code.insert(1, '#include "injury_progression_adapter.hpp"')
    if injury_club_events_hooks and page_addresses & set(INJURY_CLUB_EVENTS_ENTRIES):
        code.insert(1, '#include "injury_club_events_adapter.hpp"')
    if injury_readouts_hooks and page_addresses & set(INJURY_READOUTS_ENTRIES):
        code.insert(1, '#include "injury_readouts_adapter.hpp"')
    if injury_reporting_hooks and page_addresses & set(INJURY_REPORTING_ENTRIES):
        code.insert(1, '#include "injury_reporting_adapter.hpp"')
    if injury_bootstrap_hooks and page_addresses & set(INJURY_BOOTSTRAP_ENTRIES):
        code.insert(1, '#include "injury_bootstrap_adapter.hpp"')
    if injury_persistence_hooks and page_addresses & set(INJURY_PERSISTENCE_ENTRIES):
        code.insert(1, '#include "injury_persistence_adapter.hpp"')
    if injury_daily_hooks and page_addresses & set(INJURY_DAILY_ENTRIES):
        code.insert(1, '#include "injury_daily_adapter.hpp"')
    if awol_records_hooks and page_addresses & set(AWOL_RECORDS_ENTRIES):
        code.insert(1, '#include "awol_records_adapter.hpp"')
    if contract_query_hooks and page_addresses & set(CONTRACT_QUERY_ENTRIES):
        code.insert(1, '#include "contract_query_adapter.hpp"')
    if contract_storage_hooks and page_addresses & set(CONTRACT_STORAGE_ENTRIES):
        code.insert(1, '#include "contract_storage_adapter.hpp"')
    if contract_terms_hooks and page_addresses & set(CONTRACT_TERMS_ENTRIES):
        code.insert(1, '#include "contract_terms_adapter.hpp"')
    if contract_review_hooks and page_addresses & set(CONTRACT_REVIEW_ENTRIES):
        code.insert(1, '#include "contract_review_adapter.hpp"')
    if transfer_offer_hooks and page_addresses & set(TRANSFER_OFFER_ENTRIES):
        code.insert(1, '#include "transfer_offer_adapter.hpp"')
    if transfer_record_hooks and page_addresses & set(TRANSFER_RECORD_ENTRIES):
        code.insert(1, '#include "transfer_record_adapter.hpp"')
    if transfer_valuation_hooks and page_addresses & set(TRANSFER_VALUATION_ENTRIES):
        code.insert(1, '#include "transfer_valuation_adapter.hpp"')
    if transfer_synthesis_hooks and page_addresses & set(TRANSFER_SYNTHESIS_ENTRIES):
        code.insert(1, '#include "transfer_synthesis_adapter.hpp"')
    if club_shortlist_storage_hooks and page_addresses & set(CLUB_SHORTLIST_STORAGE_ENTRIES):
        code.insert(1, '#include "club_shortlist_storage_adapter.hpp"')
    if shortlist_category_hooks and page_addresses & set(SHORTLIST_CATEGORY_ENTRIES):
        code.insert(1, '#include "shortlist_category_adapter.hpp"')
    if shortlist_links_hooks and page_addresses & set(SHORTLIST_LINKS_ENTRIES):
        code.insert(1, '#include "shortlist_links_adapter.hpp"')
    if shortlist_recruitment_hooks and page_addresses & set(SHORTLIST_RECRUITMENT_ENTRIES):
        code.insert(1, '#include "shortlist_recruitment_adapter.hpp"')
    if shortlist_assessment_hooks and page_addresses & set(SHORTLIST_ASSESSMENT_ENTRIES):
        code.insert(1, '#include "shortlist_assessment_adapter.hpp"')
    if shortlist_quotation_hooks and page_addresses & set(SHORTLIST_QUOTATION_ENTRIES):
        code.insert(1, '#include "shortlist_quotation_adapter.hpp"')
    if shortlist_policy_hooks and page_addresses & set(SHORTLIST_POLICY_ENTRIES):
        code.insert(1, '#include "shortlist_policy_adapter.hpp"')
    if profile_qualification_hooks and page_addresses & set(PROFILE_QUALIFICATION_ENTRIES):
        code.insert(1, '#include "profile_qualification_adapter.hpp"')
    if contract_draft_hooks and page_addresses & set(CONTRACT_DRAFT_ENTRIES):
        code.insert(1, '#include "contract_draft_adapter.hpp"')
    if contract_display_hooks and page_addresses & set(CONTRACT_DISPLAY_ENTRIES):
        code.insert(1, '#include "contract_display_adapter.hpp"')
    if money_value_hooks and page_addresses & set(MONEY_VALUE_ENTRIES):
        code.insert(1, '#include "money_value_adapter.hpp"')
    if money_text_hooks and page_addresses & set(MONEY_TEXT_ENTRIES):
        code.insert(1, '#include "money_text_adapter.hpp"')
    if money_configuration_hooks and page_addresses & set(MONEY_CONFIGURATION_ENTRIES):
        code.insert(1, '#include "money_configuration_adapter.hpp"')
    if preferences_hooks and page_addresses & set(PREFERENCES_ENTRIES):
        code.insert(1, '#include "preferences_adapter.hpp"')
    if preferences_actions_hooks and page_addresses & set(PREFERENCES_ACTIONS_ENTRIES):
        code.insert(1, '#include "preferences_actions_adapter.hpp"')
    if preferences_screen_hooks and page_addresses & set(PREFERENCES_SCREEN_ENTRIES):
        code.insert(1, '#include "preferences_screen_adapter.hpp"')
    if localization_lookup_hooks and page_addresses & set(LOCALIZATION_LOOKUP_ENTRIES):
        code.insert(1, '#include "localization_lookup_adapter.hpp"')
    if localization_format_hooks and page_addresses & set(LOCALIZATION_FORMAT_ENTRIES):
        code.insert(1, '#include "localization_format_adapter.hpp"')
    if localization_catalog_hooks and page_addresses & set(LOCALIZATION_CATALOG_ENTRIES):
        code.insert(1, '#include "localization_catalog_adapter.hpp"')
    if range_sampling_hooks and page_addresses & set(RANGE_SAMPLING_ENTRIES):
        code.insert(1, '#include "range_sampling_adapter.hpp"')
    if match_target_hooks and page_addresses & set(MATCH_TARGET_ENTRIES):
        code.insert(1, '#include "match_target_adapter.hpp"')
    if match_selection_hooks and page_addresses & set(MATCH_SELECTION_ENTRIES):
        code.insert(1, '#include "match_selection_adapter.hpp"')
    if match_selection_support_hooks and page_addresses & set(MATCH_SELECTION_SUPPORT_ENTRIES):
        code.insert(1, '#include "match_selection_support_adapter.hpp"')
    if actor_encounter_hooks and page_addresses & set(ACTOR_ENCOUNTER_ENTRIES):
        code.insert(1, '#include "actor_encounter_adapter.hpp"')
    if match_event_hooks and page_addresses & set(MATCH_EVENT_ENTRIES):
        code.insert(1, '#include "match_event_adapter.hpp"')
    if actor_flags_hooks and page_addresses & set(ACTOR_FLAGS_ENTRIES):
        code.insert(1, '#include "actor_flags_adapter.hpp"')
    if actor_effects_hooks and page_addresses & set(ACTOR_EFFECTS_ENTRIES):
        code.insert(1, '#include "actor_effects_adapter.hpp"')
    if actor_action_hooks and page_addresses & set(ACTOR_ACTION_ENTRIES):
        code.insert(1, '#include "actor_action_adapter.hpp"')
    if actor_policy_hooks and page_addresses & set(ACTOR_POLICY_ENTRIES):
        code.insert(1, '#include "actor_policy_adapter.hpp"')
    if target_acceptance_hooks and page_addresses & set(TARGET_ACCEPTANCE_ENTRIES):
        code.insert(1, '#include "target_acceptance_adapter.hpp"')
    if team_spatial_hooks and page_addresses & set(TEAM_SPATIAL_ENTRIES):
        code.insert(1, '#include "team_spatial_adapter.hpp"')
    if pitch_distance_hooks and page_addresses & set(PITCH_DISTANCE_ENTRIES):
        code.insert(1, '#include "pitch_distance_adapter.hpp"')
    if actor_order_hooks and page_addresses & set(ACTOR_ORDER_ENTRIES):
        code.insert(1, '#include "actor_order_adapter.hpp"')
    if movement_choice_hooks and page_addresses & set(MOVEMENT_CHOICE_ENTRIES):
        code.insert(1, '#include "movement_choice_adapter.hpp"')
    if team_line_hooks and page_addresses & set(TEAM_LINE_ENTRIES):
        code.insert(1, '#include "team_line_adapter.hpp"')
    if ball_action_hooks and page_addresses & set(BALL_ACTION_ENTRIES):
        code.insert(1, '#include "ball_action_adapter.hpp"')
    if ball_contact_hooks and page_addresses & set(BALL_CONTACT_ENTRIES):
        code.insert(1, '#include "ball_contact_adapter.hpp"')
    if match_discipline_hooks and page_addresses & set(MATCH_DISCIPLINE_ENTRIES):
        code.insert(1, '#include "match_discipline_adapter.hpp"')
    if player_movement_hooks and page_addresses & set(PLAYER_MOVEMENT_ENTRIES):
        code.insert(1, '#include "player_movement_adapter.hpp"')
    if actor_motion_hooks and page_addresses & set(ACTOR_MOTION_ENTRIES):
        code.insert(1, '#include "actor_motion_adapter.hpp"')
    if player_decision_hooks and page_addresses & set(PLAYER_DECISION_ENTRIES):
        code.insert(1, '#include "player_decision_adapter.hpp"')
    if record_query_hooks and page_addresses & set(RECORD_QUERY_ENTRIES):
        code.insert(1, '#include "record_query_adapter.hpp"')
    if record_group_hooks and page_addresses & set(RECORD_GROUP_ENTRIES):
        code.insert(1, '#include "record_group_adapter.hpp"')
    if aggregate_object_hooks and page_addresses & set(AGGREGATE_OBJECT_ENTRIES):
        code.insert(1, '#include "aggregate_object_adapter.hpp"')
    if tree_mutation_hooks and page_addresses & set(TREE_MUTATION_ENTRIES):
        code.insert(1, '#include "tree_mutation_adapter.hpp"')
    if utility_object_hooks and page_addresses & set(UTILITY_OBJECT_ENTRIES):
        code.insert(1, '#include "utility_object_adapter.hpp"')
    if native_service_hooks and page_addresses & set(NATIVE_SERVICE_ENTRIES):
        code.insert(1, '#include "native_service_adapter.hpp"')
    if any(op[0] == 'CALLOTHER' for item in instructions for op in item['ops']):
        code.insert(1, '#include "vector_context.hpp"')
    if match_hot_page:
        if page == 0x10773 and (0x10773870 in page_addresses or 0x10773870 in selected):
            require_unsigned_division_routine(instructions, selected)
            if not compiler_arithmetic_hooks:
                code.insert(1, '#include "unsigned_division.hpp"')
                code.append("if constexpr (Turbo) if (!match_pc_profile_enabled() && try_fast_unsigned_division(s)) return true;")
        code.append("MatchPageCounter<Turbo, Finite> counter(s);")
    if page == 0x10203:
        code.insert(1, '#include "match_presentation.hpp"')
    if page in MATCH_SPEED_PAGES:
        code.insert(1, '#include "match_speed.hpp"')
    if page in (0x1028c, 0x1028d, 0x101cc):
        code.insert(1, '#include "match_debugging.hpp"')
    if page in (0x10331, 0x10715, 0x10717, 0x10718):
        code.insert(1, '#include "calendar_adapter.hpp"')
    if page in (0x10715, 0x10716, 0x10717, 0x10718):
        code.insert(1, '#include "date_subsystem_audit.hpp"')
    if page in (0x10747, 0x1077b):
        code.insert(1, '#include "rng_core_adapter.hpp"')
    if page in (0x10712, 0x10713):
        code.insert(1, '#include "background_selection_adapter.hpp"')
    if page in (0x1074d, 0x1074e):
        code.insert(1, '#include "collection_lookup_adapter.hpp"')
    if list_search_hooks and LIST_SEARCH_ENTRY in page_addresses:
        code.insert(1, '#include "list_search.hpp"')
    if any(a in selected for a in STRING_CORE_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "string_core_adapter.hpp"')
    if any(a in selected for a in MEMORY_PRIMITIVES_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "memory_primitives_adapter.hpp"')
    if any(a in selected for a in INTEGER_PARSING_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "integer_parsing_adapter.hpp"')
    if any(a in selected for a in FORMATTED_INPUT_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "formatted_input_adapter.hpp"')
    if any(a in selected for a in CHARACTER_CONVERSION_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "character_conversion_adapter.hpp"')
    if any(a in selected for a in FLOATING_INPUT_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "floating_input_adapter.hpp"')
    if any(a in selected for a in (*FORMATTED_OUTPUT_ENTRIES,0x10779ed8) if a >> 12 == page):
        code.insert(1, '#include "formatted_output_adapter.hpp"')
    if any(a in selected for a in ALLOCATOR_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "allocator_adapter.hpp"')
    if any(a in selected for a in STREAM_BUFFER_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "stream_buffer_adapter.hpp"')
    if any(a in selected for a in STREAM_IO_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "stream_io_adapter.hpp"')
    if any(a in selected for a in FILE_CONTROLLER_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "file_controller_adapter.hpp"')
    if any(a in selected for a in FILE_PATH_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "file_path_adapter.hpp"')
    if any(a in selected for a in RUNTIME_LIFECYCLE_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "runtime_lifecycle_adapter.hpp"')
    if any(a in selected for a in CODE_FRAGMENT_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "code_fragment_adapter.hpp"')
    if any(a in selected for a in CPP_OBJECT_STORAGE_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "cpp_object_storage_adapter.hpp"')
    if exception_runtime_hooks and any(a in selected for a in (*EXCEPTION_RUNTIME_ENTRIES, 0x107724f4) if a >> 12 == page):
        code.insert(1, '#include "exception_runtime_adapter.hpp"')
    if compiler_arithmetic_hooks and any(a in selected for a in COMPILER_ARITHMETIC_ENTRIES if a >> 12 == page):
        code.insert(1, '#include "compiler_arithmetic_adapter.hpp"')
    if page == 0x10119:
        code.insert(1, '#include "season_reference.hpp"')
    if any(a >> 12 == page for a in NATIVE_DRAWING_BOUNDARIES):
        code.insert(1, '#include "native_drawing.hpp"')
    code.append("switch (s.pc) {")
    if any(item["assembly"].startswith("fctiwz ") for item in instructions):
        code.insert(1, '#include "floating_point.hpp"')
    if any(fused_mnemonic(item) for item in instructions):
        code.insert(1, '#include "fused_multiply_add.hpp"')
    if match_hot_page:
        code.insert(1, '#include "match_turbo.hpp"')
        code.insert(1, '#include "match_replay.hpp"')
    if page == 0x1071a:
        code.insert(1, '#include "control_cursor.hpp"')
    if any(item["address"] == 0x10774c24 for item in instructions):
        code.insert(1, '#include "allocator_bridge.hpp"')
    for item in instructions:
        a = item["address"]
        code.append(f"case 0x{a:x}: goto i_{a:x};")
    code.extend(["default: return false;", "}"])
    hooked, isolated, special = set(), set(HOST_OBSERVED_INSTRUCTIONS), set()
    for item in instructions:
        a = item["address"]
        code.append(f"i_{a:x}: // {item['assembly']}")
        hooks_start = len(code)
        if native_sort8_hooks and a == NATIVE_SORT8_ENTRY:
            code.append("if (try_native_sort8(s)) return true;")
        if native_record_field_hooks and a in NATIVE_RECORD_FIELDS:
            code.append(f"if (try_native_record_fields(s, 0x{a:x})) return true;")
        if a == 0x1028d558:
            code.append("if (append_turbo_match_speed(s)) return true;")
        if a == 0x1028ce80:
            code.append("append_match_debugging_controls(s);")
        if a == 0x1028dc80:
            code.append("if (dispatch_match_debugging_control(s)) return true;")
        if a == 0x101cca40:
            code.append("reset_match_debugging();")
        if a in CALENDAR_ADAPTER_ENTRIES:
            code.append(f"if (try_modern_calendar(s, 0x{a:x})) return true;")
        if a in DATE_SUBSYSTEM_ENTRIES:
            code.append(f"if (try_modern_date_subsystem(s, 0x{a:x})) return true;")
        if a in NATIVE_DRAWING_BOUNDARIES:
            code.append(f"native_drawing_boundary(s, 0x{a:x});")
        if a in RNG_CORE_ENTRIES:
            code.append(f"if (try_modern_rng_core(s, 0x{a:x})) return true;")
        if a in BACKGROUND_SELECTION_ENTRIES:
            code.append(f"if (try_modern_background_selection(s, 0x{a:x})) return true;")
        if a in COLLECTION_LOOKUP_ENTRIES:
            code.append(f"if (try_modern_collection_lookup(s, 0x{a:x})) return true;")
        if a in STRING_CORE_ENTRIES:
            code.append(f"if (try_modern_string_core(s, 0x{a:x})) return true;")
        if a in MEMORY_PRIMITIVES_ENTRIES:
            code.append(f"if (try_modern_memory_primitives(s, 0x{a:x})) return true;")
        if a in INTEGER_PARSING_ENTRIES:
            code.append(f"if (try_modern_integer_parsing(s, 0x{a:x})) return true;")
        if a in FORMATTED_INPUT_ENTRIES:
            code.append(f"if (try_modern_formatted_input(s, 0x{a:x})) return true;")
        if a in CHARACTER_CONVERSION_ENTRIES:
            code.append(f"if (try_modern_character_conversion(s, 0x{a:x})) return true;")
        if a in FLOATING_INPUT_ENTRIES:
            code.append(f"if (try_modern_floating_input(s, 0x{a:x})) return true;")
        if a in FORMATTED_OUTPUT_ENTRIES:
            code.append(f"if (try_modern_formatted_output(s, 0x{a:x})) return true;")
        if a in ALLOCATOR_ENTRIES:
            code.append(f"if (try_modern_allocator(s, 0x{a:x})) return true;")
        if a in STREAM_BUFFER_ENTRIES:
            code.append(f"if (try_modern_stream_buffer(s, 0x{a:x})) return true;")
        if a in STREAM_IO_ENTRIES:
            code.append(f"if (try_modern_stream_io(s, 0x{a:x})) return true;")
        if a in FILE_CONTROLLER_ENTRIES:
            code.append(f"if (try_modern_file_controller(s, 0x{a:x})) return true;")
        if a in FILE_PATH_ENTRIES:
            code.append(f"if (try_modern_file_path(s, 0x{a:x})) return true;")
        if a in RUNTIME_LIFECYCLE_ENTRIES:
            code.append(f"if (try_modern_runtime_lifecycle(s, 0x{a:x})) return true;")
        if a in CODE_FRAGMENT_ENTRIES:
            code.append(f"if (try_modern_code_fragment(s, 0x{a:x})) return true;")
        if a in CPP_OBJECT_STORAGE_ENTRIES:
            code.append(f"if (try_modern_cpp_object_storage(s, 0x{a:x})) return true;")
        if exception_runtime_hooks and a in EXCEPTION_RUNTIME_ENTRIES:
            code.append(f"if (try_modern_exception_runtime(s, 0x{a:x})) return true;")
        if exception_runtime_hooks and a == 0x107724f4:
            code.append("if (try_modern_exception_unexpected_landing(s)) return true;")
        if compiler_arithmetic_hooks and a in COMPILER_ARITHMETIC_ENTRIES:
            code.append(compiler_arithmetic_hook(a))
        if register_context_hooks and a in REGISTER_CONTEXT_ENTRIES:
            code.append(register_context_hook(a))
        if stdlib_core_hooks and a in STDLIB_CORE_ENTRIES:
            code.append(stdlib_core_hook(a))
        if time_runtime_hooks and a in TIME_RUNTIME_ENTRIES:
            code.append(time_runtime_hook(a))
        if import_descriptor_hooks and a in IMPORT_DESCRIPTOR_ENTRIES:
            code.append(import_descriptor_hook(a))
        if engine_initialization_hooks and a in ENGINE_INITIALIZATION_ENTRIES:
            code.append(engine_initialization_hook(a))
        if record_history_hooks and a in RECORD_HISTORY_ENTRIES:
            code.append(record_history_hook(a))
        if match_engine_hooks and a in MATCH_ENGINE_ENTRIES:
            code.append(match_engine_hook(a))
        if match_actor_hooks and a in MATCH_ACTOR_ENTRIES:
            code.append(match_actor_hook(a))
        if actor_construction_hooks and a in ACTOR_CONSTRUCTION_ENTRIES:
            code.append(actor_construction_hook(a))
        if spatial_role_hooks and a in SPATIAL_ROLE_ENTRIES:
            code.append(spatial_role_hook(a))
        if marking_pressure_hooks and a in MARKING_PRESSURE_ENTRIES:
            code.append(marking_pressure_hook(a))
        if match_random_hooks and a in MATCH_RANDOM_ENTRIES:
            code.append(match_random_hook(a))
        if actor_attributes_hooks and a in ACTOR_ATTRIBUTES_ENTRIES:
            code.append(actor_attributes_hook(a))
        if injury_record_hooks and a in INJURY_RECORD_ENTRIES:
            code.append(injury_record_hook(a))
        if injury_progression_hooks and a in INJURY_PROGRESSION_ENTRIES:
            code.append(injury_progression_hook(a))
        if injury_club_events_hooks and a in INJURY_CLUB_EVENTS_ENTRIES:
            code.append(injury_club_events_hook(a))
        if injury_readouts_hooks and a in INJURY_READOUTS_ENTRIES:
            code.append(injury_readouts_hook(a))
        if injury_reporting_hooks and a in INJURY_REPORTING_ENTRIES:
            code.append(injury_reporting_hook(a))
        if injury_bootstrap_hooks and a in INJURY_BOOTSTRAP_ENTRIES:
            code.append(injury_bootstrap_hook(a))
        if injury_persistence_hooks and a in INJURY_PERSISTENCE_ENTRIES:
            code.append(injury_persistence_hook(a))
        if injury_daily_hooks and a in INJURY_DAILY_ENTRIES:
            code.append(injury_daily_hook(a))
        if awol_records_hooks and a in AWOL_RECORDS_ENTRIES:
            code.append(awol_records_hook(a))
        if contract_query_hooks and a in CONTRACT_QUERY_ENTRIES:
            code.append(contract_query_hook(a))
        if contract_storage_hooks and a in CONTRACT_STORAGE_ENTRIES:
            code.append(contract_storage_hook(a))
        if contract_terms_hooks and a in CONTRACT_TERMS_ENTRIES:
            code.append(contract_terms_hook(a))
        if contract_review_hooks and a in CONTRACT_REVIEW_ENTRIES:
            code.append(contract_review_hook(a))
        if transfer_offer_hooks and a in TRANSFER_OFFER_ENTRIES:
            code.append(transfer_offer_hook(a))
        if transfer_record_hooks and a in TRANSFER_RECORD_ENTRIES:
            code.append(transfer_record_hook(a))
        if transfer_valuation_hooks and a in TRANSFER_VALUATION_ENTRIES:
            code.append(transfer_valuation_hook(a))
        if transfer_synthesis_hooks and a in TRANSFER_SYNTHESIS_ENTRIES:
            code.append(transfer_synthesis_hook(a))
        if club_shortlist_storage_hooks and a in CLUB_SHORTLIST_STORAGE_ENTRIES:
            code.append(club_shortlist_storage_hook(a))
        if shortlist_category_hooks and a in SHORTLIST_CATEGORY_ENTRIES:
            code.append(shortlist_category_hook(a))
        if shortlist_links_hooks and a in SHORTLIST_LINKS_ENTRIES:
            code.append(shortlist_links_hook(a))
        if shortlist_recruitment_hooks and a in SHORTLIST_RECRUITMENT_ENTRIES:
            code.append(shortlist_recruitment_hook(a))
        if shortlist_assessment_hooks and a in SHORTLIST_ASSESSMENT_ENTRIES:
            code.append(shortlist_assessment_hook(a))
        if shortlist_quotation_hooks and a in SHORTLIST_QUOTATION_ENTRIES:
            code.append(shortlist_quotation_hook(a))
        if shortlist_policy_hooks and a in SHORTLIST_POLICY_ENTRIES:
            code.append(shortlist_policy_hook(a))
        if profile_qualification_hooks and a in PROFILE_QUALIFICATION_ENTRIES:
            code.append(profile_qualification_hook(a))
        if contract_draft_hooks and a in CONTRACT_DRAFT_ENTRIES:
            code.append(contract_draft_hook(a))
        if contract_display_hooks and a in CONTRACT_DISPLAY_ENTRIES:
            code.append(contract_display_hook(a))
        if money_value_hooks and a in MONEY_VALUE_ENTRIES:
            code.append(money_value_hook(a))
        if money_text_hooks and a in MONEY_TEXT_ENTRIES:
            code.append(money_text_hook(a))
        if money_configuration_hooks and a in MONEY_CONFIGURATION_ENTRIES:
            code.append(money_configuration_hook(a))
        if preferences_hooks and a in PREFERENCES_ENTRIES:
            code.append(preferences_hook(a))
        if preferences_actions_hooks and a in PREFERENCES_ACTIONS_ENTRIES:
            code.append(preferences_actions_hook(a))
        if preferences_screen_hooks and a in PREFERENCES_SCREEN_ENTRIES:
            code.append(preferences_screen_hook(a))
        if localization_lookup_hooks and a in LOCALIZATION_LOOKUP_ENTRIES:
            code.append(localization_lookup_hook(a))
        if localization_format_hooks and a in LOCALIZATION_FORMAT_ENTRIES:
            code.append(localization_format_hook(a))
        if localization_catalog_hooks and a in LOCALIZATION_CATALOG_ENTRIES:
            code.append(localization_catalog_hook(a))
        if range_sampling_hooks and a in RANGE_SAMPLING_ENTRIES:
            code.append(range_sampling_hook(a))
        if match_target_hooks and a in MATCH_TARGET_ENTRIES:
            code.append(match_target_hook(a))
        if match_selection_hooks and a in MATCH_SELECTION_ENTRIES:
            code.append(match_selection_hook(a))
        if match_selection_support_hooks and a in MATCH_SELECTION_SUPPORT_ENTRIES:
            code.append(match_selection_support_hook(a))
        if actor_encounter_hooks and a in ACTOR_ENCOUNTER_ENTRIES:
            code.append(actor_encounter_hook(a))
        if match_event_hooks and a in MATCH_EVENT_ENTRIES:
            code.append(match_event_hook(a))
        if actor_flags_hooks and a in ACTOR_FLAGS_ENTRIES:
            code.append(actor_flags_hook(a))
        if actor_effects_hooks and a in ACTOR_EFFECTS_ENTRIES:
            code.append(actor_effects_hook(a))
        if actor_action_hooks and a in ACTOR_ACTION_ENTRIES:
            code.append(actor_action_hook(a))
        if actor_policy_hooks and a in ACTOR_POLICY_ENTRIES:
            code.append(actor_policy_hook(a))
        if target_acceptance_hooks and a in TARGET_ACCEPTANCE_ENTRIES:
            code.append(target_acceptance_hook(a))
        if team_spatial_hooks and a in TEAM_SPATIAL_ENTRIES:
            code.append(team_spatial_hook(a))
        if pitch_distance_hooks and a in PITCH_DISTANCE_ENTRIES:
            code.append(pitch_distance_hook(a))
        if actor_order_hooks and a in ACTOR_ORDER_ENTRIES:
            code.append(actor_order_hook(a))
        if movement_choice_hooks and a in MOVEMENT_CHOICE_ENTRIES:
            code.append(movement_choice_hook(a))
        if team_line_hooks and a in TEAM_LINE_ENTRIES:
            code.append(team_line_hook(a))
        if ball_action_hooks and a in BALL_ACTION_ENTRIES:
            code.append(ball_action_hook(a))
        if ball_contact_hooks and a in BALL_CONTACT_ENTRIES:
            code.append(ball_contact_hook(a))
        if match_discipline_hooks and a in MATCH_DISCIPLINE_ENTRIES:
            code.append(match_discipline_hook(a))
        if player_movement_hooks and a in PLAYER_MOVEMENT_ENTRIES:
            code.append(player_movement_hook(a))
        if actor_motion_hooks and a in ACTOR_MOTION_ENTRIES:
            code.append(actor_motion_hook(a))
        if player_decision_hooks and a in PLAYER_DECISION_ENTRIES:
            code.append(player_decision_hook(a))
        if record_query_hooks and a in RECORD_QUERY_ENTRIES:
            code.append(record_query_hook(a))
        if record_group_hooks and a in RECORD_GROUP_ENTRIES:
            code.append(record_group_hook(a))
        if aggregate_object_hooks and a in AGGREGATE_OBJECT_ENTRIES:
            code.append(aggregate_object_hook(a))
        if tree_mutation_hooks and a in TREE_MUTATION_ENTRIES:
            code.append(tree_mutation_hook(a))
        if utility_object_hooks and a in UTILITY_OBJECT_ENTRIES:
            code.append(utility_object_hook(a))
        if native_service_hooks and a in NATIVE_SERVICE_ENTRIES:
            code.append(native_service_hook(a))
        if a == 0x10779ed8:
            code.append("if (try_modern_formatted_output_shared_tail(s)) return true;")
        if list_search_hooks and a == LIST_SEARCH_ENTRY:
            code.append("if (try_native_list_search(s)) return true;")
        if a == 0x101195e8:
            code.append("season_day_boundary(s, 0x101195e8);")
        if a == 0x1071a828:
            require_control_cursor_block(instructions, selected)
            code.append("if constexpr (Turbo) if (match_snapshot_requested()) {")
            code.append(f"counter.publish(0x{a:x}); capture_match_snapshot(s);")
            code.append("}")
            code.append("if constexpr (Turbo) {")
            code.append("next_control_cursor_entry(s, counter);")
            code.append("goto i_1071a864;")
            code.append("}")
        if len(code) > hooks_start or (register_context_hooks and a in CAPTURE_OBSERVATIONS):
            hooked.add(a)
            # Only an allowed owner's own hook may sit on an inlined leaf.
            owner_hooks = [OWNER_HOOK.fullmatch(line) for line in code[hooks_start:]]
            if a in CAPTURE_OBSERVATIONS or not all(
                    match and f"{match.group(1)}_adapter" in INLINE_OWNER_MODULES for match in owner_hooks):
                special.add(a)
        if match_hot_page:
            # The Turbo counter holds the count locally; see turbo_pages.py.
            code[hooks_start:] = [bound_turbo_hook(a, line) for line in code[hooks_start:]]
            if any(line.startswith("if (try_") for line in code[hooks_start:]):
                raise ValueError(f"Semantic hook at {a:x} bypasses the Turbo instruction counter")
        if direct_owner_calls is not None and a in direct_owner_entries():
            direct_owner_calls.entry_hooks[a] = code[hooks_start:]
        code.append(f"{'counter' if match_hot_page else 's'}.step(0x{a:x});")
        if register_context_hooks and a in CAPTURE_OBSERVATIONS:
            code.append(CAPTURE_OBSERVATIONS[a])
        if a == 0x101fefc8:
            code.append("s.memory.write<1>(std::uint32_t(s.r<4>(12) + 0x4070), native_match_speed_enabled() ? native_match_speed() : s.r<4>(20));")
            code.append(transfer(a + 4, transfer_page, selected))
            isolated.add(a)
            continue
        if a == 0x10774c24:
            # A stale largest-block hint can produce the same pool count after
            # a failed allocation, trapping the original replenishment loop.
            # Preserve every decreasing retry and all successful allocations;
            # only require progress on this verified failure branch.
            sequence = {
                0x10774c00: "bl 0x10774840", 0x10774c04: "or. r28,r3,r3",
                0x10774c08: "bne 0x10774c38", 0x10774c0c: "lwz r4,0x38(r1)",
                0x10774c10: "cmplwi r4,0x1c", 0x10774c14: "ble 0x10774c2c",
                0x10774c18: "lwzx r3,r31,r26", 0x10774c1c: "subi r4,r4,0x1c",
                0x10774c20: "addi r0,r3,0x4", 0x10774c24: "divwu r25,r4,r0",
                0x10774c28: "b 0x10774c30", 0x10774c2c: "li r25,0x0",
                0x10774c30: "cmplwi r25,0xa", 0x10774c34: "bge 0x10774be8"}
            by_address = {record["address"]: record for record in instructions}
            if any(by_address.get(address, {}).get("assembly") != assembly
                   for address, assembly in sequence.items()) or item["ops"] != [
                    ["INT_DIV", ["register", "100", 4], ["register", "16", 4], ["register", "0", 4]]]:
                raise ValueError("Pool retry no longer matches the audited failed-allocation loop")
            code.extend(["s.wr<4>(100, next_pool_attempt(s.r<4>(100), divide_unsigned(s.r<4>(16), s.r<4>(0))));",
                         transfer(a + 4, transfer_page, selected)])
            continue
        if a in nontrapping_divisions:
            # PowerPC divw does not trap on a zero divisor; its quotient is
            # undefined. zero_divisor_quotient_unused proved this quotient is
            # only multiplied by the same zero divisor and then overwritten,
            # so any quotient gives the same continuation. Nonzero divisors
            # and every unproven division keep the strict lowering.
            _, quotient, _, divisor = item["ops"][0]
            code.extend([f"if (s.r<4>({divisor[1]}) == 0) {{", f"s.wr<4>({quotient[1]}, 0);",
                         transfer(a + 4, transfer_page, selected), "}"])
        if a in (0x10713aec, 0x10713b3c):
            # Club and competition fallback callers (10713214 / 107133b8)
            # deliberately pass a null object. The shared selector 107139c0
            # ignores its descriptor in that case. Avoid only these audited
            # eager getter calls; nonnull lookup and other accesses are intact.
            getter = 0x100d5310 if a == 0x10713aec else 0x100d4fd0
            continuation = a + 4
            if item["assembly"] != f"bl 0x{getter:x}" or plain_direct_call(item) != getter:
                raise ValueError("Background call site no longer matches the audited release")
            code.extend(["if (s.r<4>(12) == 0) {", "s.wr<4>(2440, s.r<4>(8));",
                         f"s.wr<4>(4128, 0x{continuation:x});",
                         transfer(continuation, transfer_page, selected), "}"])
        leaves = inlining.leaves if inlining else {}
        target = item["flows"][0] if leaves and item.get("flow") == "UNCONDITIONAL_CALL" and item.get("flows") else None
        if target in leaves:
            if plain_direct_call(item) != target:
                raise ValueError(f"Unexpected direct call shape at {a:x}")
            # The bl's own effects (TOC save, link), then the leaf's body.
            for op in item["ops"][:2]:
                code.append(operation(op, a, 0, transfer_page, selected, 3, userops=userops))
            code.extend(leaf_body(leaves[target], userops, local_temporaries))
            code.append(f"{'counter' if match_hot_page else 's'}.charge({len(leaves[target]) + 1}); // inlined leaf 0x{target:x}")
            code.append(transfer(a + item["size"], transfer_page, selected))
            inlining.inlined[target] = [r["address"] for r in leaves[target]] + [target + 4 * len(leaves[target])]
            continue
        callee = plain_direct_call(item) if direct_owner_calls is not None and not match_hot_page else None
        family = direct_owner_entries().get(callee)
        if family and a not in SPECIAL_LOWERINGS:
            returned = a + item["size"]
            for op in item["ops"][:2]:
                code.append(operation(op, a, 0, transfer_page, selected, 3, userops=userops))
            code.append(f"s.pc = 0x{callee:x};")
            if returned >> 12 == page and returned in selected:
                code.append(f"if (try_modern_{family}(s, 0x{callee:x}) && continues_after_owner(s, 0x{returned:x})) "
                             f"goto i_{returned:x};")
            else:
                code.append(f"try_modern_{family}(s, 0x{callee:x});")
            code.append("return true;")
            called_families.add(family)
            direct_owner_calls.called[callee] = family
            continue
        if item["assembly"].startswith("fctiwz "):
            # Ghidra's generic TRUNC does not express PowerPC invalid-result
            # saturation or all FPSCR flags. Lower this instruction explicitly.
            operands = item["assembly"].split()[1].split(",")
            destination, source = [16384 + 16 * int(register.removeprefix("f")) for register in operands]
            ops = item["ops"]
            if [op[0] for op in ops] != ["COPY", "FLOAT_NAN", "INT_OR", "INT_OR", "TRUNC", "INT_SEXT"] or \
               ops[4][1][2] != 4 or ops[4][2] != ["register", str(source), 8] or \
               ops[5][1] != ["register", str(destination), 8]:
                raise ValueError(f"Unrecognized fctiwz export at {a:x}")
            code.append(f"original_fctiwz(s, {destination}, {source});")
            code.append(transfer(a + item["size"], transfer_page, selected))
            continue
        ops = fused_multiply_add_ops(item)
        labels = set()
        for i, op in enumerate(ops):
            if op[0] in ("BRANCH", "CBRANCH", "CALL") and op[2][0] == "const":
                offset, width = int(op[2][1]), op[2][2] * 8
                offset &= (1 << width) - 1
                if offset >= 1 << (width - 1): offset -= 1 << width
                labels.add(i + offset)
        local = LocalTemporaries.for_instruction(ops, read, write) if local_temporaries else None
        if local is not None:
            code.append("{")
        for i, op in enumerate(ops):
            if i in labels: code.append(f"op_{a:x}_{i}:")
            lowered = operation(op, a, i, transfer_page, selected, len(ops),
                                read_node=local.read if local else read,
                                write_node=local.write if local else write,
                                userops=userops)
            if transfer_hooks and a == 0x10772774 and op[0] == 'RETURN':
                if lowered.count('return true;') != 1:
                    raise ValueError('Missing original context-return boundary')
                lowered = lowered.replace('return true;', 'complete_context_transfer(s); return true;')
            code.append(lowered)
        if len(ops) in labels: code.append(f"op_{a:x}_{len(ops)}:")
        body_end = len(code)
        if transfer_hooks and a in (0x107727d4, 0x10772a94):
            code.append('record_context_function(s, std::uint32_t(s.r<4>(20)));')
        if transfer_hooks and a == 0x10772768:
            code.append('record_context_frame(s, std::uint32_t(s.r<4>(12)));')
        if a == 0x10203738:
            code.append("s.wr<4>(0, commentary_presentation_duration(s, std::uint32_t(s.r<4>(104)), std::uint32_t(s.r<4>(0))));")
        if a == 0x10147cf8:
            code.append("if (native_match_speed_enabled()) s.wr<4>(12, native_match_speed());")
        if a == 0x10147e50:
            code.append("select_native_match_speed(std::uint16_t(s.r<4>(16)));")
        if a in (0x101cce50, 0x10203574, 0x102036ac):
            reg = 20 if a == 0x102036ac else 0
            code.append(f"s.wr<4>({reg}, match_simulation_speed(std::uint32_t(s.r<4>({reg}))));")
        if len(code) > body_end:
            isolated.add(a)
        # This path is reached only if no earlier branch/return transferred out.
        code.append(transfer(a + item["size"], transfer_page, selected))
        if local is not None:
            code.append("}")
    if inlining is not None:
        inlining.special.update(special | (isolated - HOST_OBSERVED_INSTRUCTIONS) | (SPECIAL_LOWERINGS & page_addresses) | (nontrapping_divisions & page_addresses))
    if block_accounting:
        code = account_blocks(code, instructions, match_hot_page, hooked, isolated)
    code.append("}")
    if match_hot_page:
        code.extend([f"bool page_{page:x}(State& s) {{",
                     "if (match_turbo_enabled()) {",
                     "if (s.budget == UINT64_MAX) "
                     f"return page_{page:x}_impl<true, false>(s);",
                     "if (match_replay_turbo_enabled()) "
                     f"return page_{page:x}_impl<true, true>(s);",
                     "}",
                     f"return page_{page:x}_impl<false, false>(s);", "}"])
    code.append("}")
    for family in sorted(called_families):
        code.insert(1, f'#include "{family}_adapter.hpp"')
    if called_families:
        code.insert(1, '#include "direct_owner_call.hpp"')
    result = "\n".join(code) + "\n"
    require_integer_parsing_hooks(page, instructions, result)
    require_formatted_input_hooks(page, instructions, result)
    require_character_conversion_hooks(page, instructions, result)
    require_floating_input_hooks(page, instructions, result)
    require_formatted_output_hooks(page, instructions, result)
    require_allocator_hooks(page, instructions, result)
    require_stream_buffer_hooks(page, instructions, result)
    require_stream_io_hooks(page, instructions, result)
    require_file_controller_hooks(page, instructions, result)
    require_file_path_hooks(page, instructions, result)
    require_runtime_lifecycle_hooks(page, instructions, result)
    require_code_fragment_hooks(page, instructions, result)
    require_cpp_object_storage_hooks(page, instructions, result)
    if exception_runtime_hooks:
        require_exception_runtime_hooks(page, instructions, result)
    if compiler_arithmetic_hooks:
        require_compiler_arithmetic_hooks(page, instructions, result)
    if register_context_hooks:
        require_register_context_hooks(page, instructions, result)
    if stdlib_core_hooks:
        require_stdlib_core_hooks(page, instructions, result)
    if time_runtime_hooks:
        require_time_runtime_hooks(page, instructions, result)
    if import_descriptor_hooks:
        require_import_descriptor_hooks(page, instructions, result)
    if engine_initialization_hooks:
        require_engine_initialization_hooks(page, instructions, result)
    if record_history_hooks:
        require_record_history_hooks(page, instructions, result)
    if match_engine_hooks:
        require_match_engine_hooks(page, instructions, result)
    if match_actor_hooks:
        require_match_actor_hooks(page, instructions, result)
    if actor_construction_hooks:
        require_actor_construction_hooks(page, instructions, result)
    if spatial_role_hooks:
        require_spatial_role_hooks(page, instructions, result)
    if marking_pressure_hooks:
        require_marking_pressure_hooks(page, instructions, result)
    if match_random_hooks:
        require_match_random_hooks(page, instructions, result)
    if actor_attributes_hooks:
        require_actor_attributes_hooks(page, instructions, result)
    if injury_record_hooks:
        require_injury_record_hooks(page, instructions, result)
    if injury_progression_hooks:
        require_injury_progression_hooks(page, instructions, result)
    if injury_club_events_hooks:
        require_injury_club_events_hooks(page, instructions, result)
    if injury_readouts_hooks:
        require_injury_readouts_hooks(page, instructions, result)
    if injury_reporting_hooks:
        require_injury_reporting_hooks(page, instructions, result)
    if injury_bootstrap_hooks:
        require_injury_bootstrap_hooks(page, instructions, result)
    if injury_persistence_hooks:
        require_injury_persistence_hooks(page, instructions, result)
    if injury_daily_hooks:
        require_injury_daily_hooks(page, instructions, result)
    if awol_records_hooks:
        require_awol_records_hooks(page, instructions, result)
    if contract_query_hooks:
        require_contract_query_hooks(page, instructions, result)
    if contract_storage_hooks:
        require_contract_storage_hooks(page, instructions, result)
    if contract_terms_hooks:
        require_contract_terms_hooks(page, instructions, result)
    if contract_review_hooks:
        require_contract_review_hooks(page, instructions, result)
    if transfer_offer_hooks:
        require_transfer_offer_hooks(page, instructions, result)
    if transfer_record_hooks:
        require_transfer_record_hooks(page, instructions, result)
    if transfer_valuation_hooks:
        require_transfer_valuation_hooks(page, instructions, result)
    if transfer_synthesis_hooks:
        require_transfer_synthesis_hooks(page, instructions, result)
    if club_shortlist_storage_hooks:
        require_club_shortlist_storage_hooks(page, instructions, result)
    if shortlist_category_hooks:
        require_shortlist_category_hooks(page, instructions, result)
    if shortlist_links_hooks:
        require_shortlist_links_hooks(page, instructions, result)
    if shortlist_recruitment_hooks:
        require_shortlist_recruitment_hooks(page, instructions, result)
    if shortlist_assessment_hooks:
        require_shortlist_assessment_hooks(page, instructions, result)
    if shortlist_quotation_hooks:
        require_shortlist_quotation_hooks(page, instructions, result)
    if shortlist_policy_hooks:
        require_shortlist_policy_hooks(page, instructions, result)
    if profile_qualification_hooks:
        require_profile_qualification_hooks(page, instructions, result)
    if contract_draft_hooks:
        require_contract_draft_hooks(page, instructions, result)
    if contract_display_hooks:
        require_contract_display_hooks(page, instructions, result)
    if money_value_hooks:
        require_money_value_hooks(page, instructions, result)
    if money_text_hooks:
        require_money_text_hooks(page, instructions, result)
    if money_configuration_hooks:
        require_money_configuration_hooks(page, instructions, result)
    if preferences_hooks:
        require_preferences_hooks(page, instructions, result)
    if preferences_actions_hooks:
        require_preferences_actions_hooks(page, instructions, result)
    if preferences_screen_hooks:
        require_preferences_screen_hooks(page, instructions, result)
    if localization_lookup_hooks:
        require_localization_lookup_hooks(page, instructions, result)
    if localization_format_hooks:
        require_localization_format_hooks(page, instructions, result)
    if localization_catalog_hooks:
        require_localization_catalog_hooks(page, instructions, result)
    if range_sampling_hooks:
        require_range_sampling_hooks(page, instructions, result)
    if match_target_hooks:
        require_match_target_hooks(page, instructions, result)
    if match_selection_hooks:
        require_match_selection_hooks(page, instructions, result)
    if match_selection_support_hooks:
        require_match_selection_support_hooks(page, instructions, result)
    if actor_encounter_hooks:
        require_actor_encounter_hooks(page, instructions, result)
    if match_event_hooks:
        require_match_event_hooks(page, instructions, result)
    if actor_flags_hooks:
        require_actor_flags_hooks(page, instructions, result)
    if actor_effects_hooks:
        require_actor_effects_hooks(page, instructions, result)
    if actor_action_hooks:
        require_actor_action_hooks(page, instructions, result)
    if actor_policy_hooks:
        require_actor_policy_hooks(page, instructions, result)
    if target_acceptance_hooks:
        require_target_acceptance_hooks(page, instructions, result)
    if team_spatial_hooks:
        require_team_spatial_hooks(page, instructions, result)
    if pitch_distance_hooks:
        require_pitch_distance_hooks(page, instructions, result)
    if actor_order_hooks:
        require_actor_order_hooks(page, instructions, result)
    if movement_choice_hooks:
        require_movement_choice_hooks(page, instructions, result)
    if team_line_hooks:
        require_team_line_hooks(page, instructions, result)
    if ball_action_hooks:
        require_ball_action_hooks(page, instructions, result)
    if ball_contact_hooks:
        require_ball_contact_hooks(page, instructions, result)
    if match_discipline_hooks:
        require_match_discipline_hooks(page, instructions, result)
    if player_movement_hooks:
        require_player_movement_hooks(page, instructions, result)
    if actor_motion_hooks:
        require_actor_motion_hooks(page, instructions, result)
    if player_decision_hooks:
        require_player_decision_hooks(page, instructions, result)
    if record_query_hooks:
        require_record_query_hooks(page, instructions, result)
    if record_group_hooks:
        require_record_group_hooks(page, instructions, result)
    if aggregate_object_hooks:
        require_aggregate_object_hooks(page, instructions, result)
    if tree_mutation_hooks:
        require_tree_mutation_hooks(page, instructions, result)
    if utility_object_hooks:
        require_utility_object_hooks(page, instructions, result)
    if native_service_hooks:
        require_native_service_hooks(page, instructions, result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("export", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--root", action="append", type=integer, required=True)
    parser.add_argument("--limit", type=int, default=8192)
    parser.add_argument("--all-analyzed", action="store_true",
                        help="Stream every analyzed instruction into native compilation units")
    parser.add_argument("--legacy-unique-storage", action="store_true",
                        help="Materialize translation scratch for comparison with instruction-local temporaries")
    parser.add_argument("--reuse-generated", type=Path,
                        help="Seed an empty output with independent copies of prior C++; every unit is regenerated and audited")
    args = parser.parse_args()
    if args.limit < 1: raise ValueError("Instruction limit must be positive")
    metadata = json.loads((args.export / "program.json").read_text())
    if metadata["language"] != "PowerPC:BE:32:default":
        raise ValueError("Expected 32-bit big-endian PowerPC")
    if metadata["executableSHA256"] != "91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b":
        raise ValueError("This port's loader layout requires the audited CM 01/02 Mac binary")
    with sqlite3.connect(f"file:{args.export / 'instructions.sqlite'}?mode=ro", uri=True) as db:
        identity = db.execute("SELECT value FROM metadata WHERE key='executableSHA256'").fetchone()
        if identity != (metadata["executableSHA256"],): raise ValueError("Wrong instruction index")
        selected, missing, seen = {}, set(), set()
        queue = deque(args.root)
        while not args.all_analyzed and queue and len(selected) < args.limit:
            address = queue.popleft()
            if address in seen: continue
            seen.add(address)
            result = db.execute("SELECT body FROM instructions WHERE address=?", (address,)).fetchone()
            if not result:
                missing.add(address)
                continue
            item = json.loads(result[0])
            selected[address] = item
            queue.extend(item["flows"])
            if "fallthrough" in item: queue.append(item["fallthrough"])
        require_integer_parsing_index(db, None if args.all_analyzed else selected)
        require_formatted_input_index(db, None if args.all_analyzed else selected)
        require_character_conversion_index(db, None if args.all_analyzed else selected)
        require_floating_input_index(db, None if args.all_analyzed else selected)
        require_formatted_output_index(db, None if args.all_analyzed else selected)
        require_allocator_index(db, None if args.all_analyzed else selected)
        require_stream_buffer_index(db, None if args.all_analyzed else selected)
        require_stream_io_index(db, None if args.all_analyzed else selected)
        require_file_controller_index(db, None if args.all_analyzed else selected)
        require_file_path_index(db, None if args.all_analyzed else selected)
        require_runtime_lifecycle_index(db, None if args.all_analyzed else selected)
        require_code_fragment_index(db, None if args.all_analyzed else selected)
        require_cpp_object_storage_index(db, None if args.all_analyzed else selected)
        require_exception_runtime_index(db, None if args.all_analyzed else selected)
        require_compiler_arithmetic_index(db, None if args.all_analyzed else selected)
        require_register_context_index(db, None if args.all_analyzed else selected)
        require_stdlib_core_index(db, None if args.all_analyzed else selected)
        require_time_runtime_index(db, None if args.all_analyzed else selected)
        require_import_descriptor_index(db, None if args.all_analyzed else selected)
        require_native_service_index(db, None if args.all_analyzed else selected)
        require_engine_initialization_index(db, None if args.all_analyzed else selected)
        require_utility_object_index(db, None if args.all_analyzed else selected)
        require_tree_mutation_index(db, None if args.all_analyzed else selected)
        require_aggregate_object_index(db, None if args.all_analyzed else selected)
        require_record_history_index(db, None if args.all_analyzed else selected)
        require_match_engine_index(db, None if args.all_analyzed else selected)
        require_match_actor_index(db, None if args.all_analyzed else selected)
        require_actor_construction_index(db, None if args.all_analyzed else selected)
        require_spatial_role_index(db, None if args.all_analyzed else selected)
        require_marking_pressure_index(db, None if args.all_analyzed else selected)
        require_player_decision_index(db, None if args.all_analyzed else selected)
        require_actor_motion_index(db, None if args.all_analyzed else selected)
        require_match_random_index(db, None if args.all_analyzed else selected)
        require_actor_attributes_index(db, None if args.all_analyzed else selected)
        require_injury_record_index(db, None if args.all_analyzed else selected)
        require_injury_progression_index(db, None if args.all_analyzed else selected)
        require_injury_club_events_index(db, None if args.all_analyzed else selected)
        require_injury_readouts_index(db, None if args.all_analyzed else selected)
        require_injury_reporting_index(db, None if args.all_analyzed else selected)
        require_injury_bootstrap_index(db, None if args.all_analyzed else selected)
        require_injury_persistence_index(db, None if args.all_analyzed else selected)
        require_injury_daily_index(db, None if args.all_analyzed else selected)
        require_awol_records_index(db, None if args.all_analyzed else selected)
        require_contract_query_index(db, None if args.all_analyzed else selected)
        require_contract_storage_index(db, None if args.all_analyzed else selected)
        require_contract_terms_index(db, None if args.all_analyzed else selected)
        require_contract_review_index(db, None if args.all_analyzed else selected)
        require_transfer_offer_index(db, None if args.all_analyzed else selected)
        require_transfer_record_index(db, None if args.all_analyzed else selected)
        require_transfer_valuation_index(db, None if args.all_analyzed else selected)
        require_transfer_synthesis_index(db, None if args.all_analyzed else selected)
        require_club_shortlist_storage_index(db, None if args.all_analyzed else selected)
        require_shortlist_category_index(db, None if args.all_analyzed else selected)
        require_shortlist_links_index(db, None if args.all_analyzed else selected)
        require_shortlist_recruitment_index(db, None if args.all_analyzed else selected)
        require_shortlist_assessment_index(db, None if args.all_analyzed else selected)
        require_shortlist_quotation_index(db, None if args.all_analyzed else selected)
        require_shortlist_policy_index(db, None if args.all_analyzed else selected)
        require_profile_qualification_index(db, None if args.all_analyzed else selected)
        require_contract_draft_index(db, None if args.all_analyzed else selected)
        require_contract_display_index(db, None if args.all_analyzed else selected)
        require_money_value_index(db, None if args.all_analyzed else selected)
        require_money_text_index(db, None if args.all_analyzed else selected)
        require_money_configuration_index(db, None if args.all_analyzed else selected)
        require_preferences_index(db, None if args.all_analyzed else selected)
        require_preferences_actions_index(db, None if args.all_analyzed else selected)
        require_preferences_screen_index(db, None if args.all_analyzed else selected)
        require_localization_lookup_index(db, None if args.all_analyzed else selected)
        require_localization_format_index(db, None if args.all_analyzed else selected)
        require_localization_catalog_index(db, None if args.all_analyzed else selected)
        require_range_sampling_index(db, None if args.all_analyzed else selected)
        require_match_target_index(db, None if args.all_analyzed else selected)
        require_match_selection_index(db, None if args.all_analyzed else selected)
        require_match_selection_support_index(db, None if args.all_analyzed else selected)
        require_actor_encounter_index(db, None if args.all_analyzed else selected)
        require_match_event_index(db, None if args.all_analyzed else selected)
        require_actor_flags_index(db, None if args.all_analyzed else selected)
        require_actor_effects_index(db, None if args.all_analyzed else selected)
        require_target_acceptance_index(db, None if args.all_analyzed else selected)
        require_actor_action_index(db, None if args.all_analyzed else selected)
        require_actor_policy_index(db, None if args.all_analyzed else selected)
        require_team_spatial_index(db, None if args.all_analyzed else selected)
        require_pitch_distance_index(db, None if args.all_analyzed else selected)
        require_actor_order_index(db, None if args.all_analyzed else selected)
        require_movement_choice_index(db, None if args.all_analyzed else selected)
        require_team_line_index(db, None if args.all_analyzed else selected)
        require_ball_action_index(db, None if args.all_analyzed else selected)
        require_ball_contact_index(db, None if args.all_analyzed else selected)
        require_match_discipline_index(db, None if args.all_analyzed else selected)
        require_player_movement_index(db, None if args.all_analyzed else selected)
        if args.all_analyzed:
            from actor_attributes_audit import load_actor_attributes_tables
            load_actor_attributes_tables(args.export)
            from injury_record_audit import load_injury_record_tables
            load_injury_record_tables(args.export)
            from injury_progression_audit import load_injury_progression_tables
            load_injury_progression_tables(args.export)
            from injury_club_events_audit import load_injury_club_events_tables
            load_injury_club_events_tables(args.export)
            from injury_readouts_audit import load_injury_readouts_tables
            load_injury_readouts_tables(args.export)
            from injury_reporting_audit import load_injury_reporting_tables
            load_injury_reporting_tables(args.export)
            from injury_bootstrap_audit import load_injury_bootstrap_tables
            load_injury_bootstrap_tables(args.export)
            from injury_persistence_audit import load_injury_persistence_tables
            load_injury_persistence_tables(args.export)
            from injury_daily_audit import load_injury_daily_tables
            load_injury_daily_tables(args.export)
            from awol_records_audit import load_awol_records_tables
            load_awol_records_tables(args.export)
            from contract_storage_audit import load_contract_storage_tables
            load_contract_storage_tables(args.export)
            from contract_terms_audit import load_contract_terms_tables
            load_contract_terms_tables(args.export)
            from contract_review_audit import load_contract_review_tables
            load_contract_review_tables(args.export)
            from transfer_offer_audit import load_transfer_offer_tables
            load_transfer_offer_tables(args.export)
            from transfer_record_audit import load_transfer_record_tables
            load_transfer_record_tables(args.export)
            from transfer_valuation_audit import load_transfer_valuation_tables
            load_transfer_valuation_tables(args.export)
            from transfer_synthesis_audit import load_transfer_synthesis_tables
            load_transfer_synthesis_tables(args.export)
            from club_shortlist_storage_audit import load_club_shortlist_storage_tables
            load_club_shortlist_storage_tables(args.export)
            from shortlist_category_audit import load_shortlist_category_tables
            load_shortlist_category_tables(args.export)
            from shortlist_links_audit import load_shortlist_links_tables
            load_shortlist_links_tables(args.export)
            from shortlist_recruitment_audit import load_shortlist_recruitment_tables
            load_shortlist_recruitment_tables(args.export)
            from shortlist_assessment_audit import load_shortlist_assessment_tables
            load_shortlist_assessment_tables(args.export)
            from shortlist_quotation_audit import load_shortlist_quotation_tables
            load_shortlist_quotation_tables(args.export)
            from shortlist_policy_audit import load_shortlist_policy_tables
            load_shortlist_policy_tables(args.export)
            from profile_qualification_audit import load_profile_qualification_tables
            load_profile_qualification_tables(args.export)
            from contract_draft_audit import load_contract_draft_tables
            load_contract_draft_tables(args.export)
            from contract_display_audit import load_contract_display_tables
            load_contract_display_tables(args.export)
            from money_value_audit import load_money_value_tables
            load_money_value_tables(args.export)
            from money_text_audit import load_money_text_tables
            load_money_text_tables(args.export)
            from money_configuration_audit import load_money_configuration_tables
            load_money_configuration_tables(args.export)
            from preferences_audit import load_preferences_tables
            load_preferences_tables(args.export)
            from preferences_actions_audit import load_preferences_actions_tables
            load_preferences_actions_tables(args.export)
            from preferences_screen_audit import load_preferences_screen_tables
            load_preferences_screen_tables(args.export)
            from localization_lookup_audit import load_localization_lookup_tables
            load_localization_lookup_tables(args.export)
            from localization_format_audit import load_localization_format_tables
            load_localization_format_tables(args.export)
            from localization_catalog_audit import load_localization_catalog_tables
            load_localization_catalog_tables(args.export)
            from player_movement_audit import load_player_movement_tables
            load_player_movement_tables(args.export)
            from ball_action_audit import load_ball_action_tables
            load_ball_action_tables(args.export)
            from ball_contact_audit import load_ball_contact_tables
            load_ball_contact_tables(args.export)
            load_actor_action_table(args.export)
        require_record_query_index(db, None if args.all_analyzed else selected)
        require_record_group_index(db, None if args.all_analyzed else selected)
    with sqlite3.connect(f"file:{args.export / 'instructions.sqlite'}?mode=ro", uri=True) as db:
        native_leaves = load_leaves(db, None if args.all_analyzed else selected)
        native_sort = load_sort8(db, None if args.all_analyzed else selected)
        divisions = quotient_free_divisions(db)
        require_list_search(db)
        inlining = Inlining({target: body for target, body in inline_leaves(db, divisions).items()
                             if args.all_analyzed or all(address in selected for address in range(target, target + 4 * len(body) + 4, 4))})
    if args.reuse_generated:
        reused = seed_generated(args.reuse_generated, args.output)
        print(f"Seeded generated C++: {reused['clonedFiles']} copy-on-write files; {reused['copiedFiles']} regular copies", flush=True)
    args.output.mkdir(parents=True, exist_ok=True)
    # Invalidate completion before touching generated units, so interruption
    # cannot leave an old manifest claiming the new partial output is complete.
    (args.output / "compilation.json").unlink(missing_ok=True)
    pages = defaultdict(list)
    for address, item in sorted(selected.items()): pages[address >> 12].append(item)
    outputs = {}
    direct_calls = DirectOwnerCalls()
    call_targets = {item["flows"][0] for item in selected.values()
                    if item.get("flow") == "UNCONDITIONAL_CALL" and item.get("flows")}
    generated_count = len(selected)
    fused_sites = [(address, fused_site_kind(item)) for address, item in selected.items() if fused_mnemonic(item)]
    if args.all_analyzed:
        # Keep memory bounded to one code page. Game-derived instruction JSON
        # does not need to be held in a multi-gigabyte Python object graph.
        generated_count = 0
        with sqlite3.connect(f"file:{args.export / 'instructions.sqlite'}?mode=ro", uri=True) as db:
            page_ids = [row[0] for row in db.execute("SELECT DISTINCT address >> 12 FROM instructions ORDER BY address")]
            for page in page_ids:
                records = [json.loads(row[0]) for row in db.execute(
                    "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                    (page << 12, (page + 1) << 12))]
                fused_sites.extend((r["address"], fused_site_kind(r)) for r in records if fused_mnemonic(r))
                call_targets.update(r["flows"][0] for r in records
                                    if r.get("flow") == "UNCONDITIONAL_CALL" and r.get("flows"))
                path = args.output / f"page_{page:x}.cpp"
                content = generate_page(page, records, {r["address"] for r in records},
                                        local_temporaries=not args.legacy_unique_storage,
                                        userops=metadata['userops'], block_accounting=True, inlining=inlining,
                                        nontrapping_divisions=divisions, direct_owner_calls=direct_calls)
                if not path.exists() or path.read_text() != content: path.write_text(content)
                pages[page] = []
                outputs[path.name] = None
                generated_count += len(records)
        if generated_count != metadata["instructionCount"]: raise ValueError("Incomplete indexed listing")
        queue.clear()
    for page, instructions in pages.items():
        if not args.all_analyzed:
            outputs[f"page_{page:x}.cpp"] = generate_page(page, instructions, selected,
                                                       local_temporaries=not args.legacy_unique_storage,
                                                       userops=metadata['userops'], block_accounting=True, inlining=inlining,
                                        nontrapping_divisions=divisions, direct_owner_calls=direct_calls)
    inlining.require_plain()
    direct_calls.require_standard()
    print(f"Inlined leaves: {len(inlining.inlined)} routines", flush=True)
    print(f"Owners called in place: {len(direct_calls.called)}", flush=True)
    code = ['#include "aot_runtime.hpp"', '#include "aot_integrity.hpp"', "namespace cm::aot {"]
    for page in pages: code.append(f"bool page_{page:x}(State&);")
    code += ["bool dispatch_page(State& s) {", "switch (s.pc >> 12) {"]
    for page in pages: code.append(f"case 0x{page:x}: return page_{page:x}(s);")
    code += ["default: return false;", "}", "}",
             "const char* import_name(std::uint32_t address) {", "switch (address) {"]
    for symbol in metadata["symbols"]:
        if symbol["space"] == "ram" and symbol["name"].startswith(".import::"):
            code.append(f"case {symbol['address']}: return {json.dumps(symbol['name'][9:])};")
    code += ["default: return nullptr;", "}", "}",
             "void load_program(State& s, const std::filesystem::path& directory) {"]
    for segment in metadata["segments"]:
        if segment["initialized"]:
            with (args.export / segment["file"]).open("rb") as stream:
                digest = hashlib.file_digest(stream, "sha256").hexdigest()
            code.append(f"load_verified(s.memory, {segment['address']}, directory / {json.dumps(segment['file'])}, {segment['size']}, {json.dumps(digest)});")
        else: code.append(f"s.memory.map({segment['address']}, {segment['size']});")
    # Ghidra's PEF loader represents imported transition vectors as four-byte
    # placeholders. Real CFM calls dereference an eight-byte {entry, TOC} pair.
    # Resolve the exact annotated import pointer slots, never scan/patch arbitrary
    # data words that happen to equal an import address.
    imports = {s["address"] for s in metadata["symbols"]
               if s["space"] == "ram" and s["name"].startswith(".import::")}
    vectors = [s for s in metadata["symbols"] if s["space"] == "ram" and s["name"].startswith(".TVect::")]
    code.append(f"s.memory.map(0x60000000, {len(vectors) * 8});")
    for index, vector in enumerate(vectors):
        address = vector["address"]
        segment = next(s for s in metadata["segments"]
                       if s["initialized"] and s["address"] <= address < s["address"] + s["size"])
        with (args.export / segment["file"]).open("rb") as stream:
            stream.seek(address - segment["address"])
            imported = struct.unpack(">I", stream.read(4))[0]
        if imported not in imports: raise ValueError("Unrecognized imported transition vector")
        native_vector = 0x60000000 + index * 8
        code += [f"s.memory.write<4>({address}, {native_vector});",
                 f"s.memory.write<4>({native_vector}, {imported});",
                 f"s.memory.write<4>({native_vector + 4}, 0);"]
    code += ["s.memory.map(0x70000000, 0x100000);", "s.wr<4>(4, 0x700fff00);",
             "s.wr<4>(8, 0x108bba90);", f"s.wr<4>(4128, return_sentinel);", "}", "}"]
    outputs["program.cpp"] = "\n".join(code) + "\n"
    outputs["fused_sites.cpp"] = render_fused_sites(fused_sites)
    outputs["leaves.cpp"] = render_leaves(inlining.leaves, metadata['userops'])
    outputs["divisions.cpp"] = render_divisions(divisions)
    outputs["watched_routines.cpp"] = render_watched_routines(call_targets)
    outputs["native_record_fields.cpp"] = generate_native_record_fields(native_leaves,
        lambda page, records, chosen: generate_page(page, records, chosen,
            native_record_field_hooks=False, native_leaf_lowering=True, userops=metadata['userops'],
            block_accounting=True, nontrapping_divisions=divisions))
    sort_hooks = {name: False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks')}
    outputs["native_sort8.cpp"] = generate_sort8(native_sort,
        lambda page, records, chosen: generate_page(page, records, chosen,
            **sort_hooks, native_leaf_lowering=True, userops=metadata['userops'], block_accounting=True,
            nontrapping_divisions=divisions))
    # Remove only stale generated pages, after all code generation has succeeded.
    for stale in args.output.glob("page_*.cpp"):
        if stale.name not in outputs: stale.unlink()
    for filename, content in outputs.items():
        if content is None: continue
        path = args.output / filename
        if not path.exists() or path.read_text() != content: path.write_text(content)
    manifest = {"purpose": "Bounded ahead-of-time compilation experiment; not a playable game",
                "executableSHA256": metadata["executableSHA256"], "roots": args.root,
                "instructions": generated_count, "pages": len(pages),
                "allAnalyzedInstructions": args.all_analyzed,
                "instructionLocalTemporaries": not args.legacy_unique_storage,
                "semanticReplacements": [{"entry": f"0x{entry:x}", "module": module}
                                         for entry, module in SEMANTIC_REPLACEMENTS
                                         if (args.all_analyzed and entry >> 12 in pages) or entry in selected],
                "sourceFixups": [
                    {"address": f"0x{address:x}", "purpose": purpose}
                    for address, purpose in (
                        (0x10713aec, "Skip unused null-club descriptor lookup for default background"),
                        (0x10713b3c, "Skip unused null-competition descriptor lookup for default background"),
                        (0x10774c24, "Require progress when an optimistic heap hint repeats a failed pool allocation"))
                    if (args.all_analyzed and address >> 12 in pages) or address in selected],
                "quotientFreeDivisions": len(divisions),
                "missingStaticTargets": sorted(missing),
                "pendingTargets": sorted(set(queue) - seen), "files": sorted(outputs)}
    (args.output / "compilation.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Generated {generated_count:,} original instructions across {len(pages)} native compilation units")
    print(f"Missing static targets: {len(missing)}; pending targets: {len(manifest['pendingTargets'])}")


if __name__ == "__main__":
    main()
