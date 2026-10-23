# Reconstruir o núcleo básico Mac de strings e descritores em C++23 semântico

Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; escreva código, testes, documentação técnica e novas mensagens do app em inglês.

## Objetivo e continuidade

O jogo já funciona nativamente em ARM64. Substitua gradualmente o engine gerado por C++23 compreensível, preservando o comportamento original do Mac. Date, estado/inicialização RNG, apresentação Retina com escala e comandos diretos, seleção de backgrounds e consultas de coleções já foram entregues. A aparência foi aprovada; avance agora na lógica.

Reconstrua um único subsistema delimitado com quatorze entradas: comprimento, cópia, cópia limitada, concatenação, comparação por bytes, comparação limitada por bytes, busca da primeira/última ocorrência de byte e de substring, comparação pela tabela Mac completa/limitada, duplicação, transformação pela tabela Mac e wrapper de descritores. A cadeia seleção → consulta de coleção → comparação passará a executar semântica C++23 também nessa dependência; o mesmo núcleo permitirá preparar, copiar e procurar textos com as regras originais.

Este escopo amplia o handoff inicial de três comparações a pedido do usuário, fechando o núcleo básico de strings de bytes terminadas em NUL. É um subsistema funcional de operações básicas; serviços de linguagem, templates, formatação e gestão geral de memória continuam separados. Não declare todos os serviços de texto do engine reconstruídos.

O comparador pela tabela Mac possui 1.448 calls estáticos distribuídos por 26 páginas; sua variante limitada possui nove nas páginas `0x10341` e `0x10342`. A cópia possui 6.336 referências estáticas, a duplicação 12, o mapper duas e a busca de substring 84. Existem consumidores produtivos de backgrounds e catálogos, além da apresentação Date já semântica. Esses números justificam relevância, mas não provam execução em runtime nem exigem uma campanha de todos os callers. Não prometa ganho visual ou de desempenho.

Implemente a lógica real em C++ legível, com compatibilidade arquitetural explícita. Não entregue somente os wrappers, uma tradução opcode por opcode ou wrappers que executem os corpos originais das quatorze entradas em modo normal.

## Decisões de aceitação já autorizadas

- Save/Load, save/exit/reopen/load, comparação de carreiras e auditoria de serialização estão dispensados por orientação expressa do usuário em 5 de outubro de 2026. Registre persistência como não retestada; não transforme essa dispensa em PASS ou gate pendente.
- Use checks proporcionais ao bloco. Não rode a suíte inteira, todas as famílias Python, campanhas Date/RNG, replays day/week ou UI repetida por rotina. Amplie somente diante de falha concreta ou dependência realmente afetada, explicando o motivo.
- Um build consistente, comparação focada de ABI/efeitos com o original, negativos próprios, dispatch não vazio e regressões diretamente relacionadas encerram a aceitação técnica. Checks verdes não justificam novas campanhas.
- Não reabra essas decisões para pedir confirmação. Implementação, documentação, instalação protegida e commit/push normal do produto concluído estão autorizados conforme a entrega abaixo.

Este pedido é autocontido. Não importe gates antigos de outros handoffs.

## Ponto de partida verificado

- Em 5 de outubro de 2026, branch `main`, HEAD e referência local `origin/main`: `5d1efac195e73e3dddd3863bb3ac34028cb723bb`, `Reconstruct Mac collection lookup core in semantic C++23`. A entrega anterior confirmou também esse SHA no remoto `git@github.com:VictorDMor/cm0102mac.git`. Reconfira branch, remoto, estado remoto, diff e instruções antes de trabalhar.
- Não havia alterações versionadas ou staged. Havia doze handoffs anteriores untracked em `docs/features`; este arquivo será o décimo terceiro. Preserve todos os prompts existentes, inclusive os versionados, e exclua os handoffs privados do commit de produto. Não foi encontrado `AGENTS.md` aplicável nos ancestrais e projeto inspecionados; reconfira.
- `.local/recompiled/compilation.json` contém 46 `semanticReplacements`: 34 Date, três RNG, quatro backgrounds e cinco consultas de coleções. São 1.922 páginas, 1.953.157 instruções indexadas e nenhuma `missingStaticTargets`. As quatorze entradas deste pedido permanecem geradas. Resultado esperado: 60 entradas, preservando as 46 existentes na mesma ordem.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` foram reconferidos ARM64, com executável `Contents/MacOS/CM 01-02 Native Prototype` de SHA-256 `bb3ddaf091677f05124a9b82ab9afe4c2c65edb1f849d5d07abe6d6e17ebab8d`. A instalação anterior terminou em `2026-10-05T15:55:19.058465-03:00`.
- `docs/native-collection-lookup-reconstruction.md` registra 263 comparações, 27 faults, cinco negativos próprios, cinco entradas com dispatch não vazio, callbacks/transições indiretas, oito composições de backgrounds e UBSAN focado. Os dois checks de backgrounds e os audits afetados passaram. São evidências da entrega anterior; não são aceitação destas quatorze entradas. Nenhuma sessão do app foi aberta naquela entrega.
- Preserve as referências privadas existentes `.local/background-selection-reconstruction/reference/installed.app` e `.local/collection-lookup-reconstruction/reference/installed.app`. A preferência de produção `Presentation.plist` estava ausente; preserve o estado encontrado, inclusive ausência.

## Inspeção inicial dirigida

Leia `README.md`, a seção mais recente de `docs/native-port-status.md`, `docs/native-collection-lookup-reconstruction.md` e `docs/native-background-selection-reconstruction.md`. Inspecione:

- `native/collection_lookup_adapter.*`, `native/collection_lookup_checks.cpp`, `native/background_selection_adapter.*`, `native/background_selection_checks.cpp` e `native/background_checks.cpp`: consumidores semânticos, fixtures, observadores e comparação de efeitos.
- `native/aot_runtime.hpp`, `native/aot_runtime.cpp`, `native/aot_probe.cpp` e `native/CMakeLists.txt`: State, Memory, dispatch, modos privados e targets.
- `native/date_subsystem_adapter.cpp` e `native/date_presentation_checks.cpp`: seis fallbacks de apresentação Date que chamam a cópia agora incluída. Reaproveite apenas fixtures pequenos desses caminhos, sem herdar a campanha de apresentação Date.
- `native/allocator_checks.cpp` e a dependência gerada `0x10775130`: ABI e efeitos de alocação usados pela duplicação; a reconstrução do allocator fica fora.
- `native/decimal_checks.cpp`: regressão de formatação diretamente afetada, pois o scanner gerado alcança busca de byte e comprimento incluídos neste bloco.
- `tools/compile_recomp.py`, `tools/collection_lookup_audit.py`, `tools/test_collection_lookup_audit.py`, `tools/background_selection_audit.py` e `tools/test_background_selection_audit.py`: hooks de destino, manifesto e fingerprints.
- O índice original `.local/recomp-input/instructions.sqlite`, aberto somente leitura, e as páginas geradas `0x10739`, `0x1074d` e `0x1077d`. Use o export privado existente; não inicie nova exportação Ghidra nem publique listings, segmentos ou payloads proprietários.

Confirme as fronteiras, contratos e referências contra o original Mac; não transplante algoritmos Windows/ReCM nem assuma contratos de bibliotecas padrão.

## Escopo fechado e fingerprints

| Entrada | Fim exclusivo | Instruções indexadas | Função |
| --- | --- | ---: | --- |
| `0x107393f0` | `0x107394dc` | 58 | Comparação textual completa pela tabela Mac |
| `0x107394e0` | `0x107395f0` | 66 | Comparação pela mesma tabela, com limite signed16 |
| `0x1074de10` | `0x1074de30` | 8 | Wrapper ABI de comparação de descritores |
| `0x107395f0` | `0x10739640` | 20 | Duplicação de string com alocação original |
| `0x10739640` | `0x1073967c` | 15 | Transformação in-place pela tabela Mac |
| `0x1077dbd0` | `0x1077dbec` | 7 | Comprimento até NUL |
| `0x1077dbf0` | `0x1077dc0c` | 7 | Cópia progressiva incluindo NUL |
| `0x1077dc10` | `0x1077dc54` | 17 | Cópia limitada e preenchimento com zeros |
| `0x1077dc60` | `0x1077dc8c` | 11 | Concatenação progressiva |
| `0x1077dc90` | `0x1077dcc8` | 13 | Comparação por diferença de bytes unsigned8 |
| `0x1077dcd0` | `0x1077dd10` | 16 | Comparação por bytes com limite word32 |
| `0x1077dd10` | `0x1077dd44` | 12 | Busca da primeira ocorrência de byte |
| `0x1077dd50` | `0x1077dd98` | 18 | Busca da última ocorrência de byte |
| `0x1077dda0` | `0x1077de0c` | 27 | Busca de substring |

Total: 295 instruções indexadas. Há gaps internos; não calcule o número apenas pelo intervalo de endereços. Fingerprints SHA-256 de registros `address/assembly/ops/flows/fallthrough`, ordenados por endereço e serializados como JSON compacto com chaves ordenadas e UTF-8, conforme os audits existentes:

| Entrada | SHA-256 |
| --- | --- |
| `0x107393f0` | `6f374bd1dea6768a6421e7051816861b7cbf0c59614b21b308005c6581bec66d` |
| `0x107394e0` | `c23f8fb6a286136572ec06ecc9eeca532a402f40b68c32b3d067806e79628a53` |
| `0x1074de10` | `f16b28e9df93c459efaf7236ff6d18a0f15e97eb1126ed662e814e0a6ffe6f03` |
| `0x107395f0` | `ac606d3aa261c47d1fbdaa1f0d5b172bb243fcae4dc0a394155227854fbe39e4` |
| `0x10739640` | `efa50a960b458ddba3e0d46cac87f6c8058fc784e97abad96bba13fc4006878f` |
| `0x1077dbd0` | `d877b190f9567050ea7f43437dff67853054426d5b9841fe83432bfe6680d4fb` |
| `0x1077dbf0` | `774d415f9f2b8f7f1ecddcc18432a8a87aaba3a51506c94eae9dc9b462052838` |
| `0x1077dc10` | `b54b65eae1a77dca62311420c12f051c7b5ea5949d047d60287185be2cabe465` |
| `0x1077dc60` | `6dfee147a562bb700896eb5e9f31e620be83e74cbb0b07bb526f32b9f2423a91` |
| `0x1077dc90` | `5717c6b7d0892339f0b3784045f4cc34d6421c7dfbc6f56cb865028ea7c469cc` |
| `0x1077dcd0` | `79aca0a48020beeb690115fd1e124bbdcd748aa7c70be9b8501bd6299ad063ae` |
| `0x1077dd10` | `5fe3ff62a0b22e9d4c00e4e50c887920c159cbd6af02688617124168cf297b20` |
| `0x1077dd50` | `3ac24746630eecd0833c589527c2e401291a2c2ee97bea64f7f156ee8d02425b` |
| `0x1077dda0` | `eb0d586f64e7df8aba82ff0f40db56cef3169fe98328dd02254ebdbc96643208` |

A inspeção de todos os flows/fallthroughs indexados confirmou zero arestas estáticas externas para os interiores das quatorze entradas. Reconfira e registre isso no audit; não classifica entradas dinâmicas desconhecidas.

### Comparação completa

Recebe ponteiros em r3/r4. A comparação inicial de nulidade de r3 precede uma leitura obrigatória do ponteiro da tabela em `r2-0x62bc`; a tabela é carregada mesmo quando algum argumento é nulo. Qualquer ponteiro nulo retorna zero, mas uma leitura inválida desse slot pode falhar antes desse retorno.

Com argumentos presentes, lê bytes brutos da esquerda e da direita na ordem original. NUL bruto encerra a string. Outros bytes sofrem extensão signed8; o byte bruto `0xff` produz `-1` diretamente e não consulta a tabela. Para os demais, indexa a tabela pelo byte unsigned8 e interpreta o valor mapeado como signed8. Retorna exatamente `-1`, `0` ou `1`. A terminação depende do byte bruto, mesmo quando sua transformação vale zero.

Preserve o ponteiro de tabela carregado no início, seus conteúdos mutáveis e cada leitura posterior. No TOC original, o ponteiro observado é `0x1091c978`; converte minúsculas ASCII em maiúsculas e tem transformações adicionais de bytes altos. Esse conteúdo não autoriza tabela fixa, Unicode, locale do host, `strcmp` ou `strcasecmp`.

Preserve o avanço de r3/r5, r4 como base direita, índice r9, clobbers/flags e releituras do terminador direito em r4+índice. Ponteiros iguais e leituras repetidas continuam observáveis; não deduplique acessos.

### Comparação limitada

Recebe os ponteiros em r3/r4 e reduz r5 a signed16 para o limite. Mantém a nulidade, leitura inicial do slot de tabela e comparação signed8 descritas acima, mas com registradores e PCs próprios.

Antes de verificar o limite, lê o próximo byte esquerdo e, se ele não for NUL, o direito. Limite zero, negativo ou já atingido pode causar faults nessas leituras. Depois de N caracteres iguais, pode ler no índice N antes de retornar zero. Não substitua esse contrato por comparação que evita qualquer acesso quando o limite é não positivo.

Preserve índice r11, cursor direito r6, releitura da redução signed16 no encerramento, caminhos de NUL, releituras da base direita e resíduos arquiteturais. Os limites positivos dos callers observados não dispensam o contrato direto para zero, negativos e valores r5 truncados.

### Wrapper e composição por descritor

`0x1074de10` cria frame de 64 bytes, mantém backchain/spills/LR, chama `0x107393f0` pela ABI existente e devolve seu resultado. Preserve os stores antes da comparação, aliases das strings com o stack e efeitos da chamada, sem restaurar artificialmente clobbers da dependência.

O vector de backgrounds observado em `r2-0x5c38` é `0x108bd664`, com entrada `0x1074de10` e TOC `0x108bba90`. Não há calls estáticos diretos para esse wrapper; ele é usado por transição indireta. A busca de coleções continua selecionando o vector mutável de cada coleção. Não fixe globalmente o comparador nem remova o fixture alternativo que usa `0x1077d9e0` para resultados zero, `-7`, `INT_MIN`, `+19` e `INT_MAX`.

### Comprimento, cópias e concatenação

`0x1077dbd0` recebe a string em r3, lê cada byte até NUL e retorna o comprimento com aritmética word32, mantendo cursor r4 e resíduos/flags originais. Não tem guarda de ponteiro nulo. A leitura do terminador faz parte do contrato.

`0x1077dbf0` recebe destino em r3 e fonte em r4; copia para frente, incluindo NUL, e mantém o destino original como retorno r3. Preserve cada par load/store e os cursores finais r4/r5. `0x1077dc60` recebe os mesmos argumentos, primeiro percorre o destino até NUL e depois copia a fonte para frente, incluindo seu terminador. Não antecipe a leitura da fonte durante esse scan.

`0x1077dc10` recebe destino, fonte e contagem em r3/r4/r5. Copia no máximo essa contagem, incluindo o NUL encontrado; depois preenche o restante com stores individuais de zero, sem continuar lendo a fonte. Quando o limite corta uma fonte longa, não acrescenta NUL. Contagem zero não acessa os bytes dos ponteiros, mas deixa os resíduos de ponteiros/contador/flags do original.

O contador da cópia limitada é word32, com incremento inicial e decremento anterior ao trabalho, ambos com wrap; não é signed16. Preserve também XER carry e CR alterados por `subic.`. Counts enormes ou negativos reinterpretados não autorizam clamp nem bilhões de iterações de teste: use mappings mínimos para faults rápidos e discriminantes.

Cópia e concatenação mantêm o comportamento original de sobreposição. Não use `memmove`, leitura prévia de toda a fonte, bulk clear ou APIs de strings do host. Destino igual à fonte continua produzindo leituras/stores; destino adiante da fonte pode apagar um futuro NUL e caminhar até um fault. Preserve esse prefixo de efeitos em fixtures finitos.

Nos loads/stores com atualização, avance o registrador base somente depois do acesso bem-sucedido, conforme o original. Um fault não pode deixar o cursor antecipado nem apagar efeitos anteriores; mantenha também a posição original das comparações/flags entre a leitura e a escrita.

### Comparação por bytes e buscas

`0x1077dc90` lê os dois bytes unsigned8 antes de testar igualdade/NUL e retorna a diferença exata entre os primeiros bytes distintos, no intervalo `-255..255`. Não normalize para `-1/+1` nem aplique tabela de transformação. `0x1077dcd0` acrescenta limite word32 em r5, com contador r6 incrementado e decrementado com wrap; limite zero não lê nenhum byte. Preserve carry/CR e os caminhos de término, sem importar o signed16 da comparação Mac limitada.

`0x1077dd10` e `0x1077dd50` recebem string em r3 e byte procurado em r4, truncado a unsigned8. Devolvem o ponteiro da primeira ou última ocorrência, respectivamente, ou zero. A busca pelo byte zero pode devolver o terminador. Preserve a ordem de scan e os resíduos arquiteturais; não converta bytes para Unicode.

`0x1077dda0` recebe haystack em r3 e needle em r4. Needle nulo ou vazio devolve o haystack original sem lê-lo, ainda com ajustes de registradores observáveis. Needle presente e não vazio não protege haystack nulo. Preserve o scan dos candidatos, releituras de prefixos, leituras de haystack antes de needle no loop interno e retorno do primeiro candidato cujo needle termina. Não troque por busca que altera fault/access order.

### Duplicação e transformação Mac

`0x107395f0` recebe fonte em r3, cria frame de 64 bytes, chama comprimento `0x1077dbd0`, acrescenta um byte com wrap e chama allocator original `0x10775130`. Se a alocação devolver zero, retorna zero sem copiar; com resultado presente, chama `0x1077dbf0` e retorna o novo ponteiro. Preserve spills/backchain/LR, dependências, clobbers e efeitos de alocação/faults. Não adicione guarda de fonte nula nem implemente o allocator como parte deste goal.

`0x10739640` é independente da duplicação: não é chamada nem alcançada por fallthrough após ela. Carrega uma vez o ponteiro da tabela em `r2-0x618c` e transforma cada byte bruto não NUL in-place. `0xff` é regravado sem consultar a tabela; NUL bruto não é escrito. O conteúdo da tabela é consultado ao longo do scan; uma transformação para zero não interrompe o processamento dos próximos bytes brutos. Retorna r3 avançado até o NUL bruto encontrado, não o ponteiro base. Aliases entre string e tabela podem modificar futuras transformações; não cache os conteúdos.

No TOC original, essa segunda tabela está em `0x1091c878` e possui regras distintas da tabela de comparação. Preserve ambas mutáveis e relocáveis; não deduza uma a partir da outra, não fixe locale/case folding do host e não modifique a inicialização original das tabelas.

## Implementação e integração

Use nomes coerentes, por exemplo `string_core_adapter.*`, `string_core_checks.cpp` e `string_core_audit.py`. Separe operações semânticas de strings dos detalhes de ABI, acessos e fault sites, mantendo ambos explícitos e verificáveis. As folhas têm lógica semântica própria; wrapper e duplicação conservam chamadas guest às dependências.

Preserve Mac32 big-endian, wrap, decisões signed, todos os registradores/temporários observáveis, flags, PC/LR/CTR/TOC, spills/backchains/reloads, ordem de acessos e prefixos de fault nos PCs originais. O observador adversarial de `native/collection_lookup_checks.cpp` injeta mutações na leitura de chave em `0x10739484`; mantenha esse PC e sua ordem para que o fixture continue válido. Não desative o observador para fazer o check passar.

Integre hooks nos labels de destino das páginas `0x10739`, `0x1074d` e `0x1077d`, antes da primeira operação original, cobrindo calls/gotos same-page, dispatch entre páginas e vector indireto. Acrescente as quatorze entradas ao manifesto sem mudar a ordem ou seleção das 46 existentes. Preserve os null guards de clube/competição `0x10713aec` e `0x10713b3c`.

Crie seleção original privada, independente de Date, RNG, backgrounds e consultas, com escolha por entrada para comparar wrapper/comparador e duplicação/comprimento/cópia em modos mistos. Modo normal executa as quatorze implementações semânticas; allocator permanece gerado pela ABI existente. Observadores/counters opcionais são diagnósticos process-local fora de saves/snapshots; preserve os observadores de memória/desenho existentes e não introduza leituras guest só para diagnosticar.

O audit deve rejeitar corpos alterados ou parciais, conferir fingerprints, fronteiras/arestas, hooks e extensão do manifesto a 60. Atualize somente as expectativas de extensão afetadas nos audits de coleções/backgrounds, preservando seus contratos. Respeite os gaps efetivamente indexados; não copie uma asserção de corpo contíguo de outro audit para estas fronteiras.

Ciclo de vida de coleções, inserção/destruição, comparadores específicos de entidades, inicialização das tabelas, allocator, primitivas gerais de memória, classificação de caracteres, parsing numérico, linguagem/templates/formatação, drawing, sampler RNG `0x107124e0`, menus/escala, Restart, gameplay, multiplayer, serialização e otimização geral ficam fora. O serviço grande iniciado em `0x1077de10` também fica fora; o bloco básico dessa região termina em `0x1077de0c`. As tabelas e alocação continuam fornecidas pelo engine original. Preserve a assimetria bissexta Date e os demais módulos semânticos existentes.

## Aceitação focada

Crie um harness compartilhado para as quatorze entradas, por exemplo target `cm-string-core-checks` e CTest `modern_string_core_equivalence`. Original e semântico começam do mesmo estado; os outros módulos permanecem semânticos. Compare arquitetura completa, toda memória mapeada, estado de allocator/imports e acessos ordenados, incluindo prefixos de faults. Contagem de instruções e diagnósticos não fazem parte da igualdade comportamental. Use expectativas independentes de resultado, bytes escritos, ponteiros retornados, ordem de leitura, acessos à tabela e avanço de cursores; não use o adapter novo como oracle.

Cubra uma matriz pequena, justificada e discriminante:

- Ponteiros nulos separadamente e juntos, strings vazias, iguais, prefixos, diferenças iniciais/finais, ASCII com caixa diferente e resultados `-1/0/+1`.
- Bytes altos, `0xff` em cada lado sem lookup, transformação signed8, transformação para zero sem término e tabelas original/mutada que distingam comparação signed de unsigned.
- r5 com limites signed16 zero/negativo/positivo, fronteiras/truncamento relevantes e diferenças antes, no e depois do limite. Verifique a leitura antecipada no índice limite e seus faults.
- Comparadores por bytes com diferenças exatas, limites word32 zero/positivo/wrap e carry/CR, distinguindo-os dos comparadores de tabela.
- Comprimento, cópia incluindo NUL, cópia limitada curta/exata/com padding, concatenação vazia/não vazia, fonte/destino iguais e sobreposições selecionadas. Use mappings finitos para faults rápidos em counts enormes e scans sem terminador; não percorra bilhões de bytes.
- Busca primeira/última/substring com ocorrências repetidas, ausência, byte zero/alto/truncado, prefixos repetidos, match no início/fim e needle vazio/nulo. Confira ausência de leitura de haystack nos casos excepcionais.
- Duplicação vazia/não vazia, tamanho solicitado, conteúdo independente, efeitos reais de alocação e caminho de retorno zero em fixture controlado. Distinga prova da ABI quando houver intervenção de fixture de uma falha real do allocator. Inclua faults do comprimento, allocator e cópia sem reconstruir a memória.
- Transformação in-place com as duas tabelas diferenciadas, tabela mutada, byte `0xff`, transformação para zero, retorno no terminador e faults após stores parciais.
- TOC relocável, slot/tabela/strings parcialmente mapeados ou inválidos, ordem de faults, ponteiros repetidos e aliases selecionados que distingam stores/leituras. Inclua aliases do wrapper com seus spills; evite uma campanha cartesiana.
- Um negativo próprio por entrada, rejeitado na própria fronteira de retorno, com reset do diagnóstico. Não use somente uma mutação global do helper como negativo do wrapper.
- Wrapper original com comparador semântico e o inverso; duplicação original com comprimento/cópia semânticos e o inverso; bloco todo semântico. Exija dispatch não vazio das quatorze entradas no modo normal e zero execuções originais selecionadas nesses witnesses, incluindo transição real por vector e dependências da duplicação, sem exigir execução de todos os callers.
- Composição com consultas e backgrounds semânticos, descriptors reais, tabelas mutáveis, imagens não vazias e consumo RNG idêntico. Preserve o fixture adversarial de mutação do vector/filho/registradores e o comparador alternativo com retorno signed32.
- Acrescente seis pequenos witnesses dos fallbacks Date que chamam diretamente cópia: `0x107184d0`, `0x10718570`, `0x10718700`, `0x107187d0`, `0x107188a0` e `0x107189d0`, mais um ou dois casos normais de formatação que alcancem o núcleo novo. Reuse fixtures mínimos ou snapshot privado já existente, com seleção original apenas das novas strings. Isso valida uma dependência afetada; não rode a matriz inteira de apresentação Date, streams históricos ou nova campanha de carreiras.

Faça uma regeneração quando os hooks estiverem prontos e um build consistente. Mudança de State/layout ou dependência compartilhada exige rebuild de todos os objetos dependentes; não repita `clean-first` por entrada. Rode o novo harness/audit e os quatro CTests diretamente relacionados: `modern_collection_lookup_equivalence`, `modern_background_selection_equivalence`, `original_background_selection` e `original_decimal_formatting`. O último exercita o formatter `0x1077b680`, cujo scanner `0x1077a690` alcança busca de byte/comprimento agora semânticos. Rode os audits existentes de coleções/backgrounds efetivamente afetados. Não acrescente campanha de allocator apenas porque a duplicação o utiliza: seus efeitos já são comparados no harness novo e seu código não mudou. Instrumente somente adapter/harness novos com UBSAN, informando a extensão exata; não alegue whole-engine/ASan. Reexecute apenas grupos afetados por correção ou falha.

Comandos-base a partir da raiz, ajustando apenas o nome efetivo dos arquivos/targets novos:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure \
  -R '^(modern_string_core_equivalence|modern_collection_lookup_equivalence|modern_background_selection_equivalence|original_background_selection|original_decimal_formatting)$'
python3 tools/test_string_core_audit.py
python3 tools/test_collection_lookup_audit.py
python3 tools/test_background_selection_audit.py
```

Inspeção curta do app é opcional se acrescentar evidência útil de uma tela diretamente afetada. Prefira fixtures; não abra carreira pessoal nem use Save/Load para preparar coverage. Não faça contadores vivos por entrada um gate adicional. Declare precisamente se a UI não foi exercitada.

## Proteção e entrega autorizada

Preserve saves, inputs originais, fixtures, referências/candidatos históricos e todos os handoffs. Mantenha fixtures/diagnósticos novos privados e compactos sob `.local`; não copie carreiras nem recrie artefatos removidos por deduplicação. Não limpe `.local` nem faça inventários globais repetidos a cada ajuste.

Documente em inglês em `docs/native-string-core-reconstruction.md` os quatorze contratos/fingerprints, integração, dependências restantes, casos/negativos, dispatch/composições, checks executados e limites. Atualize `docs/native-port-status.md`; preserve a validade histórica dos relatórios anteriores e esclareça no estado atual que as operações básicas de strings e a comparação de descritores agora são semânticas. Diferencie evidência histórica, comparação direta, intervenções de fixtures e eventual UI. Declare que persistência não foi retestada por orientação do usuário.

Depois da implementação e checks focados, instale uma vez com `python3 tools/install_native.py`. Antes de substituir o instalado atual, crie uma nova referência privada para este bloco, preservando as duas referências anteriores. Use as proteções do instalador; confira uma vez antes/depois conteúdo e metadados dos arquivos pessoais potencialmente afetados, preferência de apresentação e handoffs. Verifique ARM64, executável fonte/candidato/instalado e hashes, bundled originals, assinatura aplicável, plist de produção sem diagnósticos e horário efetivo da instalação. Distinga referência anterior, build, candidato assinado e instalado. Não abra carreira pessoal como prova de instalação.

Revise o diff exato, preserve alterações alheias e faça commit/push normal do produto concluído e validado para `git@github.com:VictorDMor/cm0102mac.git`, sob a autorização permanente já dada pelo usuário. A entrega anterior verificou repositório privado e permissão ADMIN; reconfirme destino/permissão quando necessário e o SHA remoto após o push. Exclua os treze handoffs privados, saves, payloads, snapshots, screenshots, logs e fixtures privados. Não altere ignore/LFS para publicá-los nem force o push.

Conclua quando as quatorze entradas executarem semântica C++23 em produção, os contratos/efeitos e composições forem equivalentes, os negativos/dispatch/regressões focados passarem e documentação, instalação protegida e entrega estiverem conferidas. Apresente o bloco como subsistema básico de strings e descritores do Mac, com fronteira explícita; não declare todo o engine ou todos os serviços de texto reconstruídos. A sessão que prepara este handoff somente salva e copia o prompt; implementação, instalação e publicação pertencem à sessão que executar este pedido.
