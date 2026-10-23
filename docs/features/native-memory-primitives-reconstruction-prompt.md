/goal Reconstrua e entregue o núcleo Mac de primitivas de memória em C++23 semântico na produção ARM64. Implemente conjuntamente as seis entradas abaixo, preserve seus contratos arquiteturais e o bridge de cópia existente, valide o bloco com checks focados e faça uma única instalação/entrega ao fim. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto e continuidade

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em C++ compreensível e comportamento original Mac preservado. O jogo já funciona pelo caminho ARM64 de recompilação estática e bridges nativos. Date, estado/inicialização RNG, apresentação Retina com escala e comandos diretos, seleção de backgrounds, consultas de coleções e o núcleo básico de strings/descritores já foram entregues. A aparência foi aprovada; avance na lógica.

O próximo subsistema é o núcleo de operações sobre blocos de bytes com tamanho explícito: cópia de bloco, preenchimento e seu wrapper de retorno, comparação, busca da primeira e da última ocorrência de byte. Ele complementa as strings terminadas em NUL já semânticas e mantém a preparação dos buffers e registros usados pelo engine. Um bloco com bytes zero internos deve continuar sendo comparado, preenchido ou pesquisado até o limite informado, sem assumir terminação textual. A cópia mantém o caminho produtivo de `BlockMoveData`, incluindo sobreposição.

Entregue o bloco inteiro neste goal, com checkpoints internos e uma aceitação compartilhada. Não entregue somente wrappers nem traduza opcode por opcode. Implemente operações legíveis com compatibilidade ABI explícita; os corpos substituídos não podem ser executados como fallback no modo normal. Não prometa ganho visual ou de desempenho.

## Decisões vigentes de aceitação

- Save/Load, save/exit/reopen/load, comparação de carreiras e auditoria de serialização estão dispensados por orientação expressa do usuário em 5 de outubro de 2026. Registre persistência como **não retestada**, sem converter a dispensa em PASS ou gate pendente.
- Faça checks proporcionais ao subsistema. Não rode toda a suíte CTest/Python, campanhas Date/RNG/allocator, replays day/week, nova campanha de temporada ou UI repetida por rotina. Amplie apenas diante de falha concreta ou dependência efetivamente afetada, com motivo e stopping condition.
- Um build consistente, comparação focada de ABI/efeitos com o original, negativos próprios, dispatch não vazio e regressões diretamente relacionadas encerram a aceitação técnica. Checks verdes não justificam novas campanhas.
- Implementação, documentação, instalação protegida e commit/push normal do produto concluído e validado estão autorizados. Não reabra essas decisões para pedir confirmação. Preserve alterações alheias, dados privados e histórico.

Este pedido é autocontido. Não importe gates antigos dos handoffs Date/RNG ou de outras entregas.

## Ponto de partida verificado em 5 de outubro de 2026

- Branch `main`, HEAD e referência local `origin/main`: `91b0cfadea5ffc60994d6911e596af2960ca2955`, `Reconstruct Mac basic string and descriptor core in semantic C++23`. A entrega imediatamente anterior confirmou esse SHA também por consulta nova ao remoto `git@github.com:VictorDMor/cm0102mac.git`. Reconfira branch, remoto, diff e estado remoto antes de trabalhar.
- Não havia alterações versionadas ou staged. Havia treze handoffs privados untracked em `docs/features`, além de cinco prompts versionados; este arquivo acrescenta o décimo quarto privado. Preserve todos os prompts, inclusive os antigos/versionados, e exclua os privados do commit de produto. Não foi encontrado `AGENTS.md` aplicável no projeto e ancestrais inspecionados; reconfira instruções.
- `.local/recompiled/compilation.json` contém 60 `semanticReplacements`: 34 Date, três RNG, quatro backgrounds, cinco consultas de coleções e quatorze strings/descritores. Há 1.922 páginas, 1.953.157 instruções indexadas, zero `missingStaticTargets` e zero `pendingTargets`. As seis entradas deste pedido permanecem geradas. Resultado esperado: **66 entradas**, preservando as 60 anteriores na mesma ordem.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` foram reconferidos ARM64. O executável interno `Contents/MacOS/CM 01-02 Native Prototype` tem SHA-256 `733a5fca03917f5e74fdd35109de93caac221b522cd8b8098b0c9f509fe0e721` nos dois bundles. A instalação anterior terminou em 5 de outubro de 2026 às 18:34:58.636 em São Paulo.
- `docs/native-string-core-reconstruction.md` registra 314 comparações diretas, 62 faults, quatorze negativos próprios, quatorze entradas com dispatch não vazio, quatro modos mistos, composições de coleções/backgrounds, seis fallbacks Date, dois formatos normais e UBSAN focado. Os quatro CTests relacionados e os audits afetados passaram. São evidências da entrega anterior, não aceitação deste bloco. Nenhuma sessão do app foi aberta naquela entrega; persistência não foi retestada.
- Preserve as referências privadas `.local/background-selection-reconstruction/reference/installed.app`, `.local/collection-lookup-reconstruction/reference/installed.app` e `.local/string-core-reconstruction/reference/installed.app`, além das referências/candidatos anteriores. A referência de strings preserva o instalado anterior à última entrega, com executável `bb3ddaf091677f05124a9b82ab9afe4c2c65edb1f849d5d07abe6d6e17ebab8d`; ela não representa o instalado atual.

Esta preparação foi somente inspeção e handoff. Não implementou estas entradas, não fez build/testes de aceitação, não abriu o aplicativo nem instalou/publicou outra versão.

## Inspeção inicial dirigida

Leia `README.md`, a seção mais recente de `docs/native-port-status.md`, `docs/native-string-core-reconstruction.md` e os contratos existentes de Date/RNG apenas quando necessários para preservar dependências. Inspecione:

- `native/string_core_adapter.*` e `native/string_core_checks.cpp`: comparação completa, observadores, faults, modos privados, negativos próprios e pequenos fixtures de formatter/allocator.
- `native/aot_runtime.hpp`, `native/aot_runtime.cpp`, `native/aot_probe.cpp` e `native/CMakeLists.txt`: State, Memory, dispatch, seleção original e targets.
- `native/carbon_bridge.cpp` e `native/bridge_checks.cpp`: `CarbonLib::BlockMoveData`, cópia por buffer temporário e sobreposição. A API `Memory::copy_out` atual verifica o intervalo e não emite eventos escalares de leitura; não invente esses eventos nem mude o bridge para produzir uma prova mais conveniente.
- `native/decimal_checks.cpp`: formatter gerado `0x1077b680`. Seu caminho de `%s` com precisão explícita alcança a busca limitada incluída neste bloco, pelo call em `0x1077b0a8`.
- `native/allocator_checks.cpp` somente para entender os callers gerados do preenchimento em `0x10774af4`, `0x1077510c` e `0x10775298`; reconstrução e nova campanha do allocator ficam fora.
- `tools/compile_recomp.py`, `tools/string_core_audit.py`, `tools/test_string_core_audit.py`, `tools/test_collection_lookup_audit.py` e `tools/test_background_selection_audit.py`: hooks de destino, manifesto, fingerprints e asserts que hoje esperam 60 entradas.
- `.local/recomp-input/instructions.sqlite`, aberto read-only, e as páginas privadas geradas `0x10739`, `0x10778` e o stub `0x1077fc28`. Use o export existente; não inicie novo export Ghidra nem publique listings, segmentos, tabelas ou payloads proprietários.

Reconfirme limites, entradas internas, efeitos e dependências contra o original Mac. Nomes de bibliotecas padrão e Windows/ReCM não estabelecem os contratos deste binário.

## Escopo fechado e fingerprints

| Entrada | Fim exclusivo | Instruções indexadas | Calls diretos indexados | Operação |
| --- | --- | ---: | ---: | --- |
| `0x10739ef0` | `0x10739f2c` | 15 | 123 | Cópia de bloco por `BlockMoveData`, com retorno do destino |
| `0x10778300` | `0x10778330` | 12 | 207 | Wrapper de preenchimento que retorna o destino |
| `0x10778330` | `0x10778360` | 12 | 1 | Primeira ocorrência de byte em bloco limitado |
| `0x10778360` | `0x10778390` | 12 | 1 | Última ocorrência de byte em bloco limitado |
| `0x10778390` | `0x107783e0` | 20 | 12 | Comparação de blocos com resultado `-1/0/+1` |
| `0x107783e0` | `0x107784a0` | 48 | 1 | Preenchimento com alinhamento, words e cauda de bytes |

Total: **119 instruções indexadas**. Counts de calls são evidência estática de relevância e incluem relações internas; não provam runtime nem exigem executar todos os callers. O preenchimento leaf é chamado pelo wrapper. A cópia chama o stub gerado `0x1077fc28`; as demais operações leaf não têm subcalls diretos.

Fingerprints SHA-256 de registros `address/assembly/ops/flows/fallthrough`, ordenados por endereço e serializados como JSON compacto com chaves ordenadas e UTF-8, conforme os audits existentes:

| Entrada | SHA-256 |
| --- | --- |
| `0x10739ef0` | `05753a5ef9136a3145548dc38d58233103e180dba5e5bd108b49975c7f3f80cc` |
| `0x10778300` | `6b9670af9fecd70c81015ca9a73e470266ec3eb4ff3467391ac3ac859f5db50f` |
| `0x10778330` | `8da31292109a0dec94712af7b85a3179ffeffd3002d8a706600ae37c4fe56b12` |
| `0x10778360` | `32b1246d369b4edcef213b8d941f0bad79142c76d12188af939051d43a4c187b` |
| `0x10778390` | `f95bcb514d99050544dfba105b2be24722695b33bb32e80c6fc60580e90be7f0` |
| `0x107783e0` | `0a98cb1d92b0f33ccdd4204b7b7ec3cce64d573dc1bb2c1453dea7e9a59a1a4e` |

A inspeção de todos os flows e fallthroughs indexados confirmou zero arestas estáticas externas para os interiores das seis entradas. Reconfira e mantenha a prova no audit. Isso não classifica entradas dinâmicas desconhecidas.

### Cópia de blocos e fronteira nativa

`0x10739ef0` recebe destino/fonte/count word32 em r3/r4/r5. Preserva destino em r31, escreve spill de r31 e LR/backchain, cria frame de 64 bytes e troca a ordem dos ponteiros para chamar o stub `0x1077fc28` com fonte/destino/count. Depois recarrega TOC do slot de transição `frame+0x14`, retorna o destino preservado e restaura frame/link/r31 na ordem original. Spills e slot TOC podem aliasar dados; dependências podem clobberar registradores.

O stub carrega o vector mutável de `r2-0x7e64`, salva TOC, lê entry/TOC do vector e usa CTR. Execute essa dependência pelo caminho de ABI existente, incluindo imports, contagem de native calls, PCs e faults. O bridge atual captura o bloco-fonte antes de escrever o destino, preservando sobreposição. Count zero ainda executa o stub/import, embora o bridge não leia/escreva dados do bloco. Não substitua esse caminho por `std::memcpy`, cópia progressiva, atalho para zero ou novo bridge. O contrato é do wrapper Mac em cima da integração nativa já produtiva; o serviço Carbon não está sendo reconstruído neste goal.

### Preenchimento e wrapper

`0x107783e0` usa destino/valor/count em r3/r4/r5, truncando o valor a unsigned8. Para count menor que 32, escreve bytes progressivamente. Para count maior ou igual a 32, prepara o alinhamento de quatro bytes, repete o byte em uma word quando necessário, escreve grupos de oito words, words restantes e a cauda de até três bytes. Preserve a ordem e largura dos acessos: oito stores de quatro bytes não são um store de 32 bytes, e faults podem ocorrer após um prefixo escrito.

Preserve contadores word32, carries/CR, cursor updates somente após acesso bem-sucedido, registradores residuais e r3 do leaf. O leaf não promete retornar sempre o destino: no caminho grande r3 é reutilizado pelos contadores. Count zero evita stores de dados, mas ainda realiza as operações arquiteturais iniciais. O wrapper `0x10778300` cria frame de 64 bytes, preserva r31 e chama esse leaf pelo guest entry; ele restaura o retorno do destino e o frame/spills. Não transforme ambos no mesmo contrato nem chame o corpo original do leaf no modo normal.

### Buscas limitadas e comparação

`0x10778330` procura o primeiro byte, truncando r4 a unsigned8 e percorrendo para frente. `0x10778360` procura o último, começando de `base+count` com wrap word32 e lendo em ordem reversa. Retornam o guest address encontrado ou zero. Bytes NUL internos são dados pesquisáveis. Os contadores usam incremento inicial e decremento anterior ao acesso, com wrap, XER carry e CR. Count zero não lê dados, inclusive com ponteiros inválidos. Preserve cursores e resíduos exatamente.

`0x10778390` compara unsigned8 até o limite word32, sem terminação por NUL. Retorna exatamente `-1`, `0` ou `+1`, distinto da diferença em `[-255,255]` do comparador de strings já entregue. Em cada par lê esquerda e direita; ao encontrar divergência **relê esquerda e direita** antes da decisão final. Mantenha essas leituras, registradores/flags e a ordem de faults, inclusive quando um observer altera os bytes entre as leituras. Não substitua o comportamento por `std::memcmp` ou reutilize o contrato do comparador textual. Count zero não toca os blocos, mas mantém seus efeitos arquiteturais.

### Integração e limites

Adicione hooks antes da primeira operação nos labels de destino gerados das páginas `0x10739` e `0x10778`. Calls/gotos na mesma página, dispatch entre páginas e transições indiretas devem atingir os mesmos hooks. A produção seleciona as seis entradas semânticas por padrão; seleção original global/per-entry deste bloco fica privada e independente dos módulos já entregues. Mantenha o manifesto anterior e acrescente as seis entradas.

Não reconstrua o stub/import Carbon, allocator/pools, `Memory`, strings novamente, conversão Pascal do caller, runtime de streams/arquivos, classificação/parsing, formatter inteiro, sampler RNG `0x107124e0`, lifecycle de coleções, gameplay, multiplayer, serialização, drawing/menus/escala ou otimizações gerais. A busca reversa tem caller em `0x10776530`, num caminho de buffer/newline; preserve-o gerado, sem abrir uma campanha de I/O para provar esse único caller. Preserve a assimetria bissexta Date e os guards club/competition de `0x10713aec`/`0x10713b3c`.

## Aceitação focada e discriminante

Crie um harness compartilhado, por exemplo `cm-memory-primitives-checks` / `modern_memory_primitives_equivalence`. Original e candidato começam do mesmo estado, com os módulos antigos semânticos. Compare arquitetura completa — registradores/temporários, CR/XER/LR/CTR/TOC/PC —, toda memória mapeada, estado allocator/imports e efeitos ordenados de leitura/escrita/map/clear, incluindo prefixos antes de faults. Contagem de instruções e diagnósticos privados são excluídos da igualdade comportamental. Compare diretamente os corpos guest; `call_original` que restaura o contexto é prova adicional de callback, não substituto da ABI direta.

Use expectativas independentes de conteúdo, resultado, retorno, ordem/largura de acessos, counts e cursor/carry. Uma matriz pequena deve distinguir:

- Buscas primeira/última com repetição, ausência, NUL/byte alto e truncamento de r4; count zero, um, limite antes/no/depois do match; wrap word32 e direção reversa. Use mappings finitos para faults rápidos em counts extremos, sem percorrer bilhões de bytes.
- Comparação igual e desigual no primeiro/último byte, bytes zero internos, unsigned8, resultado exato `-1/0/+1`, count zero e diferenças fora do limite. Inclua um observer controlado que distinga as releituras de uma comparação armazenada, mantendo a mesma intervenção nos dois modos.
- Preenchimento zero/não zero/byte alto/truncado, alinhamentos 0..3, lengths pequenos e fronteiras 31/32/33, grupos de 32 bytes, words e cauda. Verifique guard bytes, largura/ordem dos stores, r3 leaf versus wrapper e faults após prefixos parciais. Não faça campanha cartesiana de tamanhos.
- Cópia vazia/não vazia, iguais e sobrepostos em ambos os sentidos; fonte ou destino inválido/insuficiente, count word32, vector/slot/TOC parcialmente mapeados, TOC relocável e aliases selecionados com frame/spills. A cópia pelo bridge mantém sua verificação bulk; não exija efeitos parciais de byte que esse caminho não produz.
- Mappings mínimos dos bytes efetivamente acessados, faults de leitura/escrita e cursores preservados em acessos que falham, sem novos guards, truncamento de counts ou correções do comportamento original.
- Um negativo próprio por entrada, rejeitado em sua primeira fronteira própria de retorno, incluindo os dois wrappers; diferencie negativo de filho/pai. Verifique reset e seleção original, sem gravar candidato como oracle.
- Preenchimento wrapper original/leaf semântico e o inverso, mais bloco todo semântico. Exija counts próprios não vazios das seis entradas e zero original-selected executions em witnesses normais, incluindo a dependência real do wrapper e ao menos uma transição indireta pelo mecanismo existente. Counters são diagnósticos process-local, não dados de save/snapshot.

Inclua duas composições pequenas com consumidores ainda gerados:

1. `0x107784c0` converte uma string NUL para buffer de comprimento inicial: chama length semântico `0x1077dbd0`, a cópia agora semântica `0x10739ef0` e grava o comprimento em um byte. Compare casos vazio/não vazio, retorno, guard bytes e efeitos; preserve esse caller gerado.
2. Um ou dois formatos `%s` com precisão explícita no formatter original `0x1077b680`, com tamanho normal e truncamento, alcançando realmente `0x10778330`. Comprove o count do filho; não presuma que os formatos decimais históricos cobrem essa busca. Configure FPSCR válido nos fixtures, como no harness de strings atual. Não amplie para parsing/formatter inteiro.

Faça uma regeneração depois de estabilizar hooks e um build consistente. Mudança de State/layout exige rebuild de todos os objetos dependentes; não repita `clean-first` por rotina. Rode o harness/audit novos e os CTests `modern_string_core_equivalence`, `original_decimal_formatting` e `native_bridges`, que preservam o núcleo anterior/formatter e o bridge usado. A atualização dos asserts de manifesto afeta os três audits strings/collection/backgrounds; execute esses audits sem transformar a extensão do manifesto em campanha de todos os subsistemas. Instrumente somente adapter/harness novos com UBSAN e informe a abrangência exata. Reexecute grupos verdes apenas após alteração/falha que os afete.

Comandos-base a partir da raiz; adapte apenas os nomes efetivos dos novos arquivos/targets:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure \
  -R '^(modern_memory_primitives_equivalence|modern_string_core_equivalence|original_decimal_formatting|native_bridges)$'
python3 tools/test_memory_primitives_audit.py
python3 tools/test_string_core_audit.py
python3 tools/test_collection_lookup_audit.py
python3 tools/test_background_selection_audit.py
```

Os nomes novos acima são sugestões a implementar, não checks já existentes/aprovados. Não faça live counters por entrada um gate adicional. Inspeção curta de tela diretamente afetada é opcional quando acrescentar evidência útil; não abra carreira pessoal nem use Save/Load para preparar coverage. Declare precisamente se a UI não foi exercitada.

## Proteção e entrega única autorizada

Preserve saves pessoais, o snapshot LFS, inputs originais, fixtures, referências/candidatos históricos, preferências e todos os handoffs. Mantenha novos fixtures/diagnósticos privados e compactos sob `.local`. Verifique espaço antes de gerar dados; não copie carreiras, não recrie capturas gigantes removidas anteriormente nem limpe `.local` ou históricos. Guarde recibos compactos e comandos reproduzíveis, sem repetir inventários globais a cada ajuste.

Escreva em inglês `docs/native-memory-primitives-reconstruction.md`: seis contratos/fingerprints, 66 entradas do manifesto, diferenças de retorno/contadores, ordem e largura de acessos, fronteira do bridge, casos/faults/negativos, dispatch, modos mistos/composições, checks e limitações. Atualize `docs/native-port-status.md`; preserve a validade histórica dos relatórios anteriores. Diferencie inspeção estática, execução direta, intervenções controladas, instalação e eventual UI. Registre persistência como não retestada por orientação do usuário.

Depois dos checks focados, instale uma vez com `python3 tools/install_native.py`. Antes de substituir o instalado atual, crie uma nova referência privada deste subsistema, por exemplo `.local/memory-primitives-reconstruction/reference/installed.app`, preservando integralmente as referências anteriores. Use as proteções do instalador e confira uma vez antes/depois conteúdo e metadados dos arquivos pessoais potencialmente afetados, preferência de apresentação e handoffs. O relatório de strings registrou 809 Game / cinco Runtime / 46 Runs e ausência de `Presentation.plist`; são observações históricas, não invariantes a impor ao estado atual.

Verifique ARM64 e hashes separados de fonte, candidato assinado, instalado e nova referência; bundled originals, assinatura aplicável, plist de produção sem diagnósticos e horário efetivo da instalação. Inclua os originais antes de assinar a fonte/candidato conforme o fluxo existente. A data da pasta `.app` não identifica o executável instalado. Não abra carreira pessoal como prova da instalação.

Revise o diff exato, preserve alterações alheias e faça commit/push normal do produto concluído e validado para o `origin` existente `git@github.com:VictorDMor/cm0102mac.git`, sob a autorização permanente dada pelo usuário. Reconfirme destino/permissão quando necessário e verifique o SHA remoto por consulta nova após o push. Exclua todos os handoffs privados, saves, segmentos, payloads, snapshots, screenshots, logs, fixtures e bundles privados. Não altere ignore/LFS para publicá-los e não force o push.

Conclua quando as seis entradas executarem C++23 semântico em produção, contratos/efeitos e composições forem equivalentes, negativos/dispatch/regressões focados passarem e documentação, instalação protegida e entrega estiverem conferidas. Relate o núcleo de primitivas delimitado e as dependências ainda geradas; não declare toda a memória, runtime C ou engine reconstruídos. A sessão que prepara este handoff somente salva e copia o prompt; implementação, instalação e publicação pertencem à sessão que executar este pedido.
