#include "debug_localization.hpp"
#include "aot_runtime.hpp"
#include <array>

namespace cm::aot {
std::string_view debug_text(unsigned language, DebugText text) {
    if (text == DebugText::underdogs) return language == 9 ? "Zebras Vencem" : "Underdogs Win";
    if (text == DebugText::underdogs_max) return language == 9 ? "Zebras Vencem MAX" : "Underdogs Win MAX";
    // English, US English, French, German, Italian, Spanish, Danish,
    // Dutch, Norwegian, Portuguese and Swedish, as in the original language menu.
    static constexpr std::array<std::array<std::string_view, 12>, 11> labels{{
        {"Best XI", "Add Goal", "Goal", "On", "Off", "Recover", "Extend Contract by 1 Year", "Cancel Sale", "Remove Release Clause", "Debugging", "Recover Players", "Play for Free"},
        {"Best XI", "Add Goal", "Goal", "On", "Off", "Recover", "Extend Contract by 1 Year", "Cancel Sale", "Remove Release Clause", "Debugging", "Recover Players", "Play for Free"},
        {"Meilleur onze", "Ajouter un but", "But", "Activ\xe9", "D\xe9sactiv\xe9", "Soigner", "Prolonger le contrat d'un an", "Annuler la vente", "Retirer la clause lib\xe9ratoire", "D\xe9" "bogage", "R\xe9" "cup\xe9rer les joueurs", "Jouer gratuitement"},
        {"Beste Elf", "Tor hinzuf\xfcgen", "Tor", "Ein", "Aus", "Heilen", "Vertrag um 1 Jahr verl\xe4ngern", "Verkauf abbrechen", "Ausstiegsklausel entfernen", "Debugging", "Spieler wiederherstellen", "Kostenlos spielen"},
        {"Migliori 11", "Aggiungi gol", "Gol", "Attivo", "Disattivo", "Recupera", "Rinnova contratto di 1 anno", "Annulla vendita", "Rimuovi clausola rescissoria", "Debug", "Recupera giocatori", "Gioca gratis"},
        {"Mejor once", "A\xf1" "adir gol", "Gol", "Activado", "Desactivado", "Recuperar", "Renovar contrato por 1 a\xf1o", "Cancelar venta", "Eliminar cl\xe1usula de rescisi\xf3n", "Depuraci\xf3" "n", "Recuperar jugadores", "Jugar gratis"},
        {"Bedste 11", "Tilf\xf8j m\xe5l", "M\xe5l", "Til", "Fra", "Restituer", "Forl\xe6ng kontrakt med 1 \xe5r", "Annuller salg", "Fjern frik\xf8" "bsklausul", "Fejlfinding", "Gendan spillere", "Spil gratis"},
        {"Beste elf", "Doelpunt toevoegen", "Doelpunt", "Aan", "Uit", "Herstellen", "Contract met 1 jaar verlengen", "Verkoop annuleren", "Afkoopclausule verwijderen", "Foutopsporing", "Spelers herstellen", "Gratis spelen"},
        {"Beste 11", "Legg til m\xe5l", "M\xe5l", "P\xe5", "Av", "Restituer", "Forny kontrakten med 1 \xe5r", "Avbryt salg", "Fjern utkj\xf8psklausul", "Feils\xf8king", "Gjenopprett spillere", "Spill gratis"},
        {"Melhor 11", "Adicionar Gol", "Gol", "Ligado", "Desligado", "Recuperar", "Renovar Contrato por 1 Ano", "Cancelar Venda", "Remover Cl\xe1usula de Venda", "Debugging", "Recuperar jogadores", "Jogar de Gra\xe7" "a"},
        {"B\xe4sta elvan", "L\xe4gg till m\xe5l", "M\xe5l", "P\xe5", "Av", "\xc5terst\xe4ll", "F\xf6rl\xe4ng kontraktet med 1 \xe5r", "Avbryt f\xf6rs\xe4ljning", "Ta bort utk\xf6psklausul", "Fels\xf6kning", "\xc5terst\xe4ll spelare", "Spela gratis"},
    }};
    return labels[language < labels.size() ? language : 0][static_cast<unsigned>(text)];
}

std::string_view debug_text(const State& s, DebugText text) {
    return debug_text(s.memory.read<1>(s.r<4>(8) - 0x35ac), text);
}
}
