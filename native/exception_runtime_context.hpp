#pragma once
#include "exception_runtime_adapter.hpp"

namespace cm::aot::exception_runtime {
// Named operations for the production exception-runtime adapter.
// Prefix helpers retain explicit component boundaries for storage/fault checks.
void restore_context(State& state);
// Capture the vector-capable throw prefix and stop at the search routine's
// entry. Its caller owns search, cleanup, transfer, and any returning epilogue.
void capture_vector_throw(State& state);
// Scalar capture has the same explicit search-entry boundary.
void capture_scalar_throw(State& state);
// Complete capture entries compose search and retain their returning tails.
void throw_vector(State& state);
void throw_scalar(State& state);
// Read the live vector-capability flag and invoke the selected capture entry.
void throw_dispatch(State& state);
// Traverse an action record or advance to the caller's live frame metadata.
void advance_action(State& state);
// Recover saved register suffixes from the caller frame into the throw context.
void copy_saved_registers(State& state);
// Execute cleanup records until the selected catch/specification boundary.
void cleanup_actions(State& state);
// Compare original encoded type descriptions and publish their word32 offset.
void match_types(State& state);
// Test the thrown type against a live guest exception-specification record.
void matches_specification(State& state);
// Clean up to a specification boundary, install its record and restore context.
void install_catch(State& state);
// Search live frame metadata, match handlers, clean up and transfer to a catch.
void search_throw(State& state);
// Preserve exception-object vtable transitions and signed-halfword deletion flags.
void destroy_derived_exception(State& state);
void destroy_base_exception(State& state);
void cleanup_exception_object(State& state);
void derived_exception_type(State& state);
void base_exception_type(State& state);
// Handler callbacks retain their live CFM descriptors and optional return paths.
void abort_facade(State& state);
void termination_facade(State& state);
void invoke_termination(State& state);
void invoke_unexpected(State& state);
void handle_unexpected(State& state);
// Original computed catch destination inside handle_unexpected's pinned body.
void resume_unexpected(State& state);
// Result distinguishes a returning epilogue from a transfer without another
// guest read of the saved link, which could fault or mutate the access prefix.
bool resume_unexpected_continuation(State& state);
}
