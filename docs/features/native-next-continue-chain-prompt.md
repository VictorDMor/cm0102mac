# Próxima cadeia nativa do Continue

Trabalhe em `/Users/victordmor/Projects/cm0102mac`. Crie um goal para selecionar pelo perfil a próxima cadeia que mais custa no Continue, reduzir sua dependência do estado PowerPC e entregar uma melhoria mensurável no Continue completo do mesmo save, preservando as decisões, os efeitos observáveis e a ordem do RNG. Execute o trabalho até a entrega ou até demonstrar, com evidência, que a hipótese não oferece ganho. Não atribua um orçamento de tokens que eu não especifiquei.

## Direção e limite

O jogo já executa código ARM64. Queremos remover o custo de representar continuamente a execução como registradores e pilha PowerPC: parâmetros e variáveis C++ reais, chamadas diretas e sincronização apenas nas fronteiras necessárias. Preserve a compatibilidade com rotinas ainda não convertidas. Escolha uma cadeia coerente e medida; não tente eliminar todo o estado legado de uma vez, não selecione módulos arbitrariamente e não inicie uma campanha geral de reconstrução. Não confunda a retirada de dependência do estado com a mera troca de nomes ou com desativar verificações.

## Ponto de partida — confira novamente

- Leia `AGENTS.md`, `CLAUDE.md`, `TASKS.md` e `docs/native-background-performance.md`. Siga a política de escalonamento do projeto quando houver ambiguidade real ou divergência persistente. Esta tarefa é otimização, não reconstrução integral de um módulo; a skill de reconstrução exclui performance tuning.
- Em 2026-11-04, `origin/main` estava em `5138d8f65c877b51ff98e2a6cd530a556ae3c72e`: `Keep side-tendency arithmetic native between legacy calls`. O app `/Users/victordmor/Applications/reCM 01-02.app` foi instalado e verificado como ARM64 a partir dessa entrega.
- O checkout principal estava em `3d7feca` com numerosas alterações concorrentes, inclusive runtime, Match PL, transferências e outros módulos. NÃO resete, limpe, descarte nem publique essas alterações. Confira status/branches e use um novo worktree isolado da revisão publicada atual. O trabalho anterior está em `.local/performance/2026-11-04-native-chain-release/source`.
- Identifique o executável instalado, sua arquitetura, hash e eventual processo em execução. Congele o baseline atual, já contendo a otimização de tendências; não compare o próximo candidato com o baseline antigo para atribuir ganho novo.

## O que já foi entregue

A cadeia de `update_side_tendencies` (`0x101df110`) passou a manter oito valores e o estado de ponto flutuante em valores nativos entre chamadas legadas, incluindo o consumo de condição para ação -1 e o interior do RNG de limites pequenos. Veja `native/match_eng_shots_tendencies.cpp`, `native/match_eng_tendency_kernel.hpp`, `native/match_eng_tendency_checks.cpp` e o commit citado.

A fórmula é compartilhada com um caminho de diagnóstico que publica registradores imediatamente. Somente ausência de observador, `NativeDrawingObserver` e a identidade exata do observador passivo do replay permitem publicação adiada; observadores arbitrários, tracing, probes e orçamento finito exigem o caminho compatível. Não amplie essa qualificação sem auditar os callbacks e as fronteiras.

A entrega anterior passou nos sete testes focados do Match Engine e preservou o snapshot final e o transcript ordenado do RNG no Continue completo. A cobertura separada confirmou 73.840 entradas no caminho nativo. Em sete pares alternados, a redução mediana pareada foi 3,4754% de tempo e 3,0603% de CPU, com sete vitórias em ambos. Esses são resultados anteriores, não o ganho da próxima tarefa.

## Entrada de medição e evidência

Use o mesmo Continue completo capturado do save de referência, de 11 a 18 de janeiro de 2002:

`.local/performance/club-interest-continue-base/Automation/captures/same-save-continue`

Ele contém `initial.snapshot`, `expected.snapshot`, `imports.gz` e `capture.txt`. Verifique a existência e a integridade antes de rodar. Não sobrescreva essa entrada nem o save original. Não substitua por um dia parcial ou por um trecho que não exercite a cadeia escolhida.

O replay da entrega instalada está em `.local/performance/2026-11-04-native-chain-release/cm-world-replay`. Os scripts `build.py` e `pair-final.py` nessa pasta explicam a montagem do candidato anterior e as medições; o script de pares ainda aponta para um baseline anterior à entrega, portanto NÃO o reutilize sem atualizar ambos os executáveis e a pasta de saída. Os arquivos locais são auxiliares: a fonte versionada e a identidade dos binários devem sustentar a reprodução. Se faltar o capture indispensável, informe o bloqueio; não fabrique equivalência ou números substitutos.

## Execução

1. Estabeleça o baseline atual e faça um perfil do Continue completo. Use amostragem nativa e instrumentos focados para distinguir tempo próprio, tempo inclusivo, custo de memória, despacho e sincronização. O censo do dispatcher pode omitir chamadas diretas ou na mesma página. Confirme cobertura real.
2. Escolha a cadeia restante com maior custo relevante e uma hipótese concreta de redução. A simulação de partidas dominava o perfil anterior do Continue, mas isso precisa ser confirmado após a última otimização. Não declare um vencedor antes de medir.
3. Audite leituras, escritas, callbacks, entradas/saídas, registradores observáveis, exceções e RNG. Mova o interior para valores e chamadas C++ nativas, mantendo estado exato nas fronteiras em que ele é necessário. Preserve os caminhos de diagnóstico e seleção original/C++.
4. Compile um candidato ARM64 comparável ao baseline. Evite misturar alterações concorrentes, instrumentação de perfil ou configurações diferentes. Não edite páginas geradas à mão. Não use fast-math, paralelismo que altere ordem, aproximações ou supressão de trabalho observável.
5. Prove equivalência contra a referência original nos casos focados e contra o baseline atual no Continue completo: decisões e escritas relevantes, estado final e sequência ordenada do RNG. Igualdade apenas da seed final, contagem de chamadas ou um teste sem cobertura não basta. Inclua erros/casos raros pertinentes e um controle que demonstre que a verificação detecta divergência.
6. Rode os checks exigidos por `CLAUDE.md`; se tocar Match Engine ou caminho quente do runtime, inclua `-R match_engine`. Confirme separadamente que a rota otimizada será usada pelo app normal, e não somente pelo harness sem observadores.
7. Meça pelo menos sete pares alternados, um processo por vez, com a mesma entrada completa e as mesmas opções. Separe profiling/verificação pesada das rodadas cronometradas. Registre tempo decorrido, CPU, redução mediana pareada, dispersão e número de vitórias. Não some percentuais de entregas anteriores. Se o resultado estiver dentro do ruído, descarte a hipótese ou investigue outra dentro deste escopo; não anuncie aceleração.

## Entrega

Mantenha a lista de trabalho e revise o diff exato. Só integre a otimização com ganho repetível e equivalência comprovada. Registre no commit o motivo, a cadeia, os números, a validação e os limites; mantenha conteúdo do jogo, snapshots, logs, saves e binários privados em `.local/`.

Após os checks, publique apenas as alterações próprias em `main`, sem force-push, e confirme a revisão remota. Instale pelo `tools/dev.sh install` quando o jogo estiver fechado, validando arquitetura e identidade do candidato. Não encerre o jogo do usuário; se estiver aberto, deixe a instalação pendente e diga isso. Preserve os saves. Gameplay e temporadas completas continuam sob QA do Victor; não inicie uma campanha adicional.

Comunique em português e escreva código, comentários e commits em inglês. Na resposta final, use os títulos exigidos pelo projeto e informe: cadeia escolhida e seu peso medido, dependência removida e fronteiras restantes, ganho no mesmo Continue completo, evidência de decisões/RNG preservados, revisão publicada e situação real da instalação. Seja explícito se não houver ganho confiável.
