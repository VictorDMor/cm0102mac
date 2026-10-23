/goal Reconstrua e entregue o bloco Mac de arenas, pools de pequenos objetos e fachadas globais de alocação/liberação/realocação em C++23 semântico na produção ARM64 do CM 01/02. Implemente conjuntamente as vinte e sete entradas abaixo, preserve o layout e os efeitos guest e a correção de progresso já produtiva, valide com checks focados, faça uma instalação protegida e um smoke curto do aplicativo instalado, documente e entregue com commit/push normal. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto e objetivo

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon, reconstruído progressivamente em C++ compreensível com comportamento original Mac preservado. O engine já funciona por recompilação estática e bridges nativos. Date, RNG core, backgrounds, consultas de coleções, strings/descritores, primitivas de memória, parsing inteiro, entrada/saída formatada, conversão de caracteres e parsing floating já têm blocos semânticos. Apresentação Retina e comandos nomeados também estão entregues.

O próximo incremento é a cadeia de armazenamento guest usada por strings e outros objetos do engine: arenas, boundary tags, freelists, pools pequenos, contextos, allocate/free/reallocate e allocate-and-clear, incluindo suas quatro fachadas globais. A duplicação de string já semântica chama `0x10775130`; a reconstrução deve tornar essa alocação semântica também. Uma sequência alocar → preencher → realocar → liberar deve preservar endereço quando o original permite, conteúdo sobrevivente, vizinhos vivos e metadados, sem trocar o modelo de memória por alocação host. A fachada global de alocação tem 1.288 calls diretos no índice consultado; isso indica relevância estática, não cobertura runtime.

Entregue o bloco funcional completo em um único goal, com caracterização e provas por entrada e uma aceitação final compartilhada. O caminho produtivo default deve executar os corpos novos. Use operações semânticas nomeadas e ABI explícito; não entregue somente investigação, wrappers executando seus próprios corpos substituídos, replay P-code ou tradução opcode por opcode. Não prometa ganho visual ou de desempenho.

## Decisões e limites

- Validação proporcional: checks do código alterado, negativos discriminantes, regressões diretamente relacionadas, uma instalação final protegida e um smoke curto. Não repita UI por entrada.
- Save/Load, save/exit/reopen/load, carreiras, dias/semanas/temporadas, partidas, auditoria de serialização, benchmarks, heap histórico capturado e suíte inteira estão fora do escopo. Persistência permanece **not retested**, sem PASS e sem gate futuro inventado.
- Preserve a assimetria ascending-year de leap da Date. Não altere gameplay, RNG, arquivos/streams, strings já entregues, locale, bridges decimais ou callbacks não pertencentes ao bloco.
- Preserve exatamente a correção de progresso de reposição de pools em `native/allocator_bridge.hpp` e no gerador. O oracle é a tradução produtiva atual com esse guard, não o loop Mac bruto que pode repetir indefinidamente. Reconstruir não autoriza uma nova política de heap ou consertos adicionais.
- Não use `malloc/free/realloc/calloc` host, containers host ou ponteiros host para substituir arenas/pools guest. Os bridges existentes podem continuar usando seu armazenamento nativo; os metadados e endereços observáveis do allocator Mac permanecem guest, big-endian e word32.
- Faça implementação, documentação, instalação protegida e commit/push normal do produto validado. Preserve alterações alheias e artefatos privados; não force push, não crie nova sessão e não implemente subsistemas adjacentes.
- Este arquivo é um handoff privado. Sua criação e cópia comprovam planejamento; não são implementação nem autorização para efeitos adicionais fora dos limites acima.

## Ponto inicial verificado e inspeção

Estado observado em 9 de outubro de 2026: branch `main`, HEAD e remoto `origin/main` em `e9577b4cc4e92630d6918d6ca3e7a2c5611dd116`, remoto `git@github.com:VictorDMor/cm0102mac.git`, tracked/staged limpos. Antes deste handoff havia 22 arquivos de prompt em `docs/features`, sendo 17 privados/untracked; este acrescenta um. Reenumere e confira hashes, em vez de presumir contagem fixa. Não versione handoffs privados por acidente.

`.local/recompiled/compilation.json` tem 1.953.157 instruções, 1.922 páginas, zero missing/pending targets e 100 `semanticReplacements` ordenados. SHA-256 do array serializado com chaves ordenadas e separadores compactos: `dee0bb555af0657479f3defa0e8a2b763aabd5717e716a213d9c25d8c75004b3`. As 78 entradas anteriores e as 22 de I/O entregues são preservadas exatamente. Append das 27 abaixo, na ordem crescente da tabela, resulta em **127**; não reordene a baseline nem conte dependências externas como entradas reconstruídas.

A entrega anterior registrou sete CTests focados, nove audits Python e UBSAN dos cinco harnesses afetados, com Release ARM64 consistente, instalação protegida e smoke curto. Esses resultados são da entrega anterior, não equivalência deste allocator. Durante a preparação deste prompt foi executado somente o check relevante de reposição: `ctest --test-dir .local/aot-build --output-on-failure -R '^original_allocator_replenishment$'`, **1/1 PASS, 2,51 s**, com os 16 casos sintéticos existentes. Isso comprova a fixture atual e o comportamento guardado existente, não os futuros corpos semânticos.

O executável interno instalado em `/Users/victordmor/Applications/CM 01-02 Native Prototype.app/Contents/MacOS/CM 01-02 Native Prototype` foi conferido como ARM64, SHA-256 `0725840a0907bc6bd866d47632fbe75606defaa98d320ecb53b50c7348fed749`. Reconfirme caminhos, branch/remoto/diff, manifesto, fonte privada e identidade instalada antes de editar; estes dados podem mudar. Nenhuma rotina deste bloco foi implementada ao preparar o prompt.

Leia instruções aplicáveis e:

- `README.md`, `docs/native-port-status.md`, `docs/native-formatted-output-reconstruction.md`, `docs/native-string-core-reconstruction.md` e `docs/native-memory-primitives-reconstruction.md`.
- `docs/performance-validation.md`, especialmente **Allocator non-progress diagnosis**: contexto histórico da correção, sem reabrir as campanhas seguintes.
- `native/allocator_bridge.hpp`, `native/allocator_checks.cpp`, `native/bridge_checks.cpp`, `native/carbon_bridge.cpp`, `native/aot_runtime.hpp`, `native/aot_runtime.cpp`, `native/CMakeLists.txt`.
- `native/string_core_adapter.cpp`, `native/memory_primitives_adapter.cpp` e seus harnesses; padrões de seleção original por entrada, counters, memória/import observers e comparação arquitetural nos adapters/harnesses entregues.
- `tools/compile_recomp.py`, `tools/formatted_output_audit.py`, os audits dos módulos entregues, `tools/test_*_audit.py`, `tools/install_native.py`.
- O índice privado `.local/recomp-input/instructions.sqlite` e `program.json`; não publique listings, bytes originais, payloads, snapshots ou fontes geradas. Original executable SHA-256: `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.

## Entradas e pins do bloco

Os nomes são interpretações funcionais orientadoras; caracterize o contrato original antes de fixar nomes, parâmetros e invariantes. Intervalos são end-exclusive. Counts são instruções realmente indexadas, não span/4; gaps/padding não são instruções inventadas. Cada fingerprint SHA-256 serializa, por endereço crescente, apenas `address`, `assembly`, `ops`, `flows`, `fallthrough`, via JSON com chaves ordenadas e separadores `(',', ':')`, UTF-8, como os audits atuais.

| Entry | End exclusive | Instructions | Functional role | SHA-256 |
|---|---|---:|---|---|
| `0x107740f0` | `0x10774184` | 37 | Arena initialization | `899991252ae8d0a90551f0d0e27aff502b549b8bc4734cf233ca038dd0418874` |
| `0x10774190` | `0x10774268` | 53 | Arena block allocation | `e91c37ea32029bb0a5850ad914d8f1b92978176ffd0354232ee6ab753b74bb94` |
| `0x10774270` | `0x10774354` | 57 | Arena block release | `a9badebe7c44a6c29d520e42ed9646e8853cefea5f1c061abefc797c46349278` |
| `0x10774360` | `0x107743d4` | 29 | Free-block unlink / allocation marking | `46db492d7b8bf0117155650d71021ceaa443cc7a0d7bc047a4b254c062969afa` |
| `0x107743e0` | `0x10774430` | 20 | Block header / boundary-tag initialization | `defee65552080892727defd8cffa736f0841303341b081347028c8ca5c09315c` |
| `0x10774430` | `0x107744d0` | 40 | Block splitting | `1b3759e1aad753dacd69eb8885ee1fe5e54b128f41865de89a2bdd167090aa7a` |
| `0x107744d0` | `0x10774568` | 38 | Previous-block coalescing | `002e8f3b47aced8869ba50259b0213cafc05c9a9a29d9c046a6f6ab10e09f933` |
| `0x10774570` | `0x1077462c` | 47 | Next-block coalescing | `346e27dbb2ba8cc8f11e08e50205fcec3ad3f26c0f9b7821ab440509d618cec5` |
| `0x10774630` | `0x10774674` | 17 | Arena-list insertion | `ad89c9bf7c181551a63b09da1151ed8545f3ba7be7172c500149ca8438cd0995` |
| `0x10774680` | `0x107746cc` | 19 | Arena-list removal | `064156d3d9f352309ea1335ba3b8a7083de97ea14cc9a21317d85609f101b39b` |
| `0x107746d0` | `0x10774754` | 33 | Arena acquisition / growth | `5a4266e828c346e4cf92a6f4a48982cc12eae20181fee480e1b0d35391ca526f` |
| `0x10774760` | `0x1077483c` | 55 | Allocation with arena growth | `f888e846ac6fded2020fef449ad6121319c015714e9ea42b384b4d5b86608fd2` |
| `0x10774840` | `0x1077491c` | 55 | Existing-arena allocation / usable-size hint | `233fa935bc02c70159aab33616bcb29fb98bc4c8e2234abaf7a331c72c5bed23` |
| `0x10774920` | `0x107749ac` | 35 | Ordinary-object release / empty-arena disposal | `651472f54721c0bfe36e403a82b80c3f5db907e2d0db84e90ae039385ae5a87c` |
| `0x107749b0` | `0x10774ad4` | 73 | Small-object pool initialization | `860f062b04be068f9350cc9a7a6360d5d98ece352b00594f7f9c897ee1655fd2` |
| `0x10774ae0` | `0x10774b08` | 10 | Allocator-context clearing | `6f079c08c2fa85903a15178d319cc28b97303b6cd66cce7b213720fe871eb8cd` |
| `0x10774b10` | `0x10774b58` | 18 | Lazy global-context access | `19336e3f73c506ecd4a97f7814da2c88c081d95e671f38d7d2294b6d949fde69` |
| `0x10774b60` | `0x10774d20` | 111 | Small-object allocation / pool replenishment | `179cd6ecfc7129e22dfb7642b997dcee4c75e71cba1de3640ca676dad800fca0` |
| `0x10774d20` | `0x10774e74` | 85 | Small-object release / pool disposal | `daf82fb8c1795572a2102f5374813720c4b6171917059acebcec855369fec39a` |
| `0x10774e80` | `0x10774ed4` | 21 | Context allocation dispatcher | `abeb332fa94f94fd97cda4eac9f5fc82fd5764ff1611d06cfb7dd76904c34a5b` |
| `0x10774ee0` | `0x10774f38` | 22 | Context release dispatcher | `224588c051e02fca7eabd023d99ef087e9d3cd6fe85bb9d767dd40176ab260db` |
| `0x10774f40` | `0x107750e0` | 104 | Context reallocation | `de1e23660c8ab1f486b7b861ad2631aa25b9341609065cce13fab2c20bdd3ce7` |
| `0x107750e0` | `0x1077512c` | 19 | Context allocation and clearing | `95446dda867a8c76c3dd7075c60f4bc164662749615e3e9d0a2e0993e8393e2b` |
| `0x10775130` | `0x10775164` | 13 | Global allocation facade | `8c93c7ba18df1a72fbb8a46d81d3e5373180e9f65b65f34ab4777a08f298263f` |
| `0x10775170` | `0x107751a4` | 13 | Global release facade | `56a985f44c579aafb7ee77d6a06cd133a3770dfdd43a1c6edbd4e5c4ec60e71e` |
| `0x107751b0` | `0x107751f4` | 17 | Global reallocation facade | `fd300973f33c95758ade4fc7b0d9da5b0b8836635c55a7a3fb6987486d206be9` |
| `0x10775200` | `0x10775240` | 16 | Global count-times-size clearing facade | `d1840afa4b2dc77cb1c531b1f4a85af844c37bd7f07c66d65f032dd856455056` |

Total: **27 entradas / 1.057 instruções**, páginas `0x10774` e `0x10775`. Somente `0x10774f40` cruza página; fixe o corpo completo e estes fragmentos:

| Entry | Page | Instructions | SHA-256 |
|---|---|---:|---|
| `0x10774f40` | `0x10774` | 48 | `4c2c60a384d74408d7a49906fb09f30964ec7191ab11857a4c60eabff422bb7e` |
| `0x10774f40` | `0x10775` | 56 | `20e909d26bf62b6c1bec4877f0024a4e49b6b6756c4b5800c576bd49c0172e54` |

A consulta completa de flows/fallthrough do índice encontrou **zero arestas externas para interior** destas 27 entradas: target dentro do corpo, diferente de entry, source fora desse corpo. Reconfirme com audit e negativos; não suponha que só callers `CALL` importam. Hooks devem estar nas labels de destino antes da primeira operação original, cobrindo goto local, dispatch cross-page e indireto. Corpos/entradas/fragmentos completamente ausentes não podem produzir manifesto completo. Fixe também o branch da correção de progresso, independentemente do pin bruto das instruções.

## Contratos a reconstruir

### Arenas, blocos e listas

Reconstrua inicialização de arena, busca circular de bloco livre, unlink, split, coalescing com anterior/próximo e inserção/remoção de arena como operações nomeadas. Preserve layout word32, owner/tag bits, tamanho/alinhamento, boundary tags, sentinel, links circulares, free-list head e largest-free-block hint. Não introduza validações, limpeza de payload ou política de fit diferentes do que o original executa.

Observações para caracterização: tamanho de bloco usa `& ~15`; flags incluem in-use `2` e previous-busy `4`; owner de bloco comum inclui low-bit `1`. Ponteiro de usuário comum é raw block + 8. `0x10774190` percorre a freelist e só split quando o restante supera 95 bytes. Com free-list head nulo, pode retornar zero sem corrigir o largest-block hint. Preserve esse comportamento; a correção de progresso fica no caller específico. Caracterize a atualização do hint após busca malsucedida e os efeitos completos de coalescing, inclusive listas com um único nó e early returns condicionais.

`0x107746d0` calcula o pedido de arena por `(needed + 47) & ~15`, com mínimo de 65.536 bytes, chama a aquisição existente e inicializa/liga a arena. `0x10774760` e `0x10774840` arredondam o pedido comum por `(size + 23) & ~15`, com mínimo 96; o primeiro permite crescimento, o segundo somente tenta arenas existentes e escreve o maior usable-size hint observado no out-param. Preserve aritmética word32, comparações unsigned/signed efetivas, overflow, ordem de leitura/escrita, retorno e todos os efeitos dos helpers/bridges.

`0x10774920` libera o ponteiro comum via owner e pode remover/devolver a arena quando ela fica inteiramente livre. Preserve ordem free/coalesce/unlink/dispose e os efeitos de `NewPtr`/`DisposePtr` através dos wrappers existentes `0x10778d50`/`0x10778d80`; esses wrappers permanecem gerados, fora das 27 substituições. Não substitua uma falha original por sucesso fictício, nem unmap uma região que o bridge atual mantém mapeada após liberação.

### Contexto e pools de pequenos objetos

`0x10774ae0` limpa 52 bytes pelo helper de fill já semântico. `0x10774b10` usa contexto global via TOC `r2-0x3b74` e flag byte `r2-0x3510`, inicializando uma vez conforme original. Preserve flag, ordem e releituras; não crie singleton host. A baseline aponta o contexto para `0x10bf7644`. Use TOC relocada e conteúdo mutável em fixtures.

A tabela de classes é lida pelo slot `r2-0x3b70`; baseline aponta para `0x108a0da0`, com seis tamanhos **4/12/28/44/60/76**. Preserve leituras live, escolha de classe e rows de links do contexto; não congele esses valores em constants host. O pool usa header de 28 bytes, slots com stride class-size + 4, owner antes do payload, links/freelist e live count. Caracterize todos os argumentos de `0x107749b0`, inclusive class index, extensão e arredondamento efetivo, stores/unrolling e casos de comprimento mínimo; não deduza a semântica de zero ou bounds pelo nome.

`0x10774b60` procura pool com espaço, ou repõe via arenas existentes/growth, inicializa slots e retorna slot + 4. Count inicial decorre de `0xfe4 / (class-size + 4)`, limitado a 256. Preserve full/nonfull list rotation, contagem e owner de cada slot. `0x10774d20` libera objetos pequenos, religa pools quando necessário e pode devolver o pool vazio à arena; caracterize de onde vem o tamanho/classificação recebido e os efeitos das listas antes de nomear esse parâmetro.

Preserve especificamente o guard produtivo em **`0x10774c24`**, no failed-allocation branch de `0x10774b60`: com previous count >= 10, `next_pool_attempt(previous, candidate)` retorna `min(previous - 1, candidate)`. Toda tentativa já decrescente fica intacta; somente repetição/aumento é limitado para garantir progresso até o fallback existente. Não altere o threshold, class sizes, request rounding, sucesso anterior ou política de crescimento. Use a mesma função ou uma operação semanticamente idêntica no mesmo branch; preserve estado arquitetural e efeitos do cálculo.

Mantenha o guard do gerador e seu audit do branch original (`0x10774c00`–`0x10774c34`) para seleção original. O pin bruto do corpo contém `divwu`; a referência executável produtiva contém a correção. Mostre equivalência com essa referência, mais expectativas independentes de progresso e não sobreposição. O modo `--expect-retry-loop` da fixture existente só é pertinente à tradução histórica sem guard; não o rode contra produção corrigida e não conte budget exhaustion como sucesso normal. Não reabra o heap histórico de 213 arenas ou a investigação da origem do hint obsoleto.

### Dispatch, realocação e fachadas globais

`0x10774e80` recebe contexto/tamanho, retorna zero para tamanho zero, direciona tamanhos abaixo de 77 aos pools pequenos e recusa `size >= 0xffffffb8` antes do caminho comum. Preserve threshold/branches efetivos, inclusive distinção entre essa facade e helpers internos. `0x10774ee0` tem null no-op e usa owner low-bit para escolher free comum ou pequeno; não adicione detecção moderna de invalid/double free sem contrato original.

`0x10774f40` recebe contexto/ponteiro/novo tamanho em r3/r4/r5. Reconstrua null-pointer allocation, new-size-zero release, small/common classification, shrink/split, crescimento in-place por bloco seguinte quando permitido, e fallback allocate → BlockMoveData → free. Preserve quantidade copiada, ordem, resíduos, retorno e manutenção do antigo payload/metadados quando a nova alocação falha, conforme branches originais. Não use `realloc` host nem copie bytes além do tamanho efetivo observado. O corpo completo cruza página: não entregue somente a primeira metade.

`0x107750e0` aloca pelo dispatcher e limpa apenas quando o retorno é não nulo, pelo fill existente. As quatro fachadas globais mantêm ABI/frame/temporários próprios e obtêm contexto por `0x10774b10`: `0x10775130` recebe size em r3; `0x10775170` recebe pointer em r3; `0x107751b0` recebe pointer/size em r3/r4; `0x10775200` recebe count/element-size em r3/r4 e usa **produto low-word32** antes do allocate-and-clear. Não troque o último por checked multiplication nem corrija wrap legado sob o nome calloc.

Caracterize frames, saves/restores, GPR/FPR/temporários, CR/XER/CTR/LR/PC/TOC/FPSCR e access prefixes de cada uma das 27 entradas, inclusive leaf helpers e wrappers. Preserve calls às primitivas de memória já semânticas e os imports existentes, com seus efeitos observáveis. Nunca normalize uma divergência arquitetural para fazer equivalência passar.

### Dependências e fronteiras

Não inclua lifecycle/atexit/init/teardown `0x10773fb0`, `0x10773ff0`, `0x10774060`, glues de aquisição/disposal, nem inicialização de streams `0x10775240` em diante. Eles permanecem dependências geradas e podem chamar as entradas novas. Não reconstrua todos os serviços do heap/OS ou altere CarbonLib::NewPtr/NewPtrClear/DisposePtr/MemError.

O bridge nativo atual mantém `State::allocated`, `available`, `heap_end`, `memory_error` e mappings; usa reservas alinhadas e reaproveita/coalesce regiões disponíveis. Um pedido sem espaço retorna zero com erro de memória; DisposePtr inválido pode falhar. Compare esses efeitos e imports, além dos metadados Mac. Não confunda owner/free-list guest com os mapas de armazenamento do bridge, nem omita diferenças por serem internas ao host.

## Integração e aceitação focada

Siga os padrões atuais com adapter semântico, harness independente e audit/pins, por exemplo `native/allocator_adapter.*`, `native/allocator_equivalence_checks.cpp`, `tools/allocator_audit.py` e `tools/test_allocator_audit.py`; a organização interna cabe à implementação. Preserve a fixture `native/allocator_checks.cpp` e a correção existente. Integre em runtime/CMake/gerador; o default produtivo usa todas as 27 entradas novas. Acrescente o manifesto em ordem crescente depois das 100 anteriores, preservando seleção/reset/counters dos outros módulos. Atualize asserts afetados de tamanho total para 127 sem enfraquecer ordem/conteúdo ou alterar a história de entregas antigas.

Seleção original global/por entrada, counters e observers são privados, process-local e não serializados; preserve reset independente. No modo original do allocator, use a tradução guardada atual; módulos anteriores ficam semânticos quando a comparação exige dependências produtivas. Nenhum caminho normal pode executar o próprio corpo substituído. Original diagnóstico, escapes e transfers desconhecidos têm contagem separada e não contam como cobertura normal.

Compare original versus semântico a partir do mesmo State e história de heap: arquitetura completa, memória guest relevante, leituras/escritas ordenadas, stores até faults, imports/arguments/retornos, mapas allocated/available, heap_end, memory_error e mappings. Endereços guest retornados precisam coincidir; não basta ambos terem payload correto. Exclua somente instruction accounting e diagnósticos privados, sem normalizar ABI, metadados, efeitos ou ordem de acesso. Adicione expectativas independentes de payload, alinhamento, guards, links, boundary tags, não sobreposição, reutilização e progresso.

Construa uma matriz pequena e discriminante que cubra:

- Arenas/listas vazias, um/múltiplos nós; fit exato, split logo abaixo/acima do threshold, remoção do último livre, free e coalescing anterior/próximo/ambos, last-object arena disposal e preservar vizinhos ocupados.
- As seis classes e fronteiras 0/1/4/5/12/13/28/29/44/45/60/61/76/77; pool parcial/full, reutilização de slot, rotação, refill e pool vazio. Mutações válidas da tabela live/TOC e contexto lazy inicializado/não inicializado.
- Reposição com hints nulo/correto/obsoleto, referência dos 16 casos existentes e retry candidato menor/igual/maior. Exercite progresso, fallback growth e falha de aquisição sem campanhas de milhares de arenas.
- Sequências allocate/write/free/reuse, realocação small↔common, shrink sem/com split, crescimento in-place e moving, null/zero, falha que mantém o objeto antigo, clear somente do tamanho pedido, count-times-size word32 wrap e limites de tamanho que falham sem alocações host gigantes.
- TOC relocada, aliases representativos, mappings mínimos, remoção de bytes realmente acessados e faults com prefixo correspondente. Caracterize ponteiros/header/links malformados com budget finito; falha ou timeout histórico não vira PASS normal.

Exija **um negativo próprio por entrada** e pelo menos um child negativo rejeitado antes do parent em cada cadeia principal (arena, pool, fachada/realocação), com efeitos/imports comparados. As 27 entradas precisam de chamadas semânticas normais não vazias, zero original selection e zero escape normal; wrappers e leaf helpers sem caller externo têm fixtures próprios. Exercite dispatch indireto gerado para cada entrada, continuidade cross-page da realocação, composições pelas quatro fachadas e pela duplicação de string já entregue, além de alguns modos mistos discriminantes. Não faça produto cartesiano de seleções nem exija UI para chamar todas as entradas. Counters por entrada sustentam cobertura; menu/startup sustentam apenas o smoke observado.

Após estabilização, regenere uma vez e faça Release ARM64 consistente, especialmente se State mudar. Comandos propostos para o novo target:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure -R '^(modern_allocator_equivalence|original_allocator_replenishment|modern_string_core_equivalence|modern_memory_primitives_equivalence|native_bridges)$'
python3 tools/test_allocator_audit.py
python3 tools/test_formatted_output_audit.py
python3 tools/test_character_conversion_audit.py
python3 tools/test_floating_input_audit.py
python3 tools/test_formatted_input_audit.py
python3 tools/test_integer_parsing_audit.py
python3 tools/test_memory_primitives_audit.py
python3 tools/test_string_core_audit.py
python3 tools/test_collection_lookup_audit.py
python3 tools/test_background_selection_audit.py
```

Use os nomes novos efetivamente implementados. Reposição, strings, memória e bridges são regressões diretamente usadas pelo bloco; os Python antigos entram pelos asserts de manifesto/integração alterados, não para reabrir subsistemas. Execute UBSAN no novo adapter/harness e em código efetivamente modificado, ligado às bibliotecas finais consistentes. Amplie checks somente por falha concreta/dependência afetada, com motivo e stopping condition. Repita geração/build/grupos verdes somente por mudança que os invalide. Linkage misto de layouts antigos/novos de State não é aceitação.

## Instalação protegida e smoke curto

Antes da atualização, preserve o app instalado com forks/xattrs/metadados em nova referência privada, por exemplo `.local/allocator-pools-reconstruction/reference/installed.app`, sem sobrescrever referências/candidatos anteriores. Reenumere e confira proteção de game files, saves/preferências, payloads originais, snapshot Git LFS, handoffs e históricos privados dos blocos entregues. Não rode o instalador durante uma sessão do usuário nem encerre sua sessão à força.

Após aceitação técnica, use `tools/install_native.py` uma vez para a atualização final. Distinga linker output, source/candidato assinado, instalado e referência pelo executável interno/ARM64/hash. Confira original payloads, assinatura e plist de produção; fixtures/diagnósticos/configuração de validação não entram no bundle. Verifique proteção imediatamente após instalação, antes do smoke.

Abra o app **instalado** em `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`, observe menu renderizado e uma resposta simples por comando nomeado; pode abrir/cancelar chooser sem carregar carreira. Encerre normalmente se ainda for exclusivamente uma sessão de teste; preserve-a se o usuário assumir a interação. Confira processo/log, ausência de falhas fatais e hash testado. Pare aí: cobertura das 27 entradas vem do harness. Logs/preferências podem mudar; não declare Runtime/Runs byte-idênticos após smoke. Save/Load, dias e partidas não entram.

## Entrega

Documente em inglês contratos/pins/layout, preservação explícita do guard, integração/manifesto, ABI/effects/faults, per-entry evidence e checks realmente executados em `docs/native-allocator-pools-reconstruction.md`; atualize `docs/native-port-status.md`. Diferencie a tradução Mac bruta, a correção produtiva histórica, o check guardado desta preparação, a equivalência nova e o smoke instalado. Registre identidade instalada e limites; persistência permanece **not retested**. Não declare reconstruídos lifecycle/streams/glues/allocator CarbonLib ou todo o runtime C.

Revise o diff exato e publique somente adapter, harness, audits, integração produtiva e documentação técnica por commit/push normal ao remoto/branch corretos, seguido de consulta nova do SHA remoto. Preserve `.gitignore`, LFS e mudanças alheias; exclua prompts privados, receipts/logs/imagens/saves/índices/payloads/fontes geradas/bundles. Não force push.

Conclua com as 27 entradas produtivas e o manifesto de 127, resultados focados, prova independente de progresso preservado, evidência do app instalado, commit publicado e limitações materiais. O goal termina com essa implementação e aceitação proporcional; não acrescente gates dispensados.
