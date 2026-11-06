# Concluir STAFF_CONTRACTS.CPP no port integral em C++23

Reconstrua integralmente `STAFF_CONTRACTS.CPP` em C++23 escrito à mão no projeto `/Users/victordmor/Projects/cm0102mac`. Feche o inventário do módulo, aproveite as implementações parciais comprovadas, substitua seus corpos gerados e transcrições baseadas em instruções, integre todas as entradas na rota normal do aplicativo e entregue a equivalência verificada.

## Objetivo e prioridade

O objetivo geral é um CM 01/02 nativo para Apple Silicon cujo código executado seja inteiramente reconstruído em C++23, preservando o comportamento do jogo. O aplicativo ARM64 já é jogável; isso não significa que a reconstrução esteja completa. Desempenho nativo continua prioritário, e retirar código gerado de um subsistema usado no recrutamento aproxima os dois objetivos.

O incremento escolhido é concluir o módulo de contratos individuais de jogadores e staff: seus registros, inicialização, termos, datas, consultas e demais operações que o inventário efetivamente atribuir a `STAFF_CONTRACTS.CPP`. Por exemplo, a mesma pessoa, contrato e data devem produzir exatamente a mesma classificação de vencimento e os mesmos efeitos nos consumidores do recrutamento, agora pela implementação C++23 normal.

Esta tarefa tem uma conclusão concreta: inventário fechado, implementação completa, todas as rotas próprias em produção e verificação sem divergências abertas. Não a transforme em outra investigação indefinida de pequenas otimizações. Uma aceleração é desejável, mas não está prometida nem substitui a reconstrução. Só anuncie ganho de desempenho se medido; caracterize e resolva regressões relevantes introduzidas pela mudança.

## Ponto de partida verificado em 4 de novembro de 2026

- O checkout principal estava em `main`, em `3d7fecadc448aee4e9fa2b49e17a6d6a89d8863d`, cinco commits atrás da referência local `origin/main`, em `61f4a5516dd7850cdda52407f85e76037c2f879b`. Esta preparação não consultou novamente o servidor remoto. Confira branch, histórico, remoto e alterações locais antes de escolher a base.
- Há numerosas alterações locais e arquivos não rastreados de outras tarefas, inclusive runtime, gerador, CMake, Match PL, Shortlist, Contracts, BRA_REG_GAUCHO e documentação. Em arquivos relevantes, há mudanças em `native/contract_terms_checks.cpp`, `native/contract_review_adapter.cpp`, `native/contract_review_presentation.cpp`, `native/contract_manager_adapter.cpp`, `native/contract_manager_queries.cpp`, `tools/compile_recomp.py` e `native/CMakeLists.txt`. Preserve todas. Use um worktree próprio com branch `codex/` quando necessário; não faça reset, limpeza, stash automático ou incorporação indiscriminada desses diffs.
- A planilha existente `CM 01/02 — Andamento da recompilação`, aba `Módulos C++`, linha 42, registra `STAFF_CONTRACTS.CPP` como `Pendente — trabalho anterior`. As faixas 43–47 candidatas, 38–42 substituídas e 39,5%–44,7% em produção são estimativas provisórias antigas. Não são inventário fechado, percentuais atuais ou comprovação de C++23 direto. Releia a planilha e localize a linha pelo nome antes de escrever.
- Existem famílias parciais de termos, revisão, inicialização, armazenamento, consultas, apresentação e contratos preliminares. Os nomes dos arquivos não provam propriedade de módulo. Por exemplo, `docs/native-contract-storage.md` reúne a inicialização `0x103727c0` e operações de `CONTRACT_MANAGER.CPP`; preserve a propriedade dos módulos vizinhos já reconstruídos.
- `native/contract_terms_expiry.cpp` contém `expiry_band`, entrada `0x10380710`, usando `SemanticGuestCall`. O adaptador inclui essa alternativa. Porém, o artefato local `.local/current/generated/page_10380.cpp` entra diretamente no corpo gerado nessa entrada. A existência da alternativa e de seus testes não demonstra produção C++23 direta. Confirme os hooks e flags no build que usar; o artefato local não identifica sozinho a versão instalada.
- O checkout contém uma otimização local do acesso à pilha dessa rotina no gerador e testes correspondentes. Preserve sua autoria e evidência. Não retome o objetivo geral de benchmarks pausado nem publique mudanças alheias como parte desta tarefa.
- O commit `61f4a55` encerrou a hipótese de tornar nativo o miolo de `Scan::value` e de sua avaliação de contribuição. O registro relata apenas 0,54% combinado de custo próprio no perfil do Continue e não implementa uma otimização. Esses números são evidência histórica do commit, não medições desta preparação. Não repita essa hipótese como objetivo desta tarefa.
- `TRANSFER_MANAGER.CPP` está explicitamente pausado em 87/214 entradas, tanto em `TASKS.md` quanto na planilha. Esta escolha não retoma esse módulo, a verificação pausada de `media.cpp` ou entregas pendentes de outros módulos.
- Esta preparação inspecionou código, testes, documentação, Git e a planilha. Não compilou, executou testes, benchmarks ou o aplicativo, nem conferiu a instalação. Estabeleça o baseline necessário sem apresentar resultados antigos como passes atuais.

## Instruções e fontes a inspecionar

Leia `AGENTS.md`, `CLAUDE.md`, `TASKS.md` e a skill `/Users/victordmor/.codex/skills/cm0102-recompile-module/SKILL.md`, incluindo suas referências de arquitetura, fidelidade e verificação. As regras atuais prevalecem sobre os critérios obsoletos dos relatórios e prompts antigos. Leia também `docs/cpp-rewrites.md`, especialmente os contratos gerais e os de `CONTRACT_MANAGER.CPP`.

Inspecione estas famílias como ponto de partida, delimitando quais entradas realmente pertencem ao módulo:

- `native/contract_terms*`, `native/contract_review*`, `native/contract_initializer*`, `native/contract_storage*`, `native/contract_queries*`, `native/contract_query_adapter*`, `native/contract_draft*` e `native/contract_display*`.
- `tools/contract_terms_audit.py`, `tools/contract_review_audit.py`, `tools/contract_storage_audit.py`, `tools/contract_query_audit.py`, `tools/contract_draft_audit.py`, `tools/contract_display_audit.py`, seus testes e geradores de referências.
- `tools/compile_recomp.py`, `native/CMakeLists.txt`, `native/guest_abi.hpp`, `native/guest_values.hpp`, `native/guest_fpu.hpp`, `native/routine_check.hpp` e `native/world_replay_main.cpp`.
- Os consumidores `native/shortlist_manager_candidates_filter.cpp`, `native/shortlist_manager_assessment_contribution.cpp` e os chamadores relevantes em `native/contract_manager*`; não amplie o escopo para reconstruir esses módulos.
- `docs/native-contract-terms.md`, `docs/native-contract-review.md`, `docs/native-contract-storage.md`, `docs/native-contract-draft.md` e a seção `Module candidates for further reconstruction` de `docs/native-background-performance.md`. Use os relatórios para contratos e pistas, distinguindo observações locais de entregas publicadas.
- `tools/dev.sh`, `tools/replay_sweep.py`, as implementações recentes `native/siprint*` e sua auditoria como exemplos de integração; não adote a arquitetura de builder das famílias antigas como destino final.

O export privado `.local/current/export/instructions.sqlite` e `program.json` estavam presentes. Use as fontes locais autorizadas para fechar a propriedade das funções. Não dependa de scripts temporários de experimentos anteriores e não publique listagens originais.

## Escopo e contratos

1. Feche primeiro o inventário: fonte/string pool, limites das funções, vizinhos, chamadas, TOC, callbacks, tail calls e entradas internas alcançáveis. Reconcilie as famílias parciais e as rotas atuais. Crie uma auditoria versionada do módulo com seus testes; registre evidência detalhada em `.local/rewrites/staff_contracts/`. Não trate a faixa da planilha ou um censo de execução como denominador.
2. Pilote a comparação original/C++ numa operação representativa, preferencialmente `0x10380710` após confirmar sua propriedade. Depois implemente blocos funcionais coerentes até fechar o módulo. Preserve e reutilize implementações já corretas quando atenderem ao contrato atual; não reescreva por estilo nem altere funções de vizinhos apenas porque compartilham um arquivo.
3. Use tipos e vistas de registros nomeados, variáveis C++, controle de fluxo comum e chamadas diretas entre operações reconstruídas. Concentre a tradução da ABI em adaptadores estreitos. Nem transcrição com `SemanticGuestCall`, algoritmo mantido num vetor de registradores, chamada ao corpo gerado próprio, nem fallback de produção para ele contam como conclusão.
4. Preserve datas e seus limites, sinais e larguras de inteiros, bits de estado, valores e cláusulas contratuais, entradas/saídas, aliases, escritas parciais em falhas, ordem dos efeitos, callbacks e transferências não locais. Preserve arredondamentos e estado de ponto flutuante observável, sequência ordenada do RNG e todos os bytes que o jogo pode ler, inclusive resíduos da pilha com leitores posteriores. Não use fast-math, novas regras, aproximações, cache global ou paralelismo que altere a ordem.
5. Audite a persistência que efetivamente pertença ao módulo e mantenha seus registros compatíveis com leitores e escritores existentes. Compare os bytes e contagens afetados; não invente novo formato, saneamento de dados ou recuperação de erro diferente do original.
6. Preserve original mode, observadores, tracing, orçamento finito, erros e rotas de verificação. O original deve continuar disponível como referência independente para comparação. Prove separadamente a seleção do C++ na configuração normal, inclusive chamadas diretas, na mesma página e entradas por callback/continuação quando existirem.
7. Mantenha explícitas as dependências externas ao inventário. Não assuma a conclusão de `CONTRACT_MANAGER.CPP`, das telas ou de `TRANSFER_MANAGER.CPP` como parte deste módulo, nem retome trabalhos pausados. Esta tarefa não inclui novos recursos de gameplay, interface, multiplayer ou mudança global de arquitetura do runtime.

## Aceitação e validação

Separe quatro estados: inventário, implementação, produção e verificação. Só declare o módulo completamente reconstruído quando os quatro estiverem fechados, com contagens auditadas e nenhuma divergência observável sem resolução.

- Compare cada entrada alterada com a referência original a partir do mesmo estado. Cubra ramos relevantes, casos raros, callbacks, erros e limites dos contratos. Inclua mutações válidas, rastreamento de argumentos e controles negativos que demonstrem a capacidade de detectar resultado ou chamada incorretos. Igualdade apenas do retorno, da seed final ou de contagens de chamadas não basta.
- Use cobertura para localizar lacunas e feche-as com casos válidos ou prova específica baseada no original. Diferencie retornos completos, interrupções, falhas e caminhos sem alcance demonstrado. Rode ao final uma configuração com todas as entradas do módulo ativas juntas, preservando a referência real.
- Execute os testes focados do novo inventário e os das famílias afetadas. Os testes existentes incluem `contract_terms_component_equivalence`, `contract_review_component_equivalence` e `contract_draft_component_equivalence`; confirme quais ainda são aplicáveis e construa somente os alvos necessários. Alguns testes antigos exigem prefixos históricos, instruction counts ou cortes obsoletos: atualize apenas os contratos substituídos pela política atual, mantendo a força da comparação observável.
- Use `tools/dev.sh generate` após integrar o gerador e confira as páginas alteradas. Faça builds serializados dos alvos afetados com `tools/dev.sh build` e execute `tools/dev.sh test -R` com seleção explícita. Não compile todos os checkers por rotina: eles ocupam muito espaço. Mudanças em caminho quente compartilhado do runtime também exigem `-R match_engine`, conforme `CLAUDE.md`.
- Use `tools/replay_sweep.py census` e os metadados para encontrar capturas existentes que alcancem as operações. Chamadas internas diretas podem faltar no censo do dispatcher; confirme a cobertura no proprietário C++ quando necessário. `.local/benchmarks/world/daily-transition` estava presente. Existe também o Continue completo de 11 a 18 de janeiro de 2002 em `.local/performance/club-interest-continue-base/Automation/captures/same-save-continue`; confirme a presença e compatibilidade antes de usá-lo.
- Faça ao menos um replay pareado relevante com estado observável final idêntico, todas as rotas próprias habilitadas e RNG ordenado preservado. Captura ausente, inválida ou sem alcance não é um passe. Se não houver entrada utilizável, construa um replay controlado original/C++ da integração necessária e registre o limite. Não gere novo capture de gameplay para preencher lacunas.
- O commit `61f4a55` registra uma falha preexistente no replay de táticas, na entrada gravada `GetDateTime`/r20. Reconfirme se esse controle for relevante; não altere seu estado esperado para obter aprovação nem atribua a falha automaticamente à reconstrução.
- Para qualquer alegação de desempenho, separe profiling/verificação pesada de timing e meça pelo menos cinco pares alternados na mesma entrada e configuração. Se medir o Continue completo, use baseline e candidato comparáveis, em série, com dispersão e vitórias registradas. Resultados de microbenchmarks ou operações parciais não provam o tempo de uma semana real. Ausência de ganho não apaga uma reconstrução correta, mas não autoriza apresentar aceleração.

Victor é responsável por carreira, QA visual/gameplay, save/reload manual, novas capturas e temporadas completas. Não execute esses fluxos nem os transforme em condição para concluir esta entrega. As provas da tarefa vêm dos originais, fixtures controladas e capturas já existentes.

## Entrega

Mantenha a lista de implementação em `TASKS.md`. Siga a política de `AGENTS.md`: Sol coordena e implementa; escale somente a menor ambiguidade concreta ou divergência persistente que satisfaça os critérios, usando `recomp_astra` quando disponível. Não delegue a conversão rotineira nem trate tamanho do módulo como motivo para escalar. Não crie novas automações, chats ou um goal com orçamento inventado.

Revise o diff próprio antes de commit; se a skill de revisão não estiver disponível, informe e faça a revisão explícita e focada prevista na skill do projeto. Deixe cobertura e diagnóstico desligados por padrão, construa o app ARM64 normal e execute os checks necessários à configuração final. Documente contratos novos ou não óbvios no local apropriado, em inglês, sem relatórios de entrega redundantes.

Concluído o módulo, atualize na mesma tarefa a planilha existente:

`https://docs.google.com/spreadsheets/d/1nAl1ojgAi6lJlIAllCFNSPQ-I-HfGgG155MeNJ85Fd4/edit`

Localize `STAFF_CONTRACTS.CPP` na aba `Módulos C++`; registre denominador fechado, rotinas verificadas, produção, validação e limites. Confira por leitura a linha e os totais afetados. Não conte duas vezes rotinas já pertencentes a módulos concluídos. Não edite o tracker antes de obter a evidência correspondente nem substitua indiscriminadamente suas fórmulas.

Faça commits apenas das alterações próprias verificadas e push normal para `main`, conforme `CLAUDE.md`, integrando mudanças concorrentes sem reescrever histórico. Registre no commit o que mudou, por quê, comandos, resultados e limites. Não publique export, conteúdo do jogo, saves, binários, logs ou capturas.

Instale com `tools/dev.sh install` quando o jogo estiver fechado, preservando dados e saves. Se o instalador recusar por jogo aberto, não encerre o processo do usuário: registre instalação pendente e continue a entrega independente. Confirme a identidade do app antes de dizer que foi instalado. Limpe somente evidência volumosa produzida nesta tarefa usando o procedimento do projeto, preservando o registro compacto e as entradas compartilhadas.

Comunique em português; código, comentários, documentação técnica e commits ficam em inglês. Ao concluir, informe as quatro dimensões do fechamento, contagens verificadas, testes realmente executados, dependências externas restantes, revisão publicada, situação da instalação e atualização do tracker. Distinga claramente reconstrução completa, entrega pendente e qualquer limite de medição.
