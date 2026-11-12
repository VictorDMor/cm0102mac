// The shared copy of the translated game (cm_aot_shared, native/CMakeLists.txt): every member of the static
// library cm_aot is force-loaded into it, so this file only gives the target a source.
namespace cm::aot { void aot_shared_anchor() {} }
