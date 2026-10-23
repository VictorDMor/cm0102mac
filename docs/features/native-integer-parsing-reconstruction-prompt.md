/goal Reconstrua e entregue o núcleo Mac de parsing de inteiros em C++23 semântico na produção ARM64 do CM 01/02. Implemente conjuntamente as cinco entradas abaixo, preserve seus contratos e dependências, valide o bloco com checks focados e um teste curto de execução do aplicativo, faça uma instalação protegida e entregue com commit/push. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto e objetivo

O produto é o CM 01/02 completo, jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em C++ compreensível e comportamento original Mac preservado. Já funciona por recompilação estática ARM64 e bridges nativos. Date, estado/inicialização RNG, apresentação Retina e comandos diretos, seleção de backgrounds, consultas de coleções, strings/descritores básicos e primitivas de memória já foram entregues.

O próximo bloco é a conversão de texto para inteiros: os parsers de magnitude de 32 e 64 bits usados pelo scanner, a conversão textual com base e end pointer, e seus dois wrappers decimais. Eles ainda usam corpos gerados. Fechar essa cadeia avança a reconstrução dos serviços textuais após strings/memória, sem abrir o formatter inteiro ou o runtime de arquivos. Preserve, por exemplo, a distinção entre consumir um prefixo numérico, informar onde ele termina, sinalizar overflow e aplicar a saturação do wrapper signed32.

Entregue as cinco entradas como um subsistema coerente, com checkpoints internos e uma aceitação compartilhada. Use operações e fases semânticas legíveis, com compatibilidade ABI explícita. Não entregue somente wrappers, investigação ou uma tradução opcode por opcode. O modo normal deve executar os novos corpos; não execute os corpos originais substituídos como fallback normal. Não prometa melhoria visual ou de desempenho.

## Decisões de validação e autorização

- O usuário dispensou campanhas de Save/Load, save/exit/reopen/load, comparação de carreiras e auditoria de serialização. Registre persistência como **não retestada**, sem convertê-la em PASS ou em gate pendente.
- A orientação mais recente acrescentou: testar pelo menos a execução, sem tornar a validação tão completa. Portanto, inclua **um smoke test curto do app efetivamente instalado**, além dos checks focados. Não repita UI por rotina, nem carregue uma carreira para preencher cobertura.
- Faça testes proporcionais: comparação original/semântico do bloco, negativos próprios, dispatch real no harness e regressões diretamente relacionadas. Não rode toda a suíte, campanhas Date/RNG/allocator, temporada, day/week replay ou benchmarks. Amplie somente diante de falha concreta ou dependência afetada, com motivo e stopping condition.
- Implementação, documentação, instalação protegida e commit/push normal de produto concluído e validado estão autorizados neste projeto. Preserve mudanças alheias e dados privados. Não crie uma nova sessão nem implemente outros subsistemas.

## Ponto de partida verificado em 6 de outubro de 2026

- Projeto `/Users/victordmor/Projects/cm0102mac`, branch `main`, HEAD, referência local `origin/main` e consulta nova ao remoto: `2a7eed69ac6f2d3cba704768661dabf788bc6f3a`, `Reconstruct Mac block memory primitives in semantic C++23`. Remoto existente: `git@github.com:VictorDMor/cm0102mac.git`. Reconfira branch, remoto e diff antes de trabalhar.
- Não havia alterações versionadas ou staged. Havia quatorze handoffs privados untracked em `docs/features`, além de cinco prompts versionados; este arquivo acrescenta o décimo quinto privado. Preserve todos, sem incluir os privados no commit de produto. Nenhum `AGENTS.md` aplicável foi encontrado na inspeção do projeto/ancestrais; reconfira as instruções vigentes.
- `tools/compile_recomp.py` e `.local/recompiled/compilation.json` contêm **66 substituições semânticas**: 34 Date, três RNG, quatro backgrounds, cinco consultas de coleções, quatorze strings/descritores e seis primitivas de memória. O manifesto tem 1.953.157 instruções e zero missing/pending static targets. As cinco entradas deste pedido ainda não são semânticas. Resultado esperado: **71 entradas**, com as 66 anteriores preservadas na mesma ordem.
- `docs/native-memory-primitives-reconstruction.md` registra a entrega anterior: 270 comparações, 96 faults, seis negativos próprios mais um negativo de filho, quatro CTests focados, dezenove testes Python e UBSAN focado aprovados. São resultados anteriores; esta preparação não os rerodou e eles não validam o novo bloco.
- O executável instalado em `/Users/victordmor/Applications/CM 01-02 Native Prototype.app/Contents/MacOS/CM 01-02 Native Prototype` foi reconferido com SHA-256 `6c00be6327cbbee65a57078320bfd2b6c530e2d1a7d9d34ba8a001e59b20e6b5`. No smoke test posterior à entrega ele iniciou em ARM64, renderizou as aberturas e foi observado na tela do elenco do Reading após interação do usuário, sem diagnóstico fatal. O usuário confirmou: “Tudo funcionando”. Isso confirma execução básica daquela versão; não é uma campanha de persistência ou saída normal.
- As frases dos relatórios anteriores sobre ausência de UI se referem à entrega técnica antes desse smoke test. Preserve a cronologia ao documentar o novo trabalho.

Este handoff foi preparado somente por inspeção do checkout, documentos, manifesto e índice Mac read-only, mais verificação do remoto e identidade instalada. Não implementou os parsers, não fez build, não executou novos testes de aceitação nem abriu/instalou o app. Reconfira estados que possam mudar; o prompt é autocontido e não depende de recibos temporários ou deste chat.

## Inspeção dirigida

Leia `README.md`, as últimas seções de `docs/native-port-status.md`, `docs/native-memory-primitives-reconstruction.md` e `docs/native-string-core-reconstruction.md`. Inspecione:

- `native/memory_primitives_adapter.*`, `native/memory_primitives_checks.cpp`, `native/string_core_adapter.*` e `native/string_core_checks.cpp`: ABI, clonar State, efeitos ordenados, faults, negativos, seleção por entrada e composições geradas.
- `native/aot_runtime.hpp`, `native/aot_runtime.cpp`, `native/aot_probe.cpp`, `native/CMakeLists.txt` e `tools/compile_recomp.py`: dispatch, memória, diagnósticos privados, hooks de destino e build consistente.
- `tools/memory_primitives_audit.py`, `tools/test_memory_primitives_audit.py` e os audits de strings/coleções/backgrounds: fingerprints, cobertura do manifesto e arestas externas. Os quatro testes de manifesto ainda esperam 66; o slice de memória atual `[60:]` deverá ficar limitado ao bloco antigo, preservando-o.
- `native/unsigned_division.hpp`, `native/unsigned_division_checks.cpp` e `tools/unsigned_division_audit.py`: dependência produtiva `0x10773870`, já otimizada e coberta. Mantenha esse caminho e seu ABI; este pedido não reconstrói divisão.
- `.local/recomp-input/instructions.sqlite`, aberto read-only, `.local/recomp-input/program.json` e páginas privadas geradas pertinentes. Use o export existente; não inicie novo export Ghidra nem publique listings, tabelas, segmentos ou payloads proprietários.
- Dependências/callers gerados: scanner `0x1077bf20`, fachada textual `0x1077da70`, callback de string `0x1077d960` e transição indireta `0x10781320`. Leia o original antes de escolher formatos de teste; nomes como `strtol`, `atoi` ou `scanf` não estabelecem automaticamente o contrato deste Mac.

## Escopo fechado

| Entrada | Fim exclusivo | Instruções indexadas | Calls diretos indexados | Responsabilidade |
| --- | --- | ---: | ---: | --- |
| `0x1077ee20` | `0x1077f180` | 215 | 3 | Parser de magnitude word32 com callback, base, limite e flags de saída |
| `0x1077f180` | `0x1077f564` | 249 | 2 | Parser de magnitude word64 com o mesmo protocolo de entrada |
| `0x1077f570` | `0x1077f664` | 61 | 2 | Conversão de string para signed32, end pointer, saturação e erro |
| `0x1077f670` | `0x1077f698` | 10 | 10 | Wrapper decimal sem end pointer |
| `0x1077f6a0` | `0x1077f6c8` | 10 | 1 | Segunda entrada ABI do wrapper decimal |

Total: **545 instruções indexadas**. Há gaps indexados; não infira counts pelo tamanho do intervalo. Counts de calls são relevância estática, incluem relações internas e não são prova de runtime. O scanner tem calls para os dois parsers em `0x1077c224`/`0x1077c254` e `0x1077c700`/`0x1077c730`; o wrapper decimal `0x1077f6a0` tem caller em `0x10205240`. Não é necessário executar todos os callers.

Fingerprints SHA-256 dos registros `address/assembly/ops/flows/fallthrough`, ordenados por endereço e serializados como JSON compacto com chaves ordenadas e UTF-8:

| Entrada | SHA-256 |
| --- | --- |
| `0x1077ee20` | `0e6f94e1876cc0214da11fe6867d125dc06d073581ecc399217f3bd399394098` |
| `0x1077f180` | `b99ed550fab592d2395d29a82f8631d3e08c0c3fa71574c256661d41ba1e8e63` |
| `0x1077f570` | `0221610ecf6b5d56509dc7fae6948e5b117b2bd895c9bd3d8e413459597f32a7` |
| `0x1077f670` | `2e45e76b5215652dce79a0ecc6023e610fef5053cb06f9800fd9424152783eb1` |
| `0x1077f6a0` | `6a14b3d0e2dd4d19b0c5a4af9fd370a2e6b9436c4534fe0bff445a3168e609e9` |

A inspeção de todos os flows e fallthroughs indexados encontrou zero arestas estáticas externas para interiores dessas cinco entradas. Reconfirme no audit; isso não classifica destinos dinâmicos desconhecidos. **O parser word32 atravessa as páginas `0x1077e` e `0x1077f`.** A pinagem deve cobrir seu corpo completo, sem aceitar somente a primeira página ou rejeitar uma página válida por comparar seu fragmento com o hash integral.

### Parsers de magnitude de 32 e 64 bits

As entradas usam r3 para base, r4 para limite, r5 para descriptor do callback, r6 para contexto, r7 para saída de quantidade consumida, r8 para flag de sinal e r9 para flag de overflow. Reconfirme as reduções e comparações signed/unsigned. O primeiro retorna magnitude em r3; o segundo em r3:r4, high:low. Aplicar sinal/saturação no núcleo indiscriminadamente mudaria o contrato.

Reconstrua fases de espaços iniciais, sinal, prefixo/base automática, dígitos, término e devolução do último caractere. Preserve bases aceitas e rejeitadas, base zero, octal/hexadecimal, limites de campo, quantidade consumida, EOF, lookahead, flags inicializadas e acúmulo modulo 2^32/2^64 mesmo após overflow. Não substitua por `std::strtol`, `std::from_chars`, tabelas Unicode ou locale do host.

Os parsers capturam ponteiros de tabelas de classificação e mapeamento por TOC (`r2-0x5a48` e `r2-0x62bc`). Os conteúdos são memória guest, e acessos/releituras/faults importam. O dispatch de fases usa tabelas via `r2-0x3b24` e `r2-0x3b28`, observadas nos payloads em `0x1091ce48` e `0x1091ce8c`. Caracterize seus destinos e preserve loads, CTR, ordem de efeitos e semântica dos destinos; não descarte as leituras simplesmente por escrever um switch legível. Teste alteração discriminante de tabelas e TOC relocada. Trate alvos fora do contrato identificado de forma compatível com o runtime existente, explicitando o limite, sem esconder execução do corpo antigo como cobertura semântica normal.

Ambos chamam o descriptor mutável pelo stub gerado `0x10781320`: ele lê entry/TOC de r12, salva TOC no frame e transfere por CTR. Preserve esse ABI, releitura do descriptor, restauração de TOC, clobbers, eventos do callback e faults. Não especialize o núcleo para um `char*` contínuo: ele também é usado pelo scanner. O término chama o callback em modo de devolução (`r5=1`); casos inválidos não autorizam um early return que elimine efeitos originais. O parser de 64 bits usa ainda `0x10773870` para cálculo do limiar de overflow; preserve o caminho existente.

### Wrapper signed32 e entradas decimais

`0x1077f570` recebe texto/end pointer/base em r3/r4/r5. Prepara um contexto de string no frame, obtém o descriptor por `r2-0x5a44` e chama o parser word32 com limite `0x7fffffff`. O descriptor observado é `0x108bdbac`, contendo entry `0x1077d960` e TOC `0x108bba90`; esses valores são evidência do payload, não constantes para substituir leituras mutáveis.

Preserve a atualização opcional do end pointer, inclusive sem conversão; a ordem entre essa escrita, leitura das flags, saturação e atualização do erro; limites `INT32_MIN`/`INT32_MAX`; negação word32 e overflow. O original escreve `0x22` no destino obtido por `r2-0x5a58` quando detecta range error. Não limpe esse estado em sucesso nem introduza erros/guards que o original não possui. Ponteiros, outputs e spills podem aliasar; um acesso inválido deve falhar no ponto correspondente após os efeitos já realizados.

`0x1077f670` e `0x1077f6a0` selecionam base 10/end pointer nulo e chamam a entrada anterior. Preserve ambos os endereços, seus frames/LR e fronteiras de retorno próprias, mesmo que compartilhem uma operação semântica.

### Integração e limites

Adicione hooks nos labels de destino antes da primeira operação original, preservando dispatch local, entre páginas e indireto. Acrescente cinco entradas ao manifesto sem reordenar as 66 anteriores. A seleção original global/per-entry deste bloco deve ser privada, independente de Date/RNG/backgrounds/coleções/strings/memória; produção usa todas as cinco semânticas. Diagnósticos e observadores são process-local, não serializados e sem acessos guest extras.

Mantenha gerados o scanner inteiro, o callback `0x1077d960`, suas entradas internas como `0x1077d9e0`, a fachada `0x1077da70`, stubs e demais serviços. Não reconstrua parsing floating `0x1077de10`, formatter de saída, locale/tabelas, stream/arquivo, divisão, allocator, gameplay, serialização ou UI. Preserve a assimetria bissexta Date, os guards club/competition e todas as referências históricas.

## Aceitação focada

Crie um harness dedicado, por exemplo `cm-integer-parsing-checks` / `modern_integer_parsing_equivalence`, e audit correspondente. Compare original e semântico a partir de State idêntico, com os módulos anteriores ainda semânticos. Compare resultados esperados independentes, todos os bytes arquiteturais de registradores/temporários, PC/LR/CTR/TOC/CR/XER, memória e efeitos ordenados, callbacks/imports e prefixos de faults. Exclua somente contabilização de instruções e diagnósticos privados; não normalize diferenças para passar.

Cubra uma matriz pequena e discriminante:

- Base 0, 2, 8, 10, 16 e 36; base inválida; vazio, espaços, sinais isolados, sinal seguido de dígitos, prefixos completos/incompletos, caracteres fora da base, maiúsculas/minúsculas, bytes altos e EOF.
- Limites zero/negativo, um, truncamento antes/depois de sinal/prefixo/dígito, além dos limites signed32, unsigned32 e unsigned64 e casos de overflow. Use inputs pequenos e EOF/fault para limites extremos, sem loops enormes.
- Quantidade consumida, end pointer presente/nulo, estado de erro previamente não zero, sinal/overflow separados, wrap da magnitude, callback read/unread e diferenças entre os contratos de núcleo e wrapper.
- TOC/descriptor/tabelas mutáveis, callback alternativo controlado, clobbers observáveis, outputs e spills aliasados, mapeamentos mínimos e faults representativos no frame, tabela, descriptor, input e outputs.
- Cinco negativos próprios de fronteira, com filho/parent distinguidos; dispatch normal não vazio por entrada com zero seleção original dessas entradas; modos mistos wrapper/parser e reset dos diagnósticos.
- Pelo menos uma transição indireta gerada e duas composições do scanner gerado: uma alcançando parser32 e outra parser64. A fachada `0x1077da70` oferece input/format/argumentos guest; confirme o protocolo e formatos contra o original antes de fixar expectativas. Verifique resultado, cursor/count e escrita no destino. Isso é integração offline, não prova de UI ou persistência.

Use budgets limitados para casos adversos. Pins/audits devem rejeitar corpo alterado/parcial, hook ausente, entrada duplicada e arestas externas novas, incluindo a fronteira de página do parser32.

Depois de estabilizar o código, regenere uma vez e faça um build Release ARM64 consistente, especialmente se State mudar. Os comandos usuais são:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure -R '^(modern_integer_parsing_equivalence|modern_string_core_equivalence|modern_memory_primitives_equivalence|original_unsigned_division)$'
python3 tools/test_integer_parsing_audit.py
python3 tools/test_memory_primitives_audit.py
python3 tools/test_string_core_audit.py
python3 tools/test_collection_lookup_audit.py
python3 tools/test_background_selection_audit.py
```

Os nomes novos acima são propostos; use os nomes efetivamente implementados. Os audits antigos entram porque seus asserts de manifesto são afetados, não para reabrir seus subsistemas. Rode UBSAN focado somente no novo adapter/harness ligado às bibliotecas consistentes. Não instrumente o engine inteiro. Repita geração/build/checks apenas por mudança ou falha que os justifique; não imponha uma única tentativa quando um reparo for necessário.

## Instalação e smoke test curto

Antes de atualizar, preserve o instalado atual em uma nova referência privada, por exemplo `.local/integer-parsing-reconstruction/reference/installed.app`, sem sobrescrever referências anteriores. Preserve candidatos/referências de memória, strings, coleções, backgrounds e demais entregas, saves, preferências, metadados, payloads, snapshot Git LFS e handoffs. Não execute o instalador enquanto o usuário estiver usando o app nem encerre sua sessão à força.

Após aceitação técnica, use `tools/install_native.py` uma vez para a atualização final, com a proteção habitual. Verifique arquitetura, executável interno por SHA-256, payloads, assinatura e plist de produção, distinguindo build/candidato/instalado/referência. Mantenha diagnósticos e fixtures privados fora do bundle de produção. Não use o timestamp da pasta `.app` como prova de identidade.

Abra o **app instalado** e observe a abertura, o menu inicial renderizado e uma resposta simples de UI. Prefira comando/menu nomeado; se necessário, abra e cancele um diálogo sem carregar uma carreira. Encerre normalmente quando a sessão for exclusivamente de teste. Pare após essa evidência. Se o usuário assumir a interação, atualize o estado observado e deixe sua sessão intacta; registre essa circunstância sem atribuir suas ações ao teste automatizado. Verifique processo/log para eventual falha e anote o hash testado. Um bloqueio concreto requer investigação focada, não uma campanha geral.

Não exija que essa abertura exercite os cinco parsers: a cobertura individual vem do harness. Não teste Save/Load, avanço de dias ou partida. A execução intencional pode criar logs/preferências; não afirme que todo Runtime/Runs ficou byte-idêntico após o smoke test.

## Entrega

Documente em inglês os contratos, fingerprints, integração, resultados realmente executados, limites e smoke test em `docs/native-integer-parsing-reconstruction.md`, com atualização de `docs/native-port-status.md`. Separe evidência anterior, checks offline e observação do app; registre persistência como não retestada e não declare reconstruído todo o sistema de texto.

Revise o diff exato e publique somente código, checks e documentação de produto no remoto privado/branch corretos, com commit/push normal e verificação nova do SHA remoto. Exclua prompts privados, recibos, logs, imagens, saves, índices, payloads e bundles; preserve `.gitignore`, regras LFS e alterações alheias. Não force push.

Conclua com as cinco entradas entregues, os resultados focados, a observação do app instalado, o commit publicado e qualquer limitação material. O goal termina com implementação produtiva e essa aceitação proporcional, sem etapas futuras obrigatórias inventadas.
