Reconstruir semanticamente o predicado independente “segunda data anterior à primeira” do CM 01/02 em C++23, integrar sua entrada no aplicativo ARM64 e demonstrar equivalência com o original, preservando o comportamento do jogo.

Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês. Quando este prompt for enviado como solicitação de implementação, execute o incremento até a entrega validada. A criação deste handoff é planejamento: não implementa o predicado nem inicia outra sessão.

## Direção e incremento escolhido

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon. A direção continua sendo substituir C++ gerado por módulos semânticos claros, preservando contratos originais em incrementos verificáveis. Adição, subtração, diferença e o predicado independente “segunda data posterior à primeira” já estão entregues. Não repita essas reconstruções.

Este incremento abrange somente a entrada complementar candidata `0x103317f0`. Ela permanece gerada e participa dos mesmos comparadores que usam o predicado posterior já entregue. O objetivo é substituir essa entrada por uma operação semântica com seu próprio contrato demonstrado. Não declare que todas as comparações de datas do engine foram reconstruídas. Conversão para mês/dia da semana, comparação ternária dos callers, diferenças de idade em registros, agendador, fixtures, competições, premiações, jogadores, transferências e contratos continuam fora do escopo. A lista de contratos é uma rota de validação, não autorização para reconstruir o subsistema de contratos. Otimizações, features e alterações gráficas continuam suspensas; não prometa ganho de velocidade sem benchmark comparável.

O usuário adiou expressamente a correção da peculiaridade bissexta da diferença. Preserve-a: o percurso ascendente usa comprimentos de first.year-1 até second.year-2; January 1 2000 menos January 1 2001 retorna -365, e a inversão +366. A regra de identificação de ano bissexto não é o problema. Uma correção dos anos percorridos mudaria o comportamento original e exige análise dos callers e decisão separada. Leia a seção deferred improvement do relatório da diferença; não implemente essa melhoria agora.

## Referência e estado de partida

Na preparação deste handoff, `main`, HEAD e consulta nova ao remoto concordaram em `6f625a94c31729852914c3ddf2dc0c87dc5cf1d0`, entrega da ordenação posterior. O arquivo preexistente `docs/features/native-date-difference-reconstruction-prompt.md` estava untracked e deve ser preservado. Este handoff é uma nova alteração de planejamento. Reconfira branch, HEAD, remoto, diff e todos os `AGENTS.md` aplicáveis. Preserve alterações alheias e o trabalho anterior de desempenho.

As entradas `0x10717890` (adição), `0x10717b20` (subtração), `0x10718130` (diferença) e `0x10331860` (segunda data posterior) já usam `native/calendar.hpp` e `native/calendar_adapter.cpp` em produção. Os hooks nos destinos gerados cobrem chamadas na mesma página, despacho entre páginas, indiretas e callbacks. As operações normais entregues executam C++ semântico. O offset -32768 dos deslocamentos preserva exclusivamente o comportamento original não terminante; não houve chamada excepcional nas execuções aceitas. Não desfaça esses contratos.

Leia os relatórios como evidência histórica, não como aprovação do novo incremento. `docs/native-date-ordering-reconstruction.md` registra 5.646 comparações diretas, 156 casos de oracle Gregorian em domínio válido, 196.608 verificações puras de dias signed16, 32/32 grupos CTest, 26 testes Python com quatro grupos opcionais executados explicitamente, auditoria de temporários e UBSAN. Os antigos fixtures de um e sete dias passaram e registraram zero chamadas do predicado posterior. Um novo fixture original de 20–21 July 2002 morning exercitou 38.051 chamadas modernas dessa entrada em cada uma de duas execuções equivalentes, com negativo isolado detectado na primeira fronteira pertinente. Essas contagens não demonstram cobertura moderna de `0x103317f0`.

A UI anterior avançou normalmente de 3 a 10 August 2002 afternoon em cópia privada. Ordenar Contract Expiry na busca de jogadores registrou 38.422 chamadas modernas da entrada posterior e 32 amostras auditáveis. Calendário, jogador/contrato e Save/saída normal/reabertura/Load foram conferidos. Isso não valida o novo predicado anterior nem equivale a replay original da interação completa na UI.

Identidades rechecadas neste planejamento:

- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`: executável ARM64, SHA-256 `b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545`.
- Candidato privado assinado `.local/performance/date-ordering-live-01/CM Validation date-ordering-live-01.app`: executável `01395e5f2e345635f414390dc89d8816103ef68dc5af9490a4ee538fd46a227c`.
- Backup do instalado anterior `.local/date-ordering-reconstruction/reference/installed.app`: executável `ca725a106fade1354e6e774364543b86dbf1eb9baa930c2ab55e096d72da0e61`.

Reconfira identidades antes de agir e preserve uma nova cópia privada executável da referência atual antes de substituir builds ou instalar candidato. Não confunda os backups históricos com a referência atual nem substitua esta por uma cópia diagnóstica assinada sem distinguir hashes.

## Inspeção e contrato a demonstrar

Leia `README.md`, `docs/native-port-status.md`, `docs/native-calendar-reconstruction.md`, `docs/native-date-difference-reconstruction.md`, `docs/native-date-ordering-reconstruction.md`, `docs/native-season-reference.md`, `native/calendar*`, `native/date_difference_checks.cpp`, `native/date_ordering_checks.cpp`, `native/callbacks.cpp`, `native/aot_runtime.hpp`, `native/aot_probe.cpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*`, `native/CMakeLists.txt`, `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`.

O índice privado `.local/recomp-input/instructions.sqlite` contém 26 instruções da candidata `0x103317f0` até o retorno `0x10331854` inclusive. O intervalo exclusivo é `[0x103317f0,0x10331858)`. Há duas chamadas diretas `bl`: `0x103317b8` na mesma página e `0x10332c48` em outra página. Ambos os callers consultam primeiro `0x10331860`; se seu resultado for falso, consultam a candidata anterior antes de seu desempate. O caller `0x10332c48` é adjacente à rota usada anteriormente pela ordenação de contratos. Isso orienta a investigação e a escolha da ação real; não prova que a nova entrada já foi exercitada ou reconstruída.

Fingerprint observado, usando exatamente os campos de `tools/calendar_audit.py`: `e93ed9feee78922e44a69beafbc099f515429b887d7ead794132f8bb59a1f18b`. A assembly observada guarda r4/r5 em r1+28/r1+32, compara anos com loads unsigned e dias com loads signed e retorna por r3. A inspeção sugere retorno booleano estrito para segunda data anterior; estabeleça-o diferencialmente. A rotina é parecida com a entrada posterior, mas a ordem de carregamento dos dias e os clobbers diferem: não reutilize seu adapter sem caracterização própria.

Proveniência rechecada dos arquivos privados: SHA-256 do índice `2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec`; `program.json` `50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0`. Metadata do índice identifica 1.953.157 instruções e PEF original `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`. Esses exports não existem em clone público limpo. Reconfirme entradas, limites, fluxos, callers e convenções; não publique instruções ou payloads privados.

Determine ordem dos operandos, igualdade, ambas as ordens, largura/representação do retorno e signedness de ano/dia. Caracterize os cinco bytes big-endian e o segundo operando passado por valor em r4/r5, incluindo os três bytes de padding. Não trate r4 como segundo ponteiro. Preserve spills completos, leituras/escritas ordenadas, frame/resíduos de pilha e aliasing com os próprios spills. Verifique clobbers de todos os registradores, CR/XER/LR/CTR/TOC, TOC de contexto, temporários, PC, memória, alocador e ausência ou presença de scratch/inicialização/imports/RNG. Não herde ausência de efeitos da entrada posterior sem evidência. Restauração por `call_original` não prova clobbers de chamada direta.

Cubra igualdade, um dia em ambos os sentidos, fronteiras de mês/ano, February 28/29, 1900/2000/2100, anos 0/65535 e extremos unsigned16, dias negativos/inválidos e extremos signed16, flags bissextas inconsistentes/não binárias, padding variável, flags de condição/carry iniciais e aliasing pertinente. Demonstre também casos em que posterior e anterior são ambos falsos por igualdade e casos em que o caller passa do primeiro para o segundo predicado. Use oracle independente em domínio confirmado e campos efetivos depois dos spills. Não normalize datas nem introduza regras corrigidas. Delimite inputs não comprovados e falhas de memória sem ampliar a conclusão.

## Implementação e comparação automática

Amplie a arquitetura mínima existente: lógica semântica pura independente da memória/ABI e adapter dos efeitos legados. Reutilize o modelo de ordenação confirmado quando apropriado, sem generalização especulativa nem duplicação gratuita. Preserve os quatro contratos entregues. A nova entrada de produção deve executar C++ semântico, com seleção original apenas em diagnósticos privados e nenhuma nova opção de UI. Acrescente auditoria para rejeitar exports divergentes e fragmentos incompletos.

Integre o hook no destino gerado da página `0x10331`, garantindo que o `goto` do caller na mesma página alcance o adapter. Teste os dois callers reais com estados controlados, despacho entre páginas, indiretas e callbacks. Diferencie testes estruturais do gerador, testes de caller com inputs sintéticos e observação de execução real no aplicativo. Não substitua outros comparadores ou trechos inlined por busca textual.

Compare original e moderno desde estados equivalentes: retorno, entradas, registradores inteiros, temporários, PC, footprint/ordem das escritas, memória inteira, stack, flags, alocador, imports/callbacks e RNG. Use orçamento finito. Acrescente contadores e amostras próprios da entrada anterior; um total agregado de ordering não distingue as duas operações. Amostras devem identificar entrada, operandos, resultado e caller/retorno. Não use contadores das operações anteriores como prova da nova.

Preserve modos separados: inteiramente original; baseline entregue com as quatro operações anteriores modernas e nova entrada original; candidato com as cinco modernas. O atual `--calendar-baseline` mantém diferença e posterior originais; `--date-ordering-baseline` mantém posterior original, com deslocamentos/diferença modernos. Preserve explicitamente essas semânticas históricas. Não redefina nomes silenciosamente nem compare candidatos que diferem em outros subsistemas. Garanta que capturas originais selecionem todas as cinco entradas originais, incluindo a nova, sem recapturar o candidato como oracle.

Reexecute os fixtures imutáveis existentes de um e sete dias e `.local/performance/date-ordering-capture-02/SeasonFixture`: comparação estrita original-contra-original, baseline entregue, duas execuções candidatas e baselines históricos pertinentes. Preserve somente a fronteira existente que admite diferença na contagem de instruções traduzidas. PC, registradores, temporários, bytes mapeados, alocador, imports/callbacks e RNG permanecem estritos. Nenhuma máscara geral ou contagem fabricada. Mantenha término por estado/transcript e orçamento finito.

Acrescente stream privado original da nova entrada e negativo isolado que inverte seu primeiro retorno moderno. Exija detecção na primeira fronteira pertinente da entrada anterior, mantendo os negativos de RNG, deslocamento, diferença e posterior. Entrega sem perturbação; ativação apenas em validação privada. Confirme hashes dos fixtures e streams antes/depois. Uma nova versão de stream que inclui a entrada adicional deve ficar separada dos streams históricos, sem substituí-los ou comparar seus hashes como se fossem o mesmo formato.

O fixture de ordenação original foi encontrado neste planejamento; seus arquivos têm hashes:

- `capture.txt`: `cf456ebc61bdbbe0e3352231dc76c508ebb5afffa2c396dbe141085365629dc0`.
- `initial.snapshot`: `f06ed20d5a7088601edb628f195a75b8f359932d672b2c96f5c8c6770beb96d9`.
- `expected.snapshot`: `63bba904650e217b35ade05f11cafb0878c12bc970289a70361996454416507f`.
- `imports.gz`: `fcf8f0f4cbf14b521f7189e2aea882f0e8b966bc044f658bd34bbcdf1ac4b853`.

Não presuma que esse fixture exercita a nova entrada. Reporte contagens reais por entrada; zero é ausência de cobertura. Se nenhum fixture a exercitar, capture sob modo inteiramente original uma sequência mínima real que a exercite, com estado/transcript final e orçamento delimitados. Compare esse mesmo fixture nos modos original, baseline e candidato, preservando os fixtures anteriores. Não exija replay de carreira completa.

## Aplicativo real e testes finais

Use uma única instância ARM64 por vez, em cópia privada dos dados graváveis preparada pelas ferramentas existentes. Não lance aplicativo durante a criação deste handoff. Checkpoints reidentificados e disponíveis sob `.local/performance/date-ordering-live-01/Runtime/Files/games/`:

- `DateOrder-03Aug2002.sav`: `327e5ad2f18a680cbb5a88bd3777523bdf262db3462d0785e10987a314077400`.
- `DateOrder-10Aug2002.sav`: `3fc8a13c6a14878b3495e139da3bcfd2c3c2356237ac9d625b462df0c8217334`.

Reconfirme disponibilidade e hashes antes de reutilizar; preserve também os checkpoints anteriores de calendário e diferença. Demonstre chamadas reais modernas da entrada anterior com argumentos/resultados auditáveis e sem fallback normal. A rota anterior foi manager → Player & Staff Search → Search Player → View Contract → Contract Expiry; use os callers e logs para confirmar a cobertura nova, sem pressupor sentido de ordenação ou necessidade de clique reverso. Se a rota não a exercitar, investigue outra interação normal pertinente e não declare conclusão sem chamada real.

Abra calendário e informações úteis de jogador/contrato, avance normalmente por intervalo delimitado, e confira Save, saída normal, reabertura e Load do candidato, preservando o estado observado. Não edite datas, saves, sementes ou memória para forçar avanço/cobertura. Não é necessário repetir passagem anual ou concluir outra temporada; anos/century boundaries têm validação direta. Relate somente datas e ações realmente observadas e separe observação de UI da equivalência automática dos fixtures.

Execute testes pertinentes e suíte completa no candidato final, incluindo a nova comparação direta, regressões dos quatro contratos, auditoria de temporários e UBSAN do gerador/adapter alterados. Verifique que todos os objetos ligados usam o layout final de `State`; recompile completamente quando houver inconsistência, usando cache somente quando consistente. Não introduza otimizações para acelerar este trabalho. Limitações históricas de ASan não constituem aprovação atual.

Comandos base, a confirmar e ampliar com o novo grupo:

```sh
cmake --build .local/aot-build -j 4
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

Só conclua com contrato caracterizado, operação semântica em produção, equivalência direta e delimitada, negativos preservados, cobertura real própria da nova entrada, persistência no app conferida e suíte final aprovada. Diferencie equivalência automática, observação de UI, inferência e limites. Se houver bloqueio essencial, documente-o sem declarar objetivo concluído.

## Proteção e entrega

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks, Finder metadata e alterações existentes. Todos os exports, dados, novos saves, snapshots, transcripts, logs, capturas e builds continuam privados e ignorados. Não altere regras de ignore/LFS. Preserve imutável `saves/Mac-Teste-Validado-Pos-Jogo.sav`, reidentificado neste planejamento com SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.

Documente em inglês contrato, arquitetura, integração, fronteiras de comparação, comandos repetíveis, chamadas reais por entrada, controles negativos, observação no app e limites. Atualize `docs/native-port-status.md`; preserve relatórios anteriores como evidência histórica e mantenha a correção bissexta adiada. Registre separadamente hashes do fonte, candidato assinado e instalado.

Ao executar este prompt como implementação, entregue instalação ARM64 validada, backup privado executável da referência atual e inventários de conteúdo/metadados antes/depois. Revise o diff exato, faça commit e push normal sem force conforme a autorização permanente para trabalho concluído e validado neste projeto, e confira o SHA remoto por consulta nova. Não publique artefatos privados nem declare reconstrução do restante do engine ou melhoria de velocidade sem evidência.
