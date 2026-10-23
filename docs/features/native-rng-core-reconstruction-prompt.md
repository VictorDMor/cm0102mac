/goal Reconstrua e entregue o núcleo de estado e inicialização do RNG do CM 01/02 Mac em C++23 semântico na produção ARM64. Implemente conjuntamente as três entradas abaixo, preserve os consumidores originais do pool e as 34 entradas Date entregues, valide a sequência e os efeitos originais e faça uma única instalação/entrega ao fim. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto e decisões vigentes

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em C++ compreensível. O jogo já funciona pelo caminho ARM64 de recompilação estática e bridges nativos. O próximo bloco é o núcleo RNG: estado do gerador, aplicação de seed e instalação do cursor/salt/limite do pool. O resultado observável é continuar jogando, fazendo sorteios, salvando e carregando com o comportamento original preservado enquanto essas três entradas passam a executar código semântico.

Entregue o bloco inteiro neste goal, com checkpoints internos; não transforme cada rotina em um novo prompt. Gameplay, regras, gráficos, otimizações e melhorias estatísticas do gerador continuam fora do escopo. Não use emulação, Wine, CrossOver, Rosetta-only ou VM.

Decisões humanas mais recentes: o usuário pediu avançar com a evidência prática disponível e encontrar eventuais bugs durante o uso, dispensando os testes prolongados restantes de Date. Também pediu limpar temporários, porque a execução ocupou quase todo o storage. A próxima aceitação deve ser proporcional ao bloco: contratos diretos completos, uma comparação integrada pertinente, cobertura real e persistência, sem reabrir a campanha semanal extensa de Date ou repetir campanhas caras sem uma falha concreta. Preserve e informe limitações; não invente PASS nem descarte divergências reais. A dispensa anterior não constitui aprovação automática de futuras falhas.

## Ponto de partida verificado em 4 October 2026

- `main`, HEAD e uma consulta nova a `origin/main` concordaram em `daba5f30b6895cc5ecba5d03c40cb337868f5cda`, entrega do subsistema Date. Reconfira branch, remoto, diff e instruções aplicáveis antes de trabalhar. Não havia `AGENTS.md` aplicável no projeto inspecionado.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` foram reconferidos ARM64, executável SHA-256 `a35a303f441d352d61e0ab0a8e154e06f14f747461310c9e6ac28ce36abaf1e5`.
- Date está entregue: 26 entradas novas e oito anteriores, manifesto de 34 entradas semânticas em produção. O relatório versionado registra 6.871 comparações diretas de ABI/efeitos, 104 acordos de streams compostos, 52 negativos próprios, 26 pares de callbacks, 39/39 CTest, descoberta Python com 35 aprovados/quatro grupos opcionais pulados e esses quatro executados explicitamente, além de UBSAN gerado/focado. São resultados da entrega Date, não aprovação deste novo núcleo RNG.
- Day e ordering passaram as campanhas completas de Date. A captura original semanal e dois replays semânticos concluíram; baselines/negativos restantes e a matriz final semanal foram interrompidos e dispensados expressamente pelo usuário. Não os marque como aprovados nem imponha sua conclusão como gate deste bloco.
- Duas progressões reais Save/normal exit/process absence/exact reopen/Original Load de Date passaram, com cobertura própria das 26 entradas novas. O checkpoint preferido atual é `.local/performance/date-subsystem-live-01/Runtime/Files/games/DateModule-04Sep2002.sav`, 34.557.282 bytes, SHA-256 `0d0ca840ef270751a2c8537191aacff301c52fb653f34c9bb5171d172fdcf86a`. A alternativa preservada `DateModule-03Sep2002.sav` na mesma pasta tem 34.464.680 bytes e SHA-256 `ce1b006a19508ff43c2507e7f7855a0dac9e9b8f1887c53ea8931a9a3138c446`. Ambos foram novamente conferidos durante esta preparação.
- O checkpoint preferido foi observado em Wednesday September 4, 2002 night (`4.9.02 NTE`), Victor Moraes/Cruzeiro. Datas/fases, fixtures/resultados e os dados observados de Luisao/contrato estão detalhados no relatório Date. Reconfirme-os no novo candidato; não use observações históricas como asserts cegos.
- A nova referência `.local/date-subsystem-reconstruction/reference/installed.app` preserva o executável anterior a Date, SHA-256 `7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063`. Ela NÃO é referência do instalado atual. Crie outra referência privada do instalado atual antes de trocar builds/instalar, preservando forks/xattrs e inventário completo. Não altere referências antigas.
- Permanecem sete prompts Date untracked em `docs/features`: `native-date-{difference,month,day-of-month,weekday,weekday-adjustment,construction-and-adjustment,subsystem}-reconstruction-prompt.md`. Preserve-os e exclua-os do commit de produto, assim como este handoff.
- A limpeza anterior removeu 368 artefatos gerados, cerca de 18,9 GB. Capturas grandes CMDAT001, arquivos de caracterização comprimidos e objetos/binaries temporários podem estar ausentes: recapture/recompile somente se necessário. Os originais, fixtures imutáveis, saves, referências, candidatos e relatórios/recibos foram preservados. Na preparação havia cerca de 28,4 GB livres; reavalie o espaço antes de produzir dados.

Esta preparação foi somente inspeção e handoff. Não implementou RNG, não abriu aplicativo, não instalou nem criou uma nova sessão.

## Leia e inspecione primeiro

Leia `README.md`, `docs/native-port-status.md`, `docs/native-season-reference.md`, `docs/native-date-subsystem-reconstruction.md` e a melhoria bissexta adiada em `docs/native-date-difference-reconstruction.md`. Preserve o contrato assimétrico de difference: January 1 2000 menos January 1 2001 é -365; a inversão é +366. Não corrija esse comportamento neste bloco.

Inspecione `native/aot_runtime.*`, `native/aot_probe.cpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/background_checks.cpp`, `native/day_advance_checks.cpp`, `native/match_arithmetic_checks.cpp`, `native/calendar_adapter.*`, `native/date_subsystem_adapter.*`, `native/date_subsystem_audit.*` e `native/CMakeLists.txt`.

Inspecione `tools/compile_recomp.py`, `tools/check_season_reference.py`, `tools/check_calendar_season.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/test_generated_temporaries.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`. Use `.local/recomp-input/instructions.sqlite` read-only e o gerado privado; não publique corpos/listagens, tabelas proprietárias, snapshots ou saves.

A referência Windows ReCM fica no `RECM_SOURCE_DIR` de `.local/native-checks/CMakeCache.txt`; leia suas instruções antes de inspecioná-la e nunca a edite. `src/cpp/engine/rng.*` pode ajudar com responsabilidades, mas seu LCG VC6, pool int32/little-endian e serviços Windows NÃO estabelecem o contrato Mac. O Mac usa outro multiplicador e um pool de amostras unsigned16/big-endian. Não transplante o RNG Windows.

## Escopo fechado: três entradas relacionadas

A inspeção estática read-only encontrou os seguintes corpos completos, 77 instruções no total. Os fingerprints usam a mesma normalização `address/assembly/ops/flows/fallthrough` de `tools/calendar_audit.py`. Reconfirme saídas, entradas internas/indiretas e efeitos antes de implementar; static calls não provam runtime coverage.

| Entrada | Fim exclusivo | Instruções | Calls diretos indexados | Responsabilidade |
| --- | --- | ---: | ---: | --- |
| `0x1077b770` | `0x1077b790` | 8 | 3.728 | Step do gerador CRT Mac |
| `0x1077b790` | `0x1077b798` | 2 | 7 | Aplicar seed ao estado CRT Mac |
| `0x107474a0` | `0x107475ac` | 67 | 4 | Seed do jogo e instalação de cursor/salt/limite do pool |

Fingerprints:

- `0x1077b770`: `947d4acb79bb75eb056f1720faf3b72a658c702e3390df399e637ed8b3b54760`
- `0x1077b790`: `b5ea8440c2bbee7a391956377ff65b3816799c5b73fe78852d08bb6671eb0cb8`
- `0x107474a0`: `b189ae6daae4a6fc7170607de6a91f982df7bba630bf562ad1792497003203db`

### Gerador e seed CRT

`0x1077b770` lê e escreve o estado uint32 no slot relativo `r2-0x34c4` (no TOC padrão, `0x108b85cc`). A sequência observada é `state = state * 0x41c64e6d + 0x3039`, com wrap uint32, retornando `(state >> 16) & 0x7fff`. `0x1077b790` grava r3 no mesmo slot e retorna sem chamar outra rotina. Demonstre todos os resíduos de registradores/temporários, CR/XER, LR/CTR/PC e ordem de acesso: a fórmula isolada não prova a ABI. Não use estado estático C++ paralelo, `std::rand`, distribuições modernas nem endereço absoluto que ignore TOC relocável.

### Inicialização do pool

`0x107474a0` recebe seed em r3; zero seleciona o serviço original de tempo `0x1077f900`. Chama o seed CRT, o serviço original de logging `0x1077b720` e o gerador duas vezes. Instala o cursor a partir de `poolBase + 2 * (draw % 51000)`, o limite inclusivo `poolBase + 2 * 50999` e o salt derivado do segundo draw módulo 65535, com a verificação original de intervalo do cursor. Preserve a ordem exata, widths, signed arithmetic intermediária, r3 residual, frame/spills e aliases; essas descrições são pontos de partida da caracterização, não autorização para substituir efeitos por fórmulas equivalentes apenas no domínio comum.

Os slots de estado observados são `r2-0x7b90`/`0x108b3f00` (ponteiro para salt), `r2-0x7b8c`/`0x108b3f04` (base do pool), `r2-0x7b88`/`0x108b3f08` (ponteiro para end) e `r2-0x7b84`/`0x108b3f0c` (ponteiro para cursor). Diferencie slot, ponteiro e valor. Tempo/logging continuam dependências geradas/bridges originais: execute-os com os mesmos argumentos, LR/contexto, callbacks/imports e efeitos. O serviço de tempo não é um novo RNG a implementar.

### Fronteira dos consumidores

O sampler limitado `0x107124e0` continua gerado, assim como escolhas de objetos/arrays e amostragem embutida nos callers. Ele foi inspecionado como consumidor: tem 350 instruções e três calls diretos, dois em seletores e um interno, com quatro subcalls ao gerador. Não inclua esses corpos, scheduler, competição ou match engine por estarem relacionados a RNG. Seus calls a `0x1077b770` devem alcançar o novo hook, inclusive na renovação do salt quando o cursor ultrapassa o limite. Preservar consumidores originais permite comparar a nova geração/initialization com a simulação existente. Não declare o RNG inteiro de todos os callers reconstruído: o bloco entregue tem exatamente as três entradas do manifesto.

## Implementação e prova dos contratos

Implemente lógica semântica legível e adapters mínimos no desenho existente. Hook nos destinos gerados das páginas `0x10747` e `0x1077b`, cobrindo gotos, dispatch, indiretas e callbacks. A produção normal usa as três entradas semânticas; seleção original fica privada e inequívoca. Não delegue seus corpos originais como fallback. Mantenha o manifesto Date de 34 entradas e acrescente estas três, totalizando 37 semantic replacements.

Compare diretamente original versus semântico para cada entrada: retorno, todos os registradores/temporários, CR/XER/LR/CTR/context TOC/PC, stack/backchains/spills/padding, toda a memória mapeada, alocador, imports/callbacks e ordem de leituras/escritas/map/clear. Execute dependências pelo caminho de ABI direta; `call_original` com restauração de contexto não substitui a comparação do corpo direto.

Cubra seeds 0/1/65535/65536/`0x7fffffff`/`0x80000000`/`0xffffffff`, sequências consecutivas, wrap uint32, TOC relocável e modificado, mappings mínimos, aliases com stack/spills/slots e ordem observável. Para initialization, cubra seed explícito, seed zero com serviço original de tempo controlado pelo harness/transcript, os dois draws, cursor no início/fim e as validações originais. Não chame relógio real em um teste que promete determinismo. Compare também a composição initializer → seed → rand e consumers → rand em wrap, sem counters fictícios.

Inclua negativos próprios nas três entradas: alterar só o primeiro resultado/estado observável daquela entrada e rejeitar na primeira fronteira própria, distinguindo negativos de filho e pai. Teste reset, seleção original, proibição de gravar candidato como oracle, truncation/checksum/EOF se houver novo stream. Um contador agregado de RNG writes não prova execução semântica de cada corpo.

Estabilize todos os novos campos de State juntos; counters/flags/samples não entram em saves/snapshots. Não replique registros/unique/memória completos em um arquivo por cada um dos milhões de draws. Use comparação completa nos harnesses diretos e evidência runtime compacta com counts por entrada, poucas amostras e comparação ordenada do estado; reutilize a verificação estrita de RNG já presente no transcript de temporada.

## Aceitação integrada proporcional

1. Após estabilizar a implementação, faça um rebuild consistente dos objetos dependentes de State, a suíte final e UBSAN focado nas unidades/páginas alteradas. Reexecute checks afetados durante desenvolvimento; não faça clean build + suíte + UI + instalação depois de cada rotina.
2. Use primeiro o fixture imutável `.local/performance/season-day-01/SeasonFixture`. Compare o baseline com as 34 entradas Date semânticas/RNG original versus candidato com Date/RNG semânticos, com pelo menos duas execuções equivalentes de cada modo, uma perturbação inicial do RNG e negativos próprios cobertos. Reconfirme fixture/executable identity e EOF. Só a contagem de instruções traduzidas pode diferir; PC, registradores/unique, memória, alocador, import/callback order e RNG consumption continuam estritos. Preserve o default reference all-original e os baselines Date existentes; use novas opções explícitas para os modos RNG.
3. Reutilize os transcripts existentes sem regravar o gigantesco CMDAT001 ou rodar todos os sete baselines de Date. Week/order ficam disponíveis para uma lacuna concreta ou regressão; uma extensão deve responder à falha observada, com stopping condition clara. Informe explicitamente entradas não exercitadas pelo replay. Initialization/seed podem ser provados na abertura/criação real isolada, sem inventar coverage do replay carregado.
4. Prepare um candidato ARM64 novo, com Runtime/Runs privados e uma instância por vez. Copie o save preferido preservando o original, faça Original Load, confira os dados do relatório Date e observe counts/amostras próprios das três entradas. A abertura normal pode prover seeding; se faltar um caminho, use uma nova carreira isolada e fluxo normal pertinente. Não force RNG, datas, resultados ou saves para atingir branches. Calls sintéticos são prova de contrato, não gameplay.
5. Faça Original Game Options → Save Game, saída normal, processo ausente, reabertura exata e Original Load antes/depois de uma progressão normal limitada. Confira data/fase, manager/club, fixtures/results e player/contract observados no Save. Preserve o comportamento original de RNG ao carregar: caracterize o que o save realmente serializa; não exija artificialmente que um estado CRT reinicializado no startup deva persistir. Replays determinísticos e reload real são evidências distintas.

File → Save nativo continua fora deste escopo (`Native menu selections require original MenuSelect dispatch`). Não imponha partida, virada mensal/anual ou temporada completa. Pare a UI após a prova necessária. Recrie bindings/AX após reabertura e confira ausência de processo antes de outra instância.

Comandos-base a adaptar ao novo harness/checker, depois de reconferir as opções atuais:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input" \
  -DCM_DATE_SNAPSHOT="$PWD/.local/performance/season-day-01/SeasonFixture/initial.snapshot"
cmake --build .local/aot-build --clean-first -j 4
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

As extensões específicas RNG devem ter comandos finais repetíveis no relatório; não trate esta preparação como execução dos novos checks. Instrumente runtime, adapters/harnesses e páginas modificadas no UBSAN focado; não alegue whole-program, bridges ou ASan sem execução correspondente. Não altere contas/registros do transcript para esconder divergências.

## Proteção, espaço e entrega única

Antes de trocar builds, inventarie atualmente `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, conteúdo, resource forks/FinderInfo, todos os xattrs, modos/mtime/flags, saves pessoais e históricos relevantes. Repita imediatamente antes e depois da instalação. Os números anteriores 809/5/43 e 858 identidades são observações históricas, não invariantes atuais. Preserve o LFS `saves/Mac-Teste-Validado-Pos-Jogo.sav`, 17.917.514 bytes, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`, ambos os saves Date preferidos, fixtures/transcripts, referências, candidatos anteriores e os oito handoffs pendentes incluindo este.

Planeje storage antes de capturar; rode tarefas grandes sequencialmente e acompanhe crescimento/espaço livre. Compare direct cases em memória ou por batches limitados. Após validar e registrar resultados/identidades, limpe apenas temporários reproduzíveis criados neste goal, com lista e bytes removidos no recibo. Não apague a árvore `.local`, backups, saves, original inputs ou histórico protegido. Guarde relatórios compactos e comandos de reprodução; diga exatamente quais payloads foram removidos. Essa limpeza está autorizada pela decisão humana já registrada acima.

Escreva `docs/native-rng-core-reconstruction.md` em inglês: manifesto das três entradas/37 semantic replacements, contratos e dependencies ainda originais, fingerprints, casos/negativos/ABI/efeitos, counts próprios por modo, amostras e limites de coverage, comparação day/EOF, UI/persistência, preservação e cleanup. Atualize `docs/native-port-status.md`; preserve os relatórios Date e sua limitação semanal dispensada. Não use apenas logs temporários como contexto para outra sessão.

Instale a versão aprovada com `tools/install_native.py` uma vez no fim. Verifique ARM64, source/signed/installed/new-reference SHA separados, inventários completos, bundled originals e ausência de diagnostic flags na produção. Registre horário efetivo da instalação e identidade do executável interno; a data da pasta `.app` pode permanecer antiga pela cópia de metadados e não prova qual build está instalada.

Revise o diff exato, exclua todos os handoffs pendentes e artefatos privados, faça commit/push normal sem force sob a autorização permanente de trabalho concluído/validado e confirme o SHA remoto por consulta nova. Não publique saves novos, original payloads ou logs; não mude ignore/LFS. Preparar este handoff não autoriza implementação, instalação ou publicação nesta sessão de planejamento.

Conclua quando as três entradas estiverem semânticas em produção, seus contratos/composição/negativos demonstrarem equivalência, o replay pertinente e os fluxos reais/persistência tiverem evidência, o bloco Date continuar preservado, e instalação, proteção, cleanup e entrega estiverem conferidos. Relate honestamente limites dos consumidores que permanecem gerados e dos caminhos não exercitados; não prometa um engine inteiro reconstruído ou ganhos de velocidade.
