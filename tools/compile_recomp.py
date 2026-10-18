#!/usr/bin/env python3
"""Translate selected, reachable original PowerPC P-code to build-time C++.

Unresolved indirect destinations stop the native program with an address. They
never fall back to interpreting instructions. Expand roots and rebuild offline.
"""
import argparse
from collections import defaultdict, deque
import json
import hashlib
from pathlib import Path
import sqlite3
import struct
from generation_storage import seed_generated
from unsigned_division_audit import require_unsigned_division_routine
from match_presentation_audit import require_match_presentation_context
from local_temporaries import LocalTemporaries
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
    tuple((entry, "shortlist_quotation_adapter") for entry in SHORTLIST_QUOTATION_ENTRIES))


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
    if address >> 12 == page and address in selected:
        return f"goto i_{address:x};"
    return f"s.pc = 0x{address:x}; return true;"


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
        else:
            raise ValueError(f"Unsupported floating operation: {name}")
    else:
        raise ValueError(f"Unsupported operation: {name}")
    return write_node(output, value)


def require_dead_register(instructions, start, offset, size):
    """Fail closed unless every original branch overwrites a register before use.

    Used only after the audited match multiply has annihilated the undefined
    quotient. Calls, indirect flow, cycles, partial writes, and unknown targets
    are not proofs. P-code inputs include overlapping subregister reads.
    """
    proven = set()

    def overlaps(node):
        return node is not None and node[0] == "register" and \
            int(node[1]) < offset + size and offset < int(node[1]) + node[2]

    def visit(address, path):
        if address in proven:
            return
        item = instructions.get(address)
        if item is None or address in path or len(path) >= 128:
            raise ValueError("Cannot prove match quotient dead on every continuation")
        for name, output, *inputs in item["ops"]:
            if any(overlaps(node) for node in inputs):
                raise ValueError(f"Match quotient read before overwrite at {address:x}")
            if overlaps(output):
                if output != ["register", str(offset), size]:
                    raise ValueError(f"Partial match quotient overwrite at {address:x}")
                proven.add(address)
                return
        if item["flow"] not in ("FALL_THROUGH", "CONDITIONAL_JUMP", "UNCONDITIONAL_JUMP"):
            raise ValueError(f"Unproven match quotient control flow at {address:x}")
        targets = list(item.get("flows", []))
        if item["flow"] != "UNCONDITIONAL_JUMP":
            targets.append(item.get("fallthrough"))
        if not targets or None in targets:
            raise ValueError(f"Missing match quotient continuation at {address:x}")
        for target in targets:
            visit(target, path | {address})
        proven.add(address)

    visit(start, set())


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


def generate_page(page, instructions, selected, local_temporaries=True, userops=None, guest_context_transfers=True, exception_runtime_hooks=True, compiler_arithmetic_hooks=True, register_context_hooks=True, stdlib_core_hooks=True, time_runtime_hooks=True, import_descriptor_hooks=True, native_service_hooks=True, engine_initialization_hooks=True, utility_object_hooks=True, tree_mutation_hooks=True, aggregate_object_hooks=True, record_group_hooks=True, record_query_hooks=True, record_history_hooks=True, match_engine_hooks=True, match_actor_hooks=True, actor_construction_hooks=True, spatial_role_hooks=True, marking_pressure_hooks=True, player_decision_hooks=True, actor_motion_hooks=True, player_movement_hooks=True, match_discipline_hooks=True, match_random_hooks=True, team_spatial_hooks=True, pitch_distance_hooks=True, actor_order_hooks=True, movement_choice_hooks=True, team_line_hooks=True, ball_action_hooks=True, ball_contact_hooks=True, actor_attributes_hooks=True, injury_record_hooks=True, injury_progression_hooks=True, injury_club_events_hooks=True, injury_readouts_hooks=True, injury_reporting_hooks=True, injury_bootstrap_hooks=True, injury_persistence_hooks=True, injury_daily_hooks=True, awol_records_hooks=True, contract_query_hooks=True, contract_storage_hooks=True, contract_terms_hooks=True, contract_review_hooks=True, transfer_offer_hooks=True, transfer_record_hooks=True, transfer_valuation_hooks=True, transfer_synthesis_hooks=True, club_shortlist_storage_hooks=True, shortlist_category_hooks=True, shortlist_links_hooks=True, shortlist_recruitment_hooks=True, shortlist_assessment_hooks=True, shortlist_quotation_hooks=True):
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
    transfer_hooks = guest_context_transfers and bool(page_addresses & {0x10772768, 0x10772774, 0x107727d4, 0x10772a94})
    if transfer_hooks:
        require_exception_runtime_contract(page, instructions)
    presentation_anchor = {0x101cc: 0x101cce38, 0x10203: 0x10203738}.get(page)
    if presentation_anchor in page_addresses or presentation_anchor in selected:
        require_match_presentation_context(page, instructions, selected)
    # Specialize accounting on selected UI/runtime pages while retaining
    # the exact finite-budget path used by validation and probes.
    match_hot_page = page in (0x1071a, 0x10773, 0x1073c)
    code = ['#include "aot_runtime.hpp"', "namespace cm::aot {",
            (f"template <bool Turbo, bool Finite> bool page_{page:x}_impl(State& s) {{" if match_hot_page
             else f"bool page_{page:x}(State& s) {{")]
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
    for item in instructions:
        a = item["address"]
        code.append(f"i_{a:x}: // {item['assembly']}")
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
        if match_hot_page:
            code.append(f"counter.step(0x{a:x});")
        else:
            code.append(f"s.step(0x{a:x});")
        if register_context_hooks and a in CAPTURE_OBSERVATIONS:
            code.append(CAPTURE_OBSERVATIONS[a])
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
                         transfer(a + 4, page, selected)])
            continue
        if a == 0x101355d4:
            # PowerPC divw without OE/Rc does not trap on a zero divisor; its
            # quotient is undefined. Here that quotient's only use is a multiply
            # by the same zero divisor, followed by an unconditional overwrite.
            # Any quotient therefore gives the same observable continuation.
            # Audit the complete use/kill sequence before choosing zero locally;
            # keep unrelated divisions strict and all nonzero cases unchanged.
            sequence = [
                "divw r3,r5,r4", "lwz r0,0xbc(r1)", "cmpwi r0,0x0",
                "mullw r0,r3,r4", "subf r13,r0,r5", "neg r0,r13",
                "ble 0x101355f4", "or r0,r13,r13", "or r13,r0,r0",
                "xoris r3,r13,0x8000"]
            by_address = {record["address"]: record for record in instructions}
            if any(by_address.get(a + i * 4, {}).get("assembly") != assembly
                   for i, assembly in enumerate(sequence)) or item["ops"] != [
                    ["INT_SDIV", ["register", "12", 4], ["register", "20", 4], ["register", "16", 4]]]:
                raise ValueError("Day-advance division no longer matches the audited quotient-independent sequence")
            code.extend(["if (s.r<4>(16) == 0) {", "s.wr<4>(12, 0);",
                         transfer(a + 4, page, selected), "}"])
        if a == 0x1024773c:
            # Same exceptional divw semantics in the match routine. Unlike the
            # day-advance site, r3 is killed along several later branches.
            sequence = [
                "divw r3,r5,r4", "fctiwz f0,f0", "stfd f0,0x9c(r1)",
                "lwz r0,0xa0(r1)", "cmpwi r0,0x0", "mullw r0,r3,r4"]
            by_address = {record["address"]: record for record in instructions}
            if any(by_address.get(a + i * 4, {}).get("assembly") != assembly
                   for i, assembly in enumerate(sequence)) or item["ops"] != [
                    ["INT_SDIV", ["register", "12", 4], ["register", "20", 4], ["register", "16", 4]]]:
                raise ValueError("Match division no longer matches the audited annihilating multiply")
            require_dead_register(by_address, 0x10247754, 12, 4)
            code.extend(["if (s.r<4>(16) == 0) {", "s.wr<4>(12, 0);",
                         transfer(a + 4, page, selected), "}"])
        if a in (0x10713aec, 0x10713b3c):
            # Club and competition fallback callers (10713214 / 107133b8)
            # deliberately pass a null object. The shared selector 107139c0
            # ignores its descriptor in that case. Avoid only these audited
            # eager getter calls; nonnull lookup and other accesses are intact.
            getter = 0x100d5310 if a == 0x10713aec else 0x100d4fd0
            continuation = a + 4
            if item["assembly"] != f"bl 0x{getter:x}" or item["ops"] != [
                    ["COPY", ["register", "2440", 4], ["register", "8", 4]],
                    ["COPY", ["register", "4128", 4], ["const", str(continuation), 4]],
                    ["CALL", None, ["ram", str(getter), 4]]]:
                raise ValueError("Background call site no longer matches the audited release")
            code.extend(["if (s.r<4>(12) == 0) {", "s.wr<4>(2440, s.r<4>(8));",
                         f"s.wr<4>(4128, 0x{continuation:x});",
                         transfer(continuation, page, selected), "}"])
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
            code.append(transfer(a + item["size"], page, selected))
            continue
        labels = set()
        for i, op in enumerate(item["ops"]):
            if op[0] in ("BRANCH", "CBRANCH", "CALL") and op[2][0] == "const":
                offset, width = int(op[2][1]), op[2][2] * 8
                offset &= (1 << width) - 1
                if offset >= 1 << (width - 1): offset -= 1 << width
                labels.add(i + offset)
        local = LocalTemporaries.for_instruction(item["ops"], read, write) if local_temporaries else None
        if local is not None:
            code.append("{")
        for i, op in enumerate(item["ops"]):
            if i in labels: code.append(f"op_{a:x}_{i}:")
            lowered = operation(op, a, i, page, selected, len(item["ops"]),
                                read_node=local.read if local else read,
                                write_node=local.write if local else write,
                                userops=userops)
            if transfer_hooks and a == 0x10772774 and op[0] == 'RETURN':
                if lowered.count('return true;') != 1:
                    raise ValueError('Missing original context-return boundary')
                lowered = lowered.replace('return true;', 'complete_context_transfer(s); return true;')
            code.append(lowered)
        if len(item["ops"]) in labels: code.append(f"op_{a:x}_{len(item['ops'])}:")
        if transfer_hooks and a in (0x107727d4, 0x10772a94):
            code.append('record_context_function(s, std::uint32_t(s.r<4>(20)));')
        if transfer_hooks and a == 0x10772768:
            code.append('record_context_frame(s, std::uint32_t(s.r<4>(12)));')
        if a == 0x10203738:
            code.append("s.wr<4>(0, commentary_presentation_duration(s, std::uint32_t(s.r<4>(104)), std::uint32_t(s.r<4>(0))));")
        # This path is reached only if no earlier branch/return transferred out.
        code.append(transfer(a + item["size"], page, selected))
        if local is not None:
            code.append("}")
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
            from player_movement_audit import load_player_movement_tables
            load_player_movement_tables(args.export)
            from ball_action_audit import load_ball_action_tables
            load_ball_action_tables(args.export)
            from ball_contact_audit import load_ball_contact_tables
            load_ball_contact_tables(args.export)
        require_record_query_index(db, None if args.all_analyzed else selected)
        require_record_group_index(db, None if args.all_analyzed else selected)
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
    generated_count = len(selected)
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
                path = args.output / f"page_{page:x}.cpp"
                content = generate_page(page, records, {r["address"] for r in records},
                                        local_temporaries=not args.legacy_unique_storage,
                                        userops=metadata['userops'])
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
                                                       userops=metadata['userops'])
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
                        (0x101355d4, "Preserve nontrapping division where zero-divisor quotient is provably unused"),
                        (0x1024773c, "Preserve match division after proving the exceptional quotient unused on every branch"),
                        (0x10713aec, "Skip unused null-club descriptor lookup for default background"),
                        (0x10713b3c, "Skip unused null-competition descriptor lookup for default background"),
                        (0x10774c24, "Require progress when an optimistic heap hint repeats a failed pool allocation"))
                    if (args.all_analyzed and address >> 12 in pages) or address in selected],
                "missingStaticTargets": sorted(missing),
                "pendingTargets": sorted(set(queue) - seen), "files": sorted(outputs)}
    (args.output / "compilation.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Generated {generated_count:,} original instructions across {len(pages)} native compilation units")
    print(f"Missing static targets: {len(missing)}; pending targets: {len(manifest['pendingTargets'])}")


if __name__ == "__main__":
    main()
