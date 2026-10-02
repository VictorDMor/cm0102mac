Reconstruir semanticamente a comparação independente entre datas do CM 01/02 em C++23, integrar a substituição no aplicativo ARM64 e demonstrar equivalência com a rotina original, preservando o comportamento do jogo.

Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e mensagens novas do aplicativo em inglês. Quando este prompt for enviado como solicitação de implementação, execute o incremento até a entrega validada. A criação deste handoff é planejamento: não aprova nem implementa o novo módulo.

## Direção e referência entregue

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon. Substitua o C++ gerado por módulos semânticos claros em incrementos verificáveis, preservando os contratos originais. Este incremento abrange uma única entrada independente de ordenação de datas, candidata `0x10331860`. Não reconstrua agora outras comparações inlined, conversão de data para mês/dia da semana, diferenças de idade em registros, agendador, fixtures, competições, jogadores, transferências ou contratos. Otimizações, features e alterações gráficas continuam suspensas. Não prometa melhoria de velocidade sem benchmark comparável.

O usuário adiou expressamente a correção da peculiaridade bissexta da diferença de datas. Preserve-a: no percurso ascendente a diferença usa os comprimentos de first.year-1 a second.year-2; January 1 2000 menos January 1 2001 retorna -365, e a inversão retorna +366. Corrigir essa regra exige análise dos callers e decisão separada; não pertence a este incremento.

Na preparação deste handoff, `main` e uma consulta nova ao remoto concordaram em `18d2001226b405cc82f07368f8ad1e8d10f31990`. A implementação anterior foi entregue em `22cfe9bd6aa2f5d95a5aea8fc0a9e620b846fc28`; o commit seguinte apenas documentou a melhoria adiada. O arquivo preexistente `docs/features/native-date-difference-reconstruction-prompt.md` estava untracked e deve ser preservado. Este handoff é uma nova alteração de planejamento. Reconfira branch, HEAD, remoto, diff e `AGENTS.md` aplicáveis antes de implementar. Preserve o trabalho anterior de desempenho e alterações alheias.

Adição `0x10717890`, subtração `0x10717b20` e diferença `0x10718130` já usam `native/calendar.hpp` e `native/calendar_adapter.cpp` em produção. Não repita nem desfaça sua reconstrução. A diferença não delega ao original em operação normal. O offset -32768 das operações de deslocamento mantém exclusivamente o comportamento original não terminante; nenhuma chamada excepcional ocorreu nos fixtures e no avanço real aceitos.

Leia os relatórios anteriores como evidência histórica, não como validação do novo módulo. `docs/native-date-difference-reconstruction.md` registra 6.892 casos diretos, 494 checks de oracle independente em domínio aplicável, 31/31 grupos CTest, 25 testes Python com quatro grupos opcionais executados explicitamente, auditoria de temporários e UBSAN. Os fixtures privados de um e sete dias preservaram a comparação estrita original-contra-original, o baseline anterior, duas execuções modernas e controles negativos de RNG, deslocamento e diferença. A única diferença permitida no estado moderno é a contagem de instruções traduzidas; não amplie essa fronteira com máscaras gerais ou contagens fabricadas.

O aplicativo instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app/Contents/MacOS/CM 01-02 Native Prototype` foi reidentificado neste planejamento com SHA-256 `ca725a106fade1354e6e774364543b86dbf1eb9baa930c2ab55e096d72da0e61`. O relatório anterior identifica o mesmo hash para o fonte e `56eae291b951c560d698447bc9468cb5586e81248fb5832ee208ee96b182eb4f` para a cópia isolada assinada. Reconfira todas as identidades e preserve uma cópia privada executável da referência atual antes de trocar builds ou instalar candidato. O backup do incremento ainda anterior permanece distinto sob `.local/date-difference-reconstruction/reference/installed.app`, hash `dbe6a1f963bb33a702bbbdc29fce17d2e5f75d822da7d09f7fc66246641ad741`.

## Inspeção e contrato

Leia `README.md`, `docs/native-port-status.md`, `docs/native-calendar-reconstruction.md`, `docs/native-date-difference-reconstruction.md`, `docs/native-season-reference.md`, `native/calendar*`, `native/date_difference_checks.cpp`, `native/callbacks.cpp`, `native/aot_runtime.hpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*`, `native/CMakeLists.txt`, `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`.

O índice privado `.local/recomp-input/instructions.sqlite` contém 26 instruções de `0x10331860` até o retorno `0x103318c4` inclusive. Há três chamadas diretas `bl` no export atual: `0x10331794` na mesma página, `0x10332c24` em outra página e `0x10717f50`. A rotina faz spills de r4/r5 em r1+28/r1+32, lê anos com `lhz`, dias com `lha` e retorna por r3. A inspeção sugere um predicado estrito sobre a segunda data ser posterior à primeira, não uma comparação ternária. Esses achados são orientação, não um contrato diferencial estabelecido.

Fingerprint observado com a seleção de campos usada em `tools/calendar_audit.py`, intervalo exclusivo `[0x10331860,0x103318c8)`: `5a74c4775a352a92e713d648900ade2eae03254c96bb84200d5b2378f24b29d2`. Reconfirme procedência do `program.json`, limites, convenção e callers. O export privado não existe em clone público limpo; não publique suas instruções ou dados.

Determine ordem dos operandos, sentido estrito ou inclusivo, largura e representação do retorno em igualdade e inversão. Não assuma segundo ponteiro: r4/r5 parecem transportar a segunda data por valor, incluindo três bytes adicionais. Caracterize leitura de flags/padding, spills, resíduos e acessos ordenados, clobbers de todos os registradores, CR/XER/LR/CTR/TOC e temporários. Verifique imports, RNG e ausência ou presença de scratch/inicialização; não herde esses efeitos das outras entradas sem evidência. A restauração de `call_original` não prova os clobbers de uma chamada direta.

Cubra igualdade, ambas as ordens, limites de mês/ano, February 28/29, 1900/2000/2100, anos 0/65535, extremos de dias signed16, dias inválidos/negativos, leap flags inconsistentes e não binárias, padding variável e aliasing pertinente com os próprios spills. Varie flags iniciais de condição e carry. Use oracle independente de ordenação apenas no domínio confirmado; não normalize datas nem corrija o original. Delimite explicitamente entradas não comprovadas.

## Implementação e comparação

Amplie a arquitetura mínima existente: operação semântica pura independente de memória/ABI e adaptador dos efeitos legados. Evite generalização especulativa e preserve as três operações já entregues. A entrada de produção deve executar C++ semântico, com original acessível apenas em diagnósticos privados, sem nova opção de UI. Acrescente auditoria que rejeite export divergente ou fragmento incompleto.

Integre o hook no destino gerado da página `0x10331`, alcançando a chamada real na mesma página, chamadas entre páginas, indiretas e callbacks. Teste os caminhos reais quando disponíveis e identifique separadamente testes estruturais de geração e chamadas sintéticas. Não substitua por busca textual as comparações embutidas nas outras rotinas, inclusive na diferença já entregue.

Compare original e moderno partindo de estados equivalentes: retorno, entradas, registradores inteiros, temporários, PC, sequência e footprint de escritas, memória inteira, stack, alocador, imports/callbacks e RNG. Use orçamento finito. Crie contadores e amostras auditáveis próprios da comparação; os contadores de adição/subtração/diferença não provam sua execução.

Preserve três modos claramente separados: inteiramente original; baseline entregue com adição/subtração/diferença modernas e comparação original; candidato com as quatro operações modernas. O atual `--calendar-baseline` representa um baseline histórico diferente, com diferença original; preserve ou renomeie essa semântica explicitamente, sem reutilizar seu nome silenciosamente para outro contrato. Não compare candidatos que diferem em outros subsistemas.

Reexecute os fixtures existentes de um e sete dias com a fronteira estrita original mantida. Acrescente stream original privado da nova entrada, hashes antes/depois, duas execuções candidatas e um controle negativo isolado da comparação: inverta um retorno booleano e exija detecção na primeira fronteira relevante. Preserve os negativos de RNG, deslocamento e diferença. Entrega sem perturbação; injeção apenas na validação privada. Não recapture o moderno como oracle.

Confirme contagens reais da nova entrada. Se algum fixture existente não a exercitar, reporte zero sem fabricar cobertura e capture sob o modo original uma sequência real mínima que a exercite, com estado/transcript final e orçamento delimitados; compare esse mesmo fixture no baseline e candidato. Não altere os fixtures anteriores e não exija replay de carreira completa.

## Aplicativo e aceitação

Use uma única instância ARM64 por vez, em cópia privada dos dados graváveis preparada pelas ferramentas existentes. Checkpoints adequados disponíveis e reidentificados neste planejamento sob `.local/performance/date-difference-live-01/Runtime/Files/games/`:

- `DateDiff-19Jul2002.sav`: `cb0700048651d2d23bd6b457dd5f6d2b4cfbefd3587cc7ddc8e932c196d7ae7e`.
- `DateDiff-03Aug2002.sav`: `256dca40fbb2ab53fc3543bdd7a8fb0c8c6ba1ebcb80e411b3f333642c3b67ae`.

Reconfirme disponibilidade/hashes antes de usar. Preserve também os checkpoints anteriores de 19 July 2002 e 5 February 2003 sob `.local/performance/calendar-live-02/Runtime/Files/games/`. A validação anterior avançou normalmente de 19 July night a 3 August afternoon, com 500.953 diferenças modernas, calendário/jogador/contrato e Save/saída normal/reabertura/Load conferidos. Isso não prova execução da comparação independente.

Demonstre chamadas reais modernas da nova entrada com argumentos/resultados auditáveis, sem fallback normal. Use os callers confirmados para escolher a ação pertinente no jogo. Abra calendário e informações de jogador/contrato úteis, avance normalmente por intervalo delimitado e valide Save, saída normal, reabertura e Load do candidato. Não edite datas, saves, sementes ou memória para forçar cobertura. Se a chamada real não ocorrer, investigue outro percurso normal pertinente e não declare conclusão sem essa evidência. Não é necessário concluir outra temporada nem repetir passagem anual; testes diretos devem cobrir anos/century boundaries.

Execute testes pertinentes e suíte completa no candidato final. Inclua UBSAN do adapter e auditoria de temporários quando modificar gerador/adapter. Verifique que todos os objetos ligados usam o layout final de `State`; cache só quando consistente. ASan não foi validado pelo incremento anterior; não transforme a limitação histórica de inicialização em aprovação atual.

Comandos base, a confirmar e ampliar com o novo grupo:

```sh
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/check_calendar_season.py .local/performance/season-day-01/SeasonFixture
python3 tools/check_calendar_season.py .local/performance/season-week-01/SeasonFixture
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

Só conclua com contrato caracterizado, comparação semântica usada em produção, equivalência direta e delimitada, negativos preservados, chamada real da nova entrada, persistência real conferida e suíte final aprovada. Diferencie equivalência automática, observação de UI, inferência e limites. Registre bloqueio essencial sem declarar conclusão.

## Proteção e entrega

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks, Finder metadata e alterações existentes. Exports, dados, novos saves, snapshots, transcripts, logs, capturas e builds continuam privados e ignorados. Não altere ignore/LFS. Preserve imutável `saves/Mac-Teste-Validado-Pos-Jogo.sav`, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.

Documente em inglês contrato, arquitetura, hooks, fronteiras de comparação, comandos repetíveis, contagens/amostras reais, negativos, validação no app e limitações. Atualize `docs/native-port-status.md`; mantenha relatórios anteriores como evidência histórica e a correção bissexta adiada. Registre separadamente hashes do fonte, candidato assinado e instalado.

Ao executar este prompt como implementação, entregue instalação ARM64 validada preservando dados, backup privado executável e inventários de conteúdo/metadados antes/depois. Revise o diff exato, faça commit e push normal sem force conforme a autorização permanente para trabalho concluído e validado neste projeto, e confira o SHA remoto por consulta nova. Não publique artefatos privados nem declare reconstrução do restante do engine ou melhoria de velocidade sem evidência.
