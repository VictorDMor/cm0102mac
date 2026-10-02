/goal Reconstruir semanticamente o subsistema de aritmética de calendário e data do CM 01/02 em C++ moderno, integrá-lo ao aplicativo nativo ARM64 e demonstrar equivalência com as rotinas originais nas fronteiras estabelecidas, preservando a essência do jogo e seus dados.

Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Converse em português; mantenha código, documentação técnica, testes e mensagens do aplicativo em inglês. Execute este incremento até a entrega validada. Não se limite a investigação, planos, wrappers que continuam usando a rotina original ou código moderno que nunca é chamado pelo aplicativo.

## Visão e ponto de partida

O produto é o CM 01/02 completo e jogável nativamente em Apple Silicon. A direção aprovada é preservar o comportamento primeiro e substituir o C++ gerado por módulos claros e sustentáveis, incrementalmente, com contratos e comparações verificáveis. A aritmética de calendário e data é o primeiro subsistema aprovado para essa reconstrução. Novas otimizações, features, regras de simulação e mudanças gráficas continuam suspensas.

A referência anterior foi entregue em `main`, commit `666563e4a0576e4938a36723a5d2248ebac80901`, com push normal e SHA remoto conferido. Na preparação deste prompt o repositório estava limpo; os arquivos de planejamento deste incremento podem aparecer como alterações locais. Confira novamente branch, HEAD, remoto, instruções aplicáveis e diff antes de agir. Preserve todas as alterações existentes e não reverta o trabalho anterior de desempenho.

O relatório anterior registra uma temporada real de 2002 concluída e a edição de 2003 em andamento, chegando a 29 March 2003 afternoon. Save, saída normal e reabertura foram verificados antes e depois da mudança anual. Comparações delimitadas de um e sete dias concordaram em execuções equivalentes e rejeitaram uma perturbação conhecida do RNG. A suíte final passou em 28/28 grupos CTest; a descoberta Python passou 20 testes, com quatro grupos opcionais também executados explicitamente contra os exports privados. Esses resultados são a referência histórica do incremento anterior, não validação automática da nova implementação. A passagem exata pela meia-noite de 31 December não foi observada diretamente. Replay completo de carreira e equivalência com hardware histórico permanecem fora do escopo comprovado.

O aplicativo instalado é `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`. A referência de código instalada e o bundle fonte anterior tinham executável SHA-256 `45a47c237d470a98fbdbc4535ce4331a150b04179411af315a009adad8a7497b`. Confira o estado atual e preserve uma cópia privada executável da referência original antes de substituir builds ou instalar o candidato. Assinatura de uma cópia isolada pode alterar seu hash: registre separadamente hashes do fonte, do candidato assinado e do instalado.

## Leia e inspecione primeiro

- `README.md`, `docs/native-port-status.md`, `docs/native-season-reference.md` e instruções `AGENTS.md` aplicáveis, se houver.
- `native/calendar_reference_checks.cpp`: contrato inicial e 1.232 casos das rotinas originais.
- `native/callbacks.cpp`, os pontos de despacho e geração AOT, `native/CMakeLists.txt` e `tools/build_aot.sh`: como chamadas originais chegam ao código executado e quais estados são restaurados pelos wrappers.
- `native/season_reference.hpp`, `native/season_reference.cpp`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.hpp`, `native/match_replay.cpp` e `tools/check_season_reference.py`: estado preservado, entradas externas, aleatoriedade e comparação delimitada existente.
- `tools/prepare_validation.py` e `tools/install_native.py`: isolamento, manifestos, metadados Mac e instalação preservando dados.
- As rotinas originais nos exports locais, especialmente `0x10717890` e `0x10717b20`, seus helpers e callers. Use `.local/recomp-input/` e diagnósticos privados existentes quando disponíveis; confirme disponibilidade e procedência. Esses arquivos não são dependências públicas disponíveis em um clone limpo.

## Contrato a caracterizar e preservar

As rotinas originais de adição e subtração são `0x10717890` e `0x10717b20`. A data ocupa cinco bytes: day-of-year de 16 bits, com base zero, no offset 0; year de 16 bits no offset 2; leap flag de 8 bits no offset 4. Preserve leitura e escrita big-endian e o layout Mac; não use layouts Windows como substitutos.

As fronteiras já identificadas incluem ponteiro da data corrente em `0x108b3f10`, fase diária em `0x108b40a4` (morning 0, afternoon 1, night 2), ponteiros de scratch em `0x108b7ae4` e `0x108b7adc`, e flags de inicialização em `0x108b84d5` e `0x108b84d3`. Confira a interpretação no código original antes de fixar o contrato moderno.

Os 1.232 casos existentes cobrem anos 1900, 1999, 2000, 2001, 2002, 2004 e 2100; January 1, February 28, March 1 e December 31; offsets positivos, negativos e zero até 730 dias; adição e subtração; outputs separados e in-place. Verificam os cinco bytes de resultado, entrada separada preservada, scratch e flags, campos RNG preservados e ausência de imports nativos nos caminhos testados. O oracle independente usa aritmética gregoriana em C++. A restauração de registradores e temporários por `call_original` prova o comportamento do wrapper: não a apresente como prova dos clobbers da rotina original sem inspecionar a chamada direta.

Antes de implementar, determine largura e sinal reais dos parâmetros, conversões, retornos, clobbers relevantes, inicialização e efeitos observáveis. Caracterize extremos do domínio de offsets, viradas de ano, regras de século, limites do ano representável, entradas inválidas, flags inconsistentes e aliasing entre entrada, saída e scratch. Amplie testes conforme o contrato encontrado. Diferencie domínio comprovadamente válido, comportamento observado e casos ainda desconhecidos. Não introduza normalização, exceções ou uma interpretação gregoriana diferente sem evidência de equivalência. Se um caso exigir compatibilidade excepcional, delimite-o e documente-o; um fallback que mantém a rotina original no caminho normal exercitado não conclui este objetivo.

## Implementação exigida

Crie um módulo C++ moderno com nomes, tipos e responsabilidades compreensíveis, separando a aritmética semântica da adaptação ao estado legado. O adaptador deve preservar os cinco bytes, convenções de chamada e efeitos de scratch/inicialização que os callers observam. Escolha a arquitetura mínima adequada ao contrato; C++23 já compila neste projeto, e mudar o padrão ou renomear C++ gerado não constitui reconstrução semântica.

Integre a substituição das duas rotinas no caminho de produção usado pelo aplicativo. Mantenha a implementação original acessível em um modo privado de referência para comparação, sem opção nova na interface do usuário. Registre evidência que demonstre quais chamadas reais usaram o módulo moderno, evitando atribuir a ele execuções que permaneceram no original. Preserve chamadas diretas, indiretas e de callbacks relevantes; confira a cobertura dos pontos de despacho.

Não reconstrua neste incremento o agendador, geração de fixtures, rollover de competições, premiações, desenvolvimento de jogadores, transferências, contratos ou o engine inteiro. Não altere formatos de save, dados de campeonato, resultados, comportamento do RNG ou fluxos já funcionais. Corrija apenas problemas necessários para a integração e equivalência deste subsistema, documentando sua causa e alcance.

## Comparação automática e repetível

Preserve os checks original-contra-original e seus controles negativos. Acrescente comparação original-contra-moderno nas fronteiras das rotinas, partindo de estados equivalentes e verificando resultado, entradas preservadas, efeitos de memória, inicialização, convenções observáveis e RNG. Use um oracle independente onde apropriado, mas o comportamento original permanece a referência de compatibilidade.

Integre também uma comparação delimitada da simulação real. Os fixtures privados existentes podem estar em `.local/performance/season-day-01/SeasonFixture` e `.local/performance/season-week-01/SeasonFixture`; confirme seus manifestos e hashes. Preserve estado inicial, entradas externas relevantes e ordem/consumo de aleatoriedade. A captura existente registra imports nativos, registradores, efeitos, callbacks e contagens originais, e compara todo o estado mapeado. A substituição semântica pode mudar contagem de instruções traduzidas ou temporários internos: o comparador atual não pode simplesmente ser considerado adequado sem revisão.

Defina uma fronteira de comparação explícita e auditável para essas diferenças internas. Preserve a verificação estrita original-contra-original. Para original-contra-moderno, demonstre por que cada diferença permitida é interna e sem efeito observável; compare estado real do jogo, data/fase, alocador, efeitos de memória e sequência RNG nas mesmas fronteiras, além de argumentos efetivos, ordem e efeitos das entradas externas relevantes. Não aplique máscaras gerais a registradores, memória, PC, imports ou RNG. Não fabrique contagens para obter aprovação, altere datas ou sementes, recapture o moderno como seu próprio oracle, ou enfraqueça os controles negativos. Garanta término delimitado mesmo se a contagem de instruções deixar de ser o mesmo relógio.

Demonstre que execuções equivalentes concordam e que uma divergência deliberada específica de calendário é detectada, como um erro de século/bissexto ou de um dia, com diagnóstico da primeira diferença relevante. Preserve também a detecção da perturbação RNG existente. Injete o erro somente em validação isolada e remova-o antes da entrega. Não exija replay completo de carreira.

## Validação no aplicativo real

Use uma única instância do jogo por vez, ARM64 real, em cópias privadas isoladas dos dados. Execute o aplicativo com a substituição moderna efetivamente habilitada e observe avanços de calendário, incluindo uma fronteira de mês e uma de ano. Obtenha checkpoints adequados por avanço normal do jogo quando necessário; não edite save, data ou memória para simular a passagem. Reaproveitar a temporada anterior como referência é permitido, mas ela não prova a nova integração. Não é necessário concluir outra temporada inteira nem toda uma carreira.

Valide Save, saída normal, reabertura e Load antes e depois da fronteira anual exercitada. Confira data/fase, clube e competição, informações pertinentes de jogadores e fixtures/resultados, confrontando o que é visível com a evidência delimitada disponível. Distinga observação na interface, comparação automática e inferência. Não declare transições anuais adicionais verificadas apenas porque a data mudou. Preserve saves originais; crie nomes novos para os checkpoints de validação.

## Testes e critérios de conclusão

Acrescente testes úteis para o contrato e integração, mantendo os 1.232 checks originais e cobrindo os riscos encontrados. Execute os testes pertinentes e a suíte completa no candidato final. Use o cache de build quando adequado; não faça otimizações para acelerar este incremento.

Os comandos de referência atuais, que devem ser conferidos no estado encontrado, incluem:

```sh
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/check_season_reference.py .local/performance/season-day-01/SeasonFixture
python3 tools/check_season_reference.py .local/performance/season-week-01/SeasonFixture
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
```

Os comandos atuais de replay estrito não demonstram original-contra-moderno sem a integração e fronteira revisadas acima. Documente os novos comandos repetíveis. Execute checks de sanitizers e a auditoria de temporários gerados quando os componentes afetados justificarem, especialmente se mudar compilador/gerador; o check existente é `python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize`. Diferencie casos opcionais pulados de grupos explicitamente executados.

Conclua apenas quando o módulo substituir efetivamente as rotinas de calendário no aplicativo, os contratos e efeitos observáveis estiverem preservados, as comparações delimitadas passarem e rejeitarem a divergência conhecida, o aplicativo e a persistência forem verificados nas fronteiras exigidas, e os testes pertinentes e a suíte completa estiverem aprovados. Se uma condição essencial continuar bloqueada, registre o bloqueio e não declare o objetivo concluído.

## Proteção de dados, documentação e entrega

Preserve os dados em `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks, Finder metadata e alterações existentes. Não publique exports originais, dados do jogo, novos saves, snapshots, transcripts, logs privados, vídeos ou builds. Mantenha esses artefatos em diretórios privados ignorados. Preserve imutável a referência Git LFS `saves/Mac-Teste-Validado-Pos-Jogo.sav`, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`; não amplie a seleção de arquivos privados versionados nem mude regras de ignore/LFS para facilitar testes.

Documente em inglês o contrato, a arquitetura mínima, a fronteira original/moderno, comandos repetíveis, evidências de chamadas reais, comparação e controles negativos, validação no aplicativo, hashes de entrega e limitações. Atualize `docs/native-port-status.md` com o progresso real e mantenha `docs/native-season-reference.md` como referência histórica. Só marque calendário como concluído após todos os critérios. Sugira o próximo subsistema com base nas fronteiras comprovadas, distinguindo recomendação de uma prioridade aprovada.

Instale a versão ARM64 validada preservando meus dados, com backup privado e inventários antes/depois, incluindo conteúdo e metadados Mac. Confira a identidade do executável instalado. Revise o diff exato, exclua artefatos privados, faça commit e push normal sem force, e confira o SHA remoto com uma consulta nova. Essa entrega está autorizada como parte deste incremento depois da validação. Não entregue uma implementação apenas testada em componentes como se fosse o jogo validado.

Na resposta final, descreva o que foi reconstruído e comprovado, o comportamento real e a persistência observados, as comparações e testes, as limitações restantes, a instalação e o commit/SHA remoto. Não declare a reconstrução do restante do engine concluída.
