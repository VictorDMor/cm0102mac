#include "debug_localization.hpp"
#include "aot_runtime.hpp"
#include <array>

namespace cm::aot {
std::string_view debug_text(unsigned language, DebugText text) {
    // English, US English, French, German, Italian, Spanish, Danish,
    // Dutch, Norwegian, Portuguese and Swedish, as in the original language menu.
    static constexpr std::array<std::array<std::string_view, 10>, 11> labels{{
        {"Best XI", "Add Goal", "Goal", "On", "Off", "Recover", "Extend Contract by 1 Year", "Cancel Sale", "Remove Release Clause", "Maximize Attributes"},
        {"Best XI", "Add Goal", "Goal", "On", "Off", "Recover", "Extend Contract by 1 Year", "Cancel Sale", "Remove Release Clause", "Maximize Attributes"},
        {"Meilleur onze", "Ajouter un but", "But", "Activ\xe9", "D\xe9sactiv\xe9", "Soigner", "Prolonger le contrat d'un an", "Annuler la vente", "Retirer la clause lib\xe9ratoire", "Maximiser les attributs"},
        {"Beste Elf", "Tor hinzuf\xfcgen", "Tor", "Ein", "Aus", "Heilen", "Vertrag um 1 Jahr verl\xe4ngern", "Verkauf abbrechen", "Ausstiegsklausel entfernen", "Attribute maximieren"},
        {"Migliori 11", "Aggiungi gol", "Gol", "Attivo", "Disattivo", "Recupera", "Rinnova contratto di 1 anno", "Annulla vendita", "Rimuovi clausola rescissoria", "Massimizza attributi"},
        {"Mejor once", "A\xf1" "adir gol", "Gol", "Activado", "Desactivado", "Recuperar", "Renovar contrato por 1 a\xf1o", "Cancelar venta", "Eliminar cl\xe1usula de rescisi\xf3n", "Maximizar atributos"},
        {"Bedste 11", "Tilf\xf8j m\xe5l", "M\xe5l", "Til", "Fra", "Restituer", "Forl\xe6ng kontrakt med 1 \xe5r", "Annuller salg", "Fjern frik\xf8" "bsklausul", "Maksimer attributter"},
        {"Beste elf", "Doelpunt toevoegen", "Doelpunt", "Aan", "Uit", "Herstellen", "Contract met 1 jaar verlengen", "Verkoop annuleren", "Afkoopclausule verwijderen", "Attributen maximaliseren"},
        {"Beste 11", "Legg til m\xe5l", "M\xe5l", "P\xe5", "Av", "Restituer", "Forny kontrakten med 1 \xe5r", "Avbryt salg", "Fjern utkj\xf8psklausul", "Maksimer attributter"},
        {"Melhor 11", "Adicionar Gol", "Gol", "Ligado", "Desligado", "Recuperar", "Renovar Contrato por 1 Ano", "Cancelar Venda", "Remover Cl\xe1usula de Venda", "Maximizar Atributos"},
        {"B\xe4sta elvan", "L\xe4gg till m\xe5l", "M\xe5l", "P\xe5", "Av", "\xc5terst\xe4ll", "F\xf6rl\xe4ng kontraktet med 1 \xe5r", "Avbryt f\xf6rs\xe4ljning", "Ta bort utk\xf6psklausul", "Maximera attribut"},
    }};
    return labels[language < labels.size() ? language : 0][static_cast<unsigned>(text)];
}

std::string_view debug_text(const State& s, DebugText text) {
    return debug_text(s.memory.read<1>(s.r<4>(8) - 0x35ac), text);
}
}
