/goal Reconstrua semanticamente em C++23 a entrada independente `0x10715a30` do CM 01/02 Mac, candidata ao ajuste de uma data para o dia da semana solicitado. Integre-a à produção ARM64 e execute este incremento até a entrega validada, com equivalência do contrato legado, chamadas reais próprias e Save/Load. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, documentação técnica, testes e novas mensagens do aplicativo em inglês.

## Produto e próximo incremento

O produto é o CM 01/02 completo, jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em código semântico compreensível. A referência atual recompila estaticamente o original Mac; não significa que todo o engine tenha sido reconstruído semanticamente. A temporada real, passagem anual e persistência correspondente estão entregues em `docs/native-season-reference.md`; não repita esse marco como pendência.

Adição, subtração, diferença, later, before, mês, dia do mês e weekday estão entregues. O próximo incremento escolhido é **somente o ajuste para weekday `0x10715a30`**: compor as operações já reconstruídas no caminho real de preparação de datas de fixtures, preservando quais datas o jogo produz. A proposta inicial é o dia solicitado mais próximo, mas essa interpretação ainda precisa de caracterização diferencial própria. Não trate a hipótese de agendamento como regra estabelecida nem acrescente regra nova ao calendário.

Construção geral de datas nas rotinas vizinhas, formatação e nomes localizados, outros comparadores, idade, contratos, transferências, agendador/competições como subsistemas, jogadores, gols/placar e demais rotinas ficam fora do escopo. Inspecionar callers ou observar a UI não autoriza reconstruí-los. Novas otimizações, features de gameplay e mudanças gráficas permanecem suspensas; não prometa ganho de velocidade.

Preserve a peculiaridade bissexta da diferença explicitamente adiada: January 1 2000 menos January 1 2001 retorna -365; a inversão retorna +366. Qualquer correção exige decisão separada, análise dos callers e validação de gameplay/persistência. Preserve também os restos negativos de weekday antes do epoch, flags inconsistentes e comportamento inválido legado; não normalize para Gregorian fora do domínio demonstrado.

## Ponto de partida verificado na preparação

- `main`, HEAD e consulta nova a `origin/main` concordaram em `b6ce6ceb7c961a1f0d4d41f032b21117a4ae5cde`, entrega de weekday. Reconfira Git, remoto, diff e `AGENTS.md` aplicáveis. Não havia `AGENTS.md` nos ancestrais ou árvore do projeto inspecionada.
- Estavam untracked os quatro prompts `docs/features/native-date-difference-reconstruction-prompt.md`, `native-date-month-reconstruction-prompt.md`, `native-date-day-of-month-reconstruction-prompt.md` e `native-date-weekday-reconstruction-prompt.md`. Preserve-os. Este quinto handoff também pode estar pendente; preparação de prompt não é mudança de produto entregue.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` foram reconferidos ARM64, executável SHA-256 `7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063`.
- O candidato privado assinado da entrega anterior `.local/performance/date-weekday-live-01/CM Validation date-weekday-live-01.app` foi reconferido ARM64, executável `ace1c411a6e863fcec0125105f763df7e81543df53ae440c151b82580060db14`. Não confunda candidato assinado com fonte/instalado.
- `.local/date-weekday-reconstruction/reference/installed.app` foi reconferido ARM64, executável `1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc`. Preserva a referência anterior a weekday, NÃO o instalado atual. Antes de trocar builds/instalar, crie uma **nova** cópia privada executável do instalado atual, preservando resource forks e extended attributes; confira identidade e inventário completo. Os backups antigos continuam imutáveis.
- O relatório entregue `docs/native-date-weekday-reconstruction.md` registra 4.240 casos diretos com ABI/memória completos, 559 chrono, 319.675 oracle Gregorian amplo, 786.432 casos puros, leituras ordenadas, mappings mínimos, independência da TOC, callers sintéticos, callbacks e negativos. A suíte final aprovou 36/36 CTest; descoberta Python 30 testes, 26 aprovados e quatro grupos opcionais pulados, também executados explicitamente. UBSAN gerado/focado passaram; 1.969 objetos ligados dependentes de State e 22 outros usaram o layout final. São resultados da entrega anterior, NÃO aprovação do ajuste. Não reexecute suítes caras somente para planejar.
- Day/week/ordering aprovaram dois originais estritos, captura inteiramente original, seis baselines e dois candidatos. Weekday próprio por candidato: 1.690 / 5.831 / 1.419. Os quatro streams privados são CMCAL002, CMMON001, CMDOM001 e CMWKD001. Os três primeiros permaneceram byte-idênticos aos históricos de dia do mês. Negativos exercitados foram rejeitados; later/before permanecem sem cobertura em day/week e cobertos em ordering.
- A UI ARM64 anterior aprovou Save/saída normal/processo ausente/reabertura exata/Load em Sunday September 1, 2002 afternoon e Tuesday September 3 morning. Após ler a fila de notícias antigas, um Continue Game normal saltou a segunda-feira e parou em notícia de proposta de transferência; September 2 não foi forçado. Três runs registraram 160 / 1.794 / 152 weekday próprios e 96 amostras válidas conferidas. O relatório registra preservação de 809/5/43 arquivos Game/Runtime/Runs, com metadados, e 745 identidades históricas. Quantidades observadas não são invariantes nem inventários novos deste incremento.

Esta preparação foi somente leitura, seleção e escrita do handoff. Não implementou ajuste, não recompilou/instalou o produto nem abriu outro chat. Reconfira todo estado que possa ter mudado.

## Leia e inspecione primeiro

Leia `README.md`, `docs/native-port-status.md`, `docs/native-season-reference.md`, `docs/native-calendar-reconstruction.md` e os relatórios `docs/native-date-{difference,ordering,before,month,day-of-month,weekday}-reconstruction.md`, incluindo a melhoria adiada de diferença. Os relatórios versionados são contexto reproduzível; não dependa desta conversa ou apenas de logs temporários.

Inspecione `native/calendar.hpp`, `native/calendar_adapter.*`, `native/calendar_checks.cpp`, `native/calendar_reference_checks.cpp`, `native/date_weekday_checks.cpp`, `native/date_day_of_month_checks.cpp`, `native/date_month_checks.cpp`, `native/date_difference_checks.cpp`, `native/date_ordering_checks.cpp`, `native/callbacks.cpp`, `native/aot_runtime.*`, `native/aot_probe.cpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*` e `native/CMakeLists.txt`.

Inspecione também `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/test_generated_temporaries.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`. Use `.local/recomp-input/instructions.sqlite` read-only e o gerado privado para caracterizar o original. Exports/payloads não estão disponíveis em clone público limpo; não os publique nem altere para facilitar testes.

## Fronteira candidata e hipótese a demonstrar

A inspeção estática read-only encontrou **59 instruções contíguas** em `[0x10715a30,0x10715b1c)`, retorno final em `0x10715b18`. A rotina anterior retorna em `0x10715a24`; há gap antes da entrada e outro antes de weekday `0x10715b20`. Não foram encontrados flows externos indexados para o interior. Reconfirme limites, fallthrough, todas as saídas, possibilidades indiretas e qualquer metadata; análise estática não esgota alvos indiretos.

Fingerprint com os campos exatos de `tools/calendar_audit.py`: `bb8d3fd075cea7d4187bfbeca8bcd9a73185ee8f731dd2ebf6b077166f0381c9`. Índice SHA-256 `2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec` e `program.json` `50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0` foram reconferidos. Proveniência: 1.953.157 instruções e PEF original `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`. Não publique listagens originais.

Há 11 `bl 0x10715a30` indexados: mesma página `0x10715860`, `0x10715a10`; entre páginas `0x10446638`, `0x10451048`, `0x104511e0`, `0x1048227c`, `0x10488ab8`, `0x10489d40`, `0x1048d270`, `0x104a0f30`, `0x104a2b24`. Os dois callers da mesma página já emitem `goto i_10715a30`; ambos fornecem entrada e saída iguais. Um caller entre páginas inspecionado fornece requested weekday 0 com entrada/saída iguais. Esses exemplos não estabelecem o contrato de todos os callers.

A hipótese estática é r3=ponteiro da data de entrada, r4=requested weekday e r5=ponteiro da saída. A rotina salva r29/r30/r31 e LR, cria frame de `0x60`, chama weekday em `0x10715a54`, copia ano/dia/flag da entrada para a saída **nessa ordem**, compara o weekday e usa subtração de calendário em `0x10715aa8` ou `0x10715ae4` quando precisa ajustar. Os temporários de saída estão em frame+`0x40` ou +`0x38`. O caller `0x10715a54` e ambas as chamadas de subtração entram nas operações já entregues.

Para inputs válidos com Monday=0..Sunday=6, o delta aparente é weekday atual menos requested weekday, estreitado signed16; se menor que -3 soma 7, se maior que 3 subtrai 7, e usa **subtração com offset signed**, que pode delegar à adição. Isso sugere escolher o weekday solicitado mais próximo, em ±3 dias, com igualdade preservando a data. Demonstre com diferencial e oracle independente antes de adotar nomes/contrato. Não substitua a regra por “sempre próximo”, “sempre futuro” ou modulo Euclidean, nem generalize a hipótese para requests inválidos.

Exemplos a demonstrar no domínio válido: Sunday September 1, 2002 solicitado como Monday deve produzir September 2; solicitado como Friday deve produzir August 30; solicitado como Sunday deve preservar a data. Esses exemplos de teste não autorizam forçar tais datas na carreira real.

Resolva a diferença entre comparação signed16 e r4 bruto completo, high bits não canônicos, estreitamentos e correção por sete feita uma única vez. Caracterize targets 0..6, -1, -32768/32767, 7 e outros fora do enum, zeros/flags 1/128/255/inconsistentes, weekday negativo original, anos 0/1/1599/1600/1601/1900/2000/2100/extremos unsigned16, dias signed16 negativos/inválidos/extremos, limites mensais/anuais e February 28/29/March 1. Use budgets finitos para caminhos extremos; não acrescente clamp ou tratamento inexistente para tornar todos os inputs válidos.

O resultado na data não basta. Demonstre r3 e todos os clobbers reais: no ramo de igualdade e nos dois ramos de ajuste podem diferir, e não se deve inventar retorno “ponteiro de saída”. Compare registros/temporários completos, CR/XER/carry, LR/CTR, TOC/context TOC, PC, stack/backchain/spills, bytes de padding e memória inteira, alocador, imports/callbacks e RNG. `call_original` restaura contexto do callback e não substitui equivalência da entrada direta.

Entrada=saída é uso real. Cubra saída separada e sobreposição parcial nos dois sentidos; a cópia ordenada ano→dia→flag não pode virar memcpy ou snapshot antecipado de toda a entrada. Cubra aliases com spills/frame, os dois temporários locais, scratch/flags das operações de shift e slots TOC quando o original permite execução bem definida. Demonstre leituras/escritas efetivas em ordem; marshalling de callback e leituras extras de diagnóstico ficam documentados separadamente. Delimite mappings ausentes, wrapping de endereços e mutação concorrente sem alegar equivalência universal.

## Implementação e composição das operações entregues

Amplie minimamente a lógica pura e o adapter existente. A produção normal deve executar ajuste semântico, compondo weekday e shifts semânticos já entregues, com ABI/efeitos legados preservados. Não use um wrapper que execute o corpo gerado de `0x10715a30` como implementação normal; não introduza nova emulação, selector na UI ou abstrações especulativas. Diagnósticos podem selecionar somente a entrada original que precisam comparar.

Instale o hook no **destino gerado** `0x10715a30`, página `0x10715`, cobrindo os dois gotos reais, dispatch entre páginas, indiretas e callbacks. Audite o corpo completo e rejeite fragments/instruções alteradas/saídas ausentes. Não substitua corpos vizinhos ou callers por busca textual. Preserve os oito hooks, contratos e manifesto entregues; a nova entrada deve aparecer explicitamente no manifesto.

Esta entrada é um **pai de chamadas já auditadas**. Preserve as fronteiras internas de weekday/subtração, incluindo LR/contexto, frame, registros, efeitos, ordem e contadores; não calcule só a data final pulando os filhos. Captura inteiramente original deve registrar também os filhos originais; a seleção original do pai em um baseline com oito operações modernas não pode desativar semanticamente os filhos nem trocar seu modo inadvertidamente. Examine o guard compartilhado de auditoria/recursão antes de reutilizá-lo. A solução deve manter os streams históricos verificáveis, sem máscaras ou relaxamento de seus campos.

Acrescente contador/amostras **próprios do ajuste**, com entrada, pointers de entrada/saída, r4 bruto/interpretado, data antes/depois, ramo/delta demonstrados e retorno/caller. Contadores de weekday/subtração não provam execução do ajuste semântico. Campos diagnósticos devem continuar fora de saves/snapshots, e tracing de leituras permanecer desabilitado em produção normal.

Separe inteiramente original, baseline entregue com oito operações modernas e ajuste original, e candidato com nove entradas modernas. Preserve significados históricos:

| Modo | Operações modernas anteriores | Ajuste |
| --- | --- | --- |
| Default reference | nenhuma | original |
| `--calendar-baseline` | add/subtract | original |
| `--date-ordering-baseline` | add/subtract/difference | original |
| `--date-before-baseline` | add/subtract/difference/later | original |
| `--date-month-baseline` | add/subtract/difference/later/before | original |
| `--date-day-of-month-baseline` | anteriores mais month | original |
| `--date-weekday-baseline` | anteriores mais day-of-month; weekday original | original |
| Nova referência, por exemplo `--date-weekday-adjustment-baseline` | todas as oito, incluindo weekday | original |
| `--modern` | todas as oito | semantic |

Captura de CLI **e de aplicativo** deve selecionar as nove entradas originais. Proíba gravação de candidato como oracle. Teste original/baseline/candidato diretamente, inclusive combinações necessárias para isolar efeitos do pai e dos filhos, sem alterar os significados históricos da CLI.

## Fixtures, streams e negativos

Reexecute os três fixtures imutáveis `.local/performance/season-day-01/SeasonFixture`, `.local/performance/season-week-01/SeasonFixture` e `.local/performance/date-ordering-capture-02/SeasonFixture`. Exija dois originais estritos, captura inteiramente original, todos os **sete** baselines pertinentes e duas execuções candidatas equivalentes por fixture. Preserve somente a exceção existente de instruções traduzidas; não masque PC, registradores, temporários, memória, alocador, imports/callbacks/RNG. EOF e término pelo transcript/estado original mantêm budget finito.

Durante a preparação, a leitura dos CMWKD001 inteiramente originais da entrega anterior encontrou 1.212 / 2.491 / 943 bordas de weekday com LR=`0x10715a58`, retorno interno do ajuste. Isso é evidência do caminho **original do pai**, sem contador próprio do novo ajuste. Demonstre coverage própria após implementação e confira a relação com chamadas filhas; não transforme esses números em uma aprovação do candidato.

Crie stream privado independente, com header explicitamente versionado (por exemplo `CMWAD001`), para os inputs/output/ABI/efeitos necessários à fronteira do ajuste. Mantenha CMCAL002/CMMON001/CMDOM001/**CMWKD001** intactos. Use novos destinos privados e registre SHA-256 antes/depois. A recaptura inteiramente original dos quatro arquivos deve ser byte-idêntica a `.local/date-weekday-reconstruction/{day,week,ordering}-edges-01`; os hashes completos constam no relatório entregue. Amplie o checker para verificar **os quatro** explicitamente, preservando usos históricos da ferramenta. Não confunda aceitar uma opção com comparar cada arquivo.

Mantenha os negativos RNG, shifts, difference, later, before, month, day-of-month e weekday onde exercitados. Acrescente negativo isolado que modifica somente o primeiro resultado observável próprio do ajuste, por exemplo o day field da saída, sem perturbar uma operação filha. Exija rejeição na primeira fronteira de ajuste, com entrada/field/offset exatos identificados, antes de uma divergência posterior. Confirme ausência de vazamento da perturbação para execução normal. Teste header/body truncados, checksum, EOF/bordas não consumidas e enforcement de captura original.

Reporte contagens próprias: zero significa ausência de cobertura. Se os fixtures não cobrirem a entrada própria, capture uma sequência real mínima pertinente **inteiramente original**, usando as ferramentas existentes. Não edite snapshot/data/RNG, não recapture candidato como oracle e não exija carreira inteira. Dados privados e histórico permanecem imutáveis.

## Aplicativo real e persistência

Use uma única instância ARM64 por vez, candidato/Runtime/Runs novos preparados pelas ferramentas existentes. Não sobrescreva `date-weekday-live-01` nem candidatos antigos. O checkpoint preferido é `.local/performance/date-weekday-live-01/Runtime/Files/games/DateWeekday-03Sep2002.sav`, SHA-256 reconferido `24bc76c79e4ebba79b5725fe01dc28181590aaa59ce2127257af8708b28d25ed`. Alternativa preservada: `DateWeekday-01Sep2002.sav` na mesma pasta, SHA-256 `bbbc4426afdfd23569d5a0784da64c0da4a46d5088a702630e022d860a704351`. Preserve ambos e todos os checkpoints anteriores.

O checkpoint preferido foi observado como Tuesday September 3 morning (`3.9.02 MNH`), Victor Moraes/Cruzeiro, na notícia de proposta do Grêmio por Abedi. Confirme isso no candidato novo. O calendário observado tinha Goias em August 31 0:3 e Coritiba/Santos/Colo Colo pendentes em September 6/8/11; Cruzeiro 16º nacional. Luisao: nascimento February 13, 1981, idade 21, uma internacionalização, condição 89%, 26 (4) jogos oficiais, um gol, duas assistências, média 7.20, forma 8-7-9-8-6, moral Good. Contrato full-time, BRL 8.750/semana, início N/D, vinculado Sim, expira December 24, 2004, rotação, sem prêmios/cláusulas. São observações históricas a reconferir, não asserts cegos sobre o candidato.

Guie a interação pelos callers/amostras: Load e preparação de datas de fixtures já exercitaram o pai original. Demonstre chamadas **semânticas próprias do ajuste** com inputs/output/caller auditáveis na interação normal. Opening calendar isoladamente não basta. Confronte amostras no domínio válido com cálculo independente de weekday/data escolhida, e preserve outputs/flags inválidos conforme o original. Registre resultados iguais, avanços/recuos curtos e caller real que ocorrerem; ramos restantes devem ficar delimitados pela prova direta/fixtures. Não manipule datas ou saves para forçar ramos da UI.

Valide Original Game Options → Save Game, saída normal, processo ausente, reabertura exata e Original Load antes e depois de uma progressão normal limitada que exercite o ajuste. Use novos nomes de save e documente a data/fase efetivamente alcançada. Se a proposta pendente exigir decisão, siga a interface normal e registre a escolha; não force transferência, partida, date boundary ou RNG. Não imponha nova temporada, virada mensal/anual ou um dia exato. Se houver partida no intervalo pertinente, use o fluxo normal e pare após a evidência necessária.

Após cada Load confira data/weekday/fase, clube, fixtures/resultados e os dados de jogador/contrato observados imediatamente antes do respectivo Save. Diferencie mudanças normais de condição/estatísticas das perdas de persistência. File → Save nativo continua limitado por `Native menu selections require original MenuSelect dispatch` e fora deste escopo. Recrie bindings CUA e índices AX após reabertura; confira ausência de processo antes de lançar outro app. Se o Mac estiver bloqueado, documente o bloqueio sem tratar testes automáticos como prova de aplicativo.

## Testes e critérios de conclusão

Execute o novo grupo direto, regressões das oito entradas, auditorias e suíte completa no candidato final, com UBSAN de adapter/gerador alterados. Use tracing ordenado de leitura e observação de mapping/write/clear para footprint completo, inclusive efeitos dos filhos e aliases. Compare toda ABI/memória/stack, não só a data escolhida. Testes de callers com inputs controlados são sintéticos; chamadas reais ficam documentadas separadamente.

Se crescer State ou mudar Memory/headers, estabilize headers e recompile completamente; confirme o layout em **todos os objetos efetivamente ligados**. Não ligue harness/sanitizer a archive de layout anterior. Só prepare candidato final depois de estabilizar fonte/build. Execute checks apropriados após mudanças reais; ASan histórico não aprova este ajuste. Não introduza otimizações para reduzir o custo de validação.

Comandos existentes a reconferir/ampliar, acrescentando harness/auditoria/stream/baseline próprios:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
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

Para os fixtures use novos destinos privados e a verificação histórica ampliada aos quatro streams. Documente os comandos exatos finais, UBSAN repetível, páginas instrumentadas, grupos opcionais pulados versus explicitamente executados, casos e contagens próprias. O relatório de weekday já contém uma receita focada que instrumenta adapter e páginas `0x10331`, `0x10332`, `0x10715`, `0x10717`, `0x10718`; amplie-a para o novo harness e dependências efetivamente alteradas. Não alegue instrumentação de bridges ou programa inteiro sem executá-la.

Só conclua após contrato demonstrado, produção semântica, equivalência direta/delimitada com composição dos filhos, negativos próprios, quatro streams históricos byte-idênticos, coverage real própria, Save/Load antes/depois da progressão, suíte final e instalação aprovados. Código/contador isolado, green tests ou commit não substituem prova do aplicativo. Se faltar condição essencial, relate-a sem declarar entrega completa.

## Proteção, documentação e entrega

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks/Finder metadata, atributos estendidos, modos, modification times e flags. Faça inventários atuais antes/preinstall/depois sem assumir 809/5/43 como invariável. Preserve a nova cópia executável da referência instalada atual; os backups históricos possuem outras identidades e não a substituem.

Exports, dados originais, saves novos, snapshots, transcripts, logs, capturas e builds continuam privados e ignorados. Não mude ignore/LFS nem publique artefatos privados. Preserve imutável `saves/Mac-Teste-Validado-Pos-Jogo.sav`, 17.917.514 bytes, SHA-256 reconferido `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`; todos os checkpoints/streams/fixtures históricos; os quatro prompts preexistentes e este handoff.

Documente em inglês o contrato, domínio/regra demonstrados, particularidades signed16/request bruto/flags, branches, ABI/stack/aliases, composição dos filhos, hooks/callers, limites, testes/negativos e contagens próprias. Registre exatamente as observações da UI e persistência, os hashes separados de fonte/candidato assinado/instalado/nova referência e os inventários de preservação. Acrescente `docs/native-date-weekday-adjustment-reconstruction.md` e atualize `docs/native-port-status.md`, preservando relatórios históricos e a melhoria bissexta adiada.

Entregue a instalação ARM64 validada com o instalador existente, preservando conteúdo/metadados pessoais. Revise o diff exato, exclua prompts pendentes e arquivos privados do commit de produto, faça commit/push normal sem force segundo a autorização permanente para trabalho concluído/validado, e confira SHA remoto por consulta nova. Essa autorização se aplica à futura implementação validada; preparar este handoff não autoriza instalar/publicar agora nem publicar dados privados. Não declare o restante do calendário, agendador ou engine reconstruído.
