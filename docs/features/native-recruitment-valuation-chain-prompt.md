# Avaliação nativa de candidatos no Continue

Implemente um núcleo C++23 nativo para a cadeia de avaliação de candidatos do recrutamento em `/Users/victordmor/Projects/cm0102mac`, começando em `Scan::value` (`0x1035e9f0`) e sua chamada a `assess_candidate_contribution` (`0x10361490`). Reduza as leituras e publicações intermediárias de registradores e temporários PowerPC, mantendo exatamente os resultados do jogo. Entregue uma redução mensurável no replay do Continue completo, ou encerre esta hipótese com evidência de que não oferece ganho confiável. Não transforme a tarefa numa investigação indefinida de outros gargalos.

## Objetivo e motivo

A prioridade do projeto é tornar o Continue mais rápido e avançar em direção ao port integral em C++23. O jogo já executa em ARM64; a oportunidade é manter os valores de trabalho em variáveis C++ e sincronizar o estado legado nas fronteiras que realmente o exigem. O resultado para o jogador deve ser o mesmo recrutamento — candidatos, pontuações, preços, propostas e escolhas — com menos espera.

Este passo sucede as otimizações de tendências e consultas de posição do Match Engine. Uma amostragem nativa anterior, sobre o baseline `5138d8f`, atribuiu cerca de 19,9% das amostras de processamento a `scan_candidates` e 16,2% a `Scan::value`, inclusivamente. São parcelas aninhadas, anteriores à última entrega; não devem ser somadas nem usadas como previsão de aceleração. Confirme o peso atual antes de implementar.

O código inspecionado mantém `Scan::Local`, `FloatLocal`, `Contribution::KeptF` e vários valores retidos ligados diretamente aos registradores emulados. Esta é uma hipótese concreta para o próximo incremento. Chamadas diretas internas já existem em `Program::call`; apenas trocar uma chamada pelo mesmo caminho direto não demonstra remoção desse custo.

## Ponto de partida verificado em 4 de novembro de 2026

- O checkout principal estava em `main`, commit `3d7fecadc448aee4e9fa2b49e17a6d6a89d8863d`, com numerosas alterações locais de outras tarefas. Entre elas: `TASKS.md`, documentação de desempenho, runtime, gerador, Match PL, Contracts, Shortlist e arquivos novos de BRA_REG_GAUCHO. Preserve tudo, inclusive arquivos não rastreados.
- A referência local `origin/main` estava em `79375502725a4ff132e4b142328099cfb236da7a` (`Keep match role table lookups in native values`), posterior a `5138d8f` e à especialização das consultas espaciais. Consulte novamente o remoto antes de escolher a base; uma referência local não prova o estado atual do servidor.
- Existem worktrees de outras tarefas sob `.local/performance/` e `.local/worktrees/`. Não os reutilize para escrever. Crie um worktree próprio a partir da revisão publicada atual, com branch `codex/` se precisar de uma branch. Não atualize o checkout principal por reset, stash automático ou limpeza.
- O commit `7937550` registra oito CTests do Match Engine, replay de 600 avanços e comparação do Continue completo com decisões de posição e RNG ordenados preservados. Registra ganho mediano pareado de 1,7261% no tempo desse replay contra `5138d8f`. São resultados da entrega anterior, não testes executados nesta preparação nem ganho atribuível ao novo trabalho.
- A preparação deste prompt fez leitura de fontes, testes, documentação, histórico e arquivos disponíveis. Não compilou, não executou o jogo, não repetiu benchmarks nem verificou a instalação atual. Reconfirme a identidade dos executáveis e a composição de seus objetos: as entregas recentes usaram builds isolados e o checkout principal contém alterações que não estão integralmente em `origin/main`.

## O que ler

Leia primeiro `AGENTS.md`, `CLAUDE.md` e `TASKS.md`. Use as instruções atuais, e não as exigências antigas dos outros prompts em `docs/features/`.

Inspecione, na base escolhida:

- `native/shortlist_manager_candidates.cpp`, `native/shortlist_manager_candidates_value.cpp` e `native/shortlist_manager_candidates_scan.hpp`: entrada, estado da varredura, avaliação, negociação e escolha.
- `native/shortlist_manager_assessment_contribution.cpp`, `native/shortlist_manager_assessment.hpp` e `native/shortlist_manager_routines_assessment.inc`: contrato de `0x10361490`, dependências e encaminhamento.
- `native/shortlist_manager_detail.hpp`, `native/shortlist_manager_records.hpp`, `native/shortlist_manager_random.hpp` e `native/shortlist_manager_adapter.cpp`: memória, cálculos, RNG, chamadas e seleção original/C++.
- `native/guest_abi.hpp`, `native/guest_values.hpp`, `native/guest_fpu.hpp`, `native/routine_check.hpp`, `native/season_reference.hpp`, `native/world_replay_main.cpp`, `native/CMakeLists.txt`, `tools/replay_sweep.py` e `tools/dev.sh`: contratos e instrumentos de verificação.
- `native/match_eng_tendency_kernel.hpp` e `native/match_pl_context.cpp` na revisão publicada: precedentes recentes para valores nativos e publicação compatível. Não copie a qualificação dos observadores sem confirmar que se aplica às chamadas do recrutamento.
- `docs/native-background-performance.md`, especialmente os experimentos de contribuição, avaliação e buscas de contratos. Há micro-otimizações rejeitadas; não repita as mesmas variantes sem uma hipótese nova sustentada pelo perfil.

## Escopo e contratos

1. Delimite o interior nativo da avaliação dentro de uma invocação. Priorize a cadeia `Scan::value` → `assess_candidate_contribution`; preserve o contrato de entrada, retorno `r3`, parâmetro de saída e efeitos laterais de `0x10361490`. Mantenha as demais chamadas públicas dessa rotina compatíveis, inclusive as de registro de interesse.
2. Use valores e estruturas C++ nomeados para o trabalho interno. Publique e recarregue os valores necessários antes/depois de chamadas legadas, imports, callbacks e qualquer fronteira que possa observar ou modificar o estado. Deixe o desenho concreto para a implementação, depois de auditar as leituras e escritas. Não introduza um cache global nem retenha dados mutáveis entre candidatos ou clubes.
3. Preserve a ordem de candidatos, empates, recusas, segunda passagem, seleção do avaliador, limites, sinais e larguras dos inteiros, arredondamentos, FPSCR, sequência do RNG e publicações de propostas/links. Preserve todos os bytes que o jogo possa ler, incluindo resíduos de pilha com leitores posteriores. Não remova escritas apenas por parecerem temporárias.
4. Preserve original mode, checker por entrada, hooks, observadores, tracing, orçamento finito, exceções e transferências não locais. Se não houver prova para uma condição, use o caminho compatível antes de produzir efeitos parciais. Um observador passivo de replay e o observador de desenho do app precisam de qualificação explícita; não suponha que qualquer observador é passivo.
5. Mantenha as dependências geradas, como a estimativa `0x10216830` e o avaliador de staff `0x101ad080`, como fronteiras quando possível. Esta tarefa não exige reconstruir módulos inteiros de avaliações, contratos ou transferências. Se o perfil mostrar que todo o custo relevante está nessas dependências e o interior escolhido não oferece uma oportunidade útil, documente essa conclusão e encerre a hipótese.
6. Não altere regras de recrutamento, formato de save, telas, resultados da simulação ou RNG. Não use fast-math, aproximações, amostragem de candidatos reduzida ou paralelismo que mude a ordem. Não altere páginas geradas manualmente. Não retome tarefas pausadas de reconstrução ou o antigo objetivo geral de benchmarks.

## Entrada e validação

Foi confirmada a presença do capture completo do Continue de 11 a 18 de janeiro de 2002 em:

`/Users/victordmor/Projects/cm0102mac/.local/performance/club-interest-continue-base/Automation/captures/same-save-continue`

Ele contém `initial.snapshot`, `expected.snapshot`, `imports.gz` e `capture.txt`. Trate-o como entrada privada e imutável. Os captures estabelecidos `.local/benchmarks/world/daily-transition` e `.local/benchmarks/ui/tactics-continue` também estavam presentes. Descubra outros controles existentes pelos metadados; confirme quais chegam às entradas alteradas. O censo do dispatcher pode omitir chamadas internas diretas, portanto verifique cobertura no proprietário C++ quando necessário.

Não dependa de scripts temporários das tarefas anteriores para construir ou verificar o candidato. Use fontes versionadas e registre comandos reproduzíveis. Se um capture necessário tiver sido removido ou for incompatível, reporte exatamente a limitação; não altere seu estado esperado para obter aprovação e não crie uma nova captura de gameplay sem pedido do Victor.

Execute nesta ordem:

1. Congele um baseline reproduzível da revisão publicada atual, já contendo `7937550` quando aplicável. Separe as configurações de Release, diagnóstico e profiling. Estabeleça um replay estrito que confere o estado esperado e RNG e faça um perfil focado do Continue. Diferencie custo próprio da avaliação, dependências e leitura do arquivo do replay; acelerar a descompressão do harness não atende ao objetivo.
2. Implemente apenas uma hipótese sustentada por esse perfil. Compare original e C++ em casos representativos e raros das branches alteradas: candidato jogador/staff quando alcançável, empréstimo/segunda passagem, ausência de contrato ou avaliador, limites de orçamento e pontuação, aliasing de saídas/pilha e falhas pertinentes. Não exija matriz exaustiva do módulo inteiro. Inclua um controle negativo no comportamento alterado.
3. Exercite o caminho otimizado separado do fallback de diagnóstico. Um checker que força o caminho antigo não verifica o novo núcleo. Demonstre também que o app normal, com observador de desenho, pode usar a otimização com os mesmos efeitos. Preserve as escolhas ordenadas, retorno/saída, escritas relevantes e RNG; seed final ou contagem de chamadas isoladas não bastam.
4. Compile os alvos afetados com `tools/dev.sh build`; consulte os testes disponíveis e execute os relevantes no worktree/build próprio. Por exemplo, `tools/dev.sh test -R '^(shortlist_manager_volatile_audit|shortlist_manager_daily_transition)$'` verifica a auditoria e a transição diária. Também existem `shortlist_manager_tactics_continue` e `shortlist_manager_detect_difference`; selecione os que fornecem evidência relevante e confirme seus inputs. Adicione uma comparação focada se esses testes não exercitarem a nova rota. Se mudar um caminho quente compartilhado do runtime ou o Match Engine, inclua os checks `-R match_engine` exigidos por `CLAUDE.md`.
5. Faça o replay estrito do Continue completo no baseline e candidato, conferindo estado final contra a entrada imutável e comparando a sequência ordenada do RNG e das decisões afetadas. Para conferir rotinas, `cm-world-replay` oferece `--check-routines`, `--only` e `--compare-f1`; confira os argumentos em `native/world_replay_main.cpp` e mantenha a seleção restrita às entradas afetadas.
6. Só depois meça pelo menos sete pares alternados baseline/candidato, um processo por vez, na mesma entrada completa e com opções iguais. `cm-world-replay <capture> 1 --timing` confere o estado final, mas dispensa a comparação de cada escrita RNG: use-o apenas para timing após a prova estrita. Separe a instrumentação pesada das rodadas cronometradas. Registre tempo decorrido, CPU, mediana das diferenças pareadas, dispersão e vitórias. Registre carga concorrente relevante sem fechar o jogo ou processos do usuário.
7. Aceite a otimização apenas com ganho repetível no Continue completo e compatibilidade comprovada. Um ganho apenas num microbenchmark, num trecho diário ou dentro da variação não basta. Se a hipótese não se confirmar, remova somente o protótipo desta tarefa, preserve a evidência compacta e informe o resultado sem prometer aceleração.

## Entrega e limites de autorização

Mantenha o trabalho da implementação em `TASKS.md` e faça a revisão do diff exigida pelo projeto. Siga `AGENTS.md` para escalonar somente uma ambiguidade concreta de comportamento ou uma divergência persistente após duas correções fundamentadas; não delegue a conversão rotineira nem afirme revisão Astra que não ocorreu.

Depois dos checks necessários, faça commit e push normal das alterações próprias para `main`, conforme `CLAUDE.md`, integrando mudanças concorrentes sem reescrever histórico. Registre no commit a hipótese, ganho, comparação, comandos e limites. Não publique fontes originais, captures, saves, binários ou logs. Esta é otimização de um módulo já reconstruído; não aumente contagens de rotinas concluídas no tracker por ter otimizado uma cadeia existente.

Instale pelo `tools/dev.sh install` quando o jogo estiver fechado, preservando dados e saves; confira arquitetura e identidade do executável instalado. Se estiver aberto, não o encerre: informe instalação pendente. Não execute QA visual, nova carreira, save/reload manual nem temporada completa; essas verificações pertencem ao Victor. Este prompt não autoriza nova automação ou chat, nem exige um goal com orçamento inventado.

Comunique em português; código, comentários, documentação técnica e commits ficam em inglês. Na resposta final, informe a cadeia realmente alterada, a dependência do estado PowerPC removida, as fronteiras restantes, os checks executados, o resultado no Continue completo, a revisão publicada e o estado real da instalação. Se não houver ganho confiável, diga isso claramente.
