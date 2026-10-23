/goal Reconstrua e entregue o subsistema de datas (Date) do CM 01/02 Mac em C++23 semântico na produção ARM64. Complete as 26 entradas pendentes inventariadas abaixo, preservando as oito entradas já entregues, e valide o conjunto de 34 entradas com equivalência, contratos, cobertura real, persistência, instalação e entrega. Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto, unidade de entrega e fronteira

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em código semântico compreensível. O usuário quer um goal prompt por subsistema, com etapas internas e checkpoints. Não conclua após a primeira rotina ou após um grupo de seis, não transforme cada etapa em outro prompt e não substitua implementação por investigação. Preserve progresso e evidências em documentação reproduzível entre turnos e compactações; termine após a entrega do módulo inteiro acordado.

Este goal abrange **Date**: representação/inicialização, construção civil/ordinal, classificação bissexta, ajuste de weekday, operações por dias/semanas e entre registros, diferença, ordenação básica, extração de componentes e fachada de apresentação/interpretação de datas, weekdays, meses e fases. Há 32 corpos contíguos do módulo em `[0x107156b0,0x10718b38)`, separados por gaps de alinhamento; mais os dois predicados independentes já entregues `0x103317f0` e `0x10331860`. São 26 corpos pendentes, com 2.898 instruções indexadas, e oito entradas entregues. Contagens delimitam o inventário e não substituem equivalência ou cobertura.

As oito entregues são add `0x10717890`, subtract `0x10717b20`, difference `0x10718130`, later `0x10331860`, before `0x103317f0`, day-of-month `0x107182f0`, month `0x10718360` e weekday `0x10715b20`. A temporada real e passagem anual também já estão entregues em `docs/native-season-reference.md`. Não repita esses marcos como pendência; preserve seus contratos e regressões.

Aceitação integrada: carregar uma carreira real, manter data/fase/textos nos painéis e notícias, construir/ajustar datas das fixtures, continuar normalmente, salvar, sair e recarregar com datas, resultados e dados de jogador/contrato preservados. Demonstre que as entradas semânticas executam nesse fluxo ou em capturas originais de outros fluxos reais pertinentes; interface igual sozinha não prova reconstrução.

A fachada de datas pertence ao escopo. Serviços gerais de linguagem/templates, strings/ordinais, diagnósticos, imports, alocação e RNG continuam dependências originais. Chamá-los conforme seus contratos é permitido; delegar o corpo de uma das 26 entradas ao original é insuficiente. Agendador, competições, regras de temporada, treino, contratos/transferências e comparadores específicos de entidades ficam fora do módulo Date. Não inclua todo caller ou código que menciona data. Gameplay, alterações gráficas e novas otimizações continuam suspensos; não prometa velocidade.

Preserve a peculiaridade bissexta de difference expressamente adiada: January 1 2000 menos January 1 2001 retorna -365; a inversão retorna +366. Correção exige autorização separada e validação de impacto. Preserve restos negativos de weekday antes do epoch, flags inconsistentes, tabelas mutáveis e comportamentos inválidos legados. Não normalize tudo para Gregorian.

## Ponto de partida verificado na preparação

- `main`, HEAD e consulta nova a `origin/main` concordaram em `b6ce6ceb7c961a1f0d4d41f032b21117a4ae5cde`, entrega de weekday. Reconfira Git, remoto, diff e `AGENTS.md` aplicáveis. Não havia `AGENTS.md` nos ancestrais ou árvore do projeto inspecionada.
- Estavam untracked os quatro prompts `docs/features/native-date-difference-reconstruction-prompt.md`, `native-date-month-reconstruction-prompt.md`, `native-date-day-of-month-reconstruction-prompt.md` e `native-date-weekday-reconstruction-prompt.md`. Preserve-os. O handoff de ajuste isolado `docs/features/native-date-weekday-adjustment-reconstruction-prompt.md` também está pendente e deve ser preservado. O prompt de seis entradas `docs/features/native-date-construction-and-adjustment-reconstruction-prompt.md` também está pendente e deve ser preservado. Este novo prompt de subsistema substitui ambos como próximo objetivo; preparação de prompt não é mudança de produto entregue.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` foram reconferidos ARM64, executável SHA-256 `7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063`.
- O candidato privado assinado da entrega anterior `.local/performance/date-weekday-live-01/CM Validation date-weekday-live-01.app` foi reconferido ARM64, executável `ace1c411a6e863fcec0125105f763df7e81543df53ae440c151b82580060db14`. Não confunda candidato assinado com fonte/instalado.
- `.local/date-weekday-reconstruction/reference/installed.app` foi reconferido ARM64, executável `1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc`. Preserva a referência anterior a weekday, NÃO o instalado atual. Antes de trocar builds/instalar, crie uma **nova** cópia privada executável do instalado atual, preservando resource forks e extended attributes; confira identidade e inventário completo. Os backups antigos continuam imutáveis.
- O relatório entregue `docs/native-date-weekday-reconstruction.md` registra 4.240 casos diretos com ABI/memória completos, 559 chrono, 319.675 oracle Gregorian amplo, 786.432 casos puros, leituras ordenadas, mappings mínimos, independência da TOC, callers sintéticos, callbacks e negativos. A suíte final aprovou 36/36 CTest; descoberta Python 30 testes, 26 aprovados e quatro grupos opcionais pulados, também executados explicitamente. UBSAN gerado/focado passaram; 1.969 objetos ligados dependentes de State e 22 outros usaram o layout final. São resultados da entrega anterior, NÃO aprovação do novo bloco. Não reexecute suítes caras somente para planejar.
- Day/week/ordering aprovaram dois originais estritos, captura inteiramente original, seis baselines e dois candidatos. Weekday próprio por candidato: 1.690 / 5.831 / 1.419. Os quatro streams privados são CMCAL002, CMMON001, CMDOM001 e CMWKD001. Os três primeiros permaneceram byte-idênticos aos históricos de dia do mês. Negativos exercitados foram rejeitados; later/before permanecem sem cobertura em day/week e cobertos em ordering.
- A UI ARM64 anterior aprovou Save/saída normal/processo ausente/reabertura exata/Load em Sunday September 1, 2002 afternoon e Tuesday September 3 morning. Após ler a fila de notícias antigas, um Continue Game normal saltou a segunda-feira e parou em notícia de proposta de transferência; September 2 não foi forçado. Três runs registraram 160 / 1.794 / 152 weekday próprios e 96 amostras válidas conferidas. O relatório registra preservação de 809/5/43 arquivos Game/Runtime/Runs, com metadados, e 745 identidades históricas. Quantidades observadas não são invariantes nem inventários novos deste incremento.

Esta preparação foi somente leitura, seleção e escrita do handoff. Não implementou o novo bloco, não recompilou/instalou o produto nem abriu outro chat. Reconfira todo estado que possa ter mudado.

## Leia e inspecione primeiro

Leia `README.md`, `docs/native-port-status.md`, `docs/native-season-reference.md`, `docs/native-calendar-reconstruction.md` e os relatórios `docs/native-date-{difference,ordering,before,month,day-of-month,weekday}-reconstruction.md`, incluindo a melhoria adiada de diferença. Os relatórios versionados são contexto reproduzível; não dependa desta conversa ou apenas de logs temporários.

Inspecione `native/calendar.hpp`, `native/calendar_adapter.*`, `native/calendar_checks.cpp`, `native/calendar_reference_checks.cpp`, `native/date_weekday_checks.cpp`, `native/date_day_of_month_checks.cpp`, `native/date_month_checks.cpp`, `native/date_difference_checks.cpp`, `native/date_ordering_checks.cpp`, `native/callbacks.cpp`, `native/aot_runtime.*`, `native/aot_probe.cpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*` e `native/CMakeLists.txt`.

Inspecione também `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/test_generated_temporaries.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`. Use `.local/recomp-input/instructions.sqlite` read-only e o gerado privado para caracterizar o original. Exports/payloads não estão disponíveis em clone público limpo; não os publique nem altere para facilitar testes.

A referência Windows ReCM pode apoiar nomes e desenho de APIs. Sua localização configurada está em `.local/native-checks/CMakeCache.txt`; leia suas instruções caso a inspecione, sem editá-la. Seu `Date8` de oito bytes/flag de quatro bytes, ABI x86, formatos e tratamento de erros NÃO estabelecem o contrato Mac de cinco bytes/flag de um byte. Prove decisões contra o original Mac; não importe omissões de efeitos da referência Windows.

## Inventário fechado e contratos a demonstrar

Preparação estática read-only: corpos, retornos e fluxos diretos/fallthrough foram inspecionados; nenhum corpo novo mostrou fallthrough externo indexado. Reconfirme fronteiras, todas as saídas, entradas internas/indiretas e efeitos antes de implementar. Papéis são hipóteses fundamentadas, ainda sem equivalência própria; calls estáticos não provam cobertura.

| Entrada | Fim exclusivo | Instruções | Calls diretos | Papel candidato |
| --- | --- | ---: | ---: | --- |
| `0x107156b0` | `0x107156f0` | 16 | 256 | Construção ordinal |
| `0x107156f0` | `0x1071587c` | 99 | 388 | Construção civil A |
| `0x10715880` | `0x1071589c` | 7 | 691 | Inicialização legada |
| `0x107158a0` | `0x10715a28` | 98 | 37 | Construção civil B |
| `0x10715a30` | `0x10715b1c` | 59 | 11 | Ajuste de weekday |
| `0x10715bc0` | `0x10715df4` | 141 | 1 | Formatação de data, variante 1 |
| `0x10715e00` | `0x1071612c` | 203 | 62 | Formatação de data, variante 2 |
| `0x10716130` | `0x107164ec` | 239 | 2 | Formatação de data, variante 3 |
| `0x107164f0` | `0x1071689c` | 235 | 5 | Formatação de data, variante 4 |
| `0x107168a0` | `0x10716afc` | 151 | 1 | Formatação de data, variante 5 |
| `0x10716b00` | `0x10716ea4` | 233 | 12 | Formatação de data, variante 6 |
| `0x10716eb0` | `0x10717268` | 238 | 49 | Formatação de data, variante 7 |
| `0x10717270` | `0x10717620` | 236 | 22 | Formatação de data, variante 8 |
| `0x10717620` | `0x10717888` | 154 | 3 | Formatação de data, variante 9 |
| `0x10717a10` | `0x10717b1c` | 67 | 58 | Adição de semanas |
| `0x10717cb0` | `0x10717dc4` | 69 | 41 | Subtração de semanas |
| `0x10717dd0` | `0x10718130` | 216 | 32 | Aritmética entre registros de data |
| `0x10718440` | `0x107184cc` | 35 | 18 | Classificação bissexta |
| `0x107184d0` | `0x10718570` | 40 | 7 | Texto de fase A |
| `0x10718570` | `0x1071864c` | 55 | 8 | Texto de fase B |
| `0x10718650` | `0x107186f8` | 42 | 9 | Classificação/interpretação via linguagem |
| `0x10718700` | `0x107187cc` | 51 | 4 | Nome de weekday A |
| `0x107187d0` | `0x1071889c` | 51 | 15 | Nome de weekday B |
| `0x107188a0` | `0x107189c4` | 73 | 10 | Nome de mês A |
| `0x107189d0` | `0x10718af0` | 72 | 16 | Nome de mês B |
| `0x10718af0` | `0x10718b38` | 18 | 9 | Predicado de mês/parâmetro |

Fingerprints completos com address/assembly/ops/flows/fallthrough de `tools/calendar_audit.py`:

- `0x107156b0`: `b5afb42c7cf2f9b2e7700106534f771e3d59aafbbeab5c46df18e96f41433f65`
- `0x107156f0`: `08e6abaf2f8492455b469905a1909c7e824ac6f163d2f7d6ac1e83261bfaf1e0`
- `0x10715880`: `2c5d4b8b0ff83a4158f0774345d56f0cf44ac6ec826142aa7224c70bf9dccf97`
- `0x107158a0`: `aec1204dbb39a1ee4bd0311231a7d2e49680176787fbebf98f068b32f66866e8`
- `0x10715a30`: `bb8d3fd075cea7d4187bfbeca8bcd9a73185ee8f731dd2ebf6b077166f0381c9`
- `0x10715bc0`: `55ae2cea35ff6b36d12b9f22f476e189ca749a740fc23451ee54518ed7ea1aaf`
- `0x10715e00`: `daa986f528ed2b54a5cd4ced5ff99446bd4dc1523eff1a036171513259374e7b`
- `0x10716130`: `f867c1f447d84346b368f2e15f8368e8361ee58b6bde32d234bc2ec8bdbedd03`
- `0x107164f0`: `350c0ef220af4a826385f10befd96fb425687e51099c24cb6a947a4ed3e67ab6`
- `0x107168a0`: `3acb83e5419540eb4a12342ca212daf79aedcdfbf4744a896c4c05cd80e97a3f`
- `0x10716b00`: `48efde613c9ee9d67341582f635b09fdee63863b34cefa5420bfca563ce2e96d`
- `0x10716eb0`: `693678498117e0afd7883bbf173245b5e01386361cc11baac4d7dddd8f40f94f`
- `0x10717270`: `f65b4cef907f4ddff01354eb91edb8628224e74f51df1f726b061dae3ef2a49c`
- `0x10717620`: `646d1966f1e0062de505dab91c3dc80c4ec39156f081e8fde34ea2f90938df9e`
- `0x10717a10`: `72f98364db6921154752f63dd87dff3143e30cd73021c533ed4b5e251b7b6b4a`
- `0x10717cb0`: `9308b6d394ccc043f08c1c7e29f541bf8ce58968f69c83f70d9dfdd18c7f932b`
- `0x10717dd0`: `a4d549b50687ae112560fd1fdf290a07c82df06947a212c5739969bca9603684`
- `0x10718440`: `3cd3bf960d288bf41c7d004205b57d0399a315382c3d7f97822f5b431e1d83e5`
- `0x107184d0`: `05d7437c508319a8a53bbc717d60ab71a850a284be757b2895d23e8d10d8ad24`
- `0x10718570`: `1154241d88fdf3b8999908a55ac1af7a98c4c47fb2afbff9dfefcfce52a43eb6`
- `0x10718650`: `c981f82653f217d24b5caa7a54f73076b6c3cca490806d5085265a490b3f7908`
- `0x10718700`: `9dd95532b99ddd2e004ec7e109d2dcc0b413f4f929fc535a3a29fbd477ea3675`
- `0x107187d0`: `a61241b4ef4fce607a4e46b64c00a39464cc636f85eae1ea2b9af1667d00fdb2`
- `0x107188a0`: `0f336655908058de27f7ba78b1d5365cfe136f635188e68a56bd394f654d328a`
- `0x107189d0`: `c8ac0223a1f8592232ddc9ae68ae2c52e61bd4c5347ebd47e16b39aa97c97a4b`
- `0x10718af0`: `e1ccc2c870ee2201ee0c7a0801256828981971e92f1c99459699fdac00817218`

Índice read-only SHA-256 `2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec`; `program.json` `50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0`; 1.953.157 instruções; PEF `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`. Exports/payloads são privados; não publique listagens originais.

### Construção ordinal e inicialização

`0x107156b0` aparenta receber r3=destino, r4=day-of-year, r5=ano; grava day/year, chama `0x10718440` em `0x107156d0`, grava flag e retorna destino em r3. Há frame de `0x40`, spill de r31/LR e efeitos do helper. Demonstre estreitamentos, ordem das escritas, uso do ano bruto versus ano armazenado e aliases de destino com stack/spills. Não acrescente normalização de dia inválido.

`0x10715880` aparenta escrever day=40, year=1972, flag=1, nessa ordem, em r3. Não lê a data anterior nem classifica ano. Demonstre r0/r4/r3 e todos os efeitos/bytes realmente preservados; não transforme esse valor em “data atual”, zero ou sentinela inventada. É uma entrada própria usada por muitos callers, não apenas uma constante a remover.

### As duas construções civis

`0x107156f0` e `0x107158a0` aparentam receber r3=destino, r4=dia one-based, r5=mês zero-based, r6=ano, r7=weekday opcional. Usam day signed16, month signed8, year unsigned16 e requested weekday signed16; reconfirme widths/high bits e ordem exata. Salvam r24..r31 e LR, criam frame de `0xb0`, leem três slots TOC logo no início, chamam leap e selecionam a tabela comum/bissexta mutável nos slots r2-`0x3fa8`/r2-`0x3fa4`. Não substitua tabelas por constantes Gregorian nem importe cegamente o footprint de month/day-of-month.

A inspeção sugere verificações day 1..31 e month 0..11, mas **não** uma validação civil completa de cada mês. Inputs como February 31 precisam ser caracterizados, não rejeitados por nova regra. Branches inválidos usam helpers `0x10739890`, `0x10768810`, `0x1077b720` e `0x10769b60`, depois ajustam day/month e escrevem um flag através de ponteiro TOC. Determine o contrato desses efeitos, ordem/imports/callbacks/strings e comportamento de falha. Esses helpers continuam fora da reconstrução: podem ser chamados como dependências originais pelos caminhos semânticos, com fronteiras preservadas. Não execute o corpo original do construtor como fallback para inputs inválidos nem elimine diagnósticos.

As variantes são muito parecidas, mas diferem em constantes dos caminhos de diagnóstico e no retorno: A recoloca o destino em r3 no final; B não faz esse movimento. Não una seus efeitos por conveniência nem invente r3 comum. Mantenha comportamentos demonstrados por variante, usando lógica compartilhada somente onde for comprovadamente igual.

Ambas chamam leap (`0x107157f8`/`0x107159a8`) e, se requested weekday signed16 for diferente de -1, chamam ajuste in-place (`0x10715860`/`0x10715a10`), com os gotos reais da mesma página. O valor -1 tem papel de skip no caller; não assuma que tenha o mesmo significado na entrada de ajuste quando chamada diretamente.

### Ajuste de weekday e leap independente

`0x10715a30` aparenta receber r3=entrada, r4=requested weekday, r5=saída; salva r29..r31/LR, cria frame de `0x60`, chama weekday em `0x10715a54`, copia ano→dia→flag e usa subtração de calendário em `0x10715aa8` ou `0x10715ae4` com temporários frame+`0x40`/`0x38`. No domínio válido, delta aparente = weekday atual menos requested, signed16; corrige uma vez por +7 se menor que -3, por -7 se maior que 3. Isso sugere o weekday mais próximo em ±3 dias. Igualdade preserva a data. Demonstre, sem impor “sempre futuro” ou modulo Euclidean.

Exemplos a demonstrar: Sunday September 1, 2002 solicitado como Monday → September 2; Friday → August 30; Sunday → data intacta. São testes, não autorização para forçar datas na carreira. Compare target bruto/signed16, -1, 7, signed16 extremos, high bits, flags inconsistentes, weekday negativo e retorno r3 diferente por ramo. Entrada=saída é uso real; sobreposição parcial pode alterar leituras posteriores e não pode virar memcpy/snapshot antecipado.

`0x10718440` já possui fingerprint auditado como dependência e lógica semântica parcial de efeitos em `leap_clobbers`, mas **a entrada independente ainda é gerada**. O corpo tem retornos em `0x10718460`, `0x107184b8`, `0x107184c0` e `0x107184c8`. Demonstre regra unsigned16, r0/r3/r4/r5, CR0 completo, XER.CA/SO e demais efeitos em cada saída; o helper embutido anterior não é prova de ABI direta nem licença para ignorar clobbers que seus callers depois sobrescreviam.

### Comparação individual e integrada

Para todas as 26 entradas pendentes, compare resultado, registradores/temporários completos, CR/XER, LR/CTR, TOC/context TOC, PC, stack/backchains/spills, padding, todos os bytes mapeados, alocador, imports/callbacks/RNG e ordem de leituras/escritas/mapping/clear. `call_original` restaura contexto e não substitui ABI da chamada direta.

Cubra anos 0/1/1599/1600/1601, 1900/2000/2100 e extremos unsigned16, day/month/request signed/high bits, flags 0/1/128/255/inconsistentes, limites mensais/anuais/February 28/29/March 1, igualdade e ±3, tables/TOC mutáveis, input/output separados/in-place/parcialmente sobrepostos e aliases com frame/scratch/slots quando houver execução original bem definida. Use oracle Gregorian apenas no domínio demonstrado, budgets finitos para extremos e preserve resultados legados fora desse domínio. Delimite falhas de mapping/wrapping e mutação concorrente sem prometer equivalência universal.

### Semanas e aritmética entre registros

`0x10717a10` e `0x10717cb0` aparentam usar r3=saída, r4=entrada, r5=número de semanas reduzido unsigned8 e multiplicado por sete. Mantêm scratches/guards TOC próprios com inicialização legada, atravessam anos, chamam leap e copiam o resultado em ordem. Demonstre primeira chamada/init já-feito, widths/high bits, semanas 0/1/255, overflow signed16, leituras tardias e aliases. Não substitua sua ABI, guards, frames e scratches por simples delegação a add/subtract.

`0x10717dd0` aparenta operar entre registros, produzir outro registro e usar comparações, inicialização, leap, recursão e helper `0x101fe9f0`. Demonstre o papel preciso, orientação dos operandos, passagem por valor, igualdade/inversão, borrow entre anos, retorno e composição. Não confunda com difference escalar `0x10718130` nem corrija a orientação legada. O helper externo continua uma dependência cujo contrato necessário deve ser documentado.

### Nove formatadores e sete helpers de apresentação/interpretação

Caracterize cada formatter inventariado: argumentos, formato curto/longo, presença de weekday/ano/fase, widths/signedness de capacidades, retornos, buffers, NUL e side effects. Há comparações estáticas com 9, 18 e 50: demonstre exclusividade/inclusividade e capacidades mínimas; não importe o limite Windows de 50 nem unifique contratos por conveniência.

Helpers são fase `0x107184d0`/`0x10718570`, classificação/interpretação via linguagem `0x10718650`, weekday `0x10718700`/`0x107187d0` e mês `0x107188a0`/`0x107189d0`. Demonstre abreviações/formas completas, índices, signedness, fases, modos de linguagem, fallback/sentinela, scratches, ponteiros retornados, bytes e ordenação de subcalls. `0x10718650` chama `0x10727880` repetidamente: não assuma parsing genérico nem antecipe leituras observáveis sem prova.

Preserve chamadas/ABI de linguagem, template e strings (`0x10727880`, `0x10727890`, `0x1077dbf0`, `0x107467a0`) e diagnósticos. Recupere a seleção de templates e a ordem de argumentos, incluindo variádicos/stack/TOC, callback e efeitos intermediários. Não hardcode inglês, não normalize acentos, não elimine scratch/diagnósticos nem use formatter padrão como substituto. Assets e textos originais permanecem privados; documente contratos sem publicar payloads/listagens.

Teste cada formatter com datas normais e limites/leap, flags inconsistentes, day inválido, fases/modos demonstrados, slots/tabelas mutáveis, null/short buffers, capacidades em ambos os lados dos thresholds e aliasing entrada/saída/scratch/stack quando definido no original. Compare bytes de saída, todos os efeitos e erros; retorno zero não autoriza descartar diagnóstico. Compare no mesmo ambiente de linguagem e cubra modos disponíveis pertinentes em fixtures isoladas, preservando configurações pessoais; não invente validação de locales ausentes.

### Predicado final do módulo

`0x10718af0` reduz r3 signed8 e r4 signed16, compara mês 9/parâmetro 5..6 e mês 3. Demonstre retornos, CR/XER e high bits contra o Mac. A hipótese de regra transitória da referência Windows não autoriza corrigir competições ou reconstruir o agendador.

## Etapas internas e fechamento

Organize o goal em inventário/contratos e infraestrutura comum; construção/ajuste/leap; semanas/aritmética; fachada de datas/helpers; aceitação integrada. Use checkpoints e testes por rotina. Ordem interna pode acompanhar dependências, sem alterar a conclusão: todo o subsistema acordado deve estar entregue.

Reconcilie este inventário com as funções/destinos reais antes de congelar o manifesto. Se evidência concreta revelar uma entrada do Date omitida, inclua-a no mesmo goal e atualize contagens/testes. Se uma atribuição for equivocada, documente a prova e a fronteira correta. Não reclassifique trabalho difícil para excluir pendências ou declare todos os sistemas consumidores reconstruídos.

## Implementação e composição do bloco

Implemente lógica semântica pura e adapters mínimos no desenho existente. Produção usa as 26 entradas pendentes semânticas, compondo as oito já entregues. Não use wrappers que executem corpos gerados das 26 entradas pendentes em produção normal. Seleção original permanece privada. Não altere corpos vizinhos/callers por busca textual. Hooks ficam nos 26 destinos gerados nas páginas `0x10715`/`0x10716`/`0x10717`/`0x10718`, cobrindo gotos, dispatch entre páginas, indiretas e callbacks. Audite todos os corpos/saídas e manifeste explicitamente as **34 entradas**.

Preserve as fronteiras internas de construção→leap/ajuste e ajuste→weekday/subtração, incluindo LR/contexto, frame/ABI, ordem, efeitos, amostras e streams. Inclua a composição formatadores→helpers/extração e semanas→leap. Examine também chamadas já embutidas nos adapters anteriores: `leap_clobbers` e inicialização de scratch podem reproduzir efeitos sem entrar no dispatcher independente. Resolva essa composição explicitamente. Não injete contadores fictícios ou masque bordas de streams para aparentar cobertura. Ajustar a composição é permitido para preservar contratos anteriores e tornar a nova integração verificável; corrigir os resultados legados não é.

Use counters/amostras e controles de negativo **por entrada**. Campos diagnósticos permanecem fora de saves/snapshots; read tracing desabilitado na produção normal. Um contador agregado ou chamadas de filhos não provam as 26 reconstruções. Documente separadamente eventuais leituras extras de diagnóstico.

Mantenha os modos históricos com seus significados: default reference nenhuma entrada moderna; calendar-baseline add/sub; date-ordering-baseline mais difference; date-before-baseline mais later; date-month-baseline mais before; date-day-of-month-baseline mais month; date-weekday-baseline mais day-of-month com weekday original. Todas as 26 novas entradas devem permanecer originais nesses modos. Acrescente baseline inequívoco como `--date-subsystem-baseline`: oito antigas modernas, 26 novas originais. `--modern` usa as 34 semânticas. Diagnósticos seletivos de cada nova entrada podem isolar problemas de composição, sem precisar repetir todo replay para cada combinação.

Captura CLI e app deve selecionar todas as 34 entradas originais. No baseline oito-modernas/26-originais, selecionar um pai original não pode desativar os filhos que deveriam ser modernos. Examine guards compartilhados de auditoria/recursão. Proíba gravação de candidato como oracle e preserve a instrumentação de dependências não reconstruídas.

## Fixtures, streams e negativos

Use uma **campanha integrada do conjunto**, não 26 campanhas completas. Reexecute os três fixtures imutáveis season-day-01, season-week-01 e date-ordering-capture-02, em `.local/performance/<nome>/SeasonFixture`. Exija dois originais estritos, captura inteiramente original, seis baselines históricos mais o novo baseline entregue e dois candidatos com 34 entradas. Só instruções traduzidas podem diferir; PC/registros/temporários/memória/alocador/imports/callbacks/RNG e EOF continuam estritos e delimitados pelo transcript original.

Registre cada nova entrada em stream privado independente ou num novo formato multiplexado explicitamente versionado, mantendo identidade/inputs/outputs/ABI/efeitos e negativa isolável por entrada. Não altere CMCAL002/CMMON001/CMDOM001/CMWKD001. Em destinos novos, a recaptura inteiramente original dos quatro deve ser byte-idêntica a `.local/date-weekday-reconstruction/{day,week,ordering}-edges-01`, com hashes no relatório entregue. Amplie o checker para comparar explicitamente os quatro, preservando compatibilidade dos usos anteriores.

Na preparação anterior, CMWKD001 originais tinham 1.212 / 2.491 / 943 bordas de weekday com LR=`0x10715a58`, caminho original do ajuste. Isso é pista concreta para um dos novos pais; não estabelece coverage própria de todas as 26 entradas pendentes. Faça matriz por entrada × fixture com contagens e modo. Zero é ausência de cobertura, nunca aprovação.

Mantenha negativos RNG/shifts/difference/later/before/month/day-of-month/weekday onde cobertos. Para cada nova entrada, altere somente seu primeiro resultado observável e exija rejeição na primeira fronteira própria com entry/field/offset, antes da divergência posterior. Para pais e filhos, não perturbe o filho para simular negativo do pai. Cubra headers/bodies truncados, checksum, EOF, bordas não consumidas, original-record enforcement e reset da perturbação.

Use fixtures adicionais somente para lacunas concretas de coverage. Uma captura inteiramente original pode atender várias entradas simultaneamente; não crie uma carreira independente por entrada nem force dates/RNG/snapshots para atingir branches. Preserve todo histórico. Demonstre caminhos válidos reais para as 26 entradas pendentes e delimite os demais ramos pelos testes próprios. Para entradas raras, selecione callers/fluxos pertinentes de notícias/painéis/fixtures, modos de apresentação/linguagem disponíveis ou criação de nova carreira isolada se necessário. Chamadas diretas artificiais são prova de contrato e integração sintética, nunca gameplay; zero chamadas reais exige resolver a lacuna ou reportar o goal incompleto.

## Aplicativo real e persistência do conjunto

Use uma única instância ARM64 por vez, candidato/Runtime/Runs novos preparados pelas ferramentas existentes. Não sobrescreva `date-weekday-live-01` nem candidatos antigos. O checkpoint preferido é `.local/performance/date-weekday-live-01/Runtime/Files/games/DateWeekday-03Sep2002.sav`, SHA-256 reconferido `24bc76c79e4ebba79b5725fe01dc28181590aaa59ce2127257af8708b28d25ed`. Alternativa preservada: `DateWeekday-01Sep2002.sav` na mesma pasta, SHA-256 `bbbc4426afdfd23569d5a0784da64c0da4a46d5088a702630e022d860a704351`. Preserve ambos e todos os checkpoints anteriores.

O checkpoint preferido foi observado como Tuesday September 3 morning (`3.9.02 MNH`), Victor Moraes/Cruzeiro, na notícia de proposta do Grêmio por Abedi. Confirme isso no candidato novo. O calendário observado tinha Goias em August 31 0:3 e Coritiba/Santos/Colo Colo pendentes em September 6/8/11; Cruzeiro 16º nacional. Luisao: nascimento February 13, 1981, idade 21, uma internacionalização, condição 89%, 26 (4) jogos oficiais, um gol, duas assistências, média 7.20, forma 8-7-9-8-6, moral Good. Contrato full-time, BRL 8.750/semana, início N/D, vinculado Sim, expira December 24, 2004, rotação, sem prêmios/cláusulas. São observações históricas a reconferir, não asserts cegos sobre o candidato.

Guie a interação pelos callers/amostras: Load e preparação de datas de fixtures já exercitaram o caminho original do ajuste. Demonstre chamadas **semânticas próprias das 26 entradas pendentes** no aplicativo e/ou fixtures capturados de gameplay original, guiadas pelos respectivos callers, com inputs/output/caller auditáveis na interação normal. Opening calendar isoladamente não basta. Confronte amostras no domínio válido com cálculo independente de construção civil/ordinal, leap flag e weekday/data escolhida, e preserve outputs/flags inválidos conforme o original. Confira também textos e fases nos painéis/notícias contra o original. Registre resultados iguais, avanços/recuos curtos e caller real que ocorrerem; ramos restantes devem ficar delimitados pela prova direta/fixtures; não atribua a todas as entradas um contador agregado. Não manipule datas ou saves para forçar ramos da UI.

Valide Original Game Options → Save Game, saída normal, processo ausente, reabertura exata e Original Load antes e depois de uma progressão normal limitada que exercite o conjunto. Use novos nomes de save e documente a data/fase efetivamente alcançada. Se a proposta pendente exigir decisão, siga a interface normal e registre a escolha; não force transferência, partida, date boundary ou RNG. Não imponha nova temporada, virada mensal/anual ou um dia exato. Se houver partida no intervalo pertinente, use o fluxo normal e pare após a evidência necessária.

Após cada Load confira data/weekday/fase, clube, fixtures/resultados e os dados de jogador/contrato observados imediatamente antes do respectivo Save. Diferencie mudanças normais de condição/estatísticas das perdas de persistência. File → Save nativo continua limitado por `Native menu selections require original MenuSelect dispatch` e fora deste escopo. Recrie bindings CUA e índices AX após reabertura; confira ausência de processo antes de lançar outro app. Se o Mac estiver bloqueado, documente o bloqueio sem tratar testes automáticos como prova de aplicativo.

## Execução eficiente e critérios de conclusão

Prepare o mapa de contratos/dependências das 26 entradas pendentes antes de estabilizar State/headers; faça implementação e testes diretos incrementalmente. Estabilize todos os campos diagnósticos necessários juntos. Faça o rebuild completo do layout final, campanha de fixtures, suíte completa/UBSAN e validação real **como aceitação integrada do bloco**. Não repita full clean build + suítes caras + Save/Load + instalação depois de cada rotina. Repita só os checks afetados enquanto desenvolve; amplie/reexecute o necessário quando novas mudanças, falhas ou preocupações concretas justificarem.

Isso reduz trabalho repetido, sem dispensar caracterização, negativos, coverage ou equivalência de cada entrada. Não remova verificações/EOF, não use stale archives, não force aceite de uma entrada parcialmente reconstruída e não introduza otimizações para baratear o teste.

Depois de estabilizar fonte/build, confirme layout de State em todos os objetos realmente ligados e prepare um candidato final novo. Rode novo(s) harness(es) direto(s) das 26 entradas pendentes, regressões das oito anteriores, auditorias e suíte completa. UBSAN deve instrumentar os adapters/gerador alterados, harnesses e páginas pertinentes; use a receita do relatório de weekday como base, incluindo `0x10715`/`0x10716`/`0x10717`/`0x10718` e demais páginas efetivamente modificadas. Não alegue bridges/whole-program/ASan sem execução correspondente.

Comandos existentes a reconferir/ampliar com harness, streams e baseline do bloco:

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

Use novos destinos privados nos replays e a validação histórica ampliada. Documente comandos finais repetíveis, grupos pulados versus explicitamente executados, casos e counts por entrada, cobertura sintética/automática/real e limites. Não confunda resultados históricos com aprovação do bloco atual.

Só conclua com **as 26 entradas pendentes** semanticamente em produção, contratos/ABI/efeitos demonstrados, composição integrada, negativos próprios, quatro streams históricos intactos, chamadas reais próprias, dois ciclos de Save/exit/reopen/Load em uma progressão normal limitada, suíte final/UBSAN, preservação e instalação aprovados. Se uma condição essencial bloquear, documente o fato e mantenha a entrega incompleta; não encerre cedo por conveniência nem substitua o objetivo por mais um prompt.

## Proteção, documentação e entrega única

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, forks/Finder metadata, todos os xattrs, modos/mtime/flags. Faça inventários atuais before/preinstall/after e uma nova referência executável do instalado atual. Backups antigos têm identidades diferentes e continuam imutáveis. Não assuma contagens 809/5/43 como invariantes.

Todos os exports/dados originais, saves novos, snapshots/transcripts/logs/capturas/builds ficam privados e ignorados. Não mude ignore/LFS. Preserve `saves/Mac-Teste-Validado-Pos-Jogo.sav`, 17.917.514 bytes, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`; históricos e os seis prompts pendentes anteriores, incluindo os handoffs de ajuste isolado e construção/ajuste que este goal substitui. Este sétimo handoff também deve ser preservado.

Escreva `docs/native-date-subsystem-reconstruction.md` em inglês, com tabela de contratos/retornos/ABI/efeitos por entrada, mapa de composição, casos/negativos/counts, coverage e observações exatas da UI/persistência. Inclua manifesto de fechamento das 34 entradas e contratos das dependências ainda geradas; não reivindique reconstrução do Langlib ou agendador. Atualize `docs/native-port-status.md`, preservando relatórios anteriores e a melhoria bissexta adiada. Registre hashes separados de source/candidato assinado/installed/nova referência e inventários completos.

Instale o conjunto ARM64 aprovado com `tools/install_native.py` uma vez ao fim da aceitação, preservando dados pessoais. Revise o diff exato, exclua prompts pendentes e artefatos privados do commit de produto, faça commit/push normal sem force conforme a autorização permanente de trabalho concluído/validado e confira SHA remoto por consulta nova. Commits intermediários coerentes podem organizar o trabalho, mas não equivalem à entrega do goal. Preparar este handoff não autoriza instalar/publicar agora, lançar chat novo ou publicar dados privados.
