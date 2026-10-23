# Reconstrução semântica do dia da semana da data Mac

Reconstrua em C++23 a entrada independente `0x10715b20`, candidata à conversão do registro interno de data do CM 01/02 para dia da semana. Trabalhe em `/Users/victordmor/Projects/cm0102mac` e execute este incremento até a entrega validada. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês. Este arquivo é um pedido completo de implementação para uma sessão nova. Sua preparação foi somente planejamento: não implementou a rotina, não recompilou nem iniciou outro chat.

## Produto e próximo incremento

O produto é o CM 01/02 completo, jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em código semântico compreensível. A referência atual recompila estaticamente o original Mac; isso não significa que todo o engine foi reconstruído semanticamente. A temporada real, passagem anual e persistência correspondente já estão entregues em `docs/native-season-reference.md`; não repita esse marco como pendência.

Adição, subtração, diferença, later, before, mês e dia do mês estão entregues. O próximo incremento escolhido é somente o dia da semana, complementando a interpretação do mesmo registro de data. Callers reais devem usar a nova operação sem mudar o dia da semana exibido, datas, fixtures ou comportamento do jogo. Demonstre o contrato antes de tratá-lo como estabelecido.

Formatação e nomes localizados de dias, ajuste de uma data para outro dia da semana (`0x10715a30`), construção de datas, comparadores ternários, idade, contratos, transferências, agendador, competições, jogadores, gols/placar e demais rotinas ficam fora do escopo. Inspecionar callers ou observar a UI não autoriza reconstruí-los. Novas otimizações, features de gameplay e mudanças gráficas permanecem suspensas; não prometa ganho de velocidade.

Preserve a peculiaridade bissexta da diferença expressamente adiada: January 1 2000 menos January 1 2001 retorna -365; a inversão retorna +366. Uma correção depende de decisão separada, análise dos callers e validação de gameplay/persistência. Não use o novo trabalho de weekday para corrigir esse percurso, normalizar datas inválidas ou alinhar ao Gregorian casos que o original trata de outra forma.

## Ponto de partida verificado na preparação

- `main`, HEAD e consulta nova a `origin/main` concordaram em `f6c279b1b074b6bcb5d9f653d79bd3dc938ff4a2`, entrega de dia do mês. Reconfira Git, remoto, diff e `AGENTS.md` aplicáveis. Não havia `AGENTS.md` nos ancestrais ou árvore do projeto inspecionada.
- Os três prompts `docs/features/native-date-difference-reconstruction-prompt.md`, `docs/features/native-date-month-reconstruction-prompt.md` e `docs/features/native-date-day-of-month-reconstruction-prompt.md` estavam untracked. Preserve-os. Este novo prompt também pode estar pendente; não confunda handoffs com mudanças de produto entregues.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` foram reconferidos: ARM64, executável SHA-256 `1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc`.
- O candidato privado assinado da entrega anterior `.local/performance/date-day-of-month-live-01/CM Validation date-day-of-month-live-01.app` foi reconferido ARM64, executável `6e5ccab6215b16680874424bd8ad51cca2c4642456047ab1f0f01ce3b3deb964`. Não confunda esse hash com fonte/instalado.
- O backup histórico `.local/date-day-of-month-reconstruction/reference/installed.app` foi reconferido ARM64 com executável `caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756`. Preserva a referência anterior ao dia do mês, NÃO o instalado atual. Antes de trocar builds/instalar, crie uma NOVA cópia privada executável do instalado atual, com resource forks e extended attributes; verifique identidade e inventário.
- O relatório entregue de dia do mês registra 3.173 casos diretos original/moderno, 1.700 oracle Gregorian, 131.072 signed-day puros, sparse mappings, mutable tables/TOC, ABI e leituras ordenadas. A suíte final aprovou 35/35 CTest; descoberta Python 29 testes, 25 aprovados e quatro grupos opcionais pulados, também executados explicitamente; UBSAN gerado/focado passaram. Foram 1.968 objetos ligados dependentes de State no layout final. São resultados da entrega anterior, NÃO aprovação de weekday. Não reexecute essas suítes caras somente para planejar.
- Os fixtures imutáveis de um dia, sete dias e ordenação aprovaram dois originais estritos, captura inteiramente original, dois candidatos, cinco baselines e negativos exercitados. Chamadas próprias de dia do mês por candidato: 8.639 / 14.209 / 8.733; mês: 8.807 / 38.256 / 8.433. Ordenação também executou 38.051 later e 12.911 before. CMCAL002/CMMON001 permaneceram byte-idênticos; dia do mês usa CMDOM001. Essas contagens não provam weekday.
- A UI ARM64 anterior aprovou Save/saída normal/processo ausente/reabertura exata/Load em 28 August 2002 night e 1 September 2002 afternoon, atravessando o mês normalmente. Três runs registraram 5 / 10.735 / 3 chamadas próprias de dia do mês e 40 amostras válidas conferidas. Dados pessoais/metadados foram preservados; 809/5/43 arquivos Game/Runtime/Runs eram a observação dessa entrega, não uma quantidade invariável.

Leia `README.md`, `docs/native-port-status.md`, `docs/native-season-reference.md`, `docs/native-calendar-reconstruction.md`, `docs/native-date-difference-reconstruction.md` (incluindo Deferred improvement), `docs/native-date-ordering-reconstruction.md`, `docs/native-date-before-reconstruction.md`, `docs/native-date-month-reconstruction.md` e `docs/native-date-day-of-month-reconstruction.md`. Os relatórios entregues são contexto reproduzível; não dependa desta conversa ou de uma interpretação de logs temporários.

Inspecione `native/calendar*`, `native/date_day_of_month_checks.cpp`, `native/date_month_checks.cpp`, `native/date_difference_checks.cpp`, `native/date_ordering_checks.cpp`, `native/callbacks.cpp`, `native/aot_runtime.hpp`, `native/aot_probe.cpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*`, `native/CMakeLists.txt`, `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/test_generated_temporaries.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`. Use `.local/recomp-input/instructions.sqlite` read-only e o código gerado privado para caracterizar o original.

## Fronteira candidata e contrato a demonstrar

A inspeção estática read-only da preparação encontrou 40 instruções contíguas em `[0x10715b20,0x10715bc0)`, última instrução/retorno em `0x10715bbc`. Há retornos condicionais/antecipados em `0x10715bac` e `0x10715bb4`; não audite somente o último retorno. A metadata de funções privada também registra corpo de 160 bytes.

Fingerprint com os campos exatos de `tools/calendar_audit.py`: `faab9fb0f48231766f5e9b0595dd109a2be7639b984ec830b14021ad64892c43`. Índice SHA-256 `2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec` e `program.json` `50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0` foram reconferidos. Proveniência: 1.953.157 instruções, PEF original `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`. Exports/payloads são privados e indisponíveis em clone público limpo; não publique listagens ou dados originais.

Foram encontradas 63 chamadas diretas `bl 0x10715b20`, incluindo uma na mesma página, `0x10715a54`. Outros exemplos: `0x1006655c`, `0x10067fd4`, `0x100faf40`, `0x101197e4`, `0x10185538`, `0x1018554c`, `0x1029d978`, `0x104946c4`, `0x104946d8`. Não foi encontrado fluxo externo indexado dirigido ao interior do corpo. Reconfirme limites, fluxo/fallthrough, destino gerado, callers e possibilidades indiretas. Quantidade estática não prova execução real; o caller da mesma página precisa atingir o hook, inclusive via `goto`.

A leitura estática sugere r3 como ponteiro da data e estas leituras em ordem: unsigned16 ano em date+2, byte flag em date+4 e signed16 dia em date+0. Ao contrário de mês/dia do mês, esta entrada lê o ano e não aparenta consultar tabelas/slots TOC. Demonstre ordem/largura/signedness, ausência de releituras ou acessos adicionais e independência da TOC, sem importar o footprint/ABI de outra conversão.

O corpo usa o ano relativo a 1600, aritmética de 365 dias e correções de divisão por 4/100/400, multiplicações signed-high e shifts/carry; uma flag não zero aparentemente subtrai um dia. A redução por sete usa aritmética signed e é seguida de estreitamento signed16 e branches diferentes para resto positivo, zero e negativo. Isso é hipótese fundamentada em assembly, ainda sem comparação diferencial própria.

Demonstre a enumeração no domínio válido: Monday/Sunday, retorno para datas conhecidas e correspondência com o que a UI mostra. Não assuma modulo Euclidean, retorno sempre 0..6, sentinela -1 ou uniformidade Gregorian em todo o domínio unsigned16. Caracterize os anos abaixo/ao redor de 1600, dias negativos/inválidos e flag inconsistente. Não transforme resto negativo em positivo nem acrescente clamp/normalização/exceção inexistente. Regras Gregorian independentes são oracle SOMENTE no domínio válido cuja correspondência tenha sido demonstrada; divergências originais ficam explicitadas e preservadas.

Resolva a semântica exata de `mulhw`, shifts aritméticos, correções de sinal e `addze`, incluindo XER.CA e as diferenças de CR0 entre as saídas condicionais. Um resultado numericamente correto não prova ABI. Compare todos os registradores inteiros/temporários, CR completo, XER completo, LR, CTR, TOC/context TOC, PC, stack, memória inteira, alocador, imports/callbacks e RNG. Confirme ausência/presença de spills, scratch, inicialização e writes. `call_original` restaura contexto no callback e não substitui equivalência da entrada direta. Delimite unmapped/wrapping/concurrent mutation; não invente equivalência universal.

Cubra sete dias consecutivos e viradas Sunday->Monday, todos os limites de mês/ano, February 28/29 e March 1, 1900/2000/2100, anos 0/1/1599/1600/1601 e extremos unsigned16, flags 0/1/128/255/inconsistentes, dias negativos/inválidos/extremos signed16 e diferentes CR/XER iniciais. Use budget finito e casos puros amplos sem confundir saturação de tipos com comportamento definido. Preserve as sete operações anteriores e suas evidências.

## Implementação e equivalência automática

Amplie minimamente a arquitetura existente com lógica semântica pura e adapter dos efeitos legados. A produção normal deve executar C++ semântico para weekday; seleção original fica restrita a diagnósticos privados. Não introduza selector na UI, wrappers que deleguem a conversão normal ao original, nova emulação ou abstrações especulativas.

Integre o hook no destino gerado de `0x10715b20` na página `0x10715`, cobrindo o caller real da mesma página via `goto`, dispatch entre páginas, indiretas e callbacks. Acrescente auditoria do corpo completo, incluindo todas as saídas, que rejeite alteração/fragmento incompleto. Teste instruções reais de callers com inputs controlados, identificando essa evidência como sintética. Não substitua corpos vizinhos/callers por busca textual. Preserve os hooks, corpos e contratos já entregues.

Compare estados original/moderno equivalentes: resultado, toda ABI/temporários/PC, todos os bytes mapeados, ordem/footprint dos efeitos, stack, alocador, imports/callbacks e RNG. Reutilize a instrumentação opcional de leituras existente; deve continuar desabilitada na produção normal. Use mappings mínimos para provar acessos efetivos quando apropriado. Acrescente contador/amostras próprios de weekday com entrada, ponteiro, ano/dia, flag, resultado e retorno/caller. Contagens das outras operações não comprovam a nova entrada. Diagnostic samples/streams podem ter leituras extras; documente separadamente o footprint de produção.

Separe inteiramente original; baseline entregue com sete operações modernas e weekday original; candidato com oito modernas. Preserve os significados históricos:

| Modo | Operações modernas anteriores | Weekday |
| --- | --- | --- |
| Default reference | nenhuma | original |
| `--calendar-baseline` | add/subtract | original |
| `--date-ordering-baseline` | add/subtract/difference | original |
| `--date-before-baseline` | add/subtract/difference/later | original |
| `--date-month-baseline` | add/subtract/difference/later/before | original |
| `--date-day-of-month-baseline` | add/subtract/difference/later/before/month | original |
| Nova referência entregue, com nome inequívoco (por exemplo `--date-weekday-baseline`) | todas as sete anteriores, incluindo day-of-month | original |
| `--modern` | todas as sete anteriores | semantic |

Captura original deve selecionar também weekday original, tanto em CLI quanto em captura de aplicativo. Não recapture candidato como oracle. Atualize o manifesto sem perder qualquer entrada entregue e mantenha campos diagnósticos fora da serialização de save/snapshot.

Reexecute `.local/performance/season-day-01/SeasonFixture`, `.local/performance/season-week-01/SeasonFixture` e `.local/performance/date-ordering-capture-02/SeasonFixture`, preservando os arquivos imutáveis. Exija dois originais estritos, captura inteiramente original, todos os seis baselines pertinentes e duas execuções candidatas equivalentes por fixture. Preserve a única diferença permitida em instruções traduzidas; não masque PC, registradores, temporários, memória, alocador, imports/callbacks/RNG. EOF e término pelo estado/transcript original devem continuar com budget finito.

Use stream privado original separado e explicitamente versionado para weekday (por exemplo `CMWKD001`). Não altere CMCAL002, CMMON001 ou CMDOM001. Registre identidades antes/depois; ao recapturar os três streams históricos em modo inteiramente original, confira byte-identidade com `.local/date-day-of-month-reconstruction/{day,week,ordering}-edges-01`, cujos hashes completos constam no relatório entregue. Amplie o checker de historical edges para cobrir também CMDOM001; não confunda uma opção presente com verificação dos três arquivos.

Acrescente negativo isolado que altera somente o primeiro retorno moderno de weekday e exija rejeição na primeira fronteira própria, com field/offset identificados. Mantenha controles RNG/shifts/difference/later/before/month/day-of-month; uma falha própria deve ser localizada antes da divergência posterior. Valide truncamento de header/body, checksum/EOF, modo original de captura e ausência de vazamento da perturbação para execução normal.

Reporte chamadas próprias por fixture: zero é ausência de cobertura. Se nenhum dos três exercitar weekday, capture em modo inteiramente original uma sequência real mínima pertinente usando ferramentas existentes e compare o mesmo fixture em original/baseline/candidato. Não edite snapshots/datas/RNG nem capture candidato para forçar aceitação. Não exija replay de carreira inteira.

## Aplicativo real e persistência

Use uma única instância ARM64 por vez e novo Runtime/Runs privado preparado pelas ferramentas existentes. O checkpoint preferido é `.local/performance/date-day-of-month-live-01/Runtime/Files/games/DateDay-01Sep2002.sav`, SHA-256 `ce48d8af426557e18976292e3bd22172241b1d61d8a575108a6f862b3583463f`, reconferido na preparação. A alternativa preservada é `DateDay-28Aug2002.sav`, na mesma pasta, SHA-256 `22a6263cd149251c58c74618d4cf73831218e74123dcee79edd4debbc2c339a3`. Confira novamente hashes e preserve ambos e todos os checkpoints históricos. Não sobrescreva o candidato de dia do mês.

O checkpoint de September 1 foi observado como Sunday, afternoon (`1.9.02 TAR`), Victor Moraes / Cruzeiro. O calendário mostrou Goias em August 31 com resultado 0:3, e Coritiba/Santos/Colo Colo pendentes em September 6/8/11. Luisao mostrou nascimento February 13, 1981, idade 21, uma internacionalização, condição 80%, 26 (4) partidas oficiais, um gol, duas assistências e média 7.20. Contrato full-time, BRL 8.750/semana, início N/D, vinculado Sim, expira December 24, 2004, rotação, sem prêmios/cláusulas. Esses são valores históricos observados; confira o candidato novo independentemente.

Demonstre chamadas semânticas próprias de weekday em interação normal de datas, guiada pelos callers/logs. Não presuma que abrir calendário basta. Registre argumentos/resultados auditáveis, confronte amostras no domínio Gregorian demonstrado com cálculo independente e confirme ausência de fallback normal. Observe o weekday na data corrente e em fixtures úteis; nomes localizados devem continuar sob controle das rotinas originais. Observe também informações úteis de jogador/contrato sem reconstruí-las.

Valide Save/saída normal/processo ausente/reabertura exata/Load antes e depois de um avanço normal que mude o dia da semana. Partindo de Sunday September 1 afternoon, prefira passos limitados de Continue Game e acompanhe até a primeira data seguinte pertinente; Sunday->Monday é o caso desejado, sem editar ou forçar September 2. Se férias forem usadas, acompanhe e interrompa no primeiro intervalo pertinente; reporte data/fase efetivamente alcançada. Se houver partida, use o fluxo normal. Não repita outra temporada ou imponha nova virada de mês/ano como pré-requisito.

Confira após cada Load data/weekday/fase, clube, fixtures/resultados e dados de jogador/contrato observados antes de salvar. Use novos nomes de saves de validação. Use original Game Options -> Save Game; File -> Save nativo continua limitado (`Native menu selections require original MenuSelect dispatch`) e fora do escopo. Recrie bindings CUA/índices AX após reabertura e confira processo ausente antes de lançar outra instância. Se o Mac estiver bloqueado, documente o bloqueio sem tratar automáticos como prova de UI/persistência.

## Testes e critério de conclusão

Execute novo grupo direto, regressões das sete operações, auditorias e suíte completa no candidato final, com UBSAN do adapter/gerador alterados. Confirme layout final de State em TODOS os objetos realmente ligados. Se adicionar campos ou mudar Memory/State, recompile completamente, estabilize headers antes do build e não execute sanitizer/harness contra archive de layout anterior. Só prepare candidato final após estabilizar fonte/build. ASan histórico não aprova weekday.

Comandos de referência, a reconferir e ampliar no estado encontrado:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake --build .local/aot-build --clean-first -j 4
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/check_calendar_season.py .local/performance/season-day-01/SeasonFixture
python3 tools/check_calendar_season.py .local/performance/season-week-01/SeasonFixture
python3 tools/check_calendar_season.py .local/performance/date-ordering-capture-02/SeasonFixture
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

Para os fixtures, use novos destinos privados e a opção de streams históricos ampliada; mantenha os formatos anteriores intactos. Documente regeneração, novo harness/stream/baseline, comandos UBSAN repetíveis e páginas instrumentadas, incluindo agora `0x10715`. Diferencie testes pulados dos opcionais explicitamente executados, e resultados novos dos históricos. Não acrescente otimizações para reduzir o tempo da tarefa.

Só conclua após contrato demonstrado, produção semântica, equivalência direta/delimitada, negativos, cobertura real própria, persistência com mudança de weekday, suíte final e instalação aprovados. Um contador isolado, green tests ou commit não substituem prova de aplicativo. Se houver bloqueio essencial, documente sem declarar entrega completa.

## Proteção, documentação e entrega

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks/Finder metadata, atributos estendidos, modos, modification times e flags. Faça inventários atuais antes/preinstall/depois sem assumir que 809/5/43 é invariável. Preserve NOVA cópia privada executável da referência instalada atual antes de substituí-la; os backups antigos têm identidades diferentes e não cumprem isso.

Exports, dados originais, saves novos, snapshots, transcripts, logs, capturas e builds continuam privados e ignorados. Não mude ignore/LFS nem publique artefatos privados. Preserve imutável `saves/Mac-Teste-Validado-Pos-Jogo.sav`, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`, todos os checkpoints/streams/fixtures históricos e os prompts preexistentes.

Documente em inglês contrato, enumeração e domínio, particularidades de anos/flags/restos, ABI/carry/leituras, arquitetura, destino gerado/callers, testes/negativos, contagens próprias, observações exatas da UI e persistência. Separe hashes de fonte/candidato assinado/instalado/backup; registre evidência de preservação. Atualize `docs/native-port-status.md` e acrescente `docs/native-date-weekday-reconstruction.md`, preservando relatórios históricos e a melhoria bissexta adiada.

Entregue instalação ARM64 validada com o instalador existente, preservando conteúdo/metadados pessoais. Revise o diff exato, commit/push normal sem force segundo a autorização permanente para trabalho concluído/validado, e confira SHA remoto por consulta nova. Essa autorização se aplica à futura implementação validada; preparar este handoff não autoriza instalar/publicar agora nem publicar dados privados. Não declare reconstrução de todas as conversões/formatação de datas ou do restante do engine.
