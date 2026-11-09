#include "debug_localization.hpp"
#include "aot_runtime.hpp"
#include <array>

namespace cm::aot {
std::string_view debug_text(unsigned language, DebugText text) {
    if (text == DebugText::match_results) return "Match Results";
    if (text == DebugText::extend_all_contracts) {
        static constexpr std::array<std::string_view, 11> labels{
            "Extend All Contracts by 1 Year", "Extend All Contracts by 1 Year",
            "Prolonger tous les contrats d'un an", "Alle Vertr\344ge um 1 Jahr verl\344ngern",
            "Rinnova tutti i contratti di 1 anno", "Renovar todos los contratos por 1 a\361o",
            "Forl\346ng alle kontrakter med 1 \345r", "Alle contracten met 1 jaar verlengen",
            "Forny alle kontrakter med 1 \345r", "Renovar Todos os Contratos por 1 Ano",
            "F\366rl\344ng alla kontrakt med 1 \345r"};
        return labels[language < labels.size() ? language : 0];
    }
    if (text == DebugText::options_sidebar) {
        static constexpr std::array<std::string_view, 11> sidebar{
            "Debugging\nOptions", "Debugging\nOptions", "Options de\nd\351bogage", "Debugging-\nOptionen",
            "Opzioni\ndi debug", "Opciones de\ndepuraci\363n", "Fejlfindings-\nindstillinger",
            "Foutopsporings-\nopties", "Feils\370kings-\nvalg", "Op\347\365es\nde Debugging", "Fels\366knings-\nalternativ"};
        return sidebar[language < sidebar.size() ? language : 0];
    }
    static constexpr std::array<std::array<std::string_view, 6>, 11> extra{{
        {"Underdogs Win", "Underdogs Win MAX", "Debugging Options", "Repeat Goal", "Red Cards (3/4)", "Injuries (5/6)"},
        {"Underdogs Win", "Underdogs Win MAX", "Debugging Options", "Repeat Goal", "Red Cards (3/4)", "Injuries (5/6)"},
        {"Les outsiders gagnent", "Outsiders gagnent MAX", "Options de d\351bogage", "R\351p\351ter le but", "Cartons rouges (3/4)", "Blessures (5/6)"},
        {"Au\337enseiter gewinnen", "Au\337enseiter gewinnen MAX", "Debugging-Optionen", "Tor wiederholen", "Rote Karten (3/4)", "Verletzungen (5/6)"},
        {"Vincono gli sfavoriti", "Sfavoriti vincono MAX", "Opzioni di debug", "Ripeti gol", "Espulsioni (3/4)", "Infortuni (5/6)"},
        {"Ganan los modestos", "Ganan los modestos MAX", "Opciones de depuraci\363n", "Repetir gol", "Tarjetas rojas (3/4)", "Lesiones (5/6)"},
        {"Underdogs vinder", "Underdogs vinder MAX", "Fejlfindingsindstillinger", "Gentag m\345l", "R\370de kort (3/4)", "Skader (5/6)"},
        {"Underdogs winnen", "Underdogs winnen MAX", "Foutopsporingsopties", "Doelpunt herhalen", "Rode kaarten (3/4)", "Blessures (5/6)"},
        {"Underdoger vinner", "Underdoger vinner MAX", "Feils\370kingsvalg", "Gjenta m\345l", "R\370de kort (3/4)", "Skader (5/6)"},
        {"Zebras Vencem", "Zebras Vencem MAX", "Op\347\365es de Debugging", "Repetir Gol", "Expuls\365es (3/4)", "Les\365es (5/6)"},
        {"Underdogs vinner", "Underdogs vinner MAX", "Fels\366kningsalternativ", "Upprepa m\345l", "R\366da kort (3/4)", "Skador (5/6)"},
    }};
    if (text >= DebugText::underdogs)
        return extra[language < extra.size() ? language : 0][unsigned(text) - unsigned(DebugText::underdogs)];
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
