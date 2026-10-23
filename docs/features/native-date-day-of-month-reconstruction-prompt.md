# Reconstrução semântica do dia do mês da data Mac

Reconstrua em C++23 a entrada independente `0x107182f0`, candidata à conversão do registro interno de data do CM 01/02 para dia do mês. Trabalhe em `/Users/victordmor/Projects/cm0102mac` e execute este incremento até a entrega validada. Converse em português; mantenha código, testes, documentação técnica e novas mensagens do aplicativo em inglês. Este arquivo é um pedido completo de implementação para uma sessão nova. Sua preparação foi somente planejamento: não implementou a rotina nem iniciou outro chat.

## Produto e próximo incremento

O produto é o CM 01/02 completo, jogável nativamente em Apple Silicon, com o engine progressivamente reconstruído em código semântico compreensível. A referência atual recompila estaticamente o original Mac; isso não significa que todo o engine foi reconstruído semanticamente. A temporada real, a passagem anual e a persistência correspondente já têm evidência entregue em `docs/native-season-reference.md`; não repita esse marco como pendência.

Adição, subtração, diferença, os dois predicados independentes de ordenação e conversão para mês estão entregues. O próximo incremento escolhido é somente o dia do mês, complementando a interpretação semântica do mesmo registro de data. O resultado esperado é que callers reais usem a nova operação sem mudar datas exibidas ou o comportamento do jogo. Demonstre esse contrato antes de tratá-lo como estabelecido.

Dia da semana, formatação, comparadores ternários, idade, contratos, transferências, agendador, competições, premiações, jogadores e as demais rotinas continuam fora do escopo. Inspecionar um caller ou observar um contrato na UI não autoriza reconstruí-los. Novas otimizações, features e mudanças gráficas permanecem suspensas; não prometa ganho de velocidade.

Preserve a peculiaridade bissexta da diferença expressamente adiada pelo usuário: January 1 2000 menos January 1 2001 retorna -365; a inversão retorna +366. Uma correção depende de decisão separada, análise dos callers e validação de gameplay/persistência. A identificação Gregorian de anos bissextos não autoriza corrigir o percurso original.

## Ponto de partida verificado na preparação

- `main`, HEAD e consulta nova a `origin/main` concordaram em `cff87aa9d5bb8318ad553751637d3d390b2f7e3f`, entrega da conversão de mês. Reconfira Git, remoto, diff e instruções `AGENTS.md` aplicáveis. Não havia `AGENTS.md` nos caminhos ancestrais ou na árvore inspecionada; isso deve ser reconferido.
- Os prompts `docs/features/native-date-difference-reconstruction-prompt.md` e `docs/features/native-date-month-reconstruction-prompt.md` estavam untracked. Preserve-os. Este novo prompt também pode estar localmente pendente; não confunda handoffs com mudanças de produto entregues.
- Fonte `.local/aot-build/CM 01-02 Native Prototype.app` e instalado `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`: ARM64, executável SHA-256 `caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756`, reconferido na preparação.
- Candidato privado assinado da entrega de mês: `.local/performance/date-month-live-01/CM Validation date-month-live-01.app`, ARM64, executável `f3e17857f83cf0922028847df8c2718b5f6fb9df83ea4d82f0f5b86e5a23de5d`, reconferido. Não confunda esse hash com o da fonte ou do instalado.
- O backup histórico `.local/date-month-reconstruction/reference/installed.app` é ARM64 e tem executável `d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1`. Ele preserva a referência anterior à entrega de mês, NÃO o instalado atual. Antes de trocar builds/instalar, crie uma NOVA cópia privada executável do instalado atual, com resource forks e extended attributes; verifique identidade e inventário.
- A entrega de mês aprovou 34/34 grupos CTest, descoberta Python com 28 testes e quatro grupos opcionais pulados, todos também executados explicitamente; auditoria de temporários e UBSAN gerado/focado passaram. O novo harness aprovou 1.684 casos diretos original/moderno, 239 oracle Gregorian e 131.072 signed-day puros. São resultados do candidato anterior, não aprovação da nova rotina; não execute essas suítes caras apenas para planejar.
- Nos fixtures imutáveis de um dia, sete dias e ordenação, cada candidato anterior executou respectivamente 8.807, 38.256 e 8.433 chamadas modernas de mês. O último também executou 38.051 later e 12.911 before. Cada fixture aprovou dois originais estritos, captura original, dois candidatos e quatro baselines; negativos exercitados foram rejeitados. A UI registrou 774/870/85 chamadas de mês e 96 amostras conferidas, com Save/saída/reabertura/Load em 27 e 28 August 2002. Nenhuma dessas contagens prova cobertura da nova entrada de dia do mês.

Leia `README.md`, `docs/native-port-status.md`, `docs/native-season-reference.md`, `docs/native-calendar-reconstruction.md`, `docs/native-date-difference-reconstruction.md` (incluindo Deferred improvement), `docs/native-date-ordering-reconstruction.md`, `docs/native-date-before-reconstruction.md` e `docs/native-date-month-reconstruction.md`. Inspecione `native/calendar*`, `native/date_month_checks.cpp`, `native/date_difference_checks.cpp`, `native/date_ordering_checks.cpp`, `native/callbacks.cpp`, `native/aot_runtime.hpp`, `native/aot_probe.cpp`, `native/season_reference.*`, `native/season_reference_main.cpp`, `native/season_reference_checks.cpp`, `native/match_replay.*`, `native/CMakeLists.txt`, `tools/compile_recomp.py`, `tools/calendar_audit.py`, `tools/test_calendar_audit.py`, `tools/check_calendar_season.py`, `tools/check_season_reference.py`, `tools/prepare_validation.py`, `tools/install_native.py` e `tools/build_aot.sh`.

## Fronteira candidata e contrato a demonstrar

A inspeção estática read-only do índice privado encontrou 24 instruções em `[0x107182f0,0x10718354)`, último retorno em `0x10718350`, e nenhuma instrução exportada no intervalo seguinte até `0x10718360`. Foram encontradas 20 chamadas diretas `bl 0x107182f0`, nenhuma na mesma página da entrada, e nenhum fluxo externo indexado dirigido ao interior do corpo. Exemplos de call sites: `0x10097f84`, `0x10097fa0`, `0x101450cc`, `0x10363940`, `0x1036395c`, `0x103a76f8`. Reconfirme limites, fluxo, destino gerado, callers e possibilidades indiretas. Quantidade estática não demonstra execução real.

Fingerprint com os campos exatos de `tools/calendar_audit.py`: `a496c80a33e42a50319536aa245bb4a3a2d2fd64363d1a5e13f277b89ada57a5`. Índice SHA-256 `2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec` e `program.json` `50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0` foram reconferidos. A proveniência documentada é de 1.953.157 instruções, PEF original `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`. Exports/payloads originais são privados e não estão disponíveis em clone público limpo; não publique listagens ou dados originais.

A leitura estática sugere r3 como ponteiro da data; primeiro o byte em offset 4, depois uma tabela selecionada através da TOC (`-0x3fa4` / `-0x3fa8`). O corpo aparentemente percorre doze limiares em ordem reversa, lê novamente o signed16 dia a cada probe e, quando encontra um limiar estritamente menor, relê o limiar selecionado, subtrai-o do dia e estreita o retorno para signed16. Isso é hipótese fundamentada em assembly, ainda sem comparação diferencial própria.

Demonstre a base do retorno no domínio válido (inclusive January 1), o sentinela quando nada é reconhecido e o comportamento para dias inválidos. Confirme se o ano é lido, como flags 0/1/128/255 escolhem a tabela e como se comporta uma flag inconsistente. Não substitua o cálculo por uma biblioteca Gregorian ou pela rotina de mês sem demonstrar preservação das leituras, valores e ABI. O mês entregue usa dois grupos de seis probes; a nova entrada aparenta ter um loop diferente e não pode herdar seus clobbers nem seu footprint.

Resolva endereços/valores das tabelas e seus slots no contexto TOC efetivo. O relatório de mês demonstra tabelas mutáveis em `0x108e14ac`/`0x108e14c4`, slots `0x108b7ae8`/`0x108b7aec` com TOC `0x108bba90`; reconfirme sua utilização pela nova entrada. Não troque dados mutáveis por constantes. Teste tabelas/pointers/TOC alternativos mapeados e limiares não monotônicos. Determine o estreitamento quando a diferença excede signed16, incluindo valores extremos em tabelas mutadas; não acrescente clamp, validação, normalização ou exceção que o original não tenha.

Caracterize ordem/largura/signedness de todas as leituras, inclusive a releitura do limiar após a busca; resultado, registradores inteiros, temporários, CR/XER/LR/CTR/TOC, PC, pilha, memória inteira, alocador, imports/callbacks e RNG. Confirme ausência/presença de spills, scratch e inicialização diretamente. `call_original` restaura contexto no callback e não substitui equivalência da entrada direta. Delimite acessos não mapeados, wrapping e mutação concorrente sem inventar equivalência universal.

Cubra todos os limites de mês, primeiro/último dia do ano, February 28/29 e March 1, 1900/2000/2100, anos extremos unsigned16, flags não binárias/inconsistentes e dias negativos/inválidos/extremos signed16. Use oracle independente somente no domínio válido demonstrado com flag consistente. Preserve as seis operações anteriores e suas evidências.

## Implementação e equivalência automática

Amplie minimamente a arquitetura atual: lógica semântica pura e adapter dos efeitos legados. A produção normal deve executar C++ semântico para a nova entrada; seleção original fica restrita a diagnósticos privados. Não introduza opções na UI, wrappers que ainda deleguem a conversão normal ao original ou abstrações especulativas.

Integre o hook no destino gerado de `0x107182f0`, cobrindo `goto` na mesma página, dispatch entre páginas, indiretas e callbacks. Acrescente auditoria do corpo completo que rejeite alterações e fragmentos incompletos. Teste instruções de callers reais com inputs controlados, marcando essa evidência como sintética. Não substitua corpos vizinhos/callers por busca textual.

Compare estados original/moderno equivalentes com orçamento finito: resultado, toda ABI/temporários/PC, todos os bytes mapeados, ordem/footprint dos efeitos, pilha, alocador e imports/callbacks/RNG. Reutilize a instrumentação opcional de leituras existente quando apropriado; ela deve ficar desabilitada na produção normal. Acrescente contador e amostras próprios do dia do mês, identificando entrada, operandos efetivos, flag/tabela, resultado e retorno/caller. Contagens de mês ou das outras operações não comprovam essa entrada.

Separe: inteiramente original; baseline entregue com seis operações modernas e dia do mês original; candidato com sete modernas. Preserve os significados históricos de `--calendar-baseline` (somente shifts modernos), `--date-ordering-baseline` (shifts/diferença modernos), `--date-before-baseline` (shifts/diferença/later modernos, before original) e `--date-month-baseline` (cinco operações anteriores modernas, mês original). Esses baselines também devem manter dia do mês original. Acrescente um nome inequívoco para a referência entregue com seis modernas. Captura original deve selecionar a nova entrada original; não recapture candidato como oracle.

Reexecute `.local/performance/season-day-01/SeasonFixture`, `.local/performance/season-week-01/SeasonFixture` e `.local/performance/date-ordering-capture-02/SeasonFixture`, preservando os arquivos imutáveis. Exija dois originais estritos, baselines históricos/delivered pertinentes e duas execuções candidatas equivalentes por fixture. Preserve a única diferença permitida na contagem de instruções traduzidas; não masque PC, registradores, temporários, memória, alocador, imports/callbacks ou RNG. Preserve EOF e término pelo estado/transcript original com orçamento finito.

Use stream privado original separado e explicitamente versionado para a nova entrada. Não altere os formatos ou streams históricos CMCAL002 e CMMON001. Registre a identidade dos streams originais existentes antes/depois; ao recapturá-los no modo inteiramente original, confira byte-identidade. Acrescente um negativo isolado que altera o primeiro retorno moderno do dia do mês e exija rejeição na sua primeira fronteira pertinente. Mantenha controles RNG/shifts/diferença/later/before/mês; erro próprio deve ser localizado antes de se diluir em divergência posterior. Valide truncamento, checksum/EOF e ausência de vazamento da perturbação para execução normal.

Reporte chamadas próprias por fixture: zero é ausência de cobertura. Se nenhum dos três exercitar a nova entrada, capture em modo inteiramente original uma sequência real mínima pertinente com as ferramentas existentes, depois compare o mesmo fixture em original/baseline/candidato. O fluxo capturado deve de fato chamar a nova entrada. Não edite snapshots/datas/RNG ou recapture candidato para forçar aceitação. Não exija replay de carreira inteira.

## Aplicativo real e persistência

Use uma única instância ARM64 por vez, com Runtime/Runs privados preparados pelas ferramentas existentes. Reutilize, após conferir hashes, `.local/performance/date-month-live-01/Runtime/Files/games/DateMonth-28Aug2002.sav`, SHA-256 `2e536421c2e2972a072c9538293b5d967b0846f3c1339bd40e6a5b63dc22a2af`, ou `DateMonth-27Aug2002.sav`, `570a254eef09e7fe38e5a535256151f834a9b2f1fa4b1a4ef856353d81e310c5`. Ambos foram reconferidos na preparação; preserve-os e os demais checkpoints históricos. Prepare um novo destino privado, sem sobrescrever o candidato de mês.

Demonstre chamadas semânticas próprias do dia do mês em interação normal de datas, guiada pelos callers/logs. Não presuma que abrir calendário basta. Registre argumentos/resultados auditáveis, confronte amostras válidas com cálculo independente e prove ausência de fallback normal. Observe calendário e informações úteis de jogador/contrato; reporte exatamente o que a UI mostra, separado da equivalência automática.

Valide Save/saída normal/processo ausente/reabertura do candidato exato/Load antes e depois de um avanço normal que atravesse uma fronteira de mês. Partindo de 28 August 2002, acompanhe o jogo e interrompa assim que uma data pertinente de September estiver observada; não force September 1 nem exija passagem visual pela meia-noite. Férias podem avançar rapidamente: acompanhe a UI, interrompa no primeiro intervalo pertinente e registre a data/fase real alcançada. Se houver partida no intervalo, use o fluxo normal do jogo; não edite save/data/memória para evitá-la. Não conclua outra temporada desnecessariamente.

Confira após Load as datas/fase, clube, fixtures/resultados e dados de jogador/contrato observados antes de salvar. Use nomes novos para saves de validação, mantendo hashes e referências originais intactos. Use o Save original por Game Options -> Save Game; a limitação existente de File -> Save nativo (`Native menu selections require original MenuSelect dispatch`) permanece fora do escopo. Recrie bindings CUA e índices AX na nova sessão. Feche normalmente e confirme processo ausente antes de abrir outra instância.

## Testes e critério de conclusão

Execute os testes pertinentes e a suíte completa no candidato final, incluindo novo grupo direto, regressões das seis operações, auditorias e UBSAN do adapter/gerador alterados. Confirme layout final de State em todos os objetos realmente ligados. Se adicionar campos ou mudar Memory/State, recompile completamente; não execute sanitizers/harnesses contra um archive com layout anterior nem altere headers durante a compilação. Só prepare o candidato final após estabilizar a fonte/build. Limitações históricas de ASan não aprovam a nova implementação.

Comandos de referência, a reconferir no estado encontrado:

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

Documente regeneração, novo grupo/stream/baseline e comandos de UBSAN repetíveis. Diferencie testes pulados de grupos explicitamente executados e resultados novos de históricos. Não faça otimizações para reduzir o tempo da tarefa.

Só conclua após contrato demonstrado, operação semântica em produção, equivalência direta/delimitada, controles negativos, cobertura real própria, persistência antes/depois da fronteira de mês e suíte final aprovados. Se houver bloqueio essencial, documente sem declarar entrega concluída. Não trate green tests, um commit ou um contador isolado como prova de aplicativo/persistência.

## Proteção, documentação e entrega

Preserve `~/Library/Application Support/CM 01-02 Native/{Game,Runtime,Runs}`, saves pessoais, resource forks e Finder metadata. A entrega anterior preservou 809/5/43 arquivos; faça inventários atuais antes/preinstall/depois, incluindo conteúdo e metadados, sem assumir que as quantidades históricas são invariantes. Preserve uma nova cópia privada executável da referência instalada atual antes de substituí-la.

Exports, dados originais, saves novos, snapshots, transcripts, logs, capturas e builds continuam privados e ignorados. Não mude ignore/LFS nem publique artefatos privados. Preserve imutável `saves/Mac-Teste-Validado-Pos-Jogo.sav`, SHA-256 `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`, os streams/checkpoints históricos e os prompts preexistentes.

Documente em inglês contrato, arquitetura, integração, limites, comandos, contagens próprias, negativos, observações da UI, hashes separados de fonte/candidato assinado/instalado/backup e evidência de preservação. Atualize `docs/native-port-status.md`, preserve os relatórios históricos e a melhoria bissexta adiada, e acrescente um relatório específico de dia do mês.

Entregue instalação ARM64 validada preservando dados e metadados pessoais. Revise o diff exato, faça commit e push normal sem force conforme a autorização permanente para trabalho concluído/validado e confira o SHA remoto por consulta nova. Essa autorização se aplica à futura implementação validada, não transforma a preparação deste handoff em implantação ou publicação de dados. Não declare reconstrução de todas as conversões de data nem do restante do engine.
