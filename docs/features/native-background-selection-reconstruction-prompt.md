# Goal: reconstruir o núcleo de seleção de backgrounds em C++23 nativo

> **Adiado por decisão do usuário em 4 October 2026.** O próximo goal passou a ser [apresentação em telas atuais e comandos diretos](native-presentation-and-direct-commands-prompt.md). Este prompt permanece como referência de um bloco futuro; não o execute junto com a melhoria visual e de controle.

Implemente e entregue, em `/Users/victordmor/Projects/cm0102mac`, o núcleo de seleção das imagens de fundo do CM 01/02 Mac: quatro entradas relacionadas, desde a escolha em coleções até a seleção por categoria ou objeto. Preserve os 34 contratos Date e as três entradas RNG já entregues. Trabalhe em português com o usuário; código, documentação técnica, testes e mensagens do aplicativo devem ficar em inglês.

## Produto, prioridade e limite do bloco

O produto é o jogo completo e jogável nativamente em Apple Silicon, com a lógica original gradualmente substituída por C++23 compreensível. A versão ARM64 atual já permite jogar. Este passo deve manter a navegação, as imagens escolhidas, os caminhos sem imagem, o consumo de RNG e a persistência compatíveis enquanto as quatro entradas passam a executar lógica semântica em produção. Não redesenhe telas, troque imagens, acrescente efeitos nem prometa melhoria de desempenho.

Entregue um único bloco funcional, com checkpoints internos e aceitação final compartilhada. A preferência do usuário é um goal por subsistema coerente, sem transformar cada rotina em um novo handoff. O escopo aqui é o **núcleo de seleção**, não todo o carregamento, armazenamento ou desenho de backgrounds.

Este bloco vem depois do núcleo RNG porque seus consumidores de seleção continuam gerados e já possuem um harness de navegação/fallback de clube e competição. O sampler genérico `0x107124e0` permanece fora: a inspeção encontrou três calls diretos, dois em ramos de seletores cujo limite foi reduzido a signed16 antes de ser comparado com ±65535, e um recursivo interno. Isso indica uma lacuna de alcançabilidade no domínio normal, não prova ausência de todos os callers indiretos. Não acrescente uma entrada sem caminho real conhecido apenas para aumentar o manifesto.

Não use emuladores, Wine, CrossOver, execução somente via Rosetta ou VM. Gameplay, regras, transferência do engine Windows, otimização geral, multiplayer, novos gráficos e reparo do menu nativo ficam fora deste goal.

## Ponto de partida verificado em 4 October 2026

- Branch `main`, HEAD e uma consulta nova a `origin/main`: `7535805ed17f2440f9c1fc30158a0dc2e8fa6546`, entrega do núcleo RNG. O commit anterior `daba5f30b6895cc5ecba5d03c40cb337868f5cda` entregou Date. Reconfira branch, remoto, diff, instruções e estado local antes de trabalhar. Não havia `AGENTS.md` aplicável nas pastas ancestrais e no projeto inspecionados.
- O manifesto privado `.local/recompiled/compilation.json` contém 37 semantic replacements: 34 Date e três RNG. As quatro entradas deste goal continuam geradas. O objetivo final é 41, sem retirar as 37 existentes.
- O executável da fonte `.local/aot-build/CM 01-02 Native Prototype.app` e o instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` foram novamente conferidos ARM64 e têm SHA-256 `1b84152cc5ddad11d43fa2b3fc7c0293b20d2dcf0a8008ee0750843867a0d9b6`. O executável interno se chama `CM 01-02 Native Prototype`.
- `docs/native-rng-core-reconstruction.md` registra 2.022 comparações diretas, 40/40 CTest, descoberta Python com 37 aprovados e quatro grupos opcionais pulados, os quatro opcionais aprovados explicitamente e UBSAN gerado/focado. Registra também dois replays day por modo e duas sequências reais Save/exit/reopen/Load do candidato final. São resultados da entrega anterior; esta preparação não executou a suíte nem validou a nova seleção.
- O replay day anterior cobriu draw/seed, mas teve zero chamadas do initializer. A cobertura das quatro novas entradas no day e na UI ainda não foi medida. A existência de `native/background_checks.cpp` é uma base de teste, não evidência de execução semântica nova.
- O usuário dispensou expressamente os testes prolongados restantes de Date e pediu avançar com evidência prática proporcional, encontrando eventuais bugs durante o uso. Preserve a limitação semanal documentada; não reabra essa campanha como gate deste bloco, não invente PASS e não trate a dispensa como aprovação de divergências futuras.
- O contrato de diferença de datas permanece assimétrico: January 1, 2000 menos January 1, 2001 retorna -365; a inversão retorna +366. Corrigir isso continua fora do escopo.
- Havia oito handoffs untracked, sem diff de arquivos versionados: `native-date-{difference,month,day-of-month,weekday,weekday-adjustment,construction-and-adjustment,subsystem}-reconstruction-prompt.md` e `native-rng-core-reconstruction-prompt.md`, todos em `docs/features`. Preserve esses oito e este novo handoff; exclua-os do commit de produto.

Esta preparação foi somente inspeção e criação do prompt. Não abriu o jogo, implementou as quatro entradas, executou nova aceitação, instalou uma versão ou criou outra sessão.

## Saves, limpeza recente e preservação

O usuário autorizou a remoção de saves duplicados. Em 4 October 2026 foram removidas 705 cópias byte a byte idênticas das pastas privadas de testes, somando 15.232.817.366 bytes. Restaram 68 arquivos: os 67 conteúdos distintos e um backup separado do save de referência. O `Reading.sav` pessoal e todos os conteúdos únicos foram preservados e verificados. A limpeza não alterou arquivos versionados.

**Não trate os inventários históricos de 773 saves ou 1.491 artefatos protegidos como o estado atual.** Alguns caminhos de saves mencionados em relatórios antigos foram intencionalmente removidos. Não classifique essa ausência como regressão do produto e não recrie as centenas de cópias. O registro local `.local/save-deduplication-20261004-215858/removed.jsonl` mapeia cada cópia removida para uma preservada; `plan.json` e `receipt.json` registram a operação. Esses recibos são auxiliares: os caminhos e hashes necessários para iniciar este trabalho estão explicitados abaixo.

Saves novamente conferidos nesta preparação:

| Uso | Caminho relativo ao projeto | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Preferido, último ciclo final RNG | `.local/performance/rng-core-final-01/Runtime/Files/games/RNGFinal-04SepResults.sav` | 34.872.328 | `44dd1f372cf9f568f63b1f2e5942c5a68f357cb816b1b2c9b58941c32580e4a3` |
| Alternativa do mesmo candidato | `.local/performance/rng-core-final-01/Runtime/Files/games/RNGFinal-04Sep2002.sav` | 34.543.289 | `18cca88066fc9e1e3655e80c0e9bf6171eb711600816eac94d5328503f931c4a` |
| Referência Date preservada | `.local/performance/date-subsystem-live-01/Runtime/Files/games/DateModule-04Sep2002.sav` | 34.557.282 | `0d0ca840ef270751a2c8537191aacff301c52fb653f34c9bb5171d172fdcf86a` |
| Alternativa Date preservada | `.local/performance/date-subsystem-live-01/Runtime/Files/games/DateModule-03Sep2002.sav` | 34.464.680 | `ce1b006a19508ff43c2507e7f7855a0dac9e9b8f1887c53ea8931a9a3138c446` |
| Referência LFS imutável | `saves/Mac-Teste-Validado-Pos-Jogo.sav` | 17.917.514 | `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb` |

O checkpoint preferido foi observado em September 4, 2002 NTE, Victor Moraes/Cruzeiro, e preservou o resultado Brazil 5:1 Uruguay no ciclo final RNG. Reconfirme data/fase, manager/club, fixtures/results, perfil e contrato de Luisao no novo candidato, usando o relatório RNG para os demais campos observados. Isso não exige reproduzir a mesma trajetória CRT entre inicializações: o Load original não restaura toda a trajetória RNG de startup como um snapshot de runtime.

Inventarie de novo os saves e artefatos existentes, o instalado e `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`. Preserve conteúdo, forks, FinderInfo, xattrs, modos, mtime e flags. Crie uma nova referência privada do **instalado atual** antes de substituir builds. `.local/rng-core-reconstruction/reference/installed.app` é a referência anterior ao RNG, com executável `a35a303f...`, e não deve ser confundida com a produção atual nem sobrescrita.

Havia aproximadamente 41,7 GB livres nesta preparação. Reavalie antes de capturar. Algumas ferramentas de teste da entrega anterior foram removidas por serem reproduzíveis; sua ausência exige reconstrução quando necessária, não restauração de payloads históricos indiscriminadamente.

## Leia e inspecione primeiro

Leia `README.md`, `docs/native-port-status.md`, `docs/native-rng-core-reconstruction.md`, `docs/native-season-reference.md`, `docs/native-date-subsystem-reconstruction.md` e a melhoria bissexta adiada em `docs/native-date-difference-reconstruction.md`.

Inspecione:

- `native/background_checks.cpp`, `native/rng_core.hpp`, `native/rng_core_adapter.*`, `native/rng_core_checks.cpp` e `native/CMakeLists.txt`.
- `native/aot_runtime.*`, `native/aot_probe.cpp`, `native/callbacks.*`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp` e os adapters/audits Date como padrões de integração e prova.
- `tools/compile_recomp.py`, `tools/rng_core_audit.py`, `tools/test_rng_core_audit.py`, `tools/calendar_audit.py`, `tools/check_rng_core_season.py`, `tools/check_season_reference.py`, `tools/test_generated_temporaries.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`.
- `.local/recomp-input/instructions.sqlite` em modo read-only e as páginas geradas privadas `0x10712`, `0x10713`, `0x1074d`, `0x1074e` e `0x1077b`, conforme as dependências realmente exercitadas. Não publique listagens, corpos, tabelas, snapshots ou dados proprietários.

Não é necessário portar ou editar o projeto Windows ReCM. O contrato é o Mac observado, incluindo suas estruturas big-endian de 32 bits, ABI e correções de compatibilidade já instaladas.

## Escopo fechado: quatro entradas, 317 instruções indexadas

Os limites abaixo foram conferidos na base local; cada intervalo está completo e termina no retorno observado. A consulta a `flows` não encontrou entradas externas indexadas no interior desses quatro corpos. Isso não prova ausência de entradas indiretas: reconfira referências, transições e callbacks antes de implementar. Calls estáticos não são runtime coverage.

| Entrada | Fim exclusivo | Instruções | Calls diretos indexados | Responsabilidade observada |
| --- | --- | ---: | ---: | --- |
| `0x107122e0` | `0x107124e0` | 128 | 3 | Escolher um item do catálogo/árvore e então uma imagem da lista principal ou alternativa |
| `0x10712a60` | `0x10712c4c` | 123 | 4 | Escolher e retornar o payload de um nó da lista |
| `0x10713960` | `0x107139b4` | 21 | 1 | Selecionar background padrão por categoria |
| `0x107139c0` | `0x10713a74` | 45 | 6 | Selecionar background por categoria e objeto/descritor, ou pelo caminho padrão |

Fingerprints SHA-256, com a normalização `address/assembly/ops/flows/fallthrough` de `tools/calendar_audit.py`:

- `0x107122e0`: `1a92a426b5789bed300658ff087bfcebd05ffd229e192c3c9b97a84da5b0578b`.
- `0x10712a60`: `074912d3cb6b49880517225d7dd83dfd5c0d8a22250def5792f745fb8cf368b8`.
- `0x10713960`: `1efc413b607f6cc6c32cd45329b54d34b846e968d5f495aeca2b8e6f42bd85a7`.
- `0x107139c0`: `c206b64c4f3b65e30292264dd4a3ea84ad30b8a794315e2e659ed496a2ed7435`.

### Escolha em coleções

`0x107122e0` recebe a coleção em r3 e o seletor principal/alternativo em r4. Nulo ou contagem efetiva zero retorna zero. A contagem é obtida de uma leitura de 32 bits em `+12`, seguida de redução signed16. A seleção acessa a árvore por `0x1074df60`, lê o payload do nó em `+4` e escolhe a lista original ou a situada em `+12`, conforme o byte efetivo do seletor. Delegue essa segunda etapa à nova `0x10712a60`, preservando efeitos e ordem.

`0x10712a60` recebe a lista em r3; sua contagem vem de uma leitura de 32 bits em `+8`, também reduzida a signed16. Após a escolha, usa `0x1074dd40` para localizar o nó e retorna seu payload em `+0`, ou zero se o resultado da busca for nulo. Não substitua os accessors originais de árvore/lista por outra estrutura neste goal.

Ambas consomem o pool unsigned16 big-endian com avanço do cursor **antes** da amostra. O limite é inclusivo; ultrapassá-lo reinicia na base e renova salt através do draw RNG já semântico. Preserve quando há zero, um ou múltiplos consumos, inclusive quando uma busca posterior retorna nulo. Cursor, salt e CRT são observáveis mesmo quando o resultado visual não muda. Não substitua a seleção por `std::rand`, `uniform_int_distribution`, um gerador novo ou estado C++ paralelo.

Os slots relativos ao TOC permanecem `r2-0x7b90` (ponteiro para salt), `r2-0x7b8c` (base do pool), `r2-0x7b88` (ponteiro para limite) e `r2-0x7b84` (ponteiro para cursor). Diferencie slots, ponteiros e valores. Preserve a ordem de loads/stores, aritmética signed/unsigned, carry, resíduos e aliases de stack/coleção/pool/slots. A descrição acima orienta a caracterização; não é autorização para ignorar efeitos da ABI.

Caracterize também os ramos compilados de limites grandes e a chamada ao sampler `0x107124e0`. Não finja alcançabilidade normal de um ramo incompatível com a redução signed16. Qualquer simplificação exige demonstrar as pré-condições no contrato completo, inclusive os clobbers dos filhos. O sampler permanece uma dependência gerada, não uma quinta substituição.

### Seleção por categoria e objeto

`0x10713960` consulta o flag de backgrounds em `r2-0x35d4`, interpreta seu byte com a semântica original e consulta a tabela de categorias em `r2-0x5c3c`. Seu cálculo de índice usa a máscara de bits original, não uma validação nova de faixa. Categoria ausente retorna zero; categoria presente segue para `0x107122e0` com seleção principal. Seu call direto está em `0x107138f0`.

`0x107139c0` recebe categoria em r3, objeto em r4, descritor em r5 e seleção principal/alternativa em r6. Com backgrounds desabilitados ou categoria ausente, retorna zero. Objeto nulo ignora o descritor e usa `0x107122e0`; objeto presente busca por descritor via `0x1074df90` e depois escolhe na lista via `0x10712a60`. Preserve a extensão signed16 própria do índice desta entrada: não a unifique cegamente com a máscara de `0x10713960`.

Os seis wrappers em `0x10713a80`, `0x10713ad0`, `0x10713b20`, `0x10713b70`, `0x10713bc0` e `0x10713c10` permanecem gerados. Também ficam originais carregamento/liberação de catálogos, aplicação do nome da imagem, getters, comparação/busca de árvore, desenho e todos os consumidores RNG embutidos fora das quatro entradas.

Preserve exatamente as correções existentes de objeto nulo nos call sites `0x10713aec` e `0x10713b3c`: clube/competição podem pedir fallback sem chamar getters inválidos. Não remova essas correções, não generalize supressão de faults e não transforme outros acessos nulos em sucesso. Os testes existentes em `native/background_checks.cpp` devem continuar distinguindo os casos válidos dos faults independentes.

## Implementação e caracterização

Produza lógica semântica legível com nomes e responsabilidades explícitas, adapters mínimos e contratos fixados por fingerprint. Não entregue apenas uma reprodução mecânica das instruções, wrappers que continuam executando os quatro corpos originais ou um módulo que o app nunca chama.

Instale hooks nos **destinos** gerados das páginas `0x10712` e `0x10713`, cobrindo gotos na mesma página, dispatch entre páginas, transições indiretas e callbacks. Produção normal deve usar as quatro entradas semânticas sem fallback aos seus corpos originais. Seleção original fica privada, explícita e independente de Date/RNG. Dependências fora do bloco continuam pelo caminho de ABI existente.

Compare original versus semântico por entrada, com todos os registradores e temporários, CR/XER/FPSCR quando afetados, LR/CTR/context TOC/PC, frame/backchain/spills/padding, toda a memória mapeada, alocador, imports/callbacks e ordem de leituras/escritas/map/clear. Chamar filhos por um wrapper que restaura clobbers não substitui equivalência da ABI direta. Compare também pais originais/filhos novos, pais novos/filhos originais e o bloco inteiro semântico.

Casos mínimos relevantes:

- Coleções nulas/vazias, um item, vários itens com payloads distintos, nó ou payload nulo, índices efetivos positivos/negativos e contagens com bits altos diferentes mas mesmo signed16. Cubra 0, 1, 2, `0x7fff`, `0x8000`, `0xffff` e valores de 32 bits que reduzam a esses casos, com mappings controlados.
- Seleção principal/alternativa com bytes efetivos 0, 1, `0x80` e `0xff`, incluindo bits altos não participantes; enable flag zero e não zero; diferenças entre a máscara de categoria e a extensão signed16 das duas entradas.
- Objeto nulo com descritor propositalmente não mapeado, demonstrando que esse caminho não o dereferencia; objeto presente com match/miss real de descritor e preservação das comparações/callbacks originais.
- Cursor no início, penúltima amostra, última amostra inclusiva e wrap; salt/sample extremos; sequências de escolhas; ausência de consumo nos retornos que precedem o draw; consumo preservado quando a seleção falha após o draw.
- TOC relocável, aliases entre slots/coleções/pool/stack, mappings mínimos e faults observáveis. Não acrescente bounds checking ou saneamento que altere o resultado original.
- Composição categoria → árvore → lista → RNG, wrappers atuais de clube/competição, transições indiretas e callbacks. Registre separadamente quais dependências permanecem geradas.

Inclua um negativo próprio para cada uma das quatro entradas: perturbe seu primeiro retorno/efeito observável e exija rejeição na primeira fronteira própria. Distinga a rejeição do filho da do pai. Teste reset de diagnósticos, seleção original, proibição de gravar semântico como oracle e truncation/checksum/EOF se introduzir um stream novo. Uma igualdade de imagem sem igualdade de consumo RNG não passa.

Estabilize juntos os campos de State. Counters, flags e observadores não entram em saves/snapshots. Use comparação completa nos harnesses e evidência runtime compacta, limitada por entrada. Não grave snapshots inteiros por escolha nem repita os arquivos gigantes Date. Leituras feitas só para diagnóstico não podem criar faults ou efeitos novos no caminho normal.

## Aceitação integrada e aplicativo real

1. **Resolva a viabilidade de cobertura cedo.** Observe os caminhos originais das quatro entradas em um candidato isolado, com contadores próprios e poucas amostras/fixtures. Explore abertura, navegação normal e seleção de categoria/clube/competição, incluindo o caller `0x107138f0`; não presuma que abrir uma tela cobre todas. Capture a entrada original com estado suficiente para reproduzir a seleção e os efeitos. Se uma entrada ainda tiver zero chamadas reais, registre a lacuna e encontre o caminho antes de declarar o bloco entregue. Injeção sintética não é gameplay.
2. **Compare o fluxo que realmente cobre o bloco.** Use as capturas originais compactas e imutáveis da UI para comparar baseline com Date/RNG semânticos + seleção original contra candidato com as 41 entradas semânticas. Exija duas execuções equivalentes por modo dos fixtures pertinentes, calls próprios correspondentes, consumo RNG/ordem/ABI/memória exatos e negativos próprios. Nunca gere o oracle a partir da implementação nova. Preserve formatos e modos históricos; uma seleção original do bloco novo não autoriza gravação de oracle Date/RNG sem satisfazer suas proteções existentes.
3. O fixture `.local/performance/season-day-01/SeasonFixture` continua disponível, com `capture.txt`, `initial.snapshot`, `expected.snapshot` e `imports.gz`. Pode ser usado como regressão integrada se houver motivo concreto, mas sua cobertura destas quatro entradas é desconhecida. Zero calls não vira PASS de seleção. Não imponha uma campanha day/week de vários replays que não execute o bloco. Os atuais `--rng-baseline`/`--rng-modern` comparam o núcleo RNG, e `--modern` sozinho não seleciona o baseline de 37 entradas desejado: inspecione os modos antes de reutilizá-los. Não relaxe as comparações de PC, registradores, memória, alocador, imports/callbacks e consumo RNG para acomodar divergências.
4. **Valide o candidato final real.** Prepare uma app ARM64 nova com Runtime/Runs privados, uma instância por vez e cópia somente do save necessário. `tools/prepare_validation.py` copia todo o Runtime recebido e adiciona `Perf-Reference.sav`; prepare uma origem mínima, com as preferências necessárias, para evitar multiplicar o histórico de saves novamente. Não use hard links para saves graváveis. Reconfira hashes antes/depois das cópias.
5. Faça Original Load do save preferido, abra as telas relevantes e demonstre chamadas próprias não zero das quatro entradas semânticas, zero chamadas próprias originais no modo normal e amostras verificadas contra a referência. Pelo menos os caminhos não vazios de árvore/lista devem produzir seleções reais; só early returns não bastam. Verifique fundos válidos, caminhos padrão/objeto e navegação sem fault. A trajetória RNG de startups distintos pode variar: equivalência exata da escolha exige o mesmo estado inicial e fica nos fixtures; não force artificialmente seeds para a UI aparentar igualdade.
6. Faça uma sequência completa Original Game Options → Save Game, saída normal, processo ausente, reabertura do mesmo candidato final e Original Load do save recém-criado. Confira data/fase, manager/club, resultados/fixtures e perfil/contrato observados; repita as telas relevantes depois do Load. Uma progressão curta só é necessária se resolver uma lacuna concreta de fluxo. Este bloco não exige partida completa, virada mensal/anual, segunda campanha de persistência ou temporada inteira sem um motivo observado.

Use Game Options → Save Game: o menu macOS File → Save ainda exige `MenuSelect` original e seu reparo está fora do escopo. Recrie bindings após reabrir. Evite consultar a aplicação por AX depois de sair, pois a API pode relançá-la. Não mude datas, resultados, coleções ou saves na UI para simular coverage.

## Validação compartilhada e comandos

Durante o desenvolvimento rode checks focados; depois de estabilizar o bloco, faça um rebuild consistente, uma suíte final e UBSAN focado nas unidades/páginas efetivamente alteradas/exercitadas. Rebuild completo é necessário se a mudança de State invalidar objetos; `clean-first` não deve ser repetido por rotina. Falhas reais exigem correção e revalidação afetada; sucesso não exige repetir campanhas caras sem nova razão.

Comandos-base, a adaptar apenas onde o novo harness exigir e depois de reconferir as opções:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input" \
  -DCM_DATE_SNAPSHOT="$PWD/.local/performance/season-day-01/SeasonFixture/initial.snapshot"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

Inclua os checks novos no CMake/Python e registre seus comandos repetíveis no relatório. Reexecute `original_background_selection` conforme o nome real encontrado no CMake e as regressões RNG afetadas. Não invente o nome de um target existente nem trate testes pulados como aprovados. UBSAN deve cobrir runtime, adapter/harness novo e páginas pertinentes; informe a extensão exata, sem alegar whole-program, bridges ou ASan sem execução correspondente.

## Espaço, instalação e entrega

Use artefatos pequenos e tarefas grandes sequenciais. Planeje crescimento e preserve uma reserva de disco. Guarde uma cópia canônica de cada novo conteúdo de save e os arquivos necessários à aceitação; não reintroduza o acúmulo de centenas de duplicatas. A limpeza de temporários reproduzíveis criados por este goal está autorizada depois de registrar resultados e identidades. Não apague saves únicos, backup de referência, fixtures imutáveis, original inputs, referências ou candidatos históricos, nem a árvore `.local`. Não amplie a limpeza anterior para novos conteúdos únicos sem autorização.

Escreva `docs/native-background-selection-reconstruction.md` em inglês e atualize `docs/native-port-status.md`. Documente o manifesto de quatro entradas/41 substituições, contratos, fingerprints, dependências geradas, compatibilidade dos null guards, casos/negativos, contadores/amostras próprios, comparação integrada e limites, UI/Save/Load, hashes/inventários, espaço e limpeza. Torne o relatório suficiente para continuidade sem depender deste chat ou de logs temporários.

Instale o candidato aprovado uma vez ao final com `tools/install_native.py`, dentro do fluxo de entrega já autorizado do projeto. Reconfira antes/depois os inventários pessoais e os artefatos preservados que **ainda existem após a deduplicação**. Separe hashes da fonte, candidato assinado, instalado e nova referência anterior à mudança. Confira ARM64, bundled originals, plist sem diagnósticos e horário efetivo da instalação; o mtime copiado da pasta `.app` não prova a versão instalada.

Revise o diff exato e preserve trabalho alheio. Sob a autorização permanente de entrega concluída e validada, faça commit/push normal, sem force, e confirme o SHA remoto por consulta nova. Exclua os nove handoffs pendentes, saves novos, payloads proprietários, snapshots, logs e artefatos privados; não altere ignore/LFS. A preparação deste prompt não autoriza implementar, instalar ou publicar durante a sessão de planejamento; essas ações pertencem à execução deste pedido de implementação.

Conclua somente quando as quatro entradas executarem C++ semântico na produção, os contratos e o consumo RNG forem equivalentes, a comparação integrada e a UI cobrirem cada entrada, Save/exit/reopen/Load estiver verificado e instalação/preservação/documentação/limpeza/entrega estiverem conferidas. Relate os limites do núcleo selecionado e das dependências ainda geradas. Não apresente um handoff, contagem de entradas ou execução parcial como reconstrução completa do engine.
