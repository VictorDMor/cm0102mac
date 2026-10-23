/goal Reconstruir semanticamente a diferença entre datas do CM 01/02 em C++23, integrar a substituição no aplicativo ARM64 e demonstrar equivalência com a rotina original, preservando o comportamento do jogo.

Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, testes, documentação técnica e mensagens do aplicativo em inglês. Execute este incremento até a entrega validada quando este prompt for enviado como solicitação de implementação. A criação deste handoff é planejamento; não significa que o novo subsistema já foi aprovado, implementado ou validado.

## Direção e ponto de partida

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon. A direção é substituir o C++ gerado por módulos semânticos claros, preservando o comportamento original por incrementos verificáveis. Este próximo incremento proposto é somente a diferença entre duas datas; a comparação embutida nessa operação pertence ao seu contrato. Não reconstrua agora comparação independente, conversão para mês/dia da semana, agendador, fixtures, rollover de competições, premiações, desenvolvimento de jogadores, transferências ou contratos. Novas otimizações, features e alterações gráficas continuam suspensas. Não prometa ganho de velocidade: o incremento anterior reduziu instruções traduzidas, mas não teve benchmark de tempo comparável.

A referência entregue está em `main`, commit `4c73274bae69b534b478ff5d42f6c1feb4d72b77`, com push normal e SHA remoto conferido. Na preparação deste handoff o repositório estava limpo; este arquivo é a nova alteração de planejamento. Confira novamente branch, HEAD, remoto, diff e instruções `AGENTS.md` aplicáveis. Preserve alterações existentes e o trabalho anterior de desempenho.

A adição `0x10717890` e a subtração `0x10717b20` já usam `native/calendar.hpp` e `native/calendar_adapter.cpp` no aplicativo. As entradas geradas interceptam chamadas diretas, indiretas e callbacks. O original permanece acessível em modo privado. O contrato preserva cinco bytes big-endian, scratch, inicialização, registradores e resíduos de pilha. O offset `-32768` mantém exclusivamente o comportamento original não terminante; não houve esse fallback nas execuções reais aceitas. Não desfaça essa implementação nem repita sua reconstrução.

O relatório `docs/native-calendar-reconstruction.md` registra 16.380 comparações diretas, 1.232 casos originais e 1.232 modernos, 30/30 grupos CTest e 23 testes Python, com quatro grupos opcionais também executados explicitamente. Os fixtures privados de um e sete dias passaram comparações estritas originais e duas execuções modernas cada, preservando controles negativos de calendário e RNG. A única diferença permitida na simulação moderna é a contagem de instruções traduzidas; PC, registradores, temporários, memória inteira, alocador, imports/callbacks e RNG permanecem verificados. UBSAN passou; ASan não chegou a `main` devido à inicialização do runtime local. Esses resultados são a referência anterior, não validação automática do novo módulo.

A interface avançou normalmente de 19 July 2002 night para 2003; Save/saída normal/reabertura/Load foram conferidos antes e depois, chegando a 5 February 2003 night. Há checkpoints privados `Calendar-Single-19Jul2002.sav` e `Calendar-Single-05Feb2003.sav` sob `.local/performance/calendar-live-02/Runtime/Files/games/`. Preserve-os e verifique disponibilidade antes de reutilizar. A meia-noite exata de 31 December, equivalência de carreira completa e hardware histórico não foram comprovadas.

O aplicativo instalado é `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`. Seu executável e o fonte validado têm SHA-256 `dbe6a1f963bb33a702bbbdc29fce17d2e5f75d822da7d09f7fc66246641ad741`; a cópia isolada assinada tem `ac3c584bd029834161aa89c5593e5ecad8a2f65193db476fd4f526490a239037`. Confira novamente essas identidades. Preserve uma cópia privada executável dessa referência antes de trocar builds ou instalar outro candidato. Não substitua a referência pela cópia diagnóstica assinada sem distinguir seus hashes.

## Inspeção e contrato a estabelecer

Leia `README.md`, `docs/native-port-status.md`, `docs/native-calendar-reconstruction.md`, `docs/native-season-reference.md`, os módulos/testes de calendário, `native/callbacks.cpp`, `native/aot_runtime.hpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*`, `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/prepare_validation.py`, `tools/install_native.py`, `native/CMakeLists.txt` e `tools/build_aot.sh`.

A inspeção de planejamento encontrou uma candidata concreta à diferença de datas em `0x10718130`, até o retorno em `0x107182ec`: 111 instruções no índice privado e 218 chamadas diretas `bl` no export atual. A assembly contém comparação de anos/dias, acumulação de comprimentos de ano, chamadas ao helper bissexto `0x10718440` e retorno por r3. O scratch observado é carregado pelo slot `0x108b7ad0`, com flag em `0x108b84d0`, considerando o TOC original. Estes achados orientam a investigação; não constituem um contrato diferencial já testado. Reconfirme entradas, limites, callers, procedência dos exports e convenções antes de substituir a rotina. Use `.local/recomp-input/instructions.sqlite` e `program.json`, que são privados e não existem em um clone público limpo.

Determine o sinal, largura e unidade do retorno, a ordem dos operandos e o comportamento em datas iguais e invertidas. Inspecione a passagem do segundo operando por registradores/stack: a rotina guarda r4/r5 e lê campos dessa representação. Não trate automaticamente esses argumentos como um segundo ponteiro ou ignore os bytes adicionais de passagem por valor. Caracterize padding, leituras/escritas ordenadas, registradores clobbered, CR/XER/LR/TOC, frame/resíduos de pilha, scratch e inicialização, aliasing e ausência ou presença de imports/RNG. A restauração de `call_original` não prova os clobbers de uma chamada direta.

Cubra mesma data, diferença de um dia em ambos os sentidos, fronteiras de mês/ano, February 28/29, 1900/2000/2100, intervalos de vários anos, anos 0/65535, dias inválidos ou negativos, leap flags inconsistentes, flags não binárias e aliasing relevante com scratch. Não suponha que a implementação original coincide sempre com uma diferença gregoriana matemática: confira qual ano o loop usa para calcular cada comprimento e preserve peculiaridades observadas. Use um oracle independente no domínio em que ele realmente se aplica. Delimite casos desconhecidos ou não terminantes, sem introduzir normalização ou correções de regras não autorizadas.

## Implementação e comparação

Amplie a arquitetura mínima existente: aritmética semântica independente da memória/ABI e adaptador do estado legado. Preserve o layout Mac e os efeitos que os callers observam. Integre a nova entrada no caminho de produção, com cobertura de chamadas diretas no mesmo page, despacho entre pages, indiretas e callbacks. A operação normal comprovada deve executar C++ semântico, não um wrapper que delega à rotina original. Mantenha seleção original somente em diagnósticos privados, sem nova opção de UI, e acrescente auditoria do contrato original para rejeitar exports divergentes ou incompletos.

Compare original e moderno partindo de estados equivalentes, incluindo retorno, entradas, registradores inteiros, temporários, PC, footprint e conteúdo de memória, scratch/flags e RNG. Acrescente contadores/evidências próprios da nova operação. Os contadores anteriores de adição/subtração não provam que a diferença moderna foi executada.

Preserve a comparação estrita original-contra-original. Reexecute a comparação delimitada da simulação real nos fixtures existentes, com adição/subtração e diferença modernas efetivamente exercitadas. Separe o modo inteiramente original do baseline já entregue com adição/subtração modernas e diferença original. Não compare candidatos que diferem inadvertidamente em outros subsistemas. Mantenha a fronteira existente que permite somente contagem de instruções; qualquer ampliação necessita justificativa específica e verificável, sem máscaras gerais ou contagens fabricadas. Garanta término delimitado por estado/transcript e orçamento finito. Não recapture o candidato moderno como seu próprio oracle.

Mantenha os controles negativos existentes. Acrescente uma perturbação isolada específica da diferença, por exemplo um retorno incorreto em um dia, detectada na primeira fronteira relevante. A execução de entrega deve estar sem perturbação; a injeção só pode ser ativada pela validação privada. Confirme hashes inalterados dos fixtures e streams originais. Não exija replay de carreira completa.

## Aplicativo real e critérios de conclusão

Use uma única instância ARM64 por vez, em cópia privada dos dados graváveis, preparada pelas ferramentas existentes. Demonstre chamadas reais da diferença moderna, com argumentos/resultados auditáveis e sem fallback normal. Abra calendário/fixtures e informações de jogador/contrato pertinentes, avance normalmente por um intervalo delimitado incluindo uma fronteira de mês e verifique Save, saída normal, reabertura e Load no candidato. Não edite datas, saves, sementes ou memória para simular avanço. Reaproveite checkpoints adequados; não é necessário concluir outra temporada. Fronteiras anuais e de século precisam de comparação direta útil; se repetir uma passagem anual na UI, registre somente o que realmente observou. A validação anual anterior não prova o novo módulo.

Execute testes pertinentes e a suíte completa no candidato final, incluindo a auditoria de temporários e UBSAN quando modificar gerador/adapter. Verifique se todos os objetos usam o layout final de `State`: o incremento anterior exigiu recompilação completa após objetos em execução terem terminado com um layout antigo. Use o cache quando consistente; não introduza otimizações para acelerar este trabalho.

Comandos base, a confirmar e ampliar com os novos grupos:

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

Só conclua com contrato caracterizado, implementação moderna usada no app, equivalência direta e delimitada, controles negativos preservados, persistência real conferida e suíte final aprovada. Diferencie equivalência automática, observação de UI, inferência e limitações. Se houver bloqueio essencial, registre-o e não declare o objetivo concluído.

## Proteção e entrega

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks, Finder metadata e alterações existentes. Todos os exports, dados, novos saves, snapshots, transcripts, logs, capturas e builds devem continuar privados e ignorados. Não altere regras de ignore/LFS. Preserve imutável `saves/Mac-Teste-Validado-Pos-Jogo.sav`, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.

Documente em inglês o contrato, arquitetura, pontos de integração, fronteiras de comparação, comandos repetíveis, chamadas reais, controles negativos, validação no aplicativo e limites. Atualize `docs/native-port-status.md`; mantenha os relatórios anteriores como evidência histórica. Registre separadamente hashes do fonte, candidato assinado e instalado.

Ao executar este prompt como solicitação de implementação, conclua com instalação ARM64 validada preservando dados, backup privado executável e inventários de conteúdo/metadados antes/depois, revisão do diff exato, commit e push normal sem force, conforme a autorização permanente para trabalho concluído e validado neste projeto. Confira o SHA remoto por consulta nova. Não publique artefatos privados nem declare reconstrução do restante do engine ou melhoria de velocidade sem evidência adequada.
