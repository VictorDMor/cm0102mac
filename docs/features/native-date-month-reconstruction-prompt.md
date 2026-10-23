# Reconstrução semântica do mês da data Mac

Reconstrua em C++23 a entrada independente candidata `0x10718360`, que converte o registro interno de data do CM 01/02 em índice de mês. Trabalhe em `/Users/victordmor/Projects/cm0102mac` e execute este incremento até a entrega validada quando este prompt for enviado como pedido de implementação. A preparação deste handoff é somente planejamento: não implementa a rotina nem inicia outra sessão.

Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês.

## Produto e próximo incremento

O objetivo é o CM 01/02 completo, jogável nativamente em Apple Silicon e progressivamente reconstruído em código semântico compreensível. A referência executável atual recompila estaticamente o original Mac; isso não equivale à reconstrução semântica do engine inteiro. A temporada real e a persistência anual já têm evidência documentada. Não repita esse marco como se estivesse pendente.

Adição, subtração, diferença e os dois predicados independentes de ordenação estão entregues. O próximo incremento escolhido é somente a conversão para mês. Isso amplia a interpretação do registro de data usada por exibição e callers sem assumir o subsistema inteiro. A entrada adjacente `0x107182f0`, aparentemente relacionada ao dia do mês, dia da semana, formatação, comparadores ternários, idade, contratos, transferências, agendador, competições, premiações e jogadores ficam fora do escopo. Não reconstrua essas rotinas incidentalmente. Otimizações, novas features e mudanças gráficas permanecem suspensas; não prometa ganho de velocidade.

Preserve a peculiaridade bissexta da diferença, expressamente adiada pelo usuário: January 1 2000 menos January 1 2001 retorna -365, e a inversão +366. Uma mudança exige decisão separada após análise dos callers e validação. A identificação Gregorian de anos bissextos não é, por si, autorização para corrigir o percurso original.

## Ponto de partida verificado na preparação

- `main`, HEAD e consulta nova ao remoto concordaram em `2bfcd55ddf74bf93a439fef64c217022c5aece1c`, entrega de date-before. Reconfira branch, remoto, diff e instruções `AGENTS.md` aplicáveis antes de agir.
- `docs/features/native-date-difference-reconstruction-prompt.md` já estava untracked e deve ser preservado. Este novo handoff também pode estar localmente pendente; não o confunda com mudança de produto entregue.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`: ARM64, executável SHA-256 `d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1`.
- Candidato privado assinado da entrega anterior: `.local/performance/date-before-live-01/CM Validation date-before-live-01.app`, hash registrado no relatório `c20788488bb734e5a97590ca75c315a6d5d4f39e75f93fbc65613892ec606511`. Reconfirme antes de reutilizar.
- O backup histórico `.local/date-before-reconstruction/reference/installed.app` é ARM64, hash `b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545`. Ele NÃO é a referência instalada atual. Preserve uma NOVA cópia privada executável do instalado atual, com resource forks e extended attributes, antes de trocar builds/instalar.
- O incremento anterior aprovou 33/33 grupos CTest, 27 testes Python com quatro grupos opcionais também executados explicitamente, auditoria de temporários e UBSAN. Ambos os predicados passaram 5.646 casos diretos / 156 Gregorian / 196.608 signed-day cada. São resultados históricos; execute novamente os testes do candidato final.
- O fixture de ordenação teve 12.911 chamadas modernas de date-before por candidato; a UI teve 13.153 e 32 amostras conferidas. Save/saída/reabertura/Load foram observados em 10 e 27 August 2002. Nenhuma dessas contagens demonstra cobertura ou contrato da nova entrada de mês.

Leia `README.md`, `docs/native-port-status.md`, `docs/native-season-reference.md`, `docs/native-calendar-reconstruction.md`, `docs/native-date-difference-reconstruction.md` (incluindo Deferred improvement), `docs/native-date-ordering-reconstruction.md` e `docs/native-date-before-reconstruction.md`. Inspecione `native/calendar*`, `native/date_difference_checks.cpp`, `native/date_ordering_checks.cpp`, `native/callbacks.cpp`, `native/aot_runtime.hpp`, `native/aot_probe.cpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*`, `native/CMakeLists.txt`, `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`.

## Fronteira candidata e contrato a demonstrar

A inspeção read-only do índice privado encontrou 54 instruções no intervalo exclusivo `[0x10718360,0x10718438)`, com último retorno em `0x10718434`, e 92 chamadas diretas `bl 0x10718360`. Exemplos de call sites observados: `0x1002a12c`, `0x1002a138`, `0x100746e0`, `0x10089e80`. Reconfirme limites, fluxo, dispatch, entradas indiretas e callers; não trate quantidade de chamadas estáticas como execução real.

Fingerprint usando os campos exatos de `tools/calendar_audit.py`: `aa64b3cf2df3f13dba6c660b2a102a3d98c0492b60a5b63b603df3e12a747798`. Proveniência dos exports privados herdados: índice SHA-256 `2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec`, `program.json` `50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0`, 1.953.157 instruções, PEF original `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`. Esses insumos não estão presentes em clone público limpo. Não publique payloads ou listagens privadas.

A assembly sugere primeiro operando por ponteiro r3, dia signed16, seleção de tabela pelo byte da flag em offset 4 e dois ponteiros relativos à TOC (`-0x3fa4` / `-0x3fa8`). Não há evidência diferencial do contrato ainda. Demonstre o significado dos valores de tabela, seus endereços resolvidos, o índice/base do retorno e o sentinela de datas não reconhecidas. Não suponha que basta calcular um mês Gregorian a partir do ano: confirme se o ano é lido, como flags 0/1/128/255 escolhem a tabela e como datas inválidas se comportam.

Caracterize ordem/largura/signedness das leituras e do retorno, registradores inteiros e temporários, CR/XER/LR/CTR/TOC, PC, pilha, memória inteira, alocador, imports/callbacks e RNG. A rotina usa CTR e cargas ordenadas; não herde clobbers de outra operação. Verifique ausência/presença de spills, scratch ou inicialização diretamente. `call_original` restaura registradores no callback e não substitui comparação de entrada direta.

Cubra cada transição de mês, primeiro/último dia do ano, February 28/29 e March 1, 1900/2000/2100, anos extremos unsigned16, flags inconsistentes/não binárias e dias negativos/inválidos/extremos signed16. Use oracle independente somente no domínio válido demonstrado, com flag consistente quando necessário. Determine se as tabelas são dados mutáveis visíveis; não substitua leituras por constantes sem demonstrar a compatibilidade. Delimite tabelas/ponteiros não comprovados e falhas de memória sem inventar equivalência universal.

## Implementação e comparação automática

Amplie minimamente a arquitetura atual: lógica semântica pura e adapter dos efeitos legados. A produção normal deve executar C++ semântico para a nova entrada, com seleção original somente em diagnósticos privados. Preserve os cinco contratos entregues. Não introduza opções na UI.

Integre o hook no destino gerado de `0x10718360`, inclusive `goto` na mesma página, despacho entre páginas, indiretas e callbacks. Acrescente auditoria que rejeite corpo divergente e fragmentos incompletos. Teste callers reais pertinentes com inputs controlados e marque essa evidência como sintética; não substitua comparadores/rotinas vizinhas por busca textual.

Compare estados originais e modernos equivalentes com orçamento finito: resultado, toda ABI, temporários/PC, memória mapeada inteira e ordem/footprint dos efeitos, stack, alocador, imports/callbacks e RNG. Acrescente contador e amostras próprios do mês, identificando entrada, operandos efetivos, tabela/flag pertinente, resultado e retorno/caller. Contadores das outras cinco operações não comprovam essa entrada.

Separe inteiramente original; baseline entregue com cinco operações modernas e mês original; candidato com seis modernas. Preserve as semânticas históricas de `--calendar-baseline` (somente shifts modernos), `--date-ordering-baseline` (shifts/diferença modernos) e `--date-before-baseline` (shifts/diferença/later modernos, before original). Não redefina nomes silenciosamente. Captura original deve selecionar também a entrada de mês original; nunca recapture candidato como oracle.

Reexecute os três fixtures imutáveis: `.local/performance/season-day-01/SeasonFixture`, `.local/performance/season-week-01/SeasonFixture` e `.local/performance/date-ordering-capture-02/SeasonFixture`. Exija dois originais estritos, baseline entregue e históricos pertinentes, e duas execuções candidatas equivalentes. Preserve a única fronteira existente de diferença permitida na contagem de instruções traduzidas; não masque PC, registradores, temporários, bytes, alocador, imports/callbacks ou RNG. Mantenha término pelo estado/transcript e orçamento finito.

Registre stream privado original da nova entrada e um negativo isolado que altera seu primeiro retorno moderno. Exija falha na primeira fronteira pertinente de mês; preserve controles de RNG, shifts, diferença, later e before. Não altere os formatos históricos nem os streams CMCAL002 já entregues: use stream separado ou nova versão explícita se estender o formato. Reconfira hashes de fixtures e streams antes/depois. Conte chamadas reais por entrada; zero é ausência de cobertura. Se nenhum fixture exercitar mês, capture em modo inteiramente original uma sequência real mínima pertinente e compare o mesmo fixture nos modos original/baseline/candidato. Não exija replay completo de carreira.

## Aplicativo e conclusão verificável

Use uma única instância ARM64 por vez e Runtime/Runs privados preparados pelas ferramentas existentes. Reutilize, após conferir os hashes, `.local/performance/date-before-live-01/Runtime/Files/games/DateBefore-10Aug2002.sav` (`dfaa610c7f7d2b53006d7b9ceb53509d939033be3573ec1e6b56c6f7c892596d`) ou `DateBefore-27Aug2002.sav` (`2056240fdc1e6ea179734d8e5e75d11c41475b36e4ee4495d91f338904a8c084`), reidentificados na preparação. Preserve os originais e demais checkpoints históricos.

Demonstre chamadas modernas próprias de mês no app por uma interação normal de calendário/datas, guiada pelos callers/logs. Não presuma que abrir calendário chama essa rotina. Registre argumentos/resultados auditáveis e ausência de fallback normal; não declare entrega sem cobertura real.

Abra calendário e informações úteis de jogador/contrato, avance normalmente por intervalo delimitado e confira Save, saída normal, processo ausente, reabertura do candidato exato e Load. Observe e reporte as datas reais; não edite datas/saves/RNG/memória para forçar cobertura ou avanço. Férias podem avançar rapidamente: acompanhe a UI e interrompa assim que o intervalo pertinente estiver observado. Não é necessário concluir outra temporada. Separe observações da UI da equivalência automática.

Use o Save original via Game Options → Save Game. O último incremento encontrou a limitação existente de File → Save nativo (`Native menu selections require original MenuSelect dispatch`); o menu de jogo funcionou. Corrigir o menu macOS não pertence a este incremento. Recrie bindings CUA e índices AX na nova sessão; não dependa de objetos desta conversa.

Execute os testes pertinentes e a suíte completa no candidato final, incluindo novo grupo direto, regressões das cinco operações, auditorias e UBSAN do adapter/gerador alterados. Confirme layout final de State em todos os objetos ligados; recompile completamente se houver inconsistência. Não introduza otimizações para reduzir o tempo da tarefa. Limitações históricas de ASan não aprovam o candidato atual.

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

Só conclua com contrato próprio demonstrado, operação semântica em produção, equivalência direta/delimitada, negativos preservados, cobertura real própria, persistência do app e suíte final aprovadas. Se houver bloqueio essencial, documente sem declarar o objetivo concluído.

## Proteção e entrega

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks e Finder metadata. O incremento anterior preservou 809/5/42 arquivos; faça novos inventários atuais antes/preinstall/depois, sem tratar essas quantidades históricas como invariantes. Todos os exports, dados, saves novos, snapshots, transcripts, logs, capturas e builds continuam privados e ignorados. Não altere ignore/LFS. Preserve imutável `saves/Mac-Teste-Validado-Pos-Jogo.sav`, hash `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.

Documente em inglês contrato, arquitetura, integração, fronteiras, comandos repetíveis, contagens próprias, negativos, observações e limites. Atualize `docs/native-port-status.md`, preservando relatórios históricos e a correção bissexta adiada. Registre separadamente hashes de fonte, candidato assinado e instalado.

Entregue instalação ARM64 validada, novo backup privado executável da referência atual e inventários de conteúdo/metadados antes/depois. Revise o diff exato, faça commit e push normal sem force conforme a autorização permanente para trabalho concluído/validado e confira o SHA remoto por consulta nova. Não publique artefatos privados nem declare reconstrução de todas as conversões de data ou do restante do engine.
