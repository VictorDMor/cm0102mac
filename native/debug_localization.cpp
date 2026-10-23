#include "debug_localization.hpp"
#include "aot_runtime.hpp"
#include <array>

namespace cm::aot {
std::string_view debug_text(unsigned language, DebugText text) {
    // English, US English, French, German, Italian, Spanish, Danish,
    // Dutch, Norwegian, Portuguese and Swedish, as in the original language menu.
    static constexpr std::array<std::array<std::string_view, 5>, 11> labels{{
        {"Best XI", "Add Goal", "Goal", "On", "Off"},
        {"Best XI", "Add Goal", "Goal", "On", "Off"},
        {"Meilleur onze", "Ajouter un but", "But", "Activ\xe9", "D\xe9sactiv\xe9"},
        {"Beste Elf", "Tor hinzuf\xfcgen", "Tor", "Ein", "Aus"},
        {"Migliori 11", "Aggiungi gol", "Gol", "Attivo", "Disattivo"},
        {"Mejor once", "A\xf1" "adir gol", "Gol", "Activado", "Desactivado"},
        {"Bedste 11", "Tilf\xf8j m\xe5l", "M\xe5l", "Til", "Fra"},
        {"Beste elf", "Doelpunt toevoegen", "Doelpunt", "Aan", "Uit"},
        {"Beste 11", "Legg til m\xe5l", "M\xe5l", "P\xe5", "Av"},
        {"Melhor 11", "Adicionar Gol", "Gol", "Ligado", "Desligado"},
        {"B\xe4sta elvan", "L\xe4gg till m\xe5l", "M\xe5l", "P\xe5", "Av"},
    }};
    return labels[language < labels.size() ? language : 0][static_cast<unsigned>(text)];
}

std::string_view debug_text(const State& s, DebugText text) {
    return debug_text(s.memory.read<1>(s.r<4>(8) - 0x35ac), text);
}
}
