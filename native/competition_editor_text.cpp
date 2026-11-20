#include "competition_editor_text.hpp"
#include <array>

namespace cm::aot {
namespace {
// Columns: English, French, German, Italian, Spanish, Danish, Dutch, Norwegian, Portuguese, Swedish.
using Row = std::array<std::string_view, 10>;
using T = EditorText;

std::size_t column(unsigned language) { return language >= 2 && language <= 10 ? language - 1 : 0; }

constexpr std::array<Row, std::size_t(T::count)> texts{{
    // menu_item
    {"Competition Editor…", "Éditeur de compétitions…", "Wettbewerbseditor…", "Editor delle competizioni…",
     "Editor de competiciones…", "Turneringseditor…", "Competitie-editor…", "Turneringsredigering…",
     "Editor de competições…", "Tävlingsredigerare…"},
    // window_title
    {"Competition Editor", "Éditeur de compétitions", "Wettbewerbseditor", "Editor delle competizioni",
     "Editor de competiciones", "Turneringseditor", "Competitie-editor", "Turneringsredigering",
     "Editor de competições", "Tävlingsredigerare"},
    // database
    {"Database:", "Base :", "Datenbank:", "Database:", "Base de datos:", "Database:", "Database:", "Database:",
     "Banco de dados:", "Databas:"},
    // name_placeholder
    {"Name of this custom database", "Nom de cette base personnalisée", "Name dieser eigenen Datenbank",
     "Nome di questo database personalizzato", "Nombre de esta base personalizada", "Navn på denne tilpassede database",
     "Naam van deze aangepaste database", "Navn på denne tilpassede databasen", "Nome deste banco customizado",
     "Namn på den här anpassade databasen"},
    // new_database
    {"New", "Nouveau", "Neu", "Nuovo", "Nuevo", "Ny", "Nieuw", "Ny", "Novo", "Ny"},
    // open
    {"Open…", "Ouvrir…", "Öffnen…", "Apri…", "Abrir…", "Åbn…", "Open…", "Åpne…", "Abrir…", "Öppna…"},
    // save
    {"Save", "Enregistrer", "Sichern", "Salva", "Guardar", "Gem", "Bewaar", "Lagre", "Salvar", "Spara"},
    // save_as
    {"Save As…", "Enregistrer sous…", "Sichern unter…", "Salva con nome…", "Guardar como…", "Gem som…",
     "Bewaar als…", "Lagre som…", "Salvar como…", "Spara som…"},
    // use_for_new
    {"Use for new careers", "Utiliser pour les nouvelles carrières", "Für neue Karrieren verwenden",
     "Usa per le nuove carriere", "Usar en las nuevas carreras", "Brug til nye karrierer", "Gebruik voor nieuwe carrières",
     "Bruk for nye karrierer", "Usar nas novas carreiras", "Använd för nya karriärer"},
    // search
    {"Nation or competition", "Pays ou compétition", "Land oder Wettbewerb", "Nazione o competizione",
     "País o competición", "Land eller turnering", "Land of competitie", "Land eller turnering", "País ou competição",
     "Land eller tävling"},
    // edition_database
    {"{0} database", "Base {0}", "Datenbank {0}", "Database {0}", "Base de datos {0}", "Database {0}", "Database {0}",
     "Database {0}", "Banco de dados {0}", "Databas {0}"},
    // effect_results
    {"counts for every result of the career", "vaut pour tous les résultats de la carrière",
     "gilt für alle Ergebnisse der Karriere", "vale per tutti i risultati della carriera",
     "vale para todos los resultados de la carrera", "gælder alle karrierens resultater",
     "geldt voor alle uitslagen van de carrière", "gjelder alle resultater i karrieren",
     "vale para todos os resultados da carreira", "gäller alla resultat i karriären"},
    // effect_fixtures
    {"copied into each fixture when it is made", "copié dans chaque match à sa création",
     "wird in jede Partie übernommen, wenn sie angesetzt wird", "copiato in ogni partita quando viene creata",
     "se copia en cada partido al crearlo", "kopieres til hver kamp, når den oprettes",
     "wordt in elke wedstrijd overgenomen bij het aanmaken", "kopieres til hver kamp når den opprettes",
     "copiado para cada partida quando ela é criada", "kopieras till varje match när den skapas"},
    // effect_season_end
    {"paid at the season's end", "versé en fin de saison", "wird am Saisonende ausgezahlt", "pagato a fine stagione",
     "se paga al final de la temporada", "udbetales ved sæsonens afslutning", "uitbetaald aan het einde van het seizoen",
     "utbetales ved sesongslutt", "pago no fim da temporada", "betalas ut vid säsongens slut"},
    // tie_break_end
    {"— (ends the list)", "— (fin de la liste)", "— (Ende der Liste)", "— (fine dell'elenco)", "— (fin de la lista)",
     "— (listen slutter)", "— (einde van de lijst)", "— (listen slutter)", "— (fim da lista)", "— (listan slutar)"},
    // tie_break_goal_difference
    {"Goal difference", "Différence de buts", "Tordifferenz", "Differenza reti", "Diferencia de goles", "Målforskel",
     "Doelsaldo", "Målforskjell", "Saldo de gols", "Målskillnad"},
    // tie_break_goals_scored
    {"Goals scored", "Buts marqués", "Erzielte Tore", "Gol fatti", "Goles a favor", "Scorede mål", "Doelpunten voor",
     "Scorede mål", "Gols marcados", "Gjorda mål"},
    // tie_break_wins
    {"Wins", "Victoires", "Siege", "Vittorie", "Victorias", "Sejre", "Overwinningen", "Seire", "Vitórias", "Vinster"},
    // tie_break_head_to_head
    {"Head-to-head (not editable yet)", "Confrontations directes (pas encore modifiable)",
     "Direkter Vergleich (noch nicht änderbar)", "Scontri diretti (non ancora modificabile)",
     "Enfrentamientos directos (aún no editable)", "Indbyrdes kampe (kan endnu ikke ændres)",
     "Onderling resultaat (nog niet aanpasbaar)", "Innbyrdes oppgjør (kan ikke endres ennå)",
     "Confronto direto (ainda não editável)", "Inbördes möten (kan inte ändras ännu)"},
    // tie_break_away_goals
    {"Away goals", "Buts à l'extérieur", "Auswärtstore", "Gol in trasferta", "Goles fuera de casa", "Udebanemål",
     "Uitdoelpunten", "Bortemål", "Gols fora de casa", "Bortamål"},
    // tie_break_keep_order
    {"Keep the order", "Conserver l'ordre", "Reihenfolge beibehalten", "Mantieni l'ordine", "Mantener el orden",
     "Behold rækkefølgen", "Volgorde behouden", "Behold rekkefølgen", "Manter a ordem", "Behåll ordningen"},
    // original
    {"original: {0}", "d'origine : {0}", "Original: {0}", "originale: {0}", "original: {0}", "oprindelig: {0}",
     "origineel: {0}", "opprinnelig: {0}", "original: {0}", "original: {0}"},
    // rounds
    {"Rounds", "Tours", "Runden", "Turni", "Rondas", "Runder", "Rondes", "Runder", "Rodadas", "Omgångar"},
    // round_of
    {"Round {0} of {1}", "Tour {0} sur {1}", "Runde {0} von {1}", "Turno {0} di {1}", "Ronda {0} de {1}",
     "Runde {0} af {1}", "Ronde {0} van {1}", "Runde {0} av {1}", "Rodada {0} de {1}", "Omgång {0} av {1}"},
    // revert
    {"Revert this competition", "Rétablir cette compétition", "Diesen Wettbewerb zurücksetzen",
     "Ripristina questa competizione", "Restaurar esta competición", "Gendan denne turnering",
     "Deze competitie herstellen", "Tilbakestill denne turneringen", "Reverter esta competição",
     "Återställ den här tävlingen"},
    // choose
    {"Choose a competition on the left.", "Choisissez une compétition à gauche.", "Wählen Sie links einen Wettbewerb.",
     "Scegli una competizione a sinistra.", "Elige una competición a la izquierda.", "Vælg en turnering til venstre.",
     "Kies links een competitie.", "Velg en turnering til venstre.", "Escolha uma competição à esquerda.",
     "Välj en tävling till vänster."},
    // empty_state
    {"No competitions are recorded yet.\n\nA competition's original values are set by the game's code, so the editor "
     "learns them from the game: start a New Game once (with the leagues you want to edit), then quit, open the app "
     "again and open the editor from the start screen.",
     "Aucune compétition n'est encore enregistrée.\n\nLes valeurs d'origine d'une compétition sont fixées par le code du "
     "jeu ; l'éditeur les apprend donc du jeu : lancez une nouvelle partie une fois (avec les championnats à modifier), "
     "puis quittez, rouvrez l'application et ouvrez l'éditeur depuis l'écran de départ.",
     "Es sind noch keine Wettbewerbe erfasst.\n\nDie Originalwerte eines Wettbewerbs legt der Spielcode fest; der Editor "
     "lernt sie daher vom Spiel: Starten Sie einmal ein neues Spiel (mit den Ligen, die Sie ändern möchten), beenden "
     "Sie es, öffnen Sie die App erneut und öffnen Sie den Editor vom Startbildschirm.",
     "Nessuna competizione è ancora registrata.\n\nI valori originali di una competizione sono fissati dal codice del "
     "gioco, quindi l'editor li impara dal gioco: avvia una nuova partita una volta (con i campionati da modificare), "
     "poi esci, riapri l'app e apri l'editor dalla schermata iniziale.",
     "Aún no hay competiciones registradas.\n\nLos valores originales de una competición los fija el código del juego, "
     "así que el editor los aprende del juego: inicia una partida nueva una vez (con las ligas que quieras editar), "
     "sal, vuelve a abrir la aplicación y abre el editor desde la pantalla inicial.",
     "Ingen turneringer er registreret endnu.\n\nEn turnerings oprindelige værdier sættes af spillets kode, så editoren "
     "lærer dem af spillet: start et nyt spil én gang (med de ligaer, du vil ændre), afslut, åbn appen igen, og åbn "
     "editoren fra startskærmen.",
     "Er zijn nog geen competities vastgelegd.\n\nDe oorspronkelijke waarden van een competitie worden door de spelcode "
     "bepaald; de editor leert ze dus van het spel: start één keer een nieuw spel (met de competities die je wilt "
     "aanpassen), sluit af, open de app opnieuw en open de editor vanaf het startscherm.",
     "Ingen turneringer er registrert ennå.\n\nEn turnerings opprinnelige verdier settes av spillets kode, så "
     "redigeringen lærer dem av spillet: start et nytt spill én gang (med ligaene du vil endre), avslutt, åpne appen "
     "igjen og åpne redigeringen fra startskjermen.",
     "Nenhuma competição foi registrada ainda.\n\nOs valores originais de uma competição são definidos pelo código do "
     "jogo, então o editor os aprende com o jogo: inicie um novo jogo uma vez (com as ligas que quer editar), saia, "
     "abra o app de novo e abra o editor na tela inicial.",
     "Inga tävlingar har registrerats ännu.\n\nEn tävlings ursprungliga värden sätts av spelets kod, så redigeraren lär "
     "sig dem av spelet: starta ett nytt spel en gång (med de ligor du vill ändra), avsluta, öppna appen igen och öppna "
     "redigeraren från startskärmen."},
    // stage
    {"stage {0}", "phase {0}", "Phase {0}", "fase {0}", "fase {0}", "fase {0}", "fase {0}", "fase {0}", "fase {0}",
     "fas {0}"},
    // cup_tag
    {"cup", "coupe", "Pokal", "coppa", "copa", "pokal", "beker", "cup", "copa", "cup"},
    // changed_values
    {"{0} changed value(s)", "{0} valeur(s) modifiée(s)", "{0} geänderte(r) Wert(e)", "{0} valore/i modificato/i",
     "{0} valor(es) cambiado(s)", "{0} ændrede værdi(er)", "{0} gewijzigde waarde(n)", "{0} endrede verdi(er)",
     "{0} valor(es) alterado(s)", "{0} ändrade värde(n)"},
    // not_saved_yet
    {"not saved yet", "pas encore enregistrée", "noch nicht gesichert", "non ancora salvato", "aún sin guardar",
     "ikke gemt endnu", "nog niet bewaard", "ikke lagret ennå", "ainda não salvo", "inte sparad ännu"},
    // uses_original
    {"new careers start with the original rules", "les nouvelles carrières commencent avec les règles d'origine",
     "neue Karrieren beginnen mit den Originalregeln", "le nuove carriere iniziano con le regole originali",
     "las nuevas carreras empiezan con las reglas originales", "nye karrierer starter med de oprindelige regler",
     "nieuwe carrières beginnen met de oorspronkelijke regels", "nye karrierer starter med de opprinnelige reglene",
     "novas carreiras começam com as regras originais", "nya karriärer börjar med originalreglerna"},
    // uses_this
    {"new careers start with this database", "les nouvelles carrières commencent avec cette base",
     "neue Karrieren beginnen mit dieser Datenbank", "le nuove carriere iniziano con questo database",
     "las nuevas carreras empiezan con esta base", "nye karrierer starter med denne database",
     "nieuwe carrières beginnen met deze database", "nye karrierer starter med denne databasen",
     "novas carreiras começam com este banco", "nya karriärer börjar med den här databasen"},
    // uses_other
    {"new careers start with another database: {0}", "les nouvelles carrières commencent avec une autre base : {0}",
     "neue Karrieren beginnen mit einer anderen Datenbank: {0}", "le nuove carriere iniziano con un altro database: {0}",
     "las nuevas carreras empiezan con otra base: {0}", "nye karrierer starter med en anden database: {0}",
     "nieuwe carrières beginnen met een andere database: {0}", "nye karrierer starter med en annen database: {0}",
     "novas carreiras começam com outro banco: {0}", "nya karriärer börjar med en annan databas: {0}"},
    // cannot_read
    {"The database cannot be read", "La base ne peut pas être lue", "Die Datenbank kann nicht gelesen werden",
     "Impossibile leggere il database", "No se puede leer la base de datos", "Databasen kan ikke læses",
     "De database kan niet worden gelezen", "Databasen kan ikke leses", "Não foi possível ler o banco de dados",
     "Databasen kan inte läsas"},
    // not_custom_database
    {"This is not a custom database this version reads", "Ce n'est pas une base personnalisée lisible par cette version",
     "Dies ist keine eigene Datenbank, die diese Version lesen kann",
     "Non è un database personalizzato leggibile da questa versione",
     "No es una base personalizada que esta versión pueda leer",
     "Dette er ikke en tilpasset database, som denne version kan læse",
     "Dit is geen aangepaste database die deze versie kan lezen",
     "Dette er ikke en tilpasset database som denne versjonen kan lese",
     "Este não é um banco customizado que esta versão consegue ler",
     "Det här är inte en anpassad databas som den här versionen kan läsa"},
    // line
    {"Line {0}: {1}", "Ligne {0} : {1}", "Zeile {0}: {1}", "Riga {0}: {1}", "Línea {0}: {1}", "Linje {0}: {1}",
     "Regel {0}: {1}", "Linje {0}: {1}", "Linha {0}: {1}", "Rad {0}: {1}"},
    // no_base_title
    {"No competitions recorded for this database's base", "Aucune compétition enregistrée pour la base d'origine",
     "Für die Basis dieser Datenbank sind keine Wettbewerbe erfasst",
     "Nessuna competizione registrata per la base di questo database",
     "No hay competiciones registradas para la base de esta base de datos",
     "Ingen turneringer er registreret for denne databases grundlag",
     "Geen competities vastgelegd voor de basis van deze database",
     "Ingen turneringer er registrert for denne databasens grunnlag",
     "Nenhuma competição registrada para a base deste banco", "Inga tävlingar registrerade för den här databasens bas"},
    // no_base_text
    {"It was made from: {0}. Start a New Game on that database once, and the editor will know its competitions.",
     "Elle a été créée à partir de : {0}. Lancez une nouvelle partie sur cette base une fois, et l'éditeur connaîtra ses "
     "compétitions.",
     "Sie wurde erstellt aus: {0}. Starten Sie einmal ein neues Spiel mit dieser Datenbank, dann kennt der Editor ihre "
     "Wettbewerbe.",
     "È stato creato da: {0}. Avvia una nuova partita con quel database una volta e l'editor ne conoscerà le competizioni.",
     "Se creó a partir de: {0}. Inicia una partida nueva con esa base una vez y el editor conocerá sus competiciones.",
     "Den er lavet ud fra: {0}. Start et nyt spil med den database én gang, så kender editoren dens turneringer.",
     "Ze is gemaakt van: {0}. Start één keer een nieuw spel met die database, dan kent de editor de competities.",
     "Den er laget fra: {0}. Start et nytt spill med den databasen én gang, så kjenner redigeringen turneringene.",
     "Ele foi feito a partir de: {0}. Inicie um novo jogo nesse banco uma vez, e o editor passará a conhecer as "
     "competições dele.",
     "Den skapades från: {0}. Starta ett nytt spel med den databasen en gång, så känner redigeraren till dess tävlingar."},
    // not_opened
    {"The database was not opened", "La base n'a pas été ouverte", "Die Datenbank wurde nicht geöffnet",
     "Il database non è stato aperto", "No se abrió la base de datos", "Databasen blev ikke åbnet",
     "De database is niet geopend", "Databasen ble ikke åpnet", "O banco de dados não foi aberto",
     "Databasen öppnades inte"},
    // not_saved
    {"The database was not saved", "La base n'a pas été enregistrée", "Die Datenbank wurde nicht gesichert",
     "Il database non è stato salvato", "No se guardó la base de datos", "Databasen blev ikke gemt",
     "De database is niet bewaard", "Databasen ble ikke lagret", "O banco de dados não foi salvo",
     "Databasen sparades inte"},
    // choice_not_saved
    {"The choice was not saved", "Le choix n'a pas été enregistré", "Die Auswahl wurde nicht gesichert",
     "La scelta non è stata salvata", "No se guardó la elección", "Valget blev ikke gemt", "De keuze is niet bewaard",
     "Valget ble ikke lagret", "A escolha não foi salva", "Valet sparades inte"},
    // save_changes
    {"Save the changes to this custom database?", "Enregistrer les modifications de cette base personnalisée ?",
     "Änderungen an dieser eigenen Datenbank sichern?", "Salvare le modifiche a questo database personalizzato?",
     "¿Guardar los cambios de esta base personalizada?", "Gem ændringerne i denne tilpassede database?",
     "Wijzigingen in deze aangepaste database bewaren?", "Lagre endringene i denne tilpassede databasen?",
     "Salvar as alterações deste banco customizado?", "Spara ändringarna i den här anpassade databasen?"},
    // cancel
    {"Cancel", "Annuler", "Abbrechen", "Annulla", "Cancelar", "Annuller", "Annuleer", "Avbryt", "Cancelar", "Avbryt"},
    // dont_save
    {"Don't Save", "Ne pas enregistrer", "Nicht sichern", "Non salvare", "No guardar", "Gem ikke", "Bewaar niet",
     "Ikke lagre", "Não salvar", "Spara inte"},
    // default_name
    {"Custom database", "Base personnalisée", "Eigene Datenbank", "Database personalizzato", "Base personalizada",
     "Tilpasset database", "Aangepaste database", "Tilpasset database", "Banco customizado", "Anpassad databas"},
    // need_number
    {"a whole number is needed", "un nombre entier est requis", "eine ganze Zahl ist erforderlich",
     "serve un numero intero", "se necesita un número entero", "et helt tal er påkrævet", "een geheel getal is vereist",
     "et heltall kreves", "é preciso um número inteiro", "ett heltal krävs"},
    // need_range
    {"a value from {0} to {1} is needed", "une valeur de {0} à {1} est requise", "ein Wert von {0} bis {1} ist erforderlich",
     "serve un valore da {0} a {1}", "se necesita un valor de {0} a {1}", "en værdi fra {0} til {1} er påkrævet",
     "een waarde van {0} tot {1} is vereist", "en verdi fra {0} til {1} kreves", "é preciso um valor de {0} a {1}",
     "ett värde från {0} till {1} krävs"},
    // places_exceed
    {"promotion, playoff and relegation places ({0}) exceed the league's {1} clubs",
     "les places de promotion, de barrage et de relégation ({0}) dépassent les {1} clubs du championnat",
     "Aufstiegs-, Playoff- und Abstiegsplätze ({0}) übersteigen die {1} Vereine der Liga",
     "i posti di promozione, playoff e retrocessione ({0}) superano i {1} club del campionato",
     "las plazas de ascenso, promoción y descenso ({0}) superan los {1} clubes de la liga",
     "oprykning-, playoff- og nedrykningspladser ({0}) overstiger ligaens {1} klubber",
     "promotie-, play-off- en degradatieplaatsen ({0}) overschrijden de {1} clubs van de competitie",
     "opprykks-, playoff- og nedrykksplasser ({0}) overstiger ligaens {1} klubber",
     "as vagas de acesso, playoff e rebaixamento ({0}) passam dos {1} clubes da liga",
     "uppflyttnings-, playoff- och nedflyttningsplatser ({0}) överstiger ligans {1} klubbar"},
    // head_to_head_refused
    {"head-to-head cannot be chosen yet", "les confrontations directes ne peuvent pas encore être choisies",
     "der direkte Vergleich kann noch nicht gewählt werden", "gli scontri diretti non si possono ancora scegliere",
     "los enfrentamientos directos aún no se pueden elegir", "indbyrdes kampe kan endnu ikke vælges",
     "onderling resultaat kan nog niet worden gekozen", "innbyrdes oppgjør kan ikke velges ennå",
     "o confronto direto ainda não pode ser escolhido", "inbördes möten kan inte väljas ännu"},
    // no_such_round
    {"the cup has no such round", "la coupe n'a pas ce tour", "der Pokal hat diese Runde nicht",
     "la coppa non ha questo turno", "la copa no tiene esa ronda", "pokalen har ikke den runde",
     "de beker heeft die ronde niet", "cupen har ikke den runden", "a copa não tem essa rodada",
     "cupen har ingen sådan omgång"},
    // warn_places
    {"Promotion and relegation places are not compared with the divisions above and below: if they do not balance, the "
     "divisions change size at the season's end.",
     "Les places de promotion et de relégation ne sont pas comparées aux divisions voisines : si elles ne s'équilibrent "
     "pas, les divisions changent de taille en fin de saison.",
     "Auf- und Abstiegsplätze werden nicht mit den Nachbarligen abgeglichen: Gleichen sie sich nicht aus, ändert sich "
     "die Größe der Ligen am Saisonende.",
     "I posti di promozione e retrocessione non vengono confrontati con le divisioni vicine: se non si bilanciano, le "
     "divisioni cambiano dimensione a fine stagione.",
     "Las plazas de ascenso y descenso no se comparan con las divisiones vecinas: si no se equilibran, las divisiones "
     "cambian de tamaño al final de la temporada.",
     "Oprykning- og nedrykningspladser sammenlignes ikke med nabodivisionerne: hvis de ikke går op, ændrer divisionerne "
     "størrelse ved sæsonens afslutning.",
     "Promotie- en degradatieplaatsen worden niet vergeleken met de divisies erboven en eronder: als ze niet in balans "
     "zijn, veranderen de divisies van grootte aan het einde van het seizoen.",
     "Opprykks- og nedrykksplasser sammenlignes ikke med nabodivisjonene: hvis de ikke går opp, endrer divisjonene "
     "størrelse ved sesongslutt.",
     "As vagas de acesso e rebaixamento não são comparadas com as divisões de cima e de baixo: se não fecharem, as "
     "divisões mudam de tamanho no fim da temporada.",
     "Upp- och nedflyttningsplatser jämförs inte med divisionerna ovanför och nedanför: om de inte går jämnt ut ändrar "
     "divisionerna storlek vid säsongens slut."},
    // warn_substitutions
    {"More substitutions than named reserves: a match allows at most as many as the bench holds.",
     "Plus de remplacements que de remplaçants : un match en autorise au plus autant que le banc en compte.",
     "Mehr Auswechslungen als Ersatzspieler: Ein Spiel erlaubt höchstens so viele, wie auf der Bank sitzen.",
     "Più sostituzioni che riserve: una partita ne consente al massimo quante ne ha la panchina.",
     "Más sustituciones que suplentes: un partido permite como máximo tantas como haya en el banquillo.",
     "Flere udskiftninger end reserver: en kamp tillader højst så mange, som bænken rummer.",
     "Meer wissels dan reserves: een wedstrijd staat er hoogstens zoveel toe als er op de bank zitten.",
     "Flere bytter enn reserver: en kamp tillater høyst så mange som benken rommer.",
     "Mais substituições que reservas: uma partida permite no máximo quantas o banco tiver.",
     "Fler byten än avbytare: en match tillåter högst så många som bänken rymmer."},
    // warn_scheduled
    {"{0} value(s) of this competition start in a later season; they are kept as they are.",
     "{0} valeur(s) de cette compétition commencent lors d'une saison ultérieure ; elles sont conservées telles quelles.",
     "{0} Wert(e) dieses Wettbewerbs gelten ab einer späteren Saison; sie bleiben unverändert erhalten.",
     "{0} valore/i di questa competizione iniziano in una stagione successiva; restano così come sono.",
     "{0} valor(es) de esta competición empiezan en una temporada posterior; se conservan tal cual.",
     "{0} værdi(er) for denne turnering gælder fra en senere sæson; de bevares, som de er.",
     "{0} waarde(n) van deze competitie gaan in een later seizoen in; ze blijven zoals ze zijn.",
     "{0} verdi(er) for denne turneringen gjelder fra en senere sesong; de beholdes som de er.",
     "{0} valor(es) desta competição começam em uma temporada futura; ficam como estão.",
     "{0} värde(n) för den här tävlingen börjar gälla en senare säsong; de behålls som de är."},
}};

struct Label {
    std::string_view id;
    Row text;
};
// By parameter id; "league.tiebreak." is a prefix, its number is appended.
constexpr Label labels[] = {
    {"league.promotion.automatic",
     {"Automatic promotion places", "Places de promotion directe", "Direkte Aufstiegsplätze", "Posti di promozione diretta",
      "Plazas de ascenso directo", "Direkte oprykningspladser", "Rechtstreekse promotieplaatsen", "Direkte opprykksplasser",
      "Vagas de acesso direto", "Direkta uppflyttningsplatser"}},
    {"league.promotion.playoff",
     {"Promotion playoff clubs", "Clubs en barrage de promotion", "Vereine in den Aufstiegs-Playoffs",
      "Club ai playoff promozione", "Clubes en la promoción de ascenso", "Klubber i oprykningsplayoff",
      "Clubs in de promotie-play-offs", "Klubber i opprykksplayoff", "Clubes no playoff de acesso",
      "Klubbar i uppflyttningskval"}},
    {"league.relegation.places",
     {"Relegation places", "Places de relégation", "Abstiegsplätze", "Posti di retrocessione", "Plazas de descenso",
      "Nedrykningspladser", "Degradatieplaatsen", "Nedrykksplasser", "Vagas de rebaixamento", "Nedflyttningsplatser"}},
    {"league.points.win",
     {"Points for a win", "Points par victoire", "Punkte für einen Sieg", "Punti per vittoria", "Puntos por victoria",
      "Point for sejr", "Punten voor winst", "Poeng for seier", "Pontos por vitória", "Poäng för vinst"}},
    {"league.points.draw",
     {"Points for a draw", "Points par match nul", "Punkte für ein Unentschieden", "Punti per pareggio",
      "Puntos por empate", "Point for uafgjort", "Punten voor gelijkspel", "Poeng for uavgjort", "Pontos por empate",
      "Poäng för oavgjort"}},
    {"league.tiebreak.",
     {"Tie-break criterion", "Critère de départage", "Kriterium bei Punktgleichheit", "Criterio di spareggio",
      "Criterio de desempate", "Kriterium ved pointlighed", "Criterium bij gelijke stand", "Kriterium ved poenglikhet",
      "Critério de desempate", "Kriterium vid lika poäng"}},
    {"league.bench",
     {"Named reserves per match", "Remplaçants inscrits par match", "Ersatzspieler pro Spiel", "Riserve per partita",
      "Suplentes por partido", "Reserver pr. kamp", "Reserves per wedstrijd", "Reserver per kamp",
      "Reservas relacionados por partida", "Avbytare per match"}},
    {"league.substitutions",
     {"Substitutions allowed", "Remplacements autorisés", "Erlaubte Auswechslungen", "Sostituzioni consentite",
      "Sustituciones permitidas", "Tilladte udskiftninger", "Toegestane wissels", "Tillatte bytter",
      "Substituições permitidas", "Tillåtna byten"}},
    {"league.prize.pool",
     {"Prize pool", "Dotation totale", "Preisgeldtopf", "Montepremi", "Bolsa de premios", "Præmiepulje", "Prijzenpot",
      "Premiepott", "Premiação total", "Prispott"}},
    {"league.prize.sharing",
     {"How the prize pool is shared", "Mode de répartition de la dotation", "Verteilung des Preisgelds",
      "Ripartizione del montepremi", "Reparto de la bolsa de premios", "Fordeling af præmiepuljen",
      "Verdeling van de prijzenpot", "Fordeling av premiepotten", "Como a premiação é dividida",
      "Fördelning av prispotten"}},
    {"league.ruling_body",
     {"Ruling body (discipline rules)", "Instance dirigeante (règles disciplinaires)", "Verband (Disziplinarregeln)",
      "Organo di governo (regole disciplinari)", "Organismo rector (reglas disciplinarias)",
      "Forbund (disciplinære regler)", "Bond (tuchtregels)", "Forbund (disiplinærregler)",
      "Entidade responsável (regras disciplinares)", "Förbund (disciplinregler)"}},
    {"league.structure.clubs",
     {"Number of clubs", "Nombre de clubs", "Anzahl der Vereine", "Numero di club", "Número de clubes", "Antal klubber",
      "Aantal clubs", "Antall klubber", "Número de clubes", "Antal klubbar"}},
    {"league.structure.meetings",
     {"Meetings per pair of clubs", "Confrontations par paire de clubs", "Begegnungen je Vereinspaar",
      "Incontri per coppia di club", "Enfrentamientos por pareja de clubes", "Indbyrdes kampe pr. klubpar",
      "Ontmoetingen per clubpaar", "Møter per klubbpar", "Confrontos por par de clubes", "Möten per klubbpar"}},
    {"league.structure.rounds",
     {"Schedule rounds", "Journées au calendrier", "Spieltage", "Giornate di calendario", "Jornadas del calendario",
      "Spillerunder", "Speelrondes", "Serierunder", "Rodadas do calendário", "Spelomgångar"}},
    {"cup.bench",
     {"Named reserves per match", "Remplaçants inscrits par match", "Ersatzspieler pro Spiel", "Riserve per partita",
      "Suplentes por partido", "Reserver pr. kamp", "Reserves per wedstrijd", "Reserver per kamp",
      "Reservas relacionados por partida", "Avbytare per match"}},
    {"cup.substitutions",
     {"Substitutions allowed", "Remplacements autorisés", "Erlaubte Auswechslungen", "Sostituzioni consentite",
      "Sustituciones permitidas", "Tilladte udskiftninger", "Toegestane wissels", "Tillatte bytter",
      "Substituições permitidas", "Tillåtna byten"}},
    {"cup.prize.pool",
     {"Prize pool", "Dotation totale", "Preisgeldtopf", "Montepremi", "Bolsa de premios", "Præmiepulje", "Prijzenpot",
      "Premiepott", "Premiação total", "Prispott"}},
    {"cup.ruling_body",
     {"Ruling body (discipline rules)", "Instance dirigeante (règles disciplinaires)", "Verband (Disziplinarregeln)",
      "Organo di governo (regole disciplinari)", "Organismo rector (reglas disciplinarias)",
      "Forbund (disciplinære regler)", "Bond (tuchtregels)", "Forbund (disiplinærregler)",
      "Entidade responsável (regras disciplinares)", "Förbund (disciplinregler)"}},
    {"cup.structure.rounds",
     {"Number of rounds", "Nombre de tours", "Anzahl der Runden", "Numero di turni", "Número de rondas", "Antal runder",
      "Aantal rondes", "Antall runder", "Número de rodadas", "Antal omgångar"}},
    {"cup.round.legs",
     {"Legs per tie", "Manches par confrontation", "Spiele je Paarung", "Gare per confronto", "Partidos por eliminatoria",
      "Kampe pr. opgør", "Wedstrijden per ontmoeting", "Kamper per oppgjør", "Jogos por confronto", "Matcher per möte"}},
    {"cup.round.replay_ties",
     {"Ties needed in a single-leg round (replays)", "Confrontations requises sur un tour à match unique (matchs à rejouer)",
      "Benötigte Paarungen in einer Runde mit einem Spiel (Wiederholungsspiele)",
      "Confronti necessari in un turno a gara unica (ripetizioni)",
      "Eliminatorias necesarias en una ronda a partido único (repeticiones)",
      "Nødvendige opgør i en runde med én kamp (omkampe)", "Benodigde ontmoetingen in een ronde met één wedstrijd (replays)",
      "Nødvendige oppgjør i en runde med én kamp (omkamper)", "Confrontos necessários em rodada de jogo único (jogos extras)",
      "Möten som krävs i en omgång med en match (omspel)"}},
    {"cup.round.prize.match",
     {"Prize per club per match played", "Prime par club et par match joué", "Prämie je Verein und Spiel",
      "Premio per club per partita giocata", "Premio por club y partido jugado", "Præmie pr. klub pr. spillet kamp",
      "Premie per club per gespeelde wedstrijd", "Premie per klubb per spilt kamp", "Prêmio por clube por partida jogada",
      "Premie per klubb och spelad match"}},
    {"cup.round.prize.winner",
     {"Winner's prize (-1: share of the pool)", "Prime du vainqueur (-1 : part de la dotation)",
      "Prämie des Siegers (-1: Anteil am Topf)", "Premio del vincitore (-1: quota del montepremi)",
      "Premio del ganador (-1: parte de la bolsa)", "Vinderens præmie (-1: andel af puljen)",
      "Premie voor de winnaar (-1: deel van de pot)", "Vinnerens premie (-1: andel av potten)",
      "Prêmio do vencedor (-1: parte da premiação)", "Vinnarens premie (-1: andel av potten)"}},
    {"cup.round.prize.loser",
     {"Loser's prize (-1: a quarter)", "Prime du perdant (-1 : un quart)", "Prämie des Verlierers (-1: ein Viertel)",
      "Premio dello sconfitto (-1: un quarto)", "Premio del perdedor (-1: una cuarta parte)",
      "Taberens præmie (-1: en fjerdedel)", "Premie voor de verliezer (-1: een kwart)", "Taperens premie (-1: en firedel)",
      "Prêmio do perdedor (-1: um quarto)", "Förlorarens premie (-1: en fjärdedel)"}},
    {"cup.round.ties",
     {"Ties in the round", "Confrontations du tour", "Paarungen der Runde", "Confronti del turno",
      "Eliminatorias de la ronda", "Opgør i runden", "Ontmoetingen in de ronde", "Oppgjør i runden",
      "Confrontos da rodada", "Möten i omgången"}},
    {"cup.round.label",
     {"Round label", "Libellé du tour", "Rundenbezeichnung", "Etichetta del turno", "Etiqueta de la ronda",
      "Rundens betegnelse", "Rondenaam", "Rundens betegnelse", "Rótulo da rodada", "Omgångens beteckning"}},
};
}  // namespace

std::string_view editor_text(unsigned language, EditorText text) {
    const auto& row = texts.at(std::size_t(text));
    const auto translated = row[column(language)];
    return translated.empty() ? row[0] : translated;
}

std::string editor_label(unsigned language, const competition_values::Parameter& p) {
    for (const auto& label : labels) {
        if (label.id == p.id) return std::string(label.text[column(language)]);
        if (label.id.ends_with('.') && p.id.starts_with(label.id))
            return std::string(label.text[column(language)]) + " " + std::string(p.id.substr(label.id.size()));
    }
    return std::string(p.label);
}

std::string filled(std::string_view text, std::initializer_list<std::string> values) {
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '{' && i + 2 < text.size() && text[i + 2] == '}' && text[i + 1] >= '0' && text[i + 1] <= '9') {
            const auto index = std::size_t(text[i + 1] - '0');
            if (index < values.size()) out += *(values.begin() + index);
            i += 2;
        } else out += text[i];
    }
    return out;
}
}
