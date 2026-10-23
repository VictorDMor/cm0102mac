/goal Reconstrua e entregue o bloco Mac de serviços textuais e numéricos de I/O formatado em C++23 semântico na produção ARM64 do CM 01/02: formatação de saída, conversão de caracteres e parsing floating de entrada com seus dois bridges faltantes. Implemente conjuntamente as vinte e duas entradas abaixo e os serviços `CarbonLib::dec2num`/`CarbonLib::nan`, preserve seus contratos e dependências, valide com checks focados, faça uma instalação protegida e um smoke test curto do aplicativo instalado, documente e entregue com commit/push normal. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto e objetivo

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon, com o engine reconstruído progressivamente em C++ compreensível e comportamento original Mac preservado. O jogo já funciona por recompilação estática ARM64 e bridges nativos. Date, RNG core, backgrounds, consultas de coleções, strings/descritores, primitivas de memória, parsing de inteiros e entrada formatada já possuem blocos semânticos. Apresentação Retina e comandos nomeados também estão entregues.

O próximo incremento fecha um bloco maior e coerente de I/O textual/numérico usado extensivamente pelo jogo, inclusive nos textos e números da interface. Inclua análise do formato de saída, dígitos/rounding, conversões inteiras e floating, formatter por descriptor, callbacks/fachadas, a cadeia bidirecional de caracteres e o parser floating que o scanner já chama. Por exemplo, a fachada bounded produz `7.25` para `%.2f`, respeita padding e retorna comprimento calculado mesmo quando o destino comporta menos bytes; o scanner deve conseguir consumir `1.5` em `%f`, armazenar 1.5 e informar quantidade consumida sem parar no serviço ausente. Preserve os contratos Mac efetivamente observados, inclusive divergências de APIs atuais.

A ampliação foi solicitada pelo usuário durante a preparação deste handoff. Ela acrescenta seis entradas de conversão de caracteres, uma entrada de parsing floating e os dois bridges que impediam conversões decimais/NaN de entrada. Complete esse bloco no mesmo goal, em vez de entregar apenas o formatter e adiar suas cadeias diretamente relacionadas. O escopo continua fechado; isso não é uma autorização para reconstruir todo o runtime C/arquivos.

Entregue o bloco completo, com checkpoints internos por entrada e uma aceitação compartilhada. Use operações semânticas nomeadas e ABI explícito; não entregue somente investigação, wrappers chamando os corpos substituídos, replay P-code ou tradução opcode por opcode. O default produtivo deve executar os novos corpos. Não prometa ganho visual ou de desempenho.

## Decisões e limites de validação

- Preserve a decisão vigente de validação proporcional: checks do código alterado, negativos e regressões diretamente relacionadas, uma instalação final protegida e um smoke curto do app instalado. Não repita UI por entrada.
- Save/Load, save/exit/reopen/load, carreiras, dias/semanas/temporadas, partidas, auditoria de serialização, benchmarks e a suíte inteira estão fora do escopo. Persistência permanece **not retested**, sem PASS e sem gate futuro inventado.
- Não corrija a assimetria ascending-year de leap da Date. `dec2num` e `nan` agora entram como bridges necessários; `num2dec` já implementado permanece dependência. A tentativa hexadecimal de entrada que esgotou budget do original exige caracterização limitada, não uma mudança não autorizada de comportamento legado nem campanha de investigação sem fim.
- Faça a implementação, documentação, instalação protegida e commit/push normal de produto validado. Preserve alterações alheias e artefatos privados. Não force push, não crie nova sessão e não implemente outros subsistemas.
- Este arquivo é um handoff para execução em uma sessão futura. Sua preparação só inspecionou o projeto, fez o check de dependência indicado abaixo e salvou/copiou o prompt; não atualizou o aplicativo nem implementou esta feature.

## Ponto de partida verificado em 7 de outubro de 2026

- Projeto `/Users/victordmor/Projects/cm0102mac`, branch `main`, HEAD e consulta nova ao remoto `origin/main`: `ad46216a4e3f94f2e81a4f5ca996f7279c135300`, `feat(native): reconstruct Mac formatted input scanner`. Remoto: `git@github.com:VictorDMor/cm0102mac.git`. Reconfira branch, remoto, HEAD e diff antes de trabalhar.
- Tracked/staged limpos. Havia dezesseis prompts privados untracked em `docs/features`, além dos cinco prompts tracked. Este handoff acrescenta o décimo sétimo privado; preserve todos e exclua os privados do commit de produto. Nenhum `AGENTS.md` aplicável foi encontrado no projeto/ancestrais; confira as instruções atuais.
- `.local/recompiled/compilation.json` contém 1.953.157 instruções, 1.922 páginas, zero missing/pending static targets e **78 substituições semânticas**. Estas vinte e duas entradas ainda usam corpos gerados. Resultado esperado: **100 substituições**, mantendo as 78 anteriores na mesma ordem. Bridges não são entradas extras nesse array. SHA-256 do array compacto `semanticReplacements`, com chaves ordenadas: `fd8333c4a205239921b990433f4397bbab56c5acb0eb43ea0e584cf3de85219f`.
- A entrega de entrada formatada registra 931 comparações, 645 faults esperados, sete negativos próprios, um negativo de filho, sete entradas com chamadas normais não vazias, sete transições indiretas, doze modos mistos, três CTests, trinta testes Python e UBSAN focado aprovados. São resultados da entrega anterior, não rerodados nesta preparação e não validam o novo formatter. Consulte `docs/native-formatted-input-reconstruction.md`.
- **Check executado nesta preparação:** `ctest --test-dir .local/aot-build --output-on-failure -R '^original_decimal_formatting$'` passou em 2,59 s: 1.064 casos de ABI do bridge decimal e catorze formatações pelos corpos originais, incluindo rounding, padding, decimal/exponencial, zero negativo, infinity e NaN. Isso confirma uma dependência/fixture já executável; não é equivalência do novo bloco.
- O executável instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app/Contents/MacOS/CM 01-02 Native Prototype` foi reconferido como Mach-O ARM64, SHA-256 `2e88cca135176aed98fe9771c06a269463e7fd16ebc14161fd66bd9c4492437e`. O smoke da entrega anterior observou menu, abertura/cancelamento do chooser e saída normal, sem carregar carreira. Não foi repetido nesta preparação. Persistência não foi retestada.

Reconfira estados que possam mudar. Não dependa deste chat, de recibos temporários ou dos logs da preparação para executar o objetivo. O índice privado Mac foi inspecionado read-only; não houve novo export Ghidra, geração, build ou instalação nesta preparação.

## Inspeção dirigida

Leia `README.md`, as últimas seções de `docs/native-port-status.md`, `docs/native-formatted-input-reconstruction.md`, `docs/native-string-core-reconstruction.md` e `docs/native-memory-primitives-reconstruction.md`. Inspecione:

- `native/formatted_input_adapter.*`, `native/formatted_input_checks.cpp`, `tools/formatted_input_audit.py` e `tools/test_formatted_input_audit.py`: State clonado, ABI/effects, callbacks mutáveis, per-entry selection/counters, faults, negativos, modos mistos e pins integrais/por página.
- `native/decimal_bridge.*` e `native/decimal_checks.cpp`: ABI CFM floating, record decimal de 42 bytes, `num2dec`, rounding e ambiente floating host/guest. As catorze expectativas originais são uma regressão direta do novo formatter.
- `native/carbon_bridge.cpp`, `native/floating_point.*` e os callers/glues originais de `nan`/`dec2num`: dispatch/import ABI, f1 e FPSCR. Consulte os headers primários locais `fp.h`/`math.h` do SDK em `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk` e a documentação original PowerPC Numerics para contratos/exceptions/NaN; compatibilidade de assinatura ARM64 não prova identidade de flags ou payload PPC.
- `native/string_core_adapter.*`, `native/memory_primitives_adapter.*`, `native/unsigned_division.hpp` e seus checks/audits: dependências produtivas já semânticas/otimizadas. Não substitua seus contratos por funções host.
- `native/aot_runtime.hpp`, `native/aot_runtime.cpp`, `native/aot_probe.cpp`, `native/CMakeLists.txt`, `tools/compile_recomp.py`: dispatch, memória guest, LR/TOC, floating state, hooks nas labels e build consistente.
- Os audits de entrada formatada/inteiros/memória/strings/coleções/backgrounds: asserts do tamanho 78 e slices antigos. Ao acrescentar o novo bloco, limite o slice de entrada formatada a `[71:78]`; mantenha inteiros `[66:71]`, memória `[60:66]`, strings `[46:60]` e os anteriores.
- `.local/recomp-input/instructions.sqlite`, aberto read-only, `.local/recomp-input/program.json` e páginas geradas `0x10777` a `0x1077b` e `0x1077d`/`0x1077e`, mais dependências pertinentes. Use o export existente; não publique listings, tabelas ou segmentos proprietários nem abra uma campanha Ghidra.
- Dependências mantidas: descriptor stub `0x10781320`, glue `num2dec` `0x107800d8` e bridge `original_num2dec`, orientação `0x1077fa50`, escrita de stream `0x10776360`/`0x10776380`, divisão word64 `0x10773a90`/`0x10773870`, comprimento/cópia/cópia limitada/busca `0x1077dbd0`/`0x1077dbf0`/`0x1077dc10`/`0x1077dd10`, BlockMoveData `0x10739ef0` e memset `0x10778330`. Nenhum desses corpos é uma nova substituição. Os glues `nan` `0x10780be8` e `dec2num` `0x10780c00` continuam gerados, mas passam a despachar para os bridges novos.

## Escopo fechado e fingerprints

As responsabilidades abaixo são inferidas das chamadas/caminhos observados; reconfirme os detalhes de contrato antes de implementar. Fingerprints são SHA-256 dos registros `address/assembly/ops/flows/fallthrough`, ordenados por endereço, em JSON compacto com chaves ordenadas e UTF-8.

| Entrada | Fim exclusivo | Instruções | Responsabilidade | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x10778db0` | `0x1077961c` | 537 | Análise da conversão e record de saída | `9d77e7c9292f155e8d5f82dbae9a95eac10fa25b31809bf25a8335d2b715d018` |
| `0x10779620` | `0x10779874` | 149 | Rounding/manipulação de dígitos | `d04782e3a2562e22408ade74e133079fe657227d1e56b841db2f4f00e3403716` |
| `0x10779880` | `0x10779b70` | 188 | Inteiro word64 para texto | `73cee661ead3f7dc13187e6ed3b7a1592571e86e073e79b7d378eaee87d6f0b3` |
| `0x10779b70` | `0x10779ec4` | 213 | Formatação floating `a/A` | `05dd8372702a1f5eaf71461f600caaf06beaa3f1c5f8c4a8d9ab77fbbec019bf` |
| `0x10779ed0` | `0x10779ff0` | 72 | Rounding do record decimal | `d5d8a676a3785276a70f5b4528ad1d674dbe9b73398d0ea665b0415eaf449ba7` |
| `0x10779ff0` | `0x1077a000` | 4 | Ajuste de comprimento/reset do record | `a7b6b9884fc4e62cda472cea5e0e690bc527320cf6962c8486a60bbe8f490b3d` |
| `0x1077a000` | `0x1077a688` | 416 | Formatação floating `e/E/f/F/g/G` | `d2efad11f7bacef895ef80e09dc1551740f6f194658cca520d40cbfbee95a1a3` |
| `0x1077a690` | `0x1077b4bc` | 905 | Formatter geral via descriptor | `f2f03b913302837f0e3cdf83c7443b6b33e03cf54a117784c64c8405a07213ce` |
| `0x1077b4c0` | `0x1077b518` | 22 | Callback de escrita em stream | `151c7ee9e4e169a7948965366ff9ef7d5d7f77bdd15b3d53211006f2ff1e285a` |
| `0x1077b520` | `0x1077b58c` | 27 | Callback de buffer bounded | `04f3344b694f63cebd1ef4015c0f84e627edfbcc1ab30442bc9120cbc9a73895` |
| `0x1077b590` | `0x1077b60c` | 31 | Fachada variádica do stream padrão | `b4299df76c0e97fbff2af11d9bccd17652b2668fc00659e58c5e6c085beba492` |
| `0x1077b610` | `0x1077b680` | 28 | Fachada variádica de stream explícito | `63859c1a7c3e18edff44de9076e0ad885a53a22f6126e14e906faa12cf20816d` |
| `0x1077b680` | `0x1077b6f0` | 28 | Fachada de buffer bounded com vetor guest | `ca9ddb3c231f9890952b7781f60951318408bc84aa32fa8f5b679f682e0d7478` |
| `0x1077b6f0` | `0x1077b71c` | 11 | Fachada de buffer sem limite explícito, vetor guest | `d09d94e55aa60fcb3079d02a4f31baf6e9a576301b62bea953f6b53ff1f9ad3b` |
| `0x1077b720` | `0x1077b768` | 18 | Fachada variádica de buffer | `1f9eadd348ef0ecc80ff3046885dcf156a1da6d0066242cf4729ab41c70715e0` |

O bloco de saída acima tem **2.649 instruções indexadas**, com gaps. Acrescente também as sete entradas relacionadas abaixo, na ordem indicada, depois das quinze anteriores:

| Entrada | Fim exclusivo | Instruções | Responsabilidade | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x10777f60` | `0x1077804c` | 59 | Classificar sequência multibyte de um a três bytes | `18e9c5e242d2a7bc609170c8b8b46a41a8f917b8c9b0d24504e3fe8bfb8478d4` |
| `0x10778050` | `0x10778170` | 72 | Converter sequência multibyte para valor wide16 | `ee7bbc5ebd365ac62f88d0c618e9ee1fe816970d04ce4dc534235bec448475a6` |
| `0x10778170` | `0x10778190` | 8 | Wrapper de conversão usado pelo scanner | `f81d79e971a21aa38f0ac88fcd49a741524566e51711a35d9b0ae5f02ae3caf0` |
| `0x10778190` | `0x10778234` | 41 | Codificar wide16 em sequência multibyte | `d8f444a8742586897868cba5be5309ced8e2c0eafea674c44c577133d5779db8` |
| `0x10778240` | `0x10778260` | 8 | Wrapper de codificação de caractere | `d1bc2f230c1440eabdfae5512df65af3610f5e07148fd0f1b77d9f798ffbd795` |
| `0x10778260` | `0x10778300` | 40 | Converter string wide16 para buffer multibyte | `7fe292fa32e6a1dfb0820710fdc91ad15d90a518237d5f791b989132d123c37e` |
| `0x1077de10` | `0x1077ee14` | 1.025 | Parser floating por descriptor usado pelo scanner | `d8a7ca0ccef635d1d52ac5f2964656fddcd485635321da6552d2668ff9e8c873` |

Total ampliado: **22 entradas / 3.902 instruções indexadas**, mais dois serviços importados. Append na ordem das duas tabelas para chegar a 100 substituições. Não calcule counts pelo tamanho dos intervalos. Há quatro corpos cross-page:

| Corpo | Página | Instruções | SHA-256 do fragmento |
| --- | --- | ---: | --- |
| `0x10778db0` | `0x10778` | 147 | `d8a3a46d7faa47934015c27534559cf58e4098bc2f64c6bce827da5bfe88089c` |
| `0x10778db0` | `0x10779` | 390 | `0b75c6d901f2c3dc15707e4deb070d000d7ba959b5feafd1bf8f6a77053e8ae3` |
| `0x1077a690` | `0x1077a` | 604 | `48186562a7f910f730dfb6b644c161918554bd5cd55e58d5d7bae19af99b8ea6` |
| `0x1077a690` | `0x1077b` | 301 | `f9c4b88b7b74ddb6bb4a64638d71bdbad3b6affd69c9431395eb7469d44ad569` |
| `0x10777f60` | `0x10777` | 40 | `55df94060c13da861c4ab8df7454ce2348e9f7bdf4c6b8b72bfa0eb4bce30581` |
| `0x10777f60` | `0x10778` | 19 | `0800ad96c12c9c9403049a0459c78e5511c64982035f9029bed3b8303591c82c` |
| `0x1077de10` | `0x1077d` | 124 | `67684ac1e0471dffaa6e72b56d7e2a49480f2853c28e2e45b7b00e942e5793f5` |
| `0x1077de10` | `0x1077e` | 901 | `13e429241415e3f20b280993cbe63ceb56b5b17b11ad9e2a77c699f11a997e44` |

A consulta nova de flows/fallthroughs encontrou **uma aresta externa para interior**: `0x10779ff4 -> 0x10779ed8`. O wrapper de quatro instruções compartilha o reset de decimal da entrada anterior. Não use a premissa falsa de zero arestas. Modele explicitamente o destino compartilhado como operação semântica e proteja a transição gerada/mista, sem fallback normal no corpo antigo. O audit deve fixar e testar somente esse par conhecido, rejeitando novas arestas ou uma remoção/mutação do destino; decidir a organização interna do hook compartilhado cabe à implementação. O destino é interno ao bloco, não uma décima sexta rotina pública no manifesto.

Nas sete entradas acrescentadas, a consulta nova encontrou zero arestas externas para interior, mantendo somente o par compartilhado do bloco de saída. Counts de calls diretos indexados: **1/3/2/1/3/0/1/3/0/0/1/146/2/2/8009** na primeira tabela e **1/1/2/1/1/1/1** na segunda. São relevância estática, incluem calls internos e não comprovam runtime. Os callbacks são referenciados por descriptor; entradas sem caller direto também precisam de fixture próprio. Não é necessário executar milhares de callers ou carregar uma carreira.

## Contratos a reconstruir

### Análise do formato

`0x10778db0` recebe formato/ponteiro para cursor de argumentos/record em r3/r4/r5, retorna o próximo endereço de formato em r3, salva r27–r31 e usa frame de 144 bytes. O record copiado tem 60 bytes; nem todos os campos de caminhos especiais são inicializados. Preserve resíduos, padding, loads/stores por campo, frame/record/cursor aliases e fault prefixes em vez de zerar o record inteiro.

Reconstrua `%%`, flags efetivamente aceitas, largura/precisão literal e por argumento `*`, valores negativos, modificadores, códigos válidos/inválidos/truncados e extensões repetidas/empacotadas observadas. Caracterize `v`, delimitadores e argumentos auxiliares antes de batizar seu significado; não apague os caminhos por serem ausentes no printf host. Preserve avanço do cursor, alinhamento de vetores de 16 bytes e tamanhos Mac32 dos argumentos.

A classificação vem da tabela guest em `r2-0x5a48`; flags e conversões fazem dispatch pelas tabelas em `r2-0x3b48` e `r2-0x3b4c`, carregando o destino e CTR. Selecione operações semânticas pelos destinos vivos, com mutação das tabelas/TOC, leituras e efeitos intactos. Caracterize destinos não identificados e preserve sua fronteira compatível com o runtime, com escape privado distinto, sem contá-los como cobertura normal.

### Dígitos, inteiros e floating de saída

Caracterize os dois helpers de rounding e o wrapper de comprimento como um grupo coerente, preservando tie/carry, limites, exponent/sign/digit count, stores e a operação de reset compartilhada. A tabela de rounding em `r2-0x3b50` é guest/live. Não substitua suas decisões por arredondamento do host.

`0x10779880` recebe magnitude word64 em r3:r4 e final de buffer em r5, constrói texto retroativamente e lê os campos de formatação dos argumentos Mac. Preserve zero/precisão zero, bases, uppercase/lowercase, sinal/prefixo, alternate form, limites e ponteiro retornado. O mapa de dígitos em `r2-0x3b54` permanece vivo. Mantenha os calls/divisão word64 existentes, incluindo o caminho já otimizado de divisão unsigned.

`0x10779b70` e `0x1077a000` usam f1, buffer/argumentos guest e glue `0x107800d8` para `num2dec`. Preserve frames, f31/FPSCR, marshaling e efeitos do bridge implementado. Reconstrua `a/A` e `e/E/f/F/g/G`, signed zero, valores finitos, infinity/NaN, precision, corte/carry/ties, alternate form e limite observado de 509 bytes, conforme original. Strings especiais, classificação e constants/formatos TOC continuam guest/live. Não use `std::format`, `snprintf`, `to_chars` ou o locale host para produzir os resultados substituídos. Não reimplemente o serviço `num2dec`.

### Formatter geral

`0x1077a690` recebe descriptor/contexto/formato/vetor guest em r3–r6, retorna quantidade calculada ou -1, salva r18–r31/f31 e usa frame de 768 bytes. Reconstrua emissão de literais, análise da conversão, extração de argumentos, conversões, padding/sign/prefixes, `%n`, repetições/packed lanes e término como operações nomeadas. Preserve a distinção entre comprimento calculado, bytes efetivamente escritos e erro do callback; não aplique regras atuais de C99 automaticamente.

Todos os emits usam o stub gerado `0x10781320`, relendo entry/TOC do descriptor. Preserve contexto/source/count em r3/r4/r5, clobbers, TOC spill/restore, argumento cursor, eventos, efeitos e faults. Retorno zero do callback encerra com erro; a semântica de cada callback pode diferir. Teste ainda um callback genérico com chunks observáveis, falha/clobber e descriptor mutável. O formatter não deve ser especializado para string host.

Mantenha calls dos helpers semânticos anteriores e integre a nova conversão semântica `0x10778260` nos caminhos de caracteres. Preserve strings/chars/widths/precision e null/invalid pointers como o original executa; não acrescente guards host nem troque os acessos bulk por uma ordem bytewise diferente. Preserve guest big-endian e todos os efeitos arquiteturais/temporários observáveis.

### Callbacks e fachadas

- `0x1077b520` usa contexto com destination/capacity/written em offsets 0/4/8. Calcula o chunk copiável, chama o BlockMoveData semântico, relê e atualiza written e retorna 1 mesmo quando truncou. Preserve wrap/comparações, overlap, zero count, context/source/destination aliases e faults. Não converta ponteiros guest em host.
- `0x1077b680` recebe destination/capacity/format/vector em r3–r6, prepara contexto em frame de 80 bytes e lê o descriptor em `r2-0x5a50`. Depois do formatter, destination não nulo recebe NUL no índice determinado por retorno/capacity; capacidade zero pode levar à escrita em destination-1. Preserve e teste esse comportamento, sem uma correção de segurança fora de escopo. Destination nulo ainda segue seu caminho original; não infira semantics de length-only pelo nome.
- `0x1077b6f0` encaminha vetor guest com capacity `0xffffffff`. `0x1077b720` usa esse limite, salva r4–r10 nos slots originais e encaminha o vetor; ambas usam frames de 64 bytes. Preserve spills, argumentos em registers/stack e fronteiras próprias.
- `0x1077b4c0` encaminha source/size/count/stream para `0x10776360`, com size 1; retorna o contexto quando count retornado coincide com o pedido, senão zero. Preserve o contrato exato, inclusive count zero, sem escrita host.
- `0x1077b610` salva stream e spills r4–r10, chama orientação `0x1077fa50` com -1 e só aceita retorno negativo; descriptor em `r2-0x5a4c`. `0x1077b590` faz o mesmo com o stream padrão obtido pelo ponteiro em `r2-0x5a68` mais `0x54`, mas tem seu vetor de argumentos próprio. Não acrescente uma regra de stream nulo ausente do original.

Valide streams **offline**, com objeto/buffer guest artificial, orientação/flags/capacidade controladas e helpers gerados preservados. Para a fachada padrão, redirecione o slot guest para esse fixture. Caracterize os campos de escrita buffering e escolha chunks que não exijam flush, arquivo ou OS; não use offsets de leitura como se fossem contrato de escrita. Inclua orientação recusada e erro retornado pelo helper. Não escreva stdout real, arquivos pessoais, rede, nem amplie para reconstruir fwrite/flush/refill/locale/arquivos.

### Conversão bidirecional de caracteres

Reconstrua as seis entradas como a cadeia Mac de sequências multibyte de um a três bytes e valores wide16. Não transplante `wchar_t` ARM64, que tem outro tamanho, nem imponha validações Unicode modernas ausentes no original.

`0x10777f60` classifica a sequência e retorna 0 para NUL, comprimento de 1/2/3 bytes, -1 para inválida ou -2 para incompleta conforme os branches observados. Preserve bytes signed, ordem/short-circuit de reads, limites e aceitação/rejeição legada, incluindo prefixes, continuation bytes, overlongs e ausência de suporte de quatro bytes. `0x10778050` recebe destino/input/limite, chama esse helper e escreve resultado guest de 16 bits; preserve destino/input nulos, limite zero, retorno e aliases. `0x10778170` mantém sua fronteira/ABI própria ao encaminhar essa conversão.

`0x10778190` reduz o caractere aos 16 bits observados e monta de um a três bytes com stores na ordem original. As duas halfwords em `r2-0x34f8`/`r2-0x34f6` são lidas/spilled antes do retorno para destino nulo; preserve conteúdo mutável, registradores e efeito do red-zone de stack. `0x10778240` encaminha a conversão com seu próprio frame. `0x10778260` percorre uma string wide16 guest, chama o wrapper e copia chunks pelo helper semântico de cópia limitada. Preserve quantidade/limite, NUL, destino nulo, partial output, cursor e comportamento de bytes extras/padding.

Valide fronteiras 0/0x7f/0x80/0x7ff/0x800/0xffff, sequência incompleta/inválida, continuation bytes e high bytes, destinos pequenos, mutação dos prefixos TOC, overlap e faults. Demonstre composições via `%lc`/`%ls` do scanner já semântico e via conversões de saída que usam o string helper. A divergência anterior de incremento de destino em `lc` versus `ls` do scanner é preservada; integrar o helper novo não autoriza corrigir o caller.

### Parsing floating de entrada e bridges faltantes

`0x1077de10` recebe limite/descriptor/contexto/ponteiro de consumed count/ponteiro de flag em r3–r7, retorna double em f1, salva r13–r31 e usa frame de 272 bytes. Reconstrua operações nomeadas de whitespace, sinal, significand, separador/exponent, infinity/NaN, término/lookahead e flags. Preserve os paths decimal/hexadecimal realmente presentes, consumo/devolução, overflow/underflow, stores, signed zero e estado floating.

Templates, classificação/mapeamento, phase destinations e constantes continuam guest/live: slots observados incluem `r2-0x3b2c`, `-0x3b30`, `-0x3b34`, `-0x3b38`, `-0x62bc`, `-0x5a48` e ponteiros para constantes `-0x5a2c` a `-0x5a3c`. Identifique cada contrato por leitura dirigida, preserve mutação, CTR/TOC, callbacks pelo stub e fault prefix. Não use `strtod`, `from_chars` ou locale host para substituir o parser.

Implemente os bridges `CarbonLib::dec2num` e `CarbonLib::nan` no dispatch produtivo:

- `dec2num` lê o record decimal guest de 42 bytes pelo argumento r3, respeita layout/sign/digit count e exponent signed16 big-endian, retorna a representação double em f1 e não altera o record. Caracterize campos/dígitos aceitos, especiais, rounding e exceptions documentadas PPC. Pode usar a função nativa Apple para a conversão de record depois de marshaling correto, com tratamento explícito das diferenças de ambiente/flags; não parseie uma string textual pelo host como substituto.
- `nan` lê a tag guest NUL-terminated em r3 e retorna quiet NaN em f1. Preserve o contrato original de tag/payload/signaling e os efeitos documentados. Não presuma que `std::nan` ARM64 tem os mesmos bits de payload PPC sem prova; use expectativas independentes e fontes primárias. O parser aplica o sinal em seus próprios caminhos.
- Confira os glues/callers reais, registradores não retornados, PC/LR/TOC, import accounting, memória/faults e ambiente floating nativo. Não altere o bridge `num2dec` que já funciona. Teste os serviços separadamente com records/tags controlados e um oracle independente de bits/exceptions; comparar dois parsers que usam o mesmo bridge novo não prova o bridge.

O harness anterior de entrada tratava três decimais e um NaN como faults de serviço ausente. Atualize esses mesmos casos para conversões bem-sucedidas com expectativas independentes, acrescente as composições `%f%n`/`%lf%n`/supressão/packed e não apenas remova os casos ou permita ambos os resultados. Relatórios/evidence histórica continuam históricos; não reescreva seus PASS/counts retroativamente.

Caracterize com budget pequeno a tentativa hexadecimal que não terminava no original. Não transforme timeout em PASS, não prolongue indefinidamente a investigação e não imponha semântica corrigida que diverge do legacy. Exercite caminhos hex executáveis quando existirem e registre com precisão qualquer limite legado restante, sem deixar conversões decimais/NaN comuns bloqueadas. Fixtures de stream floating permanecem artificiais/buffered e offline.

## Integração e aceitação focada

Organize adapters/harnesses/audits por cadeias coerentes (saída, caracteres e floating de entrada), seguindo os padrões existentes, com uma aceitação final compartilhada. A divisão interna de arquivos cabe à implementação. Hooks devem estar nas labels de destino antes da primeira operação original, cobrindo goto local, dispatch cross-page e transferências indiretas. Trate também o destino compartilhado observado. Preserve as 78 entradas/fixups anteriores e a seleção independente dos módulos. Pinne os corpos inteiros antes do manifesto e os oito fragmentos completos cross-page; um corpo/entrada/página inteiramente ausente não pode emitir manifesto completo.

Seleção original global/por entrada, counters, observers e escapes devem ser privados, process-local e não serializados. Mantenha reset independente. Nenhum caminho normal pode executar seu corpo substituído; desconhecidos/escapes e execução original diagnóstica têm evidência separada.

Compare original versus semântico desde o mesmo State com dependências anteriores semânticas: arquitetura completa, registros/temporários, memória guest, efeitos ordenados, callbacks/arguments, PC/LR/CTR/TOC/CR/XER/FPSCR e prefixos até faults. Exclua somente accounting/diagnósticos privados; não normalize divergências de ABI. Acrescente expectativas independentes de output, retorno, cursor/alinhamento, record, `%n`, guards e buffer/flags.

Matriz pequena e discriminante: literal/percent/invalid formats; flags, width/precision e `*` negativos; signed/unsigned word32/64 e tamanhos suportados; zero/max/min/bases/prefixes; strings/chars/conversões multibyte; `%n`; repetições/packed extensões; rounding/decimal/hexadecimal de saída; decimal/exponent/especiais/limites floating de entrada; buffer nulo/zero/curto/exato/maior, truncation e count versus written; callback genérico/falha; streams buffered/orientação. Use budgets limitados nos adversos, sem campanhas combinatórias.

Inclua mutações live de classificação/dígitos/rounding/dispatch/descriptor/prefixos, TOC relocada, clobbers, aliases representativos e mappings mínimos com remoção de bytes acessados. Exija um negativo próprio por entrada e pelo menos um filho rejeitado antes do parent em cada cadeia; os dois bridges têm seus controles independentes. Todas as vinte e duas entradas precisam de chamadas normais não vazias, zero original selection e zero escape normal, inclusive wrappers sem caller direto. Exercite dispatch indireto gerado para cada entrada e o tail compartilhado, composições pelas fachadas reais de buffer/stream do formatter e scanner, e alguns modos mistos discriminantes, sem produto cartesiano. Counters diagnósticos distinguem modos original/semântico; isso não permite normalizar diferenças arquiteturais ou efeitos de imports.

Após estabilização, regenere uma vez e faça um build Release ARM64 consistente, sobretudo se State mudar. Comandos propostos para os novos targets:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure -R '^(modern_formatted_output_equivalence|modern_character_conversion_equivalence|modern_floating_input_equivalence|modern_formatted_input_equivalence|original_decimal_formatting|modern_string_core_equivalence|modern_memory_primitives_equivalence)$'
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

Use os nomes novos efetivamente implementados. Decimal/string/memória/scanner são regressões diretamente usadas ou alteradas; estenda os checks decimais com os dois novos bridges. Os Python antigos entram pelos asserts de manifesto e hooks nas mesmas páginas. Execute UBSAN somente nos adapters/harnesses/bridges novos ou modificados, ligado às bibliotecas finais consistentes. Amplie checks somente por falha concreta/dependência afetada, explicitando motivo e stopping condition. Repita geração/build/grupos verdes somente por mudança que os invalide, não por ritual. Não use linkage misto de layouts antigos/novos de State como aceitação.

## Instalação protegida e smoke curto

Antes da atualização, preserve o instalado com forks/xattrs/metadados em nova referência privada, por exemplo `.local/formatted-output-reconstruction/reference/installed.app`, sem sobrescrever referências/candidatos anteriores. Preserve game files, saves/preferências, original payloads, snapshot Git LFS, todos os handoffs e históricos de entrada/inteiros/memória/strings/coleções/backgrounds. Não rode o instalador durante uma sessão do usuário nem encerre sua sessão à força.

Após aceitação técnica, use `tools/install_native.py` uma vez para a atualização final. Distinga linker output, source/candidato assinado, instalado e referência pelo executável interno/ARM64/hash. Confira original payloads, assinatura e plist de produção; fixtures, diagnóstico privado e configuração de validação não entram no bundle. Verifique proteção imediatamente após instalação, antes do smoke.

Abra o **app instalado** em `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`, observe menu renderizado e uma resposta simples por comando nomeado. Pode abrir/cancelar chooser sem carregar carreira. Encerre normalmente se ainda for exclusivamente uma sessão de teste; preserve-a se o usuário assumir a interação. Confira processo/log, ausência de falhas fatais e hash testado. Pare aí: cobertura por entrada vem do harness, sem exigir que startup chame vinte e duas rotinas. Logs/preferências podem mudar; não declare Runtime/Runs byte-idênticos depois. Não teste Save/Load, dias ou partida.

## Entrega

Documente em inglês o bloco ampliado, contratos/pins, destino compartilhado, ABI dos bridges, integração, per-entry evidence, check results realmente executados, limites de dependências/dispatch/hex legado, identidade instalada e smoke em `docs/native-formatted-output-reconstruction.md` (o título/conteúdo deve abranger as três cadeias); atualize `docs/native-port-status.md`. Diferencie a baseline original de hoje, a entrega histórica do scanner, a equivalência nova e o smoke. Registre que decimal/NaN de entrada deixaram de falhar por serviço ausente, com evidência. Persistência permanece **not retested**. Não declare reconstruídos `num2dec`, locale, fwrite ou runtime de arquivos/streams.

Revise o diff exato e publique somente adapter, harness, audits, integração produtiva e documentação técnica no remoto privado/branch corretos por commit/push normal, seguido de consulta nova do SHA remoto. Preserve `.gitignore`, LFS e mudanças alheias; exclua prompts privados, receipts, logs, imagens, saves, índices, payloads, fontes geradas e bundles. Não force push.

Conclua com as vinte e duas entradas e dois bridges entregues, resultados focados, evidência do app instalado, commit publicado e limitações materiais. O goal termina com implementação produtiva e essa aceitação proporcional; não acrescente gates dispensados.
