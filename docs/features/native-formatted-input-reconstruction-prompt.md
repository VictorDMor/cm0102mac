/goal Reconstrua e entregue o scanner Mac de entrada formatada em C++23 semântico na produção ARM64 do CM 01/02. Implemente conjuntamente as sete entradas abaixo: analisador de formato, núcleo com callbacks, callbacks de string e stream, duas fachadas textuais e a fachada variádica de stream. Preserve os contratos e as dependências existentes, valide com checks focados e um teste curto do aplicativo instalado, faça uma instalação protegida e entregue com commit/push. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto e objetivo

O produto é o CM 01/02 completo, jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em C++ compreensível e comportamento original Mac preservado. O jogo já funciona por recompilação estática ARM64 e bridges nativos. Date, RNG, seleção de backgrounds, consultas de coleções, strings/descritores básicos, primitivas de memória e parsing de inteiros têm implementações semânticas integradas. A apresentação Retina e os comandos diretos também foram entregues.

Feche agora a cadeia de entrada formatada de strings e streams. O scanner ainda é gerado, mas já usa os parsers semânticos de magnitude de 32/64 bits. A fachada variádica de string tem treze chamadas diretas indexadas em outras partes do engine; a de stream tem três. Esse bloco avança a recuperação dos serviços textuais usados pelo jogo, mantendo valores convertidos, quantidade de atribuições, cursor de entrada e comportamento de falha. Um exemplo já verificado na entrega anterior é `-123!` com `%d%n`: uma atribuição, destino signed32 igual a -123, `%n` igual a 4 e cursor no `!`. Os dois adaptadores de stream acrescentam somente 64 instruções indexadas e reutilizam o mesmo scanner, sem reconstruir serviços de arquivo.

Entregue o bloco inteiro, com semântica legível e integração produtiva. Não encerre com inventário, pseudocódigo, wrappers que ainda executam seus próprios corpos antigos ou um subset de formatos apresentado como scanner completo. Os parsers de ponto flutuante, a conversão de caracteres e os serviços de orientação/leitura/devolução/refill de streams continuam como dependências existentes; reconstruí-los não faz parte deste pedido.

## Limites de execução e decisões preservadas

O usuário quer um goal por subsistema e validação proporcional. Preserve o comportamento original Mac, inclusive a assimetria de leap da diferença de datas com anos ascendentes, cuja correção está adiada. Não expanda este trabalho para gameplay, novos formatos, Unicode/locale do host, apresentação, arquivos ou uma reescrita geral do runtime.

**Save/Load, salvar/sair/reabrir/carregar, campanhas de carreira, auditoria de serialização, avanços de dias/semanas/temporadas, partida, benchmarks e full suite estão fora da aceitação deste goal. Registre persistência como `not retested`; ela não é PASS nem uma etapa futura obrigatória.** Use o harness para cobertura individual e um smoke test curto para comprovar execução do app instalado. Amplie a investigação apenas diante de uma dependência ou falha concreta.

## Ponto de partida verificado

Na preparação deste handoff, em 6 de outubro de 2026:

- Branch `main`, HEAD e uma consulta nova de `origin/main` iguais a `c4a806699e2153002b041825bccb3f41afcbbdca`. Remoto: `git@github.com:VictorDMor/cm0102mac.git`. O tracked worktree está limpo. Há quinze prompts privados untracked anteriores, além dos cinco handoffs tracked e deste novo arquivo; preserve-os.
- Não foi encontrado `AGENTS.md` no repositório nem nos ancestrais inspecionados. Reconfira instruções e estado Git ao começar; não faça reset/clean de mudanças alheias.
- `.local/recompiled/compilation.json` contém 1.953.157 instruções, 1.922 páginas, zero missing/pending static targets e **71 substituições semânticas**. As sete entradas deste pedido ainda têm corpos gerados. O resultado esperado é **78 entradas**, preservando as 71 anteriores na mesma ordem. O SHA-256 do array JSON compacto `semanticReplacements`, com chaves ordenadas, é `1862099e2688776c5b9676d9668069638c739d79ca393382497c86fe45c3f979`.
- A entrega anterior, documentada em `docs/native-integer-parsing-reconstruction.md`, passou 435 comparações original/semântico, incluindo 222 faults, cinco negativos próprios e um de filho, cinco entradas não vazias, dois modos mistos, cinco transições indiretas, composições `%d%n` e `%llu%n`, quatro CTests focados, 24 testes Python e UBSAN focado. Esses são resultados anteriores, não foram rerodados nesta preparação e não validam o novo scanner.
- O executável instalado em `/Users/victordmor/Applications/CM 01-02 Native Prototype.app/Contents/MacOS/CM 01-02 Native Prototype` foi reconferido como Mach-O ARM64, SHA-256 `191e7c599671efb7f0f1bed0273020693c86eddc59a5d40349f0a79e28d845b4`. A entrega anterior observou o menu inicial, abertura/cancelamento do chooser de Load sem carregar uma carreira e saída normal. Os cinco parsers não foram chamados nessa abertura: a cobertura individual veio do harness. Persistência não foi retestada.

Este handoff foi preparado somente por inspeção de Git, documentos, código/checks, manifesto e índice Mac read-only, consulta do remoto e identidade instalada. Não implementou o scanner, não gerou/buildou/testou um novo candidato nem abriu/instalou o app. Reconfira estados que possam mudar. Não dependa deste chat ou de recibos temporários para executar o objetivo.

## Inspeção dirigida

Leia `README.md`, as últimas seções de `docs/native-port-status.md`, `docs/native-integer-parsing-reconstruction.md` e `docs/native-string-core-reconstruction.md`. Inspecione:

- `native/integer_parsing_adapter.*` e `native/integer_parsing_checks.cpp`: ABI, efeitos ordenados, callbacks mutáveis, faults, negativos próprios/de filho, modos mistos e as duas composições existentes do scanner. Preserve os cinco contratos inteiros e sua seleção independente.
- `native/string_core_adapter.*`, `native/string_core_checks.cpp`, `native/memory_primitives_adapter.*` e seus checks: dependências semânticas, observadores existentes e padrões de integração. O helper de comprimento `0x1077dbd0`, usado pelo analisador de formato, já é semântico.
- `native/aot_runtime.hpp`, `native/aot_runtime.cpp`, `native/aot_probe.cpp`, `native/CMakeLists.txt` e `tools/compile_recomp.py`: State, dispatch, `run_until`, TOC/LR, memória guest, hooks nas labels de destino, manifesto e build consistente.
- `tools/integer_parsing_audit.py` e `tools/test_integer_parsing_audit.py`: fingerprint do corpo inteiro, fragmentos por página, rejeição de arestas externas para interiores e hooks. Os audits de inteiros/memória/strings/coleções/backgrounds têm asserts do tamanho 71; altere apenas o necessário para 78 e preserve os slices antigos, inclusive inteiros `[66:71]`.
- `.local/recomp-input/instructions.sqlite`, aberto read-only, `.local/recomp-input/program.json` e páginas privadas geradas `0x1077b`, `0x1077c`, `0x1077d`, além das dependências pertinentes. Use o export existente; não inicie novo export Ghidra nem publique listings, tabelas ou segmentos proprietários.
- Dependências existentes: parsers `0x1077ee20` / `0x1077f180`, stub de descriptor `0x10781320`, parser de ponto flutuante `0x1077de10`, helper de caracteres `0x10778170`, orientação de stream `0x1077fa50`, leitura `0x10775c10`, devolução `0x10775d80` e refill `0x10775ad0`. Preserve suas chamadas/ABI; nenhum desses corpos entra como nova substituição neste goal.

## Escopo fechado e pinagem

| Entrada | Fim exclusivo | Instruções indexadas | Responsabilidade |
| --- | --- | ---: | --- |
| `0x1077b7a0` | `0x1077bf20` | 478 | Analisar uma especificação de conversão e produzir o record de formato |
| `0x1077bf20` | `0x1077d8cc` | 1.631 | Scanner geral de entrada formatada via descriptor/contexto/argumentos guest |
| `0x1077d960` | `0x1077d9f0` | 36 | Callback de string: leitura, devolução e consulta de EOF |
| `0x1077da70` | `0x1077dac8` | 22 | Fachada de string com vetor de argumentos guest |
| `0x1077dad0` | `0x1077db14` | 17 | Fachada variádica de string, spills r4–r10 e encadeamento |
| `0x1077d8d0` | `0x1077d954` | 33 | Callback de stream: leitura, devolução e consulta de EOF/error |
| `0x1077d9f0` | `0x1077da6c` | 31 | Fachada variádica de stream, orientação e chamada do scanner |

Total: **2.248 instruções indexadas**. Existem gaps; não infira counts pelo tamanho dos intervalos. O scanner atravessa três páginas: 55 instruções em `0x1077b`, 1.020 em `0x1077c` e 556 em `0x1077d`. O analisador de formato está na página `0x1077b`; os cinco outros corpos estão em `0x1077d`.

Fingerprints SHA-256 dos registros `address/assembly/ops/flows/fallthrough`, ordenados por endereço e serializados em JSON compacto, com chaves ordenadas e UTF-8:

| Entrada | SHA-256 |
| --- | --- |
| `0x1077b7a0` | `1f92666f58a9485bc5f015c13c3c3942fc756a123d6270a52717c6866e025c9a` |
| `0x1077bf20` | `a8d9ca1f82d1dae1139544ce0bf135f361b2f6eae0cbef423c3d6e7fc5cb5108` |
| `0x1077d960` | `9d8bfe52bc79d144184931008bacb7767f6d95c36ebe2a3b92a70c23139e7967` |
| `0x1077da70` | `d247cbccf4af1cd8df61149e1c50d024a4186777f0504193a0d9e7ee07980e12` |
| `0x1077dad0` | `7db0c98c7415dc19f6e9a6df73e14b24517a032d2c9125b1aa833485934ea585` |
| `0x1077d8d0` | `862b4f73bb03b4db4aae69453b16d81232966f15819433dc28b397eb5c5d5d19` |
| `0x1077d9f0` | `b840d438996ce92f30b4dda4c874385828932d89549225139ed261692923c91b` |

A consulta de todos os flows/fallthroughs indexados encontrou zero arestas estáticas externas para interiores dessas sete entradas. Reconfirme no audit; isso não classifica destinos dinâmicos. As chamadas diretas externas indexadas são 1/2/0/1/13/0/3, na ordem da tabela. Os callbacks têm referência indireta por descriptor; zero calls diretos não significa ausência de uso. Exemplos de callers da fachada variádica de string: `0x102d6a50`, `0x103343b8` e `0x106a905c`. A fachada de stream é chamada em `0x10144350`, `0x10144b68` e `0x10144d48`. Esses counts são relevância estática, não prova de runtime; não é necessário executar todos os callers ou identificar suas jornadas de UI.

## Contratos a reconstruir

### Analisador de formato

O ABI observado recebe formato/ponteiro para cursor de argumentos/record de saída em r3/r4/r5 e retorna o próximo endereço do formato em r3. Reconfirme as reduções signed/unsigned e todos os campos. O record tem 60 bytes e parte de um template guest obtido por `r2-0x3b40`. Preserve loads/stores por campo, spills, aliasing e a ordem em que o cursor de argumentos pode avançar.

Reconstrua `%%`, supressão de atribuição, largura, modificadores, códigos de conversão, scansets com inversão/ranges/`]` inicial, formatos inválidos/truncados e as extensões que o corpo Mac realmente aceita. Há caminhos para `v` e `@`; caracterize-os antes de nomear sua semântica. Não silencie esses caminhos por serem ausentes do `scanf` do host. O caminho `@` consome um argumento, chama o comprimento semântico e manipula espaço/pointer/count do record; preserve seus efeitos e limites.

Classificação usa a tabela guest via `r2-0x5a48`. O dispatch de códigos de conversão lê uma tabela guest via `r2-0x3b44`, carrega destino e escreve CTR. Identifique seus destinos e selecione operações semânticas pelo destino vivo, preservando leituras, CR/CTR, mutações e TOC relocada. Caracterize a fronteira de destinos fora do conjunto identificado e mantenha tratamento compatível com o runtime existente, com diagnóstico privado separado. Não conte execução do corpo antigo como cobertura semântica normal.

### Scanner

`0x1077bf20` recebe descriptor de callback/contexto/formato/vetor de argumentos em r3/r4/r5/r6. Seu frame tem 240 bytes e salva r13–r31. O analisador é chamado com cursor de argumentos no frame e record de 60 bytes. Preserve LR/TOC, flags, temporários observáveis e acessos guest; esse ABI precisa funcionar para ambas as fachadas.

Implemente operações nomeadas para espaços/literais, análise da conversão, entrada numérica, caracteres, strings/scansets, `%n`, término e atribuições. Preserve lookahead/devolução, EOF consultado pelo callback, quantidade consumida, avanço do vetor, supressão, largura, armazenamento big-endian e narrowing signed/unsigned. Diferencie assignment count do contador que governa EOF/no-conversion. Preserve as variantes e extensões efetivamente presentes no original, incluindo suas escritas repetidas/empacotadas quando aplicáveis.

O núcleo distingue conversões inteiras, `c`, `s`, `[`, `%`, `n` e conversões de ponto flutuante. Os calls para magnitude64/magnitude32 estão em `0x1077c224`/`0x1077c254` e `0x1077c700`/`0x1077c730`; mantenha os adapters semânticos anteriores. Preserve aplicação do sinal, overflow e tamanhos de destino exatamente como o scanner faz, sem importar a saturação do wrapper signed32 textual. As conversões de ponto flutuante devem chamar o parser existente e preservar suas stores float/double e estado de ponto flutuante. Caminhos de caracteres que chamam `0x10778170` mantêm esse helper.

Todos os callbacks usam o descriptor mutável pelo stub `0x10781320`, com modos observados 0 para read, 1 para unread e 2 para consulta de EOF. Preserve entry/TOC releídos, spills/restauração de TOC, clobbers, argumentos, eventos, efeitos e faults. O scanner geral não pode ser especializado para uma string host ou um callback único. Não substitua por `std::sscanf`, `std::from_chars`, tabelas Unicode ou locale do host.

### Callback e fachadas de string

O contexto do callback contém cursor word32 em offset 0 e estado EOF word32 em offset 4. Em modo read, NUL marca EOF e retorna -1; byte não nulo avança o cursor e retorna o byte. Unread pode decrementar o cursor ou limpar EOF, preservando o retorno r4. Modo 2 retorna o estado EOF; outros modos têm o resultado original. Preserve signed comparisons, bytes altos, wrap, aliasing, CR e faults, inclusive a inexistência de um guard host adicional.

A fachada `0x1077da70` recebe input/format/argumentos guest em r3/r4/r5, prepara o contexto no frame e lê o descriptor por `r2-0x5a44`. Input nulo ou primeiro byte NUL retorna -1 antes de chamar o scanner, com seus próprios efeitos de frame. A fachada `0x1077dad0` recebe input/format e argumentos em registers/stack, faz spills r4–r10 e chama a anterior. Preserve ambas as fronteiras próprias, frames de 64 bytes, ordem dos spills e varargs Mac32; não converta ponteiros guest em ponteiros host.

### Callback e fachada de stream

`0x1077d8d0` recebe stream/caractere/modo em r3/r4/r5 e tem frame próprio de 64 bytes. Modo 0 chama a leitura gerada `0x10775c10`; modo 1 passa caractere/stream em r3/r4 para `0x10775d80`. Modo 2 consulta os bytes guest nos offsets 13 e, quando necessário, 12 do stream, retornando 1 quando qualquer flag consultada estiver ativa. Preserve o short-circuit entre esses loads, ABI, retorno, outros modos, ordem de efeitos e faults. Não substitua os calls por operações host nem reconstrua os helpers de stream.

`0x1077d9f0` recebe stream/format e argumentos variádicos em registers/stack. Salva r31/LR e faz spills r4–r10 em seu frame de 64 bytes, inclusive no caminho de stream nulo. Para stream não nulo chama `0x1077fa50` com r4 igual a -1; se o retorno não for negativo, retorna -1 sem entrar no scanner. No caminho aceito, lê o descriptor por `r2-0x5a40` e chama o scanner com stream como contexto, formato salvo e vetor de argumentos no frame. Preserve a mutabilidade do descriptor, a orientação e seus efeitos, CR/TOC/LR, spills e fronteira própria.

Valide essas duas entradas offline com um stream guest artificial, buffer e flags controlados, mantendo os helpers gerados. A leitura buffered observada usa cursor em offset 40 e quantidade em offset 44; reconfirme também os campos exigidos pela orientação e devolução. Escolha input/format e espaço de lookahead que permaneçam no buffer; não provoque refill/OS para criar cobertura. Teste EOF/error diretamente pelo modo 2, ambos os caminhos da orientação e stream nulo. Não leia arquivos pessoais, não abra/destrua streams reais e não transforme essa adição em implementação do runtime de arquivos.

## Implementação produtiva e checks focados

Crie um adapter C++23 legível, harness e audit específicos deste bloco, seguindo os padrões existentes. Hooks devem estar nas labels de destino **antes da primeira operação original** em `0x1077b` e `0x1077d`, alcançando goto local, dispatch de página e transferência indireta. Mantenha as 71 substituições anteriores, fixups e caminhos de dependência. Preserve a pinagem inteira do scanner antes da emissão do manifesto e pins completos de cada fragmento; não compare um fragmento com o hash integral nem aceite somente a primeira página como corpo completo.

Seleção original global/por entrada, counters, observadores de fronteira e eventuais escapes devem ser privados, process-local, independentes dos módulos anteriores e não serializados. O default produtivo deve selecionar todas as sete entradas semânticas. Helpers de ABI são aceitáveis; um switch por endereço original/opcode ou replay P-code não é reconstrução semântica.

Valide original versus semântico desde o mesmo State, com dependências anteriores mantidas semânticas. Compare arquitetura completa, memória, ordem de efeitos, callback arguments/events, PC/LR/CTR/TOC/CR/XER e prefixos anteriores a faults, excluindo apenas accounting e diagnósticos privados. Não normalize diferenças de ABI para fazer os checks passarem. Use expectativas independentes de valores, record/cursor de formato e argumentos, count/EOF, destinos e guard bytes.

A matriz deve ser pequena e discriminante: espaços/literais/`%%`; sucesso e mismatch; input vazio/nulo; EOF antes/depois de atribuição; supressão/limites; bases e overflow através dos parsers existentes; os tamanhos de destino realmente suportados; `%n`; `c`/`s`/scansets; extensões encontradas; um conjunto curto de conversões de ponto flutuante e do helper de caracteres. Exercite todos os caminhos semânticos relevantes do bloco, sem campanhas combinatórias desnecessárias.

Inclua mutações de template/classificação/tabela de destinos/descriptor, TOC relocada, callbacks alternativos e clobbers, aliases representativos e mappings mínimos com remoção de bytes acessados. Preserve efeitos até a falha original. Teste as sete fronteiras com negativos próprios e pelo menos um negativo de filho detectado antes do parent. Exija chamadas normais não vazias por entrada, zero seleção original dessas entradas e zero escape normal. Verifique reset e alguns modos mistos discriminantes: facade/scanner, scanner/analisador, scanner/parsers e parser/callback, incluindo um modo stream, sem um produto cartesiano.

Exercite dispatch gerado indireto e composições pelas fachadas variádicas reais de string e stream, verificando destino/count/cursor e buffer/flags quando aplicável, além das duas composições `%d%n` / `%llu%n` já existentes. Um fixture de callback genérico cobre ainda entrada independente dessas fachadas. Toda essa aceitação é offline; não abra arquivos pessoais ou redes para cobrir os adaptadores de stream.

Após estabilizar o código, regenere uma vez e faça um build Release ARM64 consistente, especialmente se State mudar. Comandos usuais:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure -R '^(modern_formatted_input_equivalence|modern_integer_parsing_equivalence|modern_string_core_equivalence)$'
python3 tools/test_formatted_input_audit.py
python3 tools/test_integer_parsing_audit.py
python3 tools/test_memory_primitives_audit.py
python3 tools/test_string_core_audit.py
python3 tools/test_collection_lookup_audit.py
python3 tools/test_background_selection_audit.py
```

Os dois nomes novos são propostos; use os efetivamente implementados. Audits antigos entram porque seus asserts de manifesto são afetados; os CTests de inteiros/strings cobrem dependências e consumidores diretamente afetados. Acrescente outro check somente por dependência/falha concreta. Execute UBSAN apenas no novo adapter/harness ligado às bibliotecas consistentes. Use budgets limitados em casos adversos. Repita geração/build/checks somente por mudança ou falha que justifique; repare o necessário sem repetir grupos verdes por ritual.

## Instalação e teste curto do app

Preserve o instalado atual com forks/xattrs/metadados em uma nova referência privada, por exemplo `.local/formatted-input-reconstruction/reference/installed.app`, sem sobrescrever referências anteriores. Preserve todos os candidatos/referências de inteiros, memória, strings, coleções, backgrounds e demais entregas, saves, preferências, payloads, snapshot Git LFS e handoffs. Não execute o instalador durante uma sessão do usuário nem a encerre à força.

Após aceitação técnica, use `tools/install_native.py` uma vez para a atualização final, com a proteção habitual. Distinga output do linker, candidato assinado, instalado e referência pelo executável interno/arquitetura/hash; confira payloads originais, assinatura e plist de produção. Não inclua fixtures, diagnósticos privados ou configuração de validação no bundle. Instalação não é provada pelo timestamp da pasta `.app`.

Abra o **app instalado**, observe abertura/menu inicial renderizado e uma resposta simples de UI, preferindo comando/menu nomeado. Se necessário, abra e cancele um diálogo sem carregar uma carreira. Encerre normalmente quando for exclusivamente uma sessão de teste; preserve a sessão se o usuário assumir a interação. Confira processo/log e o hash testado. Pare depois dessa evidência, sem exigir que a abertura chame as sete entradas: a cobertura individual vem do harness. Não teste Save/Load, dias ou partida. Logs/preferências podem mudar pelo smoke; não declare Runtime/Runs byte-idênticos após a execução intencional.

## Entrega

Documente em inglês contratos, fingerprints, integração, cobertura realmente executada, limites de dispatch/dependências, identidade instalada e observação do app em `docs/native-formatted-input-reconstruction.md`, atualizando `docs/native-port-status.md`. Distinga resultados históricos, equivalência offline e smoke test. Persistência permanece `not retested`. Não declare reconstruídos o formatter de saída, o parser de ponto flutuante, locale ou o runtime de streams.

Revise o diff exato e publique somente código, checks e documentação de produto no remoto privado/branch corretos com commit/push normal, seguido de consulta nova do SHA remoto. Preserve `.gitignore`, regras LFS e mudanças alheias; exclua prompts privados, recibos, logs, imagens, saves, índices, payloads, fontes geradas e bundles. Não force push.

Conclua com as sete entradas entregues, os resultados focados, a evidência do app instalado, o commit publicado e limitações materiais. O goal termina com implementação produtiva e essa aceitação proporcional, sem gates adicionais inventados.
